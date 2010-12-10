//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: globaldefs.h,v 1.3.2.1 2009/05/03 04:14:00 terminator356 Exp $
//
//  (C) Copyright 2000 Werner Schweer (ws@seh.de)
//=========================================================

#ifndef __GLOBALDEFS_H__
#define __GLOBALDEFS_H__

// Midi Type
//    MT_GM  - General Midi
//    MT_GS  - Roland GS
//    MT_XG  - Yamaha XG

enum MType { MT_UNKNOWN=0, MT_GM, MT_GS, MT_XG };

enum AutomationType {
      AUTO_OFF, AUTO_READ, AUTO_TOUCH, AUTO_WRITE
      };

const int MAX_CHANNELS = 2;   // max audio channels
const int MAX_PLUGINS  = 4;   // plugins in mixer rack

//const int MIDI_PORTS   = 32;  // max Number of Midi Ports
const int MIDI_PORTS   = 200;  // max Number of Midi Ports

#ifndef MIDI_CHANNELS
#define MIDI_CHANNELS 16       // Channels per Port
#endif

#endif

