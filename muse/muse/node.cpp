//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: node.cpp,v 1.36.2.25 2009/12/20 05:00:35 terminator356 Exp $
//
//  (C) Copyright 2000-2004 Werner Schweer (ws@seh.de)
//=========================================================

#include <cmath>
#include <assert.h>
#include <sndfile.h>
#include <stdlib.h>

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

// Uncomment this (and make sure to set Jack buffer size high like 2048) 
//  to see process flow messages.
//#define NODE_DEBUG 
//#define FIFO_DEBUG 

// Added by Tim. p3.3.18
//#define METRONOME_DEBUG 

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
      if(this == _tmpSoloChainTrack)
        return;
      
      Track::updateInternalSoloStates();
}

//---------------------------------------------------------
//   updateInternalSoloStates
//---------------------------------------------------------

void AudioTrack::updateInternalSoloStates()
{
      if(this == _tmpSoloChainTrack)
        return;
      
      Track::updateInternalSoloStates();
      
      if(_tmpSoloChainDoIns)
      {
        if(type() == AUDIO_SOFTSYNTH)
        {
          const MidiTrackList* ml = song->midis();
          for(ciMidiTrack im = ml->begin(); im != ml->end(); ++im)
          {
            MidiTrack* mt = *im;
            if(mt->outPort() >= 0 && mt->outPort() == ((SynthI*)this)->midiPort())
              mt->updateInternalSoloStates();
          }
        }
        
        const RouteList* rl = inRoutes();
        for(ciRoute ir = rl->begin(); ir != rl->end(); ++ir)
        {
          if(ir->type == Route::TRACK_ROUTE)
            ir->track->updateInternalSoloStates();
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
}

//---------------------------------------------------------
//   updateSoloStates
//---------------------------------------------------------

void MidiTrack::updateSoloStates(bool noDec)
{
  if(noDec && !_solo)
    return;
  
  _tmpSoloChainTrack = this;
  _tmpSoloChainDoIns = false;
  _tmpSoloChainNoDec = noDec;
  updateSoloState();
  
  if(outPort() >= 0)
  {
    MidiDevice *md = midiPorts[outPort()].device();
    if(md && md->isSynti())
      ((SynthI*)md)->updateInternalSoloStates();
  }
}

//---------------------------------------------------------
//   updateSoloStates
//---------------------------------------------------------

void AudioTrack::updateSoloStates(bool noDec)
{
  if(noDec && !_solo)
    return;
  
  _tmpSoloChainTrack = this;
  _tmpSoloChainNoDec = noDec;
  updateSoloState();
  
  _tmpSoloChainDoIns = true;
  if(type() == AUDIO_SOFTSYNTH)
  {
    const MidiTrackList* ml = song->midis();
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

//void AudioTrack::copyData(unsigned pos, int dstChannels, unsigned nframes, float** dstBuffer)
void AudioTrack::copyData(unsigned pos, int dstChannels, int srcStartChan, int srcChannels, unsigned nframes, float** dstBuffer)
{
  //Changed by T356. 12/12/09. 
  // Overhaul and streamline to eliminate multiple processing during one process loop. 
  // Was causing ticking sound with synths + multiple out routes because synths were being processed multiple times.
  // Make better use of AudioTrack::outBuffers as a post-effect pre-volume cache system for multiple calls here during processing.
  // Previously only WaveTrack used them. (Changed WaveTrack as well).
  
  if(srcStartChan == -1)
    srcStartChan = 0;
    
  int srcChans = (srcChannels == -1) ? channels() : srcChannels;
  int srcTotalOutChans = totalOutChannels();
  if(channels() == 1)
    srcTotalOutChans = 1;
  
  #ifdef NODE_DEBUG
  printf("MusE: AudioTrack::copyData name:%s processed:%d\n", name().latin1(), processed());
  #endif
  
  // Special consideration for metronome: It is not part of the track list,
  //  and it has no in or out routes, yet multiple output tracks may call addData on it !
  // We can't tell how many output tracks call it, so we can only assume there might be more than one.
  // Not strictly necessary here because only addData is ever called, but just to be consistent...
  //bool usedirectbuf = (outRoutes()->size() <= 1) || (type() == AUDIO_OUTPUT);
  bool usedirectbuf = ((outRoutes()->size() <= 1) || (type() == AUDIO_OUTPUT)) && (this != metronome);
  
  int i;
  
  // p3.3.38
  //float* buffer[srcChannels];
  float* buffer[srcTotalOutChans];
  
  
  //float data[nframes * srcChannels];
  //for(i = 0; i < srcChannels; ++i)
  //      buffer[i] = data + i * nframes;
        
  // precalculate stereo volume
  double vol[2];
  double _volume = volume();
  double _pan = pan();
  vol[0] = _volume * (1.0 - _pan);
  vol[1] = _volume * (1.0 + _pan);
  float meter[srcChans];

  // Have we been here already during this process cycle?
  if(processed())
  {
    // If there is only one (or no) output routes, it's an error - we've been called more than once per process cycle!
    #ifdef NODE_DEBUG
    if(usedirectbuf)
      printf("MusE: AudioTrack::copyData Error! One or no out routes, but already processed! Copying local buffers anyway...\n");
    #endif
    
    // Is there already some data gathered from a previous call during this process cycle?
    if(_haveData)
    {
      // Point the input buffers at our local cached 'pre-volume' buffers. They need processing, so continue on after.
      //for(i = 0; i < srcChannels; ++i)
      //  buffer[i] = outBuffers[i];
      // p3.3.38
      for(i = 0; i < srcTotalOutChans; ++i)
        buffer[i] = outBuffers[i];
    }
    else
    {
      // No data was available from a previous call during this process cycle. Zero the supplied buffers and just return.
      for(i = 0; i < dstChannels; ++i) 
      {
        if(config.useDenormalBias) 
        {
          for(unsigned int q = 0; q < nframes; ++q)
            dstBuffer[i][q] = denormalBias;
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
    
    // Point the input buffers at a temporary stack buffer.
    //float data[nframes * srcChannels];
    //for(i = 0; i < srcChannels; ++i)
    //    buffer[i] = data + i * nframes;
    // p3.3.38
    float data[nframes * srcTotalOutChans];
    for(i = 0; i < srcTotalOutChans; ++i)
        buffer[i] = data + i * nframes;
  
    // getData can use the supplied buffers, or change buffer to point to its own local buffers or Jack buffers etc. 
    // For ex. if this is an audio input, Jack will set the pointers for us in AudioInput::getData!
    // p3.3.29 1/27/10 Don't do any processing at all if off. Whereas, mute needs to be ready for action at all times,
    //  so still call getData before it. Off is NOT meant to be toggled rapidly, but mute is !
    //if(!getData(pos, srcChannels, nframes, buffer) || off() || (isMute() && !_prefader)) 
    //if(off() || !getData(pos, srcChannels, nframes, buffer) || (isMute() && !_prefader)) 
    // p3.3.38
    if(off() || !getData(pos, srcTotalOutChans, nframes, buffer) || (isMute() && !_prefader)) 
    {
      #ifdef NODE_DEBUG
      printf("MusE: AudioTrack::copyData name:%s dstChannels:%d zeroing buffers\n", name().latin1(), dstChannels);
      #endif
      
      // No data was available. Zero the supplied buffers.
      unsigned int q;
      for(i = 0; i < dstChannels; ++i) 
      {
        if(config.useDenormalBias) 
        {
          for(q = 0; q < nframes; ++q)
            dstBuffer[i][q] = denormalBias;
        } 
        else
          memset(dstBuffer[i], 0, sizeof(float) * nframes);
      }  
      
      for(i = 0; i < srcChans; ++i) 
      {
        //_meter[i] = 0;
        _meter[i] = 0.0;
        
        /*
        if(!usedirectbuf)
        {
          if(config.useDenormalBias) 
          {
            for(q = 0; q < nframes; ++q)
              outBuffers[i][q] = denormalBias;
          }
          else    
            memset(outBuffers[i], 0, sizeof(float) * nframes);
        }
        */
      }
      
      _haveData = false;
      _processed = true;
      return;
    }

    //---------------------------------------------------
    // apply plugin chain
    //---------------------------------------------------

    // p3.3.41
    //fprintf(stderr, "AudioTrack::copyData %s efx apply srcChans:%d\n", name().latin1(), srcChans);
    _efxPipe->apply(srcChans, nframes, buffer);

    //---------------------------------------------------
    // aux sends
    //---------------------------------------------------

    if(hasAuxSend() && !isMute()) 
    {
      AuxList* al = song->auxs();
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
      for(i = 0; i < srcChans; ++i) 
      {
        float* p = buffer[i];
        meter[i] = 0.0;
        for(unsigned k = 0; k < nframes; ++k) 
        {
          double f = fabs(*p);
          if(f > meter[i])
            meter[i] = f;
          ++p;
        }
        //_meter[i] = lrint(meter[i] * 32767.0);
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
        if(config.useDenormalBias) 
        {
          for(q = 0; q < nframes; q++)
            dstBuffer[i][q] = denormalBias;
        } 
        else 
          memset(dstBuffer[i], 0, sizeof(float) * nframes);
      }        
      
      /*
      if(!usedirectbuf)
      {
        for(i = 0; i < srcChannels; ++i) 
        {
          if(config.useDenormalBias) 
          {
            for(q = 0; q < nframes; ++q)
              outBuffers[i][q] = denormalBias;
          }
          else    
            memset(outBuffers[i], 0, sizeof(float) * nframes);
        }
      }
      */  
      
      _haveData = false;
      _processed = true;
      return;
    }
    
    // If we're using local cached 'pre-volume' buffers, copy the input buffers (as they are right now: post-effect pre-volume) back to them. 
    if(!usedirectbuf)
    {
      //for(i = 0; i < srcChannels; ++i)
      //  AL::dsp->cpy(outBuffers[i], buffer[i], nframes);
      // p3.3.38
      for(i = 0; i < srcTotalOutChans; ++i)
        AL::dsp->cpy(outBuffers[i], buffer[i], nframes);
    }
    
    // We have some data! Set to true.
    _haveData = true;
  }
  
  // Sanity check. Is source starting channel out of range? Just zero and return.
  if(srcStartChan >= srcTotalOutChans) 
  {
    unsigned int q;
    for(i = 0; i < dstChannels; ++i)
    {
      if(config.useDenormalBias) 
      {
        for(q = 0; q < nframes; q++)
          dstBuffer[i][q] = denormalBias;
      } 
      else 
        memset(dstBuffer[i], 0, sizeof(float) * nframes);
    }        
    _processed = true;
    return;
  }
  // Force a source range to fit actual available total out channels.
  if((srcStartChan + srcChans) > srcTotalOutChans) 
    srcChans = srcTotalOutChans - srcStartChan;
  
  //---------------------------------------------------
  // apply volume
  //    postfader metering
  //---------------------------------------------------

    
  if(srcChans == dstChannels) 
  {
    if(_prefader) 
    {
      for(int c = 0; c < dstChannels; ++c) 
      {
        // p3.3.38
        //float* sp = buffer[c];
        float* sp = buffer[c + srcStartChan];
        
        float* dp = dstBuffer[c];
        for(unsigned k = 0; k < nframes; ++k)
          *dp++ = (*sp++ * vol[c]);
      }
    }
    else 
    {
      for(int c = 0; c < dstChannels; ++c) 
      {
        meter[c] = 0.0;
        
        // p3.3.38
        //float* sp = buffer[c];
        float* sp = buffer[c + srcStartChan];
        
        float* dp = dstBuffer[c];
        //printf("2 dstBuffer[c]=%d\n",long(dstBuffer[c]));
        for(unsigned k = 0; k < nframes; ++k) 
        {
          float val = *sp++ * vol[c];
          *dp++ = val;
          double f = fabs(val);
          if(f > meter[c])
            meter[c] = f;
        }
        //_meter[c] = lrint(meter[c] * 32767.0);
        _meter[c] = meter[c];
        if(_meter[c] > _peak[c])
          _peak[c] = _meter[c];
      }
    }
  }
  else if(srcChans == 1 && dstChannels == 2) 
  {
    // p3.3.38
    //float* sp = buffer[0];
    float* sp = buffer[srcStartChan];
    
    if(_prefader) 
    {
      for(int c = 0; c < dstChannels; ++c) 
      {
        float* dp = dstBuffer[c];
        for(unsigned k = 0; k < nframes; ++k)
          *dp++ = (*sp++ * vol[c]);
      }
    }
    else 
    {
      meter[0] = 0.0;
      for(unsigned k = 0; k < nframes; ++k) 
      {
        float val = *sp++;
        double f = fabs(val) * _volume;
        if(f > meter[0])
          meter[0] = f;
        *(dstBuffer[0] + k) = val * vol[0];
        *(dstBuffer[1] + k) = val * vol[1];
      }
      //_meter[0] = lrint(meter[0] * 32767.0);
      _meter[0] = meter[0];
      if(_meter[0] > _peak[0])
        _peak[0] = _meter[0];
    }
  }
  else if(srcChans == 2 && dstChannels == 1) 
  {
    // p3.3.38
    //float* sp1 = buffer[0];
    //float* sp2 = buffer[1];
    float* sp1 = buffer[srcStartChan];
    float* sp2 = buffer[srcStartChan + 1];
    
    if(_prefader) 
    {
      float* dp = dstBuffer[0];
      for(unsigned k = 0; k < nframes; ++k)
        *dp++ = (*sp1++ * vol[0] + *sp2++ * vol[1]);
    }
    else 
    {
      float* dp = dstBuffer[0];
      meter[0] = 0.0;
      meter[1] = 0.0;
      for(unsigned k = 0; k < nframes; ++k) 
      {
        float val1 = *sp1++ * vol[0];
        float val2 = *sp2++ * vol[1];
        double f1 = fabs(val1);
        if(f1 > meter[0])
          meter[0] = f1;
        double f2 = fabs(val2);
        if(f2 > meter[1])
          meter[1] = f2;
        *dp++ = (val1 + val2);
      }
      //_meter[0] = lrint(meter[0] * 32767.0);
      _meter[0] = meter[0];
      if(_meter[0] > _peak[0])
        _peak[0] = _meter[0];
      //_meter[1] = lrint(meter[1] * 32767.0);
      _meter[1] = meter[1];
      if(_meter[1] > _peak[1])
        _peak[1] = _meter[1];
    }
  }
        
  _processed = true;
}

//---------------------------------------------------------
//   addData
//---------------------------------------------------------

//void AudioTrack::addData(unsigned pos, int dstChannels, unsigned nframes, float** dstBuffer)
void AudioTrack::addData(unsigned pos, int dstChannels, int srcStartChan, int srcChannels, unsigned nframes, float** dstBuffer)
{
  //Changed by T356. 12/12/09. 
  // Overhaul and streamline to eliminate multiple processing during one process loop.
  // Was causing ticking sound with synths + multiple out routes because synths were being processed multiple times.
  // Make better use of AudioTrack::outBuffers as a post-effect pre-volume cache system for multiple calls here during processing.
  // Previously only WaveTrack used them. (Changed WaveTrack as well).
  
  //Added by Tim. p3.3.16
  #ifdef NODE_DEBUG
  printf("MusE: AudioTrack::addData name:%s processed:%d\n", name().latin1(), processed());
  #endif
  
  if (off())
  {
    _processed = true;
    return;
  } 
        
  if(srcStartChan == -1)
    srcStartChan = 0;
    
  int srcChans = (srcChannels == -1) ? channels() : srcChannels;
  int srcTotalOutChans = totalOutChannels();
  if(channels() == 1)
    srcTotalOutChans = 1;
  
  // Special consideration for metronome: It is not part of the track list,
  //  and it has no in or out routes, yet multiple output tracks may call addData on it !
  // We can't tell how many output tracks call it, so we can only assume there might be more than one.
  //bool usedirectbuf = (outRoutes()->size() <= 1) || (type() == AUDIO_OUTPUT);
  bool usedirectbuf = ((outRoutes()->size() <= 1) || (type() == AUDIO_OUTPUT)) && (this != metronome);
  
  int i;
  
  // p3.3.38
  //float* buffer[srcChannels];
  float* buffer[srcTotalOutChans];
  
  //float data[nframes * srcChannels];
  //for (i = 0; i < srcChannels; ++i)
  //      buffer[i] = data + i * nframes;
  
  // precalculate stereo volume
  double vol[2];
  double _volume = volume();
  double _pan = pan();
  vol[0] = _volume * (1.0 - _pan);
  vol[1] = _volume * (1.0 + _pan);
  float meter[srcChans];

  // Have we been here already during this process cycle?
  if(processed())
  {
    // If there is only one (or no) output routes, it's an error - we've been called more than once per process cycle!
    #ifdef NODE_DEBUG
    if(usedirectbuf)
      printf("MusE: AudioTrack::addData Error! One or no out routes, but already processed! Copying local buffers anyway...\n");
    #endif
    
    // Is there already some data gathered from a previous call during this process cycle?
    if(_haveData)
    {
      // Point the input buffers at our local cached 'pre-volume' buffers. They need processing, so continue on after.
      //for(i = 0; i < srcChannels; ++i)
      //  buffer[i] = outBuffers[i];
      // p3.3.38
      for(i = 0; i < srcTotalOutChans; ++i)
        buffer[i] = outBuffers[i];
    }
    else
      // No data was available from a previous call during this process cycle. Nothing to add, just return.
      return;  
  }
  else
  {
    // First time here during this process cycle. 
    
    // Point the input buffers at a temporary stack buffer.
    //float data[nframes * srcChannels];
    //for(i = 0; i < srcChannels; ++i)
    //  buffer[i] = data + i * nframes;
    // p3.3.38
    float data[nframes * srcTotalOutChans];
    for(i = 0; i < srcTotalOutChans; ++i)
        buffer[i] = data + i * nframes;
  
    
    // getData can use the supplied buffers, or change buffer to point to its own local buffers or Jack buffers etc. 
    // For ex. if this is an audio input, Jack will set the pointers for us.
    //if(!getData(pos, srcChannels, nframes, buffer)) 
    // p3.3.38
    if(!getData(pos, srcTotalOutChans, nframes, buffer)) 
    {
      // No data was available. Nothing to add, but zero our local buffers and the meters.
      for(i = 0; i < srcChans; ++i)
      {
        // If we're using local buffers, we must zero them so that the next thing requiring them 
        //  during this process cycle will see zeros.
        /*
        if(!usedirectbuf)
        {
          if(config.useDenormalBias) 
          {
            for(unsigned int q = 0; q < nframes; ++q)
              outBuffers[i][q] = denormalBias;
          }  
          else
            memset(outBuffers[i], 0, sizeof(float) * nframes);
        }
        */  
          
        //_meter[i] = 0;
        _meter[i] = 0.0;
      }      
      
      _haveData = false;
      _processed = true;
      return;
    }

    /*
    // p3.3.41 Added.
    unsigned int q;
    for(i = 0; i < srcChans; ++i) 
    {
      if(config.useDenormalBias) 
      {
        for(q = 0; q < nframes; ++q)
        {
          if(q & 1)
            buffer[i][q] -= denormalBias;
          else
            buffer[i][q] += denormalBias;
        }  
      } 
    }  
    */
    
    //---------------------------------------------------
    // apply plugin chain
    //---------------------------------------------------

    // p3.3.41
    //fprintf(stderr, "AudioTrack::addData %s efx apply srcChans:%d nframes:%ld %e %e %e %e\n", 
    //        name().latin1(), srcChans, nframes, buffer[0][0], buffer[0][1], buffer[0][2], buffer[0][3]);
    _efxPipe->apply(srcChans, nframes, buffer);
    // p3.3.41
    //fprintf(stderr, "AudioTrack::addData after efx: %e %e %e %e\n", 
    //        buffer[0][0], buffer[0][1], buffer[0][2], buffer[0][3]);

    //---------------------------------------------------
    // aux sends
    //---------------------------------------------------

    if(hasAuxSend() && !isMute()) 
    {
      AuxList* al = song->auxs();
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
      for(i = 0; i < srcChans; ++i) 
      {
        float* p = buffer[i];
        meter[i] = 0.0;
        for(unsigned k = 0; k < nframes; ++k) 
        {
          double f = fabs(*p);
          if(f > meter[i])
            meter[i] = f;
          ++p;
        }
        //_meter[i] = lrint(meter[i] * 32767.0);
        _meter[i] = meter[i];
        if(_meter[i] > _peak[i])
          _peak[i] = _meter[i];
      }
    }
    
    if(isMute())
    {
      // If we're using local buffers, we must zero them.
      /*
      if(!usedirectbuf)
      {
        for(i = 0; i < srcChannels; ++i)
        {
          if(config.useDenormalBias) 
          {
            for(unsigned int q = 0; q < nframes; ++q)
              outBuffers[i][q] = denormalBias;
          }  
          else
            memset(outBuffers[i], 0, sizeof(float) * nframes);
        }    
      }  
      */
      
      _haveData = false;
      _processed = true;
      return;
    }
    
    // If we're using local cached 'pre-volume' buffers, copy the input buffers (as they are right now: post-effect pre-volume) back to them. 
    if(!usedirectbuf)
    {
      //for(i = 0; i < srcChannels; ++i)
      //  AL::dsp->cpy(outBuffers[i], buffer[i], nframes);
      // p3.3.38
      for(i = 0; i < srcTotalOutChans; ++i)
        AL::dsp->cpy(outBuffers[i], buffer[i], nframes);
    }
    
    // We have some data! Set to true.
    _haveData = true;
  }  
  
  // Sanity check. Is source starting channel out of range? Just zero and return.
  if(srcStartChan >= srcTotalOutChans) 
  {
    unsigned int q;
    for(i = 0; i < dstChannels; ++i)
    {
      if(config.useDenormalBias) 
      {
        for(q = 0; q < nframes; q++)
          dstBuffer[i][q] = denormalBias;
      } 
      else 
        memset(dstBuffer[i], 0, sizeof(float) * nframes);
    }        
    _processed = true;
    return;
  }
  // Force a source range to fit actual available total out channels.
  if((srcStartChan + srcChans) > srcTotalOutChans) 
    srcChans = srcTotalOutChans - srcStartChan;
  
  //---------------------------------------------------
  // apply volume
  //    postfader metering
  //---------------------------------------------------

  if(srcChans == dstChannels) 
  {
    if(_prefader) 
    {
      for(int c = 0; c < dstChannels; ++c) 
      {
        // p3.3.38
        //float* sp = buffer[c];
        float* sp = buffer[c + srcStartChan];
        
        float* dp = dstBuffer[c];
        for(unsigned k = 0; k < nframes; ++k)
          *dp++ += (*sp++ * vol[c]);
      }
    }
    else 
    {
      for(int c = 0; c < dstChannels; ++c) 
      {
        meter[c] = 0.0;
        // p3.3.38
        //float* sp = buffer[c];
        float* sp = buffer[c + srcStartChan];
        
        float* dp = dstBuffer[c];
        for(unsigned k = 0; k < nframes; ++k) 
        {
          float val = *sp++ * vol[c];
          *dp++ += val;
          double f = fabs(val);
          if (f > meter[c])
                meter[c] = f;
        }
        //_meter[c] = lrint(meter[c] * 32767.0);
        _meter[c] = meter[c];
        if(_meter[c] > _peak[c])
          _peak[c] = _meter[c];
      }
    }
  }
  else if(srcChans == 1 && dstChannels == 2) 
  {
    // p3.3.38
    float* sp = buffer[srcStartChan];
    
    if(_prefader) 
    {
      for(int c = 0; c < dstChannels; ++c) 
      {
        float* dp = dstBuffer[c];
        //float* sp = buffer[0];
        for(unsigned k = 0; k < nframes; ++k)
          *dp++ += (*sp++ * vol[c]);
      }
    }
    else 
    {
      //float* sp = buffer[0];
      meter[0]  = 0.0;
      for(unsigned k = 0; k < nframes; ++k) 
      {
        float val = *sp++;
        double f = fabs(val) * _volume;
        if(f > meter[0])
          meter[0] = f;
        *(dstBuffer[0] + k) += val * vol[0];
        *(dstBuffer[1] + k) += val * vol[1];
      }
      //_meter[0] = lrint(meter[0] * 32767.0);
      _meter[0] = meter[0];
      if(_meter[0] > _peak[0])
        _peak[0] = _meter[0];
    }
  }
  else if(srcChans == 2 && dstChannels == 1) 
  {
    // p3.3.38
    //float* sp1 = buffer[0];
    //float* sp2 = buffer[1];
    float* sp1 = buffer[srcStartChan];
    float* sp2 = buffer[srcStartChan + 1];
    
    if(_prefader) 
    {
      float* dp = dstBuffer[0];
      for(unsigned k = 0; k < nframes; ++k)
        *dp++ += (*sp1++ * vol[0] + *sp2++ * vol[1]);
    }
    else 
    {
      float* dp = dstBuffer[0];
      meter[0] = 0.0;
      meter[1] = 0.0;
      for(unsigned k = 0; k < nframes; ++k) 
      {
        float val1 = *sp1++ * vol[0];
        float val2 = *sp2++ * vol[1];
        double f1 = fabs(val1);
        if(f1 > meter[0])
          meter[0] = f1;
        double f2 = fabs(val2);
        if(f2 > meter[1])
          meter[1] = f2;
        *dp++ += (val1 + val2);
      }
      //_meter[0] = lrint(meter[0] * 32767.0);
      _meter[0] = meter[0];
      if(_meter[0] > _peak[0])
        _peak[0] = _meter[0];
      //_meter[1] = lrint(meter[1] * 32767.0);
      _meter[1] = meter[1];
      if(_meter[1] > _peak[1])
        _peak[1] = _meter[1];
    }
  }
  
  _processed = true;
}

//---------------------------------------------------------
//   readVolume
//---------------------------------------------------------

void AudioTrack::readVolume(Xml& xml)
      {
      int ch = 0;
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
                              ch = xml.s2().toInt();
                        break;
                  case Xml::TagEnd:
                        if (xml.s1() == "volume")
                              return;
                  default:
                        break;
                  }
            }
      }

// Removed by T356
// "recfile" tag not saved anymore
/*
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
                                             path.latin1(), recFile()->strerror().latin1());
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
            //_meter[i] = 0;
            _meter[i] = 0.0;
            //_peak[i]  = 0;
            _peak[i]  = 0.0;
            }
      }

void AudioInput::setChannels(int n)
      {
      if (n == _channels)
            return;
//was ist mit:    void* jackPorts[MAX_CHANNELS];
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
      if (fifo.put(channels, n, bp, audio->pos().frame())) {
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
      
      #ifdef NODE_DEBUG
      printf("AudioTrack::getData name:%s inRoutes:%d\n", name().latin1(), rl->size());
      #endif
      
      iRoute ir = rl->begin();
      if (ir == rl->end())
            return false;
      
      if(ir->track->isMidiTrack())
        return false;
        
      #ifdef NODE_DEBUG
      printf("    calling copyData on %s...\n", ir->track->name().latin1());
      #endif
      
      // p3.3.38
      //((AudioTrack*)ir->track)->copyData(pos, channels, nframes, buffer);
      ((AudioTrack*)ir->track)->copyData(pos, channels, 
                                         //(ir->track->type() == Track::AUDIO_SOFTSYNTH && ir->channel != -1) ? ir->channel : 0,
                                         ir->channel,
                                         ir->channels,
                                         nframes, buffer);
      
      // p3.3.41
      //fprintf(stderr, "AudioTrack::getData %s data: nframes:%ld %e %e %e %e\n", name().latin1(), nframes, buffer[0][0], buffer[0][1], buffer[0][2], buffer[0][3]);
      
      ++ir;
      for (; ir != rl->end(); ++ir) {
            #ifdef NODE_DEBUG
            printf("    calling addData on %s...\n", ir->track->name().latin1());
            #endif
              
            if(ir->track->isMidiTrack())
              continue;
              
            // p3.3.38
            //((AudioTrack*)ir->track)->addData(pos, channels, nframes, buffer);
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
      if (!checkAudioDevice()) return false;
      for (int ch = 0; ch < channels; ++ch) 
      {
            void* jackPort = jackPorts[ch];
            //float* jackbuf = 0;
            
            //if (jackPort) {
            // p3.3.41 Do not get buffers of unconnected client ports. Causes repeating leftover data, can be loud, or DC !
            if (jackPort && audioDevice->connections(jackPort)) 
            {
                  //buffer[ch] = audioDevice->getBuffer(jackPort, nframes);
                  // p3.3.41 If the client port buffer is also used by another channel (connected to the same jack port), 
                  //  don't directly set pointer, copy the data instead. 
                  // Otherwise the next channel will interfere - it will overwrite the buffer !
                  // Verified symptoms: Can't use a splitter. Mono noise source on a stereo track sounds in mono. Etc...
                  // TODO: Problem: What if other Audio Input tracks share the same jack ports as this Audio Input track?
                  // Users will expect that Audio Inputs just work even if the input routes originate from the same jack port.
                  // Solution: Rather than having to iterate all other channels, and all other Audio Input tracks and check 
                  //  their channel port buffers (if that's even possible) in order to determine if the buffer is shared, 
                  //  let's just copy always, for now shall we ?
                  float* jackbuf = audioDevice->getBuffer(jackPort, nframes);
                  //memcpy(buffer[ch], jackbuf, nframes* sizeof(float));
                  AL::dsp->cpy(buffer[ch], jackbuf, nframes);
                  
                  if (config.useDenormalBias) 
                  {
                      for (unsigned int i=0; i < nframes; i++)
                              buffer[ch][i] += denormalBias;
                      
                      // p3.3.41
                      //fprintf(stderr, "AudioInput::getData %s Jack port %p efx apply channels:%d nframes:%ld %e %e %e %e\n", 
                      //        name().latin1(), jackPort, channels, nframes, buffer[0][0], buffer[0][1], buffer[0][2], buffer[0][3]);
                  }
            } 
            else 
            {
                  if (config.useDenormalBias) 
                  {
                      for (unsigned int i=0; i < nframes; i++)
                              buffer[ch][i] = denormalBias;
                  } 
                  else 
                  {
                              memset(buffer[ch], 0, nframes * sizeof(float));
                  }
                  
                  // p3.3.41
                  //fprintf(stderr, "AudioInput::getData %s No Jack port efx apply channels:%d nframes:%ld %e %e %e %e\n", 
                  //        name().latin1(), channels, nframes, buffer[0][0], buffer[0][1], buffer[0][2], buffer[0][3]);
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
      if (!checkAudioDevice()) return;
      for (int i = 0; i < channels(); ++i) {
            char buffer[128];
            snprintf(buffer, 128, "%s-%d", _name.latin1(), i);
            if (jackPorts[i])
                  audioDevice->setPortName(jackPorts[i], buffer);
            else {
                  //jackPorts[i] = audioDevice->registerInPort(buffer);
                  jackPorts[i] = audioDevice->registerInPort(buffer, false);
                  }
            }
      }

//---------------------------------------------------------
//   resetMeter
//---------------------------------------------------------

void Track::resetMeter()
      {
      for (int i = 0; i < _channels; ++i)
            //_meter[i] = 0;
            _meter[i] = 0.0;
      }

//---------------------------------------------------------
//   resetPeaks
//---------------------------------------------------------

void Track::resetPeaks()
      {
      for (int i = 0; i < _channels; ++i)
            //_peak[i] = 0;
            _peak[i] = 0.0;
            _lastActivity = 0;
      }

//---------------------------------------------------------
//   resetAllMeter
//---------------------------------------------------------

void Track::resetAllMeter()
      {
      TrackList* tl = song->tracks();
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
      return  (!noInRoute() || (this == song->bounceTrack));
      }

//---------------------------------------------------------
//   record
//---------------------------------------------------------

void AudioTrack::record()
      {
      unsigned pos = 0;
      float* buffer[_channels];

      //printf("AudioTrack: record() fifo %p, count=%d\n", &fifo, fifo.getCount());

      while(fifo.getCount()) {

            if (fifo.get(_channels, segmentSize, buffer, &pos)) {
                  printf("AudioTrack::record(): empty fifo\n");
                  return;
                  }
            if (_recFile) {
                  // Line removed by Tim. p3.3.8 Oct 28, 2009
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
                  if(song->punchin() && (audio->loopCount() == 0))
                    fr = song->lPos().frame();
                  else  
                  if((audio->loopCount() > 0) && (audio->getStartRecordPos().frame() > audio->loopFrame()))
                    fr = audio->loopFrame();
                  else
                    fr = audio->getStartRecordPos().frame();
                  // Now seek and write. If we are looping and punchout is on, don't let punchout point interfere with looping point.
                  if( (pos >= fr) && (!song->punchout() || (!song->loop() && pos < song->rPos().frame())) )
                  {
                    pos -= fr;
                    // Added by Tim. p3.3.8
                    //int position = _recFile->seek(0, SEEK_CUR);
                    //printf("AudioTrack::record loopcnt:%d lframe:%d newpos:%d curpos:%d start:%d end:%d\n", audio->loopCount(), audio->loopFrame(), pos, position, audio->getStartRecordPos().frame(), audio->getEndRecordPos().frame());
                    
                    _recFile->seek(pos, 0);
                    _recFile->write(_channels, buffer, segmentSize);
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
      if (!checkAudioDevice()) return;
      for (int i = 0; i < channels(); ++i) {
            if (jackPorts[i]) {
                  buffer[i] = audioDevice->getBuffer(jackPorts[i], nframes);
                  if (config.useDenormalBias) {
                      for (unsigned int j=0; j < nframes; j++)
                              buffer[i][j] += denormalBias;
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
      //Added by Tim. p3.3.16
      #ifdef NODE_DEBUG
      printf("MusE: AudioOutput::process name:%s processed:%d\n", name().latin1(), processed());
      #endif
      
      for (int i = 0; i < _channels; ++i) {
            buffer1[i] = buffer[i] + offset;
      }
      
      // p3.3.38
      //copyData(pos, _channels, n, buffer1);
      copyData(pos, _channels, -1, -1, n, buffer1);
}

//---------------------------------------------------------
//   silence
//---------------------------------------------------------

void AudioOutput::silence(unsigned n)
      {
      processInit(n);
      for (int i = 0; i < channels(); ++i)
          if (config.useDenormalBias) {
              for (unsigned int j=0; j < n; j++)
                  buffer[i][j] = denormalBias;
            } else {
                  memset(buffer[i], 0, n * sizeof(float));
                  }
      }

//---------------------------------------------------------
//   processWrite
//---------------------------------------------------------

void AudioOutput::processWrite()
      {
      if (audio->isRecording() && song->bounceOutput == this) {
            if (audio->freewheel()) {
                  WaveTrack* track = song->bounceTrack;
                  if (track && track->recordFlag() && track->recFile())
                        track->recFile()->write(_channels, buffer, _nframes);
                  if (recordFlag() && recFile())
                        _recFile->write(_channels, buffer, _nframes);
                  }
            else {
                  WaveTrack* track = song->bounceTrack;
                  if (track && track->recordFlag() && track->recFile())
                        track->putFifo(_channels, _nframes, buffer);
                  if (recordFlag() && recFile())
                        putFifo(_channels, _nframes, buffer);
                  }
            }
      // Changed by Tim. p3.3.18
      //if (audioClickFlag && song->click()) {
      if (sendMetronome() && audioClickFlag && song->click()) {
            
            // Added by Tim. p3.3.18
            #ifdef METRONOME_DEBUG
            printf("MusE: AudioOutput::processWrite Calling metronome->addData frame:%u channels:%d frames:%lu\n", audio->pos().frame(), _channels, _nframes);
            #endif
    
            // p3.3.38
            //metronome->addData(audio->pos().frame(), _channels, _nframes, buffer);
            metronome->addData(audio->pos().frame(), _channels, -1, -1, _nframes, buffer);
            }
      }
//---------------------------------------------------------
//   setName
//---------------------------------------------------------

void AudioOutput::setName(const QString& s)
      {
      _name = s;
      if (!checkAudioDevice()) return;
      for (int i = 0; i < channels(); ++i) {
            char buffer[128];
            snprintf(buffer, 128, "%s-%d", _name.latin1(), i);
            if (jackPorts[i]) {
                  audioDevice->setPortName(jackPorts[i], buffer);
                  }
            else {
                  //jackPorts[i] = audioDevice->registerOutPort(buffer);
                  jackPorts[i] = audioDevice->registerOutPort(buffer, false);
                  }
            }
      }

//---------------------------------------------------------
//   Fifo
//---------------------------------------------------------

Fifo::Fifo()
      {
      muse_atomic_init(&count);
      //nbuffer = FIFO_BUFFER;
      nbuffer = fifoLength;
      buffer  = new FifoBuffer*[nbuffer];
      for (int i = 0; i < nbuffer; ++i)
            buffer[i]  = new FifoBuffer;
      clear();
      }

Fifo::~Fifo()
      {
      for (int i = 0; i < nbuffer; ++i)
      {
        // p3.3.45
        if(buffer[i]->buffer)
        {
          //printf("Fifo::~Fifo freeing buffer\n");
          free(buffer[i]->buffer);
        }
          
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
      // Added by Tim. p3.3.17
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
              // Changed by Tim. p3.3.15
              //delete[] b->buffer;
              free(b->buffer);
              // p3.3.45
              b->buffer = 0;
            }     
            // Changed by Tim. p3.3.15
            //b->buffer  = new float[n];
            posix_memalign((void**)&(b->buffer), 16, sizeof(float) * n);
            // p3.3.45
            if(!b->buffer)
            {
              printf("Fifo::put could not allocate buffer segs:%d samples:%lu pos:%u\n", segs, samples, pos);
              return true;
            }
            
            b->maxSize = n;
            }
      // p3.3.45
      if(!b->buffer)
      {
        printf("Fifo::put no buffer! segs:%d samples:%lu pos:%u\n", segs, samples, pos);
        return true;
      }
      
      b->size = samples;
      b->segs = segs;
      b->pos  = pos;
      for (int i = 0; i < segs; ++i)
            //memcpy(b->buffer + i * samples, src[i], samples * sizeof(float));
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
      // Added by Tim. p3.3.17
      #ifdef FIFO_DEBUG
      printf("FIFO::get segs:%d samples:%lu\n", segs, samples);
      #endif
      
      if (muse_atomic_read(&count) == 0) {
            printf("FIFO %p underrun... %d\n", this,count.counter); //by willyfoobar: added count to output //see Fifo::put()
            return true;
            }
      FifoBuffer* b = buffer[ridx];
      // p3.3.45
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
      // Added by Tim. p3.3.17
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
              // Changed by Tim. p3.3.15
              //delete[] b->buffer;
              free(b->buffer);
              // p3.3.45
              b->buffer = 0;
            }
            
            // Changed by Tim. p3.3.15
            //b->buffer = new float[n];
            posix_memalign((void**)&(b->buffer), 16, sizeof(float) * n);
            // p3.3.45
            if(!b->buffer)
            {
              printf("Fifo::getWriteBuffer could not allocate buffer segs:%d samples:%lu pos:%u\n", segs, samples, pos);
              return true;
            }
            
            b->maxSize = n;
            }
      
      // p3.3.45
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
      if(num == _totalOutChannels)
        return;
        
      int chans = _totalOutChannels;
      // Number of allocated buffers is always MAX_CHANNELS or more, even if _totalOutChannels is less. 
      if(chans < MAX_CHANNELS)
        chans = MAX_CHANNELS;
      for(int i = 0; i < chans; ++i) 
      {
        if(outBuffers[i])
          free(outBuffers[i]);
      }
      delete[] outBuffers;
      
      _totalOutChannels = num;
      chans = num;
      // Number of allocated buffers is always MAX_CHANNELS or more, even if _totalOutChannels is less. 
      if(chans < MAX_CHANNELS)
        chans = MAX_CHANNELS;
        
      outBuffers = new float*[chans];
      for (int i = 0; i < chans; ++i)
            posix_memalign((void**)&outBuffers[i], 16, sizeof(float) * segmentSize);
      
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

