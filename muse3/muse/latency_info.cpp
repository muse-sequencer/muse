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
//     _forwardProcessed = false;
  _correctionProcessed = false;
  _processed = false;
  _trackLatency = 0.0f;
  _outputLatency = 0.0f;
  _isLatencyInputTerminal = false;
  _isLatencyOutputTerminal = false;
  _canDominateOutputLatency = false;
  _requiresInputCorrection = false;
  _canCorrectOutputLatency = false;
  _sourceCorrectionValue = 0.0f;
  _compensatorWriteOffset = 0;
}

} // namespace MusECore
