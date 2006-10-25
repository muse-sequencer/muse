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

#include "sync.h"
#include "song.h"
#include "widgets/utils.h"
#include "globals.h"
#include "midiseq.h"
#include "audio.h"
#include "driver/audiodev.h"
#include "gconfig.h"
#include "al/tempo.h"
#include "al/al.h"

int rxSyncPort = -1;         // receive from all ports
int txSyncPort = 1;

bool debugSync = false;
MTC mtcOffset;
bool extSyncFlag = false;
bool genMTCSync  = false;      // output MTC Sync
bool genMCSync   = false;      // output MidiClock Sync
bool genMMC      = false;      // output Midi Machine Control
bool acceptMTC   = false;
bool acceptMC    = true;
bool acceptMMC   = true;

static MTC mtcCurTime;
static int mtcState;    // 0-7 next expected quarter message
static bool mtcValid;
static int mtcLost;
static bool mtcSync;    // receive complete mtc frame?

static bool mcStart = false;
static int mcStartTick;

enum {
      MMC_STOP = 1,
      MMC_PLAY = 2,
      MMC_DEFERRED_PLAY = 3,
      MMC_FAST_FORWARD = 4,
      MMC_REWIND = 5,
      MMC_RECORD_STROBE = 6,  // Punch In
      MMC_RECORD_EXIT = 7,    // Punch Out
      MMC_PAUSE = 9,
      MMC_RESET = 13,
      MMC_GOTO = 0x44
      };

//---------------------------------------------------------
//  mmcInput
//    Midi Machine Control Input received
//---------------------------------------------------------

void MidiSeq::mmcInput(int id, int cmd, const Pos& pos)
      {
#if 0
      int rxDeviceId = 127;

      if (!extSyncFlag || !acceptMMC || (id != 127 && id != rxDeviceId))
            return;

      if (debugMsg)
      	printf("mmcInput: id %d cmd %02x %02x\n", id, cmd, cmd);

      switch (cmd) {
            case MMC_STOP:
                  if (debugSync)
                        printf("  MMC: STOP\n");
                  if (audio->isPlaying())
            		audioDriver->stopTransport();
            	else
                  	// reset REC
				audio->sendMsgToGui(MSG_STOP);
                  break;
            case MMC_PLAY:
            case MMC_DEFERRED_PLAY:
            	audioDriver->startTransport();
                  break;
            case MMC_FAST_FORWARD:
                  printf("MMC: FF not implemented\n");
                  break;
            case MMC_REWIND:
                  printf("MMC: REWIND not implemented\n");
                  break;
            case MMC_RECORD_STROBE:
                  printf("MMC: REC STROBE not implemented\n");
                  break;
            case MMC_RECORD_EXIT:
                  printf("MMC: REC EXIT not implemented\n");
                  break;
            case MMC_PAUSE:
			audio->sendMsgToGui(MSG_RECORD);
            	break;
            case MMC_RESET:
                  printf("MMC: RESET not implemented\n");
                  break;

            case MMC_GOTO:
                  audioDriver->seekTransport(pos.frame());
                  break;

            default:
                  printf("MMC id %x cmd %x, unknown\n", id, cmd);
                  break;
            }
#endif
      }

//---------------------------------------------------------
//   mtcInputQuarter
//    process Quarter Frame Message
//---------------------------------------------------------

void MidiSeq::mtcInputQuarter(int, unsigned char c)
      {
#if 0
      static int hour, min, sec, frame;

      if (!extSyncFlag)
            return;

      int valL = c & 0xf;
      int valH = valL << 4;

      int _state = (c & 0x70) >> 4;
      if (mtcState != _state)
            mtcLost += _state - mtcState;
      mtcState = _state + 1;

      switch(_state) {
            case 7:
                  hour  = (hour  & 0x0f) | valH;
                  break;
            case 6:
                  hour  = (hour  & 0xf0) | valL;
                  break;
            case 5:
                  min   = (min   & 0x0f) | valH;
                  break;
            case 4:
                  min   = (min   & 0xf0) | valL;
                  break;
            case 3:
                  sec   = (sec   & 0x0f) | valH;
                  break;
            case 2:
                  sec   = (sec   & 0xf0) | valL;
                  break;
            case 1:
                  frame = (frame & 0x0f) | valH;
                  break;
            case 0:  frame = (frame & 0xf0) | valL;
                  break;
            }
      frame &= 0x1f;    // 0-29
      sec   &= 0x3f;    // 0-59
      min   &= 0x3f;    // 0-59
      hour  &= 0x1f;

      if (mtcState == 8) {
            mtcValid = (mtcLost == 0);
            mtcState = 0;
            mtcLost  = 0;
            if (mtcValid) {
                  mtcCurTime.set(hour, min, sec, frame);
                  mtcSyncMsg(mtcCurTime, !mtcSync);
                  mtcSync = true;
                  }
            }
      else if (mtcValid && (mtcLost == 0)) {
            mtcCurTime.incQuarter();
            mtcSyncMsg(mtcCurTime, false);
            }
#endif
      }

//---------------------------------------------------------
//   mtcInputFull
//    process Frame Message
//---------------------------------------------------------

void MidiSeq::mtcInputFull(const unsigned char* p, int n)
      {
#if 0
      if (debugSync)
            printf("mtcInputFull\n");
      if (!extSyncFlag)
            return;

      if (p[3] != 1) {
            if (p[3] != 2) {   // silently ignore user bits
                  printf("unknown mtc msg subtype 0x%02x\n", p[3]);
                  dump(p, n);
                  }
            return;
            }
      int hour  = p[4];
      int min   = p[5];
      int sec   = p[6];
      int frame = p[7];

      frame &= 0x1f;    // 0-29
      sec   &= 0x3f;    // 0-59
      min   &= 0x3f;    // 0-59
//      int type = (hour >> 5) & 3;
      hour &= 0x1f;

      mtcCurTime.set(hour, min, sec, frame);
      mtcState = 0;
      mtcValid = true;
      mtcLost  = 0;
#endif
      }

//---------------------------------------------------------
//   nonRealtimeSystemSysex
//---------------------------------------------------------

void MidiSeq::nonRealtimeSystemSysex(const unsigned char* p, int n)
      {
#if 0
//      int chan = p[2];
      switch(p[3]) {
            case 4:
                  printf("NRT Setup\n");
                  break;
            default:
                  printf("unknown NRT Msg 0x%02x\n", p[3]);
                  dump(p, n);
                  break;
           }
#endif
      }

//---------------------------------------------------------
//   setSongPosition
//    MidiBeat is a 14 Bit value. Each MidiBeat spans
//    6 MIDI Clocks. Inother words, each MIDI Beat is a
//    16th note (since there are 24 MIDI Clocks in a
//    quarter note).
//---------------------------------------------------------

void MidiSeq::setSongPosition(int port, int midiBeat)
      {
#if 0
      if (midiInputTrace)
            printf("set song position port:%d %d\n", port, midiBeat);
      if (!extSyncFlag)
            return;
      Pos pos((config.division * midiBeat) / 4, AL::TICKS);
      audioDriver->seekTransport(pos.frame());
      if (debugSync)
            printf("setSongPosition %d\n", pos.tick());
#endif
      }

//---------------------------------------------------------
//   realtimeSystemInput
//    real time message received
//---------------------------------------------------------

void MidiSeq::realtimeSystemInput(int port, int c)
      {
#if 0
      if (midiInputTrace)
            printf("realtimeSystemInput port:%d 0x%x\n", port+1, c);

      if (midiInputTrace && (rxSyncPort != port) && rxSyncPort != -1) {
            if (debugSync)
                  printf("rxSyncPort configured as %d; received sync from port %d\n",
                     rxSyncPort, port);
            return;
            }
      if (!extSyncFlag)
            return;
      switch(c) {
            case 0xf8:  // midi clock (24 ticks / quarter note)
                  {
                  double mclock0 = curTime();
                  // Difference in time last 2 rounds:
                  double tdiff0   = mclock0 - mclock1;
                  double tdiff1   = mclock1 - mclock2;
                  double averagetimediff = 0.0;

                  if (mclock1 != 0.0) {
                        if (storedtimediffs < 24)
                        {
                           timediff[storedtimediffs] = mclock0 - mclock1;
                           storedtimediffs++;
                        }
                        else {
                              for (int i=0; i<23; i++) {
                                    timediff[i] = timediff[i+1];
                                    }
                              timediff[23] = mclock0 - mclock1;
                        }
                        // Calculate average timediff:
                        for (int i=0; i < storedtimediffs; i++) {
                              averagetimediff += timediff[i]/storedtimediffs;
                              }
                        }
                  processMidiClock();

                  // Compare w audio if playing:
                  if (audio->isPlaying() /*state == PLAY*/) {
                        //BEGIN standard setup:
                        recTick  += config.division / 24; // The one we're syncing to
                        int tempo = AL::tempomap.tempo(0);
                        unsigned curFrame = audio->pos().frame();
                        double songtick = (double(curFrame)/double(AL::sampleRate)) * config.division * 1000000.0 / double(tempo);
                        double scale = tdiff0/averagetimediff;
                        double tickdiff = songtick - ((double) recTick - 24 + scale*24.0);

                        //END standard setup
                        if (debugSync) {
                              //
                              // Create debug values for printing out which beat we're at, etc etc... yaddayadda...
                              //
                              int m, b, t;
                              audio->pos().mbt(&m, &b, &t);

                              int song_beat = b + m*4; // if the time-signature is different than 4/4, this will be wrong.
                              int sync_beat = recTick/config.division;
                              printf("pT=%.3f rT=%d diff=%.3f songB=%d syncB=%d scale=%.3f", songtick, recTick, tickdiff, song_beat, sync_beat, scale);
                              }
                        //if ((mclock2 !=0.0) && (tdiff1 > 0.0) && tickdiff != 0.0 && lastTempo != 0) {
                        if ((mclock2 !=0.0) && (tdiff1 > 0.0) && fabs(tickdiff) > 2.0 && lastTempo != 0) {
                              // Interpolate:
                              double tickdiff1 = songtick1 - recTick1;
                              double tickdiff2 = songtick2 - recTick2;
                              //double newtickdiff = tickdiff/3.0 + tickdiff1/5.0 + tickdiff2/7.0; //2 min 15 sec, 120BPM, -p 512 jackd
                              //double newtickdiff = tickdiff/4.0 + tickdiff1/4.0 + tickdiff2/4.0; // Not long... :-P
                              //double newtickdiff = tickdiff/5.0 + tickdiff1/8.0 + tickdiff2/12.0; //3,5 mins on 120BPM, -p 512 jackd
                              //double newtickdiff = tickdiff/7.0 + tickdiff1/8.0 + tickdiff2/9.0; //2 min 15 sec, 120BPM, -p 512 jackd
                              //double newtickdiff = tickdiff/5.0 + tickdiff1/8.0 + tickdiff2/16.0; //3,5 mins on 120BPM, -p 512 jackd
                              double newtickdiff = tickdiff/5.0 + tickdiff1/16.0 + tickdiff2/24.0; //5 mins 30 secs on 116BPM, -p 512 jackd
                              //double newtickdiff = tickdiff/5.0 + tickdiff1/23.0 + tickdiff2/31.0; //5 mins on 116BPM, -p 512 jackd
                              //double newtickdiff = tickdiff + tickdiff1/8.0 + tickdiff2/16.0; // Not long...

                              if (newtickdiff != 0.0) {
                                    int newTempo = AL::tempomap.tempo(0);
                                    //newTempo += int(24.0 * newtickdiff * scale);
                                    newTempo += int(24.0 * newtickdiff);
                                    if (debugSync)
                                          printf(" tdiff=%f ntd=%f lt=%d tmpo=%.3f", tdiff0, newtickdiff, lastTempo, (float)((1000000.0 * 60.0)/newTempo));
                                    AL::tempomap.setTempo(0,newTempo);
                                    }
                              if (debugSync)
                                    printf("\n");
                              }
                        else if (debugSync)
                              printf("\n");

                        //BEGIN post calc
                        lastTempo = tempo;
                        recTick2 = recTick1;
                        recTick1 = recTick;
                        mclock2 = mclock1;
                        mclock1 = mclock0;
                        songtick2 = songtick1;
                        songtick1 = songtick;
                        //END post calc
                        break;
                        } // END state play
                  //
                  // Pre-sync (when audio is not running)
                  // Calculate tempo depending on time per pulse
                  //
                  if (mclock1 == 0.0) {
//TODO3                        midiPorts[port].device()->discardInput();
                        if (debugSync)
                           printf("Discarding input from port %d\n", port);
                        }
                  if ((mclock2 != 0.0) && (tdiff0 > 0.0)) {
                        int tempo0 = int(24000000.0 * tdiff0 + .5);
                        int tempo1 = int(24000000.0 * tdiff1 + .5);
                        int tempo = AL::tempomap.tempo(0);

                        int diff0 = tempo0 - tempo;
                        int diff1 = tempo1 - tempo0;
                        if (diff0) {
                              int newTempo = tempo + diff0/8 + diff1/16;
                              if (debugSync)
                                 printf("setting new tempo %d = %f\n", newTempo, (float)((1000000.0 * 60.0)/newTempo));
                              AL::tempomap.setTempo(0, newTempo);
                              }
                        }
                  mclock2 = mclock1;
                  mclock1 = mclock0;
                  }
                  break;
            case 0xf9:  // midi tick  (every 10 msec)
                  if (mcStart) {
                        song->setPos(0, mcStartTick);
                        mcStart = false;
                        return;
                        }
                  break;
            case 0xfa:  // start
                  if (debugSync)
                        printf("   start\n");
                  if (!audio->isPlaying() /*state == IDLE*/) {
                        //seek(0);
                        audioDriver->seekTransport(0);
                        unsigned curFrame = audioDriver->framePos();
                        recTick = recTick1 = recTick2 = 0;
                        mclock1 = 0.0; mclock2 = 0.0;
                        songtick1 = songtick2 = 0;
                        if (debugSync)
                              printf("   curFrame: %d curTick: %d tempo: %d\n", curFrame, recTick, AL::tempomap.tempo(0));

                        //startPlay();
                        storedtimediffs = 0;
                        for (int i=0; i<24; i++)
                              timediff[i] = 0.0;
                        audio->msgPlay(true);
                        }
                  break;
            case 0xfb:  // continue
                  if (debugSync)
                        printf("   continue\n");
                  if (!audio->isPlaying() /*state == IDLE */) {
                        unsigned curFrame = audioDriver->framePos();
                        recTick = AL::tempomap.frame2tick(curFrame); // don't think this will work... (ml)
                        audio->msgPlay(true);
                        }
                  break;
            case 0xfc:  // stop
                  if (debugSync)
                        printf("   stop\n");
                  if (audio->isPlaying() /*state == PLAY*/)
                        audio->msgPlay(false);
                  break;
            case 0xfd:  // unknown
            case 0xfe:  // active sensing
            case 0xff:  // system reset
                  break;
            }
#endif

      }

//---------------------------------------------------------
//   mtcSyncMsg
//    process received mtc Sync
//    seekFlag - first complete mtc frame received after
//                start
//---------------------------------------------------------

void MidiSeq::mtcSyncMsg(const MTC& /*mtc*/, bool /*seekFlag*/)
      {
#if 0
      double time = mtc.time();
      if (debugSync)
            printf("mtcSyncMsg: time %f\n", time);

      if (seekFlag && state == START_PLAY) {
//            int tick = tempomap.time2tick(time);
            state = PLAY;
            sendMsgToGui(MSG_PLAY);
            return;
            }
      // double curT = curTime();

      if (tempoSN != tempomap.tempoSN()) {
            double cpos    = tempomap.tick2time(_midiTick, 0);
            samplePosStart = samplePos - lrint(cpos * sampleRate);
            rtcTickStart   = rtcTick - lrint(cpos * realRtcTicks);
            tempoSN        = tempomap.tempoSN();
            }

      //
      // diff is the time in sec MusE is out of sync
      //
      double diff = time - (double(samplePosStart)/double(sampleRate));
      if (debugSync)
            printf("   state %d diff %f\n", mtcState, diff);
#endif
      }

