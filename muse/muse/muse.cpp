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

#include "config.h"
#include <sys/mman.h>
#include "muse.h"
#include "transport.h"
#include "widgets/bigtime.h"
#include "arranger/arranger.h"
#include "midiedit/pianoroll.h"
#include "al/xml.h"
#include "conf.h"
#include "liste/listedit.h"
#include "master/masteredit.h"
#include "midiedit/drumedit.h"
#include "mixer/mixer.h"
#include "driver/audiodev.h"
#include "waveedit/waveedit.h"
#include "icons.h"
#include "widgets/mixdowndialog.h"
#include "midictrl.h"
#include "widgets/filedialog.h"
#include "plugin.h"
#include "marker/markerview.h"
#include "transpose.h"
#include "preferences.h"
#include "audio.h"
#include "midiseq.h"
#include "audioprefetch.h"
#include "audiowriteback.h"
#include "widgets/shortcutconfig.h"
#include "gconfig.h"
#include "ticksynth.h"
#include "song.h"
#include "awl/poslabel.h"
#include "al/tempo.h"
#include "shortcuts.h"
#ifdef __APPLE__
#include "driver/coremidi.h"
#else
#include "driver/alsamidi.h"
#endif
#include "midiplugin.h"
#include "midiedit/drummap.h"
#include "widgets/utils.h"
#include "instruments/editinstrument.h"
#include "part.h"
#include "projectdialog.h"
#include "templatedialog.h"
#include "midiedit/miditracker.h"
#include "projectpropsdialog.h"
#include "liste/listedit.h"

extern void initMidiInstruments();

static pthread_t watchdogThread;

static const char* infoLoopButton     = QT_TR_NOOP("loop between left mark and right mark");
static const char* infoPunchinButton  = QT_TR_NOOP("record starts at left mark");
static const char* infoPunchoutButton = QT_TR_NOOP("record stops at right mark");
static const char* infoRewindButton   = QT_TR_NOOP("rewind current position");
static const char* infoForwardButton  = QT_TR_NOOP("move current position");
static const char* infoStopButton     = QT_TR_NOOP("stop sequencer");
static const char* infoRecordButton   = QT_TR_NOOP("to record press record and then play");
static const char* infoPanicButton    = QT_TR_NOOP("send note off to all midi channels");

#define PROJECT_LIST_LEN  6
static QString* projectList[PROJECT_LIST_LEN];

extern void initIcons();
extern void initMidiSynth();
extern bool initDummyAudio();
extern void initVST();
extern void initDSSI();
extern bool initJackAudio();
extern void exitJackAudio();

QStyle* smallStyle;

int watchAudio, watchAudioPrefetch, watchMidi;
pthread_t splashThread;
MusE* muse;

//
//   Arranger Snap values
//

struct RasterVal {
      int val;
      QString label;
      };

static RasterVal rasterTable[] = {
      { 1,            QT_TR_NOOP("Off") },
      { 0,            QT_TR_NOOP("Bar") },
      { 2 * config.division, "1/2" },
      { config.division,     "1/4" },
      { config.division/2,   "1/8" },
      { config.division/4,   "1/16" }
      };

//---------------------------------------------------------
//   watchdog thread
//	realtime priorities are:
//	   realTimePriority	- priority of jack thread
//	   realTimePriority+1	- priority of midi thread
//	   realTimePriority+2	- priority of watchdog thread
//---------------------------------------------------------

static void* watchdog(void*)
      {
      int policy;
#ifndef __APPLE__
      if ((policy = sched_getscheduler (0)) < 0)
            printf("Cannot get current client scheduler: %s\n", strerror(errno));
      if (policy != SCHED_FIFO)
            printf("MusE: watchdog process _NOT_ running SCHED_FIFO\n");
#endif
      int fatal = 0;
      for (;;) {
            watchAudio = 0;
            watchMidi = 0;
            static const int WD_TIMEOUT = 3;

            int to = WD_TIMEOUT;
            while (to > 0)           // sleep can be interrupted by signal
                  to = sleep(to);

            bool timeout = false;
            if (watchMidi == 0)
                  timeout = true;
            if (watchAudio == 0)
                  timeout = true;
            if (watchAudio > 500000)
                  timeout = true;
            if (watchMidi > (config.rtcTicks * WD_TIMEOUT * 2))
                  timeout = true;
            if (timeout)
                  ++fatal;
            else
                  fatal = 0;
            if (fatal >= 3) {
                  printf("MusE: WatchDog: fatal error, realtime task timeout\n");
                  printf("   (%d,%d-%d) - stopping all services\n",
                     watchMidi, watchAudio, fatal);
                  break;
                  }
//            printf("wd %d %d %d\n", watchMidi, watchAudio, fatal);
            }
      audio->stop();
      audioWriteback->stop(true);
      audioPrefetch->stop(true);
      fatalError("watchdog timeout");
      return 0;
      }

//---------------------------------------------------------
//   seqStart
//---------------------------------------------------------

bool MusE::seqStart()
      {
      if (audioState != AUDIO_STOP) {
            printf("seqStart(): already running\n");
            return true;
            }
      audioState = AUDIO_START1;
      if (!audio->start()) {
          	QMessageBox::critical( muse, tr("Failed to start audio!"),
               tr("Was not able to start audio, check if jack is running.\n"));
          return false;
          }
      //
      // wait for jack callback
      //
      for (int i = 0; i < 60; ++i) {
            if (audioState == AUDIO_START2)
                  break;
            sleep(1);
            }
      if (audioState != AUDIO_START2) {
          	QMessageBox::critical( muse, tr("Failed to start audio!"),
               tr("Was not able to start audio, check if jack is running.\n"));
            }
      //
      // now its safe to ask the driver for realtime
      // priority

	realTimePriority = audioDriver->realtimePriority();

      //
      //  create midi thread with a higher priority than JACK
      //
      midiSeq->start(realTimePriority ? realTimePriority + 2 : 0);

      if (realTimePriority) {
            //
            //  create watchdog thread with realTimePriority+2
            //
            int priority = realTimePriority + 3;
            struct sched_param rt_param;
            memset(&rt_param, 0, sizeof(rt_param));
            rt_param.sched_priority = priority;

            pthread_attr_t* attributes = (pthread_attr_t*) malloc(sizeof(pthread_attr_t));
            pthread_attr_init(attributes);

            if (pthread_attr_setschedpolicy(attributes, SCHED_FIFO)) {
                  printf("MusE: cannot set FIFO scheduling class for RT thread\n");
                  }
            if (pthread_attr_setschedparam (attributes, &rt_param)) {
                  // printf("Cannot set scheduling priority for RT thread (%s)\n", strerror(errno));
                  }
            if (pthread_attr_setscope (attributes, PTHREAD_SCOPE_SYSTEM)) {
                  printf("MusE: Cannot set scheduling scope for RT thread\n");
                  }
            if (pthread_attr_setinheritsched(attributes, PTHREAD_EXPLICIT_SCHED)) {
                  printf("Cannot set setinheritsched for RT thread\n");
                  }
            int rv;
            if ((rv = pthread_create(&watchdogThread, attributes, ::watchdog, 0)))
                  fprintf(stderr, "MusE: creating watchdog thread failed: %s\n",
            	   strerror(rv));
            pthread_attr_destroy(attributes);
            //
            // start audio prefetch threads with realtime
            // priority
            //
            audioPrefetch->start(realTimePriority-5);
            audioWriteback->start(realTimePriority-5);
            }
      else {
            printf("MusE: Warning - starting threads without realtime priority since audio is not running realtime.\n");
            //
            // start audio prefetch threads as normal
            // (non realtime) threads
            //
            audioPrefetch->start(0);
            audioWriteback->start(0);
            }
      audioState = AUDIO_RUNNING;
      //
      // do connections
      //
      TrackList* tl = song->tracks();
      for (iTrack i = tl->begin(); i != tl->end(); ++i)
            (*i)->activate2();
      return true;
      }

//---------------------------------------------------------
//   stop
//---------------------------------------------------------

void MusE::seqStop()
      {
//      audio->msgIdle(true);
      song->setStop(true);
      song->setStopPlay(false);
      midiSeq->stop(true);
      audio->stop();
      audioWriteback->stop(true);
      audioPrefetch->stop(true);
      if (realTimePriority)
            pthread_cancel(watchdogThread);
      audioState = AUDIO_STOP;
      }

//---------------------------------------------------------
//   seqRestart
//---------------------------------------------------------

bool MusE::seqRestart()
	{
	bool restartSequencer = audioState == AUDIO_RUNNING;
	if (restartSequencer) {
		if (audio->isPlaying()) {
            	audio->msgPlay(false);
                	while (audio->isPlaying())
                  	qApp->processEvents();
			}
		seqStop();
		}

	if (!seqStart())
      	return false;

	audioDriver->graphChanged();
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
//   populateAddTrack
//    this is also used in "mixer"
//---------------------------------------------------------

void populateAddTrack(QMenu* m)
      {
      m->setSeparatorsCollapsible(false);
      m->clear();
      QAction* a;

      a = m->addSeparator();
      a->setText(QT_TR_NOOP("Midi"));
      QFont f(a->font());
      f.setBold(true);
      f.setPointSize(8);
      a->setFont(f);

      a = m->addAction(QIcon(*addtrack_addmiditrackIcon), QT_TR_NOOP("Add Midi Track"));
      a->setData(Track::MIDI);

      a = m->addAction(QIcon(*addtrack_addmiditrackIcon), QT_TR_NOOP("Add Midi Output"));
      a->setData(Track::MIDI_OUT);

      a = m->addAction(QIcon(*addtrack_addmiditrackIcon), QT_TR_NOOP("Add Midi Input"));
      a->setData(Track::MIDI_IN);

      QMenu* ps = m->addMenu(QMenu::tr("Add Midi Generator..."));

      int idx = 5000;
      for (iMidiPlugin i = midiPlugins.begin(); i != midiPlugins.end(); ++i) {
            if ((*i)->type() != MEMPI_GENERATOR)
                  continue;
            a = ps->addAction((*i)->name());
            a->setData(idx);
            }

      if (!midiOnly) {
            a = m->addSeparator();
            a->setText(QT_TR_NOOP("Audio"));
            a->setFont(f);

            a = m->addAction(QIcon(*addtrack_wavetrackIcon),   QT_TR_NOOP("Add Wave Track"));
            a->setData(Track::WAVE);
            a = m->addAction(QIcon(*addtrack_audiooutputIcon), QT_TR_NOOP("Add Audio Output"));
            a->setData(Track::AUDIO_OUTPUT);
            a = m->addAction(QIcon(*addtrack_audiogroupIcon),  QT_TR_NOOP("Add Audio Group"));
            a->setData(Track::AUDIO_GROUP);
            a = m->addAction(QIcon(*addtrack_audioinputIcon),  QT_TR_NOOP("Add Audio Input"));
            a->setData(Track::AUDIO_INPUT);

            ps = m->addMenu(QMenu::tr("Add Soft Synth..."));

            int idx = 1000;
            for (std::vector<Synth*>::iterator is = synthis.begin(); is != synthis.end(); ++is, ++idx) {
                  a = ps->addAction((*is)->name());
                  a->setData(idx);
                  }
            }
      m->connect(m, SIGNAL(triggered(QAction*)), song, SLOT(addTrack(QAction*)));
      }

//---------------------------------------------------------
//   setupTransportToolbar
//---------------------------------------------------------

void MusE::setupTransportToolbar(QToolBar* tb) const
      {
      tb->addAction(loopAction);
      tb->addAction(punchinAction);
      tb->addAction(punchoutAction);
      tb->addAction(startAction);

      QToolButton* rewindTb = new QToolButton;
      rewindTb->setDefaultAction(rewindAction);
      rewindTb->setAutoRepeat(true);
      tb->addWidget(rewindTb);
      tb->connect(rewindTb, SIGNAL(clicked()), song, SLOT(rewind()));

      QToolButton* forwardTb = new QToolButton;
      forwardTb->setDefaultAction(forwardAction);
      forwardTb->setAutoRepeat(true);
      tb->addWidget(forwardTb);
      tb->connect(forwardTb, SIGNAL(clicked()), song, SLOT(forward()));

      tb->addAction(stopAction);
      tb->addAction(playAction);
      tb->addAction(recordAction);
      }

//---------------------------------------------------------
//   MusE
//---------------------------------------------------------

MusE::MusE()
   : QMainWindow()
      {
      setWindowIcon(*museIcon);
      setIconSize(ICON_SIZE);
      setFocusPolicy(Qt::WheelFocus);

      muse                  = this;    // hack
      midiSyncConfig        = 0;
      midiRemoteConfig      = 0;
      midiPortConfig        = 0;
      metronomeConfig       = 0;
      audioConfig           = 0;
      midiFileConfig        = 0;
      midiFilterConfig      = 0;
      midiInputTransform    = 0;
      midiRhythmGenerator   = 0;
      preferencesDialog     = 0;
      softSynthesizerConfig = 0;
      midiTransformerDialog = 0;
      shortcutConfig        = 0;
      editInstrument        = 0;
      appName               = QString("MusE");
      _raster               = 0;
      audioState            = AUDIO_STOP;
      bigtime               = 0;
      mixer1                = 0;
      mixer2                = 0;
      markerView            = 0;
      exportMidiDialog      = 0;
      projectPropsDialog    = 0;
      listEditor            = 0;

      for (unsigned i = 0;; ++i) {
            if (sc[i].xml == 0)
                  break;
            shortcuts[sc[i].xml] = &sc[i];
            }

      startAction = getAction("start", this);
      startAction->setIcon(QIcon(":/xpm/start.xpm"));
      startAction->setText(tr("goto start"));
      connect(startAction, SIGNAL(triggered()), song, SLOT(rewindStart()));

      playAction = getAction("play", this);
      playAction->setIcon(QIcon(":/xpm/play.xpm"));
      playAction->setText(tr("play"));
      playAction->setCheckable(true);
      connect(playAction, SIGNAL(triggered(bool)), song, SLOT(setPlay(bool)));

      QAction* a = getAction("play_toggle", this);
      a->setShortcutContext(Qt::ApplicationShortcut);
      connect(a, SIGNAL(triggered()), SLOT(playToggle()));
      addAction(a);
      
      rewindAction  = new QAction(QIcon(":/xpm/frewind.xpm"),  "rewind",  this);
      forwardAction = new QAction(QIcon(":/xpm/fforward.xpm"), "forward", this);
      stopAction    = new QAction(QIcon(":/xpm/stop.xpm"),     "stop",    this);

      stopAction->setCheckable(true);

      song->blockSignals(true);
      heartBeatTimer = new QTimer(this);
      connect(heartBeatTimer, SIGNAL(timeout()), song, SLOT(beat()));

      //---------------------------------------------------
      //    undo/redo
      //---------------------------------------------------

      undoAction = new QAction(QIcon(*undoIcon), tr("Und&o"), this);
      undoAction->setShortcut(Qt::CTRL + Qt::Key_Z);
      undoAction->setWhatsThis(tr("undo last change to song"));
      undoAction->setEnabled(false);
      undoAction->setData(-1);
      connect(undoAction, SIGNAL(triggered()), song, SLOT(undo()));
      redoAction = new QAction(QIcon(*redoIcon), tr("Re&do"), this);
      redoAction->setShortcut(Qt::CTRL + Qt::Key_Y);
      redoAction->setWhatsThis(tr("redo last undo"));
      redoAction->setEnabled(false);
      redoAction->setData(-1);
      connect(redoAction, SIGNAL(triggered()), song, SLOT(redo()));

      //---------------------------------------------------
      //    Transport
      //---------------------------------------------------

      loopAction = new QAction(QIcon(*loop1Icon), tr("Loop"), this);
      loopAction->setWhatsThis(tr(infoLoopButton));
      loopAction->setCheckable(true);
      connect(loopAction, SIGNAL(toggled(bool)), song, SLOT(setLoop(bool)));

      punchinAction = new QAction(QIcon(*punchin1Icon), tr("Punchin"), this);
      punchinAction->setWhatsThis(tr(infoPunchinButton));
      punchinAction->setCheckable(true);
      connect(punchinAction, SIGNAL(toggled(bool)), song, SLOT(setPunchin(bool)));

      punchoutAction = new QAction(QIcon(*punchout1Icon), tr("Punchout"), this);
      punchoutAction->setWhatsThis(tr(infoPunchoutButton));
      punchoutAction->setCheckable(true);
      connect(punchoutAction, SIGNAL(toggled(bool)), song, SLOT(setPunchout(bool)));

      rewindAction->setWhatsThis(tr(infoRewindButton));

      forwardAction->setWhatsThis(tr(infoForwardButton));

      stopAction->setWhatsThis(tr(infoStopButton));
      connect(stopAction, SIGNAL(triggered(bool)), song, SLOT(setStop(bool)));


      recordAction = new QAction(*recordIcon, tr("Record"), this);
      recordAction->setWhatsThis(tr(infoRecordButton));
      recordAction->setCheckable(true);
      connect(recordAction, SIGNAL(triggered(bool)), song, SLOT(setRecord(bool)));

      panicAction = new QAction(QIcon(*panicIcon), tr("Panic"), this);
      panicAction->setWhatsThis(tr(infoPanicButton));
      connect(panicAction, SIGNAL(triggered()), song, SLOT(panic()));

#ifdef __APPLE__
      if (coreMidi.init()) {
          QMessageBox::critical(NULL, "MusE fatal error.",
            "MusE failed to initialize the\n"
            "Core midi subsystem, check\n"
            "your configuration.");
          fatalError("init core-midi failed");
          }
      midiDriver = &coreMidi;
#else
      if (alsaMidi.init()) {
          QMessageBox::critical(NULL, "MusE fatal error.",
            "MusE failed to initialize the\n"
            "Alsa midi subsystem, check\n"
            "your configuration.");
          fatalError("init alsa failed");
          }
      midiDriver = &alsaMidi;
#endif
      //----Actions


      fileOpenAction = getAction("open_project", this);
      fileOpenAction->setText(tr("Open Project"));
      fileOpenAction->setIcon(QIcon(*openIcon));

      fileSaveAction = getAction("save_project", this);
      fileSaveAction->setText(tr("Save Project"));
      fileSaveAction->setIcon(QIcon(*saveIcon));

      pianoAction = new QAction(*pianoIconSet, tr("Pianoroll"), this);
      connect(pianoAction, SIGNAL(triggered()), SLOT(startPianoroll()));

      trackerAction = new QAction(*pianoIconSet, tr("MidiTracker"), this);
      connect(trackerAction, SIGNAL(triggered()), SLOT(startMidiTrackerEditor()));

      connect(fileOpenAction, SIGNAL(triggered()), SLOT(loadProject()));
      connect(fileSaveAction, SIGNAL(triggered()), SLOT(save()));

      //--------------------------------------------------
      //    Toolbar
      //--------------------------------------------------

      tools = new QToolBar(tr("Project Buttons"));
      addToolBar(tools);

      tools->addAction(fileOpenAction);
      tools->addAction(fileSaveAction);
      tools->addAction(QWhatsThis::createAction(this));

      tools->addSeparator();
      tools->addAction(undoAction);
      tools->addAction(redoAction);

      tools1 = new EditToolBar(this, arrangerTools);
      addToolBar(tools1);

      QToolBar* transportToolbar = addToolBar(tr("Transport"));
      setupTransportToolbar(transportToolbar);

      QToolBar* panicToolbar = new QToolBar(tr("Panic"), this);
      addToolBar(panicToolbar);
      panicToolbar->addAction(panicAction);

      addToolBarBreak();

      audio          = new Audio();
      audioPrefetch  = new AudioPrefetch("Prefetch");
      audioWriteback = new AudioWriteback("Writeback");
      midiSeq        = new MidiSeq("Midi");

      //---------------------------------------------------
      //    MenuBar
      //---------------------------------------------------

      QMenuBar* mb = menuBar();

      //-------------------------------------------------------------
      //    File
      //-------------------------------------------------------------

      menu_file = mb->addMenu(tr("&Project"));

      menu_file->addAction(fileOpenAction);

      openRecent = new QMenu(tr("Open &Recent"), this);
      connect(openRecent, SIGNAL(aboutToShow()), this, SLOT(openRecentMenu()));
      connect(openRecent, SIGNAL(triggered(QAction*)), this, SLOT(selectProject(QAction*)));

      menu_ids[CMD_OPEN_RECENT] = menu_file->addMenu(openRecent);
      menu_file->addSeparator();
      menu_file->addAction(fileSaveAction);
      menu_file->addSeparator();
      menu_ids[CMD_IMPORT_MIDI] = menu_file->addAction(*openIconS, tr("Import Midifile"));
      connect(menu_ids[CMD_IMPORT_MIDI], SIGNAL(triggered()),  this, SLOT(importMidi()));
      menu_ids[CMD_EXPORT_MIDI] = menu_file->addAction(*saveIconS, tr("Export Midifile"));
      connect(menu_ids[CMD_EXPORT_MIDI], SIGNAL(triggered()),  this, SLOT(exportMidi()));
      menu_file->addSeparator();

      menu_ids[CMD_IMPORT_AUDIO] = menu_file->addAction(*openIconS, tr("Import Wave File"));
      connect(menu_ids[CMD_IMPORT_AUDIO], SIGNAL(triggered()), this, SLOT(importWave()));
      menu_ids[CMD_IMPORT_AUDIO]->setEnabled(!midiOnly);

      menu_file->addSeparator();
      menu_ids[CMD_QUIT] = menu_file->addAction(*onOffIcon, tr("&Quit"));
      connect(menu_ids[CMD_QUIT], SIGNAL(triggered()), this, SLOT(quitDoc()));
      menu_file->addSeparator();

      //-------------------------------------------------------------
      //    Edit
      //-------------------------------------------------------------

      menuEdit = mb->addMenu(tr("&Edit"));

      menuEdit->addAction(undoAction);
      menuEdit->addAction(redoAction);
      menuEdit->addSeparator();

      cutAction = menuEdit->addAction(*editcutIconSet, tr("C&ut"));
      cutAction->setData(CMD_CUT);
      cutAction->setShortcut(Qt::CTRL + Qt::Key_X);

      copyAction = menuEdit->addAction(*editcopyIconSet, tr("&Copy"));
      copyAction->setData(CMD_COPY);
      copyAction->setShortcut(Qt::CTRL + Qt::Key_C);

      pasteAction = menuEdit->addAction(*editpasteIconSet, tr("&Paste"));
      pasteAction->setData(CMD_PASTE);
      pasteAction->setShortcut(Qt::CTRL + Qt::Key_V);

      menuEditActions[CMD_DELETE] = menuEdit->addAction(*deleteIcon, tr("&Delete Parts"));
      menuEditActions[CMD_DELETE]->setData(CMD_DELETE);

      menuEdit->addSeparator();
      a = menuEdit->addAction(QIcon(*edit_track_delIcon), tr("Delete Selected Tracks"));
      a->setData(CMD_DELETE_TRACK);

      addTrack = menuEdit->addMenu(*edit_track_addIcon, tr("Add Track"));
      // delay creation of menu (at this moment the list of software
      //   synthesizer is empty):
      connect(addTrack, SIGNAL(aboutToShow()), SLOT(aboutToShowAddTrack()));

      menuEdit->addSeparator();
      select = menuEdit->addMenu(QIcon(*selectIcon), tr("Select"));
      menuEditActions[CMD_SELECT_ALL] = select->addAction(QIcon(*select_allIcon), tr("Select &All"));
      menuEditActions[CMD_SELECT_ALL]->setData(CMD_SELECT_ALL);
      menuEditActions[CMD_SELECT_NONE] = select->addAction(QIcon(*select_deselect_allIcon), tr("&Deselect All"));
      menuEditActions[CMD_SELECT_NONE]->setData(CMD_SELECT_NONE);
      menuEditActions[CMD_SELECT_INVERT] = select->addAction(QIcon(*select_invert_selectionIcon), tr("Invert &Selection"));
      menuEditActions[CMD_SELECT_INVERT]->setData(CMD_SELECT_INVERT);
      menuEditActions[CMD_SELECT_ILOOP] = select->addAction(QIcon(*select_inside_loopIcon), tr("&Inside Loop"));
      menuEditActions[CMD_SELECT_ILOOP]->setData(CMD_SELECT_ILOOP);
      menuEditActions[CMD_SELECT_OLOOP] = select->addAction(QIcon(*select_outside_loopIcon), tr("&Outside Loop"));
      menuEditActions[CMD_SELECT_OLOOP]->setData(CMD_SELECT_OLOOP);
      menuEditActions[CMD_SELECT_PARTS] = select->addAction(QIcon(*select_all_parts_on_trackIcon), tr("All &Parts on Track"));
      menuEditActions[CMD_SELECT_PARTS]->setData(CMD_SELECT_PARTS);

      menuEdit->addSeparator();
      menuEdit->addAction(pianoAction);
      menuEdit->addAction(trackerAction);
      menu_ids[CMD_OPEN_DRUMS]          = menuEdit->addAction(QIcon(*edit_drummsIcon), tr("Drums"));
      menu_ids[CMD_OPEN_LIST]           = menuEdit->addAction(QIcon(*edit_listIcon),   tr("List"));
      menu_ids[CMD_OPEN_GRAPHIC_MASTER] = menuEdit->addAction(QIcon(*mastertrack_graphicIcon),tr("Mastertrack"));
      menu_ids[CMD_OPEN_PROJECT_PROPS]  = menuEdit->addAction(*saveIcon, tr("Project Properties"));

      connect(menu_ids[CMD_OPEN_DRUMS], SIGNAL(triggered()), SLOT(startDrumEditor()));
      connect(menu_ids[CMD_OPEN_LIST],  SIGNAL(triggered()), SLOT(startListEditor()));
      connect(menu_ids[CMD_OPEN_GRAPHIC_MASTER], SIGNAL(triggered()), SLOT(startMasterEditor()));
      connect(menu_ids[CMD_OPEN_PROJECT_PROPS], SIGNAL(triggered()), SLOT(showProjectPropsDialog()));

      menuEdit->addSeparator();
      connect(menuEdit, SIGNAL(triggered(QAction*)), SLOT(cmd(QAction*)));
      connect(select,   SIGNAL(triggered(QAction*)), SLOT(cmd(QAction*)));

      midiEdit = menuEdit->addMenu(QIcon(*edit_midiIcon), tr("Midi"));
//      menu_ids[CMD_OPEN_MIDI_TRANSFORM] = midiEdit->addAction(QIcon(*midi_transformIcon), tr("Midi &Transform"), this, SLOT(startMidiTransformer()), 0);

#if 0  // TODO
      midiEdit->insertItem(tr("Modify Gate Time"), this, SLOT(modifyGateTime()));
      midiEdit->insertItem(tr("Modify Velocity"),  this, SLOT(modifyVelocity()));
      midiEdit->insertItem(tr("Crescendo"),        this, SLOT(crescendo()));
      midiEdit->insertItem(tr("Transpose"),        this, SLOT(transpose()));
      midiEdit->insertItem(tr("Thin Out"),         this, SLOT(thinOut()));
      midiEdit->insertItem(tr("Erase Event"),      this, SLOT(eraseEvent()));
      midiEdit->insertItem(tr("Note Shift"),       this, SLOT(noteShift()));
      midiEdit->insertItem(tr("Move Clock"),       this, SLOT(moveClock()));
      midiEdit->insertItem(tr("Copy Measure"),     this, SLOT(copyMeasure()));
      midiEdit->insertItem(tr("Erase Measure"),    this, SLOT(eraseMeasure()));
      midiEdit->insertItem(tr("Delete Measure"),   this, SLOT(deleteMeasure()));
      midiEdit->insertItem(tr("Create Measure"),   this, SLOT(createMeasure()));
      midiEdit->insertItem(tr("Mix Track"),        this, SLOT(mixTrack()));
#endif
      menu_ids[CMD_TRANSPOSE] = midiEdit->addAction(QIcon(*midi_transposeIcon), tr("Transpose"));
      connect(menu_ids[CMD_TRANSPOSE], SIGNAL(triggered()), this, SLOT(transpose()));

      //-------------------------------------------------------------
      //    View
      //-------------------------------------------------------------

      menuView = mb->addMenu(tr("&View"));

      tr_id = menuView->addAction(QIcon(*view_transport_windowIcon), tr("Transport Panel"));
      tr_id->setCheckable(true);
      connect(tr_id, SIGNAL(triggered(bool)), this, SLOT(showTransport(bool)));
      bt_id = menuView->addAction(QIcon(*view_bigtime_windowIcon), tr("Bigtime window"));
      bt_id->setCheckable(true);
      connect(bt_id, SIGNAL(triggered(bool)), this, SLOT(showBigtime(bool)));
      aid1a = menuView->addAction(QIcon(*mixerSIcon), tr("Mixer 1"));
      aid1a->setCheckable(true);
      connect(aid1a, SIGNAL(triggered(bool)), this, SLOT(showMixer1(bool)));
      aid1b = menuView->addAction(QIcon(*mixerSIcon), tr("Mixer 2"));
      aid1b->setCheckable(true);
      connect(aid1b, SIGNAL(triggered(bool)), this, SLOT(showMixer2(bool)));
      mk_id = menuView->addAction(QIcon(*view_markerIcon), tr("Marker"));
      mk_id->setCheckable(true);
      connect(mk_id , SIGNAL(triggered(bool)), this, SLOT(showMarker(bool)));

      //-------------------------------------------------------------
      //    Structure
      //-------------------------------------------------------------

      menuStructure = mb->addMenu(tr("&Structure"));

      menu_ids[CMD_GLOBAL_CUT] = menuStructure->addAction(tr("Global Cut"));
      connect(menu_ids[CMD_GLOBAL_CUT], SIGNAL(triggered()), this, SLOT(globalCut()));

      menu_ids[CMD_GLOBAL_INSERT] = menuStructure->addAction(tr("Global Insert"));
      connect(menu_ids[CMD_GLOBAL_INSERT], SIGNAL(triggered()), this, SLOT(globalInsert()));

      menu_ids[CMD_GLOBAL_SPLIT] = menuStructure->addAction(tr("Global Split"));
      connect(menu_ids[CMD_GLOBAL_SPLIT], SIGNAL(triggered()), this, SLOT(globalSplit()));

      menu_ids[CMD_COPY_RANGE] = menuStructure->addAction(tr("Copy Range"));
      connect(menu_ids[CMD_COPY_RANGE], SIGNAL(triggered()), this, SLOT(copyRange()));

      menu_ids[CMD_COPY_RANGE]->setEnabled(false);
      menuStructure->addSeparator();

      menu_ids[CMD_CUT_EVENTS] = menuStructure->addAction(tr("Cut Events"));
      connect(menu_ids[CMD_CUT_EVENTS], SIGNAL(triggered()), this, SLOT(cutEvents()));

      menu_ids[CMD_CUT_EVENTS]->setEnabled(false);

      //-------------------------------------------------------------
      //    Midi
      //-------------------------------------------------------------

      menu_functions = mb->addMenu(tr("&Midi"));

      menu_ids[CMD_MIDI_EDIT_INSTRUMENTS] = menu_functions->addAction(QIcon(*midi_edit_instrumentIcon), tr("Edit Instrument"));
      connect(menu_ids[CMD_MIDI_EDIT_INSTRUMENTS], SIGNAL(triggered()), this, SLOT(startEditInstrument()));
      //TODO:
      // menu_ids[CMD_MIDI_EDIT_INSTRUMENTS]->setEnabled(false);

//      midiInputPlugins = menu_functions->addMenu(QIcon(*midi_inputpluginsIcon), tr("Input Plugins"));
//      midiInputPlugins->menuAction()->setShortcut(Qt::Key_P);
//      mpid0 = midiInputPlugins->addAction(QIcon(*midi_inputplugins_transposeIcon), tr("Transpose"));
//      mpid0->setData(0);
//      mpid3 = midiInputPlugins->addAction(QIcon(*midi_inputplugins_remote_controlIcon), tr("Midi Remote Control"));
//      mpid3->setData(3);
//TD      connect(midiInputPlugins, SIGNAL(triggered(QAction*)), SLOT(startMidiInputPlugin(QAction*)));

      menu_functions->addSeparator();
      menu_ids[CMD_MIDI_RESET] = menu_functions->addAction(QIcon(*midi_reset_instrIcon), tr("Reset Instr."));
      connect(menu_ids[CMD_MIDI_RESET], SIGNAL(triggered()), this, SLOT(resetMidiDevices()));
      menu_ids[CMD_MIDI_INIT] = menu_functions->addAction(QIcon(*midi_init_instrIcon), tr("Init Instr."));
      connect(menu_ids[CMD_MIDI_INIT], SIGNAL(triggered()), this, SLOT(initMidiDevices()));
      menu_ids[CMD_MIDI_LOCAL_OFF] = menu_functions->addAction(QIcon(*midi_local_offIcon), tr("local off"));
      connect(menu_ids[CMD_MIDI_LOCAL_OFF], SIGNAL(triggered()), this, SLOT(localOff()));

      //-------------------------------------------------------------
      //    Audio
      //-------------------------------------------------------------

      menu_audio = mb->addMenu(tr("&Audio"));

      menu_ids[CMD_AUDIO_BOUNCE_TO_TRACK] = menu_audio->addAction(QIcon(*audio_bounce_to_trackIcon), tr("Bounce to Track"));
      connect(menu_ids[CMD_AUDIO_BOUNCE_TO_TRACK], SIGNAL(triggered()), this, SLOT(bounceToTrack()));

      menu_ids[CMD_AUDIO_BOUNCE_TO_FILE] = menu_audio->addAction(QIcon(*audio_bounce_to_fileIcon), tr("Bounce to File"));
      connect(menu_ids[CMD_AUDIO_BOUNCE_TO_FILE], SIGNAL(triggered()), this, SLOT(bounceToFile()));
      menu_audio->setEnabled(!midiOnly);
      menu_ids[CMD_AUDIO_RESTART] = menu_audio->addAction(QIcon(*audio_restartaudioIcon), tr("Restart Audio"));
      connect(menu_ids[CMD_AUDIO_RESTART], SIGNAL(triggered()),  this, SLOT(seqRestart()));

      //-------------------------------------------------------------
      //    Settings
      //-------------------------------------------------------------

      menuSettings = mb->addMenu(tr("Setti&ngs"));
      menu_ids[CMD_CONFIG_SHORTCUTS] = menuSettings->addAction(QIcon(*settings_configureshortcutsIcon), tr("Configure shortcuts"));
      connect(menu_ids[CMD_CONFIG_SHORTCUTS], SIGNAL(triggered()), this, SLOT(configShortCuts()));

      follow = menuSettings->addMenu(QIcon(*settings_follow_songIcon), tr("follow song"));
      //follow->menuAction()->setShortcut(Qt::Key_F);
      fid0 = follow->addAction(tr("dont follow Song"));
      fid0->setData(CMD_FOLLOW_NO);
      fid0->setCheckable(true);
      fid1 = follow->addAction(tr("follow page"));
      fid1->setData(CMD_FOLLOW_JUMP);
      fid1->setCheckable(true);
      fid2 = follow->addAction(tr("follow continuous"));
      fid2->setData(CMD_FOLLOW_CONTINUOUS);
      fid2->setCheckable(true);
      fid0->setChecked(TimeCanvas::followMode == FOLLOW_NO);
      fid1->setChecked(TimeCanvas::followMode == FOLLOW_JUMP);
      fid2->setChecked(TimeCanvas::followMode == FOLLOW_CONTINUOUS);
      connect(follow, SIGNAL(triggered(QAction*)), SLOT(cmd(QAction*)));

      menuSettings->addSeparator();
      menu_ids[CMD_CONFIG_MIDISYNC] = menuSettings->addAction(QIcon(*settings_midisyncIcon), tr("Midi Sync"));
      connect(menu_ids[CMD_CONFIG_MIDISYNC], SIGNAL(triggered()), this, SLOT(configMidiSync()));
      menu_ids[CMD_MIDI_FILE_CONFIG] = menuSettings->addAction(QIcon(*settings_midifileexportIcon), tr("Midi File Export"));
      connect(menu_ids[CMD_MIDI_FILE_CONFIG], SIGNAL(triggered()), this, SLOT(configMidiFile()));
      menuSettings->addSeparator();
      QAction* action = menuSettings->addAction(QIcon(*settings_globalsettingsIcon), tr("Preferences"));
      connect(action, SIGNAL(triggered()), this, SLOT(preferences()));

      //---------------------------------------------------
      //    Help
      //---------------------------------------------------

      mb->addSeparator();
      menu_help = mb->addMenu(tr("&Help"));

      menu_ids[CMD_OPEN_HELP] = menu_help->addAction(tr("&Manual"));
      connect(menu_ids[CMD_OPEN_HELP], SIGNAL(triggered()), this, SLOT(startHelpBrowser()));
      menu_ids[CMD_OPEN_HOMEPAGE] = menu_help->addAction(tr("&MusE homepage"));
      connect(menu_ids[CMD_OPEN_HOMEPAGE], SIGNAL(triggered()), this, SLOT(startHomepageBrowser()));
      menu_help->addSeparator();
      menu_ids[CMD_OPEN_BUG] = menu_help->addAction(tr("&Report Bug..."));
      connect(menu_ids[CMD_OPEN_BUG], SIGNAL(triggered()), this, SLOT(startBugBrowser()));
      menu_help->addSeparator();
      a = menu_help->addAction(tr("&About MusE"));
      a->setIcon(QIcon(*museIcon));
      connect(a, SIGNAL(triggered()), this, SLOT(about()));
      a = menu_help->addAction(tr("About&Qt"));
      connect(a, SIGNAL(triggered()), this, SLOT(aboutQt()));
      menu_help->addSeparator();
      a = QWhatsThis::createAction(this);
      a->setText(tr("What's &This?"));
      menu_help->addAction(a);
      menu_ids[CMD_START_WHATSTHIS] = a;

      //---------------------------------------------------
      //  ToolBar
      //---------------------------------------------------

      QToolBar* aToolBar = addToolBar(tr("Arranger"));

      QLabel* label = new QLabel(tr("Cursor"));
      label->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
      label->setIndent(3);
      aToolBar->addWidget(label);

      Awl::PosLabel* cursorPos = new Awl::PosLabel;
      aToolBar->addWidget(cursorPos);
      cursorPos->setFixedHeight(25);

      label = new QLabel(tr("Snap"));
      label->setIndent(5);
      aToolBar->addWidget(label);

      rasterCombo = new QComboBox;
      rasterCombo->setFixedHeight(24);
      aToolBar->addWidget(rasterCombo);
      for (unsigned i = 0; i < sizeof(rasterTable)/sizeof(*rasterTable); i++)
            rasterCombo->addItem(rasterTable[i].label, i);
      rasterCombo->setCurrentIndex(1);
      connect(rasterCombo, SIGNAL(activated(int)), SLOT(setRaster(int)));

      // Song len
      label = new QLabel(tr("Len"));
      label->setIndent(5);
      aToolBar->addWidget(label);

      // song length is limited to 10000 bars; the real song len is limited
      // by overflows in tick computations
      //
      QSpinBox* lenEntry = new QSpinBox;
      lenEntry->setFixedHeight(24);
      lenEntry->setRange(1, 10000);
      aToolBar->addWidget(lenEntry);
      lenEntry->setValue(song->len());
      connect(lenEntry, SIGNAL(valueChanged(int)), song, SLOT(setMeasureLen(int)));
      connect(song, SIGNAL(measureLenChanged(int)), lenEntry, SLOT(setValue(int)));

      label = new QLabel(tr("Pitch"));
      label->setIndent(5);
      aToolBar->addWidget(label);

      QSpinBox* globalPitchSpinBox = new QSpinBox;
      globalPitchSpinBox->setFixedHeight(24);
      globalPitchSpinBox->setRange(-127, 127);
      aToolBar->addWidget(globalPitchSpinBox);
      globalPitchSpinBox->setValue(song->globalPitchShift());
      globalPitchSpinBox->setToolTip(tr("midi pitch"));
      globalPitchSpinBox->setWhatsThis(tr("global midi pitch shift"));
      connect(globalPitchSpinBox, SIGNAL(valueChanged(int)), SLOT(globalPitchChanged(int)));

      label = new QLabel(tr("Tempo"));
      label->setIndent(5);
      aToolBar->addWidget(label);

      globalTempoSpinBox = new QSpinBox;
      globalTempoSpinBox->setFixedHeight(24);
      globalTempoSpinBox->setRange(50, 200);
      aToolBar->addWidget(globalTempoSpinBox);
      globalTempoSpinBox->setSuffix(QString("%"));
      globalTempoSpinBox->setValue(AL::tempomap.globalTempo());
      globalTempoSpinBox->setToolTip(tr("midi tempo"));
      globalTempoSpinBox->setWhatsThis(tr("midi tempo"));
      connect(globalTempoSpinBox, SIGNAL(valueChanged(int)), SLOT(globalTempoChanged(int)));

      QToolButton* tempo50  = new QToolButton;
      tempo50->setFixedHeight(24);
      aToolBar->addWidget(tempo50);
      tempo50->setText(QString("50%"));
      connect(tempo50, SIGNAL(clicked()), SLOT(setTempo50()));

      QToolButton* tempo100 = new QToolButton;
      tempo100->setFixedHeight(24);
      aToolBar->addWidget(tempo100);
      tempo100->setText(tr("N"));
      connect(tempo100, SIGNAL(clicked()), SLOT(setTempo100()));

      QToolButton* tempo200 = new QToolButton;
      tempo200->setFixedHeight(24);
      aToolBar->addWidget(tempo200);
      tempo200->setText(QString("200%"));
      connect(tempo200, SIGNAL(clicked()), SLOT(setTempo200()));

      //---------------------------------------------------
      //    Central Widget
      //---------------------------------------------------

      arranger = new Arranger(this);
      setCentralWidget(arranger);

      connect(tools1, SIGNAL(toolChanged(int)), SLOT(setTool(int)));
      connect(arranger, SIGNAL(toolChanged(int)), SLOT(setTool(int)));
      connect(arranger, SIGNAL(editPart(Part*)), SLOT(startEditor(Part*)));
//TODO1      connect(arranger, SIGNAL(dropSongFile(const QString&)), SLOT(loadProject(const QString&)));
//TODO1      connect(arranger, SIGNAL(dropMidiFile(const QString&)), SLOT(importMidi(const QString&)));
      connect(arranger, SIGNAL(cursorPos(const AL::Pos&,bool)), cursorPos, SLOT(setValue(const AL::Pos&,bool)));

      //---------------------------------------------------
      //  read list of "Recent Projects"
      //---------------------------------------------------

      QString prjPath(getenv("HOME"));
      prjPath += QString("/.musePrj");
      FILE* f = fopen(prjPath.toLatin1().data(), "r");
      if (f == 0) {
            if (debugMsg) {
                  fprintf(stderr, "open projectfile <%s> failed: %s",
                     prjPath.toLatin1().data(), strerror(errno));
                  }
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

      initMidiSynth();

      transport = new Transport;
      transport->hide();
	connect(transport, SIGNAL(closed()), SLOT(transportClosed()));

      QClipboard* cb = QApplication::clipboard();
      connect(cb, SIGNAL(dataChanged()), SLOT(clipboardChanged()));
      connect(cb, SIGNAL(selectionChanged()), SLOT(clipboardChanged()));
      song->blockSignals(false);
      }

//---------------------------------------------------------
//   aboutToShowAddTrack
//---------------------------------------------------------

void MusE::aboutToShowAddTrack()
      {
      populateAddTrack(addTrack);
      }

//---------------------------------------------------------
//   setRaster
//---------------------------------------------------------

void MusE::setRaster(int val)
      {
      _raster = rasterTable[val].val;
      emit rasterChanged(_raster);
      }

//---------------------------------------------------------
//   initRaster
//---------------------------------------------------------

void MusE::initRaster(int val)
      {
      for (unsigned i = 0; i < sizeof(rasterTable)/sizeof(*rasterTable); ++i) {
            if (rasterTable[i].val == val) {
                  _raster = val;
                  rasterCombo->setCurrentIndex(i);
                  return;
                  }
            }
      _raster = rasterTable[1].val;
      rasterCombo->setCurrentIndex(1);
      }

//---------------------------------------------------------
//   setHeartBeat
//---------------------------------------------------------

void MusE::setHeartBeat()
      {
      heartBeatTimer->start(1000/config.guiRefresh);
      }

//---------------------------------------------------------
//   resetDevices
//---------------------------------------------------------

void MusE::resetMidiDevices()
      {
      audio->msgResetMidiDevices();
      }

//---------------------------------------------------------
//   initMidiDevices
//---------------------------------------------------------

void MusE::initMidiDevices()
      {
      audio->msgInitMidiDevices();
      }

//---------------------------------------------------------
//   localOff
//---------------------------------------------------------

void MusE::localOff()
      {
      audio->msgLocalOff();
      }

//---------------------------------------------------------
//   loadProject
//---------------------------------------------------------

void MusE::loadProject(const QString& path)
      {
      //
      // stop audio threads if running
      //
      bool restartSequencer = audioState == AUDIO_RUNNING;
      if (restartSequencer) {
            if (audio->isPlaying()) {
                  audio->msgPlay(false);
                  while (audio->isPlaying())
                        qApp->processEvents();
                  }
            seqStop();
            }
      loadProject1(path);
      if (restartSequencer)
            seqStart();
      audio->msgSeek(song->cPos());
      }

//---------------------------------------------------------
//   loadProject1
//---------------------------------------------------------

void MusE::loadProject1(const QString& path)
      {
      QFileInfo file(path);
      QString header = tr("MusE: new project");

      if (leaveProject())
            return;

      if (mixer1)
            mixer1->clear();
      if (mixer2)
            mixer2->clear();
      
      QString name(file.fileName());
      QDir pd(QDir::homePath() + "/" + config.projectPath + "/" + path);
      
      addProject(path);       // add to history

      bool newProject = false;
      if (!pd.exists()) {
            newProject = true;
            if (!pd.mkdir(pd.path())) {
                  QString s(tr("Cannot create project folder <%1>"));
                  QMessageBox::critical(this, header, s.arg(pd.path()));
                  return;
                  }
            }
      //
      // close all toplevel windows
      //
      foreach(QWidget* w, QApplication::topLevelWidgets()) {
            if (!w->isVisible())
                  continue;
            static const char* const top[] = {
                  "DrumEdit", "PianoRoll", "MasterEdit", "WaveEdit",
                  "ListEdit", "PluginGui"
                  };
            for (unsigned i = 0; i < sizeof(top)/sizeof(*top); ++i) {
                  if (strcmp(top[i], w->metaObject()->className()) == 0) {
                        w->close();
                        break;
                        }
                  }
            }
      emit startLoadSong();

      song->setProjectPath(path);
      song->clear(false);
      song->setCreated(newProject);

      QString s = pd.absoluteFilePath(name + ".med");

      QFile f(s);

      song->blockSignals(true);

      bool rv = true;
      if (f.open(QIODevice::ReadOnly)) {
            rv = song->read(&f);
            f.close();
            }
      else {
            TemplateDialog templateDialog;
            if (templateDialog.exec() == 1) {
                  s  = templateDialog.templatePath();
                  if (!s.isEmpty()) {
                        QFile f(s);
                        if (f.open(QIODevice::ReadOnly)) {
                              rv = song->read(&f);
                              f.close();
                              }
                        else {
                              QString msg(tr("Cannot open template file\n%1"));
                              QMessageBox::critical(this, header, msg.arg(s));
                              }
                        }
                  }
            }
      if (!rv) {
            QString msg(tr("File <%1> read error"));
            QMessageBox::critical(this, header, msg.arg(s));
            }
      tr_id->setChecked(config.transportVisible);
      bt_id->setChecked(config.bigTimeVisible);

      showBigtime(config.bigTimeVisible);
      showMixer1(config.mixer1Visible);
      showMixer2(config.mixer2Visible);
      if (mixer1 && config.mixer1Visible)
            mixer1->setUpdateMixer();
      if (mixer2 && config.mixer2Visible)
            mixer2->setUpdateMixer();
      resize(config.geometryMain.size());
      move(config.geometryMain.topLeft());
      if (config.transportVisible)
            transport->show();
      transport->move(config.geometryTransport.topLeft());
      showTransport(config.transportVisible);
      song->blockSignals(false);
      transport->setMasterFlag(song->masterFlag());
      punchinAction->setChecked(song->punchin());
      punchoutAction->setChecked(song->punchout());
      loopAction->setChecked(song->loop());
      clipboardChanged();           // enable/disable "Paste"
      song->setLen(song->len());    // emit song->lenChanged() signal

      selectionChanged();           // enable/disable "Copy" & "Paste"
      arranger->endLoadSong();
      song->updatePos();
      song->updateCurrentMarker();
      //
      // send "cur" controller values to devices
      //

      TrackList* tl = song->tracks();
      for (iTrack i = tl->begin(); i != tl->end(); ++i) {
            Track* track = *i;
//            track->blockSignals(true);
            CtrlList* cl = track->controller();
            for (iCtrl ic = cl->begin(); ic != cl->end(); ++ic) {
                  Ctrl* ctrl = ic->second;
                  if (ctrl->type() & Ctrl::INT) {
                        CVal val;
                        val = ctrl->curVal();
                        if (track->isMidiTrack() && val.i == CTRL_VAL_UNKNOWN)
                              continue;
                        ctrl->setCurVal(CTRL_VAL_UNKNOWN);
                        song->setControllerVal(track, ctrl, val);
                        }
                  }
//            track->blockSignals(false);
            }
      setWindowTitle(QString("MusE: Song: ") + name);
      }

//---------------------------------------------------------
//   MusE::loadProject
//---------------------------------------------------------

void MusE::loadProject()
      {
      ProjectDialog projectDialog;
      int rv = projectDialog.exec();
      if (rv == 0)
            return;
      QString path = projectDialog.projectPath();
      if (path.isEmpty())
            return;
      loadProject(path);
      }

//---------------------------------------------------------
//   save
//---------------------------------------------------------

bool MusE::save()
      {
      QString backupCommand;

      QString name(song->projectName() + ".med");
      QFileInfo fi(song->absoluteProjectPath() + "/" + name);

      QTemporaryFile tmp(fi.path() + "/MusEXXXXXX");
      tmp.setAutoRemove(false);

      if (!tmp.open()) {
            QString s("Creating temp file failed: ");
            s += strerror(errno);
            QMessageBox::critical(this,
               tr("MusE: Create tmp file failed"), s);
            return false;
            }
      Xml xml(&tmp);
      write(xml);
      if (tmp.error()) {
            QString s = QString("Write File\n") + tmp.fileName() + QString("\nfailed: ")
               + tmp.errorString();
            QMessageBox::critical(this, tr("MusE: Write File failed"), s);
            return false;
            }
      if (!song->backupWritten()) {
            //
            // remove old backup file
            //
            QDir dir(fi.path());
            QString backupName = QString(".") + fi.fileName() + QString(",");
            dir.remove(backupName);

            //
            // rename old file to backup
            //
            QString n(fi.filePath());
            dir.rename(n, backupName);
            }
      //
      //  rename temp name to file name
      //
      tmp.rename(fi.filePath());

      song->dirty = false;
      SndFile::updateRecFiles();
      return true;
      }

//---------------------------------------------------------
//   quitDoc
//---------------------------------------------------------

void MusE::quitDoc()
      {
      close();
      }

//---------------------------------------------------------
//    leaveProject
//    return false if user aborts operation
//---------------------------------------------------------

bool MusE::leaveProject()
      {
      if (song->dirty) {
            int n = 0;
            n = QMessageBox::warning(this, appName,
               tr("The current Project contains unsaved data\n"
               "Save Current Project?"),
               tr("&Save"), tr("&Nosave"), tr("&Abort"), 0, 2);
            if (n == 0)
                  return !save();
            else if (n == 2)
                  return true;
	      //
	      // delete all wave files created in this session and not
      	// referenced any more
            // delete all if we drop the song
	      //
       	SndFile::cleanupRecFiles(n == 1);
            }
      else
            SndFile::cleanupRecFiles(true);
      //
      // if this is a new created project,
      //   delete project directory
      //
      if (song->created()) {
            // delete project directory
            QDir pp;
            if (!pp.rmdir(song->absoluteProjectPath()))
                  printf("cannot remove dir <%s>\n", song->absoluteProjectPath().toLatin1().data());
            }
      return false;
      }

//---------------------------------------------------------
//   closeEvent
//---------------------------------------------------------

void MusE::closeEvent(QCloseEvent* event)
      {
      song->setStop(true);
      //
      // wait for sequencer
      //
      while (audio->isPlaying()) {
            qApp->processEvents();
            }

      if (leaveProject()) {
            event->ignore();
            return;
            }

      seqStop();

      // save "Open Recent" list
      QString prjPath(getenv("HOME"));
      prjPath += "/.musePrj";
      FILE* f = fopen(prjPath.toLatin1().data(), "w");
      if (f) {
            for (int i = 0; i < PROJECT_LIST_LEN; ++i) {
                  fprintf(f, "%s\n", projectList[i] ? projectList[i]->toLatin1().data() : "");
                  }
            fclose(f);
            }
      exitJackAudio();
      SynthIList* sl = song->syntis();
      for (iSynthI i = sl->begin(); i != sl->end(); ++i)
            delete *i;

      // Cleanup temporary wavefiles + peakfiles used for undo
      for (std::list<QString>::iterator i = temporaryWavFiles.begin(); i != temporaryWavFiles.end(); i++) {
            QString filename = *i;
            QFileInfo f(filename);
            QDir d = f.dir();
            d.remove(filename);
            d.remove(f.baseName() + ".wca");
            }
      qApp->quit();
      }

//---------------------------------------------------------
//   showTransport
//---------------------------------------------------------

void MusE::showTransport(bool flag)
      {
      transport->setShown(flag);
      tr_id->setChecked(flag);
      if (flag)
            transport->setValues();
      }

//---------------------------------------------------------
//   printVersion
//---------------------------------------------------------

static void printVersion(const char* prog)
      {
      fprintf(stderr, "%s: Linux Music Editor; Version %s\n", prog, VERSION);
      }

//---------------------------------------------------------
//   startEditor
//---------------------------------------------------------

void MusE::startEditor(PartList* pl, int type)
      {
      switch (type) {
            case 0: startPianoroll(pl); break;
            case 1: startListEditor(pl); break;
            case 2: startMidiTrackerEditor(pl); break;
            case 3: startDrumEditor(pl); break;
            case 4: startWaveEditor(pl); break;
            }
      }

//---------------------------------------------------------
//   startEditor
//---------------------------------------------------------

void MusE::startEditor(Part* part, int type)
      {
      PartList* pl = new PartList();
      pl->add(part);
      startEditor(pl, type);
      }

//---------------------------------------------------------
//   startEditor
//---------------------------------------------------------

void MusE::startEditor(Part* part)
      {
      PartList* pl = new PartList();
      pl->add(part);
      Track* track = part->track();
      switch (track->type()) {
            case Track::MIDI:
                  {
                  MidiTrack* t = (MidiTrack*)track;
                  if (t->useDrumMap())
                        startDrumEditor(pl);
                  else
                        startPianoroll(pl);
                  }
                  break;
            case Track::WAVE:
                  startWaveEditor(pl);
                  break;
            default:
                  break;
            }
      }

//---------------------------------------------------------
//   getMidiPartsToEdit
//---------------------------------------------------------

PartList* MusE::getMidiPartsToEdit()
      {
      PartList* pl = song->getSelectedMidiParts();
      if (pl->empty()) {
            QMessageBox::critical(this, QString("MusE"), tr("Nothing to edit"));
            delete pl;
            return 0;
            }
      return pl;
      }

//---------------------------------------------------------
//   startPianoroll
//---------------------------------------------------------

void MusE::startPianoroll()
      {
      PartList* pl = getMidiPartsToEdit();
      if (pl == 0)
            return;
      startPianoroll(pl);
      }

void MusE::startPianoroll(PartList* pl)
      {
      PianoRoll* pianoroll = new PianoRoll(pl, false);
      pianoroll->show();
      connect(muse, SIGNAL(configChanged()), pianoroll, SLOT(configChanged()));
      }

//---------------------------------------------------------
//   startListEditor
//---------------------------------------------------------

void MusE::startListEditor()
      {
//      PartList* pl = getMidiPartsToEdit();
//      if (pl == 0)
//            return;
//      startListEditor(pl);
      startListEditor(0);
      }

void MusE::startListEditor(PartList* /*pl*/)
      {
      if (listEditor == 0)
            listEditor = new ListEdit(this);
      listEditor->show();
      }

void MusE::showListEditor(const Pos& pos, Track* track, Ctrl* ctrl)
      {
      if (listEditor == 0)
            listEditor = new ListEdit(this);
      listEditor->selectItem(pos, track, 0, ctrl);
      listEditor->show();
      }

//---------------------------------------------------------
//   startMidiTrackerEditor
//---------------------------------------------------------

void MusE::startMidiTrackerEditor()
      {
      PartList* pl = getMidiPartsToEdit();
      if (pl == 0)
            return;
      startMidiTrackerEditor(pl);
      }

void MusE::startMidiTrackerEditor(PartList* pl)
{
  MidiTrackerEditor* miditracker = new MidiTrackerEditor(pl, false);
  miditracker->show();
  connect(muse, SIGNAL(configChanged()), miditracker, SLOT(configChanged()));
}

//---------------------------------------------------------
//   startMasterEditor
//---------------------------------------------------------

void MusE::startMasterEditor()
      {
      MasterEdit* masterEditor = new MasterEdit();
      masterEditor->show();
      }

//---------------------------------------------------------
//   showProjectPropsDialog
//---------------------------------------------------------

void MusE::showProjectPropsDialog()
      {
      if (projectPropsDialog == 0)
            projectPropsDialog = new ProjectPropsDialog(this);
      projectPropsDialog->show();
      }

//---------------------------------------------------------
//   startDrumEditor
//---------------------------------------------------------

void MusE::startDrumEditor()
      {
      PartList* pl = getMidiPartsToEdit();
      if (pl == 0)
            return;
      startDrumEditor(pl);
      }

void MusE::startDrumEditor(PartList* pl)
      {
      DrumEdit* drumEditor = new DrumEdit(pl, false);
      drumEditor->show();
      connect(muse, SIGNAL(configChanged()), drumEditor, SLOT(configChanged()));
      }

//---------------------------------------------------------
//   startWaveEditor
//---------------------------------------------------------

void MusE::startWaveEditor()
      {
      PartList* pl = song->getSelectedWaveParts();
      if (pl->empty()) {
            QMessageBox::critical(this, QString("MusE"), tr("Nothing to edit"));
            delete pl;
            return;
            }
      startWaveEditor(pl);
      }

void MusE::startWaveEditor(PartList* pl)
      {
      WaveEdit* waveEditor = new WaveEdit(pl, false);
      waveEditor->show();
      connect(muse, SIGNAL(configChanged()), waveEditor, SLOT(configChanged()));
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
            const char* path = projectList[i]->toLatin1().data();
            const char* p = strrchr(path, '/');
            if (p == 0)
                  p = path;
            else
                  ++p;
            QAction* a = openRecent->addAction(QString(p));
            a->setData(i);
            }
      }

//---------------------------------------------------------
//   selectProject
//---------------------------------------------------------

void MusE::selectProject(QAction* a)
      {
      if (a == 0)
            return;
      int id = a->data().toInt();
      if (id < 0)
            return;
      assert(id < PROJECT_LIST_LEN);
      QString* name = projectList[id];
      if (name == 0)
            return;
      loadProject(*name);
      }

//---------------------------------------------------------
//   playToggle
//---------------------------------------------------------

void MusE::playToggle()
      {
      if (audio->isPlaying())
            song->setStop(true);
      else if (song->cpos() != song->lpos())
            song->setPos(0, song->lPos());
      else {
            Pos p(0, AL::TICKS);
            song->setPos(0, p);
            }
      }

//---------------------------------------------------------
//   kbAccel
//---------------------------------------------------------

void MusE::kbAccel(int /*key*/)
      {
#if 0 //TODOB
      if (key == shortcuts[SHRT_TOGGLE_METRO].key) {
            song->setClick(!song->click());
            }
      else if (key == shortcuts[SHRT_STOP].key) {
            song->setStop(true);
            }
      else if (key == shortcuts[SHRT_PLAY_SONG].key ) {
            song->setPlay(true);
            }
      else if (key == shortcuts[SHRT_GOTO_LEFT].key) {
            if (!song->record())
                  song->setPos(0, song->lPos());
            }
      else if (key == shortcuts[SHRT_GOTO_RIGHT].key) {
            if (!song->record())
                  song->setPos(0, song->rPos());
            }
      else if (key == shortcuts[SHRT_TOGGLE_LOOP].key) {
            song->setLoop(!song->loop());
            }
      else if (key == shortcuts[SHRT_START_REC].key) {
            if (!audio->isPlaying()) {
                  song->setRecord(!song->record());
                  }
            }
      else if (key == shortcuts[SHRT_OPEN_TRANSPORT].key) {
            showTransport(!tr_id->isChecked());
            }
      else if (key == shortcuts[SHRT_OPEN_BIGTIME].key) {
            showBigtime(!bt_id->isChecked());
            }
      else if (key == shortcuts[SHRT_OPEN_MIXER].key) {
            showMixer1(!aid1a->isChecked());
            }
      else {
            if (debugMsg)
                  printf("unknown kbAccel 0x%x\n", key);
            }
#endif
      }

//---------------------------------------------------------
//   MuseApplication
//---------------------------------------------------------

class MuseApplication : public QApplication {
      MusE* muse;

   public:
      MuseApplication(int& argc, char** argv)
         : QApplication(argc, argv)
            {
            muse = 0;
            }

      void setMuse(MusE* m) {
            muse = m;
            }

      bool notify(QObject* receiver, QEvent* event) 
            {
            bool flag = QApplication::notify(receiver, event);
            if (event->type() == QEvent::KeyPress) {
                  QKeyEvent* ke = (QKeyEvent*)event;
                  bool accepted = ke->isAccepted();
                  if (!accepted) {
                        muse->kbAccel(ke->key());
                        return true;
                        }
                  }
            return flag;
            }
      };

//---------------------------------------------------------
//   usage
//---------------------------------------------------------

static void usage(const char* prog, const char* txt)
      {
      fprintf(stderr, "%s: %s\nusage: %s flags midifile\n   Flags:",
         prog, txt, prog);
      fprintf(stderr, "   -v       print version\n");
      fprintf(stderr, "   -m       MIDI only mode\n");
      fprintf(stderr, "   -d       debug mode: no threads, no RT\n");
      fprintf(stderr, "   -D       debug mode: enable some debug messages\n");
      fprintf(stderr, "   -i       debug mode: trace midi Input\n");
      fprintf(stderr, "   -o       debug mode: trace midi Output\n");
      fprintf(stderr, "   -s       debug mode: trace sync\n");
      fprintf(stderr, "   -p       don't load LADSPA plugins\n");
#ifdef VST_SUPPORT
      fprintf(stderr, "   -V       don't load VST plugins\n");
#endif
#ifdef DSSI_SUPPORT
      fprintf(stderr, "   -I       don't load DSSI plugins\n");
#endif
      }

//---------------------------------------------------------
//   catchSignal
//    only for debugging
//---------------------------------------------------------

#if 0
static void catchSignal(int sig)
      {
      if (debugMsg)
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
//   setFollow
//---------------------------------------------------------

void MusE::setFollow(FollowMode fm)
      {
      TimeCanvas::followMode = fm;
      fid0->setChecked(fm == FOLLOW_NO);
      fid1->setChecked(fm == FOLLOW_JUMP);
      fid2->setChecked(fm == FOLLOW_CONTINUOUS);
      changeConfig(true);    // save settings
      }

//---------------------------------------------------------
//   cmd
//    some cmd's from pulldown menu
//---------------------------------------------------------

void MusE::cmd(QAction* a)
      {
      int cmd = a->data().toInt();
      TrackList* tracks = song->tracks();
      int l = song->lpos();
      int r = song->rpos();

      switch(cmd) {
            case CMD_CUT:
//TODO1                  arranger->cmd(Arranger::CMD_CUT_PART);
                  break;
            case CMD_COPY:
//TODO1                  arranger->cmd(Arranger::CMD_COPY_PART);
                  break;
            case CMD_PASTE:
//TODO1                  arranger->cmd(Arranger::CMD_PASTE_PART);
                  break;
            case CMD_DELETE:
                  {
                  TrackList* tl = song->tracks();
                  bool partsMarked = false;
                  for (iTrack it = tl->begin(); it != tl->end(); ++it) {
      	            PartList* pl2 = (*it)->parts();
                        for (iPart ip = pl2->begin(); ip != pl2->end(); ++ip) {
            	            if (ip->second->selected()) {
                  	            partsMarked = true;
                                    break;
                                    }
                              }
                        }
                  if (partsMarked)
                        song->cmdRemoveParts();
                  else
                        audio->msgRemoveTracks();
                  }
                  break;

            case CMD_DELETE_TRACK:
                  audio->msgRemoveTracks();
                  break;

            case CMD_SELECT_ALL:
            case CMD_SELECT_NONE:
            case CMD_SELECT_INVERT:
            case CMD_SELECT_ILOOP:
            case CMD_SELECT_OLOOP:
                  for (iTrack i = tracks->begin(); i != tracks->end(); ++i) {
                        PartList* parts = (*i)->parts();
                        for (iPart p = parts->begin(); p != parts->end(); ++p) {
                              bool f = false;
                              int t1 = p->second->tick();
                              int t2 = t1 + p->second->lenTick();
                              bool inside =
                                 ((t1 >= l) && (t1 < r))
                                 ||  ((t2 > l) && (t2 < r))
                                 ||  ((t1 <= l) && (t2 > r));
                              switch(cmd) {
                                    case CMD_SELECT_INVERT:
                                          f = !p->second->selected();
                                          break;
                                    case CMD_SELECT_NONE:
                                          f = false;
                                          break;
                                    case CMD_SELECT_ALL:
                                          f = true;
                                          break;
                                    case CMD_SELECT_ILOOP:
                                          f = inside;
                                          break;
                                    case CMD_SELECT_OLOOP:
                                          f = !inside;
                                          break;
                                    }
                              p->second->setSelected(f);
                              }
                        (*i)->partListChanged(); // repaints canvaswidget
                        }
                  song->update();
                  break;

            case CMD_SELECT_PARTS:
                  for (iTrack i = tracks->begin(); i != tracks->end(); ++i) {
                        if (!(*i)->selected())
                              continue;
                        PartList* parts = (*i)->parts();
                        for (iPart p = parts->begin(); p != parts->end(); ++p)
                              p->second->setSelected(true);
                        }
                  song->update();
                  break;
            case CMD_FOLLOW_NO:
                  setFollow(FOLLOW_NO);
                  break;
            case CMD_FOLLOW_JUMP:
                  setFollow(FOLLOW_JUMP);
                  break;
            case CMD_FOLLOW_CONTINUOUS:
                  setFollow(FOLLOW_CONTINUOUS);
                  break;
            }
      }

//---------------------------------------------------------
//   clipboardChanged
//---------------------------------------------------------

void MusE::clipboardChanged()
      {
#if 0 //TD
      QString subtype("partlist");
      QMimeSource* ms = QApplication::clipboard()->data(QClipboard::Clipboard);
      if (ms == 0)
            return;
      bool flag = false;
      for (int i = 0; ms->format(i); ++i) {
// printf("Format <%s\n", ms->format(i));
            if ((strncmp(ms->format(i), "text/midipartlist", 17) == 0)
               || strncmp(ms->format(i), "text/wavepartlist", 17) == 0) {
                  flag = true;
                  break;
                  }
            }
      pasteAction->setEnabled(flag);
#endif
      }

//---------------------------------------------------------
//   selectionChanged
//---------------------------------------------------------

void MusE::selectionChanged()
      {
      int k = 0;
      TrackList* tl = song->tracks();
      for (iTrack t = tl->begin(); t != tl->end(); ++t)
            k += (*t)->selected();
      cutAction->setEnabled(k == 1);
      copyAction->setEnabled(k == 1);
      song->updateSelectedTrack();
      }

//---------------------------------------------------------
//   transpose
//---------------------------------------------------------

void MusE::transpose()
      {
      Transpose *w = new Transpose();
      w->show();
      }

//---------------------------------------------------------
//   modifyGateTime
//---------------------------------------------------------

void MusE::modifyGateTime()
      {
//TODO      GateTime* w = new GateTime(this);
//      w->show();
      }

//---------------------------------------------------------
//   modifyVelocity
//---------------------------------------------------------

void MusE::modifyVelocity()
      {
      printf("not implemented\n");
      }

//---------------------------------------------------------
//   crescendo
//---------------------------------------------------------

void MusE::crescendo()
      {
      printf("not implemented\n");
      }

//---------------------------------------------------------
//   thinOut
//---------------------------------------------------------

void MusE::thinOut()
      {
      printf("not implemented\n");
      }

//---------------------------------------------------------
//   eraseEvent
//---------------------------------------------------------

void MusE::eraseEvent()
      {
      printf("not implemented\n");
      }

//---------------------------------------------------------
//   noteShift
//---------------------------------------------------------

void MusE::noteShift()
      {
      printf("not implemented\n");
      }

//---------------------------------------------------------
//   moveClock
//---------------------------------------------------------

void MusE::moveClock()
      {
      printf("not implemented\n");
      }

//---------------------------------------------------------
//   copyMeasure
//---------------------------------------------------------

void MusE::copyMeasure()
      {
      printf("not implemented\n");
      }

//---------------------------------------------------------
//   eraseMeasure
//---------------------------------------------------------

void MusE::eraseMeasure()
      {
      printf("not implemented\n");
      }

//---------------------------------------------------------
//   deleteMeasure
//---------------------------------------------------------

void MusE::deleteMeasure()
      {
      printf("not implemented\n");
      }

//---------------------------------------------------------
//   createMeasure
//---------------------------------------------------------

void MusE::createMeasure()
      {
      printf("not implemented\n");
      }

//---------------------------------------------------------
//   mixTrack
//---------------------------------------------------------

void MusE::mixTrack()
      {
      printf("not implemented\n");
      }

//---------------------------------------------------------
//   configAppearance
//---------------------------------------------------------
//
//void MusE::configAppearance()
//      {
//      if (!appearance)
//            appearance = new Appearance(arranger);
//      appearance->resetValues();
//      appearance->show();
//      }

//---------------------------------------------------------
//   preferences
//---------------------------------------------------------

void MusE::preferences()
      {
      if (!preferencesDialog)
            preferencesDialog = new PreferencesDialog(arranger);
      preferencesDialog->resetValues();
      preferencesDialog->show();
      }

//---------------------------------------------------------
//   loadTheme
//---------------------------------------------------------

void MusE::loadTheme(const QString& s)
      {
      if (style()->objectName() != s)
            QApplication::setStyle(s);
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
	loadTheme(config.style);
      QApplication::setFont(config.fonts[0]);
      updateConfiguration();
      emit configChanged();
      }

//---------------------------------------------------------
//   configShortCuts
//---------------------------------------------------------

void MusE::configShortCuts()
      {
      if (!shortcutConfig)
            shortcutConfig = new ShortcutConfig(this);
      shortcutConfig->_config_changed = false;
      if (shortcutConfig->exec())
            changeConfig(true);
      }

//---------------------------------------------------------
//   globalCut
//    - remove area between left and right locator
//    - do not touch muted track
//    - cut master track
//---------------------------------------------------------

void MusE::globalCut()
      {
      int lpos = song->lpos();
      int rpos = song->rpos();
      if ((lpos - rpos) >= 0)
            return;

      song->startUndo();
      MidiTrackList* tracks = song->midis();
      for (iMidiTrack it = tracks->begin(); it != tracks->end(); ++it) {
            MidiTrack* track = *it;
            if (track->mute())
                  continue;
            PartList* pl = track->parts();
            for (iPart p = pl->begin(); p != pl->end(); ++p) {
                  Part* part = p->second;
                  int t = part->tick();
                  int l = part->lenTick();
                  if (t + l <= lpos)
                        continue;
                  if ((t >= lpos) && ((t+l) <= rpos)) {
                        song->removePart(part);
                        }
                  else if ((t < lpos) && ((t+l) > lpos) && ((t+l) <= rpos)) {
                        // remove part tail
                        int len = lpos - t;
                        Part* nPart = new Part(*part);
                        nPart->setLenTick(len);
                        //
                        // cut Events in nPart
                        EventList* el = nPart->events();
                        iEvent ie = el->lower_bound(t + len);
                        for (; ie != el->end();) {
                              iEvent i = ie;
                              ++ie;
                              audio->msgDeleteEvent(i->second, nPart, false);
                              }
                        song->changePart(part, nPart);
                        }
                  else if ((t < lpos) && ((t+l) > lpos) && ((t+l) > rpos)) {
                        //----------------------
                        // remove part middle
                        //----------------------

                        Part* nPart = new Part(*part);
                        EventList* el = nPart->events();
                        iEvent is = el->lower_bound(lpos);
                        iEvent ie = el->upper_bound(rpos);
                        for (iEvent i = is; i != ie;) {
                              iEvent ii = i;
                              ++i;
                              audio->msgDeleteEvent(ii->second, nPart, false);
                              }

                        ie = el->lower_bound(rpos);
                        for (; ie != el->end();) {
                              iEvent i = ie;
                              ++ie;
                              Event event = i->second;
                              Event nEvent = event.clone();
                              nEvent.setTick(nEvent.tick() - (rpos-lpos));
                              audio->msgChangeEvent(event, nEvent, nPart, false);
                              }
                        nPart->setLenTick(l - (rpos-lpos));
                        song->changePart(part, nPart);
                        }
                  else if ((t >= lpos) && (t < rpos) && (t+l) > rpos) {
                        // TODO: remove part head
                        }
                  else if (t >= rpos) {
                        Part* nPart = new Part(*part);
                        int nt = part->tick();
                        nPart->setTick(nt - (rpos -lpos));
                        song->changePart(part, nPart);
                        }
                  }
            }
      // TODO: cut tempo track
      // TODO: process marker
      song->endUndo(SC_TRACK_MODIFIED | SC_PART_MODIFIED | SC_PART_REMOVED);
      }

//---------------------------------------------------------
//   globalInsert
//    - insert empty space at left locator position upto
//      right locator
//    - do not touch muted track
//    - insert in master track
//---------------------------------------------------------

void MusE::globalInsert()
      {
      unsigned lpos = song->lpos();
      unsigned rpos = song->rpos();
      if (lpos >= rpos)
            return;

      song->startUndo();
      MidiTrackList* tracks = song->midis();
      for (iMidiTrack it = tracks->begin(); it != tracks->end(); ++it) {
            MidiTrack* track = *it;
            //
            // process only non muted midi tracks
            //
            if (track->mute())
                  continue;
            PartList* pl = track->parts();
            for (iPart p = pl->begin(); p != pl->end(); ++p) {
                  Part* part = p->second;
                  unsigned t = part->tick();
                  int l = part->lenTick();
                  if (t + l <= lpos)
                        continue;
                  if (lpos >= t && lpos < (t+l)) {
                        Part* nPart = new Part(*part);
                        nPart->setLenTick(l + (rpos-lpos));
                        EventList* el = nPart->events();

                        iEvent i = el->end();
                        while (i != el->begin()) {
                              --i;
                              if (i->first < lpos)
                                    break;
                              Event event  = i->second;
                              Event nEvent = i->second.clone();
                              nEvent.setTick(nEvent.tick() + (rpos-lpos));
                              audio->msgChangeEvent(event, nEvent, nPart, false);
                              }
                        song->changePart(part, nPart);
                        }
                  else if (t > lpos) {
                        Part* nPart = new Part(*part);
                        nPart->setTick(t + (rpos -lpos));
                        song->changePart(part, nPart);
                        }
                  }
            }
      // TODO: process tempo track
      // TODO: process marker
      song->endUndo(SC_TRACK_MODIFIED | SC_PART_MODIFIED | SC_PART_REMOVED);
      }

//---------------------------------------------------------
//   globalSplit
//    - split all parts at the song position pointer
//    - do not touch muted track
//---------------------------------------------------------

void MusE::globalSplit()
      {
      int pos = song->cpos();
      song->startUndo();
      TrackList* tracks = song->tracks();
      for (iTrack it = tracks->begin(); it != tracks->end(); ++it) {
            Track* track = *it;
            PartList* pl = track->parts();
            for (iPart p = pl->begin(); p != pl->end(); ++p) {
                  Part* part = p->second;
                  int p1 = part->tick();
                  int l0 = part->lenTick();
                  if (pos > p1 && pos < (p1+l0)) {
                        Part* p1;
                        Part* p2;
                        track->splitPart(part, pos, p1, p2);
                        song->changePart(part, p1);
                        song->addPart(p2);
                        break;
                        }
                  }
            }
      song->endUndo(SC_TRACK_MODIFIED | SC_PART_MODIFIED | SC_PART_INSERTED);
      }

//---------------------------------------------------------
//   copyRange
//    - copy space between left and right locator position
//      to song position pointer
//    - dont process muted tracks
//    - create a new part for every track containing the
//      copied events
//---------------------------------------------------------

void MusE::copyRange()
      {
      QMessageBox::critical(this,
         tr("MusE: Copy Range"),
         tr("not implemented")
         );
      }

//---------------------------------------------------------
//   cutEvents
//    - make sure that all events in a part end where the
//      part ends
//    - process only marked parts
//---------------------------------------------------------

void MusE::cutEvents()
      {
      QMessageBox::critical(this,
         tr("MusE: Cut Events"),
         tr("not implemented")
         );
      }

//---------------------------------------------------------
//   checkRegionNotNull
//    return true if (rPos - lPos) <= 0
//---------------------------------------------------------

bool MusE::checkRegionNotNull()
      {
      int start = song->lPos().frame();
      int end   = song->rPos().frame();
      if (end - start <= 0) {
            QMessageBox::critical(this,
               tr("MusE: Bounce"),
               tr("set left/right marker for bounce range")
               );
            return true;
            }
      return false;
      }

//---------------------------------------------------------
//   resetAllRecordFlags
//---------------------------------------------------------

static void resetAllRecordFlags()
      {
      WaveTrackList* wtl = song->waves();
      for (iWaveTrack i = wtl->begin(); i != wtl->end(); ++i) {
            if((*i)->recordFlag())
                  song->setRecordFlag(*i, false);
            }
      MidiTrackList* mtl = song->midis();
      for (iMidiTrack i = mtl->begin(); i != mtl->end(); ++i) {
            if((*i)->recordFlag())
                  song->setRecordFlag(*i, false);
            }
      }

//---------------------------------------------------------
//   bounceToTrack
//---------------------------------------------------------

void MusE::bounceToTrack()
      {
      if (checkRegionNotNull())
            return;
      // search target track
      TrackList* tl = song->tracks();
      WaveTrack* track = 0;
      for (iTrack it = tl->begin(); it != tl->end(); ++it) {
            Track* t = *it;
            if (t->selected()) {
                  if (track) {
                        QMessageBox::critical(this,
                           tr("MusE: Bounce to Track"),
                           tr("more than one target track selected")
                           );
                        return;
                        }
                  if (t->type() != Track::WAVE) {
                        QMessageBox::critical(this,
                           tr("MusE: Bounce to Track"),
                           tr("wrong target track type,\nselect wave track as target")
                           );
                        return;
                        }
                  track = (WaveTrack*)t;
                  }
            }
      if (track == 0) {
            QMessageBox::critical(this,
               tr("MusE: Bounce to Track"),
               tr("no target track selected")
               );
            return;
            }
      song->bounceTrack = track;
      song->setRecord(true);
      resetAllRecordFlags();
      song->setRecordFlag(track, true);
      audio->msgBounce();
      }

//---------------------------------------------------------
//   bounceToFile
//---------------------------------------------------------

void MusE::bounceToFile()
      {
      if (checkRegionNotNull())
            return;
      SndFile* sf = getSndFile(0, this);
      if (sf == 0)
            return;
      OutputList* ol = song->outputs();
      AudioOutput* ao = ol->front();
      if (ao == 0) {
            QMessageBox::critical(this,
               tr("MusE: Bounce to File"),
               tr("no output track found")
               );
            return;
            }
      ao->setRecFile(sf);
      song->setRecord(true);
      resetAllRecordFlags();
      song->setRecordFlag(ao, true);
      audio->msgBounce();
      }

//---------------------------------------------------------
//   startEditInstrument
//---------------------------------------------------------

void MusE::startEditInstrument()
      {
      if (editInstrument == 0)
            editInstrument = new EditInstrument(this);
      editInstrument->show();
      }

//---------------------------------------------------------
//   updateConfiguration
//    called whenever the configuration has changed
//---------------------------------------------------------

void MusE::updateConfiguration()
      {
#if 0 //TODOB
      fileOpenAction->setShortcut(shortcuts[SHRT_OPEN].key);
      fileSaveAction->setShortcut(shortcuts[SHRT_SAVE].key);

      menuEditActions[CMD_DELETE]->setShortcut(shortcuts[SHRT_DELETE].key);
      menu_ids[CMD_OPEN_RECENT]->setShortcut(shortcuts[SHRT_OPEN_RECENT].key);
      menu_ids[CMD_IMPORT_MIDI]->setShortcut(shortcuts[SHRT_IMPORT_MIDI].key);
      menu_ids[CMD_EXPORT_MIDI]->setShortcut(shortcuts[SHRT_EXPORT_MIDI].key);
      menu_ids[CMD_IMPORT_AUDIO]->setShortcut(shortcuts[SHRT_IMPORT_AUDIO].key);
      menu_ids[CMD_QUIT]->setShortcut(shortcuts[SHRT_QUIT].key);
      menu_ids[CMD_OPEN_DRUMS]->setShortcut(shortcuts[SHRT_OPEN_DRUMS].key);
      menu_ids[CMD_OPEN_LIST]->setShortcut(shortcuts[SHRT_OPEN_LIST].key);
      menu_ids[CMD_OPEN_GRAPHIC_MASTER]->setShortcut(shortcuts[SHRT_OPEN_GRAPHIC_MASTER].key);
      menu_ids[CMD_TRANSPOSE]->setShortcut(shortcuts[SHRT_TRANSPOSE].key);

      menuEditActions[CMD_SELECT_ALL]->setShortcut(shortcuts[SHRT_SELECT_ALL].key);
      menuEditActions[CMD_SELECT_NONE]->setShortcut(shortcuts[SHRT_SELECT_NONE].key);
      menuEditActions[CMD_SELECT_INVERT]->setShortcut(shortcuts[SHRT_SELECT_INVERT].key);
      menuEditActions[CMD_SELECT_ILOOP]->setShortcut(shortcuts[SHRT_SELECT_ILOOP].key);
      menuEditActions[CMD_SELECT_OLOOP]->setShortcut(shortcuts[SHRT_SELECT_OLOOP].key);
      menuEditActions[CMD_SELECT_PARTS]->setShortcut(shortcuts[SHRT_SELECT_PRTSTRACK].key);

      tr_id->setShortcut(shortcuts[SHRT_OPEN_TRANSPORT].key);
      tr_id->setShortcutContext(Qt::ApplicationShortcut);
      bt_id->setShortcut(shortcuts[SHRT_OPEN_BIGTIME].key);
      bt_id->setShortcutContext(Qt::ApplicationShortcut);
      mk_id->setShortcut(shortcuts[SHRT_OPEN_MARKER].key);
      mk_id->setShortcutContext(Qt::ApplicationShortcut);

      pianoAction->setShortcut(shortcuts[SHRT_OPEN_PIANO].key); //pianoroll
      trackerAction->setShortcut(shortcuts[SHRT_OPEN_TRACKER].key); //midiTracker

      menu_ids[CMD_GLOBAL_CUT]->setShortcut(shortcuts[SHRT_GLOBAL_CUT].key);
      menu_ids[CMD_GLOBAL_INSERT]->setShortcut(shortcuts[SHRT_GLOBAL_INSERT].key);
      menu_ids[CMD_GLOBAL_SPLIT]->setShortcut(shortcuts[SHRT_GLOBAL_SPLIT].key);
      menu_ids[CMD_COPY_RANGE]->setShortcut(shortcuts[SHRT_COPY_RANGE].key);
      menu_ids[CMD_CUT_EVENTS]->setShortcut(shortcuts[SHRT_CUT_EVENTS].key);

      menu_ids[CMD_MIDI_EDIT_INSTRUMENTS]->setShortcut(shortcuts[SHRT_MIDI_EDIT_INSTRUMENTS].key);
      //aid1a
      //aid1b // mixer 1
      // aid2;
      // aid3;
      //mpid0 // midi plugin transpose
      //mpid3 // midi remote control

      menu_ids[CMD_MIDI_RESET]->setShortcut(shortcuts[SHRT_MIDI_RESET].key);
      menu_ids[CMD_MIDI_INIT]->setShortcut(shortcuts[SHRT_MIDI_INIT].key);
      menu_ids[CMD_MIDI_LOCAL_OFF]->setShortcut(shortcuts[SHRT_MIDI_LOCAL_OFF].key);

      menu_ids[CMD_AUDIO_BOUNCE_TO_TRACK]->setShortcut(shortcuts[CMD_AUDIO_BOUNCE_TO_TRACK].key);
      menu_ids[CMD_AUDIO_BOUNCE_TO_FILE]->setShortcut(shortcuts[CMD_AUDIO_BOUNCE_TO_FILE].key);

//      menu_ids[CMD_GLOBAL_CONFIG]->setShortcut(shortcuts[SHRT_GLOBAL_CONFIG].key);
      menu_ids[CMD_CONFIG_SHORTCUTS]->setShortcut(shortcuts[SHRT_CONFIG_SHORTCUTS].key);

      // Follow options
      fid0->setShortcut(shortcuts[SHRT_FOLLOW_NO].key);
      fid1->setShortcut(shortcuts[SHRT_FOLLOW_JUMP].key);
      fid2->setShortcut(shortcuts[SHRT_FOLLOW_CONTINUOUS].key);

      menu_ids[CMD_CONFIG_MIDISYNC]->setShortcut(shortcuts[SHRT_CONFIG_MIDISYNC].key);
      menu_ids[CMD_MIDI_FILE_CONFIG]->setShortcut(shortcuts[SHRT_MIDI_FILE_CONFIG].key);
//      menu_ids[CMD_APPEARANCE_SETTINGS]->setShortcut(shortcuts[SHRT_APPEARANCE_SETTINGS].key);

      menu_ids[CMD_OPEN_HELP]->setShortcut(shortcuts[SHRT_OPEN_HELP].key);
//      menu_ids[CMD_OPEN_HOMEPAGE]->setShortcut(shortcuts[SHRT_OPEN_HOMEPAGE].key);
//      menu_ids[CMD_OPEN_BUG]->setShortcut(shortcuts[SHRT_OPEN_BUG].key);
      menu_ids[CMD_START_WHATSTHIS]->setShortcut(shortcuts[SHRT_START_WHATSTHIS].key);
#endif

#if 0 //TD
      menuEdit->setShortcut(shortcuts[SHRT_OPEN_MIDI_TRANSFORM].key, menu_ids[CMD_OPEN_MIDI_TRANSFORM]);

      master->setShortcut(shortcuts[SHRT_OPEN_LIST_MASTER].key, menu_ids[CMD_OPEN_LIST_MASTER]);

      menuView->setShortcut(shortcuts[SHRT_OPEN_MIXER].key, aid1a);

      menuSettings->setShortcut(shortcuts[SHRT_CONFIG_METRONOME].key, menu_ids[CMD_CONFIG_METRONOME]);
      menuSettings->setShortcut(shortcuts[SHRT_CONFIG_AUDIO_PORTS].key, menu_ids[CMD_CONFIG_AUDIO_PORTS]);

      menu_audio->setShortcut(shortcuts[SHRT_AUDIO_RESTART].key, menu_ids[CMD_AUDIO_RESTART]);

//      menuAutomation->setShortcut(shortcuts[SHRT_MIXER_SNAPSHOT].key, menu_ids[CMD_MIXER_SNAPSHOT]);
//      menuAutomation->setShortcut(shortcuts[SHRT_MIXER_AUTOMATION_CLEAR].key, menu_ids[CMD_MIXER_AUTOMATION_CLEAR]);



//      select->setShortcut(shortcuts[SHRT_DESEL_PARTS].key, CMD_SELECT_NONE);

      midiInputPlugins->setShortcut(shortcuts[SHRT_MIDI_INPUT_TRANSPOSE].key, 0);
      midiInputPlugins->setShortcut(shortcuts[SHRT_MIDI_INPUT_TRANSFORM].key, 1);
      midiInputPlugins->setShortcut(shortcuts[SHRT_MIDI_INPUT_FILTER].key, 2);
      midiInputPlugins->setShortcut(shortcuts[SHRT_MIDI_REMOTE_CONTROL].key, 3);
      midiInputPlugins->setShortcut(shortcuts[SHRT_RANDOM_RHYTHM_GENERATOR].key, 4);

      addTrack->setShortcut(shortcuts[SHRT_ADD_MIDI_TRACK].key, Track::MIDI);
      addTrack->setShortcut(shortcuts[SHRT_ADD_WAVE_TRACK].key, Track::WAVE);
      addTrack->setShortcut(shortcuts[SHRT_ADD_AUDIO_OUTPUT].key, Track::AUDIO_OUTPUT);
      addTrack->setShortcut(shortcuts[SHRT_ADD_AUDIO_GROUP].key, Track::AUDIO_GROUP);
      addTrack->setShortcut(shortcuts[SHRT_ADD_AUDIO_INPUT].key, Track::AUDIO_INPUT);
#endif
      }

//---------------------------------------------------------
//   showBigtime
//---------------------------------------------------------

void MusE::showBigtime(bool on)
      {
      if (on && bigtime == 0) {
            bigtime = new BigTime(0);
            bigtime->setPos(0, song->cpos(), false);
            connect(song, SIGNAL(posChanged(int,const AL::Pos&, bool)), bigtime, SLOT(setPos(int,const AL::Pos&, bool)));
            connect(muse, SIGNAL(configChanged()), bigtime, SLOT(configChanged()));
            connect(bigtime, SIGNAL(closed()), SLOT(bigtimeClosed()));
            bigtime->resize(config.geometryBigTime.size());
            bigtime->move(config.geometryBigTime.topLeft());
            }
      if (bigtime)
            bigtime->setShown(on);
      bt_id->setChecked(on);
      }

//---------------------------------------------------------
//   showMarker
//---------------------------------------------------------

void MusE::showMarker(bool on)
      {
      if (on && markerView == 0) {
            markerView = new MarkerView;
            connect(markerView, SIGNAL(closed()), SLOT(markerClosed()));
            }
      if (markerView)
            markerView->setShown(on);
      mk_id->setChecked(on);
      }

//---------------------------------------------------------
//   markerClosed
//---------------------------------------------------------

void MusE::markerClosed()
      {
      mk_id->setChecked(false);
      markerView = 0;
      }

//---------------------------------------------------------
//   bigtimeClosed
//---------------------------------------------------------

void MusE::bigtimeClosed()
      {
      bt_id->setChecked(false);
      }

//---------------------------------------------------------
//   transportClosed
//---------------------------------------------------------

void MusE::transportClosed()
      {
      tr_id->setChecked(false);
      }

//---------------------------------------------------------
//   showMixer1
//---------------------------------------------------------

void MusE::showMixer1(bool on)
      {
      if (on && mixer1 == 0) {
            mixer1 = new Mixer(this, &(config.mixer1));
            connect(mixer1, SIGNAL(closed()), SLOT(mixer1Closed()));
            mixer1->resize(config.mixer1.geometry.size());
            mixer1->move(config.mixer1.geometry.topLeft());
            }
      if (mixer1)
            mixer1->setShown(on);
      aid1a->setChecked(on);
      }

//---------------------------------------------------------
//   showMixer2
//---------------------------------------------------------

void MusE::showMixer2(bool on)
      {
      if (on && mixer2 == 0) {
            mixer2 = new Mixer(this, &(config.mixer2));
            connect(mixer2, SIGNAL(closed()), SLOT(mixer2Closed()));
            mixer2->resize(config.mixer2.geometry.size());
            mixer2->move(config.mixer2.geometry.topLeft());
            }
      if (mixer2)
            mixer2->setShown(on);
      aid1b->setChecked(on);
      }

//---------------------------------------------------------
//   mixer1Closed
//---------------------------------------------------------

void MusE::mixer1Closed()
      {
      aid1a->setChecked(false);
      }

//---------------------------------------------------------
//   mixer2Closed
//---------------------------------------------------------

void MusE::mixer2Closed()
      {
      aid1b->setChecked(false);
      }

//---------------------------------------------------------
//   transportWindow
//---------------------------------------------------------

QWidget* MusE::transportWindow()
      {
      return transport;
      }

//---------------------------------------------------------
//   bigtimeWindow
//---------------------------------------------------------

QWidget* MusE::bigtimeWindow()
      {
      return bigtime;
      }

//---------------------------------------------------------
//   mixer1Window
//---------------------------------------------------------

QWidget* MusE::mixer1Window()
      {
	return mixer1;
      }

//---------------------------------------------------------
//   mixer2Window
//---------------------------------------------------------

QWidget* MusE::mixer2Window()
      {
	return mixer2;
      }

//---------------------------------------------------------
//   focusInEvent
//---------------------------------------------------------

void MusE::focusInEvent(QFocusEvent* ev)
      {
      if (mixer1)
            mixer1->raise();
      if (mixer2)
            mixer2->raise();
      raise();
      QMainWindow::focusInEvent(ev);
      }

//---------------------------------------------------------
//   setTool
//---------------------------------------------------------

void MusE::setTool(int tool)
      {
      tools1->set(tool);
      arranger->setTool(tool);
      }

//---------------------------------------------------------
//   globalPitchChanged
//---------------------------------------------------------

void MusE::globalPitchChanged(int val)
      {
      song->setGlobalPitchShift(val);
      }

//---------------------------------------------------------
//   globalTempoChanged
//---------------------------------------------------------

void MusE::globalTempoChanged(int val)
      {
      audio->msgSetGlobalTempo(val);
      song->update(SC_TEMPO);
      }

//---------------------------------------------------------
//   setTempo50
//---------------------------------------------------------

void MusE::setTempo50()
      {
      setGlobalTempo(50);
      }

//---------------------------------------------------------
//   setTempo100
//---------------------------------------------------------

void MusE::setTempo100()
      {
      setGlobalTempo(100);
      }

//---------------------------------------------------------
//   setTempo200
//---------------------------------------------------------

void MusE::setTempo200()
      {
      setGlobalTempo(200);
      }

//---------------------------------------------------------
//   setGlobalTempo
//---------------------------------------------------------

void MusE::setGlobalTempo(int val)
      {
      globalTempoSpinBox->setValue(val);
      }

//---------------------------------------------------------
//   main
//---------------------------------------------------------

int main(int argc, char* argv[])
      {
      museUser = QString(getenv("MUSEHOME"));
      if (museUser.isEmpty())
            museUser = QDir::homePath();
      QString museGlobal;
      const char* p = getenv("MUSE");
      museGlobal = p ? p : INSTPREFIX;

      museGlobalLib   =  museGlobal + "/lib/"   INSTALL_NAME;
      museGlobalShare =  museGlobal + "/share/" INSTALL_NAME;
      configName      = museUser + QString("/." INSTALL_NAME);
      lastMidiPath    = museUser + "/" + ::config.importMidiPath;
      lastWavePath    = museUser + "/" + ::config.importWavePath;

      srand(time(0));   // initialize random number generator
      initMidiController();
      initMidiInstruments();
      MuseApplication app(argc, argv);

      config.fonts[0] = QFont(QString("helvetica"), 10, QFont::Normal);
      config.fonts[1] = QFont(QString("helvetica"),  6, QFont::Normal);
      config.fonts[2] = QFont(QString("helvetica"), 10, QFont::Normal);
      config.fonts[3] = QFont(QString("helvetica"),  8, QFont::Bold);
      config.fonts[4] = QFont(QString("helvetica"),  8,  QFont::Bold);    // simple buttons, timescale numbers
      config.fonts[5] = QFont(QString("Courier"), 14,  QFont::Bold);

      gmDrumMap.initGm();    // init default drum map
      readConfiguration();

      QApplication::setFont(config.fonts[0]);

      // this style is used for scrollbars in mixer plugin racks:
      smallStyle = new QWindowsStyle();

      // SHOW MUSE SPLASH SCREEN
      if (config.showSplashScreen) {
            QPixmap splsh(":/xpm/splash.png");

            if (!splsh.isNull()) {
                  QSplashScreen* muse_splash = new QSplashScreen(splsh,
                     Qt::WindowStaysOnTopHint);
                  muse_splash->show();
                  QTimer* stimer = new QTimer(0);
                  muse_splash->connect(stimer, SIGNAL(timeout()), muse_splash, SLOT(close()));
                  stimer->start(6000);
                  }
            }

      char c;
      QString opts("mvdDiosP:p");

#ifdef VST_SUPPORT
      opts += "V";
#endif
#ifdef DSSI_SUPPORT
      opts += "I";
#endif
      while ((c = getopt(argc, argv, opts.toLatin1().data())) != EOF) {
            switch (c) {
                  case 'v': printVersion(argv[0]); return 0;
                  case 'd':
                        debugMode = true;
                        realTimePriority = false;
                        break;
                  case 'm': midiOnly = true; break;
                  case 'D': debugMsg = true; break;
                  case 'i': midiInputTrace = true; break;
                  case 'o': midiOutputTrace = true; break;
                  case 's': debugSync = true; break;
                  case 'p': loadPlugins = false; break;
                  case 'V': loadVST = false; break;
                  case 'I': loadDSSI = false; break;
                  default:  usage(argv[0], "bad argument"); return -1;
                  }
            }
      if (midiOnly) {
            loadDSSI    = false;
            loadPlugins = false;
            loadVST     = false;
            }

      bool useJACK = !(debugMode || midiOnly);
      if (useJACK) {
            if (initJackAudio()) {
                  if (!debugMode)
                        {
                        QMessageBox::critical(NULL, "MusE fatal error",
                           "MusE failed to find a Jack audio server.\n"
                           "Check that Jack was started.\n"
                           "If Jack was started check that it was\n"
                           "started as the same user as MusE.");
                        // fatalError("cannot start JACK");
                        }
                  else
                        {
                        fprintf(stderr, "fatal error: no JACK audio server found\n");
                        fprintf(stderr, "no audio functions available\n");
                        fprintf(stderr, "*** experimental mode -- no play possible ***\n");
                        }
                  useJACK = false;
                  debugMode = true;
                  }
            }
      if (!useJACK)
            initDummyAudio();

      argc -= optind;
      ++argc;

      if (debugMsg) {
            printf("global lib:   <%s>\n", museGlobalLib.toLatin1().data());
            printf("global share: <%s>\n", museGlobalShare.toLatin1().data());
            printf("muse home:    <%s>\n", museUser.toLatin1().data());
            printf("project dir:  <%s>\n", config.projectPath.toLatin1().data());
            printf("config file:  <%s>\n", configName.toLatin1().data());
            }

      static QTranslator translator;
      QFile f(":/muse.qm");
      if (f.exists()) {
            if (debugMsg)
                  printf("locale file found\n");
            if (translator.load(":/muse.qm")) {
                  if (debugMsg)
                        printf("locale file loaded\n");
                  }
            qApp->installTranslator(&translator);
            }
      else {
            if (debugMsg) {
                  printf("locale file not found for locale <%s>\n",
                     QLocale::system().name().toLatin1().data());
                  }
            }

      if (loadPlugins) {
            initPlugins();
            initMidiPlugins();
            }
      if (loadVST)
            initVST();
      if (loadDSSI)
            initDSSI();

      initIcons();
      if (!midiOnly)
            initMetronome();

      if (debugMsg) {
            QStringList list = app.libraryPaths();
            QStringList::Iterator it = list.begin();
            printf("QtLibraryPath:\n");
            while(it != list.end()) {
                  printf("  <%s>\n", (*it).toLatin1().data());
                  ++it;
                  }
            }

      song = new Song();
      muse = new MusE();
      app.setMuse(muse);

      //---------------------------------------------------
      //  load project
      //---------------------------------------------------

      // check for project directory:

      QDir pd(QDir::homePath() + "/" + config.projectPath);
      if (!pd.exists()) {
            // ask user to create a new project directory
            QString title(QT_TR_NOOP("MusE: create project directory"));

            QString s;
            s = "The MusE project directory\n%1\ndoes not exists";
            s = s.arg(pd.path());

            int rv = QMessageBox::question(0, 
               title,
               s,
               "Create",
               "Abort",
               QString(),
               0, 1);
            if (rv == 1)
                  exit(0);
            if (!pd.mkpath(pd.path())) {
                  // TODO: tell user why this has happened
                  QMessageBox::critical(0,
                  title, 
                  "Creating project directory failed");
                  exit(-1);
                  }
            }

      // check for template directory:

      pd.setPath(QDir::homePath() + "/" + config.templatePath);
      if (!pd.exists()) {
            // ask user to create a new template directory
            QString title(QT_TR_NOOP("MusE: create template directory"));

            QString s;
            s = "The MusE template directory\n%1\ndoes not exists";
            s = s.arg(pd.path());

            int rv = QMessageBox::question(0, 
               title,
               s,
               "Create",
               "Abort",
               QString(),
               0, 1);
            if (rv == 0) {
                  if (!pd.mkpath(pd.path())) {
                        // TODO: tell user why this has happened
                        QMessageBox::critical(0,
                        title,
                        "Creating template directory failed");
                        }
                  }
            }

      QString path;     // project path relativ to config.projectPath
      if (argc >= 2)
            path = argv[optind];    // start with first name on command line
      else if (config.startMode == START_LAST_PROJECT) {
            if (projectList[0])
                  path = *projectList[0];
            }
      else if (config.startMode == START_START_PROJECT)
            path = config.startProject;

      QString name = path.split("/").last();
      if (!path.isEmpty()) {
            QFile f(QDir::homePath() +"/"+config.projectPath+"/"+path+"/"+name+".med");
            if (!f.exists()) {
                  QString s(QT_TR_NOOP("Cannot find project <%1>"));
                  QString header(QT_TR_NOOP("MusE: load Project"));
                  QMessageBox::critical(0, header, s.arg(f.fileName()));
                  path = "";
                  }
            }
      if (path.isEmpty()) {
            //
            // ask user for a project
            //
            for (;;) {
                  ProjectDialog projectDialog;
                  projectDialog.setProjectName(name);
                  int rv = projectDialog.exec();
                  if (rv == 1) {
                        path = projectDialog.projectPath();
                        if (!path.isEmpty())
                              break;
                        }
                  // the user did not select/create a project
                  rv = QMessageBox::question(0, 
                     "MusE: create/select project",
                     "before MusE starts, you must select a project\n"
                      "or create a new one",
                     "Go Back",
                     "Abort",
                     QString(),
                     0, 1);
                  if (rv == 1)
                        exit(0);
                  }
            }

      muse->loadProject(path);
      muse->changeConfig(false);
      if (!debugMode) {
            if (mlockall(MCL_CURRENT | MCL_FUTURE))
                  perror("WARNING: Cannot lock memory:");
            }
      muse->show();
      muse->seqStart();
      int n = app.exec();
      if (n)
            fprintf(stderr, "app end %d\n", n);
      return n;
      }
      
