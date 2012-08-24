//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: node.cpp,v 1.36.2.25 2009/12/20 05:00:35 terminator356 Exp $
//
//  (C) Copyright 2000-2004 Werner Schweer (ws@seh.de)
//  (C) Copyright 2011 Tim E. Real (terminator356 on sourceforge)
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

#include <cmath>
#include <sndfile.h>
#include <stdlib.h>

#include <QString>

#include "node.h"
#include "globals.h"
#include "gconfig.h"
#include "song.h"
#include "xml.h"
#include "plugin.h"
#include "synth.h"
#include "audiodev.h"
#include "audio.h"
#include "wave.h"
#include "utils.h"      //debug
#include "ticksynth.h"  // metronome
#include "al/dsp.h"

// Turn on debugging messages
//#define NODE_DEBUG     
// Turn on constant flow of process debugging messages
//#define NODE_DEBUG_PROCESS     

//#define FIFO_DEBUG 
//#define METRONOME_DEBUG 

namespace MusECore {

//---------------------------------------------------------
//   isMute
//---------------------------------------------------------

bool MidiTrack::isMute() const
      {
      if (_solo || (_internalSolo && !_mute))
            return false;
      
      if (_soloRefCnt)
            return true;
      
      return _mute;
      }

bool AudioTrack::isMute() const
      {
      if (_solo || (_internalSolo && !_mute))
            return false;
      
      if (_soloRefCnt)
            return true;
      
      return _mute;
      }


//---------------------------------------------------------
//   setSolo
//---------------------------------------------------------

void MidiTrack::setSolo(bool val)
{
      if(_solo != val)
      {
        _solo = val;
        updateSoloStates(false);
      }
}

void AudioTrack::setSolo(bool val)
{
      if(_solo != val)
      {
        _solo = val;
        updateSoloStates(false);
      }
      
      if (isMute())
            resetMeter();
}


//---------------------------------------------------------
//   setInternalSolo
//---------------------------------------------------------

void Track::setInternalSolo(unsigned int val)
{
  _internalSolo = val;
}

//---------------------------------------------------------
//   clearSoloRefCounts
//   This is a static member function. Required for outside access.
//   Clears the internal static reference counts. 
//---------------------------------------------------------

void Track::clearSoloRefCounts()
{
  _soloRefCnt = 0;
}

//---------------------------------------------------------
//   updateSoloState
//---------------------------------------------------------

void Track::updateSoloState()
{
    if(_solo)
      _soloRefCnt++;
    else
    if(_soloRefCnt && !_tmpSoloChainNoDec)
      _soloRefCnt--;
}

//---------------------------------------------------------
//   updateInternalSoloStates
//---------------------------------------------------------

void Track::updateInternalSoloStates()
{
    if(_tmpSoloChainTrack->solo())
    {
      _internalSolo++;
      _soloRefCnt++;
    }  
    else
    if(!_tmpSoloChainNoDec) 
    {                           
      if(_internalSolo)
        _internalSolo--;
      if(_soloRefCnt)
        _soloRefCnt--;
    }  
}

//---------------------------------------------------------
//   updateInternalSoloStates
//---------------------------------------------------------

void MidiTrack::updateInternalSoloStates()
{
  if(_nodeTraversed)         // Anti circular mechanism.
  {
    fprintf(stderr, "MidiTrack::updateInternalSoloStates %s :\n  MusE Warning: Please check your routes: Circular path found!\n", name().toLatin1().constData()); 
    return;                         
  }  
  //if(this == _tmpSoloChainTrack)  
  //  return;
  
  _nodeTraversed = true; 
  
  Track::updateInternalSoloStates();
  
  _nodeTraversed = false;  // Reset.
}


//---------------------------------------------------------
//   updateInternalSoloStates
//---------------------------------------------------------

void AudioTrack::updateInternalSoloStates()
{
  if(_nodeTraversed)         // Anti circular mechanism.
  {
    fprintf(stderr, "AudioTrack::updateInternalSoloStates %s :\n  MusE Warning: Please check your routes: Circular path found!\n", name().toLatin1().constData()); 
    return;                         
  }  
  //if(this == _tmpSoloChainTrack)  
  //  return;
  
  _nodeTraversed = true; 

  Track::updateInternalSoloStates();
  
  if(_tmpSoloChainDoIns)
  {
    if(type() == AUDIO_SOFTSYNTH)
    {
      const MusECore::MidiTrackList* ml = MusEGlobal::song->midis();
      for(MusECore::ciMidiTrack im = ml->begin(); im != ml->end(); ++im)
      {
        MusECore::MidiTrack* mt = *im;
        if(mt->outPort() >= 0 && mt->outPort() == ((SynthI*)this)->midiPort())
          mt->updateInternalSoloStates();
      }
    }
    
    const RouteList* rl = inRoutes();
    for(ciRoute ir = rl->begin(); ir != rl->end(); ++ir)
    {
      if(ir->type == Route::TRACK_ROUTE)
        ir->track->updateInternalSoloStates();
      else  
      // Support Midi Port -> Audio Input solo chains. p4.0.37 Tim.
      if(ir->type == Route::MIDI_PORT_ROUTE)    
      {
        const MidiTrackList* ml = MusEGlobal::song->midis();
        for(ciMidiTrack im = ml->begin(); im != ml->end(); ++im)
        {
          MidiTrack* mt = *im;
          if(mt->outPort() == ir->midiPort && ((1 << mt->outChannel()) & ir->channel) )
            mt->updateInternalSoloStates();
        }
      }
    }
  }
  else
  {  
    const RouteList* rl = outRoutes();
    for(ciRoute ir = rl->begin(); ir != rl->end(); ++ir)
    {
      if(ir->type == Route::TRACK_ROUTE)
        ir->track->updateInternalSoloStates();
    }
  }  
  
  _nodeTraversed = false; // Reset.
}


//---------------------------------------------------------
//   updateSoloStates
//---------------------------------------------------------

void MidiTrack::updateSoloStates(bool noDec)
{
  if(noDec && !_solo)
    return;
  
  _nodeTraversed = true;  // Anti circular mechanism.
  
  _tmpSoloChainTrack = this;
  _tmpSoloChainDoIns = false;
  _tmpSoloChainNoDec = noDec;
  updateSoloState();
  
  if(outPort() >= 0)
  {
    MidiPort* mp = &MusEGlobal::midiPorts[outPort()];
    MidiDevice *md = mp->device();
    if(md && md->isSynti())
      ((SynthI*)md)->updateInternalSoloStates();
      
    // Support Midi Port -> Audio Input solo chains. p4.0.14 Tim.
    const int chbits = 1 << outChannel();
    const RouteList* rl = mp->outRoutes();
    for(ciRoute ir = rl->begin(); ir != rl->end(); ++ir)
    {
      if(ir->type == Route::TRACK_ROUTE && ir->track && ir->track->type() == Track::AUDIO_INPUT && (ir->channel & chbits) )
      {
        ir->track->updateInternalSoloStates();
      }  
    }
  }
  
  _nodeTraversed = false; // Reset.
}


//---------------------------------------------------------
//   updateSoloStates
//---------------------------------------------------------

void AudioTrack::updateSoloStates(bool noDec)
{
  if(noDec && !_solo)
    return;
  
  _nodeTraversed = true;  // Anti circular mechanism.

  _tmpSoloChainTrack = this;
  _tmpSoloChainNoDec = noDec;
  updateSoloState();
  
  _tmpSoloChainDoIns = true;
  if(type() == AUDIO_SOFTSYNTH)
  {
    const MidiTrackList* ml = MusEGlobal::song->midis();
    for(ciMidiTrack im = ml->begin(); im != ml->end(); ++im)
    {
      MidiTrack* mt = *im;
      if(mt->outPort() >= 0 && mt->outPort() == ((SynthI*)this)->midiPort())
        mt->updateInternalSoloStates();
    }
  }
  
  {
    const RouteList* rl = inRoutes();
    for(ciRoute ir = rl->begin(); ir != rl->end(); ++ir)
    {
      if(ir->type == Route::TRACK_ROUTE)
        ir->track->updateInternalSoloStates();
      else  
      // Support Midi Port -> Audio Input solo chains. p4.0.14 Tim.
      if(ir->type == Route::MIDI_PORT_ROUTE)    
      {
        const MidiTrackList* ml = MusEGlobal::song->midis();
        for(ciMidiTrack im = ml->begin(); im != ml->end(); ++im)
        {
          MidiTrack* mt = *im;
          if(mt->outPort() == ir->midiPort && ((1 << mt->outChannel()) & ir->channel) )
            mt->updateInternalSoloStates();
        }
      }
    }
  }  
  _tmpSoloChainDoIns = false;
  {
    const RouteList* rl = outRoutes();
    for(ciRoute ir = rl->begin(); ir != rl->end(); ++ir)
    {
      if(ir->type == Route::TRACK_ROUTE)
        ir->track->updateInternalSoloStates();
    }
  }  
  
  _nodeTraversed = false; // Reset.
}


//---------------------------------------------------------
//   setMute
//---------------------------------------------------------

void Track::setMute(bool val)
      {
      _mute = val;
      }

//---------------------------------------------------------
//   setOff
//---------------------------------------------------------

void Track::setOff(bool val)
      {
      _off = val;
      }


//---------------------------------------------------------
//   copyData
//---------------------------------------------------------

void AudioTrack::copyData(unsigned pos, int dstChannels, int srcStartChan, int srcChannels, unsigned nframes, float** dstBuffer)
{
  //Changed by T356. 12/12/09. 
  // Overhaul and streamline to eliminate multiple processing during one process loop. 
  // Was causing ticking sound with synths + multiple out routes because synths were being processed multiple times.
  // Make better use of AudioTrack::outBuffers as a post-effect pre-volume cache system for multiple calls here during processing.
  // Previously only WaveTrack used them. (Changed WaveTrack as well).
  
  #ifdef NODE_DEBUG_PROCESS
  printf("MusE: AudioTrack::copyData name:%s processed:%d\n", name().toLatin1().constData(), processed());
  #endif
  
  if(srcStartChan == -1)
    srcStartChan = 0;
    
  int trackChans = channels();
  int srcChans = (srcChannels == -1) ? trackChans : srcChannels;
  int srcTotalOutChans = totalOutChannels();
  if(channels() == 1)
    srcTotalOutChans = 1;
  
  // Special consideration for metronome: It is not part of the track list,
  //  and it has no in or out routes, yet multiple output tracks may call addData on it !
  // We can't tell how many output tracks call it, so we can only assume there might be more than one.
  // Not strictly necessary here because only addData is ever called, but just to be consistent...
  
  int i;
  
  float* buffer[srcTotalOutChans];
  float data[nframes * srcTotalOutChans];
  
  // precalculate stereo volume
  double vol[2];
  double _volume = controller()->value(AC_VOLUME, pos, 
                   !MusEGlobal::automation || automationType() == AUTO_OFF || !_volumeEnCtrl || !_volumeEn2Ctrl);
  double _pan    = controller()->value(AC_PAN, pos,
                   !MusEGlobal::automation || automationType() == AUTO_OFF || !_panEnCtrl || !_panEn2Ctrl);
  
  vol[0] = _volume * (1.0 - _pan);
  vol[1] = _volume * (1.0 + _pan);
  float meter[trackChans];

  // Have we been here already during this process cycle?
  if(processed())
  {
    // If there is only one (or no) output routes, it's an error - we've been called more than once per process cycle!
    // No, this is no longer an error, it's deliberate. Processing no longer done in 'chains', now done randomly.  p4.0.37
    #ifdef NODE_DEBUG_PROCESS
    printf("MusE: AudioTrack::copyData name:%s already processed _haveData:%d\n", name().toLatin1().constData(), _haveData);
    #endif
    
    // Is there already some data gathered from a previous call during this process cycle?
    if(_haveData)
    {
      // Point the input buffers at our local cached 'pre-volume' buffers. They need processing, so continue on after.
      for(i = 0; i < srcTotalOutChans; ++i)
        buffer[i] = outBuffers[i];
    }
    else
    {
      // No data was available from a previous call during this process cycle. Zero the supplied buffers and just return.
      for(i = 0; i < dstChannels; ++i) 
      {
        if(MusEGlobal::config.useDenormalBias) 
        {
          for(unsigned int q = 0; q < nframes; ++q)
            dstBuffer[i][q] = MusEGlobal::denormalBias;
        } 
        else
          memset(dstBuffer[i], 0, sizeof(float) * nframes);
      }
      return;  
    }
  }
  else 
  {
    // First time here during this process cycle. 
    
    _haveData = false;  // Reset.
    _processed = true;  // Set this now.

    if(off())  
    {  
      #ifdef NODE_DEBUG_PROCESS
      printf("MusE: AudioTrack::copyData name:%s dstChannels:%d Off, zeroing buffers\n", name().toLatin1().constData(), dstChannels);
      #endif
      
      // Track is off. Zero the supplied buffers.
      unsigned int q;
      for(i = 0; i < dstChannels; ++i) 
      {
        if(MusEGlobal::config.useDenormalBias) 
        {
          for(q = 0; q < nframes; ++q)
            dstBuffer[i][q] = MusEGlobal::denormalBias;
        } 
        else
          memset(dstBuffer[i], 0, sizeof(float) * nframes);
      }  
      
      _efxPipe->apply(0, nframes, 0);  // Just process controls only, not audio (do not 'run').    

      for(i = 0; i < trackChans; ++i) 
        _meter[i] = 0.0;
      
      return;
    }
    
    // Point the input buffers at a temporary stack buffer.
    for(i = 0; i < srcTotalOutChans; ++i)
        buffer[i] = data + i * nframes;
  
    // getData can use the supplied buffers, or change buffer to point to its own local buffers or Jack buffers etc. 
    // For ex. if this is an audio input, Jack will set the pointers for us in AudioInput::getData!
    // Don't do any processing at all if off. Whereas, mute needs to be ready for action at all times,
    //  so still call getData before it. Off is NOT meant to be toggled rapidly, but mute is !
    if(!getData(pos, srcTotalOutChans, nframes, buffer) || (isMute() && !_prefader)) 
    {
      #ifdef NODE_DEBUG_PROCESS
      printf("MusE: AudioTrack::copyData name:%s srcTotalOutChans:%d zeroing buffers\n", name().toLatin1().constData(), srcTotalOutChans);
      #endif
      
      // No data was available. Track is not off. Zero the working buffers and continue on.
      unsigned int q;
      for(i = 0; i < srcTotalOutChans; ++i)
      {  
        if(MusEGlobal::config.useDenormalBias) 
        {
          for(q = 0; q < nframes; ++q)
            buffer[i][q] = MusEGlobal::denormalBias;
        } 
        else
          memset(buffer[i], 0, sizeof(float) * nframes);
      }  
    }

    //---------------------------------------------------
    // apply plugin chain
    //---------------------------------------------------

    _efxPipe->apply(trackChans, nframes, buffer);   

    //---------------------------------------------------
    // aux sends
    //---------------------------------------------------

    if(hasAuxSend() && !isMute()) 
    {
      AuxList* al = MusEGlobal::song->auxs();
      unsigned naux = al->size();
      for(unsigned k = 0; k < naux; ++k) 
      {
        float m = _auxSend[k];
        if(m <= 0.0001)           // optimize
          continue;
        AudioAux* a = (AudioAux*)((*al)[k]);
        float** dst = a->sendBuffer();
        int auxChannels = a->channels();
        if((srcChans ==1 && auxChannels==1) || srcChans == 2) 
        {
          for(int ch = 0; ch < srcChans; ++ch) 
          {
            float* db = dst[ch % a->channels()]; // no matter whether there's one or two dst buffers
            float* sb = buffer[ch];
            for(unsigned f = 0; f < nframes; ++f) 
              *db++ += (*sb++ * m * vol[ch]);   // add to mix
          }
        }
        else if(srcChans==1 && auxChannels==2)  // copy mono to both channels
        {  
          for(int ch = 0; ch < auxChannels; ++ch) 
          {
            float* db = dst[ch % a->channels()];
            float* sb = buffer[0];
            for(unsigned f = 0; f < nframes; ++f) 
              *db++ += (*sb++ * m * vol[ch]);   // add to mix
          }
        }
      }
    }

    //---------------------------------------------------
    //    prefader metering
    //---------------------------------------------------

    if(_prefader) 
    {
      for(i = 0; i < trackChans; ++i)   
      {
        float* p = buffer[i];
        meter[i] = 0.0;
        for(unsigned k = 0; k < nframes; ++k) 
        {
          double f = fabs(*p++);
          if(f > meter[i])
            meter[i] = f;
        }
        _meter[i] = meter[i];
        if(_meter[i] > _peak[i])
          _peak[i] = _meter[i];
      }
    }
    
    if(isMute()) 
    {
      unsigned int q;
      for(i = 0; i < dstChannels; ++i)
      {
        if(MusEGlobal::config.useDenormalBias) 
        {
          for(q = 0; q < nframes; q++)
            dstBuffer[i][q] = MusEGlobal::denormalBias;
        } 
        else 
          memset(dstBuffer[i], 0, sizeof(float) * nframes);
      }        
      
      if(!_prefader) 
        for(i = 0; i < trackChans; ++i) // Must process ALL channels, even if unconnected. Only max 2 channels.
          _meter[i] = 0.0;

      return;
    }
    
    // If we're using local cached 'pre-volume' buffers, copy the input buffers (as they are right now: post-effect pre-volume) back to them. 
    for(i = 0; i < srcTotalOutChans; ++i)
      AL::dsp->cpy(outBuffers[i], buffer[i], nframes);
    
    // We have some data! Set to true.
    _haveData = true;
  }
  
  // Sanity check. Is source starting channel out of range? Just zero and return.
  if(srcStartChan >= srcTotalOutChans) 
  {
    unsigned int q;
    for(i = 0; i < dstChannels; ++i)
    {
      if(MusEGlobal::config.useDenormalBias) 
      {
        for(q = 0; q < nframes; q++)
          dstBuffer[i][q] = MusEGlobal::denormalBias;
      } 
      else 
        memset(dstBuffer[i], 0, sizeof(float) * nframes);
    }        
    return;
  }

  // Force a source range to fit actual available total out channels.
  if((srcStartChan + srcChans) > srcTotalOutChans) 
    srcChans = srcTotalOutChans - srcStartChan;
  
  //---------------------------------------------------
  // apply volume
  //    postfader metering
  //---------------------------------------------------

  #ifdef NODE_DEBUG_PROCESS
  printf("MusE: AudioTrack::copyData trackChans:%d srcTotalOutChans:%d srcStartChan:%d srcChans:%d dstChannels:%d\n", trackChans, srcTotalOutChans, srcStartChan, srcChans, dstChannels);
  #endif
      
  if(!_prefader) 
  {
    for(int c = 0; c < trackChans; ++c)     
    {
      meter[c] = 0.0;
      double v = (trackChans == 1 ? _volume : vol[c]);        
      float* sp = buffer[c];
      for(unsigned k = 0; k < nframes; ++k) 
      {
        float val = *sp++ * v;  // If the track is mono pan has no effect on meters.
        double f = fabs(val);
        if(f > meter[c])
          meter[c] = f;
      }
      _meter[c] = meter[c];
      if(_meter[c] > _peak[c])
        _peak[c] = _meter[c];
    }  
  }
    
  if(srcChans == dstChannels) 
  {
    for(int c = 0; c < dstChannels; ++c) 
    {
      double v;
      if(srcStartChan > 2 || _prefader) // Don't apply pan or volume to extra channels above 2. Or if prefader on.
        v = 1.0;
      else
      if(srcChans >= 2)         // If 2 channels apply pan normally.
        v = vol[c];
      else
      if(trackChans < 2)        // If 1 channel and track is 1 channel, don't apply pan.
        v = _volume;
      else
        v = vol[srcStartChan];  // Otherwise 1 channel but track is 2 channels. Apply the channel volume.
      
      float* sp = buffer[c + srcStartChan];
      float* dp = dstBuffer[c];
      for(unsigned k = 0; k < nframes; ++k)
        *dp++ = (*sp++ * v);
    }
  }
  else if(srcChans == 1 && dstChannels == 2) 
  {
    for(int c = 0; c < dstChannels; ++c) 
    {
      double v;
      if(srcStartChan > 2 || _prefader)      // Don't apply pan or volume to extra channels above 2. Or if prefader on.
        v = 1.0;
      else
      if(trackChans <= 1)       // If track is mono apply pan.
        v = vol[c];
      else
        v = vol[srcStartChan];  // Otherwise track is stereo, apply the same channel volume to both.
      
      float* sp = buffer[srcStartChan];
      float* dp = dstBuffer[c];
      for(unsigned k = 0; k < nframes; ++k)
        *dp++ = (*sp++ * v);
    }
  }
  else if(srcChans == 2 && dstChannels == 1) 
  {
    double v1 = ((srcStartChan > 2 || _prefader) ? 1.0 : vol[srcStartChan]);     // Don't apply pan or volume to extra channels above 2. Or if prefader on.
    double v2 = ((srcStartChan > 2 || _prefader) ? 1.0 : vol[srcStartChan + 1]); // 
    float* dp = dstBuffer[0];
    float* sp1 = buffer[srcStartChan];
    float* sp2 = buffer[srcStartChan + 1];
    for(unsigned k = 0; k < nframes; ++k)
      *dp++ = (*sp1++ * v1 + *sp2++ * v2);
  }
}

//---------------------------------------------------------
//   addData
//---------------------------------------------------------

void AudioTrack::addData(unsigned pos, int dstChannels, int srcStartChan, int srcChannels, unsigned nframes, float** dstBuffer)
{
  //Changed by T356. 12/12/09. 
  // Overhaul and streamline to eliminate multiple processing during one process loop.
  // Was causing ticking sound with synths + multiple out routes because synths were being processed multiple times.
  // Make better use of AudioTrack::outBuffers as a post-effect pre-volume cache system for multiple calls here during processing.
  // Previously only WaveTrack used them. (Changed WaveTrack as well).
  
  #ifdef NODE_DEBUG_PROCESS
  printf("MusE: AudioTrack::addData name:%s processed:%d\n", name().toLatin1().constData(), processed());
  #endif
  
  if(srcStartChan == -1)
    srcStartChan = 0;
    
  int trackChans = channels();
  int srcChans = (srcChannels == -1) ? trackChans : srcChannels;
  int srcTotalOutChans = totalOutChannels();
  if(channels() == 1)
    srcTotalOutChans = 1;
  
  // Special consideration for metronome: It is not part of the track list, DELETETHIS??
  //  and it has no in or out routes, yet multiple output tracks may call addData on it !
  // We can't tell how many output tracks call it, so we can only assume there might be more than one.
  
  int i;
  
  float* buffer[srcTotalOutChans];
  float data[nframes * srcTotalOutChans];
  
  // precalculate stereo volume
  double vol[2];
  double _volume = controller()->value(AC_VOLUME, pos, 
                   !MusEGlobal::automation || automationType() == AUTO_OFF || !_volumeEnCtrl || !_volumeEn2Ctrl);
  double _pan    = controller()->value(AC_PAN, pos,
                   !MusEGlobal::automation || automationType() == AUTO_OFF || !_panEnCtrl || !_panEn2Ctrl);

  vol[0] = _volume * (1.0 - _pan);
  vol[1] = _volume * (1.0 + _pan);
  float meter[trackChans];

  // Have we been here already during this process cycle?
  if(processed())
  {
    // If there is only one (or no) output routes, it's an error - we've been called more than once per process cycle!
    // No, this is no longer an error, it's deliberate. Processing no longer done in 'chains', now done randomly.  p4.0.37
    #ifdef NODE_DEBUG_PROCESS
    printf("MusE: AudioTrack::addData name:%s already processed _haveData:%d\n", name().toLatin1().constData(), _haveData);
    #endif
    
    // Is there already some data gathered from a previous call during this process cycle?
    if(_haveData)
    {
      // Point the input buffers at our local cached 'pre-volume' buffers. They need processing, so continue on after.
      for(i = 0; i < srcTotalOutChans; ++i)
        buffer[i] = outBuffers[i];
    }
    else
    {  
      // No data was available from a previous call during this process cycle. Nothing to add, just return.
      return;  
    }  
  }
  else
  {
    // First time here during this process cycle. 
    
    _haveData = false;  // Reset.
    _processed = true;  // Set this now.
    
    if(off())  
    {  
      #ifdef NODE_DEBUG_PROCESS
      printf("MusE: AudioTrack::addData name:%s dstChannels:%d Track is Off \n", name().toLatin1().constData(), dstChannels);
      #endif
      
      //  Nothing to zero or add...
      
      _efxPipe->apply(0, nframes, 0);  // Track is off. Just process controls only, not audio (do not 'run').    

      for(i = 0; i < trackChans; ++i) 
        _meter[i] = 0.0;
      return;
    }
      
    // Point the input buffers at a temporary stack buffer.
    for(i = 0; i < srcTotalOutChans; ++i)
        buffer[i] = data + i * nframes;
  
    // getData can use the supplied buffers, or change buffer to point to its own local buffers or Jack buffers etc. 
    // For ex. if this is an audio input, Jack will set the pointers for us.
    if(!getData(pos, srcTotalOutChans, nframes, buffer)) 
    {
      // No data was available. Track is not off. Zero the working buffers and continue on.
      unsigned int q;
      for(i = 0; i < srcTotalOutChans; ++i)
      {  
        if(MusEGlobal::config.useDenormalBias) 
        {
          for(q = 0; q < nframes; ++q)
            buffer[i][q] = MusEGlobal::denormalBias;
        } 
        else
          memset(buffer[i], 0, sizeof(float) * nframes);
      }  
    }

    //---------------------------------------------------
    // apply plugin chain
    //---------------------------------------------------
    
    _efxPipe->apply(trackChans, nframes, buffer);   
    
    //---------------------------------------------------
    // aux sends
    //---------------------------------------------------

    if(hasAuxSend() && !isMute()) 
    {
      AuxList* al = MusEGlobal::song->auxs();
      unsigned naux = al->size();
      for(unsigned k = 0; k < naux; ++k) 
      {
        float m = _auxSend[k];
        if(m <= 0.0001)           // optimize
          continue;
        AudioAux* a = (AudioAux*)((*al)[k]);
        float** dst = a->sendBuffer();
        int auxChannels = a->channels();
        if((srcChans ==1 && auxChannels==1) || srcChans==2) 
        {
          for(int ch = 0; ch < srcChans; ++ch) 
          {
            float* db = dst[ch % a->channels()];
            float* sb = buffer[ch];
            for(unsigned f = 0; f < nframes; ++f) 
              *db++ += (*sb++ * m * vol[ch]);   // add to mix
          }
        }
        else if(srcChans == 1 && auxChannels == 2) 
        {
          for(int ch = 0; ch < auxChannels; ++ch) 
          {
            float* db = dst[ch % a->channels()];
            float* sb = buffer[0];
            for(unsigned f = 0; f < nframes; ++f) 
              *db++ += (*sb++ * m * vol[ch]);   // add to mix
          }
        }
      }
    }

    //---------------------------------------------------
    //    prefader metering
    //---------------------------------------------------

    if(_prefader) 
    {
      for(i = 0; i < trackChans; ++i) 
      {
        float* p = buffer[i];
        meter[i] = 0.0;
        for(unsigned k = 0; k < nframes; ++k) 
        {
          double f = fabs(*p++);
          if(f > meter[i])
            meter[i] = f;
        }
        _meter[i] = meter[i];
        if(_meter[i] > _peak[i])
          _peak[i] = _meter[i];
      }
    }
    
    if(isMute())
    {
      if(!_prefader) 
        for(i = 0; i < trackChans; ++i)   
          _meter[i] = 0.0;
        
      return;
    }
    
    // If we're using local cached 'pre-volume' buffers, copy the input buffers (as they are right now: post-effect pre-volume) back to them. 
    for(i = 0; i < srcTotalOutChans; ++i)
      AL::dsp->cpy(outBuffers[i], buffer[i], nframes);
    
    // We have some data! Set to true.
    _haveData = true;
  }  
  
  // Sanity check. Is source starting channel out of range? Just zero and return.
  if(srcStartChan >= srcTotalOutChans) 
  {
    unsigned int q;
    for(i = 0; i < dstChannels; ++i)
    {
      if(MusEGlobal::config.useDenormalBias) 
      {
        for(q = 0; q < nframes; q++)
          dstBuffer[i][q] = MusEGlobal::denormalBias;
      } 
      else 
        memset(dstBuffer[i], 0, sizeof(float) * nframes);
    }        
    return;
  }
  
  // Force a source range to fit actual available total out channels.
  if((srcStartChan + srcChans) > srcTotalOutChans) 
    srcChans = srcTotalOutChans - srcStartChan;
  
  //---------------------------------------------------
  // apply volume
  //    postfader metering
  //---------------------------------------------------

  #ifdef NODE_DEBUG_PROCESS
  printf("MusE: AudioTrack::addData trackChans:%d srcTotalOutChans:%d srcChans:%d dstChannels:%d\n", trackChans, srcTotalOutChans, srcChans, dstChannels);
  #endif
      
  if(!_prefader) 
  {
    for(int c = 0; c < trackChans; ++c)     
    {
      meter[c] = 0.0;
      double v = (trackChans == 1 ? _volume : vol[c]);        
      float* sp = buffer[c];
      for(unsigned k = 0; k < nframes; ++k) 
      {
        float val = *sp++ * v;  // If the track is mono pan has no effect on meters.
        double f = fabs(val);
        if(f > meter[c])
          meter[c] = f;
      }
      _meter[c] = meter[c];
      if(_meter[c] > _peak[c])
        _peak[c] = _meter[c];
    }  
  }
  
  if(srcChans == dstChannels) 
  {
    for(int c = 0; c < dstChannels; ++c) 
    {
      double v;
      if(srcStartChan > 2 || _prefader)      // Don't apply pan or volume to extra channels above 2. Or if prefader on.
        v = 1.0;
      else
      if(srcChans >= 2)         // If 2 channels apply pan normally.
        v = vol[c];
      else
      if(trackChans < 2)        // If 1 channel and track is 1 channel, don't apply pan.
        v = _volume;
      else
        v = vol[srcStartChan];  // Otherwise 1 channel but track is 2 channels. Apply the channel volume.
      
      float* sp = buffer[c + srcStartChan];
      float* dp = dstBuffer[c];
      for(unsigned k = 0; k < nframes; ++k)
        *dp++ += (*sp++ * v);
    }
  }
  else if(srcChans == 1 && dstChannels == 2) 
  {
    for(int c = 0; c < dstChannels; ++c) 
    {
      double v;
      if(srcStartChan > 2 || _prefader)      // Don't apply pan or volume to extra channels above 2. Or if prefader on.
        v = 1.0;
      else
      if(trackChans <= 1)       // If track is mono apply pan.
        v = vol[c];
      else
        v = vol[srcStartChan];  // Otherwise track is stereo, apply the same channel volume to both.
      
      float* sp = buffer[srcStartChan];
      float* dp = dstBuffer[c];
      for(unsigned k = 0; k < nframes; ++k)
        *dp++ += (*sp++ * v);
    }
  }
  else if(srcChans == 2 && dstChannels == 1) 
  {
    double v1 = ((srcStartChan > 2 || _prefader) ? 1.0 : vol[srcStartChan]);     // Don't apply pan or volume to extra channels above 2. Or if prefader on.
    double v2 = ((srcStartChan > 2 || _prefader) ? 1.0 : vol[srcStartChan + 1]); // 
    float* sp1 = buffer[srcStartChan];
    float* sp2 = buffer[srcStartChan + 1];
    float* dp = dstBuffer[0];
    for(unsigned k = 0; k < nframes; ++k)
      *dp++ += (*sp1++ * v1 + *sp2++ * v2);
  }
}

//---------------------------------------------------------
//   readVolume
//---------------------------------------------------------

void AudioTrack::readVolume(Xml& xml)
      {
      for (;;) {
            Xml::Token token = xml.parse();
            switch (token) {
                  case Xml::Error:
                  case Xml::End:
                        return;
                  case Xml::TagStart:
                        xml.unknown("readVolume");
                        break;
                  case Xml::Text:
                        setVolume(xml.s1().toDouble());
                        break;
                  case Xml::Attribut:
                        if (xml.s1() == "ch")
                              //ch = xml.s2().toInt();
                              xml.s2();
                        break;
                  case Xml::TagEnd:
                        if (xml.s1() == "volume")
                              return;
                  default:
                        break;
                  }
            }
      }

// DELETETHIS 56
// Removed by T356
// "recfile" tag not saved anymore
/*

THIS CODE IS OBSOLETE! _recFile has been changed from SndFile* to SndFileR.
this code has NOT been adapted!

//---------------------------------------------------------
//   readRecfile
//---------------------------------------------------------

void AudioTrack::readRecfile(Xml& xml)
      {
      QString path;
      int channels = 2;
      int format   = SF_FORMAT_WAV | SF_FORMAT_PCM_16;

      for (;;) {
            Xml::Token token = xml.parse();
            if (token == Xml::Error || token == Xml::End)
                  break;
            const QString& tag = xml.s1();
            switch (token) {
                  case Xml::TagStart:
                        if (tag == "path")
                              path = xml.parse1();
                        else if (tag == "channels")
                              channels = xml.parseInt();
                        else if (tag == "format")
                              format = xml.parseInt();
                        else if (tag == "samplebits")
                              ;
                        else
                              xml.unknown("recfile");
                        break;
                  case Xml::TagEnd:
                        if (tag == "recfile") {
                              if (QFile::exists(path)) {
                                    setRecFile(getWave(path, true));
                                    }
                              else {
                                    setRecFile(new SndFile(path));
                                    recFile()->setFormat(format, channels, sampleRate);
                                    if (recFile()->openWrite()) {
                                          fprintf(stderr, "create wave file(%s) failed: %s\n",
                                             path.toLatin1().constData(), recFile()->strerror().toLatin1().constData());
                                          delete _recFile;
                                          _recFile = 0;
                                          }
                                    }
                              return;
                              }
                  default:
                        break;
                  }
            }
      }
*/


//---------------------------------------------------------
//   setChannels
//---------------------------------------------------------

void Track::setChannels(int n)
      {
      if(n > MAX_CHANNELS)
        _channels = MAX_CHANNELS;
      else  
        _channels = n;
      for (int i = 0; i < _channels; ++i) {
            _meter[i] = 0.0;
            _peak[i]  = 0.0;
            }
      }


void AudioInput::setChannels(int n)
      {
      if (n == _channels)
            return;
      AudioTrack::setChannels(n);
      }

void AudioOutput::setChannels(int n)
      {
      if (n == _channels)
            return;
      AudioTrack::setChannels(n);
      }

//---------------------------------------------------------
//   putFifo
//---------------------------------------------------------

void AudioTrack::putFifo(int channels, unsigned long n, float** bp)
      {
      if (fifo.put(channels, n, bp, MusEGlobal::audio->pos().frame())) {
            printf("   overrun ???\n");
            }
      }

//---------------------------------------------------------
//   getData
//    return false if no data available
//---------------------------------------------------------

bool AudioTrack::getData(unsigned pos, int channels, unsigned nframes, float** buffer)
      {
      // use supplied buffers

      RouteList* rl = inRoutes();
      
      #ifdef NODE_DEBUG_PROCESS
      printf("AudioTrack::getData name:%s inRoutes:%lu\n", name().toLatin1().constData(), rl->size());
      #endif
      
      ciRoute ir = rl->begin();
      if (ir == rl->end())
            return false;
      
      if(ir->track->isMidiTrack())
        return false;
        
      #ifdef NODE_DEBUG_PROCESS
      printf("    calling copyData on %s...\n", ir->track->name().toLatin1().constData());
      #endif
      
      ((AudioTrack*)ir->track)->copyData(pos, channels, 
                                         ir->channel,
                                         ir->channels,
                                         nframes, buffer);
      
      
      ++ir;
      for (; ir != rl->end(); ++ir) {
            #ifdef NODE_DEBUG_PROCESS
            printf("    calling addData on %s...\n", ir->track->name().toLatin1().constData());
            #endif
              
            if(ir->track->isMidiTrack())
              continue;
              
            ((AudioTrack*)ir->track)->addData(pos, channels, 
                                              //(ir->track->type() == Track::AUDIO_SOFTSYNTH && ir->channel != -1) ? ir->channel : 0,
                                              ir->channel,
                                              ir->channels,
                                              nframes, buffer);
            }
      return true;
      }

//---------------------------------------------------------
//   getData
//    return true if data
//---------------------------------------------------------

bool AudioInput::getData(unsigned, int channels, unsigned nframes, float** buffer)
      {
      if (!MusEGlobal::checkAudioDevice()) return false;
      for (int ch = 0; ch < channels; ++ch) 
      {
            void* jackPort = jackPorts[ch];
            
            // Do not get buffers of unconnected client ports. Causes repeating leftover data, can be loud, or DC !
            if (jackPort && MusEGlobal::audioDevice->connections(jackPort)) 
            {
                  //buffer[ch] = MusEGlobal::audioDevice->getBuffer(jackPort, nframes);
                  // If the client port buffer is also used by another channel (connected to the same jack port), 
                  //  don't directly set pointer, copy the data instead. 
                  // Otherwise the next channel will interfere - it will overwrite the buffer !
                  // Verified symptoms: Can't use a splitter. Mono noise source on a stereo track sounds in mono. Etc...
                  // TODO: Problem: What if other Audio Input tracks share the same jack ports as this Audio Input track?
                  // Users will expect that Audio Inputs just work even if the input routes originate from the same jack port.
                  // Solution: Rather than having to iterate all other channels, and all other Audio Input tracks and check 
                  //  their channel port buffers (if that's even possible) in order to determine if the buffer is shared, 
                  //  let's just copy always, for now shall we ?
                  float* jackbuf = MusEGlobal::audioDevice->getBuffer(jackPort, nframes);
                  AL::dsp->cpy(buffer[ch], jackbuf, nframes);
                  
                  if (MusEGlobal::config.useDenormalBias) 
                  {
                      for (unsigned int i=0; i < nframes; i++)
                              buffer[ch][i] += MusEGlobal::denormalBias;
                  }
            } 
            else 
            {
                  if (MusEGlobal::config.useDenormalBias) 
                  {
                      for (unsigned int i=0; i < nframes; i++)
                              buffer[ch][i] = MusEGlobal::denormalBias;
                  } 
                  else 
                  {
                              memset(buffer[ch], 0, nframes * sizeof(float));
                  }
            }
      }
      return true;
}

//---------------------------------------------------------
//   setName
//---------------------------------------------------------

void AudioInput::setName(const QString& s)
      {
      _name = s;
      if (!MusEGlobal::checkAudioDevice()) return;
      for (int i = 0; i < channels(); ++i) {
            char buffer[128];
            snprintf(buffer, 128, "%s-%d", _name.toLatin1().constData(), i);
            if (jackPorts[i])
                  MusEGlobal::audioDevice->setPortName(jackPorts[i], buffer);
            else {
                  jackPorts[i] = MusEGlobal::audioDevice->registerInPort(buffer, false);
                  }
            }
      }


//---------------------------------------------------------
//   resetMeter
//---------------------------------------------------------

void Track::resetMeter()
      {
      for (int i = 0; i < _channels; ++i)
            _meter[i] = 0.0;
      }

//---------------------------------------------------------
//   resetPeaks
//---------------------------------------------------------

void Track::resetPeaks()
      {
      for (int i = 0; i < _channels; ++i)
            _peak[i] = 0.0;
            _lastActivity = 0;
      }

//---------------------------------------------------------
//   resetAllMeter
//---------------------------------------------------------

void Track::resetAllMeter()
      {
      TrackList* tl = MusEGlobal::song->tracks();
      for (iTrack i = tl->begin(); i != tl->end(); ++i)
            (*i)->resetMeter();
      }


//---------------------------------------------------------
//   setRecordFlag2
//    real time part (executed in audio thread)
//---------------------------------------------------------

void AudioTrack::setRecordFlag2(bool f)
      {
      if (f == _recordFlag)
            return;
      _recordFlag = f;
      if (!_recordFlag)
            resetMeter();
      }

//---------------------------------------------------------
//   setMute
//---------------------------------------------------------

void AudioTrack::setMute(bool f)
      {
      _mute = f;
      if (_mute)
            resetAllMeter();
      }

//---------------------------------------------------------
//   setOff
//---------------------------------------------------------

void AudioTrack::setOff(bool val)
      {
      _off = val;
      if (val)
            resetAllMeter();
      }

//---------------------------------------------------------
//   setPrefader
//---------------------------------------------------------

void AudioTrack::setPrefader(bool val)
      {
      _prefader = val;
      if (!_prefader && isMute())
            resetAllMeter();
      }


//---------------------------------------------------------
//   canEnableRecord
//---------------------------------------------------------

bool WaveTrack::canEnableRecord() const
      {
      return  (!noInRoute() || (this == MusEGlobal::song->bounceTrack));
      }


//---------------------------------------------------------
//   record
//---------------------------------------------------------

void AudioTrack::record()
      {
      unsigned pos = 0;
      float* buffer[_channels];

      while(fifo.getCount()) {

            if (fifo.get(_channels, MusEGlobal::segmentSize, buffer, &pos)) {
                  printf("AudioTrack::record(): empty fifo\n");
                  return;
                  }
            if (_recFile) {
                  // Line removed by Tim. Oct 28, 2009
                  //_recFile->seek(pos, 0);
                  //
                  // Fix for recorded waves being shifted ahead by an amount
                  //  equal to start record position.
                  //
                  // From libsndfile ChangeLog:
                  // 2008-05-11  Erik de Castro Lopo  <erikd AT mega-nerd DOT com>
                  //    * src/sndfile.c
                  //    Allow seeking past end of file during write.
                  //    
                  // I don't know why this line would even be called, because the FIFOs'
                  //  'pos' members operate in absolute frames, which at this point 
                  //  would be shifted ahead by the start of the wave part.
                  // So if you begin recording a new wave part at bar 4, for example, then
                  //  this line is seeking the record file to frame 288000 even before any audio is written!
                  // Therefore, just let the write do its thing and progress naturally,
                  //  it should work OK since everything was OK before the libsndfile change...
                  //
                  // Tested: With the line, audio record looping sort of works, albiet with the start offset added to
                  //  the wave file. And it overwrites existing audio. (Check transport window 'overwrite' function. Tie in somehow...)
                  // With the line, looping does NOT work with libsndfile from around early 2007 (my distro's version until now).
                  // Therefore it seems sometime between libsndfile ~2007 and today, libsndfile must have allowed 
                  //  "seek (behind) on write", as well as the "seek past end" change of 2008...
                  //
                  // Ok, so removing that line breaks *possible* record audio 'looping' functionality, revealed with
                  //  later libsndfile. 
                  // Try this... And while we're at it, honour the punchin/punchout, and loop functions !
                  //
                  // If punchin is on, or we have looped at least once, use left marker as offset.
                  // Note that audio::startRecordPos is reset to (roughly) the left marker pos upon loop !
                  // (Not any more! I changed Audio::Process)
                  // Since it is possible to start loop recording before the left marker (with punchin off), we must 
                  //  use startRecordPos or loopFrame or left marker, depending on punchin and whether we have looped yet.
                  unsigned fr;
                  if(MusEGlobal::song->punchin() && (MusEGlobal::audio->loopCount() == 0))
                    fr = MusEGlobal::song->lPos().frame();
                  else  
                  if((MusEGlobal::audio->loopCount() > 0) && (MusEGlobal::audio->getStartRecordPos().frame() > MusEGlobal::audio->loopFrame()))
                    fr = MusEGlobal::audio->loopFrame();
                  else
                    fr = MusEGlobal::audio->getStartRecordPos().frame();
                  // Now seek and write. If we are looping and punchout is on, don't let punchout point interfere with looping point.
                  if( (pos >= fr) && (!MusEGlobal::song->punchout() || (!MusEGlobal::song->loop() && pos < MusEGlobal::song->rPos().frame())) )
                  {
                    pos -= fr;
                    
                    _recFile->seek(pos, 0);
                    _recFile->write(_channels, buffer, MusEGlobal::segmentSize);
                  }
                    
                  }
            else {
                  printf("AudioNode::record(): no recFile\n");
                  }
            }
      }

//---------------------------------------------------------
//   processInit
//---------------------------------------------------------

void AudioOutput::processInit(unsigned nframes)
      {
      _nframes = nframes;
      if (!MusEGlobal::checkAudioDevice()) return;
      for (int i = 0; i < channels(); ++i) {
            if (jackPorts[i]) {
                  buffer[i] = MusEGlobal::audioDevice->getBuffer(jackPorts[i], nframes);
                  if (MusEGlobal::config.useDenormalBias) {
                      for (unsigned int j=0; j < nframes; j++)
                              buffer[i][j] += MusEGlobal::denormalBias;
                      }
                  }
            else
                  printf("PANIC: processInit: no buffer from audio driver\n");
            }
      }

//---------------------------------------------------------
//   process
//    synthesize "n" frames at buffer offset "offset"
//    current frame position is "pos"
//---------------------------------------------------------

void AudioOutput::process(unsigned pos, unsigned offset, unsigned n)
{
      #ifdef NODE_DEBUG_PROCESS
      printf("MusE: AudioOutput::process name:%s processed:%d\n", name().toLatin1().constData(), processed());
      #endif
      
      for (int i = 0; i < _channels; ++i) {
            buffer1[i] = buffer[i] + offset;
      }
      copyData(pos, _channels, -1, -1, n, buffer1);
}

//---------------------------------------------------------
//   silence
//---------------------------------------------------------

void AudioOutput::silence(unsigned n)
      {
      processInit(n);
      for (int i = 0; i < channels(); ++i)
          if (MusEGlobal::config.useDenormalBias) {
              for (unsigned int j=0; j < n; j++)
                  buffer[i][j] = MusEGlobal::denormalBias;
            } else {
                  memset(buffer[i], 0, n * sizeof(float));
                  }
      }

//---------------------------------------------------------
//   processWrite
//---------------------------------------------------------

void AudioOutput::processWrite()
      {
      if (MusEGlobal::audio->isRecording() && MusEGlobal::song->bounceOutput == this) {
            if (MusEGlobal::audio->freewheel()) {
                  MusECore::WaveTrack* track = MusEGlobal::song->bounceTrack;
                  if (track && track->recordFlag() && track->recFile())
                        track->recFile()->write(_channels, buffer, _nframes);
                  if (recordFlag() && recFile())
                        _recFile->write(_channels, buffer, _nframes);
                  }
            else {
                  MusECore::WaveTrack* track = MusEGlobal::song->bounceTrack;
                  if (track && track->recordFlag() && track->recFile())
                        track->putFifo(_channels, _nframes, buffer);
                  if (recordFlag() && recFile())
                        putFifo(_channels, _nframes, buffer);
                  }
            }
      if (sendMetronome() && MusEGlobal::audioClickFlag && MusEGlobal::song->click()) {
            
            #ifdef METRONOME_DEBUG
            printf("MusE: AudioOutput::processWrite Calling metronome->addData frame:%u channels:%d frames:%lu\n", MusEGlobal::audio->pos().frame(), _channels, _nframes);
            #endif
            metronome->addData(MusEGlobal::audio->pos().frame(), _channels, -1, -1, _nframes, buffer);
            }
      }
//---------------------------------------------------------
//   setName
//---------------------------------------------------------

void AudioOutput::setName(const QString& s)
      {
      _name = s;
      if (!MusEGlobal::checkAudioDevice()) return;
      for (int i = 0; i < channels(); ++i) {
            char buffer[128];
            snprintf(buffer, 128, "%s-%d", _name.toLatin1().constData(), i);
            if (jackPorts[i])
                  MusEGlobal::audioDevice->setPortName(jackPorts[i], buffer);
            else
                  jackPorts[i] = MusEGlobal::audioDevice->registerOutPort(buffer, false);
            }
      }


//---------------------------------------------------------
//   Fifo
//---------------------------------------------------------

Fifo::Fifo()
      {
      muse_atomic_init(&count);
      nbuffer = MusEGlobal::fifoLength;
      buffer  = new FifoBuffer*[nbuffer];
      for (int i = 0; i < nbuffer; ++i)
            buffer[i]  = new FifoBuffer;
      clear();
      }

Fifo::~Fifo()
      {
      for (int i = 0; i < nbuffer; ++i)
      {
        if(buffer[i]->buffer)
          free(buffer[i]->buffer);
          
        delete buffer[i];
      }
            
      delete[] buffer;
      muse_atomic_destroy(&count);
      }

//---------------------------------------------------------
//   put
//    return true if fifo full
//---------------------------------------------------------

bool Fifo::put(int segs, unsigned long samples, float** src, unsigned pos)
      {
      #ifdef FIFO_DEBUG
      printf("FIFO::put segs:%d samples:%lu pos:%u\n", segs, samples, pos);
      #endif
      
      if (muse_atomic_read(&count) == nbuffer) {
            printf("FIFO %p overrun... %d\n", this, count.counter);
            return true;
            }
      FifoBuffer* b = buffer[widx];
      int n         = segs * samples;
      if (b->maxSize < n) {
            if (b->buffer)
            {
              free(b->buffer);
              b->buffer = 0;
            }     
            posix_memalign((void**)&(b->buffer), 16, sizeof(float) * n);
            if(!b->buffer)
            {
              printf("Fifo::put could not allocate buffer segs:%d samples:%lu pos:%u\n", segs, samples, pos);
              return true;
            }
            
            b->maxSize = n;
            }
      if(!b->buffer)
      {
        printf("Fifo::put no buffer! segs:%d samples:%lu pos:%u\n", segs, samples, pos);
        return true;
      }
      
      b->size = samples;
      b->segs = segs;
      b->pos  = pos;
      for (int i = 0; i < segs; ++i)
            AL::dsp->cpy(b->buffer + i * samples, src[i], samples);
      add();
      return false;
      }

//---------------------------------------------------------
//   get
//    return true if fifo empty
//---------------------------------------------------------

bool Fifo::get(int segs, unsigned long samples, float** dst, unsigned* pos)
      {
      #ifdef FIFO_DEBUG
      printf("FIFO::get segs:%d samples:%lu\n", segs, samples);
      #endif
      
      if (muse_atomic_read(&count) == 0) {
            printf("FIFO %p underrun... %d\n", this,count.counter); //by willyfoobar: added count to output //see Fifo::put()
            return true;
            }
      FifoBuffer* b = buffer[ridx];
      if(!b->buffer)
      {
        printf("Fifo::get no buffer! segs:%d samples:%lu b->pos:%u\n", segs, samples, b->pos);
        return true;
      }
      
      if (pos)
            *pos = b->pos;
      
      for (int i = 0; i < segs; ++i)
            dst[i] = b->buffer + samples * (i % b->segs);
      remove();
      return false;
      }

int Fifo::getCount()
      {
      return muse_atomic_read(&count);
      }
//---------------------------------------------------------
//   remove
//---------------------------------------------------------

void Fifo::remove()
      {
      ridx = (ridx + 1) % nbuffer;
      muse_atomic_dec(&count);
      }

//---------------------------------------------------------
//   getWriteBuffer
//---------------------------------------------------------

bool Fifo::getWriteBuffer(int segs, unsigned long samples, float** buf, unsigned pos)
      {
      #ifdef FIFO_DEBUG
      printf("Fifo::getWriteBuffer segs:%d samples:%lu pos:%u\n", segs, samples, pos);
      #endif
      
      if (muse_atomic_read(&count) == nbuffer)
            return true;
      FifoBuffer* b = buffer[widx];
      int n = segs * samples;
      if (b->maxSize < n) {
            if (b->buffer)
            {
              free(b->buffer);
              b->buffer = 0;
            }
            
            posix_memalign((void**)&(b->buffer), 16, sizeof(float) * n);
            if(!b->buffer)
            {
              printf("Fifo::getWriteBuffer could not allocate buffer segs:%d samples:%lu pos:%u\n", segs, samples, pos);
              return true;
            }
            
            b->maxSize = n;
            }
      if(!b->buffer)
      {
        printf("Fifo::getWriteBuffer no buffer! segs:%d samples:%lu pos:%u\n", segs, samples, pos);
        return true;
      }
      
      for (int i = 0; i < segs; ++i)
            buf[i] = b->buffer + i * samples;
            
      b->size = samples;
      b->segs = segs;
      b->pos  = pos;
      return false;
      }

//---------------------------------------------------------
//   add
//---------------------------------------------------------

void Fifo::add()
      {
      widx = (widx + 1) % nbuffer;
      muse_atomic_inc(&count);
      }


//---------------------------------------------------------
//   setChannels
//---------------------------------------------------------

void AudioTrack::setChannels(int n)
      {
      Track::setChannels(n);
      if (_efxPipe)
            _efxPipe->setChannels(n);
      }

//---------------------------------------------------------
//   setTotalOutChannels
//---------------------------------------------------------

void AudioTrack::setTotalOutChannels(int num)
{
      int chans = _totalOutChannels;
      if(num != chans)  
      {
        // Number of allocated buffers is always MAX_CHANNELS or more, even if _totalOutChannels is less. 
        if(chans < MAX_CHANNELS)
          chans = MAX_CHANNELS;
        if(outBuffers)
        {  
          for(int i = 0; i < chans; ++i) 
          {
            if(outBuffers[i])
              free(outBuffers[i]);
          }
          delete[] outBuffers;
        }  
        
        _totalOutChannels = num;
        chans = num;
        // Number of allocated buffers is always MAX_CHANNELS or more, even if _totalOutChannels is less. 
        if(chans < MAX_CHANNELS)
          chans = MAX_CHANNELS;
          
        outBuffers = new float*[chans];
        for (int i = 0; i < chans; ++i)
              posix_memalign((void**)&outBuffers[i], 16, sizeof(float) * MusEGlobal::segmentSize);
      }  
      chans = num;
      // Limit the actual track (meters, copying etc, all 'normal' operation) to two-channel stereo.
      if(chans > MAX_CHANNELS)
        chans = MAX_CHANNELS;
      setChannels(chans);
}

//---------------------------------------------------------
//   setTotalInChannels
//---------------------------------------------------------

void AudioTrack::setTotalInChannels(int num)
{
      if(num == _totalInChannels)
        return;
        
      _totalInChannels = num;
}

} // namespace MusECore
