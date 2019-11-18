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

#include "transport_obj.h"

namespace MusECore {

void TransportSource::prepareLatencyScan(bool can_correct_latency)
{
  _latencyInfo.initialize();
  _transportLatencyOut = 0.0f;
  _canCorrect = can_correct_latency;
}

bool TransportSource::isLatencyInputTerminal()
{
  // Have we been here before during this scan?
  // Just return the cached value.
  if(_latencyInfo._isLatencyInputTerminalProcessed)
    return _latencyInfo._isLatencyInputTerminal;

  // If we're asking for the view from the record side, check if we're
  //  passing the signal through the track via monitoring.
  if(!canPassThruLatency())
  {
    _latencyInfo._isLatencyInputTerminal = true;
    _latencyInfo._isLatencyInputTerminalProcessed = true;
    return true;
  }

//   const RouteList* rl = outRoutes();
//   for (ciRoute ir = rl->begin(); ir != rl->end(); ++ir) {
//     switch(ir->type)
//     {
//       case Route::TRACK_ROUTE:
//         if(!ir->track)
//           continue;
//         if(ir->track->isMidiTrack())
//         {
//           // TODO
//         }
//         else
//         {
//           Track* track = ir->track;
//           if(track->off()) // || 
//             //(atrack->canRecordMonitor() && (MusEGlobal::config.monitoringAffectsLatency || !atrack->isRecMonitored())))
//               //&& atrack->canRecord() && !atrack->recordFlag()))
//             continue;
//           
//           _latencyInfo._isLatencyInputTerminal = false;
//           _latencyInfo._isLatencyInputTerminalProcessed = true;
//           return false;
//         }
//       break;
// 
//       default:
//       break;
//     }
//   }

  _latencyInfo._isLatencyInputTerminal = true;
  _latencyInfo._isLatencyInputTerminalProcessed = true;
  return true;
}

bool TransportSource::isLatencyOutputTerminal()
{
  // Have we been here before during this scan?
  // Just return the cached value.
  if(_latencyInfo._isLatencyOutputTerminalProcessed)
    return _latencyInfo._isLatencyOutputTerminal;

//   const RouteList* rl = outRoutes();
//   for (ciRoute ir = rl->begin(); ir != rl->end(); ++ir) {
//     switch(ir->type)
//     {
//       case Route::TRACK_ROUTE:
//         if(!ir->track)
//           continue;
//         if(ir->track->isMidiTrack())
//         {
//           // TODO
//         }
//         else
//         {
//           Track* track = ir->track;
//           if(track->off()) // || 
//             //(atrack->canRecordMonitor() && (MusEGlobal::config.monitoringAffectsLatency || !atrack->isRecMonitored())))
//               //&& atrack->canRecord() && !atrack->recordFlag()))
//             continue;
//           
//           _latencyInfo._isLatencyOutputTerminal = false;
//           _latencyInfo._isLatencyOutputTerminalProcessed = true;
//           return false;
//         }
//       break;
// 
//       default:
//       break;
//     }
//   }
// 
//   _latencyInfo._isLatencyOutputTerminal = true;
//   _latencyInfo._isLatencyOutputTerminalProcessed = true;

  // TODO: Ask whether this transport source is 'connected' (being used).
  //       For now, assume it is.
  _latencyInfo._isLatencyOutputTerminal = false;
  _latencyInfo._isLatencyOutputTerminalProcessed = true;
  
  return true;
}

TrackLatencyInfo& TransportSource::getDominanceInfo(bool input)
{
  // Have we been here before during this scan?
  // Just return the cached value.
  if((input && _latencyInfo._canDominateInputProcessed) ||
     (!input && _latencyInfo._canDominateProcessed))
    return _latencyInfo;

  // Get the default domination for this track type.
  const bool can_dominate_lat = input ? canDominateInputLatency() : canDominateOutputLatency();
  const bool can_correct_lat = canCorrectOutputLatency() && _canCorrect;

  // Set the correction of all connected input branches,
  //  but ONLY if the track is not off.
  if(input)
  {
    _latencyInfo._canDominateInputLatency = can_dominate_lat;
  }
  else
  {
    _latencyInfo._canDominateOutputLatency = can_dominate_lat;
    // If any of the branches can dominate, then this node cannot correct.
// REMOVE Tim. lv2. Added.
//     // Special for the transport source: We use the correction values
//     //  but we do NOT report to the rest of the branch that we can correct.
//     // So we ignore _canCorrectOutputLatency.
//     // Yes, it can correct its own latency, but without further mechanisms we cannot know whether
//     //  the transport information affects the actual sound, or just an arpeggiator for example.
//     // Example:
//     // The LV2 Example Metronome depends on the transport info to directly produce the sound.
//     // That sound will have latency so correcting the transport values will correct it.
//     // In that case we would want to return 'true' here - ie. this node can indeed 'correct'
//     //  the actual sound.
//     // However, the Helm LV2 synth uses the transport info for the arpeggiator.
//     // Correcting the transport info only corrects the arpeggiator, NOT the actual sound.
//     // To correct the actual sound the midi input is corrected instead.
//     // Therefore we cannot return 'true' for certain here.
//     // By returning 'false' here, we will NOT inform the rest of the branch that we can correct,
//     //  but we WILL use the correction value anyway. If it corrects the sound, great, but we
//     //  will not report that to the branch. OK? ;-)
    _latencyInfo._canCorrectOutputLatency = can_correct_lat && !can_dominate_lat;
//     //_latencyInfo._canCorrectOutputLatency = false;
  }

  if(input)
    _latencyInfo._canDominateInputProcessed = true;
  else
    _latencyInfo._canDominateProcessed = true;

  return _latencyInfo;
}

TrackLatencyInfo& TransportSource::getDominanceLatencyInfo(bool input)
{
  // Have we been here before during this scan?
  // Just return the cached value.
  if((input && _latencyInfo._dominanceInputProcessed) ||
     (!input && _latencyInfo._dominanceProcessed))
    return _latencyInfo;

  float route_worst_latency = 0.0f;

  const bool passthru = canPassThruLatency();

  float worst_self_latency = 0.0f;

  // Set the correction of all connected input branches,
  //  but ONLY if the track is not off.
  if(input)
  {
    _latencyInfo._inputLatency = route_worst_latency;
  }
  else
  {
    if(passthru)
    {
      _latencyInfo._outputLatency = worst_self_latency + route_worst_latency;
      _latencyInfo._inputLatency = route_worst_latency;
    }
    else
    {
      _latencyInfo._outputLatency = worst_self_latency + _latencyInfo._sourceCorrectionValue;
    }
  }

  if(input)
    _latencyInfo._dominanceInputProcessed = true;
  else
    _latencyInfo._dominanceProcessed = true;

  return _latencyInfo;
}

TrackLatencyInfo& TransportSource::setCorrectionLatencyInfo(
  bool input, float finalWorstLatency, float callerBranchLatency, bool commonProjectLatency)
{
  float worst_self_latency = 0.0f;
      
  // The _trackLatency should already be calculated in the dominance scan.
  const float branch_lat = callerBranchLatency + worst_self_latency;

  // Set the correction of all connected input branches,
  //  but ONLY if the track is not off.
  if(input)
  {
  }
  else
  {
// REMOVE Tim. lv2. Added.
//     // Special for the transport source: We use the correction values
//     //  but we do NOT report to the rest of the branch that we can correct.
//     // So we ignore _canCorrectOutputLatency. See getDominanceInfo() for explanation.
    if(canCorrectOutputLatency() /*&& _latencyInfo._canCorrectOutputLatency*/)
    {
      float corr = 0.0f;
      //if(MusEGlobal::config.commonProjectLatency)
      if(commonProjectLatency)
        corr -= finalWorstLatency;

      corr -= branch_lat;
      // The _sourceCorrectionValue is initialized to zero.
      // Whichever calling branch needs the most correction gets it.
      if(corr < _latencyInfo._sourceCorrectionValue)
        _latencyInfo._sourceCorrectionValue = corr;
    }
  }

  //fprintf(stderr, "TransportSource::setCorrectionLatencyInfo() name:%s finalWorstLatency:%f branch_lat:%f corr:%f _sourceCorrectionValue:%f\n",
  //        name().toLatin1().constData(), finalWorstLatency, branch_lat, corr, _latencyInfo._sourceCorrectionValue);

  return _latencyInfo;
}

TrackLatencyInfo& TransportSource::getLatencyInfo(bool input)
{
  // Have we been here before during this scan?
  // Just return the cached value.
  if((input && _latencyInfo._inputProcessed) ||
    (!input && _latencyInfo._processed))
    return _latencyInfo;

  if(input)
    _latencyInfo._inputProcessed = true;
  else
    _latencyInfo._processed = true;

  return _latencyInfo;
}


} // namespace MusECore
