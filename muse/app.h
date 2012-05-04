//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: app.h,v 1.34.2.14 2009/11/16 11:29:33 lunar_shuttle Exp $
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

#ifndef __APP_H__
#define __APP_H__

#include "config.h"
#include "cobject.h"

#include <QFileInfo>
#include <list>

class QCloseEvent;
class QMainWindow;
class QMenu;
class QPoint;
class QRect;
class QScrollArea;
class QSignalMapper;
class QString;
class QToolBar;
class QToolButton;
class QProgressDialog;
class QMdiArea;
class QTimer;

namespace MusECore {
class AudioOutput;
class Instrument;
class MidiInstrument;
class MidiPort;
class MidiTrack;
class Part;
class PartList;
class SynthI;
class Track;
class Undo;
class WaveTrack;
class Xml;
}


namespace MusEGui {
class Appearance;
class Arranger;
class ArrangerView;
class AudioConf;
class AudioMixerApp;
class AudioRecord;
class BigTime;
class ClipListEdit;
class EditInstrument;
class EditToolBar;
class GlobalSettingsConfig;
class GlobalSettingsConfig;
class MRConfig;
class MarkerView;
class MetronomeConfig;
class MidiControllerEditDialog;
class MidiFileConfig;
class MidiFilterConfig;
class MidiInputTransformDialog;
class MidiSyncConfig;
class MidiTransformerDialog;
class PrinterConfig;
class RhythmGen;
class ScoreEdit;
class ShortcutConfig;
class TopWin;
class Transport;
class VisibleTracks;

#define MENU_ADD_SYNTH_ID_BASE 0x8000


//---------------------------------------------------------
//   MusE
//---------------------------------------------------------

class MusE : public QMainWindow
      {
      Q_OBJECT
      enum {CMD_FOLLOW_NO, CMD_FOLLOW_JUMP, CMD_FOLLOW_CONTINUOUS };

      //File menu items:
      enum { CMD_OPEN_RECENT=0, CMD_LOAD_TEMPLATE, CMD_SAVE_AS, CMD_IMPORT_MIDI,
            CMD_EXPORT_MIDI, CMD_IMPORT_PART, CMD_IMPORT_AUDIO, CMD_QUIT, CMD_OPEN_DRUMS, CMD_OPEN_WAVE,
            CMD_OPEN_LIST, CMD_OPEN_LIST_MASTER, CMD_GLOBAL_CONFIG,
            CMD_OPEN_GRAPHIC_MASTER, CMD_OPEN_MIDI_TRANSFORM, CMD_TRANSPOSE,
            CMD_GLOBAL_CUT, CMD_GLOBAL_INSERT, CMD_GLOBAL_SPLIT, CMD_COPY_RANGE,
            CMD_CUT_EVENTS, CMD_CONFIG_SHORTCUTS, CMD_CONFIG_METRONOME, CMD_CONFIG_MIDISYNC,
            CMD_MIDI_FILE_CONFIG, CMD_APPEARANCE_SETTINGS, CMD_CONFIG_MIDI_PORTS, CMD_CONFIG_AUDIO_PORTS,
            CMD_MIDI_EDIT_INSTRUMENTS, CMD_MIDI_RESET, CMD_MIDI_INIT, CMD_MIDI_LOCAL_OFF,
            CMD_MIXER_SNAPSHOT, CMD_MIXER_AUTOMATION_CLEAR, CMD_OPEN_HELP, CMD_OPEN_HOMEPAGE,
            CMD_OPEN_BUG, CMD_START_WHATSTHIS,
            CMD_AUDIO_BOUNCE_TO_FILE, CMD_AUDIO_BOUNCE_TO_TRACK, CMD_AUDIO_RESTART,
            CMD_LAST };

      // File menu actions
      QAction *fileSaveAction, *fileOpenAction, *fileNewAction, *testAction;
      QAction *fileSaveAsAction, *fileImportMidiAction, *fileExportMidiAction;
      QAction *fileImportPartAction, *fileImportWaveAction, *fileMoveWaveFiles, *quitAction;
      QAction *editSongInfoAction;
      
   private:
      QMdiArea* mdiArea;
      
      TopWin* activeTopWin;
      TopWin* currentMenuSharingTopwin;
      TopWin* waitingForTopwin;
      
      std::list<QToolBar*> requiredToolbars; //always displayed
      std::list<QToolBar*> optionalToolbars; //only displayed when no toolbar-sharing window is active
      std::list<QToolBar*> foreignToolbars;  //holds a temporary list of the toolbars of a toolbar-sharer
      std::list<QMenu*> leadingMenus;
      std::list<QMenu*> trailingMenus;
   
      // View Menu actions
      QAction *viewTransportAction, *viewBigtimeAction, *viewMixerAAction, *viewMixerBAction, *viewCliplistAction, *viewMarkerAction, *viewArrangerAction;
      QAction* fullscreenAction;

      // Midi Menu Actions
      QAction *midiEditInstAction, *midiResetInstAction, *midiInitInstActions, *midiLocalOffAction;
      QAction *midiTrpAction, *midiInputTrfAction, *midiInputFilterAction, *midiRemoteAction;
#ifdef BUILD_EXPERIMENTAL
      QAction *midiRhythmAction;
#endif

      // Audio Menu Actions
      QAction *audioBounce2TrackAction, *audioBounce2FileAction, *audioRestartAction;

      // Automation Menu Actions
      QAction *autoMixerAction, *autoSnapshotAction, *autoClearAction;

      // Window Menu Actions
      QAction* windowsCascadeAction;
      QAction* windowsTileAction;
      QAction* windowsRowsAction;
      QAction* windowsColumnsAction;
      
      // Settings Menu Actions
      QAction *settingsGlobalAction, *settingsShortcutsAction, *settingsMetronomeAction, *settingsMidiSyncAction;
      QAction *settingsMidiIOAction, *settingsAppearanceAction, *settingsMidiPortAction;
      QAction *dontFollowAction, *followPageAction, *followCtsAction;

      // Help Menu Actions
      QAction *helpManualAction, *helpHomepageAction, *helpReportAction, *helpAboutAction;

      QString appName;

      QFileInfo project;
      QToolBar *tools;
      // when adding a toolbar to the main window, remember adding it to
      // either the requiredToolbars or optionalToolbars list!

      Transport* transport;
      BigTime* bigtime;
      EditInstrument* editInstrument;
      
      // when adding a menu to the main window, remember adding it to
      // either the leadingMenus or trailingMenus list!
      QMenu *menu_file, *menuView, *menuSettings, *menuWindows, *menu_help;
      QMenu* menu_audio, *menuAutomation, *menuUtils;
      QMenu* menu_functions, *menuScriptPlugins;

      QMenu* follow;
      QMenu* midiInputPlugins;

      QWidget* midiPortConfig;
      QWidget* softSynthesizerConfig;
      MidiSyncConfig* midiSyncConfig;
      MRConfig* midiRemoteConfig;
      RhythmGen* midiRhythmGenerator;
      MetronomeConfig* metronomeConfig;
      AudioConf* audioConfig;
      MidiFileConfig* midiFileConfig;
      GlobalSettingsConfig* globalSettingsConfig;
      MidiFilterConfig* midiFilterConfig;
      MidiInputTransformDialog* midiInputTransform;
      ShortcutConfig* shortcutConfig;
      Appearance* appearance;
      AudioMixerApp* mixer1;
      AudioMixerApp* mixer2;

      Arranger* _arranger;
      ToplevelList toplevels;
      ClipListEdit* clipListEdit;
      MarkerView* markerView;
      ArrangerView* arrangerView;
      MidiTransformerDialog* midiTransformerDialog;
      QMenu* openRecent;
      
      bool writeTopwinState;
      
      bool readMidi(FILE*);
      void read(MusECore::Xml& xml, bool doReadMidiPorts, bool isTemplate);
      void processTrack(MusECore::MidiTrack* track);

      void write(MusECore::Xml& xml, bool writeTopwins) const;
      // If clear_all is false, it will not touch things like midi ports.
      bool clearSong(bool clear_all = true);
      bool save(const QString&, bool overwriteWarn, bool writeTopwins);
      void setUntitledProject();
      void setConfigDefaults();

      void setFollow();
      void readConfigParts(MusECore::Xml& xml);
      void readMidiport(MusECore::Xml& xml);
      void readMidichannel(MusECore::Xml& xml, int port);
      void readCtrl(MusECore::Xml& xml, int port, int channel);
      void readToplevels(MusECore::Xml& xml);
      MusECore::PartList* getMidiPartsToEdit();
      MusECore::Part* readPart(MusECore::Xml& xml);
      bool checkRegionNotNull();
      void loadProjectFile1(const QString&, bool songTemplate, bool doReadMidiPorts);
      void writeGlobalConfiguration(int level, MusECore::Xml&) const;
      void writeConfiguration(int level, MusECore::Xml&) const;
      void updateConfiguration();

      QSignalMapper *midiPluginSignalMapper;
      QSignalMapper *followSignalMapper;
      QSignalMapper *windowsMapper;

   signals:
      void configChanged();
      void activeTopWinChanged(MusEGui::TopWin*);

   private slots:
      void loadProject();
      bool save();
      void configGlobalSettings();
      void quitDoc();
      void about();
      void aboutQt();
      void startHelpBrowser();
      void startHomepageBrowser();
      void startBugBrowser();
      void launchBrowser(QString &whereTo);
      void importMidi();
      void importWave();
      void importPart();
      void exportMidi();
      void findUnusedWaveFiles();

      void toggleTransport(bool);
      void toggleMarker(bool);
      void toggleArranger(bool);
      void toggleBigTime(bool);
      void toggleMixer1(bool);
      void toggleMixer2(bool);

      void configMidiSync();
      void configMidiFile();
      void configShortCuts();
      void configMetronome();
      void configAppearance();

      void startSongInfo(bool editable=true);

      void writeGlobalConfiguration() const;
      void startClipList(bool);
      
      void openRecentMenu();
      void selectProject(QAction* act);
      void cmd(int);

      void startMidiInputPlugin(int);
      void hideMitPluginTranspose();
      void hideMidiInputTransform();
      void hideMidiFilterConfig();
      void hideMidiRemoteConfig();
#ifdef BUILD_EXPERIMENTAL
      void hideMidiRhythmGenerator();
#endif
      void bounceToTrack();
      void resetMidiDevices();
      void initMidiDevices();
      void localOff();
      void switchMixerAutomation();
      void takeAutomationSnapshot();
      void clearAutomation();
      void bigtimeClosed();
      void mixer1Closed();
      void mixer2Closed();
      void markerClosed();
      void arrangerClosed();

      void execDeliveredScript(int);
      void execUserScript(int);
      
      void activeTopWinChangedSlot(MusEGui::TopWin*);
      void setCurrentMenuSharingTopwin(MusEGui::TopWin*);
      
      void bringToFront(QWidget* win);
      void setFullscreen(bool);
      
      void arrangeSubWindowsRows();
      void arrangeSubWindowsColumns();
      void tileSubWindows();

   public slots:
      bool saveAs();
      void bounceToFile(MusECore::AudioOutput* ao = 0);
      void closeEvent(QCloseEvent*e);
      void loadProjectFile(const QString&);
      void loadProjectFile(const QString&, bool songTemplate, bool doReadMidiPorts);
      void toplevelDeleting(MusEGui::TopWin* tl);
      void loadTheme(const QString&);
      void loadStyleSheetFile(const QString&);
      bool seqRestart();
      void loadTemplate();
      void showBigtime(bool);
      void showMixer1(bool);
      void showMixer2(bool);
      void showMarker(bool);
      void showArranger(bool);
      void importMidi(const QString &file);
      void showDidYouKnowDialog();
      void startEditInstrument();
      void configMidiPorts();

      void startEditor(MusECore::PartList*, int);
      void startScoreQuickly();
      void startPianoroll();
      void startPianoroll(MusECore::PartList* pl, bool showDefaultCtrls = false);
      void startWaveEditor();
      void startWaveEditor(MusECore::PartList*);
      void openInScoreEdit(ScoreEdit* destination, MusECore::PartList* pl, bool allInOne=false);
      void openInScoreEdit(ScoreEdit* destination, bool allInOne=false);
      void openInScoreEdit_allInOne(QWidget* destination);
      void openInScoreEdit_oneStaffPerTrack(QWidget* destination);
      void startMasterEditor();
      void startLMasterEditor();
      void startListEditor();
      void startListEditor(MusECore::PartList*);
      void startDrumEditor();
      void startDrumEditor(MusECore::PartList* pl, bool showDefaultCtrls = false);
      void startEditor(MusECore::Track*);
      void startMidiTransformer();
      
      void focusChanged(QWidget* old, QWidget* now);
      
      void addMdiSubWindow(QMdiSubWindow*);
      void shareMenuAndToolbarChanged(MusEGui::TopWin*, bool);
      void topwinMenuInited(MusEGui::TopWin*);

      void updateWindowMenu();

   public:
      MusE();
      void loadDefaultSong(int argc, char** argv);
      Arranger* arranger() const { return _arranger; }
      ArrangerView* getArrangerView() const { return arrangerView; }
      QRect configGeometryMain;
      QProgressDialog *progress;
      bool importMidi(const QString name, bool merge);
      void kbAccel(int);
      void changeConfig(bool writeFlag);
      void seqStop();
      bool seqStart();  
      void setHeartBeat();
      void importController(int, MusECore::MidiPort*, int);
      QString projectName() { return project.fileName(); }
      QString projectTitle() const;
      QString projectPath() const;
      QString projectExtension() const;
      QWidget* mixer1Window();
      QWidget* mixer2Window();
      QWidget* transportWindow();
      QWidget* bigtimeWindow();
      bool importWaveToTrack(QString& name, unsigned tick=0, MusECore::Track* track=NULL);
      void importPartToTrack(QString& filename, unsigned tick, MusECore::Track* track);
      void showTransport(bool flag);
      
      const ToplevelList* getToplevels() { return &toplevels; }
      
      TopWin* getCurrentMenuSharingTopwin() { return currentMenuSharingTopwin; }
      
#ifdef HAVE_LASH
      void lash_idle_cb ();
#endif
      };

extern void addProject(const QString& name);
#endif

} // namespace MusEGui
