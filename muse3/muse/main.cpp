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
#include <QCommandLineParser>
#include <QCommandLineOption>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QFileInfoList>
#include <QDirIterator>
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
#include <QStandardPaths> 

#include <iostream>

#include <time.h>
#ifndef _WIN32
#include <sys/mman.h>
#endif

#include "config.h"

#ifdef ALSA_SUPPORT
#include <alsa/asoundlib.h>
#endif

#include "al/al.h"
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
#include "plugin_cache_writer.h"
#include "pluglist.h"
#include "metronome_class.h"
#include "audio_convert/audio_converter_plugin.h"
#include "audio_convert/audio_converter_settings_group.h"
#include "wave.h"

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
void initIcons(int cursorSize);
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
//   MuseApplication
//---------------------------------------------------------

class MuseApplication : public QApplication {
      MusEGui::MusE* muse;

   public:
      MuseApplication(int& argc, char** argv)
         : QApplication(argc, argv)
            {
            muse = nullptr;
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
         const bool flag = QApplication::notify(receiver, event);
         const QEvent::Type type = event->type();
         if (type == QEvent::KeyPress) {
            const QMetaObject * mo = receiver->metaObject();
            if (mo){
               if (strcmp(mo->className(), "QWidgetWindow") == 0)
                 return false;
            }
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
               if(muse)
                 muse->kbAccel(key);
               return true;
            }
         }
         else if (type == QEvent::KeyRelease) {
            QKeyEvent* ke = (QKeyEvent*)event;
            ///MusEGlobal::globalKeyState = ke->stateAfter();
            MusEGlobal::globalKeyState = ke->modifiers();
         }
         
         return flag;
      }

#ifdef HAVE_LASH
     virtual void timerEvent (QTimerEvent*) {
            if(muse && MusEGlobal::useLASH)
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

void fallbackDummy() {

  fprintf(stderr, "Falling back to dummy audio driver\n");
  QMessageBox::critical(NULL, "MusE fatal error", "MusE <b>failed</b> to find selected <b>audio server</b>.<br><br>"
                                                  "<i>MusE will continue <b>without audio support</b> (-a switch)!</i>");
  MusEGlobal::realTimeScheduling = true;
  MusECore::initDummyAudio();
}

//---------------------------------------------------------
//   printExtraHelpText
//---------------------------------------------------------

static void printExtraHelpText()
      {
      printf("\n");
#ifdef HAVE_LASH
      printf("LASH and ");
#endif
      printf("Qt options are also accepted. Some common Qt options:\n");
      printf("   -style [=] style           Set application GUI style. Motif, Windows, Platinum etc.\n"
                      "   -stylesheet [=] stylesheet Set application styleSheet\n" 
                      "   -session [=] session       Restore application from an earlier session\n"
                      "   -widgetcount               Print debug message at end, about undestroyed/maximum widgets\n"
                      "   -reverse                   Set application's layout direction to Qt::RightToLeft\n"
                      "   -graphicssystem            Set backend used for on-screen widgets/QPixmaps: raster or opengl\n"
                      "   -qmljsdebugger = port      Activate QML/JS debugger with port, formatted port:1234[,block]\n" 
      );

      printf("\n");

      printf("Some useful environment variables:\n\n");
      printf("   LANG: Help browser language suffix (en etc.)\n\n");
      printf("These variables are read ONCE upon first-time run, to fill the Plugin Paths\n"
                      " in Global Settings. Afterwards the paths can be altered in Global Settings:\n\n");
      printf("   LADSPA_PATH: Override where to look for ladspa plugins, or else\n"
                      "     ~/ladspa:~/.ladspa:/usr/local/lib64/ladspa:/usr/lib64/ladspa:/usr/local/lib/ladspa:/usr/lib/ladspa\n\n");
#ifdef DSSI_SUPPORT
      printf("   DSSI_PATH: Override where to look for dssi plugins, or else\n"
                      "     ~/dssi:~/.dssi:/usr/local/lib64/dssi:/usr/lib64/dssi:/usr/local/lib/dssi:/usr/lib/dssi\n\n" );      
      printf("   VST_PATH: Override where dssi-vst (if installed) looks for Wine vst plugins, or else\n"
                      "     ~/vst win 32bit:~/.vst win 32bit or ~/vst:~/.vst on windows\n\n");
#endif
#ifdef VST_NATIVE_SUPPORT
      printf("   LXVST_PATH: Override where to look for Linux vst plugins, or else VST_PATH, or else\n"
                      "     ~/lxvst:~/.lxvst:/usr/local/lib64/lxvst:/usr/local/lib/lxvst:/usr/lib64/lxvst:/usr/lib/lxvst\n"
                      "     also on Linux ~/vst:~/.vst:/usr/local/lib64/vst:/usr/local/lib/vst:/usr/lib64/vst:/usr/lib/vst\n\n");
#endif
#ifdef LV2_SUPPORT
      printf("   LV2_PATH: Override where to look for LV2 plugins or else\n"
                      "     ~/.lv2:/usr/local/lib/lv2:/usr/lib/lv2\n\n");
#endif
      }

enum CommandLineParseResult
{
    CommandLineOk,
    CommandLineError,
    CommandLineVersionRequested,
    CommandLineHelpRequested
};

CommandLineParseResult parseCommandLine(
  QCommandLineParser &parser, QString *errorMessage,
  QString& open_filename, AudioDriverSelect& audioType, bool& force_plugin_rescan, bool& dont_plugin_rescan)
{
  parser.setApplicationDescription(APP_DESCRIPTION);
  const QString version_string(VERSION);
  const QString git_string(GITSTRING);
  if(git_string.isEmpty())
    QCoreApplication::setApplicationVersion(version_string);
  else
    QCoreApplication::setApplicationVersion(version_string + ", (" + git_string + ")");
  const QCommandLineOption helpOption = parser.addHelpOption();
  const QCommandLineOption versionOption = parser.addVersionOption();

  parser.addPositionalArgument("filename", QCoreApplication::translate("main", "File to open"));

  QCommandLineOption option_a("a", QCoreApplication::translate("main", "Alsa midi only (using dummy audio driver)"));
  parser.addOption(option_a);

#ifdef HAVE_RTAUDIO      
  QCommandLineOption option_t("t", QCoreApplication::translate("main", "Use RtAudio driver"));
  parser.addOption(option_t);
#endif                   
  QCommandLineOption option_j("j", QCoreApplication::translate("main", "Use JAckAudio driver to connect to Jack audio server"));
  parser.addOption(option_j);
  QCommandLineOption option_J("J", QCoreApplication::translate("main", "Do not try to auto-start the Jack audio server"));
  parser.addOption(option_J);
  QCommandLineOption option_F("F", QCoreApplication::translate("main",
    "Do not auto-populate midi ports with midi devices found, at startup"));
  parser.addOption(option_F);
  QCommandLineOption option_A("A", QCoreApplication::translate("main", "Force inclusion of ALSA midi even if using Jack"));
  parser.addOption(option_A);
  QCommandLineOption option_P("P", QCoreApplication::translate("main",
    "Set audio driver real time priority to n (Dummy only, default 40. Else fixed by Jack.)"), "n");
  parser.addOption(option_P);
  QCommandLineOption option_Y("Y", QCoreApplication::translate("main",
    "Force midi real time priority to n (default: audio driver prio -1)\n"), "n");
  parser.addOption(option_Y);

  QCommandLineOption option_R("R", QCoreApplication::translate("main",
    "Force plugin cache re-creation. (Automatic if any plugin path directories changed.)"));
  parser.addOption(option_R);
  QCommandLineOption option_C("C", QCoreApplication::translate("main",
    "Do not re-create plugin cache. Avoids repeated re-creations in some circumstances. Use with care."));
  parser.addOption(option_C);
  QCommandLineOption option_p("p", QCoreApplication::translate("main", "Don't load LADSPA plugins"));
  parser.addOption(option_p);
  QCommandLineOption option_S("S", QCoreApplication::translate("main", "Don't load MESS plugins"));
  parser.addOption(option_S);
#ifdef VST_SUPPORT
  QCommandLineOption option_V("V", QCoreApplication::translate("main", "Don't load VST plugins"));
  parser.addOption(option_V);
#endif
#ifdef VST_NATIVE_SUPPORT
  QCommandLineOption option_N("N", QCoreApplication::translate("main", "Don't load LinuxVST plugins"));
  parser.addOption(option_N);
#endif
#ifdef DSSI_SUPPORT
  QCommandLineOption option_I("I", QCoreApplication::translate("main", "Don't load DSSI plugins"));
  parser.addOption(option_I);
#endif
#ifdef LV2_SUPPORT
  QCommandLineOption option_2("2", QCoreApplication::translate("main", "Don't load LV2 plugins"));
  parser.addOption(option_2);
#endif
#ifdef HAVE_LASH
  QCommandLineOption option_L("L", QCoreApplication::translate("main", "Don't use LASH"));
  parser.addOption(option_L);
#endif

  QCommandLineOption option_l(QCommandLineOption("l", QCoreApplication::translate("main",
    "Force locale to the given language/country code (xx = ") + localeList() + ")",  "xx"));
  parser.addOption(option_l);
  QCommandLineOption option_u("u", QCoreApplication::translate("main",
    "Ubuntu/unity workaround: don't allow sharing menus and mdi-subwins."));
  parser.addOption(option_u);
  QCommandLineOption option_d("d", QCoreApplication::translate("main", "Debug mode: no threads, no RT"));
  parser.addOption(option_d);
  QCommandLineOption option_D("D", QCoreApplication::translate("main",
    "Debug mode: enable some debug messages specify twice for lots of debug messages this may slow down MusE massively!"));
  parser.addOption(option_D);
  QCommandLineOption option_m("m", QCoreApplication::translate("main", "Debug mode: trace midi Input"));
  parser.addOption(option_m);
  QCommandLineOption option_M("M", QCoreApplication::translate("main", "Debug mode: trace midi Output"));
  parser.addOption(option_M);
  QCommandLineOption option_s("s", QCoreApplication::translate("main", "Debug mode: trace sync\n"));
  parser.addOption(option_s);

#ifdef PYTHON_SUPPORT
  QCommandLineOption option_y("y", QCoreApplication::translate("main", "Enable Python control support")); 
  parser.addOption(option_y);
  QCommandLineOption option_pyro_ns_host("pyro-ns-host",
    QCoreApplication::translate("main", "Pyro nameserver host name"), "hostname");
  parser.addOption(option_pyro_ns_host);
  QCommandLineOption option_pyro_ns_port("pyro-ns-port",
    QCoreApplication::translate("main", "Pyro nameserver host port"), "port");
  parser.addOption(option_pyro_ns_port);
  QCommandLineOption option_pyro_daemon_host("pyro-daemon-host",
    QCoreApplication::translate("main", "Pyro daemon host name"), "hostname");
  parser.addOption(option_pyro_daemon_host);
  QCommandLineOption option_pyro_daemon_port("pyro-daemon-port",
    QCoreApplication::translate("main", "Pyro daemon host port"), "port");
  parser.addOption(option_pyro_daemon_port);
  QCommandLineOption option_pyro_comm_timeout("pyro-comm-timeout",
    QCoreApplication::translate("main", "Pyro communication timeout in seconds"), "timeout");
  parser.addOption(option_pyro_comm_timeout);
#endif

  if(!parser.parse(QCoreApplication::arguments()))
  {
    *errorMessage = parser.errorText();
    return CommandLineError;
  }
  
  if(parser.isSet(versionOption))
    return CommandLineVersionRequested;

  if(parser.isSet(helpOption))
    return CommandLineHelpRequested;

  const QStringList used_positional_args = parser.positionalArguments();
  const int used_positional_args_sz = used_positional_args.size();
  if(used_positional_args_sz > 1)
  {
    *errorMessage = "Error: Expected only one positional argument";
    return CommandLineError;
  }
  else if(used_positional_args_sz == 1)
  {
    open_filename = used_positional_args.first();
  }

  if(parser.isSet(option_a))
    audioType = DummyAudioOverride;
        
  if(parser.isSet(option_l))
    locale_override = parser.value(option_l);

#ifdef HAVE_RTAUDIO
  if(parser.isSet(option_t))
    audioType = RtAudioOverride;
#endif
  
  if(parser.isSet(option_J))
    MusEGlobal::noAutoStartJack = true;

  if(parser.isSet(option_j))
    audioType = JackAudioOverride;

  if(parser.isSet(option_F))
    MusEGlobal::populateMidiPortsOnStart = false;

  if(parser.isSet(option_A))
    MusEGlobal::useAlsaWithJack = true;

  if(parser.isSet(option_d))
  {
    MusEGlobal::debugMode = true;
    MusEGlobal::realTimeScheduling = false;
  }

  if(parser.isSet(option_D))
  {
    if(!MusEGlobal::debugMsg)
      MusEGlobal::debugMsg=true;
    else
      MusEGlobal::heavyDebugMsg=true;
  }

  if(parser.isSet(option_m))
    MusEGlobal::midiInputTrace = true;

  if(parser.isSet(option_M))
    MusEGlobal::midiOutputTrace = true;

  if(parser.isSet(option_s))
    MusEGlobal::debugSync = true;

  if(parser.isSet(option_u))
    MusEGlobal::unityWorkaround = true;

  if(parser.isSet(option_P))
    MusEGlobal::realTimePriority = parser.value(option_P).toInt();

  if(parser.isSet(option_Y))
    MusEGlobal::midiRTPrioOverride = parser.value(option_Y).toInt();

  if(parser.isSet(option_p))
    MusEGlobal::loadPlugins = false;

  if(parser.isSet(option_R))
    force_plugin_rescan = true;

  if(parser.isSet(option_C))
    dont_plugin_rescan = true;

  if(parser.isSet(option_S))
    MusEGlobal::loadMESS = false;

#ifdef VST_SUPPORT
  if(parser.isSet(option_V))
    MusEGlobal::loadVST = false;
#endif

#ifdef VST_NATIVE_SUPPORT
  if(parser.isSet(option_N))
    MusEGlobal::loadNativeVST = false;
#endif

#ifdef DSSI_SUPPORT
  if(parser.isSet(option_I))
    MusEGlobal::loadDSSI = false;
#endif

#ifdef HAVE_LASH
  if(parser.isSet(option_L))
    MusEGlobal::useLASH = false;
#endif

#ifdef LV2_SUPPORT
  if(parser.isSet(option_2))
    MusEGlobal::loadLV2 = false;
#endif

#ifdef PYTHON_SUPPORT
  if(parser.isSet(option_y))
  {
    MusEGlobal::usePythonBridge = true;

    if(parser.isSet(option_pyro_ns_host))
      MusEGlobal::pythonBridgePyroNSHostname = parser.value(option_pyro_ns_host);

    if(parser.isSet(option_pyro_ns_port))
      MusEGlobal::pythonBridgePyroNSPort = parser.value(option_pyro_ns_port);

    if(parser.isSet(option_pyro_daemon_host))
      MusEGlobal::pythonBridgePyroDaemonHostname = parser.value(option_pyro_daemon_host);

    if(parser.isSet(option_pyro_daemon_port))
      MusEGlobal::pythonBridgePyroDaemonPort = parser.value(option_pyro_daemon_port);

    if(parser.isSet(option_pyro_comm_timeout))
      MusEGlobal::pythonBridgePyroCommTimeout = parser.value(option_pyro_comm_timeout).toFloat();
  }
#endif

  return CommandLineOk;
}


//---------------------------------------------------------
//   main
//---------------------------------------------------------

int main(int argc, char* argv[])
      {
      // Get the separator used for file paths.
      const QChar list_separator = QDir::listSeparator();

      // Get environment variables for various paths.
      // "The Qt environment manipulation functions are thread-safe, but this requires that
      //   the C library equivalent functions like getenv and putenv are not directly called."
      // "Note: on desktop Windows, qgetenv() may produce data loss if the original string
      //   contains Unicode characters not representable in the ANSI encoding.
      //  Use qEnvironmentVariable() instead. On Unix systems, this function is lossless."
      #if (QT_VERSION >= QT_VERSION_CHECK(5, 10, 0))
        const QString ladspa_path = qEnvironmentVariable("LADSPA_PATH");
        const QString dssi_path = qEnvironmentVariable("DSSI_PATH");
        const QString vst_path = qEnvironmentVariable("VST_PATH");
        // This Linux VST path is known to be used by Ardour.
        const QString lxvst_path = qEnvironmentVariable("LXVST_PATH");
        const QString lv2_path = qEnvironmentVariable("LV2_PATH");
      #else
        // "To convert the data to a QString use QString::fromLocal8Bit()."
        const QString ladspa_path = QString::fromLocal8Bit(qgetenv("LADSPA_PATH"));
        const QString dssi_path = QString::fromLocal8Bit(qgetenv("DSSI_PATH"));
        const QString vst_path = QString::fromLocal8Bit(qgetenv("VST_PATH"));
        const QString lxvst_path = QString::fromLocal8Bit(qgetenv("LXVST_PATH"));
        const QString lv2_path = QString::fromLocal8Bit(qgetenv("LV2_PATH"));
      #endif


      QString last_project_filename;
      bool last_project_was_template = false;
      bool last_project_loaded_config = false;
      bool plugin_rescan_already_done = false;
      int rv = 0;

      //==============================================
      // BEGIN Restart loop. For (re)starting the app.
      //==============================================
      
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
        QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
        QCoreApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);

        //========================
        //  Application instance:
        //========================
        
        MuseApplication app(argc_copy, argv_copy);
        if(QStyle* def_style = app.style())
        {
          const QString appStyleObjName = def_style->objectName();
          MusEGui::Appearance::getSetDefaultStyle(&appStyleObjName);
        }

        app.setOrganizationName(ORGANIZATION_NAME);
        app.setOrganizationDomain(ORGANIZATION_DOMAIN);
        app.setApplicationName(PACKAGE_NAME);
        app.setApplicationDisplayName(APP_DISPLAY_NAME);

        // NOTE: 'GenericConfigLocation' returned config dir (ie. ~./config).
        //       'ConfigLocation' also returned config dir (ie. ~./config).
        //       'AppConfigLocation' (Qt 5.5) returned config + organization name + application name dirs
        //        (ie. ~./config/MusE/MusE-qt).
        //       Beware, setting application name and organization name influence these locations.

          // "Returns a directory location where user-specific configuration files should be written.
          //  This is an application-specific directory, and the returned path is never empty.
          //  This enum value was added in Qt 5.5."
        MusEGlobal::configPath = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);

        // "Returns a directory location where user-specific non-essential (cached) data should be written.
        //  This is an application-specific directory. The returned path is never empty."
        // NOTE: This returned cache + organization name + application name dirs (ie. ~./cache/MusE/MusE-qt).
        MusEGlobal::cachePath       = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);

        MusEGlobal::museUser        = QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
        MusEGlobal::museGlobalLib   = QString(LIBDIR);
        MusEGlobal::museGlobalShare = QString(SHAREDIR);

        MusEGlobal::museProject     = MusEGlobal::museProjectInitPath; //getcwd(0, 0);
        MusEGlobal::museInstruments = MusEGlobal::museGlobalShare + "/instruments";

        MusEGlobal::configName      = MusEGlobal::configPath + "/MusE-seq.cfg";

        const QString oldConfigPath(QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation)
          + "/" + PACKAGE_NAME);

        const QString old_qtconfig_name(oldConfigPath + "/MusE-qt.conf");
        const QString new_qtconfig_name(oldConfigPath + "/MusE.conf");

        const QString new_plugin_cache_path(MusEGlobal::cachePath + "/scanner");

        // If the new-style plugin cache directory location doesn't exist yet, and an
        //  old-style plugin cache directory exists, rename the old one to the new one.
        if(!MusEGlobal::cachePath.isEmpty())
        {
          QDir new_plugin_cache_dir(new_plugin_cache_path);
          if(!new_plugin_cache_dir.exists())
          {
            const QString old_plugin_cache_path(oldConfigPath + "/scanner");
            QDir old_plugin_cache_dir(old_plugin_cache_path);
            if(old_plugin_cache_dir.exists())
            {
              QDir(MusEGlobal::cachePath).mkpath(".");
              if(!QDir().rename(old_plugin_cache_path, new_plugin_cache_path))
                fprintf(stderr, "Error renaming plugin cache dir:<%s> to:<%s>\n",
                  old_plugin_cache_path.toLocal8Bit().constData(),
                  new_plugin_cache_path.toLocal8Bit().constData());
            }
          }
        }

        // If the new-style config directory location doesn't exist yet, and an
        //  old-style config directory exists, rename the old one to the new one.
        if(!MusEGlobal::configPath.isEmpty())
        {
          QDir new_config_dir(MusEGlobal::configPath);
          if(!new_config_dir.exists())
          {
            QFileInfoList fil;
            QDir old_config_dir(oldConfigPath);
            if(old_config_dir.exists())
              fil = old_config_dir.entryInfoList(QDir::AllEntries| QDir::NoDotAndDotDot);

            // Create the new directory AFTER grabbing the existing list so that
            //  if the new directory is a subdirectory of the existing one,
            //  it will not show up in the list.
            new_config_dir.mkpath(".");

            if(!fil.isEmpty())
            {
              QFileInfo fi;
              foreach(fi, fil)
              {
                const QString afp(fi.absoluteFilePath());
                // DO NOT move the old MusE-qt config file.
                // Given an organization name and application name, that is where
                //  QSettings are stored it (ie. ~/.config/MusE not ~/.config/MusE/MusE).
                if(afp == old_qtconfig_name)
                  continue;
                const QString fn = fi.fileName();
                const QString new_fn(MusEGlobal::configPath + "/" + fn);
                if(fi.isDir())
                {
                  if(!QDir().rename(afp, new_fn))
                    fprintf(stderr, "Error renaming config dir:<%s> to:<%s>\n",
                      afp.toLocal8Bit().constData(), new_fn.toLocal8Bit().constData());
                }
                else
                {
                  QFile f(afp);
                  if(!f.rename(new_fn))
                    fprintf(stderr, "Error renaming config file:<%s> to:<%s>\n",
                      afp.toLocal8Bit().constData(), new_fn.toLocal8Bit().constData());
                }
              }
            }
          }
        }

        {
          const QString old_config_name(MusEGlobal::configPath + "/MusE.cfg");
          // Rename existing config file to new name.
          QFile oldConfigFile(old_config_name);
          if(oldConfigFile.exists())
            oldConfigFile.rename(MusEGlobal::configName);
        }

        bool cConfExists = false;
        {
          QFile cConf (MusEGlobal::configName);
          cConfExists = cConf.exists();
        }

// NOTE: This section was meant to provide a sane way for devs to easily
//        change default settings of all kinds.
//       But this scheme causes some problems. See the comments at the top
//        of gconfig.cpp for details.
#if 0
        if (!cConfExists)
        {
          QFile cConfTempl (MusEGlobal::museGlobalShare + QString("/templates/MusE-seq.cfg"));
          fprintf(stderr, "creating new config...\n");
          if (cConfTempl.copy(MusEGlobal::configName))
            fprintf(stderr, "  success.\n");
          else
            fprintf(stderr, "  FAILED!\n");
        }
#endif

        {
          QFile oldQtConfigFile(old_qtconfig_name);
          if(oldQtConfigFile.exists())
            oldQtConfigFile.rename(new_qtconfig_name);
        }

// NOTE: This section was meant to provide some sane defaults for
//        some of the window sizes and visibility.
//       But it's a risky thing - what if Qt's binary format changes?
//       Then new versions of the file have to be created and it's
//        just too much for an 'unaware' coder to have to remember.
//       We should be able to make the classes do whatever is being
//        done here to provide default sizes, positions, and visiblilty.
#if 0
        QFile cConfQt (new_qtconfig_name);
        if (! cConfQt.exists())
        {
          QFile cConfTemplQt (MusEGlobal::museGlobalShare + QString("/templates/MusE.conf"));
          fprintf(stderr, "creating new qt config...\n");
          if (cConfTemplQt.copy(cConfQt.fileName()))
            fprintf(stderr, "  success.\n");
          else
            fprintf(stderr, "  FAILED!\n");
        }
#endif

        // User instruments dir:
        MusEGlobal::museUserInstruments = MusEGlobal::configPath + "/instruments";
        // Create user instruments dir if it doesn't exist
        {
          QDir uinstrDir = QDir(MusEGlobal::museUserInstruments);
          if(!uinstrDir.exists())
          {
            fprintf(stderr, "User instrument directory does not exist. Creating it.\n");
            uinstrDir.mkpath(".");
          }
        }

        MusEGui::initShortCuts();

        // Discover available MusE audio converters, before reading configuration
        MusEGlobal::audioConverterPluginList.discover(MusEGlobal::museGlobalLib, MusEGlobal::debugMsg);
        // Default, non-local settings.
        MusEGlobal::defaultAudioConverterSettings = new MusECore::AudioConverterSettingsGroup(false);
        MusEGlobal::defaultAudioConverterSettings->populate(&MusEGlobal::audioConverterPluginList, false);
        
        MusECore::readConfiguration();

        // Need to put a sane defaults here because we can't use '~' in the file name strings.
        if(!cConfExists)
        {
          MusEGlobal::config.projectBaseFolder = MusEGlobal::museUser + QString("/MusE");
          MusEGlobal::config.startSong = "";
        }

        app.instance()->setAttribute(Qt::AA_DontShowIconsInMenus, !MusEGlobal::config.showIconsInMenus);

        //=================
        //  LADSPA paths:
        //=================
        bool found = false;
        if(MusEGlobal::config.pluginLadspaPathList.isEmpty())
        {
          if(ladspa_path.isEmpty())
          {
            MusEGlobal::config.pluginLadspaPathList << 
              (MusEGlobal::museUser + QString("/ladspa")) <<
              (MusEGlobal::museUser + QString("/.ladspa")) <<
              QString("/usr/local/lib64/ladspa") <<
              QString("/usr/local/lib/ladspa") <<
              QString("/usr/lib64/ladspa") <<
              QString("/usr/lib/ladspa");
          }
          else
          {
// QString::*EmptyParts is deprecated, use Qt::*EmptyParts, new as of 5.14.
#if QT_VERSION >= 0x050e00
            MusEGlobal::config.pluginLadspaPathList = ladspa_path.split(list_separator, Qt::SkipEmptyParts);
#else
            MusEGlobal::config.pluginLadspaPathList = ladspa_path.split(list_separator, QString::SkipEmptyParts);
#endif
            found = true;
          }
        }
        if(!found && qputenv("LADSPA_PATH", MusEGlobal::config.pluginLadspaPathList.join(list_separator).toLocal8Bit()) == 0)
          fprintf(stderr, "Error setting LADSPA_PATH\n");

        //===============
        //  DSSI paths:
        //===============
        found = false;
        if(MusEGlobal::config.pluginDssiPathList.isEmpty())
        {
          if(dssi_path.isEmpty())
          {
            MusEGlobal::config.pluginDssiPathList << 
              (MusEGlobal::museUser + QString("/dssi")) <<
              (MusEGlobal::museUser + QString("/.dssi")) <<
              QString("/usr/local/lib64/dssi") <<
              QString("/usr/local/lib/dssi") <<
              QString("/usr/lib64/dssi") <<
              QString("/usr/lib/dssi");
          }
          else
          {
// QString::*EmptyParts is deprecated, use Qt::*EmptyParts, new as of 5.14.
#if QT_VERSION >= 0x050e00
            MusEGlobal::config.pluginDssiPathList = dssi_path.split(list_separator, Qt::SkipEmptyParts);
#else
            MusEGlobal::config.pluginDssiPathList = dssi_path.split(list_separator, QString::SkipEmptyParts);
#endif
            found = true;
          }
        }
        if(!found && qputenv("DSSI_PATH", MusEGlobal::config.pluginDssiPathList.join(list_separator).toLocal8Bit()) == 0)
          fprintf(stderr, "Error setting DSSI_PATH\n");

        //=======================
        //  Win VST (*.dll) paths:
        //=======================
        found = false;
        if(MusEGlobal::config.pluginVstPathList.isEmpty())
        {
          if(vst_path.isEmpty())
          {
            MusEGlobal::config.pluginVstPathList << 
// On win, vst is usually where *.dll files are found. We don't want that with Linux *.so vst files.
// Otherwise on Linux for example, vst is where Linux vst *.so files are found.
#ifdef Q_OS_WIN
              // TODO: Refine this for Q_OS_WIN. Where exactly do we look though?
              (MusEGlobal::museUser + QString("/vst")) <<
              (MusEGlobal::museUser + QString("/.vst"));
#else
              (MusEGlobal::museUser + QString("/vst win 32bit")) <<
              (MusEGlobal::museUser + QString("/.vst win 32bit"));
#endif
          }
          else
          {
// QString::*EmptyParts is deprecated, use Qt::*EmptyParts, new as of 5.14.
#if QT_VERSION >= 0x050e00
            MusEGlobal::config.pluginVstPathList = vst_path.split(list_separator, Qt::SkipEmptyParts);
#else
            MusEGlobal::config.pluginVstPathList = vst_path.split(list_separator, QString::SkipEmptyParts);
#endif
            found = true;
          }
        }
        if(!found && qputenv("VST_PATH", MusEGlobal::config.pluginVstPathList.join(list_separator).toLocal8Bit()) == 0)
          fprintf(stderr, "Error setting VST_PATH\n");

        //=======================
        //  LinuxVST (*.so) paths:
        //=======================
        found = false;
        if(MusEGlobal::config.pluginLinuxVstPathList.isEmpty())
        {
          if(lxvst_path.isEmpty())
          {
            if(vst_path.isEmpty())
            {
              MusEGlobal::config.pluginLinuxVstPathList <<

// On win, vst is usually where *.dll files are found. We don't want that with Linux *.so vst files.
// Otherwise on Linux for example, vst is where Linux vst *.so files are found.
// On win, lxvst should be safe, likely where Linux vst *.so files might be found (if that's even a thing!).
#ifndef Q_OS_WIN
                (MusEGlobal::museUser + QString("/vst")) <<
#endif
                (MusEGlobal::museUser + QString("/lxvst")) <<

#ifndef Q_OS_WIN
                (MusEGlobal::museUser + QString("/.vst")) <<
#endif
                (MusEGlobal::museUser + QString("/.lxvst")) <<

#ifndef Q_OS_WIN
                QString("/usr/local/lib64/vst") <<
#endif
                QString("/usr/local/lib64/lxvst") <<

#ifndef Q_OS_WIN
                QString("/usr/local/lib/vst") <<
#endif
                QString("/usr/local/lib/lxvst") <<

#ifndef Q_OS_WIN
                QString("/usr/lib64/vst") <<
#endif
                QString("/usr/lib64/lxvst") <<

#ifndef Q_OS_WIN
                QString("/usr/lib/vst")  <<
#endif
                QString("/usr/lib/lxvst");

            }
            else
            {
// QString::*EmptyParts is deprecated, use Qt::*EmptyParts, new as of 5.14.
#if QT_VERSION >= 0x050e00
              MusEGlobal::config.pluginLinuxVstPathList = vst_path.split(list_separator, Qt::SkipEmptyParts);
#else
              MusEGlobal::config.pluginLinuxVstPathList = vst_path.split(list_separator, QString::SkipEmptyParts);
#endif
              found = true;
            }
          }
          else
          {
// QString::*EmptyParts is deprecated, use Qt::*EmptyParts, new as of 5.14.
#if QT_VERSION >= 0x050e00
            MusEGlobal::config.pluginLinuxVstPathList = lxvst_path.split(list_separator, Qt::SkipEmptyParts);
#else
            MusEGlobal::config.pluginLinuxVstPathList = lxvst_path.split(list_separator, QString::SkipEmptyParts);
#endif
            found = true;
          }
        }
        if(!found && qputenv("LXVST_PATH", MusEGlobal::config.pluginLinuxVstPathList.join(list_separator).toLocal8Bit()) == 0)
          fprintf(stderr, "Error setting LXVST_PATH\n");

        //==============
        //  LV2 paths:
        //==============
        // Special for LV2: Since we use the recommended lilv_world_load_all() 
        //  not lilv_world_load_bundle(), LV2_PATH seems to be the only way to set paths. 
        found = false;
        if(MusEGlobal::config.pluginLv2PathList.isEmpty())
        {
          if(lv2_path.isEmpty())
          {
            MusEGlobal::config.pluginLv2PathList <<
              (MusEGlobal::museUser + QString("/lv2")) <<
              (MusEGlobal::museUser + QString("/.lv2")) <<
              QString("/usr/local/lib64/lv2") <<
              QString("/usr/local/lib/lv2") <<
              QString("/usr/lib64/lv2") <<
              QString("/usr/lib/lv2");
          }
          else
          {
// QString::*EmptyParts is deprecated, use Qt::*EmptyParts, new as of 5.14.
#if QT_VERSION >= 0x050e00
            MusEGlobal::config.pluginLv2PathList = lv2_path.split(list_separator, Qt::SkipEmptyParts);
#else
            MusEGlobal::config.pluginLv2PathList = lv2_path.split(list_separator, QString::SkipEmptyParts);
#endif
            found = true;
          }
        }
        if(!found && qputenv("LV2_PATH", MusEGlobal::config.pluginLv2PathList.join(list_separator).toLocal8Bit()) == 0)
          fprintf(stderr, "Error setting LV2_PATH\n");

        
        // BEGIN  Parse command line options
        //----------------------------------
        QString open_filename;
        AudioDriverSelect audioType = DriverConfigSetting;
        bool force_plugin_rescan = false;
        bool dont_plugin_rescan = false;
        // A block because we don't want ths hanging around. Use it then lose it.
        {
          QCommandLineParser parser;
          QString errorMessage;
          switch (parseCommandLine(parser, &errorMessage, open_filename,
                                   audioType, force_plugin_rescan, dont_plugin_rescan))
          {
            case CommandLineOk:
                break;
            case CommandLineError:
                fputs(qPrintable(errorMessage), stderr);
                fputs("\n\n", stderr);
                fputs(qPrintable(parser.helpText()), stderr);
                printExtraHelpText();
#ifdef HAVE_LASH
                if(lash_args) lash_args_destroy(lash_args);
#endif
                return 1;
            case CommandLineVersionRequested:
                printf("%s %s\n", qPrintable(QCoreApplication::applicationName()),
                      qPrintable(QCoreApplication::applicationVersion()));
#ifdef HAVE_LASH
                if(lash_args) lash_args_destroy(lash_args);
#endif
                return 0;
            case CommandLineHelpRequested:
                // Works OK, but we want extra help text. Also the lash args destroy thingy...
                //parser.showHelp();
                //Q_UNREACHABLE();
                fputs(qPrintable(parser.helpText()), stdout);
                printExtraHelpText();
#ifdef HAVE_LASH
                if(lash_args) lash_args_destroy(lash_args);
#endif
                return 0;
          }
        }
        // END Parse command line options
        //----------------------------------

        // Set some AL library namespace debug flags as well.
        // Make sure the AL namespace variables mirror our variables.
        AL::debugMsg = MusEGlobal::debugMsg;
        AL::denormalBias = MusEGlobal::denormalBias;
        AL::division = MusEGlobal::config.division;
        AL::sampleRate = MusEGlobal::sampleRate;
        AL::mtcType = MusEGlobal::mtcType;

// REMOVE Tim. py. Removed. TEST Keep this? Think not. It was for getting the last option (the filename)
//                                when we were using getopt() but now we use QCommandLineParser. Un-needed ?
//         argc_copy -= optind;
//         ++argc_copy;

        srand(time(0));   // initialize random number generator
        //signal(SIGCHLD, catchSignal);  // interferes with initVST(). see also app.cpp, function catchSignal()

        static QTranslator translator(0);
        {
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
        }

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

        // NOTE: Set the stylesheet and style as early as possible!
        // Any later invites trouble - typically the colours may be off, 
        //  but currently with Breeze or Oxygen, MDI sub windows  may be frozen!
        // Working with Breeze maintainer to fix problem... 2017/06/06 Tim.
        MusEGui::updateThemeAndStyle();

        //-------------------------------------------------------
        //    BEGIN SHOW MUSE SPLASH SCREEN
        //-------------------------------------------------------

        QString splash_prefix;
        QSplashScreen* muse_splash = NULL;
        if (MusEGlobal::config.showSplashScreen) {
            QPixmap splsh(MusEGlobal::museGlobalShare + "/splash.png");

            if (!splsh.isNull()) {
                muse_splash = new QSplashScreen(splsh,
                  Qt::WindowStaysOnTopHint);
                muse_splash->setAttribute(Qt::WA_DeleteOnClose);  // Possibly also Qt::X11BypassWindowManagerHint
                muse_splash->show();
                splash_prefix = QString("MusE ") + QString(VERSION);
                muse_splash->showMessage(splash_prefix);
            }
        }
        
        //-------------------------------------------------------
        //    END SHOW MUSE SPLASH SCREEN
        //-------------------------------------------------------

        //-------------------------------------------------------
        //    BEGIN Plugin scanning
        //-------------------------------------------------------

        if(muse_splash)
        {
          muse_splash->showMessage(splash_prefix + QString(" Creating plugin cache files..."));
          qApp->processEvents();
        }

        bool do_rescan = false;
        if(force_plugin_rescan)
        {
          force_plugin_rescan = false;
          if(!plugin_rescan_already_done)
          {
            do_rescan = true;
            plugin_rescan_already_done = true;
          }
        }
        
        if(MusEGlobal::config.pluginCacheTriggerRescan)
        {
          do_rescan = true;
          // Done with rescan trigger. Reset it now.
          MusEGlobal::config.pluginCacheTriggerRescan = false;
        }
        
        // Scan all known plugins from the cache file, or if it does not exist
        //  create the cache file by reading plugins in a safe 'sandbox'.
        MusEPlugin::PluginScanInfoStruct::PluginType_t types = MusEPlugin::PluginScanInfoStruct::PluginTypeNone;
        if(MusEGlobal::loadPlugins)
          types |= MusEPlugin::PluginScanInfoStruct::PluginTypeLADSPA;
        if(MusEGlobal::loadMESS)
          types |= MusEPlugin::PluginScanInfoStruct::PluginTypeMESS;
        if(MusEGlobal::loadVST)
          types |= MusEPlugin::PluginScanInfoStruct::PluginTypeVST;
        if(MusEGlobal::loadNativeVST)
          types |= MusEPlugin::PluginScanInfoStruct::PluginTypeLinuxVST;
        if(MusEGlobal::loadDSSI)
          types |= (MusEPlugin::PluginScanInfoStruct::PluginTypeDSSI |
                    MusEPlugin::PluginScanInfoStruct::PluginTypeDSSIVST);
        if(MusEGlobal::loadLV2)
          types |= MusEPlugin::PluginScanInfoStruct::PluginTypeLV2;

        types |= MusEPlugin::PluginScanInfoStruct::PluginTypeUnknown;
        
        MusEPlugin::checkPluginCacheFiles(new_plugin_cache_path,
                                        // List of plugins to scan into and write to cache files from.
                                        &MusEPlugin::pluginList,
                                        // Don't bother reading any port information that might exist in the cache.
                                        false,
                                        // Whether to force recreation.
                                        do_rescan,
                                        // Whether to NOT recreate.
                                        dont_plugin_rescan,
                                        // When creating, where to find the application's own plugins.
                                        MusEGlobal::museGlobalLib,
                                        // Plugin types to check.
                                        types,
                                        // Debug messages.
                                        MusEGlobal::debugMsg);

        // Done with rescan trigger. Reset it now.
        if(do_rescan)
          MusEGlobal::config.pluginCacheTriggerRescan = false;

        //-------------------------------------------------------
        //   END Plugin scanning
        //-------------------------------------------------------


        AL::initDsp();
        
        if(muse_splash)
        {
          muse_splash->showMessage(splash_prefix + QString(" Initializing audio system..."));
          qApp->processEvents();
        }
        
        MusECore::initAudio();

        MusEGui::initIcons(MusEGlobal::config.cursorSize);

        if (MusEGlobal::loadMESS)
          MusECore::initMidiSynth(); // Need to do this now so that Add Track -> Synth menu is populated when MusE is created.

        MusEGlobal::muse = new MusEGui::MusE();
        app.setMuse(MusEGlobal::muse);
        
        MusEGui::init_function_dialogs();
        MusEGui::retranslate_function_dialogs();

        if(muse_splash)
        {
          muse_splash->showMessage(splash_prefix + QString(" Initializing audio driver..."));
          qApp->processEvents();
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
#ifdef HAVE_RTAUDIO
                if(MusECore::initRtAudio())
                  fallbackDummy();
                else
                  fprintf(stderr, "Using rtAudio\n");
#else
                fallbackDummy();
#endif
                break;
              }
            case MusEGlobal::JackAudio:
              {
                fprintf(stderr, "User JackAudio backend - backend selected through configuration\n");
                if (MusECore::initJackAudio()) 
                {
                  MusEGlobal::realTimeScheduling = true;
                  // Force default Pulse.
#ifdef HAVE_RTAUDIO
                  if(MusECore::initRtAudio(true))
                    fallbackDummy();
                  else
                    fprintf(stderr, "Using rtAudio Pulse\n");
#else
                  fallbackDummy();
#endif
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

        // setup the prefetch fifo length now that the segmentSize is known
        MusEGlobal::fifoLength = 131072 / MusEGlobal::segmentSize;
        MusECore::initAudioPrefetch();

        // Set up the wave module now that sampleRate and segmentSize are known.
        MusECore::SndFile::initWaveModule(
          &MusEGlobal::sndFiles,
          &MusEGlobal::audioConverterPluginList,
          &MusEGlobal::defaultAudioConverterSettings,
          MusEGlobal::sampleRate,
          MusEGlobal::segmentSize);
        
        if(muse_splash)
        {
          muse_splash->showMessage(splash_prefix + QString(" Initializing midi devices..."));
          qApp->processEvents();
        }

// REMOVE Tim. startup. Removed 2019/02/21. It's been six years since 1.9.9.5 release.
//        Remove this waiting part at some point if we're all good...
//
//         // WARNING Must do it this way. Call registerClient long AFTER Jack client
//         //  is created and MusE ALSA client is created (in initMidiDevices),
//         //  otherwise random crashes can occur within Jack <= 1.9.8.
//         // Fixed in Jack 1.9.9.  Tim.
        // This initMidiDevices will automatically initialize the midiSeq sequencer thread,
        //  but not start it - that's a bit later on.
        MusECore::initMidiDevices();
//         // Wait until things have settled. One second seems OK so far.
//         for(int t = 0; t < 100; ++t)
//           usleep(10000);
        // Now it is safe to call registerClient.
        MusEGlobal::audioDevice->registerClient();

        MusECore::initMidiController();
        MusECore::initMidiInstruments();
        MusECore::initMidiPorts();

        if(muse_splash)
        {
          muse_splash->showMessage(splash_prefix + QString(" Initializing plugins..."));
          qApp->processEvents();
        }

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

        // Now that all the plugins are done loading from the global plugin cache list,
        //  we are done with it. Clear it to free up memory.
        // TODO Future: Will need to keep it around if we ever switch to using the list all the time
        //       instead of separate global plugin and synth lists.
        MusEPlugin::pluginList.clear();

        MusECore::initOSC();

        MusECore::initMetronome();

        const QString metro_presets = MusEGlobal::museGlobalShare + QString("/metronome");
        MusECore::initMetronomePresets(metro_presets, &MusEGlobal::metroAccentPresets, MusEGlobal::debugMsg);
        // If the global metronome accent settings are empty, it is unlikely the user did that, or wants that.
        // More likely it indicates this is a first-time init of the global settings.
        // In any case, if empty fill the global metronome accent settings with factory presets.
        if(MusEGlobal::metroGlobalSettings.metroAccentsMap &&
           MusEGlobal::metroGlobalSettings.metroAccentsMap->empty())
        {
          // Fill with defaults.
          MusEGlobal::metroAccentPresets.defaultAccents(
            MusEGlobal::metroGlobalSettings.metroAccentsMap,
            MusECore::MetroAccentsStruct::FactoryPreset);
        }

        MusECore::initWavePreview(MusEGlobal::segmentSize);

        MusECore::enumerateJackMidiDevices();

  #ifdef HAVE_LASH
        {
          MusEGui::lash_client = 0;
          if(MusEGlobal::useLASH)
          {
            if(muse_splash)
            {
              muse_splash->showMessage(splash_prefix + QString(" Initializing LASH support..."));
              qApp->processEvents();
            }

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

#ifndef _WIN32
        if (!MusEGlobal::debugMode) {
              if (mlockall(MCL_CURRENT | MCL_FUTURE))
                    perror("WARNING: Cannot lock memory:");
              }
#endif

        if(muse_splash)
        {
          muse_splash->showMessage(splash_prefix + QString(" populating track types..."));
          qApp->processEvents();
        }

        MusEGlobal::muse->populateAddTrack(); // could possibly be done in a thread.

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
        // Set the audio device sync timeout value.
        //--------------------------------------------------
        // Enforce a 30 second timeout.
        // TODO: Split this up and have user adjustable normal (2 or 10 second default) value,
        //        plus a contribution from the total required precount time.
        //       Too bad we likely can't set it dynamically in the audio sync callback.
        // NOTE: This is also enforced casually in Song:seqSignal after a stop, start, or seek.
        MusEGlobal::audioDevice->setSyncTimeout(30000000);
              
        //--------------------------------------------------
        // Auto-fill the midi ports, if appropriate.
        // Only if NOT actually opening an existing file.
        // FIXME: Maybe check if it's a .med file (song may populate)
        //         or .mid file (always populate) or .wav file etc.
        //--------------------------------------------------
        if(MusEGlobal::populateMidiPortsOnStart &&
           ((!open_filename.isEmpty() && !QFile(open_filename).exists()) ||
           (open_filename.isEmpty() &&
           (MusEGlobal::config.startMode == 1 || MusEGlobal::config.startMode == 2) &&
           !MusEGlobal::config.startSongLoadConfig)))
          MusECore::populateMidiPorts();

        if(muse_splash)
        {
          // From this point on, slap a timer on it so that it stays up for few seconds,
          //  since closing it now might be too short display time.
          // It will auto-destory itself.
          QTimer* stimer = new QTimer(muse_splash);
          muse_splash->connect(stimer, SIGNAL(timeout()), muse_splash, SLOT(close()));
          stimer->setSingleShot(true);
          stimer->start(3000);
        }

        //--------------------------------------------------
        // Load the default song.
        //--------------------------------------------------
        // When restarting, override with the last project file name used.
        if(last_project_filename.isEmpty())
        {
          MusEGlobal::muse->loadDefaultSong(open_filename, false, false);
        }
        else
        {
          MusEGlobal::muse->loadDefaultSong(
            last_project_filename, last_project_was_template, last_project_loaded_config);
        }

        QTimer::singleShot(100, MusEGlobal::muse, SLOT(showDidYouKnowDialogIfEnabled()));

        //--------------------------------------------------
        // Start the application...
        //--------------------------------------------------

        rv = app.exec();

        //--------------------------------------------------
        // ... Application finished.
        //--------------------------------------------------

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
        
        {
          // If the current project file name exists, set the last_project_filename
          //  variable so that if restarting, it starts with that file.
          // This should be OK since fresh untitled unsaved songs will not have a
          //  file yet, they will have a unique file name that does not exist until
          //  saved, so it will either use the existing open_filename, or default
          //  to normal operation (template, last song, or blank).
          const QString s = MusEGlobal::muse->lastProjectFilePath();
          const QFileInfo fi(s);
          if(fi.exists())
          {
            last_project_filename = s;
            last_project_was_template = MusEGlobal::muse->lastProjectWasTemplate();
            last_project_loaded_config = MusEGlobal::muse->lastProjectLoadedConfig();
          }
          else
          {
            last_project_filename.clear();
            last_project_was_template = false;
            last_project_loaded_config = false;
          }
        }

        // Now delete the application.
        delete MusEGlobal::muse;
        MusEGlobal::muse = nullptr;

        // These are owned by muse and deleted above. Reset to zero now.
        MusEGlobal::undoRedo = 0;
        MusEGlobal::undoAction = 0;
        MusEGlobal::redoAction = 0;

        // Reset the option index.
        // NOTE: See optind manual for special resetting values.
        //       Traditionally 1 is set, but here we may need GNU specific 0.
        //optind = 0;

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

        // Clear and delete these.
        if(MusEGlobal::defaultAudioConverterSettings)
          delete MusEGlobal::defaultAudioConverterSettings;
        MusEGlobal::defaultAudioConverterSettings = nullptr;
        MusEGlobal::audioConverterPluginList.clearDelete();
        
        // Clear the mixer configurations.
        MusEGlobal::config.mixer1.stripOrder.clear();
        MusEGlobal::config.mixer1.stripVisibility.clear();
        MusEGlobal::config.mixer1.stripConfigList.clear();
        MusEGlobal::config.mixer2.stripOrder.clear();
        MusEGlobal::config.mixer2.stripVisibility.clear();
        MusEGlobal::config.mixer2.stripConfigList.clear();
      }

      //============================================
      // END Restart loop. For (re)starting the app.
      //============================================

      if(MusEGlobal::debugMsg) 
        fprintf(stderr, "Finished! Exiting main, return value:%d\n", rv);
      return rv;
      
      }
