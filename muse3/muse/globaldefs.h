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
      AUTO_OFF, AUTO_READ, AUTO_TOUCH, AUTO_WRITE
      };

// Can be Or'd together.
enum PluginFeature {
  PluginNoFeatures = 0x00,
  PluginFixedBlockSize = 0x01,
  PluginPowerOf2BlockSize = 0x02,
  PluginNoInPlaceProcessing = 0x04,
  PluginCoarseBlockSize = 0x08 };
typedef int PluginFeatures_t;
      
enum VstPluginFlags
{
  vstPluginNoFlags          = 0,
  canSendVstEvents          = 1 << 0,
  canSendVstMidiEvents      = 1 << 1,
  canSendVstTimeInfo        = 1 << 2,
  canReceiveVstEvents       = 1 << 3,
  canReceiveVstMidiEvents   = 1 << 4,
  canReceiveVstTimeInfo     = 1 << 5,
  canProcessVstOffline      = 1 << 6,
  canUseVstAsInsert         = 1 << 7,
  canUseVstAsSend           = 1 << 8,
  canMixVstDryWet           = 1 << 9,
  canVstMidiProgramNames    = 1 << 10
};
typedef int VstPluginFlags_t;

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

} // namespace MusECore


namespace MusEGui {
  
enum EditInstrumentTabType {
  EditInstrumentPatches=0,
  EditInstrumentDrumMaps=1, 
  EditInstrumentControllers=2, 
  EditInstrumentSysex=3, 
  EditInstrumentInitSeq=4 };

} // namespace MusEGui


#endif

