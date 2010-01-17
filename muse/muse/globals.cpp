//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: globals.cpp,v 1.15.2.11 2009/11/25 09:09:43 terminator356 Exp $
//
//  (C) Copyright 1999-2004 Werner Schweer (ws@seh.de)
//=========================================================

#include <qpixmap.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include "globals.h"
#include "config.h"

int recFileNumber = 1;

int sampleRate   = 44100;
unsigned segmentSize  = 1024U;    // segmentSize in frames (set by JACK)
unsigned fifoLength =  128;       // 131072/segmentSize
                                  // 131072 - magic number that gives a sufficient buffer size
int segmentCount = 2;

// denormal bias value used to eliminate the manifestation of denormals by
// lifting the zero level slightly above zero
// denormal problems occur when values get extremely close to zero
const float denormalBias=1e-18;

bool overrideAudioOutput = false;
bool overrideAudioInput = false;

QTimer* heartBeatTimer;

bool hIsB = true;             // call note h "b"

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
QString museInstruments;
QString museUserInstruments;

QString lastWavePath(".");
QString lastMidiPath(".");

bool debugMode = false;
bool debugMsg = false;
bool midiInputTrace = false;
bool midiOutputTrace = false;
bool realTimeScheduling = false;
int realTimePriority = 40;  // 80
int midiRTPrioOverride = -1;
bool loadPlugins = true;
bool loadVST = true;
bool loadDSSI = true;
bool usePythonBridge = false;
bool useLASH = true;

const char* midi_file_pattern[] = {
      "Midi/Kar (*.mid *.MID *.kar *.KAR *.mid.gz *.mid.bz2)",
      "Midi (*.mid *.MID *.mid.gz *.mid.bz2)",
      "Karaoke (*.kar *.KAR *.kar.gz *.kar.bz2)",
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

const char* part_file_pattern[] = {
      "part Files (*.mpt)",
      "All Files (*)",
      0
      };

const char* plug_file_pattern[] = {
      "part Files (*.pre)",
      "All Files (*)",
      0
      };
Qt::ButtonState globalKeyState;

// Midi Filter Parameter
int midiInputPorts   = 0;    // receive from all devices
int midiInputChannel = 0;    // receive all channel
int midiRecordType   = 0;    // receive all events
int midiThruType = 0;        // transmit all events
int midiFilterCtrl1 = 0;
int midiFilterCtrl2 = 0;
int midiFilterCtrl3 = 0;
int midiFilterCtrl4 = 0;

QActionGroup* undoRedo;
QAction* undoAction;
QAction* redoAction;
QActionGroup* transportAction;
QAction* playAction;
QAction* startAction;
QAction* stopAction;
QAction* rewindAction;
QAction* forwardAction;
QAction* loopAction;
QAction* punchinAction;
QAction* punchoutAction;
QAction* recordAction;
QAction* panicAction;

//AudioMixerApp* audioMixer;
MusE* muse;

int preMeasures = 2;
unsigned char measureClickNote = 63;
unsigned char measureClickVelo = 127;
unsigned char beatClickNote    = 63;
unsigned char beatClickVelo    = 70;
unsigned char clickChan = 9;
unsigned char clickPort = 0;
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
unsigned char rcStopNote = 28;
unsigned char rcRecordNote = 31;
unsigned char rcGotoLeftMarkNote = 33;
unsigned char rcPlayNote = 29;
bool automation = true;

uid_t euid, ruid;  // effective user id, real user id

bool midiSeqRunning = false;

//---------------------------------------------------------
//   doSetuid
//    Restore the effective UID to its original value.
//---------------------------------------------------------

void doSetuid()
      {
#ifndef RTCAP
      int status;
#ifdef _POSIX_SAVED_IDS
      status = seteuid (euid);
#else
      status = setreuid (ruid, euid);
#endif
      if (status < 0) {
            perror("doSetuid: Couldn't set uid");
            }
#endif
      }

//---------------------------------------------------------
//   undoSetuid
//    Set the effective UID to the real UID.
//---------------------------------------------------------

void undoSetuid()
      {
#ifndef RTCAP
      int status;

#ifdef _POSIX_SAVED_IDS
      status = seteuid (ruid);
#else
      status = setreuid (euid, ruid);
#endif
      if (status < 0) {
            fprintf(stderr, "undoSetuid: Couldn't set uid (eff:%d,real:%d): %s\n",
               euid, ruid, strerror(errno));
            exit (status);
            }
#endif
      }

