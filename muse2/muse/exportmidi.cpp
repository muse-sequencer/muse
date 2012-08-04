//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: exportmidi.cpp,v 1.9.2.1 2009/04/01 01:37:10 terminator356 Exp $
//
//  (C) Copyright 1999-2003 Werner Schweer (ws@seh.de)
//
//  This program is free software; you can redistribute it and/or
//  modify it under the terms of the GNU General Public License
//  as published by the Free Software Foundation; version 2 of
//  the License, or (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//
//=========================================================

#include <stdio.h>
#include <QString>

#include "al/sig.h"  // Tim.

#include "app.h"
#include "midifile.h"
#include "midi.h"
#include "midictrl.h"
#include "globals.h"
#include "filedialog.h"
#include "track.h"
#include "song.h"
#include "mpevent.h"
#include "event.h"
#include "marker/marker.h"
#include "drummap.h"
#include "gconfig.h"

namespace MusECore {

//---------------------------------------------------------
//   addController
//---------------------------------------------------------

static void addController(MPEventList* l, int tick, int port, int channel, int a, int b)
      {
      if (a < CTRL_14_OFFSET) {          // 7 Bit Controller
            l->add(MidiPlayEvent(tick, port, channel, ME_CONTROLLER, a, b));
            }
      else if (a < CTRL_RPN_OFFSET) {     // 14 Bit Controller
            int ctrlH = (a >> 8) & 0x7f;
            int ctrlL = a & 0x7f;
            int dataH = (b >> 7) & 0x7f;
            int dataL = b & 0x7f;
            l->add(MidiPlayEvent(tick, port, channel, ME_CONTROLLER, ctrlH, dataH));
            l->add(MidiPlayEvent(tick+1, port, channel, ME_CONTROLLER, ctrlL, dataL));
            }
      else if (a < CTRL_NRPN_OFFSET) {     // RPN 7-Bit Controller
            int ctrlH = (a >> 8) & 0x7f;
            int ctrlL = a & 0x7f;
            l->add(MidiPlayEvent(tick, port, channel, ME_CONTROLLER, CTRL_HRPN, ctrlH));
            l->add(MidiPlayEvent(tick+1, port, channel, ME_CONTROLLER, CTRL_LRPN, ctrlL));
            l->add(MidiPlayEvent(tick+2, port, channel, ME_CONTROLLER, CTRL_HDATA, b));
            }
      else if (a < CTRL_INTERNAL_OFFSET) {     // NRPN 7-Bit Controller
            int ctrlH = (a >> 8) & 0x7f;
            int ctrlL = a & 0x7f;
            l->add(MidiPlayEvent(tick, port, channel, ME_CONTROLLER, CTRL_HNRPN, ctrlH));
            l->add(MidiPlayEvent(tick+1, port, channel, ME_CONTROLLER, CTRL_LNRPN, ctrlL));
            l->add(MidiPlayEvent(tick+2, port, channel, ME_CONTROLLER, CTRL_HDATA, b));
            }
      else if (a == CTRL_PITCH) {
            int a = b + 8192;
            int b = a >> 7;
            l->add(MidiPlayEvent(tick, port, channel, ME_PITCHBEND, a & 0x7f, b & 0x7f));
            }
      else if (a == CTRL_PROGRAM) {
            int hb = (b >> 16) & 0xff;
            int lb = (b >> 8) & 0xff;
            int pr = b & 0x7f;
            int tickoffset = 0;
            switch(MusEGlobal::song->mtype()) {
                  case MT_GM:       // no HBANK/LBANK
                        break;
                  case MT_GS:
                  case MT_XG:
                  case MT_UNKNOWN:
                        if (hb != 0xff) {
                              l->add(MidiPlayEvent(tick, port, channel, ME_CONTROLLER, CTRL_HBANK, hb));
                              ++tickoffset;
                              }
                        if (lb != 0xff) {
                              l->add(MidiPlayEvent(tick+tickoffset, port, channel, ME_CONTROLLER, CTRL_LBANK, lb));
                              ++tickoffset;
                              }
                        break;
                  }
            l->add(MidiPlayEvent(tick+tickoffset, port, channel, ME_PROGRAM, pr, 0));
            }
      else if (a < CTRL_NRPN14_OFFSET) {     // RPN14 Controller
            int ctrlH = (a >> 8) & 0x7f;
            int ctrlL = a & 0x7f;
            int dataH = (b >> 7) & 0x7f;
            int dataL = b & 0x7f;
            l->add(MidiPlayEvent(tick,   port, channel, ME_CONTROLLER, CTRL_HRPN, ctrlH));
            l->add(MidiPlayEvent(tick+1, port, channel, ME_CONTROLLER, CTRL_LRPN, ctrlL));
            l->add(MidiPlayEvent(tick+2, port, channel, ME_CONTROLLER, CTRL_HDATA, dataH));
            l->add(MidiPlayEvent(tick+3, port, channel, ME_CONTROLLER, CTRL_LDATA, dataL));
            }
      else if (a < CTRL_NONE_OFFSET) {     // NRPN14 Controller
            int ctrlH = (a >> 8) & 0x7f;
            int ctrlL = a & 0x7f;
            int dataH = (b >> 7) & 0x7f;
            int dataL = b & 0x7f;
            l->add(MidiPlayEvent(tick,   port, channel, ME_CONTROLLER, CTRL_HNRPN, ctrlH));
            l->add(MidiPlayEvent(tick+1, port, channel, ME_CONTROLLER, CTRL_LNRPN, ctrlL));
            l->add(MidiPlayEvent(tick+2, port, channel, ME_CONTROLLER, CTRL_HDATA, dataH));
            l->add(MidiPlayEvent(tick+3, port, channel, ME_CONTROLLER, CTRL_LDATA, dataL));
            }
      }

} // namespace MusECore

namespace MusEGui {

//---------------------------------------------------------
//   exportMidi
//---------------------------------------------------------

void MusE::exportMidi()
      {
      MusEGui::MFile file(QString("midis"), QString(".mid"));

      FILE* fp = file.open("w", MusEGlobal::midi_file_save_pattern, this, false, true,
         tr("MusE: Export Midi"));
      if (fp == 0)
            return;
      MusECore::MidiFile mf(fp);

      MusECore::TrackList* tl = MusEGlobal::song->tracks();       // Changed to full track list so user can rearrange tracks.
      MusECore::MidiFileTrackList* mtl = new MusECore::MidiFileTrackList;

      int i = 0;
      MusECore::MidiFileTrack* mft = 0;
      for (MusECore::ciTrack im = tl->begin(); im != tl->end(); ++im) {
        
            if(!(*im)->isMidiTrack())
              continue;
              
            MusECore::MidiTrack* track = (MusECore::MidiTrack*)(*im);

            if (i == 0 || (i != 0 && MusEGlobal::config.smfFormat != 0))    // Changed to single track. Tim
            {  
              mft = new MusECore::MidiFileTrack;
              mtl->push_back(mft);
            }
            
            MusECore::MPEventList* l   = &(mft->events);
            int port         = track->outPort();
            int channel      = track->outChannel();

            //---------------------------------------------------
            //    only first midi track contains
            //          - Track Marker
            //          - copyright
            //          - time signature
            //          - tempo map
            //          - GM/GS/XG Initialization
            //---------------------------------------------------

            if (i == 0) {
                  //---------------------------------------------------
                  //    Write Track Marker
                  //
                  MusECore::MarkerList* ml = MusEGlobal::song->marker();
                  for (MusECore::ciMarker m = ml->begin(); m != ml->end(); ++m) {
                        QByteArray ba = m->second.name().toLatin1();
                        const char* name = ba.constData();
                        int len = ba.length();
                        MusECore::MidiPlayEvent ev(m->first, port, MusECore::ME_META, (unsigned char*)name, len);
                        ev.setA(0x6);
                        l->add(ev);
                        }

                  //---------------------------------------------------
                  //    Write Copyright
                  //
                  QByteArray ba = MusEGlobal::config.copyright.toLatin1();
                  const char* copyright = ba.constData();
                  if (copyright && *copyright) {
                        int len = ba.length();
                        MusECore::MidiPlayEvent ev(0, port, MusECore::ME_META, (unsigned char*)copyright, len);
                        ev.setA(0x2);
                        l->add(ev);
                        }

                  //---------------------------------------------------
                  //    Write Comment
                  //
                  //if (MusEGlobal::config.smfFormat == 0)  // Only for smf 0 added by Tim. FIXME: Is this correct? See below.
                  {
                    QString comment = track->comment();
                    if (!comment.isEmpty()) {
                          int len = comment.length();
                          MusECore::MidiPlayEvent ev(0, port, MusECore::ME_META, (const unsigned char*)(comment.toLatin1().constData()), len);
                          ev.setA(0x1);
                          l->add(ev);
                          }
                  }
                  
                  //---------------------------------------------------
                  //    Write Songtype SYSEX: GM/GS/XG
                  //

                  switch(MusEGlobal::song->mtype()) {
                        case MT_GM:
                              l->add(MusECore::MidiPlayEvent(0, port, MusECore::ME_SYSEX, MusECore::gmOnMsg, MusECore::gmOnMsgLen));
                              break;
                        case MT_GS:
                              l->add(MusECore::MidiPlayEvent(0, port, MusECore::ME_SYSEX, MusECore::gmOnMsg, MusECore::gmOnMsgLen));
                              l->add(MusECore::MidiPlayEvent(250, port, MusECore::ME_SYSEX, MusECore::gsOnMsg, MusECore::gsOnMsgLen));
                              break;
                        case MT_XG:
                              l->add(MusECore::MidiPlayEvent(0, port, MusECore::ME_SYSEX, MusECore::gmOnMsg, MusECore::gmOnMsgLen));
                              l->add(MusECore::MidiPlayEvent(250, port, MusECore::ME_SYSEX, MusECore::xgOnMsg, MusECore::xgOnMsgLen));
                              break;
                        case MT_UNKNOWN:
                              break;
                        }

                  //---------------------------------------------------
                  //    Write Tempomap
                  //
                  MusECore::TempoList* tl = &MusEGlobal::tempomap;
                  for (MusECore::ciTEvent e = tl->begin(); e != tl->end(); ++e) {
                        MusECore::TEvent* event = e->second;
                        unsigned char data[3];
                        int tempo = event->tempo;
                        data[2] = tempo & 0xff;
                        data[1] = (tempo >> 8) & 0xff;
                        data[0] = (tempo >> 16) & 0xff;
                        MusECore::MidiPlayEvent ev(event->tick, port, MusECore::ME_META, data, 3);
                        ev.setA(0x51);
                        l->add(ev);
                        }

                  //---------------------------------------------------
                  //    Write Signatures
                  //
                  const AL::SigList* sl = &AL::sigmap;
                  for (AL::ciSigEvent e = sl->begin(); e != sl->end(); ++e) {
                        AL::SigEvent* event = e->second;
                        int sz = (MusEGlobal::config.exp2ByteTimeSigs ? 2 : 4); // export 2 byte timesigs instead of 4 ?
                        unsigned char data[sz];
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
                                    fprintf(stderr, "wrong Signature; denominator is %d\n", event->sig.n);
                                    break;
                              }
                        // By T356. In muse the metronome pulse is fixed at 24 (once per quarter-note).
                        // The number of 32nd notes per 24 MIDI clock signals (per quarter-note) is 8.
                        if(!MusEGlobal::config.exp2ByteTimeSigs)
                        {
                          data[2] = 24;
                          data[3] = 8;
                        }  
                        
                        MusECore::MidiPlayEvent ev(event->tick, port, MusECore::ME_META, data, sz);
                          
                        ev.setA(0x58);
                        l->add(ev);
                        }
                  }

            //-----------------------------------
            //   track name
            //-----------------------------------

            if (i == 0 || (i != 0 && MusEGlobal::config.smfFormat != 0))
            {
              if (!track->name().isEmpty()) {
                    QByteArray ba = track->name().toLatin1();
                    const char* name = ba.constData();
                    int len = ba.length();
                    MusECore::MidiPlayEvent ev(0, port, MusECore::ME_META, (unsigned char*)name, len+1);
                    ev.setA(0x3);    // Meta Sequence/Track Name
                    l->add(ev);
                    }
            }
            
            //-----------------------------------
            //   track comment
            //-----------------------------------

            if (MusEGlobal::config.smfFormat != 0)
            {
              if (!track->comment().isEmpty()) {
                    QByteArray ba = track->comment().toLatin1();
                    const char* comment = ba.constData();
                    int len = ba.length();
                    MusECore::MidiPlayEvent ev(0, port, MusECore::ME_META, (unsigned char*)comment, len+1);
                    ev.setA(0xf);    // Meta Text
                    l->add(ev);
                    }
            }
            
            MusECore::PartList* parts = track->parts();
            for (MusECore::iPart p = parts->begin(); p != parts->end(); ++p) {
                  MusECore::MidiPart* part    = (MusECore::MidiPart*) (p->second);
                  MusECore::EventList* evlist = part->events();
                  for (MusECore::iEvent i = evlist->begin(); i != evlist->end(); ++i) {
                        MusECore::Event ev = i->second;
                        int tick = ev.tick() + part->tick();
                        switch (ev.type()) {
                              case MusECore::Note:
                                    {
                                    if (ev.velo() == 0) {
                                          printf("Warning: midi note has velocity 0, (ignored)\n");
                                          continue;
                                          }
                                    int pitch;
                                    if (track->type() == MusECore::Track::DRUM) {
                                          // Map drum-notes to the drum-map values
                                          int instr = ev.pitch();
                                          pitch = MusEGlobal::drumMap[instr].anote;
                                          }
                                    else
                                          pitch = ev.pitch();

                                    int velo  = ev.velo();
                                    int len   = ev.lenTick();

                                    //---------------------------------------
                                    //   apply trackinfo values
                                    //---------------------------------------

                                    if (track->transposition
                                       || track->velocity
                                       || track->compression != 100
                                       || track->len != 100) {
                                          pitch += track->transposition;
                                          if (pitch > 127)
                                                pitch = 127;
                                          if (pitch < 0)
                                                pitch = 0;

                                          velo += track->velocity;
                                          velo = (velo * track->compression) / 100;
                                          if (velo > 127)
                                                velo = 127;
                                          if (velo < 1)           // no off event
                                                velo = 1;
                                          len = (len *  track->len) / 100;
                                          }
                                    if (len <= 0)
                                          len = 1;
                                    l->add(MusECore::MidiPlayEvent(tick, port, channel, MusECore::ME_NOTEON, pitch, velo));
                                    
                                    if(MusEGlobal::config.expOptimNoteOffs)  // Save space by replacing note offs with note on velocity 0
                                      l->add(MusECore::MidiPlayEvent(tick+len, port, channel, MusECore::ME_NOTEON, pitch, 0));
                                    else  
                                      l->add(MusECore::MidiPlayEvent(tick+len, port, channel, MusECore::ME_NOTEOFF, pitch, velo));
                                    }
                                    break;

                              case MusECore::Controller:
                                    addController(l, tick, port, channel, ev.dataA(), ev.dataB());
                                    break;

                              case MusECore::Sysex:
                                    l->add(MusECore::MidiPlayEvent(tick, port, MusECore::ME_SYSEX, ev.eventData()));
                                    break;

                              case MusECore::PAfter:
                                    l->add(MusECore::MidiPlayEvent(tick, port, channel, MusECore::ME_AFTERTOUCH, ev.dataA(), ev.dataB()));
                                    break;

                              case MusECore::CAfter:
                                    l->add(MusECore::MidiPlayEvent(tick, port, channel, MusECore::ME_POLYAFTER, ev.dataA(), ev.dataB()));
                                    break;

                              case MusECore::Meta:
                                    {
                                    MusECore::MidiPlayEvent mpev(tick, port, MusECore::ME_META, ev.eventData());
                                    mpev.setA(ev.dataA());
                                    l->add(mpev);
                                    }
                                    break;
                              case MusECore::Wave:
                                    break;
                              }
                        }
                  }
              ++i;  
            
            }
      mf.setDivision(MusEGlobal::config.midiDivision);
      mf.setMType(MusEGlobal::song->mtype());
      mf.setTrackList(mtl, i);
      mf.write();
      
      // DELETETHIS 4 ??? or is this still an issue?
      // TESTING: Cleanup. I did not valgrind this feature in last memleak fixes, but I suspect it leaked. 
      //for(MusECore::iMidiFileTrack imft = mtl->begin(); imft != mtl->end(); ++imft)
      //  delete *imft;
      //delete mtl;
      }

} // namespace MusEGui
