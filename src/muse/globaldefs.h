//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: globaldefs.h,v 1.3.2.1 2009/05/03 04:14:00 terminator356 Exp $
//
//  (C) Copyright 2000 Werner Schweer (ws@seh.de)
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

#ifndef __GLOBALDEFS_H__
#define __GLOBALDEFS_H__

namespace MusECore {

// Midi Type
//    MT_GM  - General Midi
//    MT_GS  - Roland GS
//    MT_XG  - Yamaha XG
//    MT_GM2 - General Midi Level 2

enum MType { MT_UNKNOWN=0, MT_GM, MT_GS, MT_XG, MT_GM2 };

enum AutomationType {
      AUTO_OFF, AUTO_READ, AUTO_TOUCH, AUTO_WRITE,
      AUTO_LATCH
      };

// Can be Or'd together.
enum PluginFeature {
  PluginNoFeatures = 0x00,
  PluginFixedBlockSize = 0x01,
  PluginPowerOf2BlockSize = 0x02,
  PluginNoInPlaceProcessing = 0x04,
  PluginCoarseBlockSize = 0x08,
  PluginSupportStrictBounds = 0x10
};
typedef int PluginFeatures_t;

enum PluginBypassType {
  // The plugin has no bypass or enable feature.
  // We emulate an enable function with no controller graph.
  PluginBypassTypeEmulatedEnableFunction = 0,
  // The plugin has no bypass or enable feature.
  // We emulate an enable function with controller graph.
  PluginBypassTypeEmulatedEnableController,
  // The plugin has an enable function.
  // We provide an enable function with no controller graph.
  PluginBypassTypeEnableFunction,
  // The plugin has an enable controller port.
  // We provide an enable controller graph.
  PluginBypassTypeEnablePort,
  // The plugin has a bypass function.
  // We provide an (inverted) enable function with no controller graph.
  PluginBypassTypeBypassFunction,
  // The plugin has a bypass controller port.
  // We provide a bypass controller graph.
  PluginBypassTypeBypassPort
};

enum PluginLatencyReportingType {
  // Plugin has no latency reporting mechanism.
  PluginLatencyTypeNone = 0,
  // Plugin has a latency reporting function but no controller port.
  PluginLatencyTypeFunction,
  // Plugin has a latency reporting controller port.
  PluginLatencyTypePort
};

enum PluginFreewheelType {
  // Plugin has no freewheel mechanism.
  PluginFreewheelTypeNone = 0,
  // Plugin has a freewheel function but no controller port.
  PluginFreewheelTypeFunction,
  // Plugin has a freewheel controller port.
  PluginFreewheelTypePort
};

enum VstPluginFlags
{
  vstPluginNoFlags          = 0,
  canSendVstEvents          = 1 << 0,
  canSendVstMidiEvents      = 1 << 1,
  canReceiveVstEvents       = 1 << 3,
  canReceiveVstMidiEvents   = 1 << 4,
  canReceiveVstTimeInfo     = 1 << 5,
  canProcessVstOffline      = 1 << 6,
  canVstMidiProgramNames    = 1 << 10,
  canVstBypass              = 1 << 11
};
typedef int VstPluginFlags_t;

enum VstPluginToHostFlags
{
  vstHostNoFlags                          = 0,
  canHostSendVstEvents                    = 1 << 0,
  canHostSendVstMidiEvents                = 1 << 1,
  canHostSendVstTimeInfo                  = 1 << 2,
  canHostReceiveVstEvents                 = 1 << 3,
  canHostReceiveVstMidiEvents             = 1 << 4,
  canHostVstReportConnectionChanges       = 1 << 5,
  canHostVstAcceptIOChanges               = 1 << 6,
  canHostVstSizeWindow                    = 1 << 7,
  canHostVstOffline                       = 1 << 8,
  canHostVstOpenFileSelector              = 1 << 9,
  canHostVstCloseFileSelector             = 1 << 10,
  canHostVstStartStopProcess              = 1 << 11,
  canHostVstShellCategory                 = 1 << 12,
  canHostSendVstMidiEventFlagIsRealtime   = 1 << 13
};
typedef int VstPluginToHostFlags_t;


// Record events ring buffer size
#define MIDI_REC_FIFO_SIZE  256

// Absolute max number of plugins in mixer rack (if we ever want to increase PipelineDepth).
// Used to determine the index where special blocks (dssi ladspa controls) appear in the list of controllers.
// The special block(s) must appear AFTER any rack plugins, so we need this variable to help us
//  leave some room in case we ever want to increase the number of rack plugins.
const int MAX_PLUGINS  = 8;

// plugins in mixer rack, max up to MAX_PLUGINS
const int PipelineDepth = 8;

// max audio channels
const int MAX_CHANNELS = 2;

// max Number of Midi Ports
const int MIDI_PORTS   = 200;

// Midi channels per Port
const int MUSE_MIDI_CHANNELS = 16;

const double MIN_TEMPO_VAL = 20.0;
const double MAX_TEMPO_VAL = 400.0;

// Some non controller IDs to work with.
enum NonControllerId {
  NCTL_UNKNOWN_ID = -1,
  NCTL_TRACK_MUTE = 0,
  NCTL_TRACK_SOLO,
  // Various track properties.
  NCTL_TRACKPROP_TRANSPOSE,
  NCTL_TRACKPROP_DELAY,
  NCTL_TRACKPROP_LENGTH,
  NCTL_TRACKPROP_VELOCITY,
  NCTL_TRACKPROP_COMPRESS };

} // namespace MusECore


namespace MusEGui {
  
enum EditInstrumentTabType {
  EditInstrumentPatches=0,
  EditInstrumentDrumMaps=1, 
  EditInstrumentControllers=2, 
  EditInstrumentSysex=3, 
  EditInstrumentInitSeq=4 };

enum class MidiEventColorMode {
  blueEvents,
  pitchColorEvents,
  velocityColorEvents,
  lastInList
};

// The default amount of space before bar # 1 (or the start of a part).
const int DefaultCanvasXOrigin = -16;

} // namespace MusEGui


#endif

