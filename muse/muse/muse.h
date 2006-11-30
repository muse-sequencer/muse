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

#include "shortcuts.h"
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
class ListEdit;
class Ctrl;

extern void configMidiController();

//---------------------------------------------------------
//   MusE
//---------------------------------------------------------

class MusE : public QMainWindow // , public Ui::MuseBase
      {
      Q_OBJECT

      QAction* fileSaveAction;
      QAction* fileOpenAction;
      QAction* pianoAction;
      QAction* waveAction;
      QAction* trackerAction;
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
      ListEdit* listEditor;

      EditInstrument* editInstrument;

      QMenu *menu_file, *menuView, *menuSettings, *menu_help;
      QMenu *menuEdit, *menuStructure;
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
      QComboBox* rasterCombo;

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
      void addMidiFile(const QString name);
      void copyParts(bool);

   signals:
      void configChanged();
      void rasterChanged(int);
      void startLoadSong();

   private slots:
      void loadProject();
      void quitDoc();
      void about();
      void aboutQt();
      void startHelpBrowser();
      void startHomepageBrowser();
      void startBugBrowser();
      void importMidi();
      void importWave();
      bool importWave(const QString&);
      void exportMidi();

      void configMidiSync();
      void configMidiFile();
      void configShortCuts();

      void startMasterEditor();

      void startDrumEditor();
      void startDrumEditor(PartList* pl);
      void startEditor(Part*);
      void startEditor(PartList*, int);
      void startPianoroll();
      void startPianoroll(PartList* pl);
      void startMidiTrackerEditor();
      void startMidiTrackerEditor(PartList* pl);
      void startWaveEditor();
      void startWaveEditor(PartList*);
      void writeGlobalConfiguration() const;
      void startEditInstrument();

      void startListEditor();
      void startListEditor(PartList*);

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
      void syncChanged();
      void preferences();
      void aboutToShowAddTrack();
      void setRaster(int);
      void playToggle();

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
      void setTool(const QString&);
      void startEditor(Part*, int);
      bool save();

   public:
      MusE();
      Arranger* arranger;
      QRect configGeometryMain;
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
      void initRaster(int);

      QAction* startAction;
      QAction* rewindAction;
      QAction* forwardAction;
      QAction* stopAction;
      QAction* playAction;

      void showListEditor(const Pos&, Track*, Ctrl*);
      };

//---------------------------------------------------------
//   MuseApplication
//---------------------------------------------------------

class MuseApplication : public QApplication {
      MusE* muse;

   public:
      MuseApplication(int& argc, char** argv);
      void setMuse(MusE* m) { muse = m; }
      static Shortcut sc[];
      };

extern MusE* muse;
extern QStyle* smallStyle;
extern void addProject(const QString& name);

#endif

