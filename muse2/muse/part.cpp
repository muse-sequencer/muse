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
#include <cmath>

#include "song.h"
#include "part.h"
#include "track.h"
#include "globals.h"
#include "event.h"
#include "audio.h"
#include "wave.h"
#include "midiport.h"
#include "drummap.h"

namespace MusECore {

int Part::snGen=0;


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
  
  _clonemaster_sn = this->_sn;
}

void Part::chainClone(Part* p)
{
  // FIXME assertion
  
  this->unchainClone();

  // Make our links to the chain
  this->_prevClone = p;
  this->_nextClone = p->_nextClone;
  
  // Make the chain's links to us
  this->_nextClone->_prevClone = this;
  p->_nextClone = this;
  
  // we only chain clones. we must trust in the GUI thread that the eventlist is consistent.
  
  this->_clonemaster_sn = p->_sn;
}

void Part::rechainClone()
{
    assert(_backupClone);
    
    this->chainClone(_backupClone);
    _backupClone = NULL;
}

bool Part::isCloneOf(const Part* other) const
{
	return this->_clonemaster_sn == other->_clonemaster_sn;
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
  // At all times these must be true...
  if(p->nextClone()->prevClone() != p)
    printf("chainCheckErr: Next clone:%s %p prev clone:%s %p != %s %p\n", p->nextClone()->name().toLatin1().constData(), p->nextClone(), p->nextClone()->prevClone()->name().toLatin1().constData(), p->nextClone()->prevClone(), p->name().toLatin1().constData(), p); 
  if(p->prevClone()->nextClone() != p)
    printf("chainCheckErr: Prev clone:%s %p next clone:%s %p != %s %p\n", p->prevClone()->name().toLatin1().constData(), p->prevClone(), p->prevClone()->nextClone()->name().toLatin1().constData(), p->prevClone()->nextClone(), p->name().toLatin1().constData(), p); 
}

//---------------------------------------------------------
//   addPortCtrlEvents
//---------------------------------------------------------

void addPortCtrlEvents(Event& event, Part* part, bool doClones)
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
      MidiPort* mp = &MusEGlobal::midiPorts[mt->outPort()];
      int ch = mt->outChannel();
      unsigned len = p->lenTick();
        
      // Do not add events which are past the end of the part.
      if(event.tick() >= len)
        break;

      if(event.type() == Controller)
      {
        int tck  = event.tick() + p->tick();
        int cntrl = event.dataA();
        int val   = event.dataB();

        // Is it a drum controller event, according to the track port's instrument?
        if(mt->type() == Track::DRUM)
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

        mp->setControllerVal(ch, tck, cntrl, val, p);
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
      MidiPort* mp = &MusEGlobal::midiPorts[mt->outPort()];
      int ch = mt->outChannel();
      unsigned len = p->lenTick();
      for(ciEvent ie = p->events().begin(); ie != p->events().end(); ++ie)
      {
        const Event& ev = ie->second;
        // Added by T356. Do not add events which are past the end of the part.
        if(ev.tick() >= len)
          break;
                          
        if(ev.type() == Controller)
        {
          int tck  = ev.tick() + p->tick();
          int cntrl = ev.dataA();
          int val   = ev.dataB();
          
          // Is it a drum controller event, according to the track port's instrument?
          if(mt->type() == Track::DRUM)
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

void removePortCtrlEvents(Event& event, Part* part, bool doClones)
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
      MidiPort* mp = &MusEGlobal::midiPorts[mt->outPort()];
      int ch = mt->outChannel();
                          
      if(event.type() == Controller)
      {
        int tck  = event.tick() + p->tick();
        int cntrl = event.dataA();

        // Is it a drum controller event, according to the track port's instrument?
        if(mt->type() == Track::DRUM)
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

        mp->deleteController(ch, tck, cntrl, p);
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
      MidiPort* mp = &MusEGlobal::midiPorts[mt->outPort()];
      int ch = mt->outChannel();
      for(ciEvent ie = p->events().begin(); ie != p->events().end(); ++ie)
      {
        const Event& ev = ie->second;
                          
        if(ev.type() == Controller)
        {
          int tck  = ev.tick() + p->tick();
          int cntrl = ev.dataA();
          
          // Is it a drum controller event, according to the track port's instrument?
          if(mt->type() == Track::DRUM)
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
          
          mp->deleteController(ch, tck, cntrl, p);
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
      assert(!hasClones());
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
      return 0;
      }

Part::Part(Track* t)
      {
      _hiddenEvents = NoEventsHidden;
      _prevClone = this;
      _nextClone = this;
      _backupClone = NULL;
      _sn = newSn();
      _clonemaster_sn = _sn;
      _track      = t;
      _selected   = false;
      _mute       = false;
      _colorIndex = 0;
      }

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
	Part* clone = duplicate();
	clone->_backupClone=const_cast<Part*>(this);
	return clone;
}

Part* Part::duplicate() const
{
	Part* dup = duplicateEmpty();

	// copy the eventlist; duplicate each Event(Ptr!).
	for (MusECore::ciEvent i = _events.begin(); i != _events.end(); ++i)
		dup->_events.add(i->second.clone());

	return dup;
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
//   Part
//---------------------------------------------------------

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
        return insert(std::pair<const int, Part*> (part->frame(), part));
      else
        return insert(std::pair<const int, Part*> (part->tick(), part));
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
      unsigned epos = part->tick() + part->lenTick();

      if (epos > len())
            _len = epos;
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
//   cmdResizePart
//---------------------------------------------------------

void Song::cmdResizePart(Track* track, Part* oPart, unsigned int len, bool doClones)
      {
      switch(track->type()) {
            case Track::WAVE:
                  {
                  // TODO FINDMICH FIXME this is totally broken. we don't want to remove events just because they're beyond end-of-part.
                  // we also don't want to auto-resize the last event.
                  
                  /*
                  MusECore::WavePart* nPart = new MusECore::WavePart(*(MusECore::WavePart*)oPart);
                  EventList* el = nPart->events();
                  unsigned new_partlength = MusEGlobal::tempomap.deltaTick2frame(oPart->tick(), oPart->tick() + len);

                  // If new nr of frames is less than previous what can happen is:
                  // -   0 or more events are beginning after the new final position. Those are removed from the part
                  // -   The last event begins before new final position and ends after it. If so, it will be resized to end at new part length
                  if (new_partlength < oPart->lenFrame()) {
                        Undo operations;

                        for (iEvent i = el->begin(); i != el->end(); i++) {
                              Event e = i->second;
                              unsigned event_startframe = e.frame();
                              unsigned event_endframe = event_startframe + e.lenFrame();
                              if (event_endframe < new_partlength)
                                    continue;
                              }
                        nPart->setLenFrame(new_partlength);
                        // Do not do port controller values and clone parts. 
                        operations.push_back(UndoOp(UndoOp::Modify***Part, oPart, nPart, false, false));

                        MusEGlobal::song->applyOperationGroup(operations);
                        }
                  // If the part is expanded there can be no additional events beginning after the previous final position
                  // since those are removed if the part has been shrunk at some time (see above)
                  // The only thing we need to check is the final event: If it has data after the previous final position,
                  // we'll expand that event
                  else {
                        Undo operations;
                        if(!el->empty())
                        {
                          iEvent i = el->end();
                          i--;
                          Event last = i->second;
                          MusECore::SndFileR file = last.sndFile();
                          if (file.isNull())
                                return;
                          Event newEvent = last.clone();
                          // Do not do port controller values and clone parts. 
                          operations.push_back(UndoOp(UndoOp::ModifyEvent, newEvent, last, nPart, false, false));
                        }  
                        
                        nPart->setLenFrame(new_partlength);
                        // Do not do port controller values and clone parts. 
                        operations.push_back(UndoOp(UndoOp::Modify***Part, oPart, nPart, false, false));
                        MusEGlobal::song->applyOperationGroup(operations);
                        } */
                  }
                  break;
            case Track::MIDI:
            case Track::DRUM:
            case Track::NEW_DRUM:
                  {
                  Undo operations;
									
									unsigned orig_len=oPart->lenTick();
									MidiPart* part_it=(MidiPart*)oPart;
									do
									{
										if (part_it->lenTick()==orig_len)
										{
											// Do port controller values but not clone parts. 
											operations.push_back(UndoOp(UndoOp::ModifyPartLength, part_it, part_it->lenTick(), len, true, false));
										}
										
										part_it=(MidiPart*)part_it->nextClone();
									} while (doClones && (part_it != (MidiPart*)oPart));
                  
                  MusEGlobal::song->applyOperationGroup(operations);
                  break;
                  }
            default:
                  break;
            }
      }

//---------------------------------------------------------
//   splitPart
//    split part "part" at "tick" position
//    create two new parts p1 and p2
//---------------------------------------------------------

void Part::splitPart(int tickpos, Part*& p1, Part*& p2) const
      {
      int l1 = 0;       // len of first new part (ticks or samples)
      int l2 = 0;       // len of second new part

      int samplepos = MusEGlobal::tempomap.tick2frame(tickpos);

      switch (track()->type()) {
          case Track::WAVE:
                  l1 = samplepos - frame();
                  l2 = lenFrame() - l1;
                  break;
          case Track::MIDI:
          case Track::DRUM:
          case Track::NEW_DRUM:
                  l1 = tickpos - tick();
                  l2 = lenTick() - l1;
                  break;
            default:
                  return;
            }

      if (l1 <= 0 || l2 <= 0)
            return;

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
          case Track::NEW_DRUM:
                  p1->setLenTick(l1);
                  p2->setTick(tickpos);
                  p2->setLenTick(l2);
                  break;
            default:
                  break;
            }

      if (track()->type() == Track::WAVE) {
            int ps   = this->frame();
            int d1p1 = p1->frame();
            int d2p1 = p1->endFrame();
            int d1p2 = p2->frame();
            int d2p2 = p2->endFrame();
            for (ciEvent ie = _events.begin(); ie != _events.end(); ++ie) {
                  const Event& event = ie->second;
                  int s1 = event.frame() + ps;
                  int s2 = event.endFrame() + ps;
                  
                  if ((s2 > d1p1) && (s1 < d2p1)) {
                        Event si = event.mid(d1p1 - ps, d2p1 - ps);
                        p1->_events.add(si);
                        }
                  if ((s2 > d1p2) && (s1 < d2p2)) {
                        Event si = event.mid(d1p2 - ps, d2p2 - ps);
                        p2->_events.add(si);
                        }
                  }
            }
      else {
            for (ciEvent ie = _events.begin(); ie != _events.end(); ++ie) {
                  Event event = ie->second.clone();
                  int t = event.tick();
                  if (t >= l1) {
                        event.move(-l1);
                        p2->_events.add(event);
                        }
                  else
                        p1->_events.add(event);
                  }
            }
      }

//---------------------------------------------------------
//   cmdSplitPart
//---------------------------------------------------------

void Song::cmdSplitPart(Track* track, Part* part, int tick)
      {
      int l1 = tick - part->tick();
      int l2 = part->lenTick() - l1;
      if (l1 <= 0 || l2 <= 0)
            return;
      Part* p1;
      Part* p2;
      part->splitPart(tick, p1, p2);
      
      MusEGlobal::song->informAboutNewParts(part, p1);
      MusEGlobal::song->informAboutNewParts(part, p2);

      Undo operations;
      operations.push_back(UndoOp(UndoOp::DeletePart, part));
      operations.push_back(UndoOp(UndoOp::AddPart, p1));
      operations.push_back(UndoOp(UndoOp::AddPart, p2));
      applyOperationGroup(operations);
      }


//---------------------------------------------------------
//   cmdGluePart
//---------------------------------------------------------

void Song::cmdGluePart(Track* track, Part* oPart)
      { /* disabled for now, to be deleted
      // p3.3.54
      if(track->type() != Track::WAVE && !track->isMidiTrack())
        return;
      
      PartList* pl   = track->parts();
      Part* nextPart = 0;

      for (iPart ip = pl->begin(); ip != pl->end(); ++ip) {
            if (ip->second == oPart) {
                  ++ip;
                  if (ip == pl->end())
                        return;
                  nextPart = ip->second;
                  break;
                  }
            }

      Part* nPart = track->newPart(oPart);
      nPart->setLenTick(nextPart->tick() + nextPart->lenTick() - oPart->tick());

      // populate nPart with Events from oPart and nextPart

      EventList* sl1 = oPart->events();
      EventList* dl  = nPart->events();

      for (iEvent ie = sl1->begin(); ie != sl1->end(); ++ie)
            dl->add(ie->second);

      EventList* sl2 = nextPart->events();
      
      if(track->type() == Track::WAVE)
      {
        int frameOffset = nextPart->frame() - oPart->frame();
        for (iEvent ie = sl2->begin(); ie != sl2->end(); ++ie) 
        {
              Event event = ie->second.clone();
              event.setFrame(event.frame() + frameOffset);
              dl->add(event);
        }
      }
      else
      if(track->isMidiTrack())
      {
        int tickOffset  = nextPart->tick() - oPart->tick();
        for (iEvent ie = sl2->begin(); ie != sl2->end(); ++ie) 
        {
              Event event = ie->second.clone();
              event.setTick(event.tick() + tickOffset);
              dl->add(event);
        }
      }
            
      startUndo();
      MusEGlobal::audio->msgRemovePart(nextPart, false);
      // Indicate no undo, and do port controller values but not clone parts. 
      MusEGlobal::audio->msgChange***Part(oPart, nPart, false, true, false);
      endUndo(SC_PART_MODIFIED | SC_PART_REMOVED); */
      }

//---------------------------------------------------------
//   dump
//---------------------------------------------------------

void Part::dump(int n) const
      {
      for (int i = 0; i < n; ++i)
            putchar(' ');
      printf("Part: <%s> ", _name.toLatin1().constData());
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

int MidiPart::hasHiddenEvents() 
{
  unsigned len = lenTick();

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
}

//---------------------------------------------------------
//   hasHiddenEvents
//   Returns combination of HiddenEventsType enum.
//---------------------------------------------------------

int WavePart::hasHiddenEvents() 
{
  unsigned len = lenFrame();
  
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
}

//---------------------------------------------------------
//   ClonePart
//---------------------------------------------------------

ClonePart::ClonePart(const Part* p, int i) 
{
  cp = p;
  id = i;
  uuid_generate(uuid);
}


} // namespace MusECore
