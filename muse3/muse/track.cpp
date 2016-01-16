//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: track.cpp,v 1.34.2.11 2009/11/30 05:05:49 terminator356 Exp $
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

#include "track.h"
#include "event.h"
#include "mididev.h"
#include "midiport.h"
#include "song.h"
#include "xml.h"
#include "plugin.h"
#include "drummap.h"
#include "audio.h"
#include "globaldefs.h"
#include "route.h"
#include "drummap.h"
#include "midictrl.h"
#include "helper.h"
#include "limits.h"
#include "dssihost.h"
#include "gconfig.h"
#include "operations.h"
#include "icons.h"
#include <QMessageBox>

// Undefine if and when multiple output routes are added to midi tracks.
#define _USE_MIDI_TRACK_SINGLE_OUT_PORT_CHAN_

namespace MusECore {

unsigned int Track::_soloRefCnt  = 0;
Track* Track::_tmpSoloChainTrack = 0;
bool Track::_tmpSoloChainDoIns   = false;
bool Track::_tmpSoloChainNoDec   = false;

const char* Track::_cname[] = {
      "Midi", "Drum", "NewStyleDrum", "Wave",
      "AudioOut", "AudioIn", "AudioGroup", "AudioAux", "AudioSynth"
      };


bool MidiTrack::_isVisible=true;


//---------------------------------------------------------
//   trackListSelectedTracks
//   Fills selected list (if valid) with selected tracks in the given list.
//   Returns the number of selected tracks in the given list.
//---------------------------------------------------------

int trackListSelectedTracks(TrackList* list, TrackList* selected) {
      int c = 0;
      for (ciTrack i = list->begin(); i != list->end(); ++i) {
            if ((*i)->selected()) {
                  ++c;
                  if(selected)
                    selected->push_back((*i));
                  }
            }
      return c;
      }

//---------------------------------------------------------
//   addPortCtrlEvents
//---------------------------------------------------------

void addPortCtrlEvents(MidiTrack* t)
{
  const PartList* pl = t->cparts();
  for(ciPart ip = pl->begin(); ip != pl->end(); ++ip)
  {
    Part* part = ip->second;
    const EventList& el = part->events();
    unsigned len = part->lenTick();
    for(ciEvent ie = el.begin(); ie != el.end(); ++ie)
    {
      const Event& ev = ie->second;
      // Added by T356. Do not add events which are past the end of the part.
      if(ev.tick() >= len)
        break;
                    
      if(ev.type() == Controller)
      {
        int tick  = ev.tick() + part->tick();
        int cntrl = ev.dataA();
        int val   = ev.dataB();
        int ch = t->outChannel();
        
        MidiPort* mp = &MusEGlobal::midiPorts[t->outPort()];
        // Is it a drum controller event, according to the track port's instrument?
        if(t->type() == Track::DRUM)
        {
          MidiController* mc = mp->drumController(cntrl);
          if(mc)
          {
            int note = cntrl & 0x7f;
            cntrl &= ~0xff;
            // Default to track port if -1 and track channel if -1.
            if(MusEGlobal::drumMap[note].channel != -1)
              ch = MusEGlobal::drumMap[note].channel;
            if(MusEGlobal::drumMap[note].port != -1)
              mp = &MusEGlobal::midiPorts[MusEGlobal::drumMap[note].port];
            cntrl |= MusEGlobal::drumMap[note].anote;
          }
        }
        
        mp->setControllerVal(ch, tick, cntrl, val, part);
      }
    }
  }
}

void addPortCtrlEvents(Track* track, PendingOperationList& ops)
{
  if(!track || !track->isMidiTrack())
    return;
  const PartList* pl = track->cparts();
  for(ciPart ip = pl->begin(); ip != pl->end(); ++ip)
  {
    Part* part = ip->second;
    addPortCtrlEvents(part, part->tick(), part->lenTick(), track, ops);
  }
}

//---------------------------------------------------------
//   removePortCtrlEvents
//---------------------------------------------------------

void removePortCtrlEvents(MidiTrack* t)
{
  const PartList* pl = t->cparts();
  for(ciPart ip = pl->begin(); ip != pl->end(); ++ip)
  {
    Part* part = ip->second;
    const EventList& el = part->events();
    for(ciEvent ie = el.begin(); ie != el.end(); ++ie)
    {
      const Event& ev = ie->second;
                    
      if(ev.type() == Controller)
      {
        int tick  = ev.tick() + part->tick();
        int cntrl = ev.dataA();
        int ch = t->outChannel();
        
        MidiPort* mp = &MusEGlobal::midiPorts[t->outPort()];
        // Is it a drum controller event, according to the track port's instrument?
        if(t->type() == Track::DRUM)
        {
          MidiController* mc = mp->drumController(cntrl);
          if(mc)
          {
            int note = cntrl & 0x7f;
            cntrl &= ~0xff;
            // Default to track port if -1 and track channel if -1.
            if(MusEGlobal::drumMap[note].channel != -1)
              ch = MusEGlobal::drumMap[note].channel;
            if(MusEGlobal::drumMap[note].port != -1)
              mp = &MusEGlobal::midiPorts[MusEGlobal::drumMap[note].port];
            cntrl |= MusEGlobal::drumMap[note].anote;
          }
        }
        
        mp->deleteController(ch, tick, cntrl, part);
      }
    }
  }
}

void removePortCtrlEvents(Track* track, PendingOperationList& ops)
{
  if(!track || !track->isMidiTrack())
    return;
  const PartList* pl = track->cparts();
  for(ciPart ip = pl->begin(); ip != pl->end(); ++ip)
  {
    Part* part = ip->second;
    removePortCtrlEvents(part, track, ops);
  }
}

//---------------------------------------------------------
//   isVisible
//---------------------------------------------------------
bool Track::isVisible()
{
  switch (type())
  {
    case Track::AUDIO_AUX:
        return AudioAux::visible();
    case Track::AUDIO_GROUP:
        return AudioGroup::visible();
    case Track::AUDIO_INPUT:
        return AudioInput::visible();
    case Track::AUDIO_OUTPUT:
        return AudioOutput::visible();
    case Track::WAVE:
        return WaveTrack::visible();
    case Track::MIDI:
    case Track::DRUM:
    case Track::NEW_DRUM:
        return MidiTrack::visible();
    case Track::AUDIO_SOFTSYNTH:
        return SynthI::visible();
  default:
    break;
  }

  return false;
}


//---------------------------------------------------------
//   y
//---------------------------------------------------------

int Track::y() const
      {
      TrackList* tl = MusEGlobal::song->tracks();
      int yy = 0;
      for (ciTrack it = tl->begin(); it != tl->end(); ++it) {
            if (this == *it)
                  return yy;
            yy += (*it)->height();
            }
      // FIXME Get this when loading a song with automation graphs showing. Benign. Likely song not fully loaded yet. p4.0.32
      if(MusEGlobal::debugMsg)
        printf("Track::y(%s): track not in tracklist\n", name().toLatin1().constData());
      return -1;
      }

//---------------------------------------------------------
//   Track::init
//---------------------------------------------------------

void Track::init()
      {
      _auxRouteCount = 0;  
      _nodeTraversed = false;
      _activity      = 0;
      _lastActivity  = 0;
      _recordFlag    = false;
      _mute          = false;
      _solo          = false;
      _internalSolo  = 0;
      _off           = false;
      _channels      = 0;           // 1 - mono, 2 - stereo
      _selected      = false;
      _height        = MusEGlobal::config.trackHeight;
      _locked        = false;
      for (int i = 0; i < MAX_CHANNELS; ++i) {
            _meter[i] = 0.0;
            _peak[i]  = 0.0;
            // REMOVE Tim. Trackinfo. Added.
            _isClipped[i] = false;
            }
      }

Track::Track(Track::TrackType t)
{
      init();
      _type = t;
// REMOVE Tim. Trackinfo. Removed.
//       _isClipped = false;
}

Track::Track(const Track& t, int flags)
{
  _type         = t.type();
  // REMOVE Tim. Trackinfo. Removed.
//   _isClipped = false;

  // moved setting the unique name to Song::duplicateTracks()
  // we'll see if there is any draw back to that.
  _name = t.name();
  internal_assign(t, flags | ASSIGN_PROPERTIES);
  for (int i = 0; i < MAX_CHANNELS; ++i) {
        _meter[i] = 0.0;
        _peak[i]  = 0.0;
        // REMOVE Tim. Trackinfo. Added.
        _isClipped[i] = false;
        }
}

Track::~Track()
{
  _parts.clearDelete();
}

//---------------------------------------------------------
//   assign 
//---------------------------------------------------------

void Track::assign(const Track& t, int flags) 
{
  internal_assign(t, flags);
}

void Track::internal_assign(const Track& t, int flags)
{
      if(flags & ASSIGN_PROPERTIES)
      {
        _auxRouteCount = t._auxRouteCount;
        _nodeTraversed = t._nodeTraversed;
        _activity     = t._activity;
        _lastActivity = t._lastActivity;
        _recordFlag   = t._recordFlag;
        _mute         = t._mute;
        _solo         = t._solo;
        _internalSolo = t._internalSolo;
        _off          = t._off;
        _channels     = t._channels;
        _selected     = t.selected();
        _y            = t._y;
        _height       = t._height;
        _comment      = t.comment();
        _locked       = t.locked();
      }
}

//---------------------------------------------------------
//   trackTypeIcon
//   Static
//---------------------------------------------------------

QPixmap* Track::trackTypeIcon(TrackType type)
{
  switch(type) {
        case MusECore::Track::MIDI:
              return MusEGui::addtrack_addmiditrackIcon;
        case MusECore::Track::NEW_DRUM:
              return MusEGui::addtrack_newDrumtrackIcon;
        case MusECore::Track::DRUM:
              return MusEGui::addtrack_drumtrackIcon;
        case MusECore::Track::WAVE:
              return MusEGui::addtrack_wavetrackIcon;
        case MusECore::Track::AUDIO_OUTPUT:
              return MusEGui::addtrack_audiooutputIcon;
        case MusECore::Track::AUDIO_INPUT:
              return MusEGui::addtrack_audioinputIcon;
        case MusECore::Track::AUDIO_GROUP:
              return MusEGui::addtrack_audiogroupIcon;
        case MusECore::Track::AUDIO_AUX:
              return MusEGui::addtrack_auxsendIcon;
        case MusECore::Track::AUDIO_SOFTSYNTH:
              return MusEGui::synthIcon;
        default:
              break;
        }
  return 0;        
}

//---------------------------------------------------------
//   setDefaultName
//    generate unique name for track
//---------------------------------------------------------

void Track::setDefaultName(QString base)
      {
      int num_base = 1;  
      if(base.isEmpty())
      {  
        switch(_type) {
              case MIDI:
              case DRUM:
              case NEW_DRUM:
              case WAVE:
                    base = QString("Track");
                    break;
              case AUDIO_OUTPUT:
                    base = QString("Out");
                    break;
              case AUDIO_GROUP:
                    base = QString("Group");
                    break;
              case AUDIO_AUX:
                    base = QString("Aux");
                    break;
              case AUDIO_INPUT:
                    base = QString("Input");
                    break;
              case AUDIO_SOFTSYNTH:
                    base = QString("Synth");
                    break;
              };
        base += " ";
      }        
      else 
      {
        num_base = 2;  
        base += " #";
      }
      
      for (int i = num_base; true; ++i) {
            QString n;
            n.setNum(i);
            QString s = base + n;
            Track* track = MusEGlobal::song->findTrack(s);
            if (track == 0) {
                  setName(s);
                  break;
                  }
            }
      }

//---------------------------------------------------------
//   clearRecAutomation
//---------------------------------------------------------

void Track::clearRecAutomation(bool clearList)
{
    if(isMidiTrack())
      return;
    AudioTrack *t = static_cast<AudioTrack*>(this);
    // Re-enable all track and plugin controllers, and synth controllers if applicable.
    t->enableAllControllers();
    if(clearList)
      t->recEvents()->clear();
}

//---------------------------------------------------------
//   dump
//---------------------------------------------------------

void Track::dump() const
      {
      printf("Track <%s>: typ %d, parts %zd sel %d\n",
         _name.toLatin1().constData(), _type, _parts.size(), _selected);
      }

//---------------------------------------------------------
//   updateAuxRoute
//   Internal use. Update all the Aux ref counts of tracks dst is connected to.
//   If dst is valid, start traversal from there, not from this track.
//---------------------------------------------------------

void Track::updateAuxRoute(int refInc, Track* dst)
{
  if(isMidiTrack())
    return;
  
  if(dst)
  {  
    _nodeTraversed = true;
    dst->updateAuxRoute(refInc, NULL);
    _nodeTraversed = false;
    return;
  }  
  
  if(_type == AUDIO_AUX)
    return;
  
  if(_nodeTraversed)
  {
    fprintf(stderr, "Track::updateAuxRoute %s _auxRouteCount:%d refInc:%d :\n", name().toLatin1().constData(), _auxRouteCount, refInc); 
    if(refInc >= 0)
      fprintf(stderr, "  MusE Warning: Please check your routes: Circular path found!\n"); 
    else
      fprintf(stderr, "  MusE: Circular path removed.\n"); 
    return;
  }
  
  _nodeTraversed = true;
  
  _auxRouteCount += refInc;
  if(_auxRouteCount < 0)
  {
    fprintf(stderr, "Track::updateAuxRoute Ref underflow! %s _auxRouteCount:%d refInc:%d\n", name().toLatin1().constData(), _auxRouteCount, refInc); 
  }
  
  for (iRoute i = _outRoutes.begin(); i != _outRoutes.end(); ++i) 
  {
    if( !(*i).isValid() || (*i).type != Route::TRACK_ROUTE )
      continue;
    Track* t = (*i).track;
    t->updateAuxRoute(refInc, NULL);
  }
  
  _nodeTraversed = false;
}

//---------------------------------------------------------
//   isCircularRoute
//   If dst is valid, start traversal from there, not from this track.
//   Returns true if circular.
//---------------------------------------------------------

bool Track::isCircularRoute(Track* dst)
{
  bool rv = false;
  
  if(dst)
  {  
    _nodeTraversed = true;
    rv = dst->isCircularRoute(NULL);
    _nodeTraversed = false;
    return rv;
  }
  
  if(_nodeTraversed)
    return true;
  
  _nodeTraversed = true;
  
  for (iRoute i = _outRoutes.begin(); i != _outRoutes.end(); ++i) 
  {
    if( !(*i).isValid() || (*i).type != Route::TRACK_ROUTE )
      continue;
    Track* t = (*i).track;
    rv = t->isCircularRoute(NULL);
    if(rv)
      break; 
  }
  
  _nodeTraversed = false;
  return rv;
}

RouteCapabilitiesStruct Track::routeCapabilities() const 
{ 
  RouteCapabilitiesStruct s;
  s._trackChannels._inChannels = s._trackChannels._outChannels = _channels;
  s._trackChannels._inRoutable = s._trackChannels._outRoutable = (_channels != 0);
  return s;
}


//---------------------------------------------------------
//   MidiTrack
//---------------------------------------------------------

MidiTrack::MidiTrack()
   : Track(MIDI)
      {
      init();
      clefType=trebleClef;
      
      _drummap=new DrumMap[128];
      _drummap_hidden=new bool[128];
      
      init_drummap(true /* write drummap ordering information as well */);
      }

MidiTrack::MidiTrack(const MidiTrack& mt, int flags)
  : Track(mt, flags)
{
      _drummap=new DrumMap[128];
      _drummap_hidden=new bool[128];
      
      init_drummap(true /* write drummap ordering information as well */);

      internal_assign(mt, flags | Track::ASSIGN_PROPERTIES);  
}

void MidiTrack::internal_assign(const Track& t, int flags)
{
      if(!t.isMidiTrack())
        return;
      
      const MidiTrack& mt = (const MidiTrack&)t; 
      
      if(flags & ASSIGN_PROPERTIES)
      {
        _outPort       = mt.outPort();
        _outChannel    = mt.outChannel();
        transposition  = mt.transposition;
        velocity       = mt.velocity;
        delay          = mt.delay;
        len            = mt.len;
        compression    = mt.compression;
        _recEcho       = mt.recEcho();
        clefType       = mt.clefType;
      }  
      
      if(flags & ASSIGN_ROUTES)
      {
        for(ciRoute ir = mt._inRoutes.begin(); ir != mt._inRoutes.end(); ++ir)
          // Don't call msgAddRoute. Caller later calls msgAddTrack which 'mirrors' this routing node.
          _inRoutes.push_back(*ir); 
        
        for(ciRoute ir = mt._outRoutes.begin(); ir != mt._outRoutes.end(); ++ir)
          // Don't call msgAddRoute. Caller later calls msgAddTrack which 'mirrors' this routing node.
         _outRoutes.push_back(*ir); 

      for (MusEGlobal::global_drum_ordering_t::iterator it=MusEGlobal::global_drum_ordering.begin(); it!=MusEGlobal::global_drum_ordering.end(); it++)
        if (it->first == &mt)
        {
          it=MusEGlobal::global_drum_ordering.insert(it, *it); // duplicates the entry at it, set it to the first entry of both
          it++;                                                // make it point to the second entry
          it->first=this;
        }
      }
      else if(flags & ASSIGN_DEFAULT_ROUTES)
      {
        // Add default track <-> midiport routes. 
        int c;
        bool defOutFound = false;                /// TODO: Remove this if and when multiple output routes supported.
        const int chmask = (1 << MIDI_CHANNELS) - 1;
        for(int i = 0; i < MIDI_PORTS; ++i)
        {
          MidiPort* mp = &MusEGlobal::midiPorts[i];
          
          if(mp->device())  // Only if device is valid. 
          {
            c = mp->defaultInChannels();
            if(c)
            {
              // Don't call msgAddRoute. Caller later calls msgAddTrack which 'mirrors' this routing node.
              // All channels set or Omni? Use an Omni route:
              if(c == -1 || c == chmask)
                _inRoutes.push_back(Route(i));
              else
              // Add individual channels:  
              for(int ch = 0; ch < MIDI_CHANNELS; ++ch)
              {
                if(c & (1 << ch))
                  _inRoutes.push_back(Route(i, ch));
              }
            }
          }  
          
          if(!defOutFound)
          {
            c = mp->defaultOutChannels();
            if(c)
            {
              
#ifdef _USE_MIDI_TRACK_SINGLE_OUT_PORT_CHAN_
                if(c == -1)
                  c = 1;  // Just to be safe, shouldn't happen, default to channel 0.
                for(int ch = 0; ch < MIDI_CHANNELS; ++ch)   
                {
                  if(c & (1 << ch))
                  {
                    defOutFound = true;
                    _outPort = i;
                    if(type() != Track::DRUM) //&& type != Track::NEW_DRUM)  // Leave drum tracks at channel 10. TODO: Want new drum too?
                      _outChannel = ch;
                    break;               
                  }
                }
#else
                // All channels set or Omni? Use an Omni route:
                if(c == -1 || c == chmask)
                  _outRoutes.push_back(Route(i));
                else
                // Add individual channels:  
                for(int ch = 0; ch < MIDI_CHANNELS; ++ch)
                {
                  if(c & (1 << ch))
                    _outRoutes.push_back(Route(i, ch));
                }

#endif
              
            }
          }  
        }
      }
      
      if (flags & ASSIGN_DRUMLIST)
      {
        for (int i=0;i<128;i++) // no memcpy allowed here. dunno exactly why,
          _drummap[i]=mt._drummap[i]; // seems QString-related.
        memcpy(_drummap_hidden, mt._drummap_hidden, 128*sizeof(bool));
        update_drum_in_map();
        _drummap_tied_to_patch=mt._drummap_tied_to_patch;
        _drummap_ordering_tied_to_patch=mt._drummap_ordering_tied_to_patch;
        // TODO FINDMICH "assign" ordering as well
      }

      if(flags & ASSIGN_PARTS)
      {
        const PartList* pl = t.cparts();
        for (ciPart ip = pl->begin(); ip != pl->end(); ++ip) {
              Part* spart = ip->second;
              Part* dpart;
              if (spart->hasClones())
                  dpart = spart->createNewClone();
              else
                  dpart = spart->duplicate();

              parts()->add(dpart);
              }
      }
      
}

void MidiTrack::assign(const Track& t, int flags)
{
      Track::assign(t, flags);
      internal_assign(t, flags);
}

MidiTrack::~MidiTrack()
      {
      delete [] _drummap;
      delete [] _drummap_hidden;
      
      remove_ourselves_from_drum_ordering();
      }

void MidiTrack::remove_ourselves_from_drum_ordering()
{
  for (MusEGlobal::global_drum_ordering_t::iterator it=MusEGlobal::global_drum_ordering.begin(); it!=MusEGlobal::global_drum_ordering.end();)
    if (it->first == this)
      it=MusEGlobal::global_drum_ordering.erase(it);
    else
      it++;
}

//---------------------------------------------------------
//   init
//---------------------------------------------------------

void MidiTrack::init()
      {
      _outPort       = 0;

      // let's set the port to the last instantiated device
      // if midi-channel defaults are set in the configuration it
      // will override this setting
      for (int i = MIDI_PORTS - 1; i > -1; i--)
      {
        if (MusEGlobal::midiPorts[i].device() != NULL)
        {
          _outPort = i;
          break;
        }
      }

      _outChannel    = (type()==NEW_DRUM) ? 9 : 0;

      transposition  = 0;
      velocity       = 0;
      delay          = 0;
      len            = 100;          // percent
      compression    = 100;          // percent
      _recEcho       = true;
      }

void MidiTrack::init_drum_ordering()
{
  // first display entries with non-empty names, then with empty names.

  remove_ourselves_from_drum_ordering();

  for (int i=0;i<128;i++)
    if (_drummap[i].name!="" && _drummap[i].name!="?") // non-empty name?
      MusEGlobal::global_drum_ordering.push_back(std::pair<MidiTrack*,int>(this,i));

  for (int i=0;i<128;i++)
    if (!(_drummap[i].name!="" && _drummap[i].name!="?")) // empty name?
      MusEGlobal::global_drum_ordering.push_back(std::pair<MidiTrack*,int>(this,i));
}

void MidiTrack::init_drummap(bool write_ordering)
{
  for (int i=0;i<128;i++)
    _drummap[i]=iNewDrumMap[i];

  if (write_ordering)
    init_drum_ordering();
  
  update_drum_in_map();
  
  for (int i=0;i<128;i++)
    _drummap_hidden[i]=false;

  _drummap_tied_to_patch=true;
  _drummap_ordering_tied_to_patch=true;
}

void MidiTrack::update_drum_in_map()
{
  for (int i=0;i<127;i++)
    drum_in_map[(int)_drummap[i].enote]=i;
}

//---------------------------------------------------------
//   height
//---------------------------------------------------------
int MidiTrack::height() const
{
  if (_isVisible)
    return _height;
  return 0;
}

RouteCapabilitiesStruct MidiTrack::routeCapabilities() const 
{ 
  RouteCapabilitiesStruct s;
  s._midiPortChannels._inRoutable = true;
  s._midiPortChannels._inChannels = MIDI_CHANNELS;
  s._trackChannels._outRoutable = true;  // Support Midi Track to Audio Input Track routes (for soloing chain).
  
#ifndef _USE_MIDI_TRACK_SINGLE_OUT_PORT_CHAN_
  s._midiPortChannels._outChannels = MIDI_CHANNELS;
#endif
  
  return s;
}


//---------------------------------------------------------
//   setOutChanAndUpdate
//---------------------------------------------------------

void MidiTrack::setOutChanAndUpdate(int i)       
{ 
  if(_outChannel == i)
    return;
    
  removePortCtrlEvents(this);
  _outChannel = i; 
  addPortCtrlEvents(this);
}

//---------------------------------------------------------
//   setOutPortAndUpdate
//---------------------------------------------------------

void MidiTrack::setOutPortAndUpdate(int i)
{
  if(_outPort == i)
    return;
  
  removePortCtrlEvents(this);
  _outPort = i; 
  addPortCtrlEvents(this);
}

//---------------------------------------------------------
//   setOutPortAndChannelAndUpdate
//---------------------------------------------------------

void MidiTrack::setOutPortAndChannelAndUpdate(int port, int ch)
{
  if(_outPort == port && _outChannel == ch)
    return;
  
  removePortCtrlEvents(this);
  _outPort = port; 
  _outChannel = ch;
  addPortCtrlEvents(this);
}

//---------------------------------------------------------
//   setInPortAndChannelMask
//   For old song files with port mask (max 32 ports) and channel mask (16 channels), 
//    before midi routing was added (the iR button). 
//---------------------------------------------------------

void MidiTrack::setInPortAndChannelMask(unsigned int portmask, int chanmask) 
{ 
  //bool changed = false;
  PendingOperationList operations;
  
  for(int port = 0; port < 32; ++port)  // 32 is the old maximum number of ports.
  {
    // If the port was not used in the song file to begin with, just ignore it.
    // This saves from having all of the first 32 ports' channels connected.
    if(!MusEGlobal::midiPorts[port].foundInSongFile())
      continue;
      
    const int allch = (1 << MIDI_CHANNELS) - 1;
    // Check if Omni route will do...
    if(chanmask == allch)
    {
      // Route wanted?
      if(portmask & (1 << port))
        operations.add(MusECore::PendingOperationItem(MusECore::Route(port), MusECore::Route(this),
                                                      MusECore::PendingOperationItem::AddRoute));
      else
        operations.add(MusECore::PendingOperationItem(MusECore::Route(port), MusECore::Route(this),
                                                      MusECore::PendingOperationItem::DeleteRoute));
    }
    else
    // Add individual channels:
    for(int ch = 0; ch < MIDI_CHANNELS; ++ch)
    {
      // Route wanted?
      if(portmask & (1 << port) && (chanmask & (1 << ch)))
        operations.add(MusECore::PendingOperationItem(MusECore::Route(port, ch), MusECore::Route(this, ch),
                                                      MusECore::PendingOperationItem::AddRoute));
      else
        operations.add(MusECore::PendingOperationItem(MusECore::Route(port, ch), MusECore::Route(this, ch),
                                                      MusECore::PendingOperationItem::DeleteRoute));
    }
  }
   
//   if(changed)
//   {
//     MusEGlobal::audio->msgUpdateSoloStates();
//     MusEGlobal::song->update(SC_ROUTE);
//   }  
  
  if(!operations.empty())
  {
    operations.add(MusECore::PendingOperationItem((TrackList*)NULL, PendingOperationItem::UpdateSoloStates));
    MusEGlobal::audio->msgExecutePendingOperations(operations, true);
//     MusEGlobal::song->update(SC_ROUTE);
  }
}


//---------------------------------------------------------
//   newPart
//---------------------------------------------------------

Part* MidiTrack::newPart(Part*p, bool clone)
      {
      MidiPart* part;
      if (clone)
      {
            part = (MidiPart*)p->createNewClone();
            part->setTrack(this);
      }
      else
      {
            part = (MidiPart*)p->duplicate();
            part->setTrack(this);
      }
      return part;
      }

//---------------------------------------------------------
//   automationType
//---------------------------------------------------------

AutomationType MidiTrack::automationType() const
      {
      MidiPort* port = &MusEGlobal::midiPorts[outPort()];
      return port->automationType(outChannel());
      }

//---------------------------------------------------------
//   setAutomationType
//---------------------------------------------------------

void MidiTrack::setAutomationType(AutomationType t)
      {
      MidiPort* port = &MusEGlobal::midiPorts[outPort()];
      port->setAutomationType(outChannel(), t);
      }

//---------------------------------------------------------
//   MidiTrack::write
//---------------------------------------------------------

void MidiTrack::write(int level, Xml& xml) const
      {
      const char* tag;

      if (type() == DRUM)
            tag = "drumtrack";
      else if (type() == MIDI)
            tag = "miditrack";
      else if (type() == NEW_DRUM)
            tag = "newdrumtrack";
      else {
            printf("THIS SHOULD NEVER HAPPEN: non-midi-type in MidiTrack::write()\n");
            tag="";
      }
      
      xml.tag(level++, tag);
      Track::writeProperties(level, xml);

      xml.intTag(level, "device", outPort());
      xml.intTag(level, "channel", outChannel());
      xml.intTag(level, "locked", _locked);
      xml.intTag(level, "echo", _recEcho);

      xml.intTag(level, "transposition", transposition);
      xml.intTag(level, "velocity", velocity);
      xml.intTag(level, "delay", delay);
      xml.intTag(level, "len", len);
      xml.intTag(level, "compression", compression);
      xml.intTag(level, "automation", int(automationType()));
      xml.intTag(level, "clef", int(clefType));

      const PartList* pl = cparts();
      for (ciPart p = pl->begin(); p != pl->end(); ++p)
            p->second->write(level, xml);
      
      writeOurDrumSettings(level, xml);
      
      xml.etag(level, tag);
      }

void MidiTrack::writeOurDrumSettings(int level, Xml& xml) const
{
  xml.tag(level++, "our_drum_settings");
  
  writeOurDrumMap(level, xml, false);
  
  xml.intTag(level, "tied", _drummap_tied_to_patch);
  xml.intTag(level, "ordering_tied", _drummap_ordering_tied_to_patch);
  
  xml.etag(level, "our_drum_settings");
}

void MidiTrack::writeOurDrumMap(int level, Xml& xml, bool full) const
{
  write_new_style_drummap(level, xml, "our_drummap", _drummap, _drummap_hidden, full);
}

//---------------------------------------------------------
//   MidiTrack::read
//---------------------------------------------------------

void MidiTrack::read(Xml& xml)
      {
      unsigned int portmask = 0;
      int chanmask = 0;
      
      for (;;) {
            Xml::Token token = xml.parse();
            const QString& tag = xml.s1();
            switch (token) {
                  case Xml::Error:
                  case Xml::End:
                        goto out_of_MidiTrackRead_forloop;
                  case Xml::TagStart:
                        if (tag == "transposition")
                              transposition = xml.parseInt();
                        else if (tag == "velocity")
                              velocity = xml.parseInt();
                        else if (tag == "delay")
                              delay = xml.parseInt();
                        else if (tag == "len")
                              len = xml.parseInt();
                        else if (tag == "compression")
                              compression = xml.parseInt();
                        else if (tag == "part") {
                              Part* p = Part::readFromXml(xml, this);
                              if(p)
                                parts()->add(p);
                              }
                        // TODO: These two device and channel sections will need to change 
                        //         if and when multiple output routes are supported. 
                        //       For now just look for the first default (there's only one)...
                        else if (tag == "device") {
                                int port = xml.parseInt();
                                if(port == -1)
                                {
                                  for(int i = 0; i < MIDI_PORTS; ++i)
                                  {
                                    if(MusEGlobal::midiPorts[i].defaultOutChannels())
                                    {
                                      port = i;
                                      break;
                                    }
                                  }
                                }
                                if(port == -1)
                                  port = 0;
                                setOutPort(port);
                              }
                        else if (tag == "channel") {
                                int chan = xml.parseInt();
                                if(chan == -1)
                                {
                                  for(int i = 0; i < MIDI_PORTS; ++i)
                                  {
                                    int defchans = MusEGlobal::midiPorts[i].defaultOutChannels();
                                    for(int c = 0; c < MIDI_CHANNELS; ++c)
                                    {
                                      if(defchans & (1 << c))
                                      {
                                        chan = c;
                                        break;
                                      }
                                    }
                                    if(chan != -1)
                                      break;
                                  }
                                }
                                if(chan == -1)
                                  chan = 0;
                                setOutChannel(chan);
                              }
                        else if (tag == "inportMap")
                              portmask = xml.parseUInt();           // Obsolete but support old files.
                        else if (tag == "inchannelMap")
                              chanmask = xml.parseInt();            // Obsolete but support old files.
                        else if (tag == "locked")
                              _locked = xml.parseInt();
                        else if (tag == "echo")
                              _recEcho = xml.parseInt();
                        else if (tag == "automation")
                              setAutomationType(AutomationType(xml.parseInt()));
                        else if (tag == "clef")
                              clefType = (clefTypes)xml.parseInt();
                        else if (tag == "our_drum_settings")
                              readOurDrumSettings(xml);
                        else if (Track::readProperties(xml, tag)) {
                              // version 1.0 compatibility:
                              if (tag == "track" && xml.majorVersion() == 1 && xml.minorVersion() == 0)
                                    break;
                              xml.unknown("MidiTrack");
                              }
                        break;
                  case Xml::Attribut:
                        break;
                  case Xml::TagEnd:
                        if (tag == "miditrack" || tag == "drumtrack" || tag == "newdrumtrack") 
                        {
                          setInPortAndChannelMask(portmask, chanmask); // Support old files.
                          goto out_of_MidiTrackRead_forloop;
                        }
                  default:
                        break;
                  }
            }
      
out_of_MidiTrackRead_forloop:
      chainTrackParts(this);
      }

void MidiTrack::readOurDrumSettings(Xml& xml)
{
	for (;;)
	{
		Xml::Token token = xml.parse();
		if (token == Xml::Error || token == Xml::End)
			break;
		const QString& tag = xml.s1();
		switch (token)
		{
			case Xml::TagStart:
				if (tag == "tied") 
					_drummap_tied_to_patch = xml.parseInt();
				else if (tag == "ordering_tied") 
					_drummap_ordering_tied_to_patch = xml.parseInt();
				else if (tag == "our_drummap")
                    readOurDrumMap(xml, tag);
                else if (tag == "drummap")
                    readOurDrumMap(xml, tag, false);
                else
					xml.unknown("MidiTrack::readOurDrumSettings");
				break;

			case Xml::TagEnd:
				if (tag == "our_drum_settings")
					return;

			default:
				break;
		}
	}
}

void MidiTrack::readOurDrumMap(Xml& xml, QString tag, bool dont_init, bool compatibility)
{
  if (!dont_init) init_drummap(false);
  _drummap_tied_to_patch=false;
  _drummap_ordering_tied_to_patch=false;
  read_new_style_drummap(xml, tag.toLatin1().data(), _drummap, _drummap_hidden, compatibility);
  update_drum_in_map();
}


//---------------------------------------------------------
//   addPart
//---------------------------------------------------------

iPart Track::addPart(Part* p)
      {
      p->setTrack(this);
      return _parts.add(p);
      }

//---------------------------------------------------------
//   findPart
//---------------------------------------------------------

Part* Track::findPart(unsigned tick)
      {
      for (iPart i = _parts.begin(); i != _parts.end(); ++i) {
            Part* part = i->second;
            if (tick >= part->tick() && tick < (part->tick()+part->lenTick()))
                  return part;
            }
      return 0;
      }

//---------------------------------------------------------
//   Track::writeProperties
//---------------------------------------------------------

void Track::writeProperties(int level, Xml& xml) const
      {
      xml.strTag(level, "name", _name);
      if (!_comment.isEmpty())
            xml.strTag(level, "comment", _comment);
      xml.intTag(level, "record", _recordFlag);
      xml.intTag(level, "mute", mute());
      xml.intTag(level, "solo", solo());
      xml.intTag(level, "off", off());
      xml.intTag(level, "channels", _channels);
      xml.intTag(level, "height", _height);
      xml.intTag(level, "locked", _locked);
      if (_selected)
            xml.intTag(level, "selected", _selected);
      }

//---------------------------------------------------------
//   Track::readProperties
//---------------------------------------------------------

bool Track::readProperties(Xml& xml, const QString& tag)
      {
      if (tag == "name")
            _name = xml.parse1();
      else if (tag == "comment")
            _comment = xml.parse1();
      else if (tag == "record") {
            bool recordFlag = xml.parseInt();
            setRecordFlag1(recordFlag);
            setRecordFlag2(recordFlag);
            }
      else if (tag == "mute")
            _mute = xml.parseInt();
      else if (tag == "solo")
            _solo = xml.parseInt();
      else if (tag == "off")
            _off = xml.parseInt();
      else if (tag == "height")
            _height = xml.parseInt();
      else if (tag == "channels")
      {
        _channels = xml.parseInt();
        if(_channels > MAX_CHANNELS)
          _channels = MAX_CHANNELS;
      }      
      else if (tag == "locked")
            _locked = xml.parseInt();
      else if (tag == "selected")
            _selected = xml.parseInt();
      else
            return true;
      return false;
      }

//---------------------------------------------------------
//   writeRouting
//---------------------------------------------------------

void Track::writeRouting(int level, Xml& xml) const
{
      QString s;
      if (type() == Track::AUDIO_INPUT) 
      {
        const RouteList* rl = &_inRoutes;
        for (ciRoute r = rl->begin(); r != rl->end(); ++r) 
        {
          if(!r->name().isEmpty())
          {
            s = "Route";
            if(r->channel != -1)
              s += QString(" channel=\"%1\"").arg(r->channel);
            
            xml.tag(level++, s.toLatin1().constData());
            
            // New routing scheme.
            s = "source";
            if(r->type != Route::TRACK_ROUTE)
              s += QString(" type=\"%1\"").arg(r->type);
            s += QString(" name=\"%1\"/").arg(Xml::xmlString(r->name()));
            xml.tag(level, s.toLatin1().constData());
            
            xml.tag(level, "dest name=\"%s\"/", Xml::xmlString(name()).toLatin1().constData());
            
            xml.etag(level--, "Route");
          }
        }
      }
      
      const RouteList* rl = &_outRoutes;
      for (ciRoute r = rl->begin(); r != rl->end(); ++r) 
      {
        // p4.0.14 Ignore Audio Output to Audio Input routes. 
        // They are taken care of by Audio Input in the section above.
        if(r->type == Route::TRACK_ROUTE && r->track && r->track->type() == Track::AUDIO_INPUT) 
          continue;
            
        if(r->midiPort != -1 || !r->name().isEmpty()) 
        {
          s = "Route";
          if(r->channel != -1)
            s += QString(" channel=\"%1\"").arg(r->channel);
          if(r->channels != -1)
            s += QString(" channels=\"%1\"").arg(r->channels);
          if(r->remoteChannel != -1)
            s += QString(" remch=\"%1\"").arg(r->remoteChannel);
          
          xml.tag(level++, s.toLatin1().constData());
          
          // Allow for a regular mono or stereo track to feed a multi-channel synti. 
          xml.tag(level, "source name=\"%s\"/", Xml::xmlString(name()).toLocal8Bit().constData());
          
          s = "dest";
          
          if(r->type != Route::TRACK_ROUTE && r->type != Route::MIDI_PORT_ROUTE)
            s += QString(" type=\"%1\"").arg(r->type);

          if(r->type == Route::MIDI_PORT_ROUTE)                                          
            s += QString(" mport=\"%1\"/").arg(r->midiPort);
          else  
            s += QString(" name=\"%1\"/").arg(Xml::xmlString(r->name()));
            
          xml.tag(level, s.toLatin1().constData());
          
          xml.etag(level--, "Route");
        }
      }
}

int MidiTrack::getFirstControllerValue(int ctrl, int def)
{
  int val=def;
  unsigned tick=-1; // maximum integer
  
  for (iPart pit=parts()->begin(); pit!=parts()->end(); pit++)
  {
    Part* part=pit->second;
    if (part->tick() > tick) break; // ignore this and the rest. we won't find anything new.
    for (ciEvent eit=part->events().begin(); eit!=part->events().end(); eit++)
    {
      if (eit->first+part->tick() >= tick) break;
      if (eit->first > part->lenTick()) break; // ignore events past the end of the part
      // else if (eit->first+part->tick() < tick) and
      if (eit->second.type()==Controller && eit->second.dataA()==ctrl)
      {
        val = eit->second.dataB();
        tick = eit->first+part->tick();
        break;
      }
    }
  }

  return val;
}

int MidiTrack::getControllerChangeAtTick(unsigned tick, int ctrl, int def)
{
  for (iPart pit=parts()->begin(); pit!=parts()->end(); pit++)
  {
    Part* part=pit->second;
    if (part->tick() > tick) break; // ignore this and the rest. we'd find nothing any more
    if (part->endTick() < tick) continue; // ignore only this.
    for (ciEvent eit=part->events().begin(); eit!=part->events().end(); eit++)
    {
      if (eit->first+part->tick() > tick) break; // we won't find anything in this part from now on.
      if (eit->first > part->lenTick()) break; // ignore events past the end of the part
      if (eit->first+part->tick() < tick) continue; // ignore only this
      
      // else if (eit->first+part->tick() == tick) and
      if (eit->second.type()==Controller && eit->second.dataA()==ctrl)
        return eit->second.dataB();
    }
  }

  return def;
}

// returns the tick where this CC gets overriden by a new one
// returns UINT_MAX for "never"
unsigned MidiTrack::getControllerValueLifetime(unsigned tick, int ctrl) 
{
  unsigned result=UINT_MAX;
  
  for (iPart pit=parts()->begin(); pit!=parts()->end(); pit++)
  {
    Part* part=pit->second;
    if (part->tick() > result) break; // ignore this and the rest. we won't find anything new.
    if (part->endTick() < tick) continue; // ignore only this part, we won't find anything there.
    for (ciEvent eit=part->events().begin(); eit!=part->events().end(); eit++)
    {
      if (eit->first+part->tick() >= result) break;
      if (eit->first > part->lenTick()) break; // ignore events past the end of the part
      // else if (eit->first+part->tick() < result) and
      if (eit->first+part->tick() > tick &&
          eit->second.type()==Controller && eit->second.dataA()==ctrl)
      {
        result = eit->first+part->tick();
        break;
      }
    }
  }

  return result;
}

// returns true if the autoupdate changed something
bool MidiTrack::auto_update_drummap()
{
  if (_drummap_tied_to_patch)
  {
    int patch = getFirstControllerValue(CTRL_PROGRAM,0);
    const DrumMap* new_drummap = MusEGlobal::midiPorts[_outPort].instrument()->drummap_for_patch(patch);
    
    if (!drummaps_almost_equal(new_drummap, this->drummap(), 128))
    {
      for (int i=0;i<128;i++)
      {
        bool temp_mute=_drummap[i].mute;
        _drummap[i]=new_drummap[i];
        _drummap[i].mute=temp_mute;
      }
      
      if (_drummap_ordering_tied_to_patch)
        init_drum_ordering();
      
      return true;
    }
  }
  
  return false;
}

void MidiTrack::set_drummap_tied_to_patch(bool val)
{
  _drummap_tied_to_patch=val;
  if (val) auto_update_drummap();
}

void MidiTrack::set_drummap_ordering_tied_to_patch(bool val)
{
  _drummap_ordering_tied_to_patch=val;
  if (val && _drummap_tied_to_patch) init_drum_ordering();
}

} // namespace MusECore
