//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: app.cpp,v 1.113.2.68 2009/12/21 14:51:51 spamatica Exp $
//
//  (C) Copyright 1999-2011 Werner Schweer (ws@seh.de)
//  (C) Copyright 2011-2016 Tim E. Real (terminator356 on sourceforge)
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
#include <QTimer>
#include <QWhatsThis>
#include <QSettings>
#include <QProgressDialog>
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QSocketNotifier>
#include <QString>
#include <QStyleFactory>
#include <QTextStream>
#include <QToolButton>
#include <QInputDialog>
#include <QAction>
#include <QStringList>
#include <QPushButton>
#include <QDir>
#include <samplerate.h>

#include <errno.h>
#include <iostream>
#include <algorithm>

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
#include "components/bigtime.h"
#include "cliplist/cliplist.h"
#include "conf.h"
#include "config.h"
#include "debug.h"
#include "components/didyouknow.h"
#include "drumedit.h"
#include "components/filedialog.h"
#include "gconfig.h"
#include "globals.h"
#include "components/genset.h"
#include "gui.h"
#include "helper.h"
#include "wave_helper.h"
#include "icons.h"
#include "instruments/editinstrument.h"
#include "listedit.h"
#include "marker/markerview.h"
#include "master/masteredit.h"
#include "components/metronome.h"
#include "midifilterimpl.h"
#include "midiitransform.h"
#include "midiseq.h"
#include "midisyncimpl.h"
#include "miditransform.h"
#include "mitplugin.h"
#include "mittranspose.h"
#include "widgets/musemdiarea.h"
#include "components/mixdowndialog.h"
#include "mrconfig.h"
#include "pianoroll.h"
#include "scoreedit.h"
#ifdef PYTHON_SUPPORT
#include "remote/pyapi.h"
#endif
#ifdef BUILD_EXPERIMENTAL
  #include "rhythm.h"
#endif
#include "song.h"
#include "components/routepopup.h"
#include "components/shortcutconfig.h"
#include "components/songinfo.h"
#include "ticksynth.h"
#include "transport.h"
#include "tempo.h"
#include "tlist.h"
#include "waveedit.h"
#include "components/projectcreateimpl.h"
#include "widgets/menutitleitem.h"
#include "components/tools.h"
#include "components/unusedwavefiles.h"
#include "functions.h"
#include "components/songpos_toolbar.h"
#include "components/sig_tempo_toolbar.h"
#include "widgets/cpu_toolbar.h"
#include "components/snooper.h"
#include "songfile_discovery.h"
#include "pos.h"
#include "wave.h"
#include "wavepreview.h"
#include "track.h"
#include "part.h"

#ifdef _WIN32
#include <Windows.h>
#endif

namespace MusECore {
extern void exitJackAudio();
extern void exitDummyAudio();
extern void exitOSC();
extern void exitMidiAlsa();

#ifdef HAVE_RTAUDIO
extern void exitRtAudio();
#endif
}

namespace MusEGui {

extern void deleteIcons();

static pthread_t watchdogThread;
//ErrorHandler *error;

#define PROJECT_LIST_LEN  6
QStringList projectRecentList;

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
    int sleepOk=-1;

    while(sleepOk==-1)
        sleepOk=usleep(msleep);
}

//---------------------------------------------------------
//   seqStart
//---------------------------------------------------------

bool MusE::seqStart()
      {
      if(MusEGlobal::audio)
      {
        if(!MusEGlobal::audio->isRunning())
        {
          // Start the audio. (Re)connect audio inputs and outputs. Force-fill the audio pre-fetch buffers for the current cpos.
          if(MusEGlobal::audio->start())
          {
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
                  tr("Timeout waiting for audio to run. Check if jack is running or try another driver.\n"));
            }
          }
          else
          {
            QMessageBox::critical( MusEGlobal::muse, tr("Failed to start audio!"),
                tr("Was not able to start audio, check if jack is running or try another driver.\n"));
          }
        }
      }
      else
        fprintf(stderr, "seqStart(): audio is NULL\n");


      // Now it is safe to ask the driver for realtime priority

      int pfprio = 0;
      // TODO: Hm why is prefetch priority so high again? Was it to overcome some race problems? Should it be lowest - disk thread?
      if(MusEGlobal::audioDevice)
      {
        MusEGlobal::realTimePriority = MusEGlobal::audioDevice->realtimePriority();
        if(MusEGlobal::debugMsg)
          fprintf(stderr, "MusE::seqStart: getting audio driver MusEGlobal::realTimePriority:%d\n", MusEGlobal::realTimePriority);

        // NOTE: MusEGlobal::realTimeScheduling can be true (gotten using jack_is_realtime()),
        //  while the determined MusEGlobal::realTimePriority can be 0.
        // MusEGlobal::realTimePriority is gotten using pthread_getschedparam() on the client thread
        //  in JackAudioDevice::realtimePriority() which is a bit flawed - it reports there's no RT...
        if(MusEGlobal::realTimeScheduling)
        {
          if(MusEGlobal::realTimePriority - 5 >= 0)
            pfprio = MusEGlobal::realTimePriority - 5;
        }
        // FIXME: The realTimePriority of the Jack thread seems to always be 5 less than the value passed to jackd command.
      }
      else
        fprintf(stderr, "seqStart(): audioDevice is NULL\n");

      if(MusEGlobal::audioPrefetch)
      {
        if(!MusEGlobal::audioPrefetch->isRunning())
        {
          MusEGlobal::audioPrefetch->start(pfprio);
          // In case prefetch is not filled, do it now.
          MusEGlobal::audioPrefetch->msgSeek(MusEGlobal::audio->pos().frame(), true); // Force it upon startup only.
        }
      }
      else
        fprintf(stderr, "seqStart(): audioPrefetch is NULL\n");

      if(MusEGlobal::midiSeq)
        MusEGlobal::midiSeq->start(0); // Prio unused, set in start.

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
      if(MusEGlobal::midiSeq)
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
//   addProject to recent list
//---------------------------------------------------------

void addProject(const QString& name)
{
  if (projectRecentList.contains(name))
    return;

  projectRecentList.push_front(name);
  if (projectRecentList.size() > PROJECT_LIST_LEN)
    projectRecentList.pop_back();

}

//---------------------------------------------------------
//   MusE
//---------------------------------------------------------

MusE::MusE() : QMainWindow()
      {
      setIconSize(QSize(MusEGlobal::config.iconSize, MusEGlobal::config.iconSize));
//      setIconSize(ICON_SIZE);
      setFocusPolicy(Qt::NoFocus);
      MusEGlobal::muse      = this;    // hack
      _isRestartingApp      = false;
      clipListEdit          = 0;
      midiSyncConfig        = 0;
      midiRemoteConfig      = 0;
      midiPortConfig        = 0;
      metronomeConfig       = 0;
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
      _snooperDialog        = 0;
      //audioMixer            = 0;
      mixer1                = 0;
      mixer2                = 0;
      routeDialog           = 0;
      watchdogThread        = 0;
      editInstrument        = 0;
      //routingPopupMenu      = 0;
      progress              = 0;
      saveIncrement         = 0;
      activeTopWin          = NULL;
      currentMenuSharingTopwin = NULL;
      waitingForTopwin      = NULL;

      appName               = PACKAGE_NAME;
      setWindowTitle(appName);
      setWindowIcon(*MusEGui::museIcon);

      MusEGlobal::song = new MusECore::Song("song");
      MusEGlobal::song->blockSignals(true);
      MusEGlobal::heartBeatTimer = new QTimer(this);
      MusEGlobal::heartBeatTimer->setObjectName("timer");
      connect(MusEGlobal::heartBeatTimer, SIGNAL(timeout()), MusEGlobal::song, SLOT(beat()));
      connect(MusEGlobal::heartBeatTimer, SIGNAL(timeout()), SLOT(heartBeat()));
      connect(this, SIGNAL(activeTopWinChanged(MusEGui::TopWin*)), SLOT(activeTopWinChangedSlot(MusEGui::TopWin*)));
      connect(MusEGlobal::song, SIGNAL(sigDirty()), this, SLOT(setDirty()));

      blinkTimer = new QTimer(this);
      blinkTimer->setObjectName("blinkTimer");
      connect(blinkTimer, SIGNAL(timeout()), SLOT(blinkTimerSlot()));
      blinkTimer->start( 250 );      // Every quarter second, for a flash rate of 2 Hz.

      saveTimer = new QTimer(this);
      connect(saveTimer, SIGNAL(timeout()), this, SLOT(saveTimerSlot()));
      saveTimer->start( 60 * 1000 ); // every minute

      messagePollTimer = new QTimer(this);
      messagePollTimer->setObjectName("messagePollTimer");
      connect(messagePollTimer, SIGNAL(timeout()), SLOT(messagePollTimerSlot()));
      // A zero-millisecond poll timer. Oops, no can't do that in the gui thread,
      //  it spikes the CPU usage because it eats up all the idle time. Use say, 50Hz 20msec.
      messagePollTimer->start(20);

      //init cpuload stuff
      clock_gettime(CLOCK_REALTIME, &lastSysTime);
      lastCpuTime.tv_sec = 0;
      lastCpuTime.tv_usec = 0;
      fAvrCpuLoad = 0.0f;
      avrCpuLoadCounter = 0;
      fCurCpuLoad = 0.0f;

#ifdef PYTHON_SUPPORT
      //---------------------------------------------------
      //    Python bridge
      //---------------------------------------------------
      // Uncomment in order to enable MusE Python bridge:
      if (MusEGlobal::usePythonBridge) {
            fprintf(stderr, "Initializing python bridge!\n");
            if (startPythonBridge() == false) {
                  fprintf(stderr, "Could not initialize Python bridge\n");
                  exit(1);
                  }
            }
#endif

      //---------------------------------------------------
      //    undo/redo
      //---------------------------------------------------

      MusEGlobal::undoRedo = new QActionGroup(this);
      MusEGlobal::undoRedo->setExclusive(false);
      MusEGlobal::undoAction = new QAction(*MusEGui::undoSVGIcon, tr("Und&o"),
        MusEGlobal::undoRedo);
      MusEGlobal::redoAction = new QAction(*MusEGui::redoSVGIcon, tr("Re&do"),
        MusEGlobal::undoRedo);

      MusEGlobal::undoAction->setWhatsThis(tr("Undo last change to song"));
      MusEGlobal::redoAction->setWhatsThis(tr("Redo last undo"));
      MusEGlobal::undoAction->setEnabled(false);
      MusEGlobal::redoAction->setEnabled(false);
      connect(MusEGlobal::redoAction, SIGNAL(triggered()), MusEGlobal::song, SLOT(redo()));
      connect(MusEGlobal::undoAction, SIGNAL(triggered()), MusEGlobal::song, SLOT(undo()));

      //---------------------------------------------------
      //    Transport
      //---------------------------------------------------

      MusEGlobal::transportAction = new QActionGroup(this);
      MusEGlobal::transportAction->setExclusive(false);

      MusEGlobal::loopAction = new QAction(*MusEGui::loopSVGIcon, tr("Loop"),
                                           MusEGlobal::transportAction);
      MusEGlobal::loopAction->setCheckable(true);

      MusEGlobal::loopAction->setWhatsThis(tr("Loop between left mark and right mark"));
      connect(MusEGlobal::loopAction, SIGNAL(toggled(bool)), MusEGlobal::song, SLOT(setLoop(bool)));

      MusEGlobal::punchinAction = new QAction(*MusEGui::punchinSVGIcon, tr("Punch in"),
                                              MusEGlobal::transportAction);
      MusEGlobal::punchinAction->setCheckable(true);

      MusEGlobal::punchinAction->setWhatsThis(tr("Record starts at left mark"));
      connect(MusEGlobal::punchinAction, SIGNAL(toggled(bool)), MusEGlobal::song, SLOT(setPunchin(bool)));

      MusEGlobal::punchoutAction = new QAction(*MusEGui::punchoutSVGIcon, tr("Punch out"),
                                               MusEGlobal::transportAction);
      MusEGlobal::punchoutAction->setCheckable(true);

      MusEGlobal::punchoutAction->setWhatsThis(tr("Record stops at right mark"));
      connect(MusEGlobal::punchoutAction, SIGNAL(toggled(bool)), MusEGlobal::song, SLOT(setPunchout(bool)));

      QAction *tseparator = new QAction(this);
      tseparator->setSeparator(true);
      MusEGlobal::transportAction->addAction(tseparator);

      MusEGlobal::startAction = new QAction(*MusEGui::rewindToStartSVGIcon, tr("Start"), 
                                             MusEGlobal::transportAction);

      MusEGlobal::startAction->setWhatsThis(tr("Rewind to start position"));
      connect(MusEGlobal::startAction, SIGNAL(triggered()), MusEGlobal::song, SLOT(rewindStart()));

      MusEGlobal::rewindAction = new QAction(*MusEGui::rewindSVGIcon, tr("Rewind"), 
                                              MusEGlobal::transportAction);

      MusEGlobal::rewindAction->setWhatsThis(tr("Rewind current position"));
      connect(MusEGlobal::rewindAction, SIGNAL(triggered()), MusEGlobal::song, SLOT(rewind()));

      MusEGlobal::forwardAction = new QAction(*MusEGui::fastForwardSVGIcon, tr("Forward"), 
                                               MusEGlobal::transportAction);

      MusEGlobal::forwardAction->setWhatsThis(tr("Move current position"));
      connect(MusEGlobal::forwardAction, SIGNAL(triggered()), MusEGlobal::song, SLOT(forward()));

      MusEGlobal::stopAction = new QAction(*MusEGui::stopSVGIcon, tr("Stop"), 
                                            MusEGlobal::transportAction);
      MusEGlobal::stopAction->setCheckable(true);

      MusEGlobal::stopAction->setWhatsThis(tr("Stop sequencer"));
      MusEGlobal::stopAction->setChecked(true);
      connect(MusEGlobal::stopAction, SIGNAL(toggled(bool)), MusEGlobal::song, SLOT(setStop(bool)));

      MusEGlobal::playAction = new QAction(*MusEGui::playSVGIcon,
                     tr("Play") + " (" + shrtToStr(MusEGui::SHRT_PLAY_TOGGLE)
                         + ")<br>"+ tr("Restart rec")+" (" + QKeySequence(MusEGui::shortcuts[MusEGui::SHRT_REC_RESTART].key).toString() + ")",
                     MusEGlobal::transportAction);
      MusEGlobal::playAction->setCheckable(true);

      MusEGlobal::playAction->setWhatsThis(tr("Start sequencer play"));
      MusEGlobal::playAction->setChecked(false);
      connect(MusEGlobal::playAction, SIGNAL(toggled(bool)), MusEGlobal::song, SLOT(setPlay(bool)));

      MusEGlobal::recordAction = new QAction(*MusEGui::recMasterSVGIcon, tr("Record"), 
                                              MusEGlobal::transportAction);
      MusEGlobal::recordAction->setCheckable(true);
      MusEGlobal::recordAction->setWhatsThis(tr("To record press record and then play"));
      connect(MusEGlobal::recordAction, SIGNAL(toggled(bool)), MusEGlobal::song, SLOT(setRecord(bool)));

      MusEGlobal::panicAction = new QAction(*MusEGui::panicSVGIcon, tr("Panic"), this);

      QMenu* panicPopupMenu = new QMenu(this);
      MusEGlobal::panicAction->setMenu(panicPopupMenu);
      
      MusEGlobal::panicAction->setWhatsThis(tr("Send note off to all midi channels"));
      connect(MusEGlobal::panicAction, SIGNAL(triggered()), MusEGlobal::song, SLOT(panic()));

      MusEGlobal::metronomeAction = new QAction(*MusEGui::metronomeOnSVGIcon, tr("Metronome"), this);
      MusEGlobal::metronomeAction->setCheckable(true);
      MusEGlobal::metronomeAction->setWhatsThis(tr("Turn on/off metronome"));
      MusEGlobal::metronomeAction->setChecked(MusEGlobal::song->click());
      connect(MusEGlobal::metronomeAction, SIGNAL(toggled(bool)), MusEGlobal::song, SLOT(setClick(bool)));
      connect(MusEGlobal::song, SIGNAL(clickChanged(bool)), MusEGlobal::metronomeAction, SLOT(setChecked(bool)));

      //----Actions
      //-------- File Actions

      fileNewAction = new QAction(*MusEGui::filenewSVGIcon, tr("&New"), this);
      fileNewAction->setToolTip(tr("Create new song"));
      fileNewAction->setWhatsThis(tr("Create new song"));

      fileNewFromTemplateAction = new QAction(*MusEGui::filetemplateSVGIcon, tr("New from &Template..."), this);
      fileNewFromTemplateAction->setToolTip(tr("Create new song from template"));
      fileNewFromTemplateAction->setWhatsThis(tr("Create new song from template"));

      fileOpenAction = new QAction(*MusEGui::fileopenSVGIcon, tr("&Open..."), this);

      fileOpenAction->setToolTip(tr("Open song from file"));
      fileOpenAction->setWhatsThis(tr("Click this button to open an existing song."));

      openRecent = new QMenu(tr("Open &Recent"), this);

      fileSaveAction = new QAction(*MusEGui::filesaveSVGIcon, tr("&Save"), this);

      fileSaveAction->setToolTip(tr("Save current song"));
      fileSaveAction->setWhatsThis(tr("Click this button to save the song you are editing. You will be prompted for a file name."));

      fileSaveAsAction = new QAction(*MusEGui::filesaveasSVGIcon, tr("Save &As..."), this);

      fileCloseAction = new QAction(*MusEGui::filecloseSVGIcon, tr("&Close"), this);
      
      fileImportMidiAction = new QAction(tr("Import Midifile..."), this);
      fileExportMidiAction = new QAction(tr("Export Midifile..."), this);
      fileImportPartAction = new QAction(tr("Import Part..."), this);

      fileImportWaveAction = new QAction(tr("Import Audio File..."), this);
      fileMoveWaveFiles = new QAction(tr("Find Unused Wave Files..."), this);

      quitAction = new QAction(*MusEGui::appexitSVGIcon, tr("&Quit"), this);

      editSongInfoAction = new QAction(QIcon(*MusEGui::edit_listIcon), tr("Song Info..."), this);

      //-------- View Actions
      viewTransportAction = new QAction(QIcon(*MusEGui::view_transport_windowIcon), tr("Transport Panel..."), this);
      viewTransportAction->setCheckable(true);
      viewBigtimeAction = new QAction(QIcon(*MusEGui::view_bigtime_windowIcon), tr("Bigtime Window..."),  this);
      viewBigtimeAction->setCheckable(true);
      viewMixerAAction = new QAction(QIcon(*MusEGui::mixerSIcon), tr("Mixer A..."), this);
      viewMixerAAction->setCheckable(true);
      viewMixerBAction = new QAction(QIcon(*MusEGui::mixerSIcon), tr("Mixer B..."), this);
      viewMixerBAction->setCheckable(true);
      viewCliplistAction = new QAction(QIcon(*MusEGui::cliplistSIcon), tr("Cliplist..."), this);
      viewCliplistAction->setCheckable(true);
      viewMarkerAction = new QAction(QIcon(*MusEGui::view_markerIcon), tr("Marker View..."),  this);
      viewMarkerAction->setCheckable(true);
      viewArrangerAction = new QAction(tr("Arranger View"),  this);
      viewArrangerAction->setCheckable(true);
      fullscreenAction=new QAction(tr("Fullscreen"), this);
      fullscreenAction->setCheckable(true);
      fullscreenAction->setChecked(false);
      QMenu* master = new QMenu(tr("Mastertrack"), this);
      master->setIcon(QIcon(*edit_mastertrackIcon));
      masterGraphicAction = new QAction(QIcon(*mastertrack_graphicIcon),tr("Graphic..."), this);
      masterListAction = new QAction(QIcon(*mastertrack_listIcon),tr("List..."), this);
      master->addAction(masterGraphicAction);
      master->addAction(masterListAction);

      //-------- Midi Actions
      menuScriptPlugins = new QMenu(tr("&Plugins"), this);
      midiEditInstAction = new QAction(QIcon(*MusEGui::midi_edit_instrumentIcon), tr("Edit Instrument..."), this);
      midiInputPlugins = new QMenu(tr("Input Plugins"), this);
      midiInputPlugins->setIcon(QIcon(*MusEGui::midi_inputpluginsIcon));
      midiTrpAction = new QAction(QIcon(*MusEGui::midi_inputplugins_transposeIcon), tr("Transpose..."), this);
      midiInputTrfAction = new QAction(QIcon(*MusEGui::midi_inputplugins_midi_input_transformIcon), tr("Midi Input Transform..."), this);
      midiInputFilterAction = new QAction(QIcon(*MusEGui::midi_inputplugins_midi_input_filterIcon), tr("Midi Input Filter..."), this);
      midiRemoteAction = new QAction(QIcon(*MusEGui::midi_inputplugins_remote_controlIcon), tr("Midi Remote Control..."), this);
#ifdef BUILD_EXPERIMENTAL
      midiRhythmAction = new QAction(QIcon(*midi_inputplugins_random_rhythm_generatorIcon), tr("Rhythm Generator"), this);
#endif
      midiResetInstAction = new QAction(QIcon(*MusEGui::midi_reset_instrIcon), tr("Reset Instrument"), this);
      midiInitInstActions = new QAction(QIcon(*MusEGui::midi_init_instrIcon), tr("Init Instrument"), this);
      midiLocalOffAction = new QAction(QIcon(*MusEGui::midi_local_offIcon), tr("Local Off"), this);

      //-------- Audio Actions
      audioBounce2TrackAction = new QAction(QIcon(*MusEGui::audio_bounce_to_trackIcon), tr("Bounce to Track"), this);
      audioBounce2FileAction = new QAction(QIcon(*MusEGui::audio_bounce_to_fileIcon), tr("Bounce to File..."), this);
      audioRestartAction = new QAction(QIcon(*MusEGui::audio_restartaudioIcon), tr("Restart Audio"), this);

      //-------- Automation Actions
      autoMixerAction = new QAction(QIcon(*MusEGui::automation_mixerIcon), tr("Mixer Automation"), this);
      autoMixerAction->setCheckable(true);
      autoSnapshotAction = new QAction(QIcon(*MusEGui::automation_take_snapshotIcon), tr("Take Snapshot"), this);
      autoClearAction = new QAction(QIcon(*MusEGui::automation_clear_dataIcon), tr("Clear Automation Data"), this);

      //-------- Windows Actions
      windowsCascadeAction = new QAction(tr("Cascade"), this);
      windowsTileAction = new QAction(tr("Tile"), this);
      windowsRowsAction = new QAction(tr("In Rows"), this);
      windowsColumnsAction = new QAction(tr("In Columns"), this);


      //-------- Settings Actions
      settingsGlobalAction = new QAction(QIcon(*MusEGui::settings_globalsettingsIcon), tr("Global Settings..."), this);
      settingsAppearanceAction = new QAction(QIcon(*MusEGui::settings_appearance_settingsIcon), tr("Appearance..."), this);
      settingsShortcutsAction = new QAction(QIcon(*MusEGui::settings_configureshortcutsIcon), tr("Configure Shortcuts..."), this);
      follow = new QMenu(tr("Follow Song"), this);
      dontFollowAction = new QAction(tr("Don't Follow Song"), this);
      dontFollowAction->setCheckable(true);
      followPageAction = new QAction(tr("Follow Page"), this);
      followPageAction->setCheckable(true);
      followPageAction->setChecked(true);
      followCtsAction = new QAction(tr("Follow Continuous"), this);
      followCtsAction->setCheckable(true);

      rewindOnStopAction=new QAction(tr("Rewind on Stop"), this);
      rewindOnStopAction->setCheckable(true);
      rewindOnStopAction->setChecked(MusEGlobal::config.useRewindOnStop);

      settingsMetronomeAction = new QAction(*MusEGui::metronomeOnSVGIcon, tr("Metronome..."), this);
      settingsMidiSyncAction = new QAction(QIcon(*MusEGui::settings_midisyncIcon), tr("Midi Sync..."), this);
      settingsMidiIOAction = new QAction(QIcon(*MusEGui::settings_midifileexportIcon), tr("Midi File Import/Export..."), this);
      settingsMidiPortAction = new QAction(QIcon(*MusEGui::settings_midiport_softsynthsIcon), tr("Midi Ports / Soft Synth..."), this);

      //-------- Help Actions
      helpManualAction = new QAction(tr("&Manual..."), this);
      helpHomepageAction = new QAction(tr("&MusE Homepage..."), this);
      helpDidYouKnow = new QAction(tr("&Did You Know?"), this);

      helpReportAction = new QAction(tr("&Report Bug..."), this);
      helpAboutAction = new QAction(tr("&About MusE..."), this);

      helpSnooperAction = new QAction(tr("Snooper (developer tool)..."), this);

      //---- Connections
      //-------- File connections

      connect(fileNewAction,  SIGNAL(triggered()), SLOT(loadDefaultTemplate()));
      connect(fileNewFromTemplateAction,  SIGNAL(triggered()), SLOT(loadTemplate()));
      connect(fileOpenAction, SIGNAL(triggered()), SLOT(loadProject()));
      connect(openRecent, SIGNAL(aboutToShow()), SLOT(openRecentMenu()));
      connect(openRecent, SIGNAL(triggered(QAction*)), SLOT(selectProject(QAction*)));

      connect(fileSaveAction, SIGNAL(triggered()), SLOT(save()));
      connect(fileSaveAsAction, SIGNAL(triggered()), SLOT(saveAs()));

      connect(fileCloseAction, SIGNAL(triggered()), SLOT(fileClose()));
      
      connect(fileImportMidiAction, SIGNAL(triggered()), SLOT(importMidi()));
      connect(fileExportMidiAction, SIGNAL(triggered()), SLOT(exportMidi()));
      connect(fileImportPartAction, SIGNAL(triggered()), SLOT(importPart()));

      connect(fileImportWaveAction, SIGNAL(triggered()), SLOT(importWave()));
      connect(fileMoveWaveFiles, SIGNAL(triggered()), SLOT(findUnusedWaveFiles()));
      connect(quitAction, SIGNAL(triggered()), SLOT(quitDoc()));

      connect(editSongInfoAction, SIGNAL(triggered()), SLOT(startSongInfo()));

      //-------- View connections
      connect(viewTransportAction, SIGNAL(toggled(bool)), SLOT(toggleTransport(bool)));
      connect(viewBigtimeAction, SIGNAL(toggled(bool)), SLOT(toggleBigTime(bool)));
      connect(viewMixerAAction, SIGNAL(toggled(bool)),SLOT(toggleMixer1(bool)));
      connect(viewMixerBAction, SIGNAL(toggled(bool)), SLOT(toggleMixer2(bool)));
      connect(viewCliplistAction, SIGNAL(toggled(bool)), SLOT(startClipList(bool)));
      connect(viewMarkerAction, SIGNAL(toggled(bool)), SLOT(toggleMarker(bool)));
      connect(viewArrangerAction, SIGNAL(toggled(bool)), SLOT(toggleArranger(bool)));
      connect(masterGraphicAction, SIGNAL(triggered()), SLOT(startMasterEditor()));
      connect(masterListAction, SIGNAL(triggered()), SLOT(startLMasterEditor()));
      connect(fullscreenAction, SIGNAL(toggled(bool)), SLOT(setFullscreen(bool)));

      //-------- Midi connections
      connect(midiEditInstAction, SIGNAL(triggered()), SLOT(startEditInstrument()));
      connect(midiResetInstAction, SIGNAL(triggered()), SLOT(resetMidiDevices()));
      connect(midiInitInstActions, SIGNAL(triggered()), SLOT(initMidiDevices()));
      connect(midiLocalOffAction, SIGNAL(triggered()), SLOT(localOff()));

      connect(midiTrpAction,         &QAction::triggered, [this]() { startMidiInputPlugin(0); } );
      connect(midiInputTrfAction,    &QAction::triggered, [this]() { startMidiInputPlugin(1); } );
      connect(midiInputFilterAction, &QAction::triggered, [this]() { startMidiInputPlugin(2); } );
      connect(midiRemoteAction,      &QAction::triggered, [this]() { startMidiInputPlugin(3); } );

#ifdef BUILD_EXPERIMENTAL
      connect(midiRhythmAction,      &QAction::triggered, [this]() { startMidiInputPlugin(4); } );
#endif

      //-------- Audio connections
      connect(audioBounce2TrackAction, SIGNAL(triggered()), SLOT(bounceToTrack()));
      connect(audioBounce2FileAction, SIGNAL(triggered()), SLOT(bounceToFile()));
      connect(audioRestartAction, SIGNAL(triggered()), SLOT(seqRestart()));

      //-------- Automation connections
      connect(autoMixerAction, SIGNAL(triggered()), SLOT(switchMixerAutomation()));
      connect(autoSnapshotAction, SIGNAL(triggered()), SLOT(takeAutomationSnapshot()));
      connect(autoClearAction, SIGNAL(triggered()), SLOT(clearAutomation()));

      //-------- Settings connections
      connect(settingsGlobalAction, SIGNAL(triggered()), SLOT(configGlobalSettings()));
      connect(settingsShortcutsAction, SIGNAL(triggered()), SLOT(configShortCuts()));
      connect(settingsMetronomeAction, SIGNAL(triggered()), SLOT(configMetronome()));
      connect(settingsMidiSyncAction, SIGNAL(triggered()), SLOT(configMidiSync()));
      connect(settingsMidiIOAction, SIGNAL(triggered()), SLOT(configMidiFile()));
      connect(settingsAppearanceAction, SIGNAL(triggered()), SLOT(configAppearance()));
      connect(settingsMidiPortAction, SIGNAL(triggered()), SLOT(configMidiPorts()));

      connect(dontFollowAction, &QAction::triggered, [this]() { cmd(CMD_FOLLOW_NO); } );
      connect(followPageAction, &QAction::triggered, [this]() { cmd(CMD_FOLLOW_JUMP); } );
      connect(followCtsAction,  &QAction::triggered, [this]() { cmd(CMD_FOLLOW_CONTINUOUS); } );
      connect(rewindOnStopAction, SIGNAL(toggled(bool)), SLOT(toggleRewindOnStop(bool)));

      //-------- Help connections
      connect(helpManualAction, SIGNAL(triggered()), SLOT(startHelpBrowser()));
      connect(helpHomepageAction, SIGNAL(triggered()), SLOT(startHomepageBrowser()));
      connect(helpReportAction, SIGNAL(triggered()), SLOT(startBugBrowser()));
      connect(helpDidYouKnow, SIGNAL(triggered()), SLOT(showDidYouKnowDialog()));
      connect(helpAboutAction, SIGNAL(triggered()), SLOT(about()));
      connect(helpSnooperAction, &QAction::triggered, [this]() { startSnooper(); } );

      //--------------------------------------------------
      //    Toolbar
      //--------------------------------------------------

      // when adding a toolbar to the main window, remember adding it to
      // either the requiredToolbars or optionalToolbars list!
      // NOTICE: Please ensure that any tool bar object names here match the names
      //          assigned in the 'toolbar' creation section of TopWin::TopWin(),
      //          or any other TopWin class.
      //         This allows MusE::setCurrentMenuSharingTopwin() to do some magic
      //          to retain the original toolbar layout. If it finds an existing
      //          toolbar with the same object name, it /replaces/ it using insertToolBar(),
      //          instead of /appending/ with addToolBar().

      tools = addToolBar(tr("File Buttons"));
      tools->setObjectName("File Buttons");
      tools->addAction(fileNewAction);
      tools->addAction(fileNewFromTemplateAction);
      tools->addAction(fileOpenAction);
      tools->addAction(fileSaveAction);
      QAction* whatsthis = QWhatsThis::createAction(this);
      whatsthis->setIcon(*whatsthisSVGIcon);
      tools->addAction(whatsthis);

      QToolBar* undoToolbar = addToolBar(tr("Undo/Redo"));
      undoToolbar->setObjectName("Undo/Redo tools");
      undoToolbar->addActions(MusEGlobal::undoRedo->actions());

      QToolBar* panicToolbar = addToolBar(tr("Panic"));
      panicToolbar->setObjectName("Panic tool");
      panicToolbar->addAction(MusEGlobal::panicAction);

      QToolBar* metronomeToolbar = addToolBar(tr("Metronome"));
      metronomeToolbar->setObjectName("Metronome tool");
      metronomeToolbar->addAction(MusEGlobal::metronomeAction);

      // Already has an object name.
      cpuLoadToolbar = new CpuToolbar(tr("Cpu load"), this);
      addToolBar(cpuLoadToolbar);
      connect(cpuLoadToolbar, SIGNAL(resetClicked()), SLOT(resetXrunsCounter()));

      QToolBar* songpos_tb;
      songpos_tb = addToolBar(tr("Song Position"));
      songpos_tb->setObjectName("Song Position tool");
      songpos_tb->addWidget(new MusEGui::SongPosToolbarWidget(songpos_tb));
      songpos_tb->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
      songpos_tb->setContextMenuPolicy(Qt::PreventContextMenu);

      addToolBarBreak();
      
      QToolBar* transportToolbar = addToolBar(tr("Transport"));
      transportToolbar->setObjectName("Transport tool");
      transportToolbar->addActions(MusEGlobal::transportAction->actions());
      transportToolbar->setIconSize(QSize(MusEGlobal::config.iconSize, MusEGlobal::config.iconSize));

      // Already has an object name.
      TempoToolbar* tempo_tb = new TempoToolbar(tr("Tempo"), this);
      addToolBar(tempo_tb);
      
      // Already has an object name.
      SigToolbar* sig_tb = new SigToolbar(tr("Signature"), this);
      addToolBar(sig_tb);
      
      requiredToolbars.push_back(tools);
      requiredToolbars.push_back(cpuLoadToolbar);
      optionalToolbars.push_back(undoToolbar);
      optionalToolbars.push_back(panicToolbar);
      optionalToolbars.push_back(metronomeToolbar);
      optionalToolbars.push_back(songpos_tb);
      optionalToolbars.push_back(NULL);  // Toolbar break
      optionalToolbars.push_back(transportToolbar);
      optionalToolbars.push_back(tempo_tb);
      optionalToolbars.push_back(sig_tb);


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
      menu_file->addAction(fileNewFromTemplateAction);
      menu_file->addAction(fileOpenAction);
      menu_file->addMenu(openRecent);
      menu_file->addSeparator();
      menu_file->addAction(fileSaveAction);
      menu_file->addAction(fileSaveAsAction);
      menu_file->addSeparator();
      menu_file->addAction(fileCloseAction);
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
      menuView->addMenu(master);
//       menuView->addAction(masterGraphicAction);
//       menuView->addAction(masterListAction);
      menuView->addSeparator();
      menuView->addAction(fullscreenAction);


      //---------------------------------------------------
      //  Connect script receiver
      //---------------------------------------------------

      connect(&_scriptReceiver,
              &MusECore::ScriptReceiver::execDeliveredScriptReceived,
              [this](int id) { execDeliveredScript(id); } );
      connect(&_scriptReceiver,
              &MusECore::ScriptReceiver::execUserScriptReceived,
              [this](int id) { execUserScript(id); } );
      
      //-------------------------------------------------------------
      //    popup Midi
      //-------------------------------------------------------------

      menu_functions = new QMenu(tr("&Midi"), this);
      menuBar()->addMenu(menu_functions);
      trailingMenus.push_back(menu_functions);

      MusEGlobal::song->populateScriptMenu(menuScriptPlugins, &_scriptReceiver);
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

      panicPopupMenu->addAction(midiResetInstAction);
      panicPopupMenu->addAction(midiInitInstActions);
      panicPopupMenu->addAction(midiLocalOffAction);
      
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
      menu_audio->addSeparator();
      menu_audio->addAction(autoMixerAction);
      //menu_audio->addSeparator();
      menu_audio->addAction(autoSnapshotAction);
      menu_audio->addAction(autoClearAction);

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

      menuSettings = new QMenu(tr("Se&ttings"), this);
      menuBar()->addMenu(menuSettings);
      trailingMenus.push_back(menuSettings);

      menuSettings->addAction(settingsGlobalAction);
      menuSettings->addAction(settingsAppearanceAction);
      menuSettings->addAction(settingsShortcutsAction);
      menuSettings->addSeparator();
      menuSettings->addMenu(follow);
      follow->addAction(dontFollowAction);
      follow->addAction(followPageAction);
      follow->addAction(followCtsAction);
      menuSettings->addAction(rewindOnStopAction);
      menuSettings->addAction(settingsMetronomeAction);
      menuSettings->addSeparator();
      menuSettings->addAction(settingsMidiSyncAction);
      menuSettings->addAction(settingsMidiIOAction);
      menuSettings->addAction(settingsMidiPortAction);

      //---------------------------------------------------
      //    popup Help
      //---------------------------------------------------

      menu_help = new QMenu(tr("&Help"), this);
      menuBar()->addMenu(menu_help);
      trailingMenus.push_back(menu_help);

      menu_help->addAction(helpManualAction);
      menu_help->addAction(helpHomepageAction);
      menu_help->addAction(helpDidYouKnow);
      menu_help->addSeparator();
      menu_help->addAction(helpReportAction);
      menu_help->addAction(helpSnooperAction);
      menu_help->addSeparator();
      menu_help->addAction(helpAboutAction);

      menu_help->addAction(tr("About &Qt..."), qApp, SLOT(aboutQt()));

      //---------------------------------------------------
      //    Central Widget
      //---------------------------------------------------


      mdiArea=new MuseMdiArea(this);
      mdiArea->setOption(QMdiArea::DontMaximizeSubWindowOnActivation);
      mdiArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
      mdiArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
      mdiArea->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
      setCentralWidget(mdiArea);
      connect(windowsTileAction, SIGNAL(triggered()), this, SLOT(tileSubWindows()));
      connect(windowsRowsAction, SIGNAL(triggered()), this, SLOT(arrangeSubWindowsRows()));
      connect(windowsColumnsAction, SIGNAL(triggered()), this, SLOT(arrangeSubWindowsColumns()));
      connect(windowsCascadeAction, SIGNAL(triggered()), mdiArea, SLOT(cascadeSubWindows()));


      arrangerView = new MusEGui::ArrangerView(this);
      connect(arrangerView, SIGNAL(closed()), SLOT(arrangerClosed()));
      toplevels.push_back(arrangerView);
      arrangerView->hide();
      _arranger=arrangerView->getArranger();

      connect(tempo_tb, SIGNAL(returnPressed()), arrangerView, SLOT(focusCanvas()));
      connect(tempo_tb, SIGNAL(escapePressed()), arrangerView, SLOT(focusCanvas()));
      connect(tempo_tb, SIGNAL(masterTrackChanged(bool)), MusEGlobal::song, SLOT(setMasterFlag(bool)));
      
      connect(sig_tb,   SIGNAL(returnPressed()), arrangerView, SLOT(focusCanvas()));
      connect(sig_tb,   SIGNAL(escapePressed()), arrangerView, SLOT(focusCanvas()));

      //---------------------------------------------------
      //  read list of "Recent Projects"
      //---------------------------------------------------

      QString prjPath(MusEGlobal::configPath);
      prjPath += QString("/projects");
      QFile projFile(prjPath);
      if (!projFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        perror("open projectfile");
        projectRecentList.clear();
      }
      else
      {
        for (int i = 0; i < PROJECT_LIST_LEN; ++i)
        {
          if (projFile.atEnd()) {
            break;
          }
          projectRecentList.append(projFile.readLine().simplified());
        }
      }

      transport = new MusEGui::Transport(this, "transport");
      bigtime   = 0;

      MusEGlobal::song->blockSignals(false);

      QSettings settings;
      restoreGeometry(settings.value("MusE/geometry").toByteArray());

      MusEGlobal::song->update();
      updateWindowMenu();
      }

//---------------------------------------------------------
//   setHeartBeat
//---------------------------------------------------------

void MusE::setHeartBeat()
      {
      if(MusEGlobal::debugMsg)
        fprintf(stderr, "MusE: STARTING Heartbeat timer\n");
      MusEGlobal::heartBeatTimer->start(1000/MusEGlobal::config.guiRefresh);
      }

void MusE::stopHeartBeat()
{
  if(MusEGlobal::debugMsg)
    fprintf(stderr, "MusE: STOPPING Heartbeat timer\n");
  MusEGlobal::heartBeatTimer->stop();
}

void MusE::heartBeat()
{
  cpuLoadToolbar->setValues(MusEGlobal::song->cpuLoad(), 
                            MusEGlobal::song->dspLoad(), 
                            MusEGlobal::song->xRunsCount());
}

void MusE::populateAddTrack()
{
  arrangerView->populateAddTrack();
  arrangerView->updateShortcuts();

}

void MusE::blinkTimerSlot()
{
  MusEGlobal::blinkTimerPhase = !MusEGlobal::blinkTimerPhase;
  emit blinkTimerToggled(MusEGlobal::blinkTimerPhase);
}

void MusE::messagePollTimerSlot()
{
  if(MusEGlobal::song)
    MusEGlobal::song->processIpcInEventBuffers();
}

//---------------------------------------------------------
//   setDirty
//---------------------------------------------------------

void MusE::setDirty()
      {
      MusEGlobal::song->dirty = true;
      setWindowTitle(projectTitle(project.absoluteFilePath()) + " <unsaved changes>");
      }

//---------------------------------------------------
//  loadDefaultSong
//    if no songname entered on command line:
//    startMode: 0  - load last song
//               1  - load default template
//               2  - load configured start song
//---------------------------------------------------

void MusE::loadDefaultSong(const QString& filename_override)
{
  QString name;
  bool useTemplate = false;
  bool loadConfig = true;
  if (!filename_override.isEmpty())
        name = filename_override;
  else if (MusEGlobal::config.startMode == 0) {
              name = !projectRecentList.isEmpty() ? projectRecentList.first() : MusEGui::getUniqueUntitledName();
        fprintf(stderr, "starting with last song %s\n", name.toLatin1().constData());
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
          if (name == "default.med")
              name = MusEGlobal::museGlobalShare + QString("/templates/default.med");
          loadConfig = MusEGlobal::config.startSongLoadConfig;
        }
        useTemplate = true;
        fprintf(stderr, "starting with template %s\n", name.toLatin1().constData());
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
        fprintf(stderr, "starting with pre configured song %s\n", name.toLatin1().constData());
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
#ifdef _WIN32
          switch (rand()%10) {
#else
          switch (random()%10) {
#endif
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
//       progress->setWindowModality(Qt::WindowModal); // REMOVE Tim. Persistent routes. Removed for version warning dialog to take priority. FIXME
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
            // REMOVE Tim. Persistent routes. TESTING.
            //MusEGlobal::audio->msgIdle(true);
            }
      microSleep(100000);
      progress->setValue(10);
      loadProjectFile1(name, songTemplate, doReadMidiPorts);
      microSleep(100000);
      progress->setValue(90);
      if (restartSequencer)
            seqStart();
        // REMOVE Tim. Persistent routes. TESTING.
        //MusEGlobal::audio->msgIdle(false);
      //MusEGlobal::song->connectPorts();

      arrangerView->updateVisibleTracksButtons();
      progress->setValue(100);
      delete progress;
      progress=0;

      QApplication::restoreOverrideCursor();

      // Prompt and send init sequences.
      MusEGlobal::audio->msgInitMidiDevices(false);

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
            mixer1->clearAndDelete();
      if (mixer2)
            mixer2->clearAndDelete();
      _arranger->clear();      // clear track info
      if (clearSong(doReadMidiPorts))  // Allow not touching things like midi ports.
            return;
      progress->setValue(20);

      QFileInfo fi(name);
      if (songTemplate)
      {
            if(!fi.isReadable()) {
                QMessageBox::critical(this, QString("MusE"),
                    tr("Cannot read template"));
                QApplication::restoreOverrideCursor();
                return;
                }
            project.setFile(MusEGui::getUniqueUntitledName());
            MusEGlobal::museProject = MusEGlobal::museProjectInitPath;
            QDir::setCurrent(QDir::homePath());
            }
      else {
            fprintf(stderr, "Setting project path to %s\n", fi.absolutePath().toLocal8Bit().constData());
            MusEGlobal::museProject = fi.absolutePath();
            project.setFile(name);
            QDir::setCurrent(MusEGlobal::museProject);
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

                  if(songTemplate)
                  {
                    // The project is a template. Set the project's sample rate
                    //  to the system rate.
                    // NOTE: A template should never contain anything 'frame' related
                    //        like wave parts and events, or even audio automation graphs !
                    //       That is more under the category of say, 'demo songs'.
                    //       And here is the reason why:
                    MusEGlobal::projectSampleRate = MusEGlobal::sampleRate;
                  }
                  else
                  {
                    MusECore::Xml d_xml(f);
                    MusECore::SongfileDiscovery d_list(MusEGlobal::museProject);
                    d_list.readSongfile(d_xml);

                    // Be kind. Rewind.
                    fseek(f, 0, SEEK_SET);

                    // Is there a project sample rate setting in the song? (Setting added circa 2011).
                    if(d_list._waveList._projectSampleRateValid)
                    {
                      MusEGlobal::projectSampleRate = d_list._waveList._projectSampleRate;
                    }
                    else
                    {
                      int sugg_val;
                      QString sugg_phrase;
                      if(d_list._waveList.empty())
                      {
                        // Suggest the current system sample rate.
                        sugg_val = MusEGlobal::sampleRate;
                        sugg_phrase = 
                        tr("The project has no project sample rate (added 2011).\n"
                           "Please enter a rate. The current system rate (%1Hz)\n"
                           " is suggested, and cancelling uses it:").arg(MusEGlobal::sampleRate);
                      }
                      else
                      {
                        // Suggest the most common sample rate used in the song.
                        sugg_val = d_list._waveList.getMostCommonSamplerate();
                        sugg_phrase =
                        tr("The project has audio waves, but no project sample rate (added 2011).\n"
                           "Please enter a rate. The most common wave rate found is suggested,\n"
                           " the project was probably made with it. Cancelling uses the\n"
                           " current system rate (%1Hz):").arg(MusEGlobal::sampleRate);
                      }

                      bool ok;
                      const int res = QInputDialog::getInt(
                        this, tr("Project sample rate"),
                        sugg_phrase, sugg_val,
                        0, (10 * 1000 * 1000), 1, &ok);

                      if(ok)
                        MusEGlobal::projectSampleRate = res;
                      else
                        MusEGlobal::projectSampleRate = MusEGlobal::sampleRate;
                    }
                  
                    if (!songTemplate && 
                        //MusEGlobal::audioDevice->deviceType() != AudioDevice::DUMMY_AUDIO &&  // Why exclude dummy?
                        MusEGlobal::projectSampleRate != MusEGlobal::sampleRate)
                    {
                      QString msg = QString("The sample rate in this project (%1Hz) and the\n"
                        " current system setting (%2Hz) differ.\n"
                        "Project timing will be scaled to match the new sample rate.\n"
                        "Caution: Slight rounding errors may degrade timing accuracy.\n\n"
                        "Live realtime audio sample rate converters will be enabled\n"
                        " on audio files where required.\n"
                        "The files can be permanently converted to the new sample rate.\n\n"
                        "Save this song if you are sure you didn't mean to open it\n"
                        " at the original sample rate.").arg(MusEGlobal::projectSampleRate).arg(MusEGlobal::sampleRate);
                      QMessageBox::warning(MusEGlobal::muse,"Wrong sample rate", msg);
                      // Automatically convert the project.
                      // No: Try to keep the rate until user tells it to change.
                      //convertProjectSampleRate();
                    }
                  }
                  

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
            setWindowTitle(projectTitle(project.absoluteFilePath()));
            }
      MusEGlobal::song->dirty = false;
      progress->setValue(30);

      viewTransportAction->setChecked(MusEGlobal::config.transportVisible);
      viewBigtimeAction->setChecked(MusEGlobal::config.bigTimeVisible);
      viewMarkerAction->setChecked(MusEGlobal::config.markerVisible);
      viewArrangerAction->setChecked(MusEGlobal::config.arrangerVisible);

      autoMixerAction->setChecked(MusEGlobal::automation);

      showBigtime(MusEGlobal::config.bigTimeVisible);
      
      // NOTICE! Mixers may set their own maximum size according to their content, on SongChanged.
      //         Therefore if the mixer is ALREADY OPEN, it may have a maximum size imposed on it,
      //          which may be SMALLER than any new size we might try to set after this.
      //         So we MUST RESET maximium size now, BEFORE attempts to set size. As per docs:
      if(mixer1)
      {
        mixer1->setMaximumSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);
        mixer1->setGeometry(MusEGlobal::config.mixer1.geometry);
      }
      if(mixer2)
      {
        mixer2->setMaximumSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);
        mixer2->setGeometry(MusEGlobal::config.mixer2.geometry);
      }

      showMixer1(MusEGlobal::config.mixer1Visible);
      showMixer2(MusEGlobal::config.mixer2Visible);

// REMOVE Tim. Removed. Already taken care of by settings. Reinstated! MDI window was
//  not restoring on project reload. Didn't want to have to re-enable this, IIRC there
//  was a problem with using this (interference with other similar competing settings),
//  but here we go... Quick tested OK with normal and 'Borland/Mac' GUI modes.
      resize(MusEGlobal::config.geometryMain.size());
      move(MusEGlobal::config.geometryMain.topLeft());

      if (MusEGlobal::config.transportVisible)
            transport->show();
      transport->move(MusEGlobal::config.geometryTransport.topLeft());
      showTransport(MusEGlobal::config.transportVisible);

      progress->setValue(40);

      transport->setMasterFlag(MusEGlobal::tempomap.masterFlag());
      MusEGlobal::punchinAction->setChecked(MusEGlobal::song->punchin());
      MusEGlobal::punchoutAction->setChecked(MusEGlobal::song->punchout());
      MusEGlobal::loopAction->setChecked(MusEGlobal::song->loop());
      // Inform the rest of the app the song changed, with all flags MINUS
      //  these flags which are already sent in the call to MusE::read() above:
      MusEGlobal::song->update(~SC_TRACK_INSERTED);
      MusEGlobal::song->updatePos();
      arrangerView->clipboardChanged(); // enable/disable "Paste"
      arrangerView->selectionChanged(); // enable/disable "Copy" & "Paste"
      arrangerView->scoreNamingChanged(); // inform the score menus about the new scores and their names
      progress->setValue(50);

      // Moved here from above due to crash with a song loaded and then File->New.
      // Marker view list was not updated, had non-existent items from marker list (cleared in ::clear()).
      showMarker(MusEGlobal::config.markerVisible);
      }

//---------------------------------------------------------
//   fileClose
//---------------------------------------------------------

void MusE::fileClose()
{
  // For now we just don't read the ports, leaving the last setup intact.
  const bool doReadMidiPorts = false;
  
//   if (mixer1)
//         mixer1->clearAndDelete();
//   if (mixer2)
//         mixer2->clearAndDelete();
//   _arranger->clear();      // clear track info
  if(clearSong(doReadMidiPorts))  // Allow not touching things like midi ports.
        return;
  
  //setConfigDefaults();
  QString name(MusEGui::getUniqueUntitledName());
  MusEGlobal::museProject = MusEGlobal::museProjectInitPath;
  //QDir::setCurrent(QDir::homePath());
  QDir::setCurrent(MusEGlobal::museProject);
  project.setFile(name);
  setWindowTitle(projectTitle(name));
  
  //writeTopwinState=true;
  
  MusEGlobal::song->dirty = false;
  
  // Inform the rest of the app the song changed, with all flags.
  MusEGlobal::song->update(SC_EVERYTHING);
  MusEGlobal::song->updatePos();
  arrangerView->clipboardChanged(); // enable/disable "Paste"
  arrangerView->selectionChanged(); // enable/disable "Copy" & "Paste"
  arrangerView->scoreNamingChanged(); // inform the score menus about the new scores and their names
}
      
//---------------------------------------------------------
//   setUntitledProject
//---------------------------------------------------------

void MusE::setUntitledProject()
      {
      setConfigDefaults();
      QString name(MusEGui::getUniqueUntitledName());
      MusEGlobal::museProject = MusEGlobal::museProjectInitPath;
      QDir::setCurrent(QDir::homePath());
      project.setFile(name);
      setWindowTitle(projectTitle(name));
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
            QDir::setCurrent(QFileInfo(fn).absolutePath());
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
//   loadDefaultTemplate
//---------------------------------------------------------

void MusE::loadDefaultTemplate()
{
        loadProjectFile(MusEGlobal::museGlobalShare + QString("/templates/default.med"), true, false);
        setUntitledProject();
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
//       QString backupCommand;

      QFile currentName(name);
      if (QFile::exists(name)) {
            currentName.copy(name+".backup");
            //backupCommand.sprintf("cp \"%s\" \"%s.backup\"", name.toLatin1().constData(), name.toLatin1().constData());
            }
      else if (QFile::exists(name + QString(".med"))) {
            QString currentName2(name+".med");
            currentName.copy(name+".med.backup");
            //backupCommand.sprintf("cp \"%s.med\" \"%s.med.backup\"", name.toLatin1().constData(), name.toLatin1().constData());
            }
//      if (!backupCommand.isEmpty())
//            system(backupCommand.toLatin1().constData());

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
            setWindowTitle(projectTitle(project.absoluteFilePath()));
            saveIncrement = 0;
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
                  if (!save())      // don't quit if save failed
                  {
                        setRestartingApp(false); // Cancel any restart.
                        event->ignore();
                        QApplication::restoreOverrideCursor();
                        return;
                  }
                  }
            else if (n == 2)
            {
                  setRestartingApp(false); // Cancel any restart.
                  event->ignore();
                  QApplication::restoreOverrideCursor();
                  return;
            }
            }
      
      // NOTICE: In the TopWin constructor, recently all top levels were changed to parentless, 
      //  to fix stay-on-top behaviour that seems to have been introduced in Qt5.
      // But now, when the app closes by main mindow for example, all other top win destructors 
      //  are not called. So we must do it here.
      for (MusEGui::iToplevel i = toplevels.begin(); i != toplevels.end(); ++i) 
      {
        TopWin* tw = *i;
        // Top win has no parent? Manually delete it.
        if(!tw->parent())
          delete tw;
      }
              
      seqStop();

      MusECore::WaveTrackList* wt = MusEGlobal::song->waves();
      for (MusECore::iWaveTrack iwt = wt->begin(); iwt != wt->end(); ++iwt) {
            MusECore::WaveTrack* t = *iwt;
            if (t->recFile() && t->recFile()->samples() == 0) {
                  t->recFile()->remove();
                  }
            }

      QSettings settings;
      settings.setValue("MusE/geometry", saveGeometry());

      writeGlobalConfiguration();

      // save "Open Recent" list
      QString prjPath(MusEGlobal::configPath);
      prjPath += "/projects";
      QFile f(prjPath);
      f.open(QIODevice::WriteOnly | QIODevice::Text);
      if (f.exists()) {
        QTextStream out(&f);
        for (int i = 0; i < projectRecentList.size(); ++i) {
           out << projectRecentList[i] << "\n";
        }
      }
      if(MusEGlobal::debugMsg)
        fprintf(stderr, "MusE: Exiting JackAudio\n");
      MusECore::exitJackAudio();
      if(MusEGlobal::debugMsg)
        fprintf(stderr, "MusE: Exiting DummyAudio\n");
      MusECore::exitDummyAudio();
#ifdef HAVE_RTAUDIO
      if(MusEGlobal::debugMsg)
        fprintf(stderr, "MusE: Exiting RtAudio\n");
      MusECore::exitRtAudio();
#endif
      if(MusEGlobal::debugMsg)
        fprintf(stderr, "MusE: Exiting Metronome\n");
      MusECore::exitMetronome();

      MusEGlobal::song->cleanupForQuit();

      // Give midi devices a chance to close first, above in cleanupForQuit.
      if(MusEGlobal::debugMsg)
        fprintf(stderr, "Muse: Exiting ALSA midi\n");
      MusECore::exitMidiAlsa();

      if(MusEGlobal::debugMsg)
        fprintf(stderr, "Muse: Cleaning up temporary wavefiles + peakfiles\n");
      // Cleanup temporary wavefiles + peakfiles used for undo
      for (std::list<QString>::iterator i = MusECore::temporaryWavFiles.begin(); i != MusECore::temporaryWavFiles.end(); i++) {
            QString filename = *i;
            QFileInfo f(filename);
            QDir d = f.dir();
            d.remove(filename);
            d.remove(f.completeBaseName() + ".wca");
            }

      if(MusEGlobal::usePythonBridge)
      {
        fprintf(stderr, "Stopping MusE Pybridge...\n");
        if(stopPythonBridge() == false)
          fprintf(stderr, "MusE: Could not stop Python bridge\n");
        else
          fprintf(stderr, "MusE: Pybridge stopped\n");
      }

#ifdef HAVE_LASH
      // Disconnect gracefully from LASH.
      if(lash_client)
      {
        if(MusEGlobal::debugMsg)
          fprintf(stderr, "MusE: Disconnecting from LASH\n");
        lash_event_t* lashev = lash_event_new_with_type (LASH_Quit);
        lash_send_event(lash_client, lashev);
      }
#endif

      if(MusEGlobal::debugMsg)
        fprintf(stderr, "MusE: Exiting Dsp\n");
      AL::exitDsp();

      if(MusEGlobal::debugMsg)
        fprintf(stderr, "MusE: Exiting OSC\n");
      MusECore::exitOSC();

      delete MusEGlobal::audioPrefetch;
      delete MusEGlobal::audio;

      // Destroy the sequencer object if it exists.
      MusECore::exitMidiSequencer();

      delete MusEGlobal::song;

      if(MusEGlobal::debugMsg)
        fprintf(stderr, "MusE: Deleting icons\n");
      deleteIcons();

      if(MusEGlobal::debugMsg)
        fprintf(stderr, "MusE: Deleting all parentless dialogs and widgets\n");
      deleteParentlessDialogs();

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
            toplevels.push_back(markerView);
            }
      if(markerView->isVisible() != flag)
        markerView->setVisible(flag);
      if(viewMarkerAction->isChecked() != flag)
        viewMarkerAction->setChecked(flag);   // ??? TEST: Recursion? Does this call toggleMarker if called from menu?  No. Why? It should. REMOVE Tim. Or keep.
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
      if(viewMarkerAction->isChecked())
        viewMarkerAction->setChecked(false); // ??? TEST: Recursion? Does this call toggleMarker? Yes. REMOVE Tim. Or keep.
      if (currentMenuSharingTopwin == markerView)
        setCurrentMenuSharingTopwin(NULL);

      updateWindowMenu();

      // focus the last activated topwin which is not the marker view
      QList<QMdiSubWindow*> l = mdiArea->subWindowList(QMdiArea::StackingOrder);
      for (QList<QMdiSubWindow*>::iterator lit=l.begin(); lit!=l.end(); lit++)
        if ((*lit)->isVisible() && (*lit)->widget() != markerView)
        {
          if (MusEGlobal::debugMsg)
            fprintf(stderr, "bringing '%s' to front instead of closed marker window\n",(*lit)->widget()->windowTitle().toLatin1().data());

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
      if(arrangerView->isVisible() != flag)
        arrangerView->setVisible(flag);
      if(viewArrangerAction->isChecked() != flag)
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
      if(viewArrangerAction->isChecked())
        viewArrangerAction->setChecked(false);
      updateWindowMenu();

      // focus the last activated topwin which is not the arranger view
      QList<QMdiSubWindow*> l = mdiArea->subWindowList(QMdiArea::StackingOrder);
      for (QList<QMdiSubWindow*>::iterator lit=l.begin(); lit!=l.end(); lit++)
        if ((*lit)->isVisible() && (*lit)->widget() != arrangerView)
        {
          if (MusEGlobal::debugMsg)
            fprintf(stderr, "bringing '%s' to front instead of closed arranger window\n",(*lit)->widget()->windowTitle().toLatin1().data());

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
      if(transport->isVisible() != flag)
        transport->setVisible(flag);
      if(viewTransportAction->isChecked() != flag)
         viewTransportAction->setChecked(flag);
}

#ifdef _WIN32
static float CalculateCPULoad(unsigned long long idleTicks, unsigned long long totalTicks)
{
   static unsigned long long _previousTotalTicks = 0;
   static unsigned long long _previousIdleTicks = 0;

   unsigned long long totalTicksSinceLastTime = totalTicks-_previousTotalTicks;
   unsigned long long idleTicksSinceLastTime  = idleTicks-_previousIdleTicks;

   float ret = 1.0f-((totalTicksSinceLastTime > 0) ? ((float)idleTicksSinceLastTime)/totalTicksSinceLastTime : 0);

   _previousTotalTicks = totalTicks;
   _previousIdleTicks  = idleTicks;
   return ret;
}

static unsigned long long FileTimeToInt64(const FILETIME & ft)
{
   return (((unsigned long long)(ft.dwHighDateTime))<<32)|((unsigned long long)ft.dwLowDateTime);
}
#endif

float MusE::getCPULoad()
{
#ifdef _WIN32
   FILETIME idleTime, kernelTime, userTime;
   return GetSystemTimes(&idleTime, &kernelTime, &userTime) ? CalculateCPULoad(FileTimeToInt64(idleTime), FileTimeToInt64(kernelTime)+FileTimeToInt64(userTime)) : -1.0f;
#else
    struct rusage ru;
    struct timespec curSysTime;
    if(clock_gettime(CLOCK_REALTIME, &curSysTime) != 0)
    {
       return 0.0f;
    }
    //float fLoad = 0.0f;
    if(getrusage(RUSAGE_SELF, &ru) != 0)
    {
        return 0.0f;
    }
    long msSysElapsed = (curSysTime.tv_nsec / 1000000L) + curSysTime.tv_sec * 1000L;
    msSysElapsed -= (lastSysTime.tv_nsec / 1000000L) + lastSysTime.tv_sec * 1000L;
    long msCpuElasped = (ru.ru_utime.tv_usec / 1000L) + ru.ru_utime.tv_sec * 1000L;
    msCpuElasped -= (lastCpuTime.tv_usec / 1000L) + lastCpuTime.tv_sec * 1000L;
    if(msSysElapsed > 0)
    {
        fAvrCpuLoad += (float)((double)msCpuElasped / (double)msSysElapsed);
        avrCpuLoadCounter++;
    }
    lastCpuTime = ru.ru_utime;
    lastSysTime = curSysTime;
    if(avrCpuLoadCounter > 10)
    {
        fCurCpuLoad = (fAvrCpuLoad / (float)avrCpuLoadCounter) * 100.0f;
        fAvrCpuLoad = 0.0f;
        avrCpuLoadCounter = 0;
    }

    return fCurCpuLoad;
#endif
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
                  setWindowTitle(projectTitle(project.absoluteFilePath()));
                  addProject(name);
                  }
            else
                  MusEGlobal::museProject = tempOldProj;

            QDir::setCurrent(MusEGlobal::museProject);
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
            case MusECore::Track::NEW_DRUM: startDrumEditor(); break;
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
      MusECore::PartList* pl = MusECore::getSelectedMidiParts();
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
      MusEGui::PianoRoll* pianoroll = new MusEGui::PianoRoll(pl, this, nullptr, _arranger->cursorValue(), showDefaultCtrls);
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
      MusEGui::ListEdit* listEditor = new MusEGui::ListEdit(pl, this);
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
      MusEGui::MasterEdit* masterEditor = new MusEGui::MasterEdit(this);
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
      MusEGui::LMaster* lmaster = new MusEGui::LMaster(this);
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
      MusEGui::DrumEdit* drumEditor = new MusEGui::DrumEdit(pl, this, 0, _arranger->cursorValue(), showDefaultCtrls);
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
      MusECore::PartList* pl = MusECore::getSelectedWaveParts();
      if (pl->empty()) {
            QMessageBox::critical(this, QString("MusE"), tr("Nothing to edit"));
            return;
            }
      startWaveEditor(pl);
      }

void MusE::startWaveEditor(MusECore::PartList* pl)
      {
      MusEGui::WaveEdit* waveEditor = new MusEGui::WaveEdit(pl, this);
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


void MusE::showDidYouKnowDialogIfEnabled()
{
    if ((bool)MusEGlobal::config.showDidYouKnow == true) {
        showDidYouKnowDialog();
    }
}
//---------------------------------------------------------
//   showDidYouKnowDialog
//---------------------------------------------------------
void MusE::showDidYouKnowDialog()
{
    MusEGui::DidYouKnowWidget didYouKnow;

    QFile file(MusEGlobal::museGlobalShare + "/didyouknow.txt");
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
      fprintf(stderr, "could not open didyouknow.txt!\n");
      return;
    }

    // All tips are separated by an empty line. Lines starting with # are ignored
    QString tipMessage = "";
    while (!file.atEnd())  {
      QString line = file.readLine();

      if (!line.simplified().isEmpty() && line.at(0) != QChar('#'))
        tipMessage.append(line);

      if (!tipMessage.isEmpty() && line.simplified().isEmpty()) {
        didYouKnow.tipList.append(tipMessage);
        tipMessage="";
      }
    }
    if (!tipMessage.isEmpty()) {
      didYouKnow.tipList.append(tipMessage);
    }

    std::random_shuffle(didYouKnow.tipList.begin(),didYouKnow.tipList.end());

    didYouKnow.show();
    if( didYouKnow.exec()) {
          if (didYouKnow.dontShowCheckBox->isChecked()) {
                MusEGlobal::config.showDidYouKnow=false;
                // Save settings. Use simple version - do NOT set style or stylesheet, this has nothing to do with that.
                MusEGlobal::muse->changeConfig(true);    // save settings
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
  for (int i = 0; i < projectRecentList.size(); ++i)
  {
    if (!QFileInfo(projectRecentList[i]).exists())
        continue;

    QString fileName = QFileInfo(projectRecentList[i]).fileName();
    QAction *act = openRecent->addAction(fileName);
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
      if (id > projectRecentList.size()-1)
      {
        fprintf(stderr, "THIS SHOULD NEVER HAPPEN: id(%i) < PROJECT_LIST_LEN(%i) in MusE::selectProject!\n",id, PROJECT_LIST_LEN);
        return;
      }
      QString name = projectRecentList[id];
      if (name == "")
            return;
      loadProjectFile(name, false, true);
      }

//---------------------------------------------------------
//   toplevelDeleting
//---------------------------------------------------------

void MusE::toplevelDeleting(MusEGui::TopWin* tl)
      {
      for (MusEGui::iToplevel i = toplevels.begin(); i != toplevels.end(); ++i) {
            if (*i == tl) {

                  tl->storeInitialState();
               
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
                          fprintf(stderr, "bringing '%s' to front instead of closed window\n",(*lit)->widget()->windowTitle().toLatin1().data());

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
      fprintf(stderr, "topLevelDeleting: top level %p not found\n", tl);
      }

//---------------------------------------------------------
//   kbAccel
//---------------------------------------------------------

void MusE::kbAccel(int key)
      {
      if (key == MusEGui::shortcuts[MusEGui::SHRT_TOGGLE_METRO].key) {
            MusEGlobal::song->setClick(!MusEGlobal::song->click());
            }
      else if (key == MusEGui::shortcuts[MusEGui::SHRT_REC_RESTART].key) {
         MusEGlobal::song->restartRecording();
      }
      else if (key == MusEGui::shortcuts[MusEGui::SHRT_REC_RESTART_MULTI].key) {
         MusEGlobal::song->restartRecording(false);
      }
      else if (key == MusEGui::shortcuts[MusEGui::SHRT_PLAY_TOGGLE].key) {

            if (MusEGlobal::audio->isPlaying())
                  MusEGlobal::song->setStop(true);
            else if (!MusEGlobal::config.useOldStyleStopShortCut)
                  MusEGlobal::song->setPlay(true);
            else if (MusEGlobal::song->cpos() != MusEGlobal::song->lpos())
                  MusEGlobal::song->setPos(MusECore::Song::CPOS, MusEGlobal::song->lPos());
            else {
                  MusECore::Pos p(0, true);
                  MusEGlobal::song->setPos(MusECore::Song::CPOS, p);
                  }
            }
      else if (key == MusEGui::shortcuts[MusEGui::SHRT_STOP].key) {
            MusEGlobal::song->setStop(true);
            }
      else if (key == MusEGui::shortcuts[MusEGui::SHRT_GOTO_END].key) {
            MusECore::Pos p(MusEGlobal::song->len(), true);
            MusEGlobal::song->setPos(MusECore::Song::CPOS, p);
            }
      else if (key == MusEGui::shortcuts[MusEGui::SHRT_GOTO_START].key) {
            MusECore::Pos p(0, true);
            MusEGlobal::song->setPos(MusECore::Song::CPOS, p);
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
              spos = MusEGlobal::sigmap.raster1(spos, MusEGlobal::song->arrangerRaster());
            }
            if(spos < 0)
              spos = 0;
            MusECore::Pos p(spos,true);
            MusEGlobal::song->setPos(MusECore::Song::CPOS, p, true, true, true);
            return;
            }
      else if (key == MusEGui::shortcuts[MusEGui::SHRT_POS_INC].key) {
            int spos = MusEGlobal::sigmap.raster2(MusEGlobal::song->cpos() + 1, MusEGlobal::song->arrangerRaster());    // Nudge by +1, then snap up with raster2.
            MusECore::Pos p(spos,true);
            MusEGlobal::song->setPos(MusECore::Song::CPOS, p, true, true, true); //CDW
            return;
            }
      else if (key == MusEGui::shortcuts[MusEGui::SHRT_POS_DEC_NOSNAP].key) {
            int spos = MusEGlobal::song->cpos() - MusEGlobal::sigmap.rasterStep(MusEGlobal::song->cpos(), MusEGlobal::song->arrangerRaster());
            if(spos < 0)
              spos = 0;
            MusECore::Pos p(spos,true);
            MusEGlobal::song->setPos(MusECore::Song::CPOS, p, true, true, true);
            return;
            }
      else if (key == MusEGui::shortcuts[MusEGui::SHRT_POS_INC_NOSNAP].key) {
            MusECore::Pos p(MusEGlobal::song->cpos() + MusEGlobal::sigmap.rasterStep(MusEGlobal::song->cpos(), MusEGlobal::song->arrangerRaster()), true);
            MusEGlobal::song->setPos(MusECore::Song::CPOS, p, true, true, true);
            return;
            }
      else if (key == MusEGui::shortcuts[MusEGui::SHRT_REC_ARM_TRACK].key) {
            if (!MusEGlobal::song->record())
                toggleTrackArmSelectedTrack();
            }

      else if (key == MusEGui::shortcuts[MusEGui::SHRT_GOTO_LEFT].key) {
            if (!MusEGlobal::song->record())
                  MusEGlobal::song->setPos(MusECore::Song::CPOS, MusEGlobal::song->lPos());
            }
      else if (key == MusEGui::shortcuts[MusEGui::SHRT_GOTO_RIGHT].key) {
            if (!MusEGlobal::song->record())
                  MusEGlobal::song->setPos(MusECore::Song::CPOS, MusEGlobal::song->rPos());
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
      else if (key == MusEGui::shortcuts[MusEGui::SHRT_OPEN_MARKER].key) {
            toggleMarker(!viewMarkerAction->isChecked());
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
      else if (key == MusEGui::shortcuts[MusEGui::SHRT_CONFIG_SHORTCUTS].key) {
            configShortCuts();
            }
      else if (key == MusEGui::shortcuts[MusEGui::SHRT_PART_NORMALIZE].key) {
            MusEGlobal::song->normalizeWaveParts();
            }
      else {
            if (MusEGlobal::debugMsg)
                  fprintf(stderr, "unknown kbAccel 0x%x\n", key);
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
            fprintf(stderr, "MusE: signal %d caught\n", sig);
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
//   deleteParentlessDialogs
//   All these dialogs and/or widgets have no parent,
//    but are not considered MusE 'top-level', so could not
//    be handled via the top-levels list...
//---------------------------------------------------------

void MusE::deleteParentlessDialogs()
{
// Appearance is already a child of MusE !!!
//   if(appearance)
//   {
//     delete appearance;
//     appearance = 0;
//   }
  if(_snooperDialog)
  {
    delete _snooperDialog;
    _snooperDialog = 0;
  }
  if(metronomeConfig)
  {
    delete metronomeConfig;
    metronomeConfig = 0;
  }
  if(shortcutConfig)
  {
    delete shortcutConfig;
    shortcutConfig = 0;
  }
  if(midiSyncConfig)
  {
    delete midiSyncConfig;
    midiSyncConfig = 0;
  }
  if(midiFileConfig)
  {
    delete midiFileConfig;
    midiFileConfig = 0;
  }
  if(globalSettingsConfig)
  {
    delete globalSettingsConfig;
    globalSettingsConfig = 0;
  }

  destroy_function_dialogs();


  if(MusEGlobal::mitPluginTranspose)
  {
    delete MusEGlobal::mitPluginTranspose;
    MusEGlobal::mitPluginTranspose = 0;
  }

  if(midiInputTransform)
  {
    delete midiInputTransform;
    midiInputTransform = 0;
  }
  if(midiFilterConfig)
  {
     delete midiFilterConfig;
     midiFilterConfig = 0;
  }
  if(midiRemoteConfig)
  {
    delete midiRemoteConfig;
    midiRemoteConfig = 0;
  }
#ifdef BUILD_EXPERIMENTAL
  if(midiRhythmGenerator)
  {
    delete midiRhythmGenerator;
    midiRhythmGenerator = 0;
  }
#endif
  if(midiTransformerDialog)
  {
    delete midiTransformerDialog;
    midiTransformerDialog = 0;
  }
  if(routeDialog)
  {
    delete routeDialog;
    routeDialog = 0;
  }

}

//---------------------------------------------------------
//   configAppearance
//---------------------------------------------------------

void MusE::configAppearance()
      {
      if (!appearance)
            // NOTE: For deleting parentless dialogs and widgets, please add them to MusE::deleteParentlessDialogs().
            appearance = new MusEGui::Appearance(_arranger, this);
      appearance->resetValues();
      if(appearance->isVisible()) {
          appearance->raise();
          appearance->activateWindow();
          }
      else
          appearance->show();
      }

//---------------------------------------------------------
//   startSnooper
//---------------------------------------------------------

void MusE::startSnooper()
      {
      if (!_snooperDialog)
            // NOTE: For deleting parentless dialogs and widgets, please add them to MusE::deleteParentlessDialogs().
            _snooperDialog = new MusEGui::SnooperDialog();
      //_snooperDialog->resetValues();
      if(_snooperDialog->isVisible()) {
          _snooperDialog->raise();
          _snooperDialog->activateWindow();
          }
      else
          _snooperDialog->show();
      }

//---------------------------------------------------------
//   changeConfig
//    - called whenever configuration has changed
//    - when configuration has changed by user, call with
//      writeFlag=true to save configuration in ~/.MusE
//      simple=true Don't bother with theme, style, 
//       and font etc. updates, just emit the configChanged signal.
//---------------------------------------------------------

void MusE::changeConfig(bool writeFlag)
      {
      if (writeFlag)
            writeGlobalConfiguration();
      updateConfiguration();
      emit configChanged();
      }

//---------------------------------------------------------
//   configMetronome
//---------------------------------------------------------

void MusE::configMetronome()
      {
      if (!metronomeConfig)
      {
          // NOTE: For deleting parentless dialogs and widgets, please add them to MusE::deleteParentlessDialogs().
          metronomeConfig = new MusEGui::MetronomeConfig;
          metronomeConfig->show();
          return;
      }

      if(metronomeConfig->isVisible()) {
          metronomeConfig->raise();
          metronomeConfig->activateWindow();
          }
      else
      {
          metronomeConfig->updateValues();
          metronomeConfig->show();
      }
      }


//---------------------------------------------------------
//   configShortCuts
//---------------------------------------------------------

void MusE::configShortCuts()
      {
      if (!shortcutConfig)
      {
            // NOTE: For deleting parentless dialogs and widgets, please add them to MusE::deleteParentlessDialogs().
            shortcutConfig = new MusEGui::ShortcutConfig();
            connect(shortcutConfig, SIGNAL(saveConfig()), SLOT(configShortCutsSaveConfig()));
      }
      if(shortcutConfig->isVisible()) {
          shortcutConfig->raise();
          shortcutConfig->activateWindow();
          }
      else
          shortcutConfig->show();
      }

//---------------------------------------------------------
//   configShortCutsSaveConfig
//---------------------------------------------------------

void MusE::configShortCutsSaveConfig()
      {
      // Save settings. Use simple version - do NOT set style or stylesheet, this has nothing to do with that.
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
      MusEGlobal::song->bounceTrack = nullptr;

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

      // Switch all wave converters to offline settings mode.
      MusEGlobal::song->setAudioConvertersOfflineOperation(true);

      // This will wait a few cycles until freewheel is set and a seek is done.
      MusEGlobal::audio->msgBounce();
      MusEGlobal::song->bounceOutput = out;
      MusEGlobal::song->bounceTrack = track;
      MusEGlobal::song->setRecord(true);
      MusEGlobal::song->setRecordFlag(track, true);
      track->prepareRecording();
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
      MusEGlobal::song->bounceTrack = nullptr;
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

      // Switch all wave converters to offline settings mode.
      MusEGlobal::song->setAudioConvertersOfflineOperation(true);

      // This will wait a few cycles until freewheel is set and a seek is done.
      MusEGlobal::audio->msgBounce();
      MusEGlobal::song->bounceOutput = ao;
      ao->setRecFile(sf);
      if(MusEGlobal::debugMsg)
        fprintf(stderr, "ao->setRecFile %p\n", sf);
      MusEGlobal::song->setRecord(true, false);
      MusEGlobal::song->setRecordFlag(ao, true);
      ao->prepareRecording();
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
               tr("Set left/right marker for bounce range")
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
          int ok = save (ss.toLatin1(), false, true);
          if (ok) {
            project.setFile(ss.toLatin1());
            setWindowTitle(tr("MusE: Song: %1").arg(MusEGui::projectTitleFromFilename(project.absoluteFilePath())));
            addProject(ss.toLatin1());
            MusEGlobal::museProject = QFileInfo(ss.toLatin1()).absolutePath();
            QDir::setCurrent(MusEGlobal::museProject);
          }
          lash_send_event (lash_client, event);
        }
        break;

        case LASH_Restore_File:
        {
          /* load file */
          QString sr = QString(lash_event_get_string(event)) + QString("/lash-project-muse.med");
          loadProjectFile(sr.toLatin1(), false, true);
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
                        fprintf(stderr, "InternalError: gibt %d\n", n);
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
                            fprintf(stderr, "MusE::clearSong TopWin did not close!\n");
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

void MusE::startEditInstrument(const QString& find_instrument, EditInstrumentTabType show_tab)
    {
      if(editInstrument == 0)
      {
            editInstrument = new MusEGui::EditInstrument(this);
            editInstrument->show();
            editInstrument->findInstrument(find_instrument);
            editInstrument->showTab(show_tab);
      }
      else
      {
        if(! editInstrument->isHidden())
          editInstrument->hide();
        else
        {
          editInstrument->show();
          editInstrument->findInstrument(find_instrument);
          editInstrument->showTab(show_tab);
        }
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
              if(track->automationType() != MusECore::AUTO_OFF) // && track->automationType() != MusECore::AUTO_WRITE)
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
            if(track->automationType() != MusECore::AUTO_OFF) // && track->automationType() != MusECore::AUTO_WRITE)
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
      fileNewFromTemplateAction->setShortcut(MusEGui::shortcuts[MusEGui::SHRT_NEW_FROM_TEMPLATE].key);
      fileSaveAction->setShortcut(MusEGui::shortcuts[MusEGui::SHRT_SAVE].key);
      fileSaveAsAction->setShortcut(MusEGui::shortcuts[MusEGui::SHRT_SAVE_AS].key);

      //menu_file->setShortcut(MusEGui::shortcuts[MusEGui::SHRT_OPEN_RECENT].key, menu_ids[CMD_OPEN_RECENT]);    // Not used.
      fileImportMidiAction->setShortcut(MusEGui::shortcuts[MusEGui::SHRT_IMPORT_MIDI].key);
      fileExportMidiAction->setShortcut(MusEGui::shortcuts[MusEGui::SHRT_EXPORT_MIDI].key);
      fileImportPartAction->setShortcut(MusEGui::shortcuts[MusEGui::SHRT_IMPORT_PART].key);
      fileImportWaveAction->setShortcut(MusEGui::shortcuts[MusEGui::SHRT_IMPORT_AUDIO].key);
      quitAction->setShortcut(MusEGui::shortcuts[MusEGui::SHRT_QUIT].key);

      //menu_file->setShortcut(MusEGui::shortcuts[MusEGui::SHRT_LOAD_TEMPLATE].key, menu_ids[CMD_LOAD_TEMPLATE]);  // Not used.

      if(MusEGlobal::undoAction)
        MusEGlobal::undoAction->setShortcut(MusEGui::shortcuts[MusEGui::SHRT_UNDO].key);
      if(MusEGlobal::redoAction)
        MusEGlobal::redoAction->setShortcut(MusEGui::shortcuts[MusEGui::SHRT_REDO].key);


      //editSongInfoAction has no acceleration

      viewTransportAction->setShortcut(MusEGui::shortcuts[MusEGui::SHRT_OPEN_TRANSPORT].key);
      viewBigtimeAction->setShortcut(MusEGui::shortcuts[MusEGui::SHRT_OPEN_BIGTIME].key);
      viewMixerAAction->setShortcut(MusEGui::shortcuts[MusEGui::SHRT_OPEN_MIXER].key);
      viewMixerBAction->setShortcut(MusEGui::shortcuts[MusEGui::SHRT_OPEN_MIXER2].key);
      //viewCliplistAction has no acceleration
      masterGraphicAction->setShortcut(shortcuts[MusEGui::SHRT_OPEN_GRAPHIC_MASTER].key);
      masterListAction->setShortcut(shortcuts[MusEGui::SHRT_OPEN_LIST_MASTER].key);
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
      //settingsShortcutsAction->setShortcut(MusEGui::shortcuts[MusEGui::SHRT_CONFIG_SHORTCUTS].key); // This is global now, handled in MusE::kbAccel
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
      rewindOnStopAction->setShortcut(MusEGui::shortcuts[MusEGui::SHRT_TOGGLE_REWINDONSTOP].key);

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
            connect(MusEGlobal::song, SIGNAL(songChanged(MusECore::SongChangedStruct_t)), bigtime, SLOT(songChanged(MusECore::SongChangedStruct_t)));
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
            mixer1 = new MusEGui::AudioMixerApp(NULL, &(MusEGlobal::config.mixer1));
            connect(mixer1, SIGNAL(closed()), SLOT(mixer1Closed()));
            mixer1->setGeometry(MusEGlobal::config.mixer1.geometry);
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
            mixer2 = new MusEGui::AudioMixerApp(NULL, &(MusEGlobal::config.mixer2));
            connect(mixer2, SIGNAL(closed()), SLOT(mixer2Closed()));
            mixer2->setGeometry(MusEGlobal::config.mixer2.geometry);
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
      MusEGlobal::song->executeScript(this, MusEGlobal::song->getScriptPath(id, true).toLatin1().constData(), MusECore::getSelectedParts(), 0, false); // TODO: get quant from arranger
}

//---------------------------------------------------------
//   execUserScript
//---------------------------------------------------------
void MusE::execUserScript(int id)
{
      MusEGlobal::song->executeScript(this, MusEGlobal::song->getScriptPath(id, false).toLatin1().constData(), MusECore::getSelectedParts(), 0, false); // TODO: get quant from arranger
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
    fprintf(stderr, "\n");
    fprintf(stderr, "focusChanged: old:%p now:%p activeWindow:%p\n", old, now, qApp->activeWindow());
    if(old)
      fprintf(stderr, " old type: %s\n", typeid(*old).name());
    if(now)
      fprintf(stderr, " now type: %s\n", typeid(*now).name());
    if (dynamic_cast<QMdiSubWindow*>(now)!=0)
    {
      QWidget* tmp=dynamic_cast<QMdiSubWindow*>(now)->widget();
      if (tmp)
        fprintf(stderr, "  subwin contains %p which is a %s\n", tmp, typeid(*tmp).name());
      else
        fprintf(stderr, "  subwin contains NULL\n");
    }
    if(qApp->activeWindow())
    {
       const char *strTid = typeid(qApp->activeWindow()).name();
       fprintf(stderr, " activeWindow type: %s\n", strTid);
    }
    fprintf(stderr, "\n");
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
  {
    if(MusEGlobal::heavyDebugMsg)
      fprintf(stderr, " activeTopWin: %s\n", typeid(*activeTopWin).name());
    activeTopWin->storeInitialState();
  }

  if (currentMenuSharingTopwin && (currentMenuSharingTopwin!=activeTopWin))
  {
    if(MusEGlobal::heavyDebugMsg)
      fprintf(stderr, " currentMenuSharingTopwin: %s\n", typeid(*currentMenuSharingTopwin).name());
    currentMenuSharingTopwin->storeInitialState();
  }

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
      fprintf(stderr, "focusChanged: at widget %p with type %s\n",ptr, typeid(*ptr).name());

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
  if (MusEGlobal::debugMsg) fprintf(stderr, "ACTIVE TOPWIN CHANGED to '%s' (%p)\n", win ? win->windowTitle().toLatin1().data() : "<None>", win);

  if ( (win && (win->isMdiWin()==false) && win->sharesToolsAndMenu()) &&
       ( (mdiArea->currentSubWindow() != NULL) && (mdiArea->currentSubWindow()->isVisible()==true) ) )
  {
    if (MusEGlobal::debugMsg) fprintf(stderr, "  that's a menu sharing muse window which isn't inside the MDI area.\n");
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
    fprintf(stderr, "WARNING: THIS SHOULD NEVER HAPPEN: MusE::setCurrentMenuSharingTopwin() called with a win which does not share (%s)! ignoring...\n", win->windowTitle().toLatin1().data());
    return;
  }

  if (win!=currentMenuSharingTopwin)
  {
    MusEGui::TopWin* previousMenuSharingTopwin = currentMenuSharingTopwin;
    currentMenuSharingTopwin = NULL;

    if (MusEGlobal::debugMsg) fprintf(stderr, "MENU SHARING TOPWIN CHANGED to '%s' (%p)\n", win ? win->windowTitle().toLatin1().data() : "<None>", win);

    
    list<QToolBar*> add_toolbars;
    if(win)
      add_toolbars = win->toolbars();
    
    // empty our toolbars
    if (previousMenuSharingTopwin)
    {
      list<QToolBar*> add_foreign_toolbars;
      for (list<QToolBar*>::iterator it = foreignToolbars.begin(); it!=foreignToolbars.end(); ++it)
      {
        QToolBar* tb = *it;
        if(tb)
        {
          // Check for existing toolbar with same object name, and replace it.
          bool found = false;
          for(list<QToolBar*>::iterator i_atb = add_toolbars.begin(); i_atb!=add_toolbars.end(); ++i_atb)
          {
            QToolBar* atb = *i_atb;
            if(atb)
            {
              if(tb->objectName() == atb->objectName())
              {
                //tb->hide();
                
                if(MusEGlobal::heavyDebugMsg) 
                  fprintf(stderr, "  inserting toolbar '%s'\n", atb->windowTitle().toLatin1().data());

                found = true;
                insertToolBar(tb, atb);
                add_foreign_toolbars.push_back(atb);
                add_toolbars.remove(atb);
                atb->show();
                break;
              }
            }
          }
          
          // Remove any toolbar break that may exist before the toolbar - unless there 
          //  is a replacement is to be made, in which case leave the break intact.
          if(!found && toolBarBreak(tb))
          {
            if(MusEGlobal::heavyDebugMsg)
              fprintf(stderr, "  removing break before sharer's toolbar '%s'\n", tb->windowTitle().toLatin1().data());
            removeToolBarBreak(tb);
          }
          
          
          if(MusEGlobal::heavyDebugMsg) 
            fprintf(stderr, "  removing sharer's toolbar '%s'\n", tb->windowTitle().toLatin1().data());
          removeToolBar(tb); // this does not delete *it, which is good
          tb->setParent(NULL);
        }
      }
        
      foreignToolbars = add_foreign_toolbars;
      
    }
    else
    {
      for (list<QToolBar*>::iterator it = optionalToolbars.begin(); it!=optionalToolbars.end(); ++it)
      {
        QToolBar* tb = *it;
        if(tb)
        {
          // Check for existing toolbar with same object name, and replace it.
          for(list<QToolBar*>::iterator i_atb = add_toolbars.begin(); i_atb!=add_toolbars.end(); ++i_atb)
          {
            QToolBar* atb = *i_atb;
            if(atb)
            {
              if(tb->objectName() == atb->objectName())
              {
                //tb->hide();
                
                if(MusEGlobal::heavyDebugMsg) 
                  fprintf(stderr, "  inserting toolbar '%s'\n", atb->windowTitle().toLatin1().data());

                insertToolBar(tb, atb);
                foreignToolbars.push_back(atb);
                add_toolbars.remove(atb);
                atb->show();
                break;
              }
            }
          }
          
          if (MusEGlobal::heavyDebugMsg) 
            fprintf(stderr, "  removing optional toolbar '%s'\n", tb->windowTitle().toLatin1().data());
          removeToolBar(tb); // this does not delete *it, which is good
          tb->setParent(NULL);
        }
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
        if (MusEGlobal::heavyDebugMsg) fprintf(stderr, "  adding menu entry '%s'\n", (*it)->text().toLatin1().data());

        menuBar()->addAction(*it);
      }

      for (list<QToolBar*>::const_iterator it=add_toolbars.begin(); it!=add_toolbars.end(); ++it)
        if (*it)
        {
          if (MusEGlobal::heavyDebugMsg) fprintf(stderr, "  adding toolbar '%s'\n", (*it)->windowTitle().toLatin1().data());

          addToolBar(*it);
          foreignToolbars.push_back(*it);
          (*it)->show();
        }
        else
        {
          if (MusEGlobal::heavyDebugMsg) fprintf(stderr, "  adding toolbar break\n");

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
    fprintf(stderr, "====== DEBUG ======: topwin's menu got inited AFTER being shared!\n");
    if (!topwin->sharesToolsAndMenu()) fprintf(stderr, "======       ======: WTF, now it doesn't share any more?!?\n");
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
    // the isVisibleTo check is necessary because isVisible returns false if a
    // MdiSubWin is actually visible, but the muse main window is hidden for some reason
    {
      if (!sep)
      {
        menuWindows->addSeparator();
        sep=true;
      }
      QAction* temp=menuWindows->addAction((*it)->windowTitle());
      QWidget* tlw = static_cast<QWidget*>(*it);
      connect(temp, &QAction::triggered, [this, tlw]() { bringToFront(tlw); } );

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
      QWidget* tlw = static_cast<QWidget*>(*it);
      connect(temp, &QAction::triggered, [this, tlw]() { bringToFront(tlw); } );
    }

  windowsCascadeAction->setEnabled(there_are_subwins);
  windowsTileAction->setEnabled(there_are_subwins);
  windowsRowsAction->setEnabled(there_are_subwins);
  windowsColumnsAction->setEnabled(there_are_subwins);
}

void MusE::resetXrunsCounter()
{
   MusEGlobal::audio->resetXruns();
}

bool MusE::startPythonBridge()
{
#ifdef PYTHON_SUPPORT
  printf("Starting MusE Pybridge...\n");
  return MusECore::startPythonBridge();
#endif
  return false;
}

bool MusE::stopPythonBridge()
{ 
#ifdef PYTHON_SUPPORT
  printf("Stopping MusE Pybridge...\n");
  return MusECore::stopPythonBridge();
#endif
  return true;
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

void MusE::toggleRewindOnStop(bool onoff)
{
  MusEGlobal::config.useRewindOnStop = onoff;
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
      fprintf(stderr, "ERROR: tried to arrange subwins in columns, but there's too few space.\n");
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
      fprintf(stderr, "ERROR: tried to arrange subwins in rows, but there's too few space.\n");
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
      fprintf(stderr, "ERROR: tried to tile subwins, but there's too few space.\n");
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

QString MusE::projectTitle(QString name)
{
  return tr("MusE: Song: ") + MusEGui::projectTitleFromFilename(name);
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

void MusE::saveTimerSlot()
{
    if (MusEGlobal::config.autoSave == false ||
        MusEGlobal::museProject == MusEGlobal::museProjectInitPath ||
        MusEGlobal::song->dirty == false)
    {
        //printf("conditions not met, ignore %d %d\n", MusEGlobal::config.autoSave, MusEGlobal::song->dirty);
        return;
    }
    saveIncrement++;
    if (saveIncrement > 4) {
        // printf("five minutes passed %d %d\n", MusEGlobal::config.autoSave, MusEGlobal::song->dirty);
        // time to see if we are allowed to save, if so. Do
        if (MusEGlobal::audio->isPlaying() == false) {
            fprintf(stderr, "Performing autosave\n");
            save(project.filePath(), false, writeTopwinState);
        } else
        {
            //printf("isPlaying, can't save\n");
        }
    }
}

void MusE::toggleTrackArmSelectedTrack()
{
    // If there is only one track selected we toggle it's rec-arm status.

    int selectedTrackCount = 0;
    MusECore::WaveTrackList* wtl = MusEGlobal::song->waves();
    MusECore::TrackList selectedTracks;

    for (MusECore::iWaveTrack i = wtl->begin(); i != wtl->end(); ++i) {
          if((*i)->selected())
          {
              selectedTrackCount++;
              selectedTracks.push_back(*i);
          }
    }
    MusECore::MidiTrackList* mtl = MusEGlobal::song->midis();
    for (MusECore::iMidiTrack i = mtl->begin(); i != mtl->end(); ++i) {
          if((*i)->selected())
          {
              selectedTrackCount++;
              selectedTracks.push_back(*i);
          }
    }
    if (selectedTrackCount == 1) {
        // Let's toggle the selected instance.
        MusECore::PendingOperationList operations;
        foreach (MusECore::Track *t, selectedTracks)
        {
          bool newRecState = !t->recordFlag();
          if(!t->setRecordFlag1(newRecState))
            continue;
          operations.add(MusECore::PendingOperationItem(t, newRecState, MusECore::PendingOperationItem::SetTrackRecord));
        }
        MusEGlobal::audio->msgExecutePendingOperations(operations, true);
    }
}

//---------------------------------------------------------
//   importWave
//---------------------------------------------------------

void MusE::importWave()
{
   MusECore::Track* track = _arranger->curTrack();
   if (!track || track->type() != MusECore::Track::WAVE) {

      //just create new wave track and go on...
      if(MusEGlobal::song)
      {
         QAction act(MusEGlobal::song);
         act.setData(MusECore::Track::WAVE);
         track = MusEGlobal::song->addNewTrack(&act, NULL);
      }

      if(!track)
      {
         QMessageBox::critical(this, QString("MusE"),
                 tr("To import an audio file you have first to select a wave track"));
               return;

      }

   }
   MusECore::AudioPreviewDialog afd(this, MusEGlobal::sampleRate);
   afd.setDirectory(MusEGlobal::lastWavePath);
   afd.setWindowTitle(tr("Import Audio File"));
   /*QString fn = afd.getOpenFileName(MusEGlobal::lastWavePath, MusEGlobal::audio_file_pattern, this,
         tr("Import Audio File"), 0);
*/
   if(afd.exec() == QFileDialog::Rejected)
   {
      return;
   }

   QStringList filenames = afd.selectedFiles();
   if(filenames.size() < 1)
   {
      return;
   }
   QString fn = filenames [0];

   if (!fn.isEmpty()) {
      MusEGlobal::lastWavePath = fn;
      importWaveToTrack(fn);
   }
}

//---------------------------------------------------------
//   importWaveToTrack
//---------------------------------------------------------

bool MusE::importWaveToTrack(QString& name, unsigned tick, MusECore::Track* track)
{
   if (!track)
      track = _arranger->curTrack();

   MusECore::SndFileR f = MusECore::sndFileGetWave(name, true);

   if (f.isNull()) {
      fprintf(stderr, "import audio file failed\n");
      return true;
   }
   track->setChannels(f->channels());
   track->resetMeter();
   int samples = f->samples();
   if (MusEGlobal::sampleRate != f->samplerate()) {
      QMessageBox mbox(this);
      mbox.setWindowTitle(tr("Import Wavefile"));
      mbox.setText(tr("This wave file has a samplerate of %1 Hz,\n"
                      " as opposed to current setting %2 Hz.\n"
                      "A live, real-time samplerate converter can be used on this file.\n"
                      "Or, a copy of the file can be resampled now from %1 Hz to %2 Hz.")
                      .arg(f->samplerate()).arg(MusEGlobal::sampleRate));
      mbox.setInformativeText(tr("Do you want to use a converter or resample the file now?"));

      QPushButton* converter_button = mbox.addButton(tr("Use live converter"), QMessageBox::YesRole);
      QPushButton* resample_button = mbox.addButton(tr("Resample now"), QMessageBox::NoRole);
      mbox.addButton(tr("Cancel"), QMessageBox::RejectRole);
      mbox.setDefaultButton(converter_button);

      mbox.exec();
      if(mbox.clickedButton() != converter_button && mbox.clickedButton() != resample_button)
      {
         return true; // this removed f from the stack, dropping refcount maybe to zero and maybe deleting the thing
      }

      if(mbox.clickedButton() == converter_button)
      {
        samples = f->samplesConverted();
      }
      else if(mbox.clickedButton() == resample_button)
      {
        //save project if necessary
        //copy wave to project's folder,
        //rename it if there is a duplicate,
        //resample to project's rate

        if(MusEGlobal::museProject == MusEGlobal::museProjectInitPath)
        {
          if(!MusEGlobal::muse->saveAs())
              return true;
        }

        QFileInfo fi(f.name());
        QString projectPath = MusEGlobal::museProject + QDir::separator();
        QString fExt = "wav";
        QString fBaseName = fi.baseName();
        QString fNewPath = "";
        bool bNameIsNotUsed = false;
        for(int i = 0; i < 1000; i++)
        {
          fNewPath = projectPath + fBaseName + ((i == 0) ? "" : QString::number(i)) +  "." + fExt;
          if(!QFile(fNewPath).exists())
          {
              bNameIsNotUsed = true;
              break;
          }
        }

        if(!bNameIsNotUsed)
        {
          QMessageBox::critical(MusEGlobal::muse, tr("Wave import error"),
                                tr("There are too many wave files\n"
                                    "of the same base name as imported wave file\n"
                                    "Can not continue."));
          return true;
        }

        SF_INFO sfiNew;
        sfiNew.channels = f.channels();
        sfiNew.format = SF_FORMAT_WAV | SF_FORMAT_FLOAT;
        sfiNew.frames = 0;
        sfiNew.samplerate = MusEGlobal::sampleRate;
        sfiNew.seekable = 1;
        sfiNew.sections = 0;

        SNDFILE *sfNew = sf_open(fNewPath.toUtf8().constData(), SFM_RDWR, &sfiNew);
        if(sfNew == NULL)
        {
          QMessageBox::critical(MusEGlobal::muse, tr("Wave import error"),
                                tr("Can't create new wav file in project folder!\n") + sf_strerror(NULL));
          return true;
        }

        int srErr = 0;
        SRC_STATE *srState = src_new(SRC_SINC_BEST_QUALITY, sfiNew.channels, &srErr);
        if(!srState)
        {
          QMessageBox::critical(MusEGlobal::muse, tr("Wave import error"),
                                tr("Failed to initialize sample rate converter!"));
          sf_close(sfNew);
          QFile(fNewPath).remove();
          return true;
        }



        float fPeekMax = 1.0f; //if output save file will peek above this walue
        //it should be normalized later
        float fNormRatio = 1.0f / fPeekMax;
        int nTriesMax = 5;
        int nCurTry = 0;
        do
        {
          QProgressDialog pDlg(MusEGlobal::muse);
          pDlg.setMinimum(0);
          pDlg.setMaximum(f.samples());
          pDlg.setCancelButtonText(tr("Cancel"));
          if(nCurTry == 0)
          {
              pDlg.setLabelText(tr("Resampling wave file\n"
                                      "\"%1\"\n"
                                      "from %2 to %3 Hz...")
                                  .arg(f.name()).arg(f.samplerate()).arg(sfiNew.samplerate));
          }
          else
          {
              pDlg.setLabelText(tr("Output has clipped\n"
                                  "Resampling again and normalizing wave file\n"
                                  "\"%1\"\n"
                                  "Try %2 of %3...")
                                .arg(QFileInfo(fNewPath).fileName()).arg(nCurTry).arg(nTriesMax));
          }
          pDlg.setWindowModality(Qt::WindowModal);
          src_reset(srState);
          SRC_DATA sd;
          sd.src_ratio = ((double)MusEGlobal::sampleRate) / (double)f.samplerate();
          sf_count_t szBuf = 8192;
          float srcBuffer [szBuf];
          float dstBuffer [szBuf];
          unsigned sChannels = f.channels();
          sf_count_t szBufInFrames = szBuf / sChannels;
          sf_count_t szFInFrames = f.samples();
          sf_count_t nFramesRead = 0;
          sf_count_t nFramesWrote = 0;
          sd.end_of_input = 0;
          bool bEndOfInput = false;
          pDlg.setValue(0);

          f.seek(0, SEEK_SET);

          while(sd.end_of_input == 0)
          {
              size_t nFramesBuf = 0;
              if(bEndOfInput)
                sd.end_of_input = 1;
              else
              {
                nFramesBuf = f.readDirect(srcBuffer, szBufInFrames);
                if(nFramesBuf == 0)
                    break;
                nFramesRead += nFramesBuf;
              }

              sd.data_in = srcBuffer;
              sd.data_out = dstBuffer;
              sd.input_frames = nFramesBuf;
              sd.output_frames = szBufInFrames;
              sd.input_frames_used = 0;
              sd.output_frames_gen = 0;
              do
              {
                if(src_process(srState, &sd) != 0)
                    break;
                sd.data_in += sd.input_frames_used * sChannels;
                sd.input_frames -= sd.input_frames_used;

                if(sd.output_frames_gen > 0)
                {
                    nFramesWrote += sd.output_frames_gen;
                    //detect maximum peek value;
                    for(unsigned ch = 0; ch < sChannels; ch++)
                    {

                      for(long k = 0; k < sd.output_frames_gen; k++)
                      {
                          dstBuffer [k * sChannels + ch] *= fNormRatio; //normilize if needed
                          float fCurPeek = dstBuffer [k * sChannels + ch];
                          if(fPeekMax < fCurPeek)
                          {
                            //update maximum peek value
                            fPeekMax = fCurPeek;
                          }
                      }
                    }
                    sf_writef_float(sfNew, dstBuffer, sd.output_frames_gen);
                }
                else
                    break;

              }
              while(true);

              pDlg.setValue(nFramesRead);

              if(nFramesRead >= szFInFrames)
              {
                bEndOfInput = true;
              }

              if(pDlg.wasCanceled())//free all resources
              {
                src_delete(srState);
                sf_close(sfNew);
                f.close();
                f = nullptr;
                QFile(fNewPath).remove();
                return true;
              }
          }

          pDlg.setValue(szFInFrames);

          if(fPeekMax > 1.0f) //output has clipped. Normilize it
          {
              nCurTry++;
              sf_seek(sfNew, 0, SEEK_SET);
              f.seek(0, SEEK_SET);
              pDlg.setValue(0);
              fNormRatio = 1.0f / fPeekMax;
              fPeekMax = 1.0f;
          }
          else
              break;
        }
        while(nCurTry <= nTriesMax);

        src_delete(srState);

        sf_close(sfNew);

        f.close();
        f = nullptr;

        //reopen resampled wave again
        f = MusECore::sndFileGetWave(fNewPath, true);
        if(!f)
        {
          printf("import audio file failed\n");
          return true;
        }
        samples = f->samples();
      }
   }

   MusECore::WavePart* part = new MusECore::WavePart((MusECore::WaveTrack *)track);
   if (tick)
      part->setTick(tick);
   else
      part->setTick(MusEGlobal::song->cpos());
   part->setLenFrame(samples);

   MusECore::Event event(MusECore::Wave);
   MusECore::SndFileR sf(f);
   event.setSndFile(sf);
   event.setSpos(0);
   event.setLenFrame(samples);
   part->addEvent(event);

   part->setName(QFileInfo(f->name()).completeBaseName());
   MusEGlobal::song->applyOperation(MusECore::UndoOp(MusECore::UndoOp::AddPart, part));
   unsigned endTick = part->tick() + part->lenTick();
   if (MusEGlobal::song->len() < endTick)
      MusEGlobal::song->setLen(endTick);
   return false;
}

} //namespace MusEGui
