//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: audiotrack.cpp,v 1.14.2.21 2009/12/20 05:00:35 terminator356 Exp $
//
//  (C) Copyright 2004 Werner Schweer (ws@seh.de)
//=========================================================

#include <values.h>
#include <stdlib.h>
#include <map>

#include <QMessageBox>

#include "track.h"
#include "event.h"
#include "song.h"
#include "audio.h"
#include "wave.h"
#include "xml.h"
#include "plugin.h"
#include "audiodev.h"

// By T356. For caching jack in/out routing names BEFORE file save. 
// Jack often shuts down during file save, causing the routes to be lost in the file.
// cacheJackRouteNames() is ONLY called from MusE::save() in app.cpp
// Update: Not required any more because the real problem was Jack RT priority, which has been fixed.
/*
typedef std::multimap <const int, QString> jackRouteNameMap;
std::map <const AudioTrack*, jackRouteNameMap > jackRouteNameCache;
typedef std::multimap <const int, QString>::const_iterator ciJackRouteNameMap;
typedef std::map <const AudioTrack*, jackRouteNameMap>::const_iterator ciJackRouteNameCache;
void cacheJackRouteNames()
{
    jackRouteNameCache.clear();
    const InputList* il = song->inputs();
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
    const OutputList* ol = song->outputs();
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
//   AudioTrack
//---------------------------------------------------------

AudioTrack::AudioTrack(TrackType t)
//AudioTrack::AudioTrack(TrackType t, int num_out_bufs)
   : Track(t)
      {
      //_totalOutChannels = num_out_bufs; // Is either parameter-default MAX_CHANNELS, or custom value passed (used by syntis).
      _processed = false;
      _haveData = false;
      _sendMetronome = false;
      _prefader = false;
      _efxPipe  = new Pipeline();
      _recFile  = 0;
      _channels = 0;
      _automationType = AUTO_OFF;
      //setChannels(1);
      setChannels(2);
      addController(new CtrlList(AC_VOLUME,"Volume",0.0,1.0));
      addController(new CtrlList(AC_PAN, "Pan", -1.0, 1.0));
      addController(new CtrlList(AC_MUTE,"Mute",0.0,1.0));
      
      // Changed by Tim. p3.3.15
      //outBuffers = new float*[MAX_CHANNELS];
      //for (int i = 0; i < MAX_CHANNELS; ++i)
      //      outBuffers[i] = new float[segmentSize];
      //for (int i = 0; i < MAX_CHANNELS; ++i)
      //      posix_memalign((void**)(outBuffers + i), 16, sizeof(float) * segmentSize);
      
      // Let's allocate it all in one block, and just point the remaining buffer pointers into the block
      //  which allows faster one-shot buffer copying.
      // Nope. Nice but interferes with possibility we don't know if other buffers are contiguous (jack buffers, local stack buffers etc.).
      //posix_memalign((void**)(outBuffers), 16, sizeof(float) * segmentSize * MAX_CHANNELS);
      //for (int i = 0; i < MAX_CHANNELS; ++i)
      //  *(outBuffers + i) = sizeof(float) * segmentSize * i;
            
      // p3.3.38
      // Easy way, less desirable... Start out with enough for MAX_CHANNELS. Then multi-channel syntis can re-allocate, 
      //  via a call to (a modified!) setChannels().
      // Hard way, more desirable... Creating a synti instance passes the total channels to this constructor, overriding MAX_CHANNELS.
      _totalOutChannels = MAX_CHANNELS;
      outBuffers = new float*[_totalOutChannels];
      for (int i = 0; i < _totalOutChannels; ++i)
            posix_memalign((void**)&outBuffers[i], 16, sizeof(float) * segmentSize);
      
      // This is only set by multi-channel syntis...
      _totalInChannels = 0;
      
      bufferPos = MAXINT;
      
      setVolume(1.0);
      }

//AudioTrack::AudioTrack(const AudioTrack& t)
//  : Track(t)
AudioTrack::AudioTrack(const AudioTrack& t, bool cloneParts)
  : Track(t, cloneParts)
      {
      _totalOutChannels = t._totalOutChannels; // Is either MAX_CHANNELS, or custom value (used by syntis).
      _processed      = false;
      _haveData       = false;
      _sendMetronome  = t._sendMetronome;
      _controller     = t._controller;
      _prefader       = t._prefader;
      _auxSend        = t._auxSend;
      _efxPipe        = new Pipeline(*(t._efxPipe));
      _automationType = t._automationType;
      _inRoutes       = t._inRoutes;
      _outRoutes      = t._outRoutes;
      // Changed by Tim. p3.3.15
      //outBuffers = new float*[MAX_CHANNELS];
      //for (int i = 0; i < MAX_CHANNELS; ++i)
      //      outBuffers[i] = new float[segmentSize];
      //for (int i = 0; i < MAX_CHANNELS; ++i)
      //      posix_memalign((void**)(outBuffers + i), 16, sizeof(float) * segmentSize);
      
      // p3.3.38
      int chans = _totalOutChannels;
      // Number of allocated buffers is always MAX_CHANNELS or more, even if _totalOutChannels is less. 
      if(chans < MAX_CHANNELS)
        chans = MAX_CHANNELS;
      outBuffers = new float*[chans];
      for (int i = 0; i < chans; ++i)
            posix_memalign((void**)&outBuffers[i], 16, sizeof(float) * segmentSize);
      
      bufferPos = MAXINT;
      _recFile  = t._recFile;
      }

AudioTrack::~AudioTrack()
{
      delete _efxPipe;
      //for (int i = 0; i < MAX_CHANNELS; ++i)
      //      delete[] outBuffers[i];
      //delete[] outBuffers;
      
      // p3.3.15
      //for(int i = 0; i < MAX_CHANNELS; ++i) 
      //{
      //  if(outBuffers[i])
      //    free(outBuffers[i]);
      //}
      
      // p3.3.38
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
    for(int i = 0; i < PipelineDepth; i++)
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
      CtrlValueType t = plugin->valueType();
      CtrlList* cl = new CtrlList(id);
      cl->setRange(min, max);
      cl->setName(QString(name));
      cl->setValueType(t);
      LADSPA_PortRangeHint range = plugin->range(i);
      if(LADSPA_IS_HINT_TOGGLED(range.HintDescriptor))
        cl->setMode(CtrlList::DISCRETE);
      else  
        cl->setMode(CtrlList::INTERPOLATE);
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
  // FIXME This code is ugly.
  // At best we would like to modify the keys (IDXs) in-place and
  //  do some kind of deferred re-sort, but it can't be done...
  
  if(idx1 == idx2)
    return;
    
  if(idx1 < 0 || idx2 < 0 || idx1 >= PipelineDepth || idx2 >= PipelineDepth)
    return;
  
  CtrlList *cl;
  CtrlList *newcl;
  int id1 = (idx1 + 1) * AC_PLUGIN_CTL_BASE;
  int id2 = (idx2 + 1) * AC_PLUGIN_CTL_BASE;
  int i, j;
  
  CtrlListList tmpcll;
  CtrlVal cv(0, 0.0);
  
  for(ciCtrlList icl = _controller.begin(); icl != _controller.end(); ++icl) 
  {
    cl = icl->second;
    i = cl->id() & AC_PLUGIN_CTL_ID_MASK;
    j = cl->id() & ~((unsigned long)AC_PLUGIN_CTL_ID_MASK);
    if(j == id1 || j == id2)
    {
      newcl = new CtrlList(i | (j == id1 ? id2 : id1));
      newcl->setMode(cl->mode());
      newcl->setValueType(cl->valueType());
      newcl->setName(cl->name());
      double min, max;
      cl->range(&min, &max);
      newcl->setRange(min, max);
      newcl->setCurVal(cl->curVal());
      newcl->setDefault(cl->getDefault());
      for(iCtrl ic = cl->begin(); ic != cl->end(); ++ic) 
      {
        cv = ic->second;
        newcl->insert(std::pair<const int, CtrlVal>(cv.frame, cv));
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
  
  
  /*
  unsigned int idmask = ~AC_PLUGIN_CTL_ID_MASK;
  
  CtrlList* cl;
  CtrlList* ctl1 = 0;
  CtrlList* ctl2 = 0;
  CtrlList* newcl1 = 0;
  CtrlList* newcl2 = 0;
  CtrlVal cv(0, 0.0);
  int id1 = (idx1 + 1) * AC_PLUGIN_CTL_BASE;
  int id2 = (idx2 + 1) * AC_PLUGIN_CTL_BASE;
  int i, j;
  double min, max;
  
  for(ciCtrlList icl = _controller.begin(); icl != _controller.end(); ++icl) 
  {
    cl = icl->second;
    i = cl->id() & AC_PLUGIN_CTL_ID_MASK;
    j = cl->id() & idmask;
    
    if(j == id1)
    {
      ctl1 = cl;
      newcl1 = new CtrlList( i | id2 );
      newcl1->setMode(cl->mode());
      newcl1->setValueType(cl->valueType());
      newcl1->setName(cl->name());
      cl->range(&min, &max);
      newcl1->setRange(min, max);
      newcl1->setCurVal(cl->curVal());
      newcl1->setDefault(cl->getDefault());
      for(iCtrl ic = cl->begin(); ic != cl->end(); ++ic) 
      {
        cv = ic->second;
        newcl1->insert(std::pair<const int, CtrlVal>(cv.frame, cv));
      }
    }
    //else  
    if(j == id2)
    {
      ctl2 = cl;
      newcl2 = new CtrlList( i | id1 );
      newcl2->setMode(cl->mode());
      newcl2->setValueType(cl->valueType());
      newcl2->setName(cl->name());
      cl->range(&min, &max);
      newcl2->setRange(min, max);
      newcl2->setCurVal(cl->curVal());
      newcl2->setDefault(cl->getDefault());
      for(iCtrl ic = cl->begin(); ic != cl->end(); ++ic) 
      {
        cv = ic->second;
        newcl2->insert(std::pair<const int, CtrlVal>(cv.frame, cv));
      }
    }
  }  
  if(ctl1)
    _controller.erase(ctl1->id());
  if(ctl2)
    _controller.erase(ctl2->id());
  if(newcl1)
    //_controller.add(newcl1);
    _controller.insert(std::pair<const int, CtrlList*>(newcl1->id(), newcl1));
  if(newcl2)
    _controller.insert(std::pair<const int, CtrlList*>(newcl2->id(), newcl2));
    //_controller.add(newcl2);
  */  
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

void AudioTrack::processAutomationEvents() 
{ 
  if (_automationType != AUTO_TOUCH && _automationType != AUTO_WRITE)
        return;
  
  for (iCtrlList icl = _controller.begin(); icl != _controller.end(); ++icl) 
  {
    CtrlList* cl = icl->second;
    int id = cl->id();
    
    // Remove old events from record region.
    if (_automationType == AUTO_WRITE) 
    {
      int start = audio->getStartRecordPos().frame();
      int end   = audio->getEndRecordPos().frame();
      iCtrl   s = cl->lower_bound(start);
      iCtrl   e = cl->lower_bound(end);
      
      // Erase old events only if there were recorded events.
      for(iCtrlRec icr = _recEvents.begin(); icr != _recEvents.end(); ++icr) 
      {
        if(icr->id == id) // && icr->type == ARVT_VAL && icr->frame >= s->frame && icr->frame <= e->frame)
        {
          cl->erase(s, e);
          break;
        } 
      }
    }
    else 
    {  // type AUTO_TOUCH
      for (iCtrlRec icr = _recEvents.begin(); icr != _recEvents.end(); ++icr) 
      {
        // Don't bother looking for start, it's OK, just take the first one.
        // Needed for mousewheel and paging etc.
        //if (icr->id == id && icr->type == ARVT_START) 
        if (icr->id == id) 
        {
          int start = icr->frame;
          
          if(icr == _recEvents.end())
          {
            int end = audio->getEndRecordPos().frame();
            iCtrl s = cl->lower_bound(start);
            iCtrl e = cl->lower_bound(end);
            cl->erase(s, e);
            break;
          }
          
          iCtrlRec icrlast = icr;
          ++icr;
          for(; ; ++icr) 
          {
            if(icr == _recEvents.end())
            {
              int end = icrlast->frame;
              iCtrl s = cl->lower_bound(start);
              iCtrl e = cl->lower_bound(end);
              cl->erase(s, e);
              break;
            }
            
            if(icr->id == id && icr->type == ARVT_STOP) 
            {
              int end = icr->frame;
              // Erase everything up to, not including, this stop event's frame.
              // Because an event was already stored directly when slider released.
              if(end > start)
                --end;
                  
              iCtrl s = cl->lower_bound(start);
              iCtrl e = cl->lower_bound(end);
              
              cl->erase(s, e);
              
              break;
            }
              
            if(icr->id == id)
              icrlast = icr;
          }
          if (icr == _recEvents.end())
                break;
        }
      }
    }
    
    // Extract all recorded events for controller "id"
    //  from CtrlRecList and put into cl.
    for (iCtrlRec icr = _recEvents.begin(); icr != _recEvents.end(); ++icr) 
    {
          if (icr->id == id && (icr->type == ARVT_VAL || icr->type == ARVT_START))
                cl->add(icr->frame, icr->val);
    }
  }
  
  // Done with the recorded automation event list. Clear it.
  _recEvents.clear();
        
  // Try muse without this, so that the user can remain in automation write mode
  //  after a stop. 
  /*
  if (automationType() == AUTO_WRITE)
    {
        setAutomationType(AUTO_READ);
        song->update(SC_AUTOMATION);
    }     
  */
  
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
  if(icl == _controller.end())
    return;
  
    CtrlList* cl = icl->second;
    if(cl->empty())
      return;
    
    iCtrl s = cl->lower_bound(song->cPos().frame());
    if(s != cl->begin())
      --s;
    song->setPos(Song::CPOS, Pos(s->second.frame, false), true, false, true);
    return;
}

//---------------------------------------------------------
//   seekNextACEvent
//---------------------------------------------------------

void AudioTrack::seekNextACEvent(int id)
{
  ciCtrlList icl = _controller.find(id);
  if(icl == _controller.end())
    return;
  
    CtrlList* cl = icl->second;
    if(cl->empty())
      return;
    
    iCtrl s = cl->upper_bound(song->cPos().frame());
    if(s == cl->end())
    {
      --s;
    }
    
    song->setPos(Song::CPOS, Pos(s->second.frame, false), true, false, true);
    return;  
}

//---------------------------------------------------------
//   eraseACEvent
//---------------------------------------------------------

void AudioTrack::eraseACEvent(int id, int frame)
{
  ciCtrlList icl = _controller.find(id);
  if(icl == _controller.end())
    return;
  
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
  if(icl == _controller.end())
    return;
  
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
  if(icl == _controller.end())
    return;
  
    CtrlList* cl = icl->second;
    
    // Add will replace if found.
    cl->add(frame, val);
    return;  
}

//---------------------------------------------------------
//   volume
//---------------------------------------------------------

double AudioTrack::volume() const
      {
      ciCtrlList cl = _controller.find(AC_VOLUME);
      if (cl == _controller.end())
            return 0.0;
      
      if (automation && 
          automationType() != AUTO_OFF && _volumeEnCtrl && _volumeEn2Ctrl )
            return cl->second->value(song->cPos().frame());
      else
            return cl->second->curVal();
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
      ciCtrlList cl = _controller.find(AC_PAN);
      if (cl == _controller.end())
            return 0.0;
      
      if (automation && 
          automationType() != AUTO_OFF && _panEnCtrl && _panEn2Ctrl )
        return cl->second->value(song->cPos().frame());
      else
        return cl->second->curVal();
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
//   pluginCtrlVal
//---------------------------------------------------------

double AudioTrack::pluginCtrlVal(int ctlID) const
      {
      ciCtrlList cl = _controller.find(ctlID);
      if (cl == _controller.end())
            return 0.0;
      
      if (automation && (automationType() != AUTO_OFF))
        return cl->second->value(song->cPos().frame());
      else
        return cl->second->curVal();
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
      
void AudioTrack::recordAutomation(int n, double v)
      {
        if(!automation)
          return;
        if(audio->isPlaying())
          _recEvents.push_back(CtrlRecVal(song->cPos().frame(), n, v));
        else 
        {
          if(automationType() == AUTO_WRITE)
            _recEvents.push_back(CtrlRecVal(song->cPos().frame(), n, v));
          else 
          if(automationType() == AUTO_TOUCH)
          // In touch mode and not playing. Send directly to controller list.
          {
            iCtrlList cl = _controller.find(n);
            if (cl == _controller.end()) 
              return;
            // Add will replace if found.
            cl->second->add(song->cPos().frame(), v);
          }  
        }
      }

void AudioTrack::startAutoRecord(int n, double v)
      {
        if(!automation)
          return;
        if(audio->isPlaying())
        {
          if(automationType() == AUTO_TOUCH)
              _recEvents.push_back(CtrlRecVal(song->cPos().frame(), n, v, ARVT_START));
          else    
          if(automationType() == AUTO_WRITE)
              _recEvents.push_back(CtrlRecVal(song->cPos().frame(), n, v));
        } 
        else
        {
          if(automationType() == AUTO_TOUCH)
          // In touch mode and not playing. Send directly to controller list.
          {
            iCtrlList cl = _controller.find(n);
            if (cl == _controller.end()) 
              return;
            // Add will replace if found.
            cl->second->add(song->cPos().frame(), v);
          }    
          else    
          if(automationType() == AUTO_WRITE)
            _recEvents.push_back(CtrlRecVal(song->cPos().frame(), n, v));
        }   
      }

void AudioTrack::stopAutoRecord(int n, double v)
      {
        if(!automation)
          return;
        if(audio->isPlaying())
        {
          if(automationType() == AUTO_TOUCH)
          {
              audio->msgAddACEvent(this, n, song->cPos().frame(), v);
              _recEvents.push_back(CtrlRecVal(song->cPos().frame(), n, v, ARVT_STOP));
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
      if (hasAuxSend()) {
            int naux = song->auxs()->size();
            for (int idx = 0; idx < naux; ++idx) {
                  QString s("<auxSend idx=%1>%2</auxSend>\n");
                  xml.nput(level, s.arg(idx).arg(_auxSend[idx]).toAscii().constData());
                  }
            }
      for (ciPluginI ip = _efxPipe->begin(); ip != _efxPipe->end(); ++ip) {
            if (*ip)
                  (*ip)->writeConfiguration(level, xml);
            }
      for (ciCtrlList icl = _controller.begin(); icl != _controller.end(); ++icl) {
            const CtrlList* cl = icl->second;
            QString s("controller id=\"%1\" cur=\"%2\"");
            xml.tag(level++, s.arg(cl->id()).arg(cl->curVal()).toAscii().constData());
            int i = 0;
            for (ciCtrl ic = cl->begin(); ic != cl->end(); ++ic) {
                  QString s("%1 %2, ");
                  xml.nput(level, s.arg(ic->second.frame).arg(ic->second.val).toAscii().constData());
                  ++i;
                  if (i >= 4) {
                        xml.put(level, "");
                        i = 0;
                        }
                  }
            if (i)
                  xml.put(level, "");
            xml.etag(level--, "controller");
            }
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
            for(rackpos = 0; rackpos < PipelineDepth; ++rackpos) 
            {
              if(!(*_efxPipe)[rackpos]) 
                break;
            }
            if(rackpos < PipelineDepth)
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
      else if (tag == "automation")
            setAutomationType(AutomationType(xml.parseInt()));
      // Removed by T356
      // "recfile" tag not saved anymore
      //else if (tag == "recfile")
      //      readRecfile(xml);
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
            PluginI* p = 0;
            bool ctlfound = false;
            int m = l->id() & AC_PLUGIN_CTL_ID_MASK;
            int n = (l->id() >> AC_PLUGIN_CTL_BASE_POW) - 1;
            if(n >= 0 && n < PipelineDepth)
            {
              p = (*_efxPipe)[n];
              if(p && m < p->parameters())
                  ctlfound = true;
            }
            
            iCtrlList icl = _controller.find(l->id());
            if (icl == _controller.end())
                  _controller.add(l);
            else {
                  CtrlList* d = icl->second;
                  for (iCtrl i = l->begin(); i != l->end(); ++i)
                        d->insert(std::pair<const int, CtrlVal> (i->first, i->second));
                  
                  if(!ctlfound)
                  d->setCurVal(l->curVal());
                  
                  d->setDefault(l->getDefault());
                  delete l;
                  l = d;
                  }
            
              if(ctlfound)
                {
                  l->setCurVal(p->param(m));
                  LADSPA_PortRangeHint range = p->range(m);
                  if(LADSPA_IS_HINT_TOGGLED(range.HintDescriptor))
                    l->setMode(CtrlList::DISCRETE);
                  else  
                    l->setMode(CtrlList::INTERPOLATE);
                } 
            }
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
  for(int idx = 0; idx < PipelineDepth; ++idx)
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
  for(int idx = PipelineDepth - 1; idx >= 0; idx--)
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
  for(int idx = 0; idx < PipelineDepth; idx++)
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
      CtrlValueType t = p->valueType();
      l->setRange(min, max);
      l->setName(QString(p->paramName(i)));
      l->setValueType(t);
      LADSPA_PortRangeHint rh = p->range(i);
      if(LADSPA_IS_HINT_TOGGLED(rh.HintDescriptor))
        l->setMode(CtrlList::DISCRETE);
      else  
        l->setMode(CtrlList::INTERPOLATE);
      l->setCurVal(p->param(i));
      //l->setDefault(p->defaultValue(i));
    }  
  }
  
  // The loop is a safe way to delete while iterating 'non-linear' lists.
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
      int param = id & AC_PLUGIN_CTL_ID_MASK;
      int idx = (id >> AC_PLUGIN_CTL_BASE_POW) - 1;
      PluginI* p = (*_efxPipe)[idx];
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
    
    
    // Although this tested OK, and is the 'official' way to erase while iterating,
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

/*
//---------------------------------------------------------
//   writeRouting
//---------------------------------------------------------

void AudioTrack::writeRouting(int level, Xml& xml) const
{
      QString n;
      if (type() == Track::AUDIO_INPUT) {
                ciJackRouteNameCache circ = jackRouteNameCache.find(this);
                if(circ != jackRouteNameCache.end())
                {
                  jackRouteNameMap rm = circ->second;
                  for(ciJackRouteNameMap cirm = rm.begin(); cirm != rm.end(); ++cirm)
                  {
                    n = cirm->second;
                    if(!n.isEmpty())
                    {
                      Route dst(name(), true, cirm->first);
                      xml.tag(level++, "Route");
                      xml.strTag(level, "srcNode", n);
                      xml.strTag(level, "dstNode", dst.name());
                      xml.etag(level--, "Route");
                    }  
                  }  
                }
            }
      if(type() == Track::AUDIO_OUTPUT) 
      {
        ciJackRouteNameCache circ = jackRouteNameCache.find(this);
        if(circ != jackRouteNameCache.end())
        {
          jackRouteNameMap rm = circ->second;
          for(ciJackRouteNameMap cirm = rm.begin(); cirm != rm.end(); ++cirm)
          {
            n = cirm->second;
            if(!n.isEmpty())
            {
              Route src(name(), false, cirm->first);
              xml.tag(level++, "Route");
              xml.strTag(level, "srcNode", src.name());
              xml.strTag(level, "dstNode", n);
              xml.etag(level--, "Route");
            }  
          }  
        }
      }
      else
      {
        const RouteList* rl = &_outRoutes;
        for (ciRoute r = rl->begin(); r != rl->end(); ++r) {
            if(!r->name().isEmpty())
            {
              xml.tag(level++, "Route");
              xml.strTag(level, "srcNode", name());
              xml.strTag(level, "dstNode", r->name());
              xml.etag(level--, "Route");
            }
        }  
      }  
}
*/       
       
//---------------------------------------------------------
//   AudioInput
//---------------------------------------------------------

AudioInput::AudioInput()
   : AudioTrack(AUDIO_INPUT)
      {
      // set Default for Input Ports:
      _mute = true;
      //setVolume(1.0);
      for (int i = 0; i < MAX_CHANNELS; ++i)
            jackPorts[i] = 0;
      //_channels = 0;
      //setChannels(2);
      }

//AudioInput::AudioInput(const AudioInput& t)
//  : AudioTrack(t)
AudioInput::AudioInput(const AudioInput& t, bool cloneParts)
  : AudioTrack(t, cloneParts)
      {
      for (int i = 0; i < MAX_CHANNELS; ++i)
            jackPorts[i] = t.jackPorts[i];
      }

//---------------------------------------------------------
//   ~AudioInput
//---------------------------------------------------------

AudioInput::~AudioInput()
      {
      if (!checkAudioDevice()) return;
      for (int i = 0; i < _channels; ++i) 
          if(jackPorts[i])
            audioDevice->unregisterPort(jackPorts[i]);
      }

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

//---------------------------------------------------------
//   AudioOutput
//---------------------------------------------------------

AudioOutput::AudioOutput()
   : AudioTrack(AUDIO_OUTPUT)
      {
      for (int i = 0; i < MAX_CHANNELS; ++i)
            jackPorts[i] = 0;
      //_channels = 0;
      //setChannels(2);
      }

//AudioOutput::AudioOutput(const AudioOutput& t)
//  : AudioTrack(t)
AudioOutput::AudioOutput(const AudioOutput& t, bool cloneParts)
  : AudioTrack(t, cloneParts)
      {
      for (int i = 0; i < MAX_CHANNELS; ++i)
            jackPorts[i] = t.jackPorts[i];
      _nframes = t._nframes;
      }

//---------------------------------------------------------
//   ~AudioOutput
//---------------------------------------------------------

AudioOutput::~AudioOutput()
      {
      if (!checkAudioDevice()) return;
      for (int i = 0; i < _channels; ++i)
          if(jackPorts[i])
            audioDevice->unregisterPort(jackPorts[i]);
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
      xml.etag(level, "AudioAux");
      }

//---------------------------------------------------------
//   AudioAux
//---------------------------------------------------------

AudioAux::AudioAux()
   : AudioTrack(AUDIO_AUX)
{
      //_channels = 0;
      //setChannels(2);
      // Changed by Tim. p3.3.15
      //for (int i = 0; i < MAX_CHANNELS; ++i)
      //      buffer[i] = (i < channels()) ? new float[segmentSize] : 0;
      for(int i = 0; i < MAX_CHANNELS; ++i)
      {
        if(i < channels())
          posix_memalign((void**)(buffer + i), 16, sizeof(float) * segmentSize);
        else
          buffer[i] = 0;
      }  
}

//---------------------------------------------------------
//   AudioAux
//---------------------------------------------------------

AudioAux::~AudioAux()
      {
      // Changed by Tim. p3.3.15
      //for (int i = 0; i < channels(); ++i)
      //      delete[] buffer[i];
      for (int i = 0; i < MAX_CHANNELS; ++i) {
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
                        if (AudioTrack::readProperties(xml, tag))
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

bool AudioAux::getData(unsigned /*pos*/, int ch, unsigned /*samples*/, float** data)
      {
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
    // Changed by Tim. p3.3.15
    //for (int i = channels(); i < n; ++i)
    //      buffer[i] = new float[segmentSize];
    for(int i = channels(); i < n; ++i)
      posix_memalign((void**)(buffer + i), 16, sizeof(float) * segmentSize);
  }
  else if(n < channels()) 
  {
    // Changed by Tim. p3.3.15
    //for (int i = n; i < channels(); ++i)
    //      delete[] buffer[i];
    for(int i = n; i < channels(); ++i) 
    {
      if(buffer[i])
        free(buffer[i]);
    }
  }
  AudioTrack::setChannels(n);
}

//---------------------------------------------------------
//   setRecordFlag1
//    gui part (executed in gui thread)
//---------------------------------------------------------

bool AudioTrack::setRecordFlag1(bool f)
      {
      if (f == _recordFlag)
            return true;
      if (f) {
            if (_recFile == 0) {
                  //
                  // create soundfile for recording
                  //
                  char buffer[128];
                  QFile fil;
                  for (;;++recFileNumber) {
                     sprintf(buffer, "%s/rec%d.wav",
                        museProject.toLatin1().constData(),
                        recFileNumber);
                     fil.setFileName(QString(buffer));
                     if (!fil.exists())
                        break;
                        }
                  _recFile = new SndFile(QString(buffer));
                  _recFile->setFormat(
                     SF_FORMAT_WAV | SF_FORMAT_FLOAT,
                     _channels, sampleRate);
                  }
            if(_recFile->openWrite())
                  {
                  QMessageBox::critical(NULL, "MusE write error.", "Error creating target wave file\n" 
                                                                  "Check your configuration.");
                  return false;

                  }
            if (debugMsg)
                  printf("AudioNode::setRecordFlag1: create internal file %s\n",
                     _recFile->path().toLatin1().constData());
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
              // Added by Tim. p3.3.8
              delete _recFile;
              setRecFile(0);
              
              remove(s.toLatin1().constData());
              if(debugMsg)
                printf("AudioNode::setRecordFlag1: remove file %s\n", s.toLatin1().constData());
              //_recFile = 0;
                  }
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

