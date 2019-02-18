//=========================================================
//  MusE
//  Linux Music Editor
//  (C) Copyright 1999/2000 Werner Schweer (ws@seh.de)
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

#include "audio.h"
#include "audiodev.h"
#include "globals.h"
#include "large_int.h"
#ifdef _WIN32
#include <sys/time.h>
#endif

namespace MusECore {
  
AudioDevice::AudioDevice()
{
  _dummyState = Audio::STOP;
  _dummyPos = 0;
  // Default 10 seconds, like Jack2. Jack1 is 2 seconds.
  _syncTimeout = 10.0;
  _syncTimeoutCounter = 0.0;
  _dummyStatePending = 0;
  _dummyPosPending = 0;
  
  _isTimebaseMaster = false;
}


//--------------------------------------
// Timing functions:
//--------------------------------------

uint64_t AudioDevice::systemTimeUS() const {
  struct timeval t;
  gettimeofday(&t, 0);
  //fprintf(stderr, "%lu %lu\n", t.tv_sec, t.tv_usec);  // Note I observed values coming out of order! Causing some problems.
  return ((uint64_t)t.tv_sec * 1000000UL) + (uint64_t)t.tv_usec;
}

//--------------------------------------
// Transport functions:
//--------------------------------------

bool AudioDevice::processTransport(unsigned int frames)
{
  const int state_pending = _dummyStatePending;  // Snapshots.
  const int pos_pending   = _dummyPosPending;    //
  _dummyStatePending = -1;                       // Reset.
  _dummyPosPending = -1;                         //
  
  if(!MusEGlobal::audio->isRunning())
  {
    if(MusEGlobal::debugMsg)
      puts("Dummy sync: Called when audio is not running!\n");
    return false;
  }
      
  // STOP -> STOP, STOP -> START_PLAY, PLAY -> START_PLAY all count as 'syncing'.
  if(((_dummyState == Audio::STOP || _dummyState == Audio::PLAY) && state_pending == Audio::START_PLAY)
      || (_dummyState == Audio::STOP && state_pending == Audio::STOP) )
  {
    _syncTimeoutCounter = (float)frames / (float)MusEGlobal::sampleRate;  // (Re)start the timeout counter...
    if(pos_pending != -1)
      _dummyPos = pos_pending; // Set the new dummy position.
    if((_dummyState == Audio::STOP || _dummyState == Audio::PLAY) && state_pending == Audio::START_PLAY)
      _dummyState = Audio::START_PLAY;
  }
  else // All other states such as START_PLAY -> STOP, PLAY -> STOP.
  if(state_pending != -1 && state_pending != _dummyState)
  {
    _syncTimeoutCounter = 0.0;  // Reset.
    _dummyState = state_pending;
  }
  
  // Is the sync timeout counter running?
  if(_syncTimeoutCounter > 0.0)
  {
    //printf("Jack processAudio dummy sync: state:%d pending:%d\n", jackAudio->dummyState, state_pending);  
    // Is MusE audio ready to roll?
    if(MusEGlobal::audio->sync(_dummyState, _dummyPos))
    {
      _syncTimeoutCounter = 0.0;  // Reset.
      // We're ready. Switch to PLAY state.
      if(_dummyState == Audio::START_PLAY)
        _dummyState = Audio::PLAY;
    }
    else
    {  
      _syncTimeoutCounter += (float)frames / (float)MusEGlobal::sampleRate;
      // Has the counter surpassed the timeout limit?
      if(_syncTimeoutCounter > _syncTimeout)
      {
        if (MusEGlobal::debugMsg)
          puts("Dummy sync timeout! Starting anyway...\n");
        _syncTimeoutCounter = 0.0;  // Reset.
        // We're not ready, but no time left - gotta roll anyway. Switch to PLAY state, similar to how Jack is supposed to work.
        if(_dummyState == Audio::START_PLAY)
        {
          _dummyState = Audio::PLAY;
          // Docs say sync will be called with Rolling state when timeout expires.
          MusEGlobal::audio->sync(_dummyState, _dummyPos);
        }
      }
    }
  }
  
  // Now call the audio process.
  // Don't process while we're syncing. ToDO: May need to deliver silence in process!
  //if(jackAudio->getState() != Audio::START_PLAY)
    MusEGlobal::audio->process(frames);
  
  // Is the transport playing? Advance the transport position.
  if(_dummyState == Audio::PLAY)
    _dummyPos += frames;
        
  return true;
}

void AudioDevice::startTransport()
{
  _dummyStatePending = Audio::START_PLAY;
}

void AudioDevice::stopTransport()
{ 
  _dummyStatePending = Audio::STOP;
}

void AudioDevice::seekTransport(unsigned frame)
{
  _dummyPosPending   = frame;
  // STOP -> STOP means seek in stop mode. PLAY -> START_PLAY means seek in play mode.
  _dummyStatePending = (_dummyState == Audio::STOP ? Audio::STOP : Audio::START_PLAY);
}

void AudioDevice::seekTransport(const Pos &p)
{
  _dummyPosPending   = p.frame();
  // STOP -> STOP means seek in stop mode. PLAY -> START_PLAY means seek in play mode.
  _dummyStatePending = (_dummyState == Audio::STOP ? Audio::STOP : Audio::START_PLAY);
}


} // namespace MusECore
