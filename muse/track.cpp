//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: track.cpp,v 1.34.2.11 2009/11/30 05:05:49 terminator356 Exp $
//
//  (C) Copyright 2000-2004 Werner Schweer (ws@seh.de)
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

unsigned int Track::_soloRefCnt = 0;
Track* Track::_tmpSoloChainTrack = 0;
bool Track::_tmpSoloChainDoIns   = false;
bool Track::_tmpSoloChainNoDec   = false;

const char* Track::_cname[] = {
      "Midi", "Drum", "Wave", "AudioOut", "AudioIn", "AudioGroup", 
      "AudioAux", "AudioSynth"
      };

//---------------------------------------------------------
//   addPortCtrlEvents
//---------------------------------------------------------

void addPortCtrlEvents(MidiTrack* t)
{
  const PartList* pl = t->cparts();
  for(ciPart ip = pl->begin(); ip != pl->end(); ++ip)
  {
    Part* part = ip->second;
    const EventList* el = part->cevents();
    unsigned len = part->lenTick();
    for(ciEvent ie = el->begin(); ie != el->end(); ++ie)
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
        
        MidiPort* mp = &midiPorts[t->outPort()];
        // Is it a drum controller event, according to the track port's instrument?
        if(t->type() == Track::DRUM)
        {
          MidiController* mc = mp->drumController(cntrl);
          if(mc)
          {
            int note = cntrl & 0x7f;
            cntrl &= ~0xff;
            ch = drumMap[note].channel;
            mp = &midiPorts[drumMap[note].port];
            cntrl |= drumMap[note].anote;
          }
        }
        
        mp->setControllerVal(ch, tick, cntrl, val, part);
      }
    }
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
    const EventList* el = part->cevents();
    //unsigned len = part->lenTick();
    for(ciEvent ie = el->begin(); ie != el->end(); ++ie)
    {
      const Event& ev = ie->second;
      // Added by T356. Do not remove events which are past the end of the part.
      // No, actually, do remove ALL of them belonging to the part.
      // Just in case there are stray values left after the part end.
      //if(ev.tick() >= len)
      //  break;
                    
      if(ev.type() == Controller)
      {
        int tick  = ev.tick() + part->tick();
        int cntrl = ev.dataA();
        int ch = t->outChannel();
        
        MidiPort* mp = &midiPorts[t->outPort()];
        // Is it a drum controller event, according to the track port's instrument?
        if(t->type() == Track::DRUM)
        {
          MidiController* mc = mp->drumController(cntrl);
          if(mc)
          {
            int note = cntrl & 0x7f;
            cntrl &= ~0xff;
            ch = drumMap[note].channel;
            mp = &midiPorts[drumMap[note].port];
            cntrl |= drumMap[note].anote;
          }
        }
        
        mp->deleteController(ch, tick, cntrl, part);
      }
    }
  }
}

//---------------------------------------------------------
//   y
//---------------------------------------------------------

int Track::y() const
      {
      TrackList* tl = song->tracks();
      int yy = 0;
      for (ciTrack it = tl->begin(); it != tl->end(); ++it) {
            if (this == *it)
                  return yy;
            yy += (*it)->height();
            }
      printf("Track::y(%s): track not in tracklist\n", name().toLatin1().constData());
      return -1;
      }

//---------------------------------------------------------
//   Track::init
//---------------------------------------------------------

void Track::init()
      {
      _activity      = 0;
      _lastActivity  = 0;
      _recordFlag    = false;
      _mute          = false;
      _solo          = false;
      _internalSolo  = 0;
      _off           = false;
      _channels      = 0;           // 1 - mono, 2 - stereo
      
      _volumeEnCtrl  = true;
      _volumeEn2Ctrl = true;
      _panEnCtrl     = true;
      _panEn2Ctrl    = true;
      
      _selected      = false;
      _height        = 20;
      _locked        = false;
      for (int i = 0; i < MAX_CHANNELS; ++i) {
            //_meter[i] = 0;
            //_peak[i]  = 0;
            _meter[i] = 0.0;
            _peak[i]  = 0.0;
            }
      }

Track::Track(Track::TrackType t)
      {
      init();
      _type = t;
      }

//Track::Track(const Track& t)
Track::Track(const Track& t, bool cloneParts)
      {
      _activity     = t._activity;
      _lastActivity = t._lastActivity;
      _recordFlag   = t._recordFlag;
      _mute         = t._mute;
      _solo         = t._solo;
      _internalSolo = t._internalSolo;
      _off          = t._off;
      _channels     = t._channels;
      
      _volumeEnCtrl  = t._volumeEnCtrl;
      _volumeEn2Ctrl = t._volumeEn2Ctrl;
      _panEnCtrl     = t._panEnCtrl;
      _panEn2Ctrl    = t._panEn2Ctrl;
      
      _selected     = t.selected();
      _y            = t._y;
      _height       = t._height;
      _comment      = t.comment();
      _name         = t.name();
      _type         = t.type();
      _locked       = t.locked();

      if(cloneParts)
      {
        const PartList* pl = t.cparts();
        for (ciPart ip = pl->begin(); ip != pl->end(); ++ip) {
              Part* newPart = ip->second->clone();
              newPart->setTrack(this);
              _parts.add(newPart);
              }
      }
      else
      {
        _parts = *(t.cparts());
        // NOTE: We can't do this because of the way clipboard, cloneList, and undoOp::ModifyTrack, work.
        // A couple of schemes were conceived to deal with cloneList being invalid, but the best way is
        //  to not alter the part list here. It's a big headache because: Either the parts in the cloneList
        //  need to be reliably looked up replaced with the new ones, or the clipboard and cloneList must be cleared.
        // Fortunately the ONLY part of muse using this function is track rename (in TrackList and TrackInfo).
        // So we can get away with leaving this out: 
        //for (iPart ip = _parts.begin(); ip != _parts.end(); ++ip) 
        //      ip->second->setTrack(this);
      }
              
      for (int i = 0; i < MAX_CHANNELS; ++i) {
            //_meter[i] = 0;
            //_peak[i]  = 0;
            _meter[i] = 0.0;
            _peak[i]  = 0.0;
            }
      }

//---------------------------------------------------------
//   operator =
//   Added by Tim. Parts' track members MUST point to this track, 
//    not some other track, so simple assignment operator won't do!
//---------------------------------------------------------

Track& Track::operator=(const Track& t) 
{
      _activity     = t._activity;
      _lastActivity = t._lastActivity;
      _recordFlag   = t._recordFlag;
      _mute         = t._mute;
      _solo         = t._solo;
      _internalSolo = t._internalSolo;
      _off          = t._off;
      _channels     = t._channels;
      
      _volumeEnCtrl  = t._volumeEnCtrl;
      _volumeEn2Ctrl = t._volumeEn2Ctrl;
      _panEnCtrl     = t._panEnCtrl;
      _panEn2Ctrl    = t._panEn2Ctrl;
      
      _selected     = t.selected();
      _y            = t._y;
      _height       = t._height;
      _comment      = t.comment();
      _name         = t.name();
      _type         = t.type();
      _locked       = t.locked();

      _parts = *(t.cparts());
      // NOTE: Can't do this. See comments in copy constructor.
      //for (iPart ip = _parts.begin(); ip != _parts.end(); ++ip) 
      //      ip->second->setTrack(this);
      
      for (int i = 0; i < MAX_CHANNELS; ++i) {
            _meter[i] = t._meter[i];
            _peak[i]  = t._peak[i];
            }
     return *this;       
}

//---------------------------------------------------------
//   setDefaultName
//    generate unique name for track
//---------------------------------------------------------

void Track::setDefaultName()
      {
      QString base;
      switch(_type) {
            case MIDI:
            case DRUM:
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
      for (int i = 1; true; ++i) {
            QString n;
            n.setNum(i);
            QString s = base + n;
            Track* track = song->findTrack(s);
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
    _volumeEnCtrl  = true;
    _volumeEn2Ctrl = true;
    _panEnCtrl     = true;
    _panEn2Ctrl    = true;
    
    if(isMidiTrack())
      return;
          
    AudioTrack *t = (AudioTrack*)this;
    Pipeline *pl = t->efxPipe();
    PluginI *p; 
    for(iPluginI i = pl->begin(); i != pl->end(); ++i) 
    {
      p = *i;
      if(!p)
        continue;
      p->enableAllControllers(true);
    }
      
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
//   MidiTrack
//---------------------------------------------------------

MidiTrack::MidiTrack()
   : Track(MIDI)
      {
      init();
      _events = new EventList;
      _mpevents = new MPEventList;
      }

//MidiTrack::MidiTrack(const MidiTrack& mt)
//   : Track(mt)
MidiTrack::MidiTrack(const MidiTrack& mt, bool cloneParts)
   : Track(mt, cloneParts)
      {
      _outPort       = mt.outPort();
      _outChannel    = mt.outChannel();
      ///_inPortMask    = mt.inPortMask();
      ///_inChannelMask = mt.inChannelMask();
      _events        = new EventList;
      _mpevents      = new MPEventList;
      transposition  = mt.transposition;
      velocity       = mt.velocity;
      delay          = mt.delay;
      len            = mt.len;
      compression    = mt.compression;
      _recEcho       = mt.recEcho();
      }

MidiTrack::~MidiTrack()
      {
      delete _events;
      delete _mpevents;
      }

//---------------------------------------------------------
//   init
//---------------------------------------------------------

void MidiTrack::init()
      {
      _outPort       = 0;
      _outChannel    = 0;
      // Changed by Tim. p3.3.8
      //_inPortMask    = 0xffff;
      ///_inPortMask    = 0xffffffff;
      
      ///_inChannelMask = 0xffff;      // "ALL"
      transposition  = 0;
      velocity       = 0;
      delay          = 0;
      len            = 100;          // percent
      compression    = 100;          // percent
      _recEcho       = true;
      }

//---------------------------------------------------------
//   setOutChanAndUpdate
//---------------------------------------------------------

void MidiTrack::setOutChanAndUpdate(int i)       
{ 
  if(_outChannel == i)
    return;
    
  //removePortCtrlEvents();
  removePortCtrlEvents(this);
  _outChannel = i; 
  //addPortCtrlEvents();
  addPortCtrlEvents(this);
}

//---------------------------------------------------------
//   setOutPortAndUpdate
//---------------------------------------------------------

void MidiTrack::setOutPortAndUpdate(int i)
{
  if(_outPort == i)
    return;
  
  //removePortCtrlEvents();
  removePortCtrlEvents(this);
  _outPort = i; 
  //addPortCtrlEvents();
  addPortCtrlEvents(this);
}

//---------------------------------------------------------
//   setInPortAndChannelMask
//   For old song files with port mask (max 32 ports) and channel mask (16 channels), 
//    before midi routing was added (the iR button). p3.3.48
//---------------------------------------------------------

void MidiTrack::setInPortAndChannelMask(unsigned int portmask, int chanmask) 
{ 
  //if(!portmask || !chanmask)
  //  return;
     
  //RouteList* rl = inRoutes();
  bool changed = false;
  
  for(int port = 0; port < 32; ++port)  // 32 is the old maximum number of ports.
  {
    // p3.3.50 If the port was not used in the song file to begin with, just ignore it.
    // This saves from having all of the first 32 ports' channels connected.
    if(!midiPorts[port].foundInSongFile())
      continue;
      
    //if(!(portmask & (1 << port)))
    //  continue;
    
    // p3.3.50 Removed. Allow to connect to port with no device so user can change device later.
    //MidiPort* mp = &midiPorts[port];
    //MidiDevice* md = mp->device();
    //if(!md)
    //  continue;
        
    //for(int ch = 0; ch < MIDI_CHANNELS; ++ch)  // p3.3.50 Removed.
    //{
      //if(!(chanmask & (1 << ch)))
      //  continue;
    
      //Route aRoute(md, ch);
      //Route bRoute(this, ch);
      Route aRoute(port, chanmask);     // p3.3.50
      Route bRoute(this, chanmask);
    
      // p3.3.50 Removed.
      //iRoute iir = rl->begin();
      //for(; iir != rl->end(); ++iir) 
      //{
        //if(*iir == aRoute)
      //  if(iir->type == Route::MIDI_PORT_ROUTE && iir->midiPort == port)      // p3.3.50
      //    break;
      //}
      
      // Route wanted?
      //if((portmask & (1 << port)) && (chanmask & (1 << ch)))
      if(portmask & (1 << port))                                          // p3.3.50
      {
        // Route already exists?
        //if(iir != rl->end()) 
        //  continue;
        audio->msgAddRoute(aRoute, bRoute);
        changed = true;
      }
      else
      {
        // Route does not exist?
        //if(iir == rl->end()) 
        //  continue;
        audio->msgRemoveRoute(aRoute, bRoute);
        changed = true;
      }
    //}
  }
   
  if(changed)
  {
    audio->msgUpdateSoloStates();
    song->update(SC_ROUTE);
  }  
}

/*
//---------------------------------------------------------
//   addPortCtrlEvents
//---------------------------------------------------------

void MidiTrack::addPortCtrlEvents()
{
  const PartList* pl = cparts();
  for(ciPart ip = pl->begin(); ip != pl->end(); ++ip)
  {
    Part* part = ip->second;
    const EventList* el = part->cevents();
    for(ciEvent ie = el->begin(); ie != el->end(); ++ie)
    {
      const Event& ev = ie->second;
      if(ev.type() == Controller)
      {
        int tick  = ev.tick() + part->tick();
        int cntrl = ev.dataA();
        int val   = ev.dataB();
        int ch = _outChannel;
        
        MidiPort* mp = &midiPorts[_outPort];
        // Is it a drum controller event, according to the track port's instrument?
        if(type() == DRUM)
        {
          MidiController* mc = mp->drumController(cntrl);
          if(mc)
          {
            int note = cntrl & 0x7f;
            cntrl &= ~0xff;
            ch = drumMap[note].channel;
            mp = &midiPorts[drumMap[note].port];
            cntrl |= drumMap[note].anote;
          }
        }
        
        mp->setControllerVal(ch, tick, cntrl, val, part);
      }
    }
  }
}

//---------------------------------------------------------
//   removePortCtrlEvents
//---------------------------------------------------------

void MidiTrack::removePortCtrlEvents()
{
  const PartList* pl = cparts();
  for(ciPart ip = pl->begin(); ip != pl->end(); ++ip)
  {
    Part* part = ip->second;
    const EventList* el = part->cevents();
    for(ciEvent ie = el->begin(); ie != el->end(); ++ie)
    {
      const Event& ev = ie->second;
      if(ev.type() == Controller)
      {
        int tick  = ev.tick() + part->tick();
        int cntrl = ev.dataA();
        int ch = _outChannel;
        
        MidiPort* mp = &midiPorts[_outPort];
        // Is it a drum controller event, according to the track port's instrument?
        if(type() == DRUM)
        {
          MidiController* mc = mp->drumController(cntrl);
          if(mc)
          {
            int note = cntrl & 0x7f;
            cntrl &= ~0xff;
            ch = drumMap[note].channel;
            mp = &midiPorts[drumMap[note].port];
            cntrl |= drumMap[note].anote;
          }
        }
        
        mp->deleteController(ch, tick, cntrl, part);
      }
    }
  }
}
*/

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
//   newPart
//---------------------------------------------------------

Part* MidiTrack::newPart(Part*p, bool clone)
      {
      MidiPart* part = clone ? new MidiPart(this, p->events()) : new MidiPart(this);
      if (p) {
            part->setName(p->name());
            part->setColorIndex(p->colorIndex());

            *(PosLen*)part = *(PosLen*)p;
            part->setMute(p->mute());
            }
      
      if(clone)
        //p->chainClone(part);
        chainClone(p, part);
      
      return part;
      }

//---------------------------------------------------------
//   automationType
//---------------------------------------------------------

AutomationType MidiTrack::automationType() const
      {
      MidiPort* port = &midiPorts[outPort()];
      return port->automationType(outChannel());
      }

//---------------------------------------------------------
//   setAutomationType
//---------------------------------------------------------

void MidiTrack::setAutomationType(AutomationType t)
      {
      MidiPort* port = &midiPorts[outPort()];
      port->setAutomationType(outChannel(), t);
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
            s = QT_TRANSLATE_NOOP("@default", "Route");
            if(r->channel != -1)
              s += QString(QT_TRANSLATE_NOOP("@default", " channel=\"%1\"")).arg(r->channel);
            
            ///Route dst(name(), true, r->channel);
            //xml.tag(level++, "Route");
            xml.tag(level++, s.toAscii().constData());
            
            // p3.3.38 New routing scheme.
            ///xml.strTag(level, "srcNode", r->name());
            //xml.tag(level, "source type=\"%d\" name=\"%s\"/", r->type, r->name().toLatin1().constData());
            s = QT_TRANSLATE_NOOP("@default", "source");
            if(r->type != Route::TRACK_ROUTE)
              s += QString(QT_TRANSLATE_NOOP("@default", " type=\"%1\"")).arg(r->type);
            //s += QString(QT_TRANSLATE_NOOP("@default", " name=\"%1\"/")).arg(r->name());
            s += QString(QT_TRANSLATE_NOOP("@default", " name=\"%1\"/")).arg(Xml::xmlString(r->name()));
            xml.tag(level, s.toAscii().constData());
            
            ///xml.strTag(level, "dstNode", dst.name());
            
            //if(r->channel != -1)
            //  xml.tag(level, "dest type=\"%d\" channel=\"%d\" name=\"%s\"/", Route::TRACK_ROUTE, r->channel, name().toLatin1().constData());
            //else  
            //  xml.tag(level, "dest type=\"%d\" name=\"%s\"/", Route::TRACK_ROUTE, name().toLatin1().constData());

            //xml.tag(level, "dest name=\"%s\"/", name().toLatin1().constData());
            xml.tag(level, "dest name=\"%s\"/", Xml::xmlString(name()).toLatin1().constData());
            
            xml.etag(level--, "Route");
          }
        }
      }
      
      const RouteList* rl = &_outRoutes;
      for (ciRoute r = rl->begin(); r != rl->end(); ++r) 
      {
        //if(!r->name().isEmpty())
        if(r->midiPort != -1 || !r->name().isEmpty()) // p3.3.49
        {
          ///QString src(name());
          ///if (type() == Track::AUDIO_OUTPUT) 
          ///{ 
                ///Route s(src, false, r->channel);
                ///src = s.name();
          ///}
          
          s = QT_TRANSLATE_NOOP("@default", "Route");
          if(r->type == Route::MIDI_PORT_ROUTE)  // p3.3.50
          {
            if(r->channel != -1 && r->channel != 0)
              s += QString(QT_TRANSLATE_NOOP("@default", " channelMask=\"%1\"")).arg(r->channel);  // Use new channel mask.
          }
          else
          {
            if(r->channel != -1)
              s += QString(QT_TRANSLATE_NOOP("@default", " channel=\"%1\"")).arg(r->channel);
          }    
          if(r->channels != -1)
            s += QString(QT_TRANSLATE_NOOP("@default", " channels=\"%1\"")).arg(r->channels);
          if(r->remoteChannel != -1)
            s += QString(QT_TRANSLATE_NOOP("@default", " remch=\"%1\"")).arg(r->remoteChannel);
          
          //xml.tag(level++, "Route");
          xml.tag(level++, s.toAscii().constData());
          
          ///xml.strTag(level, "srcNode", src);
          //if(r->channel != -1)
          
          // Allow for a regular mono or stereo track to feed a multi-channel synti. 
          // thisChannel is the 'starting' channel of this source if feeding a regular track.
          //if(r->type == Route::TRACK_ROUTE && r->track->isSynti() && r->channel != -1)
          //if(isSynti() && r->thisChannel != -1)
            //xml.tag(level, "source type=\"%d\" channel=\"%d\" name=\"%s\"/", Route::TRACK_ROUTE, r->channel, name().toLatin1().constData());
          //  xml.tag(level, "source type=\"%d\" channel=\"%d\" name=\"%s\"/", Route::TRACK_ROUTE, r->thisChannel, name().toLatin1().constData());
          //else
          
          //if(r->channel != -1)
          //  xml.tag(level, "source type=\"%d\" channel=\"%d\" name=\"%s\"/", Route::TRACK_ROUTE, r->channel, name().toLatin1().constData());
          //else  
          //  xml.tag(level, "source type=\"%d\" name=\"%s\"/", Route::TRACK_ROUTE, name().toLatin1().constData());
          //xml.tag(level, "source name=\"%s\"/", name().toLatin1().constData());
          xml.tag(level, "source name=\"%s\"/", Xml::xmlString(name()).toLatin1().constData());
          
          ///xml.strTag(level, "dstNode", r->name());
          //if(r->channel != -1)
          //  xml.tag(level, "dest type=\"%d\" channel=\"%d\" name=\"%s\"/", r->type, r->channel, r->name().toLatin1().constData());
          //else  
          //  xml.tag(level, "dest type=\"%d\" name=\"%s\"/", r->type, r->name().toLatin1().constData());
          
          // Allow for a regular mono or stereo track to feed a multi-channel synti. 
          // Channel is the 'starting' channel of the destination.
          //if(r->type == Route::TRACK_ROUTE && r->track->isSynti() && r->channel != -1)
          
          //if(r->type == Route::TRACK_ROUTE && r->track->type() == Track::AUDIO_SOFTSYNTH && r->remoteChannel != -1)
          //  xml.tag(level, "dest type=\"%d\" channel=\"%d\" name=\"%s\"/", r->type, r->remoteChannel, r->name().toLatin1().constData());
          //else  
          //if(r->type == Route::MIDI_DEVICE_ROUTE)
          //  xml.tag(level, "dest devtype=\"%d\" name=\"%s\"/", r->device->deviceType(), r->name().toLatin1().constData());
          //else  
          //  xml.tag(level, "dest type=\"%d\" name=\"%s\"/", r->type, r->name().toLatin1().constData());
          
          s = QT_TRANSLATE_NOOP("@default", "dest");
          
          //if(r->type == Route::MIDI_DEVICE_ROUTE)                                      // p3.3.49 Obsolete since 1.1-RC2    
          //  s += QString(QT_TRANSLATE_NOOP("@default", " devtype=\"%1\"")).arg(r->device->deviceType());  //
          //if(r->type != Route::TRACK_ROUTE)                                            //
          if(r->type != Route::TRACK_ROUTE && r->type != Route::MIDI_PORT_ROUTE)
            s += QString(QT_TRANSLATE_NOOP("@default", " type=\"%1\"")).arg(r->type);

          //s += QString(QT_TRANSLATE_NOOP("@default", " name=\"%1\"/")).arg(r->name());
          if(r->type == Route::MIDI_PORT_ROUTE)                                          // p3.3.49 
            s += QString(QT_TRANSLATE_NOOP("@default", " mport=\"%1\"/")).arg(r->midiPort);
          else  
            s += QString(QT_TRANSLATE_NOOP("@default", " name=\"%1\"/")).arg(Xml::xmlString(r->name()));
            
          xml.tag(level, s.toAscii().constData());
          
          xml.etag(level--, "Route");
        }
      }
}
       
//---------------------------------------------------------
//   MidiTrack::write
//---------------------------------------------------------

void MidiTrack::write(int level, Xml& xml) const
      {
      const char* tag;

      if (type() == DRUM)
            tag = "drumtrack";
      else
            tag = "miditrack";
      xml.tag(level++, tag);
      Track::writeProperties(level, xml);

      xml.intTag(level, "device", outPort());
      xml.intTag(level, "channel", outChannel());
      //xml.intTag(level, "inportMap", inPortMask());
      ///xml.uintTag(level, "inportMap", inPortMask());       // Obsolete
      ///xml.intTag(level, "inchannelMap", inChannelMask());  // Obsolete
      xml.intTag(level, "locked", _locked);
      xml.intTag(level, "echo", _recEcho);

      xml.intTag(level, "transposition", transposition);
      xml.intTag(level, "velocity", velocity);
      xml.intTag(level, "delay", delay);
      xml.intTag(level, "len", len);
      xml.intTag(level, "compression", compression);
      xml.intTag(level, "automation", int(automationType()));

      const PartList* pl = cparts();
      for (ciPart p = pl->begin(); p != pl->end(); ++p)
            p->second->write(level, xml);
      xml.etag(level, tag);
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
                        return;
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
                              //Part* p = newPart();
                              //p->read(xml);
                              Part* p = 0;
                              p = readXmlPart(xml, this);
                              if(p)
                                parts()->add(p);
                              }
                        else if (tag == "device")
                              setOutPort(xml.parseInt());
                        else if (tag == "channel")
                              setOutChannel(xml.parseInt());
                        else if (tag == "inportMap")
                              //setInPortMask(xml.parseInt());
                              ///setInPortMask(xml.parseUInt());
                              //xml.skip(tag);                      // Obsolete. 
                              portmask = xml.parseUInt();           // p3.3.48: Support old files.
                        else if (tag == "inchannelMap")
                              ///setInChannelMask(xml.parseInt());
                              //xml.skip(tag);                      // Obsolete.
                              chanmask = xml.parseInt();            // p3.3.48: Support old files.
                        else if (tag == "locked")
                              _locked = xml.parseInt();
                        else if (tag == "echo")
                              _recEcho = xml.parseInt();
                        else if (tag == "automation")
                              setAutomationType(AutomationType(xml.parseInt()));
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
                        if (tag == "miditrack" || tag == "drumtrack") 
                        {
                          setInPortAndChannelMask(portmask, chanmask); // p3.3.48: Support old files.
                          return;
                        }
                  default:
                        break;
                  }
            }
      }

