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

#include <stdio.h>

#include "latency_info.h"

namespace MusECore {

void TrackLatencyInfo::initialize()
{
  _dominanceProcessed = false;
  _dominanceInputProcessed = false;
  _canDominateProcessed = false;
  _canDominateInputProcessed = false;
  _correctionProcessed = false;
  _correctionInputProcessed = false;
  _worstPluginLatencyProcessed = false;
  _worstPluginLatency = 0.0f;
  _worstPortLatencyProcessed = false;
  _worstPortLatency = 0.0f;
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
  _canCorrectOutputLatency = false;
  _sourceCorrectionValue = 0.0f;
  _compensatorWriteOffset = 0;
  
  // Special for Midi Tracks:
  _latencyOutMidiTrack = 0.0f;
  // Special for Metronome:
  _latencyOutMetronome = 0.0f;
}

void TrackLatencyInfo::dump(const char *objname, const char *header) const
{
  fprintf(stderr,
    "====%s=%s=TrackLatencyInfo=========\n"
    "_dominanceProcessed:%d\n"
    "_dominanceInputProcessed:%d\n"
    "canDominateProcessed:%d\n"
    "canDominateInputProcessed:%d\n"
    "correctionProcessed:%d\n"
    "correctionInputProcessed:%d\n"
    "worstPluginLatencyProcessed:%d\n"
    "worstPluginLatency:%f\n"
    "worstPortLatencyProcessed:%d\n"
    "worstPortLatency:%f\n"
    "processed:%d\n"
    "inputProcessed:%d\n"
    "worstSelfLatency:%f\n"
    "worstSelfLatencyMidi:%f\n"
    "worstSelfLatencyProcessed:%d\n"
    "worstSelfLatencyMidiProcessed:%d\n"
    "outputLatency:%f\n"
    "inputLatency:%f\n"
    "isLatencyInputTerminal:%d\n"
    "isLatencyOutputTerminal:%d\n"
    "isLatencyInputTerminalProcessed:%d\n"
    "isLatencyOutputTerminalProcessed:%d\n"
    "canDominateOutputLatency:%d\n"
    "canDominateInputLatency:%d\n"
    "canCorrectOutputLatency:%d\n"
    "sourceCorrectionValue:%f\n"
    "compensatorWriteOffset:%ld\n"

    "latencyOutMidiTrack:%f\n"
    "latencyOutMetronome:%f\n"
    "====================================\n",

    objname,
    header,
    _dominanceProcessed,
    _dominanceInputProcessed,
    _canDominateProcessed,
    _canDominateInputProcessed,
    _correctionProcessed,
    _correctionInputProcessed,
    _worstPluginLatencyProcessed,
    _worstPluginLatency,
    _worstPortLatencyProcessed,
    _worstPortLatency,
    _processed,
    _inputProcessed,
    _worstSelfLatency,
    _worstSelfLatencyMidi,
    _worstSelfLatencyProcessed,
    _worstSelfLatencyMidiProcessed,
    _outputLatency,
    _inputLatency,
    _isLatencyInputTerminal,
    _isLatencyOutputTerminal,
    _isLatencyInputTerminalProcessed,
    _isLatencyOutputTerminalProcessed,
    _canDominateOutputLatency,
    _canDominateInputLatency,
    _canCorrectOutputLatency,
    _sourceCorrectionValue,
    _compensatorWriteOffset,
    _latencyOutMidiTrack,
    _latencyOutMetronome
  );

}

} // namespace MusECore
