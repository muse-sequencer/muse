//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: midi.cpp,v 1.43.2.22 2009/11/09 20:28:28 terminator356 Exp $
//
//  (C) Copyright 1999/2004 Werner Schweer (ws@seh.de)
//  (C) Copyright 2011-2016 Tim E. Real (terminator356 on users dot sourceforge dot net)
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

#include "muse_math.h"
#include <errno.h>

#include "song.h"
#include "midi.h"
#include "drummap.h"
#include "event.h"
#include "globals.h"
#include "midictrl.h"
#include "marker/marker.h"
#include "midiport.h"
#include "minstrument.h"
#include "midictrl.h"
#include "sync.h"
#include "audio.h"
#include "audiodev.h"
#include "mididev.h"
#include "driver/alsamidi.h"
#include "driver/jackmidi.h"
#include "wave.h"
#include "synth.h"
#include "sync.h"
#include "midiseq.h"
#include "gconfig.h"
#include "ticksynth.h"
#include "mpevent.h"
#include "metronome_class.h"

// REMOVE Tim. Persistent routes. Added. Make this permanent later if it works OK and makes good sense.
#define _USE_MIDI_ROUTE_PER_CHANNEL_

// Undefine if and when multiple output routes are added to midi tracks.
#define _USE_MIDI_TRACK_SINGLE_OUT_PORT_CHAN_

// For debugging output: Uncomment the fprintf section.
#define DEBUG_MIDI(dev, format, args...) // fprintf(dev, format, ##args);
// For debugging metronome and precount output: Uncomment the fprintf section.
#define DEBUG_MIDI_METRONOME(dev, format, args...) // fprintf(dev, format, ##args);
// For debugging midi timing: Uncomment the fprintf section.
#define DEBUG_MIDI_TIMING(dev, format, args...) // fprintf(dev, format, ##args);
// For debugging midi event time differences.
//#define DEBUG_MIDI_TIMING_DIFFS ;

namespace MusECore {

#ifdef DEBUG_MIDI_TIMING_DIFFS
// For testing.
unsigned _lastEvTime = 0;
#endif

  
extern void dump(const unsigned char* p, int n);

#define CALC_TICK(the_tick) lrintf((float(the_tick) * float(MusEGlobal::config.division) + float(div/2)) / float(div));

/*---------------------------------------------------------
 *    midi_meta_name
 *---------------------------------------------------------*/

QString midiMetaName(int meta)
      {
      const char* s = "";
      switch (meta) {
            case 0:     s = "Text 0: Sequence Number"; break;
            case 1:     s = "Text 1: Track comment"; break;
            case 2:     s = "Text 2: Copyright"; break;
            case 3:     s = "Text 3: Sequence/Track Name"; break;
            case 4:     s = "Text 4: Instrument Name"; break;
            case 5:     s = "Text 5: Lyric"; break;
            case 6:     s = "Text 6: Marker"; break;
            case 7:     s = "Text 7: Cue Point"; break;
            case 8:     s = "Text 8"; break;
            case 9:     s = "Text 9: Device Name"; break;
            case 0x0a:  s = "Text A"; break;
            case 0x0b:  s = "Text B"; break;
            case 0x0c:  s = "Text C"; break;
            case 0x0d:  s = "Text D"; break;
            case 0x0e:  s = "Text E"; break;
            case 0x0f:  s = "Text F"; break;
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

QString nameSysex(unsigned int len, const unsigned char* buf, MidiInstrument* instr)
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
            case 0x01:  s = "Sequential Circuits"; break;
            case 0x02:  s = "Big Briar"; break;
            case 0x03:  s = "Octave / Plateau"; break;
            case 0x04:  s = "Moog"; break;
            case 0x05:  s = "Passport Designs"; break;
            case 0x06:  s = "Lexicon"; break;
            case 0x07:  s = "Kurzweil"; break;
            case 0x08:  s = "Fender"; break;
            case 0x09:  s = "Gulbransen"; break;
            case 0x0a:  s = "Delta Labas"; break;
            case 0x0b:  s = "Sound Comp."; break;
            case 0x0c:  s = "General Electro"; break;
            case 0x0d:  s = "Techmar"; break;
            case 0x0e:  s = "Matthews Research"; break;
            case 0x10:  s = "Oberheim"; break;
            case 0x11:  s = "PAIA"; break;
            case 0x12:  s = "Simmons"; break;
            case 0x13:  s = "DigiDesign"; break;
            case 0x14:  s = "Fairlight"; break;
            case 0x15:  s = "JL Cooper"; break;
            case 0x16:  s = "Lowery"; break;
            case 0x17:  s = "Lin"; break;
            case 0x18:  s = "Emu"; break;
            case 0x1b:  s = "Peavy"; break;
            case 0x20:  s = "Bon Tempi"; break;
            case 0x21:  s = "S.I.E.L"; break;
            case 0x23:  s = "SyntheAxe"; break;
            case 0x24:  s = "Hohner"; break;
            case 0x25:  s = "Crumar"; break;
            case 0x26:  s = "Solton"; break;
            case 0x27:  s = "Jellinghaus Ms"; break;
            case 0x28:  s = "CTS"; break;
            case 0x29:  s = "PPG"; break;
            case 0x2f:  s = "Elka"; break;
            case 0x36:  s = "Cheetah"; break;
            case 0x3e:  s = "Waldorf"; break;
            case 0x40:  s = "Kawai"; break;
            case 0x41:  s = "Roland"; break;
            case 0x42:  s = "Korg"; break;
            case 0x43:  s = "Yamaha"; break;
            case 0x44:  s = "Casio"; break;
            case 0x45:  s = "Akai"; break;
            case MUSE_SYNTH_SYSEX_MFG_ID:  s = "MusE Soft Synth"; break;
            case 0x7d:  s = "Educational Use"; break;
            case 0x7e:  s = "Universal: Non Real Time"; break;
            case 0x7f:  s = "Universal: Real Time"; break;
            default:    s = "??"; break;
            }

      if(instr)
      {
        // Check for user-defined sysex in instrument...
        foreach(const MusECore::SysEx* sx, instr->sysex())
        {
          if((int)len == sx->dataLen && memcmp(buf, sx->data, len) == 0)
            return s + QString(": ") + sx->name;
        }
      }

      //
      // following messages should not show up in event list
      // they are filtered while importing midi files
      //
      if ((len == gmOnMsgLen) && memcmp(buf, gmOnMsg, gmOnMsgLen) == 0)
            s += ": GM-ON";
      else if ((len == gm2OnMsgLen) && memcmp(buf, gm2OnMsg, gm2OnMsgLen) == 0)
            s += ": GM2-ON";
      else if ((len == gmOffMsgLen) && memcmp(buf, gmOffMsg, gmOffMsgLen) == 0)
            s += ": GM-OFF";
      else if ((len == gsOnMsgLen) && memcmp(buf, gsOnMsg, gsOnMsgLen) == 0)
            s += ": GS-ON";
      else if ((len == xgOnMsgLen) && memcmp(buf, xgOnMsg, xgOnMsgLen) == 0)
            s += ": XG-ON";
      return s;
      }

//---------------------------------------------------------
//   QString sysexComment
//---------------------------------------------------------

QString sysexComment(unsigned int len, const unsigned char* buf, MidiInstrument* instr)
      {
      QString s;
      if(len == 0)
        return s;

      if(instr)
      {
        // Check for user-defined sysex in instrument...
        foreach(const MusECore::SysEx* sx, instr->sysex())
        {
          if((int)len == sx->dataLen && memcmp(buf, sx->data, len) == 0)
            return sx->comment;
        }
      }

      // These are the common ones we know about so far...
      if ((len == gmOnMsgLen) && memcmp(buf, gmOnMsg, gmOnMsgLen) == 0)
            s = QObject::tr("Switch on General Midi Level 1 mode");
      else if ((len == gm2OnMsgLen) && memcmp(buf, gm2OnMsg, gm2OnMsgLen) == 0)
            s = QObject::tr("Switch on General Midi Level 2 mode");
      else if ((len == gmOffMsgLen) && memcmp(buf, gmOffMsg, gmOffMsgLen) == 0)
            s = QObject::tr("Switch off General Midi Level 1 or 2");
      else if ((len == gsOnMsgLen) && memcmp(buf, gsOnMsg, gsOnMsgLen) == 0)
            s = QObject::tr("Switch on Roland GS mode");
      else if ((len == xgOnMsgLen) && memcmp(buf, xgOnMsg, xgOnMsgLen) == 0)
            s = QObject::tr("Switch on Yamaha XG mode");
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

void buildMidiEventList(EventList* del, const MPEventList& el, MidiTrack* track,
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

      MidiInstrument::NoteOffMode nom = MidiInstrument::NoteOffAll;
      MidiPort* mp = 0;
      MidiInstrument* minstr = 0;
      const int port = track->outPort();
      if(port >= 0 && port < MusECore::MIDI_PORTS)
      {
        mp = &MusEGlobal::midiPorts[port];
        minstr = mp->instrument();
        if(minstr)
          nom = minstr->noteOffMode();
      }

      for (iMPEvent i = el.begin(); i != el.end(); ++i) {
            MidiPlayEvent ev = *i;
            if (!addSysexMeta && (ev.type() == ME_SYSEX || ev.type() == ME_META))
                  continue;
            if (!(ev.type() == ME_SYSEX || ev.type() == ME_META
               || ((ev.channel() == track->outChannel()) && (ev.port() == track->outPort()))))
                  continue;
            unsigned tick = ev.time();

            DEBUG_MIDI(stderr, "buildMidiEventList tick:%d dataA:%d dataB:%d\n",
                            ev.time(), ev.dataA(), ev.dataB());
            
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
                        else if (track->type() == Track::NEW_DRUM) {
                              int instr = track->map_drum_in(ev.dataA());
                              e.setPitch(instr);
                              }
                        else
                              e.setPitch(ev.dataA());

                        e.setVelo(ev.dataB());
                        e.setLenTick(0);
                        break;
                  case ME_NOTEOFF:
                        e.setType(Note);
                        if (track->type() == Track::DRUM) {
                              int instr = MusEGlobal::drumInmap[ev.dataA()];
                              e.setPitch(instr);
                              }
                        else if (track->type() == Track::NEW_DRUM) {
                              int instr = track->map_drum_in(ev.dataA());
                              e.setPitch(instr);
                              }
                        else
                              e.setPitch(ev.dataA());

                        e.setVelo(0);
                        e.setVeloOff(ev.dataB());
                        e.setLenTick(0);
                        break;
                  case ME_POLYAFTER:
                        e.setType(Controller);
                        e.setA((CTRL_POLYAFTER & ~0xff) | (ev.dataA() & 0x7f));
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
                                    for (; ii != el.end(); ++ii) {
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
                                                fprintf(stderr, "parameter number not defined, data 0x%x\n", datah);
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
                                          fprintf(stderr, "parameter number not defined, data 0x%x 0x%x, tick %d, channel %d\n",
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
                                    else if(track->type() == Track::NEW_DRUM)
                                    {
                                      // Is it a drum controller event, according to the track port's instrument?
                                      MidiController *mc = MusEGlobal::midiPorts[track->outPort()].drumController(ctl);
                                      if(mc)
                                        // Store an index into the drum map.
                                        e.setA((ctl & ~0xff) | track->map_drum_in(ctl & 0x7f));
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
                        e.setType(Controller);
                        e.setA(CTRL_AFTERTOUCH);
                        e.setB(ev.dataA());
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
                              case ME_META_TEXT_1_COMMENT:
                                    if (track->comment().isEmpty())
                                          track->setComment(QString((const char*)data));
                                    else
                                          track->setComment(track->comment() + "\n" + QString((const char*)data));
                                    break;
                              case ME_META_TEXT_3_TRACK_NAME: // Sequence-/TrackName
                                    track->setName(QString((char*)data));
                                    break;
                              case ME_META_TEXT_6_MARKER:
                                    {
                                    unsigned ltick  = CALC_TICK(tick);
                                    MusEGlobal::song->addMarker(QString((const char*)(data)), ltick, false);
                                    }
                                    break;
                              // Copyright is supposed to occur only at the beginning of the first track, but we don't
                              //  specifically catch it yet during import, so let's just allow it 'wherever' for now.
                              case ME_META_TEXT_2_COPYRIGHT:
                              // Lyrics are allowed anywhere.
                              case ME_META_TEXT_5_LYRIC:
                              // Cue points are supposed to occur only in the first track, but we don't support them
                              //  yet (need a list just like markers), so just allow them 'wherever' for now.
                              case ME_META_TEXT_7_CUE_POINT:
                              // Program name is allowed anywhere.
                              case ME_META_TEXT_8_PROGRAM_NAME:
                              // No documentation could be found for these, so just allow them 'wherever' for now.
                              case ME_META_TEXT_A:
                              case ME_META_TEXT_B:
                              case ME_META_TEXT_C:
                              case ME_META_TEXT_D:
                              case ME_META_TEXT_E:
                              // We don't specifically support key signature metas yet, so just allow them 'wherever' for now.
                              case ME_META_KEY_SIGNATURE:
                                    e.setType(Meta);
                                    e.setA(ev.dataA());
                                    e.setData(ev.data(), ev.len());
                                    break;
                              // Instrument and device name metas are already handled by the midi importing code.
                              case ME_META_TEXT_4_INSTRUMENT_NAME:
                              case ME_META_TEXT_9_DEVICE_NAME:
                                    break;

                              case ME_META_TEXT_F_TRACK_COMMENT:
                                    track->setComment(QString((char*)data));
                                    break;
                              case ME_META_SET_TEMPO:
                                    {
                                    unsigned tempo = data[2] + (data[1] << 8) + (data[0] <<16);
                                    unsigned ltick  = CALC_TICK(tick);
                                    // FIXME: After ca 10 mins 32 bits will not be enough... This expression has to be changed/factorized or so in some "sane" way...
                                    MusEGlobal::tempomap.addTempo(ltick, tempo);
                                    }
                                    break;
                              case ME_META_TIME_SIGNATURE:
                                    {
                                    int timesig_z = data[0];
                                    int n = data[1];
                                    int timesig_n = 1;
                                    for (int i = 0; i < n; i++)
                                          timesig_n *= 2;
                                    int ltick  = CALC_TICK(tick);
                                    MusEGlobal::sigmap.add(ltick, MusECore::TimeSignature(timesig_z, timesig_n));
                                    }
                                    break;
                              default:
                                    fprintf(stderr, "buildMidiEventList: unknown Meta 0x%x %d unabsorbed, adding instead to track:%s\n",
                                            ev.dataA(), ev.dataA(), track->name().toLatin1().constData());
                                    e.setType(Meta);
                                    e.setA(ev.dataA());
                                    e.setData(ev.data(), ev.len());
                                    break;
                              }
                        }
                        break;
                  }   // switch(ev.type()
            if (!e.empty()) {
                  e.setTick(tick);

                  //-------------------------------------------
                  //    Check for and prevent duplicate events
                  //-------------------------------------------

                  const int midi_evtype = ev.type();
                  const bool midi_noteoff = (midi_evtype == ME_NOTEOFF) || (midi_evtype == ME_NOTEON && ev.dataB() == 0);
                  const bool midi_noteon = midi_evtype == ME_NOTEON && ev.dataB() != 0;
                  const bool midi_controller = midi_evtype == ME_CONTROLLER;
                  bool noteon_found = false;
                  bool noteoff_found = false;
                  bool ctrlval_found = false;
                  //bool other_ctrlval_found = false;
                  if(i != el.begin())
                  {
                    iMPEvent k = i;
                    while(k != el.begin())
                    {
                      --k;
                      MidiPlayEvent k_ev = *k;
                      if(k_ev.channel() != ev.channel() || k_ev.port() != ev.port())
                        continue;
                      const int check_midi_evtype = k_ev.type();
                      const bool check_midi_noteoff = (check_midi_evtype == ME_NOTEOFF) || (check_midi_evtype == ME_NOTEON && k_ev.dataB() == 0);
                      const bool check_midi_noteon = check_midi_evtype == ME_NOTEON && k_ev.dataB() != 0;
                      const bool check_midi_controller = check_midi_evtype == ME_CONTROLLER;
                      if(midi_noteon || midi_noteoff)
                      {
                        if(ev.dataA() == k_ev.dataA())  // Note
                        {
                          if(check_midi_noteon)
                          {
                            // Check the instrument's note-off mode: If it does not support note-offs,
                            //  don't bother doing duplicate note-on checks.
                            // This allows drum input triggers (no note offs at all), although it is awkward to
                            //  first have to choose an output instrument with no note-off mode.
                            if(!midi_noteon || (nom != MidiInstrument::NoteOffNone))
                              noteon_found = true;
                            break;
                          }
                          if(check_midi_noteoff)
                          {
                            noteoff_found = true;
                            break;
                          }
                        }
                      }
                      else if(midi_controller)
                      {
                        if(ev.dataA() == k_ev.dataA())     // Controller number
                        {
                          if(check_midi_controller)
                          {
                            // All we can really do is prevent multiple events at the same time.
                            // We must allow multiple events at different times having the same value,
                            //  since the sender may have wanted it that way (a 'flat' graph).
                            if(ev.time() == k_ev.time())     // Event time
                              ctrlval_found = true;
                            // Optimization: Do not allow multiple events at different times having the same value.
                            // Nice, but can't really discard these, sender may have wanted it that way (a 'flat' graph).
                          #if 0
                            if(ev.dataB() == k_ev.dataB()) // Controller value
                              ctrlval_found = true;
                            else
                              other_ctrlval_found = true;
                          #endif
                            break;
                          }
                        }
                      }
                      else
                      {
                        // TODO: Other types!
                      }
                    }
                  }
                  // Accept the event only if no duplicate was found. // TODO: Other types!
                  if((midi_noteon && !noteon_found) ||
                    (midi_noteoff && !noteoff_found) ||
                    //(midi_controller && (other_ctrlval_found || !ctrlval_found)))
                    (midi_controller && !ctrlval_found) ||
                    // Accept any other type of event.
                    (!midi_noteon && !midi_noteoff && !midi_controller) )
                    mel.add(e);
                  }
            }  // i != el.end()


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
                                            fprintf(stderr, "Note len is (%d-%d)=%d, set to 1\n",
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
                          fprintf(stderr, "-no note-off! %d pitch %d velo %d\n",
                            ev.tick(), ev.pitch(), ev.velo());
                          //
                          // switch off at end of measure
                          //
                          int endTick = MusEGlobal::song->roundUpBar(ev.tick()+1);
                          ev.setLenTick(endTick-ev.tick());
                          }
                    else {
                          if (k==i) {
                            //this will never happen, because i->second has to be a NOTE ON,
                            //while k has to be a NOTE OFF. but in case something changes:
                            fprintf(stderr, "ERROR: THIS SHOULD NEVER HAPPEN: k==i in midi.cpp:buildMidiEventList()\n");
                          }
                          else {
                            mel.erase(k);
                          }
                            i = mel.begin();
                          continue;
                          }
                    }
                                    }
              }

      for (iEvent i = mel.begin(); i != mel.end(); ++i) {
            Event ev  = i->second;
            if (ev.isNoteOff()) {
                  fprintf(stderr, "+extra note-off! %d pitch %d velo %d\n",
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
//   Can be called by any thread.
//---------------------------------------------------------

void Audio::sendLocalOff()
      {
      MidiPlayEvent ev;
      ev.setTime(0);  // Immediate processing. TODO Use curFrame?
      ev.setType(MusECore::ME_CONTROLLER);
      ev.setA(MusECore::CTRL_LOCAL_OFF);
      ev.setB(0);
      for (int k = 0; k < MusECore::MIDI_PORTS; ++k) {
            for (int i = 0; i < MusECore::MUSE_MIDI_CHANNELS; ++i)
            {
                  ev.setPort(k);
                  ev.setChannel(i);
                  // This is a 'trigger' event. Send to the device, but do not send to the
                  //  midi port controllers because it leaves them in this state.
                  if(MusEGlobal::midiPorts[k].device())
                    MusEGlobal::midiPorts[k].device()->putEvent(ev, MidiDevice::NotLate);
            }
            }
      }

//---------------------------------------------------------
//   panic
//   Can be called by any thread.
//---------------------------------------------------------

void Audio::panic()
      {
      MidiPlayEvent ev;
      ev.setTime(0);  // Immediate processing. TODO Use curFrame?
      ev.setType(MusECore::ME_CONTROLLER);
      ev.setB(0);

      // TODO Reset those controllers back to unknown!
      for (int i = 0; i < MusECore::MIDI_PORTS; ++i) {
            MusECore::MidiPort* port = &MusEGlobal::midiPorts[i];
            for (int chan = 0; chan < MusECore::MUSE_MIDI_CHANNELS; ++chan) {
                  if (MusEGlobal::debugMsg)
                    fprintf(stderr, "send all sound of to midi port %d channel %d\n", i, chan);
                  
                  ev.setPort(i);
                  ev.setChannel(chan);

                  ev.setA(MusECore::CTRL_ALL_SOUNDS_OFF);
                  // This is a 'trigger' event. Send to the device, but do not send to the
                  //  midi port controllers because it leaves them in this state.
                  //port->putHwCtrlEvent(ev);
                  if(port->device())
                    port->device()->putEvent(ev, MidiDevice::NotLate);
                  
                  ev.setA(MusECore::CTRL_RESET_ALL_CTRL);
                  // This is a 'trigger' event. Send to the device, but do not send to the
                  //  midi port controllers because it leaves them in this state.
                  //port->putHwCtrlEvent(ev);
                  if(port->device())
                    port->device()->putEvent(ev, MidiDevice::NotLate);
                  }
            }
      }

//---------------------------------------------------------
//   initDevices
//    - called when instrument init sequences plus controller
//       defaults should be checked and/or sent
//    - called from arranger pulldown menu
//---------------------------------------------------------

void Audio::initDevices(bool force)
      {
      for (int i = 0; i < MusECore::MIDI_PORTS; ++i) {
            MusEGlobal::midiPorts[i].sendPendingInitializations(force);
            }
      }

//---------------------------------------------------------
//   seekMidi
//   Called from audio thread only.
//---------------------------------------------------------

void Audio::seekMidi()
{
  MusECore::MetronomeSettings* metro_settings = 
    MusEGlobal::metroUseSongSettings ? &MusEGlobal::metroSongSettings : &MusEGlobal::metroGlobalSettings;

  unsigned int pos = MusEGlobal::audio->tickPos();
  const bool playing = isPlaying();
  
  // Bit-wise channels that are used.
  int used_ports[MusECore::MIDI_PORTS];
  // Initialize the array.
  for(int i = 0; i < MusECore::MIDI_PORTS; ++i)
    used_ports[i] = 0;

  // Find all used channels on all used ports.
  bool drum_found = false;
  if(MusEGlobal::song->click() && 
     metro_settings->clickPort < MusECore::MIDI_PORTS &&
     metro_settings->clickChan < MusECore::MUSE_MIDI_CHANNELS)
    used_ports[metro_settings->clickPort] |= (1 << metro_settings->clickChan);
  MidiTrackList* tl = MusEGlobal::song->midis();
  for(ciMidiTrack imt = tl->begin(); imt != tl->end(); ++imt)
  {
    MidiTrack* mt = *imt;
    
    //------------------------------------------------------------
    //    While we are at it, flush out any track-related playback stuck notes
    //     (NOT 'live' notes) which were not put directly to the device
    //------------------------------------------------------------
    MPEventList& mel = mt->stuckNotes;
    for(iMPEvent i = mel.begin(), i_next = i; i != mel.end(); i = i_next)
    {
      ++i_next;

      MidiPlayEvent ev(*i);
      const int ev_port = ev.port();
      if(ev_port >= 0 && ev_port < MusECore::MIDI_PORTS)
      {
        MidiPort* mp = &MusEGlobal::midiPorts[ev_port];
        ev.setTime(0);  // Immediate processing. TODO Use curFrame?
        if(mp->device())
          mp->device()->putEvent(ev, MidiDevice::NotLate);
      }
      mel.erase(i);
    }

    
#ifdef _USE_MIDI_TRACK_SINGLE_OUT_PORT_CHAN_
    
    if(mt->type() == MusECore::Track::DRUM)
    {
      if(!drum_found)
      {
        drum_found = true; 
        for(int i = 0; i < DRUM_MAPSIZE; ++i)
        {
          // Default to track port if -1 and track channel if -1.
          int mport = MusEGlobal::drumMap[i].port;
          if(mport == -1)
            mport = mt->outPort();
          int mchan = MusEGlobal::drumMap[i].channel;
          if(mchan == -1)
            mchan = mt->outChannel();
          if(mport >= 0 && mport < MusECore::MIDI_PORTS && mchan >= 0 && mchan < MusECore::MUSE_MIDI_CHANNELS)
            used_ports[mport] |= (1 << mchan);
        }
      }
    }
    else
    {
        const int mport = mt->outPort();
        const int mchan = mt->outChannel();
        if(mport >= 0 && mport < MusECore::MIDI_PORTS && mchan >= 0 && mchan < MusECore::MUSE_MIDI_CHANNELS)
          used_ports[mport] |= (1 << mchan);
    }
    
#else
    MusECore::RouteList* rl = mt->outRoutes();
    for(MusECore::ciRoute ir = rl->begin(); ir != rl->end(); ++ir)
    {
      switch(ir->type)
      {
        case MusECore::Route::MIDI_PORT_ROUTE:
        {
          if(mt->type() == MusECore::Track::DRUM)
          {
            if(!drum_found)
            {
              drum_found = true; 
              for(int i = 0; i < DRUM_MAPSIZE; ++i)
              {
                // Default to track port if -1 and track channel if -1.
                int mport = MusEGlobal::drumMap[i].port;
                if(mport == -1)
                  mport = ir->midiPort;
                int mchan = MusEGlobal::drumMap[i].channel;
                if(mchan == -1)
                  mchan = ir->channel;
                if(mport >= 0 && mport < MIDI_PORTS && mchan >= 0 && mchan < MusECore::MUSE_MIDI_CHANNELS)
                  used_ports[mport] |= (1 << mchan);
              }
            }
          }
          else
          {
              const int mport = ir->midiPort;
              const int mchan = ir->channel;
              if(mport >= 0 && mport < MIDI_PORTS && mchan >= 0 && mchan < MusECore::MUSE_MIDI_CHANNELS)
                used_ports[mport] |= (1 << mchan);
          }
        }
        break;  
        
        case MusECore::Route::TRACK_ROUTE:
        case MusECore::Route::JACK_ROUTE:
        case MusECore::Route::MIDI_DEVICE_ROUTE:
        break;  
      }
    }
#endif
  }
  
  for(int i = 0; i < MusECore::MIDI_PORTS; ++i)
  {
    if(used_ports[i] == 0)
      continue;
      
    MidiPort* mp = &MusEGlobal::midiPorts[i];
    MidiDevice* md = mp->device();
    
    //---------------------------------------------------
    //    Send STOP 
    //---------------------------------------------------
      
    // Don't send if external sync is on. The master, and our sync routing system will take care of that.  
    if(!MusEGlobal::extSyncFlag)
    {
      if(mp->syncInfo().MRTOut())
      {
        // Shall we check for device write open flag to see if it's ok to send?...
        //if(!(rwFlags() & 0x1) || !(openFlags() & 1))
        //if(!(openFlags() & 1))
        //  continue;
        mp->sendStop();
      }    
    }
    
    //---------------------------------------------------
    //    If playing, clear all notes and flush out any
    //     stuck notes which were put directly to the device
    //---------------------------------------------------
    
    if(md && playing)
      md->handleSeek();
    
    //---------------------------------------------------
    //    reset sustain
    //---------------------------------------------------
    
    if(md)
    {
      for(int ch = 0; ch < MusECore::MUSE_MIDI_CHANNELS; ++ch) 
      {
        if(mp->hwCtrlState(ch, CTRL_SUSTAIN) == 127) 
        {
          const MidiPlayEvent ev(0, i, ch, ME_CONTROLLER, CTRL_SUSTAIN, 0);
          md->putEvent(ev, MidiDevice::NotLate);
        }
      }
    }
    
    MidiInstrument* instr = mp->instrument();
    MidiCtrlValListList* cll = mp->controller();

    for(iMidiCtrlValList ivl = cll->begin(); ivl != cll->end(); ++ivl) 
    {
      MidiCtrlValList* vl = ivl->second;
      int chan = ivl->first >> 24;
      if(!(used_ports[i] & (1 << chan)))  // Channel not used in song?
        continue;
      int ctlnum = vl->num();

      // Find the first non-muted value at the given tick...
      bool values_found = false;
      bool found_value = false;
      
      iMidiCtrlVal imcv = vl->lower_bound(pos);
      if(imcv != vl->end() && imcv->first == pos)
      {
        for( ; imcv != vl->end() && imcv->first == pos; ++imcv)
        {
          const Part* p = imcv->second.part;
          if(!p)
            continue;
          // Ignore values that are outside of the part.
          if(pos < p->tick() || pos >= (p->tick() + p->lenTick()))
            continue;
          values_found = true;
          // Ignore if part or track is muted or off.
          if(p->mute())
            continue;
          const Track* track = p->track();
          if(track && (track->isMute() || track->off()))
            continue;
          found_value = true;
          break;
        }
      }
      else
      {
        while(imcv != vl->begin())
        {
          --imcv;
          const Part* p = imcv->second.part;
          if(!p)
            continue;
          // Ignore values that are outside of the part.
          unsigned t = imcv->first;
          if(t < p->tick() || t >= (p->tick() + p->lenTick()))
            continue;
          values_found = true;
          // Ignore if part or track is muted or off.
          if(p->mute())
            continue;
          const Track* track = p->track();
          if(track && (track->isMute() || track->off()))
            continue;
          found_value = true;
          break;
        }
      }

      if(found_value)
      {
        int fin_port = i;
        MidiPort* fin_mp = mp;
        int fin_chan = chan;
        int fin_ctlnum = ctlnum;
        // Is it a drum controller event, according to the track port's instrument?
        if(mp->drumController(ctlnum))
        {
          if(const Part* p = imcv->second.part)
          {
            if(Track* t = p->track())
            {
              if(t->type() == MusECore::Track::NEW_DRUM)
              {
                MidiTrack* mt = static_cast<MidiTrack*>(t);
                int v_idx = ctlnum & 0x7f;
                fin_ctlnum = (ctlnum & ~0xff) | mt->drummap()[v_idx].anote;
                int map_port = mt->drummap()[v_idx].port;
                if(map_port != -1)
                {
                  fin_port = map_port;
                  fin_mp = &MusEGlobal::midiPorts[fin_port];
                }
                int map_chan = mt->drummap()[v_idx].channel;
                if(map_chan != -1)
                  fin_chan = map_chan;
              }
            }
          }
        }

        const MidiPlayEvent ev(0, fin_port, fin_chan, ME_CONTROLLER, fin_ctlnum, imcv->second.val);
        // This is the audio thread. Just set directly.
        fin_mp->setHwCtrlState(ev);
        // Don't bother sending any sustain values to the device, because we already
        //  just sent out zero sustain values, above. Just set the hw state.
        // When play resumes, the correct values are sent again if necessary in Audio::startRolling().
        if(fin_ctlnum != CTRL_SUSTAIN && fin_mp->device())
          fin_mp->device()->putEvent(ev, MidiDevice::NotLate);
      }

      // Either no value was found, or they were outside parts, or pos is in the unknown area before the first value.
      // Send instrument default initial values.  NOT for syntis. Use midiState and/or initParams for that. 
      //if((imcv == vl->end() || !done) && !MusEGlobal::song->record() && instr && !isSynti()) 
      // Hmm, without refinement we can only do this at position 0, due to possible 'skipped' values outside parts, above.
      if(instr && md && !md->isSynti() && !values_found && 
         MusEGlobal::config.midiSendCtlDefaults && !MusEGlobal::song->record() && pos == 0)
      {
        MidiControllerList* mcl = instr->controller();
        ciMidiController imc = mcl->find(vl->num());
        if(imc != mcl->end())
        {
          MidiController* mc = imc->second;
          if(mc->initVal() != CTRL_VAL_UNKNOWN)
          {
            //fprintf(stderr, "Audio::seekMidi: !values_found: calling sendEvent: ctlnum:%d val:%d\n", ctlnum, mc->initVal() + mc->bias());
            // Use sendEvent to get the optimizations and limiting. No force sending. Note the addition of bias.
            const MidiPlayEvent ev(0, i, chan, ME_CONTROLLER, ctlnum, mc->initVal() + mc->bias());
            // This is the audio thread. Just set directly.
            mp->setHwCtrlState(ev);
            md->putEvent(ev, MidiDevice::NotLate);
          }
        }
      }
      
      //---------------------------------------------------
      //    Send STOP and "set song position pointer"
      //---------------------------------------------------
        
      // Don't send if external sync is on. The master, and our sync routing system will take care of that.  
      if(!MusEGlobal::extSyncFlag)
      {
        if(mp->syncInfo().MRTOut())
        {
          int beat = (pos * 4) / MusEGlobal::config.division;
          mp->sendSongpos(beat);
        }    
      }
    }
  }
}

//---------------------------------------------------------
//   extClockHistoryTick2Frame
//    Convert tick to frame using the external clock history list.
//    The function takes a tick relative to zero (ie. relative to the first event in a processing batch).
//    The returned clock frames occurred during the previous audio cycle(s), so you may want to shift 
//     the frames forward by one audio segment size for scheduling purposes.
//    CAUTION: There must be at least one valid clock in the history,
//              otherwise it returns zero. Don't feed this a tick 
//              greater than or equal to the next tick, it will simply return
//              the very last frame, which is not very useful since 
//              that will just bunch the events together at the last frame.
//---------------------------------------------------------

unsigned int Audio::extClockHistoryTick2Frame(unsigned int tick) const
{
  if(_extClockHistorySize == 0)
  {
    fprintf(stderr, "Error: Audio::extClockTickToFrame(): empty list\n");
    return 0;
  }
  
  const int div = MusEGlobal::config.division / 24;
  if(div == 0) 
    return 0; // Prevent divide by zero.
    
  int index = tick / div;
  if(index >= _extClockHistorySize)
  {
    fprintf(stderr, "Error: Audio::extClockTickToFrame(): index:%d >= size:%d\n", index, _extClockHistorySize);
    index = _extClockHistorySize - 1;
  }

// Divide the clock period by the division and interpolate for even better resolution.
// FIXME: Darn, too bad we can't use this. It would work, but the previous cycle 
//         has no knowledge of what to put at the end, and the current cycle 
//         would end up lumping together events at the start which should have 
//         been played at end of previous cycle.
//   const unsigned int subtick = tick % div;
//   const unsigned int frame = _extClockLastFrame + double(_extClockHistory[index] - _extClockLastFrame) * (double(subtick) / double(div));
  const unsigned int frame = _extClockHistory[index].frame();
  
  return frame;
}

//---------------------------------------------------------
//   extClockHistoryTick2Frame
//    Convert frame to tick using the external clock history list.
//    The function takes an absolute linearly increasing frame and returns a tick relative to zero 
//     (ie. relative to the first event in a processing batch).
//    CAUTION: There must be at least one valid clock in the history,
//              otherwise it returns zero. Don't feed this a frame 
//              greater than or equal to the next frame, it will simply return
//              the very last tick, which is not very useful since 
//              that will just bunch the events together at the last tick.
//---------------------------------------------------------

unsigned int Audio::extClockHistoryFrame2Tick(unsigned int frame) const
{
  if(_extClockHistorySize == 0)
  {
    fprintf(stderr, "Error: Audio::extClockHistoryFrame2Tick(): empty list\n");
    return curTickPos;
  }
  
  const unsigned int div = MusEGlobal::config.division / 24;
    
  bool found = false;
  unsigned int val = 0;
  
  for(int i = _extClockHistorySize - 1; i >= 0; --i)
  {
    DEBUG_MIDI(stderr, "Audio::extClockHistoryFrame2Tick(): frame:%u i:%d _extClockHistory[i]._frame:%u\n", 
            frame, i, _extClockHistory[i].frame());
    
    if(_extClockHistory[i].frame() <= frame)
    {
      if(!found)
      {
        found = true;
        int clocks = 0;
        unsigned int offset = curTickPos;
        
        for(int k = i; k >= 0; --k)
        {
          if(_extClockHistory[k].isFirstClock())
          {
            if(_extClockHistory[k].externState() == ExtMidiClock::ExternStarted)
              offset = 0;
          }
          
          if(!_extClockHistory[k].isPlaying())
            break;
          
          if(k < i)  // Ignore first clock.
            ++clocks;
        }
        
        val = offset + clocks * div;
      }
    }
  }
  if(found)
    return val;
  
  fprintf(stderr, "Error: Audio::extClockHistoryFrame2Tick(): frame:%u out of range. Returning zero. _extClockHistorySize:%u\n", 
          frame, _extClockHistorySize);
  
  // We don't know the state of the last clock, we can only assume it was playing.
  if(curTickPos >= div)
    return curTickPos - div;
  
  return curTickPos;
}

//---------------------------------------------------------
//   collectEvents
//    collect events for next audio segment
//---------------------------------------------------------

void Audio::collectEvents(MusECore::MidiTrack* track, unsigned int cts,
                          unsigned int nts, unsigned int frames, unsigned int latency_offset)
      {
      DEBUG_MIDI_TIMING(stderr, "Audio::collectEvents: cts:%u nts:%u\n", cts, nts);
      const bool extsync = MusEGlobal::extSyncFlag;
      const int delay = track->delay;
      // If external sync is not on, we can take advantage of frame accuracy but
      //  first we must allow the next tick position to be included in the search
      //  even if it is equal to the current tick position.
      if((extsync && cts >= nts) ||
         (!extsync && cts > nts))
        return;
        
      int port    = track->outPort();
      int channel = track->outChannel();
      int defaultPort = port;
      MidiPort* mp = &MusEGlobal::midiPorts[port];
      MidiDevice* md = mp->device();

      const unsigned int pos_fr = _pos.frame() + latency_offset;
      const unsigned int next_pos_fr = pos_fr + frames;
      
      DEBUG_MIDI_TIMING(stderr, "Audio::collectEvents: pos_fr:%u next_pos_fr:%u\n", pos_fr, next_pos_fr);
      
      PartList* pl = track->parts();
      for (iPart p = pl->begin(); p != pl->end(); ++p) {
            MusECore::MidiPart* part = (MusECore::MidiPart*)(p->second);
            // don't play muted parts
            if (part->mute())
                  continue;
            const EventList& events = part->events();
            unsigned partTick = part->tick();
            unsigned partLen  = part->lenTick();
            unsigned offset = delay + partTick;
            if (offset > nts)
                  continue;
            unsigned stick = (offset > cts) ? 0 : cts - offset;
            unsigned etick = nts - offset;
            // Do not play events which are past the end of this part.
            if(etick > partLen)
              continue;

            // The start and end tick are a rough range to make the loop faster instead of having
            //  to iterate the whole list each time, comparing frames.
            // Use upper_bound because we need to include the 'next' last item because it may have a 
            //  fractional tick component that we would otherwise miss with lower_bound. The loop will
            //  decide whether to process iterated items or not by precisely comparing frames.
            // We rely on the tempo value being 'stable' during the process period - that is
            //  no user changes in-between cycles. We don't have that capability currently anyway -
            //  to break the process up into chunks (like our controllers) depending on tempo frames, 
            //  our tempo map is not frame-accurate, only tick-accurate.
            ciEvent ie   = events.lower_bound(stick);
            ciEvent iend = events.upper_bound(etick);

            DEBUG_MIDI_TIMING(stderr, "Audio::collectEvents: part events stick:%u etick:%u\n", stick, etick);
            
            for (; ie != iend; ++ie) {
                  Event ev = ie->second;
                  port = defaultPort; //Reset each loop
                  //
                  //  don't play any meta events
                  //
                  if (ev.type() == Meta)
                        continue;
                  if (track->type() == Track::DRUM) {
                        int instr = ev.pitch();
                        // ignore muted drums
                        if (ev.isNote() && MusEGlobal::drumMap[instr].mute)
                              continue;
                        }
                  else if (track->type() == Track::NEW_DRUM) {
                        int instr = ev.pitch();
                        // ignore muted drums
                        if (ev.isNote() && track->drummap()[instr].mute)
                              continue;
                        }
                  unsigned tick  = ev.tick() + offset;
                  
                  DEBUG_MIDI_TIMING(stderr, "Audio::collectEvents: event tick:%u\n", tick);
      
                  //-----------------------------------------------------------------
                  // Determining the playback scheduling frame from the event's tick:
                  //-----------------------------------------------------------------
                  unsigned frame;
                  if(extsync)
                    // If external sync is on, look up the scheduling frame from the tick,
                    //  in the external clock history list (which is cleared, re-composed, and processed each cycle).
                    // The function takes a tick relative to zero (ie. relative to the first event in this batch).
                    // The returned clock frame occurred during the previous audio cycle(s), so shift the frame 
                    //  forward by one audio segment size.
                    frame = extClockHistoryTick2Frame(tick - stick) + MusEGlobal::segmentSize;
                  else
                  {
                    // If external sync is off, look up the scheduling frame from our tempo list
                    //  ie. normal playback.
                    const unsigned int fr = MusEGlobal::tempomap.tick2frame(tick);
                    
                    DEBUG_MIDI_TIMING(stderr, "Audio::collectEvents: event: frame:%u\n", fr);
                      
                    // Take advantage of frame-accurate comparison ability here.
                    // At some point, the event's frame time and the 'swept' current range of pos frame will intersect,
                    //  so all events should be accounted for.
                    if(fr < pos_fr || fr >= next_pos_fr)
                    {
                      DEBUG_MIDI_TIMING(stderr, "Audio::collectEvents: Ignoring event\n");
                      continue;
                    }
                    
                    frame = fr - pos_fr;
                    frame += syncFrame;
                  }
                  
                  DEBUG_MIDI(stderr, "Audio::collectEvents: event: tick:%u final frame:%u\n", tick, frame);
                    
                  switch (ev.type()) {
                        case Note:
                              {
                              int len   = ev.lenTick();
                              int pitch = ev.pitch();
                              int velo  = ev.velo();
                              int veloOff = ev.veloOff();
                              if (track->type() == Track::DRUM)  {
                                    // Map drum-notes to the drum-map values
                                   int instr = ev.pitch();
                                   pitch     = MusEGlobal::drumMap[instr].anote;
                                   // Default to track port if -1 and track channel if -1.
                                   port      = MusEGlobal::drumMap[instr].port; //This changes to non-default port
                                   if(port == -1)
                                     port = track->outPort();
                                   channel   = MusEGlobal::drumMap[instr].channel;
                                   if(channel == -1)
                                     channel = track->outChannel();
                                   velo      = int(double(velo) * (double(MusEGlobal::drumMap[instr].vol) / 100.0)) ;
                                   veloOff   = int(double(veloOff) * (double(MusEGlobal::drumMap[instr].vol) / 100.0)) ;
                                   }
                              else if (track->type() == Track::NEW_DRUM)  {
                                    // Map drum-notes to the drum-map values
                                   int instr = ev.pitch();
                                   pitch     = track->drummap()[instr].anote;
                                   // Default to track port if -1 and track channel if -1.
                                   port      = track->drummap()[instr].port; //This changes to non-default port
                                   if(port == -1)
                                     port = track->outPort();
                                   channel   = track->drummap()[instr].channel;
                                   if(channel == -1)
                                     channel = track->outChannel();
                                   velo      = int(double(velo) * (double(track->drummap()[instr].vol) / 100.0)) ;
                                   veloOff   = int(double(veloOff) * (double(track->drummap()[instr].vol) / 100.0)) ;
                                   }
                              else if (track->type() == Track::MIDI) {
                                    // transpose non drum notes
                                    pitch += (track->transposition + MusEGlobal::song->globalPitchShift());
                                    }

                              if (pitch > 127)
                                    pitch = 127;
                              if (pitch < 0)
                                    pitch = 0;

                              // Apply track velocity and compression to both note-on and note-off velocity...
                              velo += track->velocity;
                              velo = (velo * track->compression) / 100;
                              if (velo > 127)
                                    velo = 127;
                              if (velo < 1)           // no off event
                                    // Zero means zero. Should mean no note at all?
                                    //velo = 1;
                                    continue;
                              veloOff += track->velocity;
                              veloOff = (veloOff * track->compression) / 100;
                              if (veloOff > 127)
                                    veloOff = 127;
                              if (veloOff < 1)
                                    veloOff = 0;

                              len = (len *  track->len) / 100;
                              if (len <= 0)     // don't allow zero length
                                    len = 1;

                              if (port == defaultPort) {
                                    if (md) {
                                          md->putEvent(
                                            MusECore::MidiPlayEvent(frame, port, channel, MusECore::ME_NOTEON, pitch, velo), 
                                              MidiDevice::NotLate, MidiDevice::PlaybackBuffer);
                                        track->addStuckNote(MusECore::MidiPlayEvent(tick + len, port, channel,
                                          MusECore::ME_NOTEOFF, pitch, veloOff));
                                      }
                                    }
                              else { //Handle events to different port than standard.
                                    MidiDevice* mdAlt = MusEGlobal::midiPorts[port].device();
                                    if (mdAlt) {
                                          mdAlt->putEvent(
                                            MusECore::MidiPlayEvent(frame, port, channel, MusECore::ME_NOTEON, pitch, velo), 
                                              MidiDevice::NotLate, MidiDevice::PlaybackBuffer);
                                        track->addStuckNote(MusECore::MidiPlayEvent(tick + len, port, channel,
                                          MusECore::ME_NOTEOFF, pitch, veloOff));
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
                                    // Default to track port if -1 and track channel if -1.
                                    port      = MusEGlobal::drumMap[instr].port; //This changes to non-default port
                                    if(port == -1)
                                      port = track->outPort();
                                    channel   = MusEGlobal::drumMap[instr].channel;
                                    if(channel == -1)
                                      channel = track->outChannel();

                                    MusECore::MidiPlayEvent mpeAlt(frame, port, channel, 
                                                                   MusECore::ME_CONTROLLER, 
                                                                   ctl | pitch,
                                                                   ev.dataB());
                                    
                                    MidiPort* mpAlt = &MusEGlobal::midiPorts[port];
                                    // TODO Maybe grab the flag from the 'Optimize Controllers' Global Setting,
                                    //       which so far was meant for (N)RPN stuff. For now, just force it.
                                    // This is the audio thread. Just set directly.
                                    mpAlt->setHwCtrlState(mpeAlt);
                                    if(MidiDevice* mdAlt = mpAlt->device())
                                      mdAlt->putEvent(mpeAlt, MidiDevice::NotLate, MidiDevice::PlaybackBuffer);
                                    
                                    break;  // Break out.
                                  }
                                }
                                else if (track->type() == Track::NEW_DRUM)
                                {
                                  int ctl   = ev.dataA();
                                  // Is it a drum controller event, according to the track port's instrument?
                                  MusECore::MidiController *mc = MusEGlobal::midiPorts[defaultPort].drumController(ctl);
                                  if(mc)
                                  {
                                    int instr = ctl & 0x7f;
                                    ctl &=  ~0xff;
                                    int pitch = track->drummap()[instr].anote & 0x7f;
                                    // Default to track port if -1 and track channel if -1.
                                    port      = track->drummap()[instr].port; //This changes to non-default port
                                    if(port == -1)
                                      port = track->outPort();
                                    channel   = track->drummap()[instr].channel;
                                    if(channel == -1)
                                      channel = track->outChannel();
                                    
                                    MusECore::MidiPlayEvent mpeAlt(frame, port, channel,
                                                                   MusECore::ME_CONTROLLER,
                                                                   ctl | pitch,
                                                                   ev.dataB());
                                    
                                    MidiPort* mpAlt = &MusEGlobal::midiPorts[port];
                                    // TODO Maybe grab the flag from the 'Optimize Controllers' Global Setting,
                                    //       which so far was meant for (N)RPN stuff. For now, just force it.
                                    // This is the audio thread. Just set directly.
                                    mpAlt->setHwCtrlState(mpeAlt);
                                    if(MidiDevice* mdAlt = mpAlt->device())
                                      mdAlt->putEvent(mpeAlt, MidiDevice::NotLate, MidiDevice::PlaybackBuffer);
                                    
                                    break;  // Break out.
                                  }
                                }
                                
                                MusECore::MidiPlayEvent mpe = ev.asMidiPlayEvent(frame, port, channel);
                                // TODO Maybe grab the flag from the 'Optimize Controllers' Global Setting,
                                //       which so far was meant for (N)RPN stuff. For now, just force it.
                                // This is the audio thread. Just set directly.
                                mp->setHwCtrlState(mpe);
                                if(md)
                                  md->putEvent(mpe, MidiDevice::NotLate, MidiDevice::PlaybackBuffer);
                              }
                              break;

                        default:
                          
                              if(md)
                              {
                                 md->putEvent(ev.asMidiPlayEvent(frame, port, channel), 
                                                  MidiDevice::NotLate, MidiDevice::PlaybackBuffer);
                              }
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

void Audio::processMidi(unsigned int frames)
      {
      const bool extsync = MusEGlobal::extSyncFlag;
      const bool playing = isPlaying();

      for (iMidiDevice id = MusEGlobal::midiDevices.begin(); id != MusEGlobal::midiDevices.end(); ++id)
      {
        MidiDevice* md = *id;
        int port = md->midiPort(); // Port should be same as event.port() from this device. Same idea event.channel().

        // Process events sent by synthesizers (which in turn may have been passed by their GUI -> synth FIFOs).
        // Receive events sent from a synth's gui thread (which might be different than our gui thread) to the audio thread.
        if(md->isSynti())
        {
          SynthI* s = (SynthI*)md;
          while (s->eventsPending())
          {
            MidiRecordEvent ev = s->receiveEvent();
            // FIXME: This is for recording the events sent by GUI.
            //        It never gets a chance to be processed since reading of
            //         record FIFOs is done only by connected input ROUTES, below.
            //        To be useful, the synth itself must be allowed to be chosen
            //         as an input route, which is simple enough, but we currently don't
            //         list synths as inputs for fear of too many INCOMPATIBLE messages
            //         from DIFFERING synths. However, we could allow ONLY THIS synth
            //         to be listed and therefore be automatically connected too, if desired.
            //md->recordEvent(ev);
            //
            // For now, instead of recording, here is the minimum that we must do:

            // Intercept any special MusE system sysex messages. (This IS the system right here.)
            bool intercepted = false;
            const int type = ev.type();
            switch(type)
            {
              case ME_SYSEX:
              {
                const unsigned char* p = ev.data();
                int n = ev.len();
                if(n >= 3)
                {
                  if(p[0] == MUSE_SYNTH_SYSEX_MFG_ID)
                  {
                    if(p[1] == MUSE_SYSEX_SYSTEM_ID && p[2] == MUSE_SYSEX_SYSTEM_UPDATE_DRUM_MAPS_ID)
                    {
                      intercepted = true;
                      if(port >= 0 && port < MusECore::MIDI_PORTS)
                        MusEGlobal::midiPorts[port].updateDrumMaps();
                    }
                  }
                }
              }
              break;

              default:
              break;
            }

            // Update hardware state so knobs and boxes are updated. Optimize to avoid re-setting existing values.
            // Same code as in MidiPort::sendEvent()
            if(!intercepted && port != -1)
              // This is the audio thread. Just set directly.
              MusEGlobal::midiPorts[port].setHwCtrlState(MidiPlayEvent(ev));
          }
        }

        // Take snapshots of the current sizes of the recording fifos,
        //  because they may change while here in process, asynchronously.
        md->beforeProcess();

        //
        // --------- Handle midi events for audio tracks -----------
        //

        if(port < 0)
          continue;

        for(int chan = 0; chan < MusECore::MUSE_MIDI_CHANNELS; ++chan)
        {
          MusECore::MidiRecFifo& rf = md->recordEvents(chan);
          int count = md->tmpRecordCount(chan);
          for(int i = 0; i < count; ++i)
          {
            const MusECore::MidiRecordEvent& event(rf.peek(i));

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
              else //if(etype == MusECore::ME_PROGRAM)
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

                  unsigned int pframe = _pos.frame();
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
                  unsigned int rec_t = playing ? ev_t : pframe;

                  if(!MusEGlobal::automation)
                    continue;
                  AutomationType at = track->automationType();
                  // Unlike our built-in gui controls, there is not much choice here but to
                  //  just do this:
                  if ( (at == AUTO_WRITE) ||
                       (at == AUTO_READ && !playing) ||
                       (at == AUTO_TOUCH) )
                    track->enableController(actrl, false);
                  if(playing)
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

      MidiTrackList* mtl = MusEGlobal::song->midis();
      for (iMidiTrack t = mtl->begin(); t != mtl->end(); ++t)
      {
            MidiTrack* track = *t;
            const int t_port = track->outPort();
            const int t_channel = track->outChannel();
            MidiPort* mp = 0;
            if(t_port >= 0 && t_port < MusECore::MIDI_PORTS)
              mp = &MusEGlobal::midiPorts[t_port];
            MidiDevice* md = 0;
            if(mp)
              md = mp->device();
            // only add track events if the track is unmuted and turned on
            if(!track->isMute() && !track->off())
            {
              if(playing)
              {
                unsigned int lat_offset = 0;
                unsigned int cur_tick = curTickPos;
                unsigned int next_tick = nextTickPos;

                //--------------------------------------------------------------------
                // Account for the midi track's latency correction and/or compensation.
                //--------------------------------------------------------------------
                // TODO How to handle when external sync is on. For now, don't try to correct.
                if(MusEGlobal::config.enableLatencyCorrection && !extsync)
                {
                  const TrackLatencyInfo& li = track->getLatencyInfo(false);
                  // This value is negative for correction.
                  const float mlat = li._sourceCorrectionValue;
                  if((int)mlat < 0)
                  {
                    // Convert to a positive offset.
                    const unsigned int l = (unsigned int)(-mlat);
                    if(l > lat_offset)
                      lat_offset = l;
                  }
                  if(lat_offset != 0)
                  {
                    Pos ppp(_pos.frame() + lat_offset, false);
                    cur_tick = ppp.tick();
                    ppp += frames;
                    next_tick = ppp.tick();
                  }
                }

                collectEvents(track, cur_tick, next_tick, frames, lat_offset);
              }
            }

            //
            //----------midi recording
            //
            const bool track_rec_flag = track->recordFlag();
// REMOVE Tim. monitor. Changed.
//            const bool track_rec_monitor = track->recMonitor(); // Separate monitor and record functions.
            const bool track_rec_monitor = track->isRecMonitored(); // Separate monitor and record functions.

            if(track_rec_monitor || track_rec_flag)
            {
                  MPEventList& rl = track->mpevents;
                  RouteList* irl = track->inRoutes();
                  for(ciRoute r = irl->begin(); r != irl->end(); ++r)
                  {
                        if(!r->isValid() || (r->type != Route::MIDI_PORT_ROUTE))
                          continue;
                        int devport = r->midiPort;
                        if (devport == -1)
                          continue;
                        MidiDevice* dev = MusEGlobal::midiPorts[devport].device();
                        if(!dev)
                          continue;

#ifdef _USE_MIDI_ROUTE_PER_CHANNEL_

                        const int r_chan = r->channel;
#else
                        const int channelMask = r->channel;
                        if(channelMask == -1 || channelMask == 0)
                          continue;
#endif // _USE_MIDI_ROUTE_PER_CHANNEL_

                        for(int channel = 0; channel < MusECore::MUSE_MIDI_CHANNELS; ++channel)
                        {

#ifdef _USE_MIDI_ROUTE_PER_CHANNEL_
                          if(r_chan != -1 && channel != r_chan)
                            continue;
#else // _USE_MIDI_ROUTE_PER_CHANNEL_
                          if(!(channelMask & (1 << channel)))
                            continue;
#endif // _USE_MIDI_ROUTE_PER_CHANNEL_

                          if(!dev->sysexFIFOProcessed())
                          {
                            // Set to the sysex fifo at first.
                            MidiRecFifo& rf = dev->recordEvents(MusECore::MUSE_MIDI_CHANNELS);
                            // Get the frozen snapshot of the size.
                            int count = dev->tmpRecordCount(MusECore::MUSE_MIDI_CHANNELS);

                            for(int i = 0; i < count; ++i)
                            {
                              MidiRecordEvent event(rf.peek(i));
                              event.setPort(t_port);
                              event.setChannel(t_channel);
                              // don't echo controller changes back to software
                              // synthesizer:
                              if(md && track_rec_monitor)
                              {
                                // Do not echo synth events back to the same synth instance under any circumstances,
                                //  not even if monitor (echo) is on.
                                if(!dev->isSynti() || dev != md)
                                {
                                  // All recorded events arrived in the previous period. Shift into this period for playback.
                                  unsigned int et = event.time();
                                  // The events arrived in the previous period. Shift into this period for playback.
                                  // The events are already biased with the last frame time.
                                  unsigned int t = et + MusEGlobal::segmentSize;
                                  // Protection from slight errors in estimated frame time.
                                  if(t >= (syncFrame + MusEGlobal::segmentSize))
                                  {
                                    DEBUG_MIDI(stderr, "Error: Audio::processMidi(): sysex: t:%u >= syncFrame:%u + segmentSize:%u (==%u)\n", 
                                            t, syncFrame, MusEGlobal::segmentSize, syncFrame + MusEGlobal::segmentSize);
                                    
                                    t = syncFrame + (MusEGlobal::segmentSize - 1);
                                  }
                                  event.setTime(t);
                                  md->putEvent(event, MidiDevice::NotLate);
                                  event.setTime(et);  // Restore for recording.
                                }
                              }

                              unsigned int et = event.time();
                              // Make sure the event is recorded in units of ticks.
                              if(extsync)
                              {
                                const unsigned int xt = extClockHistoryFrame2Tick(event.time());
                                DEBUG_MIDI(stderr, "processMidi: event time:%d dataA:%d dataB:%d curTickPos:%u set time:%u\n",
                                                event.time(), event.dataA(), event.dataB(), curTickPos, xt);
                                
                                event.setTime(xt);
                              }
                              else
                              {
                                // All recorded events arrived in the previous period. Shift into this period for record.
                                unsigned int t = et + MusEGlobal::segmentSize;
                                // Protection from slight errors in estimated frame time.
                                if(t >= (syncFrame + MusEGlobal::segmentSize))
                                {
                                  DEBUG_MIDI(stderr, "Error: Audio::processMidi(): record sysex: t:%u >= syncFrame:%u + segmentSize:%u (==%u)\n", 
                                          t, syncFrame, MusEGlobal::segmentSize, syncFrame + MusEGlobal::segmentSize);
                                  
                                  t = syncFrame + (MusEGlobal::segmentSize - 1);
                                }
                                // Be sure to allow for some (very) late events, such as
                                //  the first chunk's time in a multi-chunk sysex.
                                const unsigned int a_fr = pos().frame() + t;
                                const unsigned int fin_fr = syncFrame > a_fr ? 0 : a_fr - syncFrame;
                                event.setTime(MusEGlobal::tempomap.frame2tick(fin_fr));
                              }

                              // Is the transport recording, or, is it about to be from external sync?
                              if((recording || 
                                 (MusEGlobal::song->record() && extsync && MusEGlobal::midiSyncContainer.isPlaying())) 
                                 && track_rec_flag)
                                rl.add(event);
                              
                              event.setTime(et);  // Restore.
                            }
                            dev->setSysexFIFOProcessed(true);
                          }

                          MidiRecFifo& rf = dev->recordEvents(channel);
                          int count = dev->tmpRecordCount(channel);
                          for(int i = 0; i < count; ++i)
                          {
                                MidiRecordEvent event(rf.peek(i));
                                int defaultPort = devport;
                                int drumRecPitch=0; //prevent compiler warning: variable used without initialization
                                MidiController *mc = 0;
                                int ctl = 0;
                                int prePitch = 0, preVelo = 0;

                                event.setPort(t_port);
                                event.setChannel(t_channel);

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
                                            // Default to track port if -1 and track channel if -1.
                                            devport = MusEGlobal::drumMap[(unsigned int)MusEGlobal::drumInmap[pitch]].port;
                                            if(devport == -1)
                                              devport = track->outPort();
                                            event.setPort(devport);
                                            int mapchan = MusEGlobal::drumMap[(unsigned int)MusEGlobal::drumInmap[pitch]].channel;
                                            if(mapchan != -1)
                                              event.setChannel(mapchan);
                                            event.setA(MusEGlobal::drumMap[(unsigned int)MusEGlobal::drumInmap[pitch]].anote);
                                      }
                                      else if (track->type() == Track::NEW_DRUM)
                                      {
                                        int pitch = event.dataA();
                                        int dmindex = track->map_drum_in(pitch);
                                        //Map note that is played according to MusEGlobal::drumInmap
                                        drumRecPitch = track->drummap()[dmindex].enote;
                                        // Default to track port if -1 and track channel if -1.
                                        devport = track->drummap()[dmindex].port;
                                        if(devport == -1)
                                          devport = track->outPort();
                                        event.setPort(devport);
                                        int mapchan = track->drummap()[dmindex].channel;
                                        if(mapchan != -1)
                                          event.setChannel(mapchan);
                                        event.setA(track->drummap()[dmindex].anote);

                                        if (MusEGlobal::config.newDrumRecordCondition & MusECore::DONT_REC_HIDDEN &&
                                            track->drummap()[dmindex].hide )
                                          continue; // skip that event, proceed with the next

                                        if (MusEGlobal::config.newDrumRecordCondition & MusECore::DONT_REC_MUTED &&
                                            track->drummap()[dmindex].mute )
                                          continue; // skip that event, proceed with the next
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

                                      // Apply track velocity and compression to note-on AND note-off events.
                                      preVelo = event.dataB();
                                      int velo = preVelo + track->velocity;
                                      velo = (velo * track->compression) / 100;
                                      if (velo > 127)
                                            velo = 127;
                                      if (velo < 1)
                                            // Zero means zero. Should mean no note at all?
                                            //velo = 1;
                                            velo = 0; // Use zero as a marker to tell the playback (below) not to sound the note.

                                      event.setB(velo);
                                }
                                else if(event.type() == MusECore::ME_CONTROLLER)
                                {
                                  if(track->type() == Track::DRUM)
                                  {
                                    ctl = event.dataA();
                                    // Regardless of what port the event came from, is it a drum controller event
                                    //  according to the track port's instrument?
                                    if(mp)
                                      mc = mp->drumController(ctl);
                                    if(mc)
                                    {
                                      int pitch = ctl & 0x7f;
                                      ctl &= ~0xff;
                                      int dmindex = MusEGlobal::drumInmap[pitch] & 0x7f;
                                      //Map note that is played according to MusEGlobal::drumInmap
                                      drumRecPitch = MusEGlobal::drumMap[dmindex].enote;
                                      // Default to track port if -1 and track channel if -1.
                                      devport = MusEGlobal::drumMap[dmindex].port;
                                      if(devport == -1)
                                        devport = track->outPort();
                                      event.setPort(devport);
                                      int mapchan = MusEGlobal::drumMap[dmindex].channel;
                                      if(mapchan != -1)
                                        event.setChannel(mapchan);
                                      event.setA(ctl | MusEGlobal::drumMap[dmindex].anote);
                                    }
                                  }
                                  else if (track->type() == Track::NEW_DRUM) //FINDMICHJETZT TEST
                                  {
                                    ctl = event.dataA();
                                    // Regardless of what port the event came from, is it a drum controller event
                                    //  according to the track port's instrument?
                                    if(mp)
                                      mc = mp->drumController(ctl);
                                    if(mc)
                                    {
                                      int pitch = ctl & 0x7f; // pitch is now the incoming pitch
                                      ctl &= ~0xff;
                                      int dmindex = track->map_drum_in(pitch) & 0x7f;
                                      //Map note that is played according to drumInmap
                                      drumRecPitch = track->drummap()[dmindex].enote;
                                      // Default to track port if -1 and track channel if -1.
                                      devport = track->drummap()[dmindex].port;
                                      if(devport == -1)
                                        devport = track->outPort();
                                      event.setPort(devport);
                                      int mapchan = track->drummap()[dmindex].channel;
                                      if(mapchan != -1)
                                        event.setChannel(mapchan);
                                      event.setA(ctl | track->drummap()[dmindex].anote);

                                      if (MusEGlobal::config.newDrumRecordCondition & MusECore::DONT_REC_HIDDEN &&
                                          track->drummap()[dmindex].hide )
                                        continue; // skip that event, proceed with the next

                                      if (MusEGlobal::config.newDrumRecordCondition & MusECore::DONT_REC_MUTED &&
                                          track->drummap()[dmindex].mute )
                                        continue; // skip that event, proceed with the next
                                    }
                                  }
                                }

                                // MusE uses a fixed clocks per quarternote of 24.
                                // At standard 384 ticks per quarternote for example,
                                // 384/24=16 for a division of 16 sub-frames (16 MusE 'ticks').
                                // If ext sync, events are now time-stamped with last tick in MidiDevice::recordEvent(). p3.3.35
                                // TODO: Tested, but record resolution not so good. Switch to wall clock based separate list in MidiDevice.

                                // don't echo controller changes back to software
                                // synthesizer:

                                // Zero means zero. Should mean no note at all?
                                // If the event is marked as a note with zero velocity (above), do not sound the note.
                                if(!event.isNote() || event.dataB() != 0)
                                {

                                  // All recorded events arrived in previous period. Shift into this period for playback.
                                  //  frameoffset needed to make process happy.
                                  unsigned int et = event.time();
                                  // The events arrived in the previous period. Shift into this period for playback.
                                  // The events are already biased with the last frame time.
                                  unsigned int t = et + MusEGlobal::segmentSize;
                                  // Protection from slight errors in estimated frame time.
                                  if(t >= (syncFrame + MusEGlobal::segmentSize))
                                  {
                                    DEBUG_MIDI(stderr, "Error: Audio::processMidi(): event: t:%u >= syncFrame:%u + segmentSize:%u (==%u)\n", 
                                            t, syncFrame, MusEGlobal::segmentSize, syncFrame + MusEGlobal::segmentSize);
                                    
                                    t = syncFrame + (MusEGlobal::segmentSize - 1);
                                  }
                                  event.setTime(t);
                                  // Check if we're outputting to another port than default:
                                  if (devport == defaultPort) {
                                        event.setPort(t_port);
                                        // REMOVE Tim. monitor. Changed.
                                        //if(md && track_rec_monitor && !track->off() && !track->isMute())
                                        if(md && track_rec_monitor)
                                        {
                                          // Do not echo synth events back to the same synth instance under any circumstances,
                                          //  not even if monitor (echo) is on.
                                          if(!dev->isSynti() || dev != md)
                                          {
                                            MidiInstrument* minstr = MusEGlobal::midiPorts[t_port].instrument();
                                            const MidiInstrument::NoteOffMode nom = minstr->noteOffMode();
                                            // If the instrument has no note-off mode, do not use the stuck notes mechanism, send as is.
                                            // This allows drum input triggers (no note offs at all), although it is awkward to
                                            //  first have to choose an output instrument with no note-off mode.
                                            if(nom == MidiInstrument::NoteOffNone)
                                            {
                                              if(event.isNoteOff())
                                                // Try to remove any corresponding stuck live note.
                                                track->removeStuckLiveNote(t_port, event.channel(), event.dataA());
//                                               md->addScheduledEvent(event);
                                              md->putEvent(event, MidiDevice::NotLate);
                                            }
                                            else if(event.isNoteOff())
                                            {
                                              // Try to remove any corresponding stuck live note.
                                              // Only if a stuck live note existed do we schedule the note off to play.
                                              if(track->removeStuckLiveNote(t_port, event.channel(), event.dataA()))
//                                                 md->addScheduledEvent(event);
                                                md->putEvent(event, MidiDevice::NotLate);
                                            }
                                            else if(event.isNote())
                                            {
                                              // Check if a stuck live note exists on any track.
                                              ciMidiTrack it_other = mtl->begin();
                                              for( ; it_other != mtl->end(); ++it_other)
                                              {
                                                if((*it_other)->stuckLiveNoteExists(t_port, event.channel(), event.dataA()))
                                                  break;
                                              }
                                              // Only if NO stuck live note existed do we schedule the note on to play.
                                              if(it_other == mtl->end())
                                              {
                                                if(track->addStuckLiveNote(t_port, event.channel(), event.dataA()))
//                                                   md->addScheduledEvent(event);
                                                  md->putEvent(event, MidiDevice::NotLate);
                                              }
                                            }
                                            else
                                            {
                                              // TODO Maybe grab the flag from the 'Optimize Controllers' Global Setting,
                                              //       which so far was meant for (N)RPN stuff. For now, just force it.
                                              // This is the audio thread. Just set directly.
                                              MusEGlobal::midiPorts[t_port].setHwCtrlState(event);
                                              md->putEvent(event, MidiDevice::NotLate);
                                            }
                                          }
                                        }
                                      }
                                  else {
                                        MidiDevice* mdAlt = MusEGlobal::midiPorts[devport].device();
                                        // REMOVE Tim. monitor. Changed.
                                        //if(mdAlt && track_rec_monitor && !track->off() && !track->isMute())
                                        if(mdAlt && track_rec_monitor)
                                        {
                                          // Do not echo synth events back to the same synth instance under any circumstances,
                                          //  not even if monitor (echo) is on.
                                          if(!dev->isSynti() || dev != mdAlt)
                                          {
                                            MidiInstrument* minstr = MusEGlobal::midiPorts[devport].instrument();
                                            MidiInstrument::NoteOffMode nom = minstr->noteOffMode();
                                            // If the instrument has no note-off mode, do not use the
                                            //  stuck notes mechanism, just send as is.
                                            // If the instrument has no note-off mode, do not use the stuck notes mechanism, send as is.
                                            // This allows drum input triggers (no note offs at all), although it is awkward to
                                            //  first have to choose an output instrument with no note-off mode.
                                            if(nom == MidiInstrument::NoteOffNone)
                                            {
                                              if(event.isNoteOff())
                                                // Try to remove any corresponding stuck live note.
                                                track->removeStuckLiveNote(event.port(), event.channel(), event.dataA());
//                                               mdAlt->addScheduledEvent(event);
                                              mdAlt->putEvent(event, MidiDevice::NotLate);
                                            }
                                            else if(event.isNoteOff())
                                            {
                                              // Try to remove any corresponding stuck live note.
                                              // Only if a stuck live note existed do we schedule the note off to play.
                                              if(track->removeStuckLiveNote(event.port(), event.channel(), event.dataA()))
//                                                 mdAlt->addScheduledEvent(event);
                                                mdAlt->putEvent(event, MidiDevice::NotLate);
                                            }
                                            else if(event.isNote())
                                            {
                                              // Check if a stuck live note exists on any track.
                                              ciMidiTrack it_other = mtl->begin();
                                              for( ; it_other != mtl->end(); ++it_other)
                                              {
                                                if((*it_other)->stuckLiveNoteExists(event.port(), event.channel(), event.dataA()))
                                                  break;
                                              }
                                              // Only if NO stuck live note existed do we schedule the note on to play.
                                              if(it_other == mtl->end())
                                              {
                                                if(track->addStuckLiveNote(event.port(), event.channel(), event.dataA()))
                                                  mdAlt->putEvent(event, MidiDevice::NotLate);
                                              }
                                            }
                                            else
                                            {
                                              // TODO Maybe grab the flag from the 'Optimize Controllers' Global Setting,
                                              //       which so far was meant for (N)RPN stuff. For now, just force it.
                                              // This is the audio thread. Just set directly.
                                              MusEGlobal::midiPorts[devport].setHwCtrlState(event);
                                              mdAlt->putEvent(event, MidiDevice::NotLate);
                                            }
                                          }
                                        }
                                      }
                                  event.setTime(et);  // Restore for recording.

                                // Shall we activate meters even while rec echo is off? Sure, why not...
                                if(event.isNote() && event.dataB() > track->activity())
                                  track->setActivity(event.dataB());
                              }

                              // Is the transport recording, or, is it about to be from external sync?
                              if((recording || 
                                 (MusEGlobal::song->record() && extsync && MusEGlobal::midiSyncContainer.isPlaying())) 
                                 && track_rec_flag)
                              {
                                    unsigned int et = event.time();
                                    // Make sure the event is recorded in units of ticks.
                                    if(extsync)
                                    {
                                      const unsigned int xt = extClockHistoryFrame2Tick(event.time());
                                      DEBUG_MIDI(stderr, "processMidi: event time:%d dataA:%d dataB:%d curTickPos:%u set time:%u\n",
                                                      event.time(), event.dataA(), event.dataB(), curTickPos, xt);
                                      
                                      event.setTime(xt);
                                    }
                                    else
                                    {
                                      // REMOVE Tim. latency. Removed. Oops, with ALSA this adds undesired shift forward!
//                                       // All recorded events arrived in the previous period. Shift into this period for record.
                                      // REMOVE Tim. latency. Changed. Oops, with ALSA this adds undesired shift forward!
                                      // And with Jack midi we currently already shift forward, in the input routine!
                                      // But I'm debating where to add the correction factor - I really need to add it here
                                      //  (exactly like the WaveTrack recording correction) but a very simple fix would be
                                      //  in the Jack midi input routine to replace the current fixed segSize correction
                                      //  factor with the MidiDevice::_latencyInfo._outputLatency,
                                      //  but that is wrong although it would work.
                                      // To add the correction here may be more complicated than the WaveTrack,
                                      //  some things in the ALSA and Jack midi input routines depend on the event time
                                      //  (like midi clock, other rt events).
                                      //
                                      //
//                                       unsigned int t = et + MusEGlobal::segmentSize;
//                                       // Protection from slight errors in estimated frame time.
//                                       if(t >= (syncFrame + MusEGlobal::segmentSize))
//                                       {
//                                         DEBUG_MIDI(stderr, "Error: Audio::processMidi(): record event: t:%u >= syncFrame:%u + segmentSize:%u (==%u)\n", 
//                                                 t, syncFrame, MusEGlobal::segmentSize, syncFrame + MusEGlobal::segmentSize);
//                                         
//                                         t = syncFrame + (MusEGlobal::segmentSize - 1);
//                                       }
                                      unsigned int t = et;
                                      // Protection from slight errors in estimated frame time.
                                      if(t >= syncFrame)
                                      {
                                        DEBUG_MIDI(stderr, "Error: Audio::processMidi(): record event: t:%u >= syncFrame:%u\n", 
                                                t, syncFrame);
                                        
                                        t = syncFrame - 1;
                                      }

                                      // Be sure to allow for some (very) late events, such as
                                      //  the first chunk's time in a multi-chunk sysex.
                                      const unsigned int a_fr = pos().frame() + t;
                                      const unsigned int fin_fr = syncFrame > a_fr ? 0 : a_fr - syncFrame;
                                      event.setTime(MusEGlobal::tempomap.frame2tick(fin_fr));
                                    }

                                    // In these next steps, it is essential to set the recorded event's port
                                    //  to the track port so buildMidiEventList will accept it. Even though
                                    //  the port may have no device "<none>".
                                    //
                                    if (track->type() == Track::DRUM || track->type() == Track::NEW_DRUM)
                                    {
                                      // Is it a drum controller event?
                                      if(mc)
                                      {
                                          MusECore::MidiPlayEvent drumRecEvent = event;
                                          drumRecEvent.setA(ctl | drumRecPitch);
                                          // In this case, preVelo is simply the controller value.
                                          drumRecEvent.setB(preVelo);
                                          drumRecEvent.setPort(t_port); //rec-event to current port
                                          drumRecEvent.setChannel(t_channel); //rec-event to current channel
                                          track->mpevents.add(drumRecEvent);
                                      }
                                      else
                                      {
                                          MusECore::MidiPlayEvent drumRecEvent = event;
                                          drumRecEvent.setA(drumRecPitch);
                                          drumRecEvent.setB(preVelo);
                                          // Changed to 'port'. Events were not being recorded for a drum map entry pointing to a
                                          //  different port. That must have been wrong - buildMidiEventList would ignore that. Tim.
                                          drumRecEvent.setPort(t_port);  //rec-event to current port
                                          drumRecEvent.setChannel(t_channel); //rec-event to current channel
                                          track->mpevents.add(drumRecEvent);
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
                                          recEvent.setPort(t_port);
                                          recEvent.setChannel(t_channel);

                                          track->mpevents.add(recEvent);
                                    }
                                    // Restore. Not required.
                                    //event.setTime(et);
                              }
                        }
                  }
            }
        }

        // Must be playing for valid nextTickPos, right? But wasn't checked in Audio::processMidi().
        // MusEGlobal::audio->isPlaying() might not be true during seek right now.
        //if(MusEGlobal::audio->isPlaying())
        //if(playing)
        //{
          ciMPEvent k;
          MidiDevice* mdev;
          int mport;
          // What is the current transport frame?
          const unsigned int pos_fr = _pos.frame();
          // What is the (theoretical) next transport frame?
          const unsigned int next_pos_fr = pos_fr + frames;

          // If muted or off we want to send all playback note-offs immediately.
          if(track->isMute() || track->off())
          {
            //---------------------------------------------------
            //    Send all track-related playback note-offs (NOT 'live' note-offs)
            //     which were not put directly to the device
            //---------------------------------------------------
            MPEventList& mel = track->stuckNotes;
            if(!mel.empty())
            {
              for(k = mel.begin(); k != mel.end(); ++k)
              {
                MidiPlayEvent ev(*k);
                mport = ev.port();
                if(mport < 0)
                  continue;
                mdev = MusEGlobal::midiPorts[mport].device();
                if(!mdev)
                  continue;
                ev.setTime(0);   // Mark for immediate delivery.
                //ev.setTime(MusEGlobal::audio->midiQueueTimeStamp(k->time()));
                mdev->putEvent(ev, MidiDevice::NotLate);
              }
              mel.clear();
            }
          }
          else
          // If not muted and not off, we want to schedule all playback note-offs normally.
          {
            //---------------------------------------------------
            //    Schedule all track-related playback note-offs (NOT 'live' note-offs)
            //     which were not put directly to the device
            //    To save time this was put here instead of MidiDevice::processStuckNotes()
            //---------------------------------------------------
            MPEventList& mel = track->stuckNotes;
            if(!mel.empty())
            {
              for(k = mel.begin(); k != mel.end(); ++k)
              {
                MidiPlayEvent ev(*k);
                unsigned int off_tick = ev.time();
                // If external sync is not on, we can take advantage of frame accuracy but
                //  first we must allow the next tick position to be included in the search
                //  even if it is equal to the current tick position.
                if(extsync ? (off_tick >= nextTickPos) : (off_tick > nextTickPos))
                      break;
                mport = ev.port();
                if(mport < 0)
                  continue;
                mdev = MusEGlobal::midiPorts[mport].device();
                if(!mdev)
                  continue;
                
                unsigned int off_frame = 0;
                if(extsync)
                {
                  if(off_tick < curTickPos)
                    off_tick = curTickPos;
                  off_frame = extClockHistoryTick2Frame(off_tick - curTickPos) + MusEGlobal::segmentSize;
                }
                else
                {
                  // What is the exact transport frame that the event should be played at?
                  const unsigned int fr = MusEGlobal::tempomap.tick2frame(off_tick);
                  // Is the event frame outside of the current transport frame range?
                  if(fr >= next_pos_fr)
                    break;
                  off_frame = (fr < pos_fr) ? 0 : fr - pos_fr;
                  off_frame += syncFrame;
                }
                ev.setTime(off_frame);
                
                // TODO: DECIDE: Hm, we don't want the device to miss any note offs.
                //               So I guess schedule this as a user event rather than a playback event.
                mdev->putEvent(ev, MidiDevice::NotLate);
              }
              mel.erase(mel.begin(), k);
            }
          }

          // If no monitor or off, or not rec-armed (or muted), we want to cancel all 'live' (rec) stuck notes immediately.
          // REMOVE Tim. monitor. Changed.
          //if(!track_rec_monitor || track->off() || track->isMute())
          if(!track_rec_monitor)
          {
            //------------------------------------------------------------
            //    Send all track-related 'live' (rec) note-offs
            //     which were not put directly to the device
            //------------------------------------------------------------
            MPEventList& mel = track->stuckLiveNotes;
            if(!mel.empty())
            {
              for(k = mel.begin(); k != mel.end(); ++k)
              {
                MidiPlayEvent ev(*k);
                mport = ev.port();
                if(mport < 0)
                  continue;
                mdev = MusEGlobal::midiPorts[mport].device();
                if(!mdev)
                  continue;
                ev.setTime(0);   // Mark for immediate delivery.
                //ev.setTime(MusEGlobal::audio->midiQueueTimeStamp(k->time()));
                mdev->putEvent(ev, MidiDevice::NotLate);
              }
              mel.clear();
            }
          }
        //}
      }

      //---------------------------------------------------
      //    insert metronome clicks
      //---------------------------------------------------

      // If in PRECOUNT state, process the precount events.
      processPrecount(frames);

      // Since the latency for audio and midi may be different,
      //  process the audio and midi metronomes separately.
      processAudioMetronome(frames);
      processMidiMetronome(frames);

      // REMOVE Tim. clock. Added.
//       //---------------------------------------------------
//       //    send midi clock output events
//       //---------------------------------------------------
// 
//       _clockOutputQueueSize = 0;
//       if(!extsync)
//       {
//         const unsigned curr_audio_frame = syncFrame;
//         const unsigned next_audio_frame = curr_audio_frame + frames;
// //         const uint64_t numer = (uint64_t)MusEGlobal::config.division * (uint64_t)MusEGlobal::tempomap.globalTempo() * 10000UL;
//         const uint64_t denom = (uint64_t)MusEGlobal::config.division * (uint64_t)MusEGlobal::tempomap.globalTempo() * 10000UL;
//         const unsigned int div = MusEGlobal::config.division/24;
// 
// //         // Do not round up here since (audio) frame resolution is higher than tick resolution.
// //         const unsigned int clock_tick = muse_multiply_64_div_64_to_64(numer, curr_audio_frame,
// //           (uint64_t)MusEGlobal::sampleRate * (uint64_t)MusEGlobal::tempomap.tempo(curTickPos));
// 
// //         unsigned int clock_tick_end;
//         // Is the transport moving?
//         if(playing)
//         {
// //           unsigned int delta_tick;
// //           // Did tick position wrap around?
// //           if(curTickPos > nextTickPos)
// //             delta_tick = curTickPos - nextTickPos;
// //           else
// //             delta_tick = nextTickPos - curTickPos;
// //           clock_tick_end = clock_tick + delta_tick;
//         }
//         else
//         {
// //           // Do not round up here since (audio) frame resolution is higher than tick resolution.
// //           clock_tick_end = muse_multiply_64_div_64_to_64(numer, next_audio_frame,
// //             (uint64_t)MusEGlobal::sampleRate * (uint64_t)MusEGlobal::tempomap.tempo(curTickPos));
// 
//           uint64_t div_remainder;
//           const uint64_t div_frames = muse_multiply_64_div_64_to_64(
//             (uint64_t)MusEGlobal::sampleRate * (uint64_t)MusEGlobal::tempomap.tempo(curTickPos), div,
//             denom, LargeIntRoundNone, &div_remainder);
// 
//           // Counter too far in future? Reset.
//           if(_clockOutputCounter >= curr_audio_frame && _clockOutputCounter - curr_audio_frame >= div_frames)
//           {
//             _clockOutputCounter = curr_audio_frame;
//             _clockOutputCounterRemainder = 0;
//           }
//           // Counter too far in past? Reset.
//           else if(_clockOutputCounter < curr_audio_frame)
//           {
//             _clockOutputCounter = curr_audio_frame;
//             _clockOutputCounterRemainder = 0;
//           }
// 
//           //const uint64_t curr_clock_out_count = _clockOutputCounter + div_frames + (_clockOutputCounterRemainder + div_remainder) / denom;
//           //uint64_t next_clock_out_frame = _clockOutputCounter + div_frames + (_clockOutputCounterRemainder + div_remainder) / denom;
//           uint64_t raccum;
//           //while(next_clock_out_frame <= next_audio_frame)
//           while(_clockOutputCounter < next_audio_frame)
//           {
//             if(_clockOutputQueueSize >= _clockOutputQueueCapacity)
//               break;
//             
//             //_clockOutputQueue[_clockOutputQueueSize] = _clockOutputCounter - curr_audio_frame;
//             _clockOutputQueue[_clockOutputQueueSize] = _clockOutputCounter;
//             ++_clockOutputQueueSize;
//             raccum = _clockOutputCounterRemainder + div_remainder;
//             _clockOutputCounter += div_frames + (raccum / denom);
//             _clockOutputCounterRemainder = raccum % denom;
//           }
//           //const uint64_t next_clock_out_count = _clockOutputCounter + div_frames + (_clockOutputCounterRemainder + div_remainder) / denom;
// 
//           //if(next_audio_frame >= next_clock_out_count)
//           //{
//             
//           //}
//         }
// 
// //         // Did clock_tick wrap around?
// //         if(_clockOutputCounter > clock_tick)
// //           _clockOutputCounter = clock_tick;
// // 
// //         //const unsigned int div = MusEGlobal::config.division/24;
// //         if(clock_tick_end >= _clockOutputCounter + div)
// //         {
// //           // This will always be at least 1.
// //           const unsigned int num_clocks = (clock_tick_end - _clockOutputCounter) / div;
// //           const unsigned int clk_frame_step = frames / num_clocks;
// //           const unsigned int clk_frame_step_rem = frames % num_clocks;
// //           unsigned int clk_frame_off;
// //           for(unsigned int c = 0; c < num_clocks; ++c)
// //           {
// //             if(c >= _clockOutputQueueCapacity)
// //               break;
// //             clk_frame_off = c * clk_frame_step + (c * clk_frame_step_rem) / num_clocks;
// //           }
// // 
// //           _clockOutputCounter = clock_tick_end;
// //         }
// // 
// // 
// //         unsigned cc, f;
// //         while(1)
// //         {
// //           cc = _clockOutputCounter + div;
// //           f = Pos(cc, true).frame();
// //           //if(f
// //         }
//       }
      
      //
      // Play all midi events up to curFrame.
      //
      for(iMidiDevice id = MusEGlobal::midiDevices.begin(); id != MusEGlobal::midiDevices.end(); ++id)
      {
        MidiDevice* pl_md = *id;
//         const int pl_port = pl_md->midiPort();

        // We are done with the 'frozen' recording fifos, remove the events.
        pl_md->afterProcess();

        pl_md->processStuckNotes();
        
        // REMOVE Tim. clock. Added.
        // While we are at it, to avoid the overhead of yet another device loop,
        //  handle midi clock output here, for all device types.
//         if(!extsync && pl_port >= 0 && pl_port < MIDI_PORTS)
//         {
//           MidiPort* clk_mp = &MusEGlobal::midiPorts[pl_port];
//           // Clock out turned on?
//           if(clk_mp->syncInfo().MCOut())
//           {
//             for(unsigned int i = 0; i < _clockOutputQueueSize; ++i)
//             {
//               const MidiPlayEvent clk_ev(_clockOutputQueue[i], pl_port, 0, MusECore::ME_CLOCK, 0, 0);
//               pl_md->putEvent(clk_ev, MidiDevice::NotLate /*,MidiDevice::PlaybackBuffer*/);
//             }
//           }
//         }
        
        // ALSA devices handled by another thread.
        const MidiDevice::MidiDeviceType typ = pl_md->deviceType();
        switch(typ)
        {
          case MidiDevice::ALSA_MIDI:
          break;

          case MidiDevice::JACK_MIDI:
          case MidiDevice::SYNTH_MIDI:
            // The frame is not used by these devices but we pass it along anyway.
            // Only ALSA devices need the frame.
            pl_md->processMidi(syncFrame);
          break;
        }
      }

      // Receive hardware state events sent from various threads to this audio thread.
      // Update hardware state so gui controls are updated.
      MusEGlobal::song->processIpcOutEventBuffers();
      }

      
//---------------------------------------------------------
//   processPrecount
//---------------------------------------------------------

void Audio::processPrecount(unsigned int frames)
{
  //DEBUG_MIDI_METRONOME(stderr, "Audio::processPrecount: state:%d\n", state);
  if(state != PRECOUNT)
    return;
  
  MusECore::MetronomeSettings* metro_settings = 
    MusEGlobal::metroUseSongSettings ? &MusEGlobal::metroSongSettings : &MusEGlobal::metroGlobalSettings;

  const unsigned int nextPrecountFramePos = _precountFramePos + frames;
    
  DEBUG_MIDI_METRONOME(stderr, "Audio::processPrecount: precount: syncFrame:%u _pos.frame():%u"
    "_precountFramePos:%u precountMidiClickFrame:%u nextPrecountFramePos:%u clickno:%d\n",
    syncFrame, _pos.frame(), _precountFramePos, precountMidiClickFrame, nextPrecountFramePos, clickno);
    
  MidiDevice* md = 0;
  if(metro_settings->midiClickFlag)
    md = MusEGlobal::midiPorts[metro_settings->clickPort].device();
  
  AudioTickSound audioTickSound = MusECore::beatSound;

  while(true)
  {
    const unsigned precount_click_frame = precountMidiClickFrame + (precountMidiClickFrameRemainder ? 1 :0);
    if(precount_click_frame >= nextPrecountFramePos)
      break;
    // Don't actually sound anything if we're just running out the time.
    // There will be times when _precountFramePos >= precountTotalFrames
    //  and we must let it run out.
    if(_precountFramePos < precountTotalFrames)
    {
      if(MusEGlobal::song->click())
      {
        if ((clickno % clicksMeasure) == 0)
          audioTickSound = MusECore::measureSound;
        else
          audioTickSound = MusECore::beatSound;
        
        // We need to shift ahead in time because Jack waits one more cycle,
        //   unlike our own built-in transport which starts immediately.
        const unsigned int evtime = syncFrame + MusEGlobal::audioDevice->transportSyncToPlayDelay() +
          ((precount_click_frame < _precountFramePos) ? 0 : precount_click_frame - _precountFramePos);
          
        MusECore::MidiPlayEvent ev(evtime, metro_settings->clickPort, metro_settings->clickChan, 
          MusECore::ME_NOTEON, metro_settings->beatClickNote, metro_settings->beatClickVelo);
        
        if (audioTickSound == MusECore::measureSound) {
          ev.setA(metro_settings->measureClickNote);
          ev.setB(metro_settings->measureClickVelo);
        }
        if (md) {
          MusECore::MidiPlayEvent evmidi = ev;
          
#ifdef DEBUG_MIDI_TIMING_DIFFS
          fprintf(stderr, "EVENT TIME DIFF:%u\n", ev.time() - _lastEvTime);
                    _lastEvTime = ev.time();
#endif
          
          md->putEvent(evmidi, MidiDevice::NotLate, MidiDevice::PlaybackBuffer);
          // Internal midi paths are now all note off aware. Driver handles note offs. Convert.
          // Ticksynth has been modified too.
          evmidi.setType(MusECore::ME_NOTEOFF);
          evmidi.setB(0);
          evmidi.setTime(ev.time() + MusEGlobal::tempomap.ticks2frames(10, curTickPos));

          // The precount CANNOT use the stuck notes mechanism because the stuck notes
          //  mechanism wants ticks not frames, and it later compares those ticks
          //  with nextTickPos and converts them into frames using midiQueueTimeStamp(),
          //  which is not a valid mechanism during precount!
          // Therefore we have no choice but to directly schedule a note-off.
          // Since it is important that they not be missed, schedule these as userBuffer events.
          // Should be OK, it's unlikely that the note-off would be missed unless the user could 
          //  somehow seek during precount or something. (The userBuffer tries not to miss anything.)
          // The idea behind the stuck notes mechanism was that it converts to frames
          //  only at scheduling time so that if the user might change the tempo while
          //  playing a song, the note-off times will be properly adjusted.
          // But this precount mechanism CANNOT honour any tempo changes anyway because 
          //  we must know the total required precount frames BEFOREHAND so that it can be aligned
          //  with the metronome clicks properly. Thus negating any possible tempo change support.
          //md->addStuckNote(evmidi);
          md->putEvent(evmidi, MidiDevice::NotLate, MidiDevice::UserBuffer);
        }
        if (metro_settings->audioClickFlag) {
          ev.setA(audioTickSound);
          DEBUG_MIDI_METRONOME(stderr, "Audio::processMidi: precount: metronome->putEvent\n");
          metronome->putEvent(ev, MidiDevice::NotLate, MidiDevice::PlaybackBuffer);
          // Built-in metronome synth does not use stuck notes...
        }
      }
    }
    
    precountMidiClickFrame += framesBeat;
    
    precountMidiClickFrameRemainder += framesBeatRemainder;
    if(precountMidiClickFrameRemainder >= framesBeatDivisor)
    {
      precountMidiClickFrame++;
      precountMidiClickFrameRemainder -= framesBeatDivisor;
    }
    
    ++clickno;
  }
  
  _precountFramePos += frames;
}

//---------------------------------------------------------
//   processAudioMetronome
//---------------------------------------------------------

void Audio::processMidiMetronome(unsigned int frames)
{
      const MusECore::MetronomeSettings* metro_settings = 
      MusEGlobal::metroUseSongSettings ? &MusEGlobal::metroSongSettings : &MusEGlobal::metroGlobalSettings;

      const bool extsync = MusEGlobal::extSyncFlag;
      const bool playing = isPlaying();

      // Should the metronome be muted after precount?
      const bool precount_mute_metronome = metro_settings->precountEnableFlag
        && MusEGlobal::song->click()
        && !extsync
        && ((!MusEGlobal::song->record() && metro_settings->precountOnPlay) || MusEGlobal::song->record())
        && metro_settings->precountMuteMetronome;
      
      MidiDevice* md = 0;
      if (metro_settings->midiClickFlag && !precount_mute_metronome)
            md = MusEGlobal::midiPorts[metro_settings->clickPort].device();

      if (playing)
      {
            int bar, beat, z, n;
            unsigned tick;
            AudioTickSound audioTickSound = MusECore::beatSound;
            const MusECore::MetroAccents* accents;
            int accents_sz;
            
            unsigned int lat_offset_midi = 0;
            unsigned int cur_tick_midi = curTickPos;
            unsigned int next_tick_midi = nextTickPos;

            //--------------------------------------------------------------------
            // Account for the metronome's latency correction and/or compensation.
            //--------------------------------------------------------------------
            // TODO How to handle when external sync is on. For now, don't try to correct.
            if(MusEGlobal::config.enableLatencyCorrection && !extsync)
            {
              if(metro_settings->midiClickFlag)
              {
                const TrackLatencyInfo& li = metronome->getLatencyInfoMidi(false /*playback*/, false);
                // This value is negative for correction.
                const float mlat = li._sourceCorrectionValue;
                if((int)mlat < 0)
                {
                  // Convert to a positive offset.
                  const unsigned int l = (unsigned int)(-mlat);
                  if(l > lat_offset_midi)
                    lat_offset_midi = l;
                }
                if(lat_offset_midi != 0)
                {
                  Pos ppp(_pos.frame() + lat_offset_midi, false);
                  cur_tick_midi = ppp.tick();
                  ppp += frames;
                  next_tick_midi = ppp.tick();
                }
              }
            }

            // What is the current transport frame, adjusted?
            const unsigned int pos_fr_midi = _pos.frame() + lat_offset_midi;
            // What is the (theoretical) next transport frame?
            const unsigned int next_pos_fr_midi = pos_fr_midi + frames;

            // If external sync is not on, we can take advantage of frame accuracy but
            //  first we must allow the next tick position to be included in the search
            //  even if it is equal to the current tick position.
            while (extsync ? (midiClick < next_tick_midi) : (midiClick <= next_tick_midi))
            {
              bool do_play = true;
              unsigned int evtime = 0;
              if(extsync)
              {
                if(midiClick < cur_tick_midi)
                  midiClick = cur_tick_midi;
                evtime = extClockHistoryTick2Frame(midiClick - cur_tick_midi) + MusEGlobal::segmentSize;
              }
              else
              {
                // What is the exact transport frame that the midiClick should be played at?
                const unsigned int fr = MusEGlobal::tempomap.tick2frame(midiClick);
                // Is the midiClick frame outside of the current transport frame range?
                if(fr < pos_fr_midi || fr >= next_pos_fr_midi)
                {
                  // Break out of the loop if midiClick equals nextTickPos.
                  if(midiClick == next_tick_midi)
                    break;
                  // Continue on, but do not play any notes.
                  do_play = false;
                }
                evtime = fr - pos_fr_midi;
                evtime += syncFrame;
              }
                  
              DEBUG_MIDI_METRONOME(stderr, 
                "Audio::processMidiMetronome: playing: syncFrame:%u _pos.frame():%u midiClick:%u next_tick_midi:%u clickno:%d\n",
                syncFrame, _pos.frame(), midiClick, next_tick_midi, clickno);
              
              MusEGlobal::sigmap.tickValues(midiClick, &bar, &beat, &tick);
              MusEGlobal::sigmap.timesig(midiClick, z, n);
              // How many ticks per beat?
              const int ticks_beat = MusEGlobal::sigmap.ticks_beat(n);

              if (do_play && MusEGlobal::song->click() 
                  && (metro_settings->midiClickFlag)
                  && !precount_mute_metronome) {
                
                  if (tick == 0 && beat == 0) {
                      audioTickSound = MusECore::measureSound;
                      if (MusEGlobal::debugMsg)
                          fprintf(stderr, "meas: midiClick %d nextPos %d bar %d beat %d tick %d z %d n %d ticks_beat %d\n", 
                                  midiClick, next_tick_midi, bar, beat, tick, z, n, ticks_beat);
                  }
                  else if (tick == unsigned(ticks_beat - (ticks_beat/(n*2)))) {
                      audioTickSound = MusECore::accent2Sound;
                      if (MusEGlobal::debugMsg)
                          fprintf(stderr, "acc2: midiClick %d nextPos %d bar %d beat %d tick %d z %d n %d ticks_beat %d\n", 
                                  midiClick, next_tick_midi, bar, beat, tick, z, n, ticks_beat);
                  }
                  else if (tick == unsigned(ticks_beat - (ticks_beat/n))) {
                      audioTickSound = MusECore::accent1Sound;
                      if (MusEGlobal::debugMsg)
                          fprintf(stderr, "acc1: midiClick %d nextPos %d bar %d beat %d tick %d z %d n %d ticks_beat %d\n", 
                                  midiClick, next_tick_midi, bar, beat, tick, z, n, ticks_beat);
                  } else {
                      if (MusEGlobal::debugMsg)
                          fprintf(stderr, "beat: midiClick %d nextPos %d bar %d beat %d tick %d z %d n %d div %d\n", 
                                  midiClick, next_tick_midi, bar, beat, tick, z, n, ticks_beat);
                  }

                  MusECore::MidiPlayEvent ev(evtime, metro_settings->clickPort, metro_settings->clickChan,
                    MusECore::ME_NOTEON, metro_settings->beatClickNote, metro_settings->beatClickVelo);
                  if (audioTickSound == MusECore::measureSound) {
                    ev.setA(metro_settings->measureClickNote);
                    ev.setB(metro_settings->measureClickVelo);
                  }
                  if (audioTickSound == MusECore::accent1Sound) {
                    ev.setA(metro_settings->accentClick1);
                    ev.setB(metro_settings->accentClick1Velo);
                  }
                  if (audioTickSound == MusECore::accent2Sound) {
                    ev.setA(metro_settings->accentClick2);
                    ev.setB(metro_settings->accentClick2Velo);
                  }

                  // Should the metronome be played after precount?
                  if(!precount_mute_metronome)
                  {
                    // Don't bother sending to midi out if velocity is zero.
                    if (metro_settings->midiClickFlag && md && ev.dataB() > 0) {
                      MusECore::MidiPlayEvent evmidi = ev;
                      
  #ifdef DEBUG_MIDI_TIMING_DIFFS
                      fprintf(stderr, "EVENT TIME DIFF:%u\n", evmidi.time() - _lastEvTime);
                      _lastEvTime = evmidi.time();
  #endif
            
                      md->putEvent(evmidi, MidiDevice::NotLate, MidiDevice::PlaybackBuffer);
                      // Internal midi paths are now all note off aware. Driver handles note offs. Convert.
                      // Ticksynth has been modified too.
                      evmidi.setType(MusECore::ME_NOTEOFF);
                      evmidi.setB(0);
                      evmidi.setTime(midiClick+10);
                      md->addStuckNote(evmidi);
                    }
                  }
              }

              const int beat_mod = (beat + 1) % z;
              
              MetroAccent::AccentTypes_t acc_types = MetroAccent::NoAccent;
              if(metro_settings->metroAccentsMap)
              {
                MusECore::MetroAccentsMap::const_iterator imap = metro_settings->metroAccentsMap->find(z);
                if(imap != metro_settings->metroAccentsMap->cend())
                {
                  const MusECore::MetroAccentsStruct& mas = imap->second;
                  accents = &mas._accents;
                  accents_sz = accents->size();
                  if(beat_mod < accents_sz)
                    acc_types = accents->at(beat_mod)._accentType;
                }
              }

              // State machine to select next midiClick position.
              if (metro_settings->clickSamples == MetronomeSettings::newSamples) {
                  if (tick == 0) {//  ON key
                    if(acc_types & MetroAccent::Accent1)
                    {
                      // Cue an accent 1. (This part 'triggers' an accent sequence to begin
                      //  which automatically cues accent 2 if required then a normal beat, as shown below.)
                      midiClick = MusEGlobal::sigmap.bar2tick(bar, beat, ticks_beat - ((ticks_beat/n)));
                    }
                    else if(acc_types & MetroAccent::Accent2)
                    {
                      // Cue accent 2.
                      midiClick = MusEGlobal::sigmap.bar2tick(bar, beat, ticks_beat - (ticks_beat/(n*2)));
                    }
                    else
                    {
                      // Cue a normal beat.
                      midiClick = MusEGlobal::sigmap.bar2tick(bar, beat+1, 0);
                    }
                  }
                  else if (tick >= unsigned(ticks_beat - (ticks_beat/(n*2)))) { // second accent tick
                      // Finished accent 2. Cue a normal beat.
                      midiClick = MusEGlobal::sigmap.bar2tick(bar, beat+1, 0);
                  }
                  else if (tick < unsigned(ticks_beat - ((ticks_beat/(n*2))))) { // first accent tick
                      if(acc_types & MetroAccent::Accent2)
                        // Finished accent 1. Cue accent 2.
                        midiClick = MusEGlobal::sigmap.bar2tick(bar, beat, ticks_beat - (ticks_beat/(n*2)));
                      else
                        // Finished accent 1. Cue a normal beat.
                        midiClick = MusEGlobal::sigmap.bar2tick(bar, beat+1, 0);
                  }
              }
              else
              {
                midiClick = MusEGlobal::sigmap.bar2tick(bar, beat+1, 0);
              }
            }
      }
}

//---------------------------------------------------------
//   processAudioMetronome
//---------------------------------------------------------

void Audio::processAudioMetronome(unsigned int frames)
{
      const MusECore::MetronomeSettings* metro_settings = 
      MusEGlobal::metroUseSongSettings ? &MusEGlobal::metroSongSettings : &MusEGlobal::metroGlobalSettings;

      const bool extsync = MusEGlobal::extSyncFlag;
      const bool playing = isPlaying();

      // Should the metronome be muted after precount?
      const bool precount_mute_metronome = metro_settings->precountEnableFlag
        && MusEGlobal::song->click()
        && !extsync
        && ((!MusEGlobal::song->record() && metro_settings->precountOnPlay) || MusEGlobal::song->record())
        && metro_settings->precountMuteMetronome;
      
      if (playing)
      {
            int bar, beat, z, n;
            unsigned tick;
            AudioTickSound audioTickSound = MusECore::beatSound;
            const MusECore::MetroAccents* accents;
            int accents_sz;
            
            unsigned int lat_offset = 0;
            unsigned int cur_tick = curTickPos;
            unsigned int next_tick = nextTickPos;

            //--------------------------------------------------------------------
            // Account for the metronome's latency correction and/or compensation.
            //--------------------------------------------------------------------
            // TODO How to handle when external sync is on. For now, don't try to correct.
            if(MusEGlobal::config.enableLatencyCorrection && !extsync)
            {
              if(metro_settings->audioClickFlag)
              {
                const TrackLatencyInfo& li = metronome->getLatencyInfo(false);
                // This value is negative for correction.
                const float mlat = li._sourceCorrectionValue;
                if((int)mlat < 0)
                {
                  // Convert to a positive offset.
                  const unsigned int l = (unsigned int)(-mlat);
                  if(l > lat_offset)
                    lat_offset = l;
                }
                if(lat_offset != 0)
                {
                  Pos ppp(_pos.frame() + lat_offset, false);
                  cur_tick = ppp.tick();
                  ppp += frames;
                  next_tick = ppp.tick();
                }
              }
            }

            // What is the current transport frame, adjusted?
            const unsigned int pos_fr = _pos.frame() + lat_offset;
            // What is the (theoretical) next transport frame?
            const unsigned int next_pos_fr = pos_fr + frames;

            // If external sync is not on, we can take advantage of frame accuracy but
            //  first we must allow the next tick position to be included in the search
            //  even if it is equal to the current tick position.
            while (extsync ? (audioClick < next_tick) : (audioClick <= next_tick))
            {
              bool do_play = true;
              unsigned int evtime = 0;
              if(extsync)
              {
                if(audioClick < cur_tick)
                  audioClick = cur_tick;
                evtime = extClockHistoryTick2Frame(audioClick - cur_tick) + MusEGlobal::segmentSize;
              }
              else
              {
                // What is the exact transport frame that the click should be played at?
                const unsigned int fr = MusEGlobal::tempomap.tick2frame(audioClick);
                // Is the click frame outside of the current transport frame range?
                if(fr < pos_fr || fr >= next_pos_fr)
                {
                  // Break out of the loop if midiClick equals nextTickPos.
                  if(audioClick == next_tick)
                    break;
                  // Continue on, but do not play any notes.
                  do_play = false;
                }
                evtime = fr - pos_fr;
                evtime += syncFrame;
              }
                  
              DEBUG_MIDI_METRONOME(stderr, 
                "Audio::processAudioMetronome: playing: syncFrame:%u _pos.frame():%u audioClick:%u next_tick:%u clickno:%d\n",
                syncFrame, _pos.frame(), audioClick, next_tick, clickno);
              
              MusEGlobal::sigmap.tickValues(audioClick, &bar, &beat, &tick);
              MusEGlobal::sigmap.timesig(audioClick, z, n);
              // How many ticks per beat?
              const int ticks_beat = MusEGlobal::sigmap.ticks_beat(n);

              if (do_play && MusEGlobal::song->click() 
                  && (metro_settings->audioClickFlag)
                  && !precount_mute_metronome) {
                
                  if (tick == 0 && beat == 0) {
                      audioTickSound = MusECore::measureSound;
                      if (MusEGlobal::debugMsg)
                          fprintf(stderr, "meas: audioClick %d next_tick %d bar %d beat %d tick %d z %d n %d ticks_beat %d\n", 
                                  audioClick, next_tick, bar, beat, tick, z, n, ticks_beat);
                  }
                  else if (tick == unsigned(ticks_beat - (ticks_beat/(n*2)))) {
                      audioTickSound = MusECore::accent2Sound;
                      if (MusEGlobal::debugMsg)
                          fprintf(stderr, "acc2: audioClick %d next_tick %d bar %d beat %d tick %d z %d n %d ticks_beat %d\n", 
                                  audioClick, next_tick, bar, beat, tick, z, n, ticks_beat);
                  }
                  else if (tick == unsigned(ticks_beat - (ticks_beat/n))) {
                      audioTickSound = MusECore::accent1Sound;
                      if (MusEGlobal::debugMsg)
                          fprintf(stderr, "acc1: audioClick %d next_tick %d bar %d beat %d tick %d z %d n %d ticks_beat %d\n", 
                                  audioClick, next_tick, bar, beat, tick, z, n, ticks_beat);
                  } else {
                      if (MusEGlobal::debugMsg)
                          fprintf(stderr, "beat: audioClick %d next_tick %d bar %d beat %d tick %d z %d n %d div %d\n", 
                                  audioClick, next_tick, bar, beat, tick, z, n, ticks_beat);
                  }

                  // Should the metronome be played after precount?
                  if(!precount_mute_metronome)
                  {
                    if (metro_settings->audioClickFlag) {
                      MusECore::MidiPlayEvent ev(evtime, 0, 0,
                        MusECore::ME_NOTEON, audioTickSound, 0);
                      DEBUG_MIDI_METRONOME(stderr, "Audio::processAudioMetronome: playing: metronome->putEvent\n");
                      metronome->putEvent(ev, MidiDevice::NotLate, MidiDevice::PlaybackBuffer);
                      // Built-in metronome synth does not use stuck notes...
                    }
                  }
              }

              const int beat_mod = (beat + 1) % z;
              
              MetroAccent::AccentTypes_t acc_types = MetroAccent::NoAccent;
              if(metro_settings->metroAccentsMap)
              {
                MusECore::MetroAccentsMap::const_iterator imap = metro_settings->metroAccentsMap->find(z);
                if(imap != metro_settings->metroAccentsMap->cend())
                {
                  const MusECore::MetroAccentsStruct& mas = imap->second;
                  accents = &mas._accents;
                  accents_sz = accents->size();
                  if(beat_mod < accents_sz)
                    acc_types = accents->at(beat_mod)._accentType;
                }
              }

              // State machine to select next midiClick position.
              if (metro_settings->clickSamples == MetronomeSettings::newSamples) {
                  if (tick == 0) {//  ON key
                    if(acc_types & MetroAccent::Accent1)
                    {
                      // Cue an accent 1. (This part 'triggers' an accent sequence to begin
                      //  which automatically cues accent 2 if required then a normal beat, as shown below.)
                      audioClick = MusEGlobal::sigmap.bar2tick(bar, beat, ticks_beat - ((ticks_beat/n)));
                    }
                    else if(acc_types & MetroAccent::Accent2)
                    {
                      // Cue accent 2.
                      audioClick = MusEGlobal::sigmap.bar2tick(bar, beat, ticks_beat - (ticks_beat/(n*2)));
                    }
                    else
                    {
                      // Cue a normal beat.
                      audioClick = MusEGlobal::sigmap.bar2tick(bar, beat+1, 0);
                    }
                  }
                  else if (tick >= unsigned(ticks_beat - (ticks_beat/(n*2)))) { // second accent tick
                      // Finished accent 2. Cue a normal beat.
                      audioClick = MusEGlobal::sigmap.bar2tick(bar, beat+1, 0);
                  }
                  else if (tick < unsigned(ticks_beat - ((ticks_beat/(n*2))))) { // first accent tick
                      if(acc_types & MetroAccent::Accent2)
                        // Finished accent 1. Cue accent 2.
                        audioClick = MusEGlobal::sigmap.bar2tick(bar, beat, ticks_beat - (ticks_beat/(n*2)));
                      else
                        // Finished accent 1. Cue a normal beat.
                        audioClick = MusEGlobal::sigmap.bar2tick(bar, beat+1, 0);
                  }
              }
              else
              {
                audioClick = MusEGlobal::sigmap.bar2tick(bar, beat+1, 0);
              }
            }
      }
}


} // namespace MusECore
