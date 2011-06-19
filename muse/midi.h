//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: midi.h,v 1.4.2.2 2009/11/09 20:28:28 terminator356 Exp $
//
//  (C) Copyright 1999/2000 Werner Schweer (ws@seh.de)
//=========================================================

#ifndef __MIDI_H__
#define __MIDI_H__

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

class QString;

enum {
      ME_NOTEOFF     = 0x80,
      ME_NOTEON      = 0x90,
      ME_POLYAFTER   = 0xa0,
      ME_CONTROLLER  = 0xb0,
      ME_PROGRAM     = 0xc0,
      ME_AFTERTOUCH  = 0xd0,
      ME_PITCHBEND   = 0xe0,
      ME_SYSEX       = 0xf0,
      ME_META        = 0xff,
      ME_MTC_QUARTER = 0xf1,
      ME_SONGPOS     = 0xf2,
      ME_SONGSEL     = 0xf3,
      ME_TUNE_REQ    = 0xf6,
      ME_SYSEX_END   = 0xf7,
      ME_CLOCK       = 0xf8,
      ME_TICK        = 0xf9,
      ME_START       = 0xfa,
      ME_CONTINUE    = 0xfb,
      ME_STOP        = 0xfc,
      ME_SENSE       = 0xfe
      };

#define ME_TIMESIG      0x58

extern const unsigned char gmOnMsg[];

extern const unsigned char gsOnMsg[];
extern const unsigned char gsOnMsg2[];
extern const unsigned char gsOnMsg3[];
extern const unsigned char xgOnMsg[];
extern const unsigned char mmcDeferredPlayMsg[];
extern const unsigned char mmcStopMsg[];
extern const unsigned char mmcLocateMsg[];

extern const unsigned int gmOnMsgLen;
extern const unsigned int gsOnMsgLen;
extern const unsigned int gsOnMsg2Len;
extern const unsigned int gsOnMsg3Len;
extern const unsigned int xgOnMsgLen;
extern const unsigned int mmcDeferredPlayMsgLen;
extern const unsigned int mmcStopMsgLen;
extern const unsigned int mmcLocateMsgLen;

QString nameSysex(unsigned int len, const unsigned char* buf);
QString midiMetaName(int);

// Use these in all the synths and their guis.
// Did this here for ease, since they all include this file.
//
// A special MusE soft synth sysex manufacturer ID.
#define MUSE_SYNTH_SYSEX_MFG_ID 0x7c

class EventList;
class MPEventList;
class MidiTrack;
//extern void buildMidiEventList(EventList* mel, const MPEventList* el, MidiTrack* track, int division, bool);
extern void buildMidiEventList(EventList* mel, const MPEventList* el, MidiTrack* track, int division, bool /*addSysexMeta*/, bool /*doLoops*/);
// extern bool checkSysex(MidiTrack* track, unsigned int len, unsigned char* buf);

#endif

