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

#include "song.h"
#include "midi.h"
#include "midiedit/drummap.h"
#include "event.h"
#include "globals.h"
#include "midictrl.h"
#include "midictrl.h"
#include "audio.h"
#include "driver/mididev.h"
#include "driver/audiodev.h"
#include "wave.h"
#include "synth.h"
#include "sync.h"
#include "midiseq.h"
#include "gconfig.h"
#include "ticksynth.h"
#include "al/tempo.h"
#include "al/sig.h"
#include "part.h"
#include "midiplugin.h"
#include "midiinport.h"
#include "midioutport.h"
#include "midichannel.h"

extern void dump(const unsigned char* p, int n);

unsigned const char gmOnMsg[] = { 
      0x7e,       // Non-Real Time header
      0x7f,       // ID of target device (7f = all devices)
      0x09, 
      0x01 
      };
unsigned const char gsOnMsg[] = { 
      0x41,       // roland id
      0x10,       // Id of target device (default = 10h for roland)
      0x42,       // model id (42h = gs devices)
      0x12,       // command id (12h = data set)
      0x40,       // address & value
      0x00,
      0x7f, 
      0x00,        
      0x41        // checksum?
      };
unsigned const char xgOnMsg[] = { 
      0x43,       // yamaha id
      0x10,       // device number
      0x4c,       // model id
      0x00,       // address (high, mid, low)
      0x00, 
      0x7e, 
      0x00        // data
      };
unsigned const int  gmOnMsgLen = sizeof(gmOnMsg);
unsigned const int  gsOnMsgLen = sizeof(gsOnMsg);
unsigned const int  xgOnMsgLen = sizeof(xgOnMsg);

/*---------------------------------------------------------
 *    midi_meta_name
 *---------------------------------------------------------*/

QString midiMetaName(int meta)
      {
      const char* s = "";
      switch (meta) {
            case 0:     s = "Sequence Number"; break;
            case 1:     s = "Text Event"; break;
            case 2:     s = "Copyright"; break;
            case 3:     s = "Sequence/Track Name"; break;
            case 4:     s = "Instrument Name"; break;
            case 5:     s = "Lyric"; break;
            case 6:     s = "Marker"; break;
            case 7:     s = "Cue Point"; break;
            case 8:
            case 9:
            case 0x0a:
            case 0x0b:
            case 0x0c:
            case 0x0d:
            case 0x0e:
            case 0x0f:  s = "Text"; break;
            case 0x20:  s = "Channel Prefix"; break;
            case 0x21:  s = "Port Change"; break;
            case 0x2f:  s = "End of Track"; break;
            case 0x51:  s = "Set Tempo"; break;
            case 0x54:  s = "SMPTE Offset"; break;
            case 0x58:  s = "Time Signature"; break;
            case 0x59:  s = "Key Signature"; break;
            case 0x74:  s = "Sequencer-Specific1"; break;
            case 0x7f:  s = "Sequencer-Specific2"; break;
            default:
                  break;
            }
      return QString(s);
      }

//---------------------------------------------------------
//   QString nameSysex
//---------------------------------------------------------

QString nameSysex(unsigned int len, const unsigned char* buf)
      {
      QString s;
      switch(buf[0]) {
            case 0x00:
                  if (buf[1] == 0 && buf[2] == 0x41)
                        s = "Microsoft";
                  break;
            case 0x01:  s = "Sequential Circuits: "; break;
            case 0x02:  s = "Big Briar: "; break;
            case 0x03:  s = "Octave / Plateau: "; break;
            case 0x04:  s = "Moog: "; break;
            case 0x05:  s = "Passport Designs: "; break;
            case 0x06:  s = "Lexicon: "; break;
            case 0x07:  s = "Kurzweil"; break;
            case 0x08:  s = "Fender"; break;
            case 0x09:  s = "Gulbransen"; break;
            case 0x0a:  s = "Delta Labas"; break;
            case 0x0b:  s = "Sound Comp."; break;
            case 0x0c:  s = "General Electro"; break;
            case 0x0d:  s = "Techmar"; break;
            case 0x0e:  s = "Matthews Research"; break;
            case 0x10:  s = "Oberheim"; break;
            case 0x11:  s = "PAIA: "; break;
            case 0x12:  s = "Simmons: "; break;
            case 0x13:  s = "DigiDesign"; break;
            case 0x14:  s = "Fairlight: "; break;
            case 0x15:  s = "JL Cooper"; break;
            case 0x16:  s = "Lowery"; break;
            case 0x17:  s = "Lin"; break;
            case 0x18:  s = "Emu"; break;
            case 0x1b:  s = "Peavy"; break;
            case 0x20:  s = "Bon Tempi: "; break;
            case 0x21:  s = "S.I.E.L: "; break;
            case 0x23:  s = "SyntheAxe: "; break;
            case 0x24:  s = "Hohner"; break;
            case 0x25:  s = "Crumar"; break;
            case 0x26:  s = "Solton"; break;
            case 0x27:  s = "Jellinghaus Ms"; break;
            case 0x28:  s = "CTS"; break;
            case 0x29:  s = "PPG"; break;
            case 0x2f:  s = "Elka"; break;
            case 0x36:  s = "Cheetah"; break;
            case 0x3e:  s = "Waldorf"; break;
            case 0x40:  s = "Kawai: "; break;
            case 0x41:  s = "Roland: "; break;
            case 0x42:  s = "Korg: "; break;
            case 0x43:  s = "Yamaha: "; break;
            case 0x44:  s = "Casio"; break;
            case 0x45:  s = "Akai"; break;
            case 0x7c:  s = "MusE Soft Synth"; break;
            case 0x7d:  s = "Educational Use"; break;
            case 0x7e:  s = "Universal: Non Real Time"; break;
            case 0x7f:  s = "Universal: Real Time"; break;
            default:    s = "??: "; break;
            }
      //
      // following messages should not show up in event list
      // they are filtered while importing midi files
      //
      if ((len == gmOnMsgLen) && memcmp(buf, gmOnMsg, gmOnMsgLen) == 0)
            s += "GM-ON";
      else if ((len == gsOnMsgLen) && memcmp(buf, gsOnMsg, gsOnMsgLen) == 0)
            s += "GS-ON";
      else if ((len == xgOnMsgLen) && memcmp(buf, xgOnMsg, xgOnMsgLen) == 0)
            s += "XG-ON";
      return s;
      }

//---------------------------------------------------------
//   buildMidiEventList
//    TODO:
//      parse data increment/decrement controller
//      NRPN/RPN  fine/course data 7/14 Bit
//          must we set datah/datal to zero after change
//          of NRPN/RPN register?
//      generally: how to handle incomplete messages?
//---------------------------------------------------------

void buildMidiEventList(EventList* del, const MPEventList* el, MidiTrack* track,
   int div, bool addSysexMeta)
      {
      QString tname;
      int hbank    = 0xff;
      int lbank    = 0xff;
      int rpnh     = -1;
      int rpnl     = -1;
      int datah    = 0;
      int datal    = 0;
      int dataType = 0;   // 0 : disabled, 0x20000 : rpn, 0x30000 : nrpn

      int channel = 0;
      MidiChannel* mc = track->channel();
      if (mc)
            channel = mc->channelNo();

      EventList mel;
      int metaChannel = -1;
      for (iMPEvent i = el->begin(); i != el->end(); ++i) {
            MidiEvent ev = *i;
            if (ev.type() == ME_META) {
                  if (ev.dataA() == 0x20) {
                        metaChannel = ((char*)(ev.data()))[0];
                        }
                  if (metaChannel == channel) {
                        if (ev.dataA() == 0x4) {
                              tname = (char*)(ev.data());
                              }
                        }
                  }
            if (!addSysexMeta && (ev.type() == ME_SYSEX || ev.type() == ME_META))
                  continue;
            if (!(ev.type() == ME_SYSEX || ev.type() == ME_META || (ev.channel() == channel)))
                  continue;
            int tick = ev.time();
            Event e;
            switch(ev.type()) {
                  case ME_NOTEON:
                        e.setType(Note);

                        if (mc && mc->useDrumMap()) {
                              int instr = mc->drumMap()->inmap(ev.dataA());
                              e.setPitch(instr);
                              }
                        else
                              e.setPitch(ev.dataA());

                        e.setVelo(ev.dataB());
                        e.setLenTick(0);
                        break;
                  case ME_NOTEOFF:
                        e.setType(Note);
                        if (mc && mc->useDrumMap()) {
                              int instr = mc->drumMap()->inmap(ev.dataA());
                              e.setPitch(instr);
                              }
                        else
                              e.setPitch(ev.dataA());
                        e.setVelo(0);
                        e.setVeloOff(ev.dataB());
                        e.setLenTick(0);
                        break;
                  case ME_POLYAFTER:
                        e.setType(PAfter);
                        e.setA(ev.dataA());
                        e.setB(ev.dataB());
                        break;
                  case ME_CONTROLLER:
                        {
                        int val = ev.dataB();
                        switch(ev.dataA()) {
                              case CTRL_HBANK:
                                    hbank = val;
                                    break;

                              case CTRL_LBANK:
                                    lbank = val;
                                    break;

                              case CTRL_HDATA:
                                    datah = val;
                                    // check if a CTRL_LDATA follows
                                    // e.g. wie have a 14 bit controller:
                                    {
                                    iMPEvent ii = i;
                                    ++ii;
                                    bool found = false;
                                    for (; ii != el->end(); ++ii) {
                                          MidiEvent ev = *ii;
                                          if (ev.type() == ME_CONTROLLER) {
                                                if (ev.dataA() == CTRL_LDATA) {
                                                      // handle later
                                                      found = true;
                                                      }
                                                break;
                                                }
                                          }
                                    if (!found) {
                                          if (rpnh == -1 || rpnl == -1) {
                                                printf("parameter number not defined, data 0x%x\n", datah);
                                                }
                                          else {
                                                int ctrl = dataType | (rpnh << 8) | rpnl;
                                                e.setType(Controller);
                                                e.setA(ctrl);
                                                e.setB(datah);
                                                }
                                          }
                                    }
                                    break;

                              case CTRL_LDATA:
                                    datal = val;

                                    if (rpnh == -1 || rpnl == -1) {
                                          printf("parameter number not defined, data 0x%x 0x%x, tick %d, channel %d\n",
                                             datah, datal, tick, channel);
                                          break;
                                          }
                                    // assume that the sequence is always
                                    //    CTRL_HDATA - CTRL_LDATA
                                    // eg. that LDATA is always send last

                                    e.setType(Controller);
                                    // 14 Bit RPN/NRPN
                                    e.setA((dataType+0x30000) | (rpnh << 8) | rpnl);
                                    e.setB((datah << 7) | datal);
                                    break;

                              case CTRL_HNRPN:
                                    rpnh = val;
                                    dataType = 0x30000;
                                    break;

                              case CTRL_LNRPN:
                                    rpnl = val;
                                    dataType = 0x30000;
                                    break;

                              case CTRL_HRPN:
                                    rpnh     = val;
                                    dataType = 0x20000;
                                    break;

                              case CTRL_LRPN:
                                    rpnl     = val;
                                    dataType = 0x20000;
                                    break;

                              default:
                                    e.setType(Controller);
                                    e.setA(ev.dataA());
                                    e.setB(val);
                                    break;
                              }
                        }
                        break;

                  case ME_PROGRAM:
                        e.setType(Controller);
                        e.setA(CTRL_PROGRAM);
                        e.setB((hbank << 16) | (lbank << 8) | ev.dataA());
                        break;

                  case ME_AFTERTOUCH:
                        e.setType(CAfter);
                        e.setA(ev.dataA());
                        break;

                  case ME_PITCHBEND:
                        e.setType(Controller);
                        e.setA(CTRL_PITCH);
                        e.setB(ev.dataA());
                        break;

                  case ME_SYSEX:
                        e.setType(Sysex);
                        e.setData(ev.data(), ev.len());
                        break;

                  case ME_META:
                        {
                        const unsigned char* data = ev.data();
                        switch (ev.dataA()) {
                              case 0x01: // Text
                                    if (track->comment().isEmpty())
                                          track->setComment(QString((const char*)data));
                                    else
                                          track->setComment(track->comment() + "\n" + QString((const char*)data));
                                    break;
                              case 0x02:  // Copyright
                                    config.copyright = (char*)data;
                                    break;
                              case 0x03: // Sequence-/TrackName
                                    tname = (char*)data;
                                    break;
                              case 0x6:   // Marker
                                    {
                                    Pos pos((tick * config.division + div/2) / div, AL::TICKS);
                                    song->addMarker(QString((const char*)(data)), pos);
                                    }
                                    break;
                              case 0x4:   // Instrument Name
                              case 0x5:   // Lyrics
                              case 0x8:   // text
                              case 0x9:
                              case 0xa:
                              case 0x20:  // channel prefix
                              case 0x21:  // port change
                                    break;

                              case 0x0f:        // Track Comment
                                    track->setComment(QString((char*)data));
                                    break;

                              case 0x51:        // Tempo
                                    {
                                    int tempo = data[2] + (data[1] << 8) + (data[0] <<16);
                                    int ltick  = (tick * config.division + div/2) / div;
                                    AL::tempomap.addTempo(ltick, tempo);
                                    }
                                    break;

                              case 0x58:        // Time Signature
                                    {
                                    int timesig_z = data[0];
                                    int n = data[1];
                                    int timesig_n = 1;
                                    for (int i = 0; i < n; i++)
                                          timesig_n *= 2;
                                    int ltick  = (tick * config.division + div/2) / div;
                                    AL::sigmap.add(ltick, AL::TimeSignature(timesig_z, timesig_n));
                                    }
                                    break;

                              case 0x59:  // Key Signature
                                    printf("Meta: Key Signature %d %d\n", data[0], data[1]);
                                    // track->scale.set(data[0]);
                                    // track->scale.setMajorMinor(data[1]);
                                    break;

                              case 0x7f:  // Sequencer Specific
                                    printf("Meta: Seq specific:len %d\n", ev.len());
                                    break;

                              default:
                                    printf("unknown Meta 0x%x %d\n", ev.dataA(), ev.dataA());
                              }
                        }
                        break;
                  }   // switch(ev.type()
            if (!e.empty()) {
                  e.setTick(tick);
                  mel.add(e);
                  }
            }  // i != el->end()

      //---------------------------------------------------
      //    resolve NoteOff events
      //---------------------------------------------------

      for (iEvent i = mel.begin(); i != mel.end(); ++i) {
            Event ev  = i->second;
            if (!ev.isNote())
                  continue;
            if (ev.lenTick())
                  continue;
// printf("%d note %d %d\n", ev.tick(), ev.pitch(), ev.velo());
            if (ev.isNoteOff()) {
                  bool found = false;
                  iEvent k = i;
// printf("  start with note off\n");
                  for (++k; k != mel.end(); ++k) {
                        Event event = k->second;
                        if (event.tick() > ev.tick())
                              break;
                        if (event.isNote() && event.velo() && event.pitch() == ev.pitch()) {
                              ev.setLenTick(1);
                              ev.setVelo(event.velo());
                              ev.setVeloOff(0);
                              found = true;
// printf("    found on: %d\n", event.tick());
                              break;
                              }
                        }
                  if (!found) {
                        // maybe found note with velocity zero is really a
                        // note-on !
                        k = i;
                        found = true;
                        for (++k; k != mel.end(); ++k) {
                              Event event = k->second;
                              if (!event.isNote())
                                    continue;
                              if (event.pitch() == ev.pitch()) {
                                    if (event.velo() == 0) {
                                          found = true;
                                          int t = event.tick() - ev.tick();
                                          ev.setLenTick(t);
                                          ev.setVelo(1);
                                          ev.setVeloOff(0);
                                          mel.erase(k);
                                          i = mel.begin();
                                          }
                                    break;
                                    }
                              }
                        if (!found) {
                              printf("NOTE OFF without Note ON tick %d type %d  %d %d\n",
                                    ev.tick(), ev.type(), ev.pitch(), ev.velo());
                              }
                        else
                              continue;
                        }
                  else {
                        mel.erase(k);
                        i = mel.begin();  // DEBUG
                        continue;
                        }
                  }
            iEvent k;
            // ev is noteOn
            for (k = mel.lower_bound(ev.tick()); k != mel.end(); ++k) {
                  Event event = k->second;
// printf("  - %d(%d,%d)\n", event.tick(), event.pitch(), event.velo());
                  if (ev.isNoteOff(event)) {
                        int t = k->first - i->first;
                        if (t <= 0) {
                              if (debugMsg) {
                                    printf("Note len is (%d-%d)=%d, set to 1\n",
                                      k->first, i->first, k->first - i->first);
                                    ev.dump();
                                    event.dump();
                                    }
                              t = 1;
                              }
                        ev.setLenTick(t);
                        ev.setVeloOff(event.veloOff());
                        break;
                        }
                  }
            if (k == mel.end()) {
                  printf("  -no note-off! %d pitch %d velo %d\n",
                     ev.tick(), ev.pitch(), ev.velo());
                  //
                  // switch off at end of measure
                  //
                  int endTick = song->roundUpBar(ev.tick()+1);
                  ev.setLenTick(endTick-ev.tick());
                  }
            else {
                  mel.erase(k);
                  i = mel.begin();
                  }
            }

      for (iEvent i = mel.begin(); i != mel.end(); ++i) {
            Event ev  = i->second;
            if (ev.isNoteOff()) {
                  printf("+extra note-off! %d pitch %d velo %d\n",
                           i->first, ev.pitch(), ev.velo());
//                  ev.dump();
                  continue;
                  }
            int tick  = (ev.tick() * config.division + div/2) / div;
            if (ev.isNote()) {
                  int lenTick = (ev.lenTick() * config.division + div/2) / div;
                  ev.setLenTick(lenTick);
                  }
            ev.setTick(tick);
            if (ev.type() == Controller) {
                  int id = ev.dataA();
                  CVal val;
                  val.i = ev.dataB();

                  bool found = false;
                  for (iRoute i = track->outRoutes()->begin(); i != track->outRoutes()->end(); ++i) {
                        MidiChannel* ch = (MidiChannel*)(i->track);
                        Ctrl* c = ch->getController(id);
                        if (c) {
                              found = true;
                              c->add(tick, val);
                              }
                        }
                  if (!found) {
                        // if no managed controller, store as event
                        del->add(ev);
                        }
                  }
            else
                  del->add(ev);
            }
      if (!tname.isEmpty())
            track->setName(tname);
      }

//---------------------------------------------------------
//   initDevices
//    - called on seek to position 0
//    - called from midi pulldown menu
//---------------------------------------------------------

void Audio::initDevices()
      {
      //
      // test for explicit instrument initialization
      //
      MidiOutPortList* mpl = song->midiOutPorts();
      for (iMidiOutPort i = mpl->begin(); i != mpl->end(); ++i) {
            MidiOutPort* mp = *i;
            MidiInstrument* instr = mp->instrument();
            if (!instr)
                  continue;
            EventList* events = instr->midiInit();
            if (events->empty())
                  continue;
            for (iEvent ie = events->begin(); ie != events->end(); ++ie) {
                  MidiEvent ev(0, 0, ie->second);
                  mp->playMidiEvent(&ev);
                  }
            }
      MidiChannelList* mcl = song->midiChannel();
      for (iMidiChannel i = mcl->begin(); i != mcl->end(); ++i) {
            MidiChannel* mc = *i;
            if (mc->noInRoute())
                  continue;
            if (mc->autoRead()) {
                  CtrlList* cl = mc->controller();
                  for (iCtrl ic = cl->begin(); ic != cl->end(); ++ic) {
                        ic->second->setCurVal(CTRL_VAL_UNKNOWN);
                        }
                  }
            }
      }

//---------------------------------------------------------
//   processMidi
//    - called from midiseq thread,
//      executed in audio thread
//	Process one time slice of midi events
//---------------------------------------------------------

void Audio::processMidi()
      {
      midiBusy = true;
      MidiOutPortList* ol = song->midiOutPorts();
      for (iMidiOutPort id = ol->begin(); id != ol->end(); ++id) {
            (*id)->process(_curTickPos, _nextTickPos);
            }

      MidiInPortList* il = song->midiInPorts();
      for (iMidiInPort id = il->begin(); id != il->end(); ++id) {
            MidiInPort* port = *id;

            //
            // process routing to synti
            //
            RouteList* rl = port->outRoutes();
            for (iRoute i = rl->begin(); i != rl->end(); ++i) {
                  if (i->track->type() != Track::AUDIO_SOFTSYNTH)
                        continue;
                  SynthI* s = (SynthI*)(i->track);
                  MPEventList* dl = s->playEvents();
                  port->getEvents(0, 0, -1, dl);
                  }
            port->afterProcess();
            }
      midiBusy = false;
      }

