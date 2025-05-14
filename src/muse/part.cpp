//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: part.cpp,v 1.12.2.17 2009/06/25 05:13:02 terminator356 Exp $
//
//  (C) Copyright 1999/2000 Werner Schweer (ws@seh.de)
//  Additions, modifications (C) Copyright 2011 Tim E. Real (terminator356 on users DOT sourceforge DOT net)
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

#include <assert.h>
#include <stdio.h>
#include "muse_math.h"
#include "muse_time.h"

#include "part.h"
#include "song.h"
#include "globals.h"
#include "audio.h"
#include "wave.h"
#include "midiport.h"
#include "drummap.h"
//#include "midictrl.h"
//#include "gconfig.h"
#include "config.h"

// Forwards from header:
#include "track.h"
#include "xml.h"

// For debugging loading, saving, copy, paste, clone: Uncomment the fprintf section.
#define DEBUG_PART_LOADSAVE(dev, format, args...) // fprintf(dev, format, ##args);

namespace MusECore {

//---------------------------------------------------------
//   MidiCtrlViewState::write
//---------------------------------------------------------

void MidiCtrlViewState::write(int level, Xml& xml) const
      {
        xml.nput(level, "<ctrlViewState num=\"%d\"", _num);
        if(_perNoteVel)
          xml.nput(" perNoteVel=\"1\"");

        xml.put(" />");
      }

//---------------------------------------------------------
//   MidiPartViewState::read
//---------------------------------------------------------

void MidiCtrlViewState::read(Xml& xml)
      {
      for (;;) {
            Xml::Token token = xml.parse();
            const QString& tag = xml.s1();
            switch (token) {
                  case Xml::Error:
                  case Xml::End:
                        return;
                     case Xml::Attribut:
                        if (tag == "num")
                          _num = xml.s2().toInt();
                        else if (tag == "perNoteVel")
                          _perNoteVel = xml.s2().toInt();
                        break;
               case Xml::TagEnd:
                        if (xml.s1() == "ctrlViewState")
                              return;
                  default:
                        break;
                  }
            }
      }

//---------------------------------------------------------
//   MidiPartViewState::write
//---------------------------------------------------------

void MidiPartViewState::write(int level, Xml& xml) const
      {
      // Don't bother if it's an invalid state.
      if(!isValid())
        return;

      xml.tag(level++, "viewState xscroll=\"%d\" yscroll=\"%d\" xscale=\"%d\" yscale=\"%d\"",
              xscroll(), yscroll(), xscale(), yscale());
      
      if(!_controllers.empty()) {
        for (ciMidiCtrlViewState i = _controllers.cbegin();
            i != _controllers.cend(); ++i) {
              (*i).write(level, xml);
              }
      }

      xml.etag(--level, "viewState");
      
      }

//---------------------------------------------------------
//   MidiPartViewState::read
//---------------------------------------------------------

void MidiPartViewState::read(Xml& xml)
      {
      // Make sure to clear the controllers list first.
      clearControllers();
      for (;;) {
            Xml::Token token = xml.parse();
            const QString& tag = xml.s1();
            switch (token) {
                  case Xml::Error:
                  case Xml::End:
                        return;
                  case MusECore::Xml::TagStart:
                        if (tag == "ctrlViewState") {
                              MidiCtrlViewState mcvs;
                              mcvs.read(xml);
                              addController(mcvs);
                        }
                        else
                              xml.unknown("MidiPartViewState");
                        break;
                  case Xml::Attribut:
                        if (tag == "xscroll")
                              setXScroll(xml.s2().toInt());
                        else if (tag == "yscroll")
                              setYScroll(xml.s2().toInt());
                        else if (tag == "xscale")
                              setXScale(xml.s2().toInt());
                        else if (tag == "yscale")
                              setYScale(xml.s2().toInt());
                        break;
                  case Xml::TagEnd:
                        if (xml.s1() == "viewState")
                              return;
                  default:
                        break;
                  }
            }
      }



void Part::unchainClone()
{
  chainCheckErr(this); // FIXME proper assert!
  
  if (_backupClone) printf("THIS SHOULD NEVER HAPPEN: Part::unchainClone() called, but _backupClone was non-NULL\n");
  
  _backupClone=_prevClone;
  
  // Unchain the part.
  _prevClone->_nextClone = _nextClone;
  _nextClone->_prevClone = _prevClone;
  
  // Isolate the part.
  _prevClone = this;
  _nextClone = this;

  DEBUG_PART_LOADSAVE(stderr, "Part::unchainClone:%p %s muuid cur:%s new:%s\n",
    this, name().toLocal8Bit().constData(),
    _clonemaster_uuid.toString().toLocal8Bit().constData(), _uuid.toString().toLocal8Bit().constData());

  _clonemaster_uuid = this->_uuid;
}

void Part::chainClone(Part* p)
{
  // FIXME assertion
  assert(p);
  
  if (! (_prevClone==this && _nextClone==this)) // the part is still part of a clone chain!
  {
    printf("ERROR: THIS SHOULD NEVER HAPPEN: Part::chainClone() called, but part is already chained! I'll unchain for now, but better fix that!\n");
    this->unchainClone();
  }

  // Make our links to the chain
  this->_prevClone = p;
  this->_nextClone = p->_nextClone;
  
  // Make the chain's links to us
  this->_nextClone->_prevClone = this;
  p->_nextClone = this;
  
  // we only chain clones. we must trust in the GUI thread that the eventlist is consistent.
  
  DEBUG_PART_LOADSAVE(stderr, "Part::chainClone:%p %s part:%p %s muuid cur:%s new:%s\n",
    this, name().toLocal8Bit().constData(),
    p, p->name().toLocal8Bit().constData(),
    _clonemaster_uuid.toString().toLocal8Bit().constData(), p->_uuid.toString().toLocal8Bit().constData());

  this->_clonemaster_uuid = p->clonemaster_uuid();
}

void Part::rechainClone()
{
    if(_backupClone)
    {
      DEBUG_PART_LOADSAVE(stderr, "Part::rechainClone:%p %s muuid:%s _backupClone:%p %s muuid:%s calling chainClone...\n",
        this, name().toLocal8Bit().constData(),
        _clonemaster_uuid.toString().toLocal8Bit().constData(),
        _backupClone, _backupClone->name().toLocal8Bit().constData(),
        _backupClone->clonemaster_uuid().toString().toLocal8Bit().constData());

        this->chainClone(_backupClone);
        _backupClone = nullptr;
    }
}

bool Part::isCloneOf(const Part* other) const
{
	return this->_clonemaster_uuid == other->_clonemaster_uuid;
}

int Part::nClones() const
{
	int n=1;
	
	for(const Part* it = this->_nextClone; it!=this; it=it->_nextClone)
		n++;
	
	return n;
}


// FIXME FINDMICHJETZT TODO: weg damit!

//---------------------------------------------------------
//   unchainTrackParts
//---------------------------------------------------------

void unchainTrackParts(Track* t)
{
  PartList* pl = t->parts();
  for(iPart ip = pl->begin(); ip != pl->end(); ++ip)
    ip->second->unchainClone();
}

//---------------------------------------------------------
//   chainTrackParts
//---------------------------------------------------------

void chainTrackParts(Track* t)
{
  PartList* pl = t->parts();
  for(riPart ip = pl->rbegin(); ip != pl->rend(); ++ip) // walk through in opposite direction than we unchained them.
    ip->second->rechainClone();
}

//---------------------------------------------------------
//   chainCheckErr
//---------------------------------------------------------

void chainCheckErr(Part* p)
{
  // At all times these must be false...
  if(p->nextClone()->prevClone() != p)
    printf("chainCheckErr: Next clone:%s %p prev clone:%s %p != %s %p\n", p->nextClone()->name().toLocal8Bit().constData(), p->nextClone(), p->nextClone()->prevClone()->name().toLocal8Bit().constData(), p->nextClone()->prevClone(), p->name().toLocal8Bit().constData(), p);
  if(p->prevClone()->nextClone() != p)
    printf("chainCheckErr: Prev clone:%s %p next clone:%s %p != %s %p\n", p->prevClone()->name().toLocal8Bit().constData(), p->prevClone(), p->prevClone()->nextClone()->name().toLocal8Bit().constData(), p->prevClone()->nextClone(), p->name().toLocal8Bit().constData(), p);
}

//---------------------------------------------------------
//   addPortCtrlEvents
//---------------------------------------------------------

void addPortCtrlEvents(Part* part, bool doClones)
{
  // Traverse and process the clone chain ring until we arrive at the same part again.
  // The loop is a safety net.
  Part* p = part; 
  while(1)
  {
    Track* t = p->track();
    if(t && t->isMidiTrack())
    {
      MidiTrack* mt = (MidiTrack*)t;
      unsigned int len = p->lenTick();
      for(ciEvent ie = p->events().begin(); ie != p->events().end(); ++ie)
      {
        const Event& ev = ie->second;
        // Do not add events which are past the end of the part.
#ifdef ALLOW_LEFT_HIDDEN_EVENTS
        if((int)ev.tick() >= (int)len)
#else
        if(ev.tick() >= len)
#endif
          break;
#ifdef ALLOW_LEFT_HIDDEN_EVENTS
        if((int)ev.tick() < 0)
          continue;
#endif
                          
        if(ev.type() == Controller)
        {
          unsigned int tck  = ev.tick() + p->tick();
          int cntrl = ev.dataA();
          int val   = ev.dataB();

          MidiPort* mp;
          int ch;
          mt->mappedPortChanCtrl(&cntrl, nullptr, &mp, &ch);

          mp->setControllerVal(ch, tck, cntrl, val, p);
        }
      }
    }
    if(!doClones)
      break;
    // Get the next clone in the chain ring.
    p = p->nextClone();
    // Same as original part? Finished.
    if(p == part)
      break;
  }
}

//---------------------------------------------------------
//   removePortCtrlEvents
//---------------------------------------------------------

void removePortCtrlEvents(Part* part, bool doClones)
{
  // Traverse and process the clone chain ring until we arrive at the same part again.
  // The loop is a safety net.
  Part* p = part; 
  while(1)
  {
    Track* t = p->track();
    if(t && t->isMidiTrack())
    {
      MidiTrack* mt = (MidiTrack*)t;
//       MidiPort* mp = &MusEGlobal::midiPorts[mt->outPort()];
//       int ch = mt->outChannel();
      for(ciEvent ie = p->events().begin(); ie != p->events().end(); ++ie)
      {
        const Event& ev = ie->second;
                          
        if(ev.type() == Controller)
        {
          unsigned int tck  = ev.tick() + p->tick();
          int cntrl = ev.dataA();
          int val   = ev.dataB();
          
          // Is it a drum controller event, according to the track port's instrument?
          MidiPort* mp;
          int ch;
          mt->mappedPortChanCtrl(&cntrl, nullptr, &mp, &ch);

          mp->deleteController(ch, tck, cntrl, val, p);
        }
      }
    }  
    
    if(!doClones)
      break;
    // Get the next clone in the chain ring.
    p = p->nextClone();
    // Same as original part? Finished.
    if(p == part)
      break;
  }
}

//---------------------------------------------------------
//   addEvent
//---------------------------------------------------------

iEvent Part::addEvent(Event& p)
      {
      return _events.add(p);
      }

//---------------------------------------------------------
//   index
//---------------------------------------------------------

int PartList::index(const Part* part) const
      {
      int index = 0;
      for (ciPart i = begin(); i != end(); ++i, ++index)
            if (i->second == part) {
                  return index;
                  }
      if(MusEGlobal::debugMsg)
        printf("PartList::index(): not found!\n");
      return -1;  // don't change that value. at least MidiEditor::addNewParts relies on this
      }

//---------------------------------------------------------
//   find
//---------------------------------------------------------

Part* PartList::find(int idx)
      {
      int index = 0;
      for (iPart i = begin(); i != end(); ++i, ++index)
            if (index == idx)
                  return i->second;
      return nullptr;
      }

Part* PartList::findCloneMaster(const QUuid& cloneUuid) const
{
  for(const_iterator ip = cbegin(); ip != cend(); ++ip)
  {
    if(ip->second->clonemaster_uuid() == cloneUuid)
      return ip->second;
  }
  return nullptr;
}

//---------------------------------------------------------
//   Part
//---------------------------------------------------------

Part::Part(Track* t)
      {
      _hiddenEvents = NoEventsHidden;
      _prevClone = this;
      _nextClone = this;
      _backupClone = nullptr;
      _track      = t;
      _selected   = false;
      _mute       = false;
      _colorIndex = 0;
      _uuid = newUuid();
      _clonemaster_uuid = _uuid;
      }

Part::~Part()
{
      if (_prevClone!=this || _nextClone!=this)
      {
        if (MusEGlobal::debugMsg) {
            fprintf(stderr, "Part isn't unchained in ~Part()! Unchaining now...\n");
        }
        unchainClone();
      }  
}

QUuid Part::uuid() const { return _uuid; }
void Part::setUuid(const QUuid& uuid) { _uuid = uuid; }
QUuid Part::newUuid() const { return QUuid::createUuid(); }
QUuid Part::clonemaster_uuid() const { return _clonemaster_uuid; }
void Part::setClonemasterUuid(const QUuid& uuid) { _clonemaster_uuid = uuid; }


WavePart* WavePart::duplicateEmpty() const
{
	WavePart* part = new WavePart((WaveTrack*)this->_track);
	part->setName(name());
	part->setColorIndex(colorIndex());

	*(PosLen*)part = *(PosLen*)this;
	part->setMute(mute());
	
	return part;
}

WavePart* WavePart::duplicate() const
{
	return (WavePart*)Part::duplicate();
}

WavePart* WavePart::createNewClone() const
{
	return (WavePart*)Part::createNewClone();
}

MidiPart* MidiPart::duplicateEmpty() const
{
	MidiPart* part = new MidiPart((MidiTrack*)this->_track);
	part->setName(name());
	part->setColorIndex(colorIndex());

	*(PosLen*)part = *(PosLen*)this;
	part->setMute(mute());
	
	return part;
}

MidiPart* MidiPart::duplicate() const
{
	return (MidiPart*)Part::duplicate();
}

MidiPart* MidiPart::createNewClone() const
{
	return (MidiPart*)Part::createNewClone();
}


Part* Part::createNewClone() const
{
        Part* clone = duplicateEmpty();
        for (MusECore::ciEvent i = _events.begin(); i != _events.end(); ++i)
        {
	  Event nev = i->second.clone(); // Create a non-shared clone of the event, having the same id.
          clone->addEvent(nev); 
        }
	clone->_backupClone=const_cast<Part*>(this);
	return clone;
}

Part* Part::duplicate() const
{
	Part* dup = duplicateEmpty();

	// copy the eventlist; duplicate each Event(Ptr!).
	for (MusECore::ciEvent i = _events.begin(); i != _events.end(); ++i)
	{
		Event nev = i->second.duplicate(); // Create a duplicate of the event, excluding the _id.
		
		dup->addEvent(nev);
	}

	return dup;
}

bool Part::selectEvents(bool select, unsigned long /*t0*/, unsigned long /*t1*/)
{
  bool ret = false;
  EventList& el = nonconst_events();
  for(iEvent ie = el.begin(); ie != el.end(); ++ie)
  {
    Event& e = ie->second;
//     if(e.type() ???) // For t0 and t1
    if(e.selected() != select)
      ret = true;
    e.setSelected(select);
  }
  return ret;
}

//---------------------------------------------------------
//   WavePart
//---------------------------------------------------------

WavePart::WavePart(WaveTrack* t)
   : Part(t)
      {
      setType(FRAMES);
      }

//---------------------------------------------------------
//   findPart
//---------------------------------------------------------

iPart PartList::findPart(unsigned tick)
      {
      iPart i;
      for (i = begin(); i != end(); ++i)
            if (i->second->tick() == tick)
                  break;
      return i;
      }

//---------------------------------------------------------
//   add
//---------------------------------------------------------

iPart PartList::add(Part* part)
      {
      // Added by T356. A part list containing wave parts should be sorted by
      //  frames. WaveTrack::fetchData() relies on the sorting order, and
      //  there was a bug that waveparts were sometimes muted because of
      //  incorrect sorting order (by ticks).
      // Also, when the tempo map is changed, every wavepart would have to be
      //  re-added to the part list so that the proper sorting order (by ticks)
      //  could be achieved.
      // Note that in a med file, the tempo list is loaded AFTER all the tracks.
      // There was a bug that all the wave parts' tick values were not correct,
      // since they were computed BEFORE the tempo map was loaded.
      if(part->type() == Pos::FRAMES)
        return insert(PartListInsertPair_t(part->frame(), part));
      else
        return insert(PartListInsertPair_t(part->tick(), part));
      }

//---------------------------------------------------------
//   remove
//---------------------------------------------------------

void PartList::remove(Part* part)
      {
      iPart i;
      for (i = begin(); i != end(); ++i) {
            if (i->second == part) {
                  erase(i);
                  break;
                  }
            }
      if (i == end())
        printf("THIS SHOULD NEVER HAPPEN: could not find the part in PartList::remove()!\n");
      }

//---------------------------------------------------------
//   addPart
//---------------------------------------------------------

void Song::addPart(Part* part)
      {
      // adjust song len:
      unsigned int epos = part->tick() + part->lenTick();

#ifdef ALLOW_LEFT_HIDDEN_EVENTS
      if ((int)epos > (int)len())
#else
      if (epos > len())
#endif
        _songLenTicks = epos;

      part->track()->addPart(part);
      
      // Indicate do not do clones.
      addPortCtrlEvents(part, false);
      }

//---------------------------------------------------------
//   removePart
//---------------------------------------------------------

void Song::removePart(Part* part)
      {
      // Indicate do not do clones.
      removePortCtrlEvents(part, false);
      Track* track = part->track();
      track->parts()->remove(part);
      }

//---------------------------------------------------------
//   splitPart
//    split part "part" at "tick" position
//    create two new parts p1 and p2
//---------------------------------------------------------

void Part::splitPart(unsigned int tickpos, Part*& p1, Part*& p2) const
      {
#ifdef ALLOW_LEFT_HIDDEN_EVENTS
      MuseCount_t l1 = 0;       // len of first new part (ticks or samples)
      MuseCount_t l2 = 0;       // len of second new part

      const MuseCount_t partPosVal = MUSE_TIME_UINT_TO_INT64 posValue();
      const MuseCount_t partEndVal = MUSE_TIME_UINT_TO_INT64 endValue();
      const MuseCount_t partLenVal = MUSE_TIME_UINT_TO_INT64 lenValue();
      const MuseCount_t posPType = MUSE_TIME_UINT_TO_INT64 Pos::convert(MUSE_TIME_UINT_TO_INT64 tickpos, Pos::TICKS, type());

      if(posPType <= partPosVal || posPType >= partEndVal)
        return;
      l1 = posPType - partPosVal;
      l2 = partLenVal - l1;

      p1 = this->duplicateEmpty();   // new left part
      p2 = this->duplicateEmpty();   // new right part

      p1->setLenValue(l1);
      p2->setPosValue(posPType);
      p2->setLenValue(l2);

      for (ciEvent ie = _events.cbegin(); ie != _events.cend(); ++ie) {
            const Event event = ie->second;
            const Pos::TType eType = event.pos().type();
            const MuseCount_t partPosEType = MUSE_TIME_UINT_TO_INT64 Pos::convert(MUSE_TIME_UINT_TO_INT64 posValue(), type(), eType);
            const MuseCount_t ePosVal = MUSE_TIME_UINT_TO_INT64 event.posValue();
            const MuseCount_t absEPosVal = ePosVal + partPosEType;
            const MuseCount_t absEEndVal = absEPosVal + MUSE_TIME_UINT_TO_INT64 event.lenValue();
            switch(event.type())
            {
              case EventType::Wave:
              {
                  const MuseCount_t p1PosEType = MUSE_TIME_UINT_TO_INT64 p1->posValue(eType);
                  const MuseCount_t p1EndEType = MUSE_TIME_UINT_TO_INT64 p1->endValue(eType);
                  const MuseCount_t p2PosEType = MUSE_TIME_UINT_TO_INT64 p2->posValue(eType);
                  const MuseCount_t p2EndEType = MUSE_TIME_UINT_TO_INT64 p2->endValue(eType);

                  // Now that MusE has left part border resizing, let's use that instead of Event::mid().
                  if ((absEEndVal > p1PosEType) && (absEPosVal < p1EndEType)) {
                        Event new_event = event.duplicate();
                        // Honour the global auto expand part waves setting.
                        // Hm, actually do it all the time.
                        if(/*MusEGlobal::config.autoExpandPartWaves &&*/ absEEndVal > p1EndEType)
                          new_event.setLenValue(p1EndEType - absEPosVal);
                        p1->addEvent(new_event);
                        }
                  if ((absEEndVal > p2PosEType) && (absEPosVal < p2EndEType)) {
                        Event new_event = event.duplicate();
                        // Honour the global auto expand part waves setting.
                        // Hm, actually do it all the time.
                        //if(MusEGlobal::config.autoExpandPartWaves)
                        {
                          MuseCount_t newAbsEPos = absEPosVal;
                          if(newAbsEPos < p2PosEType)
                          {
                            new_event.setLenValue(new_event.lenValue() - (p2PosEType - newAbsEPos));
                            const MuseCount_t newAbsEPosFrames = MUSE_TIME_UINT_TO_INT64 Pos::convert(newAbsEPos, eType, Pos::FRAMES);
                            const MuseCount_t p2PosFrames = MUSE_TIME_UINT_TO_INT64 p2->posValue(Pos::FRAMES);
                            // We must trim any 'stretch' list items.
                            new_event.sndFile().stretchList()->trimLeft(
                              StretchListItem::StretchEvent | StretchListItem::SamplerateEvent | StretchListItem::PitchEvent,
                              new_event.sndFile().convertPosition(p2PosFrames - newAbsEPosFrames));
                            // The new SPos, taking into account any stretching or resampling the wave may have.
                            const int newSPos = new_event.sndFile().convertPosition(new_event.spos() + (p2PosFrames - newAbsEPosFrames));
                            new_event.setSpos(newSPos);
                            newAbsEPos = p2PosEType;
                          }
                          new_event.setPosValue(newAbsEPos - p2PosEType);
                        }
                        //else
                        //  new_event.setPosValue(MUSE_TIME_UINT_TO_INT64 new_event.posValue() - (p2PosEType - p1PosEType));
                        p2->addEvent(new_event);
                        }
              }
              break;

              case EventType::Note:
              case EventType::Controller:
              case EventType::Sysex:
              case EventType::Meta:
              {
                const MuseCount_t posEType = MUSE_TIME_UINT_TO_INT64 Pos::convert(tickpos, Pos::TICKS, eType);
                Event new_event = event.duplicate();
                if (absEPosVal >= posEType) {
                      new_event.setPosValue(absEPosVal - posEType);
                      p2->addEvent(new_event);
                      }
                else
                      p1->addEvent(new_event);
              }
              break;
            }
            }

#else

      unsigned int l1 = 0;       // len of first new part (ticks or samples)
      unsigned int l2 = 0;       // len of second new part

      unsigned int samplepos = MusEGlobal::tempomap.tick2frame(tickpos);

      switch (track()->type()) {
          case Track::WAVE:
                  if(samplepos <= frame() || lenFrame() <= l1)
                    return;
                  l1 = samplepos - frame();
                  l2 = lenFrame() - l1;
                  break;
          case Track::MIDI:
          case Track::DRUM:
                  if(tickpos <= tick() || lenTick() <= l1)
                    return;
                  l1 = tickpos - tick();
                  l2 = lenTick() - l1;
                  break;
            default:
                  return;
            }

      p1 = this->duplicateEmpty();   // new left part
      p2 = this->duplicateEmpty();   // new right part

      switch (track()->type()) {
          case Track::WAVE:
                  p1->setLenFrame(l1);
                  p2->setFrame(samplepos);
                  p2->setLenFrame(l2);
                  break;
          case Track::MIDI:
          case Track::DRUM:
                  p1->setLenTick(l1);
                  p2->setTick(tickpos);
                  p2->setLenTick(l2);
                  break;
            default:
                  break;
            }

      if (track()->type() == Track::WAVE) {
            unsigned int ps   = this->frame();
            unsigned int d1p1 = p1->frame();
            unsigned int d2p1 = p1->endFrame();
            unsigned int d1p2 = p2->frame();
            unsigned int d2p2 = p2->endFrame();
            for (ciEvent ie = _events.begin(); ie != _events.end(); ++ie) {
                  const Event& event = ie->second;
                  unsigned int s1 = event.frame() + ps;
                  unsigned int s2 = event.endFrame() + ps;

                 if ((s2 > d1p1) && (s1 < d2p1)) {
                        Event si = event.mid(d1p1 - ps, d2p1);
                        p1->addEvent(si);
                        }
                  if ((s2 > d1p2) && (s1 < d2p2)) {
                        Event si = event.mid(d1p2 - ps, d2p2);
                        p2->addEvent(si);
                        }
                  }
            }
      else {
            for (ciEvent ie = _events.begin(); ie != _events.end(); ++ie) {
                  Event event = ie->second.clone();
                  unsigned int t = event.tick();
                  if (t >= l1) {
                        event.move(-l1);
                        p2->addEvent(event);
                        }
                  else
                        p1->addEvent(event);
                  }
            }
#endif
      }

//---------------------------------------------------------
//   changePart
//---------------------------------------------------------

void Song::changePart(Part* oPart, Part* nPart)
      {
      nPart->setUuid(oPart->uuid());

      Track* oTrack = oPart->track();
      Track* nTrack = nPart->track();

      oTrack->parts()->remove(oPart);
      nTrack->parts()->add(nPart);

      // Added by T356.
      // adjust song len:
      unsigned int epos = nPart->tick() + nPart->lenTick();
      if (epos > len())
            _songLenTicks = epos;
      }

//---------------------------------------------------------
//   setViewState
//---------------------------------------------------------

void Part::setViewState(const MidiPartViewState& vs)
{
  _viewState = vs;
}

//---------------------------------------------------------
//   dump
//---------------------------------------------------------

void Part::dump(int n) const
      {
      for (int i = 0; i < n; ++i)
            putchar(' ');
      printf("Part: <%s> ", _name.toLocal8Bit().constData());
      for (int i = 0; i < n; ++i)
            putchar(' ');
      PosLen::dump();
      }


void WavePart::dump(int n) const
      {
      Part::dump(n);
      for (int i = 0; i < n; ++i)
            putchar(' ');
      printf("WavePart\n");
      }


void MidiPart::dump(int n) const
      {
      Part::dump(n);
      for (int i = 0; i < n; ++i)
            putchar(' ');
      printf("MidiPart\n");
      }

//---------------------------------------------------------
//   hasHiddenEvents
//   Returns combination of HiddenEventsType enum.
//---------------------------------------------------------

int MidiPart::hasHiddenEvents() const
{
  // TODO An idea for left-border hidden events with unsigned integer positions. 
  // It actually worked, as far as the left hidden events arrow, but really it can't work without much bigger switchovers to integer math.
  // I guess we'll try it like this.
#ifdef ALLOW_LEFT_HIDDEN_EVENTS
  unsigned int len = lenTick();
  _hiddenEvents = NoEventsHidden;  // Cache the result for later.

  const int c_comp = LeftEventsHidden | RightEventsHidden;
  for(ciEvent ev=_events.begin(); ev!=_events.end(); ++ev)
  {
    // Is the time value less than zero? Note the conversion to signed here!
    if(signed(ev->second.tick()) < 0)
      _hiddenEvents |= LeftEventsHidden;  // Cache the result for later.
    if(signed(ev->second.endTick()) > signed(len))
      _hiddenEvents |= RightEventsHidden;  // Cache the result for later.
    if(_hiddenEvents == c_comp)
      break;
  }
  return _hiddenEvents;
#else
  unsigned int len = lenTick();

  // TODO: For now, we don't support events before the left border, only events past the right border.
  for(ciEvent ev=_events.begin(); ev!=_events.end(); ev++)
  {
    if(ev->second.endTick() > len)
    {
      _hiddenEvents = RightEventsHidden;  // Cache the result for later.
      return _hiddenEvents;
    }  
  }
  _hiddenEvents = NoEventsHidden;  // Cache the result for later.
  return _hiddenEvents;
#endif
}

//---------------------------------------------------------
//   hasHiddenEvents
//   Returns combination of HiddenEventsType enum.
//---------------------------------------------------------

int WavePart::hasHiddenEvents() const
{
  // An idea for left-border hidden events with unsigned integer positions. 
  // It actually worked, as far as the left hidden events arrow, but really it can't work without much bigger switchovers to integer math.
  // I guess we'll try it like this.
#ifdef ALLOW_LEFT_HIDDEN_EVENTS
  unsigned int len = lenFrame();
  _hiddenEvents = NoEventsHidden;  // Cache the result for later.

  const int c_comp = LeftEventsHidden | RightEventsHidden;
  for(ciEvent ev=_events.begin(); ev!=_events.end(); ++ev)
  {
    // Is the time value less than zero? Note the conversion to signed here!
    if(signed(ev->second.frame()) < 0)
      _hiddenEvents |= LeftEventsHidden;  // Cache the result for later.
    if(signed(ev->second.endFrame()) > signed(len))
      _hiddenEvents |= RightEventsHidden;  // Cache the result for later.
    if(_hiddenEvents == c_comp)
      break;
  }
  return _hiddenEvents;
#else
  unsigned int len = lenFrame();

  // TODO: For now, we don't support events before the left border, only events past the right border.
  for(ciEvent ev=_events.begin(); ev!=_events.end(); ev++)
  {
    if(ev->second.endFrame() > len)
    {
      _hiddenEvents = RightEventsHidden;  // Cache the result for later.
      return _hiddenEvents;
    }  
  }
  _hiddenEvents = NoEventsHidden;  // Cache the result for later.
  return _hiddenEvents;
#endif
}

bool WavePart::openAllEvents()
{
  bool opened = false;
  const EventList& el = events();
  for(ciEvent ie = el.begin(); ie != el.end(); ++ie)
  {
    const Event& e = ie->second;
    if(e.empty())
      continue;
    SndFileR f = e.sndFile();
    if(!f.isNull() && !f.isOpen())
    {
      opened = true;
      f.openRead();
    }
  }
  return opened;
}
      
bool WavePart::closeAllEvents()
{
  bool closed = false;
  const EventList& el = events();
  for(ciEvent ie = el.begin(); ie != el.end(); ++ie)
  {
    const Event& e = ie->second;
    if(e.empty())
      continue;
    SndFileR f = e.sndFile();
    if(!f.isNull() && f.isOpen())
    {
      closed = true;
      f.close();
    }
  }
  return closed;
}

} // namespace MusECore
