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

#ifndef __APP_H__
#define __APP_H__

#include "cobject.h"
#include "widgets/tools.h"

namespace AL {
      class Xml;
      class Pos;
      };
using AL::Xml;
using AL::Pos;

class Part;
class PartList;
class Transport;
class BigTime;
class Arranger;
class Track;
class MidiSyncConfig;
class MRConfig;
class MetronomeConfig;
class AudioConf;
class MidiFileConfig;
class MidiFilterConfig;
class MarkerView;
class MidiInputTransformDialog;
class MidiTransformerDialog;
class RhythmGen;
class MidiTrack;
class ShortcutConfig;
class PreferencesDialog;
class EditInstrument;
class Mixer;
class ExportMidiDialog;

extern void configMidiController();

//---------------------------------------------------------
//   MusE
//---------------------------------------------------------

class MusE : public QMainWindow // , public Ui::MuseBase
      {
      Q_OBJECT
      enum {
            CMD_CUT, CMD_COPY, CMD_PASTE, CMD_DELETE,
            CMD_SELECT_ALL, CMD_SELECT_NONE, CMD_SELECT_INVERT,
            CMD_SELECT_ILOOP, CMD_SELECT_OLOOP, CMD_SELECT_PARTS,
            CMD_FOLLOW_NO, CMD_FOLLOW_JUMP, CMD_FOLLOW_CONTINUOUS,
            CMD_DELETE_TRACK
            };

      //File menu items:
      enum { CMD_OPEN_RECENT=0, CMD_IMPORT_MIDI,
            CMD_EXPORT_MIDI, CMD_IMPORT_AUDIO, CMD_QUIT, CMD_OPEN_DRUMS,
            CMD_OPEN_LIST, CMD_OPEN_LIST_MASTER, 
            CMD_OPEN_GRAPHIC_MASTER, CMD_OPEN_MIDI_TRANSFORM, CMD_TRANSPOSE,
            CMD_GLOBAL_CUT, CMD_GLOBAL_INSERT, CMD_GLOBAL_SPLIT, CMD_COPY_RANGE,
            CMD_CUT_EVENTS, CMD_CONFIG_SHORTCUTS, CMD_CONFIG_METRONOME, CMD_CONFIG_MIDISYNC,
            CMD_MIDI_FILE_CONFIG, CMD_CONFIG_AUDIO_PORTS,
            CMD_MIDI_EDIT_INSTRUMENTS, CMD_MIDI_RESET, CMD_MIDI_INIT, CMD_MIDI_LOCAL_OFF,
            CMD_MIXER_SNAPSHOT, CMD_MIXER_AUTOMATION_CLEAR, CMD_OPEN_HELP, CMD_OPEN_HOMEPAGE,
            CMD_OPEN_BUG, CMD_START_WHATSTHIS,
            CMD_AUDIO_BOUNCE_TO_FILE, CMD_AUDIO_BOUNCE_TO_TRACK, CMD_AUDIO_RESTART,
            CMD_OPEN_PROJECT_PROPS,
            CMD_LAST };

      QAction* menu_ids[CMD_LAST];

      QAction* fileSaveAction;
      QAction* fileOpenAction;
      QAction* pianoAction;
      QAction* fileNewAction;

      QString appName;

      QToolBar *tools;
      EditToolBar *tools1;
      int _raster;

      Transport* transport;
      QAction* tr_id;
      BigTime* bigtime;
      QAction* bt_id;
      MarkerView* markerView;
      QAction* mk_id;
	Mixer* mixer1;
      QAction* aid1a;
	Mixer* mixer2;
      QAction* aid1b;

      EditInstrument* editInstrument;

      QMenu *menu_file, *menuView, *menuSettings, *menu_help;
      QMenu *menuEdit, *menuStructure;
      QAction* menuEditActions[CMD_DELETE_TRACK + 1];
      QMenu* menu_audio;
      QMenu* menu_functions;
      QMenu* select, *master, *midiEdit, *addTrack;
      QMenu* follow;
      QMenu* midiInputPlugins;

      QAction* aid2;
      QAction* aid3;
      QAction* fid0;
      QAction* fid1;
      QAction* fid2;
      QAction* cutAction;
      QAction* copyAction;
      QAction* pasteAction;

      QWidget* midiPortConfig;
      QWidget* softSynthesizerConfig;
      MidiSyncConfig* midiSyncConfig;
      MRConfig* midiRemoteConfig;
      RhythmGen* midiRhythmGenerator;
      MetronomeConfig* metronomeConfig;
      AudioConf* audioConfig;
      MidiFileConfig* midiFileConfig;
      MidiFilterConfig* midiFilterConfig;
      MidiInputTransformDialog* midiInputTransform;
      ShortcutConfig* shortcutConfig;
      PreferencesDialog* preferencesDialog;
      ExportMidiDialog* exportMidiDialog;

      MidiTransformerDialog* midiTransformerDialog;
      QMenu* openRecent;
      QSpinBox* globalTempoSpinBox;

      QDialog* projectPropsDialog;

      //------------------------------------------

      bool readMidi(FILE*);
      void processTrack(MidiTrack* track);

      void write(Xml& xml) const;

      void setFollow(FollowMode);
      void readConfigParts(QDomNode);
      void readCtrl(QDomNode, int port, int channel);
      PartList* getMidiPartsToEdit();
      Part* readPart(QDomNode);
      bool checkRegionNotNull();
      void loadProject1(const QString&);
      void writeGlobalConfiguration(Xml&) const;
      void writeConfiguration(Xml&) const;
      void updateConfiguration();

      bool leaveProject();

      virtual void focusInEvent(QFocusEvent*);

   signals:
      void configChanged();
      void rasterChanged(int);
      void startLoadSong();

   private slots:
      void loadProject();
      bool save();
      void quitDoc();
      void about();
      void aboutQt();
      void startHelpBrowser();
      void startHomepageBrowser();
      void startBugBrowser();
      void launchBrowser(QString &whereTo);
      void importMidi();
      void importWave();
      bool importWave(const QString&);
      void exportMidi();

      void configMidiSync();
      void configMidiFile();
      void configShortCuts();

      void startMasterEditor();
      void startListEditor();
      void startListEditor(PartList*);
      void startDrumEditor();
      void startDrumEditor(PartList* pl);
      void startEditor(Part*);
      void startEditor(PartList*, int);
      void startPianoroll();
      void startPianoroll(PartList* pl);
      void startWaveEditor();
      void startWaveEditor(PartList*);
      void writeGlobalConfiguration() const;
      void startEditInstrument();
      void showProjectPropsDialog();

      void openRecentMenu();
      void selectProject(QAction*);
      void cmd(QAction*);
      void clipboardChanged();
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
      void globalCut();
      void globalInsert();
      void globalSplit();
      void copyRange();
      void cutEvents();
      void bounceToTrack();
      void resetMidiDevices();
      void initMidiDevices();
      void localOff();
      void bigtimeClosed();
      void transportClosed();
      void markerClosed();
      void mixer1Closed();
      void mixer2Closed();
      void setRaster(int);
      void syncChanged();
      void preferences();
      void aboutToShowAddTrack();

   public slots:
      void bounceToFile();
      void closeEvent(QCloseEvent*e);
      void loadProject(const QString&);
      void loadTheme(const QString&);
      bool seqStart();
      void showTransport(bool flag);
      void showBigtime(bool);
      void showMixer1(bool);
      void showMixer2(bool);
      void showMarker(bool on);
      void importMidi(const QString &file);
      void globalPitchChanged(int val);
      void globalTempoChanged(int val);
      bool seqRestart();
      void setTempo50();
      void setTempo100();
      void setTempo200();
      void setGlobalTempo(int val);
      void setTool(int);
      void startEditor(Part*, int);

   public:
      MusE();
      Arranger* arranger;
      QRect configGeometryMain;
      bool importMidi(const QString name, bool merge);
      void kbAccel(int);
      void changeConfig(bool writeFlag);

      void seqStop();
      void setHeartBeat();
      QWidget* transportWindow();
      QWidget* bigtimeWindow();
      QWidget* mixer1Window();
      QWidget* mixer2Window();
      bool importWaveToTrack(const QString& name, Track* track, const Pos&);

      void selectionChanged();

      int version;      // last *.med file version
                        // 0xaabb   aa - major version, bb minor version
	int raster() const { return _raster; }
	void setupTransportToolbar(QToolBar* tb) const;
      void readToplevels(QDomNode);

      QAction* startAction;
      QAction* rewindAction;
      QAction* forwardAction;
      QAction* stopAction;
      QAction* playAction;
      };

extern MusE* muse;
extern QStyle* smallStyle;

#endif

