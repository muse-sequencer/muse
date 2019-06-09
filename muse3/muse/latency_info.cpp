//===================================================================
//  MusE
//  Linux Music Editor
//
//  latency_info.cpp
//  (C) Copyright 2019 Tim E. Real (terminator356 on users dot sourceforge dot net)
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
//===================================================================

#include "latency_info.h"

namespace MusECore {

void TrackLatencyInfo::initialize()
{
  _dominanceProcessed = false;
  _dominanceInputProcessed = false;
  _canDominateProcessed = false;
  _canDominateInputProcessed = false;
//     _forwardProcessed = false;
  _correctionProcessed = false;
  _correctionInputProcessed = false;
    _worstPluginLatencyProcessed = false;
    _worstPluginLatency = 0.0f;
  _processed = false;
  _inputProcessed = false;
  _worstSelfLatency = 0.0f;
  _worstSelfLatencyMidi = 0.0f;
  _worstSelfLatencyProcessed = false;
  _worstSelfLatencyMidiProcessed = false;
  _outputLatency = 0.0f;
  _inputLatency = 0.0f;
  _isLatencyInputTerminal = false;
  _isLatencyOutputTerminal = false;
  _isLatencyInputTerminalProcessed = false;
  _isLatencyOutputTerminalProcessed = false;
  _canDominateOutputLatency = false;
  _canDominateInputLatency = false;
//   _requiresInputCorrection = false;
  _canCorrectOutputLatency = false;
//   _canCorrectInputLatency = false;
  _sourceCorrectionValue = 0.0f;
  _compensatorWriteOffset = 0;
  
  // Special for Midi Tracks:
  _latencyOutMidiTrack = 0.0f;
  // Special for Metronome:
  _latencyOutMetronome = 0.0f;
}

} // namespace MusECore
