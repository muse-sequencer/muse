//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: gconfig.cpp,v 1.15.2.13 2009/12/01 03:52:40 terminator356 Exp $
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

#include "gconfig.h"

namespace MusEGlobal {

/*   --- PLEASE READ BEFORE EDITING ---
 *
 * The values below are default init parameters for most MusE
 * configuration parameters.
 * They are however NOT certain to have this value!
 *
 * This is for two reasons:
 * 1) MusE has a config file which overrides these values
 * 2) When no configuration file exists a default TEMPLATE
 *    is loaded from the share dir which overwrites most
 *    of these values.
 *    In a perfect world it would overwrite all values and
 *    these would be removed.
 * 
 * SE 3 2019: In a perfect world ALL configuration values are stored here.
 *    And most are. But some are not, they are scattered throughout the app.
 *    But they are not THAT difficult to find by reverse looking up an xml tag
 *     in an existing config file to find out what uses it.
 *    We should strive to keep ALL config values here, where each item is
 *     guaranteed to have a value. Some devs (me included) are neglecting
 *     to put new values in the template. So it becomes ineffective.
 * 
 *    Therefore item 2) is no longer true. It has been disabled in main.cpp.
 *    Tim.
 */

GlobalConfigValues config = {
      QStringList(),              // pluginLadspaPathList
      QStringList(),              // pluginDssiPathList
      QStringList(),              // pluginVstPathList
      QStringList(),              // pluginLinuxVstPathList
      QStringList(),              // pluginLv2PathList
      false,                      // pluginCacheTriggerRescan Whether to trigger a plugin cache rescan.
      170,                        // globalAlphaBlend    
      {
        QColor(0xff, 0xff, 0xff),   // palette
        QColor(0xff, 0xff, 0xff),
        QColor(0xff, 0xff, 0xff),
        QColor(0xff, 0xff, 0xff),
        QColor(0xff, 0xff, 0xff),
        QColor(0xff, 0xff, 0xff),
        QColor(0xff, 0xff, 0xff),
        QColor(0xff, 0xff, 0xff),
        QColor(0xff, 0xff, 0xff),
        QColor(0xff, 0xff, 0xff),
        QColor(0xff, 0xff, 0xff),
        QColor(0xff, 0xff, 0xff),
        QColor(0xff, 0xff, 0xff),
        QColor(0xff, 0xff, 0xff),
        QColor(0xff, 0xff, 0xff),
        QColor(0xff, 0xff, 0xff)
        },
      {
        QColor(255,  232,  140),   // part colors
        QColor(0xff, 0x00, 0x00),
        QColor(0x00, 0xff, 0x00),
        QColor(0x00, 0x00, 0xff),
        QColor(0xff, 0xff, 0x00),
        QColor(0x00, 0xff, 0xff),
        QColor(0xff, 0x00, 0xff),
        QColor(0x9f, 0xc7, 0xef),
        QColor(0x00, 0xff, 0x7f),
        QColor(0x7f, 0x00, 0x00),
        QColor(0x00, 0x7f, 0x00),
        QColor(0x00, 0x00, 0x7f),
        QColor(0x7f, 0x7f, 0x3f),
        QColor(0x00, 0x7f, 0x7f),
        QColor(0x7f, 0x00, 0x7f),
        QColor(0x00, 0x7f, 0xff),
        QColor(0x00, 0x3f, 0x3f),
        QColor(170, 85, 0)
      },
      {
        QString("Default"),   // Default part color names
        QString("Refrain"),
        QString("Bridge"),
        QString("Intro"),
        QString("Coda"),
        QString("Chorus"),
        QString("Solo"),
        QString("Brass"),
        QString("Percussion"),
        QString("Drums"),
        QString("Guitar"),
        QString("Bass"),
        QString("Flute"),
        QString("Strings"),
        QString("Keyboard"),
        QString("Piano"),
        QString("Saxophone"),
        QString("Organ")
      },
      QColor(51, 114, 178),   // transportHandleColor;
      QColor(219, 65, 65),    // bigTimeForegroundColor;
      QColor(0, 0, 0),        // bigTimeBackgroundColor;
      QColor(200, 192, 171),  // waveEditBackgroundColor;
      {
        QFont(QString("arial"), 10, QFont::Normal),
        QFont(QString("arial"), 7,  QFont::Normal),    // Mixer strips and midi track info panel
        QFont(QString("arial"), 10, QFont::Normal),
        QFont(QString("arial"), 10, QFont::Bold),
        QFont(QString("arial"), 8,  QFont::Normal),    // Small numbers: Timescale and markers, part name overlay
        QFont(QString("arial"), 8,  QFont::Bold),      // Small bold numbers such as marker text
        QFont(QString("arial"), 8,  QFont::Bold, true)  // Mixer strip labels. Looks and fits better with bold + italic than bold alone, 
                                                        //  at the price of only few more pixels than Normal mode.
        },
      QColor(84, 97, 114),          // trackBg;
      QColor(109, 174, 178),        // selected track Bg;
      QColor(0x00, 0x00, 0x00),     // selected track Fg;
      Qt::gray,                     // trackSectionDividerColor;
      
      QColor(74, 150, 194),         // midiTrackLabelBg;   // Med blue
      QColor(150, 177, 189),        // drumTrackLabelBg;   // Pale pastel blue
      QColor(116, 232, 242),        // newDrumTrackLabelBg; // Light blue
      QColor(213, 128, 202),        // waveTrackLabelBg;   // magenta
      QColor(84, 185, 58),          // outputTrackLabelBg; // green
      QColor(199, 75, 64),          // inputTrackLabelBg;  // red
      QColor(236, 214, 90),         // groupTrackLabelBg;  // yellow
      QColor(142, 157, 6),          // auxTrackLabelBg;    // Med olive
      QColor(229, 157, 101),        // synthTrackLabelBg;  // Med orange
      
      QColor(215, 220, 230),     // midiTrackBg;
      QColor(215, 220, 230),     // drumTrackBg;
      QColor(215, 220, 230),     // newDrumTrackBg;
      QColor(220, 209, 217),     // waveTrackBg;
      QColor(197, 220, 206),     // outputTrackBg;
      QColor(220, 214, 206),     // inputTrackBg;
      QColor(220, 216, 202),     // groupTrackBg;
      QColor(208, 215, 220),     // auxTrackBg;
      QColor(220, 211, 202),     // synthTrackBg;
      
      QColor(98, 124, 168),      // part canvas bg
      QColor(255, 170, 0),       // ctrlGraphFg;    Medium orange
      QColor(0, 0, 0),           // mixerBg;

      QColor(0xe0, 0xe0, 0xe0),     // Ruler background
      QColor(0, 0, 0),              // Ruler text
      QColor(255, 255, 255),        // Midi editor canvas
      QColor(255, 255, 255),        // midiControllerViewBg
      QColor(255, 255, 255),        // drumListBg
      QColor(255, 255, 255),        // rulerCurrent
      Qt::gray,                     // midiCanvasBeatColor
      Qt::black,                    // midiCanvasBarColor
      Qt::lightGray,                // waveNonselectedPart
      Qt::darkGray,                 // wavePeakColor
      Qt::black,                    // waveRmsColor
      Qt::lightGray,                // wavePeakColorSelected
      Qt::white,                    // waveRmsColorSelected

      Qt::darkGray,                 // partWaveColorPeak
      QColor(20,20,20),             // partWaveColorRms
      QColor(54,54,54),             // partMidiDarkEventColor
      QColor(200,200,200),          // partMidiLightEventColor

      QColor(0,181,241  ),          // sliderBarDefaultColor
      QColor(228,203,36 ),          // sliderDefaultColor
      QColor(78,172,35  ),          // panSliderColor
      QColor(209,86,86  ),          // gainSliderColor
      QColor(190,190,39 ),          // auxSliderColor
      QColor(154,135,124),          // audioVolumeSliderColor
      QColor(153,156,124),          // midiVolumeSliderColor
      QColor(37,121,255 ),          // audioControllerSliderDefaultColor
      QColor(220,77,255 ),          // audioPropertySliderDefaultColor
      QColor(37,121,255 ),          // midiControllerSliderDefaultColor
      QColor(220,77,255 ),          // midiPropertySliderDefaultColor
      QColor(100,255,255),          // midiPatchReadoutColor
      QColor(0,221,255  ),          // audioMeterPrimaryColor
      QColor(0,221,255  ),          // midiMeterPrimaryColor
      QColor(208,145,49 ),          // rackItemBackgroundColor

      MusEGlobal::WaveOutLine,      // waveDrawing

      true,                         // useThemeIconsIfPossible Whether to try to see if various icons are available from the theme.
      
      false,                        // fixFrozenMDISubWindows Turn on a fix for frozen MDIs in Breeze/Oxygen themes.
      
      // maxAliasedPointSize At what point size to switch from aliased text to non-aliased text. 
      // Zero means always use anti-aliasing. For certain widgets that use it. May be more later.
      8, 
      
      false,                        // enableAlsaMidiDriver Whether to enable the ALSA midi driver
      384,                          // division;
      1024,                         // rtcTicks
      0,                            // curMidiSyncInPort The currently selected midi sync input port.
      true,                         // midiSendInit Send instrument initialization sequences
      true,                         // warnInitPending Warn instrument initialization sequences pending
      false,                        // midiSendCtlDefaults Send instrument controller defaults at position 0 if none in song
      false,                        // midiSendNullParameters Send null parameters after each (N)RPN event
      false,                        // midiOptimizeControllers Don't send redundant H/L parameters or H/L values
      true,                         // warnIfBadTiming Warn if timer res not good
      false,                        // velocityPerNote Whether to show per-note or all velocities
      -60,                          // int minMeter;
      -60.0,                        // double minSlider;
      false,                        // use Jack freewheel
      20,                           // int guiRefresh;
      QString(""),                  // userInstrumentsDir  // Obsolete. Must keep for compatibility.
      //QString(""),                // helpBrowser; // Obsolete
      true,                         // extendedMidi
      384,                          // division for smf export
      QString(""),                  // copyright string for smf export
      1,                            // smf export file format
      false,                        // midi export file 2 byte timesigs instead of 4
      true,                         // optimize midi export file note offs
      true,                         // expRunningStatus; Save space by using running status
      true,                         // Split imported tracks into multiple parts.
      true,                         // importMidiNewStyleDrum
      true,                         // importDevNameMetas Import Prefer Device Name metas over port number metas if both exist.
      true,                         // importInstrNameMetas Import Prefer Instrument Name metas over Mode sysexes if both exist.
      MusEGlobal::PORT_NUM_META | MusEGlobal::DEVICE_NAME_META, // exportPortsDevices. Or'd ExportPortsDevices_t flags. Export port number metas and/or device name metas.
      true,                         // exportPortDeviceSMF0 Export a port and/or device meta even for SMF0.
      MusEGlobal::MODE_SYSEX | MusEGlobal::INSTRUMENT_NAME_META, // exportModeInstr. Or'd ExportModeInstr_t flags. Export mode sysexes and/or instrument name metas.
      QString("GM"),                // importMidiDefaultInstr Default to this instrument not Generic, if no match found
      true,                         // exportDrumMapOverrides Apply drum map overrides to export
      true,                         // exportChannelOverridesToNewTrack Drum map Channel overrides go to a separate track
      1,                            // startMode
      QString(""),                  // start song path
      false,                        // startSongLoadConfig
      384,                          // gui division
      QRect(0, 0, 700, 550),        // GeometryMain;
      QRect(0, 0, 200, 100),        // GeometryTransport;
      QRect(0, 0, 600, 200),        // GeometryBigTime;
      {
         QString("Mixer A"),
         QStringList(),
         QRect(0, 0, 300, 500),        // Mixer1
         true, true, true, true,
         true, true, true, true, true,
         MusEGlobal::MixerConfig::STRIPS_TRADITIONAL_VIEW,
         QList<bool>(),
         QList<StripConfig>()
         },
      {
         QString("Mixer B"),
         QStringList(),
         QRect(200, 200, 300, 500),    // Mixer2
         true, true, true, true,
         true, true, true, true, true,
         MusEGlobal::MixerConfig::STRIPS_TRADITIONAL_VIEW,
         QList<bool>(),
         QList<StripConfig>()
      },
      true,                         // TransportVisible;
      false,                        // BigTimeVisible;
      false,                        // mixer1Visible;
      false,                        // mixer2Visible;
      false,                        // markerVisible;
      true,                         // arrangerVisible;
      true,                         // showSplashScreen
      1,                            // canvasShowPartType 1 - names, 2 events
      5,                            // canvasShowPartEvent
      true,                         // canvasShowGrid;
      QString(""),                  // canvasBgPixmap;
      QStringList(),                // canvasCustomBgList
      QString(""),                  // default styleSheetFile - For built-in set to ":/style.qss"
      QString(""),                  // style
      QString("sweep"),             // externalWavEditor
      false,                        // useOldStyleStopShortCut
      false,                        // moveArmedCheckBox
      true,                         // useDenormalBias
      false,                        // useOutputLimiter
      true,                         // showDidYouKnow
      false,                        // vstInPlace  Enable VST in-place processing

      44100,                        // Device audio preferred sample rate
      512,                          // Device audio buffer size
      0,                            // Device RtAudio selected backend

      QString("./"),                // projectBaseFolder
      true,                         // projectStoreInFolder
      true,                         // useProjectSaveDialog
      256,                          // minControlProcessPeriod
      false,                        // popupsDefaultStayOpen
      false,                        // leftMouseButtonCanDecrease
      false,                        // rangeMarkerWithoutMMB
      MusECore::DONT_REC_MUTED_OR_HIDDEN,
      true,                         // addHiddenTracks
      true,                         // unhideTracks
      MusEGlobal::PREFER_NEW,       // drumTrackPreference
      true,                         // smartFocus
      20,                           // trackHeight
      true,                         // borderlessMouse
      false,                        // autoSave
      false,                        // scrollableSubMenus
      true,                         // liveWaveUpdate
      true,                         // warnOnFileVersions Warn if file version different than current
      MusEGlobal::CONF_LV2_UI_USE_FIRST, //lv2UiBehavior
      true,                         // preferKnobsVsSliders Whether to prefer the use of knobs over sliders, esp in mixer.
      true,                         // showControlValues Whether to show the value along with label in small controls, esp in mixer.
      true,                         // monitorOnRecord  Whether to automatically monitor on record arm.
      true,                         // lineEditStyleHack Force line edit widgets to draw a frame at small sizes. Some styles refuse to draw the frame.
      false,                        // preferMidiVolumeDb Prefer midi volume as decibels instead of 0-127.
      true,                         // midiCtrlGraphMergeErase Whether to erase underlying erase target items when dragging/dropping source items.
      false,                        // midiCtrlGraphMergeEraseInclusive Whether to erase target items in-between source item groups.
      true,                         // midiCtrlGraphMergeEraseWysiwyg Whether to erase past the last item in a group to include its original source width.
      QString("klick1.wav"),        // measSample
      QString("klick2.wav"),        // beatSample
      QString("klick3.wav"),        // accent1Sample
      QString("klick4.wav"),        // accent2Sample
      MusEGlobal::RoutePreferCanonicalName,  // preferredRouteNameOrAlias
      true,                         // routerExpandVertically
      2,                            // routerGroupingChannels
      false,                        // enableLatencyCorrection.
      false,                        // correctUnterminatedOutBranchLatency
      false,                        // correctUnterminatedInBranchLatency
      false,                        // monitoringAffectsLatency
      false,                        // commonProjectLatency
      "",                           // mixdownPath
      true,                         // showNoteNamesInPianoRoll
      false                         // selectionsUndoable Whether selecting parts or events is undoable.
    };

} // namespace MusEGlobal
