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
#include "minstrument.h"
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
#include "mpevent.h"

// REMOVE Tim. Persistent routes. Added. Make this permanent later if it works OK and makes good sense.
#define _USE_MIDI_ROUTE_PER_CHANNEL_

// Undefine if and when multiple output routes are added to midi tracks.
#define _USE_MIDI_TRACK_SINGLE_OUT_PORT_CHAN_


namespace MusECore {

extern void dump(const unsigned char* p, int n);


const unsigned char gmOnMsg[]   = { 0x7e, 0x7f, 0x09, 0x01 };
const unsigned char gm2OnMsg[]  = { 0x7e, 0x7f, 0x09, 0x03 };
const unsigned char gmOffMsg[]  = { 0x7e, 0x7f, 0x09, 0x02 };
const unsigned char gsOnMsg[]   = { 0x41, 0x10, 0x42, 0x12, 0x40, 0x00, 0x7f, 0x00, 0x41 };
const unsigned char gsOnMsg2[]  = { 0x41, 0x10, 0x42, 0x12, 0x40, 0x01, 0x33, 0x50, 0x3c };
const unsigned char gsOnMsg3[]  = { 0x41, 0x10, 0x42, 0x12, 0x40, 0x01, 0x34, 0x50, 0x3b };
const unsigned char xgOnMsg[]   = { 0x43, 0x10, 0x4c, 0x00, 0x00, 0x7e, 0x00 };
const unsigned int  gmOnMsgLen  = sizeof(gmOnMsg);
const unsigned int  gm2OnMsgLen = sizeof(gm2OnMsg);
const unsigned int  gmOffMsgLen = sizeof(gmOffMsg);
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
//    sysexDuration
//    static
//---------------------------------------------------------

unsigned int sysexDuration(unsigned int len)
{
  // Midi transmission characters per second, based on standard fixed bit rate of 31250 Hz.
  // According to ALSA (aplaymidi.c), although the midi standard says one stop bit,
  //  two are commonly used. We will use two just to be sure.
  const unsigned int midi_cps = 31250 / (1 + 8 + 2);
  // Estimate the number of audio frames it should take (or took) to transmit the current midi chunk.
  unsigned int frames = (len * MusEGlobal::sampleRate) / midi_cps;
  // Add a slight delay between chunks just to be sure there's no overlap, rather a small space, and let devices catch up.
  frames += MusEGlobal::sampleRate / 200; // 1 / 200 = 5 milliseconds.
  // Let's be realistic, spread by at least one frame.
  if(frames == 0)
    frames = 1;
  return frames;
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
      if(port >= 0 && port < MIDI_PORTS)
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

            // REMOVE Tim. autoconnect. Added.
            fprintf(stderr, "buildMidiEventList tick:%d dataA:%d dataB:%d\n",
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
                              case ME_META_TEXT_1_COMMENT: // Text
                                    if (track->comment().isEmpty())
                                          track->setComment(QString((const char*)data));
                                    else
                                          track->setComment(track->comment() + "\n" + QString((const char*)data));
                                    break;
                              case ME_META_TEXT_3_TRACK_NAME: // Sequence-/TrackName
                                    track->setName(QString((char*)data));
                                    break;
                              case ME_META_TEXT_6_MARKER:   // Marker
                                    {
                                    unsigned ltick  = CALC_TICK(tick);
                                    MusEGlobal::song->addMarker(QString((const char*)(data)), ltick, false);
                                    }
                                    break;
                              case ME_META_TEXT_5_LYRIC:   // Lyrics
                              case ME_META_TEXT_8:   // text
                              case ME_META_TEXT_9_DEVICE_NAME:
                              case ME_META_TEXT_A:
                                    break;
                              case ME_META_TEXT_F_TRACK_COMMENT:        // Track Comment
                                    track->setComment(QString((char*)data));
                                    break;
                              case ME_META_SET_TEMPO:        // Tempo
                                    {
                                    unsigned tempo = data[2] + (data[1] << 8) + (data[0] <<16);
                                    unsigned ltick  = CALC_TICK(tick);
                                    // FIXME: After ca 10 mins 32 bits will not be enough... This expression has to be changed/factorized or so in some "sane" way...
                                    MusEGlobal::tempomap.addTempo(ltick, tempo);
                                    }
                                    break;
                              case ME_META_TIME_SIGNATURE:        // Time Signature
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
                              case ME_META_KEY_SIGNATURE:  // Key Signature
                                    break;
                              default:
                                    fprintf(stderr, "buildMidiEventList: unknown Meta 0x%x %d unabsorbed, adding instead to track:%s\n", ev.dataA(), ev.dataA(), track->name().toLatin1().constData());
                                    e.setType(Meta);
                                    e.setA(ev.dataA());
                                    e.setData(ev.data(), ev.len());
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
//   To be called by audio thread only.
//---------------------------------------------------------

// REMOVE Tim. autoconnect. Changed.
// void Audio::sendLocalOff()
//       {
//       for (int k = 0; k < MIDI_PORTS; ++k) {
//             for (int i = 0; i < MIDI_CHANNELS; ++i)
//                   MusEGlobal::midiPorts[k].sendEvent(MusECore::MidiPlayEvent(0, k, i, MusECore::ME_CONTROLLER, MusECore::CTRL_LOCAL_OFF, 0), true);
//             }
//       }
void Audio::sendLocalOff()
      {
      MidiPlayEvent ev;
      ev.setTime(0);  // Immediate processing. TODO Use curFrame?
      ev.setType(MusECore::ME_CONTROLLER);
      ev.setA(MusECore::CTRL_LOCAL_OFF);
      ev.setB(0);
      for (int k = 0; k < MIDI_PORTS; ++k) {
            for (int i = 0; i < MIDI_CHANNELS; ++i)
            {
                  ev.setPort(k);
                  ev.setChannel(i);
                  MidiPort::eventFifos().put(MidiPort::PlayFifo, ev);
                  if(MusEGlobal::midiPorts[k].device())
                    //MusEGlobal::midiPorts[k].device()->eventFifos()->put(MidiDevice::PlayFifo, ev);
//                     MusEGlobal::midiPorts[k].device()->addScheduledEvent(ev);
                    MusEGlobal::midiPorts[k].device()->putEvent(ev, MidiDevice::NotLate);
            }
            }
      }

//---------------------------------------------------------
//   panic
//   To be called by audio thread only.
//---------------------------------------------------------

// REMOVE Tim. autoconnect. Changed.
// void Audio::panic()
//       {
//       for (int i = 0; i < MIDI_PORTS; ++i) {
//             MusECore::MidiPort* port = &MusEGlobal::midiPorts[i];
//             if (port == 0)   // ??
//                   continue;
//             for (int chan = 0; chan < MIDI_CHANNELS; ++chan) {
//                   if (MusEGlobal::debugMsg)
//                     fprintf(stderr, "send all sound of to midi port %d channel %d\n", i, chan);
//                   port->sendEvent(MusECore::MidiPlayEvent(0, i, chan, MusECore::ME_CONTROLLER, MusECore::CTRL_ALL_SOUNDS_OFF, 0), true);
//                   port->sendEvent(MusECore::MidiPlayEvent(0, i, chan, MusECore::ME_CONTROLLER, MusECore::CTRL_RESET_ALL_CTRL, 0), true);
//                   }
//             }
//       }
// void Audio::panic()
// {
//       const int l = 4;
//       unsigned char data[l];
//       data[0] = MUSE_SYNTH_SYSEX_MFG_ID;
//       data[1] = MUSE_SYSEX_SYSTEM_ID;
//       data[2] = MUSE_SYSEX_SYSTEM_PANIC_ID;
//       data[3] = MUSE_SYSEX_SYSTEM_PANIC_ALL_SOUNDS_OFF | MUSE_SYSEX_SYSTEM_PANIC_RESET_ALL_CTRL;
//         
//       for (int i = 0; i < MIDI_PORTS; ++i)
//       {
//         MusECore::MidiPort* port = &MusEGlobal::midiPorts[i];
//         MusECore::MidiPlayEvent panic_event(0, i, MusECore::ME_SYSEX, data, l);
//         port->sendEvent(panic_event, true);
//       }
// }
void Audio::panic()
      {
      MidiPlayEvent ev;
      ev.setTime(0);  // Immediate processing. TODO Use curFrame?
      ev.setType(MusECore::ME_CONTROLLER);
      ev.setB(0);

      // TODO Reset those controllers back to unknown!
      for (int i = 0; i < MIDI_PORTS; ++i) {
            MusECore::MidiPort* port = &MusEGlobal::midiPorts[i];
            for (int chan = 0; chan < MIDI_CHANNELS; ++chan) {
                  if (MusEGlobal::debugMsg)
                    fprintf(stderr, "send all sound of to midi port %d channel %d\n", i, chan);
                  
                  ev.setPort(i);
                  ev.setChannel(chan);

                  ev.setA(MusECore::CTRL_ALL_SOUNDS_OFF);
                  MidiPort::eventFifos().put(MidiPort::PlayFifo, ev);
                  if(port->device())
                    //port->device()->eventFifos()->put(MidiDevice::PlayFifo, ev);
//                     port->device()->addScheduledEvent(ev);
                    port->device()->putEvent(ev, MidiDevice::NotLate);
                  
                  ev.setA(MusECore::CTRL_RESET_ALL_CTRL);
                  MidiPort::eventFifos().put(MidiPort::PlayFifo, ev);
                  if(port->device())
                    //port->device()->eventFifos()->put(MidiDevice::PlayFifo, ev);
//                     port->device()->addScheduledEvent(ev);
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
      for (int i = 0; i < MIDI_PORTS; ++i) {
            MusEGlobal::midiPorts[i].sendPendingInitializations(force);
            }
      }

// REMOVE Tim. autoconnect. Added.
//---------------------------------------------------------
//   seekMidi
//   Called from audio thread only.
//---------------------------------------------------------

void Audio::seekMidi()
{
  unsigned pos = MusEGlobal::audio->tickPos();
  //const bool playing = isPlaying() || (MusEGlobal::extSyncFlag.value() && MusEGlobal::midiSyncContainer.externalPlayState());
  const bool playing = isPlaying();
  
  // Bit-wise channels that are used.
  int used_ports[MIDI_PORTS];
  // Initialize the array.
  for(int i = 0; i < MIDI_PORTS; ++i)
    used_ports[i] = 0;

  // Find all used channels on all used ports.
  bool drum_found = false;
  if(MusEGlobal::song->click() && 
     MusEGlobal::clickPort < MIDI_PORTS &&
     MusEGlobal::clickChan < MIDI_CHANNELS)
    used_ports[MusEGlobal::clickPort] |= (1 << MusEGlobal::clickChan);
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

//       if((*i).port() != _port)
//         continue;
      MidiPlayEvent ev(*i);
      const int ev_port = ev.port();
      if(ev_port >= 0 && ev_port < MIDI_PORTS)
      {
        MidiPort* mp = &MusEGlobal::midiPorts[ev_port];
        ev.setTime(0);  // Immediate processing. TODO Use curFrame?
//         MusEGlobal::midiPorts[ev_port].putEvent(ev); // For immediate playback try putEvent, putMidiEvent, or sendEvent (for the optimizations).
        if(mp->device())
          //mp->device()->eventFifos()->put(MidiDevice::PlayFifo, ev);
//           mp->device()->addScheduledEvent(ev);
          mp->device()->putEvent(ev, MidiDevice::NotLate);
      }
      mel.erase(i);
    }

    
#ifdef _USE_MIDI_TRACK_SINGLE_OUT_PORT_CHAN_
//     const int port = mt->outPort();
//     const int channel = mt->outChannel();
//     if(port >= 0 && port < MIDI_PORTS && channel >= 0 && channel < MIDI_CHANNELS)
//       used_ports[port] |= (1 << channel);
    
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
//           if(mport != _port || usedChans[mchan])
//             continue;
//           usedChans[mchan] = true;
//           ++usedChanCount;
//           if(usedChanCount >= MIDI_CHANNELS)
//             break;  // All are used, done searching.
            
          if(mport >= 0 && mport < MIDI_PORTS && mchan >= 0 && mchan < MIDI_CHANNELS)
            used_ports[mport] |= (1 << mchan);
        }
      }
    }
    else
    {
//       if(mt->outPort() != _port || usedChans[mt->outChannel()])
//         continue;
//       usedChans[(*imt)->outChannel()] = true;
//       ++usedChanCount;
      
        const int mport = mt->outPort();
        const int mchan = mt->outChannel();
        if(mport >= 0 && mport < MIDI_PORTS && mchan >= 0 && mchan < MIDI_CHANNELS)
          used_ports[mport] |= (1 << mchan);
    }
//     if(usedChanCount >= MIDI_CHANNELS)
//       break;    // All are used. Done searching.
    
#else
    MusECore::RouteList* rl = mt->outRoutes();
    for(MusECore::ciRoute ir = rl->begin(); ir != rl->end(); ++ir)
    {
      switch(ir->type)
      {
        case MusECore::Route::MIDI_PORT_ROUTE:
        {
//           const int port = ir->midiPort;
//           const int channel = ir->channel;
//           if(port >= 0 && port < MIDI_PORTS && channel >= 0 && channel < MIDI_CHANNELS)
//             used_ports[port] |= (1 << channel);
          
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
      //           if(mport != _port || usedChans[mchan])
      //             continue;
      //           usedChans[mchan] = true;
      //           ++usedChanCount;
      //           if(usedChanCount >= MIDI_CHANNELS)
      //             break;  // All are used, done searching.
                  
                if(mport >= 0 && mport < MIDI_PORTS && mchan >= 0 && mchan < MIDI_CHANNELS)
                  used_ports[mport] |= (1 << mchan);
              }
            }
          }
          else
          {
      //       if(mt->outPort() != _port || usedChans[mt->outChannel()])
      //         continue;
      //       usedChans[(*imt)->outChannel()] = true;
      //       ++usedChanCount;
            
              const int mport = ir->midiPort;
              const int mchan = ir->channel;
              if(mport >= 0 && mport < MIDI_PORTS && mchan >= 0 && mchan < MIDI_CHANNELS)
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
  
  for(int i = 0; i < MIDI_PORTS; ++i)
  {
    if(used_ports[i] == 0)
      continue;
      
    MidiPort* mp = &MusEGlobal::midiPorts[i];
    MidiDevice* md = mp->device();
    
    //---------------------------------------------------
    //    Send STOP 
    //---------------------------------------------------
      
    // Don't send if external sync is on. The master, and our sync routing system will take care of that.  
    if(!MusEGlobal::extSyncFlag.value())
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
// REMOVE Tim. autoconnect. Changed.
//     if(md && (MusEGlobal::audio->isPlaying())
    if(md && playing)
      md->handleSeek();
    
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
      if(imcv != vl->end() && imcv->first == (int)pos)
      {
        for( ; imcv != vl->end() && imcv->first == (int)pos; ++imcv)
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

        // Don't bother sending any sustain values if not playing. Just set the hw state.
// REMOVE Tim. autoconnect. Changed.
//         if(fin_ctlnum == CTRL_SUSTAIN && !MusEGlobal::audio->isPlaying())
        if(fin_ctlnum == CTRL_SUSTAIN && !playing)
          fin_mp->setHwCtrlState(fin_chan, CTRL_SUSTAIN, imcv->second.val);
        else
        {
          // Use sendEvent to get the optimizations and limiting. But force if there's a value at this exact position.
          // NOTE: Why again was this forced? There was a reason. Think it was RJ in response to bug rep, then I modded.
          // A reason not to force: If a straight line is drawn on graph, multiple identical events are stored
          //  (which must be allowed). So seeking through them here sends them all redundantly, not good. // REMOVE Tim.
          //fprintf(stderr, "MidiDevice::handleSeek: found_value: calling sendEvent: ctlnum:%d val:%d\n", ctlnum, imcv->second.val);
//           fin_mp->sendEvent(MidiPlayEvent(0, fin_port, fin_chan, ME_CONTROLLER, fin_ctlnum, imcv->second.val), false); //, imcv->first == pos);
          const MidiPlayEvent ev(0, fin_port, fin_chan, ME_CONTROLLER, fin_ctlnum, imcv->second.val);
          MidiPort::eventFifos().put(MidiPort::PlayFifo, ev);
          if(fin_mp->device())
//             fin_mp->device()->addScheduledEvent(ev);
            fin_mp->device()->putEvent(ev, MidiDevice::NotLate);
          //mp->sendEvent(MidiPlayEvent(0, _port, chan, ME_CONTROLLER, ctlnum, imcv->second.val), pos == 0 || imcv->first == pos);
        }
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
            //fprintf(stderr, "Audio::handleSeek: !values_found: calling sendEvent: ctlnum:%d val:%d\n", ctlnum, mc->initVal() + mc->bias());
            // Use sendEvent to get the optimizations and limiting. No force sending. Note the addition of bias.
//             mp->sendEvent(MidiPlayEvent(0, i, chan, ME_CONTROLLER, ctlnum, mc->initVal() + mc->bias()), false);
            const MidiPlayEvent ev(0, i, chan, ME_CONTROLLER, ctlnum, mc->initVal() + mc->bias());
            MidiPort::eventFifos().put(MidiPort::PlayFifo, ev);
            if(mp->device())
//               mp->device()->addScheduledEvent(ev);
              mp->device()->putEvent(ev, MidiDevice::NotLate);
          }
        }
      }
      
      //---------------------------------------------------
      //    reset sustain
      //---------------------------------------------------
      
      for(int ch = 0; ch < MIDI_CHANNELS; ++ch) 
      {
        if(mp->hwCtrlState(ch, CTRL_SUSTAIN) == 127) 
        {
          const MidiPlayEvent ev(0, i, ch, ME_CONTROLLER, CTRL_SUSTAIN, 0);
//           mp->putEvent(ev);
          MidiPort::eventFifos().put(MidiPort::PlayFifo, ev);
          if(mp->device())
//             mp->device()->addScheduledEvent(ev);
            mp->device()->putEvent(ev, MidiDevice::NotLate);
        }
      }
      
      //---------------------------------------------------
      //    Send STOP and "set song position pointer"
      //---------------------------------------------------
        
      // Don't send if external sync is on. The master, and our sync routing system will take care of that.  
      if(!MusEGlobal::extSyncFlag.value())
      {
        if(mp->syncInfo().MRTOut())
        {
          //mp->sendStop();   // Moved above
          int beat = (pos * 4) / MusEGlobal::config.division;
          mp->sendSongpos(beat);
        }    
      }
    }
  }
  
//   //---------------------------------------------------
//   //    Send STOP 
//   //---------------------------------------------------
//     
//   // Don't send if external sync is on. The master, and our sync routing system will take care of that.  
//   if(!MusEGlobal::extSyncFlag.value())
//   {
//     if(mp->syncInfo().MRTOut())
//     {
//       // Shall we check for device write open flag to see if it's ok to send?...
//       //if(!(rwFlags() & 0x1) || !(openFlags() & 1))
//       //if(!(openFlags() & 1))
//       //  continue;
//       mp->sendStop();
//     }    
//   }

//   //---------------------------------------------------
//   //    If playing, clear all notes and flush out any
//   //     stuck notes which were put directly to the device
//   //---------------------------------------------------
//   
//   if(MusEGlobal::audio->isPlaying()) 
//   {
//     _playEvents.clear();
//     for(iMPEvent i = _stuckNotes.begin(); i != _stuckNotes.end(); ++i) 
//     {
//       MidiPlayEvent ev(*i);
//       ev.setTime(0);
//       putEvent(ev);  // For immediate playback try putEvent, putMidiEvent, or sendEvent (for the optimizations).
//     }
//     _stuckNotes.clear();
//   }
  
  //---------------------------------------------------
  //    Send new controller values
  //---------------------------------------------------
    
//   // Find channels on this port used in the song...
//   bool usedChans[MIDI_CHANNELS];
//   int usedChanCount = 0;
//   for(int i = 0; i < MIDI_CHANNELS; ++i)
//     usedChans[i] = false;
//   if(MusEGlobal::song->click() && MusEGlobal::clickPort == _port)
//   {
//     usedChans[MusEGlobal::clickChan] = true;
//     ++usedChanCount;
//   }
//   bool drum_found = false;
//   for(ciMidiTrack imt = MusEGlobal::song->midis()->begin(); imt != MusEGlobal::song->midis()->end(); ++imt)
//   {
//     //------------------------------------------------------------
//     //    While we are at it, flush out any track-related playback stuck notes
//     //     (NOT 'live' notes) which were not put directly to the device
//     //------------------------------------------------------------
//     MPEventList& mel = (*imt)->stuckNotes;
//     for(iMPEvent i = mel.begin(), i_next = i; i != mel.end(); i = i_next)
//     {
//       ++i_next;
// 
//       if((*i).port() != _port)
//         continue;
//       MidiPlayEvent ev(*i);
//       ev.setTime(0);
//       putEvent(ev); // For immediate playback try putEvent, putMidiEvent, or sendEvent (for the optimizations).
//       mel.erase(i);
//     }
//     
//     if((*imt)->type() == MusECore::Track::DRUM)
//     {
//       if(!drum_found)
//       {
//         drum_found = true; 
//         for(int i = 0; i < DRUM_MAPSIZE; ++i)
//         {
//           // Default to track port if -1 and track channel if -1.
//           int mport = MusEGlobal::drumMap[i].port;
//           if(mport == -1)
//             mport = (*imt)->outPort();
//           int mchan = MusEGlobal::drumMap[i].channel;
//           if(mchan == -1)
//             mchan = (*imt)->outChannel();
//           if(mport != _port || usedChans[mchan])
//             continue;
//           usedChans[mchan] = true;
//           ++usedChanCount;
//           if(usedChanCount >= MIDI_CHANNELS)
//             break;  // All are used, done searching.
//         }
//       }
//     }
//     else
//     {
//       if((*imt)->outPort() != _port || usedChans[(*imt)->outChannel()])
//         continue;
//       usedChans[(*imt)->outChannel()] = true;
//       ++usedChanCount;
//     }
// 
//     if(usedChanCount >= MIDI_CHANNELS)
//       break;    // All are used. Done searching.
//   }   
  
//   for(iMidiCtrlValList ivl = cll->begin(); ivl != cll->end(); ++ivl) 
//   {
//     MidiCtrlValList* vl = ivl->second;
//     int chan = ivl->first >> 24;
//     if(!usedChans[chan])  // Channel not used in song?
//       continue;
//     int ctlnum = vl->num();
// 
//     // Find the first non-muted value at the given tick...
//     bool values_found = false;
//     bool found_value = false;
//     
//     iMidiCtrlVal imcv = vl->lower_bound(pos);
//     if(imcv != vl->end() && imcv->first == (int)pos)
//     {
//       for( ; imcv != vl->end() && imcv->first == (int)pos; ++imcv)
//       {
//         const Part* p = imcv->second.part;
//         if(!p)
//           continue;
//         // Ignore values that are outside of the part.
//         if(pos < p->tick() || pos >= (p->tick() + p->lenTick()))
//           continue;
//         values_found = true;
//         // Ignore if part or track is muted or off.
//         if(p->mute())
//           continue;
//         const Track* track = p->track();
//         if(track && (track->isMute() || track->off()))
//           continue;
//         found_value = true;
//         break;
//       }
//     }
//     else
//     {
//       while(imcv != vl->begin())
//       {
//         --imcv;
//         const Part* p = imcv->second.part;
//         if(!p)
//           continue;
//         // Ignore values that are outside of the part.
//         unsigned t = imcv->first;
//         if(t < p->tick() || t >= (p->tick() + p->lenTick()))
//           continue;
//         values_found = true;
//         // Ignore if part or track is muted or off.
//         if(p->mute())
//           continue;
//         const Track* track = p->track();
//         if(track && (track->isMute() || track->off()))
//           continue;
//         found_value = true;
//         break;
//       }
//     }
// 
//     if(found_value)
//     {
//       int fin_port = _port;
//       MidiPort* fin_mp = mp;
//       int fin_chan = chan;
//       int fin_ctlnum = ctlnum;
//       // Is it a drum controller event, according to the track port's instrument?
//       if(mp->drumController(ctlnum))
//       {
//         if(const Part* p = imcv->second.part)
//         {
//           if(Track* t = p->track())
//           {
//             if(t->type() == MusECore::Track::NEW_DRUM)
//             {
//               MidiTrack* mt = static_cast<MidiTrack*>(t);
//               int v_idx = ctlnum & 0x7f;
//               fin_ctlnum = (ctlnum & ~0xff) | mt->drummap()[v_idx].anote;
//               int map_port = mt->drummap()[v_idx].port;
//               if(map_port != -1)
//               {
//                 fin_port = map_port;
//                 fin_mp = &MusEGlobal::midiPorts[fin_port];
//               }
//               int map_chan = mt->drummap()[v_idx].channel;
//               if(map_chan != -1)
//                 fin_chan = map_chan;
//             }
//           }
//         }
//       }
// 
//       // Don't bother sending any sustain values if not playing. Just set the hw state.
//       if(fin_ctlnum == CTRL_SUSTAIN && !MusEGlobal::audio->isPlaying())
//         fin_mp->setHwCtrlState(fin_chan, CTRL_SUSTAIN, imcv->second.val);
//       else
//       {
//         // Use sendEvent to get the optimizations and limiting. But force if there's a value at this exact position.
//         // NOTE: Why again was this forced? There was a reason. Think it was RJ in response to bug rep, then I modded.
//         // A reason not to force: If a straight line is drawn on graph, multiple identical events are stored
//         //  (which must be allowed). So seeking through them here sends them all redundantly, not good. // REMOVE Tim.
//         //fprintf(stderr, "MidiDevice::handleSeek: found_value: calling sendEvent: ctlnum:%d val:%d\n", ctlnum, imcv->second.val);
//         fin_mp->sendEvent(MidiPlayEvent(0, fin_port, fin_chan, ME_CONTROLLER, fin_ctlnum, imcv->second.val), false); //, imcv->first == pos);
//         //mp->sendEvent(MidiPlayEvent(0, _port, chan, ME_CONTROLLER, ctlnum, imcv->second.val), pos == 0 || imcv->first == pos);
//       }
//     }
// 
//     // Either no value was found, or they were outside parts, or pos is in the unknown area before the first value.
//     // Send instrument default initial values.  NOT for syntis. Use midiState and/or initParams for that. 
//     //if((imcv == vl->end() || !done) && !MusEGlobal::song->record() && instr && !isSynti()) 
//     // Hmm, without refinement we can only do this at position 0, due to possible 'skipped' values outside parts, above.
//     if(!values_found && MusEGlobal::config.midiSendCtlDefaults && !MusEGlobal::song->record() && pos == 0 && instr && !isSynti())
//     {
//       MidiControllerList* mcl = instr->controller();
//       ciMidiController imc = mcl->find(vl->num());
//       if(imc != mcl->end())
//       {
//         MidiController* mc = imc->second;
//         if(mc->initVal() != CTRL_VAL_UNKNOWN)
//         {
//           //fprintf(stderr, "MidiDevice::handleSeek: !values_found: calling sendEvent: ctlnum:%d val:%d\n", ctlnum, mc->initVal() + mc->bias());
//           // Use sendEvent to get the optimizations and limiting. No force sending. Note the addition of bias.
//           mp->sendEvent(MidiPlayEvent(0, _port, chan, ME_CONTROLLER, ctlnum, mc->initVal() + mc->bias()), false);
//         }
//       }
//     }
//   }
  
//   //---------------------------------------------------
//   //    reset sustain
//   //---------------------------------------------------
//   
//   for(int ch = 0; ch < MIDI_CHANNELS; ++ch) 
//   {
//     if(mp->hwCtrlState(ch, CTRL_SUSTAIN) == 127) 
//     {
//       const MidiPlayEvent ev(0, _port, ch, ME_CONTROLLER, CTRL_SUSTAIN, 0);
//       putEvent(ev);
//     }
//   }
//   
//   //---------------------------------------------------
//   //    Send STOP and "set song position pointer"
//   //---------------------------------------------------
//     
//   // Don't send if external sync is on. The master, and our sync routing system will take care of that.  
//   if(!MusEGlobal::extSyncFlag.value())
//   {
//     if(mp->syncInfo().MRTOut())
//     {
//       //mp->sendStop();   // Moved above
//       int beat = (pos * 4) / MusEGlobal::config.division;
//       mp->sendSongpos(beat);
//     }    
//   }
}

//---------------------------------------------------------
//   extClockHistoryTick2Frame
//    Convert tick to frame using the external clock history list.
//    The function takes a tick relative to zero (ie. relative to the first event in a processing batch).
//    The returned clock frames occured during the previous audio cycle(s), so you may want to shift 
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

// unsigned int Audio::extClockHistoryFrame2Tick(unsigned int frame) const
// {
//   if(_extClockHistorySize == 0)
//   {
//     fprintf(stderr, "Error: Audio::extClockHistoryFrame2Tick(): empty list\n");
// //     return 0;
//     return curTickPos;
//   }
//   
//   const int div = MusEGlobal::config.division / 24;
//     
//   bool found = false;
//   unsigned int val = 0;
//   
//   for(int i = 0; i < _extClockHistorySize; ++i)
//   {
//     // REMOVE Tim. autoconnect. Added.
//     fprintf(stderr, "Audio::extClockHistoryFrame2Tick(): frame:%u i:%d _extClockHistory[i]._frame:%u\n", frame, i, _extClockHistory[i].frame());
//     if(_extClockHistory[i].frame() >= frame)
//     {
//       if(!found)
//       {
//         found = true;
//         unsigned int offset;
//         switch(_extClockHistory[i].externState())
//         {
//           case ExtMidiClock::ExternStarted:
//             offset = 0;
//           break;
//           
//           case ExtMidiClock::ExternContinued:
//             offset = curTickPos;
//           break;
//           
//           case ExtMidiClock::ExternStopped:
//           case ExtMidiClock::ExternStarting:
//           case ExtMidiClock::ExternContinuing:
//             fprintf(stderr, "Error: Audio::extClockHistoryFrame2Tick(): frame:%u i:%d externState is:%d\n", 
//                     frame, i, _extClockHistory[i].externState());
//             offset = curTickPos;
//           break;
//         }
//         
//         int clocks = 0;
//         //int clocks = 1;
//         if(i > 0)
//         {
//           for(int k = i - 1; k >= 0; --k)
//           {
//             if(!_extClockHistory[k].isPlaying())
//               break;
//             ++clocks;
//           }
//         }
//         val = offset + clocks * div;
//       }
//     }
//   }
//   if(found)
//     return val;
//   
//   fprintf(stderr, "Error: Audio::extClockHistoryFrame2Tick(): frame:%u out of range. Returning next tick:%u (clock:%d)\n", 
//           frame, _extClockHistorySize * div, _extClockHistorySize);
//   
//   int clocks = 0;
//   //int clocks = 1;
//   for(int k = _extClockHistorySize - 1; k >= 0; --k)
//   {
//     if(!_extClockHistory[k].isPlaying())
//       break;
//     ++clocks;
//   }
//   val = clocks * div;
//   return val;
// }

unsigned int Audio::extClockHistoryFrame2Tick(unsigned int frame) const
{
  if(_extClockHistorySize == 0)
  {
    fprintf(stderr, "Error: Audio::extClockHistoryFrame2Tick(): empty list\n");
//     return 0;
    return curTickPos;
  }
  
  const unsigned int div = MusEGlobal::config.division / 24;
    
  bool found = false;
  unsigned int val = 0;
  
  for(int i = _extClockHistorySize - 1; i >= 0; --i)
  {
    // REMOVE Tim. autoconnect. Added.
    fprintf(stderr, "Audio::extClockHistoryFrame2Tick(): frame:%u i:%d _extClockHistory[i]._frame:%u\n", 
            frame, i, _extClockHistory[i].frame());
    if(_extClockHistory[i].frame() <= frame)
    {
      if(!found)
      {
        found = true;
        int clocks = 0;
        unsigned int offset = curTickPos;
        
        //if(i > 0)
        //{
          //for(int k = i - 1; k >= 0; --k)
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
        //}
        val = offset + clocks * div;
      }
    }
  }
  if(found)
    return val;
  
  fprintf(stderr, "Error: Audio::extClockHistoryFrame2Tick(): frame:%u out of range. Returning zero. _extClockHistorySize:%u\n", 
          frame, _extClockHistorySize);
  
  //return 0;
  //unsigned int offset = curTickPos;
  //if(_extClockHistory[0].isFirstClock())
  //{
  //  if(_extClockHistory[0].externState() == ExtMidiClock::ExternStarted)
  //    offset = 0;
  //}
  //return offset;
  
  //if(_extClockHistory[0].externState() == ExtMidiClock::ExternStarted)
  //{
  // We don't know the state of the last clock, we can only assume it was playing.
    if(curTickPos >= div)
      return curTickPos - div;
  //}
  return curTickPos;
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

      MidiPort* mp = &MusEGlobal::midiPorts[port];
      MidiDevice* md = mp->device();

      PartList* pl = track->parts();
      for (iPart p = pl->begin(); p != pl->end(); ++p) {
            MusECore::MidiPart* part = (MusECore::MidiPart*)(p->second);
            // dont play muted parts
            if (part->mute())
                  continue;
            const EventList& events = part->events();
            unsigned partTick = part->tick();
            unsigned partLen  = part->lenTick();
            int delay         = track->delay;

            if (cts > nts) {
                  fprintf(stderr, "processMidi: FATAL: cur > next %d > %d\n",
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

            ciEvent ie   = events.lower_bound(stick);
            ciEvent iend = events.lower_bound(etick);

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
                  else if (track->type() == Track::NEW_DRUM) {
                        int instr = ev.pitch();
                        // ignore muted drums
                        if (ev.isNote() && track->drummap()[instr].mute)
                              continue;
                        }
                  unsigned tick  = ev.tick() + offset;
                  
// REMOVE Tim. autoconnect. Changed.                  
//                   unsigned frame = MusEGlobal::tempomap.tick2frame(tick) + frameOffset;
                  //-----------------------------------------------------------------
                  // Determining the playback scheduling frame from the event's tick:
                  //-----------------------------------------------------------------
                  unsigned frame;
                  if(MusEGlobal::extSyncFlag.value())
                    // If external sync is on, look up the scheduling frame from the tick,
                    //  in the external clock history list (which is cleared, re-composed, and processed each cycle).
                    // The function takes a tick relative to zero (ie. relative to the first event in this batch).
                    // The returned clock frame occured during the previous audio cycle(s), so shift the frame 
                    //  forward by one audio segment size.
                    frame = extClockHistoryTick2Frame(tick - stick) + MusEGlobal::segmentSize;
                  else
                  {
                    // If external sync is off, look up the scheduling frame from our tempo list
                    //  ie. normal playback.
// REMOVE Tim. autoconnect. Changed.
                    //frame = MusEGlobal::tempomap.tick2frame(tick) + frameOffset;
                    const unsigned int fr = MusEGlobal::tempomap.tick2frame(tick);
                    const unsigned int pos_fr = pos().frame();
                    frame = (fr < pos_fr) ? 0 : fr - pos_fr;
                    frame += syncFrame;
                  }
                  
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
                              if (len <= 0)     // dont allow zero length
                                    len = 1;

                              if (port == defaultPort) {
                                    if (md) {
// REMOVE Tim. autoconnect. Removed.                                      
//                                         // If syncing to external midi sync, we cannot use the tempo map.
//                                         // Therefore we cannot get sub-tick resolution. Just use ticks instead of frames. p3.3.25
//                                         if(MusEGlobal::extSyncFlag.value())
//                                           md->addScheduledEvent(MusECore::MidiPlayEvent(tick, port, channel, MusECore::ME_NOTEON, pitch, velo));
//                                         else
//                                           md->addScheduledEvent(MusECore::MidiPlayEvent(frame, port, channel, MusECore::ME_NOTEON, pitch, velo));
//                                           md->putEvent(MusECore::MidiPlayEvent(
//                                           md->putPlaybackEvent(MusECore::MidiPlayEvent(
//                                             frame, port, channel, MusECore::ME_NOTEON, pitch, velo), MidiDevice::NotLate);
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
// REMOVE Tim. autoconnect. Removed.                                      
//                                         if(MusEGlobal::extSyncFlag.value())  // p3.3.25
//                                           mdAlt->addScheduledEvent(MusECore::MidiPlayEvent(tick, port, channel, MusECore::ME_NOTEON, pitch, velo));
//                                         else
//                                           mdAlt->addScheduledEvent(MusECore::MidiPlayEvent(frame, port, channel, MusECore::ME_NOTEON, pitch, velo));
//                                           mdAlt->putEvent(MusECore::MidiPlayEvent(
//                                           mdAlt->putPlaybackEvent(MusECore::MidiPlayEvent(
//                                             frame, port, channel, MusECore::ME_NOTEON, pitch, velo), MidiDevice::NotLate);
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

// REMOVE Tim. autoconnect. Changed / added.
                                    
//                                     // If syncing to external midi sync, we cannot use the tempo map.
//                                     // Therefore we cannot get sub-tick resolution. Just use ticks instead of frames. p3.3.25
//                                     MusECore::MidiPlayEvent mpeAlt(MusEGlobal::extSyncFlag.value() ? tick : frame,
                                    MusECore::MidiPlayEvent mpeAlt(frame, port, channel, 
                                                                   MusECore::ME_CONTROLLER, 
                                                                   ctl | pitch,
                                                                   ev.dataB());
                                    
                                    MidiPort* mpAlt = &MusEGlobal::midiPorts[port];
                                    // TODO Maybe grab the flag from the 'Optimize Controllers' Global Setting,
                                    //       which so far was meant for (N)RPN stuff. For now, just force it.
//                                     if(mpAlt->sendHwCtrlState(mpeAlt, true))
                                    MidiPort::eventFifos().put(MidiPort::PlayFifo, mpeAlt);
//                                     {
                                      if(MidiDevice* mdAlt = mpAlt->device())
//                                         mdAlt->addScheduledEvent(mpeAlt);
//                                         mdAlt->putEvent(mpeAlt, MidiDevice::NotLate);
//                                         mdAlt->putPlaybackEvent(mpeAlt, MidiDevice::NotLate);
                                        mdAlt->putEvent(mpeAlt, MidiDevice::NotLate, MidiDevice::PlaybackBuffer);
//                                     }
                                    
//                                     MidiDevice* mdAlt = mpAlt->device();
//                                     if(mdAlt)
//                                     {
//                                       // If syncing to external midi sync, we cannot use the tempo map.
//                                       // Therefore we cannot get sub-tick resolution. Just use ticks instead of frames. p3.3.25
//                                       if(MusEGlobal::extSyncFlag.value())
//                                         mdAlt->addScheduledEvent(MusECore::MidiPlayEvent(tick, port, channel,
//                                                                              MusECore::ME_CONTROLLER, ctl | pitch, ev.dataB()));
//                                       else
//                                         mdAlt->addScheduledEvent(MusECore::MidiPlayEvent(frame, port, channel,
//                                                                              MusECore::ME_CONTROLLER, ctl | pitch, ev.dataB()));
//                                     }
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
                                    
// REMOVE Tim. autoconnect. Changed / added.
                                    
//                                     // If syncing to external midi sync, we cannot use the tempo map.
//                                     // Therefore we cannot get sub-tick resolution. Just use ticks instead of frames. p3.3.25
//                                     MusECore::MidiPlayEvent mpeAlt(MusEGlobal::extSyncFlag.value() ? tick : frame,
                                    MusECore::MidiPlayEvent mpeAlt(frame, port, channel,
                                                                   MusECore::ME_CONTROLLER,
                                                                   ctl | pitch,
                                                                   ev.dataB());
                                    
                                    MidiPort* mpAlt = &MusEGlobal::midiPorts[port];
                                    // TODO Maybe grab the flag from the 'Optimize Controllers' Global Setting,
                                    //       which so far was meant for (N)RPN stuff. For now, just force it.
//                                     if(mpAlt->sendHwCtrlState(mpeAlt, true))
                                    MidiPort::eventFifos().put(MidiPort::PlayFifo, mpeAlt);
//                                     {
                                      if(MidiDevice* mdAlt = mpAlt->device())
//                                         mdAlt->addScheduledEvent(mpeAlt);
//                                         mdAlt->putEvent(mpeAlt, MidiDevice::NotLate);
//                                         mdAlt->putPlaybackEvent(mpeAlt, MidiDevice::NotLate);
                                        mdAlt->putEvent(mpeAlt, MidiDevice::NotLate, MidiDevice::PlaybackBuffer);
//                                     }
                                    
//                                     MidiDevice* mdAlt = mpAlt->device();
//                                     if(mdAlt)
//                                     {
//                                       // If syncing to external midi sync, we cannot use the tempo map.
//                                       // Therefore we cannot get sub-tick resolution. Just use ticks instead of frames. p3.3.25
//                                       if(MusEGlobal::extSyncFlag.value())
//                                         mdAlt->addScheduledEvent(MusECore::MidiPlayEvent(tick, port, channel,
//                                                                              MusECore::ME_CONTROLLER, ctl | pitch, ev.dataB()));
//                                       else
//                                         mdAlt->addScheduledEvent(MusECore::MidiPlayEvent(frame, port, channel,
//                                                                              MusECore::ME_CONTROLLER, ctl | pitch, ev.dataB()));
//                                     }
                                    break;  // Break out.
                                  }
                                }
                                
//                                 // If syncing to external midi sync, we cannot use the tempo map.
//                                 // Therefore we cannot get sub-tick resolution. Just use ticks instead of frames. p3.3.25
//                                 MusECore::MidiPlayEvent mpe(MusEGlobal::extSyncFlag.value() ? tick : frame,
                                MusECore::MidiPlayEvent mpe(frame, port, channel, ev);
                                // TODO Maybe grab the flag from the 'Optimize Controllers' Global Setting,
                                //       which so far was meant for (N)RPN stuff. For now, just force it.
//                                 if(mp->sendHwCtrlState(mpe, true))
                                MidiPort::eventFifos().put(MidiPort::PlayFifo, mpe);
//                                 {
                                  if(md)
//                                     md->addScheduledEvent(mpe);
//                                     md->putEvent(mpe, MidiDevice::NotLate);
//                                     md->putPlaybackEvent(mpe, MidiDevice::NotLate);
                                    md->putEvent(mpe, MidiDevice::NotLate, MidiDevice::PlaybackBuffer);
//                                 }
                                
//                                 if(MusEGlobal::extSyncFlag.value())  // p3.3.25
//                                 {
//                                   md->addScheduledEvent(MusECore::MidiPlayEvent(tick, port, channel, ev));
//                                 }
//                                 else
//                                 {
//                                   md->addScheduledEvent(MusECore::MidiPlayEvent(frame, port, channel, ev));
//                                 }
                              }
                              break;

                        default:
                          
                              if(md)
                              {
//                                 // If syncing to external midi sync, we cannot use the tempo map.
//                                 // Therefore we cannot get sub-tick resolution. Just use ticks instead of frames. p3.3.25
//                                 md->addScheduledEvent(MusECore::MidiPlayEvent(
//                                     MusEGlobal::extSyncFlag.value() ? tick : frame,
//                                  md->addScheduledEvent(MusECore::MidiPlayEvent(frame, port, channel, ev));
//                                  md->putPlaybackEvent(MusECore::MidiPlayEvent(frame, port, channel, ev), MidiDevice::NotLate);
                                 md->putEvent(MusECore::MidiPlayEvent(frame, port, channel, ev), 
                                                  MidiDevice::NotLate, MidiDevice::PlaybackBuffer);
                                
//                                 if(MusEGlobal::extSyncFlag.value())  // p3.3.25
//                                   md->addScheduledEvent(MusECore::MidiPlayEvent(tick, port, channel, ev));
//                                 else
//                                   md->addScheduledEvent(MusECore::MidiPlayEvent(frame, port, channel, ev));
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

void Audio::processMidi()
      {
// REMOVE Tim. autoconnect. Removed.
//       MusEGlobal::midiBusy=true;

      // An experiment, to try and pass input through without having to rec-arm a track. Disabled for now.
      //const bool no_mute_midi_input = false;

      const bool extsync = MusEGlobal::extSyncFlag.value();
// REMOVE Tim. autoconnect. Added.
      //const bool playing = isPlaying() || (MusEGlobal::extSyncFlag.value() && MusEGlobal::midiSyncContainer.externalPlayState());
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
                      if(port >= 0 && port < MIDI_PORTS)
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
// REMOVE Tim. autoconnect. Changed.
//               MusEGlobal::midiPorts[port].sendHwCtrlState(MidiPlayEvent(ev)); // Don't care about return value.
              MidiPort::eventFifos().put(MidiPort::PlayFifo, ev);
          }
        }

// REMOVE Tim. autoconnect. Removed. Moved early into Audio::process.
//         md->collectMidiEvents();

        // Take snapshots of the current sizes of the recording fifos,
        //  because they may change while here in process, asynchronously.
        md->beforeProcess();

        //
        // --------- Handle midi events for audio tracks -----------
        //

        if(port < 0)
          continue;

        for(int chan = 0; chan < MIDI_CHANNELS; ++chan)
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
// REMOVE Tim. autoconnect. Changed.
//                   unsigned int rec_t = isPlaying() ? ev_t : pframe;
                  unsigned int rec_t = playing ? ev_t : pframe;

                  if(!MusEGlobal::automation)
                    continue;
                  AutomationType at = track->automationType();
                  // Unlike our built-in gui controls, there is not much choice here but to
                  //  just do this:
                  if ( (at == AUTO_WRITE) ||
// REMOVE Tim. autoconnect. Changed.
//                        (at == AUTO_READ && !MusEGlobal::audio->isPlaying()) ||
                       (at == AUTO_READ && !playing) ||
                       (at == AUTO_TOUCH) )
                    track->enableController(actrl, false);
// REMOVE Tim. autoconnect. Changed.
//                   if(isPlaying())
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
            int port = track->outPort();
            MidiDevice* md = MusEGlobal::midiPorts[port].device();
// REMOVE Tim. autoconnect. Removed.
//             if(md)
            {
              // only add track events if the track is unmuted and turned on
              if(!track->isMute() && !track->off())
              {
// REMOVE Tim. autoconnect. Changed.
//                 if(isPlaying() && (curTickPos < nextTickPos))
                if(playing && (curTickPos < nextTickPos))
                  collectEvents(track, curTickPos, nextTickPos);
              }
            }

            //
            //----------midi recording
            //
            const bool track_rec_flag = track->recordFlag();
            const bool track_rec_monitor = track->recMonitor(); // Separate monitor and record functions.

            if(track_rec_monitor || track_rec_flag)
            {
                  MPEventList& rl = track->mpevents;
                  MidiPort* tport = &MusEGlobal::midiPorts[port];
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

                        for(int channel = 0; channel < MIDI_CHANNELS; ++channel)
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
                            MidiRecFifo& rf = dev->recordEvents(MIDI_CHANNELS);
                            // Get the frozen snapshot of the size.
                            int count = dev->tmpRecordCount(MIDI_CHANNELS);

                            for(int i = 0; i < count; ++i)
                            {
                              MidiRecordEvent event(rf.peek(i));
                              event.setPort(port);
                              // dont't echo controller changes back to software
                              // synthesizer:
                              if(md && track_rec_monitor)
                              {
                                // Do not echo synth events back to the same synth instance under any circumstances,
                                //  not even if monitor (echo) is on.
                                if(!dev->isSynti() || dev != md)
                                {
                                  // All recorded events arrived in the previous period. Shift into this period for playback.
                                  unsigned int et = event.time();
  #ifdef _AUDIO_USE_TRUE_FRAME_
                                  unsigned int t = et - _previousPos.frame() + _pos.frame() + frameOffset;
  #else
// REMOVE Tim. autoconnect. Changed.
//                                   unsigned int t = et + frameOffset;
                                  //unsigned int t = et + syncFrame;
                                  // The events arrived in the previous period. Shift into this period for playback.
                                  // The events are already biased with the last frame time.
                                  unsigned int t = et + MusEGlobal::segmentSize;
                                  // Protection from slight errors in estimated frame time.
                                  if(t >= (syncFrame + MusEGlobal::segmentSize))
                                  {
                                    // REMOVE Tim. autoconnect. Added.
                                    fprintf(stderr, "Error: Audio::processMidi(): sysex: t:%u >= syncFrame:%u + segmentSize:%u (==%u)\n", 
                                            t, syncFrame, MusEGlobal::segmentSize, syncFrame + MusEGlobal::segmentSize);
                                    t = syncFrame + (MusEGlobal::segmentSize - 1);
                                  }
  #endif
                                  event.setTime(t);
//                                   md->addScheduledEvent(event);
                                  md->putEvent(event, MidiDevice::NotLate);
                                  event.setTime(et);  // Restore for recording.
                                }
                              }

                              unsigned int et = event.time();
                              // Make sure the event is recorded in units of ticks.
                              if(extsync)
                              {
                                const unsigned int xt = extClockHistoryFrame2Tick(event.time());
                                // REMOVE Tim. autoconnect. Added.
                                fprintf(stderr, "processMidi: event time:%d dataA:%d dataB:%d curTickPos:%u set time:%u\n",
                                                event.time(), event.dataA(), event.dataB(), curTickPos, xt);
                                // REMOVE Tim. autoconnect. Changed.
//                                 event.setTime(event.tick());  // HACK: Transfer the tick to the frame time
                                event.setTime(xt);
                              }
                              else
                              {
// REMOVE Tim. autoconnect. Changed.
//                                 event.setTime(MusEGlobal::tempomap.frame2tick(event.time()));
                                // All recorded events arrived in the previous period. Shift into this period for record.
#ifdef _AUDIO_USE_TRUE_FRAME_
                                unsigned int t = et - _previousPos.frame() + _pos.frame() + frameOffset;
#else
                                unsigned int t = et + MusEGlobal::segmentSize;
                                // Protection from slight errors in estimated frame time.
                                if(t >= (syncFrame + MusEGlobal::segmentSize))
                                {
                                  // REMOVE Tim. autoconnect. Added.
                                  fprintf(stderr, "Error: Audio::processMidi(): record sysex: t:%u >= syncFrame:%u + segmentSize:%u (==%u)\n", 
                                          t, syncFrame, MusEGlobal::segmentSize, syncFrame + MusEGlobal::segmentSize);
                                  t = syncFrame + (MusEGlobal::segmentSize - 1);
                                }
#endif
                                // Be sure to allow for some (very) late events, such as
                                //  the first chunk's time in a multi-chunk sysex.
                                const unsigned int a_fr = pos().frame() + t;
                                const unsigned int fin_fr = syncFrame > a_fr ? 0 : a_fr - syncFrame;
                                event.setTime(MusEGlobal::tempomap.frame2tick(fin_fr));
                              }

// REMOVE Tim. autoconnect. Changed.
//                               if(recording && track_rec_flag)
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
                                    mc = tport->drumController(ctl);
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
                                    mc = tport->drumController(ctl);
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

                                // dont't echo controller changes back to software
                                // synthesizer:

                                // Zero means zero. Should mean no note at all?
                                // If the event is marked as a note with zero velocity (above), do not sound the note.
                                if(!event.isNote() || event.dataB() != 0)
                                {

                                  // All recorded events arrived in previous period. Shift into this period for playback.
                                  //  frameoffset needed to make process happy.
                                  unsigned int et = event.time();
#ifdef _AUDIO_USE_TRUE_FRAME_
                                  unsigned int t = et - _previousPos.frame() + _pos.frame() + frameOffset;
#else
// REMOVE Tim. autoconnect. Changed.
//                                   unsigned int t = et + frameOffset;
                                  //unsigned int t = et + syncFrame;
                                  // The events arrived in the previous period. Shift into this period for playback.
                                  // The events are already biased with the last frame time.
                                  unsigned int t = et + MusEGlobal::segmentSize;
                                  // Protection from slight errors in estimated frame time.
                                  if(t >= (syncFrame + MusEGlobal::segmentSize))
                                  {
                                    // REMOVE Tim. autoconnect. Added.
                                    fprintf(stderr, "Error: Audio::processMidi(): event: t:%u >= syncFrame:%u + segmentSize:%u (==%u)\n", 
                                            t, syncFrame, MusEGlobal::segmentSize, syncFrame + MusEGlobal::segmentSize);
                                    t = syncFrame + (MusEGlobal::segmentSize - 1);
                                  }
#endif
                                  event.setTime(t);
                                  // Check if we're outputting to another port than default:
                                  if (devport == defaultPort) {
                                        event.setPort(port);
                                        if(md && track_rec_monitor && !track->off() && !track->isMute())
                                        {
                                          // Do not echo synth events back to the same synth instance under any circumstances,
                                          //  not even if monitor (echo) is on.
                                          if(!dev->isSynti() || dev != md)
                                          {
                                            MidiInstrument* minstr = MusEGlobal::midiPorts[port].instrument();
                                            const MidiInstrument::NoteOffMode nom = minstr->noteOffMode();
                                            // If the instrument has no note-off mode, do not use the stuck notes mechanism, send as is.
                                            // This allows drum input triggers (no note offs at all), although it is awkward to
                                            //  first have to choose an output instrument with no note-off mode.
                                            if(nom == MidiInstrument::NoteOffNone)
                                            {
                                              if(event.isNoteOff())
                                                // Try to remove any corresponding stuck live note.
                                                track->removeStuckLiveNote(port, event.channel(), event.dataA());
//                                               md->addScheduledEvent(event);
                                              md->putEvent(event, MidiDevice::NotLate);
                                            }
                                            else if(event.isNoteOff())
                                            {
                                              // Try to remove any corresponding stuck live note.
                                              // Only if a stuck live note existed do we schedule the note off to play.
                                              if(track->removeStuckLiveNote(port, event.channel(), event.dataA()))
//                                                 md->addScheduledEvent(event);
                                                md->putEvent(event, MidiDevice::NotLate);
                                            }
                                            else if(event.isNote())
                                            {
                                              // Check if a stuck live note exists on any track.
                                              ciMidiTrack it_other = mtl->begin();
                                              for( ; it_other != mtl->end(); ++it_other)
                                              {
                                                if((*it_other)->stuckLiveNoteExists(port, event.channel(), event.dataA()))
                                                  break;
                                              }
                                              // Only if NO stuck live note existed do we schedule the note on to play.
                                              if(it_other == mtl->end())
                                              {
                                                if(track->addStuckLiveNote(port, event.channel(), event.dataA()))
//                                                   md->addScheduledEvent(event);
                                                  md->putEvent(event, MidiDevice::NotLate);
                                              }
                                            }
                                            else
                                            {
// REMOVE Tim. autoconnect. Changed / added.
                                              // TODO Maybe grab the flag from the 'Optimize Controllers' Global Setting,
                                              //       which so far was meant for (N)RPN stuff. For now, just force it.
//                                               if(MusEGlobal::midiPorts[port].sendHwCtrlState(event), true)
                                              MidiPort::eventFifos().put(MidiPort::PlayFifo, event);
//                                                 md->addScheduledEvent(event);
                                                md->putEvent(event, MidiDevice::NotLate);

//                                               md->addScheduledEvent(event);
//                                               //else
//                                               //  MusEGlobal::midiPorts[port].sendHwCtrlState(event); // Don't care about return value.
                                            }
                                          }
                                        }
                                      }
                                  else {
                                        MidiDevice* mdAlt = MusEGlobal::midiPorts[devport].device();
                                        if(mdAlt && track_rec_monitor && !track->off() && !track->isMute())
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
//                                                   mdAlt->addScheduledEvent(event);
                                                  mdAlt->putEvent(event, MidiDevice::NotLate);
                                              }
                                            }
                                            else
                                            {
// REMOVE Tim. autoconnect. Changed / added.
                                              // TODO Maybe grab the flag from the 'Optimize Controllers' Global Setting,
                                              //       which so far was meant for (N)RPN stuff. For now, just force it.
//                                               if(MusEGlobal::midiPorts[devport].sendHwCtrlState(event), true)
                                              MidiPort::eventFifos().put(MidiPort::PlayFifo, event);
//                                                 mdAlt->addScheduledEvent(event);
                                                mdAlt->putEvent(event, MidiDevice::NotLate);
                                              
//                                               mdAlt->addScheduledEvent(event);
//                                               //else
//                                               //  MusEGlobal::midiPorts[devport].sendHwCtrlState(event); // Don't care about return value.
                                            }
                                          }
                                        }
                                      }
                                  event.setTime(et);  // Restore for recording.

                                // Shall we activate meters even while rec echo is off? Sure, why not...
                                if(event.isNote() && event.dataB() > track->activity())
                                  track->setActivity(event.dataB());
                              }

// REMOVE Tim. autoconnect. Changed.
//                               if (recording && track_rec_flag)
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
                                      // REMOVE Tim. autoconnect. Added.
                                      fprintf(stderr, "processMidi: event time:%d dataA:%d dataB:%d curTickPos:%u set time:%u\n",
                                                      event.time(), event.dataA(), event.dataB(), curTickPos, xt);
                                      // REMOVE Tim. autoconnect. Changed.
//                                       event.setTime(event.tick());  // HACK: Transfer the tick to the frame time
                                      event.setTime(xt);
                                    }
                                    else
                                    {
// REMOVE Tim. autoconnect. Changed.
//                                       event.setTime(MusEGlobal::tempomap.frame2tick(event.time()));
                                      // All recorded events arrived in the previous period. Shift into this period for record.
      #ifdef _AUDIO_USE_TRUE_FRAME_
                                      unsigned int t = et - _previousPos.frame() + _pos.frame() + frameOffset;
      #else
                                      unsigned int t = et + MusEGlobal::segmentSize;
                                      // Protection from slight errors in estimated frame time.
                                      if(t >= (syncFrame + MusEGlobal::segmentSize))
                                      {
                                        // REMOVE Tim. autoconnect. Added.
                                        fprintf(stderr, "Error: Audio::processMidi(): record event: t:%u >= syncFrame:%u + segmentSize:%u (==%u)\n", 
                                                t, syncFrame, MusEGlobal::segmentSize, syncFrame + MusEGlobal::segmentSize);
                                        t = syncFrame + (MusEGlobal::segmentSize - 1);
                                      }
      #endif
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
                                          drumRecEvent.setPort(port); //rec-event to current port
                                          drumRecEvent.setChannel(track->outChannel()); //rec-event to current channel
                                          track->mpevents.add(drumRecEvent);
                                      }
                                      else
                                      {
                                          MusECore::MidiPlayEvent drumRecEvent = event;
                                          drumRecEvent.setA(drumRecPitch);
                                          drumRecEvent.setB(preVelo);
                                          // Changed to 'port'. Events were not being recorded for a drum map entry pointing to a
                                          //  different port. That must have been wrong - buildMidiEventList would ignore that. Tim.
                                          drumRecEvent.setPort(port);  //rec-event to current port
                                          drumRecEvent.setChannel(track->outChannel()); //rec-event to current channel
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
                                          recEvent.setPort(port);
                                          recEvent.setChannel(track->outChannel());

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
// REMOVE Tim. autoconnect. Changed.
//                 mdev->putEvent(ev);
//                 mdev->putEvent(ev, MidiDevice::PlayFifo, MidiDevice::NotLate);
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
                if(k->time() >= nextTickPos)
                      break;
                MidiPlayEvent ev(*k);
// REMOVE Tim. autoconnect. Changed.                  
//                 if(extsync)              // p3.3.25
//                 {
// //                   ev.setTime(k->time());
//                   unsigned int evt = k->time();
//                   if(evt < curTickPos)
//                     evt = curTickPos;
//                   ev.setTime(extClockHistoryTick2Frame(evt - curTickPos) + MusEGlobal::segmentSize);
//                 }
//                 else
//                   ev.setTime(MusEGlobal::tempomap.tick2frame(k->time()) + frameOffset);
                ev.setTime(MusEGlobal::audio->midiQueueTimeStamp(k->time()));
                mport = ev.port();
                if(port < 0)
                  continue;
                mdev = MusEGlobal::midiPorts[mport].device();
                if(!mdev)
                  continue;
                //_playEvents.add(ev);
//                 mdev->addScheduledEvent(ev);
//                
                // TODO: DECIDE: Hm, we don't want the device to miss any note offs.
                //               So I guess schedule this as a user event rather than a playback event.
                mdev->putEvent(ev, MidiDevice::NotLate);
                //mdev->putPlaybackEvent(ev, MidiDevice::NotLate);
              }
              mel.erase(mel.begin(), k);
            }
          }

          // If no echo or off, or not rec-armed (or muted), we want to cancel all 'live' (rec) stuck notes immediately.
//           if(!track->recEcho() || track->off() ||
//              (no_mute_midi_input && (track->isMute())) ||
//              (!no_mute_midi_input && !track_rec_flag))
          // If no monitor or off, or not rec-armed (or muted), we want to cancel all 'live' (rec) stuck notes immediately.
          if(!track_rec_monitor || track->off() ||
             //(no_mute_midi_input && (track->isMute())) ||
             //(!no_mute_midi_input && !track_rec_flag))
             track->isMute())
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
// REMOVE Tim. autoconnect. Changed.
//                 mdev->putEvent(ev);
//                 mdev->putEvent(ev, MidiDevice::PlayFifo, MidiDevice::NotLate);
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

      MidiDevice* md = 0;
      if (MusEGlobal::midiClickFlag)
            md = MusEGlobal::midiPorts[MusEGlobal::clickPort].device();
// REMOVE Tim. autoconnect. Changed.
//       if (MusEGlobal::song->click() && (isPlaying() || state == PRECOUNT)) {
      if (MusEGlobal::song->click() && (playing || state == PRECOUNT)) {
            int bar, beat, z, n;
            unsigned tick;
            AudioTickSound audioTickSound = MusECore::beatSound;
            while (midiClick < nextTickPos) {
// REMOVE Tim. autoconnect. Changed.
//                   if (isPlaying()) {
                  if (playing) {
                    AL::sigmap.tickValues(midiClick, &bar, &beat, &tick);
                    AL::sigmap.timesig(midiClick, z, n);

                    //n = 2;
                    if (tick == 0 && beat == 0) {
                        audioTickSound = MusECore::measureSound;
                        if (MusEGlobal::debugMsg)
                            fprintf(stderr, "meas: midiClick %d nextPos %d bar %d beat %d tick %d z %d n %d div %d\n", midiClick, nextTickPos, bar, beat, tick, z, n, MusEGlobal::config.division);
                    }
                    else if (tick == unsigned(MusEGlobal::config.division - (MusEGlobal::config.division/(n*2)))) {
                        audioTickSound = MusECore::accent2Sound;
                        if (MusEGlobal::debugMsg)
                            fprintf(stderr, "acc2: midiClick %d nextPos %d bar %d beat %d tick %d z %d n %d div %d\n", midiClick, nextTickPos, bar, beat, tick, z, n, MusEGlobal::config.division);
                    }
                    else if (tick == unsigned(MusEGlobal::config.division - (MusEGlobal::config.division/n))) {
                        audioTickSound = MusECore::accent1Sound;
                        if (MusEGlobal::debugMsg)
                            fprintf(stderr, "acc1: midiClick %d nextPos %d bar %d beat %d tick %d z %d n %d div %d\n", midiClick, nextTickPos, bar, beat, tick, z, n, MusEGlobal::config.division);
                    } else {
                        if (MusEGlobal::debugMsg)
                            fprintf(stderr, "beat: midiClick %d nextPos %d bar %d beat %d tick %d z %d n %d div %d\n", midiClick, nextTickPos, bar, beat, tick, z, n, MusEGlobal::config.division);
                    }
                  }

                  else if (state == PRECOUNT) {
                    if ((clickno % clicksMeasure) == 0) {
                        audioTickSound = MusECore::measureSound;
                    }
                  }
// REMOVE Tim. autoconnect. Changed.                  
//                   int evtime = extsync ? midiClick : MusEGlobal::tempomap.tick2frame(midiClick) + frameOffset;  // p3.3.25
                  unsigned int evtime = MusEGlobal::audio->midiQueueTimeStamp(midiClick);

                  MusECore::MidiPlayEvent ev(evtime, MusEGlobal::clickPort, MusEGlobal::clickChan, MusECore::ME_NOTEON, MusEGlobal::beatClickNote, MusEGlobal::beatClickVelo);
                  if (audioTickSound == MusECore::measureSound) {
                    ev.setA(MusEGlobal::measureClickNote);
                    ev.setB(MusEGlobal::measureClickVelo);
                  }
                  if (audioTickSound == MusECore::accent1Sound) {
                    ev.setA(MusEGlobal::accentClick1);
                    ev.setB(MusEGlobal::accentClick1Velo);
                  }
                  if (audioTickSound == MusECore::accent2Sound) {
                    ev.setA(MusEGlobal::accentClick2);
                    ev.setB(MusEGlobal::accentClick2Velo);
                  }
                  if (md) {
                    MusECore::MidiPlayEvent evmidi = ev;
//                     md->addScheduledEvent(evmidi);
//                     md->putEvent(evmidi, MidiDevice::NotLate);
//                     md->putPlaybackEvent(evmidi, MidiDevice::NotLate);
                    md->putEvent(evmidi, MidiDevice::NotLate, MidiDevice::PlaybackBuffer);
                    // Internal midi paths are now all note off aware. Driver handles note offs. Convert.
                    // Ticksynth has been modified too.
                    evmidi.setType(MusECore::ME_NOTEOFF);
                    evmidi.setB(0);
                    evmidi.setTime(midiClick+10);
                    md->addStuckNote(evmidi);
                  }
                  if (MusEGlobal::audioClickFlag) {
                    ev.setA(audioTickSound);
//                     metronome->addScheduledEvent(ev);
//                     metronome->putEvent(ev, MidiDevice::NotLate);
//                     metronome->putPlaybackEvent(ev, MidiDevice::NotLate);
                    metronome->putEvent(ev, MidiDevice::NotLate, MidiDevice::PlaybackBuffer);
                    // Built-in metronome synth does not use stuck notes...
                  }
// REMOVE Tim. autoconnect. Changed.
//                   if (isPlaying()) {
                  if (playing) {
                      // State machine to select next midiClick position.
                      if (MusEGlobal::clickSamples == MusEGlobal::newSamples) {
                          if (tick == 0) {//  ON key
                            if (beat % 2)
                              midiClick = AL::sigmap.bar2tick(bar, beat+1, 0);
                            else
                              midiClick = AL::sigmap.bar2tick(bar, beat, MusEGlobal::config.division - ((MusEGlobal::config.division/n)));
                          }
                          else if (tick >= unsigned(MusEGlobal::config.division - (MusEGlobal::config.division/(n*2)))) { // second accent tick
                              midiClick = AL::sigmap.bar2tick(bar, beat+1, 0);
                          }
                          else if (tick < unsigned(MusEGlobal::config.division - ((MusEGlobal::config.division/(n*2))))) { // first accent tick
                              midiClick = AL::sigmap.bar2tick(bar, beat, MusEGlobal::config.division - (MusEGlobal::config.division/(n*2)));
                          }
                      }
                      else {
                        midiClick = AL::sigmap.bar2tick(bar, beat+1, 0);
                      }
                  }
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
        MidiDevice* pl_md = *id;
        // We are done with the 'frozen' recording fifos, remove the events.
        pl_md->afterProcess();

// REMOVE Tim. autoconnect. Added.
        pl_md->processStuckNotes();
        // Tell devices which process in another thread (like ALSA) to prepare if necessary,
        //  for example transfer the play events list to a fifo for the other thread to read.
//         pl_md->preparePlayEventFifo();
        
        // ALSA devices handled by another thread.
        const MidiDevice::MidiDeviceType typ = pl_md->deviceType();
        switch(typ)
        {
          case MidiDevice::ALSA_MIDI:
          break;

          case MidiDevice::JACK_MIDI:
          case MidiDevice::SYNTH_MIDI:
// REMOVE Tim. autoconnect. Changed.
//             pl_md->processMidi();
            // The frame is not used by these devices but we pass it along anyway.
            // Only ALSA devices need the frame.
            pl_md->processMidi(syncFrame);
          break;
        }
      }

//       //
//       // Receive events sent from our gui thread to this audio thread.
//       // Update hardware controller gui knobs, sliders etc.
//       //
//       for(int igmp = 0; igmp < MIDI_PORTS; ++igmp)
//         MusEGlobal::midiPorts[igmp].processGui2AudioEvents();
      // Receive hardware state events sent from various threads to this audio thread.
      // Update hardware state so gui controls are updated.
      // Static.
      MidiPort::processGui2AudioEvents();

// REMOVE Tim. autoconnect. Removed.
//       MusEGlobal::midiBusy=false;
      }

} // namespace MusECore
