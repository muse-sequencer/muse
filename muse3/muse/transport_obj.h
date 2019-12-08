//=========================================================
//  MusE
//  Linux Music Editor
//
//  latency_info.h
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
//=========================================================

#ifndef __TRANSPORT_OBJ_H__
#define __TRANSPORT_OBJ_H__

#include "latency_info.h"

namespace MusECore {

class TransportSource
{
private:
  TrackLatencyInfo _latencyInfo;
  float _transportLatencyOut;
  bool _canCorrect;

public:
  TransportSource() { _latencyInfo.initialize(); _transportLatencyOut = 0.0f; _canCorrect = false; }

  // Initializes this track's latency information in preparation for a latency scan.
  void prepareLatencyScan(bool can_correct_latency = false);
  // Whether this track (and the branch it is in) can force other parallel branches to
  //  increase their latency compensation to match this one.
  // If false, this branch will NOT disturb other parallel branches' compensation,
  //  intead only allowing compensation UP TO the worst case in other branches.
  bool canDominateOutputLatency() const { return false; }
  bool canDominateInputLatency() const { return false; }
  // Whether this track (and the branch it is in) can force other parallel branches to
  //  increase their latency compensation to match this one - IF this track is an end-point
  //  and the branch allows domination.
  // If false, this branch will NOT disturb other parallel branches' compensation,
  //  intead only allowing compensation UP TO the worst case in other branches.
  bool canDominateEndPointLatency() const { return false; }
  // Whether this track and its branch can correct for latency, not just compensate.
  // Special for transport source: Yes, it can correct its own latency, but without
  //  further mechanisms we cannot know whether the transport information affects the 
  //  actual sound, or just an arpeggiator for example. See getDominanceInfo() for info.
  bool canCorrectOutputLatency() const { return true; }
  // Whether the track can pass latency values through, the SAME as if record monitor is
  //  supported and on BUT does not require record monitor support.
  // This is for example in the metronome MetronomeSynthI, since it is unique in that it
  //  can correct its own latency unlike other synths, but it does not 'pass through'
  //  the latency values to what drives it like other synths.
  bool canPassThruLatency() const { return false; }
  // Whether any of the connected output routes are effectively connected.
  // That means track is not off, track is monitored where applicable, etc,
  //   ie. signal can actually flow.
  // For Wave Tracks for example, asks whether the track is an end-point from the view of the input side.
  bool isLatencyInputTerminal();
  // Whether any of the connected output routes are effectively connected.
  // That means track is not off, track is monitored where applicable, etc,
  //   ie. signal can actually flow.
  // For Wave Tracks for example, asks whether the track is an end-point from the view of the playback side.
  bool isLatencyOutputTerminal();

  TrackLatencyInfo& getDominanceInfo(bool input);
  TrackLatencyInfo& getDominanceLatencyInfo(bool input);
  // The finalWorstLatency is the grand final worst-case latency, of any output track or open branch,
  //  determined in the complete getDominanceLatencyInfo() scan.
  // The callerBranchLatency is the inherent branch latency of the calling track, or zero if calling from
  //  the very top outside of the branch heads (outside of output tracks or open branches).
  // The callerBranchLatency is accumulated as setCorrectionLatencyInfo() is called on each track
  //  in a branch of the graph.
  TrackLatencyInfo& setCorrectionLatencyInfo(
    bool input, float finalWorstLatency,
    float callerBranchLatency = 0.0f, bool commonProjectLatency = false);
  // Argument 'input': Whether we want the input side of the track. For example un-monitored wave tracks
  //  are considered two separate paths with a recording input side and a playback output side.
  TrackLatencyInfo& getLatencyInfo(bool input);
  
  float transportLatencyOut() const { return _transportLatencyOut; }
  void setTransportLatencyOut(float f) { _transportLatencyOut = f; }
};

} // namespace MusECore

#endif
