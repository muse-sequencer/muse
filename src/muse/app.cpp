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

#include <QDesktopWidget>
#include <QClipboard>
#include <QMessageBox>
#include <QShortcut>
#include <QWhatsThis>
//#include <QSettings>
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QSocketNotifier>
//#include <QStyleFactory>
#include <QTextStream>
#include <QInputDialog>
#include <QStringList>
#include <QPushButton>
#include <QDir>
#include <QFile>
#include <QIODevice>
#include <QProcess>
#include <QStatusBar>
#if QT_VERSION >= 0x050b00
#include <QScreen>
#endif

#include <samplerate.h>

#include <iostream>
#include <algorithm>
//#include <typeinfo>
#include <random>

#include "app.h"
#include "master/lmaster.h"
#include "al/dsp.h"
#include "audio.h"
#include "audiodev.h"
#include "audioprefetch.h"
// FIXME Move cliplist into components ?
#include "cliplist/cliplist.h"
//#include "debug.h"
#include "components/didyouknow.h"
#include "drumedit.h"
#include "components/filedialog.h"
#include "gconfig.h"
#include "globals.h"
//#include "gui.h"
#include "helper.h"
#include "wave_helper.h"
#include "icons.h"
#include "listedit.h"
#include "midiseq.h"
#include "mitplugin.h"
#include "mittranspose.h"
#include "components/mixdowndialog.h"
#include "mrconfig.h"
#include "pianoroll.h"
#ifdef PYTHON_SUPPORT
#include "remote/pyapi.h"
#endif
#include "song.h"
#include "components/routepopup.h"
#include "components/savenewrevisiondialog.h"
#include "components/songinfo.h"
#include "ticksynth.h"
#include "tempo.h"
#include "tlist.h"
#include "waveedit.h"
#include "components/projectcreateimpl.h"
//#include "widgets/menutitleitem.h"
#include "components/unusedwavefiles.h"
#include "functions.h"
#include "components/songpos_toolbar.h"
#include "components/sig_tempo_toolbar.h"
#include "songfile_discovery.h"
#include "pos.h"
#include "wave.h"
#include "wavepreview.h"
#include "shortcuts.h"
#include "rectoolbar.h"
#include "postoolbar.h"
#include "synctoolbar.h"
#include "libs/file/file.h"

#ifdef _WIN32
#include <Windows.h>
#endif

// Forwards from header:
#include <QCloseEvent>
#include <QMenu>
#include <QToolBar>
//#include <QToolButton>
#include <QProgressDialog>
#include <QTimer>
//#include <QMdiSubWindow>
#include <QDockWidget>
#include <QAction>
#include "track.h"
//#include "minstrument.h"
#include "midiport.h"
#include "part.h"
//#include "synth.h"
#include "undo.h"
#include "appearance.h"
#include "arranger.h"
#include "arrangerview.h"
#include "amixer.h"
#include "bigtime.h"
//#include "cliplist.h"
#include "editinstrument.h"
//#include "tools.h"
#include "genset.h"
//#include "mrconfig.h"
#include "marker/markerview.h"
#include "metronome.h"
#include "conf.h"
#include "midifilterimpl.h"
#include "midiitransform.h"
#include "miditransform.h"
#include "midisyncimpl.h"
#include "scoreedit.h"
#include "shortcutconfig.h"
#include "transport.h"
#include "visibletracks.h"
#include "routedialog.h"
#include "cpu_toolbar.h"
#include "musemdiarea.h"
#include "snooper.h"
#include "xml.h"
#ifdef BUILD_EXPERIMENTAL
  #include "rhythm.h"
#endif
#include "midieditor.h"
#include "listedit.h"
#include "pianoroll.h"
#include "waveedit.h"
#include "drumedit.h"
#include "master/masteredit.h"

// For debugging song clearing and loading: Uncomment the fprintf section.
#define DEBUG_LOADING_AND_CLEARING(dev, format, args...) // fprintf(dev, format, ##args);

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

QStringList projectRecentList;

#ifdef HAVE_LASH
#include <lash/lash.h>
lash_client_t * lash_client = 0;
#endif /* HAVE_LASH */

//int watchAudioPrefetch, watchMidi;
//pthread_t splashThread;



#ifndef USE_SENDPOSTEDEVENTS_FOR_TOPWIN_CLOSE
MusE::LoadingFinishStruct::LoadingFinishStruct(Type type, Flags flags, const QString &fileName)
  : _type(type), _flags(flags), _fileName(fileName)
{
}

MusE::ObjectDestructionStruct::ObjectDestructionStruct(const QMetaObject::Connection& conn, bool waitForDelete)
{
  _conn = conn;
  _waitForDelete = waitForDelete;
}

MusE::ObjectDestructions::Iterator MusE::ObjectDestructions::findObject(QObject* obj, bool findWait)
{
  ObjectDestructions::Iterator it = QMap<QObject*, ObjectDestructionStruct>::find(obj);
  const bool found = it != end();
  if(found)
  {
    if(it.value()._waitForDelete == findWait)
      return it;
  }
  return end();
}
MusE::ObjectDestructions::ConstIterator MusE::ObjectDestructions::findObject(QObject* obj, bool findWait) const
{
  ObjectDestructions::ConstIterator it = QMap<QObject*, ObjectDestructionStruct>::constFind(obj);
  const bool found = it != constEnd();
  if(found && it.value()._waitForDelete == findWait)
    return it;
  return constEnd();
}
bool MusE::ObjectDestructions::hasWaitingObjects() const
{
  for(ObjectDestructions::ConstIterator it = constBegin(); it != constEnd(); ++it)
  {
    if(it.value()._waitForDelete)
      return true;
  }
  return false;
}
bool MusE::ObjectDestructions::markAll(bool asWait)
{
  for(ObjectDestructions::Iterator it = begin(); it != end(); ++it)
    it.value()._waitForDelete = asWait;
  return !empty();
}
#endif

#ifndef USE_SENDPOSTEDEVENTS_FOR_TOPWIN_CLOSE
void MusE::executeLoadingFinish()
{
  const int sz = _loadingFinishStructList.size();
  for(int i = 0; i < sz; ++i)
  {
    const LoadingFinishStruct& lfs = _loadingFinishStructList.at(i);

    switch(lfs._type)
    {
      case LoadingFinishStruct::LoadProjectFile:
        DEBUG_LOADING_AND_CLEARING(stderr, "MusE::executeLoadingFinish lfs idx:%d type:%d (LoadProjectFile)\n", i, lfs._type);

        finishLoadProjectFile(lfs._flags & MusE::LoadingFinishStruct::RestartSequencer);
        break;
      case LoadingFinishStruct::LoadProjectFile1:
        DEBUG_LOADING_AND_CLEARING(stderr, "MusE::executeLoadingFinish lfs idx:%d type:%d (LoadProjectFile1)\n", i, lfs._type);

        finishLoadProjectFile1(
          lfs._fileName,
          lfs._flags & MusE::LoadingFinishStruct::SongTemplate,
          lfs._flags & MusE::LoadingFinishStruct::DoReadMidiPorts);
        break;
      case LoadingFinishStruct::ClearSong:
        DEBUG_LOADING_AND_CLEARING(stderr, "MusE::executeLoadingFinish lfs idx:%d type:%d (ClearSong)\n", i, lfs._type);

        finishClearSong(
          lfs._flags & MusE::LoadingFinishStruct::ClearAll);
        break;
      case LoadingFinishStruct::LoadTemplate:
        DEBUG_LOADING_AND_CLEARING(stderr, "MusE::executeLoadingFinish lfs idx:%d type:%d (LoadTemplate)\n", i, lfs._type);

        finishLoadTemplate();
        break;
      case LoadingFinishStruct::LoadDefaultTemplate:
        DEBUG_LOADING_AND_CLEARING(stderr, "MusE::executeLoadingFinish lfs idx:%d type:%d (LoadDefaultTemplate)\n", i, lfs._type);

        finishLoadDefaultTemplate();
        break;
      case LoadingFinishStruct::FileClose:
        DEBUG_LOADING_AND_CLEARING(stderr, "MusE::executeLoadingFinish lfs idx:%d type:%d (FileClose)\n", i, lfs._type);

        finishFileClose(lfs._flags & MusE::LoadingFinishStruct::RestartSequencer);
        break;
    }
  }
  _loadingFinishStructList.clear();
}
#endif

#ifndef USE_SENDPOSTEDEVENTS_FOR_TOPWIN_CLOSE
void MusE::objectDestroyed(QObject *obj)
{
  // Find any object regardless of being marked to wait for.
  ObjectDestructions::Iterator it = _pendingObjectDestructions.find(obj);
  const bool found = it != _pendingObjectDestructions.end();
  if(found)
    _pendingObjectDestructions.erase(it);

  DEBUG_LOADING_AND_CLEARING(stderr, "MusE::objectDestroyed obj:%p found:%d _pendingObjectDestructions (after erase):%d\n",
    obj, found, _pendingObjectDestructions.size());

  // Still more deletions to wait for?
  if(_pendingObjectDestructions.hasWaitingObjects())
    return;

  // All top level deletions that we were waiting for have now been deleted.
  // Now it is safe to execute the finishing functions and clear the finishing list.
  executeLoadingFinish();
}
#endif

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
      // Set the prefetch thread priority to zero so that it doesn't run in realtime.
      // It will have a default priority (usually OTHER).
      const int pfprio = 0;
      if(MusEGlobal::audioPrefetch)
      {
        if(!MusEGlobal::audioPrefetch->isRunning())
        {
          MusEGlobal::audioPrefetch->start(pfprio);
          //
          // wait for audio prefetch to start
          //
          for(int i = 0; i < 60; ++i)
          {
            if(MusEGlobal::audioPrefetch->isRunning())
              break;
            sleep(1);
          }
          if(!MusEGlobal::audioPrefetch->isRunning())
          {
            QMessageBox::critical( MusEGlobal::muse, tr("Failed to start audio disk prefetch!"),
                tr("Timeout waiting for audio disk prefetch thread to run.\n"));
          }
// TESTING: Shouldn't be required now. audio->start() below will seek the transport which will seek the prefetch. I think...
//           else
//           {
//             // Diagnostics.
//             fprintf(stderr, "MusE::seqStart: Calling audioPrefetch->msgSeek: audio pos:%d\n", MusEGlobal::audio->pos().frame());
//
//             // In case prefetch is not filled, do it now.
//             MusEGlobal::audioPrefetch->msgSeek(MusEGlobal::audio->pos().frame(), true); // Force it upon startup only.
//           }
        }
      }
      else
        fprintf(stderr, "seqStart(): audioPrefetch is NULL\n");

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

void MusE::addProjectToRecentList(const QString& name)
{
  if (projectRecentList.contains(name))
    return;

  projectRecentList.push_front(name);
  if (projectRecentList.size() > MusEGlobal::config.recentListLength)
    projectRecentList.pop_back();

  saveProjectRecentList();
}

void MusE::saveProjectRecentList()
{
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
}

//---------------------------------------------------------
//   MusE
//---------------------------------------------------------

MusE::MusE() : QMainWindow()
      {
      setIconSize(QSize(MusEGlobal::config.iconSize, MusEGlobal::config.iconSize));
      setFocusPolicy(Qt::NoFocus);
      MusEGlobal::muse      = this;    // hack
      _isRestartingApp      = false;
      midiSyncConfig        = nullptr;
      midiRemoteConfig      = nullptr;
      midiPortConfig        = nullptr;
      metronomeConfig       = nullptr;
      midiFileConfig        = nullptr;
      midiFilterConfig      = nullptr;
      midiInputTransform    = nullptr;
#ifdef BUILD_EXPERIMENTAL
      midiRhythmGenerator   = nullptr;
#endif
      globalSettingsConfig  = nullptr;
      arrangerView          = nullptr;
      softSynthesizerConfig = nullptr;
      midiTransformerDialog = nullptr;
      shortcutConfig        = nullptr;
      appearance            = nullptr;
      _snooperDialog        = nullptr;
      //audioMixer            = 0;
      mixer1                = nullptr;
      mixer2                = nullptr;
      routeDialog           = nullptr;
      watchdogThread        = 0;
      editInstrument        = nullptr;
      //routingPopupMenu      = 0;
      progress              = nullptr;
      saveIncrement         = 0;
      activeTopWin          = nullptr;
      currentMenuSharingTopwin = nullptr;
      waitingForTopwin      = nullptr;
      _lastProjectWasTemplate = false;
      _lastProjectLoadedConfig = true;
#ifndef USE_SENDPOSTEDEVENTS_FOR_TOPWIN_CLOSE
      _busyWithLoading      = false;
#endif

      appName               = PACKAGE_NAME;
      setWindowTitle(appName);
      setWindowIcon(*MusEGui::museIcon);

      MusEGlobal::globalRasterizer = new Rasterizer(MusEGlobal::config.division, this);

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

      setCorner(Qt::BottomRightCorner, Qt::RightDockWidgetArea);
      setCorner(Qt::TopRightCorner, Qt::RightDockWidgetArea);

      markerDock = new QDockWidget("Markers", this);
      markerDock->setObjectName("markerDock");
//      markerDock->setAllowedAreas(Qt::BottomDockWidgetArea | Qt::RightDockWidgetArea);
      markerView = new MusEGui::MarkerView(markerDock);
      markerDock->setWidget(markerView);
      addDockWidget(Qt::RightDockWidgetArea, markerDock);
      markerDock->hide();

      masterListDock = new QDockWidget("Mastertrack List", this);
      masterListDock->setObjectName("masterListDock");
      // listMasterDock->setAllowedAreas(Qt::BottomDockWidgetArea | Qt::RightDockWidgetArea);
      masterList = new MusEGui::LMaster(this);
      masterListDock->setWidget(masterList);
      addDockWidget(Qt::RightDockWidgetArea, masterListDock);
      masterListDock->hide();

      clipListDock = new QDockWidget("Clip List", this);
      clipListDock->setObjectName("clipListDock");
//      clipListDock->setAllowedAreas(Qt::BottomDockWidgetArea | Qt::RightDockWidgetArea);
      clipListEdit = new MusEGui::ClipListEdit(clipListDock);
      clipListDock->setWidget(clipListEdit);
      addDockWidget(Qt::RightDockWidgetArea, clipListDock);
      clipListDock->hide();

      dockMixerA = MusEGlobal::config.mixerDockedA;
      dockMixerB = MusEGlobal::config.mixerDockedB;

      if (dockMixerA) {
          mixer1Dock = new QDockWidget("Mixer A", this);
          mixer1Dock->setObjectName("mixer1Dock");
          mixer1Dock->setAllowedAreas(Qt::BottomDockWidgetArea | Qt::TopDockWidgetArea);
          mixer1 = new MusEGui::AudioMixerApp(this, &(MusEGlobal::config.mixer1), true);
          mixer1Dock->setWidget(mixer1);
          addDockWidget(Qt::BottomDockWidgetArea, mixer1Dock);
          mixer1Dock->hide();
          mixer1->setMinimumHeight(400);

          connect(mixer1Dock, &QDockWidget::topLevelChanged, [this](bool b){ mixer1DockTopLevelChanged(b); });
      }

      if (dockMixerB) {
          mixer2Dock = new QDockWidget("Mixer B", this);
          mixer2Dock->setObjectName("mixer2Dock");
          mixer2Dock->setAllowedAreas(Qt::BottomDockWidgetArea | Qt::TopDockWidgetArea);
          mixer2 = new MusEGui::AudioMixerApp(this, &(MusEGlobal::config.mixer2), true);
          mixer2Dock->setWidget(mixer2);
          addDockWidget(Qt::BottomDockWidgetArea, mixer2Dock);
          mixer2Dock->hide();
          mixer2->setMinimumHeight(400);

          connect(mixer2Dock, &QDockWidget::topLevelChanged, [this](bool b){ mixer2DockTopLevelChanged(b); });

      }


      //---------------------------------------------------
      //    undo/redo
      //---------------------------------------------------

      MusEGlobal::undoRedo = new QActionGroup(this);
      MusEGlobal::undoRedo->setExclusive(false);
      MusEGlobal::undoAction = new QAction(*MusEGui::undoSVGIcon, tr("Und&o"),
        MusEGlobal::undoRedo);
      MusEGlobal::redoAction = new QAction(*MusEGui::redoSVGIcon, tr("Re&do"),
        MusEGlobal::undoRedo);

      MusEGlobal::undoAction->setWhatsThis(tr("Undo last change to project"));
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
      MusEGlobal::loopAction->setStatusTip(tr("Loop between left mark and right mark"));
      connect(MusEGlobal::loopAction, SIGNAL(toggled(bool)), MusEGlobal::song, SLOT(setLoop(bool)));

      MusEGlobal::punchinAction = new QAction(*MusEGui::punchinSVGIcon, tr("Punch in"),
                                              MusEGlobal::transportAction);
      MusEGlobal::punchinAction->setCheckable(true);

      MusEGlobal::punchinAction->setWhatsThis(tr("Record starts at left mark"));
      MusEGlobal::punchinAction->setStatusTip(tr("Recording starts at left mark"));
      connect(MusEGlobal::punchinAction, SIGNAL(toggled(bool)), MusEGlobal::song, SLOT(setPunchin(bool)));

      MusEGlobal::punchoutAction = new QAction(*MusEGui::punchoutSVGIcon, tr("Punch out"),
                                               MusEGlobal::transportAction);
      MusEGlobal::punchoutAction->setCheckable(true);

      MusEGlobal::punchoutAction->setWhatsThis(tr("Record stops at right mark"));
      MusEGlobal::punchoutAction->setStatusTip(tr("Recording stops at right mark"));
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
      connect(MusEGlobal::rewindAction, SIGNAL(triggered()), MusEGlobal::song, SLOT(rewindStep()));

      MusEGlobal::forwardAction = new QAction(*MusEGui::fastForwardSVGIcon, tr("Forward"), 
                                               MusEGlobal::transportAction);

      MusEGlobal::forwardAction->setWhatsThis(tr("Move current position"));
      connect(MusEGlobal::forwardAction, SIGNAL(triggered()), MusEGlobal::song, SLOT(forwardStep()));

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
      MusEGlobal::recordAction->setStatusTip(tr("To record press record and then play"));
      connect(MusEGlobal::recordAction, SIGNAL(toggled(bool)), MusEGlobal::song, SLOT(setRecord(bool)));

      MusEGlobal::panicAction = new QAction(*MusEGui::panicSVGIcon, tr("Panic"), this);

      QMenu* panicPopupMenu = new QMenu(this);
      MusEGlobal::panicAction->setMenu(panicPopupMenu);
      MusEGlobal::panicAction->setObjectName("PanicButton");
      
//      MusEGlobal::panicAction->setWhatsThis(tr("Send note off to all midi channels")); // wrong?
      MusEGlobal::panicAction->setStatusTip(tr("Panic button: Send 'all sounds off' and 'reset all controls' to all midi channels. Press F1 for help."));
      connect(MusEGlobal::panicAction, SIGNAL(triggered()), MusEGlobal::song, SLOT(panic()));

      MusEGlobal::metronomeAction = new QAction(*MusEGui::metronomeOnSVGIcon, tr("Metronome"), this);
      MusEGlobal::metronomeAction->setObjectName("MetronomeButton");
      MusEGlobal::metronomeAction->setCheckable(true);
      MusEGlobal::metronomeAction->setWhatsThis(tr("Turn on/off metronome"));
      MusEGlobal::metronomeAction->setStatusTip(tr("Metronome on/off. Press F1 for help."));
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
      fileSaveAsNewProjectAction = new QAction(*MusEGui::filesaveProjectSVGIcon, tr("Save As New &Project..."), this);
      fileSaveRevisionAction = new QAction(*MusEGui::filesaveRevisionSVGIcon, tr("Save New Re&vision"), this);
      fileSaveAsTemplateAction = new QAction(*MusEGui::filesaveTemplateSVGIcon, tr("Save As Te&mplate..."), this);

      fileCloseAction = new QAction(*MusEGui::filecloseSVGIcon, tr("&Close"), this);
      
      fileImportMidiAction = new QAction(tr("Import Midi File..."), this);
      fileExportMidiAction = new QAction(tr("Export Midi File..."), this);
      fileExportMidiSelectedVisibleAction = new QAction(tr("Export Selected Visible Tracks To Midi File..."), this);
      fileImportPartAction = new QAction(tr("Import Part..."), this);
      fileExportSelectedPartsAction = new QAction(tr("Export Selected Parts To Midi File..."), this);

      fileImportWaveAction = new QAction(tr("Import Audio File..."), this);
      fileMoveWaveFiles = new QAction(tr("Find Unused Wave Files..."), this);

      quitAction = new QAction(*MusEGui::appexitSVGIcon, tr("&Quit"), this);

      editSongInfoAction = new QAction(tr("Edit Project Description..."), this);

      //-------- View Actions
      viewTransportAction = new QAction(*MusEGui::transportSVGIcon, tr("Transport Panel"), this);
      viewTransportAction->setCheckable(true);
      viewBigtimeAction = new QAction(*MusEGui::bigtimeSVGIcon, tr("Bigtime Window"),  this);
      viewBigtimeAction->setCheckable(true);
      viewMixerAAction = new QAction(*MusEGui::mixerSVGIcon, tr("Mixer A"), this);
      viewMixerAAction->setCheckable(true);
      viewMixerBAction = new QAction(*MusEGui::mixerSVGIcon, tr("Mixer B"), this);
      viewMixerBAction->setCheckable(true);

      viewMarkerAction = markerDock->toggleViewAction();
      viewCliplistAction = clipListDock->toggleViewAction();

      toggleDocksAction = new QAction(tr("Show Docks"), this);
      toggleDocksAction->setCheckable(true);
      toggleDocksAction->setChecked(true);
      toggleDocksAction->setStatusTip(tr("Toggle display of currently visible dock windows."));

      fullscreenAction=new QAction(tr("Fullscreen"), this);
      fullscreenAction->setCheckable(true);
      fullscreenAction->setChecked(false);
      fullscreenAction->setStatusTip(tr("Display MusE main window in full screen mode."));

//      QMenu* master = new QMenu(tr("Mastertrack"), this);
//      master->setIcon(QIcon(*edit_mastertrackIcon));
      masterGraphicAction = new QAction(QIcon(*mastereditSVGIcon),tr("Mastertrack Graphic..."), this);
      masterListAction = masterListDock->toggleViewAction();
//      masterListAction = new QAction(QIcon(*mastertrack_listIcon),tr("List..."), this);
//      master->addAction(masterGraphicAction);
//      master->addAction(masterListAction);

      //-------- Midi Actions
      midiEditInstAction = new QAction(*MusEGui::editInstrumentSVGIcon, tr("Edit Instrument..."), this);
//      midiInputPlugins = new QMenu(tr("Input Plugins"), this);
      midiTrpAction = new QAction(*MusEGui::midiInputTransposeSVGIcon, tr("Input Transpose..."), this);
      midiInputTrfAction = new QAction(*MusEGui::midiInputTransformSVGIcon, tr("Input Transform..."), this);
      midiInputFilterAction = new QAction(*MusEGui::midiInputFilterSVGIcon, tr("Input Filter..."), this);
      midiRemoteAction = new QAction(*MusEGui::midiInputRemoteSVGIcon, tr("Remote Control..."), this);
#ifdef BUILD_EXPERIMENTAL
      midiRhythmAction = new QAction(QIcon(*midi_inputplugins_random_rhythm_generatorIcon), tr("Rhythm Generator"), this);
#endif
      midiResetInstAction = new QAction(*MusEGui::midiResetSVGIcon, tr("Reset Instrument"), this);
      midiResetInstAction->setStatusTip(tr("Send 'note-off' command to all midi channels."));
      midiInitInstActions = new QAction(*MusEGui::midiInitSVGIcon, tr("Init Instrument"), this);
      midiInitInstActions->setStatusTip(tr("Send initialization messages as found in instrument definition."));
      midiLocalOffAction = new QAction(*MusEGui::midiLocalOffSVGIcon, tr("Local Off"), this);
      midiLocalOffAction->setStatusTip(tr("Send 'local-off' command to all midi channels."));

      //-------- Audio Actions
      audioBounce2TrackAction = new QAction(*MusEGui::downmixTrackSVGIcon, tr("Render Downmix to Selected Wave Track"), this);
      audioBounce2FileAction = new QAction(*MusEGui::downmixFileSVGIcon, tr("Render Downmix to a File..."), this);
      audioRestartAction = new QAction(*MusEGui::restartSVGIcon, tr("Restart Audio"), this);

      //-------- Automation Actions
// REMOVE Tim. automation. Removed.
// Deprecated. MusEGlobal::automation is now fixed TRUE
//   for now until we decide what to do with it.
//       autoMixerAction = new QAction(QIcon(*MusEGui::automation_mixerIcon), tr("Mixer Automation"), this);
//       autoMixerAction->setCheckable(true);
      autoSnapshotAction = new QAction(*MusEGui::snapshotSVGIcon, tr("Take Automation Snapshot"), this);
      autoClearAction = new QAction(*MusEGui::clearSVGIcon, tr("Clear Automation Data"), this);

       //-------- Settings Actions
      settingsGlobalAction = new QAction(*MusEGui::settingsSVGIcon, tr("Global Settings..."), this);
      settingsAppearanceAction = new QAction(*MusEGui::appearanceSVGIcon, tr("Appearance..."), this);
      settingsShortcutsAction = new QAction(*MusEGui::keySVGIcon, tr("Keyboard Shortcuts..."), this);
      follow = new QMenu(tr("Follow Song"), this);
      follow->setObjectName("CheckmarkOnly");
      QActionGroup *followAG = new QActionGroup(this);
      followAG->setExclusive(true);
      dontFollowAction = new QAction(tr("Don't Follow Song"), followAG);
      dontFollowAction->setCheckable(true);
      followPageAction = new QAction(tr("Follow Page"), followAG);
      followPageAction->setCheckable(true);
      followCtsAction = new QAction(tr("Follow Continuous"), followAG);
      followCtsAction->setCheckable(true);
      followPageAction->setChecked(true);

      rewindOnStopAction=new QAction(tr("Rewind on Stop"), this);
      rewindOnStopAction->setShortcut(shortcuts[SHRT_TOGGLE_REWINDONSTOP].key);
      rewindOnStopAction->setCheckable(true);
      rewindOnStopAction->setChecked(MusEGlobal::config.useRewindOnStop);

      settingsMetronomeAction = new QAction(*MusEGui::metronomeOnSVGIcon, tr("Metronome..."), this);
      settingsMidiSyncAction = new QAction(*MusEGui::midiSyncSVGIcon, tr("Midi Sync..."), this);
      settingsMidiIOAction = new QAction(*MusEGui::midiExportImportSVGIcon, tr("Midi File Import/Export..."), this);
      settingsMidiPortAction = new QAction(*MusEGui::ankerSVGIcon, tr("Midi Ports/Soft Synths..."), this);

      //-------- Help Actions
      helpManualAction = new QAction(tr("&Manual (Wiki)..."), this);
      helpHomepageAction = new QAction(tr("MusE &Homepage..."), this);
      helpDidYouKnow = new QAction(tr("&Did You Know?"), this);

      helpReportAction = new QAction(tr("&Report Bug..."), this);
      helpAboutAction = new QAction(tr("&About MusE..."), this);

      helpSnooperAction = new QAction(tr("Snooper (Developer Tool)..."), this);

      //---- Connections
      //-------- File connections

      connect(fileNewAction,  SIGNAL(triggered()), SLOT(loadDefaultTemplate()));
      connect(fileNewFromTemplateAction,  SIGNAL(triggered()), SLOT(loadTemplate()));
      connect(fileOpenAction, SIGNAL(triggered()), SLOT(loadProject()));
      connect(openRecent, SIGNAL(aboutToShow()), SLOT(openRecentMenu()));
      connect(openRecent, SIGNAL(triggered(QAction*)), SLOT(selectProject(QAction*)));

      connect(fileSaveAction, SIGNAL(triggered()), SLOT(save()));
      connect(fileSaveAsAction, SIGNAL(triggered()), SLOT(saveAs()));
      connect(fileSaveAsNewProjectAction, SIGNAL(triggered()), SLOT(saveAsNewProject()));
      connect(fileSaveRevisionAction, SIGNAL(triggered()), SLOT(saveNewRevision()));
      connect(fileSaveAsTemplateAction, SIGNAL(triggered()), SLOT(saveAsTemplate()));

      connect(fileCloseAction, SIGNAL(triggered()), SLOT(fileClose()));
      
      connect(fileImportMidiAction, SIGNAL(triggered()), SLOT(importMidi()));
      connect(fileExportMidiAction, &QAction::triggered, [this]() { exportMidi(); } );
      connect(fileExportMidiSelectedVisibleAction, &QAction::triggered, [this]()
        { exportMidi(true /*selected visible tracks only*/); } );
      connect(fileExportSelectedPartsAction, &QAction::triggered, [this]()
        { exportMidi(false, true /*selected parts only*/, MusEGlobal::config.exportSelectedPartsAlignToBar0); } );
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
//      connect(viewArrangerAction, SIGNAL(toggled(bool)), SLOT(toggleArranger(bool)));
      connect(masterGraphicAction, SIGNAL(triggered()), SLOT(startMasterEditor()));
//      connect(masterListAction, SIGNAL(triggered()), SLOT(startLMasterEditor()));
      connect(toggleDocksAction, SIGNAL(toggled(bool)), SLOT(toggleDocks(bool)));
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
// REMOVE Tim. automation. Removed.
// Deprecated. MusEGlobal::automation is now fixed TRUE
//   for now until we decide what to do with it.
//       connect(autoMixerAction, SIGNAL(triggered()), SLOT(switchMixerAutomation()));
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
      tools->setObjectName("File buttons");
      tools->addAction(fileNewAction);
      tools->addAction(fileNewFromTemplateAction);
      tools->addAction(fileOpenAction);
      tools->addAction(fileSaveAction);
      QAction* whatsthis = QWhatsThis::createAction(this);
      whatsthis->setIcon(*whatsthisSVGIcon);
      tools->addAction(whatsthis);

      QToolBar* undo_tools = addToolBar(tr("Undo/Redo"));
      undo_tools->setObjectName("Undo/Redo");
      undo_tools->addActions(MusEGlobal::undoRedo->actions());

      QToolBar* panic_toolbar = addToolBar(tr("Panic"));
      panic_toolbar->setObjectName("Panic tool");
      panic_toolbar->addAction(MusEGlobal::panicAction);

      QToolBar* metronome_toolbar = addToolBar(tr("Metronome"));
      metronome_toolbar->setObjectName("Metronome tool");
      metronome_toolbar->addAction(MusEGlobal::metronomeAction);

      cpuLoadToolbar = new CpuToolbar(tr("Cpu Load"), this);
      addToolBar(cpuLoadToolbar);
      cpuLoadToolbar->hide(); // hide as a default, the info is now in status bar too
      connect(cpuLoadToolbar, SIGNAL(resetClicked()), SLOT(resetXrunsCounter()));

      QToolBar* songpos_tb = addToolBar(tr("Timeline"));
      songpos_tb->setObjectName("Timeline tool");
      songpos_tb->addWidget(new MusEGui::SongPosToolbarWidget(songpos_tb));
      songpos_tb->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
      songpos_tb->setContextMenuPolicy(Qt::PreventContextMenu);
      addToolBar(Qt::BottomToolBarArea, songpos_tb);

      QToolBar* transportToolbar = addToolBar(tr("Transport"));
      transportToolbar->setObjectName("Transport tool");
      transportToolbar->addActions(MusEGlobal::transportAction->actions());
      transportToolbar->setIconSize(QSize(MusEGlobal::config.iconSize, MusEGlobal::config.iconSize));

      RecToolbar *recToolbar = new RecToolbar(tr("Recording"), this);
      addToolBar(recToolbar);

      SyncToolbar *syncToolbar = new SyncToolbar(tr("Sync"), this);
      addToolBar(syncToolbar);

      addToolBarBreak();

      TempoToolbar* tempo_tb = new TempoToolbar(tr("Tempo"), this);
      addToolBar(tempo_tb);
      
      SigToolbar* sig_tb = new SigToolbar(tr("Signature"), this);
      addToolBar(sig_tb);
      
      PosToolbar *posToolbar = new PosToolbar(tr("Position"), this);
      addToolBar(posToolbar);

      requiredToolbars.push_back(tools);
      requiredToolbars.push_back(cpuLoadToolbar);

      optionalToolbars.push_back(undo_tools);
      optionalToolbars.push_back(panic_toolbar);
      optionalToolbars.push_back(metronome_toolbar);
      optionalToolbars.push_back(songpos_tb);
      optionalToolbars.push_back(nullptr);  // Toolbar break
      optionalToolbars.push_back(transportToolbar);
      optionalToolbars.push_back(recToolbar);
      optionalToolbars.push_back(syncToolbar);
      optionalToolbars.push_back(posToolbar);
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
      menu_file->addAction(fileSaveRevisionAction);
      menu_file->addAction(fileSaveAsNewProjectAction);
      menu_file->addAction(fileSaveAsTemplateAction);
      menu_file->addSeparator();
      menu_file->addAction(fileCloseAction);
      menu_file->addSeparator();
      menu_file->addAction(editSongInfoAction);
      menu_file->addSeparator();
      menu_file->addAction(fileImportMidiAction);
      menu_file->addAction(fileExportMidiAction);
      menu_file->addAction(fileExportMidiSelectedVisibleAction);
      menu_file->addSeparator();
      menu_file->addAction(fileImportPartAction);
      menu_file->addAction(fileExportSelectedPartsAction);
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
      menuView->addSeparator();
//      menuView->addAction(viewArrangerAction);
//      menuView->addMenu(master);
      menuView->addAction(masterGraphicAction);
      menuView->addAction(masterListAction);
      menuView->addAction(viewMarkerAction);
      menuView->addAction(viewCliplistAction);
      menuView->addSeparator();
      menuView->addAction(toggleDocksAction);
      menuView->addAction(fullscreenAction);
      
      //-------------------------------------------------------------
      //    popup Midi
      //-------------------------------------------------------------

      menu_functions = new QMenu(tr("&Midi"), this);
      menuBar()->addMenu(menu_functions);
      trailingMenus.push_back(menu_functions);

      menu_functions->addAction(midiEditInstAction);

      menu_functions->addSeparator();
//      menu_functions->addMenu(midiInputPlugins);
      menu_functions->addAction(midiTrpAction);
      menu_functions->addAction(midiInputTrfAction);
      menu_functions->addAction(midiInputFilterAction);
      menu_functions->addAction(midiRemoteAction);
//      midiInputPlugins->addAction(midiTrpAction);
//      midiInputPlugins->addAction(midiInputTrfAction);
//      midiInputPlugins->addAction(midiInputFilterAction);
//      midiInputPlugins->addAction(midiRemoteAction);
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
// REMOVE Tim. automation. Removed.
// Deprecated. MusEGlobal::automation is now fixed TRUE
//   for now until we decide what to do with it.
//       menu_audio->addAction(autoMixerAction);
      //menu_audio->addSeparator();
      menu_audio->addAction(autoSnapshotAction);
      menu_audio->addAction(autoClearAction);

      //-------------------------------------------------------------
      //    popup Windows
      //-------------------------------------------------------------

      menuWindows = new QMenu(tr("&Windows"), this);
      menuBar()->addMenu(menuWindows);
      trailingMenus.push_back(menuWindows);

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
      follow->addActions(followAG->actions());
      menuSettings->addAction(rewindOnStopAction);
      menuSettings->addAction(settingsMetronomeAction);
      menuSettings->addSeparator();
      menuSettings->addAction(settingsMidiSyncAction);
      menuSettings->addAction(settingsMidiIOAction);
      menuSettings->addAction(settingsMidiPortAction);

      //---------------------------------------------------
      //    popup Help
      //---------------------------------------------------

      menuHelp = new QMenu(tr("&Help"), this);
      menuBar()->addMenu(menuHelp);
      trailingMenus.push_back(menuHelp);

      menuHelp->addAction(helpManualAction);
      menuHelp->addAction(whatsthis);
      menuHelp->addAction(helpHomepageAction);
      menuHelp->addAction(helpDidYouKnow);
      menuHelp->addSeparator();
      menuHelp->addAction(helpReportAction);
      menuHelp->addAction(helpSnooperAction);
      menuHelp->addSeparator();
      menuHelp->addAction(helpAboutAction);

      menuHelp->addAction(tr("About &Qt..."), qApp, SLOT(aboutQt()));

      //---------------------------------------------------
      //    Central Widget
      //---------------------------------------------------


      mdiArea=new MuseMdiArea(this);
//      mdiArea->setOption(QMdiArea::DontMaximizeSubWindowOnActivation);
      mdiArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
      mdiArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
      mdiArea->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);

      mdiArea->setViewMode(QMdiArea::TabbedView);
      mdiArea->setTabsMovable(true);
      mdiArea->setTabPosition(QTabWidget::South);
      QTabBar* tb = mdiArea->findChild<QTabBar*>();
      if (tb) {
          tb->setExpanding(false);
//          tb->setAutoHide(true);
      }
//      viewArrangerAction->setEnabled(false);

      setCentralWidget(mdiArea);

      arrangerView = new MusEGui::ArrangerView(this);
//      connect(arrangerView, SIGNAL(closed()), SLOT(arrangerClosed()));
      toplevels.push_back(arrangerView);
//      arrangerView->hide();
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
        for (int i = 0; i < MusEGlobal::config.recentListLength; ++i)
        {
          if (projFile.atEnd()) {
            break;
          }
          projectRecentList.append(projFile.readLine().simplified());
        }
      }

      transport = new MusEGui::Transport(this, "transport");
      bigtime   = nullptr;

      MusEGlobal::song->blockSignals(false);

      if (MusEGlobal::config.geometryMain.size().width()) {
          resize(MusEGlobal::config.geometryMain.size());
          move(MusEGlobal::config.geometryMain.topLeft());
      }
      else
          centerAndResize();

      setAndAdjustFonts();

      MusEGlobal::song->update();
      updateWindowMenu();
}

MusE::~MusE()
{
  DEBUG_LOADING_AND_CLEARING(stderr, "~MusE:%p\n", this);

#ifndef USE_SENDPOSTEDEVENTS_FOR_TOPWIN_CLOSE
  // This destructor is called before the TopWin destructors (which seem to be called with deleteLate()).
  // So we need to manually disconnect them now, otherwise by the time we are notified of their destruction
  //  in MusE::objectDestroyed(), MusE has already been destroyed, causing crashes!
  for(ObjectDestructions::ConstIterator it = _pendingObjectDestructions.constBegin();
      it != _pendingObjectDestructions.constEnd(); ++it)
    disconnect(it.value()._conn);
  _pendingObjectDestructions.clear();
#endif
}

//---------------------------------------------------------
//   setAndAdjustFonts
//---------------------------------------------------------
void MusE::setAndAdjustFonts() {

    ensurePolished();
    MusEGlobal::config.fonts[0].setFamily(font().family());
    MusEGlobal::config.fonts[0].setPointSize(font().pointSize());
    MusEGlobal::config.fonts[0].setBold(font().bold());
    MusEGlobal::config.fonts[0].setItalic(font().italic());

    // init font family with system font (the original sans-serif default looked terrible on KDE)
    for (int i = 1; i < NUM_FONTS; i++) {
        if (MusEGlobal::config.fonts[i].family().isEmpty())
            MusEGlobal::config.fonts[i].setFamily(font().family());
    }

    if (MusEGlobal::config.autoAdjustFontSize) {
        int fs = font().pointSize();
        MusEGlobal::config.fonts[1].setPointSize(qRound(fs * MusEGlobal::FntFac::F1));
        MusEGlobal::config.fonts[2].setPointSize(qRound(fs * MusEGlobal::FntFac::F2));
        MusEGlobal::config.fonts[3].setPointSize(qRound(fs * MusEGlobal::FntFac::F3));
        MusEGlobal::config.fonts[4].setPointSize(qRound(fs * MusEGlobal::FntFac::F4));
        MusEGlobal::config.fonts[5].setPointSize(qRound(fs * MusEGlobal::FntFac::F5));
        MusEGlobal::config.fonts[6].setPointSize(qRound(fs * MusEGlobal::FntFac::F6));
    }
}

//---------------------------------------------------------
//   centerAndResize
//---------------------------------------------------------

void MusE::centerAndResize() {

    // set sensible initial sizes/positions for mainwin/transport (kybos)

// Class QDesktopWidget deprecated as of Qt 5.11
#if QT_VERSION >= 0x050b00
    const QRect screenRect = qApp->primaryScreen()->availableGeometry();
#else
    const QRect screenRect = qApp->desktop()->availableGeometry();
#endif
    const QSize screenSize = screenRect.size();
    int width = screenSize.width();
    int height = screenSize.height();
    width *= 0.9; // 90% of the screen size
    height *= 0.9; // 90% of the screen size
    const QSize newSize( width, height );

    setGeometry( QStyle::alignedRect(
                     Qt::LeftToRight,
                     Qt::AlignCenter,
                     newSize,
                     screenRect
                     )
               );

    MusEGlobal::config.geometryMain = geometry();

    if (MusEGlobal::config.transportVisible) {
        QRect r( geometry().x() + (width / 2),
                 geometry().y() + (height / 10),
                 0, 0);
        MusEGlobal::config.geometryTransport = r;
        // don't position the window here, it's done when file/template is loaded
    }
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
    if (cpuLoadToolbar->isVisible())
        cpuLoadToolbar->setValues(MusEGlobal::song->cpuLoad(),
                                  MusEGlobal::song->dspLoad(),
                                  MusEGlobal::song->xRunsCount());

    if (statusBar()->isVisible())
        cpuStatusBar->setValues(MusEGlobal::song->cpuLoad(),
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

void MusE::loadDefaultSong(const QString& filename_override, bool use_template, bool load_config)
{
  DEBUG_LOADING_AND_CLEARING(stderr, "MusE::loadDefaultSong: filename_override:%s use_template:%d load_config %d\n",
              filename_override.toLocal8Bit().constData(), use_template, load_config);

  QString name;
  bool useTemplate = false;
  bool loadConfig = true;
  if (!filename_override.isEmpty())
  {
        name = filename_override;
        useTemplate = use_template;
        loadConfig = load_config;
  }
  else if (MusEGlobal::config.startMode == 0) {
              name = !projectRecentList.isEmpty() ? projectRecentList.first() : MusEGui::getUniqueUntitledName();
        fprintf(stderr, "starting with last song %s\n", name.toLocal8Bit().constData());
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
        fprintf(stderr, "starting with template %s\n", name.toLocal8Bit().constData());
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
        fprintf(stderr, "starting with pre configured song %s\n", name.toLocal8Bit().constData());
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
      DEBUG_LOADING_AND_CLEARING(stderr, "MusE::loadProjectFile(name:%s)\n", name.toLocal8Bit().constData());

      loadProjectFile(name, false, false);
      }

#ifdef USE_SENDPOSTEDEVENTS_FOR_TOPWIN_CLOSE

bool MusE::loadProjectFile(const QString& name, bool songTemplate, bool doReadMidiPorts)
      {
      if(!progress)
          progress = new QProgressDialog(this);

      QString label = "Loading project " + QFileInfo(name).fileName();
      progress->setLabelText(label);
      // TESTED: ApplicationModal appears to be what we want.
      //         The progress dialog always appears on top of all windows including mixers and plugin UIs.
      //         And yet, when an incident dialog or message box appears during startup or song loading,
      //          it appears on top of the progress dialog. Seems to be the solution.
      progress->setWindowModality(Qt::ApplicationModal);
      progress->setCancelButton(nullptr);
      if (!songTemplate)
        progress->setMinimumDuration(0); // if we are loading a template it will probably be fast and we can wait before showing the dialog

      //
      // stop audio threads if running
      //
      progress->setValue(0);
      qApp->processEvents();
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
      qApp->processEvents();

      bool loadOk = loadProjectFile1(name, songTemplate, doReadMidiPorts);
      microSleep(100000);
      progress->setValue(90);

      qApp->processEvents();

      if (restartSequencer)
          seqStart();
        // REMOVE Tim. Persistent routes. TESTING.
        //MusEGlobal::audio->msgIdle(false);
      //MusEGlobal::song->connectPorts();

      arrangerView->updateVisibleTracksButtons();
      progress->setValue(100);

      qApp->processEvents();
      delete progress;
      progress = nullptr;

      // Prompt and send init sequences.
      MusEGlobal::audio->msgInitMidiDevices(false);

      if (MusEGlobal::song->getSongInfo().length()>0 && MusEGlobal::song->showSongInfoOnStartup()) {
          startSongInfo(false);
        }

      return loadOk;
      }

#else

bool MusE::loadProjectFile(const QString& name, bool songTemplate, bool doReadMidiPorts, bool *doRestartSequencer)
      {
      DEBUG_LOADING_AND_CLEARING(stderr, "MusE::loadProjectFile: name:%s songTemplate:%d doReadMidiPorts:%d _busyWithLoading:%d\n",
        name.toLocal8Bit().constData(), songTemplate, doReadMidiPorts, _busyWithLoading);

      // Are we already busy waiting for something while loading or closing another project?
      if(_busyWithLoading)
        return false;

      _busyWithLoading = true;

      if(!progress)
          progress = new QProgressDialog(this);

      QString label = "Loading project " + QFileInfo(name).fileName();
      progress->setLabelText(label);
      // TESTED: ApplicationModal appears to be what we want.
      //         The progress dialog always appears on top of all windows including mixers and plugin UIs.
      //         And yet, when an incident dialog or message box appears during startup or song loading,
      //          it appears on top of the progress dialog. Seems to be the solution.
      progress->setWindowModality(Qt::ApplicationModal);
      progress->setCancelButton(nullptr);
      if (!songTemplate)
        progress->setMinimumDuration(0); // if we are loading a template it will probably be fast and we can wait before showing the dialog

      //
      // stop audio threads if running
      //
      progress->setValue(0);
      qApp->processEvents();
      bool restartSequencer = MusEGlobal::audio->isRunning();
      if(doRestartSequencer)
        *doRestartSequencer = restartSequencer;
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
      qApp->processEvents();

      const bool loadOk = loadProjectFile1(name, songTemplate, doReadMidiPorts);
      if(!loadOk)
      {
        // Clear these, they might not be empty.
        _pendingObjectDestructions.clear();
        _loadingFinishStructList.clear();
        finishLoadProjectFile(restartSequencer);
        return false;
      }

      // If there is nothing to wait for to be deleted, then just continue finishing.
      if(!_pendingObjectDestructions.hasWaitingObjects())
      {
        // Should already be clear, but just in case.
        _loadingFinishStructList.clear();
        finishLoadProjectFile(restartSequencer);
      }
      else
      {
        _loadingFinishStructList.append(MusE::LoadingFinishStruct(
          MusE::LoadingFinishStruct::LoadProjectFile,
          (restartSequencer ? MusE::LoadingFinishStruct::RestartSequencer : MusE::LoadingFinishStruct::NoFlag)));
      }

      return true;
      }

#endif

#ifndef USE_SENDPOSTEDEVENTS_FOR_TOPWIN_CLOSE
bool MusE::finishLoadProjectFile(bool restartSequencer)
      {
      DEBUG_LOADING_AND_CLEARING(stderr, "MusE::finishLoadProjectFile: restartSequencer:%d\n", restartSequencer);

      microSleep(100000);
      progress->setValue(90);

      qApp->processEvents();

      if (restartSequencer)
          seqStart();
        // REMOVE Tim. Persistent routes. TESTING.
        //MusEGlobal::audio->msgIdle(false);
      //MusEGlobal::song->connectPorts();

      arrangerView->updateVisibleTracksButtons();
      progress->setValue(100);

      qApp->processEvents();
      delete progress;
      progress = nullptr;

      // Prompt and send init sequences.
      MusEGlobal::audio->msgInitMidiDevices(false);

      if (MusEGlobal::song->getSongInfo().length()>0 && MusEGlobal::song->showSongInfoOnStartup()) {
          startSongInfo(false);
        }

      _busyWithLoading = false;
      return true;
      }
#endif

//---------------------------------------------------------
//   loadProjectFile
//    load *.med, *.mid, *.kar
//
//    template - if true, load file but do not change
//                project name
//    doReadMidiPorts  - also read midi port configuration
//
//    returns false if aborted
//---------------------------------------------------------

#ifdef USE_SENDPOSTEDEVENTS_FOR_TOPWIN_CLOSE

bool MusE::loadProjectFile1(const QString& name, bool songTemplate, bool doReadMidiPorts)
      {
      if (!clearSong(doReadMidiPorts))  // Allow not touching things like midi ports.
            return false;

      if (mixer1)
            mixer1->clearAndDelete();
      if (mixer2)
            mixer2->clearAndDelete();
      _arranger->clear();      // clear track info

      MusEGlobal::recordAction->setChecked(false);

      progress->setValue(20);
      qApp->processEvents();

      QFileInfo fi(name);
      if (songTemplate)
      {
            if(!fi.isReadable()) {
                QMessageBox::critical(this, QString("MusE"),
                    tr("Cannot read template"));
                return false;
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

      _lastProjectFilePath = name;
      _lastProjectWasTemplate = songTemplate;
      _lastProjectLoadedConfig = doReadMidiPorts;

      QString ex = fi.completeSuffix().toLower();
      QString mex = ex.section('.', -1, -1);
      if((mex == "gz") || (mex == "bz2"))
        mex = ex.section('.', -2, -2);

      if (ex.isEmpty() || mex == "med") {
            //
            //  read *.med file
            //
            MusEFile::File f(fi.filePath(), QString(".med"), this);
            const bool isCompressed = f.isCompressed();

            const MusEFile::File::ErrorCode ecode = fileOpen(f, QIODevice::ReadOnly, this, true);
            if(ecode != MusEFile::File::NoError)
            {
              switch(ecode)
              {
                case MusEFile::File::NoError:
                break;

                case MusEFile::File::GeneralError:
                  // FIXME: TODO: Correct replacement for previous "if (errno == ENOENT)" ?
                  setConfigDefaults();
                break;

                default:
                  // FIXME: TODO: Correct replacement for previous "if (errno != ENOENT)" ?
                  QMessageBox::critical(this, QString("MusE"), tr("File open error"));
                  setUntitledProject();
                  _lastProjectFilePath = QString();
                break;
              }
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

                    // If it is a compressed file we cannot seek the stream, we must reopen it.
                    if(isCompressed)
                    {
                      f.close();
                      const MusEFile::File::ErrorCode ecode = fileOpen(f, QIODevice::ReadOnly, this, true);
                      if(ecode != MusEFile::File::NoError)
                      {
                        switch(ecode)
                        {
                          case MusEFile::File::NoError:
                          break;

                          case MusEFile::File::GeneralError:
                            // FIXME: TODO: Correct replacement for previous "if (errno == ENOENT)" ?
                            setConfigDefaults();
                          break;

                          default:
                            // FIXME: TODO: Correct replacement for previous "if (errno != ENOENT)" ?
                            QMessageBox::critical(this, QString("MusE"), tr("File open error"));
                            setUntitledProject();
                            _lastProjectFilePath = QString();
                          break;
                        }
                      }
                    }
                    else
                    {
                      // Be kind. Rewind.
                      f.reset();
                    }

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
                        "Caution: Accuracy and sound quality may vary with rate and settings.\n\n"
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

                  MusECore::Xml xml(f.iodevice());
                  read(xml, doReadMidiPorts, songTemplate);

                  // FIXME: TODO: Correct replacement for "ferror(f)" ?
                  const MusEFile::File::ErrorCode ecode = f.error();
                  const QString etxt = f.errorString();
                  f.close();
                  if (ecode != MusEFile::File::NoError) {
                        QMessageBox::critical(this, QString("MusE"),
                          tr("File read error") + QString(": ") + etxt);
                        setUntitledProject();
                        _lastProjectFilePath = QString();
                        }
                  }
            }
      else if (mex == "mid" || mex == "kar") {
            setConfigDefaults();
            if (!importMidi(name, false))
            {
                  setUntitledProject();
                  _lastProjectFilePath = QString();
            }
            }
      else {
            QMessageBox::critical(this, QString("MusE"),
               tr("Unknown File Format: %1").arg(ex));
            setUntitledProject();
            _lastProjectFilePath = QString();
            }
      if (!songTemplate) {
            addProjectToRecentList(project.absoluteFilePath());
            setWindowTitle(projectTitle(project.absoluteFilePath()));
            }

      for (const auto& it : toplevels) {
          if (it->isMdiWin() && it->type() == TopWin::ARRANGER) {
              mdiArea->setActiveSubWindow(it->getMdiWin());
              break;
          }
      }

      MusEGlobal::song->dirty = false;
      progress->setValue(30);
      qApp->processEvents();

      viewTransportAction->setChecked(MusEGlobal::config.transportVisible);
      viewBigtimeAction->setChecked(MusEGlobal::config.bigTimeVisible);
      viewMarkerAction->setChecked(MusEGlobal::config.markerVisible);
//      viewArrangerAction->setChecked(MusEGlobal::config.arrangerVisible);

// REMOVE Tim. automation. Removed.
// Deprecated. MusEGlobal::automation is now fixed TRUE
//   for now until we decide what to do with it.
//       autoMixerAction->setChecked(MusEGlobal::automation);

      showBigtime(MusEGlobal::config.bigTimeVisible);

      // NOTICE! Mixers may set their own maximum size according to their content, on SongChanged.
      //         Therefore if the mixer is ALREADY OPEN, it may have a maximum size imposed on it,
      //          which may be SMALLER than any new size we might try to set after this.
      //         So we MUST RESET maximum size now, BEFORE attempts to set size. As per docs:
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

// Loading a file should not manipulate the geometry of the main window (kybos)
//      resize(MusEGlobal::config.geometryMain.size());
//      move(MusEGlobal::config.geometryMain.topLeft());

      transport->move(MusEGlobal::config.geometryTransport.topLeft());
      showTransport(MusEGlobal::config.transportVisible);

      progress->setValue(40);
      qApp->processEvents();

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
      qApp->processEvents();

      // Moved here from above due to crash with a song loaded and then File->New.
      // Marker view list was not updated, had non-existent items from marker list (cleared in ::clear()).
      showMarker(MusEGlobal::config.markerVisible);

      return true;
      }

#else

bool MusE::loadProjectFile1(const QString& name, bool songTemplate, bool doReadMidiPorts)
      {
      DEBUG_LOADING_AND_CLEARING(stderr, "MusE::loadProjectFile1: name:%s songTemplate:%d doReadMidiPorts:%d\n",
        name.toLocal8Bit().constData(), songTemplate, doReadMidiPorts);

      const bool isOk = clearSong(doReadMidiPorts);  // Allow not touching things like midi ports.
      if(!isOk)
        return false;

      // If there is nothing to wait for to be deleted, then just continue finishing.
      if(!_pendingObjectDestructions.hasWaitingObjects())
      {
        // Should already be clear, but just in case.
        _loadingFinishStructList.clear();
        finishLoadProjectFile1(name, songTemplate, doReadMidiPorts);
      }
      else
      {
        _loadingFinishStructList.append(MusE::LoadingFinishStruct(
          MusE::LoadingFinishStruct::LoadProjectFile1,
          (songTemplate ? MusE::LoadingFinishStruct::SongTemplate : MusE::LoadingFinishStruct::NoFlag) |
          (doReadMidiPorts ? MusE::LoadingFinishStruct::DoReadMidiPorts : MusE::LoadingFinishStruct::NoFlag),
          name));
      }
      return true;
      }

#endif

#ifndef USE_SENDPOSTEDEVENTS_FOR_TOPWIN_CLOSE

bool MusE::finishLoadProjectFile1(const QString& name, bool songTemplate, bool doReadMidiPorts)
      {
      DEBUG_LOADING_AND_CLEARING(stderr, "MusE::finishLoadProjectFile1: name:%s songTemplate:%d doReadMidiPorts:%d\n",
        name.toLocal8Bit().constData(), songTemplate, doReadMidiPorts);

      MusEGlobal::recordAction->setChecked(false);

      progress->setValue(20);
      qApp->processEvents();

      QFileInfo fi(name);
      if (songTemplate)
      {
            if(!fi.isReadable()) {
                QMessageBox::critical(this, QString("MusE"),
                    tr("Cannot read template"));
                return false;
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

      _lastProjectFilePath = name;
      _lastProjectWasTemplate = songTemplate;
      _lastProjectLoadedConfig = doReadMidiPorts;

      QString ex = fi.completeSuffix().toLower();
      QString mex = ex.section('.', -1, -1);
      if((mex == "gz") || (mex == "bz2"))
        mex = ex.section('.', -2, -2);

      if (ex.isEmpty() || mex == "med") {
            //
            //  read *.med file
            //
            MusEFile::File f(fi.filePath(), QString(".med"), this);
            const bool isCompressed = f.isCompressed();

            const MusEFile::File::ErrorCode ecode = fileOpen(f, QIODevice::ReadOnly, this, true);
            if(ecode != MusEFile::File::NoError)
            {
              switch(ecode)
              {
                case MusEFile::File::NoError:
                break;

                case MusEFile::File::GeneralError:
                  // FIXME: TODO: Correct replacement for previous "if (errno == ENOENT)" ?
                  setConfigDefaults();
                break;

                default:
                  // FIXME: TODO: Correct replacement for previous "if (errno != ENOENT)" ?
                  QMessageBox::critical(this, QString("MusE"), tr("File open error"));
                  setUntitledProject();
                  _lastProjectFilePath = QString();
                break;
              }
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
                    MusECore::Xml d_xml(f.iodevice());
                    MusECore::SongfileDiscovery d_list(MusEGlobal::museProject);
                    d_list.readSongfile(d_xml);

                    // If it is a compressed file we cannot seek the stream, we must reopen it.
                    if(isCompressed)
                    {
                      f.close();
                      const MusEFile::File::ErrorCode ecode = fileOpen(f, QIODevice::ReadOnly, this, true);
                      if(ecode != MusEFile::File::NoError)
                      {
                        switch(ecode)
                        {
                          case MusEFile::File::NoError:
                          break;

                          case MusEFile::File::GeneralError:
                            // FIXME: TODO: Correct replacement for previous "if (errno == ENOENT)" ?
                            setConfigDefaults();
                          break;

                          default:
                            // FIXME: TODO: Correct replacement for previous "if (errno != ENOENT)" ?
                            QMessageBox::critical(this, QString("MusE"), tr("File open error"));
                            setUntitledProject();
                            _lastProjectFilePath = QString();
                          break;
                        }
                      }
                    }
                    else
                    {
                      // Be kind. Rewind.
                      f.reset();
                    }

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
                        "Caution: Accuracy and sound quality may vary with rate and settings.\n\n"
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

                  MusECore::Xml xml(f.iodevice());
                  // NOTE: During loading, new items may be added to _pendingObjectDestructions.
                  //       They will be marked as not waiting for deletion so that they do not
                  //        interfere with items marked as waiting for deletion, during closing/loading.
                  read(xml, doReadMidiPorts, songTemplate);

                  // FIXME: TODO: Correct replacement for "ferror(f)" ?
                  const MusEFile::File::ErrorCode ecode = f.error();
                  const QString etxt = f.errorString();
                  f.close();
                  if (ecode != MusEFile::File::NoError) {
                        QMessageBox::critical(this, QString("MusE"),
                          tr("File read error") + QString(": ") + etxt);
                        setUntitledProject();
                        _lastProjectFilePath = QString();
                        }
                  }
            }
      else if (mex == "mid" || mex == "kar") {
            setConfigDefaults();
            if (!importMidi(name, false))
            {
                  setUntitledProject();
                  _lastProjectFilePath = QString();
            }
            }
      else {
            QMessageBox::critical(this, QString("MusE"),
               tr("Unknown File Format: %1").arg(ex));
            setUntitledProject();
            _lastProjectFilePath = QString();
            }
      if (!songTemplate) {
            addProjectToRecentList(project.absoluteFilePath());
            setWindowTitle(projectTitle(project.absoluteFilePath()));
            }

      for (const auto& it : toplevels) {
          if (it->isMdiWin() && it->type() == TopWin::ARRANGER) {
              mdiArea->setActiveSubWindow(it->getMdiWin());
              break;
          }
      }

      MusEGlobal::song->dirty = false;
      progress->setValue(30);
      qApp->processEvents();

      viewTransportAction->setChecked(MusEGlobal::config.transportVisible);
      viewBigtimeAction->setChecked(MusEGlobal::config.bigTimeVisible);
      viewMarkerAction->setChecked(MusEGlobal::config.markerVisible);
//      viewArrangerAction->setChecked(MusEGlobal::config.arrangerVisible);

// REMOVE Tim. automation. Removed.
// Deprecated. MusEGlobal::automation is now fixed TRUE
//   for now until we decide what to do with it.
//       autoMixerAction->setChecked(MusEGlobal::automation);

      showBigtime(MusEGlobal::config.bigTimeVisible);

      // NOTICE! Mixers may set their own maximum size according to their content, on SongChanged.
      //         Therefore if the mixer is ALREADY OPEN, it may have a maximum size imposed on it,
      //          which may be SMALLER than any new size we might try to set after this.
      //         So we MUST RESET maximum size now, BEFORE attempts to set size. As per docs:
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

// Loading a file should not manipulate the geometry of the main window (kybos)
//      resize(MusEGlobal::config.geometryMain.size());
//      move(MusEGlobal::config.geometryMain.topLeft());

      transport->move(MusEGlobal::config.geometryTransport.topLeft());
      showTransport(MusEGlobal::config.transportVisible);

      progress->setValue(40);
      qApp->processEvents();

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
      qApp->processEvents();

      // Moved here from above due to crash with a song loaded and then File->New.
      // Marker view list was not updated, had non-existent items from marker list (cleared in ::clear()).
      showMarker(MusEGlobal::config.markerVisible);

      return true;
      }
#endif

//---------------------------------------------------------
//   fileClose
//---------------------------------------------------------

#ifdef USE_SENDPOSTEDEVENTS_FOR_TOPWIN_CLOSE

void MusE::fileClose()
{
    // For now we just don't read the ports, leaving the last setup intact.
    const bool doReadMidiPorts = false;

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
    qApp->processEvents();

    // Allow not touching things like midi ports.
    const bool clearOk = clearSong(doReadMidiPorts);

    microSleep(100000);
    qApp->processEvents();

    if (restartSequencer)
        seqStart();
    if(!clearOk)
        return;

    MusEGlobal::recordAction->setChecked(false);

    //setConfigDefaults();
    QString name(MusEGui::getUniqueUntitledName());
    MusEGlobal::museProject = MusEGlobal::museProjectInitPath;
    //QDir::setCurrent(QDir::homePath());
    QDir::setCurrent(MusEGlobal::museProject);
    project.setFile(name);
    _lastProjectFilePath = QString();
    _lastProjectWasTemplate = false;
    _lastProjectLoadedConfig = true;

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

#else

void MusE::fileClose()
{
    DEBUG_LOADING_AND_CLEARING(stderr, "MusE::fileClose: _busyWithLoading:%d\n", _busyWithLoading);

    // Are we already busy waiting for something while loading or closing another project?
    if(_busyWithLoading)
      return;

    _busyWithLoading = true;

    // For now we just don't read the ports, leaving the last setup intact.
    const bool doReadMidiPorts = false;

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
    qApp->processEvents();

    // Allow not touching things like midi ports.
    const bool clearOk = clearSong(doReadMidiPorts);

    microSleep(100000);
    qApp->processEvents();

    if(!clearOk)
    {
      if (restartSequencer)
        seqStart();
      _busyWithLoading = false;
      return;
    }

    // If there is nothing to wait for to be deleted, then just continue finishing.
    if(!_pendingObjectDestructions.hasWaitingObjects())
    {
      // Should already be clear, but just in case.
      _loadingFinishStructList.clear();
      finishFileClose(restartSequencer);
    }
    else
    {
      _loadingFinishStructList.append(MusE::LoadingFinishStruct(
        MusE::LoadingFinishStruct::FileClose,
        (restartSequencer ? MusE::LoadingFinishStruct::RestartSequencer : MusE::LoadingFinishStruct::NoFlag)));
    }
}

#endif

#ifndef USE_SENDPOSTEDEVENTS_FOR_TOPWIN_CLOSE
void MusE::finishFileClose(bool restartSequencer)
{
    DEBUG_LOADING_AND_CLEARING(stderr, "MusE::finishFileClose: restartSequencer:%d\n", restartSequencer);

    microSleep(100000);
    qApp->processEvents();

    if (restartSequencer)
        seqStart();

    MusEGlobal::recordAction->setChecked(false);

    //setConfigDefaults();
    const QString name(MusEGui::getUniqueUntitledName());
    MusEGlobal::museProject = MusEGlobal::museProjectInitPath;
    //QDir::setCurrent(QDir::homePath());
    QDir::setCurrent(MusEGlobal::museProject);
    project.setFile(name);
    _lastProjectFilePath = QString();
    _lastProjectWasTemplate = false;
    _lastProjectLoadedConfig = true;

    setWindowTitle(projectTitle(name));

    //writeTopwinState=true;

    MusEGlobal::song->dirty = false;

    // Inform the rest of the app the song changed, with all flags.
    MusEGlobal::song->update(SC_EVERYTHING);
    MusEGlobal::song->updatePos();
    arrangerView->clipboardChanged(); // enable/disable "Paste"
    arrangerView->selectionChanged(); // enable/disable "Copy" & "Paste"
    arrangerView->scoreNamingChanged(); // inform the score menus about the new scores and their names

    _busyWithLoading = false;
}
#endif

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
//   MusE::loadProject
//---------------------------------------------------------

void MusE::loadProject()
      {
      DEBUG_LOADING_AND_CLEARING(stderr, "MusE::loadProject: _busyWithLoading:%d\n", _busyWithLoading);

      // Are we already busy waiting for something while loading or closing another project?
      if(_busyWithLoading)
        return;

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

#ifdef USE_SENDPOSTEDEVENTS_FOR_TOPWIN_CLOSE

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

#else

void MusE::loadTemplate()
      {
      DEBUG_LOADING_AND_CLEARING(stderr, "MusE::loadTemplate: _busyWithLoading:%d\n", _busyWithLoading);

      // Are we already busy waiting for something while loading or closing another project?
      if(_busyWithLoading)
        return;

      bool doReadMidiPorts;
      const QString fn = MusEGui::getOpenFileName(QString("templates"), MusEGlobal::med_file_pattern, this,
                                               tr("MusE: load template"), &doReadMidiPorts, MusEGui::MFileDialog::GLOBAL_VIEW);
      if (fn.isEmpty())
        return;

      bool restartSequencer = false;
      const bool isOk = loadProjectFile(fn, true, doReadMidiPorts, &restartSequencer);
      if (!isOk)
        return;

      // If there is nothing to wait for to be deleted, then just continue finishing.
      if(!_pendingObjectDestructions.hasWaitingObjects())
      {
        // Should already be clear, but just in case.
        _loadingFinishStructList.clear();
        finishLoadTemplate();
      }
      else
      {
        _loadingFinishStructList.append(MusE::LoadingFinishStruct(
          MusE::LoadingFinishStruct::LoadTemplate));
      }
      }

#endif

#ifndef USE_SENDPOSTEDEVENTS_FOR_TOPWIN_CLOSE
void MusE::finishLoadTemplate()
{
  DEBUG_LOADING_AND_CLEARING(stderr, "MusE::finishLoadTemplate\n");

  setUntitledProject();
}
#endif

//---------------------------------------------------------
//   loadDefaultTemplate
//---------------------------------------------------------

#ifdef USE_SENDPOSTEDEVENTS_FOR_TOPWIN_CLOSE

void MusE::loadDefaultTemplate()
{
    bool isOk = loadProjectFile(MusEGlobal::museGlobalShare + QString("/templates/default.med"), true, false);

    if (isOk)
      setUntitledProject();
}

#else

void MusE::loadDefaultTemplate()
{
    DEBUG_LOADING_AND_CLEARING(stderr, "MusE::loadDefaultTemplate: _busyWithLoading:%d\n", _busyWithLoading);

    // Are we already busy waiting for something while loading or closing another project?
    if(_busyWithLoading)
      return;

    const QString fn(MusEGlobal::museGlobalShare + QString("/templates/default.med"));
    bool restartSequencer = false;
    const bool isOk = loadProjectFile(fn, true, false, &restartSequencer);
    if (!isOk)
      return;

    // If there is nothing to wait for to be deleted, then just continue finishing.
    if(!_pendingObjectDestructions.hasWaitingObjects())
    {
      // Should already be clear, but just in case.
      _loadingFinishStructList.clear();
      finishLoadDefaultTemplate();
    }
    else
    {
      _loadingFinishStructList.append(MusE::LoadingFinishStruct(
        MusE::LoadingFinishStruct::LoadDefaultTemplate));
    }
}

#endif

#ifndef USE_SENDPOSTEDEVENTS_FOR_TOPWIN_CLOSE
void MusE::finishLoadDefaultTemplate()
{
    DEBUG_LOADING_AND_CLEARING(stderr, "MusE::finishLoadDefaultTemplate\n");

    setUntitledProject();
}
#endif

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
            //backupCommand.sprintf("cp \"%s\" \"%s.backup\"", name.toLocal8Bit().constData(), name.toLocal8Bit().constData());
            }
      else if (QFile::exists(name + QString(".med"))) {
            QString currentName2(name+".med");
            currentName.copy(name+".med.backup");
            //backupCommand.sprintf("cp \"%s.med\" \"%s.med.backup\"", name.toLocal8Bit().constData(), name.toLocal8Bit().constData());
            }
//      if (!backupCommand.isEmpty())
//            system(backupCommand.toLocal8Bit().constData());

      MusEFile::File f(name, QString(".med"), this);
      MusEFile::File::ErrorCode res = MusEGui::fileOpen(f, QIODevice::WriteOnly, this, false, overwriteWarn);
      if (res != MusEFile::File::NoError)
            return false;
      MusECore::Xml xml(f.iodevice());
      write(xml, writeTopwins);
      if (f.error() != MusEFile::File::NoError) {
            QString s = "Write File\n" + name + "\nfailed: "
               + f.errorString();
            QMessageBox::critical(this,
               tr("MusE: Write File failed"), s);
            f.close();
            // REMOVE Tim. tmp. FIXME TODO: What is the Qt equivalent of this? Why unlink?
            unlink(name.toLocal8Bit().constData());
            return false;
            }
      else {
            f.close();
            MusEGlobal::song->dirty = false;
            setWindowTitle(projectTitle(project.absoluteFilePath()));
            saveIncrement = 0;
            setStatusBarText(tr("Project saved."), 600);
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
                                 tr("The current project contains unsaved data.\n"
                                    "Save current project?"),
                                 tr("&Save"), tr("&Discard"), tr("&Cancel"), 0, 2);
        if (n == 0) {
            if (!save())      // don't quit if save failed
            {
                setRestartingApp(false); // Cancel any restart.
                event->ignore();
                return;
            }
        }
        else if (n == 2)
        {
            setRestartingApp(false); // Cancel any restart.
            event->ignore();
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

    // Don't use saveGeoemetry for the main window: Qt has issues (data gets invalid)
    //    when both standard MusE and AppImage are used on the same PC
    //      QSettings settings;
    //      settings.setValue("MusE/geometry", saveGeometry());

    MusEGlobal::config.geometryMain = geometry();

    // must be done here as the close events of child windows are not always called on quit
    saveStateTopLevels();

    saveStateExtra();

    writeGlobalConfiguration();

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
        QFileInfo fi(filename);
        QDir d = fi.dir();
        d.remove(filename);
        d.remove(fi.completeBaseName() + ".wca");
    }

    if(MusEGlobal::usePythonBridge)
    {
        fprintf(stderr, "Stopping MusE Pybridge...\n");
        if(!stopPythonBridge())
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
//   showMarker
//---------------------------------------------------------

void MusE::showMarker(bool flag)
{
    markerDock->setVisible(flag);
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

void MusE::saveAsNewProject()
{
  auto storedProject = project;
  project = QFileInfo();
  auto storedMusEProject = MusEGlobal::museProject;
  MusEGlobal::museProject = MusEGlobal::museProjectInitPath;
  saveAs(true);
  if (MusEGlobal::museProject == MusEGlobal::museProjectInitPath )
  {
    // change was rejected, restore the old project
    project = storedProject;
    MusEGlobal::museProject = storedMusEProject;
  }
}

void MusE::saveNewRevision()
{
  if (MusEGlobal::museProject == MusEGlobal::museProjectInitPath )
  {
    saveAs();
    return;
  }

  QString newFilePath = "";
  QString oldProjectFileName = project.filePath();
  MusEGui::SaveNewRevisionDialog newRevision(MusEGlobal::muse, project);

  newFilePath = newRevision.getNewRevision();

  if (newFilePath.isEmpty())
  {
    // could not set revision automatically, open dialog.
    newFilePath = newRevision.getNewRevisionWithDialog();
  }

  if (newFilePath.isEmpty())
    return;

  bool ok = save(newFilePath, true, writeTopwinState);
  if (ok)
  {
    project.setFile(newFilePath);
    _lastProjectFilePath = newFilePath;
    _lastProjectWasTemplate = false;
    _lastProjectLoadedConfig = true;
    setWindowTitle(projectTitle(project.absoluteFilePath()));

    // replace project in lastProjects
    if (projectRecentList.contains(oldProjectFileName))
      projectRecentList.removeAt(projectRecentList.indexOf(oldProjectFileName));
    addProjectToRecentList(newFilePath);

    project.setFile(newFilePath);
  }
}
//---------------------------------------------------------
//   saveAs
//---------------------------------------------------------

bool MusE::saveAs(bool overrideProjectSaveDialog)
{
  QString name;
  // if this is the initial save and ProjectDialog is enabled, use that.
  if (overrideProjectSaveDialog ||
      (MusEGlobal::config.useProjectSaveDialog && MusEGlobal::museProject == MusEGlobal::museProjectInitPath))
  {
    MusEGui::ProjectCreateImpl pci(MusEGlobal::muse);
    pci.setWriteTopwins(writeTopwinState);
    if (pci.exec() == QDialog::Rejected) {
      return false;
    }

    MusEGlobal::song->setSongInfo(pci.getSongInfo(), true);
    name = pci.getProjectPath();
    writeTopwinState=pci.getWriteTopwins();
  }
  else
  {
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
      _lastProjectFilePath = name;
      _lastProjectWasTemplate = false;
      _lastProjectLoadedConfig = true;
      setWindowTitle(projectTitle(project.absoluteFilePath()));
      addProjectToRecentList(name);
    }
    else
      MusEGlobal::museProject = tempOldProj;

    QDir::setCurrent(MusEGlobal::museProject);
  }

  return ok;
}
//---------------------------------------------------------
//   saveAsTemplate
//---------------------------------------------------------

void MusE::saveAsTemplate()
{
  QString templatesDir = MusEGlobal::configPath + QString("/") + "templates";

  printf ("templates dir %s\n", templatesDir.toLocal8Bit().data());

  QDir dirmanipulator;
  if (!dirmanipulator.mkpath(templatesDir)) {
    QMessageBox::warning(this,"Path error","Could not create templates directory", QMessageBox::Ok);
    return;
  }
  QString name;
  name = MusEGui::getSaveFileName(QString("templates"), MusEGlobal::med_file_save_pattern, this, tr("MusE: Save As"), &writeTopwinState, MFileDialog::USER_VIEW);
  if (name.isEmpty())
    return;

  auto finalPath = QFileInfo(name).absolutePath();
  if (!dirmanipulator.mkpath(finalPath)) {
    QMessageBox::warning(this,"Path error","Can't create final project path", QMessageBox::Ok);
    return;
  }
  save(name, true, false);
}
//---------------------------------------------------------
//   startEditor
//---------------------------------------------------------

void MusE::startEditor(MusECore::PartList* pl, int type)
      {
      switch (type) {
            case 0: startPianoroll(pl, true); break;
#ifdef MOVE_LISTEDIT_FROM_DOCK_TO_WINDOW_PULL1099
            case 1: startListEditor(pl, true); break;
#else
            case 1: startListEditor(pl); break;
#endif
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
      MusECore::PartList* pl = MusECore::getSelectedMidiParts();
      if (pl->empty()) {
            QMessageBox::critical(this, QString("MusE"), tr("Nothing to edit"));
            return nullptr;
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
 if (pl == nullptr)
    return;
 openInScoreEdit(destination, pl, allInOne);
}

void MusE::openInScoreEdit(MusEGui::ScoreEdit* destination, MusECore::PartList* pl, bool allInOne)
{
 if (destination==nullptr) // if no destination given, create a new one
 {
      destination = new MusEGui::ScoreEdit(this, nullptr, _arranger->cursorValue());
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
 openInScoreEdit_oneStaffPerTrack(nullptr);
}

//---------------------------------------------------------
//   startPianoroll
//---------------------------------------------------------

void MusE::startPianoroll(bool newwin)
{
    MusECore::PartList* pl = getMidiPartsToEdit();
    if (pl == nullptr)
        return;

    if (!filterInvalidParts(TopWin::PIANO_ROLL, pl))
        return;

    startPianoroll(pl, true, newwin);
}

MusEGui::PianoRoll* MusE::startPianoroll(MusECore::PartList* pl, bool showDefaultCtrls, bool newwin, bool *createdNotFound)
{
    if (!filterInvalidParts(TopWin::PIANO_ROLL, pl))
    {
        if(createdNotFound)
          *createdNotFound = false;
        return nullptr;
    }

    if (!newwin)
    {
      MusEGui::PianoRoll* pianoroll = static_cast<MusEGui::PianoRoll*>(findOpenEditor(TopWin::PIANO_ROLL, pl));
      if(pianoroll)
      {
        if(createdNotFound)
          *createdNotFound = false;
        return pianoroll;
      }
    }

    MusEGui::PianoRoll* pianoroll = new MusEGui::PianoRoll(pl, this, nullptr, _arranger->cursorValue(), showDefaultCtrls);
    toplevels.push_back(pianoroll);
    pianoroll->setOpenInNewWin(newwin);
    pianoroll->show();
    connect(pianoroll, SIGNAL(isDeleting(MusEGui::TopWin*)), SLOT(toplevelDeleting(MusEGui::TopWin*)));
// REMOVE Tim. wave. Removed
//     connect(MusEGlobal::muse, SIGNAL(configChanged()), pianoroll, SLOT(configChanged()));
    updateWindowMenu();
    if(createdNotFound)
      *createdNotFound = true;
    return pianoroll;
}

bool MusE::filterInvalidParts(const TopWin::ToplevelType type, MusECore::PartList* pl) {

    for (auto it = pl->begin(); it != pl->end(); ) {
        if ((it->second->track()->type() == MusECore::Track::MIDI && type == TopWin::PIANO_ROLL)
                || (it->second->track()->type() == MusECore::Track::DRUM && type == TopWin::DRUM))
            it++;
        else
            it = pl->erase(it);
    }

    if (pl->empty()) {
          QMessageBox::critical(this, QString("MusE"), tr("No valid parts selected"));
          return false;
    }

    return true;
}

MusEGui::MidiEditor* MusE::findOpenEditor(const TopWin::ToplevelType type, MusECore::PartList* pl) {

    if (QGuiApplication::keyboardModifiers() & Qt::ControlModifier
            && QGuiApplication::keyboardModifiers() & Qt::AltModifier)
        return nullptr;

    for (const auto& it : toplevels) {
        if (it->type() != type)
            continue;

        MusEGui::MidiEditor* med = dynamic_cast<MusEGui::MidiEditor*>(it);
        if (med == nullptr)
            return nullptr;

        if(pl)
        {
          const MusECore::PartList* pl_tmp = med->parts();

          if (pl_tmp->size() != pl->size())
              continue;

          bool found = false;
          for (const auto& it_pl : *pl)
          {
              found = false;
              for (const auto& it_pl_tmp : *pl_tmp)
              {
                  if (it_pl.second->uuid() == it_pl_tmp.second->uuid())
                  {
                      found = true;
                      break;
                  }
              }
              if (!found)
                  break;
          }

          if (!found)
              continue;
        }

        med->setHScrollOffset(_arranger->cursorValue());

        if (it->isMdiWin())
            mdiArea->setActiveSubWindow(it->getMdiWin());
        else
            it->activateWindow();

        return med;
    }

    return nullptr;
}

//---------------------------------------------------------
//   startListenEditor
//---------------------------------------------------------

void MusE::startListEditor(bool newwin)
      {
      MusECore::PartList* pl = getMidiPartsToEdit();
      if (pl == nullptr)
            return;
      startListEditor(pl, newwin);
      }

void MusE::startListEditor(MusECore::PartList* pl, bool newwin)
{
    if (!newwin && findOpenListEditor(pl))
        return;

#ifndef MOVE_LISTEDIT_FROM_DOCK_TO_WINDOW_PULL1099
    QDockWidget* dock = new QDockWidget("List Editor", this);
#endif

//    dock->setAllowedAreas(Qt::BottomDockWidgetArea | Qt::RightDockWidgetArea);
    MusEGui::ListEdit* listEditor = new MusEGui::ListEdit(pl, this);

#ifdef MOVE_LISTEDIT_FROM_DOCK_TO_WINDOW_PULL1099
    listEditor->setOpenInNewWin(newwin);
    listEditor->show();
#else
    dock->setWidget(listEditor);
#endif

    {
        int bar1, bar2, xx;
        unsigned x;
        const auto p = pl->begin()->second;
        MusEGlobal::sigmap.tickValues(p->tick(), &bar1, &xx, &x);
        MusEGlobal::sigmap.tickValues(p->tick() + p->lenTick(), &bar2, &xx, &x);

#ifndef MOVE_LISTEDIT_FROM_DOCK_TO_WINDOW_PULL1099
        dock->setWindowTitle("Event List <" + p->name() + QString("> %1-%2").arg(bar1+1).arg(bar2+1));
#endif
    }

#ifndef MOVE_LISTEDIT_FROM_DOCK_TO_WINDOW_PULL1099
    dock->setObjectName(dock->windowTitle());
    addDockWidget(Qt::BottomDockWidgetArea, dock);
//    addTabbedDock(Qt::BottomDockWidgetArea, dock);
    dock->setAttribute(Qt::WA_DeleteOnClose);
#endif

// REMOVE Tim. wave. Removed.
//     connect(MusEGlobal::muse,SIGNAL(configChanged()), listEditor, SLOT(configChanged()));
}

MusEGui::ListEdit* MusE::findOpenListEditor(MusECore::PartList* pl) {

    if (QGuiApplication::keyboardModifiers() & Qt::ControlModifier
            && QGuiApplication::keyboardModifiers() & Qt::AltModifier)
        return nullptr;

    const unsigned int pl_sz = pl->size();
    if(pl_sz == 0)
        return nullptr;

    for (const auto& d : findChildren<QDockWidget*>()) {
        if (strcmp(d->widget()->metaObject()->className(), "MusEGui::ListEdit") != 0)
            continue;

        MusEGui::ListEdit* le = static_cast<MusEGui::ListEdit*>(d->widget());
        const MusECore::PartList* pl_tmp = le->parts();

        MusECore::ciPart ip = pl->cbegin();
        for(; ip != pl->cend(); ++ip)
        {
          MusECore::ciPart ip_tmp = pl_tmp->cbegin();
          for(; ip_tmp != pl_tmp->cend(); ++ip_tmp)
          {
            if(ip_tmp->second->uuid() == ip->second->uuid())
              break;
          }
          if(ip_tmp == pl_tmp->cend())
            break;
        }
        if(ip != pl->cend())
          continue;

        if (!d->isVisible())
            toggleDocksAction->setChecked(true);
        d->raise();

        return le;
    }

    return nullptr;
}

//---------------------------------------------------------
//   startMasterEditor
//---------------------------------------------------------

MusEGui::MasterEdit* MusE::startMasterEditor(bool *createdNotFound)
{
    DEBUG_LOADING_AND_CLEARING(stderr, "MusE::startMasterEditor\n");

    MusEGui::MasterEdit* me = static_cast<MusEGui::MasterEdit*>(findOpenEditor(TopWin::MASTER));
    if(me)
    {
      if (me->isMdiWin())
          mdiArea->setActiveSubWindow(me->getMdiWin());
      else
          me->activateWindow();
      if(createdNotFound)
        *createdNotFound = false;
      return me;
    }
    else
    {
      me = new MusEGui::MasterEdit(this);
      toplevels.push_back(me);
      me->show();
      connect(me, SIGNAL(isDeleting(MusEGui::TopWin*)), SLOT(toplevelDeleting(MusEGui::TopWin*)));
      // Already done in master edit.
      //connect(MusEGlobal::muse, SIGNAL(configChanged()), me, SLOT(configChanged()));
      updateWindowMenu();
      if(createdNotFound)
        *createdNotFound = true;
      return me;
    }
}

//---------------------------------------------------------
//   startLMasterEditor
//---------------------------------------------------------

void MusE::showMasterList(bool show)
{
    masterListDock->setVisible(show);
}

//---------------------------------------------------------
//   startDrumEditor
//---------------------------------------------------------

void MusE::startDrumEditor(bool newwin)
{
    MusECore::PartList* pl = getMidiPartsToEdit();
    if (pl == nullptr)
        return;

    if (!filterInvalidParts(TopWin::DRUM, pl))
        return;

    startDrumEditor(pl, true, newwin);
}

MusEGui::DrumEdit* MusE::startDrumEditor(MusECore::PartList* pl, bool showDefaultCtrls, bool newwin, bool *createdNotFound)
{
    if (!filterInvalidParts(TopWin::DRUM, pl))
    {
        if(createdNotFound)
          *createdNotFound = false;
        return nullptr;
    }

    if (!newwin)
    {
      MusEGui::DrumEdit* drumEditor = static_cast<MusEGui::DrumEdit*>(findOpenEditor(TopWin::DRUM, pl));
      if(drumEditor)
      {
        if(createdNotFound)
          *createdNotFound = false;
        return drumEditor;
      }
    }

    MusEGui::DrumEdit* drumEditor = new MusEGui::DrumEdit(pl, this, nullptr, _arranger->cursorValue(), showDefaultCtrls);
    toplevels.push_back(drumEditor);
    drumEditor->setOpenInNewWin(newwin);
    drumEditor->show();
    connect(drumEditor, SIGNAL(isDeleting(MusEGui::TopWin*)), SLOT(toplevelDeleting(MusEGui::TopWin*)));
// REMOVE Tim. wave. Removed
//     connect(MusEGlobal::muse, SIGNAL(configChanged()), drumEditor, SLOT(configChanged()));
    updateWindowMenu();
    if(createdNotFound)
      *createdNotFound = true;
    return drumEditor;
}

//---------------------------------------------------------
//   startWaveEditor
//---------------------------------------------------------

void MusE::startWaveEditor(bool newwin)
{
    MusECore::PartList* pl = MusECore::getSelectedWaveParts();
    if (pl->empty()) {
        QMessageBox::critical(this, QString("MusE"), tr("Nothing to edit"));
        return;
    }
    startWaveEditor(pl, newwin);
}

MusEGui::WaveEdit* MusE::startWaveEditor(MusECore::PartList* pl, bool newwin, bool *createdNotFound)
{
    if (! newwin)
    {
      MusEGui::WaveEdit *waveEditor = static_cast<MusEGui::WaveEdit*>(findOpenEditor(TopWin::WAVE, pl));
      if(waveEditor)
      {
        if(createdNotFound)
          *createdNotFound = false;
        return waveEditor;
      }
    }

    MusEGui::WaveEdit* waveEditor = new MusEGui::WaveEdit(pl, this);
    toplevels.push_back(waveEditor);
    waveEditor->show();
    waveEditor->setOpenInNewWin(newwin);
// REMOVE Tim. wave. Removed.
//     connect(MusEGlobal::muse, SIGNAL(configChanged()), waveEditor, SLOT(configChanged()));
    connect(waveEditor, SIGNAL(isDeleting(MusEGui::TopWin*)), SLOT(toplevelDeleting(MusEGui::TopWin*)));
    updateWindowMenu();
    if(createdNotFound)
      *createdNotFound = true;
    return waveEditor;
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

    std::random_device randomDevice;
    std::shuffle(didYouKnow.tipList.begin(), didYouKnow.tipList.end(), randomDevice);

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

void MusE::showClipList(bool show)
{
    clipListDock->setVisible(show);
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
        fprintf(stderr, "THIS SHOULD NEVER HAPPEN: id(%i) < recent len(%i) in MusE::selectProject!\n",
                id, MusEGlobal::config.recentListLength);
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
                activeTopWin=nullptr;
                emit activeTopWinChanged(nullptr);

                // focus the last activated topwin which is not the deleting one
                QList<QMdiSubWindow*> list = mdiArea->subWindowList(QMdiArea::StackingOrder);
                for (QList<QMdiSubWindow*>::const_reverse_iterator lit=list.rbegin(); lit!=list.rend(); lit++)
                    if ((*lit)->isVisible() && (*lit)->widget() != tl)
                    {
                        if (MusEGlobal::debugMsg)
                            fprintf(stderr, "bringing '%s' to front instead of closed window\n",(*lit)->widget()->windowTitle().toLocal8Bit().data());

                        bringToFront((*lit)->widget());

                        // If the TopWin has a canvas and a focusCanvas(), call it to focus the canvas.
                        TopWin* tw = dynamic_cast<TopWin*>( (*lit)->widget());
                        if(tw)
                          tw->focusCanvas();

                        break;
                    }
            }

            if (tl == currentMenuSharingTopwin)
                setCurrentMenuSharingTopwin(nullptr);

            toplevels.erase(i);

            if (tl->type() == MusEGui::TopWin::SCORE)
                arrangerView->updateScoreMenus();

            updateWindowMenu();
            return;
        }
    }
    fprintf(stderr, "topLevelDeleting: top level %p not found\n", tl);
}

//---------------------------------------------------------
//   kbAccel - Global keyboard accelerators
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
            else
                  MusEGlobal::song->setPlay(true);
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
              spos = MusEGlobal::sigmap.raster1(spos, _arranger->rasterVal());
            }
            if(spos < 0)
              spos = 0;
            MusECore::Pos p(spos,true);
            MusEGlobal::song->setPos(MusECore::Song::CPOS, p, true, true, true);
            return;
            }
      else if (key == MusEGui::shortcuts[MusEGui::SHRT_POS_INC].key) {
            // Nudge by +1, then snap up with raster2.
            int spos = MusEGlobal::sigmap.raster2(MusEGlobal::song->cpos() + 1, _arranger->rasterVal());
            MusECore::Pos p(spos,true);
            MusEGlobal::song->setPos(MusECore::Song::CPOS, p, true, true, true); //CDW
            return;
            }
      else if (key == MusEGui::shortcuts[MusEGui::SHRT_POS_DEC_NOSNAP].key) {
            int spos = MusEGlobal::song->cpos() - MusEGlobal::sigmap.rasterStep(MusEGlobal::song->cpos(),
              _arranger->rasterVal());
            if(spos < 0)
              spos = 0;
            MusECore::Pos p(spos,true);
            MusEGlobal::song->setPos(MusECore::Song::CPOS, p, true, true, true);
            return;
            }
      else if (key == MusEGui::shortcuts[MusEGui::SHRT_POS_INC_NOSNAP].key) {
            MusECore::Pos p(MusEGlobal::song->cpos() + MusEGlobal::sigmap.rasterStep(MusEGlobal::song->cpos(),
              _arranger->rasterVal()), true);
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
      else if (key == MusEGui::shortcuts[MusEGui::SHRT_TOGGLE_REWINDONSTOP].key) {
            rewindOnStopAction->activate(QAction::Trigger);
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
                  break;
            case CMD_FOLLOW_JUMP:
                  MusEGlobal::song->setFollow(MusECore::Song::JUMP);
                  break;
            case CMD_FOLLOW_CONTINUOUS:
                  MusEGlobal::song->setFollow(MusECore::Song::CONTINUOUS);
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
    _snooperDialog = nullptr;
  }
  if(metronomeConfig)
  {
    delete metronomeConfig;
    metronomeConfig = nullptr;
  }
  if(shortcutConfig)
  {
    delete shortcutConfig;
    shortcutConfig = nullptr;
  }
  if(midiSyncConfig)
  {
    delete midiSyncConfig;
    midiSyncConfig = nullptr;
  }
  if(midiFileConfig)
  {
    delete midiFileConfig;
    midiFileConfig = nullptr;
  }
  if(globalSettingsConfig)
  {
    delete globalSettingsConfig;
    globalSettingsConfig = nullptr;
  }

  destroy_function_dialogs();


  if(MusEGlobal::mitPluginTranspose)
  {
    delete MusEGlobal::mitPluginTranspose;
    MusEGlobal::mitPluginTranspose = nullptr;
  }

  if(midiInputTransform)
  {
    delete midiInputTransform;
    midiInputTransform = nullptr;
  }
  if(midiFilterConfig)
  {
     delete midiFilterConfig;
     midiFilterConfig = nullptr;
  }
  if(midiRemoteConfig)
  {
    delete midiRemoteConfig;
    midiRemoteConfig = nullptr;
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
    midiTransformerDialog = nullptr;
  }
  if(routeDialog)
  {
    delete routeDialog;
    routeDialog = nullptr;
  }

}

//---------------------------------------------------------
//   configAppearance
//---------------------------------------------------------

void MusE::configAppearance()
{
    if (!appearance) {
        // NOTE: For deleting parentless dialogs and widgets, please add them to MusE::deleteParentlessDialogs().
        appearance = new MusEGui::Appearance(this);
        appearance->resetValues();
    }

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

void MusE::bounceToTrack(MusECore::AudioOutput* ao)
      {
      if (MusEGlobal::audio->bounce())
        return;

      MusEGlobal::song->bounceOutput = nullptr;
      MusEGlobal::song->bounceTrack = nullptr;

      if(MusEGlobal::song->waves()->empty())
      {
        QMessageBox::critical(this,
            tr("MusE: Record Downmix to Track"),
            tr("No wave tracks found")
            );
        return;
      }

      MusECore::OutputList* ol = MusEGlobal::song->outputs();
      if(ol->empty())
      {
        QMessageBox::critical(this,
            tr("MusE: Record Downmix to Track"),
            tr("No audio output tracks found")
            );
        return;
      }

      if(checkRegionNotNull())
        return;

      MusECore::AudioOutput* out = nullptr;
      // If only one output, pick it, else pick the first selected.
      if (ao)
          out = ao;
      else if(ol->size() == 1)
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
              out = nullptr;
              break;
            }
            out = o;
          }
        }
        if(!out)
        {
          QMessageBox::critical(this,
              tr("MusE: Record Downmix to Track"),
              tr("Select one audio output track,\nand one target wave track")
              );
          return;
        }
      }

      // search target track
      MusECore::TrackList* tl = MusEGlobal::song->tracks();
      MusECore::WaveTrack* track = nullptr;

      for (MusECore::iTrack it = tl->begin(); it != tl->end(); ++it) {
            MusECore::Track* t = *it;
            if (t->selected()) {
                    if(t->type() != MusECore::Track::WAVE && t->type() != MusECore::Track::AUDIO_OUTPUT) {
                        track = nullptr;
                        break;
                    }
                    if(t->type() == MusECore::Track::WAVE)
                    {
                      if(track)
                      {
                        track = nullptr;
                        break;
                      }
                      track = (MusECore::WaveTrack*)t;
                    }

                  }
            }

      if (track == nullptr) {
          if(ol->size() == 1) {
            QMessageBox::critical(this,
               tr("MusE: Record Downmix to Track"),
               tr("Select one target wave track")
               );
            return;
          }
          else
          {
            QMessageBox::critical(this,
               tr("MusE: Record Downmix to Track"),
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
    MusEGlobal::song->bounceOutput = nullptr;
    MusEGlobal::song->bounceTrack = nullptr;
    if(!ao)
    {
        MusECore::OutputList* ol = MusEGlobal::song->outputs();
        if(ol->empty())
        {
            QMessageBox::critical(this,
                                  tr("MusE: Record Downmix to File"),
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
                        ao = nullptr;
                        break;
                    }
                    ao = o;
                }
            }
            if (ao == nullptr) {
                QMessageBox::critical(this,
                                      tr("MusE: Record Downmix to File"),
                                      tr("Select one audio output track")
                                      );
                return;
            }
        }
    }

    if (checkRegionNotNull())
        return;

    MusECore::SndFile* sf = MusECore::getSndFile(nullptr, this);
    if (sf == nullptr)
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
               tr("Render Downmix"),
               tr("Set left and right markers for downmix range")
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
          int ok = save (ss, false, true);
          if (ok) {
            project.setFile(ss);
            _lastProjectFilePath = ss;
            _lastProjectWasTemplate = false;
            _lastProjectLoadedConfig = true;
            setWindowTitle(tr("MusE: Song: %1").arg(MusEGui::projectTitleFromFilename(project.absoluteFilePath())));
            addProjectToRecentList(ss);
            MusEGlobal::museProject = QFileInfo(ss).absolutePath();
            QDir::setCurrent(MusEGlobal::museProject);
          }
          lash_send_event (lash_client, event);
        }
        break;

        case LASH_Restore_File:
        {
          /* load file */
          QString sr = QString(lash_event_get_string(event)) + QString("/lash-project-muse.med");
          loadProjectFile(sr, false, true);
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
//    return false if operation aborted
//    called with sequencer stopped
//    If clear_all is false, it will not touch things like midi ports.
//---------------------------------------------------------

#ifdef USE_SENDPOSTEDEVENTS_FOR_TOPWIN_CLOSE

bool MusE::clearSong(bool clear_all)
{
    if (MusEGlobal::song->dirty) {
        int n = 0;
        n = QMessageBox::warning(this, appName,
                                 tr("The current project contains unsaved data.\n"
                                    "Save current project before continuing?"),
                                 tr("&Save"), tr("&Discard"), tr("&Cancel"), 0, 2);
        switch (n) {
        case 0:
            if (!save())      // abort if save failed
                return false;
            break;
        case 1:
            break;
        case 2:
            return false;
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

    // We must make a working COPY of the top level list because some of their
    //  closeEvent() functions eventually call MusE::topLevelDeleting() which
    //  erases the top level from the list! So we have a delete-while-iterate
    //  situation but we don't have access to the erased iterator, and it's
    //  kind of murky whether they would even call topLevelDeleting().
    const MusEGui::ToplevelList tll = toplevels;
    for (const auto& i : tll) {
        MusEGui::TopWin* tl = i;
        switch (tl->type()) {
        case MusEGui::TopWin::ARRANGER:
            break;
        case MusEGui::TopWin::PIANO_ROLL:
        case MusEGui::TopWin::SCORE:
#ifdef MOVE_LISTEDIT_FROM_DOCK_TO_WINDOW_PULL1099
        case MusEGui::TopWin::LISTE:
#else
        //case MusEGui::TopWin::LISTE:
#endif
        case MusEGui::TopWin::DRUM:
        case MusEGui::TopWin::MASTER:
        case MusEGui::TopWin::WAVE:
        {
            if(tl->isVisible())   // Don't keep trying to close, only if visible.
            {
                if(!tl->close())
                {
                  fprintf(stderr, "MusE::clearSong TopWin:%p did not close! Waiting...\n", tl);
                  while(!tl->close())
                    qApp->processEvents();
                }
            }
        }
        case MusEGui::TopWin::TOPLEVELTYPE_LAST_ENTRY: //to avoid a warning
            break;
        }
    }

    // Just some useful info and previous tests and attempts...
    //
    // Are any of the windows marked as 'delete on close'?
    // Then we MUST wait until they delete, otherwise crashes will happen
    //  when the song is cleared due to still-active signal/slot connections
    //  and attempts by the various top levels to access non-existant tracks etc.
    // It turns out that close() will call deleteLater() - NOT delete!
    // Simply calling processEvents() several times did not let them delete.
    // From help on QCoreApplication::processEvents():
    // "In the event that you are running a local loop which calls this function continuously, without an event loop,
    //   the DeferredDelete events will not be processed. This can affect the behaviour of widgets, e.g. QToolTip,
    //   that rely on DeferredDelete events to function properly. An alternative would be to call sendPostedEvents()
    //   from within that local loop."

    //===================================================================================
    // From https://lists.qt-project.org/pipermail/qt-interest-old/2010-April/022513.html
    //
    // However, deleting a QObject with:
    //      someObj->deleteLater();
    //  and then doing:
    //      qApp->sendPostedEvents();
    //  does not result in 'someObj' being deleted. For that to happen, I have to explicitly call:
    //      qApp->sendPostedEvents(0, QEvent::DeferredDelete);
    //
    // Is this a bug or expected behavior?  The documentation of
    //  sendPostedEvents() would suggest that simply all events are dispatched,
    //  and does not state any exceptions (like DeferredDelete).
    //===================================================================================
    // TESTED: Yep. Apparently that's still true today. WTF?
    // Posted bug report. Immediate reply from Qt bugs:
    // "Not a bug. Deferred deletions depend on the loop nesting level and calling processEvents or sendPostedEvents
    //  directly will not cause those you've just posted to get processed. You need return."

    //qApp->sendPostedEvents();
    qApp->sendPostedEvents(nullptr, QEvent::DeferredDelete);

    closeDocks();

    microSleep(100000);
    _arranger->songIsClearing();
    MusEGlobal::song->clear(true, clear_all);
    microSleep(100000);
    return true;
}

#else

bool MusE::clearSong(bool clear_all)
{
    DEBUG_LOADING_AND_CLEARING(stderr, "MusE::clearSong: clear_all:%d _busyWithLoading:%d\n",
      clear_all, _busyWithLoading);

//     // Are we already busy waiting for something while loading or closing another project?
//     if(_busyWithLoading)
//       return false;

    if (MusEGlobal::song->dirty) {
        int n = 0;
        n = QMessageBox::warning(this, appName,
                                 tr("The current project contains unsaved data.\n"
                                    "Save current project before continuing?"),
                                 tr("&Save"), tr("&Discard"), tr("&Cancel"), 0, 2);
        switch (n) {
        case 0:
            if (!save())      // abort if save failed
                return false;
            break;
        case 1:
            break;
        case 2:
            return false;
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

    // Mark all objects currently in the list as waiting to be deleted.
    // NOTE: During loading, new items may be added to _pendingObjectDestructions.
    // They will be marked as not waiting for deletion so that they do not
    //  interfere with items marked as waiting for deletion, during closing/loading.
    const bool hasWaitingObjects =_pendingObjectDestructions.markAll(true);

    // Are there objects that we must wait for to be deleted? Append a loading finish structure.
    if(hasWaitingObjects)
    {
      _loadingFinishStructList.append(MusE::LoadingFinishStruct(
        MusE::LoadingFinishStruct::ClearSong,
        clear_all ? MusE::LoadingFinishStruct::ClearAll : MusE::LoadingFinishStruct::NoFlag));
    }

    //====================================================================================
    // BEGIN Closing and/or deleting. Some deletions may be immediate, others delete later.
    //====================================================================================

    if (mixer1)
          mixer1->clearAndDelete();
    if (mixer2)
          mixer2->clearAndDelete();
    _arranger->clear();      // clear track info

    // We must make a working COPY of the top level list because some of their
    //  closeEvent() functions eventually call MusE::topLevelDeleting() which
    //  erases the top level from the list! So we have a delete-while-iterate
    //  situation but we don't have access to the erased iterator, and it's
    //  kind of murky whether they would even call topLevelDeleting().
    const MusEGui::ToplevelList tll = toplevels;
    for (const auto& i : tll) {
        MusEGui::TopWin* tl = i;
        switch (tl->type()) {
        case MusEGui::TopWin::ARRANGER:
            break;
        case MusEGui::TopWin::PIANO_ROLL:
        case MusEGui::TopWin::SCORE:
#ifdef MOVE_LISTEDIT_FROM_DOCK_TO_WINDOW_PULL1099
        case MusEGui::TopWin::LISTE:
#else
        //case MusEGui::TopWin::LISTE:
#endif
        case MusEGui::TopWin::DRUM:
        case MusEGui::TopWin::MASTER:
        case MusEGui::TopWin::WAVE:
        {
            if(tl->isVisible())   // Don't keep trying to close, only if visible.
            {
                DEBUG_LOADING_AND_CLEARING(stderr, "MusE::clearSong closing TopWin:%p <%s>\n",
                  tl, TopWin::typeName(tl->type()).toLocal8Bit().constData());

                if(!tl->close())
                {
                  // It is possible something held it up from closing.
                  fprintf(stderr, "MusE::clearSong TopWin:%p <%s> did not close! Waiting...\n",
                    tl, TopWin::typeName(tl->type()).toLocal8Bit().constData());
                  while(!tl->close())
                    qApp->processEvents();
                }
            }
        }
        case MusEGui::TopWin::TOPLEVELTYPE_LAST_ENTRY: //to avoid a warning
            break;
        }
    }
    //====================================================================================
    // END Closing and/or deleting.
    //====================================================================================

    // If there is nothing to wait for to be deleted, then just continue finishing.
    // Note it is possible some of the objects may have been already immediately deleted
    //  by closing the top wins, and they would have been removed from the list by our
    //  destroyed() handler, so we check again whether the list is empty.
    // If there WERE objects waiting, but NOW there are none, it means immediate deletions
    //  took place which emptied the list, which upon being emptied in destroyed() handler
    //  ALREADY called finishClearSong(), so do NOT continue finishing.
    const bool nowHasWaitingObjects = _pendingObjectDestructions.hasWaitingObjects();
    if(!nowHasWaitingObjects)
    {
      // Should already be clear, but just in case.
      _loadingFinishStructList.clear();
      if(!hasWaitingObjects)
        finishClearSong(clear_all);
    }

    // Just some useful info and previous tests and attempts...
    //
    // Are any of the windows marked as 'delete on close'?
    // Then we MUST wait until they delete, otherwise crashes will happen
    //  when the song is cleared due to still-active signal/slot connections
    //  and attempts by the various top levels to access non-existant tracks etc.
    // It turns out that close() will call deleteLater() - NOT delete!
    // Simply calling processEvents() several times did not let them delete.
    // From help on QCoreApplication::processEvents():
    // "In the event that you are running a local loop which calls this function continuously, without an event loop,
    //   the DeferredDelete events will not be processed. This can affect the behaviour of widgets, e.g. QToolTip,
    //   that rely on DeferredDelete events to function properly. An alternative would be to call sendPostedEvents()
    //   from within that local loop."

    //===================================================================================
    // From https://lists.qt-project.org/pipermail/qt-interest-old/2010-April/022513.html
    //
    // However, deleting a QObject with:
    //      someObj->deleteLater();
    //  and then doing:
    //      qApp->sendPostedEvents();
    //  does not result in 'someObj' being deleted. For that to happen, I have to explicitly call:
    //      qApp->sendPostedEvents(0, QEvent::DeferredDelete);
    //
    // Is this a bug or expected behavior?  The documentation of
    //  sendPostedEvents() would suggest that simply all events are dispatched,
    //  and does not state any exceptions (like DeferredDelete).
    //===================================================================================
    // TESTED: Yep. Apparently that's still true today. ???
    // Posted bug report. Immediate reply from Qt bugs:
    // "Not a bug. Deferred deletions depend on the loop nesting level and calling processEvents or sendPostedEvents
    //  directly will not cause those you've just posted to get processed. You need return."

//     //qApp->sendPostedEvents();
//     qApp->sendPostedEvents(nullptr, QEvent::DeferredDelete);
//
//     closeDocks();
//
//     microSleep(100000);
//     _arranger->songIsClearing();
//     MusEGlobal::song->clear(true, clear_all);
//     microSleep(100000);
    return true;
}

#endif

#ifndef USE_SENDPOSTEDEVENTS_FOR_TOPWIN_CLOSE
bool MusE::finishClearSong(bool clear_all)
{
    DEBUG_LOADING_AND_CLEARING(stderr, "MusE::finishClearSong\n");

    closeDocks();

    microSleep(100000);
    _arranger->songIsClearing();
    MusEGlobal::song->clear(true, clear_all);
    microSleep(100000);
    return true;

}
#endif

//---------------------------------------------------------
//   startEditInstrument
//---------------------------------------------------------

void MusE::startEditInstrument(const QString& find_instrument, EditInstrumentTabType show_tab)
    {
      if(editInstrument == nullptr)
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
      // Clear all rec event lists.
      MusEGlobal::song->clearRecAutomation();
      // Force re-enable all track and plugin controllers, and synth controllers if applicable.
      MusEGlobal::song->reenableTouchedControllers(true);

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

// REMOVE Tim. automation. Removed.
// Deprecated. MusEGlobal::automation is now fixed TRUE
//   for now until we decide what to do with it.
//       autoMixerAction->setChecked(MusEGlobal::automation);
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

      MusECore::Undo undo;

      const int frame = MusEGlobal::audio->curFramePos();
      MusECore::TrackList* tracks = MusEGlobal::song->tracks();
      for (MusECore::iTrack i = tracks->begin(); i != tracks->end(); ++i) {
            if ((*i)->isMidiTrack())
                  continue;
            MusECore::AudioTrack* track = static_cast<MusECore::AudioTrack*>(*i);
            MusECore::CtrlListList* cll = track->controller();

            for (MusECore::ciCtrlList icl = cll->cbegin(); icl != cll->cend(); ++icl) {
                  MusECore::CtrlList *cl = icl->second;
                  // Do not include hidden controller lists.
                  // It's OK if isVisible() is false - that's just whether it's being displayed.
                  if(cl->dontShow() /*|| !cl->isVisible()*/ )
                    continue;

                  // Need to update current 'manual' values from the automation values at this time.
                  if(track->automationType() != MusECore::AUTO_OFF) // && track->automationType() != MusECore::AUTO_WRITE)
                    cl->updateCurValue(frame);

                  const double val = cl->curVal();
                  // Add will replace if found.
                  // Here is a tough decision regarding choice of discrete vs. interpolated:
                  // Do we obey the discrete/interpolated toolbar button?
                  // Given the (now) reduced role of interpolated graphs, maybe best to force these points to discrete. (Tim)
                  undo.push_back(MusECore::UndoOp(MusECore::UndoOp::AddAudioCtrlVal,
                    track, double(cl->id()), double(frame), val, double(MusECore::CtrlVal::VAL_SELECTED)));
                    // The undo system automatically sets the VAL_DISCRETE flag if the controller mode is DISCRETE.
                    // | (MusEGlobal::config.audioAutomationDrawDiscrete ? MusECore::CtrlVal::VAL_DISCRETE : MusECore::CtrlVal::VAL_NOFLAGS)));
                  }
            }

      MusEGlobal::song->applyOperationGroup(undo);
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
      fileSaveAsNewProjectAction->setShortcut(MusEGui::shortcuts[MusEGui::SHRT_SAVE_AS_NEW_PROJECT].key);
      fileSaveRevisionAction->setShortcut(MusEGui::shortcuts[MusEGui::SHRT_SAVE_REVISION].key);
      fileSaveAsTemplateAction->setShortcut(MusEGui::shortcuts[MusEGui::SHRT_SAVE_AS_TEMPLATE].key);
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

// REMOVE Tim. automation. Removed.
// Deprecated. MusEGlobal::automation is now fixed TRUE
//   for now until we decide what to do with it.
//       autoMixerAction->setShortcut(MusEGui::shortcuts[MusEGui::SHRT_MIXER_AUTOMATION].key);
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
      toggleDocksAction->setShortcut(MusEGui::shortcuts[MusEGui::SHRT_TOGGLE_DOCKS].key);
      //rewindOnStopAction->setShortcut(MusEGui::shortcuts[MusEGui::SHRT_TOGGLE_REWINDONSTOP].key); moved to global shortcuts in MusE::kbAccel

      //arrangerView->updateMusEGui::Shortcuts(); //commented out by flo: is done via signal

      updateStatusBar();
}

//---------------------------------------------------------
//   showBigtime
//---------------------------------------------------------

void MusE::showBigtime(bool on)
{
    if (on && bigtime == nullptr) {
        bigtime = new MusEGui::BigTime(this);
        bigtime->setPos(0, MusEGlobal::song->cpos(), false);
        connect(MusEGlobal::song, SIGNAL(posChanged(int, unsigned, bool)), bigtime, SLOT(setPos(int, unsigned, bool)));
        connect(MusEGlobal::muse, SIGNAL(configChanged()), bigtime, SLOT(configChanged()));
        connect(MusEGlobal::song, SIGNAL(songChanged(MusECore::SongChangedStruct_t)), bigtime, SLOT(songChanged(MusECore::SongChangedStruct_t)));
        connect(bigtime, SIGNAL(closed()), SLOT(bigtimeClosed()));
    }
    if (bigtime) {
        bigtime->resize(MusEGlobal::config.geometryBigTime.size());
        bigtime->move(MusEGlobal::config.geometryBigTime.topLeft());
        bigtime->setVisible(on);
    }
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
    if (dockMixerA) {
        mixer1Dock->setVisible(on);
    } else {
        if (on && mixer1 == nullptr) {
            mixer1 = new MusEGui::AudioMixerApp(this, &(MusEGlobal::config.mixer1), false);
            connect(mixer1, SIGNAL(closed()), SLOT(mixer1Closed()));
            mixer1->setGeometry(MusEGlobal::config.mixer1.geometry);
        }
        if (mixer1)
            mixer1->setVisible(on);
    }

    viewMixerAAction->setChecked(on);
}


//---------------------------------------------------------
//   showMixer2
//---------------------------------------------------------

void MusE::showMixer2(bool on)
{
    if (dockMixerB) {
        mixer1Dock->setVisible(on);
    } else {
        if (on && mixer2 == nullptr) {
            mixer2 = new MusEGui::AudioMixerApp(this, &(MusEGlobal::config.mixer2), false);
            connect(mixer2, SIGNAL(closed()), SLOT(mixer2Closed()));
            mixer2->setGeometry(MusEGlobal::config.mixer2.geometry);
        }
        if (mixer2)
            mixer2->setVisible(on);
    }

    viewMixerBAction->setChecked(on);
}

//---------------------------------------------------------
//   toggleMixer1
//---------------------------------------------------------

void MusE::toggleMixer1(bool checked)
{
    if (dockMixerA)
        mixer1Dock->setVisible(checked);
    else
        showMixer1(checked);
}

//---------------------------------------------------------
//   toggleMixer2
//---------------------------------------------------------

void MusE::toggleMixer2(bool checked)
{
    if (dockMixerB)
        mixer2Dock->setVisible(checked);
    else
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

void MusE::mixer1DockTopLevelChanged(bool)
{
    if (mixer1Dock->isFloating())
    {
        mixer1Dock->setWindowFlags(Qt::CustomizeWindowHint |
            Qt::Window | Qt::WindowMinimizeButtonHint |
            Qt::WindowMaximizeButtonHint |
            Qt::WindowCloseButtonHint);
        mixer1Dock->show();
    }
}

void MusE::mixer2DockTopLevelChanged(bool)
{
    if (mixer2Dock->isFloating())
    {
        mixer2Dock->setWindowFlags(Qt::CustomizeWindowHint |
                                   Qt::Window | Qt::WindowMinimizeButtonHint |
                                   Qt::WindowMaximizeButtonHint |
                                   Qt::WindowCloseButtonHint);
        mixer2Dock->show();
    }
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
    if (dynamic_cast<QMdiSubWindow*>(now) != nullptr)
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
  if ( (dynamic_cast<QMdiSubWindow*>(ptr) != nullptr) &&
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

    if ( (dynamic_cast<MusEGui::TopWin*>(ptr) != nullptr) || // *ptr is a TopWin or a derived class
         (ptr==this) )                               // the main window is selected
      break;
    ptr=dynamic_cast<QWidget*>(ptr->parent()); //in the unlikely case that ptr is a QObject, this returns nullptr, which stops the loop
  }

  MusEGui::TopWin* win=dynamic_cast<MusEGui::TopWin*>(ptr);
  // ptr is either nullptr, this or the pointer to a TopWin

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
  if (MusEGlobal::debugMsg) fprintf(stderr, "ACTIVE TOPWIN CHANGED to '%s' (%p)\n",
    win ? win->windowTitle().toLocal8Bit().data() : "<None>", win);

  if (win && (win->sharesToolsAndMenu()))
    setCurrentMenuSharingTopwin(win);
}



void MusE::setCurrentMenuSharingTopwin(MusEGui::TopWin* win)
{
  if (win && (win->sharesToolsAndMenu()==false))
  {
    fprintf(stderr, "WARNING: THIS SHOULD NEVER HAPPEN: MusE::setCurrentMenuSharingTopwin()"
      " called with a win which does not share (%s)! ignoring...\n", win->windowTitle().toLocal8Bit().data());
    return;
  }

  if (win!=currentMenuSharingTopwin)
  {
    MusEGui::TopWin* previousMenuSharingTopwin = currentMenuSharingTopwin;
    currentMenuSharingTopwin = nullptr;

    if (MusEGlobal::debugMsg) fprintf(stderr, "MENU SHARING TOPWIN CHANGED to '%s' (%p)\n",
      win ? win->windowTitle().toLocal8Bit().data() : "<None>", win);

    
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
                  fprintf(stderr, "  inserting toolbar '%s'\n", atb->windowTitle().toLocal8Bit().data());

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
              fprintf(stderr, "  removing break before sharer's toolbar '%s'\n", tb->windowTitle().toLocal8Bit().data());
            removeToolBarBreak(tb);
          }
          
          
          if(MusEGlobal::heavyDebugMsg) 
            fprintf(stderr, "  removing sharer's toolbar '%s'\n", tb->windowTitle().toLocal8Bit().data());
          removeToolBar(tb); // this does not delete *it, which is good
          tb->setParent(nullptr);
        }
      }
        
      foreignToolbars = add_foreign_toolbars;
      
    }
    else
    {
      for (list<QToolBar*>::iterator it = optionalToolbars.begin(); it!=optionalToolbars.end(); ++it)
      {
        QToolBar* tb = *it;
        if (tb)
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
                  fprintf(stderr, "  inserting toolbar '%s'\n", atb->windowTitle().toLocal8Bit().data());

                insertToolBar(tb, atb);
                foreignToolbars.push_back(atb);
                add_toolbars.remove(atb);
                atb->show();
                break;
              }
            }
          }
          
          if (MusEGlobal::heavyDebugMsg) 
            fprintf(stderr, "  removing optional toolbar '%s'\n", tb->windowTitle().toLocal8Bit().data());
          removeToolBar(tb); // this does not delete *it, which is good
          tb->setParent(nullptr);
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
        if (MusEGlobal::heavyDebugMsg) fprintf(stderr, "  adding menu entry '%s'\n", (*it)->text().toLocal8Bit().data());

        menuBar()->addAction(*it);
      }

      for (list<QToolBar*>::const_iterator it=add_toolbars.begin(); it!=add_toolbars.end(); ++it)
        if (*it)
        {
          if (MusEGlobal::heavyDebugMsg) fprintf(stderr, "  adding toolbar '%s'\n", (*it)->windowTitle().toLocal8Bit().data());

          addToolBar(*it);
          foreignToolbars.push_back(*it);
          (*it)->show();
        }
        else
        {
          if (MusEGlobal::heavyDebugMsg) fprintf(stderr, "  adding toolbar break\n");

          addToolBarBreak();
          foreignToolbars.push_back(nullptr);
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

void MusE::setActiveMdiSubWindow(QMdiSubWindow* win)
{
    mdiArea->setActiveSubWindow(win);
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
        setCurrentMenuSharingTopwin(nullptr);
    }
  }
}

void MusE::topwinMenuInited(MusEGui::TopWin* topwin)
{
  if (topwin == nullptr)
    return;

  if (topwin == waitingForTopwin)
  {
    if (waitingForTopwin->deleting())
    {
      waitingForTopwin = nullptr;
    }
    else
    {
      activeTopWin=waitingForTopwin;
      waitingForTopwin = nullptr;
      emit activeTopWinChanged(activeTopWin);
    }
  }
  else if (topwin == currentMenuSharingTopwin)
  {
    fprintf(stderr, "====== DEBUG ======: topwin's menu got inited AFTER being shared!\n");
    if (!topwin->sharesToolsAndMenu()) fprintf(stderr, "======       ======: WTF, now it doesn't share any more?!?\n");
    setCurrentMenuSharingTopwin(nullptr);
    setCurrentMenuSharingTopwin(topwin);
  }
}

void MusE::updateWindowMenu()
{
    menuWindows->clear(); // frees memory automatically

    for (const auto& it : toplevels) {
        if (it->isMdiWin()) {
            QAction* temp = menuWindows->addAction(it->windowTitle());
            temp->setIcon(it->typeIcon(it->type()));
            QWidget* tlw = static_cast<QWidget*>(it);
            connect(temp, &QAction::triggered, [this, tlw]() { bringToFront(tlw); } );

            if (it->type() == TopWin::ARRANGER) { // should be always on top
                temp->setShortcut(shortcuts[SHRT_ARRANGER].key);
                if (toplevels.size() > 1)
                    menuWindows->addSeparator();
            }
        }
    }

    bool sep = false;
    for (const auto& it : toplevels) {
        if (!it->isMdiWin()) {
            if (!sep && toplevels.size() > 2) {
                menuWindows->addSeparator();
                sep = true;
            }
            QAction* temp = menuWindows->addAction(it->windowTitle());
            temp->setIcon(it->typeIcon(it->type()));
            QWidget* tlw = static_cast<QWidget*>(it);
            connect(temp, &QAction::triggered, [this, tlw]() { bringToFront(tlw); } );
        }
    }
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
    win->showMaximized();
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

QString MusE::projectTitle(QString name)
{
  return tr("MusE Project: ") + MusEGui::projectTitleFromFilename(name);
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
         track = MusEGlobal::song->addNewTrack(&act, nullptr);
      }

      if(!track)
      {
         QMessageBox::critical(this, QString("MusE"),
                 tr("Failed to import wave track"));
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

        QFileInfo fi(f.name());

        //remove old peak-file to cut down on clutter. It will be recreated at the new wave location
        QString cacheName = fi.absolutePath() + QString("/") + fi.completeBaseName() + QString(".wca");
        QFile::remove(cacheName);

        if(MusEGlobal::museProject == MusEGlobal::museProjectInitPath)
        {
          if(!MusEGlobal::muse->saveAs())
              return true;
        }

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

        SNDFILE *sfNew = sf_open(fNewPath.toLocal8Bit().constData(), SFM_RDWR, &sfiNew);
        if(sfNew == nullptr)
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
          QFile::remove(fNewPath);
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


int MusE::arrangerRaster() const
{
  return _arranger->rasterVal();
}

int MusE::currentPartColorIndex() const
{
  if(_arranger)
    return _arranger->currentPartColorIndex();
  return 0;
}


bool MusE::restoreState(const QByteArray &state, int version) {

    QList<QDockWidget *> docksVisible;
    for (const auto& d : findChildren<QDockWidget *>()) {
        if (d->isVisible()) {
            docksVisible.prepend(d);
            d->hide();
        }
    }

    bool ret = QMainWindow::restoreState(state, version);

    for (const auto& d : findChildren<QDockWidget *>()) {
        if (d->isVisible()) {
            if (docksVisible.contains(d))
                docksVisible.removeOne(d);
            else
                d->hide();
        }
    }

    for (const auto& d : docksVisible) {
        d->show();
    }


//    if (!docksVisible.isEmpty()) {

//        QVector<QDockWidget*> areaDocks;
//        for (const auto& d : findChildren<QDockWidget*>()) {
//            if (dockWidgetArea(d) == Qt::BottomDockWidgetArea
//                    && strcmp(d->widget()->metaObject()->className(), "MusEGui::ListEdit") == 0
//                    && !docksVisible.contains(d))
//                areaDocks.prepend(d);
//        }

//        if (!(areaDocks.isEmpty() && docksVisible.count() == 1)) {
//            auto& cur = areaDocks.isEmpty() ? docksVisible.first() : areaDocks.last();
//            for (const auto& d : docksVisible) {
//                if (areaDocks.isEmpty() && d == docksVisible.first())
//                    continue;;
//                tabifyDockWidget(cur, d);
//                QTimer::singleShot(0, [d](){ d->raise(); });
//                cur = d;
//            }
//        }
//    }

    return ret;
}

void MusE::toggleDocks(bool show) {
    if (show) {
        if (!hiddenDocks.isEmpty()) {
            for (const auto& d : hiddenDocks)
                d->show();
            hiddenDocks.clear();
        }
    } else {
        hiddenDocks.clear();
        for (const auto& d : findChildren<QDockWidget *>()) {
            if (d->isVisible()) {
                hiddenDocks.prepend(d);
                d->hide();
            }
        }
    }
}

void MusE::closeDocks() {

    hiddenDocks.clear();
    toggleDocksAction->setChecked(true);

    for (const auto& d : findChildren<QDockWidget *>()) {
        if (strcmp(d->widget()->metaObject()->className(), "MusEGui::ListEdit") == 0)
            d->close();
        else if (d->isVisible()) {
            d->hide();
        }
    }
}

void MusE::addTabbedDock(Qt::DockWidgetArea area, QDockWidget *dock)
{
    QVector<QDockWidget*> areaDocks;
    for (const auto& d : findChildren<QDockWidget*>()) {
        if (dockWidgetArea(d) == area)
            areaDocks.append(d);
    }

    if (areaDocks.empty()) {
        addDockWidget(area, dock);
    } else {
        tabifyDockWidget(areaDocks.last(), dock);
//        dock->raise(); // doesn't work, Qt problem (kybos)
        QTimer::singleShot(0, [dock](){ dock->raise(); });
    }
}

void MusE::saveStateTopLevels() {

    for (const auto& it : toplevels) {
        if (activeTopWin && (activeTopWin == it))
            it->storeInitialState();
        it->storeSettings();
    }
}

void MusE::saveStateExtra() {

    MusEGlobal::config.transportVisible = transport->isVisible();
    MusEGlobal::config.geometryTransport.setTopLeft(transport->pos());
    // size part not used, transport window has fixed size

    if (bigtime) {
        MusEGlobal::config.bigTimeVisible = bigtime->isVisible();
        MusEGlobal::config.geometryBigTime.setTopLeft(bigtime->pos());
        MusEGlobal::config.geometryBigTime.setSize(bigtime->size());
    }

    if (mixer1) {
        MusEGlobal::config.mixer1Visible = mixer1->isVisible();
        MusEGlobal::config.mixer1.geometry = mixer1->geometry();
    }

    if (mixer2) {
        MusEGlobal::config.mixer2Visible = mixer2->isVisible();
        MusEGlobal::config.mixer2.geometry = mixer2->geometry();
    }
}

void MusE::initStatusBar() {

    statusBar()->setSizeGripEnabled(false);
    statusBar()->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Minimum);

    cpuStatusBar = new CpuStatusBar(statusBar());
    connect(cpuStatusBar, SIGNAL(resetClicked()), SLOT(resetXrunsCounter()));
    statusBar()->addPermanentWidget(cpuStatusBar);

    QString s = QString("%1 | Sample rate: %2Hz | Segment size: %3 | Segment count: %4")
            .arg(MusEGlobal::audioDevice->driverName())
            .arg(MusEGlobal::sampleRate)
            .arg(MusEGlobal::segmentSize)
            .arg(MusEGlobal::segmentCount);
    statusBar()->addWidget(new QLabel(s));

    updateStatusBar();
}

void MusE::updateStatusBar() {
    statusBar()->setVisible(MusEGlobal::config.showStatusBar);
}

void MusE::setStatusBarText(const QString &message, int timeout) {
    if (MusEGlobal::config.showStatusBar)
        statusBar()->showMessage(message, timeout);
}

void MusE::clearStatusBarText() {
    if (MusEGlobal::config.showStatusBar)
        statusBar()->clearMessage();
}

QMenu* MusE::createPopupMenu() {
    QMenu* menu= QMainWindow::createPopupMenu();
    menu->setObjectName("CheckmarkOnly");
    return menu;
}

#ifndef USE_SENDPOSTEDEVENTS_FOR_TOPWIN_CLOSE
void MusE::addPendingObjectDestruction(QObject* obj)
{
  const QMetaObject::Connection conn = connect(obj, &QObject::destroyed, [this](QObject *obj) { objectDestroyed(obj); } );
  if(conn)
    _pendingObjectDestructions.insert(obj, ObjectDestructionStruct(conn));
}
#endif
} //namespace MusEGui
