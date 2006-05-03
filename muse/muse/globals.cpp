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

#include "globals.h"
enum AudioState audioState;

unsigned segmentSize  = 1024U;    // segmentSize in frames (set by JACK)
int segmentCount = 2;

bool overrideAudioOutput = false;
bool overrideAudioInput = false;

QTimer* heartBeatTimer;

const signed char sharpTab[14][7] = {
      { 0, 3, -1, 2, 5, 1, 4 },
      { 0, 3, -1, 2, 5, 1, 4 },
      { 0, 3, -1, 2, 5, 1, 4 },
      { 0, 3, -1, 2, 5, 1, 4 },
      { 2, 5,  1, 4, 7, 3, 6 },
      { 2, 5,  1, 4, 7, 3, 6 },
      { 2, 5,  1, 4, 7, 3, 6 },
      { 4, 0,  3, -1, 2, 5, 1 },
      { 7, 3,  6, 2, 5, 1, 4 },
      { 5, 8,  4, 7, 3, 6, 2 },
      { 3, 6,  2, 5, 1, 4, 7 },
      { 1, 4,  0, 3, 6, 2, 5 },
      { 6, 2,  5, 1, 4, 0, 3 },
      { 0, 3, -1, 2, 5, 1, 4 },
      };
const signed char flatTab[14][7]  = {
      { 4, 1, 5, 2, 6, 3, 7 },
      { 4, 1, 5, 2, 6, 3, 7 },
      { 4, 1, 5, 2, 6, 3, 7 },
      { 4, 1, 5, 2, 6, 3, 7 },
      { 6, 3, 7, 4, 8, 5, 9 },
      { 6, 3, 7, 4, 8, 5, 9 },
      { 6, 3, 7, 4, 8, 5, 9 },

      { 1, 5, 2, 6, 3, 7, 4 },
      { 4, 1, 5, 2, 6, 3, 7 },
      { 2, 6, 3, 7, 4, 8, 5 },
      { 7, 4, 1, 5, 2, 6, 3 },
      { 5, 2, 6, 3, 7, 4, 8 },
      { 3, 0, 4, 1, 5, 2, 6 },
      { 4, 1, 5, 2, 6, 3, 7 },
      };

QString museGlobalLib;
QString museGlobalShare;
QString museUser;
QString museProject;
QString museProjectInitPath("./");
QString configName;

QString lastWavePath(".");
QString lastMidiPath(".");

bool debugMode = false;
bool debugMsg = false;
bool midiInputTrace = false;
bool midiOutputTrace = false;
int realTimePriority = 50;
bool loadPlugins = true;
bool loadVST = true;
bool loadDSSI = true;
bool midiOnly = false;

const char* midi_file_pattern[] = {
      "Midi/Kar (*.mid *.MID *.kar *.KAR *.mid.gz *.mid.bz2)",
      "Midi (*.mid *.MID *.mid.gz *.mid.bz2)",
      "Karaoke (*.kar *.KAR *.kar.gz *.kar.bz2)",
      "All Files (*)",
      0
      };
QString medFilePattern =
      "med Files (*.med *.med.gz *.med.bz2);;"
      "All Files (*)";

const char* med_midi_file_pattern[] = {
      "med Files (*.med *.med.gz *.med.bz2)",
      "Midi/Kar (*.mid *.kar *.mid.gz *.mid.bz2)",
      "Midi (*.mid *.mid.gz *.mid.bz2)",
      "Karaoke (*.kar *.kar.gz *.kar.bz2)",
      "All Files (*)",
      0
      };
const char* med_file_pattern[] = {
      "med Files (*.med *.med.gz *.med.bz2)",
      "All Files (*)",
      0
      };
const char* image_file_pattern[] = {
      "(*.jpg *.gif *.png)",
      "(*.jpg)",
      "(*.gif)",
      "(*.png)",
      "All Files (*)",
      0
      };

const char* ctrl_file_pattern[] = {
      "ctrl Files (*.ctrl *.ctrl.gz *.ctrl.bz2)",
      "All Files (*)",
      0
      };

QAction* undoAction;
QAction* redoAction;

QAction* loopAction;
QAction* punchinAction;
QAction* punchoutAction;
QAction* recordAction;
QAction* panicAction;

int preMeasures = 2;
bool precountEnableFlag = false;
bool precountFromMastertrackFlag = false;
int precountSigZ = 4;
int precountSigN = 4;
bool precountPrerecord = false;
bool precountPreroll = false;
bool midiClickFlag   = true;
bool audioClickFlag  = true;
float audioClickVolume = 0.1f;

bool rcEnable = false;
bool midiSeqRunning = false;

