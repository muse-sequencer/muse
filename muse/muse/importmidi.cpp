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

#include "globals.h"
#include "muse.h"
#include "song.h"
#include "widgets/filedialog.h"
#include "midi.h"
#include "midifile.h"
#include "transport.h"
#include "midiedit/drummap.h"
#include "al/sig.h"
#include "al/tempo.h"
#include "instruments/minstrument.h"
#include "gconfig.h"
#include "driver/mididev.h"
#include "part.h"
#include "importmidi.h"
#include "projectdialog.h"
#include "templatedialog.h"
#include "audio.h"
#include "mixer/mixer.h"
#include "arranger/arranger.h"
#include "midictrl.h"
#include "midiinport.h"
#include "midioutport.h"

//---------------------------------------------------------
//   ImportMidiDialog
//---------------------------------------------------------

ImportMidiDialog::ImportMidiDialog(QWidget* parent)
  : QDialog(parent)
      {
      setupUi(this);
      bg = new QButtonGroup(this);
      bg->setExclusive(true);
      bg->addButton(addToProject, 0);
      bg->addButton(createNewProject, 1);
      createNewProject->setChecked(true);
      connect(selectTemplate,SIGNAL(clicked()), SLOT(selectTemplateClicked()));
      connect(selectProject, SIGNAL(clicked()), SLOT(selectProjectClicked()));
      }

//---------------------------------------------------------
//   selectTemplateClicked
//---------------------------------------------------------

void ImportMidiDialog::selectTemplateClicked()
      {
      TemplateDialog templateDialog;
      templateDialog.setTemplatePath(templateName->text());
      int rv = templateDialog.exec();
      if (rv == 0)
            return;
      templateName->setText(templateDialog.templatePath());
      }

//---------------------------------------------------------
//   selectProjectClicked
//---------------------------------------------------------

void ImportMidiDialog::selectProjectClicked()
      {
      ProjectDialog projectDialog;
      projectDialog.setProjectName(projectName->text());
      int rv = projectDialog.exec();
      if (rv == 0)
            return;
      projectName->setText(projectDialog.projectName());
      }

//---------------------------------------------------------
//   setProjectName
//---------------------------------------------------------

void ImportMidiDialog::setProjectName(const QString& name)
      {
      projectName->setText(name);
      }

//---------------------------------------------------------
//   setTemplateName
//---------------------------------------------------------

void ImportMidiDialog::setTemplateName(const QString& name)
      {
      templateName->setText(name);
      }

//---------------------------------------------------------
//   importMidi
//---------------------------------------------------------

void MusE::importMidi()
      {
      importMidi(QString());
      }

void MusE::importMidi(const QString &file)
      {
      QString fn;
      QStringList pattern;

      const char** s = midi_file_pattern;
      while (*s)
            pattern << *s++;

      if (file.isEmpty()) {
            fn = getOpenFileName(lastMidiPath, pattern, this,
               tr("MusE: Import Midi"));
            lastMidiPath = fn;
            if (fn.isEmpty())
                  return;
            }
      else
            fn = file;

      QFileInfo fi(fn);

      ImportMidiDialog mid(this);
      mid.setProjectName(fi.baseName());
      mid.setTemplateName("");

      if (mid.exec() == 0)
            return;

      if (mid.doCreateNewProject()) {
            QString header = tr("MusE: import midi file");
            QString path(mid.projectName->text());
            QDir pd(QDir::homePath() + "/" + config.projectPath + "/" + path);

            if (leaveProject())
                  return;
            if (!pd.mkdir(pd.path())) {
                  QString s(tr("Cannot create project folder <%1>"));
                  QMessageBox::critical(this, header, s.arg(pd.path()));
                  return;
                  }
            if (audio->isPlaying()) {
                  audio->msgPlay(false);
                  while (audio->isPlaying())
                        qApp->processEvents();
                  }
            addProject(path);       // add to history
            seqStop();
            if (mixer1)
                  mixer1->clear();
            if (mixer2)
                  mixer2->clear();
            //===========================================================
            //
            // close all toplevel windows
            //
            foreach(QWidget* w, QApplication::topLevelWidgets()) {
                  if (!w->isVisible())
                        continue;
                  if (strcmp("DrumEdit", w->metaObject()->className()) == 0)
                        w->close();
                  else if (strcmp("PianoRoll", w->metaObject()->className()) == 0)
                        w->close();
                  else if (strcmp("MasterEdit", w->metaObject()->className()) == 0)
                        w->close();
                  else if (strcmp("WaveEdit", w->metaObject()->className()) == 0)
                        w->close();
                  else if (strcmp("ListEdit", w->metaObject()->className()) == 0)
                        w->close();
                  }
            emit startLoadSong();
            song->setProjectPath(path);
            song->clear(false);
            song->setCreated(true);

            QString s = mid.templateName->text();
            bool rv = true;
            if (!s.isEmpty()) {
                  QFile f(s);
                  if (f.open(QIODevice::ReadOnly)) {
                        rv = song->read(&f);
                        f.close();
                        }
                  else {
                        QString msg(tr("Cannot open template file\n%1"));
                        QMessageBox::critical(this, header, msg.arg(s));
                        }
                  }
            if (!rv) {
                  QString msg(tr("File <%1> read error"));
                  QMessageBox::critical(this, header, msg.arg(s));
                  }
            addMidiFile(fn);

            tr_id->setChecked(config.transportVisible);
            bt_id->setChecked(config.bigTimeVisible);

            //
            // dont emit song->update():
            song->blockSignals(true);

            showBigtime(config.bigTimeVisible);
            showMixer1(config.mixer1Visible);
            showMixer2(config.mixer2Visible);
            if (mixer1 && config.mixer1Visible)
                  mixer1->setUpdateMixer();
            if (mixer2 && config.mixer2Visible)
                  mixer2->setUpdateMixer();

            resize(config.geometryMain.size());
            move(config.geometryMain.topLeft());

            if (config.transportVisible)
                  transport->show();
            transport->move(config.geometryTransport.topLeft());
            showTransport(config.transportVisible);

            song->blockSignals(false);

            transport->setMasterFlag(song->masterFlag());
            punchinAction->setChecked(song->punchin());
            punchoutAction->setChecked(song->punchout());
            loopAction->setChecked(song->loop());
            clipboardChanged();           // enable/disable "Paste"
            song->setLen(song->len());    // emit song->lenChanged() signal

            //
            // add connected channels
            //
            TrackList* tl       = song->tracks();
#if 0
            MidiChannelList* mcl = song->midiChannel();
            for (iMidiChannel i = mcl->begin(); i != mcl->end(); ++i) {
                  MidiChannel* mc = (MidiChannel*)*i;
                  if (mc->noInRoute() || song->trackExists(mc))
                        continue;
                  tl->push_back(mc);
                  }
#endif

            selectionChanged();           // enable/disable "Copy" & "Paste"
            arranger->endLoadSong();
            song->updatePos();
            //
            // send "cur" controller values to devices
            //

            for (iTrack i = tl->begin(); i != tl->end(); ++i) {
                  Track* track = *i;
                  track->blockSignals(true);
                  CtrlList* cl = track->controller();
                  for (iCtrl ic = cl->begin(); ic != cl->end(); ++ic) {
                        Ctrl* ctrl = ic->second;
                        if (ctrl->type() & Ctrl::INT) {
                              CVal val;
                              val = ctrl->curVal();
                              ctrl->setCurVal(CTRL_VAL_UNKNOWN);
                              song->setControllerVal(track, ctrl, val);
                              }
                        }
                  track->blockSignals(false);
                  }
            setWindowTitle(QString("MusE: Song: ") + path);
            seqStart();
            audio->msgSeek(song->cPos());
            }
      else {
            // add to project
            addMidiFile(fn);
            song->update(-1);
            }
      }

//---------------------------------------------------------
//   addMidiFile
//---------------------------------------------------------

void MusE::addMidiFile(const QString name)
      {
      QFile* fp = fileOpen(this, name, QString(".mid"), QIODevice::ReadOnly);
      if (fp == 0)
            return;
      MidiFile mf;
      bool rv = mf.read(fp);
      fp->close();
      delete fp;

      if (rv) {
            QString s(tr("reading midifile\n  "));
            s += name;
            s += tr("\nfailed: ");
            s += mf.error();
            QMessageBox::critical(this, QString("MusE"), s);
            return;
            }
      MidiFileTrackList* etl = mf.trackList();
      int division           = mf.division();

      MidiOutPort* outPort = 0;

      if (song->midiOutPorts()->empty()) {
            outPort = new MidiOutPort();
            outPort->setDefaultName();
            song->insertTrack0(outPort, -1);

#if 0
            //
            // route output to preferred midi device
            //
            if (!config.defaultMidiOutputDevice.isEmpty()) {
                  Route dst(config.defaultMidiOutputDevice, 0, Route::MIDIPORT);
                  outPort->outRoutes()->push_back(dst);
                  }
#endif
            //
            // set preferred instrument
            //
            MidiInstrument* instr = 0;    // genericMidiInstrument;
            for (iMidiInstrument mi = midiInstruments.begin(); mi != midiInstruments.end(); ++mi) {
                  if ((*mi)->iname() == config.defaultMidiInstrument) {
                        instr = *mi;
                        break;
                        }
                  }

            //
            // if midi file is GM/GS/XG this overrides the preferred
            // instrument setting

            if (mf.midiType() != MT_GENERIC) {
                  MidiInstrument* instr2 = 0;
                  for (iMidiInstrument i = midiInstruments.begin(); i != midiInstruments.end(); ++i) {
                        MidiInstrument* mi = *i;
                        switch(mf.midiType()) {
                              case MT_GM:
                                    if (mi->iname() == "GM")
                                          instr2 = mi;
                                    break;
                              case MT_GS:
                                    if (mi->iname() == "GS")
                                          instr2 = mi;
                                    break;
                              case MT_XG:
                                    if (mi->iname() == "XG")
                                          instr2 = mi;
                                    break;
                              case MT_GENERIC: // cannot happen
                                    break;
                              }
                        if (instr2)
                              break;
                        }
                  if (instr2)
                        instr = instr2;
                  }
            if (instr == 0)
                  instr = genericMidiInstrument;
            outPort->setInstrument(instr);
            }
      else
            outPort = song->midiOutPorts()->front();

      //
      // create MidiTrack and copy events to ->events()
      //    - combine note on/off events
      //    - calculate tick value for internal resolution
      //

      foreach(const MidiFileTrack* t, *etl) {
            const MidiEventList& el = t->events;
            if (el.empty())
                  continue;
            //
            // if we split the track, SYSEX and META events go into
            // the first target track

            bool first = true;
            for (int channel = 0; channel < MIDI_CHANNELS; ++channel) {
                  //
                  // check if there are any events for channel in track:
                  //
                  iMidiEvent i;
                  for (i = el.begin(); i != el.end(); ++i) {
                        MidiEvent ev = *i;
                        if (ev.type() != ME_SYSEX && ev.type() != ME_META && ev.channel() == channel)
                              break;
                        }
                  if (i == el.end())
                        continue;

                  MidiTrack* track = new MidiTrack();
                  if (t->isDrumTrack)
                        track->setUseDrumMap(true);
//TODOB                  track->outRoutes()->push_back(Route(outPort->channel(channel), -1, Route::TRACK));
//                  if (inPort && config.connectToAllMidiTracks) {
//                        for (int ch = 0; ch < MIDI_CHANNELS; ++ch) {
//                              Route src(inPort, ch, Route::TRACK);
//                              track->inRoutes()->push_back(src);
//                              }
//                        }

                  EventList* mel = track->events();
                  buildMidiEventList(mel, &el, track, channel, division, first);
                  first = false;

                  for (iEvent i = mel->begin(); i != mel->end(); ++i) {
                        Event event = i->second;
                        if (event.type() == Controller) {
                              int ctrl = event.dataA();
                              MidiInstrument* instr = outPort->instrument();
                              track->addMidiController(instr, ctrl);
                              CVal val;
                              val.i = event.dataB();
                              track->addControllerVal(ctrl, event.tick(), val);
                              }
                        }
                  processTrack(track);
                  if (track->name().isEmpty())
                        track->setDefaultName();
                  song->insertTrack0(track, -1);
                  }
            if (first) {
                  //
                  // track does only contain non-channel messages
                  // (SYSEX or META)
                  //
                  MidiTrack* track = new MidiTrack();
                  EventList* mel = track->events();
                  buildMidiEventList(mel, &el, track, 0, division, true);
                  processTrack(track);
                  if (track->name().isEmpty())
                        track->setDefaultName();
                  song->insertTrack0(track, -1);
                  }
            }

      TrackList* tl = song->tracks();
      if (!tl->empty()) {
            Track* track = tl->front();
            track->setSelected(true);
            }
      unsigned int l = 1;
      MidiTrackList* mtl = song->midis();
      for (iMidiTrack t = mtl->begin(); t != mtl->end(); ++t) {
            MidiTrack* track = *t;
            PartList* parts = track->parts();
            for (iPart p = parts->begin(); p != parts->end(); ++p) {
                  unsigned last = p->second->tick() + p->second->lenTick();
                  if (last > l)
                        l = last;
                  }
            }
      song->setLen(l);
      AL::TimeSignature sig = AL::sigmap.timesig(0);
      int z = sig.z;
      int n = sig.n;

      transport->setTimesig(z, n);
//    int tempo = AL::tempomap.tempo(0);
//    transport->setTempo(tempo);

      bool masterF = !AL::tempomap.empty();
      song->setMasterFlag(masterF);
      transport->setMasterFlag(masterF);

      song->updatePos();
      }

//---------------------------------------------------------
//   processTrack
//    divide events into parts
//---------------------------------------------------------

void MusE::processTrack(MidiTrack* track)
      {
      EventList* tevents = track->events();
      if (tevents->empty())
            return;

      //---------------------------------------------------
      //    create parts
      //    Break midi tracks into parts.
      //    A new part is created when a gap of at least
      //    one measure is detected. Part len is aligned
      //    to one measure.
      //---------------------------------------------------

      PartList* pl = track->parts();

      int lastTick = 0;
      for (iEvent i = tevents->begin(); i != tevents->end(); ++i) {
            Event event = i->second;
            int epos = event.tick() + event.lenTick();
            if (epos > lastTick)
                  lastTick = epos;
            }

      int len = song->roundUpBar(lastTick+1);
      int bar2, beat;
      unsigned tick;
      AL::sigmap.tickValues(len, &bar2, &beat, &tick);

      QString partname = track->name();

      int lastOff = 0;
      int st = -1;      // start tick current part
      int x1 = 0;       // start tick current measure
      int x2 = 0;       // end tick current measure

      for (int bar = 0; bar < bar2; ++bar, x1 = x2) {
            x2 = AL::sigmap.bar2tick(bar+1, 0, 0);
            if (lastOff > x2) {
                  // this measure is busy!
                  continue;
                  }
            iEvent i1 = tevents->lower_bound(x1);
            iEvent i2 = tevents->lower_bound(x2);

            if (i1 == i2) {   // empty?
                  if (st != -1) {
                        Part* part = new Part(track);
                        part->ref();
                        part->setType(AL::TICKS);
                        part->setTick(st);
                        part->setLenTick(x1-st);
                        part->setName(partname);
                        pl->add(part);
                        st = -1;
                        }
                  }
            else {
                  if (st == -1)
                        st = x1;    // begin new  part
                  //HACK:
                  //lastOff:
                  for (iEvent i = i1; i != i2; ++i) {
                        Event event = i->second;
                        if (event.type() == Note) {
                              int off = event.tick() + event.lenTick();
                              if (off > lastOff)
                                    lastOff = off;
                              }
                        }
                  }
            }
      if (st != -1) {
            Part* part = new Part(track);
            part->ref();
            part->setType(AL::TICKS);
            part->setTick(st);
            part->setLenTick(x2-st);
            part->setName(partname);
            pl->add(part);
            }

      //-------------------------------------------------------------
      //    assign events to parts
      //-------------------------------------------------------------

      for (iPart p = pl->begin(); p != pl->end(); ++p) {
            Part* part = p->second;
            int stick = part->tick();
            int etick = part->tick() + part->lenTick();
            iEvent r1 = tevents->lower_bound(stick);
            iEvent r2 = tevents->lower_bound(etick);
            int startTick = part->tick();

            EventList* el = part->events();
            for (iEvent i = r1; i != r2; ++i) {
                  Event ev = i->second;
                  int ntick = ev.tick() - startTick;
                  ev.setTick(ntick);
                  el->add(ev, ntick);
                  }
            tevents->erase(r1, r2);
            }

      if (tevents->size())
            printf("-----------events left: %zd\n", tevents->size());
      for (iEvent i = tevents->begin(); i != tevents->end(); ++i) {
            printf("%d===\n", i->first);
            i->second.dump();
            }
      // all events should be processed:
      assert(tevents->empty());
      }

