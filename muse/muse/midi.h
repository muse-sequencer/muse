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
#include <qstring.h>

enum {
      ME_NOTEOFF    = 0x80,
      ME_NOTEON     = 0x90,
      ME_POLYAFTER  = 0xa0,
      ME_CONTROLLER = 0xb0,
      ME_PROGRAM    = 0xc0,
      ME_AFTERTOUCH = 0xd0,
      ME_PITCHBEND  = 0xe0,
      ME_SYSEX      = 0xf0,
      ME_META       = 0xff,
      ME_SONGPOS    = 0xf2,
      ME_CLOCK      = 0xf8,
      ME_START      = 0xfa,
      ME_CONTINUE   = 0xfb,
      ME_STOP       = 0xfc,
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

class EventList;
class MPEventList;
class MidiTrack;
//extern void buildMidiEventList(EventList* mel, const MPEventList* el, MidiTrack* track, int division, bool);
extern void buildMidiEventList(EventList* mel, const MPEventList* el, MidiTrack* track, int division, bool /*addSysexMeta*/, bool /*doLoops*/);
// extern bool checkSysex(MidiTrack* track, unsigned int len, unsigned char* buf);

#endif

