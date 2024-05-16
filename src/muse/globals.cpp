//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: globals.cpp,v 1.15.2.11 2009/11/25 09:09:43 terminator356 Exp $
//
//  (C) Copyright 1999-2004 Werner Schweer (ws@seh.de)
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

#include <stdio.h>

#include <QActionGroup>
#include <QDir>
#include <QFileInfo>

#include "globals.h"

namespace MusEGlobal {

int sampleRate   = 44100;
unsigned segmentSize  = 1024U;    // segmentSize in frames (set by JACK)
unsigned fifoLength =  128;       // 131072/segmentSize
                                  // 131072 - magic number that gives a sufficient buffer size
int segmentCount = 2;

//   NOTE: For now, this is TEMPORARILY set to the project sample rate during song loading,
//          then at the END of song loading is immediately set to the real current rate.
//         See comments in Song::read() at the "samplerate" tag section.
int projectSampleRate = sampleRate;
const int numAudioSampleRates = 8;
const int selectableAudioSampleRates[] = {
      22050, 32000, 44100, 48000, 64000, 88200, 96000, 192000
      };

MusECore::SndFileList sndFiles;

MusECore::AudioConverterPluginList audioConverterPluginList;
// This global variable is a pointer so that we can replace it quickly with a new one in RT operations.
MusECore::AudioConverterSettingsGroup* defaultAudioConverterSettings = nullptr;

// denormal bias value used to eliminate the manifestation of denormals by
// lifting the zero level slightly above zero
// denormal problems occur when values get extremely close to zero
const float denormalBias=1e-18;

bool overrideAudioOutput = false;
bool overrideAudioInput = false;

const QString selectableAudioBackendDevices[] = {
    "Jack Audio (default)",
    "Midi only",
    "RtAudio Pulse Audio",
    "RtAudio ALSA",
    "RtAudio OSS - Open Sound System",
    "RtAudio best guess"
};

const int numRtAudioDevices = 6;

MusEGui::Rasterizer *globalRasterizer = nullptr;

// This holds all the unit symbols found. Instead of storing many copies of the same texts
//  (dB, Hz, frames etc.) all over the place, we store an index into this list.
MusECore::ValueUnits_t valueUnits;

QTimer* heartBeatTimer;

bool blinkTimerPhase = false;

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
QString configPath;
QString cachePath;
QString museInstruments;
QString museUserInstruments;

QString lastWavePath(".");
QString lastMidiPath(".");

bool debugMode = false;
bool unityWorkaround = false;
bool debugMsg = false;
bool heavyDebugMsg = false;
bool midiInputTrace = false;
bool midiOutputTrace = false;
bool realTimeScheduling = false;
int realTimePriority = 40;  // 80
int midiRTPrioOverride = -1;
bool loadPlugins = true;
bool loadMESS = true;
bool loadVST = true;
bool loadNativeVST = true;
bool loadDSSI = true;
bool loadLV2 = true;
bool usePythonBridge = false;
QString pythonBridgePyroNSHostname;
QString pythonBridgePyroNSPort;
QString pythonBridgePyroDaemonHostname;
QString pythonBridgePyroDaemonPort;
float pythonBridgePyroCommTimeout = 5.0;
bool useLASH = true;
bool useAlsaWithJack = false;
bool noAutoStartJack = false;
bool populateMidiPortsOnStart = true;

const char* midi_file_pattern[] = {
      QT_TRANSLATE_NOOP("file_patterns", "Midi/Kar (*.mid *.MID *.kar *.KAR *.mid.gz *.mid.bz2)"),
      QT_TRANSLATE_NOOP("file_patterns", "Midi (*.mid *.MID *.mid.gz *.mid.bz2)"),
      QT_TRANSLATE_NOOP("file_patterns", "Karaoke (*.kar *.KAR *.kar.gz *.kar.bz2)"),
      QT_TRANSLATE_NOOP("file_patterns", "All Files (*)"),
      nullptr
      };

//FIXME: By T356 01/19/2010
// If saving as a compressed file (gz or bz2),
//  the file is a pipe, and pipes can't seek !
// This results in a corrupted midi file from MidiFile::writeTrack(). 
// So exporting compressed midi has simply been disabled here for now...
// For re-enabling, add .mid.gz and .mid.bz2 and same for .kar again
const char* midi_file_save_pattern[] = {
      QT_TRANSLATE_NOOP("file_patterns", "Midi (*.mid)"),
      QT_TRANSLATE_NOOP("file_patterns", "Karaoke (*.kar)"),
      QT_TRANSLATE_NOOP("file_patterns", "All Files (*)"),
    nullptr
      };

const char* med_file_pattern[] = {
      QT_TRANSLATE_NOOP("file_patterns", "all known files (*.med *.med.gz *.med.bz2 *.mid *.midi *.kar)"),
      QT_TRANSLATE_NOOP("file_patterns", "med Files (*.med *.med.gz *.med.bz2)"),
      QT_TRANSLATE_NOOP("file_patterns", "Uncompressed med Files (*.med)"),
      QT_TRANSLATE_NOOP("file_patterns", "gzip compressed med Files (*.med.gz)"),
      QT_TRANSLATE_NOOP("file_patterns", "bzip2 compressed med Files (*.med.bz2)"),
      QT_TRANSLATE_NOOP("file_patterns", "mid Files (*.mid *.midi *.kar *.MID *.MIDI *.KAR)"),
      QT_TRANSLATE_NOOP("file_patterns", "All Files (*)"),
    nullptr
      };
const char* med_file_save_pattern[] = {
      QT_TRANSLATE_NOOP("file_patterns", "Uncompressed med Files (*.med)"),
      QT_TRANSLATE_NOOP("file_patterns", "gzip compressed med Files (*.med.gz)"),
      QT_TRANSLATE_NOOP("file_patterns", "bzip2 compressed med Files (*.med.bz2)"),
      QT_TRANSLATE_NOOP("file_patterns", "All Files (*)"),
    nullptr
      };
const char* project_create_file_save_pattern[] = {
      QT_TRANSLATE_NOOP("file_patterns", "Uncompressed med Files (*.med)"),
      QT_TRANSLATE_NOOP("file_patterns", "gzip compressed med Files (*.med.gz)"),
      QT_TRANSLATE_NOOP("file_patterns", "bzip2 compressed med Files (*.med.bz2)"),
    nullptr
      };

const char* image_file_pattern[] = {
      QT_TRANSLATE_NOOP("file_patterns", "(*.jpg *.gif *.png)"),
      QT_TRANSLATE_NOOP("file_patterns", "(*.jpg)"),
      QT_TRANSLATE_NOOP("file_patterns", "(*.gif)"),
      QT_TRANSLATE_NOOP("file_patterns", "(*.png)"),
      QT_TRANSLATE_NOOP("file_patterns", "All Files (*)"),
    nullptr
      };

const char* part_file_pattern[] = {
      QT_TRANSLATE_NOOP("file_patterns", "part Files (*.mpt *.mpt.gz *.mpt.bz2)"),
      QT_TRANSLATE_NOOP("file_patterns", "All Files (*)"),
    nullptr
      };
const char* part_file_save_pattern[] = {
      QT_TRANSLATE_NOOP("file_patterns", "part Files (*.mpt)"),
      QT_TRANSLATE_NOOP("file_patterns", "gzip compressed part Files (*.mpt.gz)"),
      QT_TRANSLATE_NOOP("file_patterns", "bzip2 compressed part Files (*.mpt.bz2)"),
      QT_TRANSLATE_NOOP("file_patterns", "All Files (*)"),
    nullptr
      };

const char* preset_file_pattern[] = {
      QT_TRANSLATE_NOOP("file_patterns", "Presets (*.pre *.pre.gz *.pre.bz2)"),
      QT_TRANSLATE_NOOP("file_patterns", "All Files (*)"),
      nullptr
      };

const char* preset_file_save_pattern[] = {
      QT_TRANSLATE_NOOP("file_patterns", "Presets (*.pre)"),
      QT_TRANSLATE_NOOP("file_patterns", "gzip compressed presets (*.pre.gz)"),
      QT_TRANSLATE_NOOP("file_patterns", "bzip2 compressed presets (*.pre.bz2)"),
      QT_TRANSLATE_NOOP("file_patterns", "All Files (*)"),
      nullptr
      };

const char* drum_map_file_pattern[] = {
      QT_TRANSLATE_NOOP("file_patterns", "Presets (*.map *.map.gz *.map.bz2)"),
      QT_TRANSLATE_NOOP("file_patterns", "All Files (*)"),
      nullptr
};

const char* drum_map_file_save_pattern[] = {
      QT_TRANSLATE_NOOP("file_patterns", "Presets (*.map)"),
      QT_TRANSLATE_NOOP("file_patterns", "gzip compressed presets (*.map.gz)"),
      QT_TRANSLATE_NOOP("file_patterns", "bzip2 compressed presets (*.map.bz2)"),
      QT_TRANSLATE_NOOP("file_patterns", "All Files (*)"),
      nullptr
};

const char* audio_file_pattern[] = {
      QT_TRANSLATE_NOOP("file_patterns", "Wave/Binary (*.wav *.ogg *.flac *.bin)"),
      QT_TRANSLATE_NOOP("file_patterns", "Wave (*.wav *.ogg *.flac)"),
      QT_TRANSLATE_NOOP("file_patterns", "Binary (*.bin)"),
      QT_TRANSLATE_NOOP("file_patterns", "All Files (*)"),
      nullptr
};

const char* colors_config_file_pattern[] = {
      QT_TRANSLATE_NOOP("file_patterns", "Color configuration files (*.cfc)"),
      QT_TRANSLATE_NOOP("file_patterns", "All Files (*)"),
      nullptr
};

const char* stylesheet_file_pattern[] = {
    QT_TRANSLATE_NOOP("file_patterns", "Qt style sheets (*.qss)"),
    QT_TRANSLATE_NOOP("file_patterns", "All Files (*)"),
    nullptr
};

Qt::KeyboardModifiers globalKeyState;

// Midi Filter Parameter
int midiInputPorts   = 0;    // receive from all devices
int midiInputChannel = 0;    // receive all channel
int midiRecordType   = 0;    // receive all events
int midiThruType = 0;        // transmit all events
int midiFilterCtrl1 = 0;     // val 0 means no controller filtered. val >= 1 means controller number (val - 1)
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
QAction* metronomeAction;
QAction* cpuLoadAction;

MusEGui::MusE* muse = nullptr;

MusECore::MetroAccentsPresetsMap metroAccentPresets;
MusECore::MetronomeSettings metroGlobalSettings;
MusECore::MetronomeSettings metroSongSettings;
// Whether to use the global or song metronome settings.
bool metroUseSongSettings = false;


MusECore::MidiRemote midiRemote;
// Whether the midi remote control dialog box is currently learning.
bool midiRemoteIsLearning = false;
// Whether to use the global or song metronome settings.
bool midiRemoteUseSongSettings = false;

// Whether the midi to audio assignment dialog box is currently learning.
bool midiToAudioAssignIsLearning = false;

// REMOVE Tim. automation. Remove this.
// Deprecated. MusEGlobal::automation is now fixed TRUE
//   for now until we decide what to do with it.
bool automation = true;

const QString inputRoutingToolTipBase = QObject::tr("Input routing");
const QString noInputRoutingToolTipWarn = inputRoutingToolTipBase + QString("\n") + QObject::tr("Warning: No input routes! Click to connect...");

const QString outputRoutingToolTipBase = QObject::tr("Output routing");
const QString noOutputRoutingToolTipWarn = outputRoutingToolTipBase + QString("\n") + QObject::tr("Warning: No output routes! Click to connect...");

// REMOVE Tim. setuid. Removed.
//uid_t euid, ruid;  // effective user id, real user id

bool midiSeqRunning = false;

QString defaultStyle = "Fusion";

//---------------------------------------------------------
//   convertFrame4ProjectSampleRate
//---------------------------------------------------------

unsigned convertFrame4ProjectSampleRate(unsigned frame, unsigned frame_sample_rate)
{
  return double(frame) * double(frame_sample_rate) / double(MusEGlobal::projectSampleRate);
}

//---------------------------------------------------------
//   doSetuid
//    Restore the effective UID to its original value.
//---------------------------------------------------------

void doSetuid()
      {
// BUG REMOVE Tim. setuid. Removed. Ancient stuff no longer required?
//     Tests OK under several scenarios: Jack started externally,
//      Jack started by MusE, Jack started as root, dummy driver etc.
//     Was causing crash with new DrumGizmo 0.9.11
// #ifndef RTCAP
//       int status;
// #ifdef _POSIX_SAVED_IDS
//       status = seteuid (euid);
// #else
//       status = setreuid (ruid, euid);
// #endif
//       if (status < 0) {
//             perror("doSetuid: Couldn't set uid");
//             }
// #endif
      }

//---------------------------------------------------------
//   undoSetuid
//    Set the effective UID to the real UID.
//---------------------------------------------------------

void undoSetuid()
      {
// BUG REMOVE Tim. setuid. Removed. Ancient stuff no longer required?
//     Tests OK under several scenarios: Jack started externally,
//      Jack started by MusE, Jack started as root, dummy driver etc.
//     Was causing crash with new DrumGizmo 0.9.11
// #ifndef RTCAP
//       int status;
//
// #ifdef _POSIX_SAVED_IDS
//       status = seteuid (ruid);
// #else
//       status = setreuid (euid, ruid);
// #endif
//       if (status < 0) {
//             fprintf(stderr, "undoSetuid: Couldn't set uid (eff:%d,real:%d): %s\n",
//                euid, ruid, strerror(errno));
//             exit (status);
//             }
// #endif
      }

//---------------------------------------------------------
//   getUniqueTmpfileName
//---------------------------------------------------------
bool getUniqueTmpfileName(QString subDir, QString ext, QString& newFilename)
{
    // Check if tmp-directory exists under project path
    QString tmpInDir = museProject + "/" + subDir;
    QDir absDir(tmpInDir);
    tmpInDir = absDir.cleanPath(absDir.absolutePath());

    QFileInfo tmpdirfi(tmpInDir);
    if (!tmpdirfi.isDir()) {
        // Try to create a tmpdir
        QDir projdir(museProject);
        if (!projdir.mkdir(subDir)) {
            printf("Could not create tmp dir %s!\n", tmpInDir.toLatin1().data() );
            return false;
        }
    }

    tmpdirfi.setFile(tmpInDir);

    if (!tmpdirfi.isWritable()) {
        printf("Temp directory is not writable - aborting\n");
        return false;
    }

    QDir tmpdir = tmpdirfi.dir();

    // Find a new filename
    for (int i=0; i<10000; i++) {
        QString filename = "muse_tmp";
        filename.append(QString::number(i));
        if (!ext.startsWith("."))
            filename.append(".");
        filename.append(ext);

        if (!tmpdir.exists(tmpInDir +"/" + filename)) {
            newFilename = tmpInDir + "/" + filename;
            if (debugMsg)
                printf("returning temporary filename %s\n", newFilename.toLatin1().data());
            return true;
        }

    }

    printf("Could not find a suitable tmpfilename (more than 10000 tmpfiles in tmpdir - clean up!\n");
    return false;
}

} // namespace MusEGlobal
