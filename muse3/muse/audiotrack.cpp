//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: audiotrack.cpp,v 1.14.2.21 2009/12/20 05:00:35 terminator356 Exp $
//
//  (C) Copyright 2004 Werner Schweer (ws@seh.de)
//  (C) Copyright 2013 Tim E. Real (terminator356 on users dot sourceforge dot net)
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

#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <map>

#include <QMessageBox>

#include "globaldefs.h"
#include "track.h"
#include "event.h"
#include "song.h"
#include "audio.h"
#include "wave.h"
#include "xml.h"
#include "plugin.h"
#include "audiodev.h"
#include "synth.h"
#include "dssihost.h"
#include "vst_native.h"
#include "app.h"
#include "controlfifo.h"
#include "fastlog.h"
#include "gconfig.h"
#include "latency_compensator.h"

namespace MusECore {

bool AudioAux::_isVisible=true;
bool AudioInput::_isVisible=true;
bool AudioOutput::_isVisible=true;
bool AudioGroup::_isVisible =true;
bool WaveTrack::_isVisible=true;

// DELETETHIS 40. this caching stuff seems to be not used any more
// By T356. For caching jack in/out routing names BEFORE file save.
// Jack often shuts down during file save, causing the routes to be lost in the file.
// cacheJackRouteNames() is ONLY called from MusE::save() in app.cpp
// Update: Not required any more because the real problem was Jack RT priority, which has been fixed.
// Keep this around for now. It may come in handy if we want to preserve route names with dummy audio driver!
/*
typedef std::multimap <const int, QString> jackRouteNameMap;
std::map <const AudioTrack*, jackRouteNameMap > jackRouteNameCache;
typedef std::multimap <const int, QString>::const_iterator ciJackRouteNameMap;
typedef std::map <const AudioTrack*, jackRouteNameMap>::const_iterator ciJackRouteNameCache;
void cacheJackRouteNames()
{
    jackRouteNameCache.clear();
    const InputList* il = MusEGlobal::song->inputs();
    for(ciAudioInput iai = il->begin(); iai != il->end(); ++iai)
    {
      const RouteList* rl = (*iai)->inRoutes();
      if(!rl->empty())
      {
        jackRouteNameMap rm = jackRouteNameMap();
        for(ciRoute r = rl->begin(); r != rl->end(); ++r)
          rm.insert(std::pair<const int, QString>(r->channel, r->name()));
        jackRouteNameCache.insert(std::pair<const AudioTrack*, jackRouteNameMap>(*iai, rm));
      }
    }
    const OutputList* ol = MusEGlobal::song->outputs();
    for(ciAudioOutput iao = ol->begin(); iao != ol->end(); ++iao)
    {
      const RouteList* rl = (*iao)->outRoutes();
      if(!rl->empty())
      {
        jackRouteNameMap rm = jackRouteNameMap();
        for(ciRoute r = rl->begin(); r != rl->end(); ++r)
          rm.insert(std::pair<const int, QString>(r->channel, r->name()));
        jackRouteNameCache.insert(std::pair<const AudioTrack*, jackRouteNameMap>(*iao, rm));
      }
    }
}
*/

//---------------------------------------------------------
//   init_buffers
//---------------------------------------------------------

void AudioTrack::initBuffers()
{
  int chans = _totalOutChannels;
  // Number of allocated buffers is always MAX_CHANNELS or more, even if _totalOutChannels is less.
  if(chans < MusECore::MAX_CHANNELS)
    chans = MusECore::MAX_CHANNELS;
  if(!outBuffers)
  {
    outBuffers = new float*[chans];
    for(int i = 0; i < chans; ++i)
    {
#ifdef _WIN32
      outBuffers[i] = (float *) _aligned_malloc(16, sizeof(float) * MusEGlobal::segmentSize);
      if(outBuffers[i] == NULL)
      {
          fprintf(stderr, "ERROR: AudioTrack::init_buffers: _aligned_malloc returned error: NULL. Aborting!\n");
          abort();
      }
#else
      int rv = posix_memalign((void**)&outBuffers[i], 16, sizeof(float) * MusEGlobal::segmentSize);
      if(rv != 0)
      {
        fprintf(stderr, "ERROR: AudioTrack::init_buffers: posix_memalign returned error:%d. Aborting!\n", rv);
        abort();
      }
#endif
    }
  }
  for(int i = 0; i < chans; ++i)
  {
    if(MusEGlobal::config.useDenormalBias)
    {
      for(unsigned q = 0; q < MusEGlobal::segmentSize; ++q)
        outBuffers[i][q] = MusEGlobal::denormalBias;
    }
    else
      memset(outBuffers[i], 0, sizeof(float) * MusEGlobal::segmentSize);
  }

  if(!outBuffersExtraMix)
  {
    outBuffersExtraMix = new float*[MusECore::MAX_CHANNELS];
    for(int i = 0; i < MusECore::MAX_CHANNELS; ++i)
    {
#ifdef _WIN32
      outBuffersExtraMix[i] = (float *) _aligned_malloc(16, sizeof(float) * MusEGlobal::segmentSize);
      if(outBuffersExtraMix[i] == NULL)
      {
          fprintf(stderr, "ERROR: AudioTrack::init_buffers: _aligned_malloc outBuffersMonoMix returned error: NULL. Aborting!\n");
          abort();
      }
#else
      int rv = posix_memalign((void**)&outBuffersExtraMix[i], 16, sizeof(float) * MusEGlobal::segmentSize);
      if(rv != 0)
      {
        fprintf(stderr, "ERROR: AudioTrack::init_buffers: posix_memalign outBuffersMonoMix returned error:%d. Aborting!\n", rv);
        abort();
      }
#endif
    }
  }
  for(int i = 0; i < MusECore::MAX_CHANNELS; ++i)
  {
    if(MusEGlobal::config.useDenormalBias)
    {
      for(unsigned q = 0; q < MusEGlobal::segmentSize; ++q)
        outBuffersExtraMix[i][q] = MusEGlobal::denormalBias;
    }
    else
      memset(outBuffersExtraMix[i], 0, sizeof(float) * MusEGlobal::segmentSize);
  }

  if(!_dataBuffers)
  {
    _dataBuffers = new float*[_totalOutChannels];
    for(int i = 0; i < _totalOutChannels; ++i)
    {
#ifdef _WIN32
      _dataBuffers[i] = (float *) _aligned_malloc(16, sizeof(float) * MusEGlobal::segmentSize);
      if(_dataBuffers[i] == NULL)
      {
          fprintf(stderr, "ERROR: AudioTrack::init_buffers: _aligned_malloc _dataBuffers returned error: NULL. Aborting!\n");
          abort();
      }
#else
      int rv = posix_memalign((void**)&_dataBuffers[i], 16, sizeof(float) * MusEGlobal::segmentSize);
      if(rv != 0)
      {
        fprintf(stderr, "ERROR: AudioTrack::init_buffers: posix_memalign _dataBuffers returned error:%d. Aborting!\n", rv);
        abort();
      }
#endif
    }
  }
  for(int i = 0; i < _totalOutChannels; ++i)
  {
    if(MusEGlobal::config.useDenormalBias)
    {
      for(unsigned q = 0; q < MusEGlobal::segmentSize; ++q)
        _dataBuffers[i][q] = MusEGlobal::denormalBias;
    }
    else
      memset(_dataBuffers[i], 0, sizeof(float) * MusEGlobal::segmentSize);
  }

  // REMOVE Tim. latency. Added.
//   if(!_latencyComp)
//     //_latencyComp = new LatencyCompensator(_totalOutChannels);
//     _latencyComp = new LatencyCompensator(totalProcessBuffers());
  
  if(!audioInSilenceBuf)
  {
#ifdef _WIN32
    audioInSilenceBuf = (float *) _aligned_malloc(16, sizeof(float) * MusEGlobal::segmentSize);
    if(audioInSilenceBuf == NULL)
    {
      fprintf(stderr, "ERROR: AudioTrack::init_buffers: _aligned_malloc returned error: NULL. Aborting!\n");
      abort();
    }
#else
    int rv = posix_memalign((void**)&audioInSilenceBuf, 16, sizeof(float) * MusEGlobal::segmentSize);
    if(rv != 0)
    {
      fprintf(stderr, "ERROR: AudioTrack::init_buffers: posix_memalign returned error:%d. Aborting!\n", rv);
      abort();
    }
#endif
    if(MusEGlobal::config.useDenormalBias)
    {
      for(unsigned q = 0; q < MusEGlobal::segmentSize; ++q)
        audioInSilenceBuf[q] = MusEGlobal::denormalBias;
    }
    else
      memset(audioInSilenceBuf, 0, sizeof(float) * MusEGlobal::segmentSize);
  }

  if(!audioOutDummyBuf)
  {
#ifdef _WIN32
    audioOutDummyBuf = (float *) _aligned_malloc(16, sizeof(float) * MusEGlobal::segmentSize);
    if(audioOutDummyBuf == NULL)
    {
      fprintf(stderr, "ERROR: AudioTrack::init_buffers: _aligned_malloc returned error: NULL. Aborting!\n");
      abort();
    }
#else
    int rv = posix_memalign((void**)&audioOutDummyBuf, 16, sizeof(float) * MusEGlobal::segmentSize);
    if(rv != 0)
    {
      fprintf(stderr, "ERROR: AudioTrack::init_buffers: posix_memalign returned error:%d. Aborting!\n", rv);
      abort();
    }
#endif
    if(MusEGlobal::config.useDenormalBias)
    {
      for(unsigned q = 0; q < MusEGlobal::segmentSize; ++q)
        audioOutDummyBuf[q] = MusEGlobal::denormalBias;
    }
    else
      memset(audioOutDummyBuf, 0, sizeof(float) * MusEGlobal::segmentSize);
  }

  if(!_controls && _controlPorts != 0)
  {
    _controls = new Port[_controlPorts];
    ciCtrlList icl = _controller.begin();
    for(unsigned long k = 0; k < _controlPorts; ++k)
    {
      double val = 0.0;
      if(icl != _controller.end())
      {
        // Since the list is sorted by id, if no match is found just let k catch up to the id.
        if((unsigned long)icl->second->id() == k)
        {
          val = icl->second->getDefault();
          ++icl;
        }
      }
      _controls[k].idx    = k;
      _controls[k].dval    = val;
      _controls[k].enCtrl = true;
    }
  }
}

//---------------------------------------------------------
//   AudioTrack
//---------------------------------------------------------

AudioTrack::AudioTrack(TrackType t)
   : Track(t)
      {
       // REMOVE Tim. latency. Added.
      _totalOutChannels = MAX_CHANNELS;
      _latencyComp = new LatencyCompensator();
      
      _processed = false;
      _haveData = false;
      _sendMetronome = false;
      _prefader = false;
      _efxPipe  = new Pipeline();
      recFileNumber = 1;
      _channels = 0;
      _automationType = AUTO_OFF;
      setChannels(2);

      addController(new CtrlList(AC_VOLUME,"Volume",0.001,3.163 /* roughly 10 db */, VAL_LOG));
      addController(new CtrlList(AC_PAN, "Pan", -1.0, 1.0, VAL_LINEAR));
      addController(new CtrlList(AC_MUTE,"Mute",0.0,1.0, VAL_LINEAR, true /*don't show in arranger */));
      _controlPorts = 3;

      _curVolume = 0.0;
      _curVol1 = 0.0;
      _curVol2 = 0.0;

      _controls = 0;
      outBuffers = 0;
      outBuffersExtraMix = 0;
      audioInSilenceBuf = 0;
      audioOutDummyBuf = 0;
      _dataBuffers = 0;

// REMOVE Tim. latency. Removed. Moved above.
//       _totalOutChannels = MAX_CHANNELS;

      // This is only set by multi-channel syntis...
      _totalInChannels = 0;

      initBuffers();

      setVolume(1.0);
      _gain = 1.0;
      }

AudioTrack::AudioTrack(const AudioTrack& t, int flags)
  :  Track(t, flags)
      {
       // REMOVE Tim. latency. Added.
      _latencyComp = new LatencyCompensator();
      
      _processed      = false;
      _haveData       = false;
      _efxPipe        = new Pipeline();                 // Start off with a new pipeline.
      recFileNumber = 1;

      addController(new CtrlList(AC_VOLUME,"Volume",0.001,3.163 /* roughly 10 db */, VAL_LOG));
      addController(new CtrlList(AC_PAN, "Pan", -1.0, 1.0, VAL_LINEAR));
      addController(new CtrlList(AC_MUTE,"Mute",0.0,1.0, VAL_LINEAR, true /*don't show in arranger */));
      _controlPorts = 3;

      _curVolume = 0.0;
      _curVol1 = 0.0;
      _curVol2 = 0.0;

      // Don't allocate outBuffers here. Let internal_assign() call setTotalOutChannels to set them up.
      _controls = 0;
      outBuffers = 0;
      outBuffersExtraMix = 0;
      audioInSilenceBuf = 0;
      audioOutDummyBuf = 0;
      _dataBuffers = 0;

      _totalOutChannels = 0;

      // This is only set by multi-channel syntis...
      _totalInChannels = 0;

      _recFile = NULL;

      internal_assign(t, flags | ASSIGN_PROPERTIES);
      }

void AudioTrack::internal_assign(const Track& t, int flags)
{
      if(t.isMidiTrack())
        return;

      const AudioTrack& at = (const AudioTrack&)t;

      if(flags & ASSIGN_PROPERTIES)
      {
        _sendMetronome  = at._sendMetronome;
        _prefader       = at._prefader;
        _auxSend        = at._auxSend;
        _automationType = at._automationType;
        _gain           = at._gain;

        if(!(flags & ASSIGN_STD_CTRLS))
        {
          // Copy the standard controller block...
          ciCtrlList icl          = at._controller.begin();
          ciCtrlList icl_this     = _controller.begin();
          ciCtrlList icl_end      = at._controller.lower_bound(AC_PLUGIN_CTL_BASE);
          ciCtrlList icl_this_end = _controller.lower_bound(AC_PLUGIN_CTL_BASE);
          int id, id_this;
          CtrlList* cl, *cl_this;
          while(icl != icl_end && icl_this != icl_this_end)
          {
            cl      = icl->second;
            cl_this = icl_this->second;
            id      = cl->id();
            id_this = cl_this->id();
            if(id < id_this)
              ++icl;      // Let id catch up to this id.
            else if(id > id_this)
              ++icl_this; // Let this id catch up to id.
            else
            {
              // Match found. Copy properties but not values.
              cl_this->assign(*cl, CtrlList::ASSIGN_PROPERTIES);
              ++icl;
              ++icl_this;
            }
          }

          // Copy the special synth controller block...
          const int synth_id = (int)genACnum(MusECore::MAX_PLUGINS, 0);     // The beginning of the special synth controller block.
          const int synth_id_end = synth_id + AC_PLUGIN_CTL_BASE; // The end of the special block.
          icl           = at._controller.lower_bound(synth_id);
          icl_this      = _controller.lower_bound(synth_id);
          icl_end       = at._controller.lower_bound(synth_id_end);
          icl_this_end  = _controller.lower_bound(synth_id_end);
          while(icl != icl_end && icl_this != icl_this_end)
          {
            cl      = icl->second;
            cl_this = icl_this->second;
            id      = cl->id();
            id_this = cl_this->id();
            if(id < id_this)
              ++icl;      // Let id catch up to this id.
            else if(id > id_this)
              ++icl_this; // Let this id catch up to id.
            else
            {
              // Match found. Copy properties but not values.
              cl_this->assign(*cl, CtrlList::ASSIGN_PROPERTIES);
              ++icl;
              ++icl_this;
            }
          }
        }

        // This will set up or reallocate the outBuffers. _controlPorts must be valid by now.
        setTotalOutChannels(at._totalOutChannels);

        // This is only set by multi-channel syntis...
        setTotalInChannels(at._totalInChannels);

        // FIXME: setChannels also called in setTotalOutChannels above, causing redundant efxpipe setChannels.
        setChannels(at.channels()); // Set track channels (max 2).

        unsigned long cp = _controlPorts;
        if(at._controlPorts < cp)
          cp = at._controlPorts;
        for(unsigned long k = 0; k < cp; ++k)
          _controls[k] = at._controls[k];  // Assign the structures.
      }

      if(flags & ASSIGN_PLUGINS)
      {
        delete _efxPipe;
        _efxPipe = new Pipeline(*(at._efxPipe), this);  // Make copies of the plugins.
      }

      if(flags & (ASSIGN_STD_CTRLS | ASSIGN_PLUGIN_CTRLS))
      {
        const int synth_id = (int)genACnum(MusECore::MAX_PLUGINS, 0);     // The beginning of the special synth controller block.
        const int synth_id_end = synth_id + AC_PLUGIN_CTL_BASE; // The end of the special block.
        ciCtrlList icl, icl_end, icl_this, icl_this_end;
        int id, id_this;
        CtrlList* cl, *cl_this;

        if(flags & ASSIGN_STD_CTRLS)
        {
          // Copy the standard controller block...
          icl          = at._controller.begin();
          icl_this     = _controller.begin();
          icl_end      = at._controller.lower_bound(AC_PLUGIN_CTL_BASE);
          icl_this_end = _controller.lower_bound(AC_PLUGIN_CTL_BASE);
          while(icl != icl_end && icl_this != icl_this_end)
          {
            cl      = icl->second;
            cl_this = icl_this->second;
            id      = cl->id();
            id_this = cl_this->id();
            if(id < id_this)
              ++icl;      // Let id catch up to this id.
            else if(id > id_this)
              ++icl_this; // Let this id catch up to id.
            else
            {
              // Match found. Copy properties and values.
              cl_this->assign(*cl, CtrlList::ASSIGN_PROPERTIES | CtrlList::ASSIGN_VALUES);
              ++icl;
              ++icl_this;
            }
          }

          // Copy the special synth controller block...
          icl           = at._controller.lower_bound(synth_id);
          icl_this      = _controller.lower_bound(synth_id);
          icl_end       = at._controller.lower_bound(synth_id_end);
          icl_this_end  = _controller.lower_bound(synth_id_end);
          while(icl != icl_end && icl_this != icl_this_end)
          {
            cl      = icl->second;
            cl_this = icl_this->second;
            id      = cl->id();
            id_this = cl_this->id();
            if(id < id_this)
              ++icl;      // Let id catch up to this id.
            else if(id > id_this)
              ++icl_this; // Let this id catch up to id.
            else
            {
              // Match found. Copy properties and values.
              cl_this->assign(*cl, CtrlList::ASSIGN_PROPERTIES | CtrlList::ASSIGN_VALUES);
              ++icl;
              ++icl_this;
            }
          }
        }

        if(flags & ASSIGN_PLUGIN_CTRLS)
        {
          // Copy all plugin controller blocks...
          icl           = at._controller.lower_bound(AC_PLUGIN_CTL_BASE);
          icl_this      = _controller.lower_bound(AC_PLUGIN_CTL_BASE);
          icl_end       = at._controller.lower_bound(synth_id);
          icl_this_end  = _controller.lower_bound(synth_id);
          while(icl != icl_end && icl_this != icl_this_end)
          {
            cl      = icl->second;
            cl_this = icl_this->second;
            id      = cl->id();
            id_this = cl_this->id();
            if(id < id_this)
              ++icl;      // Let id catch up to this id.
            else if(id > id_this)
              ++icl_this; // Let this id catch up to id.
            else
            {
              // Match found. Copy properties and values.
              cl_this->assign(*cl, CtrlList::ASSIGN_PROPERTIES | CtrlList::ASSIGN_VALUES);
              ++icl;
              ++icl_this;
            }
          }
        }
      }

      if(flags & ASSIGN_ROUTES)
      {
        for(ciRoute ir = at._inRoutes.begin(); ir != at._inRoutes.end(); ++ir)
        {
          // Defer all Jack routes to Audio Input and Output copy constructors or assign !
          if(ir->type == Route::JACK_ROUTE)
            continue;
          // Don't call msgAddRoute. Caller later calls msgAddTrack which 'mirrors' this routing node.
          _inRoutes.push_back(*ir);
        }

        for(ciRoute ir = at._outRoutes.begin(); ir != at._outRoutes.end(); ++ir)
        {
          // Defer all Jack routes to Audio Input and Output copy constructors or assign !
          if(ir->type == Route::JACK_ROUTE)
            continue;
          // Don't call msgAddRoute. Caller later calls msgAddTrack which 'mirrors' this routing node.
          _outRoutes.push_back(*ir);
        }
      }
      else if(flags & ASSIGN_DEFAULT_ROUTES)
      {
        //
        //  add default route to master
        //
        OutputList* ol = MusEGlobal::song->outputs();
        if (!ol->empty()) {
              AudioOutput* ao = ol->front();
              switch(type()) {
                    case Track::WAVE:
                    case Track::AUDIO_AUX:
                          // Don't call msgAddRoute. Caller later calls msgAddTrack which 'mirrors' this routing node.
                          _outRoutes.push_back(Route(ao));
                          break;
                    // It should actually never get here now, but just in case.
                    case Track::AUDIO_SOFTSYNTH:
                          // Don't call msgAddRoute. Caller later calls msgAddTrack which 'mirrors' this routing node.
                          // Add an Omni route.
                          _outRoutes.push_back(Route(ao));
                          break;
                    default:
                          break;
                    }
              }
      }
}

void AudioTrack::assign(const Track& t, int flags)
{
      Track::assign(t, flags);
      internal_assign(t, flags);
}

AudioTrack::~AudioTrack()
{
      delete _efxPipe;

      if(audioInSilenceBuf)
        free(audioInSilenceBuf);

      if(audioOutDummyBuf)
        free(audioOutDummyBuf);

      // REMOVE Tim. latency. Added.
      if(_latencyComp)
        delete _latencyComp;
      
      if(_dataBuffers)
      {
        for(int i = 0; i < _totalOutChannels; ++i)
        {
          if(_dataBuffers[i])
            free(_dataBuffers[i]);
        }
        delete[] _dataBuffers;
      }

      if(outBuffersExtraMix)
      {
        for(int i = 0; i < MusECore::MAX_CHANNELS; ++i)
        {
          if(outBuffersExtraMix[i])
            free(outBuffersExtraMix[i]);
        }
        delete[] outBuffersExtraMix;
      }

      int chans = _totalOutChannels;
      // Number of allocated buffers is always MAX_CHANNELS or more, even if _totalOutChannels is less.
      if(chans < MusECore::MAX_CHANNELS)
        chans = MusECore::MAX_CHANNELS;
      if(outBuffers)
      {
        for(int i = 0; i < chans; ++i)
        {
          if(outBuffers[i])
            free(outBuffers[i]);
        }
        delete[] outBuffers;
      }

      if(_controls)
        delete[] _controls;

      _controller.clearDelete();
}

//---------------------------------------------------------
//   deleteAllEfxGuis
//---------------------------------------------------------

void AudioTrack::deleteAllEfxGuis()
{
  if(_efxPipe)
    _efxPipe->deleteAllGuis();
}

//---------------------------------------------------------
//   clearEfxList
//---------------------------------------------------------

void AudioTrack::clearEfxList()
{
  if(_efxPipe)
    for(int i = 0; i < MusECore::PipelineDepth; i++)
      (*_efxPipe)[i] = 0;
}

//---------------------------------------------------------
//   newPart
//---------------------------------------------------------

Part* AudioTrack::newPart(Part*, bool /*clone*/)
      {
      return 0;
      }

//---------------------------------------------------------
//   addPlugin
//---------------------------------------------------------

void AudioTrack::addPlugin(PluginI* plugin, int idx)
{
  if (plugin == 0)
  {
    PluginI* oldPlugin = (*_efxPipe)[idx];
    if (oldPlugin)
    {
      oldPlugin->setID(-1);
      oldPlugin->setTrack(0);

      int controller = oldPlugin->parameters();
      for (int i = 0; i < controller; ++i)
      {
        int id = genACnum(idx, i);
        removeController(id);
      }
    }
  }
  efxPipe()->insert(plugin, idx);
  setupPlugin(plugin, idx);
}

//---------------------------------------------------------
//   setupPlugin
//---------------------------------------------------------

void AudioTrack::setupPlugin(PluginI* plugin, int idx)
{
  if (plugin)
  {
    plugin->setID(idx);
    plugin->setTrack(this);

    int controller = plugin->parameters();
    for (int i = 0; i < controller; ++i)
    {
      int id = genACnum(idx, i);
      const char* name = plugin->paramName(i);
      float min, max;
      plugin->range(i, &min, &max);
      CtrlList* cl = new CtrlList(id);
      cl->setRange(min, max);
      cl->setName(QString(name));
      cl->setValueType(plugin->ctrlValueType(i));
      cl->setMode(plugin->ctrlMode(i));
      cl->setCurVal(plugin->param(i));
      addController(cl);
    }
  }
}

//---------------------------------------------------------
//   addAuxSend
//---------------------------------------------------------

void AudioTrack::addAuxSend(int n)
      {
      int nn = _auxSend.size();
      for (int i = nn; i < n; ++i) {
            _auxSend.push_back(0.0);
            _auxSend[i] = 0.0;  //??
            }
      }

//---------------------------------------------------------
//   addAuxSendOperation
//---------------------------------------------------------

void AudioTrack::addAuxSendOperation(int n, PendingOperationList& ops)
      {
      int nn = _auxSend.size();
      for (int i = nn; i < n; ++i)
            ops.add(PendingOperationItem(&_auxSend, 0.0, PendingOperationItem::AddAuxSendValue));
      }

//---------------------------------------------------------
//   addController
//---------------------------------------------------------

void AudioTrack::addController(CtrlList* list)
      {
      _controller.add(list);
      }

//---------------------------------------------------------
//   removeController
//---------------------------------------------------------

void AudioTrack::removeController(int id)
      {
      AudioMidiCtrlStructMap amcs;
      _controller.midiControls()->find_audio_ctrl_structs(id, &amcs);
      for(ciAudioMidiCtrlStructMap iamcs = amcs.begin(); iamcs != amcs.end(); ++ iamcs)
        _controller.midiControls()->erase(*iamcs);
      iCtrlList i = _controller.find(id);
      if (i == _controller.end()) {
            printf("AudioTrack::removeController id %d not found\n", id);
            return;
            }
      _controller.erase(i);
      }

//---------------------------------------------------------
//   swapControllerIDX
//---------------------------------------------------------

void AudioTrack::swapControllerIDX(int idx1, int idx2)
{
  if(idx1 == idx2 || idx1 < 0 || idx2 < 0 || idx1 >= MusECore::PipelineDepth || idx2 >= MusECore::PipelineDepth)
    return;

  CtrlList *cl;
  CtrlList *newcl;
  int id1 = (idx1 + 1) * AC_PLUGIN_CTL_BASE;
  int id2 = (idx2 + 1) * AC_PLUGIN_CTL_BASE;
  int id_mask = ~((int)AC_PLUGIN_CTL_ID_MASK);
  int i, j;

  CtrlListList tmpcll;
  CtrlVal cv(0, 0.0);

  for(ciCtrlList icl = _controller.begin(); icl != _controller.end(); ++icl)
  {
    cl = icl->second;
    i = cl->id() & AC_PLUGIN_CTL_ID_MASK;
    j = cl->id() & id_mask;
    if(j == id1 || j == id2)
    {
      newcl = new CtrlList(i | (j == id1 ? id2 : id1), cl->dontShow());
      newcl->setMode(cl->mode());
      newcl->setValueType(cl->valueType());
      newcl->setName(cl->name());
      double min, max;
      cl->range(&min, &max);
      newcl->setRange(min, max);
      newcl->setCurVal(cl->curVal());
      newcl->setDefault(cl->getDefault());
      newcl->setColor(cl->color());
      newcl->setVisible(cl->isVisible());
      for(iCtrl ic = cl->begin(); ic != cl->end(); ++ic)
      {
        cv = ic->second;
        newcl->insert(CtrlListInsertPair_t(cv.frame, cv));
      }
      tmpcll.insert(std::pair<const int, CtrlList*>(newcl->id(), newcl));
    }
    else
    {
      newcl = new CtrlList();
      *newcl = *cl;
      tmpcll.insert(std::pair<const int, CtrlList*>(newcl->id(), newcl));
    }
  }

  for(iCtrlList ci = _controller.begin(); ci != _controller.end(); ++ci)
    delete (*ci).second;

  _controller.clear();

  for(ciCtrlList icl = tmpcll.begin(); icl != tmpcll.end(); ++icl)
  {
    newcl = icl->second;
    _controller.insert(std::pair<const int, CtrlList*>(newcl->id(), newcl));
  }

  // Remap midi to audio controls...
  MidiAudioCtrlMap* macm = _controller.midiControls();
  for(iMidiAudioCtrlMap imacm = macm->begin(); imacm != macm->end(); ++imacm)
  {
    int actrl = imacm->second.audioCtrlId();
    int id = actrl & id_mask;
    actrl &= AC_PLUGIN_CTL_ID_MASK;
    if(id == id1)
      actrl |= id2;
    else if(id == id2)
      actrl |= id1;
    else
      continue;
    imacm->second.setAudioCtrlId(actrl);
  }
}

//---------------------------------------------------------
//   setAutomationType
//---------------------------------------------------------

void AudioTrack::setAutomationType(AutomationType t)
{
  // Clear pressed and touched and rec event list.
  clearRecAutomation(true);

  // Now set the type.
  _automationType = t;
}

//---------------------------------------------------------
//   processAutomationEvents
//---------------------------------------------------------

void AudioTrack::processAutomationEvents(Undo* operations)
{
  if(_automationType != AUTO_TOUCH && _automationType != AUTO_WRITE)
    return;

  // Use either the supplied operations list or a local one.
  Undo ops;
  Undo& opsr = operations ? (*operations) : ops;

  for(ciCtrlList icl = _controller.begin(); icl != _controller.end(); ++icl)
  {
    CtrlList* cl = icl->second;
    CtrlList& clr = *icl->second;
    int id = cl->id();

    // Were there any recorded events for this controller?
    bool do_it = false;
    for(ciCtrlRec icr = _recEvents.begin(); icr != _recEvents.end(); ++icr)
    {
      if(icr->id == id)
      {
        do_it = true;
        break;
      }
    }
    if(!do_it)
      continue;

    // The Undo system will take 'ownership' of these and delete them at the appropriate time.
    CtrlList* erased_list_items = new CtrlList(clr, CtrlList::ASSIGN_PROPERTIES);
    CtrlList* added_list_items = new CtrlList(clr, CtrlList::ASSIGN_PROPERTIES);

    // Remove old events from record region.
    if(_automationType == AUTO_WRITE)
    {
      int start = MusEGlobal::audio->getStartRecordPos().frame();
      int end   = MusEGlobal::audio->getEndRecordPos().frame();
      iCtrl   s = cl->lower_bound(start);
      iCtrl   e = cl->lower_bound(end);
      erased_list_items->insert(s, e);
    }
    else
    {  // type AUTO_TOUCH
      for(ciCtrlRec icr = _recEvents.begin(); icr != _recEvents.end(); ++icr)
      {
        // Don't bother looking for start, it's OK, just take the first one.
        // Needed for mousewheel and paging etc.
        if(icr->id != id)
          continue;

        int start = icr->frame;

        if(icr == _recEvents.end())
        {
          int end = MusEGlobal::audio->getEndRecordPos().frame();
          iCtrl s = cl->lower_bound(start);
          iCtrl e = cl->lower_bound(end);
          erased_list_items->insert(s, e);
          break;
        }

        ciCtrlRec icrlast = icr;
        ++icr;
        for(; ; ++icr)
        {
          if(icr == _recEvents.end())
          {
            int end = icrlast->frame;
            iCtrl s = cl->lower_bound(start);
            iCtrl e = cl->lower_bound(end);
            erased_list_items->insert(s, e);
            break;
          }

          if(icr->id == id && icr->type == ARVT_STOP)
          {
            int end = icr->frame;
            iCtrl s = cl->lower_bound(start);
            iCtrl e = cl->lower_bound(end);
            erased_list_items->insert(s, e);
            break;
          }

          if(icr->id == id)
            icrlast = icr;
        }
        if(icr == _recEvents.end())
              break;
      }
    }

    // Extract all recorded events for controller "id"
    //  from CtrlRecList and put into new_list.
    for(ciCtrlRec icr = _recEvents.begin(); icr != _recEvents.end(); ++icr)
    {
          if(icr->id == id)
          {
                // Must optimize these types otherwise multiple vertices appear on flat straight lines in the graphs.
                CtrlValueType vtype = cl->valueType();
                if(!cl->empty() && (cl->mode() == CtrlList::DISCRETE || vtype == VAL_BOOL || vtype == VAL_INT))
                {
                  iCtrl icl_prev = cl->lower_bound(icr->frame);
                  if(icl_prev != cl->begin())
                    --icl_prev;
                  if(icl_prev->second.val == icr->val)
                    continue;
                }
                // Now add the value.
                added_list_items->add(icr->frame, icr->val);
          }
    }

    if(erased_list_items->empty() && added_list_items->empty())
    {
      delete erased_list_items;
      delete added_list_items;
    }
    else
      opsr.push_back(UndoOp(UndoOp::ModifyAudioCtrlValList, &_controller, erased_list_items, added_list_items));
  }

  // Done with the recorded automation event list. Clear it.
  _recEvents.clear();

  if(!operations)
    MusEGlobal::song->applyOperationGroup(ops);
}

//---------------------------------------------------------
//   setControllerMode
//---------------------------------------------------------

void AudioTrack::setControllerMode(int ctlID, CtrlList::Mode m)
      {
      ciCtrlList cl = _controller.find(ctlID);
      if(cl == _controller.end())
        return;

      cl->second->setMode(m);
      }

//---------------------------------------------------------
//   clearControllerEvents
//---------------------------------------------------------

void AudioTrack::clearControllerEvents(int id)
{
  ciCtrlList icl = _controller.find(id);
  if(icl == _controller.end())
    return;

  CtrlList* cl = icl->second;
  cl->clear();
  return;
}

//---------------------------------------------------------
//   seekPrevACEvent
//---------------------------------------------------------

void AudioTrack::seekPrevACEvent(int id)
{
  ciCtrlList icl = _controller.find(id);
  if(icl == _controller.end()){
    return;
  }

    CtrlList* cl = icl->second;
    if(cl->empty())
      return;

    iCtrl s = cl->lower_bound(MusEGlobal::audio->pos().frame());
    if(s != cl->begin())
      --s;

    MusEGlobal::song->setPos(Song::CPOS, Pos(s->second.frame, false), false, true, false);
    return;
}

//---------------------------------------------------------
//   seekNextACEvent
//---------------------------------------------------------

void AudioTrack::seekNextACEvent(int id)
{
  ciCtrlList icl = _controller.find(id);
  if(icl == _controller.end()) {
    return;
  }

    CtrlList* cl = icl->second;
    if(cl->empty())
      return;

    iCtrl s = cl->upper_bound(MusEGlobal::audio->pos().frame());

    if(s == cl->end())
    {
      --s;
    }

    MusEGlobal::song->setPos(Song::CPOS, Pos(s->second.frame, false), false, true, false);
    return;
}

//---------------------------------------------------------
//   eraseACEvent
//---------------------------------------------------------

void AudioTrack::eraseACEvent(int id, int frame)
{
  ciCtrlList icl = _controller.find(id);
  if(icl == _controller.end()) {
    return;
  }


    CtrlList* cl = icl->second;
    if(cl->empty())
      return;

    iCtrl s = cl->find(frame);
    if(s != cl->end())
      cl->erase(s);
    return;
}

//---------------------------------------------------------
//   eraseRangeACEvents
//---------------------------------------------------------

void AudioTrack::eraseRangeACEvents(int id, int frame1, int frame2)
{
  ciCtrlList icl = _controller.find(id);
  if(icl == _controller.end()) {
    return;
  }

    CtrlList* cl = icl->second;
    if(cl->empty())
      return;

    iCtrl s = cl->lower_bound(frame1);
    iCtrl e = cl->lower_bound(frame2);
    cl->erase(s, e);
    return;
}

//---------------------------------------------------------
//   addACEvent
//---------------------------------------------------------

void AudioTrack::addACEvent(int id, int frame, double val)
{
  ciCtrlList icl = _controller.find(id);
  if(icl == _controller.end()) {
    return;
  }

    CtrlList* cl = icl->second;

    // Add will replace if found.
    cl->add(frame, val);
    return;
}

//---------------------------------------------------------
//   changeACEvent
//---------------------------------------------------------

void AudioTrack::changeACEvent(int id, int frame, int newframe, double newval)
{
  ciCtrlList icl = _controller.find(id);
  if(icl == _controller.end())
    return;
  CtrlList* cl = icl->second;
  iCtrl ic = cl->find(frame);
  if(ic != cl->end())
    cl->erase(ic);
  cl->insert(CtrlListInsertPair_t(newframe, CtrlVal(newframe, newval)));
}

//---------------------------------------------------------
//   trackLatency
//---------------------------------------------------------

float AudioTrack::trackLatency(int /*channel*/) const
{
  if(!_efxPipe)
    return 0.0;
  return _efxPipe->latency();
}

// REMOVE Tim. latency. Added.
//---------------------------------------------------------
//   getDominanceLatencyInfo
//---------------------------------------------------------

TrackLatencyInfo& AudioTrack::getDominanceLatencyInfo()
{
      // Have we been here before during this scan?
      // Just return the cached value.
      if(_latencyInfo._dominanceProcessed)
        return _latencyInfo;
      
      RouteList* rl = inRoutes();
      //const RouteList* rl = inRoutes();
      float route_worst_latency = 0.0f;
      float track_worst_chan_latency = 0.0f;
      
      // This value has a range from 0 (worst) to positive inf (best) or close to it.
//       float route_worst_out_corr = outputLatencyCorrection();
      // Get the default domination for this track type.
      bool can_dominate_out_lat = canDominateOutputLatency();
      // Get the default correction ability for this track type.
      //bool can_correct_out_lat = canCorrectOutputLatency();

      // Gather latency info from all connected input branches,
      //  but ONLY if the track is not off.
      if(!off())
      {
        bool item_found = false;
        // Only if monitoring is not available, or it is and in fact is monitored.
        if(!canRecordMonitor() || (canRecordMonitor() && isRecMonitored()))
        {
          for (iRoute ir = rl->begin(); ir != rl->end(); ++ir) {
                if(ir->type != Route::TRACK_ROUTE || !ir->track || ir->track->isMidiTrack())
                  continue;
                AudioTrack* atrack = static_cast<AudioTrack*>(ir->track);
    //             const int atrack_out_channels = atrack->totalOutChannels();
    //             const int src_ch = ir->remoteChannel <= -1 ? 0 : ir->remoteChannel;
    //             const int src_chs = ir->channels;
    //             int fin_src_chs = src_chs;
    //             if(src_ch + fin_src_chs >  atrack_out_channels)
    //               fin_src_chs = atrack_out_channels - src_ch;
    //             const int next_src_chan = src_ch + fin_src_chs;
    //             // The goal is to have equal latency output on all channels on this track.
    //             for(int i = src_ch; i < next_src_chan; ++i)
    //             {
    //               const float lat = atrack->trackLatency(i);
    //               if(lat > worst_case_latency)
    //                 worst_case_latency = lat;
    //             }
                const TrackLatencyInfo& li = atrack->getDominanceLatencyInfo();
                
                // Temporarily store these values conveniently in the actual route.
                // They will be used by the latency compensator in the audio process pass.
                ir->canDominateLatency = li._canDominateOutputLatency;
    //             ir->audioLatencyOut = li._outputLatency;
                ir->canCorrectOutputLatency = li._canCorrectOutputLatency;
                
    //             // Override the current worst value if the latency is greater,
    //             //  but ONLY if the branch can dominate.
    //             if(li._canDominateOutputLatency && li._outputLatency > route_worst_latency)
    //               route_worst_latency = li._outputLatency;
                
                // Is it the first found item?
                if(item_found)
                {
                  // Override the current values with this item's values ONLY if required.
                  
                  //if(li._outputAvailableCorrection < route_worst_out_corr)
                  //  route_worst_out_corr = li._outputAvailableCorrection;
                  
                  // If any one of the branches can dominate the latency,
                  //  that overrides any which cannot.
                  if(li._canDominateOutputLatency)
                  {
                    can_dominate_out_lat = true;
                    // Override the current worst value if the latency is greater,
                    //  but ONLY if the branch can dominate.
                    if(li._outputLatency > route_worst_latency)
                      route_worst_latency = li._outputLatency;
                  }
                }
                else
                {
                  item_found = true;
                  // Override the defaults with this first item's values.
                  //route_worst_out_corr = li._outputAvailableCorrection;
                  can_dominate_out_lat = li._canDominateOutputLatency;
                  // Override the default worst value, but ONLY if the branch can dominate.
                  if(can_dominate_out_lat)
                    route_worst_latency = li._outputLatency;
                }
          }
        }
        
  //       // Now that we know the worst-case latency of the connected branches,
  //       //  adjust each of the conveniently stored temporary latency values
  //       //  in the routes according to whether they can dominate...
  //       item_found = false;
  //       for (iRoute ir = rl->begin(); ir != rl->end(); ++ir) {
  //             if(ir->type != Route::TRACK_ROUTE || !ir->track || ir->track->isMidiTrack())
  //               continue;
  //             
  //             // If the branch cannot dominate the latency, force it to be
  //             //  equal to the worst-case value.
  //             if(!ir->canDominateLatency)
  //               ir->audioLatencyOut = route_worst_latency;
  //       }
              
        // Adjust for THIS track's contribution to latency.
        // The goal is to have equal latency output on all channels on this track.
        const int track_out_channels = totalProcessBuffers(); // totalOutChannels();
        for(int i = 0; i < track_out_channels; ++i)
        {
          const float lat = trackLatency(i);
          if(lat > track_worst_chan_latency)
              track_worst_chan_latency = lat;
        }
      }
      
      // The absolute latency of signals leaving this track is the sum of
      //  any connected route latencies and this track's latency.
      _latencyInfo._trackLatency  = track_worst_chan_latency;
      _latencyInfo._outputLatency = track_worst_chan_latency + route_worst_latency;
      //_latencyInfo._outputAvailableCorrection = route_worst_out_corr;
      _latencyInfo._canDominateOutputLatency = can_dominate_out_lat;
      //_latencyInfo._canCorrectOutputLatency = can_correct_out_lat;
      _latencyInfo._canCorrectOutputLatency = canCorrectOutputLatency();
      // Take advantage of this first stage to initialize the track's
      //  correction value to zero.
      _latencyInfo._sourceCorrectionValue = 0.0f;
      // Take advantage of this first stage to initialize the track's
      //  write offset to zero.
      _latencyInfo._compensatorWriteOffset = 0;
      // Set whether this track is a branch end point.
      //_latencyInfo._isLatencyOuputTerminal = isLatencyOutputTerminal();

      _latencyInfo._dominanceProcessed = true;
      return _latencyInfo;
}

//---------------------------------------------------------
//   setCorrectionLatencyInfo
//---------------------------------------------------------

void AudioTrack::setCorrectionLatencyInfo(float finalWorstLatency, float callerBranchLatency)
{
      // Have we been here before during this scan?
      // Just return the cached value.
      if(_latencyInfo._correctionProcessed)
        return;
      
      // The _trackLatency should already be calculated in the dominance scan.
      const float track_lat = callerBranchLatency + _latencyInfo._trackLatency;
      // Set the correction of all connected input branches,
      //  but ONLY if the track is not off.
      if(!off())
      {
        // Only if monitoring is not available, or it is and in fact is monitored.
        if(!canRecordMonitor() || (canRecordMonitor() && isRecMonitored()))
        {
          const RouteList* rl = inRoutes();
          for (ciRoute ir = rl->begin(); ir != rl->end(); ++ir) {
                if(ir->type != Route::TRACK_ROUTE || !ir->track || ir->track->isMidiTrack())
                  continue;
                AudioTrack* atrack = static_cast<AudioTrack*>(ir->track);
                atrack->setCorrectionLatencyInfo(finalWorstLatency, track_lat);
          }
        }
      }
      
      _latencyInfo._correctionProcessed = true;
}

//---------------------------------------------------------
//   getLatencyInfo
//---------------------------------------------------------

TrackLatencyInfo& AudioTrack::getLatencyInfo()
{
      // Have we been here before during this scan?
      // Just return the cached value.
      if(_latencyInfo._processed)
        return _latencyInfo;
      
      RouteList* rl = inRoutes();
      //const RouteList* rl = inRoutes();
      float route_worst_latency = 0.0f;
      //float track_worst_chan_latency = 0.0f;
      
      // This value has a range from 0 (worst) to positive inf (best) or close to it.
      //float route_worst_out_corr = outputLatencyCorrection();
      // Get the default domination for this track type.
//       bool can_dominate_out_lat = canDominateOutputLatency();
      
      // Gather latency info from all connected input branches,
      //  but ONLY if the track is not off.
      if(!off())
      {
        bool item_found = false;
        for (iRoute ir = rl->begin(); ir != rl->end(); ++ir) {
              if(ir->type != Route::TRACK_ROUTE || !ir->track || ir->track->isMidiTrack())
                continue;
              AudioTrack* atrack = static_cast<AudioTrack*>(ir->track);
  //             const int atrack_out_channels = atrack->totalOutChannels();
  //             const int src_ch = ir->remoteChannel <= -1 ? 0 : ir->remoteChannel;
  //             const int src_chs = ir->channels;
  //             int fin_src_chs = src_chs;
  //             if(src_ch + fin_src_chs >  atrack_out_channels)
  //               fin_src_chs = atrack_out_channels - src_ch;
  //             const int next_src_chan = src_ch + fin_src_chs;
  //             // The goal is to have equal latency output on all channels on this track.
  //             for(int i = src_ch; i < next_src_chan; ++i)
  //             {
  //               const float lat = atrack->trackLatency(i);
  //               if(lat > worst_case_latency)
  //                 worst_case_latency = lat;
  //             }
              const TrackLatencyInfo& li = atrack->getLatencyInfo();
              
              // Temporarily store these values conveniently in the actual route.
              // They will be used by the latency compensator in the audio process pass.
//               ir->canDominateLatency = li._canDominateOutputLatency;
              //ir->canCorrectOutputLatency = li._canCorrectOutputLatency;
              ir->audioLatencyOut = li._outputLatency;
              
  //             // Override the current worst value if the latency is greater,
  //             //  but ONLY if the branch can dominate.
  //             if(li._canDominateOutputLatency && li._outputLatency > route_worst_latency)
  //               route_worst_latency = li._outputLatency;
  //             // Override the current worst value if the latency is greater.
  //             if(li._outputLatency > route_worst_latency)
  //               route_worst_latency = li._outputLatency;
              
              // Is it the first found item?
              if(item_found)
              {
                // Override the current values with this item's values ONLY if required.
                
                //if(li._outputAvailableCorrection < route_worst_out_corr)
                //  route_worst_out_corr = li._outputAvailableCorrection;
                
                // If any one of the branches can dominate the latency,
                //  that overrides any which cannot.
//                 if(li._canDominateOutputLatency)
//                 {
//                   can_dominate_out_lat = true;
                  // Override the current worst value if the latency is greater,
                  //  but ONLY if the branch can dominate.
                  if(li._outputLatency > route_worst_latency)
                    route_worst_latency = li._outputLatency;
//                 }
              }
              else
              {
                item_found = true;
                // Override the defaults with this first item's values.
                //route_worst_out_corr = li._outputAvailableCorrection;
//                 can_dominate_out_lat = li._canDominateOutputLatency;
                // Override the default worst value, but ONLY if the branch can dominate.
//                 if(can_dominate_out_lat)
                  route_worst_latency = li._outputLatency;
              }
        }
        
//         // Adjust for THIS track's contribution to latency.
//         // The goal is to have equal latency output on all channels on this track.
//         const int track_out_channels = totalProcessBuffers(); // totalOutChannels();
//         for(int i = 0; i < track_out_channels; ++i)
//         {
//           const float lat = trackLatency(i);
//           if(lat > track_worst_chan_latency)
//               track_worst_chan_latency = lat;
//         }
        
      
  //       // Now add the track's own correction value, if any.
  //       // The correction value is NEGATIVE, so simple summation is used.
  //       route_worst_latency += _latencyInfo._sourceCorrectionValue;

  //       // Override the current worst value if the track's own correction value is greater.
  //       // Note that the correction value is always NEGATIVE.
  //       if(_latencyInfo._sourceCorrectionValue > route_worst_latency)
  //         route_worst_latency = _latencyInfo._sourceCorrectionValue;
        
        // Now that we know the worst-case latency of the connected branches,
        //  adjust each of the conveniently stored temporary latency values
        //  in the routes according to whether they can dominate...
        for (iRoute ir = rl->begin(); ir != rl->end(); ++ir) {
              if(ir->type != Route::TRACK_ROUTE || !ir->track || ir->track->isMidiTrack())
                continue;
              
              // If the branch cannot dominate the latency, force it to be
              //  equal to the worst-case value.
              //if(!ir->canDominateLatency)
              // If the branch cannot correct the latency, force it to be
              //  equal to the worst-case value.
//               if(!ir->canCorrectOutputLatency)
//                 ir->audioLatencyOut = route_worst_latency;
              
              
              // Prepare the latency value to be passed to the compensator's writer,
              //  by adjusting each route latency value. ie. the route with the worst-case
              //  latency will get ZERO delay, while routes having smaller latency will get
              //  MORE delay, to match all the signal timings together.
              // The route's audioLatencyOut should have already been calculated and
              //  conveniently stored in the route.
              ir->audioLatencyOut = route_worst_latency - ir->audioLatencyOut;
              // Should not happen, but just in case.
              if((long int)ir->audioLatencyOut < 0)
                ir->audioLatencyOut = 0.0f;
        }
            
      }
      
      // The absolute latency of signals leaving this track is the sum of
      //  any connected route latencies and this track's latency.
//       _latencyInfo._trackLatency  = track_worst_chan_latency;
//       _latencyInfo._outputLatency = track_worst_chan_latency + route_worst_latency;
      // The _trackLatency should have been already calculated from the dominance scan.
      _latencyInfo._outputLatency = _latencyInfo._trackLatency + route_worst_latency;
      //_latencyInfo._outputAvailableCorrection = route_worst_out_corr;
//       _latencyInfo._canDominateOutputLatency = can_dominate_out_lat;

      _latencyInfo._processed = true;
      return _latencyInfo;
}

// REMOVE Tim. latency. Added.
// //---------------------------------------------------------
// //   getForwardLatencyInfo
// //---------------------------------------------------------
// 
// TrackLatencyInfo& AudioTrack::getForwardLatencyInfo()
// {
//       // Has the normal reverse latency been processed yet?
//       // We need some of the info from the reverse scanning.
//       // If all goes well, all nodes should be reverse-processed by now.
//       // But in case this one hasn't do it now, starting from this node backwards.
//       //getLatencyInfo();
// 
//       // Have we been here before during this forward scan in this process cycle?
//       // Just return the cached value.
//       if(_latencyInfo._forwardProcessed)
//         return _latencyInfo;
//       
//       const RouteList* rl = outRoutes();
//       float route_worst_latency = 0.0f;
//       float rev_route_worst_latency = 0.0f;
//       
// //       // This value has a range from 0 (worst) to negative inf (best) or close to it.
//       // This value has a range from 0 (worst) to positive inf (best) or close to it.
//       //float route_worst_out_corr = 0.0f;
//       float route_worst_out_corr = outputLatencyCorrection();
//       // This value has a range from 0 (worst) to positive inf (best) or close to it.
//       float rev_route_worst_out_corr = outputLatencyCorrection();
//       // Get the default correction flag for this track type.
//       bool req_correct_in_lat = requiresInputLatencyCorrection();
//       
//       bool item_found = false;
//       //bool rev_item_found = false;
//       for (ciRoute o_r = rl->begin(); o_r != rl->end(); ++o_r) {
//             if(o_r->type != Route::TRACK_ROUTE || !o_r->track || o_r->track->isMidiTrack())
//               continue;
//             AudioTrack* atrack = static_cast<AudioTrack*>(o_r->track);
//             const TrackLatencyInfo& fwd_li = atrack->getForwardLatencyInfo();
//             // This should not cost anything - it should already be cached from the reverse scan.
//             const TrackLatencyInfo& rev_li = atrack->getLatencyInfo();
//             
//             if(fwd_li._forwardOutputLatency > route_worst_latency)
//               route_worst_latency = fwd_li._forwardOutputLatency;
//             if(rev_li._outputLatency > rev_route_worst_latency)
//               rev_route_worst_latency = rev_li._outputLatency;
//             
//             if(item_found)
//             {
//               // Override the current values with this item's values ONLY if required.
//               
//               if(fwd_li._outputAvailableCorrection < route_worst_out_corr)
//                 route_worst_out_corr = fwd_li._outputAvailableCorrection;
//               
//               if(rev_li._outputAvailableCorrection < rev_route_worst_out_corr)
//                 rev_route_worst_out_corr = rev_li._outputAvailableCorrection;
//               
//               // If any one of the branches require latency correction,
//               //  that overrides any which do not.
//               if(fwd_li._requiresInputCorrection)
//                 req_correct_in_lat = true;
//             }
//             else
//             {
//               // Override the defaults with this first item's values.
//               route_worst_out_corr = fwd_li._outputAvailableCorrection;
//               rev_route_worst_out_corr = rev_li._outputAvailableCorrection;
//               req_correct_in_lat = fwd_li._requiresInputCorrection;
//               item_found = true;
//             }
//             
// //             // This value has a range from 0 (worst) to positive inf (best) or close to it.
// //             float i_route_worst_out_corr = 0.0f;
// //             bool i_item_found = false;
// //             const RouteList* irl = atrack->inRoutes();
// //             for (ciRoute i_r = irl->begin(); i_r != irl->end(); ++i_r) {
// //                   if(i_r->type != Route::TRACK_ROUTE || !i_r->track || i_r->track->isMidiTrack())
// //                     continue;
// //                   AudioTrack* ir_track = static_cast<AudioTrack*>(i_r->track);
// //                   TrackLatencyInfo ir_li = ir_track->getLatencyInfo();
// //                   if(i_item_found)
// //                   {
// //                     //if(ir_li._outputAvailableCorrection > i_route_worst_out_corr)
// //                     if(ir_li._outputAvailableCorrection < route_worst_out_corr)
// //                       i_route_worst_out_corr = ir_li._outputAvailableCorrection;
// //                   }
// //                   else
// //                   {
// //                     i_route_worst_out_corr = ir_li._outputAvailableCorrection;
// //                     i_item_found = true;
// //                   }
// //             }
// 
//       }
//             
// //       // Adjust for THIS track's contribution to latency.
// //       // The goal is to have equal latency output on all channels on this track.
// //       const int track_out_channels = totalProcessBuffers(); // totalOutChannels();
// //       float track_worst_chan_latency = 0.0f;
// //       for(int i = 0; i < track_out_channels; ++i)
// //       {
// //         const float lat = trackLatency(i);
// //         if(lat > track_worst_chan_latency)
// //             track_worst_chan_latency = lat;
// //       }
//       
// //       // The absolute latency of signals leaving this track is the sum of
// //       //  any connected route latencies and this track's latency.
// //       _latencyInfo._forwardTrackLatency  = track_worst_chan_latency;
// //       _latencyInfo._forwardOutputLatency = track_worst_chan_latency + route_worst_latency;
//       _latencyInfo._forwardOutputLatency = rev_route_worst_latency;
// //       _latencyInfo._forwardOutputAvailableCorrection = route_worst_out_corr;
//       _latencyInfo._requiresInputCorrection = req_correct_in_lat;
// 
//       _latencyInfo._forwardProcessed = true;
//       return _latencyInfo;
// }

void AudioTrack::setLatencyCompWriteOffset(float worstCase)
{
  const long unsigned int wc = worstCase;
  const long unsigned int ol = _latencyInfo._outputLatency;
  if(ol > wc)
    _latencyInfo._compensatorWriteOffset = 0;
  else
    _latencyInfo._compensatorWriteOffset = wc - ol;
}

//---------------------------------------------------------
//   volume
//---------------------------------------------------------

double AudioTrack::volume() const
      {
      return _controller.value(AC_VOLUME, MusEGlobal::audio->curFramePos(),
                               !MusEGlobal::automation || automationType() == AUTO_OFF || !_controls[AC_VOLUME].enCtrl);
      }

//---------------------------------------------------------
//   setVolume
//---------------------------------------------------------

void AudioTrack::setVolume(double val)
      {
      iCtrlList cl = _controller.find(AC_VOLUME);
      if (cl == _controller.end()) {
            printf("no volume controller %s %zd\n",
               name().toLatin1().constData(), _controller.size());
            return;
            }
      cl->second->setCurVal(val);
      }

//---------------------------------------------------------
//   pan
//---------------------------------------------------------

double AudioTrack::pan() const
      {
      return _controller.value(AC_PAN, MusEGlobal::audio->curFramePos(),
                               !MusEGlobal::automation || automationType() == AUTO_OFF || !_controls[AC_PAN].enCtrl);
      }

//---------------------------------------------------------
//   setPan
//---------------------------------------------------------

void AudioTrack::setPan(double val)
      {
      iCtrlList cl = _controller.find(AC_PAN);
      if (cl == _controller.end()) {
            printf("no pan controller\n");
            return;
            }
      cl->second->setCurVal(val);
      }

//---------------------------------------------------------
//   pan
//---------------------------------------------------------

double AudioTrack::gain() const
      {
        return _gain;
      }

//---------------------------------------------------------
//   setPan
//---------------------------------------------------------

void AudioTrack::setGain(double val)
      {
        _gain = val;
      }

//---------------------------------------------------------
//   pluginCtrlVal
//---------------------------------------------------------

double AudioTrack::pluginCtrlVal(int ctlID) const
      {
      bool en = true;
      if(ctlID < AC_PLUGIN_CTL_BASE)
      {
        if((unsigned long)ctlID < _controlPorts)
          en = _controls[ctlID].enCtrl;
      }
      else
      {
        if(ctlID < (int)genACnum(MusECore::MAX_PLUGINS, 0))  // The beginning of the special synth controller block.
        {
          en = _efxPipe->controllerEnabled(ctlID);
        }
        else
        {
          if(type() == AUDIO_SOFTSYNTH)
          {
            const SynthI* synth = static_cast<const SynthI*>(this);
            const SynthIF* sif = synth->sif();
            if(sif)
            {
              int in_ctrl_idx = ctlID & AC_PLUGIN_CTL_ID_MASK;
              en = sif->controllerEnabled(in_ctrl_idx);
            }
          }
        }
      }

      return _controller.value(ctlID, MusEGlobal::audio->curFramePos(),
                               !MusEGlobal::automation || automationType() == AUTO_OFF || !en);
      }

//---------------------------------------------------------
//   setPluginCtrlVal
//---------------------------------------------------------

void AudioTrack::setPluginCtrlVal(int param, double val)
{
  iCtrlList cl = _controller.find(param);
  if (cl == _controller.end())
    return;

  cl->second->setCurVal(val);
}

//---------------------------------------------------------
//   addScheduledControlEvent
//   returns true if event cannot be delivered
//---------------------------------------------------------

bool AudioTrack::addScheduledControlEvent(int track_ctrl_id, double val, unsigned frame)
{
  if(track_ctrl_id < AC_PLUGIN_CTL_BASE)
  {
    // Send these controllers directly to the track's own FIFO.
    ControlEvent ce;
    ce.unique = false;
    ce.fromGui = false;
    ce.idx = track_ctrl_id;
    ce.value = val;
    // Time-stamp the event. timestamp() is circular, which is making it impossible to deal with 'modulo' events which
    //  slip in 'under the wire' before processing the ring buffers. So try this linear timestamp instead:
    ce.frame = frame;
    if(_controlFifo.put(ce))
    {
      fprintf(stderr, "AudioTrack::addScheduledControlEvent: fifo overflow: in control number:%d\n", track_ctrl_id);
      return true;
    }
    return false;
  }
  else
  {
    if(track_ctrl_id < (int)genACnum(MusECore::MAX_PLUGINS, 0))  // The beginning of the special synth controller block.
      return _efxPipe->addScheduledControlEvent(track_ctrl_id, val, frame);
    else
    {
      if(type() == AUDIO_SOFTSYNTH)
      {
        const SynthI* synth = static_cast<const SynthI*>(this);
        SynthIF* sif = synth->sif();
        if(sif)
        {
          int in_ctrl_idx = track_ctrl_id & AC_PLUGIN_CTL_ID_MASK;
          return sif->addScheduledControlEvent(in_ctrl_idx, val, frame);
        }
      }
    }
  }
  return true;
}

//---------------------------------------------------------
//   enableController
//   Enable or disable gui controls.
//   Used during automation recording to inhibit gui controls
//    from playback controller stream
//---------------------------------------------------------

void AudioTrack::enableController(int track_ctrl_id, bool en)
{
  if(track_ctrl_id < AC_PLUGIN_CTL_BASE)
  {
    if((unsigned long)track_ctrl_id < _controlPorts)
      _controls[track_ctrl_id].enCtrl = en;
  }
  else
  {
    if(track_ctrl_id < (int)genACnum(MusECore::MAX_PLUGINS, 0))  // The beginning of the special synth controller block.
      _efxPipe->enableController(track_ctrl_id, en);
    else
    {
      if(type() == AUDIO_SOFTSYNTH)
      {
        const SynthI* synth = static_cast<const SynthI*>(this);
        SynthIF* sif = synth->sif();
        if(sif)
        {
          int in_ctrl_idx = track_ctrl_id & AC_PLUGIN_CTL_ID_MASK;
          sif->enableController(in_ctrl_idx, en);
        }
      }
    }
  }
}

//---------------------------------------------------------
//   controllerEnabled
//---------------------------------------------------------

bool AudioTrack::controllerEnabled(int track_ctrl_id) const
      {
      if(track_ctrl_id < AC_PLUGIN_CTL_BASE)
      {
        if((unsigned long)track_ctrl_id < _controlPorts)
          return _controls[track_ctrl_id].enCtrl;
        return false;
      }
      else
      {
        if(track_ctrl_id < (int)genACnum(MusECore::MAX_PLUGINS, 0))  // The beginning of the special synth controller block.
        {
          return _efxPipe->controllerEnabled(track_ctrl_id);
        }
        else
        {
          if(type() == AUDIO_SOFTSYNTH)
          {
            const SynthI* synth = static_cast<const SynthI*>(this);
            const SynthIF* sif = synth->sif();
            if(sif)
            {
              int in_ctrl_idx = track_ctrl_id & AC_PLUGIN_CTL_ID_MASK;
              return sif->controllerEnabled(in_ctrl_idx);
            }
          }
        }
      }
      return false;
      }

//---------------------------------------------------------
//   enableAllControllers
//   Enable all track and plugin controllers, and synth controllers if applicable.
//---------------------------------------------------------

void AudioTrack::enableAllControllers()
{
    // Enable track controllers:
    for(unsigned long i = 0; i < _controlPorts; ++i)
      _controls[i].enCtrl = true;

    // Enable plugin controllers:
    Pipeline *pl = efxPipe();
    PluginI *p;
    for(iPluginI i = pl->begin(); i != pl->end(); ++i)
    {
      p = *i;
      if(!p)
        continue;
      p->enableAllControllers(true);
    }

    // Enable synth controllers:
    if(type() == AUDIO_SOFTSYNTH)
    {
      const SynthI* synth = static_cast<const SynthI*>(this);
      SynthIF* sif = synth->sif();
      if(sif)
        sif->enableAllControllers(true);
    }
}

void AudioTrack::recordAutomation(int n, double v)
      {
        if(!MusEGlobal::automation)
          return;
        if(MusEGlobal::audio->isPlaying())
          _recEvents.push_back(CtrlRecVal(MusEGlobal::audio->curFramePos(), n, v));
        else
        {
          if(automationType() == AUTO_WRITE)
            _recEvents.push_back(CtrlRecVal(MusEGlobal::audio->curFramePos(), n, v));
          else
          if(automationType() == AUTO_TOUCH)
          // In touch mode and not playing. Send directly to controller list.
          {
            iCtrlList cl = _controller.find(n);
            if (cl == _controller.end())
              return;
            // Add will replace if found.
            cl->second->add(MusEGlobal::audio->curFramePos(), v);
          }
        }
      }

void AudioTrack::startAutoRecord(int n, double v)
      {
        if(!MusEGlobal::automation)
          return;
        if(MusEGlobal::audio->isPlaying())
        {
          if(automationType() == AUTO_TOUCH)
              _recEvents.push_back(CtrlRecVal(MusEGlobal::audio->curFramePos(), n, v, ARVT_START));
          else
          if(automationType() == AUTO_WRITE)
              _recEvents.push_back(CtrlRecVal(MusEGlobal::audio->curFramePos(), n, v));
        }
        else
        {
          if(automationType() == AUTO_TOUCH)
          // In touch mode and not playing. Send directly to controller list.
          {
            // FIXME: Unsafe? Should sync by sending a message, but that'll really slow it down with large audio bufs.
            iCtrlList cl = _controller.find(n);
            if (cl == _controller.end())
              return;
            // Add will replace if found.
            cl->second->add(MusEGlobal::audio->curFramePos(), v);
          }
          else
          if(automationType() == AUTO_WRITE)
            _recEvents.push_back(CtrlRecVal(MusEGlobal::audio->curFramePos(), n, v));
        }
      }

void AudioTrack::stopAutoRecord(int n, double v)
      {
        if(!MusEGlobal::automation)
          return;
        if(MusEGlobal::audio->isPlaying())
        {
          if(automationType() == AUTO_TOUCH)
          {
              MusEGlobal::song->applyOperation(UndoOp(UndoOp::AddAudioCtrlVal,
                             this, n, MusEGlobal::audio->curFramePos(), v));
              _recEvents.push_back(CtrlRecVal(MusEGlobal::audio->curFramePos(), n, v, ARVT_STOP));
          }
        }
      }

//---------------------------------------------------------
//   AudioTrack::writeProperties
//---------------------------------------------------------

void AudioTrack::writeProperties(int level, Xml& xml) const
      {
      Track::writeProperties(level, xml);
      xml.intTag(level, "prefader", prefader());
      xml.intTag(level, "sendMetronome", sendMetronome());
      xml.intTag(level, "automation", int(automationType()));
      xml.doubleTag(level, "gain", _gain);
      if (hasAuxSend()) {
            int naux = MusEGlobal::song->auxs()->size();
            for (int idx = 0; idx < naux; ++idx) {
                  QString s("<auxSend idx=\"%1\">%2</auxSend>\n");  // Aux fix from Remon, thanks.
                  xml.nput(level, s.arg(idx).arg(_auxSend[idx]).toLatin1().constData());
                  }
            }
      for (ciPluginI ip = _efxPipe->begin(); ip != _efxPipe->end(); ++ip) {
            if (*ip)
                  (*ip)->writeConfiguration(level, xml);
            }
      _controller.write(level, xml);
      }

//---------------------------------------------------------
//   readAuxSend
//---------------------------------------------------------

void AudioTrack::readAuxSend(Xml& xml)
      {
      unsigned idx = 0;
      double val;
      for (;;) {
            Xml::Token token = xml.parse();
            const QString& tag = xml.s1();
            switch (token) {
                  case Xml::Error:
                  case Xml::End:
                        return;
                  case Xml::Attribut:
                        if (tag == "idx")
                              idx = xml.s2().toInt();
                        break;
                  case Xml::Text:
                        val = tag.toDouble();
                        break;
                  case Xml::TagEnd:
                        if (xml.s1() == "auxSend") {
                              if (_auxSend.size() < idx+1)
                                    _auxSend.push_back(val);
                              else
                                    _auxSend[idx] = val;
                              return;
                              }
                  default:
                        break;
                  }
            }
      }

//---------------------------------------------------------
//   AudioTrack::readProperties
//---------------------------------------------------------

bool AudioTrack::readProperties(Xml& xml, const QString& tag)
      {
      if (tag == "plugin")
      {
            int rackpos;
            for(rackpos = 0; rackpos < MusECore::PipelineDepth; ++rackpos)
            {
              if(!(*_efxPipe)[rackpos])
                break;
            }
            if(rackpos < MusECore::PipelineDepth)
            {
              PluginI* pi = new PluginI();
              pi->setTrack(this);
              pi->setID(rackpos);
              if(pi->readConfiguration(xml, false))
                delete pi;
              else
                (*_efxPipe)[rackpos] = pi;
            }
            else
              printf("can't load plugin - plugin rack is already full\n");
      }
      else if (tag == "auxSend")
            readAuxSend(xml);
      else if (tag == "prefader")
            _prefader = xml.parseInt();
      else if (tag == "sendMetronome")
            _sendMetronome = xml.parseInt();
      else if (tag == "gain")
            _gain = xml.parseDouble();
      else if (tag == "automation")
            setAutomationType(AutomationType(xml.parseInt()));
      else if (tag == "controller") {
            CtrlList* l = new CtrlList();
            l->read(xml);

            // Since (until now) muse wrote a 'zero' for plugin controller current value
            //  in the XML file, we can't use that value, now that plugin automation is added.
            // We must take the value from the plugin control value.
            // Otherwise we break all existing .med files with plugins, because the gui
            //  controls would all be set to zero.
            // But we will allow for the (unintended, useless) possibility of a controller
            //  with no matching plugin control.
            const PluginIBase* p = 0;
            bool ctlfound = false;
            unsigned m = l->id() & AC_PLUGIN_CTL_ID_MASK;
            int n = (l->id() >> AC_PLUGIN_CTL_BASE_POW) - 1;
            if(n >= 0 && n < MusECore::PipelineDepth)
              p = (*_efxPipe)[n];
            // Support a special block for synth controllers.
            else if(n == MusECore::MAX_PLUGINS && type() == AUDIO_SOFTSYNTH)
            {
              const SynthI* synti = static_cast < SynthI* > (this);
              const SynthIF* sif = synti->sif();
              if(sif)
                p = static_cast < const PluginIBase* > (sif);
            }

            if(p && m < p->parameters())
              ctlfound = true;

            iCtrlList icl = _controller.find(l->id());
            if (icl == _controller.end())
                  _controller.add(l);
            else {
                  CtrlList* d = icl->second;
                  for (iCtrl i = l->begin(); i != l->end(); ++i)
                        d->insert(CtrlListInsertPair_t(i->first, i->second));

                  if(!ctlfound)
                        d->setCurVal(l->curVal());
                  d->setColor(l->color());
                  d->setVisible(l->isVisible());
                  d->setDefault(l->getDefault());
                  delete l;
                  l = d;
                  }

              if(ctlfound)
                {
                  l->setCurVal(p->param(m));
                  l->setValueType(p->ctrlValueType(m));
                  l->setMode(p->ctrlMode(m));
                }
            }
      else if (tag == "midiMapper")
            _controller.midiControls()->read(xml);
      else
            return Track::readProperties(xml, tag);
      return false;
      }

//---------------------------------------------------------
//   showPendingPluginNativeGuis
//   This is needed because OSC needs all tracks with plugins to be already
//    added to their track lists so it can find them and show their native guis.
//---------------------------------------------------------

void AudioTrack::showPendingPluginNativeGuis()
{
  for(int idx = 0; idx < MusECore::PipelineDepth; ++idx)
  {
    PluginI* p = (*_efxPipe)[idx];
    if(!p)
      continue;

    if(p->isShowNativeGuiPending())
      p->showNativeGui(true);
  }
}

//---------------------------------------------------------
//   mapRackPluginsToControllers
//---------------------------------------------------------

void AudioTrack::mapRackPluginsToControllers()
{
  // Iterate all possible plugin controller indexes...
  for(int idx = MusECore::PipelineDepth - 1; idx >= 0; idx--)
  {
    iCtrlList icl = _controller.lower_bound((idx + 1) * AC_PLUGIN_CTL_BASE);
    if(icl == _controller.end() || ((icl->second->id() >> AC_PLUGIN_CTL_BASE_POW) - 1) != idx)
      continue;

    // We found some controllers with that index. Now iterate the plugin rack...
    for(int i = idx; i >= 0; i--)
    {
      PluginI* p = (*_efxPipe)[i];
      if(!p)
        continue;

      // We found a plugin at a rack position. If the rack position is not the same as the controller index...
      if(i != idx)
      {
        (*_efxPipe)[i] = 0;
        (*_efxPipe)[idx] = p;
      }
      p->setID(idx);

      // It is now safe to update the controllers.
      p->updateControllers();

      break;
    }
  }

  // No matter of the outcome of the above - rack position is not too critical -
  //  making sure that each control has a controller is important. Otherwise they
  //  are stuck at zero can't be adjusted.
  // Muse med files created before the automation patches (before 0.9pre1) may have broken
  //  controller sections, so this will allow more tolerance of them.
  for(int idx = 0; idx < MusECore::PipelineDepth; idx++)
  {
    PluginI* p = (*_efxPipe)[idx];
    if(!p)
      continue;

    if(p->id() != idx)
      p->setID(idx);

    int j = p->parameters();

    for(int i = 0; i < j; i++)
    {
      int id = genACnum(idx, i);
      CtrlList* l = 0;

      ciCtrlList icl = _controller.find(id);
      if(icl == _controller.end())
      {
        l = new CtrlList(id);
        addController(l);
      }
      else
        l = icl->second;

      // Force all of these now, even though they may have already been set. With a pre-
      //  0.9pre1 med file with broken controller sections they may not be set correct.
      float min, max;
      p->range(i, &min, &max);
      l->setRange(min, max);
      l->setName(QString(p->paramName(i)));
      l->setValueType(p->ctrlValueType(i));
      l->setMode(p->ctrlMode(i));
      l->setCurVal(p->param(i));
    }
  }

  // FIXME: The loop is a safe way to delete while iterating lists.
  bool loop;
  do
  {
    loop = false;
    for(ciCtrlList icl = _controller.begin(); icl != _controller.end(); ++icl)
    {
      CtrlList* l = icl->second;
      int id = l->id();
      // Ignore volume, pan, mute etc.
      if(id < AC_PLUGIN_CTL_BASE)
        continue;

      unsigned param = id & AC_PLUGIN_CTL_ID_MASK;
      int idx = (id >> AC_PLUGIN_CTL_BASE_POW) - 1;

      const PluginIBase* p = 0;
      if(idx >= 0 && idx < MusECore::PipelineDepth)
        p = (*_efxPipe)[idx];
      // Support a special block for synth controllers.
      else if(idx == MusECore::MAX_PLUGINS && type() == AUDIO_SOFTSYNTH)
      {
        const SynthI* synti = static_cast < const SynthI* > (this);
        SynthIF* sif = synti->sif();
        if(sif)
          p = static_cast < const PluginIBase* > (sif);
      }

      // If there's no plugin at that rack position, or the param is out of range of
      //  the number of controls in the plugin, then it's a stray controller. Delete it.
      // Future: Leave room for possible bypass controller at AC_PLUGIN_CTL_ID_MASK -1.
      //if(!p || (param >= p->parameters() && (param != AC_PLUGIN_CTL_ID_MASK -1)))
      if(!p || (param >= p->parameters()))
      {
        _controller.erase(id);

        loop = true;
        break;
      }
    }
  }
  while (loop);


    // DELETETHIS 40 i DO trust the below. some container's erase functions
    // return an iterator to the next, so sometimes you need it=erase(it)
    // instead of erase(it++).
    // i'm happy with both AS LONG the above does not slow down things.
    // when in doubt, i'd prefer the below however.
    // so either remove the below completely (if the above works fast),
    // or remove the above and use the below.
    // CAUTION: the below isn't quite up-to-date! first recheck.
    // this "not-being-up-to-date" is another reason for NOT keeping such
    // comments!

    // FIXME: Although this tested OK, and is the 'official' way to erase while iterating,
    //  I don't trust it. I'm weary of this method. The technique didn't work
    //  in Audio::msgRemoveTracks(), see comments there.
    /*

    // Now delete any stray controllers which don't belong to anything.
    for(iCtrlList icl = _controller.begin(); icl != _controller.end(); )
    {
      CtrlList* l = icl->second;
      int id = l->id();
      // Ignore volume, pan, mute etc.
      if(id < AC_PLUGIN_CTL_BASE)
      {
        ++icl;
        continue;
      }

      int param = id & AC_PLUGIN_CTL_ID_MASK;
      int idx = (id >> AC_PLUGIN_CTL_BASE_POW) - 1;
      PluginI* p = (*_efxPipe)[idx];
      // If there's no plugin at that rack position, or the param is out of range of
      //  the number of controls in the plugin, then it's a stray controller. Delete it.
      // Future: Leave room for possible bypass controller at AC_PLUGIN_CTL_ID_MASK -1.
      //if(!p || (param >= p->parameters() && (param != AC_PLUGIN_CTL_ID_MASK -1)))
      if(!p || (param >= p->parameters()))
        _controller.erase(icl++);
      else
        ++icl;
    }
    */
}

RouteCapabilitiesStruct AudioTrack::routeCapabilities() const
{
  RouteCapabilitiesStruct s;
  s._trackChannels._inChannels = s._trackChannels._outChannels = totalProcessBuffers();
  s._trackChannels._inRoutable = s._trackChannels._outRoutable = (s._trackChannels._inChannels != 0);
  return s;
}

//---------------------------------------------------------
//   AudioInput
//---------------------------------------------------------

AudioInput::AudioInput()
   : AudioTrack(AUDIO_INPUT)
      {
      // set Default for Input Ports:
      setChannels(1);
      for (int i = 0; i < MusECore::MAX_CHANNELS; ++i)
            jackPorts[i] = 0;
      }

AudioInput::AudioInput(const AudioInput& t, int flags)
  : AudioTrack(t, flags)
{
  for (int i = 0; i < MusECore::MAX_CHANNELS; ++i)
        jackPorts[i] = 0;

  // Register ports.
  if(MusEGlobal::checkAudioDevice())
  {
    for (int i = 0; i < channels(); ++i)
    {
      char buffer[128];
      snprintf(buffer, 128, "%s-%d", _name.toLatin1().constData(), i);
      jackPorts[i] = MusEGlobal::audioDevice->registerInPort(buffer, false);
    }
  }
  internal_assign(t, flags);
}

void AudioInput::assign(const Track& t, int flags)
{
  AudioTrack::assign(t, flags);
  internal_assign(t, flags);
}

void AudioInput::internal_assign(const Track& t, int flags)
{
  if(t.type() != AUDIO_INPUT)
    return;

  const AudioInput& at = (const AudioInput&)t;

  if(flags & ASSIGN_ROUTES)
  {
    for(ciRoute ir = at._inRoutes.begin(); ir != at._inRoutes.end(); ++ir)
    {
      // Defer all Jack routes to these copy constructors or assign !
      if(ir->type != Route::JACK_ROUTE)
        continue;
      // FIXME Must use msgAddRoute instead of _inRoutes.push_back, because it connects to Jack.
      // The track is still fresh has not been added to track lists yet. Will cause audio processing problems ?
      MusEGlobal::audio->msgAddRoute(*ir, Route(this, ir->channel, ir->channels));
    }
  }
}

//---------------------------------------------------------
//   ~AudioInput
//---------------------------------------------------------

AudioInput::~AudioInput()
      {
      if (!MusEGlobal::checkAudioDevice()) return;
      for (int i = 0; i < _channels; ++i)
          if(jackPorts[i])
              MusEGlobal::audioDevice->unregisterPort(jackPorts[i]);
      }

//---------------------------------------------------------
//   latency
//---------------------------------------------------------

float AudioInput::trackLatency(int channel) const
{
  float l = AudioTrack::trackLatency(channel);

  if(!MusEGlobal::checkAudioDevice())
    return l;

  void* jackPort = jackPorts[channel];
  if(jackPort)
    l += MusEGlobal::audioDevice->portLatency(jackPort, true);
  return l;
}

// REMOVE Tim. latency. Added.
// //---------------------------------------------------------
// //   outputLatency
// //---------------------------------------------------------
// 
// float AudioInput::outputLatency() const
// {
//       const RouteList* rl = inRoutes();
//       float worst_case_latency = 0.0f;
//       for (ciRoute ir = rl->begin(); ir != rl->end(); ++ir) {
//             if(ir->track->isMidiTrack())
//               continue;
//             AudioTrack* atrack = static_cast<AudioTrack*>(ir->track);
//             if(!atrack)
//               continue;
//             const int atrack_out_channels = atrack->totalOutChannels();
//             const int src_ch = ir->remoteChannel <= -1 ? 0 : ir->remoteChannel;
//             const int src_chs = ir->channels;
//             int fin_src_chs = src_chs;
//             if(src_ch + fin_src_chs >  atrack_out_channels)
//               fin_src_chs = atrack_out_channels - src_ch;
//             const int next_src_chan = src_ch + fin_src_chs;
//             float atrack_worst_case_latency = 0.0f;
//             // The goal is to have equal latency output on all channels on this track.
//             for(int i = src_ch; i < next_src_chan; ++i)
//             {
//               const float lat = atrack->trackLatency(i);
//               if(lat > atrack_worst_case_latency)
//                 atrack_worst_case_latency = lat;
//             }
//             if(atrack_worst_case_latency > worst_case_latency)
//               worst_case_latency = atrack_worst_case_latency;
//             }
//             
//       // TODO: Adjust for THIS track's contribution to latency.
//       
//       return worst_case_latency;
// }

//---------------------------------------------------------
//   write
//---------------------------------------------------------

void AudioInput::write(int level, Xml& xml) const
      {
      xml.tag(level++, "AudioInput");
      AudioTrack::writeProperties(level, xml);
      xml.etag(level, "AudioInput");
      }

//---------------------------------------------------------
//   read
//---------------------------------------------------------

void AudioInput::read(Xml& xml)
      {
      for (;;) {
            Xml::Token token = xml.parse();
            const QString& tag = xml.s1();
            switch (token) {
                  case Xml::Error:
                  case Xml::End:
                        return;
                  case Xml::TagStart:
                        if (AudioTrack::readProperties(xml, tag))
                              xml.unknown("AudioInput");
                        break;
                  case Xml::Attribut:
                        break;
                  case Xml::TagEnd:
                        if (tag == "AudioInput") {
                              setName(name());  // allocate jack ports
                              mapRackPluginsToControllers();
                              return;
                              }
                  default:
                        break;
                  }
            }
      }

RouteCapabilitiesStruct AudioInput::routeCapabilities() const
{
  RouteCapabilitiesStruct s = AudioTrack::routeCapabilities();

  // Support Midi Track to Audio Input Track routes (for soloing chain).
  s._trackChannels._inRoutable = true;
  s._trackChannels._inChannels = 0;

  s._jackChannels._inRoutable = false;
  s._jackChannels._inChannels = totalProcessBuffers();
  return s;
}

//---------------------------------------------------------
//   AudioOutput
//---------------------------------------------------------

AudioOutput::AudioOutput()
   : AudioTrack(AUDIO_OUTPUT)
      {
      _nframes = 0;
      for (int i = 0; i < MAX_CHANNELS; ++i)
            jackPorts[i] = 0;
      }

AudioOutput::AudioOutput(const AudioOutput& t, int flags)
  : AudioTrack(t, flags)
{
  for (int i = 0; i < MusECore::MAX_CHANNELS; ++i)
        jackPorts[i] = 0;
  _nframes = 0;

  // Register ports.
  if(MusEGlobal::checkAudioDevice())
  {
    for (int i = 0; i < channels(); ++i)
    {
      char buffer[128];
      snprintf(buffer, 128, "%s-%d", _name.toLatin1().constData(), i);
      jackPorts[i] = MusEGlobal::audioDevice->registerOutPort(buffer, false);
    }
  }
  internal_assign(t, flags);
}

void AudioOutput::assign(const Track& t, int flags)
{
  AudioTrack::assign(t, flags);
  internal_assign(t, flags);
}

void AudioOutput::internal_assign(const Track& t, int flags)
{
  if(t.type() != AUDIO_OUTPUT)
    return;

  const AudioOutput& at = (const AudioOutput&)t;

  if(flags & ASSIGN_ROUTES)
  {
    for(ciRoute ir = at._outRoutes.begin(); ir != at._outRoutes.end(); ++ir)
    {
      // Defer all Jack routes to these copy constructors or assign !
      if(ir->type != Route::JACK_ROUTE)
        continue;
      // FIXME Must use msgAddRoute instead of _outRoutes.push_back, because it connects to Jack.
      // The track is still fresh has not been added to track lists yet. Will cause audio processing problems ?
      MusEGlobal::audio->msgAddRoute(Route(this, ir->channel, ir->channels), *ir);
    }
  }
}

//---------------------------------------------------------
//   ~AudioOutput
//---------------------------------------------------------

AudioOutput::~AudioOutput()
      {
      if (!MusEGlobal::checkAudioDevice()) return;
      for (int i = 0; i < _channels; ++i)
          if(jackPorts[i])
            MusEGlobal::audioDevice->unregisterPort(jackPorts[i]);
      }

//---------------------------------------------------------
//   latency
//---------------------------------------------------------

float AudioOutput::trackLatency(int channel) const
{
  float l = AudioTrack::trackLatency(channel);

  if(!MusEGlobal::checkAudioDevice())
    return l;

  void* jackPort = jackPorts[channel];
  if(jackPort)
    l += MusEGlobal::audioDevice->portLatency(jackPort, false);
  return l;
}

//---------------------------------------------------------
//   write
//---------------------------------------------------------

void AudioOutput::write(int level, Xml& xml) const
      {
      xml.tag(level++, "AudioOutput");
      AudioTrack::writeProperties(level, xml);
      xml.etag(level, "AudioOutput");
      }

//---------------------------------------------------------
//   read
//---------------------------------------------------------

void AudioOutput::read(Xml& xml)
      {
      for (;;) {
            Xml::Token token = xml.parse();
            const QString& tag = xml.s1();
            switch (token) {
                  case Xml::Error:
                  case Xml::End:
                        return;
                  case Xml::TagStart:
                        if (AudioTrack::readProperties(xml, tag))
                              xml.unknown("AudioOutput");
                        break;
                  case Xml::Attribut:
                        break;
                  case Xml::TagEnd:
                        if (tag == "AudioOutput") {
                              setName(name());  // allocate jack ports
                              mapRackPluginsToControllers();
                              return;
                              }
                  default:
                        break;
                  }
            }
      }

RouteCapabilitiesStruct AudioOutput::routeCapabilities() const
{
  RouteCapabilitiesStruct s = AudioTrack::routeCapabilities();

  // Support Midi Track to Audio Input Track routes (for soloing chain).
  s._trackChannels._outRoutable = true;
  s._trackChannels._outChannels = 0;

  s._jackChannels._outRoutable = false;
  s._jackChannels._outChannels = totalProcessBuffers();
  return s;
}

//---------------------------------------------------------
//   write
//---------------------------------------------------------

void AudioGroup::write(int level, Xml& xml) const
      {
      xml.tag(level++, "AudioGroup");
      AudioTrack::writeProperties(level, xml);
      xml.etag(level, "AudioGroup");
      }

//---------------------------------------------------------
//   read
//---------------------------------------------------------

void AudioGroup::read(Xml& xml)
      {
      for (;;) {
            Xml::Token token = xml.parse();
            const QString& tag = xml.s1();
            switch (token) {
                  case Xml::Error:
                  case Xml::End:
                        return;
                  case Xml::TagStart:
                        if (AudioTrack::readProperties(xml, tag))
                              xml.unknown("AudioGroup");
                        break;
                  case Xml::Attribut:
                        break;
                  case Xml::TagEnd:
                        if (tag == "AudioGroup")
                        {
                              mapRackPluginsToControllers();
                              return;
                        }
                  default:
                        break;
                  }
            }
      }

//---------------------------------------------------------
//   write
//---------------------------------------------------------

void AudioAux::write(int level, Xml& xml) const
      {
      xml.tag(level++, "AudioAux");
      AudioTrack::writeProperties(level, xml);
      xml.intTag(level, "index", _index);
      xml.etag(level, "AudioAux");
      }


//---------------------------------------------------------
//   getNextAuxIndex
//---------------------------------------------------------
int getNextAuxIndex()
{
    int curAux=0;
    AuxList * al = MusEGlobal::song->auxs();
    for (MusECore::iAudioAux i = al->begin(); i != al->end(); ++i)
    {
        MusECore::AudioAux* ax = *i;
        printf("aux index %d\n", ax->index());
        if (ax->index() > curAux)
        {
            printf("found new index! %d\n", ax->index());
            curAux = ax->index();
        }
    }
    return curAux+1;
}

//---------------------------------------------------------
//   AudioAux
//---------------------------------------------------------

AudioAux::AudioAux()
   : AudioTrack(AUDIO_AUX)
{
      _index = getNextAuxIndex();
      for(int i = 0; i < MusECore::MAX_CHANNELS; ++i)
      {
        if(i < channels())
        {
#ifdef _WIN32
          buffer[i] = (float *) _aligned_malloc(16, sizeof(float) * MusEGlobal::segmentSize);
          if(buffer[i] == NULL)
          {
            fprintf(stderr, "ERROR: AudioAux ctor: _aligned_malloc returned error: NULL. Aborting!\n");
            abort();
          }
#else
          int rv = posix_memalign((void**)(buffer + i), 16, sizeof(float) * MusEGlobal::segmentSize);
          if(rv != 0)
          {
            fprintf(stderr, "ERROR: AudioAux ctor: posix_memalign returned error:%d. Aborting!\n", rv);
            abort();
          }
#endif
          if(MusEGlobal::config.useDenormalBias)
          {
            for(unsigned q = 0; q < MusEGlobal::segmentSize; ++q)
              buffer[i][q] = MusEGlobal::denormalBias;
          }
          else
            memset(buffer[i], 0, sizeof(float) * MusEGlobal::segmentSize);
        }
        else
          buffer[i] = 0;
      }
}

AudioAux::AudioAux(const AudioAux& t, int flags)
   : AudioTrack(t, flags)
{
      _index = getNextAuxIndex();
      for(int i = 0; i < MusECore::MAX_CHANNELS; ++i)
      {
        if(i < channels())
        {
#ifdef _WIN32
          buffer[i] = (float *) _aligned_malloc(16, sizeof(float) * MusEGlobal::segmentSize);
          if(buffer[i] == NULL)
          {
            fprintf(stderr, "ERROR: AudioAux ctor: _aligned_malloc returned error: NULL. Aborting!\n");
            abort();
          }
#else
          int rv = posix_memalign((void**)(buffer + i), 16, sizeof(float) * MusEGlobal::segmentSize);
          if(rv != 0)
          {
            fprintf(stderr, "ERROR: AudioAux ctor: posix_memalign returned error:%d. Aborting!\n", rv);
            abort();
          }
#endif
          if(MusEGlobal::config.useDenormalBias)
          {
            for(unsigned q = 0; q < MusEGlobal::segmentSize; ++q)
              buffer[i][q] = MusEGlobal::denormalBias;
          }
          else
            memset(buffer[i], 0, sizeof(float) * MusEGlobal::segmentSize);
        }
        else
          buffer[i] = 0;
      }
}
//---------------------------------------------------------
//   AudioAux
//---------------------------------------------------------

AudioAux::~AudioAux()
{
      for (int i = 0; i < MusECore::MAX_CHANNELS; ++i) {
            if (buffer[i])
                free(buffer[i]);
      }
}

//---------------------------------------------------------
//   read
//---------------------------------------------------------

void AudioAux::read(Xml& xml)
      {
      for (;;) {
            Xml::Token token = xml.parse();
            const QString& tag = xml.s1();
            switch (token) {
                  case Xml::Error:
                  case Xml::End:
                        return;
                  case Xml::TagStart:
                      if (tag == "index")
                        _index = xml.parseInt();
                      else if (AudioTrack::readProperties(xml, tag))
                              xml.unknown("AudioAux");
                        break;
                  case Xml::Attribut:
                        break;
                  case Xml::TagEnd:
                        if (tag == "AudioAux")
                        {
                              mapRackPluginsToControllers();
                              return;
                        }
                  default:
                        break;
                  }
            }
      }

//---------------------------------------------------------
//   getData
//---------------------------------------------------------

bool AudioAux::getData(unsigned pos, int ch, unsigned samples, float** data)
      {
      // Make sure all the aux-supporting tracks are processed first so aux data is gathered.
      TrackList* tl = MusEGlobal::song->tracks();
      AudioTrack* track;
      for(ciTrack it = tl->begin(); it != tl->end(); ++it)
      {
        if((*it)->isMidiTrack())
          continue;
        track = (AudioTrack*)(*it);
        // If there are any Aux route paths to the track, defer processing until the second main track processing pass.
        if(!track->processed() && track->hasAuxSend() && !track->auxRefCount())
        {
          int chans = track->channels();
          // Just a dummy buffer.
          float* buff[chans];
          float buff_data[samples * chans];
          for (int i = 0; i < chans; ++i)
                buff[i] = buff_data + i * samples;

          track->copyData(pos, -1, chans, chans, -1, -1, samples, buff);
        }
      }

      for (int i = 0; i < ch; ++i)
            data[i] = buffer[i % channels()];
      return true;
      }

//---------------------------------------------------------
//   setChannels
//---------------------------------------------------------

void AudioAux::setChannels(int n)
{
  if(n > channels())
  {
    for(int i = channels(); i < n; ++i)
    {
#ifdef _WIN32
      buffer[i] = (float *) _aligned_malloc(16, sizeof(float) * MusEGlobal::segmentSize);
      if(buffer[i] == NULL)
      {
        fprintf(stderr, "ERROR: AudioTrack::setChannels: _aligned_malloc returned error: NULL. Aborting!\n");
        abort();
      }
#else
      int rv = posix_memalign((void**)(buffer + i), 16, sizeof(float) * MusEGlobal::segmentSize);
      if(rv != 0)
      {
        fprintf(stderr, "ERROR: AudioAux::setChannels: posix_memalign returned error:%d. Aborting!\n", rv);
        abort();
      }
#endif
      if(MusEGlobal::config.useDenormalBias)
      {
        for(unsigned q = 0; q < MusEGlobal::segmentSize; ++q)
          buffer[i][q] = MusEGlobal::denormalBias;
      }
      else
        memset(buffer[i], 0, sizeof(float) * MusEGlobal::segmentSize);
    }
  }
  else if(n < channels())
  {
    for(int i = n; i < channels(); ++i)
    {
      if(buffer[i])
        free(buffer[i]);
    }
  }
  AudioTrack::setChannels(n);
}

//---------------------------------------------------------
//   setName
//---------------------------------------------------------

QString AudioAux::auxName()
{
    return  QString("%1:").arg(index())+ name();
}

//---------------------------------------------------------
//   setRecordFlag1
//    gui part (executed in gui thread)
//---------------------------------------------------------

bool AudioTrack::setRecordFlag1(bool f)
      {
      if(!canRecord())
            return false;
      if (f == _recordFlag)
            return true;
      if (f) {
        if (_recFile.isNull() && MusEGlobal::song->record()) {
          // this rec-enables a track if the global arm already was done
          // the standard case would be that rec-enable be done there
          prepareRecording();
        }

      }
      else {
            if (_recFile) {
              // this file has not been processed and can be
              // deleted
              // We should only arrive here if going from a 'record-armed' state
              //  to a non record-armed state. Because otherwise after actually
              //  recording, the _recFile pointer is made into an event,
              //  then _recFile is made zero before this function is called.
              QString s = _recFile->path();
              setRecFile(NULL);

              remove(s.toLatin1().constData());
              if(MusEGlobal::debugMsg)
                printf("AudioNode::setRecordFlag1: remove file %s if it exists\n", s.toLatin1().constData());
            }
          }
      return true;
      }


//---------------------------------------------------------
//   prepareRecording
//     normally called from MusEGlobal::song->setRecord to defer creating
//     wave files until MusE is globally rec-enabled
//     also called from track->setRecordFlag (above)
//     if global rec enable already was done
//---------------------------------------------------------
bool AudioTrack::prepareRecording()
{
      if(MusEGlobal::debugMsg)
        printf("prepareRecording for track %s\n", name().toLatin1().constData());

      if (_recFile.isNull()) {
            //
            // create soundfile for recording
            //
            const QString fbase = QString("%1/").arg(MusEGlobal::museProject) +
                                  QObject::tr("TRACK") +
                                  QString("_%1_").arg(name().simplified().replace(" ","_")) +
                                  QObject::tr("TAKE");
            QFile fil;
            for (;;++recFileNumber) {
               fil.setFileName(fbase + QString("_%1.wav").arg(recFileNumber));
               if (!fil.exists())
                  break;
                  }
            _recFile = new MusECore::SndFile(fil.fileName());

            _recFile->setFormat(
               SF_FORMAT_WAV | SF_FORMAT_FLOAT,
               _channels, MusEGlobal::sampleRate);
      }

      if (MusEGlobal::debugMsg)
          printf("AudioTrack::prepareRecording: init internal file %s\n", _recFile->path().toLatin1().constData());

      if(_recFile->openWrite())
            {
            QMessageBox::critical(NULL, "MusE write error.", "Error creating target wave file\n"
                                                            "Check your configuration.");
            return false;

            }
      return true;
}
double AudioTrack::auxSend(int idx) const
      {
      if (unsigned(idx) >= _auxSend.size()) {
            printf("%s auxSend: bad index: %d >= %zd\n",
               name().toLatin1().constData(), idx, _auxSend.size());
            return 0.0;
            }
      return _auxSend[idx];
      }

void AudioTrack::setAuxSend(int idx, double v)
      {
      if (unsigned(idx) >= _auxSend.size()) {
            printf("%s setAuxSend: bad index: %d >= %zd\n",
               name().toLatin1().constData(), idx, _auxSend.size());
            return;
            }
      _auxSend[idx] = v;
      }

//---------------------------------------------------------
//   height
//---------------------------------------------------------
int AudioOutput::height() const
{
  if (_isVisible)
    return _height;
  return 0;
}
int AudioInput::height() const
{
  if (_isVisible)
    return _height;
  return 0;
}
int AudioAux::height() const
{
  if (_isVisible)
    return _height;
  return 0;
}
int AudioGroup::height() const
{
  if (_isVisible)
    return _height;
  return 0;
}

int WaveTrack::height() const
{
  if (_isVisible)
    return _height;
  return 0;
}

} // namespace MusECore
