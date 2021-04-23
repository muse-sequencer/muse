//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: app.h,v 1.34.2.14 2009/11/16 11:29:33 lunar_shuttle Exp $
//
//  (C) Copyright 1999-2004 Werner Schweer (ws@seh.de)
//  (C) Copyright 2016 Tim E. Real (terminator356 on sourceforge)
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
#include "globaldefs.h"
#include "cobject.h"

#include <QFileInfo>
#include <QMainWindow>
#include <QRect>
#include <QString>
#include <QPointer>

#include <list>
#include <time.h>
#include <sys/time.h>
#if defined(__FreeBSD__)
#include <unistd.h>
#endif


// Forward declarations:
class QCloseEvent;
class QMenu;
class QToolBar;
class QToolButton;
class QProgressDialog;
class QTimer;
class QMdiSubWindow;
class MuseMdiArea;
class QDockWidget;

namespace MusECore {
class AudioOutput;
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
class AudioMixerApp;
class BigTime;
class ClipListEdit;
class EditInstrument;
class EditToolBar;
class GlobalSettingsConfig;
class MRConfig;
class MarkerView;
class LMaster;
class MetronomeConfig;
class MidiFileConfig;
class MidiFilterConfig;
class MidiInputTransformDialog;
class MidiSyncConfig;
class MidiTransformerDialog;
class RhythmGen;
class ScoreEdit;
class ShortcutConfig;
class TopWin;
class TopLevelList;
class Transport;
class VisibleTracks;
class RouteDialog;
class CpuToolbar;
class CpuStatusBar;
class SnooperDialog;
class MasterEdit;

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
    QAction *fileSaveAction, *fileOpenAction, *fileNewAction, *fileNewFromTemplateAction;
    QAction *fileSaveRevisionAction, *fileSaveAsAction, *fileSaveAsNewProjectAction, *fileSaveAsTemplateAction;
    QAction *fileImportMidiAction, *fileExportMidiAction;
    QAction *fileImportPartAction, *fileImportWaveAction, *fileMoveWaveFiles, *quitAction;
    QAction *fileCloseAction;
    QAction *editSongInfoAction;

    MuseMdiArea* mdiArea;

    TopWin* activeTopWin;
    TopWin* currentMenuSharingTopwin;
    TopWin* waitingForTopwin;

    std::list<QToolBar*> requiredToolbars; //always displayed
    std::list<QToolBar*> optionalToolbars; //only displayed when no toolbar-sharing window is active
    std::list<QToolBar*> foreignToolbars;  //holds a temporary list of the toolbars of a toolbar-sharer
    std::list<QMenu*> leadingMenus;
    std::list<QMenu*> trailingMenus;

    QList<QDockWidget *> hiddenDocks;

    // View Menu actions
    QAction *viewTransportAction, *viewBigtimeAction, *viewMixerAAction, *viewMixerBAction, *viewCliplistAction, *viewMarkerAction;
    QAction *fullscreenAction, *toggleDocksAction;
    QAction *masterGraphicAction, *masterListAction;

    // Midi Menu Actions
    QAction *midiEditInstAction, *midiResetInstAction, *midiInitInstActions, *midiLocalOffAction;
    QAction *midiTrpAction, *midiInputTrfAction, *midiInputFilterAction, *midiRemoteAction;
#ifdef BUILD_EXPERIMENTAL
    QAction *midiRhythmAction;
#endif

    // Audio Menu Actions
    QAction *audioBounce2TrackAction, *audioBounce2FileAction, *audioRestartAction;

    // Automation Menu Actions
    // REMOVE Tim. automation. Removed.
    // Deprecated. MusEGlobal::automation is now fixed TRUE
    //  for now until we decide what to do with it.
    //       QAction *autoMixerAction;
    QAction *autoSnapshotAction, *autoClearAction;

    // Settings Menu Actions
    QAction *settingsGlobalAction, *settingsShortcutsAction, *settingsMetronomeAction, *settingsMidiSyncAction;
    QAction *settingsMidiIOAction, *settingsAppearanceAction, *settingsMidiPortAction;
    QAction *dontFollowAction, *followPageAction, *followCtsAction;
    QAction *rewindOnStopAction;
    // Help Menu Actions
    QAction *helpManualAction, *helpHomepageAction, *helpReportAction, *helpAboutAction, *helpDidYouKnow, *helpSnooperAction;

    QString appName;

    QFileInfo project;
    QString _lastProjectFilePath;
    bool _lastProjectWasTemplate;
    bool _lastProjectLoadedConfig;
    QToolBar *tools;
    CpuToolbar* cpuLoadToolbar;
    CpuStatusBar* cpuStatusBar;

    // when adding a toolbar to the main window, remember adding it to
    // either the requiredToolbars or optionalToolbars list!

    Transport* transport;
    BigTime* bigtime;
    EditInstrument* editInstrument;

    // when adding a menu to the main window, remember adding it to
    // either the leadingMenus or trailingMenus list!
    QMenu *menu_file, *menuView, *menuSettings, *menuWindows, *menu_help;
    QMenu* menu_audio, *menuUtils;
    QMenu* menu_functions; // *menuScriptPlugins;

    QMenu* follow;
//    QMenu* midiInputPlugins;

    QWidget* midiPortConfig;
    QWidget* softSynthesizerConfig;
    MidiSyncConfig* midiSyncConfig;
    MRConfig* midiRemoteConfig;
#ifdef BUILD_EXPERIMENTAL
    RhythmGen* midiRhythmGenerator;
#endif
    MetronomeConfig* metronomeConfig;
    MidiFileConfig* midiFileConfig;
    GlobalSettingsConfig* globalSettingsConfig;
    MidiFilterConfig* midiFilterConfig;
    MidiInputTransformDialog* midiInputTransform;
    ShortcutConfig* shortcutConfig;
    Appearance* appearance;
    SnooperDialog* _snooperDialog;
    AudioMixerApp* mixer1;
    AudioMixerApp* mixer2;
    RouteDialog* routeDialog;
    // NOTE: For deleting parentless dialogs and widgets, please add them to deleteParentlessDialogs().
    void deleteParentlessDialogs();

    Arranger* _arranger;
    ToplevelList toplevels;
    ClipListEdit* clipListEdit;
    QDockWidget* clipListDock;
    MarkerView* markerView;
    QDockWidget* markerDock;
    LMaster* masterList;
    QDockWidget* masterListDock;
    ArrangerView* arrangerView;
    MidiTransformerDialog* midiTransformerDialog;
    QMenu* openRecent;
    QPointer<MasterEdit> masterEditor;

    bool writeTopwinState;
    // Set to restart MusE (almost) from scratch before calling close().
    bool _isRestartingApp;

    bool readMidi(FILE*);
    void read(MusECore::Xml& xml, bool doReadMidiPorts, bool isTemplate);
    void processTrack(MusECore::MidiTrack* track);

    void write(MusECore::Xml& xml, bool writeTopwins) const;
    // If clear_all is false, it will not touch things like midi ports.
    bool clearSong(bool clear_all = true);
    bool save(const QString&, bool overwriteWarn, bool writeTopwins);
    void setUntitledProject();
    void setConfigDefaults();

    void readConfigParts(MusECore::Xml& xml);
    void readToplevels(MusECore::Xml& xml);
    MusECore::PartList* getMidiPartsToEdit();
    MusECore::Part* readPart(MusECore::Xml& xml);
    bool checkRegionNotNull();
    void loadProjectFile1(const QString&, bool songTemplate, bool doReadMidiPorts);
    // Write global configuration.
    void writeGlobalConfiguration(int level, MusECore::Xml&) const;
    // Write song specific configuration.
    void writeConfiguration(int level, MusECore::Xml&) const;
    void updateConfiguration();
    QString projectTitle(QString name);
    void toggleTrackArmSelectedTrack();
    void centerAndResize();
    void closeDocks();
    void addTabbedDock(Qt::DockWidgetArea area, QDockWidget *widget);
    void saveStateExtra();
    void saveStateTopLevels();
    bool findOpenEditor(const TopWin::ToplevelType type, MusECore::PartList* pl);
    bool findOpenListEditor(MusECore::PartList* pl);
    bool filterInvalidParts(const TopWin::ToplevelType type, MusECore::PartList* pl);
    void updateStatusBar();
    void setAndAdjustFonts();

    QTimer *saveTimer;
    QTimer *blinkTimer;
    QTimer *messagePollTimer;
    int saveIncrement;

    timeval lastCpuTime;
    timespec lastSysTime;
    float fAvrCpuLoad;
    int avrCpuLoadCounter;
    float fCurCpuLoad;

signals:
    void configChanged();
    void activeTopWinChanged(MusEGui::TopWin*);
    void blinkTimerToggled(bool state);

private slots:
    void heartBeat();
    void blinkTimerSlot();
    void saveTimerSlot();
    void messagePollTimerSlot();
    void loadProject();
    bool save();
    void configGlobalSettings();
    void quitDoc();
    void about();
    void aboutQt();
    void startHelpBrowser();
    void startHomepageBrowser();
    void startBugBrowser();
    void importMidi();
    void importWave();
    void importPart();
    void exportMidi();
    void findUnusedWaveFiles();

    void toggleTransport(bool);
    void toggleBigTime(bool);
    void toggleMixer1(bool);
    void toggleMixer2(bool);

    void configMidiSync();
    void configMidiFile();
    void configShortCuts();
    void configShortCutsSaveConfig();
    void configMetronome();
    void configAppearance();

    void startSongInfo(bool editable=true);

    void writeGlobalConfiguration() const;
    void showClipList(bool);

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
    void resetMidiDevices();
    void initMidiDevices();
    void localOff();
    void switchMixerAutomation();
    void takeAutomationSnapshot();
    void clearAutomation();
    void bigtimeClosed();
    void mixer1Closed();
    void mixer2Closed();

    void activeTopWinChangedSlot(MusEGui::TopWin*);
    void setCurrentMenuSharingTopwin(MusEGui::TopWin*);

    void bringToFront(QWidget* win);
    void setFullscreen(bool);
    void setDirty();
    void toggleRewindOnStop(bool);
    void toggleDocks(bool show);

public slots:
    void saveNewRevision();
    void saveAsNewProject();
    bool saveAs(bool overrideProjectSaveDialog = false);
    void saveAsTemplate();
    void bounceToFile(MusECore::AudioOutput* ao = nullptr);
    void bounceToTrack(MusECore::AudioOutput* ao = nullptr);
    void closeEvent(QCloseEvent*event) override;
    void loadProjectFile(const QString&);
    void loadProjectFile(const QString&, bool songTemplate, bool doReadMidiPorts);
    void fileClose();
    void toplevelDeleting(MusEGui::TopWin* tl);
    bool seqRestart();
    void loadTemplate();
    void loadDefaultTemplate();
    void showBigtime(bool);
    void showMixer1(bool);
    void showMixer2(bool);
    void startRouteDialog();
    void showMarker(bool);
    void importMidi(const QString &file);
    void showDidYouKnowDialogIfEnabled();
    void showDidYouKnowDialog();
    void startEditInstrument(const QString& find_instrument = QString(), EditInstrumentTabType show_tab = EditInstrumentPatches);
    void configMidiPorts();

    void startEditor(MusECore::PartList*, int);
    void startScoreQuickly();
    void startPianoroll(bool newwin = false);
    void startPianoroll(MusECore::PartList* pl, bool showDefaultCtrls = false, bool newwin = false);
    void startWaveEditor(bool newwin = false);
    void startWaveEditor(MusECore::PartList*, bool newwin = false);
    void openInScoreEdit(ScoreEdit* destination, MusECore::PartList* pl, bool allInOne=false);
    void openInScoreEdit(ScoreEdit* destination, bool allInOne=false);
    void openInScoreEdit_allInOne(QWidget* destination);
    void openInScoreEdit_oneStaffPerTrack(QWidget* destination);
    void startMasterEditor();
    void showMasterList(bool);
    void startListEditor(bool newwin = false);
    void startListEditor(MusECore::PartList*, bool newwin = false);
    void startDrumEditor(bool newwin = false);
    void startDrumEditor(MusECore::PartList* pl, bool showDefaultCtrls = false, bool newwin = false);
    void startEditor(MusECore::Track*);
    void startMidiTransformer();
    void startSnooper();

    void focusChanged(QWidget* old, QWidget* now);

    void addMdiSubWindow(QMdiSubWindow*);
    void shareMenuAndToolbarChanged(MusEGui::TopWin*, bool);
    void topwinMenuInited(MusEGui::TopWin*);
    void setActiveMdiSubWindow(QMdiSubWindow*);


    void updateWindowMenu();

    void resetXrunsCounter();

    bool startPythonBridge();
    bool stopPythonBridge();

    void addProjectToRecentList(const QString& name);
    void saveProjectRecentList();

public:
    MusE();

    void populateAddTrack();

    // Loads a default song according to settings, or filename_override if not empty.
    // If filename_override is not empty, use_template and load_config are used.
    void loadDefaultSong(const QString& filename_override, bool use_template, bool load_config);
    bool loadConfigurationColors(QWidget* parent = 0);
    bool saveConfigurationColors(QWidget* parent = 0);
    // Whether to restart MusE (almost) from scratch when calling close().
    bool restartingApp() const { return _isRestartingApp;}
    // Set to restart MusE (almost) from scratch before calling close().
    void setRestartingApp(bool v) { _isRestartingApp = v;}
    Arranger* arranger() const { return _arranger; }
    int arrangerRaster() const;
    int currentPartColorIndex() const;

    ArrangerView* getArrangerView() const { return arrangerView; }
    QRect configGeometryMain;
    QProgressDialog *progress;
    bool importMidi(const QString name, bool merge);
    void kbAccel(int);

    // writeFlag: Write to configuration file.
    void changeConfig(bool writeFlag);

    void seqStop();
    bool seqStart();
    // Starts the ALSA midi portion of the sequencer. audioDevice must be valid.
    // Returns true if successful or already running.
    //       bool seqStartMidi();
    void setHeartBeat();
    void stopHeartBeat();
    void importController(int, MusECore::MidiPort*, int);
    const QFileInfo& projectFileInfo() const { return project; }
    QString lastProjectFilePath() const { return _lastProjectFilePath; };
    bool lastProjectWasTemplate() const { return _lastProjectWasTemplate; }
    bool lastProjectLoadedConfig() const { return _lastProjectLoadedConfig; }
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
    bool restoreState(const QByteArray &state, int version = 0);

    const ToplevelList* getToplevels() { return &toplevels; }

    TopWin* getCurrentMenuSharingTopwin() { return currentMenuSharingTopwin; }

    float getCPULoad();
    void initStatusBar();
    void setStatusBarText(const QString &message, int timeout = 0);
    void clearStatusBarText();

    void launchBrowser(QString &whereTo);

    QMenu* createPopupMenu() override;

#ifdef HAVE_LASH
    void lash_idle_cb ();
#endif
};

extern void addProject(const QString& name);

} // namespace MusEGui

#endif // __APP_H__
