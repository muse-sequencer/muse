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

//#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <map>

#include <QMessageBox>
#include <QFile>

#include "globals.h"
#include "globaldefs.h"
#include "track.h"
#include "event.h"
#include "song.h"
#include "audio.h"
#include "wave.h"
#include "xml.h"
#include "undo.h"
#include "plugin.h"
#include "audiodev.h"
#include "synth.h"
#include "app.h"
#include "controlfifo.h"
#include "fastlog.h"
#include "gconfig.h"
#include "latency_compensator.h"
#include "ticksynth.h"
#include "xml_statistics.h"

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
      if(outBuffers[i] == nullptr)
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
      if(outBuffersExtraMix[i] == nullptr)
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
      if(_dataBuffers[i] == nullptr)
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

  if(!audioInSilenceBuf)
  {
#ifdef _WIN32
    audioInSilenceBuf = (float *) _aligned_malloc(16, sizeof(float) * MusEGlobal::segmentSize);
    if(audioInSilenceBuf == nullptr)
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
    if(audioOutDummyBuf == nullptr)
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

AudioTrack::AudioTrack(TrackType t, int channels)
   : Track(t)
      {
      _totalOutChannels = MAX_CHANNELS;
      _latencyComp = new LatencyCompensator();
      _recFilePos = 0;
      _previousLatency = 0.0f;

      _processed = false;
      _haveData = false;
      _sendMetronome = false;
      _prefader = false;
      _efxPipe  = new Pipeline();
      recFileNumber = 1;
      _channels = 0;
      _automationType = AUTO_OFF;
      setChannels(channels);

      CtrlList *cl = new CtrlList(AC_VOLUME,"Volume",0.0,3.16227766017 /* roughly 10 db */, VAL_LOG, 1.0);
      cl->setValueUnit(MusEGlobal::valueUnits.addSymbol("dB"));
      cl->setDisplayHint(CtrlList::DisplayLogDB);
      addController(cl);

      addController(new CtrlList(AC_PAN, "Pan", -1.0, 1.0, VAL_LINEAR, 0.0));
      addController(new CtrlList(AC_MUTE,"Mute",0.0,1.0, VAL_LINEAR, 0.0, true /*don't show in arranger */));
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

      // This is only set by multi-channel syntis...
      _totalInChannels = 0;

      initBuffers();

      setVolume(1.0);
      setPan(0.0);
      _gain = 1.0;
      }

AudioTrack::AudioTrack(const AudioTrack& t, int flags)
  :  Track(t, flags)
      {
      _latencyComp = new LatencyCompensator();
      _recFilePos = 0;
      _previousLatency = 0.0f;

      _processed      = false;
      _haveData       = false;
      _efxPipe        = new Pipeline();                 // Start off with a new pipeline.
      recFileNumber = 1;

      CtrlList *cl = new CtrlList(AC_VOLUME,"Volume",0.0,3.16227766017 /* roughly 10 db */, VAL_LOG, 1.0);
      cl->setValueUnit(MusEGlobal::valueUnits.addSymbol("dB"));
      cl->setDisplayHint(CtrlList::DisplayLogDB);
      addController(cl);

      addController(new CtrlList(AC_PAN, "Pan", -1.0, 1.0, VAL_LINEAR, 0.0));
      addController(new CtrlList(AC_MUTE,"Mute",0.0,1.0, VAL_LINEAR, 0.0, true /*don't show in arranger */));
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

      _recFile = nullptr;

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
        // The beginning of the special synth controller block.
        const int synth_id = (int)genACnum(MusECore::MAX_PLUGINS, 0);
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
      _erasedController.clearDelete();
      _noEraseController.clearDelete();
}

void AudioTrack::fixOldColorScheme()
{
  int numgreenctrls = 0;
  for(ciCtrlList icl = _controller.cbegin(); icl != _controller.cend(); ++icl)
  {
    const CtrlList* cl = icl->second;
    const int id = cl->id();
    if(id < AC_PLUGIN_CTL_BASE)
      continue;
    const QColor c = cl->color();
    if(c.red() == 0 && c.green() == 255 && c.blue() == 0)
      ++numgreenctrls;
    if(numgreenctrls >= 2)
      break;
  }

  if(numgreenctrls >= 2)
  {
    for(iCtrlList icl = _controller.begin(); icl != _controller.end(); ++icl)
    {
      CtrlList* cl = icl->second;
      const int id = cl->id();
      if(id < AC_PLUGIN_CTL_BASE)
        continue;
      const QColor c = cl->color();
      if(c.red() == 0 && c.green() == 255 && c.blue() == 0)
        cl->initColor(id);
    }
  }
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
//   newPart
//---------------------------------------------------------

Part* AudioTrack::newPart(Part*, bool /*clone*/)
      {
      return 0;
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

    // Grab the information from the plugin if it exists.
    if(plugin->plugin())
    {
      int params = plugin->parameters();
      for (int i = 0; i < params; ++i)
      {
        int id = genACnum(idx, i);
        CtrlList* cl = new CtrlList(id);
        addController(cl);
      }
    }
    else
    // Plugin is missing. Grab the information from the plugin's persistent properies.
    {
      const PluginConfiguration &pc = plugin->initialConfiguration();
      for(ciPluginControlList ipcl = pc._initParams.cbegin(); ipcl != pc._initParams.cend(); ++ipcl)
      {
        const PluginControlConfig &cc = ipcl->second;
        const int id = genACnum(idx, cc._ctlnum);
        CtrlList* cl = new CtrlList(id);
        addController(cl);
      }
    }
    plugin->setupControllers(controller());
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
//   addController
//---------------------------------------------------------

bool AudioTrack::addController(CtrlList* list)
      {
      return _controller.add(list);
      }

//---------------------------------------------------------
//   removeController
//---------------------------------------------------------

void AudioTrack::removeController(int id)
      {
      AudioMidiCtrlStructMap amcs;
      // Include NULL tracks in search.
      MusEGlobal::song->midiAssignments()->find_audio_ctrl_structs(MidiAudioCtrlStruct::AudioControl, id, this, false, true, &amcs);
      for(ciAudioMidiCtrlStructMap iamcs = amcs.begin(); iamcs != amcs.end(); ++ iamcs)
        MusEGlobal::song->midiAssignments()->erase(*iamcs);
      iCtrlList i = _controller.find(id);
      if (i == _controller.end()) {
            printf("AudioTrack::removeController id %d not found\n", id);
            return;
            }
      _controller.erase(i);
      }

bool AudioTrack::addControllerFromXml(CtrlList* l)
{
  // Controller IDs less than zero cannot be added.
  if(l->id() >= 0)
  {
      // Try to add the controller to the list.
      std::pair<CtrlListList::iterator, bool> res = _controller.insert(std::pair<const int, CtrlList*>(l->id(), l));
      // Controller already exists?
      if(!res.second)
      {
        CtrlList* d = res.first->second;
        // The track controller should be empty at this point.
        // The given controller contains the desired items.
        // Simply swap the items in the two controller lists.
        d->swap(*l);
        // And... make sure to assign the properties.
        // We can't use simple assign methods because some of the
        //  information in the given controller is not supplied
        //  in the XML and is blank, such as name. Don't overwrite with that.
        // NOTE: Very old versions may have written a 'zero' for plugin controller current value
        //  in the XML file. We can't use that value. We must take the value from the plugin port value.
        // Otherwise we break all existing .med files with plugins, because the gui controls would all be set to zero.
        // If it's a plugin or synth controller, it's OK, the value will be set later from the port value.
        d->setCurVal(l->curVal());
        d->setColor(l->color());
        d->setVisible(l->isVisible());
        d->setDefault(l->getDefault());
        // Yes we want this one as well. The caller will either set it beforehand or will
        //  set it later in (or after) mapRackPluginsToControllers() for example.
        // Special: If the destination controller's flag is already set, DO NOT upset it by resetting it.
        if(l->dontShow())
          d->setDontShow(true);
        // Done with the given controller. Delete it.
        delete l;
        l = d;
      }
    }
    else
    {
      // The controller is orphaned now. Delete it.
      delete l;
      return false;
    }

    return true;
}

bool AudioTrack::setupController(CtrlList* l)
{
  if(!l || l->id() < 0)
    return false;

  const PluginConfiguration *pc = nullptr;
  const PluginIBase* p = 0;
  unsigned m = l->id() & AC_PLUGIN_CTL_ID_MASK;
  int n = (l->id() >> AC_PLUGIN_CTL_BASE_POW) - 1;
  if(n >= 0 && n < MusECore::PipelineDepth)
  {
    const PluginI *pi = (*_efxPipe)[n];
    p = pi;
    if(!pi->plugin())
      pc = &pi->initialConfiguration();
  }
  // Support a special block for synth controllers.
  else if(n == MusECore::MAX_PLUGINS && type() == AUDIO_SOFTSYNTH)
  {
    const SynthI* synti = static_cast < SynthI* > (this);
    const SynthIF* sif = synti->sif();
    if(sif)
      p = static_cast < const PluginIBase* > (sif);
    if(!sif || !synti->synth())
      pc = &synti->initialConfiguration();
  }

  // Plugin or synth is missing? Use the persistent information.
  if(pc)
  {
    // The plugin is missing. If the original file version is valid and less than 4
    //  we need to hide all the automation controllers because the range, type, mode
    //  are not available so the graphs cannot be scaled properly.
    const bool isPersistentPre4 = pc->_fileVerMaj >= 0 && pc->_fileVerMaj < 4;
    ciPluginControlList ipcl = pc->_initParams.find(m);
    if(ipcl != pc->_initParams.cend())
    {
      const PluginControlConfig &cc = ipcl->second;
      l->setRange(cc._min, cc._max);
      l->setName(cc._name);
      l->setValueType(cc._valueType);
      l->setMode(cc._ctlMode);
      l->setCurVal(cc._val);
      // Set the value units index.
      l->setValueUnit(cc._valueUnit);
    }
    if(isPersistentPre4)
      l->setDontShow(true);
    return true;
  }
  else
  {
    if(p && m < p->parameters())
    {
      float min, max;
      p->range(m, &min, &max);
      l->setRange(min, max);
      l->setName(QString(p->paramName(m)));
      l->setValueType(p->ctrlValueType(m));
      l->setMode(p->ctrlMode(m));
      l->setCurVal(p->param(m));
      // Set the value units index.
      l->setValueUnit(p->valueUnit(m));
      return true;
    }
  }

  return false;
}

//---------------------------------------------------------
//   setAutomationType
//---------------------------------------------------------

void AudioTrack::setAutomationType(AutomationType t)
{
  // Clear rec event list.
  clearRecAutomation();
  // Re-enable all track and plugin controllers, and synth controllers if applicable.
  enableAllControllers();

  // Now set the type.
  _automationType = t;
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

    MusEGlobal::song->setPos(Song::CPOS, Pos(s->first, false), false, true, false);
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

    MusEGlobal::song->setPos(Song::CPOS, Pos(s->first, false), false, true, false);
    return;
}

//---------------------------------------------------------
//   useLatencyCorrection
//---------------------------------------------------------

bool AudioTrack::useLatencyCorrection() const
{
  return _latencyComp && MusEGlobal::config.enableLatencyCorrection;
}

//---------------------------------------------------------
//   selfLatencyAudio
//---------------------------------------------------------

float AudioTrack::selfLatencyAudio(int /*channel*/) const
{
  if(!_efxPipe)
    return 0.0;
  return _efxPipe->latency();
}

//---------------------------------------------------------
//   getDominanceInfo
//---------------------------------------------------------

TrackLatencyInfo& AudioTrack::getDominanceInfo(bool input)
{
  // Have we been here before during this scan?
  // Just return the cached value.
  if((input && _latencyInfo._canDominateInputProcessed) ||
     (!input && _latencyInfo._canDominateProcessed))
    return _latencyInfo;

  // Get the default domination for this track type.
  bool can_dominate_lat = input ? canDominateInputLatency() : canDominateOutputLatency();
  bool can_correct_lat = canCorrectOutputLatency();

  const bool passthru = canPassThruLatency();

  bool item_found = false;

  if(!off() && (passthru || input))
  {
    RouteList* rl = inRoutes();
    for (iRoute ir = rl->begin(); ir != rl->end(); ++ir)
    {
      switch(ir->type)
      {
        case Route::TRACK_ROUTE:
          if(!ir->track)
            continue;
          if(ir->track->isMidiTrack())
          {
            // TODO ?
          }
          else
          {
            Track* track = ir->track;
            if(!track->off())
            {
              const TrackLatencyInfo& li = track->getDominanceInfo(false);

              // Whether the branch can dominate or correct latency or if we
              //  want to allow unterminated input branches to
              //  participate in worst branch latency calculations.
              const bool participate = 
                (li._canCorrectOutputLatency ||
                li._canDominateOutputLatency ||
                MusEGlobal::config.correctUnterminatedInBranchLatency);

              if(participate)
              {
                // Is it the first found item?
                if(item_found)
                {
                  // If any one of the branches can dominate the latency,
                  //  that overrides any which cannot.
                  if(li._canDominateOutputLatency)
                    can_dominate_lat = true;
                  if(li._canCorrectOutputLatency)
                    can_correct_lat = true;
                }
                else
                {
                  item_found = true;
                  // Override the defaults with this first item's values.
                  can_dominate_lat = li._canDominateOutputLatency;
                  can_correct_lat = li._canCorrectOutputLatency;
                }
              }
            }
          }
        break;

        default:
        break;
      }
    }

    // Special for the built-in metronome.
    //if(!off() && !MusECore::metronome->off() && (passthru || input) && sendMetronome())
    if(!MusECore::metronome->off() && sendMetronome())
    {
      const TrackLatencyInfo& li = MusECore::metronome->getDominanceInfo(false);
          
      // Whether the branch can dominate or correct latency or if we
      //  want to allow unterminated input branches to
      //  participate in worst branch latency calculations.
      const bool participate = 
        (li._canCorrectOutputLatency ||
        li._canDominateOutputLatency ||
        MusEGlobal::config.correctUnterminatedInBranchLatency);

      if(participate)
      {
        // Is it the first found item?
        if(item_found)
        {
          // If any one of the branches can dominate the latency,
          //  that overrides any which cannot.
          if(li._canDominateOutputLatency)
            can_dominate_lat = true;
          if(li._canCorrectOutputLatency)
            can_correct_lat = true;
        }
        else
        {
          item_found = true;
          can_dominate_lat = li._canDominateOutputLatency;
          can_correct_lat = li._canCorrectOutputLatency;
        }
      }
    }
  }

  // Set the correction of all connected input branches,
  //  but ONLY if the track is not off.
  if(!off())
  {
    if(input)
    {
      _latencyInfo._canDominateInputLatency = can_dominate_lat;
    }
    else
    {
      _latencyInfo._canDominateOutputLatency = can_dominate_lat;
      // If any of the branches can dominate, then this node cannot correct.
      _latencyInfo._canCorrectOutputLatency = can_correct_lat && !can_dominate_lat;
    }
  }

  if(input)
    _latencyInfo._canDominateInputProcessed = true;
  else
    _latencyInfo._canDominateProcessed = true;

  return _latencyInfo;
}

//---------------------------------------------------------
//   getDominanceLatencyInfo
//---------------------------------------------------------

TrackLatencyInfo& AudioTrack::getDominanceLatencyInfo(bool input)
{
  // Have we been here before during this scan?
  // Just return the cached value.
  if((input && _latencyInfo._dominanceInputProcessed) ||
     (!input && _latencyInfo._dominanceProcessed))
    return _latencyInfo;

  float route_worst_latency = 0.0f;

  const bool passthru = canPassThruLatency();

  bool item_found = false;

  float worst_self_latency = 0.0f;
  if(!input && !off())
    worst_self_latency = getWorstSelfLatencyAudio();
  
  if(!off() && (passthru || input))
  {
    RouteList* rl = inRoutes();
    for (iRoute ir = rl->begin(); ir != rl->end(); ++ir)
    {
      switch(ir->type)
      {
        case Route::TRACK_ROUTE:
          if(!ir->track)
            continue;
          if(ir->track->isMidiTrack())
          {
            // TODO ?
          }
          else
          {
            Track* track = ir->track;

            //if(!off() && !track->off() && (passthru || input))
            if(!track->off())
            {
              const TrackLatencyInfo& li = track->getDominanceLatencyInfo(false);

              // Whether the branch can dominate or correct latency or if we
              //  want to allow unterminated input branches to
              //  participate in worst branch latency calculations.
              const bool participate = 
                (li._canCorrectOutputLatency ||
                li._canDominateOutputLatency ||
                MusEGlobal::config.correctUnterminatedInBranchLatency);

              if(participate)
              {
                // Is it the first found item?
                if(item_found)
                {
                  // If any one of the branches can dominate the latency,
                  //  that overrides any which cannot.
                  if(li._canDominateOutputLatency)
                  {
                    // Override the current worst value if the latency is greater,
                    //  but ONLY if the branch can dominate.
                    //if(li._outputLatency > route_worst_latency)
                    //  route_worst_latency = li._outputLatency;
                  }
                  // Override the current worst value if the latency is greater,
                  //  but ONLY if the branch can dominate.
                  if(li._outputLatency > route_worst_latency)
                    route_worst_latency = li._outputLatency;
                }
                else
                {
                  item_found = true;
                  // Override the default worst value, but ONLY if the branch can dominate.
                  //if(li._canDominateOutputLatency)
                    route_worst_latency = li._outputLatency;
                }
              }
            }
          }
        break;

        default:
        break;
      }
    }

    // Special for the built-in metronome.
    //if(!off() && !MusECore::metronome->off() && (passthru || input) && sendMetronome())
    if(!MusECore::metronome->off() && sendMetronome())
    {
      const TrackLatencyInfo& li = MusECore::metronome->getDominanceLatencyInfo(false);
          
      // Whether the branch can dominate or correct latency or if we
      //  want to allow unterminated input branches to
      //  participate in worst branch latency calculations.
      const bool participate = 
        (li._canCorrectOutputLatency ||
        li._canDominateOutputLatency ||
        MusEGlobal::config.correctUnterminatedInBranchLatency);

      if(participate)
      {
        // Is it the first found item?
        if(item_found)
        {
          // If any one of the branches can dominate the latency,
          //  that overrides any which cannot.
          if(li._canDominateOutputLatency)
          {
            // Override the current worst value if the latency is greater,
            //  but ONLY if the branch can dominate.
            //if(li._outputLatency > route_worst_latency)
            //  route_worst_latency = li._outputLatency;
          }
          // Override the current worst value if the latency is greater,
          //  but ONLY if the branch can dominate.
          if(li._outputLatency > route_worst_latency)
            route_worst_latency = li._outputLatency;
        }
        else
        {
          item_found = true;
          // Override the default worst value, but ONLY if the branch can dominate.
          //if(li._canDominateOutputLatency)
            route_worst_latency = li._outputLatency;
        }
      }
    }
  }

  // Set the correction of all connected input branches,
  //  but ONLY if the track is not off.
  if(!off())
  {
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
  }

  if(input)
    _latencyInfo._dominanceInputProcessed = true;
  else
    _latencyInfo._dominanceProcessed = true;

  return _latencyInfo;
}

//---------------------------------------------------------
//   getWorstPluginLatencyAudio
//---------------------------------------------------------

float AudioTrack::getWorstPluginLatencyAudio()
{
  // Have we been here before during this scan?
  // Just return the cached value.
  if(_latencyInfo._worstPluginLatencyProcessed)
    return _latencyInfo._worstPluginLatency;

  float worst_lat = 0.0f;
  // Include the effects rack latency.
  if(_efxPipe)
    worst_lat += _efxPipe->latency();
  
  _latencyInfo._worstPluginLatency = worst_lat;
  _latencyInfo._worstPluginLatencyProcessed = true;
  return _latencyInfo._worstPluginLatency;
}

//---------------------------------------------------------
//   getWorstSelfLatencyAudio
//---------------------------------------------------------

float AudioTrack::getWorstSelfLatencyAudio()
{
  // Have we been here before during this scan?
  // Just return the cached value.
  if(_latencyInfo._worstSelfLatencyProcessed)
    return _latencyInfo._worstSelfLatency;

  // Include the effects rack latency and any synth latency and any port latency.
  _latencyInfo._worstSelfLatency = getWorstPluginLatencyAudio() + getWorstPortLatencyAudio();
  
  // The absolute latency of signals leaving this track is the sum of
  //  any connected route latencies and this track's latency.
  _latencyInfo._worstSelfLatencyProcessed = true;
  return _latencyInfo._worstSelfLatency;
}

//---------------------------------------------------------
//   setCorrectionLatencyInfo
//---------------------------------------------------------

TrackLatencyInfo& AudioTrack::setCorrectionLatencyInfo(bool input, float finalWorstLatency, float callerBranchLatency)
{
  const bool passthru = canPassThruLatency();

  float worst_self_latency = 0.0f;
  if(!input && !off())
    worst_self_latency = getWorstSelfLatencyAudio();
      
  // The _trackLatency should already be calculated in the dominance scan.
  const float branch_lat = callerBranchLatency + worst_self_latency;

  if(!off() && (passthru || input))
  {
    RouteList* rl = inRoutes();
    for (iRoute ir = rl->begin(); ir != rl->end(); ++ir)
    {
      if(ir->type != Route::TRACK_ROUTE || !ir->track || ir->track->isMidiTrack())
        continue;
      Track* track = ir->track;
      //if(!off() && !track->off() && (passthru || input))
      if(!track->off())
        track->setCorrectionLatencyInfo(false, finalWorstLatency, branch_lat);
    }

    // Special for the built-in metronome.
    //if(!off() && !MusECore::metronome->off() && (passthru || input) && sendMetronome())
    if(!MusECore::metronome->off() && sendMetronome())
    {
      MusECore::metronome->setCorrectionLatencyInfo(false, finalWorstLatency, branch_lat);
    }
  }

  // Set the correction of all connected input branches,
  //  but ONLY if the track is not off.
  if(!off())
  {
    if(input)
    {
    }
    else
    {
      if(canCorrectOutputLatency() && _latencyInfo._canCorrectOutputLatency)
      {
        float corr = 0.0f;
        if(MusEGlobal::config.commonProjectLatency)
          corr -= finalWorstLatency;

        corr -= branch_lat;
        // The _sourceCorrectionValue is initialized to zero.
        // Whichever calling branch needs the most correction gets it.
        if(corr < _latencyInfo._sourceCorrectionValue)
          _latencyInfo._sourceCorrectionValue = corr;
      }
    }

    //fprintf(stderr, "AudioTrack::setCorrectionLatencyInfo() name:%s finalWorstLatency:%f branch_lat:%f corr:%f _sourceCorrectionValue:%f\n",
    //        name().toLocal8Bit().constData(), finalWorstLatency, branch_lat, corr, _latencyInfo._sourceCorrectionValue);
  }

  return _latencyInfo;
}

//---------------------------------------------------------
//   getLatencyInfo
//---------------------------------------------------------

TrackLatencyInfo& AudioTrack::getLatencyInfo(bool input)
{
  // Have we been here before during this scan?
  // Just return the cached value.
  if((input && _latencyInfo._inputProcessed) ||
    (!input && _latencyInfo._processed))
    return _latencyInfo;

  float route_worst_latency = _latencyInfo._inputLatency;

  const bool passthru = canPassThruLatency();

  if(passthru || input)
  {
    RouteList* rl = inRoutes();

    // Now that we know the worst-case latency of the connected branches,
    //  adjust each of the conveniently stored temporary latency values
    //  in the routes according to whether they can dominate...
    for (iRoute ir = rl->begin(); ir != rl->end(); ++ir)
    {
      if(ir->type != Route::TRACK_ROUTE || !ir->track || ir->track->isMidiTrack())
        continue;

      Track* track = ir->track;

      // Default to zero.
      ir->audioLatencyOut = 0.0f;

      if(!off() && !track->off())
      {
        const TrackLatencyInfo& li = track->getLatencyInfo(false);
        const bool participate =
          (li._canCorrectOutputLatency ||
          li._canDominateOutputLatency ||
          MusEGlobal::config.correctUnterminatedInBranchLatency);

        if(participate)
        {
          // Prepare the latency value to be passed to the compensator's writer,
          //  by adjusting each route latency value. ie. the route with the worst-case
          //  latency will get ZERO delay, while routes having smaller latency will get
          //  MORE delay, to match all the signal timings together.
          // The route's audioLatencyOut should have already been calculated and
          //  conveniently stored in the route.
          ir->audioLatencyOut = route_worst_latency - li._outputLatency;
          // Should not happen, but just in case.
          if((long int)ir->audioLatencyOut < 0)
            ir->audioLatencyOut = 0.0f;
        }
      }
    }

    // Special for the built-in metronome.
    // Default to zero.
    _latencyInfo._latencyOutMetronome = 0.0f;
    if(!off() && !MusECore::metronome->off() && sendMetronome())
    {
      TrackLatencyInfo& li = MusECore::metronome->getLatencyInfo(false);

      const bool participate =
        li._canCorrectOutputLatency ||
        li._canDominateOutputLatency ||
        MusEGlobal::config.correctUnterminatedInBranchLatency;

      if(participate)
      {
        // TODO: FIXME: Where to store? We have no route to store it in.
        // Prepare the latency value to be passed to the compensator's writer,
        //  by adjusting each route latency value. ie. the route with the worst-case
        //  latency will get ZERO delay, while routes having smaller latency will get
        //  MORE delay, to match all the signal timings together.
        // The route's audioLatencyOut should have already been calculated and
        //  conveniently stored in the route.

//             ir->audioLatencyOut = route_worst_latency - ir->audioLatencyOut;
//             // Should not happen, but just in case.
//             if((long int)ir->audioLatencyOut < 0)
//               ir->audioLatencyOut = 0.0f;

        // Special for metronome: We don't have metronome routes yet.
        // So we must store this information here just for the metronome.
        li._latencyOutMetronome = route_worst_latency - li._outputLatency;
        // Should not happen, but just in case.
        if((long int)li._latencyOutMetronome < 0)
          li._latencyOutMetronome = 0.0f;
      }
    }
  }

  if(input)
    _latencyInfo._inputProcessed = true;
  else
    _latencyInfo._processed = true;

  return _latencyInfo;
}

void AudioTrack::setLatencyCompWriteOffset(float worstCase)
{
  // If independent branches are NOT to affect project latency,
  //  then there should be no need for any extra delay in the branch.
  if(!MusEGlobal::config.commonProjectLatency)
  {
    _latencyInfo._compensatorWriteOffset = 0;
    //fprintf(stderr, "AudioTrack::setLatencyCompWriteOffset() name:%s worstCase:%f _outputLatency:%f _compensatorWriteOffset:%lu\n",
    //        name().toLocal8Bit().constData(), worstCase, _latencyInfo._outputLatency, _latencyInfo._compensatorWriteOffset);
    return;
  }
    
  if(_latencyInfo._canDominateOutputLatency)
  {
    const long unsigned int wc = worstCase;
    const long unsigned int ol = _latencyInfo._outputLatency;
    if(ol > wc)
      _latencyInfo._compensatorWriteOffset = 0;
    else
      _latencyInfo._compensatorWriteOffset = wc - ol;
  }
  else
  {
//     if(_latencyInfo._outputLatency < 0)
      _latencyInfo._compensatorWriteOffset = 0;
//     else
//       _latencyInfo._compensatorWriteOffset = _latencyInfo._outputLatency;
  }

  //fprintf(stderr,
  //  "AudioTrack::setLatencyCompWriteOffset() name:%s worstCase:%f"
  //  " _outputLatency:%f _canDominateOutputLatency:%d _compensatorWriteOffset:%lu\n",
  //      name().toLocal8Bit().constData(), worstCase, _latencyInfo._outputLatency,
  //      _latencyInfo._canDominateOutputLatency, _latencyInfo._compensatorWriteOffset);
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
               name().toLocal8Bit().constData(), _controller.size());
            return;
            }
      cl->second->setCurVal(val);
      // Notify the GUI to redraw the controller.
      // Yes, we're already in the GUI thread. But take advantage of the
      //  ring buffer's leisurely processing rate to avoid overloading the GUI.
      if(MusEGlobal::song)
        MusEGlobal::song->putIpcCtrlGUIMessage(CtrlGUIMessage(this, AC_VOLUME));
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
      // Notify the GUI to redraw the controller.
      // Yes, we're already in the GUI thread. But take advantage of the
      //  ring buffer's leisurely processing rate to avoid overloading the GUI.
      if(MusEGlobal::song)
        MusEGlobal::song->putIpcCtrlGUIMessage(CtrlGUIMessage(this, AC_PAN));
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
  // Notify the GUI to redraw the controller.
  if(MusEGlobal::song)
    MusEGlobal::song->putIpcCtrlGUIMessage(CtrlGUIMessage(this, param));
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
          if(automationType() == AUTO_WRITE || automationType() == AUTO_TOUCH || automationType() == AUTO_LATCH)
          // In touch mode and not playing. Send directly to controller list.
          {
            const unsigned int frame = MusEGlobal::audio->curFramePos();
            _recEvents.addInitial(CtrlRecVal(frame, n, v, CtrlRecVal::ARVT_IGNORE));

            iCtrlList cl = _controller.find(n);
            if (cl == _controller.end())
              return;
            // Force a discrete value, to be faithful to the incoming stream because
            //  if someone leaves a controller at a value for a while then moves
            //  the controller, this point and the next would be a long interpolation.

            // Modify will add if not found.
            // Use modify instead of add so that if there is an existing interpolated point,
            //  users can modify it using a controller without disturbing the mode.
            cl->second->modify(frame, v,
              CtrlVal::VAL_SELECTED | CtrlVal::VAL_DISCRETE,
              CtrlVal::VAL_MODIFY_VALUE | CtrlVal::VAL_SELECTED /*| CtrlVal::VAL_DISCRETE*/,
              CtrlVal::VAL_MODIFY_VALUE | CtrlVal::VAL_SELECTED | CtrlVal::VAL_DISCRETE);

            // Notify the GUI to update any local structures etc.
            // Yes, we're already in the GUI thread. But take advantage of the
            //  ring buffer's leisurely processing rate to avoid overloading the GUI.
            // The item has already been added, so we just want a GUI update soon-ish...
            if(MusEGlobal::song)
              MusEGlobal::song->putIpcCtrlGUIMessage(
                CtrlGUIMessage(this, cl->second->id(), frame, v, CtrlGUIMessage::ADDED));
          }
        }
      }

void AudioTrack::startAutoRecord(int n, double v)
      {
        if(!MusEGlobal::automation)
          return;
        if(MusEGlobal::audio->isPlaying())
        {
          if(automationType() == AUTO_WRITE || automationType() == AUTO_TOUCH || automationType() == AUTO_LATCH)
              _recEvents.push_back(CtrlRecVal(MusEGlobal::audio->curFramePos(), n, v));
        }
        else
        {
          if(automationType() == AUTO_WRITE || automationType() == AUTO_TOUCH || automationType() == AUTO_LATCH)
          // In touch mode and not playing. Send directly to controller list.
          {
            const unsigned int frame = MusEGlobal::audio->curFramePos();
            _recEvents.addInitial(CtrlRecVal(frame, n, v, CtrlRecVal::ARVT_IGNORE));

            // FIXME: Unsafe? Should sync by sending a message, but that'll really slow it down with large audio bufs.
            iCtrlList cl = _controller.find(n);
            if (cl == _controller.end())
              return;
            // Force a discrete value, to be faithful to the incoming stream because
            //  if someone leaves a controller at a value for a while then moves
            //  the controller, this point and the next would be a long interpolation.

            // Modify will add if not found.
            // Use modify instead of add so that if there is an existing interpolated point,
            //  users can modify it using a controller without disturbing the mode.
            cl->second->modify(frame, v,
              CtrlVal::VAL_SELECTED | CtrlVal::VAL_DISCRETE,
              CtrlVal::VAL_MODIFY_VALUE | CtrlVal::VAL_SELECTED /*| CtrlVal::VAL_DISCRETE*/,
              CtrlVal::VAL_MODIFY_VALUE | CtrlVal::VAL_SELECTED | CtrlVal::VAL_DISCRETE);

            // Notify the GUI to update any local structures etc.
            // Yes, we're already in the GUI thread. But take advantage of the
            //  ring buffer's leisurely processing rate to avoid overloading the GUI.
            // The item has already been added, so we just want a GUI update soon-ish...
            if(MusEGlobal::song)
              MusEGlobal::song->putIpcCtrlGUIMessage(
                CtrlGUIMessage(this, cl->second->id(), frame, v, CtrlGUIMessage::ADDED));
          }
        }
      }

void AudioTrack::stopAutoRecord(int n, double v)
      {
        if(!MusEGlobal::automation)
          return;
        if(MusEGlobal::audio->isPlaying())
        {
          if(automationType() == AUTO_TOUCH)
              _recEvents.push_back(CtrlRecVal(MusEGlobal::audio->curFramePos(), n, v, CtrlRecVal::ARVT_STOP));
        }
        else
        {
          if(/*automationType() == AUTO_WRITE ||*/ automationType() == AUTO_TOUCH /*|| automationType() == AUTO_LATCH*/)
              _recEvents.addInitial(CtrlRecVal(MusEGlobal::audio->curFramePos(), n, v, CtrlRecVal::ARVT_STOP | CtrlRecVal::ARVT_IGNORE));
        }
      }

//---------------------------------------------------------
//   AudioTrack::writeProperties
//---------------------------------------------------------

void AudioTrack::writeProperties(int level, Xml& xml) const
      {
      xml.tag(level++, "AudioTrack");
      Track::writeProperties(level, xml);
      xml.intTag(level, "prefader", prefader());
      xml.intTag(level, "sendMetronome", sendMetronome());
      xml.intTag(level, "automation", int(automationType()));
      xml.doubleTag(level, "gain", _gain);
      if (hasAuxSend()) {
            int naux = MusEGlobal::song->auxs()->size();
            for (int idx = 0; idx < naux; ++idx) {
                  QString s("<auxSend idx=\"%1\">%2</auxSend>\n");  // Aux fix from Remon, thanks.
                  xml.nput(level, s.arg(idx).arg(_auxSend[idx]).toUtf8().constData());
                  }
            }
      for (ciPluginI ip = _efxPipe->begin(); ip != _efxPipe->end(); ++ip) {
            if (*ip)
                  // Write the plugin. Also write the automation controllers and midi mapping
                  //  and strip away the rack position id bits.
                  (*ip)->writeConfiguration(level, xml, true);
            }
      // Range of controllers. (track ctrls -> effect rack plugin ctrls -> synth plugin ctrls).
      const int startId = 0;
      const int   endId = genACnum(0, 0);
      // Write the block of controllers.
      controller()->write(level, xml, startId, endId);

      xml.etag(--level, "AudioTrack");
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
              PluginI* pi = new PluginI();
              // Set the track now, in case anything needs it BEFORE the plugin is set, below.
              pi->setTrack(this);
              if(pi->readConfiguration(xml, false, channels()))
              {
                // Be sure to clear and delete the controller list.
                pi->initialConfiguration()._ctrlListList.clearDelete();
                delete pi;
                pi = nullptr;
              }
              else
              {
                // If a plugin id was read and there's no existing plugin at that rack slot.
                const int conf_id = pi->initialConfiguration()._id;
                if(conf_id >= 0 && conf_id < MusECore::PipelineDepth &&
                  (!(*_efxPipe)[conf_id]))
                {
                  pi->setID(conf_id);
                }
                else
                // No plugin id was read or there's an existing plugin at that rack slot.
                {
                  int rackpos;
                  // Look for an empty rack slot.
                  for(rackpos = 0; rackpos < MusECore::PipelineDepth; ++rackpos)
                  {
                    if(!(*_efxPipe)[rackpos])
                      break;
                  }
                  // Valid empty rack slot found?
                  if(rackpos < MusECore::PipelineDepth)
                  {
                    pi->setID(rackpos);
                  }
                  else
                  // No valid empty rack slot found.
                  {
                    // Be sure to clear and delete the controller list.
                    pi->initialConfiguration()._ctrlListList.clearDelete();
                    delete pi;
                    pi = nullptr;
                    fprintf(stderr, "can't load plugin - plugin rack is already full\n");
                  }
                }

                if(pi)
                {
                  PluginConfiguration &pic = pi->initialConfiguration();
                  (*_efxPipe)[pi->id()] = pi;

                  //---------------------------------------------------------
                  // If any automation controllers were included with in XML,
                  //  convert controller IDs and transfer to given list.
                  // This is for song file versions >= 4
                  //---------------------------------------------------------
                  CtrlListList &conf_cll = pic._ctrlListList;
                  for(ciCtrlList icl = conf_cll.cbegin(); icl != conf_cll.cend(); )
                  {
                    CtrlList *cl = icl->second;
                    // Strip away the controller's rack position bits,
                    //  leaving just the controller numbers.
                    // Still, they should already be stripped by now.
                    const int m = cl->id() & AC_PLUGIN_CTL_ID_MASK;
                    // Generate the new id.
                    const unsigned long new_id = genACnum(pi->id(), m);
                    cl->setId(new_id);
                    // This takes ownership of the controller and will either
                    //  add/transfer it to the controller list, or delete it.
                    // We won't bother calling setupController() afterwards, although we could
                    //  because in song file version >= 4 we know the rack position of the plugin.
                    // We'll just do it in (or after) mapRackPluginsToControllers() for example.
                    if(!addControllerFromXml(cl))
                      fprintf(stderr, "AudioTrack::readProperties: Error: Could not add plugin controller #%ld!\n", new_id);

                    // Done with the item. Erase it. Iterator will point to the next item.
                    icl = conf_cll.erase(icl);
                  }
                  // All of the items should be erased by now.

                  //---------------------------------------------------------
                  // If any midi controller mappings were included with in XML,
                  //  convert controller IDs and transfer to given list.
                  // This is for song file versions >= 4
                  //---------------------------------------------------------
                  MidiAudioCtrlMap *macm = MusEGlobal::song->midiAssignments();
                  MidiAudioCtrlMap &conf_macm = pic._midiAudioCtrlMap;
                  if(macm)
                  {
                    for(iMidiAudioCtrlMap imacm = conf_macm.begin(); imacm != conf_macm.end(); )
                    {
                      MidiAudioCtrlStruct &macs = imacm->second;
                      // Strip away the controller ID's rack position bits,
                      //  leaving just the controller numbers.
                      // Still, they should already be stripped by now.
                      const int m = macs.id() & AC_PLUGIN_CTL_ID_MASK;
                      // Generate the new id.
                      const unsigned long new_id = genACnum(pi->id(), m);
                      macs.setId(new_id);
                      macs.setTrack(this);
                      macm->add_ctrl_struct(imacm->first, macs);
                      // Done with the item. Erase it. Iterator will point to the next item.
                      imacm = conf_macm.erase(imacm);
                    }
                    // All of the items should be erased by now.
                  }
                  else
                  {
                    // Mappings were not transferred. Clear the list.
                    conf_macm.clear();
                  }


              }
            }
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
            if(l->read(xml) && l->id() >= 0)
            {
              // This takes ownership of the controller and will either
              //  add/transfer it to the controller list, or delete it.
              // We cannot call setupController() just yet afterwards, that needs
              //  be done later in (or after) mapRackPluginsToControllers() for example.
              if(!addControllerFromXml(l))
                fprintf(stderr, "AudioTrack::readProperties: Error: Could not add controller #%d!\n", l->id());
            }
            else
            {
              // Controller is orphaned now. Delete it.
              delete l;
            }
          }
      // Obsolete. Handled by class Track now. Keep for compatibility.
      // Tag midiMapper is obsolete, changed to midiAssign now.
      else if (tag == "midiAssign" || tag == "midiMapper")
            // Any assignments read go to this track.
            MusEGlobal::song->midiAssignments()->read(xml, this);

      // Added in song file version 4.
      else if (tag == "Track")
            Track::read(xml);

      // Obsolete. Keep for compatibility.
      else if(xml.isVersionLessThan(4, 0))
            return Track::readProperties(xml, tag);

      return false;
      }

//---------------------------------------------------------
//   read
//---------------------------------------------------------

void AudioTrack::read(Xml& xml)
      {
      for (;;) {
            Xml::Token token = xml.parse();
            const QString& tag = xml.s1();
            switch (token) {
                  case Xml::Error:
                  case Xml::End:
                        return;
                  case Xml::TagStart:
                        if (readProperties(xml, tag))
                              xml.unknown("AudioTrack");
                        break;
                  case Xml::Attribut:
                        break;
                  case Xml::TagEnd:
                        if (tag == "AudioTrack") {
                              return;
                              }
                  default:
                        break;
                  }
            }
      }

//---------------------------------------------------------
//   showPendingPluginGuis
//---------------------------------------------------------

void AudioTrack::showPendingPluginGuis()
{
  for(int idx = 0; idx < MusECore::PipelineDepth; ++idx)
  {
    PluginI* p = (*_efxPipe)[idx];
    if(!p)
      continue;

    if(p->isShowGuiPending())
      p->showGui(true);

    if(p->isShowNativeGuiPending())
      p->showNativeGui(true);
  }
}

void AudioTrack::updateUiWindowTitles()
{
  for(int idx = 0; idx < MusECore::PipelineDepth; ++idx)
  {
    PluginI* p = (*_efxPipe)[idx];
    if(!p)
      continue;

    p->updateGuiWindowTitle();
    p->updateNativeGuiWindowTitle();
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
      if(p->id() != idx)
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

    // Get the number of parameters. If the plugin is missing, use the persistent information.
    const int j = p->plugin() ? p->parameters() : p->initialConfiguration()._initParams.size();

    for(int i = 0; i < j; i++)
    {
      CtrlList *cl;
      int id = genACnum(idx, i);
      ciCtrlList icl = controller()->find(id);
      if(icl == controller()->end())
      {
        cl = new CtrlList(id);
        addController(cl);
      }
      else
      {
        cl = icl->second;
      }

      // Force all of these now, even though they may have already been set. With a pre-
      //  0.9pre1 med file with broken controller sections they may not be set correct.
      setupController(cl);
    }
  }

  // Delete non-existent controllers.
  for(ciCtrlList icl = _controller.cbegin(); icl != _controller.cend(); )
  {
    CtrlList* l = icl->second;
    int id = l->id();
    // Ignore volume, pan, mute etc.
    if(id < AC_PLUGIN_CTL_BASE)
    {
      ++icl;
      continue;
    }

    unsigned param = id & AC_PLUGIN_CTL_ID_MASK;
    int idx = (id >> AC_PLUGIN_CTL_BASE_POW) - 1;

    bool do_remove = false;
    if(idx >= 0 && idx < MusECore::PipelineDepth)
    {
      const PluginI *p = (*_efxPipe)[idx];
      // If there is a PluginI at that slot but its plugin is missing,
      //  do NOT remove the loaded controllers, to preserve data upon re-saving.
      if(!p || (p->plugin() && param >= p->parameters()))
        do_remove = true;
    }
    // Support a special block for synth controllers.
    else if(idx == MusECore::MAX_PLUGINS && type() == AUDIO_SOFTSYNTH)
    {
      const SynthI* synti = static_cast < const SynthI* > (this);
      SynthIF* sif = synti->sif();
      // If the sif is missing, do NOT remove the loaded controllers, to preserve data upon re-saving.
      if(sif)
      {
        if(param >= sif->parameters())
          do_remove = true;
      }
    }

    // If there's no plugin at that rack position, or the param is out of range of
    //  the number of controls in the plugin, then it's a stray controller. Delete it.
    // Future: Leave room for possible bypass controller at AC_PLUGIN_CTL_ID_MASK -1.
    //if(!p || (param >= p->parameters() && (param != AC_PLUGIN_CTL_ID_MASK -1)))
    if(do_remove)
    {
      // C++11.
// REMOVE Tim. tmp. Added comment.
      // FIXME TODO: Shouldn't the list be deleted before removal ???
      icl = _controller.erase(icl);
    }
    else
    {
      ++icl;
    }
  }
}

RouteCapabilitiesStruct AudioTrack::routeCapabilities() const
{
  RouteCapabilitiesStruct s;
  s._trackChannels._inChannels = s._trackChannels._outChannels = totalProcessBuffers();
  s._trackChannels._inRoutable = s._trackChannels._outRoutable = (s._trackChannels._inChannels != 0);
  return s;
}

void AudioTrack::guiHeartBeat()
{
  auto p = efxPipe();
  if(p)
    p->guiHeartBeat();
}

//---------------------------------------------------------
//   AudioInput
//---------------------------------------------------------

AudioInput::AudioInput()
   // Default 1 channel for audio inputs.
   : AudioTrack(AUDIO_INPUT, 1)
      {
      for (int i = 0; i < MusECore::MAX_CHANNELS; ++i)
            jackPorts[i] = 0;
      }

AudioInput::AudioInput(const AudioInput& t, int flags)
  : AudioTrack(t, flags)
{
  for (int i = 0; i < MusECore::MAX_CHANNELS; ++i)
        jackPorts[i] = 0;

  // It is pointless to try to register ports right now since the
  //  track names are currently the same. The registration will fail
  //  due to duplicate port names.
  // Therefore the caller MUST set a unique track name afterwards,
  //  which does succeed at registering the ports.

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
      // Don't call msgAddRoute. Caller later calls msgAddTrack which 'mirrors' this routing node.
      _inRoutes.push_back(*ir);
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
//   selfLatencyAudio
//---------------------------------------------------------

float AudioInput::selfLatencyAudio(int channel) const
{
  float l = AudioTrack::selfLatencyAudio(channel);

  if(!MusEGlobal::checkAudioDevice())
    return l;

  void* jackPort = jackPorts[channel];
  if(jackPort)
    l += MusEGlobal::audioDevice->portLatency(jackPort, true);
  return l;
}

//---------------------------------------------------------
//   getWorstPortLatencyAudio
//---------------------------------------------------------

float AudioInput::getWorstPortLatencyAudio()
{
  // Have we been here before during this scan?
  // Just return the cached value.
  if(_latencyInfo._worstPortLatencyProcessed)
    return _latencyInfo._worstPortLatency;

  float worst_lat = 0.0f;
  // Include any port latencies.
  if(MusEGlobal::checkAudioDevice())
  {
    const int track_out_channels = totalProcessBuffers(); // totalOutChannels();
    for(int i = 0; i < track_out_channels; ++i)
    {
      void* jackPort = jackPorts[i];
      if(jackPort)
      {
        // true = we want the capture latency.
        const float lat = MusEGlobal::audioDevice->portLatency(jackPort, true);
        if(lat > worst_lat)
          worst_lat = lat;
      }
    }
  }
  
  _latencyInfo._worstPortLatency = worst_lat;
  _latencyInfo._worstPortLatencyProcessed = true;
  return _latencyInfo._worstPortLatency;
}

bool AudioInput::canDominateOutputLatency() const
{
  return !off();
}

//---------------------------------------------------------
//   write
//---------------------------------------------------------

void AudioInput::write(int level, Xml& xml, XmlWriteStatistics*) const
      {
      xml.tag(level++, "AudioInput");
      AudioTrack::writeProperties(level, xml);
      xml.etag(--level, "AudioInput");
      }

//---------------------------------------------------------
//   read
//---------------------------------------------------------

void AudioInput::read(Xml& xml, XmlReadStatistics*)
      {
      for (;;) {
            Xml::Token token = xml.parse();
            const QString& tag = xml.s1();
            switch (token) {
                  case Xml::Error:
                  case Xml::End:
                        return;
                  case Xml::TagStart:
                        if(tag == "AudioTrack")
                              AudioTrack::read(xml);

                        // Obsolete. Keep for compatibility.
                        else if (!xml.isVersionLessThan(4, 0) || AudioTrack::readProperties(xml, tag))
                              xml.unknown("AudioInput");
                        break;
                  case Xml::Attribut:
                        break;
                  case Xml::TagEnd:
                        if (tag == "AudioInput") {
                              fixOldColorScheme();
                              registerPorts();  // allocate jack ports
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
      _outputLatencyComp = new LatencyCompensator();

      _nframes = 0;
      for (int i = 0; i < MAX_CHANNELS; ++i)
            jackPorts[i] = 0;
      }

AudioOutput::AudioOutput(const AudioOutput& t, int flags)
  : AudioTrack(t, flags)
{
  _outputLatencyComp = new LatencyCompensator();

  for (int i = 0; i < MusECore::MAX_CHANNELS; ++i)
        jackPorts[i] = 0;
  _nframes = 0;

  // It is pointless to try to register ports right now since the
  //  track names are currently the same. The registration will fail
  //  due to duplicate port names.
  // Therefore the caller MUST set a unique track name afterwards,
  //  which does succeed at registering the ports.

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
      // Don't call msgAddRoute. Caller later calls msgAddTrack which 'mirrors' this routing node.
      _outRoutes.push_back(*ir);
    }
  }
}

//---------------------------------------------------------
//   ~AudioOutput
//---------------------------------------------------------

AudioOutput::~AudioOutput()
      {
      // FIXME Never runs this on close because device is nulled first.
      //       But possibly it's benign because it may already disconnect before it gets here...
      if (MusEGlobal::checkAudioDevice())
      {
        for (int i = 0; i < _channels; ++i)
          if(jackPorts[i])
            MusEGlobal::audioDevice->unregisterPort(jackPorts[i]);
      }

      if(_outputLatencyComp)
        delete _outputLatencyComp;
      }

//---------------------------------------------------------
//   setChannels
//---------------------------------------------------------

void AudioOutput::setChannels(int n)
      {
      AudioTrack::setChannels(n);
      if(useLatencyCorrection() && _outputLatencyComp)
        _outputLatencyComp->setChannels(totalProcessBuffers());
      }

//---------------------------------------------------------
//   selfLatencyAudio
//---------------------------------------------------------

float AudioOutput::selfLatencyAudio(int channel) const
{
  float l = AudioTrack::selfLatencyAudio(channel);

  if(!MusEGlobal::checkAudioDevice())
    return l;

  void* jackPort = jackPorts[channel];
  if(jackPort)
    l += MusEGlobal::audioDevice->portLatency(jackPort, false);
  return l;
}

//---------------------------------------------------------
//   getWorstPortLatencyAudio
//---------------------------------------------------------

float AudioOutput::getWorstPortLatencyAudio()
{
  // Have we been here before during this scan?
  // Just return the cached value.
  if(_latencyInfo._worstPortLatencyProcessed)
    return _latencyInfo._worstPortLatency;

  float worst_lat = 0.0f;
  // Include any port latencies.
  if(MusEGlobal::checkAudioDevice())
  {
    const int track_out_channels = totalProcessBuffers(); // totalOutChannels();
    for(int i = 0; i < track_out_channels; ++i)
    {
      void* jackPort = jackPorts[i];
      if(jackPort)
      {
        // false = we want the playback latency.
        const float lat = MusEGlobal::audioDevice->portLatency(jackPort, false);
        if(lat > worst_lat)
          worst_lat = lat;
      }
    }
  }
  
  _latencyInfo._worstPortLatency = worst_lat;
  _latencyInfo._worstPortLatencyProcessed = true;
  return _latencyInfo._worstPortLatency;
}

bool AudioOutput::isLatencyInputTerminal()
{
  // Have we been here before during this scan?
  // Just return the cached value.
  if(_latencyInfo._isLatencyInputTerminalProcessed)
    return _latencyInfo._isLatencyInputTerminal;

  _latencyInfo._isLatencyInputTerminal = true;
  _latencyInfo._isLatencyInputTerminalProcessed = true;
  return true;
}
bool AudioOutput::isLatencyOutputTerminal()
{ 
  // Have we been here before during this scan?
  // Just return the cached value.
  if(_latencyInfo._isLatencyOutputTerminalProcessed)
    return _latencyInfo._isLatencyOutputTerminal;

  _latencyInfo._isLatencyOutputTerminal = true;
  _latencyInfo._isLatencyOutputTerminalProcessed = true;
  return true;
}

//---------------------------------------------------------
//   applyOutputLatencyComp
//---------------------------------------------------------

void AudioOutput::applyOutputLatencyComp(unsigned nframes)
{
  if(!useLatencyCorrection() || !_outputLatencyComp)
    return;

  // Include any port latencies.
  if(MusEGlobal::checkAudioDevice())
  {
    // We want the audio output track's worst port latency.
    //const TrackLatencyInfo& li = getLatencyInfo(false /*output*/);
    //const float route_worst_case_latency = li._worstPortLatency;
    const float port_worst_case_latency = getWorstPortLatencyAudio();

    // 'buffer' is only MAX_CHANNELS deep, unlike some of our allocated audio buffers.
    const int track_out_channels = MusECore::MAX_CHANNELS; //totalProcessBuffers(); // totalOutChannels();
    for(int i = 0; i < track_out_channels; ++i)
    {
      if(!buffer[i])
        continue;
      // Prepare the latency value to be passed to the compensator's writer,
      //  by adjusting each channel latency value. ie. the channel with the worst-case
      //  latency will get ZERO delay, while channels having smaller latency will get
      //  MORE delay, to match all the signal timings together.
      void* jackPort = jackPorts[i];
      if(jackPort)
      {
        // false = we want the playback latency.
        const float lat = port_worst_case_latency - MusEGlobal::audioDevice->portLatency(jackPort, false);
        unsigned long offset = 0;
        if((long int)lat > 0)
          offset = lat;

        // Write the channel buffer to the latency compensator.
        // It will be read back later, in-place.
        _outputLatencyComp->write(i, nframes, offset /* + latencyCompWriteOffset() */, buffer[i]);
        // Read back the latency compensated signal, using the channel buffer in-place.
        _outputLatencyComp->read(i, nframes, buffer[i]);
      }
    }
  }
}

//---------------------------------------------------------
//   write
//---------------------------------------------------------

void AudioOutput::write(int level, Xml& xml, XmlWriteStatistics*) const
      {
      xml.tag(level++, "AudioOutput");
      AudioTrack::writeProperties(level, xml);
      xml.etag(--level, "AudioOutput");
      }

//---------------------------------------------------------
//   read
//---------------------------------------------------------

void AudioOutput::read(Xml& xml, XmlReadStatistics*)
      {
      for (;;) {
            Xml::Token token = xml.parse();
            const QString& tag = xml.s1();
            switch (token) {
                  case Xml::Error:
                  case Xml::End:
                        return;
                  case Xml::TagStart:
                        if(tag == "AudioTrack")
                              AudioTrack::read(xml);

                        // Obsolete. Keep for compatibility.
                        else if (!xml.isVersionLessThan(4, 0) || AudioTrack::readProperties(xml, tag))
                              xml.unknown("AudioOutput");
                        break;
                  case Xml::Attribut:
                        break;
                  case Xml::TagEnd:
                        if (tag == "AudioOutput") {
                              fixOldColorScheme();
                              registerPorts();  // allocate jack ports
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

void AudioGroup::write(int level, Xml& xml, XmlWriteStatistics*) const
      {
      xml.tag(level++, "AudioGroup");
      AudioTrack::writeProperties(level, xml);
      xml.etag(--level, "AudioGroup");
      }

//---------------------------------------------------------
//   read
//---------------------------------------------------------

void AudioGroup::read(Xml& xml, XmlReadStatistics*)
      {
      for (;;) {
            Xml::Token token = xml.parse();
            const QString& tag = xml.s1();
            switch (token) {
                  case Xml::Error:
                  case Xml::End:
                        return;
                  case Xml::TagStart:
                        if(tag == "AudioTrack")
                              AudioTrack::read(xml);

                        // Obsolete. Keep for compatibility.
                        else if (!xml.isVersionLessThan(4, 0) || AudioTrack::readProperties(xml, tag))
                              xml.unknown("AudioGroup");
                        break;
                  case Xml::Attribut:
                        break;
                  case Xml::TagEnd:
                        if (tag == "AudioGroup")
                        {
                              fixOldColorScheme();
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

void AudioAux::write(int level, Xml& xml, XmlWriteStatistics*) const
      {
      xml.tag(level++, "AudioAux");
      AudioTrack::writeProperties(level, xml);
      xml.intTag(level, "index", _index);
      xml.etag(--level, "AudioAux");
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
          if(buffer[i] == nullptr)
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
          if(buffer[i] == nullptr)
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

void AudioAux::read(Xml& xml, XmlReadStatistics*)
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
                      else if(tag == "AudioTrack")
                        AudioTrack::read(xml);

                      // Obsolete. Keep for compatibility.
                      else if (!xml.isVersionLessThan(4, 0) || AudioTrack::readProperties(xml, tag))
                              xml.unknown("AudioAux");
                        break;
                  case Xml::Attribut:
                        break;
                  case Xml::TagEnd:
                        if (tag == "AudioAux")
                        {
                              fixOldColorScheme();
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
      if(off())
        return false;

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
  const int old_chans = channels();
  AudioTrack::setChannels(n);
  const int new_chans = channels();
  if(new_chans > old_chans)
  {
    for(int i = old_chans; i < new_chans; ++i)
    {
#ifdef _WIN32
      buffer[i] = (float *) _aligned_malloc(16, sizeof(float) * MusEGlobal::segmentSize);
      if(buffer[i] == nullptr)
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
  else if(new_chans < old_chans)
  {
    for(int i = new_chans; i < old_chans; ++i)
    {
      if(buffer[i])
        free(buffer[i]);
    }
  }
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
              setRecFile(nullptr);

              QFile::remove(s);
              if(MusEGlobal::debugMsg)
                printf("AudioNode::setRecordFlag1: remove file %s if it exists\n", s.toLocal8Bit().constData());
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
        printf("prepareRecording for track %s\n", name().toLocal8Bit().constData());

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
          printf("AudioTrack::prepareRecording: init internal file %s\n", _recFile->path().toLocal8Bit().constData());

      if(_recFile->openWrite())
            {
            QMessageBox::critical(nullptr, "MusE write error.", "Error creating target wave file\n"
                                                            "Check your configuration.");
            return false;

            }

      // For bounce operations: Reset these.
      _recFilePos = 0;
      _previousLatency = 0.0f;

      return true;
}
double AudioTrack::auxSend(int idx) const
      {
      if (unsigned(idx) >= _auxSend.size()) {
            printf("%s auxSend: bad index: %d >= %zd\n",
               name().toLocal8Bit().constData(), idx, _auxSend.size());
            return 0.0;
            }
      return _auxSend[idx];
      }

void AudioTrack::setAuxSend(int idx, double v)
      {
      if (unsigned(idx) >= _auxSend.size()) {
            printf("%s setAuxSend: bad index: %d >= %zd\n",
               name().toLocal8Bit().constData(), idx, _auxSend.size());
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
