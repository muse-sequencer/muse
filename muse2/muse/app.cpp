//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: app.cpp,v 1.113.2.68 2009/12/21 14:51:51 spamatica Exp $
//
//  (C) Copyright 1999-2004 Werner Schweer (ws@seh.de)
//=========================================================

#include "config.h"

#include <string>
#include <map>
#include <assert.h>
#include <getopt.h>
#include <errno.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <signal.h>
#include <stdarg.h>

#include <q3buttongroup.h>
#include <q3popupmenu.h>
#include <qmessagebox.h>
#include <qclipboard.h>
#include <qsocketnotifier.h>
#include <qtextcodec.h>
#include <qstylefactory.h>
#include <qmenubar.h>
#include <qapplication.h>
#include <qtimer.h>
#include <qstyle.h>
#include <qsplashscreen.h>
#include <qobject.h>
//Added by qt3to4:
#include <QTimerEvent>
#include <Q3CString>
#include <QFocusEvent>
#include <QTranslator>
#include <QKeyEvent>
#include <QEvent>
#include <Q3ActionGroup>
#include <QPixmap>
#include <QCloseEvent>

#include "app.h"
#include "popupmenu.h"
#include "transport.h"
#include "bigtime.h"
#include "arranger.h"
#include "pianoroll.h"
#include "xml.h"
#include "midi.h"
#include "conf.h"
#include "listedit.h"
#include "master/masteredit.h"
#include "master/lmaster.h"
#include "drumedit.h"
#include "ttoolbar.h"
#include "amixer.h"
#include "cliplist/cliplist.h"
#include "midiport.h"
#include "audiodev.h"
#include "mididev.h"
#include "waveedit.h"
#include "icons.h"
#include "minstrument.h"
#include "mixdowndialog.h"
#include "midictrl.h"
#include "filedialog.h"
#include "plugin.h"
#include "marker/markerview.h"
#include "transpose.h"
#include "appearance.h"
#include "gatetime.h"
#include "metronome.h"
#include "debug.h"
#include "event.h"
#include "audio.h"
#include "midiseq.h"
#include "audioprefetch.h"
#include "wave.h"
#include "shortcutconfig.h"
#include "gconfig.h"
#include "driver/jackaudio.h"
#include "track.h"
#include "ticksynth.h"
#include "instruments/editinstrument.h"
#include "synth.h"
#include "remote/pyapi.h"
#include "al/dsp.h"

#ifdef DSSI_SUPPORT
#include "dssihost.h"
#endif

#ifdef VST_SUPPORT
#include "vst.h"
#endif

#include <alsa/asoundlib.h>
#include "songinfo.h"
#include "didyouknow.h"
#include <q3textedit.h>

//extern void cacheJackRouteNames();

static pthread_t watchdogThread;
//ErrorHandler *error;
static const char* fileOpenText =
      QT_TR_NOOP("Click this button to open a <em>new song</em>.<br>"
      "You can also select the <b>Open command</b> from the File menu.");
static const char* fileSaveText =
      QT_TR_NOOP("Click this button to save the song you are "
      "editing.  You will be prompted for a file name.\n"
      "You can also select the Save command from the File menu.");
static const char* fileNewText        = QT_TR_NOOP("Create New Song");

static const char* infoLoopButton     = QT_TR_NOOP("loop between left mark and right mark");
static const char* infoPunchinButton  = QT_TR_NOOP("record starts at left mark");
static const char* infoPunchoutButton = QT_TR_NOOP("record stops at right mark");
static const char* infoStartButton    = QT_TR_NOOP("rewind to start position");
static const char* infoRewindButton   = QT_TR_NOOP("rewind current position");
static const char* infoForwardButton  = QT_TR_NOOP("move current position");
static const char* infoStopButton     = QT_TR_NOOP("stop sequencer");
static const char* infoPlayButton     = QT_TR_NOOP("start sequencer play");
static const char* infoRecordButton   = QT_TR_NOOP("to record press record and then play");
static const char* infoPanicButton    = QT_TR_NOOP("send note off to all midi channels");

#define PROJECT_LIST_LEN  6
static QString* projectList[PROJECT_LIST_LEN];

extern void initIcons();
extern void initMidiSynth();
extern bool initJackAudio();
extern void exitJackAudio();
extern bool initDummyAudio();
extern void exitDummyAudio();
extern void initVST_fst_init();
extern void initVST();
extern void initDSSI();
// p3.3.39
extern void initOSC();
extern void exitOSC();

#ifdef HAVE_LASH
#include <lash/lash.h>
lash_client_t * lash_client = 0;
extern snd_seq_t * alsaSeq;
#endif /* HAVE_LASH */

int watchAudio, watchAudioPrefetch, watchMidi;
pthread_t splashThread;


//PyScript *pyscript;
// void MusE::runPythonScript()
// {
//  QString script("test.py");
// // pyscript->runPythonScript(script);
// }

//---------------------------------------------------------
//   getCapabilities
//---------------------------------------------------------

static void getCapabilities()
      {
#ifdef RTCAP
#ifdef __linux__
      const char* napp = getenv("GIVERTCAP");
      if (napp == 0)
            napp = "givertcap";
      int pid = fork();
      if (pid == 0) {
            if (execlp(napp, napp, 0) == -1)
                  perror("exec givertcap failed");
            }
      else if (pid == -1) {
            perror("fork givertcap failed");
            }
      else {
            waitpid(pid, 0, 0);
            }
#endif // __linux__
#endif
      }


//---------------------------------------------------------
//   sleep function
//---------------------------------------------------------
void microSleep(long msleep)
{
    bool sleepOk=-1;

    while(sleepOk==-1)
        sleepOk=usleep(msleep);
}

// Removed p3.3.17
/* 
//---------------------------------------------------------
//   watchdog thread
//---------------------------------------------------------

static void* watchdog(void*)
      {
      doSetuid();

      struct sched_param rt_param;
      memset(&rt_param, 0, sizeof(rt_param));
      rt_param.sched_priority = sched_get_priority_max(SCHED_FIFO);
      int rv = pthread_setschedparam(pthread_self(), SCHED_FIFO, &rt_param);
      if (rv != 0)
            perror("Set realtime scheduler");

      int policy;
      if (pthread_getschedparam(pthread_self(), &policy, &rt_param)!= 0) {
            printf("Cannot get current client scheduler: %s\n", strerror(errno));
            }
      if (policy != SCHED_FIFO)
            printf("watchdog process %d _NOT_ running SCHED_FIFO\n", getpid());
      else if (debugMsg)
            printf("watchdog set to SCHED_FIFO priority %d\n",
               sched_get_priority_max(SCHED_FIFO));

      undoSetuid();
      int fatal = 0;
      for (;;) {
            watchAudio = 0;
            watchMidi = 0;
            static const int WD_TIMEOUT = 3;

            // sleep can be interrpted by signals:
            int to = WD_TIMEOUT;
            while (to > 0)
                  to = sleep(to);

            bool timeout = false;
            if (midiSeqRunning && watchMidi == 0)
            {
                  printf("midiSeqRunning = %i watchMidi %i\n", midiSeqRunning, watchMidi);
                  timeout = true;
            }
            if (watchAudio == 0)
                  timeout = true;
            if (watchAudio > 500000)
                  timeout = true;
            if (timeout)
                  ++fatal;
            else
                  fatal = 0;
            if (fatal >= 3) {
                  printf("WatchDog: fatal error, realtime task timeout\n");
                  printf("   (%d,%d-%d) - stopping all services\n",
                     watchMidi, watchAudio, fatal);
                  break;
                  }
//            printf("wd %d %d %d\n", watchMidi, watchAudio, fatal);
            }
      audio->stop(true);
      audioPrefetch->stop(true);
      printf("watchdog exit\n");
      exit(-1);
      }
*/

//---------------------------------------------------------
//   seqStart
//---------------------------------------------------------

bool MusE::seqStart()
      {
      // Changed by Tim. p3.3.17
      
      /*
      if (audio->isRunning()) {
            printf("seqStart(): already running\n");
            return true;
            }
      
      if (realTimeScheduling) {
            //
            //  create watchdog thread with max priority
            //
            doSetuid();
            struct sched_param rt_param;
            memset(&rt_param, 0, sizeof(rt_param));
            rt_param.sched_priority = realTimePriority +1;//sched_get_priority_max(SCHED_FIFO);

            pthread_attr_t* attributes = (pthread_attr_t*) malloc(sizeof(pthread_attr_t));
            pthread_attr_init(attributes);

//             if (pthread_attr_setschedpolicy(attributes, SCHED_FIFO)) {
//                   printf("MusE: cannot set FIFO scheduling class for RT thread\n");
//                   }
//             if (pthread_attr_setschedparam (attributes, &rt_param)) {
//                   // printf("Cannot set scheduling priority for RT thread (%s)\n", strerror(errno));
//                   }
//             if (pthread_attr_setscope (attributes, PTHREAD_SCOPE_SYSTEM)) {
//                   printf("MusE: Cannot set scheduling scope for RT thread\n");
//                   }
            if (pthread_create(&watchdogThread, attributes, ::watchdog, 0))
                  perror("MusE: creating watchdog thread failed:");
            pthread_attr_destroy(attributes);
            undoSetuid();
            }
      audioPrefetch->start();
      audioPrefetch->msgSeek(0, true); // force
      midiSeqRunning = !midiSeq->start();
      
      if (!audio->start()) {
          QMessageBox::critical( muse, tr(QString("Failed to start audio!")),
              tr(QString("Was not able to start audio, check if jack is running.\n")));
          return false;
          }

      return true;
      */
      
      if (audio->isRunning()) {
            printf("seqStart(): already running\n");
            return true;
            }
      
      if (!audio->start()) {
          QMessageBox::critical( muse, tr(QString("Failed to start audio!")),
              tr(QString("Was not able to start audio, check if jack is running.\n")));
          return false;
          }

      //
      // wait for jack callback
      //
      for(int i = 0; i < 60; ++i) 
      {
        //if (audioState == AUDIO_START2)
        if(audio->isRunning())
          break;
        sleep(1);
      }
      //if (audioState != AUDIO_START2) {
      if(!audio->isRunning()) 
      {
        QMessageBox::critical( muse, tr("Failed to start audio!"),
            tr("Timeout waiting for audio to run. Check if jack is running.\n"));
      }
      //
      // now its safe to ask the driver for realtime
      // priority
      
      realTimePriority = audioDevice->realtimePriority();
      if(debugMsg)
        printf("MusE::seqStart: getting audio driver realTimePriority:%d\n", realTimePriority);
      
      // Disabled by Tim. p3.3.22
      /*
      if(realTimeScheduling) 
      {
            //
            //  create watchdog thread with max priority
            //
            doSetuid();
            struct sched_param rt_param;
            memset(&rt_param, 0, sizeof(rt_param));
            rt_param.sched_priority = realTimePriority + 1;//sched_get_priority_max(SCHED_FIFO);

            pthread_attr_t* attributes = (pthread_attr_t*) malloc(sizeof(pthread_attr_t));
            pthread_attr_init(attributes);

//             if (pthread_attr_setschedpolicy(attributes, SCHED_FIFO)) {
//                   printf("MusE: cannot set FIFO scheduling class for RT thread\n");
//                   }
//             if (pthread_attr_setschedparam (attributes, &rt_param)) {
//                   // printf("Cannot set scheduling priority for RT thread (%s)\n", strerror(errno));
//                   }
//             if (pthread_attr_setscope (attributes, PTHREAD_SCOPE_SYSTEM)) {
//                   printf("MusE: Cannot set scheduling scope for RT thread\n");
//                   }
            if (pthread_create(&watchdogThread, attributes, ::watchdog, 0))
                  perror("MusE: creating watchdog thread failed");
            pthread_attr_destroy(attributes);
            undoSetuid();
      }
      */
      
      //int policy;
      //if ((policy = sched_getscheduler (0)) < 0) {
      //      printf("Cannot get current client scheduler: %s\n", strerror(errno));
      //      }
      //if (policy != SCHED_FIFO)
      //      printf("midi thread %d _NOT_ running SCHED_FIFO\n", getpid());
      
      
      //audioState = AUDIO_RUNNING;
      // Changed by Tim. p3.3.22
      /*
      //if(realTimePriority) 
      if(realTimeScheduling) 
      {
        int pr = realTimePriority;
        if(pr > 5)
          pr -= 5;
        else
          pr = 0;  
        audioPrefetch->start(pr);
        //audioWriteback->start(realTimePriority - 5);
      }
      else 
      {
        audioPrefetch->start(0);
        //audioWriteback->start(0);
      }
      */
      
      int pfprio = 0;
      int midiprio = 0;
      
      // NOTE: realTimeScheduling can be true (gotten using jack_is_realtime()),
      //  while the determined realTimePriority can be 0.
      // realTimePriority is gotten using pthread_getschedparam() on the client thread 
      //  in JackAudioDevice::realtimePriority() which is a bit flawed - it reports there's no RT...
      if(realTimeScheduling) 
      {
        //if(realTimePriority < 5)
        //  printf("MusE: WARNING: Recommend setting audio realtime priority to a higher value!\n");
        /*
        if(realTimePriority == 0)
        {
          pfprio = 1;
          midiprio = 2;
        }  
        else
        if(realTimePriority == 1)
        {
          pfprio = 2;
          midiprio = 3;
        }  
        else
        if(realTimePriority == 2)
        {
          pfprio = 1;
          midiprio = 3;
        }  
        else
        if(realTimePriority == 3)
        {
          pfprio = 1;
          //midiprio = 2;
          // p3.3.37
          midiprio = 4;
        }  
        else
        if(realTimePriority == 4)
        {
          pfprio = 1;
          //midiprio = 3;
          // p3.3.37
          midiprio = 5;
        }  
        else
        if(realTimePriority == 5)
        {
          pfprio = 1;
          //midiprio = 3;
          // p3.3.37
          midiprio = 6;
        }  
        else
        */
        {
          //pfprio = realTimePriority - 5;
          // p3.3.40
          pfprio = realTimePriority + 1;
          
          //midiprio = realTimePriority - 2;
          // p3.3.37
          //midiprio = realTimePriority + 1;
          // p3.3.40
          midiprio = realTimePriority + 2;
        }  
      }
      
      if(midiRTPrioOverride > 0)
        midiprio = midiRTPrioOverride;
      
      // FIXME FIXME: The realTimePriority of the Jack thread seems to always be 5 less than the value passed to jackd command.
      //if(midiprio == realTimePriority)
      //  printf("MusE: WARNING: Midi realtime priority %d is the same as audio realtime priority %d. Try a different setting.\n", 
      //         midiprio, realTimePriority);
      //if(midiprio == pfprio)
      //  printf("MusE: WARNING: Midi realtime priority %d is the same as audio prefetch realtime priority %d. Try a different setting.\n", 
      //         midiprio, pfprio);
      
      audioPrefetch->start(pfprio);
      
      audioPrefetch->msgSeek(0, true); // force
      
      //midiSeqRunning = !midiSeq->start(realTimeScheduling ? realTimePriority : 0);
      // Changed by Tim. p3.3.22
      //midiSeq->start(realTimeScheduling ? realTimePriority : 0);
      midiSeq->start(midiprio);
      
      int counter=0;
      while (++counter) {
        //if (counter > 10) {
        if (counter > 1000) {
            fprintf(stderr,"midi sequencer thread does not start!? Exiting...\n");
            exit(33);
        }
        midiSeqRunning = midiSeq->isRunning();
        if (midiSeqRunning)
          break;
        usleep(1000);
        printf("looping waiting for sequencer thread to start\n");
      }
      if(!midiSeqRunning)
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
      midiSeqRunning = false;

      song->setStop(true);
      song->setStopPlay(false);
      midiSeq->stop(true);
      audio->stop(true);
      audioPrefetch->stop(true);
      if (realTimeScheduling && watchdogThread)
            pthread_cancel(watchdogThread);
      }

//---------------------------------------------------------
//   seqRestart
//---------------------------------------------------------

bool MusE::seqRestart()
{
    bool restartSequencer = audio->isRunning();
    if (restartSequencer) {
          if (audio->isPlaying()) {
                audio->msgPlay(false);
                while (audio->isPlaying())
                      qApp->processEvents();
                }
          seqStop();
          }
    if(!seqStart())
        return false;

    audioDevice->graphChanged();
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
//   populateAddSynth
//---------------------------------------------------------

/*
struct addSynth_cmp_str 
{
   bool operator()(std::string a, std::string b) 
   {
      return (a < b);
   }
};
*/

Q3PopupMenu* populateAddSynth(QWidget* parent, QObject* obj = 0, const char* slot = 0)
{
  Q3PopupMenu* synp = new Q3PopupMenu(parent);
  
  //typedef std::multimap<std::string, int, addSynth_cmp_str > asmap;
  typedef std::multimap<std::string, int > asmap;
  
  //typedef std::multimap<std::string, int, addSynth_cmp_str >::iterator imap;
  typedef std::multimap<std::string, int >::iterator imap;
  
  MessSynth* synMESS   = 0;
  Q3PopupMenu* synpMESS = 0;
  asmap mapMESS;

  #ifdef DSSI_SUPPORT
  DssiSynth* synDSSI   = 0;
  Q3PopupMenu* synpDSSI = 0;
  asmap mapDSSI;
  #endif                  
  
  #ifdef VST_SUPPORT
  VstSynth*  synVST    = 0;
  Q3PopupMenu* synpVST  = 0;
  asmap mapVST;
  #endif                  
  
  // Not necessary, but what the heck.
  Q3PopupMenu* synpOther = 0;
  asmap mapOther;
  
  //const int synth_base_id = 0x1000;
  int ii = 0;
  for(std::vector<Synth*>::iterator i = synthis.begin(); i != synthis.end(); ++i) 
  {
    synMESS = dynamic_cast<MessSynth*>(*i);
    if(synMESS)
    {
      mapMESS.insert( std::pair<std::string, int> (std::string(synMESS->description().lower().latin1()), ii) );
    }
    else
    {
      
      #ifdef DSSI_SUPPORT
      synDSSI = dynamic_cast<DssiSynth*>(*i);
      if(synDSSI)
      {
        mapDSSI.insert( std::pair<std::string, int> (std::string(synDSSI->description().lower().latin1()), ii) );
      }
      else
      #endif                      
      
      {
        #ifdef VST_SUPPORT
        synVST = dynamic_cast<VstSynth*>(*i);
        if(synVST)
        {
          mapVST.insert( std::pair<std::string, int> (std::string(synVST->description().lower().latin1()), ii) );
        }
        else
        #endif                      
        
        {
          mapOther.insert( std::pair<std::string, int> (std::string((*i)->description().lower().latin1()), ii) );
        }
      }
    }
  
    ++ii;
  }
  
  int sz = synthis.size();
  for(imap i = mapMESS.begin(); i != mapMESS.end(); ++i) 
  {
    int idx = i->second;
    if(idx > sz)           // Sanity check
      continue;
    Synth* s = synthis[idx];
    if(s)
    {
      // No MESS sub-menu yet? Create it now.
      if(!synpMESS)
        synpMESS = new Q3PopupMenu(parent);
      synpMESS->insertItem(QT_TR_NOOP(s->description()) + " <" + QT_TR_NOOP(s->name()) + ">", MENU_ADD_SYNTH_ID_BASE + idx);
    }  
  }
  
  #ifdef DSSI_SUPPORT
  for(imap i = mapDSSI.begin(); i != mapDSSI.end(); ++i) 
  {
    int idx = i->second;
    if(idx > sz)           
      continue;
    Synth* s = synthis[idx];
    if(s)
    {
      // No DSSI sub-menu yet? Create it now.
      if(!synpDSSI)
        synpDSSI = new Q3PopupMenu(parent);
      synpDSSI->insertItem(QT_TR_NOOP(s->description()) + " <" + QT_TR_NOOP(s->name()) + ">", MENU_ADD_SYNTH_ID_BASE + idx);
    }  
  }
  #endif
  
  #ifdef VST_SUPPORT
  for(imap i = mapVST.begin(); i != mapVST.end(); ++i) 
  {
    int idx = i->second;
    if(idx > sz)           
      continue;
    Synth* s = synthis[idx];
    if(s)
    {
      // No VST sub-menu yet? Create it now.
      if(!synpVST)
        synpVST = new Q3PopupMenu(parent);
      synpVST->insertItem(QT_TR_NOOP(s->description()) + " <" + QT_TR_NOOP(s->name()) + ">", MENU_ADD_SYNTH_ID_BASE + idx);
    }  
  }
  #endif
  
  for(imap i = mapOther.begin(); i != mapOther.end(); ++i) 
  {
    int idx = i->second;
    if(idx > sz)          
      continue;
    Synth* s = synthis[idx];
    // No Other sub-menu yet? Create it now.
    if(!synpOther)
      synpOther = new Q3PopupMenu(parent);
    synpOther->insertItem(QT_TR_NOOP(s->description()) + " <" + QT_TR_NOOP(s->name()) + ">", MENU_ADD_SYNTH_ID_BASE + idx);
  }
  
  if(synpMESS)
  {
    synp->insertItem(*synthIcon, QT_TR_NOOP("MESS"), synpMESS, Track::AUDIO_SOFTSYNTH);
    if(obj && slot)
      QObject::connect(synpMESS, SIGNAL(activated(int)), obj,  slot);
  }
  
  #ifdef DSSI_SUPPORT
  if(synpDSSI)
  {
    synp->insertItem(*synthIcon, QT_TR_NOOP("DSSI"), synpDSSI, Track::AUDIO_SOFTSYNTH);
    if(obj && slot)
      QObject::connect(synpDSSI, SIGNAL(activated(int)), obj,  slot);
  }  
  #endif
  
  #ifdef VST_SUPPORT
  if(synpVST)
  {
    synp->insertItem(*synthIcon, QT_TR_NOOP("FST"), synpVST, Track::AUDIO_SOFTSYNTH);
    if(obj && slot)
      QObject::connect(synpVST, SIGNAL(activated(int)), obj,  slot);
  }  
  #endif
  
  if(synpOther)
  {
    synp->insertItem(*synthIcon, QObject::tr("Other"), synpOther, Track::AUDIO_SOFTSYNTH);
    if(obj && slot)
      QObject::connect(synpOther, SIGNAL(activated(int)), obj,  slot);
  }
  
  return synp;
}

//---------------------------------------------------------
//   populateAddTrack
//    this is also used in "mixer"
//---------------------------------------------------------

void populateAddTrack(Q3PopupMenu* addTrack)
      {
      addTrack->insertItem(QIcon(*addtrack_addmiditrackIcon),
         QT_TR_NOOP("Add Midi Track"), Track::MIDI);
      addTrack->insertItem(QIcon(*addtrack_drumtrackIcon),
         QT_TR_NOOP("Add Drum Track"), Track::DRUM);
      addTrack->insertItem(QIcon(*addtrack_wavetrackIcon),
         QT_TR_NOOP("Add Wave Track"), Track::WAVE);
      addTrack->insertItem(QIcon(*addtrack_audiooutputIcon),
         QT_TR_NOOP("Add Audio Output"), Track::AUDIO_OUTPUT);
      addTrack->insertItem(QIcon(*addtrack_audiogroupIcon),
         QT_TR_NOOP("Add Audio Group"), Track::AUDIO_GROUP);
      addTrack->insertItem(QIcon(*addtrack_audioinputIcon),
         QT_TR_NOOP("Add Audio Input"), Track::AUDIO_INPUT);
      addTrack->insertItem(QIcon(*addtrack_auxsendIcon),
         QT_TR_NOOP("Add Aux Send"), Track::AUDIO_AUX);
         
      // Create a sub-menu and fill it with found synth types. Make addTrack the owner.
      Q3PopupMenu* synp = populateAddSynth(addTrack, song, SLOT(addNewTrack(int)));
      // Add the sub-menu to the given menu.
      addTrack->insertItem(*synthIcon, QT_TR_NOOP("Add Synth"), synp, Track::AUDIO_SOFTSYNTH);
         
      //addTrack->connect(addTrack, SIGNAL(activated(int)), song, SLOT(addTrack(int)));
      addTrack->connect(addTrack, SIGNAL(activated(int)), song, SLOT(addNewTrack(int)));
      //synp->connect(synp, SIGNAL(activated(int)), song, SLOT(addNewTrack(int)));
      }

//---------------------------------------------------------
//   MusE
//---------------------------------------------------------

MusE::MusE(int argc, char** argv) : Q3MainWindow(0, "mainwindow")
      {
      // By T356. For LADSPA plugins in plugin.cpp
      // QWidgetFactory::addWidgetFactory( new PluginWidgetFactory ); ddskrjo
      
      setFocusPolicy(Qt::WheelFocus);
      muse                  = this;    // hack
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
      softSynthesizerConfig = 0;
      midiTransformerDialog = 0;
      shortcutConfig        = 0;
      appearance            = 0;
      //audioMixer            = 0;
      mixer1                = 0;
      mixer2                = 0;
      watchdogThread        = 0;
      editInstrument        = 0;
      routingPopupMenu      = 0;
      
      appName               = QString("MusE");

      song           = new Song("song");
      song->blockSignals(true);
      heartBeatTimer = new QTimer(this, "timer");
      connect(heartBeatTimer, SIGNAL(timeout()), song, SLOT(beat()));

#ifdef ENABLE_PYTHON
      //---------------------------------------------------
      //    Python bridge
      //---------------------------------------------------
      // Uncomment in order to enable MusE Python bridge:
      if (usePythonBridge) {
            printf("Initializing python bridge!\n");
            if (initPythonBridge() == false) {
                  printf("Could not initialize Python bridge\n");
                  exit(1);
                  }
            }
#endif

      //---------------------------------------------------
      //    undo/redo
      //---------------------------------------------------
      undoRedo = new Q3ActionGroup(this, tr("UndoRedo"), false);
      undoAction = new Q3Action(tr("undo"), QIcon(*undoIconS), tr("Und&o"), // ddskrjo
        Qt::CTRL+Qt::Key_Z, undoRedo, "undo");
      redoAction = new Q3Action(tr("redo"), QIcon(*redoIconS), tr("Re&do"), // ddskrjo
        Qt::CTRL+Qt::Key_Y, undoRedo, "redo");
      undoAction->setWhatsThis(tr("undo last change to song"));
      redoAction->setWhatsThis(tr("redo last undo"));
      undoAction->setEnabled(false);
      redoAction->setEnabled(false);
      connect(redoAction, SIGNAL(activated()), song, SLOT(redo()));
      connect(undoAction, SIGNAL(activated()), song, SLOT(undo()));

      //---------------------------------------------------
      //    Transport
      //---------------------------------------------------

      transportAction = new Q3ActionGroup(this, tr("Transport"), false);

      loopAction = new Q3Action(tr("loop"), QIcon(*loop1Icon),
         tr("Loop"), 0, transportAction, "loop", true);
      loopAction->setWhatsThis(tr(infoLoopButton));
      connect(loopAction, SIGNAL(toggled(bool)), song, SLOT(setLoop(bool)));

      punchinAction = new Q3Action(tr("punchin"), QIcon(*punchin1Icon),
         tr("Punchin"), 0, transportAction, "Punchin", true);
      punchinAction->setWhatsThis(tr(infoPunchinButton));
      connect(punchinAction, SIGNAL(toggled(bool)), song, SLOT(setPunchin(bool)));

      punchoutAction = new Q3Action(tr("punchout"), QIcon(*punchout1Icon),
         tr("Punchout"), 0, transportAction, "punchout", true);
      punchoutAction->setWhatsThis(tr(infoPunchoutButton));
      connect(punchoutAction, SIGNAL(toggled(bool)), song, SLOT(setPunchout(bool)));

      transportAction->addSeparator();

      startAction = new Q3Action(tr("start"), QIcon(*startIcon),
         tr("Start"), 0, transportAction, "start");
      startAction->setWhatsThis(tr(infoStartButton));
      connect(startAction, SIGNAL(activated()), song, SLOT(rewindStart()));

      rewindAction = new Q3Action(tr("rewind"), QIcon(*frewindIcon),
         tr("Rewind"), 0, transportAction, "rewind");
      rewindAction->setWhatsThis(tr(infoRewindButton));
      connect(rewindAction, SIGNAL(activated()), song, SLOT(rewind()));

      forwardAction = new Q3Action(tr("forward"), QIcon(*fforwardIcon),
         tr("Forward"), 0, transportAction, "forward");
      forwardAction->setWhatsThis(tr(infoForwardButton));
      connect(forwardAction, SIGNAL(activated()), song, SLOT(forward()));

      stopAction = new Q3Action(tr("stop"), QIcon(*stopIcon),
         tr("Stop"), 0, transportAction, "stop", true);
      stopAction->setWhatsThis(tr(infoStopButton));
      stopAction->setOn(true);
      connect(stopAction, SIGNAL(toggled(bool)), song, SLOT(setStop(bool)));

      playAction = new Q3Action(tr("play"),  QIcon(*playIcon),
         tr("Play"), 0, transportAction, "play", true);
      playAction->setWhatsThis(tr(infoPlayButton));
      playAction->setOn(false);
      connect(playAction, SIGNAL(toggled(bool)), song, SLOT(setPlay(bool)));

      recordAction = new Q3Action(tr("record"),  QIcon(*recordIcon),
         tr("Record"), 0, transportAction, "record", true);
      recordAction->setWhatsThis(tr(infoRecordButton));
      connect(recordAction, SIGNAL(toggled(bool)), song, SLOT(setRecord(bool)));

      panicAction = new Q3Action(tr("panic"),  QIcon(*panicIcon),
         tr("Panic"), 0, 0, "panic", false);
      panicAction->setWhatsThis(tr(infoPanicButton));
      connect(panicAction, SIGNAL(activated()), song, SLOT(panic()));

      initMidiInstruments();
      initMidiPorts();
      ::initMidiDevices();

      //----Actions

      fileNewAction = new Q3Action(tr("new"),
        QIcon(*filenewIcon), tr("&New"), 0, this, "new"); // ddskrjo
      fileNewAction->setToolTip(tr(fileNewText));
      fileNewAction->setWhatsThis(tr(fileNewText));

      fileOpenAction = new Q3Action(tr("open"),
        QIcon(*openIcon), tr("&Open"), 0, this, "open"); // ddskrjo
      fileOpenAction->setToolTip(tr(fileOpenText));
      fileOpenAction->setWhatsThis(tr(fileOpenText));

      fileSaveAction = new Q3Action(tr("save"),
        QIcon(*saveIcon), tr("&Save"), 0, this, "save"); // ddskrjo
      fileSaveAction->setToolTip(tr(fileSaveText));
      fileSaveAction->setWhatsThis(tr(fileSaveText));

      pianoAction = new Q3Action(tr("pianoroll"),
        *pianoIconSet, tr("Pianoroll"), 0, this, "pianoroll");
      connect(pianoAction, SIGNAL(activated()), SLOT(startPianoroll()));

//       markerAction = new QAction(tr("marker"), QIconSet(*view_markerIcon), tr("Marker"),
//         0, this, "marker");
//       connect(markerAction, SIGNAL(activated()), SLOT(startMarkerView()));

      connect(fileNewAction,  SIGNAL(activated()), SLOT(loadTemplate()));
      connect(fileOpenAction, SIGNAL(activated()), SLOT(loadProject()));
      connect(fileSaveAction, SIGNAL(activated()), SLOT(save()));

      //--------------------------------------------------
      //    Toolbar
      //--------------------------------------------------

      tools = new Q3ToolBar(tr("File Buttons"), this);
      fileNewAction->addTo(tools);
      fileOpenAction->addTo(tools);
      fileSaveAction->addTo(tools);

      //
      //    Whats This
      //
      Q3WhatsThis::whatsThisButton(tools);

      tools->addSeparator();
      undoRedo->addTo(tools);

      tools1 = new EditToolBar(this, arrangerTools);

      Q3ToolBar* transportToolbar = new Q3ToolBar(this);
      transportAction->addTo(transportToolbar);

      Q3ToolBar* panicToolbar = new Q3ToolBar(this);
      panicAction->addTo(panicToolbar);

      if (realTimePriority < sched_get_priority_min(SCHED_FIFO))
            realTimePriority = sched_get_priority_min(SCHED_FIFO);
      else if (realTimePriority > sched_get_priority_max(SCHED_FIFO))
            realTimePriority = sched_get_priority_max(SCHED_FIFO);

      // If we requested to force the midi thread priority...
      if(midiRTPrioOverride > 0)
      {
        if (midiRTPrioOverride < sched_get_priority_min(SCHED_FIFO))
            midiRTPrioOverride = sched_get_priority_min(SCHED_FIFO);
        else if (midiRTPrioOverride > sched_get_priority_max(SCHED_FIFO))
            midiRTPrioOverride = sched_get_priority_max(SCHED_FIFO);
      }
            
      // Changed by Tim. p3.3.17
      //midiSeq       = new MidiSeq(realTimeScheduling ? realTimePriority : 0, "Midi");
      midiSeq       = new MidiSeq("Midi");
      audio         = new Audio();
      //audioPrefetch = new AudioPrefetch(0, "Disc");
      audioPrefetch = new AudioPrefetch("Prefetch");

      //---------------------------------------------------
      //    Popups
      //---------------------------------------------------

//       QPopupMenu *foo = new QPopupMenu(this);
//       testAction = new QAction(foo,"testPython");
//       testAction->addTo(foo);
//       menuBar()->insertItem(tr("&testpython"), foo);
//       connect(testAction, SIGNAL(activated()), this, SLOT(runPythonScript()));


      //-------------------------------------------------------------
      //    popup File
      //-------------------------------------------------------------

      menu_file = new Q3PopupMenu(this);
      menuBar()->insertItem(tr("&File"), menu_file);
      fileNewAction->addTo(menu_file);
      fileOpenAction->addTo(menu_file);
      openRecent = new Q3PopupMenu(menu_file);
      connect(openRecent, SIGNAL(aboutToShow()), this, SLOT(openRecentMenu()));
      connect(openRecent, SIGNAL(activated(int)), this, SLOT(selectProject(int)));
      menu_ids[CMD_OPEN_RECENT] = menu_file->insertItem(tr("Open &Recent"), openRecent, 0);
      menu_file->insertSeparator();
      fileSaveAction->addTo(menu_file);
      menu_ids[CMD_SAVE_AS] = menu_file->insertItem(tr("Save &As"), this, SLOT(saveAs()), 0, -2);
      menu_file->insertSeparator();
      menu_ids[CMD_IMPORT_MIDI] = menu_file->insertItem(*openIconS, tr("Import Midifile"), this, SLOT(importMidi()), 0, -2);
      menu_ids[CMD_EXPORT_MIDI] = menu_file->insertItem(*saveIconS, tr("Export Midifile"), this, SLOT(exportMidi()), 0, -2);
      menu_ids[CMD_IMPORT_PART]   = menu_file->insertItem(*openIconS, tr("Import Part"), this, SLOT(importPart()), 0, -2);
      menu_file->insertSeparator();
      menu_ids[CMD_IMPORT_AUDIO] = menu_file->insertItem(*openIconS, tr("Import Wave File"), this, SLOT(importWave()), 0, -2);


      menu_file->insertSeparator();
      menu_ids[CMD_QUIT] = menu_file->insertItem(*exitIconS, tr("&Quit"), this, SLOT(quitDoc()), 0, -2);
      menu_file->insertSeparator();

      //-------------------------------------------------------------
      //    popup Edit
      //-------------------------------------------------------------

      menuEdit = new Q3PopupMenu(this);
      undoRedo->addTo(menuEdit);
      menuEdit->insertSeparator();
      menuBar()->insertItem(tr("&Edit"), menuEdit);

      menuEdit->insertItem(*editcutIconSet, tr("C&ut"),   CMD_CUT);
      menuEdit->setAccel(Qt::CTRL+Qt::Key_X, CMD_CUT);
      menuEdit->insertItem(*editcopyIconSet, tr("&Copy"),  CMD_COPY);
      menuEdit->setAccel(Qt::CTRL+Qt::Key_C, CMD_COPY);
      menuEdit->insertItem(*editpasteIconSet, tr("&Paste"), CMD_PASTE);
      menuEdit->setAccel(Qt::CTRL+Qt::Key_V, CMD_PASTE);
      menuEdit->insertItem(*editpasteIconSet, tr("&Insert"), CMD_INSERT);
      menuEdit->setAccel(Qt::CTRL+Qt::SHIFT+Qt::Key_I, CMD_INSERT);
      menuEdit->insertItem(*editpasteCloneIconSet, tr("Paste c&lone"), CMD_PASTE_CLONE);
      menuEdit->setAccel(Qt::CTRL+Qt::SHIFT+Qt::Key_V, CMD_PASTE_CLONE);
      menuEdit->insertItem(*editpaste2TrackIconSet, tr("Paste to &track"), CMD_PASTE_TO_TRACK);
      menuEdit->setAccel(Qt::CTRL+Qt::Key_B, CMD_PASTE_TO_TRACK);
      menuEdit->insertItem(*editpasteClone2TrackIconSet, tr("Paste clone to trac&k"), CMD_PASTE_CLONE_TO_TRACK);
      menuEdit->setAccel(Qt::CTRL+Qt::SHIFT+Qt::Key_B, CMD_PASTE_CLONE_TO_TRACK);

      menuEdit->insertItem(*editpasteIconSet, tr("&Insert empty measure"), CMD_INSERTMEAS);
      menuEdit->setAccel(Qt::CTRL+Qt::SHIFT+Qt::Key_X, CMD_INSERTMEAS);
      menuEdit->insertSeparator();
      menuEdit->insertItem(QIcon(*edit_track_delIcon),
         tr("Delete Selected Tracks"), CMD_DELETE_TRACK);

      addTrack = new Q3PopupMenu(this);
      // Moved below. Have to wait until synths are available...
      //populateAddTrack(addTrack);
      
      menuEdit->insertItem(QIcon(*edit_track_addIcon),
         tr("Add Track"), addTrack);

      select = new Q3PopupMenu(this);
      select->insertItem(QIcon(*select_allIcon),
         tr("Select &All"),  CMD_SELECT_ALL);
      select->insertItem(QIcon(*select_deselect_allIcon),
         tr("&Deselect All"), CMD_SELECT_NONE);
      menuEdit->insertSeparator();
      select->insertItem(QIcon(*select_invert_selectionIcon),
         tr("Invert &Selection"), CMD_SELECT_INVERT);
      select->insertItem(QIcon(*select_inside_loopIcon),
         tr("&Inside Loop"), CMD_SELECT_ILOOP);
      select->insertItem(QIcon(*select_outside_loopIcon),
         tr("&Outside Loop"), CMD_SELECT_OLOOP);
      select->insertItem(QIcon(*select_all_parts_on_trackIcon),
         tr("All &Parts on Track"), CMD_SELECT_PARTS);
      menuEdit->insertItem(QIcon(*selectIcon),
         tr("Select"), select);
      menuEdit->insertSeparator();

      pianoAction->addTo(menuEdit);
      menu_ids[CMD_OPEN_DRUMS] = menuEdit->insertItem(
         QIcon(*edit_drummsIcon), tr("Drums"), this, SLOT(startDrumEditor()), 0);
      menu_ids[CMD_OPEN_LIST]  = menuEdit->insertItem(
         QIcon(*edit_listIcon), tr("List"), this, SLOT(startListEditor()), 0);
      menu_ids[CMD_OPEN_WAVE]  = menuEdit->insertItem(
         QIcon(*edit_waveIcon), tr("Wave"), this, SLOT(startWaveEditor()), 0);

      master = new Q3PopupMenu(this);
      master->setCheckable(false);
      menu_ids[CMD_OPEN_GRAPHIC_MASTER] = master->insertItem(
        QIcon(*mastertrack_graphicIcon),tr("Graphic"), this, SLOT(startMasterEditor()), 0);
      menu_ids[CMD_OPEN_LIST_MASTER] = master->insertItem(
  QIcon(*mastertrack_listIcon),tr("List"), this, SLOT(startLMasterEditor()), 0);
      menuEdit->insertItem(QIcon(*edit_mastertrackIcon),
         tr("Mastertrack"), master, Qt::Key_F);

      menuEdit->insertSeparator();
      connect(menuEdit, SIGNAL(activated(int)), SLOT(cmd(int)));
      connect(select, SIGNAL(activated(int)), SLOT(cmd(int)));

      midiEdit = new Q3PopupMenu(this);
      midiEdit->setCheckable(false);
#if 0  // TODO
      menu_ids[CMD_OPEN_MIDI_TRANSFORM] = midiEdit->insertItem(
         QIcon(*midi_transformIcon), tr("Midi &Transform"), this, SLOT(startMidiTransformer()), 0);
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
      menu_ids[CMD_TRANSPOSE] = midiEdit->insertItem(
         QIcon(*midi_transposeIcon), tr("Transpose"), this, SLOT(transpose()), 0);
      menuEdit->insertItem(
         QIcon(*edit_midiIcon), tr("Midi"), midiEdit);

      menuEdit->insertSeparator();
      menuEdit->insertItem(
          QIcon(*edit_listIcon), tr("Song info"), this, SLOT(startSongInfo()), 0);
      //-------------------------------------------------------------
      //    popup View
      //-------------------------------------------------------------

      menuView = new Q3PopupMenu(this);
      menuView->setCheckable(true);
      menuBar()->insertItem(tr("View"), menuView);

      tr_id = menuView->insertItem(
         QIcon(*view_transport_windowIcon), tr("Transport Panel"), this, SLOT(toggleTransport()), 0);
      bt_id = menuView->insertItem(
         QIcon(*view_bigtime_windowIcon), tr("Bigtime window"),  this, SLOT(toggleBigTime()), 0);
      //aid1  = menuView->insertItem(
      //   QIconSet(*mixerSIcon), tr("Mixer"), this, SLOT(toggleMixer()), 0);
      aid1a  = menuView->insertItem(
         QIcon(*mixerSIcon), tr("Mixer A"), this, SLOT(toggleMixer1()), 0);
      aid1b  = menuView->insertItem(
         QIcon(*mixerSIcon), tr("Mixer B"), this, SLOT(toggleMixer2()), 0);
      // p3.2.24
      aid2  = menuView->insertItem(
         QIcon(*cliplistSIcon), tr("Cliplist"), this, SLOT(startClipList()), 0);
      mr_id = menuView->insertItem(
         QIcon(*view_markerIcon), tr("Marker View"),  this, SLOT(toggleMarker()), 0);
      //markerAction->addTo(menuView);

      
      //-------------------------------------------------------------
      //    popup Structure
      //-------------------------------------------------------------

      menuStructure = new Q3PopupMenu(this);
      menuStructure->setCheckable(false);
      menuBar()->insertItem(tr("&Structure"), menuStructure);
      menu_ids[CMD_GLOBAL_CUT] = menuStructure->insertItem(tr("Global Cut"),    this, SLOT(globalCut()), 0);
      menu_ids[CMD_GLOBAL_INSERT] = menuStructure->insertItem(tr("Global Insert"), this, SLOT(globalInsert()), 0);
      menu_ids[CMD_GLOBAL_SPLIT] = menuStructure->insertItem(tr("Global Split"),  this, SLOT(globalSplit()), 0);
      menu_ids[CMD_COPY_RANGE] = menuStructure->insertItem(tr("Copy Range"),    this, SLOT(copyRange()), 0);
      menuStructure->setItemEnabled(menu_ids[CMD_COPY_RANGE], false);
      menuStructure->insertSeparator();
      menu_ids[CMD_CUT_EVENTS] = menuStructure->insertItem(tr("Cut Events"),    this, SLOT(cutEvents()), 0);
      menuStructure->setItemEnabled(menu_ids[CMD_CUT_EVENTS], false);

      //-------------------------------------------------------------
      //    popup Midi
      //-------------------------------------------------------------

      midiInputPlugins = new Q3PopupMenu(this);
      midiInputPlugins->setCheckable(false);
      mpid0 = midiInputPlugins->insertItem(
         QIcon(*midi_inputplugins_transposeIcon), tr("Transpose"), 0);
      mpid1 = midiInputPlugins->insertItem(
         QIcon(*midi_inputplugins_midi_input_transformIcon), tr("Midi Input Transform"), 1);
      mpid2 = midiInputPlugins->insertItem(
         QIcon(*midi_inputplugins_midi_input_filterIcon), tr("Midi Input Filter"), 2);
      mpid3 = midiInputPlugins->insertItem(
         QIcon(*midi_inputplugins_remote_controlIcon), tr("Midi Remote Control"), 3);
/*
**      mpid4 = midiInputPlugins->insertItem(
**         QIconSet(*midi_inputplugins_random_rhythm_generatorIcon), tr("Random Rhythm Generator"), 4);
*/
      connect(midiInputPlugins, SIGNAL(activated(int)), SLOT(startMidiInputPlugin(int)));

//      midiInputPlugins->setItemEnabled(mpid4, false);

      menu_functions = new Q3PopupMenu(this);
      menu_functions->setCheckable(true);
      menuBar()->insertItem(tr("&Midi"), menu_functions);
      menu_functions->setCaption(tr("Midi"));

      menuScriptPlugins = new Q3PopupMenu(this);
      song->populateScriptMenu(menuScriptPlugins, this);
      menu_functions->insertItem(tr("&Plugins"), menuScriptPlugins);
      
      menu_ids[CMD_MIDI_EDIT_INSTRUMENTS] = menu_functions->insertItem(
         QIcon(*midi_edit_instrumentIcon), tr("Edit Instrument"), this, SLOT(startEditInstrument()), 0);
      menu_functions->insertItem(
         QIcon(*midi_inputpluginsIcon), tr("Input Plugins"), midiInputPlugins, Qt::Key_P);
      menu_functions->insertSeparator();
      menu_ids[CMD_MIDI_RESET] = menu_functions->insertItem(
         QIcon(*midi_reset_instrIcon), tr("Reset Instr."), this, SLOT(resetMidiDevices()), 0);
      menu_ids[CMD_MIDI_INIT] = menu_functions->insertItem(
         QIcon(*midi_init_instrIcon), tr("Init Instr."),  this, SLOT(initMidiDevices()), 0);
      menu_ids[CMD_MIDI_LOCAL_OFF] = menu_functions->insertItem(
         QIcon(*midi_local_offIcon), tr("local off"), this, SLOT(localOff()), 0);

      //-------------------------------------------------------------
      //    popup Audio
      //-------------------------------------------------------------

      menu_audio = new Q3PopupMenu(this);
      menu_audio->setCheckable(true);
      menuBar()->insertItem(tr("&Audio"), menu_audio);
      menu_ids[CMD_AUDIO_BOUNCE_TO_TRACK] = menu_audio->insertItem(
         QIcon(*audio_bounce_to_trackIcon), tr("Bounce to Track"), this, SLOT(bounceToTrack()), 0);
      menu_ids[CMD_AUDIO_BOUNCE_TO_FILE] = menu_audio->insertItem(
         QIcon(*audio_bounce_to_fileIcon), tr("Bounce to File"), this, SLOT(bounceToFile()), 0);
      menu_audio->insertSeparator();
      menu_ids[CMD_AUDIO_RESTART] = menu_audio->insertItem(
         QIcon(*audio_restartaudioIcon), tr("Restart Audio"), this, SLOT(seqRestart()), 0);

      //-------------------------------------------------------------
      //    popup Automation
      //-------------------------------------------------------------

      menuAutomation = new Q3PopupMenu(this);
      menuAutomation->setCheckable(true);
      menuBar()->insertItem(tr("Automation"), menuAutomation);
      autoId = menuAutomation->insertItem(
         QIcon(*automation_mixerIcon), tr("Mixer Automation"), this, SLOT(switchMixerAutomation()), 0);
      menuAutomation->insertSeparator();
      menu_ids[CMD_MIXER_SNAPSHOT] = menuAutomation->insertItem(
         QIcon(*automation_take_snapshotIcon), tr("Take Snapshot"), this, SLOT(takeAutomationSnapshot()), 0);
      menu_ids[CMD_MIXER_AUTOMATION_CLEAR] = menuAutomation->insertItem(
         QIcon(*automation_clear_dataIcon), tr("Clear Automation Data"), this, SLOT(clearAutomation()), 0);
      menuAutomation->setItemEnabled(menu_ids[CMD_MIXER_AUTOMATION_CLEAR], false);

      //-------------------------------------------------------------
      //    popup Settings
      //-------------------------------------------------------------

      follow = new Q3PopupMenu(this);
      follow->setCheckable(false);
      fid0 = follow->insertItem(tr("dont follow Song"), CMD_FOLLOW_NO);
      fid1 = follow->insertItem(tr("follow page"), CMD_FOLLOW_JUMP);
      fid2 = follow->insertItem(tr("follow continuous"), CMD_FOLLOW_CONTINUOUS);
      follow->setItemChecked(fid1, true);
      connect(follow, SIGNAL(activated(int)), SLOT(cmd(int)));

      menuSettings = new Q3PopupMenu(this);
      menuSettings->setCheckable(false);
      menuBar()->insertItem(tr("Settings"), menuSettings);
      menu_ids[CMD_GLOBAL_CONFIG] = menuSettings->insertItem(
         QIcon(*settings_globalsettingsIcon), tr("Global Settings"), this, SLOT(configGlobalSettings()),0);
      menu_ids[CMD_CONFIG_SHORTCUTS] = menuSettings->insertItem(
         QIcon(*settings_configureshortcutsIcon), tr("Configure shortcuts"), this, SLOT(configShortCuts()), 0);
      menuSettings->insertItem(
         QIcon(*settings_follow_songIcon), tr("follow song"), follow, Qt::Key_F);
      menu_ids[CMD_CONFIG_METRONOME] = menuSettings->insertItem(
         QIcon(*settings_metronomeIcon), tr("Metronome"), this, SLOT(configMetronome()), 0);
      menuSettings->insertSeparator();
      menu_ids[CMD_CONFIG_MIDISYNC] = menuSettings->insertItem(
         QIcon(*settings_midisyncIcon), tr("Midi Sync"), this, SLOT(configMidiSync()), 0);
      menu_ids[CMD_MIDI_FILE_CONFIG] = menuSettings->insertItem(
         QIcon(*settings_midifileexportIcon), tr("Midi File Import/Export"), this, SLOT(configMidiFile()), 0);
      menuSettings->insertSeparator();
      menu_ids[CMD_APPEARANCE_SETTINGS] = menuSettings->insertItem(
         QIcon(*settings_appearance_settingsIcon), tr("Appearance settings"), this, SLOT(configAppearance()), 0);
      menuSettings->insertSeparator();
      menu_ids[CMD_CONFIG_MIDI_PORTS] = menuSettings->insertItem(
         QIcon(*settings_midiport_softsynthsIcon), tr("Midi Ports / Soft Synth"), this, SLOT(configMidiPorts()), 0);

      //---------------------------------------------------
      //    popup Help
      //---------------------------------------------------

      menuBar()->insertSeparator();
      menu_help = new Q3PopupMenu(this);
      menu_help->setCheckable(false);
      menuBar()->insertItem(tr("&Help"), menu_help);

      menu_ids[CMD_OPEN_HELP] = menu_help->insertItem(tr("&Manual"), this, SLOT(startHelpBrowser()), 0);
      menu_ids[CMD_OPEN_HOMEPAGE] = menu_help->insertItem(tr("&MusE homepage"), this, SLOT(startHomepageBrowser()), 0);
      menu_help->insertSeparator();
      menu_ids[CMD_OPEN_BUG] = menu_help->insertItem(tr("&Report Bug..."), this, SLOT(startBugBrowser()), 0);
      menu_help->insertSeparator();
      menu_help->insertItem(tr("&About MusE"), this, SLOT(about()));
      //menu_help->insertItem(tr("About&Qt"), this, SLOT(aboutQt()));
      //menu_help->insertSeparator();
      //menu_ids[CMD_START_WHATSTHIS] = menu_help->insertItem(tr("What's &This?"), this, SLOT(whatsThis()), 0);

      //---------------------------------------------------
      //    Central Widget
      //---------------------------------------------------

      arranger = new Arranger(this, "arranger");
      setCentralWidget(arranger);

      connect(tools1, SIGNAL(toolChanged(int)), arranger, SLOT(setTool(int)));
      connect(arranger, SIGNAL(editPart(Track*)), SLOT(startEditor(Track*)));
      connect(arranger, SIGNAL(dropSongFile(const QString&)), SLOT(loadProjectFile(const QString&)));
      connect(arranger, SIGNAL(dropMidiFile(const QString&)), SLOT(importMidi(const QString&)));
      connect(arranger, SIGNAL(startEditor(PartList*,int)),  SLOT(startEditor(PartList*,int)));
      connect(arranger, SIGNAL(toolChanged(int)), tools1, SLOT(set(int)));
      connect(this, SIGNAL(configChanged()), arranger, SLOT(configChanged()));

      connect(arranger, SIGNAL(setUsedTool(int)), SLOT(setUsedTool(int)));

      //---------------------------------------------------
      //  read list of "Recent Projects"
      //---------------------------------------------------

      QString prjPath(getenv("HOME"));
      prjPath += QString("/.musePrj");
      FILE* f = fopen(prjPath.latin1(), "r");
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

      initMidiSynth();

      populateAddTrack(addTrack);
      
      transport = new Transport(this, "transport");
      bigtime   = 0;

      QClipboard* cb = QApplication::clipboard();
      connect(cb, SIGNAL(dataChanged()), SLOT(clipboardChanged()));
      connect(cb, SIGNAL(selectionChanged()), SLOT(clipboardChanged()));
      connect(arranger, SIGNAL(selectionChanged()), SLOT(selectionChanged()));

      //---------------------------------------------------
      //  load project
      //    if no songname entered on command line:
      //    startMode: 0  - load last song
      //               1  - load default template
      //               2  - load configured start song
      //---------------------------------------------------

      QString name;
      bool useTemplate = false;
      if (argc >= 2)
            name = argv[0];
      else if (config.startMode == 0) {
            if (argc < 2)
                  name = projectList[0] ? *projectList[0] : QString("untitled");
            else
                  name = argv[0];
            printf("starting with selected song %s\n", config.startSong.latin1());
            }
      else if (config.startMode == 1) {
            printf("starting with default template\n");
            name = museGlobalShare + QString("/templates/default.med");
            useTemplate = true;
            }
      else if (config.startMode == 2) {
            printf("starting with pre configured song %s\n", config.startSong.latin1());
            name = config.startSong;
      }
      song->blockSignals(false);
      loadProjectFile(name, useTemplate, true);
      changeConfig(false);

      song->update();
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
      // Added by T356
      //audio->msgIdle(true);
      
      audio->msgInitMidiDevices();
      
      // Added by T356
      //audio->msgIdle(false);
      }

//---------------------------------------------------------
//   localOff
//---------------------------------------------------------

void MusE::localOff()
      {
      audio->msgLocalOff();
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

void MusE::loadProjectFile(const QString& name, bool songTemplate, bool loadAll)
      {
      //
      // stop audio threads if running
      //
      bool restartSequencer = audio->isRunning();
      if (restartSequencer) {
            if (audio->isPlaying()) {
                  audio->msgPlay(false);
                  while (audio->isPlaying())
                        qApp->processEvents();
                  }
            seqStop();
            }
      microSleep(200000);
      loadProjectFile1(name, songTemplate, loadAll);
      microSleep(200000);
      if (restartSequencer)
            seqStart();

      if (song->getSongInfo().length()>0)
          startSongInfo(false);
      }

//---------------------------------------------------------
//   loadProjectFile
//    load *.med, *.mid, *.kar
//
//    template - if true, load file but do not change
//                project name
//    loadAll  - load song data + configuration data
//---------------------------------------------------------

void MusE::loadProjectFile1(const QString& name, bool songTemplate, bool loadAll)
      {
      //if (audioMixer)
      //      audioMixer->clear();
      if (mixer1)
            mixer1->clear();
      if (mixer2)
            mixer2->clear();
      arranger->clear();      // clear track info
      if (clearSong())
            return;

      QFileInfo fi(name);
      if (songTemplate) {
            if (!fi.isReadable()) {
                  QMessageBox::critical(this, QString("MusE"),
                     tr("Cannot read template"));
                  return;
                  }
            project.setFile("untitled");
            }
      else {
            printf("Setting project path to %s\n", fi.dirPath(true).latin1());
            museProject = fi.dirPath(true);
            project.setFile(name);
            }
      // Changed by T356. 01/19/2010. We want the complete extension here. 
      //QString ex = fi.extension(false).lower();
      //if (ex.length() == 3)
      //      ex += ".";
      //ex = ex.left(4);
      QString ex = fi.extension(true).lower();
      QString mex = ex.section('.', -1, -1);  
      if((mex == "gz") || (mex == "bz2"))
        mex = ex.section('.', -2, -2);  
        
      //if (ex.isEmpty() || ex == "med.") {
      if (ex.isEmpty() || mex == "med") {
            //
            //  read *.med file
            //
            bool popenFlag;
            FILE* f = fileOpen(this, fi.filePath(), QString(".med"), "r", popenFlag, true);
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
                  Xml xml(f);
                  read(xml, !loadAll);
                  bool fileError = ferror(f);
                  popenFlag ? pclose(f) : fclose(f);
                  if (fileError) {
                        QMessageBox::critical(this, QString("MusE"),
                           tr("File read error"));
                        setUntitledProject();
                        }
                  }
            }
      //else if (ex == "mid." || ex == "kar.") {
      else if (mex == "mid" || mex == "kar") {
            setConfigDefaults();
            if (!importMidi(name, false))
                  setUntitledProject();
            }
      else {
            QMessageBox::critical(this, QString("MusE"),
               tr("Unknown File Format: ") + ex);
            setUntitledProject();
            }
      if (!songTemplate) {
            addProject(project.absFilePath());
            setCaption(QString("MusE: Song: ") + project.baseName(true));
            }
      song->dirty = false;

      menuView->setItemChecked(tr_id, config.transportVisible);
      menuView->setItemChecked(bt_id, config.bigTimeVisible);
      menuView->setItemChecked(mr_id, config.markerVisible);
      menuAutomation->setItemChecked(autoId, automation);

      if (loadAll) {
            showBigtime(config.bigTimeVisible);
            //showMixer(config.mixerVisible);
            showMixer1(config.mixer1Visible);
            showMixer2(config.mixer2Visible);
            
            // Added p3.3.43 Make sure the geometry is correct because showMixerX() will NOT 
            //  set the geometry if the mixer has already been created.
            if(mixer1)
            {
              //if(mixer1->geometry().size() != config.mixer1.geometry.size())   // p3.3.53 Moved below
              //  mixer1->resize(config.mixer1.geometry.size());
              
              if(mixer1->geometry().topLeft() != config.mixer1.geometry.topLeft())
                mixer1->move(config.mixer1.geometry.topLeft());
            }
            if(mixer2)
            {
              //if(mixer2->geometry().size() != config.mixer2.geometry.size())   // p3.3.53 Moved below
              //  mixer2->resize(config.mixer2.geometry.size());
              
              if(mixer2->geometry().topLeft() != config.mixer2.geometry.topLeft())
                mixer2->move(config.mixer2.geometry.topLeft());
            }
            
            showMarker(config.markerVisible);
            resize(config.geometryMain.size());
            move(config.geometryMain.topLeft());

            if (config.transportVisible)
                  transport->show();
            transport->move(config.geometryTransport.topLeft());
            showTransport(config.transportVisible);
            }

      transport->setMasterFlag(song->masterFlag());
      punchinAction->setOn(song->punchin());
      punchoutAction->setOn(song->punchout());
      loopAction->setOn(song->loop());
      song->update();
      song->updatePos();
      clipboardChanged(); // enable/disable "Paste"
      selectionChanged(); // enable/disable "Copy" & "Paste"
      
      // p3.3.53 Try this AFTER the song update above which does a mixer update... Tested OK - mixers resize properly now.
      if (loadAll) 
      {
            if(mixer1)
            {
              if(mixer1->geometry().size() != config.mixer1.geometry.size())
              {
                //printf("MusE::loadProjectFile1 resizing mixer1 x:%d y:%d w:%d h:%d\n", config.mixer1.geometry.x(), 
                //                                                                       config.mixer1.geometry.y(), 
                //                                                                       config.mixer1.geometry.width(), 
                //                                                                       config.mixer1.geometry.height()
                //                                                                       );  
                mixer1->resize(config.mixer1.geometry.size());
              }
            }  
            if(mixer2)
            {
              if(mixer2->geometry().size() != config.mixer2.geometry.size())
              {
                //printf("MusE::loadProjectFile1 resizing mixer2 x:%d y:%d w:%d h:%d\n", config.mixer2.geometry.x(), 
                //                                                                       config.mixer2.geometry.y(), 
                //                                                                       config.mixer2.geometry.width(), 
                //                                                                       config.mixer2.geometry.height()
                //                                                                       );  
                mixer2->resize(config.mixer2.geometry.size());
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
      QString name("untitled");
      museProject = QFileInfo(name).dirPath(true);
      project.setFile(name);
      setCaption(tr("MusE: Song: ") + project.baseName(true));
      }

//---------------------------------------------------------
//   setConfigDefaults
//---------------------------------------------------------

void MusE::setConfigDefaults()
      {
      readConfiguration();    // used for reading midi files
#if 0
      if (readConfiguration()) {
            //
            // failed to load config file
            // set buildin defaults
            //
            configTransportVisible = false;
            configBigTimeVisible   = false;

            for (int channel = 0; channel < 2; ++channel)
                  song->addTrack(Track::AUDIO_GROUP);
            AudioTrack* out = (AudioTrack*)song->addTrack(Track::AUDIO_OUTPUT);
            AudioTrack* in  = (AudioTrack*)song->addTrack(Track::AUDIO_INPUT);

            // set some default routes
            std::list<QString> il = audioDevice->inputPorts();
            int channel = 0;
            for (std::list<QString>::iterator i = il.begin(); i != il.end(); ++i, ++channel) {
                  if (channel == 2)
                        break;
                  audio->msgAddRoute(Route(out,channel), Route(*i,channel));
                  }
            channel = 0;
            std::list<QString> ol = audioDevice->outputPorts();
            for (std::list<QString>::iterator i = ol.begin(); i != ol.end(); ++i, ++channel) {
                  if (channel == 2)
                        break;
                  audio->msgAddRoute(Route(*i, channel), Route(in,channel));
                  }
            }
#endif
      song->dirty = false;
      }

//---------------------------------------------------------
//   setFollow
//---------------------------------------------------------

void MusE::setFollow()
      {
      Song::FollowMode fm = song->follow();
      follow->setItemChecked(fid0, fm == Song::NO);
      follow->setItemChecked(fid1, fm == Song::JUMP);
      follow->setItemChecked(fid2, fm == Song::CONTINUOUS);
      }

//---------------------------------------------------------
//   MusE::loadProject
//---------------------------------------------------------

void MusE::loadProject()
      {
      bool loadAll;
      QString fn = getOpenFileName(QString(""), med_file_pattern, this,
         tr("MusE: load project"), &loadAll);
      if (!fn.isEmpty()) {
            museProject = QFileInfo(fn).dirPath(true);
            loadProjectFile(fn, false, loadAll);
            }
      }

//---------------------------------------------------------
//   loadTemplate
//---------------------------------------------------------

void MusE::loadTemplate()
      {
      QString fn = getOpenFileName(QString("templates"), med_file_pattern, this,
         tr("MusE: load template"), 0);
      if (!fn.isEmpty()) {
            // museProject = QFileInfo(fn).dirPath(true);
            loadProjectFile(fn, true, true);
            setUntitledProject();
            }
      }

//---------------------------------------------------------
//   save
//---------------------------------------------------------

bool MusE::save()
      {
      if (project.baseName(true) == "untitled")
            return saveAs();
      else
            return save(project.filePath(), false);
      }

//---------------------------------------------------------
//   save
//---------------------------------------------------------

bool MusE::save(const QString& name, bool overwriteWarn)
      {
      QString backupCommand;

      // By T356. Cache the jack in/out route names BEFORE saving. 
      // Because jack often shuts down during save, causing the routes to be lost in the file.
      // Not required any more...
      //cacheJackRouteNames();
      
      if (QFile::exists(name)) {
            backupCommand.sprintf("cp \"%s\" \"%s.backup\"", name.latin1(), name.latin1());
            }
      else if (QFile::exists(name + QString(".med"))) {
            backupCommand.sprintf("cp \"%s.med\" \"%s.med.backup\"", name.latin1(), name.latin1());
            }
      if (!backupCommand.isEmpty())
            system(backupCommand.latin1());

      bool popenFlag;
      FILE* f = fileOpen(this, name, QString(".med"), "w", popenFlag, false, overwriteWarn);
      if (f == 0)
            return false;
      Xml xml(f);
      write(xml);
      if (ferror(f)) {
            QString s = "Write File\n" + name + "\nfailed: "
               //+ strerror(errno);
               + QString(strerror(errno));                 // p4.0.0
            QMessageBox::critical(this,
               tr("MusE: Write File failed"), s);
            popenFlag? pclose(f) : fclose(f);
            unlink(name.latin1());
            return false;
            }
      else {
            popenFlag? pclose(f) : fclose(f);
            song->dirty = false;
            return true;
            }
      }

//---------------------------------------------------------
//   quitDoc
//---------------------------------------------------------

void MusE::quitDoc()
      {
      close(true);
      }

//---------------------------------------------------------
//   closeEvent
//---------------------------------------------------------

void MusE::closeEvent(QCloseEvent*)
      {
      song->setStop(true);
      //
      // wait for sequencer
      //
      while (audio->isPlaying()) {
            qApp->processEvents();
            }
      if (song->dirty) {
            int n = 0;
            n = QMessageBox::warning(this, appName,
               tr("The current Project contains unsaved data\n"
               "Save Current Project?"),
               tr("&Save"), tr("&Skip"), tr("&Abort"), 0, 2);
            if (n == 0) {
                  if (!save())      // dont quit if save failed
                        return;
                  }
            else if (n == 2)
                  return;
            }
      seqStop();

      WaveTrackList* wt = song->waves();
      for (iWaveTrack iwt = wt->begin(); iwt != wt->end(); ++iwt) {
            WaveTrack* t = *iwt;
            if (t->recFile() && t->recFile()->samples() == 0) {
                  t->recFile()->remove();
                  }
            }

      // save "Open Recent" list
      QString prjPath(getenv("HOME"));
      prjPath += "/.musePrj";
      FILE* f = fopen(prjPath.latin1(), "w");
      if (f) {
            for (int i = 0; i < PROJECT_LIST_LEN; ++i) {
                  fprintf(f, "%s\n", projectList[i] ? projectList[i]->latin1() : "");
                  }
            fclose(f);
            }
      if(debugMsg)
        printf("Muse: Exiting JackAudio\n");
      exitJackAudio();
      if(debugMsg)
        printf("Muse: Exiting DummyAudio\n");
      exitDummyAudio();
      if(debugMsg)
        printf("Muse: Exiting Metronome\n");
      exitMetronome();
      
      // p3.3.47
      // Make sure to clear the menu, which deletes any sub menus.
      if(routingPopupMenu)
        routingPopupMenu->clear();
      
      // Changed by Tim. p3.3.14
      //SynthIList* sl = song->syntis();
      //for (iSynthI i = sl->begin(); i != sl->end(); ++i)
      //      delete *i;
      song->cleanupForQuit();

      if(debugMsg)
        printf("Muse: Cleaning up temporary wavefiles + peakfiles\n");
      // Cleanup temporary wavefiles + peakfiles used for undo
      for (std::list<QString>::iterator i = temporaryWavFiles.begin(); i != temporaryWavFiles.end(); i++) {
            QString filename = *i;
            QFileInfo f(filename);
            QDir d = f.dir();
            d.remove(filename);
            d.remove(f.baseName(true) + ".wca");
            }
      
      // Added by Tim. p3.3.14
      
#ifdef HAVE_LASH
      // Disconnect gracefully from LASH.
      if(lash_client)
      {
        if(debugMsg)
          printf("Muse: Disconnecting from LASH\n");
        lash_event_t* lashev = lash_event_new_with_type (LASH_Quit);
        lash_send_event(lash_client, lashev);
      }
#endif      
      
      if(debugMsg)
        printf("Muse: Exiting Dsp\n");
      AL::exitDsp();
      
      if(debugMsg)
        printf("Muse: Exiting OSC\n");
      exitOSC();
      
      // p3.3.47
      delete audioPrefetch;
      delete audio;
      delete midiSeq;
      delete song;
      
      qApp->quit();
      }

//---------------------------------------------------------
//   toggleMarker
//---------------------------------------------------------

void MusE::toggleMarker()
      {
      showMarker(!menuView->isItemChecked(mr_id));
      }

//---------------------------------------------------------
//   showMarker
//---------------------------------------------------------

void MusE::showMarker(bool flag)
      {
      //printf("showMarker %d\n",flag);
      if (markerView == 0) {
            markerView = new MarkerView(this);

            // Removed p3.3.43 
            // Song::addMarker() already emits a 'markerChanged'.
            //connect(arranger, SIGNAL(addMarker(int)), markerView, SLOT(addMarker(int)));
            
            connect(markerView, SIGNAL(closed()), SLOT(markerClosed()));
            toplevels.push_back(Toplevel(Toplevel::MARKER, (unsigned long)(markerView), markerView));
            markerView->show();
            }

      markerView->setShown(flag);
      menuView->setItemChecked(mr_id, flag);
      }

//---------------------------------------------------------
//   markerClosed
//---------------------------------------------------------

void MusE::markerClosed()
      {
      menuView->setItemChecked(mr_id, false);
      }

//---------------------------------------------------------
//   toggleTransport
//---------------------------------------------------------

void MusE::toggleTransport()
      {
      showTransport(!menuView->isItemChecked(tr_id));
      }

//---------------------------------------------------------
//   showTransport
//---------------------------------------------------------

void MusE::showTransport(bool flag)
      {
      transport->setShown(flag);
      menuView->setItemChecked(tr_id, flag);
      }

//---------------------------------------------------------
//   getRoutingPopupMenu
//---------------------------------------------------------

PopupMenu* MusE::getRoutingPopupMenu()
{
  if(!routingPopupMenu)
    routingPopupMenu = new PopupMenu(this);
  return routingPopupMenu;
}

//---------------------------------------------------------
//   updateRouteMenus
//---------------------------------------------------------

//void MusE::updateRouteMenus(Track* track)
void MusE::updateRouteMenus(Track* track, QObject* master)    // p3.3.50
{
      //if(!track || track != gRoutingPopupMenuMaster || track->type() == Track::AUDIO_AUX)
      //if(!track || track->type() == Track::AUDIO_AUX)
      if(!track || gRoutingPopupMenuMaster != master)  // p3.3.50
        return;
        
      //QPopupMenu* pup = muse->getORoutesPopup();
      PopupMenu* pup = getRoutingPopupMenu();
      
      if(pup->count() == 0)
        return;
        
      // p4.0.1 Protection since reverting to regular (self-extinguishing) menu behaviour here in muse2.
      if(!pup->isVisible())
      {
        //printf("MusE::updateRouteMenus menu is not visible\n");
        return;
      }
        
      //AudioTrack* t = (AudioTrack*)track;
      RouteList* rl = gIsOutRoutingPopupMenu ? track->outRoutes() : track->inRoutes();

      /*
      iRoute iorl = orl->begin();
      for(; iorl != orl->end(); ++iorl) 
      {
        iRouteMenuMap imm = ormm->begin();
        for(; imm != ormm->end(); ++imm) 
        {
          if(*iorl == imm->second)
          {
            orpup->setItemChecked(imm->first, true);
            break;
          }
        }
        //if(imm == ormm->end()) 
        //{
        //}
        
      }
      //if (iorl == orl->end()) 
      //{
      //}
      */     
           
      iRouteMenuMap imm = gRoutingMenuMap.begin();
      for(; imm != gRoutingMenuMap.end(); ++imm) 
      {
        // p3.3.50 Ignore the 'toggle' items.
        if(imm->second.type == Route::MIDI_PORT_ROUTE && 
           imm->first >= (MIDI_PORTS * MIDI_CHANNELS) && imm->first < (MIDI_PORTS * MIDI_CHANNELS + MIDI_PORTS))   
          continue;
          
        //bool found = false;
        iRoute irl = rl->begin();
        for(; irl != rl->end(); ++irl) 
        {
          if(imm->second.type == Route::MIDI_PORT_ROUTE)                                     // p3.3.50 Is the map route a midi port route?
          {
            if(irl->type == Route::MIDI_PORT_ROUTE && irl->midiPort == imm->second.midiPort  // Is the track route a midi port route?
               && (irl->channel & imm->second.channel) == imm->second.channel)               // Is the exact channel mask bit(s) set?
            {
              //found = true;
              break;
            }
          }
          else
          if(*irl == imm->second)
          {
            //found = true;
            break;
          }
        }
        //pup->setItemChecked(imm->first, found);
        pup->setItemChecked(imm->first, irl != rl->end());
      }
      
      
      return;
}      
      
//---------------------------------------------------------
//   routingPopupMenuActivated
//---------------------------------------------------------

void MusE::routingPopupMenuActivated(Track* track, int n)
{
      //if(!track || (track != gRoutingPopupMenuMaster))
      if(!track)
        return;
        
      if(track->isMidiTrack())
      {
        PopupMenu* pup = getRoutingPopupMenu();
        
        //printf("MusE::routingPopupMenuActivated midi n:%d count:%d\n", n, pup->count());
        
        if(pup->count() == 0)
          return;
          
        //MidiTrack* t = (MidiTrack*)track;
        RouteList* rl = gIsOutRoutingPopupMenu ? track->outRoutes() : track->inRoutes();
        
        if(n == -1) 
        {
          //printf("MusE::routingPopupMenuActivated midi n = -1\n");
          ///delete pup;
          ///pup = 0;
          return;
        }
        else
        {
          //int mdidx = n / MIDI_CHANNELS;
          //int ch = n % MIDI_CHANNELS;
          //int chbit = 1 << ch;              // p3.3.50
          //int chmask = 0;                   
          
          //if(n >= MIDI_PORTS * MIDI_CHANNELS)   // p3.3.50  Toggle channels.
          //{    
            //for (int i = 0; i < MIDI_CHANNELS; i++)
              //muse->routingPopupMenuActivated(selected, i + MIDI_CHANNELS * (n-1000));
              //muse->routingPopupMenuActivated(selected, i + MIDI_CHANNELS * (n - MIDI_PORTS * MIDI_CHANNELS));   // p3.3.50
          //  chbit = (1 << MIDI_CHANNELS) - 1;
          //}
          //if(debugMsg)
            //printf("MusE::routingPopupMenuActivated mdidx:%d ch:%d\n", mdidx, ch);
            
          // p3.3.50
          iRouteMenuMap imm = gRoutingMenuMap.find(n);
          if(imm == gRoutingMenuMap.end())
            return;
          if(imm->second.type != Route::MIDI_PORT_ROUTE)
            return;
          Route &aRoute = imm->second;
          int chbit = aRoute.channel;
          Route bRoute(track, chbit);
          int mdidx = aRoute.midiPort;

          MidiPort* mp = &midiPorts[mdidx];
          MidiDevice* md = mp->device();
          if(!md)
          {
            ///delete pup;
            return;
          }
          
          //if(!(md->rwFlags() & 2))
          if(!(md->rwFlags() & (gIsOutRoutingPopupMenu ? 1 : 2)))
          {
            ///delete pup;
            return;
          }  
          
          //QString s(pup->text(n));
          //QT_TR_NOOP(md->name())
          
          //Route srcRoute(s, false, -1);
          
          //Route aRoute(md, ch);
          //Route aRoute(mdidx, ch);      // p3.3.49
          //Route aRoute(mdidx, chbit);     // p3.3.50 In accordance with new channel mask, use the bit position.
          
          //Route srcRoute(md, -1);
          //Route dstRoute(track, -1);
          //Route bRoute(track, ch);
          //Route bRoute(track, chbit);     // p3.3.50 

          //if (track->type() == Track::AUDIO_INPUT)
          //      srcRoute.channel = dstRoute.channel = n & 0xf;
          
          int chmask = 0;                   
          iRoute iir = rl->begin();
          for (; iir != rl->end(); ++iir) 
          {
            //if(*iir == (dst ? bRoute : aRoute))
            //if(*iir == aRoute)
            if(iir->type == Route::MIDI_PORT_ROUTE && iir->midiPort == mdidx)    // p3.3.50 Is there already a route to this port?
            {
                  chmask = iir->channel;  // p3.3.50 Grab the channel mask.
                  break;
            }      
          }
          //if (iir != rl->end()) 
          if ((chmask & chbit) == chbit)             // p3.3.50 Is the channel's bit(s) set?
          {
            // disconnect
            if(gIsOutRoutingPopupMenu)
            {
              //printf("MusE::routingPopupMenuActivated removing route src track name: %s dst device name: %s\n", track->name().latin1(), md->name().latin1());
              audio->msgRemoveRoute(bRoute, aRoute);
            }
            else
            {
              //printf("MusE::routingPopupMenuActivated removing route src device name: %s dst track name: %s\n", md->name().latin1(), track->name().latin1());
              audio->msgRemoveRoute(aRoute, bRoute);
            }
          }
          else 
          {
            // connect
            if(gIsOutRoutingPopupMenu)
            {
              //printf("MusE::routingPopupMenuActivated adding route src track name: %s dst device name: %s\n", track->name().latin1(), md->name().latin1());
              audio->msgAddRoute(bRoute, aRoute);
            }
            else
            {
              //printf("MusE::routingPopupMenuActivated adding route src device name: %s dst track name: %s\n", md->name().latin1(), track->name().latin1());
              audio->msgAddRoute(aRoute, bRoute);
            }  
          }
          
          //printf("MusE::routingPopupMenuActivated calling msgUpdateSoloStates\n");
          audio->msgUpdateSoloStates();
          //printf("MusE::routingPopupMenuActivated calling song->update\n");
          song->update(SC_ROUTE);
        }
      }
      else
      {
        // TODO: Try to move code from AudioStrip::routingPopupMenuActivated into here.
        
        /*
        PopupMenu* pup = getRoutingPopupMenu();
        
        printf("MusE::routingPopupMenuActivated audio n:%d count:%d\n", n, pup->count());
        
        if(pup->count() == 0)
          return;
          
        AudioTrack* t = (AudioTrack*)track;
        RouteList* rl = gIsOutRoutingPopupMenu ? t->outRoutes() : t->inRoutes();
        
        //QPoint ppt = QCursor::pos();
        
        if(n == -1) 
        {
          //printf("MusE::routingPopupMenuActivated audio n = -1 deleting popup...\n");
          printf("MusE::routingPopupMenuActivated audio n = -1\n");
          ///delete pup;
          ///pup = 0;
          return;
        }
        else
        //if(n == 0)
        //{
          //printf("MusE::routingPopupMenuActivated audio n = 0 = tearOffHandle\n");
          //oR->setDown(false);     
        //  return;
        //}
        //else
        {
            if(gIsOutRoutingPopupMenu)
            {  
              QString s(pup->text(n));
              
              //printf("AudioStrip::routingPopupMenuActivated audio text:%s\n", s.latin1());
              
              if(track->type() == Track::AUDIO_OUTPUT)
              {
                ///delete orpup;
                
                int chan = n & 0xf;
                
                //Route srcRoute(t, -1);
                //Route srcRoute(t, chan, chans);
                //Route srcRoute(t, chan, 1);
                Route srcRoute(t, chan);
                
                //Route dstRoute(s, true, -1);
                Route dstRoute(s, true, -1, Route::JACK_ROUTE);
                //Route dstRoute(s, true, 0, Route::JACK_ROUTE);
    
                //srcRoute.channel = dstRoute.channel = chan;
                dstRoute.channel = chan;
                //dstRoute.channels = 1;
    
                // check if route src->dst exists:
                iRoute irl = rl->begin();
                for (; irl != rl->end(); ++irl) {
                      if (*irl == dstRoute)
                            break;
                      }
                if (irl != rl->end()) {
                      // disconnect if route exists
                      audio->msgRemoveRoute(srcRoute, dstRoute);
                      }
                else {
                      // connect if route does not exist
                      audio->msgAddRoute(srcRoute, dstRoute);
                      }
                audio->msgUpdateSoloStates();
                song->update(SC_ROUTE);
                
                // p3.3.47
                //pup->popup(ppt, 0);
                
                //oR->setDown(false);   
                return;
                
                // p3.3.46
                ///goto _redisplay;
              }
              
              iRouteMenuMap imm = gRoutingMenuMap.find(n);
              if(imm == gRoutingMenuMap.end())
              {  
                ///delete orpup;
                //oR->setDown(false);     // orpup->exec() catches mouse release event
                return;
              }  
              
              //int chan = n >> 16;
              //int chans = (chan >> 15) + 1; // Bit 31 MSB: Mono or stereo.
              //chan &= 0xffff;
              //int chan = imm->second.channel;
              //int chans = imm->second.channels; 
              
              //Route srcRoute(t, -1);
              //srcRoute.remoteChannel = chan;
              //Route srcRoute(t, chan, chans);
              Route srcRoute(t, imm->second.channel, imm->second.channels);
              //Route srcRoute(t, imm->second.channel);
              srcRoute.remoteChannel = imm->second.remoteChannel;
              
              //Route dstRoute(s, true, -1);
              //Route dstRoute(s, true, -1, Route::TRACK_ROUTE);
              Route &dstRoute = imm->second;
  
              // check if route src->dst exists:
              iRoute irl = rl->begin();
              for (; irl != rl->end(); ++irl) {
                    if (*irl == dstRoute)
                          break;
                    }
              if (irl != rl->end()) {
                    // disconnect if route exists
                    audio->msgRemoveRoute(srcRoute, dstRoute);
                    }
              else {
                    // connect if route does not exist
                    audio->msgAddRoute(srcRoute, dstRoute);
                    }
              audio->msgUpdateSoloStates();
              song->update(SC_ROUTE);
                
              // p3.3.46
              //oR->setDown(false);     
              ///goto _redisplay;
              
              // p3.3.47
              //pup->popup(ppt, 0);
            }  
            else
            {
              QString s(pup->text(n));
              
              if(track->type() == Track::AUDIO_INPUT)
              {
                ///delete pup;
                int chan = n & 0xf;
                
                Route srcRoute(s, false, -1, Route::JACK_ROUTE);
                Route dstRoute(t, chan);
                
                srcRoute.channel = chan;
                
                iRoute irl = rl->begin();
                for(; irl != rl->end(); ++irl) 
                {
                  if(*irl == srcRoute)
                    break;
                }
                if(irl != rl->end()) 
                  // disconnect
                  audio->msgRemoveRoute(srcRoute, dstRoute);
                else 
                  // connect
                  audio->msgAddRoute(srcRoute, dstRoute);
                
                audio->msgUpdateSoloStates();
                song->update(SC_ROUTE);
                //iR->setDown(false);     // pup->exec() catches mouse release event
                return;
                
                // p3.3.46
                ///goto _redisplay;
              }
              
              iRouteMenuMap imm = gRoutingMenuMap.find(n);
              if(imm == gRoutingMenuMap.end())
              {  
                //delete pup;
                //iR->setDown(false);     // pup->exec() catches mouse release event
                return;
              }  
              
              //int chan = n >> 16;
              //int chans = (chan >> 15) + 1; // Bit 31 MSB: Mono or stereo.
              //chan &= 0xffff;
              //int chan = imm->second.channel;
              //int chans = imm->second.channels; 
              
              //Route srcRoute(s, false, -1);
              //Route srcRoute(s, false, -1, Route::TRACK_ROUTE);
              Route &srcRoute = imm->second;
              
              //Route dstRoute(t, -1);
              //Route dstRoute(t, chan, chans);
              Route dstRoute(t, imm->second.channel, imm->second.channels);
              //Route dstRoute(t, imm->second.channel);
              dstRoute.remoteChannel = imm->second.remoteChannel;
  
              iRoute irl = rl->begin();
              for (; irl != rl->end(); ++irl) {
                    if (*irl == srcRoute)
                          break;
                    }
              if (irl != rl->end()) {
                    // disconnect
                    audio->msgRemoveRoute(srcRoute, dstRoute);
                    }
              else {
                    // connect
                    audio->msgAddRoute(srcRoute, dstRoute);
                    }
              audio->msgUpdateSoloStates();
              song->update(SC_ROUTE);
              
              // p3.3.46
              //iR->setDown(false);     
              ///goto _redisplay;
              
              
              
              
            }
                
        }
       */
       
      }
      //else
      //{
      //}
            
      ///delete pup;
      //oR->setDown(false);     
}

//---------------------------------------------------------
//   routingPopupMenuAboutToHide
//---------------------------------------------------------

void MusE::routingPopupMenuAboutToHide()
{
      // p3.3.47
      //printf("MusE::routingPopupMenuAboutToHide\n");
      //if(track)
      //  printf("%s", track->name().latin1());
      //printf("\n");
      
      // Hmm, can't do this? Sub-menus stay open with this. Re-arranged, testing... Nope.
      //PopupMenu* pup = muse->getRoutingPopupMenu();
      //pup->disconnect();
      //pup->clear();
      
      // p4.0.1 Removed. IIRC These lines were not strictly necessary in muse-1, 
      //  and here in muse-2 we reverted back to regular Q3PopupMenu behaviour for now, 
      //  which is self-extinguishing, so these lines cannot be enabled - 
      //  gRoutingPopupMenuMaster and gRoutingMenuMap are required for routingPopupMenuActivated().
      //gRoutingMenuMap.clear();
      //gRoutingPopupMenuMaster = 0;
}

//---------------------------------------------------------
//   prepareRoutingPopupMenu
//---------------------------------------------------------

PopupMenu* MusE::prepareRoutingPopupMenu(Track* track, bool dst)
{
  if(!track)
    return 0;
    
  //QPoint ppt = QCursor::pos();
  
  if(track->isMidiTrack())
  {
  
    //QPoint ppt = parent->rect().bottomLeft();
      
    //if(dst)
    //{
      // TODO 
      
    //}
    //else
    //{
      RouteList* rl = dst ? track->outRoutes() : track->inRoutes();
      //Route dst(track, -1);
    
      ///QPopupMenu* pup = new QPopupMenu(parent);
      
      PopupMenu* pup = getRoutingPopupMenu();
      pup->disconnect();
      //connect(pup, SIGNAL(activated(int)), SLOT(routingPopupMenuActivated(int)));
      //connect(pup, SIGNAL(aboutToHide()), SLOT(routingPopupMenuAboutToHide()));
        
      pup->setCheckable(true);
      
      int gid = 0;
      //int n;    
      
    // Routes can't be re-read until the message sent from msgAddRoute1() 
    //  has had time to be sent and actually affected the routes.
    ///_redisplay:
      
      pup->clear();
      gRoutingMenuMap.clear();
      gid = 0;
      
      //MidiInPortList* tl = song->midiInPorts();
      //for(iMidiInPort i = tl->begin();i != tl->end(); ++i) 
      for(int i = 0; i < MIDI_PORTS; ++i)
      {
        //MidiInPort* track = *i;
        // NOTE: Could possibly list all devices, bypassing ports, but no, let's stick with ports.
        MidiPort* mp = &midiPorts[i];
        MidiDevice* md = mp->device();
        if(!md)
          continue;
        
        if(!(md->rwFlags() & (dst ? 1 : 2)))
          continue;
          
        //printf("MusE::prepareRoutingPopupMenu adding submenu portnum:%d\n", i);
        
        //QMenu* m = menu->addMenu(track->name());
        //QPopupMenu* subp = new QPopupMenu(parent);
        //PopupMenu* subp = new PopupMenu(this);
        PopupMenu* subp = new PopupMenu();
        connect(subp, SIGNAL(activated(int)), pup, SIGNAL(activated(int)));
        //connect(subp, SIGNAL(aboutToHide()), pup, SIGNAL(aboutToHide()));
        
        int chanmask = 0;
        // p3.3.50 To reduce number of routes required, from one per channel to just one containing a channel mask. 
        // Look for the first route to this midi port. There should always be only a single route for each midi port, now.
        for(iRoute ir = rl->begin(); ir != rl->end(); ++ir)   
        {
          if(ir->type == Route::MIDI_PORT_ROUTE && ir->midiPort == i) 
          {
            // We have a route to the midi port. Grab the channel mask.
            chanmask = ir->channel;
            break;
          }
        }
        
        for(int ch = 0; ch < MIDI_CHANNELS; ++ch) 
        {
          //QAction* a = m->addAction(QString("Channel %1").arg(ch+1));
          //subp->insertItem(QT_TR_NOOP(QString("Channel %1").arg(ch+1)), i * MIDI_CHANNELS + ch);
          gid = i * MIDI_CHANNELS + ch;
          
          //printf("MusE::prepareRoutingPopupMenu inserting gid:%d\n", gid);
          
          subp->insertItem(QString("Channel %1").arg(ch+1), gid);
          //a->setCheckable(true);
          //Route src(track, ch, RouteNode::TRACK);
          //Route src(md, ch);
          //Route r = Route(src, dst);
          //a->setData(QVariant::fromValue(r));
          //a->setChecked(rl->indexOf(r) != -1);
          
          //Route srcRoute(md, ch);
          //Route srcRoute(i, ch);     // p3.3.49 New: Midi port route.
          int chbit = 1 << ch;
          Route srcRoute(i, chbit);    // p3.3.50 In accordance with new channel mask, use the bit position.
          
          gRoutingMenuMap.insert( pRouteMenuMap(gid, srcRoute) );
          
          //for(iRoute ir = rl->begin(); ir != rl->end(); ++ir)   // p3.3.50 Removed.
          //{
            //if(*ir == dst) 
          //  if(*ir == srcRoute) 
          //  {
          //    subp->setItemChecked(id, true);
          //    break;
          //  }
          //}
          if(chanmask & chbit)                  // p3.3.50 Is the channel already set? Show item check mark.
            subp->setItemChecked(gid, true);
        }
        //subp->insertItem(QString("Toggle all"), 1000+i);
        // p3.3.50 One route with all channel bits set.
        gid = MIDI_PORTS * MIDI_CHANNELS + i;           // Make sure each 'toggle' item gets a unique id.
        subp->insertItem(QString("Toggle all"), gid);      
        Route togRoute(i, (1 << MIDI_CHANNELS) - 1);    // Set all channel bits.
        gRoutingMenuMap.insert( pRouteMenuMap(gid, togRoute) );
        
        pup->insertItem(QT_TR_NOOP(md->name()), subp);
      }
          
      /*
      QPopupMenu* pup = new QPopupMenu(iR);
      pup->setCheckable(true);
      //MidiTrack* t = (MidiTrack*)track;
      RouteList* irl = track->inRoutes();
  
      MidiTrack* t = (MidiTrack*)track;
      int gid = 0;
      for (int i = 0; i < channel; ++i) 
      {
            char buffer[128];
            snprintf(buffer, 128, "%s %d", tr("Channel").latin1(), i+1);
            MenuTitleItem* titel = new MenuTitleItem(QString(buffer));
            pup->insertItem(titel);
  
            if (!checkAudioDevice()) return;
            std::list<QString> ol = audioDevice->outputPorts();
            for (std::list<QString>::iterator ip = ol.begin(); ip != ol.end(); ++ip) {
                  int id = pup->insertItem(*ip, (gid * 16) + i);
                  Route dst(*ip, true, i);
                  ++gid;
                  for (iRoute ir = irl->begin(); ir != irl->end(); ++ir) {
                        if (*ir == dst) {
                              pup->setItemChecked(id, true);
                              break;
                              }
                        }
                  }
            if (i+1 != channel)
                  pup->insertSeparator();
      }
      */
      
      if(pup->count() == 0)
      {
        ///delete pup;
        gRoutingPopupMenuMaster = 0;
        //pup->clear();
        //pup->disconnect();
        gRoutingMenuMap.clear();
        //oR->setDown(false);     
        return 0;
      }
      
      gIsOutRoutingPopupMenu = dst;
      return pup;
    }
    
    return 0;
}

//---------------------------------------------------------
//   saveAs
//---------------------------------------------------------

bool MusE::saveAs()
      {
//      QString name = getSaveFileName(museProject, med_file_pattern, this,
//      QString name = getSaveFileName(QString(""), med_file_pattern, this,
      QString name = getSaveFileName(QString(""), med_file_save_pattern, this,
         tr("MusE: Save As"));
      bool ok = false;
      if (!name.isEmpty()) {
            QString tempOldProj = museProject;
            museProject = QFileInfo(name).dirPath(true);
            ok = save(name, true);
            if (ok) {
                  project.setFile(name);
                  setCaption(tr("MusE: Song: ") + project.baseName(true));
                  addProject(name);
                  }
            else
                  museProject = tempOldProj;
            }

      return ok;
      }

//---------------------------------------------------------
//   printVersion
//---------------------------------------------------------

static void printVersion(const char* prog)
      {
      fprintf(stderr, "%s: Linux Music Editor; Version %s, (svn revision %s)\n", prog, VERSION, SVNVERSION);
      }

//---------------------------------------------------------
//   startEditor
//---------------------------------------------------------

void MusE::startEditor(PartList* pl, int type)
      {
      switch (type) {
            case 0: startPianoroll(pl); break;
            case 1: startListEditor(pl); break;
            case 3: startDrumEditor(pl); break;
            case 4: startWaveEditor(pl); break;
            }
      }

//---------------------------------------------------------
//   startEditor
//---------------------------------------------------------

void MusE::startEditor(Track* t)
      {
      switch (t->type()) {
            case Track::MIDI: startPianoroll(); break;
            case Track::DRUM: startDrumEditor(); break;
            case Track::WAVE: startWaveEditor(); break;
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
      
      PianoRoll* pianoroll = new PianoRoll(pl, this, 0, arranger->cursorValue());
      pianoroll->show();
      toplevels.push_back(Toplevel(Toplevel::PIANO_ROLL, (unsigned long)(pianoroll), pianoroll));
      connect(pianoroll, SIGNAL(deleted(unsigned long)), SLOT(toplevelDeleted(unsigned long)));
      connect(muse, SIGNAL(configChanged()), pianoroll, SLOT(configChanged()));
      }

//---------------------------------------------------------
//   startListenEditor
//---------------------------------------------------------

void MusE::startListEditor()
      {
      PartList* pl = getMidiPartsToEdit();
      if (pl == 0)
            return;
      startListEditor(pl);
      }

void MusE::startListEditor(PartList* pl)
      {
      ListEdit* listEditor = new ListEdit(pl);
      listEditor->show();
      toplevels.push_back(Toplevel(Toplevel::LISTE, (unsigned long)(listEditor), listEditor));
      connect(listEditor, SIGNAL(deleted(unsigned long)), SLOT(toplevelDeleted(unsigned long)));
      connect(muse,SIGNAL(configChanged()), listEditor, SLOT(configChanged()));
      }

//---------------------------------------------------------
//   startMasterEditor
//---------------------------------------------------------

void MusE::startMasterEditor()
      {
      MasterEdit* masterEditor = new MasterEdit();
      masterEditor->show();
      toplevels.push_back(Toplevel(Toplevel::MASTER, (unsigned long)(masterEditor), masterEditor));
      connect(masterEditor, SIGNAL(deleted(unsigned long)), SLOT(toplevelDeleted(unsigned long)));
      }

//---------------------------------------------------------
//   startLMasterEditor
//---------------------------------------------------------

void MusE::startLMasterEditor()
      {
      LMaster* lmaster = new LMaster();
      lmaster->show();
      toplevels.push_back(Toplevel(Toplevel::LMASTER, (unsigned long)(lmaster), lmaster));
      connect(lmaster, SIGNAL(deleted(unsigned long)), SLOT(toplevelDeleted(unsigned long)));
      connect(muse, SIGNAL(configChanged()), lmaster, SLOT(configChanged()));
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
      
      DrumEdit* drumEditor = new DrumEdit(pl, this, 0, arranger->cursorValue());
      drumEditor->show();
      toplevels.push_back(Toplevel(Toplevel::DRUM, (unsigned long)(drumEditor), drumEditor));
      connect(drumEditor, SIGNAL(deleted(unsigned long)), SLOT(toplevelDeleted(unsigned long)));
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
            return;
            }
      startWaveEditor(pl);
      }

void MusE::startWaveEditor(PartList* pl)
      {
      WaveEdit* waveEditor = new WaveEdit(pl);
      waveEditor->show();
      connect(muse, SIGNAL(configChanged()), waveEditor, SLOT(configChanged()));
      toplevels.push_back(Toplevel(Toplevel::WAVE, (unsigned long)(waveEditor), waveEditor));
      connect(waveEditor, SIGNAL(deleted(unsigned long)), SLOT(toplevelDeleted(unsigned long)));
      }


//---------------------------------------------------------
//   startSongInfo
//---------------------------------------------------------
void MusE::startSongInfo(bool editable)
      {
  printf("startSongInfo!!!!\n");
        SongInfo info;
        info.songInfoText->setText(song->getSongInfo());
        info.songInfoText->setReadOnly(!editable);
        info.show();
        if( info.exec() == QDialog::Accepted) {
          if (editable)
            song->setSongInfo(info.songInfoText->text());
        }

      }

//---------------------------------------------------------
//   showDidYouKnowDialog
//---------------------------------------------------------
void MusE::showDidYouKnowDialog()
      {
      if ((bool)config.showDidYouKnow == true) {
            printf("show did you know dialog!!!!\n");
            DidYouKnow dyk;
            dyk.tipText->setText("To get started with MusE why don't you try some demo songs available at http://demos.muse-sequencer.org/");
            dyk.show();
            if( dyk.exec()) {
                  if (dyk.dontShowCheckBox->isChecked()) {
                        printf("disables dialog!\n");
                        config.showDidYouKnow=false;
                        muse->changeConfig(true);    // save settings
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

void MusE::startClipList()
      {
      if (clipListEdit == 0) {
            clipListEdit = new ClipListEdit();
            toplevels.push_back(Toplevel(Toplevel::CLIPLIST, (unsigned long)(clipListEdit), clipListEdit));
            connect(clipListEdit, SIGNAL(deleted(unsigned long)), SLOT(toplevelDeleted(unsigned long)));
            }
      clipListEdit->show();
      menu_audio->setItemChecked(aid2, true);
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
            const char* path = projectList[i]->latin1();
            const char* p = strrchr(path, '/');
            if (p == 0)
                  p = path;
            else
                  ++p;
            openRecent->insertItem(QString(p), i);
            }
      }

//---------------------------------------------------------
//   selectProject
//---------------------------------------------------------

void MusE::selectProject(int id)
      {
      if (id < 0)
            return;
      assert(id < PROJECT_LIST_LEN);
      QString* name = projectList[id];
      if (name == 0)
            return;
      loadProjectFile(*name, false, true);
      }

//---------------------------------------------------------
//   toplevelDeleted
//---------------------------------------------------------

void MusE::toplevelDeleted(unsigned long tl)
      {
      for (iToplevel i = toplevels.begin(); i != toplevels.end(); ++i) {
            if (i->object() == tl) {
                  switch(i->type()) {
                        case Toplevel::MARKER:
                              break;
                        case Toplevel::CLIPLIST:
                              menu_audio->setItemChecked(aid2, false);
                              return;
                        // the followin editors can exist in more than
                        // one instantiation:
                        case Toplevel::PIANO_ROLL:
                        case Toplevel::LISTE:
                        case Toplevel::DRUM:
                        case Toplevel::MASTER:
                        case Toplevel::WAVE:
                        case Toplevel::LMASTER:
                              break;
                        }
                  toplevels.erase(i);
                  return;
                  }
            }
      printf("topLevelDeleted: top level %lx not found\n", tl);
      //assert(false);
      }

//---------------------------------------------------------
//   ctrlChanged
//    midi ctrl value changed
//---------------------------------------------------------

#if 0
void MusE::ctrlChanged()
      {
      arranger->updateInspector();
      }
#endif

//---------------------------------------------------------
//   kbAccel
//---------------------------------------------------------

void MusE::kbAccel(int key)
      {
      if (key == shortcuts[SHRT_TOGGLE_METRO].key) {
            song->setClick(!song->click());
            }
      else if (key == shortcuts[SHRT_PLAY_TOGGLE].key) {
            if (audio->isPlaying())
                  //song->setStopPlay(false);
                  song->setStop(true);
            else if (!config.useOldStyleStopShortCut)
                  song->setPlay(true);
            else if (song->cpos() != song->lpos())
                  song->setPos(0, song->lPos());
            else {
                  Pos p(0, true);
                  song->setPos(0, p);
                  }
            }
      else if (key == shortcuts[SHRT_STOP].key) {
            //song->setPlay(false);
            song->setStop(true);
            }
      else if (key == shortcuts[SHRT_GOTO_START].key) {
            Pos p(0, true);
            song->setPos(0, p);
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
      else if (key == shortcuts[SHRT_REC_CLEAR].key) {
            if (!audio->isPlaying()) {
                  song->clearTrackRec();
                  }
            }
      else if (key == shortcuts[SHRT_OPEN_TRANSPORT].key) {
            toggleTransport();
            }
      else if (key == shortcuts[SHRT_OPEN_BIGTIME].key) {
            toggleBigTime();
            }
      //else if (key == shortcuts[SHRT_OPEN_MIXER].key) {
      //      toggleMixer();
      //      }
      else if (key == shortcuts[SHRT_OPEN_MIXER].key) {
            toggleMixer1();
            }
      else if (key == shortcuts[SHRT_OPEN_MIXER2].key) {
            toggleMixer2();
            }
      else if (key == shortcuts[SHRT_NEXT_MARKER].key) {
            if (markerView)
              markerView->nextMarker();
            }
      else if (key == shortcuts[SHRT_PREV_MARKER].key) {
            if (markerView)
              markerView->prevMarker();
            }
      else {
            if (debugMsg)
                  printf("unknown kbAccel 0x%x\n", key);
            }
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
#ifdef HAVE_LASH
            if(useLASH)
              startTimer (300);
#endif
            }

      bool notify(QObject* receiver, QEvent* event) {
            bool flag = QApplication::notify(receiver, event);
            if (event->type() == QEvent::KeyPress) {
                  QKeyEvent* ke = (QKeyEvent*)event;
                  globalKeyState = ke->stateAfter();
                  bool accepted = ke->isAccepted();
                  if (!accepted) {
                        int key = ke->key();
                        if (ke->state() & Qt::ShiftModifier)
                              key += Qt::SHIFT;
                        if (ke->state() & Qt::AltModifier)
                              key += Qt::ALT;
                        if (ke->state() & Qt::ControlModifier)
                              key+= Qt::CTRL;
                        muse->kbAccel(key);
                        return true;
                        }
                  }
            if (event->type() == QEvent::KeyRelease) {
                  QKeyEvent* ke = (QKeyEvent*)event;
                  globalKeyState = ke->stateAfter();
                  }

            return flag;
            }

#ifdef HAVE_LASH
     virtual void timerEvent (QTimerEvent * /* e */) {
            if(useLASH)
              muse->lash_idle_cb ();
            }
#endif /* HAVE_LASH */

      };

//---------------------------------------------------------
//   usage
//---------------------------------------------------------

static void usage(const char* prog, const char* txt)
      {
     fprintf(stderr, "%s: %s\nusage: %s flags midifile\n   Flags:\n",
         prog, txt, prog);
      fprintf(stderr, "   -h       this help\n");
      fprintf(stderr, "   -v       print version\n");
      fprintf(stderr, "   -d       debug mode: no threads, no RT\n");
      fprintf(stderr, "   -D       debug mode: enable some debug messages\n");
      fprintf(stderr, "   -m       debug mode: trace midi Input\n");
      fprintf(stderr, "   -M       debug mode: trace midi Output\n");
      fprintf(stderr, "   -s       debug mode: trace sync\n");
      fprintf(stderr, "   -a       no audio\n");
      //fprintf(stderr, "   -P  n    set real time priority to n (default: 50)\n");
      fprintf(stderr, "   -P  n    set audio driver real time priority to n (Dummy only, default 40. Else fixed by Jack.)\n");
      fprintf(stderr, "   -Y  n    force midi real time priority to n (default: audio driver prio +2)\n");
      fprintf(stderr, "   -p       don't load LADSPA plugins\n");
#ifdef ENABLE_PYTHON
      fprintf(stderr, "   -y       enable Python control support\n");
#endif
#ifdef VST_SUPPORT
      fprintf(stderr, "   -V       don't load VST plugins\n");
#endif
#ifdef DSSI_SUPPORT
      fprintf(stderr, "   -I       don't load DSSI plugins\n");
#endif
#ifdef HAVE_LASH
      fprintf(stderr, "   -L       don't use LASH\n");
#endif
      fprintf(stderr, "useful environment variables:\n");
      fprintf(stderr, "   MUSE             override library and shared directories location\n");
      fprintf(stderr, "   MUSEHOME         override user home directory (HOME/)\n");
      fprintf(stderr, "   MUSEINSTRUMENTS  override user instrument directory (MUSEHOME/muse_instruments)\n");
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
//   main
//---------------------------------------------------------

int main(int argc, char* argv[])
      {
      
//      error = ErrorHandler::create(argv[0]);
      ruid = getuid();
      euid = geteuid();
      undoSetuid();
      getCapabilities();
      int noAudio = false;

      const char* mu = getenv("MUSEHOME");
      if(mu)
        museUser = QString(mu);
      if(museUser.isEmpty())
        museUser = QString(getenv("HOME"));
            
      QString museGlobal;
      const char* p = getenv("MUSE");
      if (p)
            museGlobal = p;

      if (museGlobal.isEmpty()) {
            QString museGlobal(INSTPREFIX);
            QString museGlobalLibDir(INSTLIBDIR);
            
            //museGlobalLib   =  museGlobalLibDir + "/muse";
            //museGlobalShare =  museGlobal + "/share/muse";
            museGlobalLib   =  museGlobalLibDir + QString("/") + QString(INSTALL_NAME);   // p4.0.2
            museGlobalShare =  museGlobal + QString("/share/") + QString(INSTALL_NAME);   //
            }
      else {
            museGlobalLib   = museGlobal + "/lib";
            museGlobalShare = museGlobal + "/share";
            }
      museProject = museProjectInitPath; //getcwd(0, 0);
      configName  = QString(getenv("HOME")) + QString("/.MusE");

      museInstruments = museGlobalShare + QString("/instruments");
      
      const char* ins = getenv("MUSEINSTRUMENTS");
      if(ins)
        museUserInstruments = QString(ins);
      if(museUserInstruments.isEmpty())
        museUserInstruments = museUser + QString("/muse_instruments");

#ifdef HAVE_LASH
      lash_args_t * lash_args = 0;
      if(useLASH)
        lash_args = lash_extract_args (&argc, &argv);
#endif

      srand(time(0));   // initialize random number generator
//      signal(SIGCHLD, catchSignal);  // interferes with initVST()
      initMidiController();
      QApplication::setColorSpec(QApplication::ManyColor);
      MuseApplication app(argc, argv);

      initShortCuts();
      readConfiguration();

      if (config.useDenormalBias)
          printf("Denormal protection enabled.\n");
      // SHOW MUSE SPLASH SCREEN
      if (config.showSplashScreen) {
            QPixmap splsh(museGlobalShare + "/splash.png");

            if (!splsh.isNull()) {
                  QSplashScreen* muse_splash = new QSplashScreen(splsh,
                     Qt::WStyle_StaysOnTop | Qt::WDestructiveClose);
                  muse_splash->show();
                  QTimer* stimer = new QTimer(0);
                  muse_splash->connect(stimer, SIGNAL(timeout()), muse_splash, SLOT(close()));
                  stimer->start(6000, true);
                  }
            }
      int i;
      
      QString optstr("ahvdDmMsP:Y:py");
#ifdef VST_SUPPORT
      optstr += QString("V");
#endif
#ifdef DSSI_SUPPORT
      optstr += QString("I");
#endif
#ifdef HAVE_LASH
      optstr += QString("L");
#endif

//#ifdef VST_SUPPORT
//      while ((i = getopt(argc, argv, "ahvdDmMsVP:py")) != EOF) {
//#else
//      while ((i = getopt(argc, argv, "ahvdDmMsP:py")) != EOF) {
//#endif
      
      while ((i = getopt(argc, argv, optstr.latin1())) != EOF) {
      char c = (char)i;
            switch (c) {
                  case 'v': printVersion(argv[0]); return 0;
                  case 'd':
                        debugMode = true;
                        realTimeScheduling = false;
                        break;
                  case 'a':
                        noAudio = true;
                        break;
                  case 'D': debugMsg = true; break;
                  case 'm': midiInputTrace = true; break;
                  case 'M': midiOutputTrace = true; break;
                  case 's': debugSync = true; break;
                  case 'P': realTimePriority = atoi(optarg); break;
                  case 'Y': midiRTPrioOverride = atoi(optarg); break;
                  case 'p': loadPlugins = false; break;
                  case 'V': loadVST = false; break;
                  case 'I': loadDSSI = false; break;
                  case 'L': useLASH = false; break;
                  case 'y': usePythonBridge = true; break;
                  case 'h': usage(argv[0], argv[1]); return -1;
                  default:  usage(argv[0], "bad argument"); return -1;
                  }
            }
      
      AL::initDsp();
      
      if (debugMsg)
            printf("Start euid: %d ruid: %d, Now euid %d\n",
               euid, ruid, geteuid());
      if (debugMode) {
            initDummyAudio();
            realTimeScheduling = false;
            }
      else if (noAudio) {
            initDummyAudio();
            realTimeScheduling = true;
            //if (debugMode) {              // ??
            //          realTimeScheduling = false;
            //          }
            }
      else if (initJackAudio()) {
            if (!debugMode)
                  {
                  QMessageBox::critical(NULL, "MusE fatal error", "MusE <b>failed</b> to find a <b>Jack audio server</b>.<br><br>"
                                                                  "<i>MusE will continue without audio support (-a switch)!</i><br><br>"
                                                                  "If this was not intended check that Jack was started. "
                                                                  "If Jack <i>was</i> started check that it was\n"
                                                                  "started as the same user as MusE.\n");

                  initDummyAudio();
                  noAudio = true;
                  realTimeScheduling = true;
                  if (debugMode) {
                            realTimeScheduling = false;
                            }
                  }
            else
                  {
                  fprintf(stderr, "fatal error: no JACK audio server found\n");
                  fprintf(stderr, "no audio functions available\n");
                  fprintf(stderr, "*** experimental mode -- no play possible ***\n");
                  initDummyAudio();
                  //realTimeScheduling = audioDevice->isRealtime();
                  }
            realTimeScheduling = true;
            }
      else
            realTimeScheduling = audioDevice->isRealtime();

      useJackTransport.setValue(true);
      // setup the prefetch fifo length now that the segmentSize is known
      // Changed by Tim. p3.3.17
      // Changed to 4 *, JUST FOR TEST!!!
      fifoLength = 131072/segmentSize;
      //fifoLength = (131072/segmentSize) * 4;
      
      
      argc -= optind;
      ++argc;

      if (debugMsg) {
            printf("global lib:       <%s>\n", museGlobalLib.latin1());
            printf("global share:     <%s>\n", museGlobalShare.latin1());
            printf("muse home:        <%s>\n", museUser.latin1());
            printf("project dir:      <%s>\n", museProject.latin1());
            printf("user instruments: <%s>\n", museUserInstruments.latin1());
            }

      static QTranslator translator(0);
      QString locale(QTextCodec::locale());
      if (locale != "C") {
            QString loc("muse_");
            loc += QString(QTextCodec::locale());
            if (translator.load(loc, QString(".")) == false) {
                  QString lp(museGlobalShare);
                  lp += QString("/locale");
                  if (translator.load(loc, lp) == false) {
                        printf("no locale <%s>/<%s>\n", loc.latin1(), lp.latin1());
                        }
                  }
            app.installTranslator(&translator);
            }

      if (locale == "de") {
            printf("locale de\n");
            hIsB = false;
            }

      if (loadPlugins)
            initPlugins();

      if (loadVST)
            initVST();

      if(loadDSSI)
        initDSSI();
      
      // p3.3.39
      initOSC();
      
      initIcons();

      initMetronome();
      //QApplication::clipboard()->setSelectionMode(false); ddskrjo

      QApplication::addLibraryPath(museGlobalLib + "/qtplugins");
      if (debugMsg) {
            QStringList list = app.libraryPaths();
            QStringList::Iterator it = list.begin();
            printf("QtLibraryPath:\n");
            while(it != list.end()) {
                  printf("  <%s>\n", (*it).latin1());
                  ++it;
                  }
            }

      muse = new MusE(argc, &argv[optind]);
      app.setMuse(muse);
      muse->setIcon(*museIcon);
      // Added by Tim. p3.3.22
      if (!debugMode) {
            if (mlockall(MCL_CURRENT | MCL_FUTURE))
                  perror("WARNING: Cannot lock memory:");
            }
      
      muse->show();
      muse->seqStart();

#ifdef HAVE_LASH
      {
        if(useLASH)
        {
          int lash_flags = LASH_Config_File;
          const char *muse_name = PACKAGE_NAME;
          lash_client = lash_init (lash_args, muse_name, lash_flags, LASH_PROTOCOL(2,0));
          lash_alsa_client_id (lash_client, snd_seq_client_id (alsaSeq));
          if (!noAudio) {
                // p3.3.38
                //char *jack_name = ((JackAudioDevice*)audioDevice)->getJackName();
                const char *jack_name = audioDevice->clientName();
                lash_jack_client_name (lash_client, jack_name);
          }      
        }
      }
#endif /* HAVE_LASH */
      QTimer::singleShot(100, muse, SLOT(showDidYouKnowDialog()));
      
      return app.exec();
      // p3.3.47 
      //int rv = app.exec();
      // FIXME: Can't do, seg fault at MarkerView::~MarkerView() 
      //  due to already deleted undoRedo.
      //delete muse; 
      //return rv;
      
      }

#if 0
//---------------------------------------------------------
//   configPart
//---------------------------------------------------------

void MusE::configPart(int id)
      {
      if (id < 3) {
            partConfig->setItemChecked(0, id == 0);
            partConfig->setItemChecked(1, id == 1);
            partConfig->setItemChecked(2, id == 2);
            arranger->setShowPartType(id);
            for (int i = 3; i < 10; ++i) {
                  partConfig->setItemEnabled(i, id == 2);
                  }
            }
      else {
            bool flag = !partConfig->isItemChecked(id);
            partConfig->setItemChecked(id, flag);
            int val = arranger->showPartEvent();
            if (flag) {
                  val |= 1 << (id-3);
                  }
            else {
                  val &= ~(1 << (id-3));
                  }
            arranger->setShowPartEvent(val);
            }
      }
#endif

//---------------------------------------------------------
//   cmd
//    some cmd's from pulldown menu
//---------------------------------------------------------

void MusE::cmd(int cmd)
      {
      TrackList* tracks = song->tracks();
      int l = song->lpos();
      int r = song->rpos();

      switch(cmd) {
            case CMD_CUT:
                  arranger->cmd(Arranger::CMD_CUT_PART);
                  break;
            case CMD_COPY:
                  arranger->cmd(Arranger::CMD_COPY_PART);
                  break;
            case CMD_PASTE:
                  arranger->cmd(Arranger::CMD_PASTE_PART);
                  break;
            case CMD_PASTE_CLONE:
                  arranger->cmd(Arranger::CMD_PASTE_CLONE_PART);
                  break;
            case CMD_PASTE_TO_TRACK:
                  arranger->cmd(Arranger::CMD_PASTE_PART_TO_TRACK);
                  break;
            case CMD_PASTE_CLONE_TO_TRACK:
                  arranger->cmd(Arranger::CMD_PASTE_CLONE_PART_TO_TRACK);
                  break;
            case CMD_INSERT:
                  arranger->cmd(Arranger::CMD_INSERT_PART);
                  break;
            case CMD_INSERTMEAS:
                  arranger->cmd(Arranger::CMD_INSERT_EMPTYMEAS);
                  break;
            case CMD_DELETE:
                  song->startUndo();
                  if (song->msgRemoveParts()) {
                        song->endUndo(SC_PART_REMOVED);
                        break;
                        }
                  else
                        audio->msgRemoveTracks();
                  song->endUndo(SC_TRACK_REMOVED);
                  break;
            case CMD_DELETE_TRACK:
                  song->startUndo();
                  audio->msgRemoveTracks();
                  song->endUndo(SC_TRACK_REMOVED);
                  audio->msgUpdateSoloStates();
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
                  song->setFollow(Song::NO);
                  setFollow();
                  break;
            case CMD_FOLLOW_JUMP:
                  song->setFollow(Song::JUMP);
                  setFollow();
                  break;
            case CMD_FOLLOW_CONTINUOUS:
                  song->setFollow(Song::CONTINUOUS);
                  setFollow();
                  break;
            }
      }

//---------------------------------------------------------
//   clipboardChanged
//---------------------------------------------------------

void MusE::clipboardChanged()
      {
      Q3CString subtype("partlist");
      QMimeSource* ms = QApplication::clipboard()->data(QClipboard::Clipboard);
      if (ms == 0)
            return;
      bool flag = false;
      for (int i = 0; ms->format(i); ++i) {
// printf("Format <%s\n", ms->format(i));
            if ((strncmp(ms->format(i), "text/midipartlist", 17) == 0)
               || (strncmp(ms->format(i), "text/wavepartlist", 17) == 0) 
               // Added by T356. Support mixed .mpt files.
               || (strncmp(ms->format(i), "text/mixedpartlist", 18) == 0)) {
                  flag = true;
                  break;
                  }
            }
      menuEdit->setItemEnabled(CMD_PASTE, flag);
      menuEdit->setItemEnabled(CMD_INSERT, flag);
      menuEdit->setItemEnabled(CMD_PASTE_CLONE, flag);
      menuEdit->setItemEnabled(CMD_PASTE_TO_TRACK, flag);
      menuEdit->setItemEnabled(CMD_PASTE_CLONE_TO_TRACK, flag);
      }

//---------------------------------------------------------
//   selectionChanged
//---------------------------------------------------------

void MusE::selectionChanged()
      {
      bool flag = arranger->isSingleSelection();
      menuEdit->setItemEnabled(CMD_CUT, flag);
      //menuEdit->setItemEnabled(CMD_COPY, flag); // Now possible
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
      GateTime* w = new GateTime(this);
      w->show();
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

void MusE::configAppearance()
      {
      if (!appearance)
            appearance = new Appearance(arranger);
      appearance->resetValues();
      if(appearance->isVisible()) {
          appearance->raise();
          appearance->setActiveWindow();
          }
      else
          appearance->show();
      }

//---------------------------------------------------------
//   loadTheme
//---------------------------------------------------------

void MusE::loadTheme(QString s)
      {
      if (style()->name() != s)
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
      QApplication::setFont(config.fonts[0], true);
      // Added by Tim. p3.3.6
      //printf("MusE::changeConfig writeFlag:%d emitting configChanged\n", writeFlag);
      
      emit configChanged();
      updateConfiguration();
      }

//---------------------------------------------------------
//   configMetronome
//---------------------------------------------------------

void MusE::configMetronome()
      {
      if (!metronomeConfig)
            metronomeConfig = new MetronomeConfig(this, "metronome");

      if(metronomeConfig->isVisible()) {
          metronomeConfig->raise();
          metronomeConfig->setActiveWindow();
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
            shortcutConfig = new ShortcutConfig(this, "shortcutconfig");
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
      TrackList* tracks = song->tracks();
      for (iTrack it = tracks->begin(); it != tracks->end(); ++it) {
            MidiTrack* track = dynamic_cast<MidiTrack*>(*it);
            if (track == 0 || track->mute())
                  continue;
            PartList* pl = track->parts();
            for (iPart p = pl->begin(); p != pl->end(); ++p) {
                  Part* part = p->second;
                  int t = part->tick();
                  int l = part->lenTick();
                  if (t + l <= lpos)
                        continue;
                  if ((t >= lpos) && ((t+l) <= rpos)) {
                        audio->msgRemovePart(part, false);
                        }
                  else if ((t < lpos) && ((t+l) > lpos) && ((t+l) <= rpos)) {
                        // remove part tail
                        int len = lpos - t;
                        MidiPart* nPart = new MidiPart(*(MidiPart*)part);
                        nPart->setLenTick(len);
                        //
                        // cut Events in nPart
                        EventList* el = nPart->events();
                        iEvent ie = el->lower_bound(t + len);
                        for (; ie != el->end();) {
                              iEvent i = ie;
                              ++ie;
                              // Indicate no undo, and do not do port controller values and clone parts. 
                              //audio->msgDeleteEvent(i->second, nPart, false);
                              audio->msgDeleteEvent(i->second, nPart, false, false, false);
                              }
                        // Indicate no undo, and do port controller values and clone parts. 
                        //audio->msgChangePart(part, nPart, false);
                        audio->msgChangePart(part, nPart, false, true, true);
                        }
                  else if ((t < lpos) && ((t+l) > lpos) && ((t+l) > rpos)) {
                        //----------------------
                        // remove part middle
                        //----------------------

                        MidiPart* nPart = new MidiPart(*(MidiPart*)part);
                        EventList* el = nPart->events();
                        iEvent is = el->lower_bound(lpos);
                        iEvent ie = el->upper_bound(rpos);
                        for (iEvent i = is; i != ie;) {
                              iEvent ii = i;
                              ++i;
                              // Indicate no undo, and do not do port controller values and clone parts. 
                              //audio->msgDeleteEvent(ii->second, nPart, false);
                              audio->msgDeleteEvent(ii->second, nPart, false, false, false);
                              }

                        ie = el->lower_bound(rpos);
                        for (; ie != el->end();) {
                              iEvent i = ie;
                              ++ie;
                              Event event = i->second;
                              Event nEvent = event.clone();
                              nEvent.setTick(nEvent.tick() - (rpos-lpos));
                              // Indicate no undo, and do not do port controller values and clone parts. 
                              //audio->msgChangeEvent(event, nEvent, nPart, false);
                              audio->msgChangeEvent(event, nEvent, nPart, false, false, false);
                              }
                        nPart->setLenTick(l - (rpos-lpos));
                        // Indicate no undo, and do port controller values and clone parts. 
                        //audio->msgChangePart(part, nPart, false);
                        audio->msgChangePart(part, nPart, false, true, true);
                        }
                  else if ((t >= lpos) && (t < rpos) && (t+l) > rpos) {
                        // TODO: remove part head
                        }
                  else if (t >= rpos) {
                        MidiPart* nPart = new MidiPart(*(MidiPart*)part);
                        int nt = part->tick();
                        nPart->setTick(nt - (rpos -lpos));
                        // Indicate no undo, and do port controller values but not clone parts. 
                        //audio->msgChangePart(part, nPart, false);
                        audio->msgChangePart(part, nPart, false, true, false);
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
      TrackList* tracks = song->tracks();
      for (iTrack it = tracks->begin(); it != tracks->end(); ++it) {
            MidiTrack* track = dynamic_cast<MidiTrack*>(*it);
            //
            // process only non muted midi tracks
            //
            if (track == 0 || track->mute())
                  continue;
            PartList* pl = track->parts();
            for (iPart p = pl->begin(); p != pl->end(); ++p) {
                  Part* part = p->second;
                  unsigned t = part->tick();
                  int l = part->lenTick();
                  if (t + l <= lpos)
                        continue;
                  if (lpos >= t && lpos < (t+l)) {
                        MidiPart* nPart = new MidiPart(*(MidiPart*)part);
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
                              // Indicate no undo, and do not do port controller values and clone parts. 
                              //audio->msgChangeEvent(event, nEvent, nPart, false);
                              audio->msgChangeEvent(event, nEvent, nPart, false, false, false);
                              }
                        // Indicate no undo, and do port controller values and clone parts. 
                        //audio->msgChangePart(part, nPart, false);
                        audio->msgChangePart(part, nPart, false, true, true);
                        }
                  else if (t > lpos) {
                        MidiPart* nPart = new MidiPart(*(MidiPart*)part);
                        nPart->setTick(t + (rpos -lpos));
                        // Indicate no undo, and do port controller values but not clone parts. 
                        //audio->msgChangePart(part, nPart, false);
                        audio->msgChangePart(part, nPart, false, true, false);
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
                        // Indicate no undo, and do port controller values but not clone parts. 
                        //audio->msgChangePart(part, p1, false);
                        audio->msgChangePart(part, p1, false, true, false);
                        audio->msgAddPart(p2, false);
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

#if 0
//---------------------------------------------------------
//   openAudioFileManagement
//---------------------------------------------------------
void MusE::openAudioFileManagement()
      {
      if (!audioFileManager) {
            audioFileManager = new AudioFileManager(this, "audiofilemanager", false);
            audioFileManager->show();
            }
      audioFileManager->setShown(true);
      }
#endif
//---------------------------------------------------------
//   bounceToTrack
//---------------------------------------------------------

void MusE::bounceToTrack()
      {
      if(audio->bounce())
        return;
      
      song->bounceOutput = 0;
      
      if(song->waves()->empty())
      {
        QMessageBox::critical(this,
            tr("MusE: Bounce to Track"),
            tr("No wave tracks found")
            );
        return;
      }
      
      OutputList* ol = song->outputs();
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
      
      AudioOutput* out = 0;
      // If only one output, pick it, else pick the first selected.
      if(ol->size() == 1)
        out = ol->front();
      else
      {
        for(iAudioOutput iao = ol->begin(); iao != ol->end(); ++iao) 
        {
          AudioOutput* o = *iao;
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
      TrackList* tl = song->tracks();
      WaveTrack* track = 0;
      
      for (iTrack it = tl->begin(); it != tl->end(); ++it) {
            Track* t = *it;
            if (t->selected()) {
                    if(t->type() != Track::WAVE && t->type() != Track::AUDIO_OUTPUT) {
                        track = 0;
                        break;
                    }
                    if(t->type() == Track::WAVE)
                    { 
                      if(track)
                      {
                        track = 0;
                        break;
                      }
                      track = (WaveTrack*)t;
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
      song->bounceOutput = out;
      song->bounceTrack = track;
      song->setRecord(true);
      song->setRecordFlag(track, true);
      audio->msgBounce();
      }

//---------------------------------------------------------
//   bounceToFile
//---------------------------------------------------------

void MusE::bounceToFile(AudioOutput* ao)
      {
      if(audio->bounce())
        return;
      song->bounceOutput = 0;
      if(!ao)
      {
        OutputList* ol = song->outputs();
        if(ol->empty())
        {
          QMessageBox::critical(this,
              tr("MusE: Bounce to Track"),
              tr("No audio output tracks found")
              );
          return;
        }
        // If only one output, pick it, else pick the first selected.
        if(ol->size() == 1)
          ao = ol->front();
        else
        {
          for(iAudioOutput iao = ol->begin(); iao != ol->end(); ++iao) 
          {
            AudioOutput* o = *iao;
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
      
      SndFile* sf = getSndFile(0, this, 0);
      if (sf == 0)
            return;
            
      song->bounceOutput = ao;
      ao->setRecFile(sf);
      song->setRecord(true, false);
      song->setRecordFlag(ao, true);
      audio->msgBounce();
      }

#ifdef HAVE_LASH
//---------------------------------------------------------
//   lash_idle_cb
//---------------------------------------------------------
#include <iostream>
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
          int ok = save (ss.ascii(), false);
          if (ok) {
            project.setFile(ss.ascii());
            setCaption(tr("MusE: Song: ") + project.baseName(true));
            addProject(ss.ascii());
            museProject = QFileInfo(ss.ascii()).dirPath(true);
          }
          lash_send_event (lash_client, event);
    }
    break;

        case LASH_Restore_File:
    {
          /* load file */
          QString sr = QString(lash_event_get_string(event)) + QString("/lash-project-muse.med");
          loadProjectFile(sr.ascii(), false, true);
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
//---------------------------------------------------------

bool MusE::clearSong()
      {
      if (song->dirty) {
            int n = 0;
            n = QMessageBox::warning(this, appName,
               tr("The current Project contains unsaved data\n"
               "Load overwrites current Project:\n"
               "Save Current Project?"),
               tr("&Save"), tr("&Skip"), tr("&Abort"), 0, 2);
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
      if (audio->isPlaying()) {
            audio->msgPlay(false);
            while (audio->isPlaying())
                  qApp->processEvents();
            }

again:
      for (iToplevel i = toplevels.begin(); i != toplevels.end(); ++i) {
            Toplevel tl = *i;
            unsigned long obj = tl.object();
            switch (tl.type()) {
                  case Toplevel::CLIPLIST:
                  case Toplevel::MARKER:
                        break;
                  case Toplevel::PIANO_ROLL:
                  case Toplevel::LISTE:
                  case Toplevel::DRUM:
                  case Toplevel::MASTER:
                  case Toplevel::WAVE:
                  case Toplevel::LMASTER:
                        ((QWidget*)(obj))->close(true);
                        goto again;
                  }
            }
      song->clear(false);
      return false;
      }

//---------------------------------------------------------
//   startEditInstrument
//---------------------------------------------------------

void MusE::startEditInstrument()
    {
      if(editInstrument == 0)
      {
            editInstrument = new EditInstrument(this);
            editInstrument->show();
      }
      else
      {
        if(editInstrument->isShown())
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
      automation = !automation;
      // Clear all pressed and touched and rec event lists.
      song->clearRecAutomation(true);

printf("automation = %d\n", automation);
      menuAutomation->setItemChecked(autoId, automation);
      }

//---------------------------------------------------------
//   clearAutomation
//---------------------------------------------------------

void MusE::clearAutomation()
      {
      printf("not implemented\n");
      }

//---------------------------------------------------------
//   takeAutomationSnapshot
//---------------------------------------------------------

void MusE::takeAutomationSnapshot()
      {
      int frame = song->cPos().frame();
      TrackList* tracks = song->tracks();
      for (iTrack i = tracks->begin(); i != tracks->end(); ++i) {
            if ((*i)->isMidiTrack())
                  continue;
            AudioTrack* track = (AudioTrack*)*i;
            CtrlListList* cll = track->controller();
            for (iCtrlList icl = cll->begin(); icl != cll->end(); ++icl) {
                  double val = icl->second->curVal();
                  icl->second->add(frame, val);
                  }
            }
      }

//---------------------------------------------------------
//   updateConfiguration
//    called whenever the configuration has changed
//---------------------------------------------------------

void MusE::updateConfiguration()
      {
      fileOpenAction->setAccel(shortcuts[SHRT_OPEN].key);
      fileNewAction->setAccel(shortcuts[SHRT_NEW].key);
      fileSaveAction->setAccel(shortcuts[SHRT_SAVE].key);

      menu_file->setAccel(shortcuts[SHRT_OPEN_RECENT].key, menu_ids[CMD_OPEN_RECENT]);
      menu_file->setAccel(shortcuts[SHRT_LOAD_TEMPLATE].key, menu_ids[CMD_LOAD_TEMPLATE]);
      menu_file->setAccel(shortcuts[SHRT_SAVE_AS].key, menu_ids[CMD_SAVE_AS]);
      menu_file->setAccel(shortcuts[SHRT_IMPORT_MIDI].key, menu_ids[CMD_IMPORT_MIDI]);
      menu_file->setAccel(shortcuts[SHRT_EXPORT_MIDI].key, menu_ids[CMD_EXPORT_MIDI]);
      menu_file->setAccel(shortcuts[SHRT_IMPORT_PART].key, menu_ids[CMD_IMPORT_PART]);
      menu_file->setAccel(shortcuts[SHRT_IMPORT_AUDIO].key, menu_ids[CMD_IMPORT_AUDIO]);
      menu_file->setAccel(shortcuts[SHRT_QUIT].key, menu_ids[CMD_QUIT]);

      menuEdit->setAccel(Qt::Key_Delete, CMD_DELETE);
      menuEdit->setAccel(shortcuts[SHRT_OPEN_DRUMS].key, menu_ids[CMD_OPEN_DRUMS]);
      menuEdit->setAccel(shortcuts[SHRT_OPEN_LIST].key, menu_ids[CMD_OPEN_LIST]);
      menuEdit->setAccel(shortcuts[SHRT_OPEN_WAVE].key, menu_ids[CMD_OPEN_WAVE]);
      menuEdit->setAccel(shortcuts[SHRT_OPEN_MIDI_TRANSFORM].key, menu_ids[CMD_OPEN_MIDI_TRANSFORM]);

      midiEdit->setAccel(shortcuts[SHRT_TRANSPOSE].key, menu_ids[CMD_TRANSPOSE]);

      master->setAccel(shortcuts[SHRT_OPEN_GRAPHIC_MASTER].key, menu_ids[CMD_OPEN_GRAPHIC_MASTER]);
      master->setAccel(shortcuts[SHRT_OPEN_LIST_MASTER].key, menu_ids[CMD_OPEN_LIST_MASTER]);

      menuStructure->setAccel(shortcuts[SHRT_GLOBAL_CUT].key, menu_ids[CMD_GLOBAL_CUT]);
      menuStructure->setAccel(shortcuts[SHRT_GLOBAL_INSERT].key, menu_ids[CMD_GLOBAL_INSERT]);
      menuStructure->setAccel(shortcuts[SHRT_GLOBAL_SPLIT].key, menu_ids[CMD_GLOBAL_SPLIT]);
      menuStructure->setAccel(shortcuts[SHRT_COPY_RANGE].key, menu_ids[CMD_COPY_RANGE]);
      menuStructure->setAccel(shortcuts[SHRT_CUT_EVENTS].key, menu_ids[CMD_CUT_EVENTS]);

      menuView->setAccel(shortcuts[SHRT_OPEN_TRANSPORT].key, tr_id);
      menuView->setAccel(shortcuts[SHRT_OPEN_BIGTIME].key, bt_id);
      //menuView->setAccel(shortcuts[SHRT_OPEN_MIXER].key, aid1);
      menuView->setAccel(shortcuts[SHRT_OPEN_MIXER].key, aid1a);
      menuView->setAccel(shortcuts[SHRT_OPEN_MIXER2].key, aid1b);
//      menuView->setAccel(shortcuts[SHRT_OPEN_CLIPS].key, aid2);
//      markerAction->setAccel(shortcuts[SHRT_OPEN_MARKER].key );
      menuView->setAccel(shortcuts[SHRT_OPEN_MARKER].key, mr_id );

      menuSettings->setAccel(shortcuts[SHRT_GLOBAL_CONFIG].key, menu_ids[CMD_GLOBAL_CONFIG]);
      menuSettings->setAccel(shortcuts[SHRT_CONFIG_SHORTCUTS].key, menu_ids[CMD_CONFIG_SHORTCUTS]);
      menuSettings->setAccel(shortcuts[SHRT_CONFIG_METRONOME].key, menu_ids[CMD_CONFIG_METRONOME]);
      menuSettings->setAccel(shortcuts[SHRT_CONFIG_MIDISYNC].key, menu_ids[CMD_CONFIG_MIDISYNC]);
      menuSettings->setAccel(shortcuts[SHRT_APPEARANCE_SETTINGS].key, menu_ids[CMD_APPEARANCE_SETTINGS]);
      menuSettings->setAccel(shortcuts[SHRT_CONFIG_MIDI_PORTS].key, menu_ids[CMD_CONFIG_MIDI_PORTS]);
      menuSettings->setAccel(shortcuts[SHRT_CONFIG_AUDIO_PORTS].key, menu_ids[CMD_CONFIG_AUDIO_PORTS]);

//      menu_functions->setAccel(shortcuts[SHRT_MIDI_EDIT_INSTRUMENTS].key, menu_ids[CMD_MIDI_EDIT_INSTRUMENTS]);
      menu_functions->setAccel(shortcuts[SHRT_MIDI_RESET].key, menu_ids[CMD_MIDI_RESET]);
      menu_functions->setAccel(shortcuts[SHRT_MIDI_INIT].key, menu_ids[CMD_MIDI_INIT]);
      menu_functions->setAccel(shortcuts[SHRT_MIDI_LOCAL_OFF].key, menu_ids[CMD_MIDI_LOCAL_OFF]);

      menu_audio->setAccel(shortcuts[SHRT_AUDIO_BOUNCE_TO_TRACK].key, menu_ids[CMD_AUDIO_BOUNCE_TO_TRACK]);
      menu_audio->setAccel(shortcuts[SHRT_AUDIO_BOUNCE_TO_FILE].key , menu_ids[CMD_AUDIO_BOUNCE_TO_FILE]);
      menu_audio->setAccel(shortcuts[SHRT_AUDIO_RESTART].key, menu_ids[CMD_AUDIO_RESTART]);

      menuAutomation->setAccel(shortcuts[SHRT_MIXER_AUTOMATION].key, autoId);
      menuAutomation->setAccel(shortcuts[SHRT_MIXER_SNAPSHOT].key, menu_ids[CMD_MIXER_SNAPSHOT]);
      menuAutomation->setAccel(shortcuts[SHRT_MIXER_AUTOMATION_CLEAR].key, menu_ids[CMD_MIXER_AUTOMATION_CLEAR]);

      menu_help->setAccel(menu_ids[CMD_OPEN_HELP], shortcuts[SHRT_OPEN_HELP].key);
      menu_help->setAccel(menu_ids[CMD_START_WHATSTHIS], shortcuts[SHRT_START_WHATSTHIS].key);
      pianoAction->setAccel(shortcuts[SHRT_OPEN_PIANO].key);

      select->setAccel(shortcuts[SHRT_SELECT_ALL].key, CMD_SELECT_ALL);

//      select->setAccel(shortcuts[SHRT_DESEL_PARTS].key, CMD_SELECT_NONE);
      select->setAccel(shortcuts[SHRT_SELECT_NONE].key, CMD_SELECT_NONE);

      select->setAccel(shortcuts[SHRT_SELECT_INVERT].key, CMD_SELECT_INVERT);
      select->setAccel(shortcuts[SHRT_SELECT_ILOOP].key, CMD_SELECT_ILOOP);
      select->setAccel(shortcuts[SHRT_SELECT_OLOOP].key, CMD_SELECT_OLOOP);
      select->setAccel(shortcuts[SHRT_SELECT_PRTSTRACK].key, CMD_SELECT_PARTS);
      follow->setAccel(shortcuts[SHRT_FOLLOW_JUMP].key, CMD_FOLLOW_JUMP);
      follow->setAccel(shortcuts[SHRT_FOLLOW_NO].key, CMD_FOLLOW_NO);
      follow->setAccel(shortcuts[SHRT_FOLLOW_CONTINUOUS].key, CMD_FOLLOW_CONTINUOUS);
      midiInputPlugins->setAccel(shortcuts[SHRT_MIDI_INPUT_TRANSPOSE].key, 0);
      midiInputPlugins->setAccel(shortcuts[SHRT_MIDI_INPUT_TRANSFORM].key, 1);
      midiInputPlugins->setAccel(shortcuts[SHRT_MIDI_INPUT_FILTER].key, 2);
      midiInputPlugins->setAccel(shortcuts[SHRT_MIDI_REMOTE_CONTROL].key, 3);
      midiInputPlugins->setAccel(shortcuts[SHRT_RANDOM_RHYTHM_GENERATOR].key, 4);

      addTrack->setAccel(shortcuts[SHRT_ADD_MIDI_TRACK].key, Track::MIDI);
      addTrack->setAccel(shortcuts[SHRT_ADD_DRUM_TRACK].key, Track::DRUM);
      addTrack->setAccel(shortcuts[SHRT_ADD_WAVE_TRACK].key, Track::WAVE);
      addTrack->setAccel(shortcuts[SHRT_ADD_AUDIO_OUTPUT].key, Track::AUDIO_OUTPUT);
      addTrack->setAccel(shortcuts[SHRT_ADD_AUDIO_GROUP].key, Track::AUDIO_GROUP);
      addTrack->setAccel(shortcuts[SHRT_ADD_AUDIO_INPUT].key, Track::AUDIO_INPUT);
      addTrack->setAccel(shortcuts[SHRT_ADD_AUDIO_AUX].key, Track::AUDIO_AUX);
      }

//---------------------------------------------------------
//   showBigtime
//---------------------------------------------------------

void MusE::showBigtime(bool on)
      {
      if (on && bigtime == 0) {
            bigtime = new BigTime(0);
            bigtime->setPos(0, song->cpos(), false);
            connect(song, SIGNAL(posChanged(int, unsigned, bool)), bigtime, SLOT(setPos(int, unsigned, bool)));
            connect(muse, SIGNAL(configChanged()), bigtime, SLOT(configChanged()));
            connect(bigtime, SIGNAL(closed()), SLOT(bigtimeClosed()));
            bigtime->resize(config.geometryBigTime.size());
            bigtime->move(config.geometryBigTime.topLeft());
            }
      if (bigtime)
            bigtime->setShown(on);
      menuView->setItemChecked(bt_id, on);
      }

//---------------------------------------------------------
//   toggleBigTime
//---------------------------------------------------------

void MusE::toggleBigTime()
      {
      showBigtime(!menuView->isItemChecked(bt_id));
      }

//---------------------------------------------------------
//   bigtimeClosed
//---------------------------------------------------------

void MusE::bigtimeClosed()
      {
      menuView->setItemChecked(bt_id, false);
      }

//---------------------------------------------------------
//   showMixer
//---------------------------------------------------------

/*
void MusE::showMixer(bool on)
      {
      if (on && audioMixer == 0) {
            audioMixer = new AudioMixerApp(this);
            connect(audioMixer, SIGNAL(closed()), SLOT(mixerClosed()));
            audioMixer->resize(config.geometryMixer.size());
            audioMixer->move(config.geometryMixer.topLeft());
            }
      if (audioMixer)
            audioMixer->setShown(on);
      menuView->setItemChecked(aid1, on);
      }
*/

//---------------------------------------------------------
//   showMixer1
//---------------------------------------------------------

void MusE::showMixer1(bool on)
      {
      if (on && mixer1 == 0) {
            mixer1 = new AudioMixerApp(this, &(config.mixer1));
            connect(mixer1, SIGNAL(closed()), SLOT(mixer1Closed()));
            mixer1->resize(config.mixer1.geometry.size());
            mixer1->move(config.mixer1.geometry.topLeft());
            }
      if (mixer1)
            mixer1->setShown(on);
      menuView->setItemChecked(aid1a, on);
      }

//---------------------------------------------------------
//   showMixer2
//---------------------------------------------------------

void MusE::showMixer2(bool on)
      {
      if (on && mixer2 == 0) {
            mixer2 = new AudioMixerApp(this, &(config.mixer2));
            connect(mixer2, SIGNAL(closed()), SLOT(mixer2Closed()));
            mixer2->resize(config.mixer2.geometry.size());
            mixer2->move(config.mixer2.geometry.topLeft());
            }
      if (mixer2)
            mixer2->setShown(on);
      menuView->setItemChecked(aid1b, on);
      }

//---------------------------------------------------------
//   toggleMixer
//---------------------------------------------------------

/*
void MusE::toggleMixer()
      {
      showMixer(!menuView->isItemChecked(aid1));
      }
*/

//---------------------------------------------------------
//   toggleMixer1
//---------------------------------------------------------

void MusE::toggleMixer1()
      {
      printf("toggle mixer1\n");
      //showMixer1(!menuView->isItemChecked(aid1a));
      showMixer1(true);
      }

//---------------------------------------------------------
//   toggleMixer2
//---------------------------------------------------------

void MusE::toggleMixer2()
      {
      showMixer2(!menuView->isItemChecked(aid1b));
      }

//---------------------------------------------------------
//   mixerClosed
//---------------------------------------------------------

/*
void MusE::mixerClosed()
      {
      menuView->setItemChecked(aid1, false);
      }
*/

//---------------------------------------------------------
//   mixer1Closed
//---------------------------------------------------------

void MusE::mixer1Closed()
      {
      //aid1a->setChecked(false);
      menuView->setItemChecked(aid1a, false);
      }

//---------------------------------------------------------
//   mixer2Closed
//---------------------------------------------------------

void MusE::mixer2Closed()
      {
      //aid1b->setChecked(false);
      menuView->setItemChecked(aid1b, false);
      }


//QWidget* MusE::mixerWindow()     { return audioMixer; }
QWidget* MusE::mixer1Window()     { return mixer1; }
QWidget* MusE::mixer2Window()     { return mixer2; }

QWidget* MusE::transportWindow() { return transport; }
QWidget* MusE::bigtimeWindow()   { return bigtime; }

//---------------------------------------------------------
//   focusInEvent
//---------------------------------------------------------

void MusE::focusInEvent(QFocusEvent* ev)
      {
      //if (audioMixer)
      //      audioMixer->raise();
      if (mixer1)
            mixer1->raise();
      if (mixer2)
            mixer2->raise();
      raise();
      Q3MainWindow::focusInEvent(ev);
      }

//---------------------------------------------------------
//   setUsedTool
//---------------------------------------------------------

void MusE::setUsedTool(int tool)
      {
      tools1->set(tool);
      }


//---------------------------------------------------------
//   execDeliveredScript
//---------------------------------------------------------
void MusE::execDeliveredScript(int id)
{
      //QString scriptfile = QString(INSTPREFIX) + SCRIPTSSUFFIX + deliveredScriptNames[id];
      song->executeScript(song->getScriptPath(id, true), song->getSelectedMidiParts(), 0, false); // TODO: get quant from arranger
}
//---------------------------------------------------------
//   execUserScript
//---------------------------------------------------------
void MusE::execUserScript(int id)
{
      song->executeScript(song->getScriptPath(id, false), song->getSelectedMidiParts(), 0, false); // TODO: get quant from arranger
}

