//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: track.cpp,v 1.34.2.11 2009/11/30 05:05:49 terminator356 Exp $
//
//  (C) Copyright 2000-2004 Werner Schweer (ws@seh.de)
//  (C) Copyright 2011, 2016 Tim E. Real (terminator356 on sourceforge)
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
#include "mpevent.h"
#include "midi.h"
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
int Track::_selectionOrderCounter = 0;

const char* Track::_cname[] = {
      "Midi", "Drum", "NewStyleDrum", "Wave",
      "AudioOut", "AudioIn", "AudioGroup", "AudioAux", "AudioSynth"
      };


bool MidiTrack::_isVisible=true;


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
        int val = ev.dataB();
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

        mp->deleteController(ch, tick, cntrl, val, part);
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
      _selectionOrder = 0;
      _height        = MusEGlobal::config.trackHeight;
      _locked        = false;
      _recMonitor    = false;
      for (int i = 0; i < MusECore::MAX_CHANNELS; ++i) {
            _meter[i] = 0.0;
            _peak[i]  = 0.0;
            _isClipped[i] = false;
            }
      }

Track::Track(Track::TrackType t)
{
      init();
      _type = t;
}

Track::Track(const Track& t, int flags)
{
  _type         = t.type();
  // moved setting the unique name to Song::duplicateTracks()
  // we'll see if there is any draw back to that.
  _name = t.name();
  internal_assign(t, flags | ASSIGN_PROPERTIES);
  for (int i = 0; i < MusECore::MAX_CHANNELS; ++i) {
        _meter[i] = 0.0;
        _peak[i]  = 0.0;
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
        _selectionOrder = t.selectionOrder();
        _y            = t._y;
        _height       = t._height;
        _comment      = t.comment();
        _locked       = t.locked();
        _recMonitor   = t._recMonitor;
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
//   trackTypeColor
//   Static
//---------------------------------------------------------

QColor Track::trackTypeColor(TrackType type)
{
  switch(type) {
        case MusECore::Track::MIDI:
              return MusEGlobal::config.midiTrackBg;
        case MusECore::Track::NEW_DRUM:
              return MusEGlobal::config.newDrumTrackBg;
        case MusECore::Track::DRUM:
              return MusEGlobal::config.drumTrackBg;
        case MusECore::Track::WAVE:
              return MusEGlobal::config.waveTrackBg;
        case MusECore::Track::AUDIO_OUTPUT:
              return MusEGlobal::config.outputTrackBg;
        case MusECore::Track::AUDIO_INPUT:
              return MusEGlobal::config.inputTrackBg;
        case MusECore::Track::AUDIO_GROUP:
              return MusEGlobal::config.groupTrackBg;
        case MusECore::Track::AUDIO_AUX:
              return MusEGlobal::config.auxTrackBg;
        case MusECore::Track::AUDIO_SOFTSYNTH:
              return MusEGlobal::config.synthTrackBg;
        default:
              break;
        }
  return QColor();
}

//---------------------------------------------------------
//   trackTypeLabelColor
//   Static
//---------------------------------------------------------

QColor Track::trackTypeLabelColor(TrackType type)
{
  switch(type) {
        case MusECore::Track::MIDI:
              return MusEGlobal::config.midiTrackLabelBg;
        case MusECore::Track::NEW_DRUM:
              return MusEGlobal::config.newDrumTrackLabelBg;
        case MusECore::Track::DRUM:
              return MusEGlobal::config.drumTrackLabelBg;
        case MusECore::Track::WAVE:
              return MusEGlobal::config.waveTrackLabelBg;
        case MusECore::Track::AUDIO_OUTPUT:
              return MusEGlobal::config.outputTrackLabelBg;
        case MusECore::Track::AUDIO_INPUT:
              return MusEGlobal::config.inputTrackLabelBg;
        case MusECore::Track::AUDIO_GROUP:
              return MusEGlobal::config.groupTrackLabelBg;
        case MusECore::Track::AUDIO_AUX:
              return MusEGlobal::config.auxTrackLabelBg;
        case MusECore::Track::AUDIO_SOFTSYNTH:
              return MusEGlobal::config.synthTrackLabelBg;
        default:
              break;
        }
  return QColor();
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
//   setSelected
//---------------------------------------------------------

void Track::setSelected(bool f)
{ 
  if(f && !_selected)
    _selectionOrder = _selectionOrderCounter++;
  _selected = f; 
}

//---------------------------------------------------------
//   dump
//---------------------------------------------------------

void Track::dump() const
      {
      printf("Track <%s>: typ %d, parts %zd sel %d sel order%d\n",
         _name.toLatin1().constData(), _type, _parts.size(), _selected, _selectionOrder);
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
      _workingDrumMapPatchList = new WorkingDrumMapPatchList();

      init_drummap(true /* write drummap ordering information as well */);
      }

MidiTrack::MidiTrack(const MidiTrack& mt, int flags)
  : Track(mt, flags)
{
      _drummap=new DrumMap[128];
      _workingDrumMapPatchList = new WorkingDrumMapPatchList();

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
        const int chmask = (1 << MusECore::MUSE_MIDI_CHANNELS) - 1;
        for(int i = 0; i < MusECore::MIDI_PORTS; ++i)
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
              for(int ch = 0; ch < MusECore::MUSE_MIDI_CHANNELS; ++ch)
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
                for(int ch = 0; ch < MusECore::MUSE_MIDI_CHANNELS; ++ch)   
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
                for(int ch = 0; ch < MusECore::MUSE_MIDI_CHANNELS; ++ch)
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
        update_drum_in_map();
        _drummap_ordering_tied_to_patch=mt._drummap_ordering_tied_to_patch;
        // TODO FINDMICH "assign" ordering as well

        if(mt._workingDrumMapPatchList)
          *_workingDrumMapPatchList = *mt._workingDrumMapPatchList;
      }

      const bool dup = flags & ASSIGN_DUPLICATE_PARTS;
      const bool cpy = flags & ASSIGN_COPY_PARTS;
      const bool cln = flags & ASSIGN_CLONE_PARTS;
      if(dup || cpy || cln)
      {
        const PartList* pl = t.cparts();
        for (ciPart ip = pl->begin(); ip != pl->end(); ++ip) {
              Part* spart = ip->second;
              Part* dpart = 0;
              if(dup)
                dpart = spart->hasClones() ? spart->createNewClone() : spart->duplicate();
              else if(cpy)
                dpart = spart->duplicate();
              else if(cln)
                dpart = spart->createNewClone();
              if(dpart)
              {
                dpart->setTrack(this);
                parts()->add(dpart);
              }
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
      if(_workingDrumMapPatchList)
        delete _workingDrumMapPatchList;
      delete [] _drummap;
      remove_ourselves_from_drum_ordering();
      }


bool MidiTrack::setRecordFlag2AndCheckMonitor(bool f)
{
  if(canRecord())
    _recordFlag = f;

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

void MidiTrack::convertToType(TrackType trackType)
{
  if(trackType == MusECore::Track::MIDI  ||  trackType == MusECore::Track::NEW_DRUM)
  {
    //
    //    Drum -> Midi
    //
    MusECore::PartList* pl = parts();
    for (MusECore::iPart ip = pl->begin(); ip != pl->end(); ++ip) {
      for (MusECore::ciEvent ie = ip->second->events().begin(); ie != ip->second->events().end(); ++ie) {
        MusECore::Event ev = ie->second;
        if(ev.type() == MusECore::Note)
        {
          int pitch = ev.pitch();
          pitch = MusEGlobal::drumMap[pitch].enote;
          ev.setPitch(pitch);
        }
        else
          if(ev.type() == MusECore::Controller)
          {
            int ctl = ev.dataA();
            // Is it a drum controller event, according to the track port's instrument?
            MusECore::MidiController *mc = MusEGlobal::midiPorts[outPort()].drumController(ctl);
            if(mc)
              // Change the controller event's index into the drum map to an instrument note.
              ev.setA((ctl & ~0xff) | MusEGlobal::drumMap[ctl & 0x7f].enote);
          }
      }
    }
    setType(trackType);
  }
  else if(trackType == MusECore::Track::DRUM)
  {
    //
    //    Midi -> Drum
    //

    // Default to track port if -1 and track channel if -1. No need anymore to ask to change all items.

    // Delete all port controller events.
    MusEGlobal::song->changeAllPortDrumCtrlEvents(false);

    MusECore::PartList* pl = parts();
    for (MusECore::iPart ip = pl->begin(); ip != pl->end(); ++ip) {
      for (MusECore::ciEvent ie = ip->second->events().begin(); ie != ip->second->events().end(); ++ie) {
        MusECore::Event ev = ie->second;
        if (ev.type() == MusECore::Note)
        {
          int pitch = ev.pitch();
          pitch = MusEGlobal::drumInmap[pitch];
          ev.setPitch(pitch);
        }
        else
        {
          if(ev.type() == MusECore::Controller)
          {
            int ctl = ev.dataA();
            // Is it a drum controller event, according to the track port's instrument?
            MusECore::MidiController *mc = MusEGlobal::midiPorts[outPort()].drumController(ctl);
            if(mc)
              // Change the controller event's instrument note to an index into the drum map.
              ev.setA((ctl & ~0xff) | MusEGlobal::drumInmap[ctl & 0x7f]);
          }
        }
      }
    }

    setType(MusECore::Track::DRUM);

    // Add all port controller events.
    MusEGlobal::song->changeAllPortDrumCtrlEvents(true);
  }
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
      for (int i = MusECore::MIDI_PORTS - 1; i > -1; i--)
      {
        if (MusEGlobal::midiPorts[i].device() != NULL)
        {
          _outPort = i;
          break;
        }
      }

      _outChannel    = (type()==NEW_DRUM) ? 9 : 0;

      _curDrumPatchNumber = CTRL_VAL_UNKNOWN;

      transposition  = 0;
      velocity       = 0;
      delay          = 0;
      len            = 100;          // percent
      compression    = 100;          // percent
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

  _drummap_ordering_tied_to_patch=true;
}

void MidiTrack::update_drum_in_map()
{
  for (int i = 0; i < 128; ++i)
    drum_in_map[(int)_drummap[i].enote] = i;
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

//---------------------------------------------------------
//   routeCapabilities
//---------------------------------------------------------

RouteCapabilitiesStruct MidiTrack::routeCapabilities() const 
{ 
  RouteCapabilitiesStruct s;
  s._midiPortChannels._inRoutable = true;
  s._midiPortChannels._inChannels = MusECore::MUSE_MIDI_CHANNELS;
  s._trackChannels._outRoutable = true;  // Support Midi Track to Audio Input Track routes (for soloing chain).
  
#ifndef _USE_MIDI_TRACK_SINGLE_OUT_PORT_CHAN_
  s._midiPortChannels._outChannels = MusECore::MUSE_MIDI_CHANNELS;
#endif
  
  return s;
}

//---------------------------------------------------------
//   noOutRoute
//---------------------------------------------------------

bool MidiTrack::noOutRoute() const  
{ 
  
  return _outRoutes.empty()
  
#ifdef _USE_MIDI_TRACK_SINGLE_OUT_PORT_CHAN_
    && (outChannel() < 0 || outPort() < 0 || !MusEGlobal::midiPorts[outPort()].device())
#endif
  ;
}

//---------------------------------------------------------
//   setOutChannel
//---------------------------------------------------------

MidiTrack::ChangedType_t MidiTrack::setOutChannel(int i, bool doSignal)
{
  if(_outChannel == i)
    return NothingChanged;
  _outChannel = i;
  ChangedType_t res = ChannelChanged;
  if(updateDrummap(doSignal))
    res |= DrumMapChanged;
  return res;
}

//---------------------------------------------------------
//   setOutPort
//---------------------------------------------------------

MidiTrack::ChangedType_t MidiTrack::setOutPort(int i, bool doSignal)
{
  if(_outPort == i)
    return NothingChanged;
  _outPort = i;
  ChangedType_t res = PortChanged;
  if(updateDrummap(doSignal))
    res |= DrumMapChanged;
  return res;
}

//---------------------------------------------------------
//   setOutChanAndUpdate
//---------------------------------------------------------

MidiTrack::ChangedType_t MidiTrack::setOutChanAndUpdate(int i, bool doSignal)
{ 
  if(_outChannel == i)
    return NothingChanged;
    
  removePortCtrlEvents(this);
  _outChannel = i; 
  ChangedType_t res = ChannelChanged;
  if(updateDrummap(doSignal))
    res |= DrumMapChanged;
  addPortCtrlEvents(this);
  return res;
}

//---------------------------------------------------------
//   setOutPortAndUpdate
//---------------------------------------------------------

MidiTrack::ChangedType_t MidiTrack::setOutPortAndUpdate(int i, bool doSignal)
{
  if(_outPort == i)
    return NothingChanged;
  
  removePortCtrlEvents(this);
  _outPort = i; 
  ChangedType_t res = PortChanged;
  if(updateDrummap(doSignal))
    res |= DrumMapChanged;
  addPortCtrlEvents(this);
  return res;
}

//---------------------------------------------------------
//   setOutPortAndChannelAndUpdate
//---------------------------------------------------------

MidiTrack::ChangedType_t MidiTrack::setOutPortAndChannelAndUpdate(int port, int ch, bool doSignal)
{
  if(_outPort == port && _outChannel == ch)
    return NothingChanged;
  
  removePortCtrlEvents(this);
  _outPort = port; 
  _outChannel = ch;
  ChangedType_t res = PortChanged | ChannelChanged;
  if(updateDrummap(doSignal))
    res |= DrumMapChanged;
  addPortCtrlEvents(this);
  return res;
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
      
    const int allch = (1 << MusECore::MUSE_MIDI_CHANNELS) - 1;
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
    for(int ch = 0; ch < MusECore::MUSE_MIDI_CHANNELS; ++ch)
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
      if(!p)
      {
        part = new MidiPart(this);
      }
      else
      {
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
      }
      return part;
      }

//---------------------------------------------------------
//   addStuckNote
//---------------------------------------------------------

bool MidiTrack::addStuckNote(const MidiPlayEvent& ev)
{
  stuckNotes.add(ev);
  return true;
}
      
//---------------------------------------------------------
//   addStuckLiveNote
//   Return true if note was added.
//---------------------------------------------------------

bool MidiTrack::addStuckLiveNote(int port, int chan, int note, int vel)
{
//   for(ciMPEvent k = stuckLiveNotes.begin(); k != stuckLiveNotes.end(); ++k)
//   {
//     // We're looking for port, channel, and note. Time and velocity are not relevant.
//     if((*k).port() == port &&
//        (*k).channel() == chan &&
//        (*k).dataA() == note)
//       return false;
//   }
  stuckLiveNotes.add(MidiPlayEvent(0, port, chan, ME_NOTEOFF, note, vel)); // Mark for immediate playback
  return true;
}

//---------------------------------------------------------
//   removeStuckLiveNote
//   Return true if note was removed.
//---------------------------------------------------------

bool MidiTrack::removeStuckLiveNote(int port, int chan, int note)
{
  for(ciMPEvent k = stuckLiveNotes.begin(); k != stuckLiveNotes.end(); ++k)
  {
    // We're looking for port, channel, and note. Time and velocity are not relevant.
    if((*k).port() == port &&
       (*k).channel() == chan &&
       (*k).dataA() == note)
    {
      stuckLiveNotes.erase(k);
      return true;
    }
  }
  return false;
}

//---------------------------------------------------------
//   stuckLiveNoteExists
//   Return true if note exists.
//---------------------------------------------------------

bool MidiTrack::stuckLiveNoteExists(int port, int chan, int note)
{
  for(ciMPEvent k = stuckLiveNotes.begin(); k != stuckLiveNotes.end(); ++k)
  {
    // We're looking for port, channel, and note. Time and velocity are not relevant.
    if((*k).port() == port &&
       (*k).channel() == chan &&
       (*k).dataA() == note)
      return true;
  }
  return false;
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
  _workingDrumMapPatchList->write(level, xml);
  xml.intTag(level, "ordering_tied", _drummap_ordering_tied_to_patch);
  xml.etag(level, "our_drum_settings");
}

void MidiTrack::MidiCtrlRemapOperation(int index, int newPort, int newChan, int newNote, MidiCtrlValRemapOperation* rmop)
{
  if(type() != Track::NEW_DRUM || _outPort < 0 || _outPort >= MIDI_PORTS)
    return;

  // Default to track port if -1 and track channel if -1.
  if(newPort == -1)
    newPort = _outPort;

  if(newChan == -1)
    newChan = _outChannel;

  MidiPort* trackmp = &MusEGlobal::midiPorts[_outPort];

  int dm_ch = _drummap[index].channel;
  if(dm_ch == -1)
    dm_ch = _outChannel;
  int dm_port = _drummap[index].port;
  if(dm_port == -1)
    dm_port = _outPort;
  MidiPort* dm_mp = &MusEGlobal::midiPorts[dm_port];

  MidiCtrlValListList* dm_mcvll = dm_mp->controller();
  MidiCtrlValList* v_mcvl;
  int v_ch, v_ctrl, v_idx;
  for(iMidiCtrlValList idm_mcvl = dm_mcvll->begin(); idm_mcvl != dm_mcvll->end(); ++idm_mcvl)
  {
    v_ch = idm_mcvl->first >> 24;
    if(v_ch != dm_ch)
      continue;
    v_mcvl = idm_mcvl->second;
    v_ctrl = v_mcvl->num();

    // Is it a drum controller, according to the track port's instrument?
    if(!trackmp->drumController(v_ctrl))
      continue;

    v_idx = v_ctrl & 0xff;
    if(v_idx != _drummap[index].anote)
      continue;

    // Does this midi control value list need to be changed (values moved etc)?
    iMidiCtrlVal imcv = v_mcvl->begin();
    for( ; imcv != v_mcvl->end(); ++imcv)
    {
      const MidiCtrlVal& mcv = imcv->second;
      if(mcv.part && mcv.part->track() == this)
        break;
    }
    if(imcv != v_mcvl->end())
    {
      // A contribution from a part on this track was found.
      // We must compose a new list, or get an existing one and schedule the existing
      //  one for iterator erasure and pointer deletion.
      // Add the erase iterator. Add will ignore if the erase iterator already exists.
      rmop->_midiCtrlValLists2bErased.add(dm_port, idm_mcvl);
      // Insert the delete pointer. Insert will ignore if the delete pointer already exists.
      rmop->_midiCtrlValLists2bDeleted.insert(v_mcvl);

      MidiCtrlValListList* op_mcvll;
      iMidiCtrlValLists2bAdded_t imcvla = rmop->_midiCtrlValLists2bAdded.find(dm_port);
      if(imcvla == rmop->_midiCtrlValLists2bAdded.end())
      {
        op_mcvll = new MidiCtrlValListList();
        rmop->_midiCtrlValLists2bAdded.insert(MidiCtrlValLists2bAddedInsertPair_t(dm_port, op_mcvll));
      }
      else
        op_mcvll = imcvla->second;

      MidiCtrlValList* op_mcvl;
      iMidiCtrlValList imcvl = op_mcvll->find(dm_ch, v_ctrl);
      if(imcvl == op_mcvll->end())
      {
        op_mcvl = new MidiCtrlValList(v_ctrl);
        op_mcvll->add(dm_ch, op_mcvl);
        // Assign the contents of the original list to the new list.
        *op_mcvl = *v_mcvl;
      }
      else
        op_mcvl = imcvl->second;

      // Remove from the list any contributions from this track.
      iMidiCtrlVal iopmcv = op_mcvl->begin();
      for( ; iopmcv != op_mcvl->end(); )
      {
        const MidiCtrlVal& mcv = iopmcv->second;
        if(mcv.part && mcv.part->track() == this)
        {
          iMidiCtrlVal iopmcv_save = iopmcv;
          ++iopmcv_save;
          op_mcvl->erase(iopmcv);
          iopmcv = iopmcv_save;
        }
        else
          ++iopmcv;
      }
    }

    // We will be making changes to the list pointed to by the new settings.
    // We must schedule the existing one for iterator erasure and pointer deletion.
    MidiPort* dm_mp_new = &MusEGlobal::midiPorts[newPort];
    MidiCtrlValListList* dm_mcvll_new = dm_mp_new->controller();
    MidiCtrlValList* v_mcvl_new = 0;
    const int v_ctrl_new = (v_ctrl & ~0xff) | newNote;
    iMidiCtrlValList idm_mcvl_new = dm_mcvll_new->find(newChan, v_ctrl_new);
    if(idm_mcvl_new != dm_mcvll_new->end())
    {
      v_mcvl_new = idm_mcvl_new->second;
      // Add the erase iterator. Add will ignore if the erase iterator already exists.
      rmop->_midiCtrlValLists2bErased.add(newPort, idm_mcvl_new);
      // Insert the delete pointer. Insert will ignore if the delete pointer already exists.
      rmop->_midiCtrlValLists2bDeleted.insert(v_mcvl_new);
    }

    // Create a new list of lists, or get an existing one.
    MidiCtrlValListList* op_mcvll_new;
    iMidiCtrlValLists2bAdded_t imcvla_new = rmop->_midiCtrlValLists2bAdded.find(newPort);
    if(imcvla_new == rmop->_midiCtrlValLists2bAdded.end())
    {
      op_mcvll_new = new MidiCtrlValListList();
      rmop->_midiCtrlValLists2bAdded.insert(MidiCtrlValLists2bAddedInsertPair_t(newPort, op_mcvll_new));
    }
    else
      op_mcvll_new = imcvla_new->second;

    // Compose a new list for replacement, or get an existing one.
    MidiCtrlValList* op_mcvl_new;
    iMidiCtrlValList imcvl_new = op_mcvll_new->find(newChan, v_ctrl_new);
    if(imcvl_new == op_mcvll_new->end())
    {
      op_mcvl_new = new MidiCtrlValList(v_ctrl_new);
      op_mcvll_new->add(newChan, op_mcvl_new);
      // Assign the contents of the original list to the new list.
      if(v_mcvl_new)
        *op_mcvl_new = *v_mcvl_new;
    }
    else
      op_mcvl_new = imcvl_new->second;

    // Add to the list any contributions from this track.
    for(ciMidiCtrlVal imcv_new = v_mcvl->begin(); imcv_new != v_mcvl->end(); ++imcv_new)
    {
      const MidiCtrlVal& mcv = imcv_new->second;
      if(mcv.part && mcv.part->track() == this)
      {
        op_mcvl_new->addMCtlVal(imcv_new->first, mcv.val, mcv.part);
      }
    }
  }
}

void MidiTrack::dumpMap()
{
  if(type() != NEW_DRUM)
    return;
  const int port = outPort();
  if(port < 0 || port >= MusECore::MIDI_PORTS)
    return;
  MidiPort* mp = &MusEGlobal::midiPorts[port];
  const int chan = outChannel();
  const int patch = mp->hwCtrlState(chan, MusECore::CTRL_PROGRAM);

  fprintf(stderr, "Drum map for patch:%d\n\n", patch);

  fprintf(stderr, "name\t\tvol\tqnt\tlen\tchn\tprt\tlv1\tlv2\tlv3\tlv4\tenote\t\tanote\\ttmute\thide\n");

  DrumMap all_dm,
#ifdef _USE_INSTRUMENT_OVERRIDES_
    instr_dm, instrdef_dm,
#endif
    track_dm, trackdef_dm;

  for(int index = 0; index < 128; ++index)
  {
    getMapItem(patch, index, all_dm, WorkingDrumMapEntry::AllOverrides);
    getMapItem(patch, index, track_dm, WorkingDrumMapEntry::TrackOverride);
    getMapItem(patch, index, trackdef_dm, WorkingDrumMapEntry::TrackDefaultOverride);
#ifdef _USE_INSTRUMENT_OVERRIDES_
    getMapItem(patch, index, instr_dm, WorkingDrumMapEntry::InstrumentOverride);
    getMapItem(patch, index, instrdef_dm, WorkingDrumMapEntry::InstrumentDefaultOverride);
#endif

    fprintf(stderr, "Index:%d ", index);
    fprintf(stderr, "All overrides:\n");
    all_dm.dump();

#ifdef _USE_INSTRUMENT_OVERRIDES_
    fprintf(stderr, "Instrument override:\n");
    instr_dm.dump();
    fprintf(stderr, "Instrument default override:\n");
    instrdef_dm.dump();
#endif

    fprintf(stderr, "Track override:\n");
    track_dm.dump();
    fprintf(stderr, "Track default override:\n");
    trackdef_dm.dump();

    fprintf(stderr, "\n");
  }
}


//---------------------------------------------------------
//   MidiTrack::read
//---------------------------------------------------------

void MidiTrack::read(Xml& xml)
      {
      unsigned int portmask = 0;
      int chanmask = 0;
      bool portmask_found = false;
      bool chanmask_found = false;
      
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
                                  for(int i = 0; i < MusECore::MIDI_PORTS; ++i)
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
                                  for(int i = 0; i < MusECore::MIDI_PORTS; ++i)
                                  {
                                    int defchans = MusEGlobal::midiPorts[i].defaultOutChannels();
                                    for(int c = 0; c < MusECore::MUSE_MIDI_CHANNELS; ++c)
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
                        {
                              portmask = xml.parseUInt();           // Obsolete but support old files.
                              portmask_found = true;
                        }
                        else if (tag == "inchannelMap")
                        {
                              chanmask = xml.parseInt();            // Obsolete but support old files.
                              chanmask_found = true;
                        }
                        else if (tag == "locked")
                              _locked = xml.parseInt();
                        else if (tag == "echo")                     // Obsolete but support old files.
                              setRecMonitor(xml.parseInt());
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
                          if(portmask_found && chanmask_found)
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
  bool doUpdateDrummap = false;
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
          xml.parseInt(); // Obsolete.
        else if (tag == "ordering_tied")
          _drummap_ordering_tied_to_patch = xml.parseInt();

        else if (tag == "our_drummap" ||  // OBSOLETE. Support old files.
                 tag == "drummap" ||      // OBSOLETE. Support old files.
                 tag == "drumMapPatch")
        {
          // false = Do not fill in unused items.
          _workingDrumMapPatchList->read(xml, false);
          doUpdateDrummap = true;
        }

        else
          xml.unknown("our_drum_settings");
        break;

      case Xml::TagEnd:
        if (tag == "our_drum_settings")
        {
          if(doUpdateDrummap)
          {
            // We must ensure that there are NO duplicate enote fields,
            //  since the instrument map may have changed by now.
            //normalizeWorkingDrumMapPatchList();

            updateDrummap(false);
          }
          return;
        }

      default:
        break;
    }
  }
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

bool Track::selectEvents(bool select, unsigned long t0, unsigned long t1)
{
  bool ret = false;
  PartList* pl = parts();
  for(iPart ip = pl->begin(); ip != pl->end(); ++ip)
  {
    if(ip->second->selectEvents(select, t0, t1))
      ret = true;
  }
  return ret;
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
      xml.intTag(level, "recMonitor", _recMonitor);
      if (_selected)
      {
            xml.intTag(level, "selected", _selected);
            xml.intTag(level, "selectionOrder", _selectionOrder);
      }
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
        if(_channels > MusECore::MAX_CHANNELS)
          _channels = MusECore::MAX_CHANNELS;
      }      
      else if (tag == "locked")
            _locked = xml.parseInt();
      else if (tag == "recMonitor")
            setRecMonitor(xml.parseInt());
      else if (tag == "selected")
            _selected = xml.parseInt();
      else if (tag == "selectionOrder")
            _selectionOrder = xml.parseInt();
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
        // Ignore Audio Output to Audio Input routes.
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

// returns the tick where this CC gets overridden by a new one
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

//---------------------------------------------------------
//   updateDrummap
//   If audio is running (and not idle) this should only be called by the rt audio thread.
//   Returns true if map was changed.
//---------------------------------------------------------

bool MidiTrack::updateDrummap(int doSignal)
{
  if(type() != Track::NEW_DRUM || _outPort < 0 || _outPort >= MIDI_PORTS)
    return false;
  MidiPort* mp = &MusEGlobal::midiPorts[_outPort];
  const int patch = mp->hwCtrlState(_outChannel, CTRL_PROGRAM);
  bool map_changed;
  DrumMap ndm;

  map_changed = false;
  for(int i = 0; i < 128; i++)
  {
    getMapItem(patch, i, ndm, WorkingDrumMapEntry::AllOverrides);
    DrumMap& tdm = _drummap[i];
    if(ndm != tdm)
    {
      tdm = ndm;
      map_changed = true;
    }
    // Be sure to update the drum input note map. Probably wise (and easy) to do it always.
    drum_in_map[(int)tdm.enote] = i;
  }

  // Ensure there are NO duplicate enote fields. Returns true if somethng was changed.
  if(normalizeDrumMap(patch))
    map_changed = true;

  if(map_changed)
  {
    // Update the drum in (enote) map.
    update_drum_in_map();

    // TODO Move this to gui thread where it's safe to do so - this is only gui stuff.
    if(drummap_ordering_tied_to_patch())
      // TODO This is not exactly rt friendly since it may de/allocate.
      init_drum_ordering();
  }

  // TODO Do this outside since we may be called as part of multiple tracks operations.
  if(map_changed && doSignal)
  {
    // It is possible we are being called from gui thread already, in audio idle mode.
    // Will this still work, and not conflict with audio sending the same message?
    // Are we are not supposed to write to an fd from different threads?
    if(!MusEGlobal::audio || MusEGlobal::audio->isIdle())
      // Directly emit SC_DRUMMAP song changed signal.
      MusEGlobal::song->update(SC_DRUMMAP);
    else
      // Tell the gui to emit SC_DRUMMAP song changed signal.
      MusEGlobal::audio->sendMsgToGui('D'); // Drum map changed.

    return true;
  }

  return map_changed;
}

void MidiTrack::set_drummap_ordering_tied_to_patch(bool val)
{
  _drummap_ordering_tied_to_patch=val;
  if (val) init_drum_ordering();
}

void MidiTrack::modifyWorkingDrumMap(WorkingDrumMapList& list, bool isReset, bool includeDefault, bool
#ifdef _USE_INSTRUMENT_OVERRIDES_
isInstrumentMod
#endif
, bool doWholeMap)
{
  //if(!isDrumTrack())
  if(type() != NEW_DRUM)
    return;
  const int port = outPort();
  if(port < 0 || port >= MusECore::MIDI_PORTS)
    return;
  MidiPort* mp = &MusEGlobal::midiPorts[port];
  const int chan = outChannel();
  const int patch = mp->hwCtrlState(chan, MusECore::CTRL_PROGRAM);

  int index;
  int idx_end;
  int other_index;
  int fields;
  int cur_enote;
  int new_enote;
//   DrumMap orig_dm;
  DrumMap other_dm;
  WorkingDrumMapEntry other_wdme;
#ifdef _USE_INSTRUMENT_OVERRIDES_
  MidiInstrument* instr = mp->instrument();
#endif
  for(iWorkingDrumMapPatch_t iwdp = list.begin(); iwdp != list.end(); ++iwdp)
  {
    index = doWholeMap ? 0 : iwdp->first;
    idx_end = doWholeMap ? 128 : index + 1;
    for( ; index < idx_end; ++index)
    {
      DrumMap& dm = _drummap[index];
      WorkingDrumMapEntry& wdme = iwdp->second;

      fields = wdme._fields;

#ifdef _USE_INSTRUMENT_OVERRIDES_
      if(isInstrumentMod)
      {
        if(instr)
          instr->setWorkingDrumMapItem(patch, index, wdme, isReset);
      }
      else
#endif

      // FIXME Possible non realtime-friendly allocation. There will be adding new list and copying of 'name' QString here.
      if(isReset)
      {
//         cur_enote = dm.enote;
        _workingDrumMapPatchList->remove(patch, index, wdme._fields, includeDefault);
        getMapItem(patch, index, dm, WorkingDrumMapEntry::AllOverrides);
// REMOVE Tim. newdrums. Removed.
//         new_enote = dm.enote;
//         other_index = drum_in_map[new_enote];
//
//         if(fields & WorkingDrumMapEntry::ENoteField && other_index != index)
//         {
//           // In doWholeMap mode, a previous index iteration may have already cleared the other ENote field.
//           // So do this only if there is a track override on the ENote field.
//           if(//doWholeMap &&
//             (isWorkingMapItem(other_index, WorkingDrumMapEntry::ENoteField, patch) &
//               (WorkingDrumMapEntry::TrackOverride | WorkingDrumMapEntry::TrackDefaultOverride)))
//           {
//             // Here we need to see the original map item value /before/ any overrides, so that we can
//             //  tell whether this other_index brute-force 'reset' value is still technically an
//             //  override, and either remove or add (modify) the list appropriately.
//             getMapItem(patch, other_index, other_dm, WorkingDrumMapEntry::NoOverride);
//             if(other_dm.enote == cur_enote)
//             {
//               // The values are equal. This is technically no longer a track override and we may remove it.
//               _workingDrumMapPatchList->remove(patch, other_index, WorkingDrumMapEntry::ENoteField, includeDefault);
//             }
//             else
//             {
//               // The values are not equal. This is technically still a track override, so add (modify) it.
//               other_dm.enote = cur_enote;
//               WorkingDrumMapEntry other_wdme(other_dm, WorkingDrumMapEntry::ENoteField);
//               _workingDrumMapPatchList->add(patch, other_index, other_wdme);
//             }
//
//             _drummap[other_index].enote = cur_enote;
//             drum_in_map[cur_enote] = other_index;
//           }
//           drum_in_map[new_enote] = index;
//         }
      }
      else
      {
        cur_enote = dm.enote;
        if(includeDefault)
        {
          // We are 'promoting' the fields to default patch list...
          other_wdme._fields = fields;
          other_wdme._mapItem = dm;
          // Add the item to the default patch drum list.
          _workingDrumMapPatchList->add(CTRL_PROGRAM_VAL_DONT_CARE, index, other_wdme);
          // Now remove the item from the non-default patch drum list.
          if(patch != CTRL_PROGRAM_VAL_DONT_CARE)
            _workingDrumMapPatchList->remove(patch, index, WorkingDrumMapEntry::AllFields, false); // Do not include defaults.
        }
        else
        {
          if(doWholeMap)
          {
            if(fields == WorkingDrumMapEntry::AllFields)
            {
              other_wdme._fields = fields;
              other_wdme._mapItem = dm;
              _workingDrumMapPatchList->add(patch, index, other_wdme);
            }
            else
              _workingDrumMapPatchList->add(patch, index, wdme);
          }
          else
          {
            _workingDrumMapPatchList->add(patch, index, wdme);
            getMapItem(patch, index, dm, WorkingDrumMapEntry::AllOverrides);
          }
        }

        if(!doWholeMap && (fields & WorkingDrumMapEntry::ENoteField))
        {
          new_enote = dm.enote;
          other_index = drum_in_map[new_enote];
          // If there is already another track override on the other index we must change it.
          if(isWorkingMapItem(other_index, WorkingDrumMapEntry::ENoteField, patch) != WorkingDrumMapEntry::NoOverride)
          {
            other_dm.enote = cur_enote;
            //WorkingDrumMapEntry other_wdme(other_dm, WorkingDrumMapEntry::ENoteField);
            other_wdme._mapItem = other_dm;
            other_wdme._fields = WorkingDrumMapEntry::ENoteField;
            if(includeDefault)
            {
              _workingDrumMapPatchList->add(CTRL_PROGRAM_VAL_DONT_CARE, other_index, other_wdme);
              // Now remove the item from the non-default patch drum list.
              if(patch != CTRL_PROGRAM_VAL_DONT_CARE)
                _workingDrumMapPatchList->remove(patch, other_index, WorkingDrumMapEntry::ENoteField, false); // Do not include defaults.
            }
            else
              _workingDrumMapPatchList->add(patch, other_index, other_wdme);

            //_drummap[other_index].enote = cur_enote;
            //drum_in_map[cur_enote] = other_index;
            //drum_in_map[new_enote] = index;
          }
        }
      }
    }
  }

  // Ensure there are NO duplicate enote fields.
  //if(normalizeDrumMap(patch))
    // If anything changed, update the drum in map.
  //  update_drum_in_map();
  updateDrummap(false); // No signal.
}

void MidiTrack::setWorkingDrumMap(WorkingDrumMapPatchList* list, bool
#ifdef _USE_INSTRUMENT_OVERRIDES_
isInstrumentMod
#endif
)
{
  //if(!isDrumTrack())
  if(type() != NEW_DRUM)
    return;

#ifdef _USE_INSTRUMENT_OVERRIDES_
  if(isInstrumentMod)
  {
// TODO
//     const int port = outPort();
//     if(port < 0 || port >= MIDI_PORTS)
//       return;
//     MidiPort* mp = &MusEGlobal::midiPorts[port];
//     MidiInstrument* instr = mp->instrument();
//     instr->setWorkingDrumMap();
    return;
  }
#endif

  _workingDrumMapPatchList = list;

  // We must ensure that there are NO duplicate enote fields,
  //  since the instrument map may have changed by now.
  //normalizeWorkingDrumMapPatchList();

  updateDrummap(false); // No signal.
  update_drum_in_map();
}

void MidiTrack::getMapItemAt(int tick, int index, DrumMap& dest_map, int overrideType) const
{
  //if(!isDrumTrack())
  if(type() != NEW_DRUM)
  {
    dest_map = iNewDrumMap[index];
    return;
  }
  const int port = outPort();
  if(port < 0 || port >= MusECore::MIDI_PORTS)
  {
    dest_map = iNewDrumMap[index];
    return;
  }
  const MidiPort* mp = &MusEGlobal::midiPorts[port];
  const int track_chan = outChannel();

  // Get the patch number at tick, contributed by any part,
  //  ignoring values outside of their parts. We must include
  //  muted or off parts or tracks in the search since this is an
  //  operation that must not be affected by mute or off.
  const int track_patch = mp->getVisibleCtrl(track_chan, tick, MusECore::CTRL_PROGRAM, true, true, true);

  // Get the instrument's map item, and include any requested overrides.
  getMapItem(track_patch, index, dest_map, overrideType);
}

void MidiTrack::getMapItem(int patch, int index, DrumMap& dest_map, int overrideType) const
{
  //if(!isDrumTrack())
  if(type() != NEW_DRUM)
  {
    dest_map = iNewDrumMap[index];
    return;
  }
  const int port = outPort();
  if(port < 0 || port >= MusECore::MIDI_PORTS)
  {
    dest_map = iNewDrumMap[index];
    return;
  }
  const MidiPort* mp = &MusEGlobal::midiPorts[port];
  const MidiInstrument* midi_instr = mp->instrument();
  if(!midi_instr)
  {
    dest_map = iNewDrumMap[index];
    return;
  }

  // Get the instrument's map item, and include any requested overrides.
  const int channel = outChannel();
  midi_instr->getMapItem(channel, patch, index, dest_map, overrideType);

  // Did we request to include any track default patch overrides?
  if(overrideType & WorkingDrumMapEntry::TrackDefaultOverride)
  {
    // Get any track default patch overrides.
    const WorkingDrumMapEntry* def_wdm = _workingDrumMapPatchList->find(CTRL_PROGRAM_VAL_DONT_CARE, index, false); // No default.
    if(def_wdm)
    {
      if(def_wdm->_fields & WorkingDrumMapEntry::NameField)
        dest_map.name = def_wdm->_mapItem.name;

      if(def_wdm->_fields & WorkingDrumMapEntry::VolField)
        dest_map.vol = def_wdm->_mapItem.vol;

      if(def_wdm->_fields & WorkingDrumMapEntry::QuantField)
        dest_map.quant = def_wdm->_mapItem.quant;

      if(def_wdm->_fields & WorkingDrumMapEntry::LenField)
        dest_map.len = def_wdm->_mapItem.len;

      if(def_wdm->_fields & WorkingDrumMapEntry::ChanField)
        dest_map.channel = def_wdm->_mapItem.channel;

      if(def_wdm->_fields & WorkingDrumMapEntry::PortField)
        dest_map.port = def_wdm->_mapItem.port;

      if(def_wdm->_fields & WorkingDrumMapEntry::Lv1Field)
        dest_map.lv1 = def_wdm->_mapItem.lv1;

      if(def_wdm->_fields & WorkingDrumMapEntry::Lv2Field)
        dest_map.lv2 = def_wdm->_mapItem.lv2;

      if(def_wdm->_fields & WorkingDrumMapEntry::Lv3Field)
        dest_map.lv3 = def_wdm->_mapItem.lv3;

      if(def_wdm->_fields & WorkingDrumMapEntry::Lv4Field)
        dest_map.lv4 = def_wdm->_mapItem.lv4;

      if(def_wdm->_fields & WorkingDrumMapEntry::ENoteField)
        dest_map.enote = def_wdm->_mapItem.enote;

      if(def_wdm->_fields & WorkingDrumMapEntry::ANoteField)
        dest_map.anote = def_wdm->_mapItem.anote;

      if(def_wdm->_fields & WorkingDrumMapEntry::MuteField)
        dest_map.mute = def_wdm->_mapItem.mute;

      if(def_wdm->_fields & WorkingDrumMapEntry::HideField)
        dest_map.hide = def_wdm->_mapItem.hide;
    }
  }

  // Did we request to include any track overrides?
  if(!(overrideType & WorkingDrumMapEntry::TrackOverride))
    return;

  // Get any track overrides.
  const WorkingDrumMapEntry* wdm = _workingDrumMapPatchList->find(patch, index, false); // No default.
  if(!wdm)
    return;

  if(wdm->_fields & WorkingDrumMapEntry::NameField)
    dest_map.name = wdm->_mapItem.name;

  if(wdm->_fields & WorkingDrumMapEntry::VolField)
    dest_map.vol = wdm->_mapItem.vol;

  if(wdm->_fields & WorkingDrumMapEntry::QuantField)
    dest_map.quant = wdm->_mapItem.quant;

  if(wdm->_fields & WorkingDrumMapEntry::LenField)
    dest_map.len = wdm->_mapItem.len;

  if(wdm->_fields & WorkingDrumMapEntry::ChanField)
    dest_map.channel = wdm->_mapItem.channel;

  if(wdm->_fields & WorkingDrumMapEntry::PortField)
    dest_map.port = wdm->_mapItem.port;

  if(wdm->_fields & WorkingDrumMapEntry::Lv1Field)
    dest_map.lv1 = wdm->_mapItem.lv1;

  if(wdm->_fields & WorkingDrumMapEntry::Lv2Field)
    dest_map.lv2 = wdm->_mapItem.lv2;

  if(wdm->_fields & WorkingDrumMapEntry::Lv3Field)
    dest_map.lv3 = wdm->_mapItem.lv3;

  if(wdm->_fields & WorkingDrumMapEntry::Lv4Field)
    dest_map.lv4 = wdm->_mapItem.lv4;

  if(wdm->_fields & WorkingDrumMapEntry::ENoteField)
    dest_map.enote = wdm->_mapItem.enote;

  if(wdm->_fields & WorkingDrumMapEntry::ANoteField)
    dest_map.anote = wdm->_mapItem.anote;

  if(wdm->_fields & WorkingDrumMapEntry::MuteField)
    dest_map.mute = wdm->_mapItem.mute;

  if(wdm->_fields & WorkingDrumMapEntry::HideField)
    dest_map.hide = wdm->_mapItem.hide;
}

int MidiTrack::isWorkingMapItem(int index, int fields, int patch) const
{
  int ret = WorkingDrumMapEntry::NoOverride;
  if(type() != NEW_DRUM)
    return ret;

  // Is there an instrument override for this drum map item?
  const int port = outPort();
  if(port >= 0 && port < MusECore::MIDI_PORTS)
  {
    const MidiPort* mp = &MusEGlobal::midiPorts[port];
    // Grab the patch number while we are here, if we asked for it.
    if(patch == -1)
    {
      const int chan = outChannel();
      patch = mp->hwCtrlState(chan, CTRL_PROGRAM);
    }
#ifdef _USE_INSTRUMENT_OVERRIDES_
    const MidiInstrument* midi_instr = mp->instrument();
    if(midi_instr)
      ret |= midi_instr->isWorkingMapItem(patch, index, fields);
#endif
  }

  // Is there a local track default patch override for this drum map item?
  const WorkingDrumMapEntry* def_wdm = _workingDrumMapPatchList->find(CTRL_PROGRAM_VAL_DONT_CARE, index, false); // No default.
  if(def_wdm && (def_wdm->_fields & fields))
    ret |= WorkingDrumMapEntry::TrackDefaultOverride;

  if(patch != -1)
  {
    // Is there a local track override for this drum map item?
    const WorkingDrumMapEntry* wdm = _workingDrumMapPatchList->find(patch, index, false); // No default.
    if(wdm && (wdm->_fields & fields))
      ret |= WorkingDrumMapEntry::TrackOverride;
  }

  return ret;
}

bool MidiTrack::normalizeDrumMap(int patch)
{
  if(type() != NEW_DRUM)
    return false;
  //WorkingDrumMapList* wdml = _workingDrumMapPatchList->find(patch, true);
  WorkingDrumMapList* wdml = _workingDrumMapPatchList->find(patch, false);
  WorkingDrumMapList* def_wdml = 0;
  if(patch != CTRL_PROGRAM_VAL_DONT_CARE)
    def_wdml = _workingDrumMapPatchList->find(CTRL_PROGRAM_VAL_DONT_CARE, false);

  int index = 0;
  DrumMap dm;
  char enote;
  bool changed = false;

  bool used_index[128];
  int used_enotes[128];
  for(int i = 0; i < 128; ++i)
  {
    used_index[i] = false;
    used_enotes[i] = 0;
  }
  char unused_enotes[128];
  int unused_enotes_sz = 0;
  char unused_index[128];
  int unused_index_sz = 0;
  int unused_enotes_cnt = 0;

  // Find all the used enote fields and their indexes in the working list.
  if(wdml)
  {
    for(iWorkingDrumMapPatch_t iwdml = wdml->begin(); iwdml != wdml->end(); ++iwdml)
    {
      WorkingDrumMapEntry& wdme = iwdml->second;
      if(wdme._fields & WorkingDrumMapEntry::ENoteField)
      {
        used_index[iwdml->first] = true;
        //++used_enotes[(unsigned char)wdme._mapItem.enote];
      }
    }
  }

  // Add all the used enote fields and their indexes in the default patch working list.
  if(def_wdml)
  {
    for(iWorkingDrumMapPatch_t iwdml = def_wdml->begin(); iwdml != def_wdml->end(); ++iwdml)
    {
      WorkingDrumMapEntry& wdme = iwdml->second;
      if(wdme._fields & WorkingDrumMapEntry::ENoteField)
      {
        used_index[iwdml->first] = true;
        //++used_enotes[(unsigned char)wdme._mapItem.enote];
      }
    }
  }

  // Find all the used enote fields and their indexes in the working list.
  if(wdml)
  {
    for(iWorkingDrumMapPatch_t iwdml = wdml->begin(); iwdml != wdml->end(); ++iwdml)
    {
      WorkingDrumMapEntry& wdme = iwdml->second;
      if(wdme._fields & WorkingDrumMapEntry::ENoteField)
      {
        //used_index[iwdml->first] = true;
        ++used_enotes[(unsigned char)wdme._mapItem.enote];
      }
    }
  }

  // Find all the unused indexes and enotes so far in the working list.
  unused_index_sz = 0;
  unused_enotes_sz = 0;
  for(int i = 0; i < 128; ++i)
  {
    if(!used_index[i])
      unused_index[unused_index_sz++] = i;
    if(used_enotes[i] == 0)
      unused_enotes[unused_enotes_sz++] = i;
  }

  // Ensure there are NO duplicate enotes in the existing working list items so far.
  unused_enotes_cnt = 0;
  if(wdml)
  {
    for(iWorkingDrumMapPatch_t iwdml = wdml->begin(); iwdml != wdml->end(); ++iwdml)
    {
      WorkingDrumMapEntry& wdme = iwdml->second;
      if(wdme._fields & WorkingDrumMapEntry::ENoteField)
      {
        // More than 1 (this) usage?
        if(used_enotes[(unsigned char)wdme._mapItem.enote] > 1)
        {
          fprintf(stderr, "MidiTrack::normalizeWorkingDrumMap: Warning: Duplicate enote:%d found. Overriding it.\n",
                  wdme._mapItem.enote);
          if(unused_enotes_cnt >= unused_enotes_sz)
          {
            fprintf(stderr, "MidiTrack::normalizeWorkingDrumMap: Error: unused_enotes_cnt >= unused_enotes_sz:%d\n",
                    unused_enotes_sz);
            break;
          }
          --used_enotes[(unsigned char)wdme._mapItem.enote];
          //wdme._mapItem.enote = unused_enotes[unused_enotes_cnt++];
          // Get the instrument item.
          index = iwdml->first;
          // Modify the enote field.
          enote = unused_enotes[unused_enotes_cnt++];
          _drummap[index].enote = enote;
          ++used_enotes[(unsigned char)enote];
          changed = true;
        }
      }
    }
  }

  // Find all the used enote fields and their indexes in the default patch working list.
  if(def_wdml)
  {
    for(iWorkingDrumMapPatch_t iwdml = def_wdml->begin(); iwdml != def_wdml->end(); ++iwdml)
    {
      WorkingDrumMapEntry& wdme = iwdml->second;
      if(wdme._fields & WorkingDrumMapEntry::ENoteField)
      {
        //used_index[iwdml->first] = true;
        // If there is already a non-default patch enote override for this index,
        //  do not increment used_notes, the non-default one takes priority over this default one.
        if(wdml)
        {
          ciWorkingDrumMapPatch_t def_iwdml = wdml->find(iwdml->first);
          if(def_iwdml != wdml->end())
          {
            const WorkingDrumMapEntry& def_wdme = def_iwdml->second;
            if(def_wdme._fields & WorkingDrumMapEntry::ENoteField)
              continue;
          }
        }
        ++used_enotes[(unsigned char)wdme._mapItem.enote];
      }
    }
  }

  // Find all the unused indexes and enotes so far in the working list.
  unused_enotes_sz = 0;
  for(int i = 0; i < 128; ++i)
  {
    if(used_enotes[i] == 0)
      unused_enotes[unused_enotes_sz++] = i;
  }

  // Ensure there are NO duplicate enotes in the existing default patch working list items so far.
  unused_enotes_cnt = 0;
  if(def_wdml)
  {
    for(iWorkingDrumMapPatch_t iwdml = def_wdml->begin(); iwdml != def_wdml->end(); ++iwdml)
    {
      WorkingDrumMapEntry& wdme = iwdml->second;
      if(wdme._fields & WorkingDrumMapEntry::ENoteField)
      {
        // If there is already a non-default patch enote override for this index,
        //  skip this one, the non-default one takes priority over this default one.
        if(wdml)
        {
          ciWorkingDrumMapPatch_t def_iwdml = wdml->find(iwdml->first);
          if(def_iwdml != wdml->end())
          {
            const WorkingDrumMapEntry& def_wdme = def_iwdml->second;
            if(def_wdme._fields & WorkingDrumMapEntry::ENoteField)
              continue;
          }
        }

        // More than 1 (this) usage?
        if(used_enotes[(unsigned char)wdme._mapItem.enote] > 1)
        {
          fprintf(stderr, "MidiTrack::normalizeWorkingDrumMap: Warning: Duplicate default enote:%d found. Overriding it.\n",
                  wdme._mapItem.enote);
          if(unused_enotes_cnt >= unused_enotes_sz)
          {
            fprintf(stderr, "MidiTrack::normalizeWorkingDrumMap: Error: Default unused_enotes_cnt >= unused_enotes_sz:%d\n",
                    unused_enotes_sz);
            break;
          }
          --used_enotes[(unsigned char)wdme._mapItem.enote];
          //wdme._mapItem.enote = unused_enotes[unused_enotes_cnt++];
          // Get the instrument item.
          index = iwdml->first;
          // Modify the enote field.
          enote = unused_enotes[unused_enotes_cnt++];
          _drummap[index].enote = enote;
          ++used_enotes[(unsigned char)enote];
          changed = true;
        }
      }
    }
  }

  // Add all used enotes in the unused enote indexes (the instrument fields).
  for(int i = 0; i < unused_index_sz; ++i)
    ++used_enotes[(unsigned char)_drummap[(unsigned char)unused_index[i]].enote];

  // Find all the unused enotes.
  unused_enotes_sz = 0;
  for(int i = 0; i < 128; ++i)
  {
    if(used_enotes[i] == 0)
      unused_enotes[unused_enotes_sz++] = i;
  }

  // Ensure there are NO duplicate enotes in the unused enote map fields (the instrument fields).
  unused_enotes_cnt = 0;
  for(int i = 0; i < unused_index_sz; ++i)
  {
    // Get the instrument item.
    index = unused_index[i];
    enote = _drummap[index].enote;

    // More than 1 (this) usage?
    if(used_enotes[(unsigned char)enote] > 1)
    {
      if(unused_enotes_cnt >= unused_enotes_sz)
      {
        fprintf(stderr, "MidiTrack::normalizeWorkingDrumMap: Error filling background items: unused_enotes_cnt >= unused_enotes_sz:%d\n",
                unused_enotes_sz);
        break;
      }

      --used_enotes[(unsigned char)enote];

      // Modify the enote field.
      _drummap[index].enote = unused_enotes[unused_enotes_cnt++];
      ++used_enotes[(unsigned char)_drummap[index].enote];
      changed = true;
    }
  }

  return changed;
}

bool MidiTrack::normalizeDrumMap()
{
  if(type() != NEW_DRUM)
    return false;
  const int port = outPort();
  if(port < 0 || port >= MusECore::MIDI_PORTS)
    return false;
  const int chan = outChannel();
  const int patch = MusEGlobal::midiPorts[port].hwCtrlState(chan, MusECore::CTRL_PROGRAM);
  return normalizeDrumMap(patch);
}

} // namespace MusECore
