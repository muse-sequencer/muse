//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: app.h,v 1.34.2.14 2009/11/16 11:29:33 lunar_shuttle Exp $
//
//  (C) Copyright 1999-2004 Werner Schweer (ws@seh.de)
//=========================================================

#ifndef __APP_H__
#define __APP_H__

#include "config.h"
#include "cobject.h"
#include "tools.h"

class QCloseEvent;
class QFileInfo;
class QFocusEvent;
class QMainWindow;
class QMenu;
class QPoint;
class QRect;
class QToolBar;
class QToolButton;

class Part;
class PartList;
class Transport;
class BigTime;
class Arranger;
class Instrument;
class PopupMenu;
class PopupView;
class Track;
class PrinterConfig;
class MidiSyncConfig;
class MRConfig;
class MetronomeConfig;
class AudioConf;
class Xml;
class AudioMixerApp;
class ClipListEdit;
class AudioRecord;
class MidiFileConfig;
class MidiFilterConfig;
class MarkerView;
class GlobalSettingsConfig;
class MidiControllerEditDialog;
class MidiInputTransformDialog;
class MidiTransformerDialog;
class SynthI;
class RhythmGen;
class MidiTrack;
class MidiInstrument;
class MidiPort;
class ShortcutConfig;
class Appearance;
class WaveTrack;
class AudioOutput;
class EditInstrument;

#define MENU_ADD_SYNTH_ID_BASE 0x1000

//---------------------------------------------------------
//   MusE
//---------------------------------------------------------

class MusE : public QMainWindow
      {
      Q_OBJECT
      enum {CMD_CUT, CMD_COPY, CMD_PASTE, CMD_INSERT, CMD_INSERTMEAS, CMD_PASTE_CLONE,
            CMD_PASTE_TO_TRACK, CMD_PASTE_CLONE_TO_TRACK, CMD_DELETE,
            CMD_SELECT_ALL, CMD_SELECT_NONE, CMD_SELECT_INVERT,
            CMD_SELECT_ILOOP, CMD_SELECT_OLOOP, CMD_SELECT_PARTS,
            CMD_FOLLOW_NO, CMD_FOLLOW_JUMP, CMD_FOLLOW_CONTINUOUS ,
            CMD_DELETE_TRACK
            };

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

      //int menu_ids[CMD_LAST];

      // File menu actions
      QAction *fileSaveAction, *fileOpenAction, *fileNewAction, *testAction;
      QAction *fileSaveAsAction, *fileImportMidiAction, *fileExportMidiAction, *fileImportPartAction, *fileImportWaveAction, *quitAction;

      // Edit Menu actions
      QAction *editCutAction, *editCopyAction, *editPasteAction, *editInsertAction, *editPasteCloneAction, *editPaste2TrackAction;
      QAction *editPasteC2TAction, *editInsertEMAction, *editDeleteSelectedAction, *editSelectAllAction, *editDeselectAllAction;
      QAction *editInvertSelectionAction, *editInsideLoopAction, *editOutsideLoopAction, *editAllPartsAction;
      QAction *trackMidiAction, *trackDrumAction, *trackWaveAction, *trackAOutputAction, *trackAGroupAction;
      QAction *trackAInputAction, *trackAAuxAction;
      QAction *startPianoEditAction, *startDrumEditAction, *startListEditAction, *startWaveEditAction;
      QAction *masterGraphicAction, *masterListAction;
      QAction *midiTransposeAction;
      QAction *midiTransformerAction;
      QAction *editSongInfoAction;

      // View Menu actions
      QAction *viewTransportAction, *viewBigtimeAction, *viewMixerAAction, *viewMixerBAction, *viewCliplistAction, *viewMarkerAction;

      // Structure Menu actions
      QAction *strGlobalCutAction, *strGlobalInsertAction, *strGlobalSplitAction, *strCopyRangeAction, *strCutEventsAction;

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

      // Settings Menu Actions
      QAction *settingsGlobalAction, *settingsShortcutsAction, *settingsMetronomeAction, *settingsMidiSyncAction;
      QAction *settingsMidiIOAction, *settingsAppearanceAction, *settingsMidiPortAction;
      QAction *dontFollowAction, *followPageAction, *followCtsAction;

      // Help Menu Actions
      QAction *helpManualAction, *helpHomepageAction, *helpReportAction, *helpAboutAction;

      QString appName;

      QFileInfo project;
      QToolBar *tools;
      EditToolBar *tools1;

      Transport* transport;
      BigTime* bigtime;
      EditInstrument* editInstrument;
      
      QMenu *menu_file, *menuView, *menuSettings, *menu_help;
      QMenu *menuEdit, *menuStructure;
      QMenu* menu_audio, *menuAutomation;
      QMenu* menu_functions, *menuScriptPlugins;
      QMenu* select, *master, *midiEdit, *addTrack;

      // Special 'stay-open' menu for routes.
      PopupMenu* routingPopupMenu; 
      //PopupView* routingPopupView; 


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

      ToplevelList toplevels;
      ClipListEdit* clipListEdit;
      MarkerView* markerView;
      MidiTransformerDialog* midiTransformerDialog;
      QMenu* openRecent;
      
      bool readMidi(FILE*);
      void read(Xml& xml, bool skipConfig);
      void processTrack(MidiTrack* track);

      void write(Xml& xml) const;
      bool clearSong();
      bool save(const QString&, bool);
      void setUntitledProject();
      void setConfigDefaults();

      void setFollow();
      void readConfigParts(Xml& xml);
      void readMidiport(Xml& xml);
      void readMidichannel(Xml& xml, int port);
      void readCtrl(Xml& xml, int port, int channel);
      void readToplevels(Xml& xml);
      PartList* getMidiPartsToEdit();
      Part* readPart(Xml& xml);
      bool checkRegionNotNull();
      void loadProjectFile1(const QString&, bool songTemplate, bool loadAll);
      void writeGlobalConfiguration(int level, Xml&) const;
      void writeConfiguration(int level, Xml&) const;
      void updateConfiguration();

      virtual void focusInEvent(QFocusEvent*);

      QSignalMapper *editSignalMapper;
      QSignalMapper *midiPluginSignalMapper;
      QSignalMapper *followSignalMapper;

   signals:
      void configChanged();

   private slots:
      //void runPythonScript();
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

      void toggleTransport(bool);
      void toggleMarker(bool);
      void toggleBigTime(bool);
      //void toggleMixer();
      void toggleMixer1(bool);
      void toggleMixer2(bool);

      void configMidiPorts();
      void configMidiSync();
      void configMidiFile();
      void configShortCuts();
      void configMetronome();
      void configAppearance();
      void startEditor(PartList*, int);
      void startMasterEditor();
      void startLMasterEditor();
      void startListEditor();
      void startListEditor(PartList*);
      void startDrumEditor();
      void startDrumEditor(PartList*);
      void startEditor(Track*);
      void startPianoroll();
      void startPianoroll(PartList* pl);
      void startWaveEditor();
      void startWaveEditor(PartList*);
      void startSongInfo(bool editable=true);

      void startMidiTransformer();
      void writeGlobalConfiguration() const;
      void startEditInstrument();
      void startClipList(bool);
      
      void openRecentMenu();
      void selectProject(QAction* act);
      void cmd(int);
      void clipboardChanged();
      void selectionChanged();
      void transpose();
      void modifyGateTime();
      void modifyVelocity();
      void crescendo();
      void thinOut();
      void eraseEvent();
      void noteShift();
      void moveClock();
      void copyMeasure();
      void eraseMeasure();
      void deleteMeasure();
      void createMeasure();
      void mixTrack();
      void startMidiInputPlugin(int);
      void hideMitPluginTranspose();
      void hideMidiInputTransform();
      void hideMidiFilterConfig();
      void hideMidiRemoteConfig();
      void hideMidiRhythmGenerator();
      void globalCut();
      void globalInsert();
      void globalSplit();
      void copyRange();
      void cutEvents();
      void bounceToTrack();
      void resetMidiDevices();
      void initMidiDevices();
      void localOff();
      void switchMixerAutomation();
      void takeAutomationSnapshot();
      void clearAutomation();
      void bigtimeClosed();
      //void mixerClosed();
      void mixer1Closed();
      void mixer2Closed();
      void markerClosed();

      void execDeliveredScript(int);
      void execUserScript(int);

   public slots:
      bool saveAs();
      void bounceToFile(AudioOutput* ao = 0);
      void closeEvent(QCloseEvent*e);
      void loadProjectFile(const QString&);
      void loadProjectFile(const QString&, bool songTemplate, bool loadAll);
      void toplevelDeleted(unsigned long tl);
      void loadTheme(const QString&);
      void loadStyleSheetFile(const QString&);
      bool seqRestart();
      void loadTemplate();
      void showBigtime(bool);
      //void showMixer(bool);
      void showMixer1(bool);
      void showMixer2(bool);
      void showMarker(bool);
      void importMidi(const QString &file);
      void setUsedTool(int);
      void showDidYouKnowDialog();
      
      void routingPopupMenuAboutToHide();

   public:
      MusE(int argc, char** argv);
      Arranger* arranger;
      QRect configGeometryMain;
      bool importMidi(const QString name, bool merge);
      void kbAccel(int);
      void changeConfig(bool writeFlag);

      void seqStop();
      bool seqStart();
      void setHeartBeat();
      void importController(int, MidiPort*, int);
      //QWidget* mixerWindow();
      QWidget* mixer1Window();
      QWidget* mixer2Window();
      QWidget* transportWindow();
      QWidget* bigtimeWindow();
      bool importWaveToTrack(QString& name, unsigned tick=0, Track* track=NULL);
      void importPartToTrack(QString& filename, unsigned tick, Track* track);

      void showTransport(bool flag);

      // Special 'stay-open' menu for routes.
      PopupMenu* getRoutingPopupMenu();
      PopupMenu* prepareRoutingPopupMenu(Track* /*track*/, bool /*dst*/);
      void routingPopupMenuActivated(Track* /*track*/, int /*id*/);
      void updateRouteMenus(Track* /*track*/, QObject* /*master*/);
      // Testing...
      //PopupView* getRoutingPopupView();
      //PopupView* prepareRoutingPopupView(Track* /*track*/, bool /*dst*/);
      //void routingPopupViewActivated(Track* /*track*/, int /*id*/);
      
#ifdef HAVE_LASH
      void lash_idle_cb ();
#endif
      };

extern void addProject(const QString& name);
#endif

