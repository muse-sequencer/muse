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
#include "midictrl.h"
#include "operations.h"

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
  
  this->_clonemaster_sn = p->_sn;
}

void Part::rechainClone()
{
    if(_backupClone)
    {
        this->chainClone(_backupClone);
        _backupClone = NULL;
    }
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

void addPortCtrlEvents(Event& event, Part* part)
{
  Track* t = part->track();
  if(t && t->isMidiTrack())
  {
    MidiTrack* mt = (MidiTrack*)t;
    MidiPort* mp = &MusEGlobal::midiPorts[mt->outPort()];
    int ch = mt->outChannel();
    unsigned len = part->lenTick();
    // Do not add events which are past the end of the part.
    if(event.tick() < len)
    {
      if(event.type() == Controller)
      {
	int tck  = event.tick() + part->tick();
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
	mp->setControllerVal(ch, tck, cntrl, val, part);
      }
    }
  }
}

void addPortCtrlEvents(const Event& event, Part* part, unsigned int tick, unsigned int len, Track* track, PendingOperationList& ops)
{
  if(!track || !track->isMidiTrack())
    return;
  // Do not add events which are past the end of the part.
  if(event.tick() >= len)
    return;
  
  MidiTrack* mt = (MidiTrack*)track;
  MidiPort* mp = &MusEGlobal::midiPorts[mt->outPort()];
  int ch = mt->outChannel();
  
  if(event.type() == Controller)
  {
    int tck  = event.tick() + tick;
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
    
    MidiCtrlValListList* mcvll = mp->controller();
    MidiCtrlValList* mcvl = NULL;
    iMidiCtrlValList imcvll = mcvll->find(ch, cntrl);
    if(imcvll == mcvll->end()) 
    {
      PendingOperationItem poi(mcvll, 0, ch, cntrl, PendingOperationItem::AddMidiCtrlValList);
      if(ops.findAllocationOp(poi) == ops.end())
      {
        mcvl = new MidiCtrlValList(cntrl);
        poi._mcvl = mcvl;
        ops.add(poi);
      }
    }
    else
    {
      mcvl = imcvll->second;
      iMidiCtrlVal imcv = mcvl->findMCtlVal(tck, part);
      if(imcv != mcvl->end()) 
      {
        ops.add(PendingOperationItem(mcvl, imcv, val, PendingOperationItem::ModifyMidiCtrlVal));
        return;
      }
    }    
    //assert(mcvl != NULL); //FIXME: Can this happen? (danvd). UPDATE: Yes, it can (danvd)
    if(mcvl != NULL)
    {
      ops.add(PendingOperationItem(mcvl, part, tck, val, PendingOperationItem::AddMidiCtrlVal));
    }
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
//   addPortCtrlEvents
//---------------------------------------------------------

void addPortCtrlEvents(Part* part, unsigned int tick, unsigned int len, Track* track, PendingOperationList& ops)
{
  if(!track || !track->isMidiTrack())
    return;
  for(ciEvent ie = part->events().begin(); ie != part->events().end(); ++ie)
  {
    // Do not add events which are past the end of the part.
    if(ie->second.tick() >= (unsigned int)len)
      return; // Done
    addPortCtrlEvents(ie->second, part, tick, len, track, ops);
  }
}

void removePortCtrlEvents(Event& event, Part* part)
{
  Track* t = part->track();
  if(t && t->isMidiTrack())
  {
    MidiTrack* mt = (MidiTrack*)t;
    MidiPort* mp = &MusEGlobal::midiPorts[mt->outPort()];
    int ch = mt->outChannel();
    if(event.type() == Controller)
    {
      int tck  = event.tick() + part->tick();
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
      mp->deleteController(ch, tck, cntrl, part);
    }
  }
}

void removePortCtrlEvents(const Event& event, Part* part, Track* track, PendingOperationList& ops)
{
  if(!track || !track->isMidiTrack())
    return;
  
  MidiTrack* mt = (MidiTrack*)track;
  MidiPort* mp = &MusEGlobal::midiPorts[mt->outPort()];
  int ch = mt->outChannel();
  if(event.type() == Controller)
  {
    int tck  = event.tick() + part->tick();
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

    MidiCtrlValListList* mcvll = mp->controller();
    iMidiCtrlValList cl = mcvll->find(ch, cntrl);
    if (cl == mcvll->end()) {
                fprintf(stderr, "removePortCtrlEvents: controller %d(0x%x) for channel %d not found size %zd\n",
                    cntrl, cntrl, ch, mcvll->size());
          return;
          }
    MidiCtrlValList* mcvl = cl->second;
    iMidiCtrlVal imcv = mcvl->findMCtlVal(tck, part);
    if (imcv == mcvl->end()) {
            fprintf(stderr, "removePortCtrlEvents (%d): not found (size %zd)\n", tck, mcvl->size());
          return;
          }
    ops.add(PendingOperationItem(mcvl, imcv, PendingOperationItem::DeleteMidiCtrlVal));
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

void removePortCtrlEvents(Part* part, Track* track, PendingOperationList& ops)
{
  if(!track || !track->isMidiTrack())
    return;
  unsigned len = part->lenValue();
  for(ciEvent ie = part->events().begin(); ie != part->events().end(); ++ie)
  {
    // Do not attempt to remove events which are past the end of the part.
    // They should never be added in the first place, and cause a benign error in the event function.
    if(ie->second.posValue() >= len)
      return; // Done
    removePortCtrlEvents(ie->second, part, track, ops);
  }
}

void modifyPortCtrlEvents(const Event& old_event, const Event& event, Part* part, PendingOperationList& ops)
{
  Track* t = part->track();
  if(!t || !t->isMidiTrack())
    return;
  if(old_event.type() != Controller || event.type() != Controller)
    return;
  MidiTrack* mt = static_cast<MidiTrack*>(t);
  
  MidiPort* mp_erase = &MusEGlobal::midiPorts[mt->outPort()];
  MidiPort* mp_add = mp_erase;
  int ch = mt->outChannel();
  
  int tck_erase  = old_event.tick() + part->tick();
  int cntrl_erase = old_event.dataA();
  iMidiCtrlVal imcv_erase;
  bool found_erase = false;
  // Is it a drum controller old_event, according to the track port's instrument?
  if(mt->type() == Track::DRUM)
  {
    MidiController* mc = mp_erase->drumController(cntrl_erase);
    if(mc)
    {
      int note = cntrl_erase & 0x7f;
      cntrl_erase &= ~0xff;
      // Default to track port if -1 and track channel if -1.
      if(MusEGlobal::drumMap[note].channel != -1)
        ch = MusEGlobal::drumMap[note].channel;
      if(MusEGlobal::drumMap[note].port != -1)
        mp_erase = &MusEGlobal::midiPorts[MusEGlobal::drumMap[note].port];
      cntrl_erase |= MusEGlobal::drumMap[note].anote;
    }
  }

  MidiCtrlValListList* mcvll_erase = mp_erase->controller();
  MidiCtrlValList* mcvl_erase = 0;
  iMidiCtrlValList cl_erase = mcvll_erase->find(ch, cntrl_erase);
  if(cl_erase == mcvll_erase->end()) 
  {
    if(MusEGlobal::debugMsg)
      printf("deleteController: controller %d(0x%x) for channel %d not found size %zd\n",
              cntrl_erase, cntrl_erase, ch, mcvll_erase->size());
  }
  else
  {
    mcvl_erase = cl_erase->second;
    imcv_erase = mcvl_erase->findMCtlVal(tck_erase, part);
    if(imcv_erase == mcvl_erase->end()) 
    {
      if(MusEGlobal::debugMsg)
        printf("MidiCtrlValList::delMCtlVal(%d): not found (size %zd)\n", tck_erase, mcvl_erase->size());
    }
    else
      found_erase = true;
  }

  
  unsigned len = part->lenTick();
  // Do not add events which are past the end of the part.
  if(event.tick() < len)
  {
    int tck_add  = event.tick() + part->tick();
    int cntrl_add = event.dataA();
    int val_add   = event.dataB();
    // Is it a drum controller event, according to the track port's instrument?
    if(mt->type() == Track::DRUM)
    {
      MidiController* mc_add = mp_add->drumController(cntrl_add);
      if(mc_add)
      {
        int note = cntrl_add & 0x7f;
        cntrl_add &= ~0xff;
        // Default to track port if -1 and track channel if -1.
        if(MusEGlobal::drumMap[note].channel != -1)
          ch = MusEGlobal::drumMap[note].channel;
        if(MusEGlobal::drumMap[note].port != -1)
          mp_add = &MusEGlobal::midiPorts[MusEGlobal::drumMap[note].port];
        cntrl_add |= MusEGlobal::drumMap[note].anote;
      }
    }

    MidiCtrlValList* mcvl_add;
    MidiCtrlValListList* mcvll_add = mp_add->controller();
    iMidiCtrlValList imcvll_add = mcvll_add->find(ch, cntrl_add);
    if(imcvll_add == mcvll_add->end()) 
    {
      if(found_erase)
        ops.add(PendingOperationItem(mcvl_erase, imcv_erase, PendingOperationItem::DeleteMidiCtrlVal));
      PendingOperationItem poi(mcvll_add, 0, ch, cntrl_add, PendingOperationItem::AddMidiCtrlValList);
      if(ops.findAllocationOp(poi) == ops.end())
      {
        poi._mcvl = new MidiCtrlValList(cntrl_add);
        ops.add(poi);
      }
      ops.add(PendingOperationItem(poi._mcvl, part, tck_add, val_add, PendingOperationItem::AddMidiCtrlVal));
      return;
    }
    else
    {
      mcvl_add = imcvll_add->second;
      iMidiCtrlVal imcv_add = mcvl_add->findMCtlVal(tck_add, part);
      if(imcv_add != mcvl_add->end()) 
      {
        if(tck_erase == tck_add && mcvl_erase == mcvl_add)
          ops.add(PendingOperationItem(mcvl_add, imcv_add, val_add, PendingOperationItem::ModifyMidiCtrlVal));
        else
        {
          if(found_erase)
            ops.add(PendingOperationItem(mcvl_erase, imcv_erase, PendingOperationItem::DeleteMidiCtrlVal));
          ops.add(PendingOperationItem(mcvl_add, part, tck_add, val_add, PendingOperationItem::AddMidiCtrlVal));
        }
        return;
      }
      else
      {
        if(found_erase)
          ops.add(PendingOperationItem(mcvl_erase, imcv_erase, PendingOperationItem::DeleteMidiCtrlVal));
        ops.add(PendingOperationItem(mcvl_add, part, tck_add, val_add, PendingOperationItem::AddMidiCtrlVal));
      }
    }
  }
  else
  {
    if(found_erase)
      ops.add(PendingOperationItem(mcvl_erase, imcv_erase, PendingOperationItem::DeleteMidiCtrlVal));
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

      
void PartList::addOperation(Part* part, PendingOperationList& ops)
{
  // There is protection, in the catch-all Undo::insert(), from failure here (such as double add, del + add, add + del)
  //  which might cause addPortCtrlEvents() without parts or without corresponding removePortCtrlEvents etc.
  ops.add(PendingOperationItem(this, part, PendingOperationItem::AddPart));
  addPortCtrlEvents(part, part->posValue(), part->lenValue(), part->track(), ops);
}

void PartList::delOperation(Part* part, PendingOperationList& ops)
{
  // There is protection, in the catch-all Undo::insert(), from failure here (such as double del, del + add, add + del)
  //  which might cause addPortCtrlEvents() without parts or without corresponding removePortCtrlEvents etc.
  removePortCtrlEvents(part, part->track(), ops);
  iPart i;
  for (i = begin(); i != end(); ++i) {
        if (i->second == part) {
              ops.add(PendingOperationItem(this, i, PendingOperationItem::DeletePart));
              return;
              }
        }
  printf("THIS SHOULD NEVER HAPPEN: could not find the part in PartList::delOperation()!\n");
}
      
void PartList::movePartOperation(Part* part, int new_pos, PendingOperationList& ops, Track* track)
{
  removePortCtrlEvents(part, part->track(), ops);
  iPart i = end();
  if(track)
  {
    for (i = begin(); i != end(); ++i) {
          if (i->second == part) 
                break;
          }
    if(i == end())
      printf("THIS SHOULD NEVER HAPPEN: could not find the part in PartList::movePartOperation()!\n");
  }
  
  ops.add(PendingOperationItem(i, part, new_pos, PendingOperationItem::MovePart, track));

  if(!track)
    track = part->track();
  
  addPortCtrlEvents(part, new_pos, part->lenValue(), track, ops);
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
            case Track::MIDI:
            case Track::DRUM:
            case Track::NEW_DRUM:
                  {
                  Undo operations;
                                                                        
                  unsigned orig_len = oPart->lenValue();
                  Part* part_it = oPart;
                  do
                  {
                      if(part_it->lenValue() == orig_len)
                        operations.push_back(UndoOp(UndoOp::ModifyPartLength, part_it, orig_len, len, Pos::TICKS));
                          
                      part_it = part_it->nextClone();
                  } while (doClones && (part_it != oPart));
                  
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
                        p1->addEvent(si);
                        }
                  if ((s2 > d1p2) && (s1 < d2p2)) {
                        Event si = event.mid(d1p2 - ps, d2p2 - ps);
                        p2->addEvent(si);
                        }
                  }
            }
      else {
            for (ciEvent ie = _events.begin(); ie != _events.end(); ++ie) {
                  Event event = ie->second.clone();
                  int t = event.tick();
                  if (t >= l1) {
                        event.move(-l1);
                        p2->addEvent(event);
                        }
                  else
                        p1->addEvent(event);
                  }
            }
      }

//---------------------------------------------------------
//   changePart
//---------------------------------------------------------

void Song::changePart(Part* oPart, Part* nPart)
      {
      nPart->setSn(oPart->sn());

      Track* oTrack = oPart->track();
      Track* nTrack = nPart->track();

      oTrack->parts()->remove(oPart);
      nTrack->parts()->add(nPart);

      // Added by T356.
      // adjust song len:
      unsigned epos = nPart->tick() + nPart->lenTick();
      if (epos > len())
            _len = epos;
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

int MidiPart::hasHiddenEvents() const
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

int WavePart::hasHiddenEvents() const
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
  is_deleted = false;
  uuid_generate(uuid);
}


} // namespace MusECore
