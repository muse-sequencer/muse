//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: app.cpp,v 1.113.2.68 2009/12/21 14:51:51 spamatica Exp $
//
//  (C) Copyright 1999-2011 Werner Schweer (ws@seh.de)
//  (C) Copyright 2011 Tim E. Real (terminator356 on sourceforge)
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

#include <typeinfo>

#include <QClipboard>
#include <QMessageBox>
#include <QShortcut>
#include <QSignalMapper>
#include <QTimer>
#include <QWhatsThis>
#include <QSettings>
#include <QProgressDialog>
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QSocketNotifier>  
#include <QString>
#include <QStyleFactory>

#include <iostream>

#include "app.h"
#include "master/lmaster.h"
#include "al/dsp.h"
#include "amixer.h"
#include "appearance.h"
#include "arranger.h"
#include "arrangerview.h"
#include "audio.h"
#include "audiodev.h"
#include "audioprefetch.h"
#include "bigtime.h"
#include "cliplist/cliplist.h"
#include "conf.h"
#include "config.h"
#include "debug.h"
#include "didyouknow.h"
#include "drumedit.h"
#include "filedialog.h"
#include "gconfig.h"
#include "gui.h"
#include "helper.h"
#include "icons.h"
#include "instruments/editinstrument.h"
#include "listedit.h"
#include "marker/markerview.h"
#include "master/masteredit.h"
#include "metronome.h"
#include "midiseq.h"
#include "mixdowndialog.h"
#include "pianoroll.h"
#include "scoreedit.h"
#include "remote/pyapi.h"
#include "routepopup.h"
#include "shortcutconfig.h"
#include "songinfo.h"
#include "ticksynth.h"
#include "transport.h"
#include "waveedit.h"
#include "widgets/projectcreateimpl.h"
#include "widgets/menutitleitem.h"
#include "tools.h"
#include "widgets/unusedwavefiles.h"
#include "functions.h"
#include "songpos_toolbar.h"
#include "sig_tempo_toolbar.h"

namespace MusECore {
extern void exitJackAudio();
extern void exitDummyAudio();
extern void exitOSC();
extern void exitMidiAlsa();
}

namespace MusEGui {

extern void deleteIcons();

//extern void cacheJackRouteNames();

static pthread_t watchdogThread;
//ErrorHandler *error;

#define PROJECT_LIST_LEN  6
static QString* projectList[PROJECT_LIST_LEN];

#ifdef HAVE_LASH
#include <lash/lash.h>
lash_client_t * lash_client = 0;
#endif /* HAVE_LASH */

int watchAudioPrefetch, watchMidi;
pthread_t splashThread;





//---------------------------------------------------------
//   sleep function
//---------------------------------------------------------
void microSleep(long msleep)
{
    bool sleepOk=-1;

    while(sleepOk==-1)
        sleepOk=usleep(msleep);
}

//---------------------------------------------------------
//   seqStart
//---------------------------------------------------------

bool MusE::seqStart()
      {
      if (MusEGlobal::audio->isRunning()) {
            printf("seqStart(): already running\n");
            return true;
            }
      
      if (!MusEGlobal::audio->start()) {
          QMessageBox::critical( MusEGlobal::muse, tr("Failed to start audio!"),
              tr("Was not able to start audio, check if jack is running.\n"));
          return false;
          }

      //
      // wait for jack callback
      //
      for(int i = 0; i < 60; ++i) 
      {
        if(MusEGlobal::audio->isRunning())
          break;
        sleep(1);
      }
      if(!MusEGlobal::audio->isRunning()) 
      {
        QMessageBox::critical( MusEGlobal::muse, tr("Failed to start audio!"),
            tr("Timeout waiting for audio to run. Check if jack is running.\n"));
      }
      //
      // now its safe to ask the driver for realtime
      // priority
      
      MusEGlobal::realTimePriority = MusEGlobal::audioDevice->realtimePriority();
      if(MusEGlobal::debugMsg)
        printf("MusE::seqStart: getting audio driver MusEGlobal::realTimePriority:%d\n", MusEGlobal::realTimePriority);
      
      int pfprio = 0;
      int midiprio = 0;
      
      // NOTE: MusEGlobal::realTimeScheduling can be true (gotten using jack_is_realtime()),
      //  while the determined MusEGlobal::realTimePriority can be 0.
      // MusEGlobal::realTimePriority is gotten using pthread_getschedparam() on the client thread 
      //  in JackAudioDevice::realtimePriority() which is a bit flawed - it reports there's no RT...
      if(MusEGlobal::realTimeScheduling) 
      {
        {
          pfprio = MusEGlobal::realTimePriority + 1;
          midiprio = MusEGlobal::realTimePriority + 2;
        }  
      }
      
      if(MusEGlobal::midiRTPrioOverride > 0)
        midiprio = MusEGlobal::midiRTPrioOverride;
      
      // FIXME FIXME: The MusEGlobal::realTimePriority of the Jack thread seems to always be 5 less than the value passed to jackd command.
      
      MusEGlobal::audioPrefetch->start(pfprio);
      
      MusEGlobal::audioPrefetch->msgSeek(0, true); // force
      
      MusEGlobal::midiSeq->start(midiprio);
      
      int counter=0;
      while (++counter) {
        if (counter > 1000) {
            fprintf(stderr,"midi sequencer thread does not start!? Exiting...\n");
            exit(33);
        }
        MusEGlobal::midiSeqRunning = MusEGlobal::midiSeq->isRunning();
        if (MusEGlobal::midiSeqRunning)
          break;
        usleep(1000);
        if(MusEGlobal::debugMsg)
          printf("looping waiting for sequencer thread to start\n");
      }
      if(!MusEGlobal::midiSeqRunning)
      {
        fprintf(stderr, "midiSeq is not running! Exiting...\n");
        exit(33);
      }  
      return true;
      }
      
//---------------------------------------------------------
//   stop
//---------------------------------------------------------

void MusE::seqStop()
      {
      // label sequencer as disabled before it actually happened to minimize race condition
      MusEGlobal::midiSeqRunning = false;

      MusEGlobal::song->setStop(true);
      MusEGlobal::song->setStopPlay(false);
      MusEGlobal::midiSeq->stop(true);
      MusEGlobal::audio->stop(true);
      MusEGlobal::audioPrefetch->stop(true);
      if (MusEGlobal::realTimeScheduling && watchdogThread)
            pthread_cancel(watchdogThread);
      }

//---------------------------------------------------------
//   seqRestart
//---------------------------------------------------------

bool MusE::seqRestart()
{
    bool restartSequencer = MusEGlobal::audio->isRunning();
    if (restartSequencer) {
          if (MusEGlobal::audio->isPlaying()) {
                MusEGlobal::audio->msgPlay(false);
                while (MusEGlobal::audio->isPlaying())
                      qApp->processEvents();
                }
          seqStop();
          }

    if(!seqStart())
        return false;

    MusEGlobal::audioDevice->graphChanged();
    return true;
}

//---------------------------------------------------------
//   addProject
//---------------------------------------------------------

void addProject(const QString& name)
      {
      for (int i = 0; i < PROJECT_LIST_LEN; ++i) {
            if (projectList[i] == 0)
                  break;
            if (name == *projectList[i]) {
                  int dst = i;
                  int src = i+1;
                  int n = PROJECT_LIST_LEN - i - 1;
                  delete projectList[i];
                  for (int k = 0; k < n; ++k)
                        projectList[dst++] = projectList[src++];
                  projectList[dst] = 0;
                  break;
                  }
            }
      QString** s = &projectList[PROJECT_LIST_LEN - 2];
      QString** d = &projectList[PROJECT_LIST_LEN - 1];
      if (*d)
            delete *d;
      for (int i = 0; i < PROJECT_LIST_LEN-1; ++i)
            *d-- = *s--;
      projectList[0] = new QString(name);
      }

//---------------------------------------------------------
//   MusE
//---------------------------------------------------------

MusE::MusE() : QMainWindow()
      {
      setIconSize(ICON_SIZE);
      setFocusPolicy(Qt::NoFocus);   
      MusEGlobal::muse      = this;    // hack
      clipListEdit          = 0;
      midiSyncConfig        = 0;
      midiRemoteConfig      = 0;
      midiPortConfig        = 0;
      metronomeConfig       = 0;
      audioConfig           = 0;
      midiFileConfig        = 0;
      midiFilterConfig      = 0;
      midiInputTransform    = 0;
      midiRhythmGenerator   = 0;
      globalSettingsConfig  = 0;
      markerView            = 0;
      arrangerView          = 0;
      softSynthesizerConfig = 0;
      midiTransformerDialog = 0;
      shortcutConfig        = 0;
      appearance            = 0;
      //audioMixer            = 0;
      mixer1                = 0;
      mixer2                = 0;
      watchdogThread        = 0;
      editInstrument        = 0;
      //routingPopupMenu      = 0;
      progress              = 0;
      activeTopWin          = NULL;
      currentMenuSharingTopwin = NULL;
      waitingForTopwin      = NULL;

      appName               = QString("MusE");
      setWindowTitle(appName);
      setWindowIcon(*MusEGui::museIcon);
      midiPluginSignalMapper = new QSignalMapper(this);
      followSignalMapper = new QSignalMapper(this);
      windowsMapper = new QSignalMapper(this);
      connect(windowsMapper, SIGNAL(mapped(QWidget*)), SLOT(bringToFront(QWidget*)));

      MusEGlobal::song = new MusECore::Song("song");
      MusEGlobal::song->blockSignals(true);
      MusEGlobal::heartBeatTimer = new QTimer(this);
      MusEGlobal::heartBeatTimer->setObjectName("timer");
      connect(MusEGlobal::heartBeatTimer, SIGNAL(timeout()), MusEGlobal::song, SLOT(beat()));
      connect(this, SIGNAL(activeTopWinChanged(MusEGui::TopWin*)), SLOT(activeTopWinChangedSlot(MusEGui::TopWin*)));
      
#ifdef ENABLE_PYTHON
      //---------------------------------------------------
      //    Python bridge
      //---------------------------------------------------
      // Uncomment in order to enable MusE Python bridge:
      if (MusEGlobal::usePythonBridge) {
            printf("Initializing python bridge!\n");
            if (MusECore::initPythonBridge() == false) {
                  printf("Could not initialize Python bridge\n");
                  exit(1);
                  }
            }
#endif

      //---------------------------------------------------
      //    undo/redo
      //---------------------------------------------------
      
      MusEGlobal::undoRedo = new QActionGroup(this);
      MusEGlobal::undoRedo->setExclusive(false);
      MusEGlobal::undoAction = new QAction(QIcon(*MusEGui::undoIconS), tr("Und&o"), 
        MusEGlobal::undoRedo);
      MusEGlobal::redoAction = new QAction(QIcon(*MusEGui::redoIconS), tr("Re&do"), 
        MusEGlobal::undoRedo);

      MusEGlobal::undoAction->setWhatsThis(tr("undo last change to song"));
      MusEGlobal::redoAction->setWhatsThis(tr("redo last undo"));
      MusEGlobal::undoAction->setEnabled(false);
      MusEGlobal::redoAction->setEnabled(false);
      connect(MusEGlobal::redoAction, SIGNAL(activated()), MusEGlobal::song, SLOT(redo()));
      connect(MusEGlobal::undoAction, SIGNAL(activated()), MusEGlobal::song, SLOT(undo()));

      //---------------------------------------------------
      //    Transport
      //---------------------------------------------------
      
      MusEGlobal::transportAction = new QActionGroup(this);
      MusEGlobal::transportAction->setExclusive(false);
      
      MusEGlobal::loopAction = new QAction(QIcon(*MusEGui::loop1Icon),
         tr("Loop"), MusEGlobal::transportAction);
      MusEGlobal::loopAction->setCheckable(true);

      MusEGlobal::loopAction->setWhatsThis(tr("loop between left mark and right mark"));
      connect(MusEGlobal::loopAction, SIGNAL(toggled(bool)), MusEGlobal::song, SLOT(setLoop(bool)));
      
      MusEGlobal::punchinAction = new QAction(QIcon(*MusEGui::punchin1Icon),
         tr("Punchin"), MusEGlobal::transportAction);
      MusEGlobal::punchinAction->setCheckable(true);

      MusEGlobal::punchinAction->setWhatsThis(tr("record starts at left mark"));
      connect(MusEGlobal::punchinAction, SIGNAL(toggled(bool)), MusEGlobal::song, SLOT(setPunchin(bool)));

      MusEGlobal::punchoutAction = new QAction(QIcon(*MusEGui::punchout1Icon),
         tr("Punchout"), MusEGlobal::transportAction);
      MusEGlobal::punchoutAction->setCheckable(true);

      MusEGlobal::punchoutAction->setWhatsThis(tr("record stops at right mark"));
      connect(MusEGlobal::punchoutAction, SIGNAL(toggled(bool)), MusEGlobal::song, SLOT(setPunchout(bool)));

      QAction *tseparator = new QAction(this);
      tseparator->setSeparator(true);
      MusEGlobal::transportAction->addAction(tseparator);

      MusEGlobal::startAction = new QAction(QIcon(*MusEGui::startIcon),
         tr("Start"), MusEGlobal::transportAction);

      MusEGlobal::startAction->setWhatsThis(tr("rewind to start position"));
      connect(MusEGlobal::startAction, SIGNAL(activated()), MusEGlobal::song, SLOT(rewindStart()));

      MusEGlobal::rewindAction = new QAction(QIcon(*MusEGui::frewindIcon),
         tr("Rewind"), MusEGlobal::transportAction);

      MusEGlobal::rewindAction->setWhatsThis(tr("rewind current position"));
      connect(MusEGlobal::rewindAction, SIGNAL(activated()), MusEGlobal::song, SLOT(rewind()));

      MusEGlobal::forwardAction = new QAction(QIcon(*MusEGui::fforwardIcon),
         tr("Forward"), MusEGlobal::transportAction);

      MusEGlobal::forwardAction->setWhatsThis(tr("move current position"));
      connect(MusEGlobal::forwardAction, SIGNAL(activated()), MusEGlobal::song, SLOT(forward()));

      MusEGlobal::stopAction = new QAction(QIcon(*MusEGui::stopIcon),
         tr("Stop"), MusEGlobal::transportAction);
      MusEGlobal::stopAction->setCheckable(true);

      MusEGlobal::stopAction->setWhatsThis(tr("stop sequencer"));
      MusEGlobal::stopAction->setChecked(true);
      connect(MusEGlobal::stopAction, SIGNAL(toggled(bool)), MusEGlobal::song, SLOT(setStop(bool)));

      MusEGlobal::playAction = new QAction(QIcon(*MusEGui::playIcon),
         tr("Play"), MusEGlobal::transportAction);
      MusEGlobal::playAction->setCheckable(true);

      MusEGlobal::playAction->setWhatsThis(tr("start sequencer play"));
      MusEGlobal::playAction->setChecked(false);
      connect(MusEGlobal::playAction, SIGNAL(toggled(bool)), MusEGlobal::song, SLOT(setPlay(bool)));

      MusEGlobal::recordAction = new QAction(QIcon(*MusEGui::recordIcon),
         tr("Record"), MusEGlobal::transportAction);
      MusEGlobal::recordAction->setCheckable(true);
      MusEGlobal::recordAction->setWhatsThis(tr("to record press record and then play"));
      connect(MusEGlobal::recordAction, SIGNAL(toggled(bool)), MusEGlobal::song, SLOT(setRecord(bool)));

      MusEGlobal::panicAction = new QAction(QIcon(*MusEGui::panicIcon),
         tr("Panic"), this);

      MusEGlobal::panicAction->setWhatsThis(tr("send note off to all midi channels"));
      connect(MusEGlobal::panicAction, SIGNAL(activated()), MusEGlobal::song, SLOT(panic()));

      //----Actions
      //-------- File Actions

      fileNewAction = new QAction(QIcon(*MusEGui::filenewIcon), tr("&New"), this); 
      fileNewAction->setToolTip(tr("Create New Song"));
      fileNewAction->setWhatsThis(tr("Create New Song"));

      fileOpenAction = new QAction(QIcon(*MusEGui::openIcon), tr("&Open"), this); 

      fileOpenAction->setToolTip(tr("Click this button to open a <em>new song</em>.<br>"
      "You can also select the <b>Open command</b> from the File menu."));
      fileOpenAction->setWhatsThis(tr("Click this button to open a <em>new song</em>.<br>"
      "You can also select the <b>Open command</b> from the File menu."));

      openRecent = new QMenu(tr("Open &Recent"), this);

      fileSaveAction = new QAction(QIcon(*MusEGui::saveIcon), tr("&Save"), this); 

      fileSaveAction->setToolTip(tr("Click this button to save the song you are "
      "editing.  You will be prompted for a file name.\n"
      "You can also select the Save command from the File menu."));
      fileSaveAction->setWhatsThis(tr("Click this button to save the song you are "
      "editing.  You will be prompted for a file name.\n"
      "You can also select the Save command from the File menu."));

      fileSaveAsAction = new QAction(tr("Save &As"), this);

      fileImportMidiAction = new QAction(tr("Import Midifile"), this);
      fileExportMidiAction = new QAction(tr("Export Midifile"), this);
      fileImportPartAction = new QAction(tr("Import Part"), this);

      fileImportWaveAction = new QAction(tr("Import Wave File"), this);
      fileMoveWaveFiles = new QAction(tr("Find unused wave files"), this);

      quitAction = new QAction(tr("&Quit"), this);

      editSongInfoAction = new QAction(QIcon(*MusEGui::edit_listIcon), tr("Song Info"), this);

      //-------- View Actions
      viewTransportAction = new QAction(QIcon(*MusEGui::view_transport_windowIcon), tr("Transport Panel"), this);
      viewTransportAction->setCheckable(true);
      viewBigtimeAction = new QAction(QIcon(*MusEGui::view_bigtime_windowIcon), tr("Bigtime Window"),  this);
      viewBigtimeAction->setCheckable(true);
      viewMixerAAction = new QAction(QIcon(*MusEGui::mixerSIcon), tr("Mixer A"), this);
      viewMixerAAction->setCheckable(true);
      viewMixerBAction = new QAction(QIcon(*MusEGui::mixerSIcon), tr("Mixer B"), this);
      viewMixerBAction->setCheckable(true);
      viewCliplistAction = new QAction(QIcon(*MusEGui::cliplistSIcon), tr("Cliplist"), this);
      viewCliplistAction->setCheckable(true);
      viewMarkerAction = new QAction(QIcon(*MusEGui::view_markerIcon), tr("Marker View"),  this);
      viewMarkerAction->setCheckable(true);
      viewArrangerAction = new QAction(tr("Arranger View"),  this);
      viewArrangerAction->setCheckable(true);
      fullscreenAction=new QAction(tr("Fullscreen"), this);
      fullscreenAction->setCheckable(true);
      fullscreenAction->setChecked(false);

      //-------- Midi Actions
      menuScriptPlugins = new QMenu(tr("&Plugins"), this);
      midiEditInstAction = new QAction(QIcon(*MusEGui::midi_edit_instrumentIcon), tr("Edit Instrument"), this);
      midiInputPlugins = new QMenu(tr("Input Plugins"), this);
      midiInputPlugins->setIcon(QIcon(*MusEGui::midi_inputpluginsIcon));
      midiTrpAction = new QAction(QIcon(*MusEGui::midi_inputplugins_transposeIcon), tr("Transpose"), this);
      midiInputTrfAction = new QAction(QIcon(*MusEGui::midi_inputplugins_midi_input_transformIcon), tr("Midi Input Transform"), this);
      midiInputFilterAction = new QAction(QIcon(*MusEGui::midi_inputplugins_midi_input_filterIcon), tr("Midi Input Filter"), this);
      midiRemoteAction = new QAction(QIcon(*MusEGui::midi_inputplugins_remote_controlIcon), tr("Midi Remote Control"), this);
#ifdef BUILD_EXPERIMENTAL
      midiRhythmAction = new QAction(QIcon(*midi_inputplugins_random_rhythm_generatorIcon), tr("Rhythm Generator"), this);
#endif
      midiResetInstAction = new QAction(QIcon(*MusEGui::midi_reset_instrIcon), tr("Reset Instr."), this);
      midiInitInstActions = new QAction(QIcon(*MusEGui::midi_init_instrIcon), tr("Init Instr."), this);
      midiLocalOffAction = new QAction(QIcon(*MusEGui::midi_local_offIcon), tr("Local Off"), this);

      //-------- Audio Actions
      audioBounce2TrackAction = new QAction(QIcon(*MusEGui::audio_bounce_to_trackIcon), tr("Bounce to Track"), this);
      audioBounce2FileAction = new QAction(QIcon(*MusEGui::audio_bounce_to_fileIcon), tr("Bounce to File"), this);
      audioRestartAction = new QAction(QIcon(*MusEGui::audio_restartaudioIcon), tr("Restart Audio"), this);

      //-------- Automation Actions
      autoMixerAction = new QAction(QIcon(*MusEGui::automation_mixerIcon), tr("Mixer Automation"), this);
      autoMixerAction->setCheckable(true);
      autoSnapshotAction = new QAction(QIcon(*MusEGui::automation_take_snapshotIcon), tr("Take Snapshot"), this);
      autoClearAction = new QAction(QIcon(*MusEGui::automation_clear_dataIcon), tr("Clear Automation Data"), this);

      //-------- Windows Actions
      windowsCascadeAction = new QAction(tr("Cascade"), this);
      windowsTileAction = new QAction(tr("Tile"), this);
      windowsRowsAction = new QAction(tr("In rows"), this);
      windowsColumnsAction = new QAction(tr("In columns"), this);
      
      
      //-------- Settings Actions
      settingsGlobalAction = new QAction(QIcon(*MusEGui::settings_globalsettingsIcon), tr("Global Settings"), this);
      settingsShortcutsAction = new QAction(QIcon(*MusEGui::settings_configureshortcutsIcon), tr("Configure Shortcuts"), this);
      follow = new QMenu(tr("Follow Song"), this);
      dontFollowAction = new QAction(tr("Don't Follow Song"), this);
      dontFollowAction->setCheckable(true);
      followPageAction = new QAction(tr("Follow Page"), this);
      followPageAction->setCheckable(true);
      followPageAction->setChecked(true);
      followCtsAction = new QAction(tr("Follow Continuous"), this);
      followCtsAction->setCheckable(true);

      settingsMetronomeAction = new QAction(QIcon(*MusEGui::settings_metronomeIcon), tr("Metronome"), this);
      settingsMidiSyncAction = new QAction(QIcon(*MusEGui::settings_midisyncIcon), tr("Midi Sync"), this);
      settingsMidiIOAction = new QAction(QIcon(*MusEGui::settings_midifileexportIcon), tr("Midi File Import/Export"), this);
      settingsAppearanceAction = new QAction(QIcon(*MusEGui::settings_appearance_settingsIcon), tr("Appearance Settings"), this);
      settingsMidiPortAction = new QAction(QIcon(*MusEGui::settings_midiport_softsynthsIcon), tr("Midi Ports / Soft Synth"), this);

      //-------- Help Actions
      helpManualAction = new QAction(tr("&Manual"), this);
      helpHomepageAction = new QAction(tr("&MusE Homepage"), this);
      helpReportAction = new QAction(tr("&Report Bug..."), this);
      helpAboutAction = new QAction(tr("&About MusE"), this);


      //---- Connections
      //-------- File connections

      connect(fileNewAction,  SIGNAL(activated()), SLOT(loadTemplate()));
      connect(fileOpenAction, SIGNAL(activated()), SLOT(loadProject()));
      connect(openRecent, SIGNAL(aboutToShow()), SLOT(openRecentMenu()));
      connect(openRecent, SIGNAL(triggered(QAction*)), SLOT(selectProject(QAction*)));
      
      connect(fileSaveAction, SIGNAL(activated()), SLOT(save()));
      connect(fileSaveAsAction, SIGNAL(activated()), SLOT(saveAs()));

      connect(fileImportMidiAction, SIGNAL(activated()), SLOT(importMidi()));
      connect(fileExportMidiAction, SIGNAL(activated()), SLOT(exportMidi()));
      connect(fileImportPartAction, SIGNAL(activated()), SLOT(importPart()));

      connect(fileImportWaveAction, SIGNAL(activated()), SLOT(importWave()));
      connect(fileMoveWaveFiles, SIGNAL(activated()), SLOT(findUnusedWaveFiles()));
      connect(quitAction, SIGNAL(activated()), SLOT(quitDoc()));

      connect(editSongInfoAction, SIGNAL(activated()), SLOT(startSongInfo()));

      //-------- View connections
      connect(viewTransportAction, SIGNAL(toggled(bool)), SLOT(toggleTransport(bool)));
      connect(viewBigtimeAction, SIGNAL(toggled(bool)), SLOT(toggleBigTime(bool)));
      connect(viewMixerAAction, SIGNAL(toggled(bool)),SLOT(toggleMixer1(bool)));
      connect(viewMixerBAction, SIGNAL(toggled(bool)), SLOT(toggleMixer2(bool)));
      connect(viewCliplistAction, SIGNAL(toggled(bool)), SLOT(startClipList(bool)));
      connect(viewMarkerAction, SIGNAL(toggled(bool)), SLOT(toggleMarker(bool)));
      connect(viewArrangerAction, SIGNAL(toggled(bool)), SLOT(toggleArranger(bool)));
      connect(fullscreenAction, SIGNAL(toggled(bool)), SLOT(setFullscreen(bool)));

      //-------- Midi connections
      connect(midiEditInstAction, SIGNAL(activated()), SLOT(startEditInstrument()));
      connect(midiResetInstAction, SIGNAL(activated()), SLOT(resetMidiDevices()));
      connect(midiInitInstActions, SIGNAL(activated()), SLOT(initMidiDevices()));
      connect(midiLocalOffAction, SIGNAL(activated()), SLOT(localOff()));

      connect(midiTrpAction, SIGNAL(triggered()), midiPluginSignalMapper, SLOT(map()));
      connect(midiInputTrfAction, SIGNAL(triggered()), midiPluginSignalMapper, SLOT(map()));
      connect(midiInputFilterAction, SIGNAL(triggered()), midiPluginSignalMapper, SLOT(map()));
      connect(midiRemoteAction, SIGNAL(triggered()), midiPluginSignalMapper, SLOT(map()));

      midiPluginSignalMapper->setMapping(midiTrpAction, 0);
      midiPluginSignalMapper->setMapping(midiInputTrfAction, 1);
      midiPluginSignalMapper->setMapping(midiInputFilterAction, 2);
      midiPluginSignalMapper->setMapping(midiRemoteAction, 3);

#ifdef BUILD_EXPERIMENTAL
      connect(midiRhythmAction, SIGNAL(triggered()), midiPluginSignalMapper, SLOT(map()));
      midiPluginSignalMapper->setMapping(midiRhythmAction, 4);
#endif

      connect(midiPluginSignalMapper, SIGNAL(mapped(int)), this, SLOT(startMidiInputPlugin(int)));
      
      //-------- Audio connections
      connect(audioBounce2TrackAction, SIGNAL(activated()), SLOT(bounceToTrack()));
      connect(audioBounce2FileAction, SIGNAL(activated()), SLOT(bounceToFile()));
      connect(audioRestartAction, SIGNAL(activated()), SLOT(seqRestart()));

      //-------- Automation connections
      connect(autoMixerAction, SIGNAL(activated()), SLOT(switchMixerAutomation()));
      connect(autoSnapshotAction, SIGNAL(activated()), SLOT(takeAutomationSnapshot()));
      connect(autoClearAction, SIGNAL(activated()), SLOT(clearAutomation()));

      //-------- Settings connections
      connect(settingsGlobalAction, SIGNAL(activated()), SLOT(configGlobalSettings()));
      connect(settingsShortcutsAction, SIGNAL(activated()), SLOT(configShortCuts()));
      connect(settingsMetronomeAction, SIGNAL(activated()), SLOT(configMetronome()));
      connect(settingsMidiSyncAction, SIGNAL(activated()), SLOT(configMidiSync()));
      connect(settingsMidiIOAction, SIGNAL(activated()), SLOT(configMidiFile()));
      connect(settingsAppearanceAction, SIGNAL(activated()), SLOT(configAppearance()));
      connect(settingsMidiPortAction, SIGNAL(activated()), SLOT(configMidiPorts()));

      connect(dontFollowAction, SIGNAL(triggered()), followSignalMapper, SLOT(map()));
      connect(followPageAction, SIGNAL(triggered()), followSignalMapper, SLOT(map()));
      connect(followCtsAction, SIGNAL(triggered()), followSignalMapper, SLOT(map()));

      followSignalMapper->setMapping(dontFollowAction, CMD_FOLLOW_NO);
      followSignalMapper->setMapping(followPageAction, CMD_FOLLOW_JUMP);
      followSignalMapper->setMapping(followCtsAction, CMD_FOLLOW_CONTINUOUS);

      connect(followSignalMapper, SIGNAL(mapped(int)), this, SLOT(cmd(int)));

      //-------- Help connections
      connect(helpManualAction, SIGNAL(activated()), SLOT(startHelpBrowser()));
      connect(helpHomepageAction, SIGNAL(activated()), SLOT(startHomepageBrowser()));
      connect(helpReportAction, SIGNAL(activated()), SLOT(startBugBrowser()));
      connect(helpAboutAction, SIGNAL(activated()), SLOT(about()));

      //--------------------------------------------------
      //    Toolbar
      //--------------------------------------------------
      
      // when adding a toolbar to the main window, remember adding it to
      // either the requiredToolbars or optionalToolbars list!

      QToolBar* songpos_tb;
      songpos_tb = addToolBar(tr("Song Position"));
      songpos_tb->setObjectName("Song Position");
      songpos_tb->addWidget(new MusEGui::SongPosToolbarWidget(songpos_tb));
      songpos_tb->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
      songpos_tb->setContextMenuPolicy(Qt::PreventContextMenu);
      
      QToolBar* tempo_tb;
      tempo_tb = addToolBar(tr("Tempo"));
      tempo_tb->setObjectName("Tempo");
      MusEGui::TempoToolbarWidget* tempo_tb_widget = new MusEGui::TempoToolbarWidget(tempo_tb);
      tempo_tb->addWidget(tempo_tb_widget);
      
      QToolBar* sig_tb;
      sig_tb = addToolBar(tr("Signature"));
      sig_tb->setObjectName("Signature");
      MusEGui::SigToolbarWidget* sig_tb_widget = new MusEGui::SigToolbarWidget(tempo_tb);
      sig_tb->addWidget(sig_tb_widget);
      
      tools = addToolBar(tr("File Buttons"));
      tools->setObjectName("File Buttons");
      tools->addAction(fileNewAction);
      tools->addAction(fileOpenAction);
      tools->addAction(fileSaveAction);
      tools->addAction(QWhatsThis::createAction(this));
      
      QToolBar* undoToolbar = addToolBar(tr("Undo/Redo"));
      undoToolbar->setObjectName("Undo/Redo (global)");
      undoToolbar->addActions(MusEGlobal::undoRedo->actions());

      QToolBar* transportToolbar = addToolBar(tr("Transport"));
      transportToolbar->setObjectName("Transport (global)");
      transportToolbar->addActions(MusEGlobal::transportAction->actions());

      QToolBar* panicToolbar = addToolBar(tr("Panic"));
      panicToolbar->setObjectName("Panic (global)");
      panicToolbar->addAction(MusEGlobal::panicAction);

      requiredToolbars.push_back(tools);
      optionalToolbars.push_back(songpos_tb);
      optionalToolbars.push_back(sig_tb);
      optionalToolbars.push_back(tempo_tb);
      optionalToolbars.push_back(undoToolbar);
      optionalToolbars.push_back(transportToolbar);
      optionalToolbars.push_back(panicToolbar);

       QSocketNotifier* ss = new QSocketNotifier(MusEGlobal::audio->getFromThreadFdr(), QSocketNotifier::Read, this); 
       connect(ss, SIGNAL(activated(int)), MusEGlobal::song, SLOT(seqSignal(int)));  

      //---------------------------------------------------
      //    Popups
      //---------------------------------------------------

      // when adding a menu to the main window, remember adding it to
      // either the leadingMenus or trailingMenus list!
      // also do NOT use menuBar()->addMenu(QString&), but ALWAYS
      // create the menu with new QMenu and add it afterwards.
      // the menu's owner must be this and not this->menuBar()!


      //-------------------------------------------------------------
      //    popup File
      //-------------------------------------------------------------

      menu_file = new QMenu(tr("&File"), this);
      menuBar()->addMenu(menu_file);
      leadingMenus.push_back(menu_file);
      menu_file->addAction(fileNewAction);
      menu_file->addAction(fileOpenAction);
      menu_file->addMenu(openRecent);
      menu_file->addSeparator();
      menu_file->addAction(fileSaveAction);
      menu_file->addAction(fileSaveAsAction);
      menu_file->addSeparator();
      menu_file->addAction(editSongInfoAction);
      menu_file->addSeparator();
      menu_file->addAction(fileImportMidiAction);
      menu_file->addAction(fileExportMidiAction);
      menu_file->addAction(fileImportPartAction);
      menu_file->addSeparator();
      menu_file->addAction(fileImportWaveAction);
      menu_file->addSeparator();
      menu_file->addAction(fileMoveWaveFiles);
      menu_file->addSeparator();
      menu_file->addAction(quitAction);
      menu_file->addSeparator();



      //-------------------------------------------------------------
      //    popup View
      //-------------------------------------------------------------

      menuView = new QMenu(tr("&View"), this);
      menuBar()->addMenu(menuView);
      trailingMenus.push_back(menuView);

      menuView->addAction(viewTransportAction);
      menuView->addAction(viewBigtimeAction);
      menuView->addAction(viewMixerAAction);
      menuView->addAction(viewMixerBAction);
      menuView->addAction(viewCliplistAction);
      menuView->addAction(viewMarkerAction);
      menuView->addAction(viewArrangerAction);
      menuView->addSeparator();
      menuView->addAction(fullscreenAction);

      
      //-------------------------------------------------------------
      //    popup Midi
      //-------------------------------------------------------------

      menu_functions = new QMenu(tr("&Midi"), this);
      menuBar()->addMenu(menu_functions);
      trailingMenus.push_back(menu_functions);
      
      MusEGlobal::song->populateScriptMenu(menuScriptPlugins, this);
      menu_functions->addMenu(menuScriptPlugins);
      menu_functions->addAction(midiEditInstAction);
      menu_functions->addMenu(midiInputPlugins);
      midiInputPlugins->addAction(midiTrpAction);
      midiInputPlugins->addAction(midiInputTrfAction);
      midiInputPlugins->addAction(midiInputFilterAction);
      midiInputPlugins->addAction(midiRemoteAction);
#ifdef BUILD_EXPERIMENTAL
      midiInputPlugins->addAction(midiRhythmAction);
#endif

      menu_functions->addSeparator();
      menu_functions->addAction(midiResetInstAction);
      menu_functions->addAction(midiInitInstActions);
      menu_functions->addAction(midiLocalOffAction);

      //-------------------------------------------------------------
      //    popup Audio
      //-------------------------------------------------------------

      menu_audio = new QMenu(tr("&Audio"), this);
      menuBar()->addMenu(menu_audio);
      trailingMenus.push_back(menu_audio);
      
      menu_audio->addAction(audioBounce2TrackAction);
      menu_audio->addAction(audioBounce2FileAction);
      menu_audio->addSeparator();
      menu_audio->addAction(audioRestartAction);


      //-------------------------------------------------------------
      //    popup Automation
      //-------------------------------------------------------------

      menuAutomation = new QMenu(tr("A&utomation"), this);
      menuBar()->addMenu(menuAutomation);
      trailingMenus.push_back(menuAutomation);
      
      menuAutomation->addAction(autoMixerAction);
      menuAutomation->addSeparator();
      menuAutomation->addAction(autoSnapshotAction);
      menuAutomation->addAction(autoClearAction);

      //-------------------------------------------------------------
      //    popup Windows
      //-------------------------------------------------------------

      menuWindows = new QMenu(tr("&Windows"), this);
      menuBar()->addMenu(menuWindows);
      trailingMenus.push_back(menuWindows);
      
      menuWindows->addAction(windowsCascadeAction); 
      menuWindows->addAction(windowsTileAction); 
      menuWindows->addAction(windowsRowsAction); 
      menuWindows->addAction(windowsColumnsAction); 

      //-------------------------------------------------------------
      //    popup Settings
      //-------------------------------------------------------------

      menuSettings = new QMenu(tr("MusE Se&ttings"), this);
      menuBar()->addMenu(menuSettings);
      trailingMenus.push_back(menuSettings);
      
      menuSettings->addAction(settingsGlobalAction);
      menuSettings->addAction(settingsShortcutsAction);
      menuSettings->addMenu(follow);
      follow->addAction(dontFollowAction);
      follow->addAction(followPageAction);
      follow->addAction(followCtsAction);
      menuSettings->addAction(settingsMetronomeAction);
      menuSettings->addSeparator();
      menuSettings->addAction(settingsMidiSyncAction);
      menuSettings->addAction(settingsMidiIOAction);
      menuSettings->addSeparator();
      menuSettings->addAction(settingsAppearanceAction);
      menuSettings->addSeparator();
      menuSettings->addAction(settingsMidiPortAction);

      //---------------------------------------------------
      //    popup Help
      //---------------------------------------------------

      menu_help = new QMenu(tr("&Help"), this);
      menuBar()->addMenu(menu_help);
      trailingMenus.push_back(menu_help);
      
      menu_help->addAction(helpManualAction);
      menu_help->addAction(helpHomepageAction);
      menu_help->addSeparator();
      menu_help->addAction(helpReportAction);
      menu_help->addSeparator();
      menu_help->addAction(helpAboutAction);

      menu_help->addAction(tr("About &Qt"), qApp, SLOT(aboutQt()));

      //---------------------------------------------------
      //    Central Widget
      //---------------------------------------------------

      
      mdiArea=new QMdiArea(this);
      mdiArea->setOption(QMdiArea::DontMaximizeSubWindowOnActivation);
      mdiArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
      mdiArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
      setCentralWidget(mdiArea);
      connect(windowsTileAction, SIGNAL(activated()), this, SLOT(tileSubWindows()));
      connect(windowsRowsAction, SIGNAL(activated()), this, SLOT(arrangeSubWindowsRows()));
      connect(windowsColumnsAction, SIGNAL(activated()), this, SLOT(arrangeSubWindowsColumns()));
      connect(windowsCascadeAction, SIGNAL(activated()), mdiArea, SLOT(cascadeSubWindows()));


      arrangerView = new MusEGui::ArrangerView(this);
      connect(arrangerView, SIGNAL(closed()), SLOT(arrangerClosed()));
      toplevels.push_back(arrangerView);
      arrangerView->hide();
      _arranger=arrangerView->getArranger();
      
      connect(tempo_tb_widget, SIGNAL(returnPressed()), arrangerView, SLOT(focusCanvas()));
      connect(tempo_tb_widget, SIGNAL(escapePressed()), arrangerView, SLOT(focusCanvas()));
      connect(sig_tb_widget,   SIGNAL(returnPressed()), arrangerView, SLOT(focusCanvas()));
      connect(sig_tb_widget,   SIGNAL(escapePressed()), arrangerView, SLOT(focusCanvas()));
      
      //---------------------------------------------------
      //  read list of "Recent Projects"
      //---------------------------------------------------

      QString prjPath(MusEGlobal::configPath);
      prjPath += QString("/projects");
      FILE* f = fopen(prjPath.toLatin1().constData(), "r");
      if (f == 0) {
            perror("open projectfile");
            for (int i = 0; i < PROJECT_LIST_LEN; ++i)
                  projectList[i] = 0;
            }
      else {
            for (int i = 0; i < PROJECT_LIST_LEN; ++i) {
                  char buffer[256];
                  if (fgets(buffer, 256, f)) {
                        int n = strlen(buffer);
                        if (n && buffer[n-1] == '\n')
                              buffer[n-1] = 0;
                        projectList[i] = *buffer ? new QString(buffer) : 0;
                        }
                  else
                        break;
                  }
            fclose(f);
            }

      arrangerView->populateAddTrack();
      arrangerView->updateShortcuts();
      
      transport = new MusEGui::Transport(this, "transport");
      bigtime   = 0;

      MusEGlobal::song->blockSignals(false);
      
      changeConfig(false);
      QSettings settings("MusE", "MusE-qt");
      restoreGeometry(settings.value("MusE/geometry").toByteArray());

      MusEGlobal::song->update();
      updateWindowMenu();
      }

//---------------------------------------------------------
//   setHeartBeat
//---------------------------------------------------------

void MusE::setHeartBeat()
      {
      MusEGlobal::heartBeatTimer->start(1000/MusEGlobal::config.guiRefresh);
      }

//---------------------------------------------------
//  loadDefaultSong
//    if no songname entered on command line:
//    startMode: 0  - load last song
//               1  - load default template
//               2  - load configured start song
//---------------------------------------------------

void MusE::loadDefaultSong(int argc, char** argv)
{
  QString name;
  bool useTemplate = false;
  bool loadConfig = true;
  if (argc >= 2)
        name = argv[0];
  else if (MusEGlobal::config.startMode == 0) {
        if (argc < 2)
              name = projectList[0] ? *projectList[0] : MusEGui::getUniqueUntitledName();  
        else
              name = argv[0];
        printf("starting with selected song %s\n", MusEGlobal::config.startSong.toLatin1().constData());
        }
  else if (MusEGlobal::config.startMode == 1) {
        if(MusEGlobal::config.startSong.isEmpty()) // Sanity check to avoid some errors later
        {
          name = MusEGlobal::museGlobalShare + QString("/templates/default.med");
          loadConfig = false;
        }
        else
        {
          name = MusEGlobal::config.startSong;
          loadConfig = MusEGlobal::config.startSongLoadConfig;
        }
        useTemplate = true;
        printf("starting with template %s\n", name.toLatin1().constData());
        }
  else if (MusEGlobal::config.startMode == 2) {
        if(MusEGlobal::config.startSong.isEmpty()) // Sanity check to avoid some errors later
        {
          name = MusEGlobal::museGlobalShare + QString("/templates/default.med");
          useTemplate = true;
          loadConfig = false;
        }
        else
        {
          name = MusEGlobal::config.startSong;
          loadConfig = MusEGlobal::config.startSongLoadConfig;
        }
        printf("starting with pre configured song %s\n", MusEGlobal::config.startSong.toLatin1().constData());
  }
  loadProjectFile(name, useTemplate, loadConfig);  
}
      
//---------------------------------------------------------
//   resetDevices
//---------------------------------------------------------

void MusE::resetMidiDevices()
      {
      MusEGlobal::audio->msgResetMidiDevices();
      }

//---------------------------------------------------------
//   initMidiDevices
//---------------------------------------------------------

void MusE::initMidiDevices()
      {
      //MusEGlobal::audio->msgIdle(true); 
      MusEGlobal::audio->msgInitMidiDevices();
      //MusEGlobal::audio->msgIdle(false); 
      }

//---------------------------------------------------------
//   localOff
//---------------------------------------------------------

void MusE::localOff()
      {
      MusEGlobal::audio->msgLocalOff();
      }

//---------------------------------------------------------
//   loadProjectFile
//    load *.med, *.mid, *.kar
//
//    template - if true, load file but do not change
//                project name
//---------------------------------------------------------

// for drop:
void MusE::loadProjectFile(const QString& name)
      {
      loadProjectFile(name, false, false);
      }

void MusE::loadProjectFile(const QString& name, bool songTemplate, bool doReadMidiPorts)
      {
      QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

      if(!progress)
        progress = new QProgressDialog();
      QString label = "loading project "+QFileInfo(name).fileName();
        if (!songTemplate) {
          switch (random()%10) {
        case 0:
            label.append("\nThe best song in the world?");
          break;
        case 1:
            label.append("\nAwesome stuff!");
          break;
        case 2:
            label.append("\nCool rhythms!");
          break;
        case 3:
            label.append("\nA truly lovely song");
          break;
        default:
          break;
        }
      }
      progress->setLabelText(label);
      progress->setWindowModality(Qt::WindowModal);
      progress->setCancelButton(0);
      if (!songTemplate)
        progress->setMinimumDuration(0); // if we are loading a template it will probably be fast and we can wait before showing the dialog
      
      //
      // stop audio threads if running
      //
      progress->setValue(0);
      bool restartSequencer = MusEGlobal::audio->isRunning();
      if (restartSequencer) {
            if (MusEGlobal::audio->isPlaying()) {
                  MusEGlobal::audio->msgPlay(false);
                  while (MusEGlobal::audio->isPlaying())
                        qApp->processEvents();
                  }
            seqStop();
            }
      microSleep(100000);
      progress->setValue(10);
      loadProjectFile1(name, songTemplate, doReadMidiPorts);
      microSleep(100000);
      progress->setValue(90);
      if (restartSequencer)
            seqStart();

      arrangerView->updateVisibleTracksButtons();
      progress->setValue(100);
      delete progress;
      progress=0;

      QApplication::restoreOverrideCursor();

      if (MusEGlobal::song->getSongInfo().length()>0 && MusEGlobal::song->showSongInfoOnStartup()) {
          startSongInfo(false);
        }
      }

//---------------------------------------------------------
//   loadProjectFile
//    load *.med, *.mid, *.kar
//
//    template - if true, load file but do not change
//                project name
//    doReadMidiPorts  - also read midi port configuration
//---------------------------------------------------------

void MusE::loadProjectFile1(const QString& name, bool songTemplate, bool doReadMidiPorts)
      {
      if (mixer1)
            mixer1->clear();
      if (mixer2)
            mixer2->clear();
      _arranger->clear();      // clear track info
      if (clearSong(doReadMidiPorts))  // Allow not touching things like midi ports. 
            return;
      progress->setValue(20);

      QFileInfo fi(name);
      if (songTemplate) {
            if (!fi.isReadable()) {
                  QMessageBox::critical(this, QString("MusE"),
                     tr("Cannot read template"));
                  QApplication::restoreOverrideCursor();
                  return;
                  }
            project.setFile(MusEGui::getUniqueUntitledName());  
            MusEGlobal::museProject = MusEGlobal::museProjectInitPath;
            }
      else {
            printf("Setting project path to %s\n", fi.absolutePath().toLatin1().constData());
            MusEGlobal::museProject = fi.absolutePath();
            project.setFile(name);
            }
      QString ex = fi.completeSuffix().toLower();
      QString mex = ex.section('.', -1, -1);  
      if((mex == "gz") || (mex == "bz2"))
        mex = ex.section('.', -2, -2);  
        
      if (ex.isEmpty() || mex == "med") {
            //
            //  read *.med file
            //
            bool popenFlag;
            FILE* f = MusEGui::fileOpen(this, fi.filePath(), QString(".med"), "r", popenFlag, true);
            if (f == 0) {
                  if (errno != ENOENT) {
                        QMessageBox::critical(this, QString("MusE"),
                           tr("File open error"));
                        setUntitledProject();
                        }
                  else
                        setConfigDefaults();
                  }
            else {
                  MusECore::Xml xml(f);
                  read(xml, doReadMidiPorts, songTemplate);
                  bool fileError = ferror(f);
                  popenFlag ? pclose(f) : fclose(f);
                  if (fileError) {
                        QMessageBox::critical(this, QString("MusE"),
                           tr("File read error"));
                        setUntitledProject();
                        }
                  }
            }
      else if (mex == "mid" || mex == "kar") {
            setConfigDefaults();
            if (!importMidi(name, false))
                  setUntitledProject();
            }
      else {
            QMessageBox::critical(this, QString("MusE"),
               tr("Unknown File Format: %1").arg(ex));
            setUntitledProject();
            }
      if (!songTemplate) {
            addProject(project.absoluteFilePath());
            setWindowTitle(QString("MusE: Song: ") + MusEGui::projectTitleFromFilename(project.absoluteFilePath()));
            }
      MusEGlobal::song->dirty = false;
      progress->setValue(30);

      viewTransportAction->setChecked(MusEGlobal::config.transportVisible);
      viewBigtimeAction->setChecked(MusEGlobal::config.bigTimeVisible);
      viewMarkerAction->setChecked(MusEGlobal::config.markerVisible);
      viewArrangerAction->setChecked(MusEGlobal::config.arrangerVisible);

      autoMixerAction->setChecked(MusEGlobal::automation);

      showBigtime(MusEGlobal::config.bigTimeVisible);
      showMixer1(MusEGlobal::config.mixer1Visible);
      showMixer2(MusEGlobal::config.mixer2Visible);
      
      // Added p3.3.43 Make sure the geometry is correct because showMixerX() will NOT 
      //  set the geometry if the mixer has already been created.
      if(mixer1)
      {
        //if(mixer1->geometry().size() != MusEGlobal::config.mixer1.geometry.size())   // Moved below
        //  mixer1->resize(MusEGlobal::config.mixer1.geometry.size());
        
        if(mixer1->geometry().topLeft() != MusEGlobal::config.mixer1.geometry.topLeft())
          mixer1->move(MusEGlobal::config.mixer1.geometry.topLeft());
      }
      if(mixer2)
      {
        //if(mixer2->geometry().size() != MusEGlobal::config.mixer2.geometry.size())   // Moved below
        //  mixer2->resize(MusEGlobal::config.mixer2.geometry.size());
        
        if(mixer2->geometry().topLeft() != MusEGlobal::config.mixer2.geometry.topLeft())
          mixer2->move(MusEGlobal::config.mixer2.geometry.topLeft());
      }
      
      //showMarker(MusEGlobal::config.markerVisible);  // Moved below. Tim.
      resize(MusEGlobal::config.geometryMain.size());
      move(MusEGlobal::config.geometryMain.topLeft());

      if (MusEGlobal::config.transportVisible)
            transport->show();
      transport->move(MusEGlobal::config.geometryTransport.topLeft());
      showTransport(MusEGlobal::config.transportVisible);
      
      progress->setValue(40);

      transport->setMasterFlag(MusEGlobal::song->masterFlag());
      MusEGlobal::punchinAction->setChecked(MusEGlobal::song->punchin());
      MusEGlobal::punchoutAction->setChecked(MusEGlobal::song->punchout());
      MusEGlobal::loopAction->setChecked(MusEGlobal::song->loop());
      MusEGlobal::song->update();
      MusEGlobal::song->updatePos();
      arrangerView->clipboardChanged(); // enable/disable "Paste"
      arrangerView->selectionChanged(); // enable/disable "Copy" & "Paste"
      arrangerView->scoreNamingChanged(); // inform the score menus about the new scores and their names
      progress->setValue(50);

      // Try this AFTER the song update above which does a mixer update... Tested OK - mixers resize properly now.
      if(mixer1)
      {
        if(mixer1->geometry().size() != MusEGlobal::config.mixer1.geometry.size())
          mixer1->resize(MusEGlobal::config.mixer1.geometry.size());
      }  
      if(mixer2)
      {
        if(mixer2->geometry().size() != MusEGlobal::config.mixer2.geometry.size())
          mixer2->resize(MusEGlobal::config.mixer2.geometry.size());
      }  
      
      // Moved here from above due to crash with a song loaded and then File->New.
      // Marker view list was not updated, had non-existent items from marker list (cleared in ::clear()).
      showMarker(MusEGlobal::config.markerVisible); 
      
      if (songTemplate)
      {
        // maximize the arranger when in traditional SDI mode
        if (MusEGui::TopWin::_defaultSubwin[MusEGui::TopWin::ARRANGER])
        {
          bool maximizeArranger=true;
          for (int i=0; i<MusEGui::TopWin::TOPLEVELTYPE_LAST_ENTRY; i++)
            if ((i!=MusEGui::TopWin::ARRANGER) && MusEGui::TopWin::_defaultSubwin[i])
            {
              maximizeArranger=false;
              break;
            }
          
          if (maximizeArranger)
          {
            arrangerView->showMaximized();
            bringToFront(arrangerView);
          }
        }
      }
      
      }

//---------------------------------------------------------
//   setUntitledProject
//---------------------------------------------------------

void MusE::setUntitledProject()
      {
      setConfigDefaults();
      QString name(MusEGui::getUniqueUntitledName());  
      MusEGlobal::museProject = "./"; 
      project.setFile(name);
      setWindowTitle(tr("MusE: Song: %1").arg(MusEGui::projectTitleFromFilename(name)));
      writeTopwinState=true;
      }

//---------------------------------------------------------
//   setConfigDefaults
//---------------------------------------------------------

void MusE::setConfigDefaults()
      {
      MusECore::readConfiguration();    // used for reading midi files
      MusEGlobal::song->dirty = false;
      }

//---------------------------------------------------------
//   setFollow
//---------------------------------------------------------

void MusE::setFollow()
      {
      MusECore::Song::FollowMode fm = MusEGlobal::song->follow();
      
      dontFollowAction->setChecked(fm == MusECore::Song::NO);
      followPageAction->setChecked(fm == MusECore::Song::JUMP);
      followCtsAction->setChecked(fm == MusECore::Song::CONTINUOUS);
      }

//---------------------------------------------------------
//   MusE::loadProject
//---------------------------------------------------------

void MusE::loadProject()
      {
      bool doReadMidiPorts;
      QString fn = MusEGui::getOpenFileName(QString(""), MusEGlobal::med_file_pattern, this,
         tr("MusE: load project"), &doReadMidiPorts);
      if (!fn.isEmpty()) {
            MusEGlobal::museProject = QFileInfo(fn).absolutePath();
            loadProjectFile(fn, false, doReadMidiPorts);
            }
      }

//---------------------------------------------------------
//   loadTemplate
//---------------------------------------------------------

void MusE::loadTemplate()
      {
      bool doReadMidiPorts;
      QString fn = MusEGui::getOpenFileName(QString("templates"), MusEGlobal::med_file_pattern, this,
                                               tr("MusE: load template"), &doReadMidiPorts, MusEGui::MFileDialog::GLOBAL_VIEW);
      if (!fn.isEmpty()) {
            loadProjectFile(fn, true, doReadMidiPorts);
            setUntitledProject();
            }
      }

//---------------------------------------------------------
//   save
//---------------------------------------------------------

bool MusE::save()
      {
      if (MusEGlobal::museProject == MusEGlobal::museProjectInitPath )  
            return saveAs();
      else
            return save(project.filePath(), false, writeTopwinState);
      }

//---------------------------------------------------------
//   save
//---------------------------------------------------------

bool MusE::save(const QString& name, bool overwriteWarn, bool writeTopwins)
      {
      QString backupCommand;

      if (QFile::exists(name)) {
            backupCommand.sprintf("cp \"%s\" \"%s.backup\"", name.toLatin1().constData(), name.toLatin1().constData());
            }
      else if (QFile::exists(name + QString(".med"))) {
            backupCommand.sprintf("cp \"%s.med\" \"%s.med.backup\"", name.toLatin1().constData(), name.toLatin1().constData());
            }
      if (!backupCommand.isEmpty())
            system(backupCommand.toLatin1().constData());

      bool popenFlag;
      FILE* f = MusEGui::fileOpen(this, name, QString(".med"), "w", popenFlag, false, overwriteWarn);
      if (f == 0)
            return false;
      MusECore::Xml xml(f);
      write(xml, writeTopwins);
      if (ferror(f)) {
            QString s = "Write File\n" + name + "\nfailed: "
               + QString(strerror(errno));                 
            QMessageBox::critical(this,
               tr("MusE: Write File failed"), s);
            popenFlag? pclose(f) : fclose(f);
            unlink(name.toLatin1().constData());
            return false;
            }
      else {
            popenFlag? pclose(f) : fclose(f);
            MusEGlobal::song->dirty = false;
            return true;
            }
      }

//---------------------------------------------------------
//   quitDoc
//---------------------------------------------------------

void MusE::quitDoc()
      {
      close();
      }

//---------------------------------------------------------
//   closeEvent
//---------------------------------------------------------

void MusE::closeEvent(QCloseEvent* event)
      {
      QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
      MusEGlobal::song->setStop(true);
      //
      // wait for sequencer
      //
      while (MusEGlobal::audio->isPlaying()) {
            qApp->processEvents();
            }
      if (MusEGlobal::song->dirty) {
            int n = 0;
            n = QMessageBox::warning(this, appName,
               tr("The current Project contains unsaved data\n"
               "Save Current Project?"),
               tr("&Save"), tr("S&kip"), tr("&Cancel"), 0, 2);
            if (n == 0) {
                  if (!save())      // dont quit if save failed
                  {
                        event->ignore();
                        QApplication::restoreOverrideCursor();
                        return;
                  }      
                  }
            else if (n == 2)
            {
                  event->ignore();
                  QApplication::restoreOverrideCursor();
                  return;
            }      
            }
      seqStop();

      MusECore::WaveTrackList* wt = MusEGlobal::song->waves();
      for (MusECore::iWaveTrack iwt = wt->begin(); iwt != wt->end(); ++iwt) {
            MusECore::WaveTrack* t = *iwt;
            if (t->recFile() && t->recFile()->samples() == 0) {
                  t->recFile()->remove();
                  }
            }

      QSettings settings("MusE", "MusE-qt");
      settings.setValue("MusE/geometry", saveGeometry());
      
      writeGlobalConfiguration();

      // save "Open Recent" list
      QString prjPath(MusEGlobal::configPath);
      prjPath += "/projects";
      FILE* f = fopen(prjPath.toLatin1().constData(), "w");
      if (f) {
            for (int i = 0; i < PROJECT_LIST_LEN; ++i) {
                  fprintf(f, "%s\n", projectList[i] ? projectList[i]->toLatin1().constData() : "");
                  }
            fclose(f);
            }
      if(MusEGlobal::debugMsg)
        printf("MusE: Exiting JackAudio\n");
      MusECore::exitJackAudio();
      if(MusEGlobal::debugMsg)
        printf("MusE: Exiting DummyAudio\n");
      MusECore::exitDummyAudio();
      if(MusEGlobal::debugMsg)
        printf("MusE: Exiting Metronome\n");
      MusECore::exitMetronome();
      
      MusEGlobal::song->cleanupForQuit();

      // Give midi devices a chance to close first, above in cleanupForQuit.
      if(MusEGlobal::debugMsg)
        printf("Muse: Exiting ALSA midi\n");
      MusECore::exitMidiAlsa();

      if(MusEGlobal::debugMsg)
        printf("Muse: Cleaning up temporary wavefiles + peakfiles\n");
      // Cleanup temporary wavefiles + peakfiles used for undo
      for (std::list<QString>::iterator i = MusECore::temporaryWavFiles.begin(); i != MusECore::temporaryWavFiles.end(); i++) {
            QString filename = *i;
            QFileInfo f(filename);
            QDir d = f.dir();
            d.remove(filename);
            d.remove(f.completeBaseName() + ".wca");
            }
      
#ifdef HAVE_LASH
      // Disconnect gracefully from LASH. 
      if(lash_client)
      {
        if(MusEGlobal::debugMsg)
          printf("MusE: Disconnecting from LASH\n");
        lash_event_t* lashev = lash_event_new_with_type (LASH_Quit);
        lash_send_event(lash_client, lashev);
      }
#endif      
      
      if(MusEGlobal::debugMsg)
        printf("MusE: Exiting Dsp\n");
      AL::exitDsp();
      
      if(MusEGlobal::debugMsg)
        printf("MusE: Exiting OSC\n");
      MusECore::exitOSC();
      
      delete MusEGlobal::audioPrefetch;
      delete MusEGlobal::audio;
      delete MusEGlobal::midiSeq;
      delete MusEGlobal::song;
      
      if(MusEGlobal::debugMsg)
        printf("MusE: Deleting icons\n");
      deleteIcons();
      
      qApp->quit();
      }

//---------------------------------------------------------
//   toggleMarker
//---------------------------------------------------------

void MusE::toggleMarker(bool checked)
      {
      showMarker(checked);
      }

//---------------------------------------------------------
//   showMarker
//---------------------------------------------------------

void MusE::showMarker(bool flag)
      {
      if (markerView == 0) {
            markerView = new MusEGui::MarkerView(this);

            connect(markerView, SIGNAL(closed()), SLOT(markerClosed()));
            markerView->show();
            toplevels.push_back(markerView);
            }
      markerView->setVisible(flag);
      viewMarkerAction->setChecked(flag);
      if (!flag)
        if (currentMenuSharingTopwin == markerView)
          setCurrentMenuSharingTopwin(NULL);
      
      updateWindowMenu();
      }

//---------------------------------------------------------
//   markerClosed
//---------------------------------------------------------

void MusE::markerClosed()
      {
      viewMarkerAction->setChecked(false);
      if (currentMenuSharingTopwin == markerView)
        setCurrentMenuSharingTopwin(NULL);

      updateWindowMenu();

      // focus the last activated topwin which is not the marker view
      QList<QMdiSubWindow*> l = mdiArea->subWindowList(QMdiArea::StackingOrder);
      for (QList<QMdiSubWindow*>::iterator lit=l.begin(); lit!=l.end(); lit++)
        if ((*lit)->isVisible() && (*lit)->widget() != markerView)
        {
          if (MusEGlobal::debugMsg)
            printf("bringing '%s' to front instead of closed marker window\n",(*lit)->widget()->windowTitle().toAscii().data());

          bringToFront((*lit)->widget());

          break; 
        }
      
      }

//---------------------------------------------------------
//   toggleArranger
//---------------------------------------------------------

void MusE::toggleArranger(bool checked)
      {
      showArranger(checked);
      }

//---------------------------------------------------------
//   showArranger
//---------------------------------------------------------

void MusE::showArranger(bool flag)
      {
      arrangerView->setVisible(flag);
      viewArrangerAction->setChecked(flag);
      if (!flag)
        if (currentMenuSharingTopwin == arrangerView)
          setCurrentMenuSharingTopwin(NULL);
      updateWindowMenu();
      }

//---------------------------------------------------------
//   arrangerClosed
//---------------------------------------------------------

void MusE::arrangerClosed()
      {
      viewArrangerAction->setChecked(false);
      updateWindowMenu();

      // focus the last activated topwin which is not the arranger view
      QList<QMdiSubWindow*> l = mdiArea->subWindowList(QMdiArea::StackingOrder);
      for (QList<QMdiSubWindow*>::iterator lit=l.begin(); lit!=l.end(); lit++)
        if ((*lit)->isVisible() && (*lit)->widget() != arrangerView)
        {
          if (MusEGlobal::debugMsg)
            printf("bringing '%s' to front instead of closed arranger window\n",(*lit)->widget()->windowTitle().toAscii().data());

          bringToFront((*lit)->widget());

          break; 
        }
      
      }

//---------------------------------------------------------
//   toggleTransport
//---------------------------------------------------------

void MusE::toggleTransport(bool checked)
      {
      showTransport(checked);
      }

//---------------------------------------------------------
//   showTransport
//---------------------------------------------------------

void MusE::showTransport(bool flag)
      {
      transport->setVisible(flag);
      viewTransportAction->setChecked(flag);
      }

//---------------------------------------------------------
//   saveAs
//---------------------------------------------------------

bool MusE::saveAs()
      {
      QString name;
        if (MusEGlobal::config.useProjectSaveDialog) {
            MusEGui::ProjectCreateImpl pci(MusEGlobal::muse);
            pci.setWriteTopwins(writeTopwinState);
            if (pci.exec() == QDialog::Rejected) {
              return false;
            }

            MusEGlobal::song->setSongInfo(pci.getSongInfo(), true);
            name = pci.getProjectPath();
            writeTopwinState=pci.getWriteTopwins();
          } else {
            name = MusEGui::getSaveFileName(QString(""), MusEGlobal::med_file_save_pattern, this, tr("MusE: Save As"), &writeTopwinState);
            if (name.isEmpty())
              return false;
          }

        MusEGlobal::museProject = QFileInfo(name).absolutePath();
        QDir dirmanipulator;
        if (!dirmanipulator.mkpath(MusEGlobal::museProject)) {
          QMessageBox::warning(this,"Path error","Can't create project path", QMessageBox::Ok);
          return false;
        }
      
      bool ok = false;
      if (!name.isEmpty()) {
            QString tempOldProj = MusEGlobal::museProject;
            MusEGlobal::museProject = QFileInfo(name).absolutePath();
            ok = save(name, true, writeTopwinState);
            if (ok) {
                  project.setFile(name);
                  setWindowTitle(tr("MusE: Song: %1").arg(MusEGui::projectTitleFromFilename(name)));
                  addProject(name);
                  }
            else
                  MusEGlobal::museProject = tempOldProj;
            }

      return ok;
      }

//---------------------------------------------------------
//   startEditor
//---------------------------------------------------------

void MusE::startEditor(MusECore::PartList* pl, int type)
      {
      switch (type) {
            case 0: startPianoroll(pl, true); break;
            case 1: startListEditor(pl); break;
            case 3: startDrumEditor(pl, true); break;
            case 4: startWaveEditor(pl); break;
            }
      }

//---------------------------------------------------------
//   startEditor
//---------------------------------------------------------

void MusE::startEditor(MusECore::Track* t)
      {
      switch (t->type()) {
            case MusECore::Track::MIDI: startPianoroll(); break;  
            case MusECore::Track::DRUM: startDrumEditor(); break;
            case MusECore::Track::WAVE: startWaveEditor(); break;
            default:
                  break;
            }
      }

//---------------------------------------------------------
//   getMidiPartsToEdit
//---------------------------------------------------------

MusECore::PartList* MusE::getMidiPartsToEdit()
      {
      MusECore::PartList* pl = MusEGlobal::song->getSelectedMidiParts();
      if (pl->empty()) {
            QMessageBox::critical(this, QString("MusE"), tr("Nothing to edit"));
            return 0;
            }
      return pl;
      }


//---------------------------------------------------------
//   startScoreEdit
//---------------------------------------------------------

void MusE::openInScoreEdit_oneStaffPerTrack(QWidget* dest)
{
	openInScoreEdit((MusEGui::ScoreEdit*)dest, false);
}

void MusE::openInScoreEdit_allInOne(QWidget* dest)
{
	openInScoreEdit((MusEGui::ScoreEdit*)dest, true);
}

void MusE::openInScoreEdit(MusEGui::ScoreEdit* destination, bool allInOne)
{
	MusECore::PartList* pl = getMidiPartsToEdit();
	if (pl == 0)
				return;
	openInScoreEdit(destination, pl, allInOne);
}

void MusE::openInScoreEdit(MusEGui::ScoreEdit* destination, MusECore::PartList* pl, bool allInOne)
{
	if (destination==NULL) // if no destination given, create a new one
	{
      destination = new MusEGui::ScoreEdit(this, 0, _arranger->cursorValue());
      toplevels.push_back(destination);
      destination->show();
      connect(destination, SIGNAL(isDeleting(MusEGui::TopWin*)), SLOT(toplevelDeleting(MusEGui::TopWin*)));
      connect(destination, SIGNAL(name_changed()), arrangerView, SLOT(scoreNamingChanged()));
      //connect(muse, SIGNAL(configChanged()), destination, SLOT(config_changed()));
      //commented out by flo, because the ScoreEditor connects to all 
      //relevant signals on his own
      
      arrangerView->updateScoreMenus();
      updateWindowMenu();
  }
  
  destination->add_parts(pl, allInOne);
}

void MusE::startScoreQuickly()
{
	openInScoreEdit_oneStaffPerTrack(NULL);
}

//---------------------------------------------------------
//   startPianoroll
//---------------------------------------------------------

void MusE::startPianoroll()
      {
      MusECore::PartList* pl = getMidiPartsToEdit();
      if (pl == 0)
            return;
      startPianoroll(pl, true);
      }

void MusE::startPianoroll(MusECore::PartList* pl, bool showDefaultCtrls)
      {
      
      MusEGui::PianoRoll* pianoroll = new MusEGui::PianoRoll(pl, this, 0, _arranger->cursorValue());
      if(showDefaultCtrls)       
        pianoroll->addCtrl();
      toplevels.push_back(pianoroll);
      pianoroll->show();
      connect(pianoroll, SIGNAL(isDeleting(MusEGui::TopWin*)), SLOT(toplevelDeleting(MusEGui::TopWin*)));
      connect(MusEGlobal::muse, SIGNAL(configChanged()), pianoroll, SLOT(configChanged()));
      updateWindowMenu();
      }

//---------------------------------------------------------
//   startListenEditor
//---------------------------------------------------------

void MusE::startListEditor()
      {
      MusECore::PartList* pl = getMidiPartsToEdit();
      if (pl == 0)
            return;
      startListEditor(pl);
      }

void MusE::startListEditor(MusECore::PartList* pl)
      {
      MusEGui::ListEdit* listEditor = new MusEGui::ListEdit(pl);
      toplevels.push_back(listEditor);
      listEditor->show();
      connect(listEditor, SIGNAL(isDeleting(MusEGui::TopWin*)), SLOT(toplevelDeleting(MusEGui::TopWin*)));
      connect(MusEGlobal::muse,SIGNAL(configChanged()), listEditor, SLOT(configChanged()));
      updateWindowMenu();
      }

//---------------------------------------------------------
//   startMasterEditor
//---------------------------------------------------------

void MusE::startMasterEditor()
      {
      MusEGui::MasterEdit* masterEditor = new MusEGui::MasterEdit();
      toplevels.push_back(masterEditor);
      masterEditor->show();
      connect(masterEditor, SIGNAL(isDeleting(MusEGui::TopWin*)), SLOT(toplevelDeleting(MusEGui::TopWin*)));
      updateWindowMenu();
      }

//---------------------------------------------------------
//   startLMasterEditor
//---------------------------------------------------------

void MusE::startLMasterEditor()
      {
      MusEGui::LMaster* lmaster = new MusEGui::LMaster();
      toplevels.push_back(lmaster);
      lmaster->show();
      connect(lmaster, SIGNAL(isDeleting(MusEGui::TopWin*)), SLOT(toplevelDeleting(MusEGui::TopWin*)));
      connect(MusEGlobal::muse, SIGNAL(configChanged()), lmaster, SLOT(configChanged()));
      updateWindowMenu();
      }

//---------------------------------------------------------
//   startDrumEditor
//---------------------------------------------------------

void MusE::startDrumEditor()
      {
      MusECore::PartList* pl = getMidiPartsToEdit();
      if (pl == 0)
            return;
      startDrumEditor(pl, true);
      }

void MusE::startDrumEditor(MusECore::PartList* pl, bool showDefaultCtrls)
      {
      MusEGui::DrumEdit* drumEditor = new MusEGui::DrumEdit(pl, this, 0, _arranger->cursorValue());
      if(showDefaultCtrls)       
        drumEditor->addCtrl();
      toplevels.push_back(drumEditor);
      drumEditor->show();
      connect(drumEditor, SIGNAL(isDeleting(MusEGui::TopWin*)), SLOT(toplevelDeleting(MusEGui::TopWin*)));
      connect(MusEGlobal::muse, SIGNAL(configChanged()), drumEditor, SLOT(configChanged()));
      updateWindowMenu();
      }

//---------------------------------------------------------
//   startWaveEditor
//---------------------------------------------------------

void MusE::startWaveEditor()
      {
      MusECore::PartList* pl = MusEGlobal::song->getSelectedWaveParts();
      if (pl->empty()) {
            QMessageBox::critical(this, QString("MusE"), tr("Nothing to edit"));
            return;
            }
      startWaveEditor(pl);
      }

void MusE::startWaveEditor(MusECore::PartList* pl)
      {
      MusEGui::WaveEdit* waveEditor = new MusEGui::WaveEdit(pl);
      waveEditor->show();
      toplevels.push_back(waveEditor);
      connect(MusEGlobal::muse, SIGNAL(configChanged()), waveEditor, SLOT(configChanged()));
      connect(waveEditor, SIGNAL(isDeleting(MusEGui::TopWin*)), SLOT(toplevelDeleting(MusEGui::TopWin*)));
      updateWindowMenu();
      }


//---------------------------------------------------------
//   startSongInfo
//---------------------------------------------------------
void MusE::startSongInfo(bool editable)
      {
        MusEGui::SongInfoWidget info;
        info.viewCheckBox->setChecked(MusEGlobal::song->showSongInfoOnStartup());
        info.viewCheckBox->setEnabled(editable);
        info.songInfoText->setPlainText(MusEGlobal::song->getSongInfo());
        info.songInfoText->setReadOnly(!editable);
        info.setModal(true);
        info.show();
        if( info.exec() == QDialog::Accepted) {
          if (editable) {
            MusEGlobal::song->setSongInfo(info.songInfoText->toPlainText(), info.viewCheckBox->isChecked());
          }
        }

      }

//---------------------------------------------------------
//   showDidYouKnowDialog
//---------------------------------------------------------
void MusE::showDidYouKnowDialog()
      {
      if ((bool)MusEGlobal::config.showDidYouKnow == true) {
        MusEGui::DidYouKnowWidget dyk;

        QFile file(MusEGlobal::museGlobalShare + "/didyouknow.txt");
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
          printf("could not open didyouknow.txt!\n");
          return;
        }

        while (!file.atEnd())  {
          dyk.tipList.append(file.readLine());
        }

        //dyk.tipList.append(tr("To get started with MusE why don't you visit the tutorials at <br><a href=\"http://muse-sequencer.org\">http://muse-sequencer.org/index.php/Support</a>"));
        //dyk.tipList.append(tr("MusE can act as a realtime audio mixer if you connect it to jack!"));

        dyk.show();
        if( dyk.exec()) {
              if (dyk.dontShowCheckBox->isChecked()) {
                    MusEGlobal::config.showDidYouKnow=false;
                    MusEGlobal::muse->changeConfig(true);    // save settings
                    }
              }
        }
      }
//---------------------------------------------------------
//   startDefineController
//---------------------------------------------------------


//---------------------------------------------------------
//   startClipList
//---------------------------------------------------------

void MusE::startClipList(bool checked)
      {
      if (clipListEdit == 0) {
            clipListEdit = new MusEGui::ClipListEdit(this);
            toplevels.push_back(clipListEdit);
            connect(clipListEdit, SIGNAL(isDeleting(MusEGui::TopWin*)), SLOT(toplevelDeleting(MusEGui::TopWin*)));
            }
      clipListEdit->show();
      viewCliplistAction->setChecked(checked);
      updateWindowMenu();
      }

//---------------------------------------------------------
//   fileMenu
//---------------------------------------------------------

void MusE::openRecentMenu()
      {
      openRecent->clear();
      for (int i = 0; i < PROJECT_LIST_LEN; ++i) {
            if (projectList[i] == 0)
                  break;
            QByteArray ba = projectList[i]->toLatin1();
            const char* path = ba.constData();
            const char* p = strrchr(path, '/');
            if (p == 0)
                  p = path;
            else
                  ++p;
	    QAction *act = openRecent->addAction(QString(p));
	    act->setData(i);
            }
      }

//---------------------------------------------------------
//   selectProject
//---------------------------------------------------------

void MusE::selectProject(QAction* act)
      {
      if (!act)
            return;
      int id = act->data().toInt();
      if (!(id < PROJECT_LIST_LEN))
      {
        printf("THIS SHOULD NEVER HAPPEN: id(%i) < PROJECT_LIST_LEN(%i) in MusE::selectProject!\n",id, PROJECT_LIST_LEN);
        return;
      }
      QString* name = projectList[id];
      if (name == 0)
            return;
      loadProjectFile(*name, false, true);
      }

//---------------------------------------------------------
//   toplevelDeleting
//---------------------------------------------------------

void MusE::toplevelDeleting(MusEGui::TopWin* tl)
      {
      for (MusEGui::iToplevel i = toplevels.begin(); i != toplevels.end(); ++i) {
            if (*i == tl) {
                  
                  if (tl == activeTopWin)
                  {
                    activeTopWin=NULL;
                    emit activeTopWinChanged(NULL);

                    // focus the last activated topwin which is not the deleting one
                    QList<QMdiSubWindow*> l = mdiArea->subWindowList(QMdiArea::StackingOrder);
                    for (QList<QMdiSubWindow*>::iterator lit=l.begin(); lit!=l.end(); lit++)
                      if ((*lit)->isVisible() && (*lit)->widget() != tl)
                      {
                        if (MusEGlobal::debugMsg)
                          printf("bringing '%s' to front instead of closed window\n",(*lit)->widget()->windowTitle().toAscii().data());

                        bringToFront((*lit)->widget());

                        break; 
                      }
                  }
              
                  if (tl == currentMenuSharingTopwin)
                    setCurrentMenuSharingTopwin(NULL);
              
              
                  bool mustUpdateScoreMenus=false;
                  switch(tl->type()) {
                        case MusEGui::TopWin::MARKER:
                        case MusEGui::TopWin::ARRANGER:
                              break;
                        case MusEGui::TopWin::CLIPLIST:
                              viewCliplistAction->setChecked(false); 
                              if (currentMenuSharingTopwin == clipListEdit)
                                setCurrentMenuSharingTopwin(NULL);
                              updateWindowMenu(); 
                              return;

                        // the following editors can exist in more than
                        // one instantiation:
                        case MusEGui::TopWin::PIANO_ROLL:
                        case MusEGui::TopWin::LISTE:
                        case MusEGui::TopWin::DRUM:
                        case MusEGui::TopWin::MASTER:
                        case MusEGui::TopWin::WAVE:
                        case MusEGui::TopWin::LMASTER:
                              break;
                        case MusEGui::TopWin::SCORE:
                              mustUpdateScoreMenus=true;
                        
                        case MusEGui::TopWin::TOPLEVELTYPE_LAST_ENTRY: //to avoid a warning
                          break;
                        }
                  toplevels.erase(i);   
                  if (mustUpdateScoreMenus)
                        arrangerView->updateScoreMenus();
                  updateWindowMenu();
                  return;
                  }
            }
      printf("topLevelDeleting: top level %p not found\n", tl);
      }

//---------------------------------------------------------
//   kbAccel
//---------------------------------------------------------

void MusE::kbAccel(int key)
      {
      if (key == MusEGui::shortcuts[MusEGui::SHRT_TOGGLE_METRO].key) {
            MusEGlobal::song->setClick(!MusEGlobal::song->click());
            }
      else if (key == MusEGui::shortcuts[MusEGui::SHRT_PLAY_TOGGLE].key) {
            if (MusEGlobal::audio->isPlaying())
                  MusEGlobal::song->setStop(true);
            else if (!MusEGlobal::config.useOldStyleStopShortCut)
                  MusEGlobal::song->setPlay(true);
            else if (MusEGlobal::song->cpos() != MusEGlobal::song->lpos())
                  MusEGlobal::song->setPos(0, MusEGlobal::song->lPos());
            else {
                  MusECore::Pos p(0, true);
                  MusEGlobal::song->setPos(0, p);
                  }
            }
      else if (key == MusEGui::shortcuts[MusEGui::SHRT_STOP].key) {
            MusEGlobal::song->setStop(true);
            }
      else if (key == MusEGui::shortcuts[MusEGui::SHRT_GOTO_START].key) {
            MusECore::Pos p(0, true);
            MusEGlobal::song->setPos(0, p);
            }
      else if (key == MusEGui::shortcuts[MusEGui::SHRT_PLAY_SONG].key ) {
            MusEGlobal::song->setPlay(true);
            }
      
      // Normally each editor window handles these, to inc by the editor's raster snap value.
      // But users were asking for a global version - "they don't work when I'm in mixer or transport".
      // Since no editor claimed the key event, we don't know a specific editor's snap setting,
      //  so adopt a policy where the arranger is the 'main' raster reference, I guess...
      else if (key == MusEGui::shortcuts[MusEGui::SHRT_POS_DEC].key) {
            int spos = MusEGlobal::song->cpos();
            if(spos > 0) 
            {
              spos -= 1;     // Nudge by -1, then snap down with raster1.
              spos = AL::sigmap.raster1(spos, MusEGlobal::song->arrangerRaster());
            }  
            if(spos < 0)
              spos = 0;
            MusECore::Pos p(spos,true);
            MusEGlobal::song->setPos(0, p, true, true, true);
            return;
            }
      else if (key == MusEGui::shortcuts[MusEGui::SHRT_POS_INC].key) {
            int spos = AL::sigmap.raster2(MusEGlobal::song->cpos() + 1, MusEGlobal::song->arrangerRaster());    // Nudge by +1, then snap up with raster2.
            MusECore::Pos p(spos,true);
            MusEGlobal::song->setPos(0, p, true, true, true); //CDW
            return;
            }
      else if (key == MusEGui::shortcuts[MusEGui::SHRT_POS_DEC_NOSNAP].key) {
            int spos = MusEGlobal::song->cpos() - AL::sigmap.rasterStep(MusEGlobal::song->cpos(), MusEGlobal::song->arrangerRaster());
            if(spos < 0)
              spos = 0;
            MusECore::Pos p(spos,true);
            MusEGlobal::song->setPos(0, p, true, true, true);
            return;
            }
      else if (key == MusEGui::shortcuts[MusEGui::SHRT_POS_INC_NOSNAP].key) {
            MusECore::Pos p(MusEGlobal::song->cpos() + AL::sigmap.rasterStep(MusEGlobal::song->cpos(), MusEGlobal::song->arrangerRaster()), true);
            MusEGlobal::song->setPos(0, p, true, true, true);
            return;
            }
            
      else if (key == MusEGui::shortcuts[MusEGui::SHRT_GOTO_LEFT].key) {
            if (!MusEGlobal::song->record())
                  MusEGlobal::song->setPos(0, MusEGlobal::song->lPos());
            }
      else if (key == MusEGui::shortcuts[MusEGui::SHRT_GOTO_RIGHT].key) {
            if (!MusEGlobal::song->record())
                  MusEGlobal::song->setPos(0, MusEGlobal::song->rPos());
            }
      else if (key == MusEGui::shortcuts[MusEGui::SHRT_TOGGLE_LOOP].key) {
            MusEGlobal::song->setLoop(!MusEGlobal::song->loop());
            }
      else if (key == MusEGui::shortcuts[MusEGui::SHRT_START_REC].key) {
            if (!MusEGlobal::audio->isPlaying()) {
                  MusEGlobal::song->setRecord(!MusEGlobal::song->record());
                  }
            }
      else if (key == MusEGui::shortcuts[MusEGui::SHRT_REC_CLEAR].key) {
            if (!MusEGlobal::audio->isPlaying()) {
                  MusEGlobal::song->clearTrackRec();
                  }
            }
      else if (key == MusEGui::shortcuts[MusEGui::SHRT_OPEN_TRANSPORT].key) {
            toggleTransport(!viewTransportAction->isChecked());
            }
      else if (key == MusEGui::shortcuts[MusEGui::SHRT_OPEN_BIGTIME].key) {
            toggleBigTime(!viewBigtimeAction->isChecked());
            }
      else if (key == MusEGui::shortcuts[MusEGui::SHRT_OPEN_MIXER].key) {
            toggleMixer1(!viewMixerAAction->isChecked());
            }
      else if (key == MusEGui::shortcuts[MusEGui::SHRT_OPEN_MIXER2].key) {
            toggleMixer2(!viewMixerBAction->isChecked());
            }
      else if (key == MusEGui::shortcuts[MusEGui::SHRT_NEXT_MARKER].key) {
            if (markerView)
              markerView->nextMarker();
            }
      else if (key == MusEGui::shortcuts[MusEGui::SHRT_PREV_MARKER].key) {
            if (markerView)
              markerView->prevMarker();
            }
      else {
            if (MusEGlobal::debugMsg)
                  printf("unknown kbAccel 0x%x\n", key);
            }
      }

#if 0 
//---------------------------------------------------------
//   catchSignal
//    only for debugging
//---------------------------------------------------------

// if enabling this code, also enable the line containing
// "catchSignal" in main.cpp
static void catchSignal(int sig)
      {
      if (MusEGlobal::debugMsg)
            fprintf(stderr, "MusE: signal %d catched\n", sig);
      if (sig == SIGSEGV) {
            fprintf(stderr, "MusE: segmentation fault\n");
            abort();
            }
      if (sig == SIGCHLD) {
            M_DEBUG("caught SIGCHLD - child died\n");
            int status;
            int n = waitpid (-1, &status, WNOHANG);
            if (n > 0) {
                  fprintf(stderr, "SIGCHLD for unknown process %d received\n", n);
                  }
            }
      }
#endif

//---------------------------------------------------------
//   cmd
//    some cmd's from pulldown menu
//---------------------------------------------------------

void MusE::cmd(int cmd)
      {
      switch(cmd) {
            case CMD_FOLLOW_NO:
                  MusEGlobal::song->setFollow(MusECore::Song::NO);
                  setFollow();
                  break;
            case CMD_FOLLOW_JUMP:
                  MusEGlobal::song->setFollow(MusECore::Song::JUMP);
                  setFollow();
                  break;
            case CMD_FOLLOW_CONTINUOUS:
                  MusEGlobal::song->setFollow(MusECore::Song::CONTINUOUS);
                  setFollow();
                  break;
            }
      }





//---------------------------------------------------------
//   configAppearance
//---------------------------------------------------------

void MusE::configAppearance()
      {
      if (!appearance)
            appearance = new MusEGui::Appearance(_arranger);
      appearance->resetValues();
      if(appearance->isVisible()) {
          appearance->raise();
          appearance->activateWindow();
          }
      else
          appearance->show();
      }

//---------------------------------------------------------
//   loadTheme
//---------------------------------------------------------

void MusE::loadTheme(const QString& s)
      {
      QStringList sl = QStyleFactory::keys();
      if (s.isEmpty() || sl.indexOf(s) == -1) {
        if(MusEGlobal::debugMsg)
          printf("Set style does not exist, setting default.\n");
        qApp->setStyle(Appearance::defaultStyle);
        qApp->style()->setObjectName(Appearance::defaultStyle);
      }
      else if (qApp->style()->objectName() != s)
      {
            qApp->setStyle(s);
            qApp->style()->setObjectName(s);   
      }      
      }

//---------------------------------------------------------
//   loadStyleSheetFile
//---------------------------------------------------------

void MusE::loadStyleSheetFile(const QString& s)
{
    if(s.isEmpty())
    {
      qApp->setStyleSheet(s);
      return;
    }
      
    QFile cf(s);
    if (cf.open(QIODevice::ReadOnly)) {
          QByteArray ss = cf.readAll();
          QString sheet(QString::fromUtf8(ss.data()));
          qApp->setStyleSheet(sheet);
          cf.close();
          }
    else
          printf("loading style sheet <%s> failed\n", qPrintable(s));
}

//---------------------------------------------------------
//   configChanged
//    - called whenever configuration has changed
//    - when configuration has changed by user, call with
//      writeFlag=true to save configuration in ~/.MusE
//---------------------------------------------------------

void MusE::changeConfig(bool writeFlag)
      {
      if (writeFlag)
            writeGlobalConfiguration();
      
      loadTheme(MusEGlobal::config.style);
      QApplication::setFont(MusEGlobal::config.fonts[0]);
      if(!MusEGlobal::config.styleSheetFile.isEmpty())
        loadStyleSheetFile(MusEGlobal::config.styleSheetFile);
      
      emit configChanged();
      updateConfiguration();
      }

//---------------------------------------------------------
//   configMetronome
//---------------------------------------------------------

void MusE::configMetronome()
      {
      if (!metronomeConfig)
          metronomeConfig = new MusEGui::MetronomeConfig;

      if(metronomeConfig->isVisible()) {
          metronomeConfig->raise();
          metronomeConfig->activateWindow();
          }
      else
          metronomeConfig->show();
      }


//---------------------------------------------------------
//   configShortCuts
//---------------------------------------------------------

void MusE::configShortCuts()
      {
      if (!shortcutConfig)
            shortcutConfig = new MusEGui::ShortcutConfig(this);
      shortcutConfig->_config_changed = false;
      if (shortcutConfig->exec())
            changeConfig(true);
      }


//---------------------------------------------------------
//   bounceToTrack
//---------------------------------------------------------

void MusE::bounceToTrack()
      {
      if(MusEGlobal::audio->bounce())
        return;
      
      MusEGlobal::song->bounceOutput = 0;
      
      if(MusEGlobal::song->waves()->empty())
      {
        QMessageBox::critical(this,
            tr("MusE: Bounce to Track"),
            tr("No wave tracks found")
            );
        return;
      }
      
      MusECore::OutputList* ol = MusEGlobal::song->outputs();
      if(ol->empty())
      {
        QMessageBox::critical(this,
            tr("MusE: Bounce to Track"),
            tr("No audio output tracks found")
            );
        return;
      }
      
      if(checkRegionNotNull())
        return;
      
      MusECore::AudioOutput* out = 0;
      // If only one output, pick it, else pick the first selected.
      if(ol->size() == 1)
        out = ol->front();
      else
      {
        for(MusECore::iAudioOutput iao = ol->begin(); iao != ol->end(); ++iao) 
        {
	  MusECore::AudioOutput* o = *iao;
          if(o->selected()) 
          {
            if(out) 
            {
              out = 0;
              break;
            }
            out = o;
          }
        }
        if(!out) 
        {
          QMessageBox::critical(this,
              tr("MusE: Bounce to Track"),
              tr("Select one audio output track,\nand one target wave track")
              );
          return;
        }
      }
      
      // search target track
      MusECore::TrackList* tl = MusEGlobal::song->tracks();
      MusECore::WaveTrack* track = 0;
      
      for (MusECore::iTrack it = tl->begin(); it != tl->end(); ++it) {
            MusECore::Track* t = *it;
            if (t->selected()) {
                    if(t->type() != MusECore::Track::WAVE && t->type() != MusECore::Track::AUDIO_OUTPUT) {
                        track = 0;
                        break;
                    }
                    if(t->type() == MusECore::Track::WAVE)
                    { 
                      if(track)
                      {
                        track = 0;
                        break;
                      }
                      track = (MusECore::WaveTrack*)t;
                    }  
                    
                  }  
            }
            
      if (track == 0) {
          if(ol->size() == 1) {
            QMessageBox::critical(this,
               tr("MusE: Bounce to Track"),
               tr("Select one target wave track")
               );
            return;
          }
          else 
          {
            QMessageBox::critical(this,
               tr("MusE: Bounce to Track"),
               tr("Select one target wave track,\nand one audio output track")
               );
            return;
          }  
      }

      MusEGlobal::song->setPos(0,MusEGlobal::song->lPos(),0,true,true);
      MusEGlobal::song->bounceOutput = out;
      MusEGlobal::song->bounceTrack = track;
      MusEGlobal::song->setRecord(true);
      MusEGlobal::song->setRecordFlag(track, true);
      track->prepareRecording();
      MusEGlobal::audio->msgBounce();
      MusEGlobal::song->setPlay(true);
      }

//---------------------------------------------------------
//   bounceToFile
//---------------------------------------------------------

void MusE::bounceToFile(MusECore::AudioOutput* ao)
      {
      if(MusEGlobal::audio->bounce())
        return;
      MusEGlobal::song->bounceOutput = 0;
      if(!ao)
      {
	MusECore::OutputList* ol = MusEGlobal::song->outputs();
        if(ol->empty())
        {
          QMessageBox::critical(this,
              tr("MusE: Bounce to File"),
              tr("No audio output tracks found")
              );
          return;
        }
        // If only one output, pick it, else pick the first selected.
        if(ol->size() == 1)
          ao = ol->front();
        else
        {
          for(MusECore::iAudioOutput iao = ol->begin(); iao != ol->end(); ++iao) 
          {
	    MusECore::AudioOutput* o = *iao;
            if(o->selected()) 
            {
              if(ao) 
              {
               ao = 0;
               break;
              }
              ao = o;
            }
          }
          if (ao == 0) {
                QMessageBox::critical(this,
                  tr("MusE: Bounce to File"),
                  tr("Select one audio output track")
                  );
                return;
          }
        }
      }
      
      if (checkRegionNotNull())
            return;
      
      MusECore::SndFile* sf = MusECore::getSndFile(0, this);
      if (sf == 0)
            return;
            
      MusEGlobal::song->setPos(0,MusEGlobal::song->lPos(),0,true,true);
      MusEGlobal::song->bounceOutput = ao;
      ao->setRecFile(sf);
      if(MusEGlobal::debugMsg)
        printf("ao->setRecFile %p\n", sf);
      MusEGlobal::song->setRecord(true, false);
      MusEGlobal::song->setRecordFlag(ao, true);
      ao->prepareRecording();
      MusEGlobal::audio->msgBounce();
      MusEGlobal::song->setPlay(true);
      }


//---------------------------------------------------------
//   checkRegionNotNull
//    return true if (rPos - lPos) <= 0
//---------------------------------------------------------

bool MusE::checkRegionNotNull()
      {
      int start = MusEGlobal::song->lPos().frame();
      int end   = MusEGlobal::song->rPos().frame();
      if (end - start <= 0) {
            QMessageBox::critical(this,
               tr("MusE: Bounce"),
               tr("set left/right marker for bounce range")
               );
            return true;
            }
      return false;
      }


#ifdef HAVE_LASH
//---------------------------------------------------------
//   lash_idle_cb
//---------------------------------------------------------

void
MusE::lash_idle_cb ()
{
  lash_event_t * event;
  if (!lash_client)
    return;

  while ( (event = lash_get_event (lash_client)) )
  {
      switch (lash_event_get_type (event))
      {
        case LASH_Save_File:
        {
          /* save file */
          QString ss = QString(lash_event_get_string(event)) + QString("/lash-project-muse.med");
          int ok = save (ss.toAscii(), false, true);
          if (ok) {
            project.setFile(ss.toAscii());
            setWindowTitle(tr("MusE: Song: %1").arg(MusEGui::projectTitleFromFilename(project.absoluteFilePath())));
            addProject(ss.toAscii());
            MusEGlobal::museProject = QFileInfo(ss.toAscii()).absolutePath();
          }
          lash_send_event (lash_client, event);
        }
        break;

        case LASH_Restore_File:
        {
          /* load file */
          QString sr = QString(lash_event_get_string(event)) + QString("/lash-project-muse.med");
          loadProjectFile(sr.toAscii(), false, true);
          lash_send_event (lash_client, event);
        }
        break;

        case LASH_Quit:
        {
          /* quit muse */
          std::cout << "MusE::lash_idle_cb Received LASH_Quit"
                    << std::endl;
          lash_event_destroy (event);
        }
        break;

        default:
        {
          std::cout << "MusE::lash_idle_cb Received unknown LASH event of type "
                    << lash_event_get_type (event)
                    << std::endl;
          lash_event_destroy (event);
        }
        break;
      }
  }
}
#endif /* HAVE_LASH */

//---------------------------------------------------------
//   clearSong
//    return true if operation aborted
//    called with sequencer stopped
//    If clear_all is false, it will not touch things like midi ports.
//---------------------------------------------------------

bool MusE::clearSong(bool clear_all)
      {
      if (MusEGlobal::song->dirty) {
            int n = 0;
            n = QMessageBox::warning(this, appName,
               tr("The current Project contains unsaved data\n"
               "Load overwrites current Project:\n"
               "Save Current Project?"),
               tr("&Save"), tr("S&kip"), tr("&Abort"), 0, 2);
            switch (n) {
                  case 0:
                        if (!save())      // abort if save failed
                              return true;
                        break;
                  case 1:
                        break;
                  case 2:
                        return true;
                  default:
                        printf("InternalError: gibt %d\n", n);
                  }
            }
      if (MusEGlobal::audio->isPlaying()) {
            MusEGlobal::audio->msgPlay(false);
            while (MusEGlobal::audio->isPlaying())
                  qApp->processEvents();
            }
      microSleep(100000);

again:
      for (MusEGui::iToplevel i = toplevels.begin(); i != toplevels.end(); ++i) {
            MusEGui::TopWin* tl = *i;
            switch (tl->type()) {
                  case MusEGui::TopWin::CLIPLIST:
                  case MusEGui::TopWin::MARKER:
                  case MusEGui::TopWin::ARRANGER:
                        break;
                  case MusEGui::TopWin::PIANO_ROLL:
                  case MusEGui::TopWin::SCORE:
                  case MusEGui::TopWin::LISTE:
                  case MusEGui::TopWin::DRUM:
                  case MusEGui::TopWin::MASTER:
                  case MusEGui::TopWin::WAVE:
                  case MusEGui::TopWin::LMASTER:
                  {  
                        if(tl->isVisible())   // Don't keep trying to close, only if visible.
                        {  
                          if(!tl->close())
                            printf("MusE::clearSong TopWin did not close!\n");  
                          goto again;
                        }  
                  }
                  case MusEGui::TopWin::TOPLEVELTYPE_LAST_ENTRY: //to avoid a warning
                    break;
                  }
            }
      microSleep(100000);  
      _arranger->songIsClearing();
      MusEGlobal::song->clear(true, clear_all);
      microSleep(100000);  
      return false;
      }

//---------------------------------------------------------
//   startEditInstrument
//---------------------------------------------------------

void MusE::startEditInstrument()
    {
      if(editInstrument == 0)
      {
            editInstrument = new MusEGui::EditInstrument(this);
            editInstrument->show();
      }
      else
      {
        if(! editInstrument->isHidden())
          editInstrument->hide();
        else      
          editInstrument->show();
      }
      
    }

//---------------------------------------------------------
//   switchMixerAutomation
//---------------------------------------------------------

void MusE::switchMixerAutomation()
      {
      // Could be intensive, try idling instead of a single message.  
      MusEGlobal::audio->msgIdle(true);
      
      MusEGlobal::automation = ! MusEGlobal::automation;
      // Clear all pressed and touched and rec event lists.
      MusEGlobal::song->clearRecAutomation(true);
      
      // If going to OFF mode, need to update current 'manual' values from the automation values at this time...   
      if(!MusEGlobal::automation)
      {
        MusECore::TrackList* tracks = MusEGlobal::song->tracks();
        for (MusECore::iTrack i = tracks->begin(); i != tracks->end(); ++i) {
              if ((*i)->isMidiTrack())
                    continue;
              MusECore::AudioTrack* track = static_cast<MusECore::AudioTrack*>(*i);
              if(track->automationType() != AUTO_OFF) // && track->automationType() != AUTO_WRITE)
                track->controller()->updateCurValues(MusEGlobal::audio->curFramePos());
              }
      }
        
      MusEGlobal::audio->msgIdle(false);
      
      autoMixerAction->setChecked(MusEGlobal::automation);
      }

//---------------------------------------------------------
//   clearAutomation
//---------------------------------------------------------

void MusE::clearAutomation()
      {
      QMessageBox::StandardButton b = QMessageBox::warning(this, appName,
          tr("This will clear all automation data on\n all audio tracks!\nProceed?"),
          QMessageBox::Ok | QMessageBox::Cancel, QMessageBox::Cancel);
      
      if(b != QMessageBox::Ok)
        return;
        
      // Could be intensive, try idling instead of a single message.  
      MusEGlobal::audio->msgIdle(true);
      
      MusECore::TrackList* tracks = MusEGlobal::song->tracks();
      for (MusECore::iTrack i = tracks->begin(); i != tracks->end(); ++i) {
            if ((*i)->isMidiTrack())
                  continue;
            static_cast<MusECore::AudioTrack*>(*i)->controller()->clearAllAutomation();
            }
            
      MusEGlobal::audio->msgIdle(false);
      }

//---------------------------------------------------------
//   takeAutomationSnapshot
//---------------------------------------------------------

void MusE::takeAutomationSnapshot()
      {
      QMessageBox::StandardButton b = QMessageBox::warning(this, appName,
          tr("This takes an automation snapshot of\n all controllers on all audio tracks,\n"
             " at the current position.\nProceed?"),
          QMessageBox::Ok | QMessageBox::Cancel, QMessageBox::Cancel);
      
      if(b != QMessageBox::Ok)
        return;
        
      // Could be intensive, try idling instead of a single message.  
      MusEGlobal::audio->msgIdle(true);
      
      int frame = MusEGlobal::audio->curFramePos();
      MusECore::TrackList* tracks = MusEGlobal::song->tracks();
      for (MusECore::iTrack i = tracks->begin(); i != tracks->end(); ++i) {
            if ((*i)->isMidiTrack())
                  continue;
	    MusECore::AudioTrack* track = static_cast<MusECore::AudioTrack*>(*i);
            MusECore::CtrlListList* cll = track->controller();
            // Need to update current 'manual' values from the automation values at this time.   
            if(track->automationType() != AUTO_OFF) // && track->automationType() != AUTO_WRITE)
              cll->updateCurValues(frame);
            
            for (MusECore::iCtrlList icl = cll->begin(); icl != cll->end(); ++icl) {
                  double val = icl->second->curVal();
                  icl->second->add(frame, val);
                  }
            }
            
      MusEGlobal::audio->msgIdle(false);
      }

//---------------------------------------------------------
//   updateConfiguration
//    called whenever the configuration has changed
//---------------------------------------------------------

void MusE::updateConfiguration()
      {
      fileOpenAction->setShortcut(MusEGui::shortcuts[MusEGui::SHRT_OPEN].key);
      fileNewAction->setShortcut(MusEGui::shortcuts[MusEGui::SHRT_NEW].key);
      fileSaveAction->setShortcut(MusEGui::shortcuts[MusEGui::SHRT_SAVE].key);
      fileSaveAsAction->setShortcut(MusEGui::shortcuts[MusEGui::SHRT_SAVE_AS].key);

      //menu_file->setShortcut(MusEGui::shortcuts[MusEGui::SHRT_OPEN_RECENT].key, menu_ids[CMD_OPEN_RECENT]);    // Not used.
      fileImportMidiAction->setShortcut(MusEGui::shortcuts[MusEGui::SHRT_IMPORT_MIDI].key);
      fileExportMidiAction->setShortcut(MusEGui::shortcuts[MusEGui::SHRT_EXPORT_MIDI].key);
      fileImportPartAction->setShortcut(MusEGui::shortcuts[MusEGui::SHRT_IMPORT_PART].key);
      fileImportWaveAction->setShortcut(MusEGui::shortcuts[MusEGui::SHRT_IMPORT_AUDIO].key);
      quitAction->setShortcut(MusEGui::shortcuts[MusEGui::SHRT_QUIT].key);
      
      //menu_file->setShortcut(MusEGui::shortcuts[MusEGui::SHRT_LOAD_TEMPLATE].key, menu_ids[CMD_LOAD_TEMPLATE]);  // Not used.

      MusEGlobal::undoAction->setShortcut(MusEGui::shortcuts[MusEGui::SHRT_UNDO].key);  
      MusEGlobal::redoAction->setShortcut(MusEGui::shortcuts[MusEGui::SHRT_REDO].key);


      //editSongInfoAction has no acceleration

      viewTransportAction->setShortcut(MusEGui::shortcuts[MusEGui::SHRT_OPEN_TRANSPORT].key);
      viewBigtimeAction->setShortcut(MusEGui::shortcuts[MusEGui::SHRT_OPEN_BIGTIME].key);
      viewMixerAAction->setShortcut(MusEGui::shortcuts[MusEGui::SHRT_OPEN_MIXER].key);
      viewMixerBAction->setShortcut(MusEGui::shortcuts[MusEGui::SHRT_OPEN_MIXER2].key);
      //viewCliplistAction has no acceleration
      viewMarkerAction->setShortcut(MusEGui::shortcuts[MusEGui::SHRT_OPEN_MARKER].key);

      
      // midiEditInstAction does not have acceleration
      midiResetInstAction->setShortcut(MusEGui::shortcuts[MusEGui::SHRT_MIDI_RESET].key);
      midiInitInstActions->setShortcut(MusEGui::shortcuts[MusEGui::SHRT_MIDI_INIT].key);
      midiLocalOffAction->setShortcut(MusEGui::shortcuts[MusEGui::SHRT_MIDI_LOCAL_OFF].key);
      midiTrpAction->setShortcut(MusEGui::shortcuts[MusEGui::SHRT_MIDI_INPUT_TRANSPOSE].key);
      midiInputTrfAction->setShortcut(MusEGui::shortcuts[MusEGui::SHRT_MIDI_INPUT_TRANSFORM].key);
      midiInputFilterAction->setShortcut(MusEGui::shortcuts[MusEGui::SHRT_MIDI_INPUT_FILTER].key);
      midiRemoteAction->setShortcut(MusEGui::shortcuts[MusEGui::SHRT_MIDI_REMOTE_CONTROL].key);
#ifdef BUILD_EXPERIMENTAL
      midiRhythmAction->setShortcut(MusEGui::shortcuts[MusEGui::SHRT_RANDOM_RHYTHM_GENERATOR].key);
#endif

      audioBounce2TrackAction->setShortcut(MusEGui::shortcuts[MusEGui::SHRT_AUDIO_BOUNCE_TO_TRACK].key);
      audioBounce2FileAction->setShortcut(MusEGui::shortcuts[MusEGui::SHRT_AUDIO_BOUNCE_TO_FILE].key);
      audioRestartAction->setShortcut(MusEGui::shortcuts[MusEGui::SHRT_AUDIO_RESTART].key);

      autoMixerAction->setShortcut(MusEGui::shortcuts[MusEGui::SHRT_MIXER_AUTOMATION].key);
      autoSnapshotAction->setShortcut(MusEGui::shortcuts[MusEGui::SHRT_MIXER_SNAPSHOT].key);
      autoClearAction->setShortcut(MusEGui::shortcuts[MusEGui::SHRT_MIXER_AUTOMATION_CLEAR].key);

      settingsGlobalAction->setShortcut(MusEGui::shortcuts[MusEGui::SHRT_GLOBAL_CONFIG].key);
      settingsShortcutsAction->setShortcut(MusEGui::shortcuts[MusEGui::SHRT_CONFIG_SHORTCUTS].key);
      settingsMetronomeAction->setShortcut(MusEGui::shortcuts[MusEGui::SHRT_CONFIG_METRONOME].key);
      settingsMidiSyncAction->setShortcut(MusEGui::shortcuts[MusEGui::SHRT_CONFIG_MIDISYNC].key);
      // settingsMidiIOAction does not have acceleration
      settingsAppearanceAction->setShortcut(MusEGui::shortcuts[MusEGui::SHRT_APPEARANCE_SETTINGS].key);
      settingsMidiPortAction->setShortcut(MusEGui::shortcuts[MusEGui::SHRT_CONFIG_MIDI_PORTS].key);


      dontFollowAction->setShortcut(MusEGui::shortcuts[MusEGui::SHRT_FOLLOW_NO].key);
      followPageAction->setShortcut(MusEGui::shortcuts[MusEGui::SHRT_FOLLOW_JUMP].key);
      followCtsAction->setShortcut(MusEGui::shortcuts[MusEGui::SHRT_FOLLOW_CONTINUOUS].key);
      
      helpManualAction->setShortcut(MusEGui::shortcuts[MusEGui::SHRT_OPEN_HELP].key);
      fullscreenAction->setShortcut(MusEGui::shortcuts[MusEGui::SHRT_FULLSCREEN].key);
      
      //arrangerView->updateMusEGui::Shortcuts(); //commented out by flo: is done via signal
      }

//---------------------------------------------------------
//   showBigtime
//---------------------------------------------------------

void MusE::showBigtime(bool on)
      {
      if (on && bigtime == 0) {
            bigtime = new MusEGui::BigTime(this);
            bigtime->setPos(0, MusEGlobal::song->cpos(), false);
            connect(MusEGlobal::song, SIGNAL(posChanged(int, unsigned, bool)), bigtime, SLOT(setPos(int, unsigned, bool)));
            connect(MusEGlobal::muse, SIGNAL(configChanged()), bigtime, SLOT(configChanged()));
            connect(bigtime, SIGNAL(closed()), SLOT(bigtimeClosed()));
            bigtime->resize(MusEGlobal::config.geometryBigTime.size());
            bigtime->move(MusEGlobal::config.geometryBigTime.topLeft());
            }
      if (bigtime)
            bigtime->setVisible(on);
      viewBigtimeAction->setChecked(on);
      }

//---------------------------------------------------------
//   toggleBigTime
//---------------------------------------------------------

void MusE::toggleBigTime(bool checked)
      {
      showBigtime(checked);
      }

//---------------------------------------------------------
//   bigtimeClosed
//---------------------------------------------------------

void MusE::bigtimeClosed()
      {
      viewBigtimeAction->setChecked(false);
      }


//---------------------------------------------------------
//   showMixer1
//---------------------------------------------------------

void MusE::showMixer1(bool on)
      {
      if (on && mixer1 == 0) {
            mixer1 = new MusEGui::AudioMixerApp(this, &(MusEGlobal::config.mixer1));
            connect(mixer1, SIGNAL(closed()), SLOT(mixer1Closed()));
            mixer1->resize(MusEGlobal::config.mixer1.geometry.size());
            mixer1->move(MusEGlobal::config.mixer1.geometry.topLeft());
            }
      if (mixer1)
            mixer1->setVisible(on);
      viewMixerAAction->setChecked(on);
      }

//---------------------------------------------------------
//   showMixer2
//---------------------------------------------------------

void MusE::showMixer2(bool on)
      {
      if (on && mixer2 == 0) {
            mixer2 = new MusEGui::AudioMixerApp(this, &(MusEGlobal::config.mixer2));
            connect(mixer2, SIGNAL(closed()), SLOT(mixer2Closed()));
            mixer2->resize(MusEGlobal::config.mixer2.geometry.size());
            mixer2->move(MusEGlobal::config.mixer2.geometry.topLeft());
            }
      if (mixer2)
            mixer2->setVisible(on);
      viewMixerBAction->setChecked(on);
      }

//---------------------------------------------------------
//   toggleMixer1
//---------------------------------------------------------

void MusE::toggleMixer1(bool checked)
      {
      showMixer1(checked);
      }

//---------------------------------------------------------
//   toggleMixer2
//---------------------------------------------------------

void MusE::toggleMixer2(bool checked)
      {
      showMixer2(checked);
      }

//---------------------------------------------------------
//   mixer1Closed
//---------------------------------------------------------

void MusE::mixer1Closed()
      {
      viewMixerAAction->setChecked(false);
      }

//---------------------------------------------------------
//   mixer2Closed
//---------------------------------------------------------

void MusE::mixer2Closed()
      {
      viewMixerBAction->setChecked(false);
      }


QWidget* MusE::mixer1Window()     { return mixer1; }
QWidget* MusE::mixer2Window()     { return mixer2; }

QWidget* MusE::transportWindow() { return transport; }
QWidget* MusE::bigtimeWindow()   { return bigtime; }

//---------------------------------------------------------
//   execDeliveredScript
//---------------------------------------------------------
void MusE::execDeliveredScript(int id)
{
      MusEGlobal::song->executeScript(MusEGlobal::song->getScriptPath(id, true).toLatin1().constData(), MusEGlobal::song->getSelectedMidiParts(), 0, false); // TODO: get quant from arranger
}

//---------------------------------------------------------
//   execUserScript
//---------------------------------------------------------
void MusE::execUserScript(int id)
{
      MusEGlobal::song->executeScript(MusEGlobal::song->getScriptPath(id, false).toLatin1().constData(), MusEGlobal::song->getSelectedMidiParts(), 0, false); // TODO: get quant from arranger
}

//---------------------------------------------------------
//   findUnusedWaveFiles
//---------------------------------------------------------
void MusE::findUnusedWaveFiles()
{
    MusEGui::UnusedWaveFiles unused(MusEGlobal::muse);
    unused.exec();
}

void MusE::focusChanged(QWidget* old, QWidget* now)  
{
  if(MusEGlobal::heavyDebugMsg)
  {
    printf("\n");   
    printf("focusChanged: old:%p now:%p activeWindow:%p\n", old, now, qApp->activeWindow());  
    if(old)                                                   
      printf(" old type: %s\n", typeid(*old).name());  
    if(now)                                                  
      printf(" now type: %s\n", typeid(*now).name());  
    if (dynamic_cast<QMdiSubWindow*>(now)!=0)
    {
      QWidget* tmp=dynamic_cast<QMdiSubWindow*>(now)->widget();
      if (tmp)
        printf("  subwin contains %p which is a %s\n", tmp, typeid(*tmp).name());
      else
        printf("  subwin contains NULL\n");
    }
    if(qApp->activeWindow())                                                   
      printf(" activeWindow type: %s\n", typeid(*qApp->activeWindow()).name());  
    printf("\n");   
  }
  
  // NOTE: FYI: This is what is required if, for 'Smart Focus', we try simply calling clearFocus from each relevant control
  //    upon Return/Enter/Escape or whatever. 
  // It's nice to be able to do that, but this was crash-prone and I don't like it. Instead each relevant control connects
  //  signals to slots in the editors which set focus on the canvases AND activate their top windows. 
  // Who knows, this code might be needed in some way. Informational, please keep.  Tim.
  //
  /*
  // Allow focus proxy to do its job (if set).
  if(now == this)
  {
    if(mdiArea)
    {
      QMdiSubWindow* mw = mdiArea->activeSubWindow();
      if(mw && mw->widget()->focusProxy())  // Did we set a focus proxy on the window?
        //mw->widget()->setFocus(); // Give focus to the window so proxy gets it.
        mw->widget()->focusProxy()->setFocus(); // Give focus directly to the proxy.
    }  
  }
  else
  if(!now)
  {
    QWidget* aw = qApp->activeWindow();  
    if(aw)
    {
      if(aw == this) // Active top-level window is MusE?
      {
        if(mdiArea)
        {
          QMdiSubWindow* mw = mdiArea->activeSubWindow();
          if(mw && mw->widget()->focusProxy())  // Did we set a focus proxy on the window?
            //mw->widget()->setFocus(); // Give focus to the window so proxy gets it.
            mw->widget()->focusProxy()->setFocus(); // Give focus directly to the proxy.
        }  
      }
      else   // Active top-level window is outside the MusE mdi window.
      {
        if(aw->focusProxy())  // Did we set a focus proxy on the window?
        {
          //aw->setFocus(); // Give focus to the window so proxy gets it.   
          aw->focusProxy()->setFocus(); // Give focus directly to the proxy.
          if(!aw->focusProxy()->isActiveWindow())
            aw->focusProxy()->activateWindow();
        }
      }
    }  
  }
  */
  
  QWidget* ptr=now;

  if (activeTopWin)
    activeTopWin->storeInitialState();
  
  if (currentMenuSharingTopwin && (currentMenuSharingTopwin!=activeTopWin))
    currentMenuSharingTopwin->storeInitialState();

  // if the activated widget is a QMdiSubWindow containing some TopWin
  if ( (dynamic_cast<QMdiSubWindow*>(ptr)!=0) &&
       (dynamic_cast<MusEGui::TopWin*>( ((QMdiSubWindow*)ptr)->widget() )!=0) )
  {
    MusEGui::TopWin* tmp = (MusEGui::TopWin*) ((QMdiSubWindow*)ptr)->widget();
    if (tmp->initalizing())
    {
      waitingForTopwin=tmp;
      return;
    }
    else
    {
      ptr=tmp;
      // go on.
    }
  }

  while (ptr)
  {
    if (MusEGlobal::heavyDebugMsg)  
      printf("focusChanged: at widget %p with type %s\n",ptr, typeid(*ptr).name());
    
    if ( (dynamic_cast<MusEGui::TopWin*>(ptr)!=0) || // *ptr is a TopWin or a derived class
         (ptr==this) )                               // the main window is selected
      break;
    ptr=dynamic_cast<QWidget*>(ptr->parent()); //in the unlikely case that ptr is a QObject, this returns NULL, which stops the loop
  }
  
  MusEGui::TopWin* win=dynamic_cast<MusEGui::TopWin*>(ptr);
  // ptr is either NULL, this or the pointer to a TopWin
  
  // if the main win or some deleting topwin is selected,
  // don't treat that as "none", but also don't handle it
  if (ptr!=this && (!win || !win->deleting()) )
  {
    // now 'win' is either NULL or the pointer to the active TopWin
    if (win!=activeTopWin)
    {
      activeTopWin=win;
      emit activeTopWinChanged(activeTopWin);
    }
  }
}


void MusE::activeTopWinChangedSlot(MusEGui::TopWin* win)
{
  if (MusEGlobal::debugMsg) printf("ACTIVE TOPWIN CHANGED to '%s' (%p)\n", win ? win->windowTitle().toAscii().data() : "<None>", win);
  
  if ( (win && (win->isMdiWin()==false) && win->sharesToolsAndMenu()) &&
       ( (mdiArea->currentSubWindow() != NULL) && (mdiArea->currentSubWindow()->isVisible()==true) ) )
  {
    if (MusEGlobal::debugMsg) printf("  that's a menu sharing muse window which isn't inside the MDI area.\n");
    // if a window gets active which a) is a muse window, b) is not a mdi subwin and c) shares menu- and toolbar,
    // then unfocus the MDI area and/or the currently active MDI subwin. otherwise you'll be unable to use win's
    // tools or menu entries, as whenever you click at them, they're replaced by the currently active MDI subwin's
    // menus and toolbars.
    // as unfocusing the MDI area and/or the subwin does not work for some reason, we must do this workaround:
    // simply focus anything in the main window except the mdi area.
    menuBar()->setFocus(Qt::MenuBarFocusReason);
  }
  
  if (win && (win->sharesToolsAndMenu()))
    setCurrentMenuSharingTopwin(win);
}



void MusE::setCurrentMenuSharingTopwin(MusEGui::TopWin* win)
{
  if (win && (win->sharesToolsAndMenu()==false))
  {
    printf("WARNING: THIS SHOULD NEVER HAPPEN: MusE::setCurrentMenuSharingTopwin() called with a win which does not share (%s)! ignoring...\n", win->windowTitle().toAscii().data());
    return;
  }
  
  if (win!=currentMenuSharingTopwin)
  {
    MusEGui::TopWin* previousMenuSharingTopwin = currentMenuSharingTopwin;
    currentMenuSharingTopwin = NULL;
    
    if (MusEGlobal::debugMsg) printf("MENU SHARING TOPWIN CHANGED to '%s' (%p)\n", win ? win->windowTitle().toAscii().data() : "<None>", win);
    
    // empty our toolbars
    if (previousMenuSharingTopwin)
    {
      for (list<QToolBar*>::iterator it = foreignToolbars.begin(); it!=foreignToolbars.end(); it++)
        if (*it) 
        {
          if (MusEGlobal::heavyDebugMsg) printf("  removing sharer's toolbar '%s'\n", (*it)->windowTitle().toAscii().data());
          removeToolBar(*it); // this does not delete *it, which is good
          (*it)->setParent(NULL);
        }

      foreignToolbars.clear();
    }
    else
    {
      for (list<QToolBar*>::iterator it = optionalToolbars.begin(); it!=optionalToolbars.end(); it++)
        if (*it) 
        {
          if (MusEGlobal::heavyDebugMsg) printf("  removing optional toolbar '%s'\n", (*it)->windowTitle().toAscii().data());
          removeToolBar(*it); // this does not delete *it, which is good
          (*it)->setParent(NULL);
        }
    }
    
    //empty our menu
    menuBar()->clear();
        
    
    
    
    for (list<QMenu*>::iterator it = leadingMenus.begin(); it!=leadingMenus.end(); it++)
      menuBar()->addMenu(*it);

    if (win)
    {
      const QList<QAction*>& actions=win->menuBar()->actions();
      for (QList<QAction*>::const_iterator it=actions.begin(); it!=actions.end(); it++)
      {
        if (MusEGlobal::heavyDebugMsg) printf("  adding menu entry '%s'\n", (*it)->text().toAscii().data());
        
        menuBar()->addAction(*it);
      }


      
      const list<QToolBar*>& toolbars=win->toolbars();
      for (list<QToolBar*>::const_iterator it=toolbars.begin(); it!=toolbars.end(); it++)
        if (*it)
        {
          if (MusEGlobal::heavyDebugMsg) printf("  adding toolbar '%s'\n", (*it)->windowTitle().toAscii().data());
          
          addToolBar(*it);
          foreignToolbars.push_back(*it);
          (*it)->show();
        }
        else
        {
          if (MusEGlobal::heavyDebugMsg) printf("  adding toolbar break\n");
          
          addToolBarBreak();
          foreignToolbars.push_back(NULL);
        }
    }

    for (list<QMenu*>::iterator it = trailingMenus.begin(); it!=trailingMenus.end(); it++)
      menuBar()->addMenu(*it);
      

    currentMenuSharingTopwin=win;
    
    if (win)
      win->restoreMainwinState(); //restore toolbar positions in main window
  }
}

void MusE::addMdiSubWindow(QMdiSubWindow* win)
{
  mdiArea->addSubWindow(win);
}

void MusE::shareMenuAndToolbarChanged(MusEGui::TopWin* win, bool val)
{
  if (val)
  {
    if ((win == activeTopWin) && (win != currentMenuSharingTopwin))
      setCurrentMenuSharingTopwin(win);
  }
  else
  {
    if (win == currentMenuSharingTopwin)
    {
      if (activeTopWin && (win != activeTopWin) && (activeTopWin->sharesToolsAndMenu()))
        setCurrentMenuSharingTopwin(activeTopWin);
      else
        setCurrentMenuSharingTopwin(NULL);
    }
  }
}

void MusE::topwinMenuInited(MusEGui::TopWin* topwin)
{
  if (topwin==NULL)
    return;
    
  if (topwin == waitingForTopwin)
  {
    if (waitingForTopwin->deleting())
    {
      waitingForTopwin=NULL;
    }
    else
    {
      activeTopWin=waitingForTopwin;
      waitingForTopwin=NULL;
      emit activeTopWinChanged(activeTopWin);
    }
  }
  else if (topwin == currentMenuSharingTopwin)
  {
    printf("====== DEBUG ======: topwin's menu got inited AFTER being shared!\n");
    if (!topwin->sharesToolsAndMenu()) printf("======       ======: WTF, now it doesn't share any more?!?\n");
    setCurrentMenuSharingTopwin(NULL);
    setCurrentMenuSharingTopwin(topwin);
  }
}

void MusE::updateWindowMenu()
{
  bool sep;
  bool there_are_subwins=false;
  
  menuWindows->clear(); // frees memory automatically
  
  menuWindows->addAction(windowsCascadeAction);
  menuWindows->addAction(windowsTileAction);
  menuWindows->addAction(windowsRowsAction);
  menuWindows->addAction(windowsColumnsAction);
  
  sep=false;
  for (MusEGui::iToplevel it=toplevels.begin(); it!=toplevels.end(); it++)
    if (((*it)->isVisible() || (*it)->isVisibleTo(this)) && (*it)->isMdiWin())
    // the isVisibleTo check is neccessary because isVisible returns false if a
    // MdiSubWin is actually visible, but the muse main window is hidden for some reason
    {
      if (!sep)
      {
        menuWindows->addSeparator();
        sep=true;
      }
      QAction* temp=menuWindows->addAction((*it)->windowTitle());
      connect(temp, SIGNAL(activated()), windowsMapper, SLOT(map()));
      windowsMapper->setMapping(temp, static_cast<QWidget*>(*it));
      
      there_are_subwins=true;
    }

  sep=false;
  for (MusEGui::iToplevel it=toplevels.begin(); it!=toplevels.end(); it++)
    if (((*it)->isVisible() || (*it)->isVisibleTo(this)) && !(*it)->isMdiWin())
    {
      if (!sep)
      {
        menuWindows->addSeparator();
        sep=true;
      }
      QAction* temp=menuWindows->addAction((*it)->windowTitle());
      connect(temp, SIGNAL(activated()), windowsMapper, SLOT(map()));
      windowsMapper->setMapping(temp, static_cast<QWidget*>(*it));
    }
  
  windowsCascadeAction->setEnabled(there_are_subwins);
  windowsTileAction->setEnabled(there_are_subwins);
  windowsRowsAction->setEnabled(there_are_subwins);
  windowsColumnsAction->setEnabled(there_are_subwins);
}

void MusE::bringToFront(QWidget* widget)
{
  MusEGui::TopWin* win=dynamic_cast<MusEGui::TopWin*>(widget);
  if (!win) return;
  
  if (win->isMdiWin())
  {
    win->show();
    mdiArea->setActiveSubWindow(win->getMdiWin());
  }
  else
  {
    win->activateWindow();
    win->raise();
  }
  
  activeTopWin=win;
  emit activeTopWinChanged(win);
}

void MusE::setFullscreen(bool val)
{
  if (val)
    showFullScreen();
  else
    showNormal();
}



list<QMdiSubWindow*> get_all_visible_subwins(QMdiArea* mdiarea)
{
  QList<QMdiSubWindow*> wins = mdiarea->subWindowList();
  list<QMdiSubWindow*> result;
  
  // always put the arranger at the top of the list, if visible
  
  for (QList<QMdiSubWindow*>::iterator it=wins.begin(); it!=wins.end(); it++)
    if ((*it)->isVisible() && ((*it)->isMinimized()==false))
      if (dynamic_cast<MusEGui::TopWin*>((*it)->widget())->type()==MusEGui::TopWin::ARRANGER)
        result.push_back(*it);
  
  for (QList<QMdiSubWindow*>::iterator it=wins.begin(); it!=wins.end(); it++)
    if ((*it)->isVisible() && ((*it)->isMinimized()==false))
      if (dynamic_cast<MusEGui::TopWin*>((*it)->widget())->type()!=MusEGui::TopWin::ARRANGER)
        result.push_back(*it);
  
  return result;
}

void MusE::arrangeSubWindowsColumns()
{
  list<QMdiSubWindow*> wins=get_all_visible_subwins(mdiArea);
  int n=wins.size();
  
  if (n==0)
    return;
  //else if (n==1)
  //  (*wins.begin())->showMaximized(); // commented out by flo. i like it better that way.
  else
  {
    int width = mdiArea->width();
    int height = mdiArea->height();
    int x_add = (*wins.begin())->frameGeometry().width() - (*wins.begin())->geometry().width();
    int y_add = (*wins.begin())->frameGeometry().height() - (*wins.begin())->geometry().height();
    int width_per_win = width/n;
    
    if (x_add >= width_per_win)
    {
      printf("ERROR: tried to arrange subwins in columns, but there's too few space.\n");
      return;
    }
    
    int i=0;
    for (list<QMdiSubWindow*>::iterator it=wins.begin(); it!=wins.end(); it++, i++)
    {
      int left = (float) width*i/n;
      int right = (float) width*(i+1.0)/n;
      
      (*it)->move(left,0);
      (*it)->resize(right-left-x_add, height-y_add);
    }
  }
}

void MusE::arrangeSubWindowsRows()
{
  list<QMdiSubWindow*> wins=get_all_visible_subwins(mdiArea);
  int n=wins.size();
  
  if (n==0)
    return;
  //else if (n==1)
  //  (*wins.begin())->showMaximized(); // commented out by flo. i like it better that way.
  else
  {
    int width = mdiArea->width();
    int height = mdiArea->height();
    int x_add = (*wins.begin())->frameGeometry().width() - (*wins.begin())->geometry().width();
    int y_add = (*wins.begin())->frameGeometry().height() - (*wins.begin())->geometry().height();
    int height_per_win = height/n;
    
    if (y_add >= height_per_win)
    {
      printf("ERROR: tried to arrange subwins in rows, but there's too few space.\n");
      return;
    }
    
    int i=0;
    for (list<QMdiSubWindow*>::iterator it=wins.begin(); it!=wins.end(); it++, i++)
    {
      int top = (float) height*i/n;
      int bottom = (float) height*(i+1.0)/n;
      
      (*it)->move(0,top);
      (*it)->resize(width-x_add, bottom-top-y_add);
    }
  }  
}

void MusE::tileSubWindows()
{
  list<QMdiSubWindow*> wins=get_all_visible_subwins(mdiArea);
  int n=wins.size();
  
  if (n==0)
    return;
  //else if (n==1)
  //  (*wins.begin())->showMaximized(); // commented out by flo. i like it better that way.
  else
  {
    int nx,ny;
    nx=ceil(sqrt(n));
    ny=ceil((double)n/nx);
    
    int width = mdiArea->width();
    int height = mdiArea->height();
    int x_add = (*wins.begin())->frameGeometry().width() - (*wins.begin())->geometry().width();
    int y_add = (*wins.begin())->frameGeometry().height() - (*wins.begin())->geometry().height();
    int height_per_win = height/ny;
    int width_per_win = height/nx;
    
    if ((x_add >= width_per_win) || (y_add >= height_per_win))
    {
      printf("ERROR: tried to tile subwins, but there's too few space.\n");
      return;
    }
    
    int i=0, j=0;
    for (list<QMdiSubWindow*>::iterator it=wins.begin(); it!=wins.end(); it++, i++)
    {
      if (i>=nx)
      {
        i=0;
        j++;
      }
      
      int top = (float) height*j/ny;
      int bottom = (float) height*(j+1.0)/ny;
      int left = (float) width*i/nx;
      int right = (float) width*(i+1.0)/nx;
      
      (*it)->move(left,top);
      (*it)->resize(right-left-x_add, bottom-top-y_add);
    }
  }
}

QString MusE::projectTitle() const
{ 
  return MusEGui::projectTitleFromFilename(project.fileName());
}

QString MusE::projectPath() const 
{ 
  return MusEGui::projectPathFromFilename(project.absoluteFilePath()); 
}

QString MusE::projectExtension() const
{
  return MusEGui::projectExtensionFromFilename(project.fileName()); 
}

} //namespace MusEGui
