//=============================================================================
//  MusE
//  Linux Music Editor
//  $Id:$
//
//  Copyright (C) 2002-2006 by Werner Schweer and others
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License version 2.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//=============================================================================

#include "muse.h"
#include "midifile.h"
#include "midi.h"
#include "midictrl.h"
#include "globals.h"
#include "widgets/filedialog.h"
#include "song.h"
#include "midievent.h"
#include "event.h"
#include "midiedit/drummap.h"
#include "gconfig.h"
#include "al/sig.h"
#include "al/tempo.h"
#include "al/marker.h"
#include "part.h"
#include "exportmidi.h"

//---------------------------------------------------------
//   addController
//---------------------------------------------------------

static void addController(MidiEventList* l, int tick, int /*port*/, int channel, int a, int b)
      {
      if (a < 0x1000) {          // 7 Bit Controller
            l->insert(MidiEvent(tick, channel, ME_CONTROLLER, a, b));
            }
      else if (a < 0x20000) {     // 14 Bit Controller
            int ctrlH = (a >> 8) & 0x7f;
            int ctrlL = a & 0x7f;
            int dataH = (b >> 7) & 0x7f;
            int dataL = b & 0x7f;
            l->insert(MidiEvent(tick, channel, ME_CONTROLLER, ctrlH, dataH));
            l->insert(MidiEvent(tick+1, channel, ME_CONTROLLER, ctrlL, dataL));
            }
      else if (a < 0x30000) {     // RPN 7-Bit Controller
            int ctrlH = (a >> 8) & 0x7f;
            int ctrlL = a & 0x7f;
            l->insert(MidiEvent(tick, channel, ME_CONTROLLER, CTRL_HRPN, ctrlH));
            l->insert(MidiEvent(tick+1, channel, ME_CONTROLLER, CTRL_LRPN, ctrlL));
            l->insert(MidiEvent(tick+2, channel, ME_CONTROLLER, CTRL_HDATA, b));
            }
      else if (a < 0x40000) {     // NRPN 7-Bit Controller
            int ctrlH = (a >> 8) & 0x7f;
            int ctrlL = a & 0x7f;
            l->insert(MidiEvent(tick, channel, ME_CONTROLLER, CTRL_HNRPN, ctrlH));
            l->insert(MidiEvent(tick+1, channel, ME_CONTROLLER, CTRL_LNRPN, ctrlL));
            l->insert(MidiEvent(tick+2, channel, ME_CONTROLLER, CTRL_HDATA, b));
            }
      else if (a == CTRL_PITCH) {
            int a = b + 8192;
            int b = a >> 7;
            l->insert(MidiEvent(tick, channel, ME_PITCHBEND, a & 0x7f, b & 0x7f));
            }
      else if (a == CTRL_PROGRAM) {
            int hb = (b >> 16) & 0xff;
            int lb = (b >> 8) & 0xff;
            int pr = b & 0x7f;
            int tickoffset = 0;
            if (hb != 0xff) {
                  l->insert(MidiEvent(tick, channel, ME_CONTROLLER, CTRL_HBANK, hb));
                  ++tickoffset;
                  }
            if (lb != 0xff) {
                  l->insert(MidiEvent(tick+tickoffset, channel, ME_CONTROLLER, CTRL_LBANK, lb));
                  ++tickoffset;
                  }
            l->insert(MidiEvent(tick+tickoffset, channel, ME_PROGRAM, pr, 0));
            }
      else if (a < 0x60000) {     // RPN14 Controller
            int ctrlH = (a >> 8) & 0x7f;
            int ctrlL = a & 0x7f;
            int dataH = (b >> 7) & 0x7f;
            int dataL = b & 0x7f;
            l->insert(MidiEvent(tick,   channel, ME_CONTROLLER, CTRL_HRPN, ctrlH));
            l->insert(MidiEvent(tick+1, channel, ME_CONTROLLER, CTRL_LRPN, ctrlL));
            l->insert(MidiEvent(tick+2, channel, ME_CONTROLLER, CTRL_HDATA, dataH));
            l->insert(MidiEvent(tick+3, channel, ME_CONTROLLER, CTRL_LDATA, dataL));
            }
      else if (a < 0x70000) {     // NRPN14 Controller
            int ctrlH = (a >> 8) & 0x7f;
            int ctrlL = a & 0x7f;
            int dataH = (b >> 7) & 0x7f;
            int dataL = b & 0x7f;
            l->insert(MidiEvent(tick,   channel, ME_CONTROLLER, CTRL_HNRPN, ctrlH));
            l->insert(MidiEvent(tick+1, channel, ME_CONTROLLER, CTRL_LNRPN, ctrlL));
            l->insert(MidiEvent(tick+2, channel, ME_CONTROLLER, CTRL_HDATA, dataH));
            l->insert(MidiEvent(tick+3, channel, ME_CONTROLLER, CTRL_LDATA, dataL));
            }
      }

//---------------------------------------------------------
//   exportMidi
//---------------------------------------------------------

void MusE::exportMidi()
      {
      if (exportMidiDialog == 0)
            exportMidiDialog = new ExportMidiDialog(song->projectPath());
      int rv = exportMidiDialog->exec();
      if (rv == 0)
            return;
	QString name = exportMidiDialog->filename->text();
	QFile fp(name);
      if (fp.exists()) {
            QString s(QWidget::tr("File\n") + name + QWidget::tr("\nexists"));
            int rv = QMessageBox::warning(this,
               QWidget::tr("MusE: export midi file:"),
               s,
               QWidget::tr("Overwrite"),
               QWidget::tr("Quit"), QString::null, 0, 1);
            if (rv == 1)
                  return;
            }

      MidiFile mf;
      int format = exportMidiDialog->smf0->isChecked() ? 0 : 1;
      mf.setFormat(format);

      MidiFileTrackList* mtl = new MidiFileTrackList;

      MidiTrackList* mt = song->midis();
      for (iMidiTrack im = mt->begin(); im != mt->end(); ++im) {
            MidiTrack* track = *im;

            MidiFileTrack* mft = new MidiFileTrack;
            mtl->push_back(mft);

            MidiEventList* l = &(mft->events);
            int port       = 0; // track->channel()->port();
            int channel    = 0;
            channel        = track->channelNo();

            //-----------------------------------
            //   managed controller
            //-----------------------------------

      	CtrlList* cll = track->controller();
            for (iCtrl ivl = cll->begin(); ivl != cll->end(); ++ivl) {
            	Ctrl* c  = ivl->second;
                  int id   = c->id();
                  for (iCtrlVal iv = c->begin(); iv != c->end(); ++iv) {
                  	int tick = iv.key();
                        int val  = iv.value().i;
                        addController(l, tick, port, channel, id, val);
                        }
                  }

            //-----------------------------------
            //   track name
            //-----------------------------------

            if (!track->name().isEmpty()) {
                  const char* name = track->name().toAscii().data();
                  int len = strlen(name);
                  MidiEvent ev(0, ME_META, (unsigned char*)name, len+1);
                  ev.setA(0x3);    // Meta Sequence/Track Name
                  l->insert(ev);
                  }

            //-----------------------------------
            //   track comment
            //-----------------------------------

            if (!track->comment().isEmpty()) {
                  const char* comment = track->comment().toAscii().data();
                  int len = strlen(comment);
                  MidiEvent ev(0, ME_META, (unsigned char*)comment, len+1);
                  ev.setA(0xf);    // Meta Text
                  l->insert(ev);
                  }
            PartList* parts = track->parts();
            for (iPart p = parts->begin(); p != parts->end(); ++p) {
                  Part* part    = p->second;
                  EventList* evlist = part->events();
                  for (iEvent i = evlist->begin(); i != evlist->end(); ++i) {
                        Event ev = i->second;
                        int tick = ev.tick() + part->tick();

                        switch (ev.type()) {
                              case Note:
                                    {
                                    if (ev.velo() == 0) {
                                          printf("Warning: midi note has velocity 0, (ignored)\n");
                                          continue;
                                          }
                                    int pitch = ev.pitch();
                                    int velo  = ev.velo();
                                    int len   = ev.lenTick();

                                    //---------------------------------------
                                    //   apply trackinfo values
                                    //---------------------------------------

                                    if (track->transposition()
                                       || track->velocity()
                                       || track->compression() != 100
                                       || track->len() != 100) {
                                          pitch += track->transposition();
                                          if (pitch > 127)
                                                pitch = 127;
                                          if (pitch < 0)
                                                pitch = 0;

                                          velo += track->velocity();
                                          velo = (velo * track->compression()) / 100;
                                          if (velo > 127)
                                                velo = 127;
                                          if (velo < 1)           // no off event
                                                velo = 1;
                                          len = (len *  track->len()) / 100;
                                          }
                                    if (len <= 0)
                                          len = 1;
                                    l->insert(MidiEvent(tick, channel, ME_NOTEON, pitch, velo));
                                    l->insert(MidiEvent(tick+len, channel, ME_NOTEON, pitch, 0));
                                    }
                                    break;

                              case Controller:
                                    addController(l, tick, port, channel, ev.dataA(), ev.dataB());
                                    break;

                              case Sysex:
                                    l->insert(MidiEvent(tick, ME_SYSEX, ev.eventData()));
                                    break;

                              case PAfter:
                                    l->insert(MidiEvent(tick, channel, ME_AFTERTOUCH, ev.dataA(), ev.dataB()));
                                    break;

                              case CAfter:
                                    l->insert(MidiEvent(tick, channel, ME_POLYAFTER, ev.dataA(), ev.dataB()));
                                    break;

                              case Meta:
                                    {
                                    MidiEvent mpev(tick, ME_META, ev.eventData());
                                    mpev.setA(ev.dataA());
                                    l->insert(mpev);
                                    }
                                    break;
                              case Wave:
                                    break;
                              }
                        }
                  }
            }

	MidiFileTrack* mft = mtl->front();
	MidiEventList* l     = &(mft->events);

      //---------------------------------------------------
      //    Write Track Marker
      //
      AL::MarkerList* ml = song->marker();
      for (AL::ciMarker m = ml->begin(); m != ml->end(); ++m) {
            const char* name = m->second.name().toAscii().data();
            int len = strlen(name);
            MidiEvent ev(m->first, ME_META, (unsigned char*)name, len);
            ev.setA(0x6);
            l->insert(ev);
            }

      //---------------------------------------------------
      //    Write Copyright
      //
      const char* copyright = config.copyright.toAscii().data();
      if (copyright && *copyright) {
            int len = strlen(copyright);
            MidiEvent ev(0, ME_META, (unsigned char*)copyright, len);
            ev.setA(0x2);
            l->insert(ev);
            }

      //---------------------------------------------------
      //    Write Tempomap
      //
      AL::TempoList* tl = &AL::tempomap;
      for (AL::ciTEvent e = tl->begin(); e != tl->end(); ++e) {
            AL::TEvent* event = e->second;
            unsigned char data[3];
            int tempo = event->tempo;
            data[2] = tempo & 0xff;
            data[1] = (tempo >> 8) & 0xff;
            data[0] = (tempo >> 16) & 0xff;
            MidiEvent ev(event->tick, ME_META, data, 3);
            ev.setA(0x51);
            l->insert(ev);
            }

      //---------------------------------------------------
      //    Write Signatures
      //
      const AL::SigList* sl = &AL::sigmap;
      for (AL::ciSigEvent e = sl->begin(); e != sl->end(); ++e) {
            AL::SigEvent* event = e->second;
            unsigned char data[2];
            data[0] = event->sig.z;
            switch(event->sig.n) {
                  case 1:  data[1] = 0; break;
                  case 2:  data[1] = 1; break;
                  case 4:  data[1] = 2; break;
                  case 8:  data[1] = 3; break;
                  case 16: data[1] = 4; break;
                  case 32: data[1] = 5; break;
                  case 64: data[1] = 6; break;
                  default:
                        fprintf(stderr, "falsche Signatur; nenner %d\n", event->sig.n);
                        break;
                  }
            MidiEvent ev(event->tick, ME_META, data, 2);
            ev.setA(0x58);
            l->insert(ev);
            }

      mf.setDivision(config.midiDivision);
      mf.setTrackList(mtl);
      fp.open(QIODevice::WriteOnly);
      mf.write(&fp);
      fp.close();
      }

//---------------------------------------------------------
//   ExportMidiDialog
//---------------------------------------------------------

ExportMidiDialog::ExportMidiDialog(const QString& name, QWidget* parent)
   : QDialog(parent)
	{
      setupUi(this);

      smf0->setChecked(config.smfFormat == 0);
      smf1->setChecked(config.smfFormat == 1);
      QFileInfo fi(name);
      QString s(fi.path() + "/" + fi.baseName() + ".mid");
      label->setText(tr("Save ") + fi.baseName() + tr(" as smf midi file"));
      filename->setText(s);
      connect(fileButton, SIGNAL(clicked()), SLOT(startFileBrowser()));
      }

//---------------------------------------------------------
//   startFileBrowser
//---------------------------------------------------------

void ExportMidiDialog::startFileBrowser()
      {
      QString s = QFileDialog::getSaveFileName(
         (QWidget*)this,
         tr("MusE: export midi smf file"),
         QFileInfo(filename->text()).path(),
         QString("Midi Files (*.mid *.kar *.MID)"),
         0,
         QFileDialog::DontConfirmOverwrite);
      if (!s.isEmpty())
            filename->setText(s);
      }

