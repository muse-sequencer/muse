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

#ifndef GLOBALS_H
#define GLOBALS_H

#include <sys/types.h>
#include "mtc.h"

enum AudioState {
	AUDIO_STOP,
      AUDIO_START1,
      AUDIO_START2,
      AUDIO_RUNNING
      };
extern enum AudioState audioState;

extern unsigned segmentSize;
extern int segmentCount;

extern bool overrideAudioOutput;
extern bool overrideAudioInput;

class QTimer;
extern QTimer* heartBeatTimer;

extern const signed char sharpTab[14][7];
extern const signed char flatTab[14][7];

extern QString museGlobalLib;
extern QString museGlobalShare;
extern QString museUser;
extern QString configName;

extern QString lastWavePath;
extern QString lastMidiPath;

extern bool debugMode;
extern bool midiInputTrace;
extern bool midiOutputTrace;
extern bool debugMsg;
extern bool debugSync;
extern bool loadPlugins;
extern bool loadVST;
extern bool loadDSSI;
extern bool midiOnly;

extern int realTimePriority;
extern const char* midi_file_pattern[];
extern QString medFilePattern;
extern const char* med_file_pattern[];
extern const char* med_midi_file_pattern[];
extern const char* image_file_pattern[];
extern const char* ctrl_file_pattern[];

#define CMD_RANGE_ALL         0
#define CMD_RANGE_SELECTED    1
#define CMD_RANGE_LOOP        2

extern QAction* undoAction;
extern QAction* redoAction;

extern QAction* loopAction;
extern QAction* punchinAction;
extern QAction* punchoutAction;
extern QAction* recordAction;
extern QAction* panicAction;

extern int preMeasures;
extern bool precountEnableFlag;
extern bool precountFromMastertrackFlag;
extern int precountSigZ;
extern int precountSigN;
extern bool precountPrerecord;
extern bool precountPreroll;
extern bool midiClickFlag;
extern bool audioClickFlag;
extern double audioClickVolume;

extern bool rcEnable;
#endif

