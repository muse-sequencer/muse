//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: globals.h,v 1.10.2.11 2009/11/25 09:09:43 terminator356 Exp $
//
//  (C) Copyright 1999/2000 Werner Schweer (ws@seh.de)
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

#ifndef GLOBALS_H
#define GLOBALS_H

#include <sys/types.h>

#include <QString>
#include <QAction>
#include <QActionGroup>
#include <QTimer>
#include <QToolButton>

#include "value.h"
#include "mtc.h"
#include "metronome_class.h"
#include "wave.h"
#include "audio_convert/audio_converter_plugin.h"
#include "audio_convert/audio_converter_settings_group.h"
#include "rasterizer.h"
#include "value_units.h"
#include "mplugins/midiremote.h"

#include <unistd.h>

namespace MusEGui {
class MusE;
}

namespace MusEGlobal {

extern const float denormalBias;

extern int sampleRate;
extern unsigned segmentSize;
extern unsigned fifoLength; // inversely proportional to segmentSize
extern int segmentCount;
extern int projectSampleRate;
extern const int numAudioSampleRates;
extern const int selectableAudioSampleRates[];

extern MusECore::SndFileList sndFiles;

extern MusECore::AudioConverterPluginList audioConverterPluginList;
// This global variable is a pointer so that we can replace it quickly with a new one in RT operations.
extern MusECore::AudioConverterSettingsGroup* defaultAudioConverterSettings;

extern bool overrideAudioOutput;
extern bool overrideAudioInput;

extern const QString selectableAudioBackendDevices[];
extern const int numRtAudioDevices;
enum SelectableAudioBackendDevices {
  JackAudio = 0,
  DummyAudio = 1,
  RtAudioPulse = 2,
  RtAudioAlsa = 3,
  RtAudioOss = 4,
  RtAudioChoice = 5
  //RtAudioJack, -- it's just stupid to keep this option
};

extern QTimer* heartBeatTimer;

extern MusEGui::Rasterizer *globalRasterizer;

extern MusECore::ValueUnits_t valueUnits;

extern bool blinkTimerPhase;

extern bool hIsB;

extern const signed char sharpTab[14][7];
extern const signed char flatTab[14][7];

extern QString museGlobalLib;
extern QString museGlobalShare;
extern QString museUser;
extern QString museProject;
extern QString museProjectInitPath;
extern QString configName;
extern QString configPath;
extern QString museInstruments;
extern QString museUserInstruments;
extern QString cachePath;

extern QString lastWavePath;
extern QString lastMidiPath;

extern bool debugMode;
extern bool midiInputTrace;
extern bool midiOutputTrace;
extern bool unityWorkaround;
extern bool debugMsg;
extern bool heavyDebugMsg;
extern bool debugSync;
extern bool loadPlugins;
extern bool loadMESS;
extern bool loadVST;
extern bool loadNativeVST;
extern bool loadDSSI;
extern bool usePythonBridge;
extern QString pythonBridgePyroNSHostname;
extern QString pythonBridgePyroNSPort;
extern QString pythonBridgePyroDaemonHostname;
extern QString pythonBridgePyroDaemonPort;
extern float pythonBridgePyroCommTimeout;
extern bool useLASH;
extern bool loadLV2;
extern bool useAlsaWithJack;
extern bool noAutoStartJack;
extern bool populateMidiPortsOnStart;

extern bool realTimeScheduling;
extern int realTimePriority;
extern int midiRTPrioOverride;

extern const char* midi_file_pattern[];
extern const char* midi_file_save_pattern[];
extern const char* med_file_pattern[];
extern const char* med_file_save_pattern[];
extern const char* project_create_file_save_pattern[];
extern const char* image_file_pattern[];
extern const char* part_file_pattern[];
extern const char* part_file_save_pattern[];
extern const char* preset_file_pattern[];
extern const char* preset_file_save_pattern[];
extern const char* drum_map_file_pattern[];
extern const char* drum_map_file_save_pattern[];
extern const char* audio_file_pattern[];
extern const char* colors_config_file_pattern[];
//extern const char* stylesheet_file_pattern[];

extern Qt::KeyboardModifiers globalKeyState;

extern int midiInputPorts;          //!< receive from all devices
extern int midiInputChannel;        //!< receive all channel
extern int midiRecordType;          //!< receive all events

#define MIDI_FILTER_NOTEON    1
#define MIDI_FILTER_POLYP     2
#define MIDI_FILTER_CTRL      4
#define MIDI_FILTER_PROGRAM   8
#define MIDI_FILTER_AT        16
#define MIDI_FILTER_PITCH     32
#define MIDI_FILTER_SYSEX     64

extern int midiThruType;            // transmit all events
extern int midiFilterCtrl1;
extern int midiFilterCtrl2;
extern int midiFilterCtrl3;
extern int midiFilterCtrl4;

#define CMD_RANGE_ALL         0
#define CMD_RANGE_SELECTED    1
#define CMD_RANGE_LOOP        2

extern QActionGroup* undoRedo;
extern QAction* undoAction;
extern QAction* redoAction;

extern QActionGroup* transportAction;
extern QAction* playAction;
extern QAction* startAction;
extern QAction* stopAction;
extern QAction* rewindAction;
extern QAction* forwardAction;
extern QAction* loopAction;
extern QAction* punchinAction;
extern QAction* punchoutAction;
extern QAction* recordAction;
extern QAction* panicAction;
extern QAction* metronomeAction;
extern QAction* cpuLoadAction;

extern MusEGui::MusE* muse;

extern MusECore::MetroAccentsPresetsMap metroAccentPresets;
extern MusECore::MetronomeSettings metroGlobalSettings;
extern MusECore::MetronomeSettings metroSongSettings;
// Whether to use the global or song metronome settings.
extern bool metroUseSongSettings;

extern MusECore::MidiRemote midiRemote;
// Whether the midi remote control dialog box is currently learning.
extern bool midiRemoteIsLearning;
// Whether to use the global or song metronome settings.
extern bool midiRemoteUseSongSettings;

// Whether the midi to audio assignment dialog box is currently learning.
extern bool midiToAudioAssignIsLearning;

extern bool midiSeqRunning;
extern bool automation;

extern const QString inputRoutingToolTipBase;
extern const QString outputRoutingToolTipBase;
extern const QString noInputRoutingToolTipWarn;
extern const QString noOutputRoutingToolTipWarn;


#define JACK_MIDI_OUT_PORT_SUFFIX "_out"
#define JACK_MIDI_IN_PORT_SUFFIX  "_in"

extern void doSetuid();
extern void undoSetuid();
extern bool checkAudioDevice();
extern bool getUniqueTmpfileName(QString subDir, QString ext, QString& newFilename);

extern unsigned convertFrame4ProjectSampleRate(unsigned frame, unsigned frame_sample_rate);

extern QString defaultStyle;
} // namespace MusEGlobal

#endif

