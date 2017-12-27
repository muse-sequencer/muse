//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: app.cpp,v 1.113.2.68 2009/12/21 14:51:51 spamatica Exp $
//
//  (C) Copyright 1999-2011 Werner Schweer (ws@seh.de)
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

#include <QApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QFileInfoList>
#include <QKeyEvent>
#include <QMessageBox>
#include <QLocale>
#include <QSplashScreen>
#include <QTimer>
#include <QTranslator>
#include <QIcon>
#include <QString>
#include <QStringList>
#include <QStyle>
#include <QStyleFactory>
#include <iostream>

#include <unistd.h>
#include <time.h>
#include <sys/mman.h>

#include "config.h"

#ifdef ALSA_SUPPORT
#include <alsa/asoundlib.h>
#endif

#include "al/dsp.h"
#include "app.h"
#include "audio.h"
#include "audiodev.h"
#include "gconfig.h"
#include "globals.h"
#include "helper.h"
#include "sync.h"
#include "functions.h"
#include "appearance.h"
#include "midiseq.h"
#include "minstrument.h"  
#include "midiport.h"
#include "mididev.h"
#include "plugin.h"
#include "wavepreview.h"
#include "audio_convert/audio_converter_plugin.h"
#include "audio_convert/audio_converter_settings_group.h"

#ifdef HAVE_LASH
#include <lash/lash.h>
#endif

namespace MusECore {
extern bool initDummyAudio();
#ifdef HAVE_RTAUDIO
extern bool initRtAudio(bool forceDefault = false);
#endif
extern bool initJackAudio();
extern void initMidiController();
extern void initMetronome();
extern void initOSC();
extern void initVST();
extern void initVST_Native();
extern void initPlugins();
extern void initDSSI();
#ifdef LV2_SUPPORT
extern void initLV2();
extern void deinitLV2();
#endif
extern void readConfiguration();

extern void initMidiSequencer();   
extern void exitMidiSequencer();
extern void initAudio();
extern void initAudioPrefetch();   
extern void initMidiSynth();

#ifdef ALSA_SUPPORT
extern snd_seq_t * alsaSeq;
#endif

extern void setAlsaClientName(const char*);
}

namespace MusEGui {
void initIcons();
void initShortCuts();
#ifdef HAVE_LASH
extern lash_client_t * lash_client;
#endif
extern QStringList projectRecentList;
}

enum AudioDriverSelect {
  DriverConfigSetting,
  DummyAudioOverride,
  JackAudioOverride,
  RtAudioOverride,

};

static QString locale_override;

//---------------------------------------------------------
//   printVersion
//---------------------------------------------------------

static void printVersion(const char* prog)
      {
      if (strcmp("", GITSTRING))
            fprintf(stderr, "%s: Linux Music Editor; Version %s, (%s)\n", prog, VERSION, GITSTRING);
      else
            fprintf(stderr, "%s: Linux Music Editor; Version %s\n", prog, VERSION);
      }

//---------------------------------------------------------
//   MuseApplication
//---------------------------------------------------------

class MuseApplication : public QApplication {
      MusEGui::MusE* muse;

   public:
      MuseApplication(int& argc, char** argv)
         : QApplication(argc, argv)
            {
            muse = 0;
            }


      void setMuse(MusEGui::MusE* m) {
            muse = m;
            
            connect(this,SIGNAL(focusChanged(QWidget*,QWidget*)),muse,SLOT(focusChanged(QWidget*,QWidget*)));
#ifdef HAVE_LASH
            if(MusEGlobal::useLASH)
              startTimer (300);
#endif
            }

      bool notify(QObject* receiver, QEvent* event) {
         bool flag = QApplication::notify(receiver, event);
         if (event->type() == QEvent::KeyPress) {
#if QT_VERSION >= 0x050000
            const QMetaObject * mo = receiver->metaObject();
            bool forQWidgetWindow = false;
            if (mo){
               if (strcmp(mo->className(), "QWidgetWindow") == 0)
                  forQWidgetWindow = true;
            }
            if(forQWidgetWindow){
               return false;
            }
#endif
            QKeyEvent* ke = (QKeyEvent*)event;
            MusEGlobal::globalKeyState = ke->modifiers();
            bool accepted = ke->isAccepted();
            if (!accepted) {
               int key = ke->key();
               if (((QInputEvent*)ke)->modifiers() & Qt::ShiftModifier)
                  key += Qt::SHIFT;
               if (((QInputEvent*)ke)->modifiers() & Qt::AltModifier)
                  key += Qt::ALT;
               if (((QInputEvent*)ke)->modifiers() & Qt::ControlModifier)
                  key+= Qt::CTRL;
               muse->kbAccel(key);
               return true;
            }
         }
         if (event->type() == QEvent::KeyRelease) {
            QKeyEvent* ke = (QKeyEvent*)event;
            ///MusEGlobal::globalKeyState = ke->stateAfter();
            MusEGlobal::globalKeyState = ke->modifiers();
         }

         return flag;
      }

#ifdef HAVE_LASH
     virtual void timerEvent (QTimerEvent*) {
            if(MusEGlobal::useLASH)
              muse->lash_idle_cb ();
            }
#endif /* HAVE_LASH */

      };

//---------------------------------------------------------
//   localeList
//---------------------------------------------------------

static QString localeList()
      {
      // Find out what translations are available:
      QStringList deliveredLocaleListFiltered;
      QString distLocale = MusEGlobal::museGlobalShare + "/locale";
      QFileInfo distLocaleFi(distLocale);
      if (distLocaleFi.isDir()) {
            QDir dir = QDir(distLocale);
            QStringList deliveredLocaleList = dir.entryList();
            for(QStringList::iterator it = deliveredLocaleList.begin(); it != deliveredLocaleList.end(); ++it) {
                  QString item = *it;
                  if (item.endsWith(".qm")) {
                        int inipos = item.indexOf("muse_") + 5;
                        int finpos = item.lastIndexOf(".qm");
                        deliveredLocaleListFiltered << item.mid(inipos, finpos - inipos);
                        }
                  }
            return deliveredLocaleListFiltered.join(",");
            }
      return QString("No translations found!");
      }

//---------------------------------------------------------
//   usage
//---------------------------------------------------------

static void usage(const char* prog, const char* txt)
      {
      fprintf(stderr, "\n");
      fprintf(stderr, "%s: %s\nUsage: %s flags midifile\n   Flags:\n",
         prog, txt, prog);
      fprintf(stderr, "   -h       This help\n");
      fprintf(stderr, "   -v       Print version\n");
      fprintf(stderr, "   -a       Alsa midi only (using dummy audio driver)\n");
#ifdef HAVE_RTAUDIO
      fprintf(stderr, "   -t       Use RtAudio driver (with Pulse Audio driver).\n");
#endif
      fprintf(stderr, "   -j       Use JAckAudio driver to connect to Jack audio server\n");
      fprintf(stderr, "   -J       Do not try to auto-start the Jack audio server\n");
      fprintf(stderr, "   -F       Do not auto-populate midi ports with midi devices found, at startup\n");
      fprintf(stderr, "   -A       Force inclusion of ALSA midi even if using Jack\n");
      fprintf(stderr, "   -P  n    Set audio driver real time priority to n\n");
      fprintf(stderr, "                        (Dummy only, default 40. Else fixed by Jack.)\n");
      fprintf(stderr, "   -Y  n    Force midi real time priority to n (default: audio driver prio -1)\n");
      fprintf(stderr, "\n");
      fprintf(stderr, "   -p       Don't load LADSPA plugins\n");
#ifdef VST_SUPPORT
      fprintf(stderr, "   -V       Don't load VST plugins\n");
#endif
#ifdef VST_NATIVE_SUPPORT
      fprintf(stderr, "   -N       Don't load Native VST plugins\n");
#endif
#ifdef DSSI_SUPPORT
      fprintf(stderr, "   -I       Don't load DSSI plugins\n");
#endif
#ifdef LV2_SUPPORT
      fprintf(stderr, "   -2       Don't load LV2 plugins\n");
#endif
#ifdef HAVE_LASH
      fprintf(stderr, "   -L       Don't use LASH\n");
#endif
#ifdef ENABLE_PYTHON
      fprintf(stderr, "   -y       Enable Python control support\n");
#endif
      fprintf(stderr, "\n");
      fprintf(stderr, "   -l  xx   Force locale to the given language/country code\n");
      fprintf(stderr, "            (xx = %s)\n", localeList().toLatin1().constData());
      fprintf(stderr, "   -u       Ubuntu/unity workaround: don't allow sharing menus\n");
      fprintf(stderr, "                                     and mdi-subwins.\n");
      fprintf(stderr, "\n");
      fprintf(stderr, "   -d       Debug mode: no threads, no RT\n");
      fprintf(stderr, "   -D       Debug mode: enable some debug messages\n");
      fprintf(stderr, "                        specify twice for lots of debug messages\n");
      fprintf(stderr, "                        this may slow down MusE massively!\n");
      fprintf(stderr, "   -m       Debug mode: trace midi Input\n");
      fprintf(stderr, "   -M       Debug mode: trace midi Output\n");
      fprintf(stderr, "   -s       Debug mode: trace sync\n");
      fprintf(stderr, "\n");
#ifdef HAVE_LASH
      fprintf(stderr, "LASH and ");
#endif
      fprintf(stderr, "Qt options are also accepted. Some common Qt options:\n");
      fprintf(stderr, "   -style [=] style           Set application GUI style. Motif, Windows, Platinum etc.\n"
                      "   -stylesheet [=] stylesheet Set application styleSheet\n" 
                      "   -session [=] session       Restore application from an earlier session\n"
                      "   -widgetcount               Print debug message at end, about undestroyed/maximum widgets\n"
                      "   -reverse                   Set application's layout direction to Qt::RightToLeft\n"
                      "   -graphicssystem            Set backend used for on-screen widgets/QPixmaps: raster or opengl\n"
                      "   -qmljsdebugger = port      Activate QML/JS debugger with port, formatted port:1234[,block]\n" 
      );

      fprintf(stderr, "\n");

      fprintf(stderr, "Some useful environment variables:\n\n");
      fprintf(stderr, "   LANG: Help browser language suffix (en etc.)\n\n");
      fprintf(stderr, "These variables are read ONCE upon first-time run, to fill the Plugin Paths\n"
                      " in Global Settings. Afterwards the paths can be altered in Global Settings:\n\n");
      fprintf(stderr, "   LADSPA_PATH: Override where to look for ladspa plugins, or else\n"
                      "     ~/ladspa:/usr/local/lib64/ladspa:/usr/lib64/ladspa:/usr/local/lib/ladspa:/usr/lib/ladspa\n\n");
#ifdef DSSI_SUPPORT
      fprintf(stderr, "   DSSI_PATH: Override where to look for dssi plugins (dssi-vst plugins: VST_PATH), or else\n"
                      "     ~/dssi:/usr/local/lib64/dssi:/usr/lib64/dssi:/usr/local/lib/dssi:/usr/lib/dssi\n\n" );      
#endif
#ifdef VST_NATIVE_SUPPORT
      fprintf(stderr, "   VST_NATIVE_PATH: Override where to look for native vst plugins, or else VST_PATH, or else\n"
                      "     ~/.vst:~/vst:/usr/local/lib64/vst:/usr/local/lib/vst:/usr/lib64/vst:/usr/lib/vst\n\n");
#endif
#ifdef LV2_SUPPORT
      fprintf(stderr, "   LV2_PATH: Override where to look for LV2 plugins or else\n"
                      "     ~/.lv2:/usr/local/lib/lv2:/usr/lib/lv2\n\n");
#endif

      fprintf(stderr, "\n");
      }


void fallbackDummy() {

  fprintf(stderr, "Falling back to dummy audio driver\n");
  QMessageBox::critical(NULL, "MusE fatal error", "MusE <b>failed</b> to find a <b>Jack audio server</b>.<br><br>"
                                                  "<i>MusE will continue <b>without audio support</b> (-a switch)!</i><br><br>"
                                                  "If this was not intended check that Jack was started. "
                                                  "If Jack <i>was</i> started check that it was\n"
                                                  "started as the same user as MusE.\n");
  MusEGlobal::realTimeScheduling = true;
  MusECore::initDummyAudio();
}

//---------------------------------------------------------
//   main
//---------------------------------------------------------

int main(int argc, char* argv[])
      {
      MusEGlobal::museUser = QString(getenv("HOME"));
      MusEGlobal::museGlobalLib   = QString(LIBDIR);
      MusEGlobal::museGlobalShare = QString(SHAREDIR);
      MusEGlobal::museProject = MusEGlobal::museProjectInitPath; //getcwd(0, 0);
      MusEGlobal::museInstruments = MusEGlobal::museGlobalShare + QString("/instruments");

      // Create config dir if it doesn't exist
      QDir cPath = QDir(MusEGlobal::configPath);
      if (! cPath.exists())
            cPath.mkpath(".");
      
      QFile cConf (MusEGlobal::configName);
      QFile cConfTempl (MusEGlobal::museGlobalShare + QString("/templates/MusE.cfg"));
      bool cConfExists = cConf.exists();
      if (!cConfExists)
      {
        fprintf(stderr, "creating new config...\n");
        if (cConfTempl.copy(MusEGlobal::configName))
          fprintf(stderr, "  success.\n");
        else
          fprintf(stderr, "  FAILED!\n");
      }

      QFile cConfQt (MusEGlobal::configPath + QString("/MusE-qt.conf"));
      QFile cConfTemplQt (MusEGlobal::museGlobalShare + QString("/templates/MusE-qt.conf"));
      if (! cConfQt.exists())
      {
        fprintf(stderr, "creating new qt config...\n");
        if (cConfTemplQt.copy(cConfQt.fileName()))
          fprintf(stderr, "  success.\n");
        else
          fprintf(stderr, "  FAILED!\n");
      }

      MusEGui::initShortCuts();
      
      // REMOVE Tim. samplerate. Added.
      // Discover available MusE audio converters, before reading configuration
      MusEGlobal::audioConverterPluginList.discover();
      //MusEGlobal::defaultAudioConverterSettings.populate(&MusEGlobal::audioConverterPluginList, false);
      MusEGlobal::defaultAudioConverterSettings = new MusECore::AudioConverterSettingsGroup(false); // Default, non-local settings.
      MusEGlobal::defaultAudioConverterSettings->populate(&MusEGlobal::audioConverterPluginList, false);
      
      MusECore::readConfiguration();
      
      // Need to put a sane defaults here because we can't use '~' in the file name strings.
      if(!cConfExists)
      {
        MusEGlobal::config.projectBaseFolder = MusEGlobal::museUser + QString("/MusE");
        MusEGlobal::config.startSong = "";
      }
      
      if(MusEGlobal::config.pluginLadspaPathList.isEmpty())
      {
        QString pth(getenv("LADSPA_PATH"));
        if(pth.isEmpty())
        {
          MusEGlobal::config.pluginLadspaPathList << 
            (MusEGlobal::museUser + QString("/ladspa")) <<
            QString("/usr/local/lib64/ladspa") <<
            QString("/usr/local/lib/ladspa") <<
            QString("/usr/lib64/ladspa") <<
            QString("/usr/lib/ladspa");
            setenv("LADSPA_PATH", MusEGlobal::config.pluginLadspaPathList.join(":").toLatin1().constData(), true);
        }
        else
          MusEGlobal::config.pluginLadspaPathList = pth.split(":", QString::SkipEmptyParts);
      }
      else
        setenv("LADSPA_PATH", MusEGlobal::config.pluginLadspaPathList.join(":").toLatin1().constData(), true);
      
      if(MusEGlobal::config.pluginDssiPathList.isEmpty())
      {
        QString pth(getenv("DSSI_PATH"));
        if(pth.isEmpty())
        {
          MusEGlobal::config.pluginDssiPathList << 
            (MusEGlobal::museUser + QString("/dssi")) <<
            QString("/usr/local/lib64/dssi") <<
            QString("/usr/local/lib/dssi") <<
            QString("/usr/lib64/dssi") <<
            QString("/usr/lib/dssi");
            setenv("DSSI_PATH", MusEGlobal::config.pluginDssiPathList.join(":").toLatin1().constData(), true);
        }
        else
          MusEGlobal::config.pluginDssiPathList = pth.split(":", QString::SkipEmptyParts);
      }
      else
        setenv("DSSI_PATH", MusEGlobal::config.pluginDssiPathList.join(":").toLatin1().constData(), true);
      
      if(MusEGlobal::config.pluginVstPathList.isEmpty())
      {
        QString pth(getenv("VST_PATH"));
        if(!pth.isEmpty())
          MusEGlobal::config.pluginVstPathList = pth.split(":", QString::SkipEmptyParts);
        else
        {
          MusEGlobal::config.pluginVstPathList << 
            (MusEGlobal::museUser + QString("/.vst")) <<
            QString("/usr/local/lib64/vst") <<
            QString("/usr/local/lib/vst") <<
            QString("/usr/lib64/vst") <<
            QString("/usr/lib/vst");
            setenv("VST_PATH", MusEGlobal::config.pluginVstPathList.join(":").toLatin1().constData(), true);
        }
      }
      else
        setenv("VST_PATH", MusEGlobal::config.pluginVstPathList.join(":").toLatin1().constData(), true);
      
      if(MusEGlobal::config.pluginLinuxVstPathList.isEmpty())
      {
        QString pth(getenv("LINUX_VST_PATH"));
        if(!pth.isEmpty())
          MusEGlobal::config.pluginLinuxVstPathList = pth.split(":", QString::SkipEmptyParts);
        else
        {
          pth = QString(getenv("VST_PATH"));
          if(!pth.isEmpty())
            MusEGlobal::config.pluginLinuxVstPathList = pth.split(":", QString::SkipEmptyParts);
          else
          {
            MusEGlobal::config.pluginLinuxVstPathList << 
              (MusEGlobal::museUser + QString("/.vst")) <<
              QString("/usr/local/lib64/vst") <<
              QString("/usr/local/lib/vst") <<
              QString("/usr/lib64/vst") <<
              QString("/usr/lib/vst");
              setenv("LINUX_VST_PATH", MusEGlobal::config.pluginLv2PathList.join(":").toLatin1().constData(), true);
          }
        }
      }
      else
        setenv("LINUX_VST_PATH", MusEGlobal::config.pluginLinuxVstPathList.join(":").toLatin1().constData(), true);
      
      // Special for LV2: Since we use the recommended lilv_world_load_all() 
      //  not lilv_world_load_bundle(), LV2_PATH seems to be the only way to set paths. 
      if(MusEGlobal::config.pluginLv2PathList.isEmpty())
      {
        QString pth(getenv("LV2_PATH"));
        if(pth.isEmpty())
        {
          MusEGlobal::config.pluginLv2PathList <<
            (MusEGlobal::museUser + QString("/.lv2")) <<
            QString("/usr/local/lib64/lv2") <<
            QString("/usr/local/lib/lv2") <<
            QString("/usr/lib64/lv2") <<
            QString("/usr/lib/lv2");
            setenv("LV2_PATH", MusEGlobal::config.pluginLv2PathList.join(":").toLatin1().constData(), true);
        }
        else
          MusEGlobal::config.pluginLv2PathList = pth.split(":", QString::SkipEmptyParts);
      }
      else
        setenv("LV2_PATH", MusEGlobal::config.pluginLv2PathList.join(":").toLatin1().constData(), true);

      int rv = 0;
      bool is_restarting = true; // First-time init true.
      while(is_restarting)
      {
        is_restarting = false;

        // Make working copies of the arguments.
        const int argument_count = argc;
        int argc_copy = argc;
        char** argv_copy = 0;
        if(argument_count > 0)
        {
          argv_copy = (char**)malloc(argument_count * sizeof(char*));
          int len = 0;
          for(int i = 0; i < argument_count; ++i)
          {
            argv_copy[i] = 0;
            if(argv[i])
            {
              len = strlen(argv[i]);
              argv_copy[i] = (char*)malloc(len + 2);
              strcpy(argv_copy[i], argv[i]);
            }
          }
        }

        // Let LASH remove its recognized arguments first (generally longer than Qt's).
        // Tip: LADISH's LASH emulation (current 1.0) does not take any arguments.
  #ifdef HAVE_LASH
        lash_args_t * lash_args = 0;
        lash_args = lash_extract_args (&argc_copy, &argv_copy);
  #endif

        // Now create the application, and let Qt remove recognized arguments.
        MuseApplication app(argc_copy, argv_copy);
        if(QStyle* def_style = app.style())
        {
          const QString appStyleObjName = def_style->objectName();
          MusEGui::Appearance::getSetDefaultStyle(&appStyleObjName);
        }
        
        // NOTE: Set the stylesheet and style as early as possible!
        // Any later invites trouble - typically the colours may be off, 
        //  but currently with Breeze or Oxygen, MDI sub windows  may be frozen!
        // Working with Breeze maintainer to fix problem... 2017/06/06 Tim.
        MusEGui::updateThemeAndStyle();

        QString optstr("atJjFAhvdDumMsP:Y:l:py");
  #ifdef VST_SUPPORT
        optstr += QString("V");
  #endif
  #ifdef VST_NATIVE_SUPPORT
        optstr += QString("N");
  #endif
  #ifdef DSSI_SUPPORT
        optstr += QString("I");
  #endif
  #ifdef HAVE_LASH
        optstr += QString("L");
  #endif
  #ifdef LV2_SUPPORT
        optstr += QString("2");
  #endif

        AudioDriverSelect audioType = DriverConfigSetting;
        int i;

        // Now read the remaining arguments as our own...
        while ((i = getopt(argc_copy, argv_copy, optstr.toLatin1().constData())) != EOF) {
        char c = (char)i;
              switch (c) {
                    case 'v': printVersion(argv_copy[0]);
  #ifdef HAVE_LASH
                          if(lash_args) lash_args_destroy(lash_args);
  #endif
                          return 0;
                    case 'a':
                          audioType = DummyAudioOverride;
                          break;
                    case 't':
                          audioType = RtAudioOverride;
                          break;
                    case 'J':
                          MusEGlobal::noAutoStartJack = true;
                          break;
                    case 'j':
                          audioType = JackAudioOverride;
                          break;
                    case 'F':
                          MusEGlobal::populateMidiPortsOnStart = false;
                          break;
                    case 'A':
                          MusEGlobal::useAlsaWithJack = true;
                          break;
                    case 'd':
                          MusEGlobal::debugMode = true;
                          MusEGlobal::realTimeScheduling = false;
                          break;
                    case 'D':
                          if (!MusEGlobal::debugMsg)
                                MusEGlobal::debugMsg=true;
                          else
                                MusEGlobal::heavyDebugMsg=true;
                          break;
                    case 'm': MusEGlobal::midiInputTrace = true; break;
                    case 'M': MusEGlobal::midiOutputTrace = true; break;
                    case 's': MusEGlobal::debugSync = true; break;
                    case 'u': MusEGlobal::unityWorkaround = true; break;
                    case 'P': MusEGlobal::realTimePriority = atoi(optarg); break;
                    case 'Y': MusEGlobal::midiRTPrioOverride = atoi(optarg); break;
                    case 'p': MusEGlobal::loadPlugins = false; break;
                    case 'V': MusEGlobal::loadVST = false; break;
                    case 'N': MusEGlobal::loadNativeVST = false; break;
                    case 'I': MusEGlobal::loadDSSI = false; break;
                    case 'L': MusEGlobal::useLASH = false; break;
                    case '2': MusEGlobal::loadLV2 = false; break;
                    case 'y': MusEGlobal::usePythonBridge = true; break;
                    case 'l': locale_override = QString(optarg); break;
                    case 'h': usage(argv_copy[0], argv_copy[1]);
  #ifdef HAVE_LASH
                          if(lash_args) lash_args_destroy(lash_args);
  #endif
                          return -1;
                    default:  usage(argv_copy[0], "bad argument");
  #ifdef HAVE_LASH
                          if(lash_args) lash_args_destroy(lash_args);
  #endif
                          return -1;
                    }
              }

        argc_copy -= optind;
        ++argc_copy;

        srand(time(0));   // initialize random number generator
        //signal(SIGCHLD, catchSignal);  // interferes with initVST(). see also app.cpp, function catchSignal()

        static QTranslator translator(0);
        QString locale(QLocale::system().name());
        if (locale_override.length() >0 )
            locale = locale_override;
        if (locale != "C") {
            QString loc("muse_");
            loc += locale;
            if (translator.load(loc, QString(".")) == false) {
                  QString lp(MusEGlobal::museGlobalShare);
                  lp += QString("/locale");
                  if (translator.load(loc, lp) == false) {
                        fprintf(stderr, "no locale <%s>/<%s>\n", loc.toLatin1().constData(), lp.toLatin1().constData());
                  }
            }
            app.installTranslator(&translator);
        }

        QLocale def_loc(locale);
        QLocale::setDefault(def_loc);

        fprintf(stderr, "LOCALE %s\n",QLocale().name().toLatin1().data());

        if (QLocale().name() == "de" || locale_override == "de") {
          fprintf(stderr, "locale de - setting 'note h is B' override parameter.\n");
          MusEGlobal::hIsB = false;
        }

        QApplication::addLibraryPath(MusEGlobal::museGlobalLib + "/qtplugins");
        if (MusEGlobal::debugMsg) {
              QStringList list = app.libraryPaths();
              QStringList::Iterator it = list.begin();
              fprintf(stderr, "QtLibraryPath:\n");
              while(it != list.end()) {
                    fprintf(stderr, "  <%s>\n", (*it).toLatin1().constData());
                    ++it;
                    }
              }

        // Create user templates dir if it doesn't exist
        QDir utemplDir = QDir(MusEGlobal::configPath + QString("/templates"));
        if(!utemplDir.exists())
        {
          utemplDir.mkpath(".");
          // Support old versions: Copy existing templates over.
          QDir old_utemplDir = QDir(QString(getenv("HOME")) + QString("/templates"));
          if(old_utemplDir.exists())
          {
            // We really just want these, even though it's possible other filenames were saved.
            // Another application might have used that directory.
            QStringList flt;
            flt << "*.med" << "*.med.gz" << "*.med.bz2" << "*.mid" << "*.midi" << "*.kar";
            old_utemplDir.setNameFilters(flt);

            QFileInfoList fil = old_utemplDir.entryInfoList();
            QFileInfo fi;
            foreach(fi, fil)
            {
              QString fn = fi.fileName();
              QFile f(fi.absoluteFilePath());
              f.copy(utemplDir.absolutePath() + "/" + fn);
            }
          }
        }

        // Create user instruments dir if it doesn't exist
        QString uinstrPath = MusEGlobal::configPath + QString("/instruments");
        QDir uinstrDir = QDir(uinstrPath);
        if(!uinstrDir.exists())
          uinstrDir.mkpath(".");
        if(!MusEGlobal::config.userInstrumentsDir.isEmpty() && MusEGlobal::config.userInstrumentsDir != uinstrPath)  // Only if it is different.
        {
          // Support old versions: Copy existing instruments over.
          QDir old_uinstrDir(MusEGlobal::config.userInstrumentsDir);
          if(old_uinstrDir.exists())
          {
            QStringList flt; flt << "*.idf";
            old_uinstrDir.setNameFilters(flt);

            QFileInfoList fil = old_uinstrDir.entryInfoList();
            QFileInfo fi;
            foreach(fi, fil)
            {
              QString fn = fi.fileName();
              QFile f(fi.absoluteFilePath());
              QFile newf(uinstrDir.absolutePath() + "/" + fn);
              if(!newf.exists())
              {
                f.copy(newf.fileName());
              }
            }
          }
        }
        MusEGlobal::museUserInstruments = uinstrPath;

        AL::initDsp();
        MusECore::initAudio();

        MusEGui::initIcons();

        MusECore::initMidiSynth(); // Need to do this now so that Add Track -> Synth menu is populated when MusE is created.

        MusEGlobal::muse = new MusEGui::MusE();
        app.setMuse(MusEGlobal::muse);

        MusEGui::init_function_dialogs();
        MusEGui::retranslate_function_dialogs();

        // SHOW MUSE SPLASH SCREEN
        if (MusEGlobal::config.showSplashScreen) {
            QPixmap splsh(MusEGlobal::museGlobalShare + "/splash.png");

            if (!splsh.isNull()) {
                QSplashScreen* muse_splash = new QSplashScreen(splsh,
                  Qt::WindowStaysOnTopHint);
                muse_splash->setAttribute(Qt::WA_DeleteOnClose);  // Possibly also Qt::X11BypassWindowManagerHint
                muse_splash->show();
                muse_splash->showMessage("MusE " + QString(VERSION) );
                QTimer* stimer = new QTimer(0);
                muse_splash->connect(stimer, SIGNAL(timeout()), muse_splash, SLOT(close()));
                stimer->setSingleShot(true);
                stimer->start(6000);
                QApplication::processEvents();
            }
        }

        if (MusEGlobal::config.useDenormalBias) {
            fprintf(stderr, "Denormal protection enabled.\n");
        }
        if (MusEGlobal::debugMsg) {
            fprintf(stderr, "global lib:       <%s>\n", MusEGlobal::museGlobalLib.toLatin1().constData());
            fprintf(stderr, "global share:     <%s>\n", MusEGlobal::museGlobalShare.toLatin1().constData());
            fprintf(stderr, "muse home:        <%s>\n", MusEGlobal::museUser.toLatin1().constData());
            fprintf(stderr, "project dir:      <%s>\n", MusEGlobal::museProject.toLatin1().constData());
            fprintf(stderr, "user instruments: <%s>\n", MusEGlobal::museUserInstruments.toLatin1().constData());
        }

        //rlimit lim; getrlimit(RLIMIT_RTPRIO, &lim);
        //fprintf(stderr, "RLIMIT_RTPRIO soft:%d hard:%d\n", lim.rlim_cur, lim.rlim_max);    // Reported 80, 80 even with non-RT kernel.
        if (MusEGlobal::realTimePriority < sched_get_priority_min(SCHED_FIFO))
              MusEGlobal::realTimePriority = sched_get_priority_min(SCHED_FIFO);
        else if (MusEGlobal::realTimePriority > sched_get_priority_max(SCHED_FIFO))
              MusEGlobal::realTimePriority = sched_get_priority_max(SCHED_FIFO);
        // If we requested to force the midi thread priority...
        if(MusEGlobal::midiRTPrioOverride > 0)
        {
          if (MusEGlobal::midiRTPrioOverride < sched_get_priority_min(SCHED_FIFO))
              MusEGlobal::midiRTPrioOverride = sched_get_priority_min(SCHED_FIFO);
          else if (MusEGlobal::midiRTPrioOverride > sched_get_priority_max(SCHED_FIFO))
              MusEGlobal::midiRTPrioOverride = sched_get_priority_max(SCHED_FIFO);
        }

#ifdef HAVE_LASH
        bool using_jack = false;
#endif
        if (MusEGlobal::debugMode) {
            MusEGlobal::realTimeScheduling = false;
            MusECore::initDummyAudio();
        }
        else if (audioType == DummyAudioOverride) {
            fprintf(stderr, "Force Dummy Audio driver\n");
            MusEGlobal::realTimeScheduling = true;
            MusECore::initDummyAudio();
        }
#ifdef HAVE_RTAUDIO
        else if (audioType == RtAudioOverride) {
            fprintf(stderr, "Force RtAudio with Pulse Backend\n");
            MusEGlobal::realTimeScheduling = true;
            if(MusECore::initRtAudio(true))
              fallbackDummy();
            else
              fprintf(stderr, "Using rtAudio\n");
        }
#endif
        else if (audioType == JackAudioOverride) {
          if(MusECore::initJackAudio()) 
            fallbackDummy();
          else
          {
#ifdef HAVE_LASH
            using_jack = true;
#endif
            fprintf(stderr, "...Using Jack\n");
          }
        }
        else if (audioType == DriverConfigSetting) {
          fprintf(stderr, "Select audio device from configuration : %d\n", MusEGlobal::config.deviceAudioBackend);
          switch (MusEGlobal::config.deviceAudioBackend) {
            case MusEGlobal::DummyAudio:
              {
                fprintf(stderr, "User DummyAudio backend - selected through configuration\n");
                MusEGlobal::realTimeScheduling = true;
                MusECore::initDummyAudio();
                break;
              }
            case MusEGlobal::RtAudioAlsa:
            case MusEGlobal::RtAudioOss:
//            case MusEGlobal::RtAudioJack:
            case MusEGlobal::RtAudioChoice:
            case MusEGlobal::RtAudioPulse:
              {
                fprintf(stderr, "User RtAudio backend - backend selected through configuration: ");
                if(MusEGlobal::config.deviceAudioBackend >= MusEGlobal::numRtAudioDevices)
                  fprintf(stderr, "Unknown");
                else
                  fprintf(stderr, "%s",
                    MusEGlobal::selectableAudioBackendDevices[MusEGlobal::config.deviceAudioBackend].
                      toLatin1().constData());
                fprintf(stderr, "\n");

                MusEGlobal::realTimeScheduling = true;
                if(MusECore::initRtAudio())
                  fallbackDummy();
                else
                  fprintf(stderr, "Using rtAudio\n");
              
                break;
              }
            case MusEGlobal::JackAudio:
              {
                fprintf(stderr, "User JackAudio backend - backend selected through configuration\n");
                if (MusECore::initJackAudio()) 
                {
                  MusEGlobal::realTimeScheduling = true;
                  // Force default Pulse.
                  if(MusECore::initRtAudio(true))
                    fallbackDummy();
                  else
                    fprintf(stderr, "Using rtAudio Pulse\n");
                }
                else
                {
#ifdef HAVE_LASH
                  using_jack = true;
#endif
                  fprintf(stderr, "Using Jack\n");
                }
                
                break;
              }
          }
        }

        MusEGlobal::realTimeScheduling = MusEGlobal::audioDevice->isRealtime();

        // ??? With Jack2 this reports true even if it is not running realtime.
        // Jack says: "Cannot use real-time scheduling (RR/10)(1: Operation not permitted)". The kernel is non-RT.
        // I cannot seem to find a reliable answer to the question, even with dummy audio and system calls.

        MusEGlobal::useJackTransport.setValue(true);

        // setup the prefetch fifo length now that the segmentSize is known
        MusEGlobal::fifoLength = 131072 / MusEGlobal::segmentSize;
        MusECore::initAudioPrefetch();

        // WARNING Must do it this way. Call registerClient long AFTER Jack client is created and MusE ALSA client is
        // created (in initMidiDevices), otherwise random crashes can occur within Jack <= 1.9.8. Fixed in Jack 1.9.9.  Tim.
        // This initMidiDevices will automatically initialize the midiSeq sequencer thread, but not start it - that's a bit later on.
        MusECore::initMidiDevices();
        // Wait until things have settled. One second seems OK so far.
        for(int t = 0; t < 100; ++t)
          usleep(10000);
        // Now it is safe to call registerClient.
        MusEGlobal::audioDevice->registerClient();

        MusECore::initMidiController();
        MusECore::initMidiInstruments();
        MusECore::initMidiPorts();

        if (MusEGlobal::loadPlugins)
              MusECore::initPlugins();

        if (MusEGlobal::loadVST)
              MusECore::initVST();

        if (MusEGlobal::loadNativeVST)
              MusECore::initVST_Native();

        if(MusEGlobal::loadDSSI)
              MusECore::initDSSI();
  #ifdef LV2_SUPPORT
        if(MusEGlobal::loadLV2)
              MusECore::initLV2();
  #endif

        MusECore::initOSC();

        MusECore::initMetronome();

        MusECore::initWavePreview();

        MusECore::enumerateJackMidiDevices();

  #ifdef HAVE_LASH
        {
          MusEGui::lash_client = 0;
          if(MusEGlobal::useLASH)
          {
            int lash_flags = LASH_Config_File;
            const char *muse_name = PACKAGE_NAME;
            MusEGui::lash_client = lash_init (lash_args, muse_name, lash_flags, LASH_PROTOCOL(2,0));
  #ifdef ALSA_SUPPORT
            if(MusECore::alsaSeq)
              lash_alsa_client_id (MusEGui::lash_client, snd_seq_client_id (MusECore::alsaSeq));
  #endif
            //if (audioType != DummyAudio) {
            if (using_jack) {
                  const char *jack_name = MusEGlobal::audioDevice->clientName();
                  lash_jack_client_name (MusEGui::lash_client, jack_name);
            }
          }
          if(lash_args)
            lash_args_destroy(lash_args);
        }
  #endif /* HAVE_LASH */

        if (!MusEGlobal::debugMode) {
              if (mlockall(MCL_CURRENT | MCL_FUTURE))
                    perror("WARNING: Cannot lock memory:");
              }

        MusEGlobal::muse->show();

        // Let the configuration settings take effect. Do not save.
        MusEGlobal::muse->changeConfig(false);
        // Set style and stylesheet, and do not force the style
        //MusEGui::updateThemeAndStyle(); // Works better if called just after app created, above.

        MusEGlobal::muse->seqStart();
        
        // If the sequencer object was created, report timing.
        if(MusEGlobal::midiSeq)
          MusEGlobal::midiSeq->checkAndReportTimingResolution();

        //--------------------------------------------------
        // Auto-fill the midi ports, if appropriate.
        //--------------------------------------------------
        if(MusEGlobal::populateMidiPortsOnStart &&
          argc_copy < 2 &&
          (MusEGlobal::config.startMode == 1 || MusEGlobal::config.startMode == 2) &&
          !MusEGlobal::config.startSongLoadConfig)
          MusECore::populateMidiPorts();

        //--------------------------------------------------
        // Load the default song.
        //--------------------------------------------------
        MusEGlobal::muse->loadDefaultSong(argc_copy, &argv_copy[optind]);

        QTimer::singleShot(100, MusEGlobal::muse, SLOT(showDidYouKnowDialog()));

        rv = app.exec();
        if(MusEGlobal::debugMsg)
          fprintf(stderr, "app.exec() returned:%d\nDeleting main MusE object\n", rv);

        if (MusEGlobal::loadPlugins)
        {
          for (MusECore::iPlugin i = MusEGlobal::plugins.begin(); i != MusEGlobal::plugins.end(); ++i)
              delete (*i);
          MusEGlobal::plugins.clear();
        }

        MusECore::exitWavePreview();

  #ifdef LV2_SUPPORT
        if(MusEGlobal::loadLV2)
              MusECore::deinitLV2();
  #endif

        // In case the sequencer object is still alive, make sure to destroy it now.
        MusECore::exitMidiSequencer();

        // Grab the restart flag before deleting muse.
        is_restarting = MusEGlobal::muse->restartingApp();

        // Now delete the application.
        delete MusEGlobal::muse;

        // These are owned by muse and deleted above. Reset to zero now.
        MusEGlobal::undoRedo = 0;
        MusEGlobal::undoAction = 0;
        MusEGlobal::redoAction = 0;

        // Reset the option index.
        // NOTE: See optind manual for special resetting values.
        //       Traditionally 1 is set, but here we may need GNU specific 0.
        optind = 0;

        // Free the working copies of the arguments.
        if(argv_copy)
        {
          for(int i = 0; i < argument_count; ++i)
          {
            if(argv_copy[i])
              free(argv_copy[i]);
          }
          free(argv_copy);
        }

        // Reset these before restarting, seems to work better, 
        //  makes a difference with the MDI freezing problem, above.
        app.setStyleSheet("");
        app.setStyle(MusEGlobal::config.style);
        
        // Reset the recently opened list.
        MusEGui::projectRecentList.clear();
      }

      // REMOVE Tim. samplerate. Added.
      if(MusEGlobal::defaultAudioConverterSettings)
        delete MusEGlobal::defaultAudioConverterSettings;
      
      if(MusEGlobal::debugMsg) 
        fprintf(stderr, "Finished! Exiting main, return value:%d\n", rv);
      return rv;
      
      }
