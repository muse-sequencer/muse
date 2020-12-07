//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: gconfig.h,v 1.12.2.10 2009/12/01 03:52:40 terminator356 Exp $
//
//  (C) Copyright 1999-2003 Werner Schweer (ws@seh.de)
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

#ifndef __CONFIG_H__
#define __CONFIG_H__

#define NUM_PARTCOLORS 18
#define NUM_FONTS 7

#include <QColor>
#include <QFont>
#include <QRect>
#include <QString>
#include <QList>
#include <QStringList>

namespace MusECore {
class Xml;

enum newDrumRecordCondition_t
{
  REC_ALL = 0,
  DONT_REC_HIDDEN = 1,
  DONT_REC_MUTED = 2,
  DONT_REC_MUTED_OR_HIDDEN = 3
};

}

namespace MusEGlobal {

// font scaling factors
struct FntFac {
    static constexpr double F1 = 0.7;
    static constexpr double F2 = 0.8;
    static constexpr double F3 = 1.0;
    static constexpr double F4 = 0.8;
    static constexpr double F5 = 0.8;
    static constexpr double F6 = 0.8;
};

// Obsolete. There is only 'New' drum tracks now.
// enum drumTrackPreference_t
// {
//   PREFER_OLD = 0,
//   PREFER_NEW = 1,
//   ONLY_OLD = 2,
//   ONLY_NEW = 3
// };

// Or'd together
enum ExportPortsDevices_t
{
  PORT_NUM_META = 0x01,
  DEVICE_NAME_META = 0x02,
};

// Or'd together
enum ExportModeInstr_t
{
  MODE_SYSEX = 0x01,
  INSTRUMENT_NAME_META = 0x02,
};

enum RouteNameAliasPreference { RoutePreferCanonicalName = 0, RoutePreferFirstAlias = 1, RoutePreferSecondAlias = 2 };

enum WaveDrawing { WaveRmsPeak=1, WaveOutLine=2 };

struct StripConfig {
  // The corresponding track's serial number. Can be -1.
  int _serial;
  // The corresponding track's index in the song file. Can be -1.
  // Temporary during loading to avoid using globally or locally
  //  'unique' identifiers in the song file, such as the serial,
  //  to resolve references.
  int _tmpFileIdx;

  bool _visible;
  int _width;
  bool _deleted;

  StripConfig();
  StripConfig(int trackSerial, bool visible, int width);

  bool isNull() const;

  void write(int level, MusECore::Xml& xml) const;
  void read(MusECore::Xml& xml);
};

typedef QList<StripConfig> StripConfigList_t;
typedef StripConfigList_t::iterator iStripConfigList;
typedef StripConfigList_t::const_iterator ciStripConfigList;

//---------------------------------------------------------
//   MixerConfig
//---------------------------------------------------------

struct MixerConfig {
  enum DisplayOrder {
        STRIPS_TRADITIONAL_VIEW = -1004,
        STRIPS_EDITED_VIEW = -1003,
        STRIPS_ARRANGER_VIEW = -1002,
      };
      QString name;
      // Obsolete. Keep for old song support.
      QStringList stripOrder;
      QRect geometry;
      bool showMidiTracks;
      bool showDrumTracks;
      bool showNewDrumTracks;
      bool showInputTracks;
      bool showOutputTracks;
      bool showWaveTracks;
      bool showGroupTracks;
      bool showAuxTracks;
      bool showSyntiTracks;
      DisplayOrder displayOrder;
      // Obsolete. Keep for old song support.
      QList<bool> stripVisibility;
      // This replaces stripOrder and stripVisibility.
      // NOTE: To avoid having to put this information within a track,
      //  we keep it conveniently here. But this means it does not
      //  participate in the UNDO/REDO system. If a track is 'deleted'
      //  the information here MUST be allowed to exist so that undoing
      //  the track 'delete' finds the info.
      // Thus it acts sort of 'in parallel' to the undo system and is similar
      //  to the undo list (it never dies). The redo list can die, and we
      //  could safely remove these corresponding items. FIXME TODO: DO THAT!
      // Therefore the list must generally be protected from haphazard item
      //  removal for the duration of the song file session.
      // When writing to file BE SURE to ignore items with no corresponding track,
      //  to filter out all the undesired 'deleted' ones.
      StripConfigList_t stripConfigList;

      void write(int level, MusECore::Xml& xml, bool global) const;
      void read(MusECore::Xml& xml);
      };

//---------------------------------------------------------
//   GlobalConfigValues
//---------------------------------------------------------
enum CONF_LV2_UI_BEHAVIOR {
   CONF_LV2_UI_USE_FIRST = 0,
   CONF_LV2_UI_ASK_ONCE,
   CONF_LV2_UI_ASK_ALWAYS
};

enum CONF_SET_RANGE_MARKERS {
    CONF_SET_MARKERS_MID_RIGHT = 0,
    CONF_SET_MARKERS_CTRL_RIGHT_RIGHT,
    CONF_SET_MARKERS_CTRL_LEFT_CTRL_RIGHT,
};

struct GlobalConfigValues {
      QStringList pluginLadspaPathList;
      QStringList pluginDssiPathList;
      QStringList pluginVstPathList;
      QStringList pluginLinuxVstPathList;
      QStringList pluginLv2PathList;
      bool pluginCacheTriggerRescan; // Whether to trigger a plugin cache rescan.

      int globalAlphaBlend;
      QColor palette[16];
      QColor partColors[NUM_PARTCOLORS];
      QString partColorNames[NUM_PARTCOLORS];
      QColor transportHandleColor;
      QColor bigTimeForegroundColor;
      QColor bigTimeBackgroundColor;
      QColor waveEditBackgroundColor;
      QFont fonts[NUM_FONTS];
      bool autoAdjustFontSize;
      QColor trackBg;
      QColor selectTrackBg;
      QColor selectTrackFg;
      QColor selectTrackCurBg;
      QColor trackSectionDividerColor;
      
      QColor midiTrackLabelBg;
      QColor drumTrackLabelBg;
      QColor newDrumTrackLabelBg;
      QColor waveTrackLabelBg;
      QColor outputTrackLabelBg;
      QColor inputTrackLabelBg;
      QColor groupTrackLabelBg;
      QColor auxTrackLabelBg;
      QColor synthTrackLabelBg;

      QColor midiTrackBg;
      QColor drumTrackBg;
      QColor newDrumTrackBg;
      QColor waveTrackBg;
      QColor outputTrackBg;
      QColor inputTrackBg;
      QColor groupTrackBg;
      QColor auxTrackBg;
      QColor synthTrackBg;
      
      QColor partCanvasBg;
      QColor partCanvasCoarseRasterColor;
      QColor partCanvasBeatRasterColor;
      QColor partCanvasFineRasterColor;
      QColor ctrlGraphFg;
      QColor ctrlGraphSel;
//      QColor mixerBg;

      QColor rulerBg;
      QColor rulerFg;
      QColor midiCanvasBg;
      QColor midiControllerViewBg;
      QColor drumListBg;
      QColor drumListFont;
      QColor drumListSel;
      QColor drumListSelFont;
      QColor rulerCurrent;
      QColor midiCanvasFineColor;
      QColor midiCanvasBeatColor;
      QColor midiCanvasBarColor;
      QColor midiItemColor;
      QColor midiItemSelectedColor;
      QColor dummyPartColor;
      QColor midiDividerColor;
      QColor pianoCurrentKey;
      QColor pianoPressedKey;
      QColor pianoSelectedKey;

      QColor waveNonselectedPart;
      QColor wavePeakColor;
      QColor waveRmsColor;
      QColor wavePeakColorSelected;
      QColor waveRmsColorSelected;

      QColor partWaveColorPeak;
      QColor partWaveColorRms;
      QColor partMidiDarkEventColor;
      QColor partMidiLightEventColor;

      QColor sliderBarColor;
      QColor sliderBackgroundColor;
      QColor panSliderColor;
      QColor gainSliderColor;
      QColor auxSliderColor;
      QColor audioVolumeSliderColor;
      QColor midiVolumeSliderColor;
      QColor audioVolumeHandleColor;
      QColor midiVolumeHandleColor;
      QColor audioControllerSliderColor;
      QColor audioPropertySliderColor;
      QColor midiControllerSliderColor;
      QColor midiPropertySliderColor;
      QColor midiPatchReadoutColor;
      QColor knobFontColor;

      QColor audioMeterPrimaryColor;
      QColor midiMeterPrimaryColor;
      QColor meterBackgroundColor;
      
      QColor rackItemBackgroundColor;
      QColor rackItemBgActiveColor;
      QColor rackItemFontColor;
      QColor rackItemFontActiveColor;
      QColor rackItemBorderColor;
      QColor rackItemFontColorHover;

      QColor midiInstrumentBackgroundColor;
      QColor midiInstrumentBgActiveColor;
      QColor midiInstrumentFontColor;
      QColor midiInstrumentFontActiveColor;
      QColor midiInstrumentBorderColor;

      QColor markerColor;
      QColor rangeMarkerColor;
      QColor positionMarkerColor;
      QColor currentPositionColor;

      WaveDrawing waveDrawing;

      // At what point size to switch from aliased text to non-aliased text. Zero means always use anti-aliasing. 
      // For certain widgets that use it. May be more later.
      int maxAliasedPointSize; 

      int iconSize;
      int cursorSize;
      bool cascadeStylesheets;
      bool showIconsInMenus;
      bool useNativeStandardDialogs;

      bool enableAlsaMidiDriver; // Whether to enable the ALSA midi driver
      int division;
      int rtcTicks;
      int curMidiSyncInPort;     // The currently selected midi sync input port.
      bool midiSendInit;         // Send instrument initialization sequences
      bool warnInitPending;      // Warn instrument initialization sequences pending
      bool midiSendCtlDefaults;  // Send instrument controller defaults at position 0 if none in song
      bool midiSendNullParameters; // Send null parameters after each (N)RPN event
      bool midiOptimizeControllers; // Don't send redundant H/L parameters or H/L values
      bool warnIfBadTiming;      // Warn if timer res not good
      bool velocityPerNote;      // Whether to show per-note or all velocities
      int minMeter;
      double minSlider;
      bool freewheelMode;
      int guiRefresh;
      QString userInstrumentsDir;  // Obsolete. Must keep for compatibility.

      bool extendedMidi;      // extended smf format
      int midiDivision;       // division for smf export
      QString copyright;      // copyright string for smf export
      int smfFormat;          // smf export file type
      bool exp2ByteTimeSigs;  // Export 2 byte time sigs instead of 4 bytes
      bool expOptimNoteOffs;  // Save space by replacing note offs with note on velocity 0
      bool expRunningStatus;  // Save space by using running status
      bool importMidiSplitParts; // Split imported tracks into multiple parts.
// Obsolete. There is only 'New' drum tracks now.
//       bool importMidiNewStyleDrum; // Use new style drum tracks
      bool importDevNameMetas;    // Import Prefer Device Name metas over port number metas if both exist.
      bool importInstrNameMetas;  // Import Prefer Instrument Name metas over Mode sysexes if both exist.
      int exportPortsDevices;     // Or'd ExportPortsDevices_t flags. Export port number metas and/or device name metas.
      bool exportPortDeviceSMF0;  // Export a port and/or device meta even for SMF0.
      int exportModeInstr;        // Or'd ExportModeInstr_t flags. Export mode sysexes and/or instrument name metas.
      QString importMidiDefaultInstr;  // Default to this instrument not Generic, if no match found
      bool exportDrumMapOverrides; // Apply Port, Channel, and ANote drum map overrides to export
      bool exportChannelOverridesToNewTrack; // Drum map Channel overrides go to a separate track

      int startMode;          // 0 - start with last song
                              // 1 - start with default template
                              // 2 - start with song
      QString startSong;      // path for start song
      bool startSongLoadConfig;  // Whether to load configuration with the start template or song


      QRect geometryMain;
      QRect geometryTransport;
      QRect geometryBigTime;
      MixerConfig mixer1;
      MixerConfig mixer2;
      bool transportVisible;
      bool bigTimeVisible;
      bool mixer1Visible;
      bool mixer2Visible;
      bool markerVisible;
//      bool arrangerVisible;

      bool showSplashScreen;
      int canvasShowPartType;       // 1 - names, 2 events
      int canvasShowPartEvent;      //
      bool canvasShowGrid;
      bool canvasShowGridHorizontalAlways;
      bool canvasShowGridBeatsAlways;
      QString canvasBgPixmap;
      QStringList canvasCustomBgList;
//      QString styleSheetFile;
//      QString style;
      QString theme;

      QString externalWavEditor;
      bool useOldStyleStopShortCut;
      bool useRewindOnStop;
      bool moveArmedCheckBox;
      bool useDenormalBias;
      bool useOutputLimiter;
      bool showDidYouKnow;
      bool vstInPlace; // Enable VST in-place processing
      int deviceAudioSampleRate;
      int deviceAudioBufSize;
      int deviceAudioBackend;
      bool useJackTransport;
      bool timebaseMaster;

      QString projectBaseFolder;
      bool projectStoreInFolder;
      bool useProjectSaveDialog;
      unsigned long minControlProcessPeriod;
      bool popupsDefaultStayOpen;
      bool leftMouseButtonCanDecrease;
//      bool rangeMarkerWithoutMMB;
      CONF_SET_RANGE_MARKERS rangeMarkersSet;
      MusECore::newDrumRecordCondition_t newDrumRecordCondition;
      bool addHiddenTracks;
      bool unhideTracks;
// Obsolete. There is only 'New' drum tracks now.
//       drumTrackPreference_t drumTrackPreference;
      bool smartFocus;
      int trackHeight;
      bool borderlessMouse;
      bool autoSave;
      bool scrollableSubMenus;
      bool liveWaveUpdate;   //live update wave tracks while recording
      bool warnOnFileVersions; // Warn if file version different than current
      CONF_LV2_UI_BEHAVIOR lv2UiBehavior;
      int audioEffectsRackVisibleItems; // Number of rack slots visible at one time in the rack.
      bool preferKnobsVsSliders; // Whether to prefer the use of knobs over sliders, esp in mixer.
      bool showControlValues; // Whether to show the value along with label in small controls, esp in mixer.
      bool monitorOnRecord;  // Whether to automatically monitor on record arm.
      bool lineEditStyleHack; // Force line edit widgets to draw a frame at small sizes. Some styles refuse to draw the frame.
      bool preferMidiVolumeDb; // Prefer midi volume as decibels instead of 0-127.
      // NOTE: The following are similar to the paste dialog function options, stored separately.
      bool midiCtrlGraphMergeErase; // Whether to erase underlying erase target items when dragging/dropping source items.
      bool midiCtrlGraphMergeEraseInclusive; // Whether to erase target items in-between source item groups.
      bool midiCtrlGraphMergeEraseWysiwyg; // Whether to erase past the last item in a group to include its original source width.
      RouteNameAliasPreference preferredRouteNameOrAlias;
      bool routerExpandVertically; // Whether to expand the router items vertically. (Good use of space but slow!)
      // How to group the router channels together for easier multi-channel manipulation.
      int routerGroupingChannels;
      // Whether to enable latency correction/compensation.
      bool enableLatencyCorrection;
      // Whether to include unterminated output branches in latency correction calculations.
      bool correctUnterminatedOutBranchLatency;
      // Whether to include unterminated input branches in latency correction calculations.
      bool correctUnterminatedInBranchLatency;
      // Whether a track's monitoring feature affects latency.
      bool monitoringAffectsLatency;
      // Whether completely independent branches share a common latency.
      bool commonProjectLatency;
      QString mixdownPath;
      bool showNoteNamesInPianoRoll;
      bool showNoteTooltips;
      // Whether selecting parts or events is undoable.
      // If set, it can be somewhat tedious for the user to step through all the undo/redo items.
      bool selectionsUndoable;
      // Revert native GUI scaling on HiDPI
      bool noPluginScaling;
      // Open MDI windows maximized
//      bool openMDIWinMaximized;
      bool keepTransportWindowOnTop;
      bool showStatusBar;
      };




extern GlobalConfigValues config;
} // namespace MusEGlobal

#endif

