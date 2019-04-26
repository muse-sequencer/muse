//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: node.cpp,v 1.36.2.25 2009/12/20 05:00:35 terminator356 Exp $
//
//  (C) Copyright 2000-2004 Werner Schweer (ws@seh.de)
//  (C) Copyright 2011-2013 Tim E. Real (terminator356 on sourceforge)
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

#include "muse_math.h"
#include <sndfile.h>
#include <stdlib.h>
#include <stdio.h>

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
#include "wavepreview.h"
#include "al/dsp.h"
#include "latency_compensator.h"

// REMOVE Tim. Persistent routes. Added. Make this permanent later if it works OK and makes good sense.
#define _USE_SIMPLIFIED_SOLO_CHAIN_

// Turn on debugging messages
//#define NODE_DEBUG
// Turn on constant flow of process debugging messages
//#define NODE_DEBUG_PROCESS

//#define FIFO_DEBUG
//#define METRONOME_DEBUG

namespace MusECore {

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
#ifndef _USE_SIMPLIFIED_SOLO_CHAIN_
      else
      // Support Midi Port -> Audio Input solo chains.
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
#endif
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
  }

#ifdef _USE_SIMPLIFIED_SOLO_CHAIN_
  // Support Midi Track to Audio Input track soloing chain routes.
  // Support omni routes only, because if channels are supported, the graphical router becomes more complicated.
  const RouteList* rl = outRoutes();
  for(ciRoute ir = rl->begin(); ir != rl->end(); ++ir)
  {
    if(ir->type == Route::TRACK_ROUTE && ir->track && ir->track->type() == Track::AUDIO_INPUT && ir->channel == -1)
      ir->track->updateInternalSoloStates();
  }
#else
  // Support Midi Port -> Audio Input solo chains.
  const int chbits = 1 << outChannel();
  const RouteList* rl = mp->outRoutes();
  for(ciRoute ir = rl->begin(); ir != rl->end(); ++ir)
  {
    if(ir->type == Route::TRACK_ROUTE && ir->track && ir->track->type() == Track::AUDIO_INPUT && (ir->channel & chbits) )
      ir->track->updateInternalSoloStates();
  }
#endif

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

  const RouteList* rl = inRoutes();
  for(ciRoute ir = rl->begin(); ir != rl->end(); ++ir)
  {
    if(ir->type == Route::TRACK_ROUTE)
      ir->track->updateInternalSoloStates();
#ifndef _USE_SIMPLIFIED_SOLO_CHAIN_
    else
    // Support Midi Port -> Audio Input solo chains.
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
#endif
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
//   processTrackCtrls
//   If trackChans is 0, just process controllers only, not audio (do not 'run').
//---------------------------------------------------------

void AudioTrack::processTrackCtrls(unsigned pos, int trackChans, unsigned nframes, float** buffer)
{
  const unsigned long syncFrame = MusEGlobal::audio->curSyncFrame();
  const unsigned long min_per = (MusEGlobal::config.minControlProcessPeriod > nframes) ? nframes : MusEGlobal::config.minControlProcessPeriod;
  unsigned long sample = 0;

  const AutomationType at = automationType();
  const bool no_auto = !MusEGlobal::automation || at == AUTO_OFF;
  CtrlListList* cll = controller();
  CtrlList* vol_ctrl = 0;
  CtrlList* pan_ctrl = 0;
  {
    ciCtrlList icl = cll->find(AC_VOLUME);
    if(icl == cll->end())
      return;
    vol_ctrl = icl->second;
    icl = cll->find(AC_PAN);
    if(icl == cll->end())
      return;
    pan_ctrl = icl->second;
  }

  int cur_slice = 0;
  while(sample < nframes)
  {
    unsigned long nsamp = nframes - sample;
    const unsigned long slice_frame = pos + sample;

    // Process automation control values, while also determining the maximum acceptable
    //  size of this run. Further processing, from FIFOs for example, can lower the size
    //  from there, but this section determines where the next highest maximum frame
    //  absolutely needs to be for smooth playback of the controller value stream...
    //
    if(trackChans != 0 && !_prefader)  // Don't bother if we don't want to run, or prefader is on.
    {
      ciCtrlList icl = cll->begin();
      for(unsigned long k = 0; k < _controlPorts; ++k)
      {
        CtrlList* cl = (icl != cll->end() ? icl->second : NULL);
        CtrlInterpolate& ci = _controls[k].interp;
        // Always refresh the interpolate struct at first, since things may have changed.
        // Or if the frame is outside of the interpolate range - and eStop is not true.  // FIXME TODO: Be sure these comparisons are correct.
        if(cur_slice == 0 || (!ci.eStop && MusEGlobal::audio->isPlaying() &&
            (slice_frame < (unsigned long)ci.sFrame || (ci.eFrameValid && slice_frame >= (unsigned long)ci.eFrame)) ) )
        {
          if(cl && (unsigned long)cl->id() == k)
          {
            cl->getInterpolation(slice_frame, no_auto || !_controls[k].enCtrl, &ci);
            if(icl != cll->end())
              ++icl;
          }
          else
          {
            // No matching controller, or end. Just copy the current value into the interpolator.
            // Keep the current icl iterator, because since they are sorted by frames,
            //  if the IDs didn't match it means we can just let k catch up with icl.
            ci.sFrame   = 0;
            ci.eFrame   = 0;
            ci.eFrameValid = false;
            ci.sVal     = _controls[k].dval;
            ci.eVal     = ci.sVal;
            ci.doInterp = false;
            ci.eStop    = false;
          }
        }
        else
        {
          if(ci.eStop && ci.eFrameValid && slice_frame >= (unsigned long)ci.eFrame)  // FIXME TODO: Get that comparison right.
          {
            // Clear the stop condition and set up the interp struct appropriately as an endless value.
            ci.sFrame   = 0; //ci->eFrame;
            ci.eFrame   = 0;
            ci.eFrameValid = false;
            ci.sVal     = ci.eVal;
            ci.doInterp = false;
            ci.eStop    = false;
          }
          if(icl != cll->end())
            ++icl;
        }

        if(MusEGlobal::audio->isPlaying())
        {
          unsigned long samps = nsamp;
          if(ci.eFrameValid)
            samps = (unsigned long)ci.eFrame - slice_frame;
          if(samps < nsamp)
            nsamp = samps;
        }

#ifdef NODE_DEBUG_PROCESS
        fprintf(stderr, "AudioTrack::processTrackCtrls k:%lu sample:%lu frame:%lu nextFrame:%d nsamp:%lu \n", k, sample, slice_frame, ci.eFrame, nsamp);
#endif
      }
    }

#ifdef NODE_DEBUG_PROCESS
    fprintf(stderr, "AudioTrack::processTrackCtrls sample:%lu nsamp:%lu\n", sample, nsamp);
#endif

    //
    // Process all control ring buffer items valid for this time period...
    //
    bool found = false;
    unsigned long frame = 0;
    unsigned long evframe;
    while(!_controlFifo.isEmpty())
    {
      const ControlEvent& v = _controlFifo.peek();
      // The events happened in the last period or even before that. Shift into this period with + n. This will sync with audio.
      // If the events happened even before current frame - n, make sure they are counted immediately as zero-frame.
      evframe = (syncFrame > v.frame + nframes) ? 0 : v.frame - syncFrame + nframes;

      // Protection. Observed this condition. Why? Supposed to be linear timestamps.
      if(found && evframe < frame)
      {
        fprintf(stderr, 
          "AudioTrack::processTrackCtrls *** Error: Event out of order: evframe:%lu < frame:%lu idx:%lu val:%f unique:%d syncFrame:%lu nframes:%u v.frame:%lu\n",
          evframe, frame, v.idx, v.value, v.unique, syncFrame, nframes, v.frame);

        // No choice but to ignore it.
        _controlFifo.remove();               // Done with the ring buffer's item. Remove it.
        continue;
      }

      if(evframe >= nframes                                            // Next events are for a later period.
          || (!found && !v.unique && (evframe - sample >= nsamp))      // Next events are for a later run in this period. (Autom took prio.)
          || (found && !v.unique && (evframe - sample >= min_per)))    // Eat up events within minimum slice - they're too close.
        break;
//       _controlFifo.remove();               // Done with the ring buffer's item. Remove it.

      if(v.idx >= _controlPorts) // Sanity check
      {
        _controlFifo.remove();               // Done with the ring buffer's item. Remove it.
        break;
      }

      found = true;
      frame = evframe;

      if(trackChans != 0 && !_prefader)    // Only if we want to run, and prefader is off.
      {
        CtrlInterpolate* ci = &_controls[v.idx].interp;
        ci->eFrame = frame;
        ci->eFrameValid = true;
        ci->eVal   = v.value;
        ci->eStop  = true;
      }

      // Need to update the automation value, otherwise it overwrites later with the last automation value.
      setPluginCtrlVal(v.idx, v.value);
      
      _controlFifo.remove();               // Done with the ring buffer's item. Remove it.
    }

    if(found && trackChans != 0 && !_prefader)    // If a control FIFO item was found, takes priority over automation controller stream.
      nsamp = frame - sample;

    if(sample + nsamp > nframes)    // Safety check.
      nsamp = nframes - sample;

    // TODO: Don't allow zero-length runs. This could/should be checked in the control loop instead.
    // Note this means it is still possible to get stuck in the top loop (at least for a while).
    if(nsamp != 0)
    {
      if(trackChans != 0 && !_prefader)
      {
        const CtrlInterpolate& vol_interp = _controls[AC_VOLUME].interp;
        const CtrlInterpolate& pan_interp = _controls[AC_PAN].interp;
        unsigned long k;
        //const float up_fact = 1.002711275;      // 3.01.. dB / 256
        //const float down_fact = 0.997296056;
        const double up_fact = 1.003471749;      // 3.01.. dB / 200
        const double down_fact = 0.996540262;

        float *sp1, *sp2, *dp1, *dp2;
        sp1 = sp2 = dp1 = dp2 = NULL;
        double _volume, v, _pan, v1, v2;

        if(trackChans == 1)
        {
          sp1 = sp2 = buffer[0] + sample;
          dp1 = outBuffersExtraMix[0] + sample;
          dp2 = outBuffersExtraMix[1] + sample;
        }
        else
        {
          sp1  = buffer[0] + sample;
          sp2  = buffer[1] + sample;
          dp1  = outBuffers[0] + sample;
          dp2  = outBuffers[1] + sample;
        }

        if(trackChans != 2)
        {
          const int start_ch = trackChans == 1 ? 0 : 2;

          k = 0;
          if(vol_interp.doInterp && MusEGlobal::audio->isPlaying())
          {
            for( ; k < nsamp; ++k)
            {
              _volume = vol_ctrl->interpolate(slice_frame + k, vol_interp);
              v = _volume * _gain;
              if(v > _curVolume)
              {
                if(_curVolume == 0.0)
                  _curVolume = 0.001;  // Kick-start it from zero at -30dB.
                _curVolume *= up_fact;
                if(_curVolume >= v)
                  _curVolume = v;
              }
              else
              if(v < _curVolume)
              {
                _curVolume *= down_fact;
                if(_curVolume <= v || _curVolume <= 0.001)  // Or if less than -30dB.
                  _curVolume = v;
              }
              const unsigned long smp = sample + k;
              for(int ch = start_ch; ch < trackChans; ++ch)
                *(outBuffers[ch] + smp) = *(buffer[ch] + smp) * _curVolume;
            }
            _controls[AC_VOLUME].dval = _volume;    // Update the port.
          }
          else
          {
            if(vol_interp.doInterp) // And not playing...
              _volume = vol_ctrl->interpolate(pos, vol_interp);
            else
              _volume = vol_interp.sVal;
            _controls[AC_VOLUME].dval = _volume;    // Update the port.
            v = _volume * _gain;
            if(v > _curVolume)
            {
              //fprintf(stderr, "A %f %f\n", v, _curVolume);
              if(_curVolume == 0.0)
                _curVolume = 0.001;  // Kick-start it from zero at -30dB.
              for( ; k < nsamp; ++k)
              {
                _curVolume *= up_fact;
                if(_curVolume >= v)
                {
                  _curVolume = v;
                  break;
                }
                const unsigned long smp = sample + k;
                for(int ch = start_ch; ch < trackChans; ++ch)
                  *(outBuffers[ch] + smp) = *(buffer[ch] + smp) * _curVolume;
              }
            }
            else
            if(v < _curVolume)
            {
              //fprintf(stderr, "B %f %f\n", v, _curVolume);
              for( ; k < nsamp; ++k)
              {
                _curVolume *= down_fact;
                if(_curVolume <= v || _curVolume <= 0.001)  // Or if less than -30dB.
                {
                  _curVolume = v;
                  break;
                }
                const unsigned long smp = sample + k;
                for(int ch = start_ch; ch < trackChans; ++ch)
                  *(outBuffers[ch] + smp) = *(buffer[ch] + smp) * _curVolume;
              }
            }

            const unsigned long next_smp = sample + nsamp;
            for(unsigned long smp = sample + k; smp < next_smp; ++smp)
            {
              for(int ch = start_ch; ch < trackChans; ++ch)
                *(outBuffers[ch] + smp) = *(buffer[ch] + smp) * _curVolume;
            }
          }
        }

        k = 0;
        if((vol_interp.doInterp || pan_interp.doInterp) && MusEGlobal::audio->isPlaying())
        {
          for( ; k < nsamp; ++k)
          {
            _volume = vol_ctrl->interpolate(slice_frame + k, vol_interp);
            v = _volume * _gain;
            _pan = pan_ctrl->interpolate(slice_frame + k, pan_interp);
            v1 = v * (1.0 - _pan);
            v2 = v * (1.0 + _pan);
            if(v1 > _curVol1)
            {
              //fprintf(stderr, "C %f %f \n", v1, _curVol1);
              if(_curVol1 == 0.0)
                _curVol1 = 0.001;  // Kick-start it from zero at -30dB.
              _curVol1 *= up_fact;
              if(_curVol1 >= v1)
                _curVol1 = v1;
            }
            else
            if(v1 < _curVol1)
            {
              //fprintf(stderr, "D %f %f \n", v1, _curVol1);
              _curVol1 *= down_fact;
              if(_curVol1 <= v1 || _curVol1 <= 0.001)  // Or if less than -30dB.
                _curVol1 = v1;
            }
            *dp1++ = *sp1++ * _curVol1;

            if(v2 > _curVol2)
            {
              //fprintf(stderr, "E %f %f \n", v2, _curVol2);
              if(_curVol2 == 0.0)
                _curVol2 = 0.001;  // Kick-start it from zero at -30dB.
              _curVol2 *= up_fact;
              if(_curVol2 >= v2)
                _curVol2 = v2;
            }
            else
            if(v2 < _curVol2)
            {
              //fprintf(stderr, "F %f %f \n", v2, _curVol2);
              _curVol2 *= down_fact;
              if(_curVol2 <= v2 || _curVol2 <= 0.001)   // Or if less than -30dB.
                _curVol2 = v2;
            }
            *dp2++ = *sp2++ * _curVol2;
          }
          _controls[AC_VOLUME].dval = _volume;    // Update the ports.
          _controls[AC_PAN].dval = _pan;
        }
        else
        {
          if(vol_interp.doInterp)  // And not playing...
            _volume = vol_ctrl->interpolate(pos, vol_interp);
          else
            _volume = vol_interp.sVal;
          if(pan_interp.doInterp)  // And not playing...
            _pan = pan_ctrl->interpolate(pos, pan_interp);
          else
            _pan = pan_interp.sVal;
          _controls[AC_VOLUME].dval = _volume;    // Update the ports.
          _controls[AC_PAN].dval = _pan;
          v = _volume * _gain;
          v1  = v * (1.0 - _pan);
          v2  = v * (1.0 + _pan);
          if(v1 > _curVol1)
          {
            //fprintf(stderr, "C %f %f \n", v1, _curVol1);
            if(_curVol1 == 0.0)
              _curVol1 = 0.001;  // Kick-start it from zero at -30dB.
            for( ; k < nsamp; ++k)
            {
              _curVol1 *= up_fact;
              if(_curVol1 >= v1)
              {
                _curVol1 = v1;
                break;
              }
              *dp1++ = *sp1++ * _curVol1;
            }
          }
          else
          if(v1 < _curVol1)
          {
            //fprintf(stderr, "D %f %f \n", v1, _curVol1);
            for( ; k < nsamp; ++k)
            {
              _curVol1 *= down_fact;
              if(_curVol1 <= v1 || _curVol1 <= 0.001)  // Or if less than -30dB.
              {
                _curVol1 = v1;
                break;
              }
              *dp1++ = *sp1++ * _curVol1;
            }
          }
          for( ; k < nsamp; ++k)
            *dp1++ = *sp1++ * _curVol1;

          k = 0;
          if(v2 > _curVol2)
          {
            //fprintf(stderr, "E %f %f \n", v2, _curVol2);
            if(_curVol2 == 0.0)
              _curVol2 = 0.001;  // Kick-start it from zero at -30dB.
            for( ; k < nsamp; ++k)
            {
              _curVol2 *= up_fact;
              if(_curVol2 >= v2)
              {
                _curVol2 = v2;
                break;
              }
              *dp2++ = *sp2++ * _curVol2;
            }
          }
          else
          if(v2 < _curVol2)
          {
            //fprintf(stderr, "F %f %f \n", v2, _curVol2);
            for( ; k < nsamp; ++k)
            {
              _curVol2 *= down_fact;
              if(_curVol2 <= v2 || _curVol2 <= 0.001)   // Or if less than -30dB.
              {
                _curVol2 = v2;
                break;
              }
              *dp2++ = *sp2++ * _curVol2;
            }
          }
          for( ; k < nsamp; ++k)
            *dp2++ = *sp2++ * _curVol2;
        }
      }

#ifdef NODE_DEBUG_PROCESS
      fprintf(stderr, "AudioTrack::processTrackCtrls end of sample:%lu nsamp:%lu\n", sample, nsamp);
#endif

      sample += nsamp;
    }

#ifdef NODE_DEBUG_PROCESS
    fprintf(stderr, "AudioTrack::processTrackCtrls end of cur_slice:%d\n", cur_slice);
#endif

    ++cur_slice; // Slice is done. Moving on to any next slice now...
  }
}

//---------------------------------------------------------
//   copyData
//---------------------------------------------------------

void AudioTrack::copyData(unsigned pos,
                          int dstStartChan, int requestedDstChannels, int availDstChannels,
                          int srcStartChan, int srcChannels,
                          unsigned nframes, float** dstBuffer,
                          bool add, const bool* addArray)
{
  //Changed by T356. 12/12/09.
  // Overhaul and streamline to eliminate multiple processing during one process loop.
  // Was causing ticking sound with synths + multiple out routes because synths were being processed multiple times.
  // Make better use of AudioTrack::outBuffers as a post-effect pre-volume cache system for multiple calls here during processing.
  // Previously only WaveTrack used them. (Changed WaveTrack as well).

  #ifdef NODE_DEBUG_PROCESS
  fprintf(stderr, "MusE: AudioTrack::copyData name:%s processed:%d _haveData:%d\n", name().toLatin1().constData(), processed(), _haveData);
  #endif

  if(srcStartChan == -1)
    srcStartChan = 0;
  if(dstStartChan == -1)
    dstStartChan = 0;

  // Only the destination knows how many destination channels there are,
  //  while only the source (this track) knows how many source channels there are.
  // So take care of the source channels here, and let the caller handle the destination channels.
  const int trackChans = channels();
  const int srcTotalOutChans = totalProcessBuffers();
  const int requestedSrcChans = (srcChannels == -1) ? srcTotalOutChans : srcChannels;
  int availableSrcChans = requestedSrcChans;
  // Force a source range to fit actual available total out channels.
  if((srcStartChan + availableSrcChans) > srcTotalOutChans)
    availableSrcChans = srcTotalOutChans - srcStartChan;

  // Special consideration for metronome: It is not part of the track list,
  //  and it has no in or out routes, yet multiple output tracks may call addData on it!
  // We can't tell how many output tracks call it, so we can only assume there might be more than one.
  // Not strictly necessary here because only addData is ever called, but just to be consistent...

  int i;

  // Protection for pre-allocated _dataBuffers.
  if(nframes > MusEGlobal::segmentSize)
  {
    fprintf(stderr, "MusE: Error: AudioTrack::copyData: nframes:%u > segmentSize:%u\n", nframes, MusEGlobal::segmentSize);
    nframes = MusEGlobal::segmentSize;
  }

  float* buffer[srcTotalOutChans];
  double meter[trackChans];

  #ifdef NODE_DEBUG_PROCESS
    fprintf(stderr, "MusE: AudioTrack::copyData "
                    "trackChans:%d srcTotalOutChans:%d srcStartChan:%d srcChannels:%d "
                    "dstStartChan:%d"
                    "requestedSrcChans:%d availableSrcChans:%d "
                    "requestedDstChannels:%d availDstChannels:%d\n",
            trackChans, srcTotalOutChans, srcStartChan, srcChannels,
            dstStartChan,
            requestedSrcChans, availableSrcChans,
            requestedDstChannels, availDstChannels);
    #endif

  // Have we been here already during this process cycle?
  if(processed())
  {
    // Is there already some data gathered from a previous call during this process cycle?
    if(_haveData)
    {
      if(requestedSrcChans == 1 && requestedDstChannels >= 2)
      {
        const int cnt = availDstChannels > 2 ? 2 : availDstChannels;
        int c = 0;
        if(availableSrcChans >= 1)
        {
          for( ; c < cnt; ++c)
          {
            float* sp;
            if(!_prefader && srcStartChan == 0 && trackChans == 1)
              sp = outBuffersExtraMix[c];  // Use the pre-panned mono-to-stereo extra buffers.
            else
              sp = outBuffers[srcStartChan]; // In all other cases use the main buffers.
            float* dp = dstBuffer[c + dstStartChan];
            if(addArray ? addArray[c + dstStartChan] : add)
            {
              for(unsigned k = 0; k < nframes; ++k)
                *dp++ += *sp++;
            }
            else
              AL::dsp->cpy(dp, sp, nframes);
          }
        }
        // Zero the rest of the supplied buffers.
        for(i = dstStartChan + c; i < (dstStartChan + availDstChannels); ++i)
        {
          if(addArray ? addArray[i] : add)
            continue;
          if(MusEGlobal::config.useDenormalBias)
          {
            for(unsigned int q = 0; q < nframes; ++q)
              dstBuffer[i][q] = MusEGlobal::denormalBias;
          }
          else
            memset(dstBuffer[i], 0, sizeof(float) * nframes);
        }
      }
      else if(requestedSrcChans >= 2 && requestedDstChannels == 1)
      {
        const int cnt = availableSrcChans > 2 ? 2 : availableSrcChans;
        if(availDstChannels >= 1)
        {
          for(int sch = 0; sch < cnt; ++sch)
          {
            float* sp = outBuffers[srcStartChan + sch];
            float* dp = dstBuffer[dstStartChan];
            if((addArray ? addArray[dstStartChan] : add) || sch != 0)
            {
              for(unsigned k = 0; k < nframes; ++k)
                *dp++ += *sp++;
            }
            else
              AL::dsp->cpy(dp, sp, nframes);
          }
        }
        else if(addArray ? !addArray[dstStartChan] : !add)
        {
          // Zero the supplied buffer.
          if(MusEGlobal::config.useDenormalBias)
          {
            for(unsigned int q = 0; q < nframes; ++q)
              dstBuffer[dstStartChan][q] = MusEGlobal::denormalBias;
          }
          else
            memset(dstBuffer[dstStartChan], 0, sizeof(float) * nframes);
        }
      }
      else
      {
        const int cnt = availableSrcChans < availDstChannels ? availableSrcChans : availDstChannels;
        for(int c = 0; c < cnt; ++c)
        {
          float* sp = outBuffers[c + srcStartChan];
          float* dp = dstBuffer[c + dstStartChan];
          if(addArray ? addArray[c + dstStartChan] : add)
          {
            for(unsigned k = 0; k < nframes; ++k)
              *dp++ += *sp++;
          }
          else
            AL::dsp->cpy(dp, sp, nframes);
        }
        // Zero the rest of the supplied buffers.
        for(i = dstStartChan + cnt; i < (dstStartChan + availDstChannels); ++i)
        {
          if(addArray ? addArray[i] : add)
            continue;
          if(MusEGlobal::config.useDenormalBias)
          {
            for(unsigned int q = 0; q < nframes; ++q)
              dstBuffer[i][q] = MusEGlobal::denormalBias;
          }
          else
            memset(dstBuffer[i], 0, sizeof(float) * nframes);
        }
      }
    }
    else
    {
      // No data was available from a previous call during this process cycle.

      //Zero the supplied buffers and just return.
      for(i = dstStartChan; i < (dstStartChan + availDstChannels); ++i)
      {
        if(addArray ? addArray[i] : add)
          continue;
        if(MusEGlobal::config.useDenormalBias)
        {
          for(unsigned int q = 0; q < nframes; ++q)
            dstBuffer[i][q] = MusEGlobal::denormalBias;
        }
        else
          memset(dstBuffer[i], 0, sizeof(float) * nframes);
      }
    }
    return;
  }
  else
  {
    // First time here during this process cycle.

    _haveData = false;  // Reset.
    _processed = true;  // Set this now.

    // Start by clearing the meters. There may be multiple contributions to them below.
    for(i = 0; i < trackChans; ++i)
      _meter[i] = 0.0;

    if(off())
    {
      #ifdef NODE_DEBUG_PROCESS
      fprintf(stderr, "MusE: AudioTrack::copyData name:%s dstChannels:%d Off, zeroing buffers\n", name().toLatin1().constData(), availDstChannels);
      #endif

      // Track is off. Zero the supplied buffers.
      for(i = dstStartChan; i < (dstStartChan + availDstChannels); ++i)
      {
        if(addArray ? addArray[i] : add)
          continue;
        if(MusEGlobal::config.useDenormalBias)
        {
          for(unsigned int q = 0; q < nframes; ++q)
            dstBuffer[i][q] = MusEGlobal::denormalBias;
        }
        else
          memset(dstBuffer[i], 0, sizeof(float) * nframes);
      }

      _efxPipe->apply(pos, 0, nframes, 0);  // Just process controls only, not audio (do not 'run').
      processTrackCtrls(pos, 0, nframes, 0);

      //for(i = 0; i < trackChans; ++i)
      //  _meter[i] = 0.0;

      return;
    }

    // Point the input buffers at a temporary buffer.
    for(i = 0; i < srcTotalOutChans; ++i)
        buffer[i] = _dataBuffers[i];

    // getData can use the supplied buffers, or change buffer to point to its own local buffers or Jack buffers etc.
    // For ex. if this is an audio input, Jack will set the pointers for us in AudioInput::getData!
    // Don't do any processing at all if off. Whereas, mute needs to be ready for action at all times,
    //  so still call getData before it. Off is NOT meant to be toggled rapidly, but mute is !
    // Since the meters are cleared above, getData can contribute (add) to them directly and return HaveMeterDataOnly
    //  if it does not want to pass the audio for listening.
    if(!getData(pos, srcTotalOutChans, nframes, buffer))
    {
      #ifdef NODE_DEBUG_PROCESS
      fprintf(stderr, "MusE: AudioTrack::copyData name:%s srcTotalOutChans:%d zeroing buffers\n", name().toLatin1().constData(), srcTotalOutChans);
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

    // Allow it to process even if muted so that when mute is turned off, left-over buffers (reverb tails etc) can die away.
    _efxPipe->apply(pos, trackChans, nframes, buffer);

    //---------------------------------------------------
    // apply volume, pan
    //---------------------------------------------------

    processTrackCtrls(pos, trackChans, nframes, buffer);

    const int valid_out_bufs = _prefader ? 0 : trackChans;

    //---------------------------------------------------
    //    metering
    //---------------------------------------------------

    // FIXME TODO Need multichannel changes here?
    for(int c = 0; c < trackChans; ++c)
    {
      meter[c] = 0.0;
      float* sp = (c >= valid_out_bufs) ? buffer[c] : outBuffers[c]; // Optimize: Don't all valid outBuffers just for meters
      for(unsigned k = 0; k < nframes; ++k)
      {
        const double f = fabs(*sp++); // If the track is mono pan has no effect on meters.
        if(f > meter[c])
          meter[c] = f;
      }
      if(meter[c] > _meter[c])
        _meter[c] = meter[c];
      if(_meter[c] > _peak[c])
        _peak[c] = _meter[c];

      if(_meter [c] > 1.0)
         _isClipped[c] = true;
    }

// REMOVE Tim. monitor. Changed.
//    if(isMute())
    // Are both playback and input are muted?
    if(isMute() && !isRecMonitored())
    {
      // Nothing to do. Zero the supplied buffers.
      for(i = dstStartChan; i < (dstStartChan + availDstChannels); ++i)
      {
        if(addArray ? addArray[i] : add)
          continue;
        if(MusEGlobal::config.useDenormalBias)
        {
          for(unsigned int q = 0; q < nframes; q++)
            dstBuffer[i][q] = MusEGlobal::denormalBias;
        }
        else
          memset(dstBuffer[i], 0, sizeof(float) * nframes);
      }
      return; // We're outta here.
    }

    // Copy whole blocks that we can get away with here outside of the track control processing loop.
    for(i = valid_out_bufs; i < srcTotalOutChans; ++i)
      AL::dsp->cpy(outBuffers[i], buffer[i], nframes);

    // We now have some data! Set to true.
    _haveData = true;

    //---------------------------------------------------
    // aux sends
    //---------------------------------------------------

    // FIXME TODO Need multichannel changes here? Yes
    if(hasAuxSend())
    {
      AuxList* al = MusEGlobal::song->auxs();
      unsigned naux = al->size();
      for(unsigned k = 0; k < naux; ++k)
      {
        double m = _auxSend[k];
        if(m <= 0.0001)           // optimize
          continue;
        AudioAux* a = (AudioAux*)((*al)[k]);
        float** dst = a->sendBuffer();
        int auxChannels = a->channels();
        if((availableSrcChans ==1 && auxChannels==1) || availableSrcChans == 2)
        {
          for(int ch = 0; ch < availableSrcChans; ++ch)
          {
            float* db = dst[ch % a->channels()]; // no matter whether there's one or two dst buffers
            float* sb = outBuffers[ch];
            for(unsigned f = 0; f < nframes; ++f)
              *db++ += (*sb++ * m);   // add to mix
          }
        }
        else if(availableSrcChans==1 && auxChannels==2)  // copy mono to both channels
        {
          for(int ch = 0; ch < auxChannels; ++ch)
          {
            float* db = dst[ch % a->channels()];
            float* sb = outBuffers[0];
            for(unsigned f = 0; f < nframes; ++f)
              *db++ += (*sb++ * m);   // add to mix
          }
        }
      }
    }

    //---------------------------------------------------
    //    copy to destination buffers
    //---------------------------------------------------

    // FIXME TODO Need multichannel changes here?
    // Sanity check. Is source starting channel out of range? Just zero and return.
    if(srcStartChan >= srcTotalOutChans)
    {
      for(i = dstStartChan; i < (dstStartChan + availDstChannels); ++i)
      {
        if(addArray ? addArray[i] : add)
          continue;
        if(MusEGlobal::config.useDenormalBias)
        {
          for(unsigned int q = 0; q < nframes; q++)
            dstBuffer[i][q] = MusEGlobal::denormalBias;
        }
        else
          memset(dstBuffer[i], 0, sizeof(float) * nframes);
      }
      return;
    }

    if(requestedSrcChans == 1 && requestedDstChannels >= 2)
    {
      const int cnt = availDstChannels > 2 ? 2 : availDstChannels;
      int c = 0;
      if(availableSrcChans >= 1)
      {
        for( ; c < cnt; ++c)
        {
          float* sp;
          if(!_prefader && srcStartChan == 0 && trackChans == 1)
            sp = outBuffersExtraMix[c];  // Use the pre-panned mono-to-stereo extra buffers.
          else
            sp = outBuffers[srcStartChan]; // In all other cases use the main buffers.
          float* dp = dstBuffer[c + dstStartChan];
          if(addArray ? addArray[c + dstStartChan] : add)
          {
            for(unsigned k = 0; k < nframes; ++k)
              *dp++ += *sp++;
          }
          else
            AL::dsp->cpy(dp, sp, nframes);
        }
      }
      // Zero the rest of the supplied buffers.
      for(i = dstStartChan + c; i < (dstStartChan + availDstChannels); ++i)
      {
        if(addArray ? addArray[i] : add)
          continue;
        if(MusEGlobal::config.useDenormalBias)
        {
          for(unsigned int q = 0; q < nframes; ++q)
            dstBuffer[i][q] = MusEGlobal::denormalBias;
        }
        else
          memset(dstBuffer[i], 0, sizeof(float) * nframes);
      }
    }
    else if(requestedSrcChans >= 2 && requestedDstChannels == 1)
    {
      const int cnt = availableSrcChans > 2 ? 2 : availableSrcChans;
      if(availDstChannels >= 1)
      {
        for(int sch = 0; sch < cnt; ++sch)
        {
          float* sp = outBuffers[srcStartChan + sch];
          float* dp = dstBuffer[dstStartChan];
          if((addArray ? addArray[dstStartChan] : add) || sch != 0)
          {
            for(unsigned k = 0; k < nframes; ++k)
              *dp++ += *sp++;
          }
          else
            AL::dsp->cpy(dp, sp, nframes);
        }
      }
      else if(addArray ? !addArray[dstStartChan] : !add)
      {
        // Zero the supplied buffer.
        if(MusEGlobal::config.useDenormalBias)
        {
          for(unsigned int q = 0; q < nframes; ++q)
            dstBuffer[dstStartChan][q] = MusEGlobal::denormalBias;
        }
        else
          memset(dstBuffer[dstStartChan], 0, sizeof(float) * nframes);
      }
    }
    else //if(srcChans == dstChans)
    {
      const int cnt = availableSrcChans < availDstChannels ? availableSrcChans : availDstChannels;
      for(int c = 0; c < cnt; ++c)
      {
        float* sp = outBuffers[c + srcStartChan];
        float* dp = dstBuffer[c + dstStartChan];
        if(addArray ? addArray[c + dstStartChan] : add)
        {
          for(unsigned k = 0; k < nframes; ++k)
            *dp++ += *sp++;
        }
        else
          AL::dsp->cpy(dp, sp, nframes);
      }
      // Zero the rest of the supplied buffers.
      for(i = dstStartChan + cnt; i < (dstStartChan + availDstChannels); ++i)
      {
        if(addArray ? addArray[i] : add)
          continue;
        if(MusEGlobal::config.useDenormalBias)
        {
          for(unsigned int q = 0; q < nframes; ++q)
            dstBuffer[i][q] = MusEGlobal::denormalBias;
        }
        else
          memset(dstBuffer[i], 0, sizeof(float) * nframes);
      }
    }
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

//---------------------------------------------------------
//   setChannels
//---------------------------------------------------------

void Track::setChannels(int n)
      {
      if(n > MusECore::MAX_CHANNELS)
        _channels = MusECore::MAX_CHANNELS;
      else
        _channels = n;
      for (int i = 0; i < _channels; ++i) {
            _meter[i] = 0.0;
            _peak[i]  = 0.0;
            }
      }


// REMOVE Tim. latency. Removed.
// void AudioInput::setChannels(int n)
//       {
//       if (n == _channels)
//             return;
//       AudioTrack::setChannels(n);
//       }

// REMOVE Tim. latency. Removed.
// void AudioOutput::setChannels(int n)
//       {
//       if (n == _channels)
//             return;
//       AudioTrack::setChannels(n);
//       }

bool AudioTrack::isLatencyInputTerminal()
{
//   bool res = true;
  
  // REMOVE Tim. latency. Added. FLAG latency rec.
  // TODO Refine this with a case or something, specific for say Aux tracks, Group tracks etc.
//   if(!off() && ((!canRecordMonitor() || (canRecordMonitor() && isRecMonitored()))
//      //|| (canRecord() && recordFlag())
//      ))

  // If we're asking for the view from the record side, check if we're
  //  passing the signal through the track via monitoring.
  if(off() || (canRecordMonitor() && (!MusEGlobal::config.monitoringAffectsLatency || !isRecMonitored())))
     //&& canRecord() && !recordFlag())
  {
    _latencyInfo._isLatencyInputTerminal = true;
    return true;
  }
  
//   if(!off() && 
//     (!record || ((!canRecordMonitor() || (canRecordMonitor() && isRecMonitored()))
//      //|| (canRecord() && recordFlag())
//      )))
//   {
    const RouteList* rl = outRoutes();
    for (ciRoute ir = rl->begin(); ir != rl->end(); ++ir) {
      switch(ir->type)
      {
        case Route::TRACK_ROUTE:
          if(!ir->track)
            continue;
          if(ir->track->isMidiTrack())
          {
            // TODO
          }
          else
          {
            AudioTrack* atrack = static_cast<AudioTrack*>(ir->track);
//             if(!atrack->off() && 
              // REMOVE Tim. latency. Added. FLAG latency rec.
              // TODO Refine this with a case or something, specific for say Aux tracks, Group tracks etc.
//               ((!atrack->canRecordMonitor() || (atrack->canRecordMonitor() && atrack->isRecMonitored()))
//                 || (atrack->canRecord() && atrack->recordFlag()))
//               )
//             {
// //               res = false;
//               _latencyInfo._isLatencyInputTerminal = false;
//               return false;
// //               break;
//             }
            
            
            if(atrack->off()) // || 
              //(atrack->canRecordMonitor() && (MusEGlobal::config.monitoringAffectsLatency || !atrack->isRecMonitored())))
               //&& atrack->canRecord() && !atrack->recordFlag()))
              continue;
            
            _latencyInfo._isLatencyInputTerminal = false;
            return false;
          }
        break;

        default:
        break;
      }
    }
//   }

//   _latencyInfo._isLatencyInputTerminal = res;
//   return res;
  _latencyInfo._isLatencyInputTerminal = true;
  return true;
}

bool AudioTrack::isLatencyOutputTerminal()
{
//   bool res = true;
  
  // REMOVE Tim. latency. Added. FLAG latency rec.
  // TODO Refine this with a case or something, specific for say Aux tracks, Group tracks etc.
//   if(!off() && ((!canRecordMonitor() || (canRecordMonitor() && isRecMonitored()))
//      //|| (canRecord() && recordFlag())
//      ))

//   // If we're asking for the view from the record side, check if we're
//   //  passing the signal through the track via monitoring.
//   if(off() || (record && canRecordMonitor() && !isRecMonitored()))
//      //&& canRecord() && !recordFlag())
//   {
//     _latencyInfo._isLatencyOuputTerminal = true;
//     return true;
//   }
  
//   if(!off() && 
//     (!record || ((!canRecordMonitor() || (canRecordMonitor() && isRecMonitored()))
//      //|| (canRecord() && recordFlag())
//      )))
//   {
    const RouteList* rl = outRoutes();
    for (ciRoute ir = rl->begin(); ir != rl->end(); ++ir) {
      switch(ir->type)
      {
        case Route::TRACK_ROUTE:
          if(!ir->track)
            continue;
          if(ir->track->isMidiTrack())
          {
            // TODO
          }
          else
          {
            AudioTrack* atrack = static_cast<AudioTrack*>(ir->track);
//             if(!atrack->off() && 
              // REMOVE Tim. latency. Added. FLAG latency rec.
              // TODO Refine this with a case or something, specific for say Aux tracks, Group tracks etc.
//               ((!atrack->canRecordMonitor() || (atrack->canRecordMonitor() && atrack->isRecMonitored()))
//                 || (atrack->canRecord() && atrack->recordFlag()))
//               )
//             {
// //               res = false;
//               _latencyInfo._isLatencyOutputTerminal = false;
//               return false;
// //               break;
//             }
            
            if(atrack->off()) // || 
              //(atrack->canRecordMonitor() && (MusEGlobal::config.monitoringAffectsLatency || !atrack->isRecMonitored())))
               //&& atrack->canRecord() && !atrack->recordFlag()))
              continue;
            
            _latencyInfo._isLatencyOutputTerminal = false;
            return false;
          }
        break;

        default:
        break;
      }
    }
//   }

//   _latencyInfo._isLatencyOutputTerminal = res;
//   return res;
  _latencyInfo._isLatencyOutputTerminal = true;
  return true;
}

//---------------------------------------------------------
//   putFifo
//---------------------------------------------------------

// REMOVE Tim. latency. Changed.
// void AudioTrack::putFifo(int channels, unsigned long n, float** bp)
bool AudioTrack::putFifo(int channels, unsigned long n, float** bp)
{
  const RouteList* rl = inRoutes();
  float route_worst_case_latency = 0.0f;
  for (ciRoute ir = rl->begin(); ir != rl->end(); ++ir) {
        if(ir->type != Route::TRACK_ROUTE || !ir->track || ir->track->isMidiTrack())
          continue;
        AudioTrack* atrack = static_cast<AudioTrack*>(ir->track);
        //const float lat = atrack->outputLatency();
        //if(lat > route_worst_case_latency)
        //  route_worst_case_latency = lat;
        const TrackLatencyInfo& li = atrack->getLatencyInfo();
        if(li._outputLatency > route_worst_case_latency)
          route_worst_case_latency = li._outputLatency;
        }
        
  // REMOVE Tim. latency. Added. No, this would only be for playback.
  // Adjust for THIS track's contribution to latency.
  // The goal is to have equal latency output on all channels on this track.
  //const int track_out_channels = totalOutChannels();
//       const int track_out_channels = totalProcessBuffers();
//       float track_worst_case_chan_latency = 0.0f;
//       for(int i = 0; i < track_out_channels; ++i)
//       {
//         const float lat = trackLatency(i);
//         if(lat > track_worst_case_chan_latency)
//           track_worst_case_chan_latency = lat;
//       }
  
  // The absolute latency of signals leaving this track is the sum of
  //  any connected route latencies and this track's latency.
//   return route_worst_case_latency + track_worst_case_chan_latency;

  
  
  
  
  
  
// REMOVE Tim. latency. Added. Moved here and changed from WaveTrack::getData().
/*        
  unsigned int lat = 0;
  bool lat_found = false;
  
  if(!noInRoute())
  {
    //---------------------------------------------
    // Try to determine the input latency:
    //---------------------------------------------

    const RouteList* rl = inRoutes();
    bool first = true;
    for(ciRoute ir = rl->begin(); ir != rl->end(); ++ir)
    {
      const Route& r = *ir;
      if(r.type == Route::TRACK_ROUTE)
      {
        const Track* t = r.track;
        // Continue if midi track or circular route, or muted.
        if(!t || t == this || t->isMidiTrack() || t->isMute())
          continue;
        const AudioTrack* at = static_cast<const AudioTrack*>(t);
        const int chans = at->channels();
        int ch = 0;
        for( ; ch < chans; ++ch)
        {
          //const unsigned int l = (unsigned int)at->latency(ch);
          const unsigned int l = outputLatency();
          if(first)
          {
            first = false;
            lat_found = true;
            lat = l;
          }
          else if(l != lat)
          {
            lat_found = false;
            break;
          }
        }
        if(ch < chans)
          break;
      }
    }
  }

  const unsigned int fin_lat = lat_found ? lat : 0;
  
  // REMOVE Tim. latency. Added.
  if(lat_found)
    fprintf(stderr, "AudioTrack::putFifo: latency:%d\n", fin_lat);*/

  //const unsigned int fin_frame = MusEGlobal::audio->pos().frame() + fin_lat;
  const unsigned int fin_frame = MusEGlobal::audio->pos().frame();
        
        
  // REMOVE Tim. latency. Added.
//   fprintf(stderr, "AudioTrack::putFifo: latency:%f\n", route_worst_case_latency);
        
        
// REMOVE Tim. latency. Changed.
//       if (fifo.put(channels, n, bp, MusEGlobal::audio->pos().frame())) {
//             fprintf(stderr, "   overrun ???\n");
//             }
  if(fifo.put(channels, n, bp, fin_frame, route_worst_case_latency))
  {
    fprintf(stderr, "AudioTrack::putFifo: fifo overrun: frame:%d, channels:%d, nframes:%lu\n", fin_frame, channels, n);
    return false;
  }
  
  return true;
}

//---------------------------------------------------------
//   getData
//    return false if no data available
//---------------------------------------------------------

bool AudioTrack::getData(unsigned pos, int channels, unsigned nframes, float** buffer)
      {
      // use supplied buffers

// REMOVE Tim. latency. Changed.
//       RouteList* rl = inRoutes();
      const RouteList* rl = inRoutes();
      //const int rl_sz = rl->size();

      #ifdef NODE_DEBUG_PROCESS
      fprintf(stderr, "AudioTrack::getData name:%s channels:%d inRoutes:%d\n", name().toLatin1().constData(), channels, int(rl->size()));
      #endif

      int dst_ch, dst_chs, src_ch, src_chs, fin_dst_chs, next_chan, i;
      unsigned int q;
//       float fl;
      unsigned long int l;
      
      bool have_data = false;
      bool used_in_chan_array[channels];
      for(i = 0; i < channels; ++i)
        used_in_chan_array[i] = false;

// // REMOVE Tim. latency. Added.
// //       struct buf_pos_struct {
// //         int _channels;
// //         unsigned long _pos;
// //         buf_pos_struct(int channels, unsigned long pos) { _channels = channels; _pos = pos; }
// //       };
// //       int tot_chans = 0;
// //       for (ciRoute ir = rl->begin(); ir != rl->end(); ++ir) {
// //             if(ir->type != Route::TRACK_ROUTE || !ir->track || ir->track->isMidiTrack())
// //               continue;
// //             AudioTrack* atrack = static_cast<AudioTrack*>(ir->track);
// //           ++tot_chans;
// //       }
// //       int idx = 0;
//       //unsigned long latency_array[rl_sz]; // REMOVE Tim. latency. Added.
//       //int latency_array_cnt = 0;
//       unsigned long route_worst_case_latency = 0;
//       // This value has a range from 0 (worst) to positive inf (best) or close to it.
//       float route_worst_out_corr = 0.0f;
//       bool item_found = false;
//       //for (ciRoute ir = rl->begin(); ir != rl->end(); ++ir, ++latency_array_cnt) {
//       for (iRoute ir = rl->begin(); ir != rl->end(); ++ir) {
//             if(ir->type != Route::TRACK_ROUTE || !ir->track || ir->track->isMidiTrack())
//               continue;
//             AudioTrack* atrack = static_cast<AudioTrack*>(ir->track);
// //             const int atrack_out_channels = atrack->totalOutChannels();
// //             const int src_ch = ir->remoteChannel <= -1 ? 0 : ir->remoteChannel;
// //             const int src_chs = ir->channels;
// //             int fin_src_chs = src_chs;
// //             if(src_ch + fin_src_chs >  atrack_out_channels)
// //               fin_src_chs = atrack_out_channels - src_ch;
// //             const int next_src_chan = src_ch + fin_src_chs;
// //             // The goal is to have equal latency output on all channels on this track.
// //             for(int i = src_ch; i < next_src_chan; ++i)
// //             {
// //               const float lat = atrack->trackLatency(i);
// //               if(lat > worst_case_latency)
// //                 worst_case_latency = lat;
// //             }
//             //const unsigned long lat = atrack->outputLatency();
//             //latency_array[latency_array_cnt] = lat;
//             //if(lat > route_worst_case_latency)
//             //  route_worst_case_latency = lat;
//             const TrackLatencyInfo li = atrack->getLatencyInfo();
//             //latency_array[latency_array_cnt] = li._outputLatency;
//             ir->audioLatencyOut = li._outputLatency;
//             if(li._outputLatency > route_worst_case_latency)
//               route_worst_case_latency = li._outputLatency;
//             
//             if(item_found)
//             {
// //               if(li._outputAvailableCorrection > route_worst_out_corr)
//               if(li._outputAvailableCorrection < route_worst_out_corr)
//                 route_worst_out_corr = li._outputAvailableCorrection;
//             }
//             else
//             {
//               route_worst_out_corr = li._outputAvailableCorrection;
//               item_found = true;
//             }
//             
//             }
//             
//       // Adjust for THIS track's contribution to latency.
//       // The goal is to have equal latency output on all channels on this track.
//       const int track_out_channels = totalOutChannels();
//       //const int track_out_channels = totalProcessBuffers();
//       unsigned long track_worst_case_chan_latency = 0;
//       for(int i = 0; i < track_out_channels; ++i)
//       {
//         const unsigned long lat = trackLatency(i);
//         if(lat > track_worst_case_chan_latency)
//           track_worst_case_chan_latency = lat;
//       }
//       
//       //return worst_case_latency;
//       //latency_array[0] = latency_array[0]; // REMOVE. Just to compile...
//       
//       const unsigned long total_latency = route_worst_case_latency + track_worst_case_chan_latency;
      
      
//       // Now prepare the latency array to be passed to the compensator's writer,
//       //  by adjusting each route latency value. ie. the route with the worst-case
//       //  latency will get ZERO delay, while routes having smaller latency will get
//       //  MORE delay, to match all the signals' timings together.
//       for(int i = 0; i < rl_sz; ++i)
//         latency_array[i] = total_latency - latency_array[i];
      

      // REMOVE Tim. latency. Added.
      //latency_array_cnt = 0;
      // The latency info should have already been calculated so that this returns the cached values.
//       const TrackLatencyInfo li = getLatencyInfo();
      
// REMOVE Tim. latency. Changed.
//       for (ciRoute ir = rl->begin(); ir != rl->end(); ++ir) {
      //for (ciRoute ir = rl->begin(); ir != rl->end(); ++ir, ++latency_array_cnt) {
      for (ciRoute ir = rl->begin(); ir != rl->end(); ++ir) {
// REMOVE Tim. latency. Changed.
//             if(ir->track->isMidiTrack())
            if(ir->type != Route::TRACK_ROUTE || !ir->track || ir->track->isMidiTrack())
              continue;

            // Only this track knows how many destination channels there are,
            //  while only the route track knows how many source channels there are.
            // So take care of the destination channels here, and let the route track handle the source channels.
            dst_ch = ir->channel <= -1 ? 0 : ir->channel;
            if(dst_ch >= channels)
              continue;
            dst_chs = ir->channels <= -1 ? channels : ir->channels;
            src_ch = ir->remoteChannel <= -1 ? 0 : ir->remoteChannel;
            src_chs = ir->channels;

            fin_dst_chs = dst_chs;
            if(dst_ch + fin_dst_chs > channels)
              fin_dst_chs = channels - dst_ch;

            #ifdef NODE_DEBUG_PROCESS
            fprintf(stderr, "    calling copy/addData on %s dst_ch:%d dst_chs:%d fin_dst_chs:%d src_ch:%d src_chs:%d ...\n",
                    ir->track->name().toLatin1().constData(),
                    dst_ch, dst_chs, fin_dst_chs,
                    src_ch, src_chs);
            #endif

            static_cast<AudioTrack*>(ir->track)->copyData(pos,
                                                          dst_ch, dst_chs, fin_dst_chs,
                                                          src_ch, src_chs,
                                                          nframes, buffer,
// REMOVE Tim. latency. Changed.
//                                                           false, used_in_chan_array);
                                                          false);
            
            // REMOVE Tim. latency. Added.
            // Write the buffers to the latency compensator.
            // By now, each copied channel should have the same latency, 
            //  so we use this convenient common-latency version of write.
            // Also factor in the write offset required by AudioOutput tracks.
            // TODO: Make this per-channel.
            if(_latencyComp)
            {
              //_latencyComp->write(nframes, latency_array[latency_array_cnt], buffer);
              //_latencyComp->write(nframes, latency_array[latency_array_cnt] + latencyCompWriteOffset(), buffer);
              
              // Prepare the latency value to be passed to the compensator's writer,
              //  by adjusting each route latency value. ie. the route with the worst-case
              //  latency will get ZERO delay, while routes having smaller latency will get
              //  MORE delay, to match all the signal timings together.
              // The route's audioLatencyOut should have already been calculated and
              //  conveniently stored in the route.
//               fl = li._outputLatency - ir->audioLatencyOut;
//               l = fl;
//               // Should not happen, but just in case.
//               if(l < 0)
//                 l = 0;
              if((long int)ir->audioLatencyOut < 0)
                l = 0;
              else
                l = ir->audioLatencyOut;
              _latencyComp->write(nframes, l + latencyCompWriteOffset(), buffer);
            }
            
            next_chan = dst_ch + fin_dst_chs;
            for(i = dst_ch; i < next_chan; ++i)
              used_in_chan_array[i] = true;
            have_data = true;
            }

      for(i = 0; i < channels; ++i)
      {
        if(used_in_chan_array[i])
        {
          // REMOVE Tim. latency. Added.
          // Read back the latency compensated signals, using the buffers in-place.
          if(_latencyComp)
            _latencyComp->read(i, nframes, buffer[i]);
        
          continue;
        }
        // Fill unused channels with silence.
        // Channel is unused. Zero the supplied buffer.
        if(MusEGlobal::config.useDenormalBias)
        {
          for(q = 0; q < nframes; ++q)
            buffer[i][q] = MusEGlobal::denormalBias;
        }
        else
          memset(buffer[i], 0, sizeof(float) * nframes);
      }

      return have_data;
      }

//---------------------------------------------------------
//   getData
//    return true if data
//---------------------------------------------------------

bool AudioInput::getData(unsigned, int channels, unsigned nframes, float** buffer)
      {
      if (!MusEGlobal::checkAudioDevice()) return false;
      
      
// REMOVE Tim. latency. Added.
      unsigned long latency_array[channels]; // REMOVE Tim. latency. Added.
      unsigned long worst_case_latency = 0;
      for(int i = 0; i < channels; ++i) {
        // For Audio Input tracks this is the track latency (rack plugins etc) 
        //  PLUS the Jack latency, on the given channel.
        // Note that if there are multiple connections to one of our Jack ports,
        //  we ask Jack for the maximum latency, even though there's nothing we 
        //  can do about it since the data has already been mixed.
        // (Therefore always best for user to use separate ports or channels or 
        //  input tracks for each input connection.)
        const float f = trackLatency(i);
        latency_array[i] = f;
        if(f > worst_case_latency)
          worst_case_latency = f;
      }
      // Adjust the array values to arrive at forward write offsets to be passed to the latency compensator.
      for(int i = 0; i < channels; ++i)
        latency_array[i] = worst_case_latency - latency_array[i];


      
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

// REMOVE Tim. latency. Changed.
//                   if (MusEGlobal::config.useDenormalBias)
                  // Take care of the denormals now if there's no latency compensator.
                  if (!_latencyComp && MusEGlobal::config.useDenormalBias)
                  {
                      for (unsigned int i=0; i < nframes; i++)
                              buffer[ch][i] += MusEGlobal::denormalBias;
                  }
                  
                  // REMOVE Tim. latency. Added.
                  // Write the buffers to the latency compensator.
                  // They will be read back later, in-place.
                  if(_latencyComp)
                    _latencyComp->write(ch, nframes, latency_array[ch], buffer[ch]);
            }
            else
            {
                // REMOVE Tim. latency. Added.
                // Don't bother setting the denormal data here if using the latency compensator.
                if(!_latencyComp)
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
      }
      
      
      // REMOVE Tim. latency. Added.
//       if(MusEGlobal::audio->isPlaying())
//       {
//         fprintf(stderr, "AudioInput::getData() name:%s\n",
//                 name().toLatin1().constData());
//         for(int ch = 0; ch < _channels; ++ch)
//         {
//           fprintf(stderr, "channel:%d peak:", ch);
//           float val;
//           float peak = 0.0f;
//           const float* buf = buffer[ch];
//           for(unsigned int smp = 0; smp < nframes; ++smp)
//           {
//             val = buf[smp];
//             if(val > peak)
//               peak = val;
//           }
//           const int dots = peak * 20;
//           for(int d = 0; d < dots; ++d)
//             fprintf(stderr, "*");
//           fprintf(stderr, "\n");
//         }
//       }
      
      
      // REMOVE Tim. latency. Added.
      if(_latencyComp)
      {
        // Read back the latency compensated signals, using the buffers in-place.
        _latencyComp->read(nframes, buffer);
        
        if(MusEGlobal::config.useDenormalBias)
        {
          for(int ch = 0; ch < channels; ++ch)
          {
            for(unsigned int i = 0; i < nframes; i++)
              buffer[ch][i] += MusEGlobal::denormalBias;
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
      for (int i = 0; i < _channels; ++i) {
            _peak[i] = 0.0;
      }
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
      if(!canRecord())
            return;
      if (f == _recordFlag)
            return;
      _recordFlag = f;
      if (!_recordFlag)
            resetMeter();
      }

bool AudioTrack::setRecordFlag2AndCheckMonitor(bool f)
{
  if (f != _recordFlag && canRecord())
  {
    _recordFlag = f;
    if (!_recordFlag)
        resetMeter();
  }

  if(MusEGlobal::config.monitorOnRecord && canRecordMonitor())
  {
    if(f != _recMonitor)
    {
      _recMonitor = f;
      return true;
    }
  }
  return false;
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
      unsigned latency = 0;
      float* buffer[_channels];
      while(fifo.getCount()) {
// REMOVE Tim. latency. Changed.
//             if (fifo.get(_channels, MusEGlobal::segmentSize, buffer, &pos)) {
            if (fifo.get(_channels, MusEGlobal::segmentSize, buffer, &pos, &latency)) {
                  fprintf(stderr, "AudioTrack::record(): empty fifo\n");
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
                    // Tested: With the line, audio record looping sort of works, albeit with the start offset added to
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
                      fr = MusEGlobal::audio->getStartRecordPos().frame() > MusEGlobal::song->lPos().frame() ?
                            MusEGlobal::audio->getStartRecordPos().frame() : MusEGlobal::song->lPos().frame();
                    else
                    if((MusEGlobal::audio->loopCount() > 0) && (MusEGlobal::audio->getStartRecordPos().frame() > MusEGlobal::audio->loopFrame()))
                      fr = MusEGlobal::audio->loopFrame();
                    else
                      fr = MusEGlobal::audio->getStartRecordPos().frame();
                    // Now seek and write. If we are looping and punchout is on, don't let punchout point interfere with looping point.
                    if( (pos >= fr) && (!MusEGlobal::song->punchout() || (!MusEGlobal::song->loop() && pos < MusEGlobal::song->rPos().frame())) )
                    {
                      pos -= fr;
                      
// REMOVE Tim. basic latency. Added.
                      if(pos >= latency)
                      {
//                         // REMOVE Tim. latency. Added.
//                         fprintf(stderr, "AudioNode::record(): pos:%u latency:%u\n", pos, latency);
//                         for(int ch = 0; ch < _channels; ++ch)
//                         {
//                           fprintf(stderr, "channel:%d peak:", ch);
//                           float val;
//                           float peak = 0.0f;
//                           const float* buf = buffer[ch];
//                           for(unsigned int smp = 0; smp < MusEGlobal::segmentSize; ++smp)
//                           {
//                             val = buf[smp];
//                             if(val > peak)
//                               peak = val;
//                           }
//                           const int dots = peak * 20;
//                           for(int d = 0; d < dots; ++d)
//                             fprintf(stderr, "*");
//                           fprintf(stderr, "\n");
//                         }


                        pos -= latency;
                      
                        // FIXME If we are to support writing compressed file types, we probably shouldn't be seeking here. REMOVE Tim. Wave.
                        _recFile->seek(pos, 0);
                        _recFile->write(_channels, buffer, MusEGlobal::segmentSize);
                      }
                    }

                    }
              else {
                    fprintf(stderr, "AudioNode::record(): no recFile\n");
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
                  fprintf(stderr, "PANIC: processInit: no buffer from audio driver\n");
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
      fprintf(stderr, "MusE: AudioOutput::process name:%s processed:%d\n", name().toLatin1().constData(), processed());
      #endif

      for (int i = 0; i < _channels; ++i) {
            buffer1[i] = buffer[i] + offset;
      }
      copyData(pos, -1, _channels, _channels, -1, -1, n, buffer1);
      
      
      // REMOVE Tim. latency. Added.
//       if(MusEGlobal::audio->isPlaying())
//       {
//         fprintf(stderr, "AudioOutput::process() name:%s pos:%u offset:%u\n",
//                 name().toLatin1().constData(), pos, offset);
//         for(int ch = 0; ch < _channels; ++ch)
//         {
//           fprintf(stderr, "channel:%d peak:", ch);
//           float val;
//           float peak = 0.0f;
//           const float* buf = buffer1[ch];
//           for(unsigned int smp = 0; smp < n; ++smp)
//           {
//             val = buf[smp];
//             if(val > peak)
//               peak = val;
//           }
//           const int dots = peak * 20;
//           for(int d = 0; d < dots; ++d)
//             fprintf(stderr, "*");
//           fprintf(stderr, "\n");
//         }
//       }
            
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
      MusECore::MetronomeSettings* metro_settings = 
        MusEGlobal::metroUseSongSettings ? &MusEGlobal::metroSongSettings : &MusEGlobal::metroGlobalSettings;

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
      if (sendMetronome() && metro_settings->audioClickFlag && MusEGlobal::song->click()) {

            #ifdef METRONOME_DEBUG
            fprintf(stderr, "MusE: AudioOutput::processWrite Calling metronome->addData frame:%u channels:%d frames:%lu\n",
                    MusEGlobal::audio->pos().frame(), _channels, _nframes);
            #endif
            metronome->copyData(MusEGlobal::audio->pos().frame(), -1, _channels, _channels, -1, -1, _nframes, buffer, true);
            }

            MusEGlobal::wavePreview->addData(_channels, _nframes, buffer);
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

void Fifo::clear()
{
  #ifdef FIFO_DEBUG
  fprintf(stderr, "FIFO::clear count:%d\n", muse_atomic_read(&count));
  #endif

  ridx = 0;
  widx = 0;
  muse_atomic_set(&count, 0);
}

//---------------------------------------------------------
//   put
//    return true if fifo full
//---------------------------------------------------------

// REMOVE Tim. latency. Changed.
// bool Fifo::put(int segs, unsigned long samples, float** src, unsigned pos)
bool Fifo::put(int segs, unsigned long samples, float** src, unsigned pos, unsigned latency)
      {
      #ifdef FIFO_DEBUG
      fprintf(stderr, "FIFO::put segs:%d samples:%lu pos:%u count:%d\n", segs, samples, pos, muse_atomic_read(&count));
      #endif

      if (muse_atomic_read(&count) == nbuffer) {
            fprintf(stderr, "FIFO %p overrun... %d\n", this, muse_atomic_read(&count));
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
#ifdef _WIN32
            b->buffer = (float *) _aligned_malloc(16, sizeof(float *) * n);
            if(b->buffer == NULL)
            {
               fprintf(stderr, "Fifo::put could not allocate buffer segs:%d samples:%lu pos:%u\n", segs, samples, pos);
               return true;
            }
#else
            int rv = posix_memalign((void**)&(b->buffer), 16, sizeof(float) * n);
            if(rv != 0 || !b->buffer)
            {
              fprintf(stderr, "Fifo::put could not allocate buffer segs:%d samples:%lu pos:%u\n", segs, samples, pos);
              return true;
            }
#endif
            b->maxSize = n;
            }
      if(!b->buffer)
      {
        fprintf(stderr, "Fifo::put no buffer! segs:%d samples:%lu pos:%u\n", segs, samples, pos);
        return true;
      }

      b->size = samples;
      b->segs = segs;
      b->pos  = pos;
      b->latency = latency;
      for (int i = 0; i < segs; ++i)
            AL::dsp->cpy(b->buffer + i * samples, src[i], samples);
      add();
      return false;
      }

//---------------------------------------------------------
//   get
//    return true if fifo empty
//---------------------------------------------------------

// REMOVE Tim. latency. Changed.
// bool Fifo::get(int segs, unsigned long samples, float** dst, unsigned* pos)
bool Fifo::get(int segs, unsigned long samples, float** dst, unsigned* pos, unsigned* latency)
      {
      #ifdef FIFO_DEBUG
      fprintf(stderr, "FIFO::get segs:%d samples:%lu count:%d\n", segs, samples, muse_atomic_read(&count));
      #endif

      if (muse_atomic_read(&count) == 0) {
            fprintf(stderr, "FIFO %p underrun\n", this);
            return true;
            }
      FifoBuffer* b = buffer[ridx];
      if(!b->buffer)
      {
        fprintf(stderr, "Fifo::get no buffer! segs:%d samples:%lu b->pos:%u\n", segs, samples, b->pos);
        return true;
      }

      if (pos)
            *pos = b->pos;
      if (latency)
            *latency = b->latency;

      for (int i = 0; i < segs; ++i)
            dst[i] = b->buffer + samples * (i % b->segs);
      remove();
      return false;
      }

int Fifo::getCount()
      {
      return muse_atomic_read(&count);
      }

bool Fifo::isEmpty()
      {
      return muse_atomic_read(&count) == 0;
      }

//---------------------------------------------------------

//---------------------------------------------------------
//   remove
//---------------------------------------------------------

void Fifo::remove()
      {
      #ifdef FIFO_DEBUG
      fprintf(stderr, "Fifo::remove count:%d\n", muse_atomic_read(&count));
      #endif

      ridx = (ridx + 1) % nbuffer;
      muse_atomic_dec(&count);
      }

//---------------------------------------------------------
//   getWriteBuffer
//---------------------------------------------------------

bool Fifo::getWriteBuffer(int segs, unsigned long samples, float** buf, unsigned pos)
      {
      #ifdef FIFO_DEBUG
      fprintf(stderr, "Fifo::getWriteBuffer segs:%d samples:%lu pos:%u\n", segs, samples, pos);
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

            int rv = posix_memalign((void**)&(b->buffer), 16, sizeof(float) * n);
            if(rv != 0 || !b->buffer)
            {
              fprintf(stderr, "Fifo::getWriteBuffer could not allocate buffer segs:%d samples:%lu pos:%u\n", segs, samples, pos);
              return true;
            }

            b->maxSize = n;
            }
      if(!b->buffer)
      {
        fprintf(stderr, "Fifo::getWriteBuffer no buffer! segs:%d samples:%lu pos:%u\n", segs, samples, pos);
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
      #ifdef FIFO_DEBUG
      fprintf(stderr, "Fifo::add count:%d\n", muse_atomic_read(&count));
      #endif

      widx = (widx + 1) % nbuffer;
      muse_atomic_inc(&count);
      }

//---------------------------------------------------------
//   setParam
//---------------------------------------------------------

void AudioTrack::setParam(unsigned long i, double val)
{
  addScheduledControlEvent(i, val, MusEGlobal::audio->curFrame());
}

//---------------------------------------------------------
//   param
//---------------------------------------------------------

double AudioTrack::param(unsigned long i) const
{
  return _controls[i].dval;
}

//---------------------------------------------------------
//   setChannels
//---------------------------------------------------------

void AudioTrack::setChannels(int n)
      {
      Track::setChannels(n);
      if (_efxPipe)
            _efxPipe->setChannels(_channels);
      
      // REMOVE Tim. latency. Added.
      if(_latencyComp)
        _latencyComp->setChannels(totalProcessBuffers());
      }

//---------------------------------------------------------
//   setTotalOutChannels
//---------------------------------------------------------

void AudioTrack::setTotalOutChannels(int num)
{
      int chans = _totalOutChannels;
      if(num != chans)
      {
        if(_dataBuffers)
        {
          for(int i = 0; i < _totalOutChannels; ++i)
          {
            if(_dataBuffers[i])
            {
              free(_dataBuffers[i]);
              _dataBuffers[i] = NULL;
            }
          }
          delete[] _dataBuffers;
          _dataBuffers = NULL;
        }

        _totalOutChannels = num;
        int new_chans = num;
        // Number of allocated buffers is always MAX_CHANNELS or more, even if _totalOutChannels is less.
        if(new_chans < MusECore::MAX_CHANNELS)
          new_chans = MusECore::MAX_CHANNELS;
        if(chans < MusECore::MAX_CHANNELS)
          chans = MusECore::MAX_CHANNELS;
        if(new_chans != chans)
        {
          if(outBuffers)
          {
            for(int i = 0; i < chans; ++i)
            {
              if(outBuffers[i])
              {
                free(outBuffers[i]);
                outBuffers[i] = NULL;
              }
            }
            delete[] outBuffers;
            outBuffers = NULL;
          }
        }

        initBuffers();
      }
      chans = num;
      // Limit the actual track (meters, copying etc, all 'normal' operation) to two-channel stereo.
      if(chans > MusECore::MAX_CHANNELS)
        chans = MusECore::MAX_CHANNELS;
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
