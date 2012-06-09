//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: midi.cpp,v 1.43.2.22 2009/11/09 20:28:28 terminator356 Exp $
//
//  (C) Copyright 1999/2004 Werner Schweer (ws@seh.de)
//  (C) Copyright 2011 Tim E. Real (terminator356 on users dot sourceforge dot net)
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

#include <cmath>
#include <errno.h>

#include "song.h"
#include "midi.h"
#include "drummap.h"
#include "event.h"
#include "globals.h"
#include "midictrl.h"
#include "marker/marker.h"
#include "midiport.h"
#include "midictrl.h"
#include "sync.h"
#include "audio.h"
#include "mididev.h"
#include "driver/alsamidi.h"
#include "driver/jackmidi.h"
#include "wave.h"
#include "synth.h"
#include "sync.h"
#include "midiseq.h"
#include "gconfig.h"
#include "ticksynth.h"

namespace MusECore {

extern void dump(const unsigned char* p, int n);


const unsigned char gmOnMsg[]   = { 0x7e, 0x7f, 0x09, 0x01 };
const unsigned char gsOnMsg[]   = { 0x41, 0x10, 0x42, 0x12, 0x40, 0x00, 0x7f, 0x00, 0x41 };
const unsigned char gsOnMsg2[]  = { 0x41, 0x10, 0x42, 0x12, 0x40, 0x01, 0x33, 0x50, 0x3c };
const unsigned char gsOnMsg3[]  = { 0x41, 0x10, 0x42, 0x12, 0x40, 0x01, 0x34, 0x50, 0x3b };
const unsigned char xgOnMsg[]   = { 0x43, 0x10, 0x4c, 0x00, 0x00, 0x7e, 0x00 };
const unsigned int  gmOnMsgLen  = sizeof(gmOnMsg);
const unsigned int  gsOnMsgLen  = sizeof(gsOnMsg);
const unsigned int  gsOnMsg2Len = sizeof(gsOnMsg2);
const unsigned int  gsOnMsg3Len = sizeof(gsOnMsg3);
const unsigned int  xgOnMsgLen  = sizeof(xgOnMsg);

const unsigned char mmcDeferredPlayMsg[] = { 0x7f, 0x7f, 0x06, 0x03 };
const unsigned char mmcStopMsg[] =         { 0x7f, 0x7f, 0x06, 0x01 };
const unsigned char mmcLocateMsg[] =       { 0x7f, 0x7f, 0x06, 0x44, 0x06, 0x01, 0, 0, 0, 0, 0 };

const unsigned int  mmcDeferredPlayMsgLen = sizeof(mmcDeferredPlayMsg);
const unsigned int  mmcStopMsgLen = sizeof(mmcStopMsg);
const unsigned int  mmcLocateMsgLen = sizeof(mmcLocateMsg);

#define CALC_TICK(the_tick) lrintf((float(the_tick) * float(MusEGlobal::config.division) + float(div/2)) / float(div));
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
      if(len == 0)
        return s;
      switch(buf[0]) {
            case 0x00:
                  if(len < 3)
                    return s;
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
            case MUSE_SYNTH_SYSEX_MFG_ID:  s = "MusE Soft Synth"; break;     // p4.0.27
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
//      generally: how to handle incomplete messages
//---------------------------------------------------------

void buildMidiEventList(EventList* del, const MPEventList* el, MidiTrack* track,
   int div, bool addSysexMeta, bool doLoops)
      {
      int hbank    = 0xff;
      int lbank    = 0xff;
      int rpnh     = -1;
      int rpnl     = -1;
      int datah    = 0;
      int datal    = 0;
      int dataType = 0;   // 0 : disabled, 0x20000 : rpn, 0x30000 : nrpn

      EventList mel;

      for (iMPEvent i = el->begin(); i != el->end(); ++i) {
            MidiPlayEvent ev = *i;
            if (!addSysexMeta && (ev.type() == ME_SYSEX || ev.type() == ME_META))
                  continue;
            if (!(ev.type() == ME_SYSEX || ev.type() == ME_META
               || ((ev.channel() == track->outChannel()) && (ev.port() == track->outPort()))))
                  continue;
            unsigned tick = ev.time();
            
            if(doLoops)
            {
              if(tick >= MusEGlobal::song->lPos().tick() && tick < MusEGlobal::song->rPos().tick())
              {
                int loopn = ev.loopNum();
                int loopc = MusEGlobal::audio->loopCount();
                int cmode = MusEGlobal::song->cycleMode(); // CYCLE_NORMAL, CYCLE_MIX, CYCLE_REPLACE
                // If we want REPLACE and the event was recorded in a previous loop, 
                //  just ignore it. This will effectively ignore ALL previous loop events inside
                //  the left and right markers, regardless of where recording was started or stopped.
                // We want to keep any loop 0 note-offs from notes which crossed over the left marker. 
                // To avoid more searching here, just keep ALL note-offs from loop 0, and let code below 
                //  sort out and keep which ones had note-ons.
                if(!(ev.isNoteOff() && loopn == 0))
                {
                  if(cmode == Song::CYCLE_REPLACE && loopn < loopc)
                    continue;

                  // If we want NORMAL, same as REPLACE except keep all events from the previous loop
                  //  from rec stop position to right marker (and beyond).
                  if(cmode == Song::CYCLE_NORMAL)
                  {
                    // Not sure of accuracy here. Adjust? Adjusted when used elsewhere?
                    unsigned endRec = MusEGlobal::audio->getEndRecordPos().tick();
                    if((tick < endRec && loopn < loopc) || (tick >= endRec && loopn < (loopc - 1)))
                      continue;
                  } 
                }  
              }
            }
            
            Event e;
            switch(ev.type()) {
                  case ME_NOTEON:
                        e.setType(Note);

                        if (track->type() == Track::DRUM) {
                              int instr = MusEGlobal::drumInmap[ev.dataA()];
                              e.setPitch(instr);
                              }
                        else
                        {
                              e.setPitch(ev.dataA());
                        }
                        
                        e.setVelo(ev.dataB());
                        e.setLenTick(0);
                        break;
                  case ME_NOTEOFF:
                        e.setType(Note);
                        if (track->type() == Track::DRUM) {
                              int instr = MusEGlobal::drumInmap[ev.dataA()];
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
                                          MidiPlayEvent ev = *ii;
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
                                             datah, datal, tick, track->outChannel());
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
                                    int ctl = ev.dataA();
                                    e.setA(ctl);
                                    
                                    if(track->type() == Track::DRUM) 
                                    {
                                      // Is it a drum controller event, according to the track port's instrument?
                                      MidiController *mc = MusEGlobal::midiPorts[track->outPort()].drumController(ctl);
                                      if(mc)
                                        // Store an index into the drum map.
                                        e.setA((ctl & ~0xff) | MusEGlobal::drumInmap[ctl & 0x7f]);
                                    }
                                          
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
                              case 0x03: // Sequence-/TrackName
                                    track->setName(QString((char*)data));
                                    break;
                              case 0x6:   // Marker
                                    {
                                    unsigned ltick  = CALC_TICK(tick);
                                    MusEGlobal::song->addMarker(QString((const char*)(data)), ltick, false);
                                    }
                                    break;
                              case 0x5:   // Lyrics
                              case 0x8:   // text
                              case 0x9:
                              case 0xa:
                                    break;

                              case 0x0f:        // Track Comment
                                    track->setComment(QString((char*)data));
                                    break;
                              case 0x51:        // Tempo
                                    {
                                    unsigned tempo = data[2] + (data[1] << 8) + (data[0] <<16);
                                    unsigned ltick  = CALC_TICK(tick);
                                    // FIXME: After ca 10 mins 32 bits will not be enough... This expression has to be changed/factorized or so in some "sane" way...
                                    MusEGlobal::tempomap.addTempo(ltick, tempo);
                                    }
                                    break;
                              case 0x58:        // Time Signature
                                    {
                                    int timesig_z = data[0];
                                    int n = data[1];
                                    int timesig_n = 1;
                                    for (int i = 0; i < n; i++)
                                          timesig_n *= 2;
                                    int ltick  = CALC_TICK(tick);
                                    AL::sigmap.add(ltick, AL::TimeSignature(timesig_z, timesig_n));
                                    }
                                    break;
                              case 0x59:  // Key Signature
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
      //    read NoteOn events and remove corresponding NoteOffs
      //---------------------------------------------------

        for (iEvent i = mel.begin(); i != mel.end(); ++i) {
              Event ev  = i->second;
              if (ev.isNote()) {
                    if (!ev.isNoteOff()) {
                    
                    // If the event length is not zero, it means the event and its 
                    //  note on/off have already been taken care of. So ignore it.
                    if(ev.lenTick() != 0)
                      continue;
                    
                    iEvent k;
                    for (k = mel.lower_bound(ev.tick()); k != mel.end(); ++k) {
                          Event event = k->second;
                          if (ev.isNoteOff(event)) {
                                int t = k->first - i->first;
                                if (t <= 0) {
                                      if (MusEGlobal::debugMsg) {
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
                          printf("-no note-off! %d pitch %d velo %d\n",
                            ev.tick(), ev.pitch(), ev.velo());
                          //
                          // switch off at end of measure
                          //
                          int endTick = MusEGlobal::song->roundUpBar(ev.tick()+1);
                          ev.setLenTick(endTick-ev.tick());
                          }
                    else {
                          if (k==i) 
                            //this will never happen, because i->second has to be a NOTE ON,
                            //while k has to be a NOTE OFF. but in case something changes:
                            printf("ERROR: THIS SHOULD NEVER HAPPEN: k==i in midi.cpp:buildMidiEventList()\n");
                          else
                            mel.erase(k);
                            i = mel.begin();   // p4.0.34
                          continue;
                          }
                    }
									}
              }

      
      for (iEvent i = mel.begin(); i != mel.end(); ++i) {
            Event ev  = i->second;
            if (ev.isNoteOff()) {
                  printf("+extra note-off! %d pitch %d velo %d\n",
                           i->first, ev.pitch(), ev.velo());
                  continue;
                  }
            int tick  = CALC_TICK(ev.tick());
            if (ev.isNote()) {
                  int lenTick = CALC_TICK(ev.lenTick());
                  ev.setLenTick(lenTick);
                  }
            ev.setTick(tick);
            del->add(ev);
            }
      }

} // namespace MusECore

namespace MusECore {

//---------------------------------------------------------
//   midiPortsChanged
//---------------------------------------------------------

void Audio::midiPortsChanged()
      {
      write(sigFd, "P", 1);
      }

//---------------------------------------------------------
//   sendLocalOff
//---------------------------------------------------------

void Audio::sendLocalOff()
      {
      for (int k = 0; k < MIDI_PORTS; ++k) {
            for (int i = 0; i < MIDI_CHANNELS; ++i)
                  MusEGlobal::midiPorts[k].sendEvent(MusECore::MidiPlayEvent(0, k, i, MusECore::ME_CONTROLLER, MusECore::CTRL_LOCAL_OFF, 0), true);
            }
      }

//---------------------------------------------------------
//   panic
//---------------------------------------------------------

void Audio::panic()
      {
      for (int i = 0; i < MIDI_PORTS; ++i) {
            MusECore::MidiPort* port = &MusEGlobal::midiPorts[i];
            if (port == 0)   // ??
                  continue;
            for (int chan = 0; chan < MIDI_CHANNELS; ++chan) {
                  if (MusEGlobal::debugMsg)
                    printf("send all sound of to midi port %d channel %d\n", i, chan);
                  port->sendEvent(MusECore::MidiPlayEvent(0, i, chan, MusECore::ME_CONTROLLER, MusECore::CTRL_ALL_SOUNDS_OFF, 0), true);
                  port->sendEvent(MusECore::MidiPlayEvent(0, i, chan, MusECore::ME_CONTROLLER, MusECore::CTRL_RESET_ALL_CTRL, 0), true);
                  }
            }
      }

//---------------------------------------------------------
//   initDevices
//    - called on seek to position 0
//    - called from arranger pulldown menu
//---------------------------------------------------------

void Audio::initDevices()
      {
      //
      // mark all used ports
      //
      bool activePorts[MIDI_PORTS];
      for (int i = 0; i < MIDI_PORTS; ++i)
            activePorts[i] = false;

      MusECore::MidiTrackList* tracks = MusEGlobal::song->midis();
      for (MusECore::iMidiTrack it = tracks->begin(); it != tracks->end(); ++it) {
            MusECore::MidiTrack* track = *it;
            activePorts[track->outPort()] = true;
            }
      if (MusEGlobal::song->click())
            activePorts[MusEGlobal::clickPort] = true;

      //
      // test for explicit instrument initialization
      //

      for (int i = 0; i < MIDI_PORTS; ++i) {
            if (!activePorts[i])
                  continue;

            MusECore::MidiPort* port        = &MusEGlobal::midiPorts[i];
            MusECore::MidiInstrument* instr = port->instrument();
            MidiDevice* md        = port->device();

            if (instr && md) {
                  EventList* events = instr->midiInit();
                  if (events->empty())
                        continue;
                  for (iEvent ie = events->begin(); ie != events->end(); ++ie) {
                        MusECore::MidiPlayEvent ev(0, i, 0, ie->second);
                        md->putEvent(ev);
                        }
                  activePorts[i] = false;  // no standard initialization
                  }
            }
      //
      // First all ports are initialized to GM and then are changed
      // to XG/GS in order to prevent that devices with more than one
      // port, e.g. Korg NS5R, toggle between GM and XG/GS several times.
      //
      // Standard initialization...
      for (int i = 0; i < MIDI_PORTS; ++i) {
            if (!activePorts[i])
                  continue;
            MusECore::MidiPort* port = &MusEGlobal::midiPorts[i];
            switch(MusEGlobal::song->mtype()) {
                  case MT_GS:
                  case MT_UNKNOWN:
                        break;
                  case MT_GM:
                  case MT_XG:
                        port->sendGmOn();
                        break;
                  }
            }
      for (int i = 0; i < MIDI_PORTS; ++i) {
            if (!activePorts[i])
                  continue;
            MusECore::MidiPort* port = &MusEGlobal::midiPorts[i];
            switch(MusEGlobal::song->mtype()) {
                  case MT_UNKNOWN:
                        break;
                  case MT_GM:
                        port->sendGmInitValues();
                        break;
                  case MT_GS:
                        port->sendGsOn();
                        port->sendGsInitValues();
                        break;
                  case MT_XG:
                        port->sendXgOn();
                        port->sendXgInitValues();
                        break;
                  }
            }
      }

//---------------------------------------------------------
//   collectEvents
//    collect events for next audio segment
//---------------------------------------------------------

void Audio::collectEvents(MusECore::MidiTrack* track, unsigned int cts, unsigned int nts)
      {
      int port    = track->outPort();
      int channel = track->outChannel();
      int defaultPort = port;

      MidiDevice* md          = MusEGlobal::midiPorts[port].device();

      PartList* pl = track->parts();
      for (iPart p = pl->begin(); p != pl->end(); ++p) {
            MusECore::MidiPart* part = (MusECore::MidiPart*)(p->second);
            // dont play muted parts
            if (part->mute())
                  continue;
            EventList* events = part->events();
            unsigned partTick = part->tick();
            unsigned partLen  = part->lenTick();
            int delay         = track->delay;

            if (cts > nts) {
                  printf("processMidi: FATAL: cur > next %d > %d\n",
                     cts, nts);
                  return;
                  }
            unsigned offset = delay + partTick;
            if (offset > nts)
                  continue;
            unsigned stick = (offset > cts) ? 0 : cts - offset;
            unsigned etick = nts - offset;
            // Do not play events which are past the end of this part. 
            if(etick > partLen)
              continue;
              
            iEvent ie   = events->lower_bound(stick);
            iEvent iend = events->lower_bound(etick);

            for (; ie != iend; ++ie) {
                  Event ev = ie->second;
                  port = defaultPort; //Reset each loop
                  //
                  //  dont play any meta events
                  //
                  if (ev.type() == Meta)
                        continue;
                  if (track->type() == Track::DRUM) {
                        int instr = ev.pitch();
                        // ignore muted drums
                        if (ev.isNote() && MusEGlobal::drumMap[instr].mute)
                              continue;
                        }
                  unsigned tick  = ev.tick() + offset;
                  unsigned frame = MusEGlobal::tempomap.tick2frame(tick) + frameOffset;
                  switch (ev.type()) {
                        case Note:
                              {
                              int len   = ev.lenTick();
                              int pitch = ev.pitch();
                              int velo  = ev.velo();
                              if (track->type() == Track::DRUM)  {
                                    //
                                    // Map drum-notes to the drum-map values
                                    //
                                   int instr = ev.pitch();
                                   pitch     = MusEGlobal::drumMap[instr].anote;
                                   port      = MusEGlobal::drumMap[instr].port; //This changes to non-default port
                                   channel   = MusEGlobal::drumMap[instr].channel;
                                   velo      = int(double(velo) * (double(MusEGlobal::drumMap[instr].vol) / 100.0)) ;
                                   }
                              else {
                                    //
                                    // transpose non drum notes
                                    //
                                    pitch += (track->transposition + MusEGlobal::song->globalPitchShift());
                                    }

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
                              if (len <= 0)     // dont allow zero length
                                    len = 1;
                              int veloOff = ev.veloOff();

                              if (port == defaultPort) {
                                    // If syncing to external midi sync, we cannot use the tempo map.
                                    // Therefore we cannot get sub-tick resolution. Just use ticks instead of frames. p3.3.25
                                    if(MusEGlobal::extSyncFlag.value())
                                      md->addScheduledEvent(MusECore::MidiPlayEvent(tick, port, channel, MusECore::ME_NOTEON, pitch, velo));
                                    else
                                      md->addScheduledEvent(MusECore::MidiPlayEvent(frame, port, channel, MusECore::ME_NOTEON, pitch, velo));
                                      
                                    md->addStuckNote(MusECore::MidiPlayEvent(tick + len, port, channel,
                                       veloOff ? MusECore::ME_NOTEOFF : MusECore::ME_NOTEON, pitch, veloOff));   
                                    }
                              else { //Handle events to different port than standard.
                                    MidiDevice* mdAlt = MusEGlobal::midiPorts[port].device();
                                    if (mdAlt) {
                                        if(MusEGlobal::extSyncFlag.value())  // p3.3.25
                                          mdAlt->addScheduledEvent(MusECore::MidiPlayEvent(tick, port, channel, MusECore::ME_NOTEON, pitch, velo));                                          
                                        else  
                                          mdAlt->addScheduledEvent(MusECore::MidiPlayEvent(frame, port, channel, MusECore::ME_NOTEON, pitch, velo));                                          
                                          
                                        mdAlt->addStuckNote(MusECore::MidiPlayEvent(tick + len, port, channel,
                                          veloOff ? MusECore::ME_NOTEOFF : MusECore::ME_NOTEON, pitch, veloOff));
                                      }
                                    }
                              
                              if(velo > track->activity())
                                track->setActivity(velo);
                              }
                              break;

                        case Controller:
                              {
                                if (track->type() == Track::DRUM)  
                                {
                                  int ctl   = ev.dataA();
                                  // Is it a drum controller event, according to the track port's instrument?
                                  MusECore::MidiController *mc = MusEGlobal::midiPorts[defaultPort].drumController(ctl);
                                  if(mc)
                                  {
                                    int instr = ctl & 0x7f;
                                    ctl &=  ~0xff;
                                    int pitch = MusEGlobal::drumMap[instr].anote & 0x7f;
                                    port      = MusEGlobal::drumMap[instr].port; //This changes to non-default port
                                    channel   = MusEGlobal::drumMap[instr].channel;
                                    MidiDevice* mdAlt = MusEGlobal::midiPorts[port].device();
                                    if(mdAlt) 
                                    {
                                      // If syncing to external midi sync, we cannot use the tempo map.
                                      // Therefore we cannot get sub-tick resolution. Just use ticks instead of frames. p3.3.25
                                      if(MusEGlobal::extSyncFlag.value())
                                        mdAlt->addScheduledEvent(MusECore::MidiPlayEvent(tick, port, channel, 
                                                                             MusECore::ME_CONTROLLER, ctl | pitch, ev.dataB()));
                                      else
                                        mdAlt->addScheduledEvent(MusECore::MidiPlayEvent(frame, port, channel, 
                                                                             MusECore::ME_CONTROLLER, ctl | pitch, ev.dataB()));
                                    }  
                                    break;
                                  }  
                                }
                                if(MusEGlobal::extSyncFlag.value())  // p3.3.25
                                  md->addScheduledEvent(MusECore::MidiPlayEvent(tick, port, channel, ev));
                                else  
                                  md->addScheduledEvent(MusECore::MidiPlayEvent(frame, port, channel, ev));
                              }     
                              break;
                        
                        default:
                              if(MusEGlobal::extSyncFlag.value())  // p3.3.25
                                md->addScheduledEvent(MusECore::MidiPlayEvent(tick, port, channel, ev));
                              else
                                md->addScheduledEvent(MusECore::MidiPlayEvent(frame, port, channel, ev));
                                
                              break;
                        }
                  }
            }
      }

//---------------------------------------------------------
//   processMidi
//    - collects midi events for current audio segment and
//       sends them to midi thread
//    - current audio segment position is (curTickPos, nextTickPos)
//    - called from midiseq thread,
//      executed in audio thread
//---------------------------------------------------------

void Audio::processMidi()
      {
      MusEGlobal::midiBusy=true;
      
      bool extsync = MusEGlobal::extSyncFlag.value();
      
      //
      // TODO: syntis should directly write into recordEventList
      //
      for (iMidiDevice id = MusEGlobal::midiDevices.begin(); id != MusEGlobal::midiDevices.end(); ++id) 
      {
        MidiDevice* md = *id;

        // klumsy hack for MESS synti devices:
        if(md->isSynti())
        {
          SynthI* s = (SynthI*)md;
          while (s->eventsPending()) 
          {
            MusECore::MidiRecordEvent ev = s->receiveEvent();
            md->recordEvent(ev);
          }
        }
        
        md->collectMidiEvents();
        
        // Take snapshots of the current sizes of the recording fifos, 
        //  because they may change while here in process, asynchronously.
        md->beforeProcess();
        
        //
        // --------- Handle midi events for audio tracks -----------
        // 
        
        int port = md->midiPort(); // Port should be same as event.port() from this device. Same idea event.channel().
        if(port < 0)
          continue;
        
        for(int chan = 0; chan < MIDI_CHANNELS; ++chan)     
        {
          MusECore::MidiRecFifo& rf = md->recordEvents(chan);
          int count = md->tmpRecordCount(chan);
          for(int i = 0; i < count; ++i) 
          {
            MusECore::MidiRecordEvent event(rf.peek(i));

            int etype = event.type();
            if(etype == MusECore::ME_CONTROLLER || etype == MusECore::ME_PITCHBEND || etype == MusECore::ME_PROGRAM)
            {
              int ctl, val;
              if(etype == MusECore::ME_CONTROLLER)
              {
                ctl = event.dataA();
                val = event.dataB();
              }
              else if(etype == MusECore::ME_PITCHBEND)
              {
                ctl = MusECore::CTRL_PITCH;
                val = event.dataA();
              }
              else if(etype == MusECore::ME_PROGRAM)
              {
                ctl = MusECore::CTRL_PROGRAM;
                val = event.dataA();
              }
              
              // Midi learn! 
              MusEGlobal::midiLearnPort = port;
              MusEGlobal::midiLearnChan = chan;
              MusEGlobal::midiLearnCtrl = ctl;
              
              // Send to audio tracks...
              for (MusECore::iTrack t = MusEGlobal::song->tracks()->begin(); t != MusEGlobal::song->tracks()->end(); ++t) 
              {
                if((*t)->isMidiTrack())
                  continue;
                MusECore::AudioTrack* track = static_cast<MusECore::AudioTrack*>(*t);
                MidiAudioCtrlMap* macm = track->controller()->midiControls();
                int h = macm->index_hash(port, chan, ctl);
                std::pair<ciMidiAudioCtrlMap, ciMidiAudioCtrlMap> range = macm->equal_range(h);
                for(ciMidiAudioCtrlMap imacm = range.first; imacm != range.second; ++imacm)
                {
                  const MidiAudioCtrlStruct* macs = &imacm->second;
                  int actrl = macs->audioCtrlId();
                  
                  iCtrlList icl = track->controller()->find(actrl); 
                  if(icl == track->controller()->end())
                    continue;
                  CtrlList* cl = icl->second;
                  double dval = midi2AudioCtrlValue(cl, macs, ctl, val);
                  
                  // Time here needs to be frames always. 
                  unsigned int ev_t = event.time();
                  unsigned int t = ev_t;
                  
#ifdef _AUDIO_USE_TRUE_FRAME_
                  unsigned int pframe = _previousPos.frame();
#else
                  unsigned int pframe = _pos.frame();
#endif
                  if(pframe > t)  // Technically that's an error, shouldn't happen
                    t = 0;
                  else
                    // Subtract the current audio position frame
                    t -= pframe;  
                  
                  // Add the current running sync frame to make the control processing happy
                  t += syncFrame;
                  track->addScheduledControlEvent(actrl, dval, t);
                    
                  // Rec automation...

                  // For the record time, if stopped we don't want the circular running position,
                  //  just the static one.
                  unsigned int rec_t = isPlaying() ? ev_t : pframe;
                  
                  if(!MusEGlobal::automation)
                    continue;
                  AutomationType at = track->automationType();
                  // Unlike our built-in gui controls, there is not much choice here but to 
                  //  just do this:
                  if ( (at == AUTO_WRITE) ||
                       (at == AUTO_TOUCH && MusEGlobal::audio->isPlaying()) )
                  //if(isPlaying() && (at == AUTO_WRITE || at == AUTO_TOUCH)) DELETETHIS
                    track->enableController(actrl, false);
                  if(isPlaying())
                  {
                    if(at == AUTO_WRITE || at == AUTO_TOUCH)
                      track->recEvents()->push_back(CtrlRecVal(rec_t, actrl, dval));      
                  }
                  else 
                  {
                    if(at == AUTO_WRITE)
                      track->recEvents()->push_back(CtrlRecVal(rec_t, actrl, dval));    
                    else if(at == AUTO_TOUCH)
                      // In touch mode and not playing. Send directly to controller list.
                      // Add will replace if found.
                      cl->add(rec_t, dval);     
                  }
                }
              }  
            }
          }   
        }
      }

      for (MusECore::iMidiTrack t = MusEGlobal::song->midis()->begin(); t != MusEGlobal::song->midis()->end(); ++t) 
      {
            MusECore::MidiTrack* track = *t;
            int port = track->outPort();
            MidiDevice* md = MusEGlobal::midiPorts[port].device();
            if(md)
            {
              // only add track events if the track is unmuted
              if(!track->isMute()) 
              {
                if(isPlaying() && (curTickPos < nextTickPos))
                  collectEvents(track, curTickPos, nextTickPos);
              }
            }  

            //
            //----------midi recording
            //
            if (track->recordFlag()) 
            {
                  MusECore::MPEventList* rl = track->mpevents();
                  MusECore::MidiPort* tport = &MusEGlobal::midiPorts[port];
                  RouteList* irl = track->inRoutes();
                  for(ciRoute r = irl->begin(); r != irl->end(); ++r)
                  {
                        if(!r->isValid() || (r->type != Route::MIDI_PORT_ROUTE))   // p3.3.49
                          continue;
                        int devport = r->midiPort;                                 // 
                        if (devport == -1)
                          continue;
                        MidiDevice* dev = MusEGlobal::midiPorts[devport].device();             // 
                        if(!dev)
                          continue;
                        int channelMask = r->channel;   
                        if(channelMask == -1 || channelMask == 0)
                          continue;
                        for(int channel = 0; channel < MIDI_CHANNELS; ++channel)     
                        {
                          if(!(channelMask & (1 << channel)))
                            continue;
                          if(!dev->sysexFIFOProcessed())
                          {
                            // Set to the sysex fifo at first.
                            MusECore::MidiRecFifo& rf = dev->recordEvents(MIDI_CHANNELS);
                            // Get the frozen snapshot of the size.
                            int count = dev->tmpRecordCount(MIDI_CHANNELS);
                          
                            for(int i = 0; i < count; ++i) 
                            {
                              MusECore::MidiRecordEvent event(rf.peek(i));
                              event.setPort(port);
                              // dont't echo controller changes back to software
                              // synthesizer:
                              if(!dev->isSynti() && md && track->recEcho())
                              {
                                // All recorded events arrived in the previous period. Shift into this period for playback.
                                unsigned int et = event.time();
#ifdef _AUDIO_USE_TRUE_FRAME_
                                unsigned int t = et - _previousPos.frame() + _pos.frame() + frameOffset;
#else                                
                                unsigned int t = et + frameOffset;
#endif
                                event.setTime(t);
                                md->addScheduledEvent(event);
                                event.setTime(et);  // Restore for recording.
                              }
                              
                              // Make sure the event is recorded in units of ticks.  
                              if(extsync)  
                                event.setTime(event.tick());  // HACK: Transfer the tick to the frame time
                              else
                                event.setTime(MusEGlobal::tempomap.frame2tick(event.time()));
                                
                              if(recording) 
                                rl->add(event);
                            }      
                            dev->setSysexFIFOProcessed(true);
                          }
                          
                          MusECore::MidiRecFifo& rf = dev->recordEvents(channel);
                          int count = dev->tmpRecordCount(channel);
                          for(int i = 0; i < count; ++i) 
                          {
                                MusECore::MidiRecordEvent event(rf.peek(i));
                                int defaultPort = devport;
                                int drumRecPitch=0; //prevent compiler warning: variable used without initialization
                                MusECore::MidiController *mc = 0;
                                int ctl = 0;
                                
                                //Hmmm, hehhh... 
                                // TODO: Clean up a bit around here when it comes to separate events for rec & for playback. 
                                // But not before 0.7 (ml)
  
                                int prePitch = 0, preVelo = 0;
  
                                event.setChannel(track->outChannel());
                                
                                if (event.isNote() || event.isNoteOff()) 
                                {
                                      //
                                      // apply track values
                                      //
  
                                      //Apply drum inkey:
                                      if (track->type() == Track::DRUM) 
                                      {
                                            int pitch = event.dataA();
                                            //Map note that is played according to MusEGlobal::drumInmap
                                            drumRecPitch = MusEGlobal::drumMap[(unsigned int)MusEGlobal::drumInmap[pitch]].enote;
                                            devport = MusEGlobal::drumMap[(unsigned int)MusEGlobal::drumInmap[pitch]].port;
                                            event.setPort(devport);
                                            channel = MusEGlobal::drumMap[(unsigned int)MusEGlobal::drumInmap[pitch]].channel;
                                            event.setA(MusEGlobal::drumMap[(unsigned int)MusEGlobal::drumInmap[pitch]].anote);
                                            event.setChannel(channel);
                                      }
                                      else 
                                      { //Track transpose if non-drum
                                            prePitch = event.dataA();
                                            int pitch = prePitch + track->transposition;
                                            if (pitch > 127)
                                                  pitch = 127;
                                            if (pitch < 0)
                                                  pitch = 0;
                                            event.setA(pitch);
                                      }
  
                                      if (!event.isNoteOff()) 
                                      {
                                            preVelo = event.dataB();
                                            int velo = preVelo + track->velocity;
                                            velo = (velo * track->compression) / 100;
                                            if (velo > 127)
                                                  velo = 127;
                                            if (velo < 1)
                                                  velo = 1;
                                            event.setB(velo);
                                      }
                                }
                                else
                                if(event.type() == MusECore::ME_CONTROLLER)
                                {
                                  if(track->type() == Track::DRUM) 
                                  {
                                    ctl = event.dataA();
                                    // Regardless of what port the event came from, is it a drum controller event 
                                    //  according to the track port's instrument?
                                    mc = tport->drumController(ctl);
                                    if(mc)
                                    {
                                      int pitch = ctl & 0x7f;
                                      ctl &= ~0xff;
                                      int dmindex = MusEGlobal::drumInmap[pitch] & 0x7f;
                                      //Map note that is played according to MusEGlobal::drumInmap
                                      drumRecPitch = MusEGlobal::drumMap[dmindex].enote;
                                      devport = MusEGlobal::drumMap[dmindex].port;
                                      event.setPort(devport);
                                      channel = MusEGlobal::drumMap[dmindex].channel;
                                      event.setA(ctl | MusEGlobal::drumMap[dmindex].anote);
                                      event.setChannel(channel);
                                    }  
                                  }
                                }
                                
                                // MusE uses a fixed clocks per quarternote of 24. 
                                // At standard 384 ticks per quarternote for example, 
                                // 384/24=16 for a division of 16 sub-frames (16 MusE 'ticks').
                                // If ext sync, events are now time-stamped with last tick in MidiDevice::recordEvent(). p3.3.35
                                // TODO: Tested, but record resolution not so good. Switch to wall clock based separate list in MidiDevice.
  
                                // dont't echo controller changes back to software
                                // synthesizer:
  
                                if (!dev->isSynti()) 
                                {
                                  // All recorded events arrived in previous period. Shift into this period for playback. 
                                  //  frameoffset needed to make process happy.
                                  unsigned int et = event.time();
#ifdef _AUDIO_USE_TRUE_FRAME_
                                  unsigned int t = et - _previousPos.frame() + _pos.frame() + frameOffset;
#else
                                  unsigned int t = et + frameOffset;
#endif
                                  event.setTime(t);  
                                  // Check if we're outputting to another port than default:
                                  if (devport == defaultPort) {
                                        event.setPort(port);
                                        if(md && track->recEcho())
                                          md->addScheduledEvent(event);
                                        }
                                  else {
                                        // Hmm, this appears to work, but... Will this induce trouble with md->setNextPlayEvent??
                                        MidiDevice* mdAlt = MusEGlobal::midiPorts[devport].device();
                                        if(mdAlt && track->recEcho())
                                          mdAlt->addScheduledEvent(event);
                                        }
                                  event.setTime(et);  // Restore for recording.
                                  
                                  // Shall we activate meters even while rec echo is off? Sure, why not...
                                  if(event.isNote() && event.dataB() > track->activity())
                                    track->setActivity(event.dataB());
                                }
                                
                                // Make sure the event is recorded in units of ticks.  
                                if(extsync)  
                                  event.setTime(event.tick());  // HACK: Transfer the tick to the frame time
                                else
                                  event.setTime(MusEGlobal::tempomap.frame2tick(event.time()));
  
                                // Special handling of events stored in rec-lists. a bit hACKish. TODO: Clean up (after 0.7)! :-/ (ml)
                                if (recording) 
                                {
                                      // In these next steps, it is essential to set the recorded event's port 
                                      //  to the track port so buildMidiEventList will accept it. Even though 
                                      //  the port may have no device "<none>".
                                      //
                                      if (track->type() == Track::DRUM) 
                                      {
                                        // Is it a drum controller event?
                                        if(mc)
                                        {    
                                            MusECore::MidiPlayEvent drumRecEvent = event;
                                            drumRecEvent.setA(ctl | drumRecPitch);
                                            // In this case, preVelo is simply the controller value.
                                            drumRecEvent.setB(preVelo);
                                            drumRecEvent.setPort(port); //rec-event to current port
                                            drumRecEvent.setChannel(track->outChannel()); //rec-event to current channel
                                            rl->add(drumRecEvent);
                                        }
                                        else
                                        {
                                            MusECore::MidiPlayEvent drumRecEvent = event;
                                            drumRecEvent.setA(drumRecPitch);
                                            drumRecEvent.setB(preVelo);
                                            // Tested: Events were not being recorded for a drum map entry pointing to a 
                                            //  different port. This must have been wrong - buildMidiEventList would ignore this. Tim.
                                            drumRecEvent.setPort(port);  //rec-event to current port
                                            
                                            drumRecEvent.setChannel(track->outChannel()); //rec-event to current channel
                                            rl->add(drumRecEvent);
                                        }    
                                      }
                                      else 
                                      {
                                            // Restore record-pitch to non-transposed value since we don't want the note transposed twice next
                                            MusECore::MidiPlayEvent recEvent = event;
                                            if (prePitch)
                                                  recEvent.setA(prePitch);
                                            if (preVelo)
                                                  recEvent.setB(preVelo);
                                            recEvent.setPort(port);
                                            recEvent.setChannel(track->outChannel());
                                                  
                                            rl->add(recEvent);
                                      }
                                }
                            }
                        }
                  }
            }
      }

      //---------------------------------------------------
      //    insert metronome clicks
      //---------------------------------------------------

      MidiDevice* md = 0;
      if (MusEGlobal::midiClickFlag)
            md = MusEGlobal::midiPorts[MusEGlobal::clickPort].device();
      if (MusEGlobal::song->click() && (isPlaying() || state == PRECOUNT)) {
            int bar, beat;
            unsigned tick;
            bool isMeasure = false;
            while (midiClick < nextTickPos) {
                  if (isPlaying()) {
                        AL::sigmap.tickValues(midiClick, &bar, &beat, &tick);
                        isMeasure = beat == 0;
                        }
                  else if (state == PRECOUNT) {
                        isMeasure = (clickno % clicksMeasure) == 0;
                        }
                  int evtime = extsync ? midiClick : MusEGlobal::tempomap.tick2frame(midiClick) + frameOffset;  // p3.3.25
                  
                  if (md) {
                        MusECore::MidiPlayEvent ev(evtime, MusEGlobal::clickPort, MusEGlobal::clickChan, MusECore::ME_NOTEON,
                           MusEGlobal::beatClickNote, MusEGlobal::beatClickVelo);
                        
                        if (isMeasure) {
                              ev.setA(MusEGlobal::measureClickNote);
                              ev.setB(MusEGlobal::measureClickVelo);
                              }
                        md->addScheduledEvent(ev);
                        
                        ev.setB(0);
                        ev.setTime(midiClick+10);
                        md->addStuckNote(ev);
                        }
                  if (MusEGlobal::audioClickFlag) {
                        MusECore::MidiPlayEvent ev(evtime, 0, 0, MusECore::ME_NOTEON, 0, 0);
                        ev.setA(isMeasure ? 0 : 1);
                        metronome->addScheduledEvent(ev);
                        // Built-in metronome synth does not use stuck notes...
                        }
                  if (isPlaying())
                        midiClick = AL::sigmap.bar2tick(bar, beat+1, 0);
                  else if (state == PRECOUNT) {
                        midiClick += ticksBeat;
                        if (clickno)
                              --clickno;
                        else
                              state = START_PLAY;
                        }
                  }
            }
      
      //
      // Play all midi events up to curFrame.
      //
      for(iMidiDevice id = MusEGlobal::midiDevices.begin(); id != MusEGlobal::midiDevices.end(); ++id) 
      {
        // We are done with the 'frozen' recording fifos, remove the events. 
        (*id)->afterProcess();   // p4.0.34
        
        // ALSA devices handled by another thread.
        if((*id)->deviceType() != MidiDevice::ALSA_MIDI)
          (*id)->processMidi();
      }
      MusEGlobal::midiBusy=false;
      }

} // namespace MusECore
