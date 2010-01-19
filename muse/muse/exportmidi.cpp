//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: exportmidi.cpp,v 1.9.2.1 2009/04/01 01:37:10 terminator356 Exp $
//
//  (C) Copyright 1999-2003 Werner Schweer (ws@seh.de)
//=========================================================

#include <stdio.h>
#include <qstring.h>

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

//---------------------------------------------------------
//   addController
//---------------------------------------------------------

static void addController(MPEventList* l, int tick, int port, int channel, int a, int b)
      {
      if (a < 0x1000) {          // 7 Bit Controller
            l->add(MidiPlayEvent(tick, port, channel, ME_CONTROLLER, a, b));
            }
      else if (a < 0x20000) {     // 14 Bit Controller
            int ctrlH = (a >> 8) & 0x7f;
            int ctrlL = a & 0x7f;
            int dataH = (b >> 7) & 0x7f;
            int dataL = b & 0x7f;
            l->add(MidiPlayEvent(tick, port, channel, ME_CONTROLLER, ctrlH, dataH));
            l->add(MidiPlayEvent(tick+1, port, channel, ME_CONTROLLER, ctrlL, dataL));
            }
      else if (a < 0x30000) {     // RPN 7-Bit Controller
            int ctrlH = (a >> 8) & 0x7f;
            int ctrlL = a & 0x7f;
            l->add(MidiPlayEvent(tick, port, channel, ME_CONTROLLER, CTRL_HRPN, ctrlH));
            l->add(MidiPlayEvent(tick+1, port, channel, ME_CONTROLLER, CTRL_LRPN, ctrlL));
            l->add(MidiPlayEvent(tick+2, port, channel, ME_CONTROLLER, CTRL_HDATA, b));
            }
      else if (a < 0x40000) {     // NRPN 7-Bit Controller
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
            switch(song->mtype()) {
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
      else if (a < 0x60000) {     // RPN14 Controller
            int ctrlH = (a >> 8) & 0x7f;
            int ctrlL = a & 0x7f;
            int dataH = (b >> 7) & 0x7f;
            int dataL = b & 0x7f;
            l->add(MidiPlayEvent(tick,   port, channel, ME_CONTROLLER, CTRL_HRPN, ctrlH));
            l->add(MidiPlayEvent(tick+1, port, channel, ME_CONTROLLER, CTRL_LRPN, ctrlL));
            l->add(MidiPlayEvent(tick+2, port, channel, ME_CONTROLLER, CTRL_HDATA, dataH));
            l->add(MidiPlayEvent(tick+3, port, channel, ME_CONTROLLER, CTRL_LDATA, dataL));
            }
      else if (a < 0x70000) {     // NRPN14 Controller
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

//---------------------------------------------------------
//   exportMidi
//---------------------------------------------------------

void MusE::exportMidi()
      {
      MFile file(QString("midis"), QString(".mid"));

      //FILE* fp = file.open("w", midi_file_pattern, this, false, true,
      FILE* fp = file.open("w", midi_file_save_pattern, this, false, true,
         tr("MusE: Export Midi"));
      if (fp == 0)
            return;
      MidiFile mf(fp);

      MidiTrackList* tl = song->midis();
      int ntracks = tl->size();
      MidiFileTrackList* mtl = new MidiFileTrackList;

      int i = 0;
      for (iMidiTrack im = tl->begin(); im != tl->end(); ++im, ++i) {
            MidiTrack* track = *im;
            MidiFileTrack* mft = new MidiFileTrack;
            mtl->push_back(mft);
            MPEventList* l   = &(mft->events);
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
                  MarkerList* ml = song->marker();
                  for (ciMarker m = ml->begin(); m != ml->end(); ++m) {
                        const char* name = m->second.name().latin1();
                        int len = strlen(name);
                        MidiPlayEvent ev(m->first, port, ME_META, (unsigned char*)name, len);
                        ev.setA(0x6);
                        l->add(ev);
                        }

                  //---------------------------------------------------
                  //    Write Copyright
                  //
                  const char* copyright = config.copyright.latin1();
                  if (copyright && *copyright) {
                        int len = strlen(copyright);
                        MidiPlayEvent ev(0, port, ME_META, (unsigned char*)copyright, len);
                        ev.setA(0x2);
                        l->add(ev);
                        }

                  //---------------------------------------------------
                  //    Write Coment
                  //
                  QString comment = track->comment();
                  if (!comment.isEmpty()) {
                        int len = comment.length();
                        MidiPlayEvent ev(0, port, ME_META, (const unsigned char*)(comment.latin1()), len);
                        ev.setA(0x1);
                        l->add(ev);
                        }

                  //---------------------------------------------------
                  //    Write Songtype SYSEX: GM/GS/XG
                  //

                  switch(song->mtype()) {
                        case MT_GM:
                              l->add(MidiPlayEvent(0, port, ME_SYSEX, gmOnMsg, gmOnMsgLen));
                              break;
                        case MT_GS:
                              l->add(MidiPlayEvent(0, port, ME_SYSEX, gmOnMsg, gmOnMsgLen));
                              l->add(MidiPlayEvent(250, port, ME_SYSEX, gsOnMsg, gsOnMsgLen));
                              break;
                        case MT_XG:
                              l->add(MidiPlayEvent(0, port, ME_SYSEX, gmOnMsg, gmOnMsgLen));
                              l->add(MidiPlayEvent(250, port, ME_SYSEX, xgOnMsg, xgOnMsgLen));
                              break;
                        case MT_UNKNOWN:
                              break;
                        }

                  //---------------------------------------------------
                  //    Write Tempomap
                  //
                  TempoList* tl = &tempomap;
                  for (ciTEvent e = tl->begin(); e != tl->end(); ++e) {
                        TEvent* event = e->second;
                        unsigned char data[3];
                        int tempo = event->tempo;
                        data[2] = tempo & 0xff;
                        data[1] = (tempo >> 8) & 0xff;
                        data[0] = (tempo >> 16) & 0xff;
                        MidiPlayEvent ev(event->tick, port, ME_META, data, 3);
                        ev.setA(0x51);
                        l->add(ev);
                        }

                  //---------------------------------------------------
                  //    Write Signatures
                  //
                  const SigList* sl = &sigmap;
                  for (ciSigEvent e = sl->begin(); e != sl->end(); ++e) {
                        SigEvent* event = e->second;
                        int sz = (config.exp2ByteTimeSigs ? 2 : 4); // export 2 byte timesigs instead of 4 ?
                        unsigned char data[sz];
                        data[0] = event->z;
                        switch(event->n) {
                              case 1:  data[1] = 0; break;
                              case 2:  data[1] = 1; break;
                              case 4:  data[1] = 2; break;
                              case 8:  data[1] = 3; break;
                              case 16: data[1] = 4; break;
                              case 32: data[1] = 5; break;
                              case 64: data[1] = 6; break;
                              default:
                                    fprintf(stderr, "falsche Signatur; nenner %d\n", event->n);
                                    break;
                              }
                        // By T356. In muse the metronome pulse is fixed at 24 (once per quarter-note).
                        // The number of 32nd notes per 24 MIDI clock signals (per quarter-note) is 8.
                        if(!config.exp2ByteTimeSigs)
                        {
                          data[2] = 24;
                          data[3] = 8;
                        }  
                        
                        MidiPlayEvent ev(event->tick, port, ME_META, data, sz);
                          
                        ev.setA(0x58);
                        l->add(ev);
                        }
                  }

            //-----------------------------------
            //   track name
            //-----------------------------------

            if (!track->name().isEmpty()) {
                  const char* name = track->name().latin1();
                  int len = strlen(name);
                  MidiPlayEvent ev(0, port, ME_META, (unsigned char*)name, len+1);
                  ev.setA(0x3);    // Meta Sequence/Track Name
                  l->add(ev);
                  }

            //-----------------------------------
            //   track comment
            //-----------------------------------

            if (!track->comment().isEmpty()) {
                  const char* comment = track->comment().latin1();
                  int len = strlen(comment);
                  MidiPlayEvent ev(0, port, ME_META, (unsigned char*)comment, len+1);
                  ev.setA(0xf);    // Meta Text
                  l->add(ev);
                  }
            PartList* parts = track->parts();
            for (iPart p = parts->begin(); p != parts->end(); ++p) {
                  MidiPart* part    = (MidiPart*) (p->second);
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
                                    int pitch;
                                    if (track->type() == Track::DRUM) {
                                          //
                                          // Map drum-notes to the drum-map values
                                          //
                                          int instr = ev.pitch();
                                          pitch = drumMap[instr].anote;
                                          // port  = drumMap[instr].port;
                                          // channel = drumMap[instr].channel;
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
                                    l->add(MidiPlayEvent(tick, port, channel, ME_NOTEON, pitch, velo));
                                    
                                    if(config.expOptimNoteOffs)  // Save space by replacing note offs with note on velocity 0
                                      l->add(MidiPlayEvent(tick+len, port, channel, ME_NOTEON, pitch, 0));
                                    else  
                                      l->add(MidiPlayEvent(tick+len, port, channel, ME_NOTEOFF, pitch, velo));
                                    }
                                    break;

                              case Controller:
                                    addController(l, tick, port, channel, ev.dataA(), ev.dataB());
                                    break;

                              case Sysex:
                                    l->add(MidiPlayEvent(tick, port, ME_SYSEX, ev.eventData()));
                                    break;

                              case PAfter:
                                    l->add(MidiPlayEvent(tick, port, channel, ME_AFTERTOUCH, ev.dataA(), ev.dataB()));
                                    break;

                              case CAfter:
                                    l->add(MidiPlayEvent(tick, port, channel, ME_POLYAFTER, ev.dataA(), ev.dataB()));
                                    break;

                              case Meta:
                                    {
                                    MidiPlayEvent mpev(tick, port, ME_META, ev.eventData());
                                    mpev.setA(ev.dataA());
                                    l->add(mpev);
                                    }
                                    break;
                              case Wave:
                                    break;
                              }
                        }
                  }
            }
      mf.setDivision(config.midiDivision);
      mf.setMType(song->mtype());
      mf.setTrackList(mtl, ntracks);
      mf.write();
      }

