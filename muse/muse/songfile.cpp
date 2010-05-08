//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: songfile.cpp,v 1.25.2.12 2009/11/04 15:06:07 spamatica Exp $
//
//  (C) Copyright 1999/2000 Werner Schweer (ws@seh.de)
//=========================================================

#include <assert.h>
#include <qmessagebox.h>
#include <uuid/uuid.h>

#include "app.h"
#include "song.h"
#include "arranger.h"
#include "transport.h"
#include "cobject.h"
#include "drumedit.h"
#include "pianoroll.h"
#include "globals.h"
#include "xml.h"
#include "drummap.h"
#include "event.h"
#include "marker/marker.h"
#include "midiport.h"
#include "audio.h"
#include "mitplugin.h"
#include "wave.h"
#include "midictrl.h"
#include "amixer.h"
#include "conf.h"
#include "driver/jackmidi.h"

//struct ClonePart {
      //const EventList* el;
//      const Part* cp;
//      int id;
      //ClonePart(const EventList* e, int i) : el(e), id(i) {}
//      ClonePart(const Part* p, int i) : cp(p), id(i) {}
//      };

//typedef std::list<ClonePart> CloneList;
//typedef CloneList::iterator iClone;

//---------------------------------------------------------
//   ClonePart
//---------------------------------------------------------

ClonePart::ClonePart(const Part* p, int i) 
{
  cp = p;
  id = i;
  uuid_generate(uuid);
}

//static CloneList cloneList;
//static CloneList copyCloneList;
CloneList cloneList;
//CloneList copyCloneList;

/*
//---------------------------------------------------------
//   updateCloneList
//---------------------------------------------------------

void updateCloneList(Part* oPart, Part* nPart)
{
  for(iClone i = cloneList.begin(); i != cloneList.end(); ++i) 
  {
    if(i->cp == oPart) 
    {
      i->cp = nPart;
      break;
    }
  }
}

void updateCloneList(PartList* oParts, PartList* nParts)
{
  for(iPart ip = oParts->begin(); ip != oParts->end(); ++ip) 
  {
    for(iClone i = cloneList.begin(); i != cloneList.end(); ++i) 
    {
      if(i->cp == oPart) 
      {
        i->cp = nPart;
        break;
      }
    }  
  }
}

//---------------------------------------------------------
//   clearClipboardAndCloneList
//---------------------------------------------------------

void clearClipboardAndCloneList()
{
  //QApplication::clipboard()->clear(QClipboard::Clipboard);
  cloneList.clear();
}
*/

//---------------------------------------------------------
//   NKey::write
//---------------------------------------------------------

void NKey::write(int level, Xml& xml) const
      {
      xml.intTag(level, "key", val);
      }

//---------------------------------------------------------
//   NKey::read
//---------------------------------------------------------

void NKey::read(Xml& xml)
      {
      for (;;) {
            Xml::Token token = xml.parse();
            switch (token) {
                  case Xml::Error:
                  case Xml::End:
                        return;
                  case Xml::Text:
                        val = xml.s1().toInt();
                        break;
                  case Xml::TagEnd:
                        if (xml.s1() == "key")
                              return;
                  default:
                        break;
                  }
            }
      }

//---------------------------------------------------------
//   Scale::write
//---------------------------------------------------------

void Scale::write(int level, Xml& xml) const
      {
      xml.intTag(level, "scale", val);
      }

//---------------------------------------------------------
//   Scale::read
//---------------------------------------------------------

void Scale::read(Xml& xml)
      {
      for (;;) {
            Xml::Token token = xml.parse();
            switch (token) {
                  case Xml::Error:
                  case Xml::End:
                        return;
                  case Xml::Text:
                        val = xml.s1().toInt();
                        break;
                  case Xml::TagEnd:
                        if (xml.s1() == "scale")
                              return;
                  default:
                        break;
                  }
            }
      }

//---------------------------------------------------------
//   readXmlPart
//---------------------------------------------------------

Part* readXmlPart(Xml& xml, Track* track, bool doClone, bool toTrack)
      {
      int id = -1;
      Part* npart = 0;
      uuid_t uuid; 
      uuid_clear(uuid);
      bool uuidvalid = false;
      bool clone = true;
      bool wave = false;
      bool isclone = false;

      for (;;) {
            Xml::Token token = xml.parse();
            const QString& tag = xml.s1();
            switch (token) {
                  case Xml::Error:
                  case Xml::End:
                        return npart;
                  case Xml::TagStart:
                        // If the part has not been created yet...
                        if(!npart)
                        {
                          // If an id was found...
                          if(id != -1)
                          {
                            for(iClone i = cloneList.begin(); i != cloneList.end(); ++i) 
                            {
                              // Is a matching part found in the clone list?
                              if(i->id == id) 
                              {
                                // If it's a regular paste (not paste clone), and the original part is
                                //  not a clone, defer so that a new copy is created in TagStart above.
                                //if(!doClone && i->cp->cevents()->arefCount() <= 1)
                                //if(!doClone && !isclone)
                                //  break;
                                  
                                // This makes a clone, chains the part, and increases ref counts.
                                npart = track->newPart((Part*)i->cp, true);
                                break;
                              }
                            }
                          }  
                          else
                          // If a uuid was found...
                          if(uuidvalid)
                          {
                            for(iClone i = cloneList.begin(); i != cloneList.end(); ++i) 
                            {
                              // Is a matching part found in the clone list?
                              if(uuid_compare(uuid, i->uuid) == 0) 
                              {
                                Track* cpt = i->cp->track();
                                // If we want to paste to the given track...
                                if(toTrack)
                                {
                                  // If the given track type is not the same as the part's 
                                  //  original track type, we can't continue. Just return.
                                  if(!track || cpt->type() != track->type())
                                  {
                                    xml.skip("part");
                                    return 0;
                                  }  
                                }
                                else
                                // ...else we want to paste to the part's original track.
                                {
                                  // Make sure the track exists (has not been deleted).
                                  if((cpt->isMidiTrack() && song->midis()->find(cpt) != song->midis()->end()) || 
                                      (cpt->type() == Track::WAVE && song->waves()->find(cpt) != song->waves()->end()))
                                    track = cpt;   
                                  else
                                  // Track was not found. Try pasting to the given track, as above...
                                  {
                                    if(!track || cpt->type() != track->type())
                                    {
                                      // No luck. Just return.
                                      xml.skip("part");
                                      return 0;
                                    }  
                                  }
                                }
                                
                                // If it's a regular paste (not paste clone), and the original part is
                                //  not a clone, defer so that a new copy is created in TagStart above.
                                //if(!doClone && i->cp->cevents()->arefCount() <= 1)
                                if(!doClone && !isclone)
                                  break;
                                  
                                // This makes a clone, chains the part, and increases ref counts.
                                npart = track->newPart((Part*)i->cp, true);
                                break;
                              }
                            }  
                          }
                        
                          // If the part still has not been created yet...
                          if(!npart)
                          {
                            // A clone was not created from any matching part. Create a non-clone part now.
                            if(!track)
                            {
                              xml.skip("part");
                              return 0;
                            }  
                            // If we're pasting to selected track and the 'wave' 
                            //  variable is valid, check for mismatch...
                            if(toTrack && uuidvalid)
                            {
                              // If both the part and track are not midi or wave...
                              if((wave && track->isMidiTrack()) || 
                                (!wave && track->type() == Track::WAVE))
                              {
                                xml.skip("part");
                                return 0;
                              }   
                            }
                              
                            if (track->isMidiTrack())
                              npart = new MidiPart((MidiTrack*)track);
                            else if (track->type() == Track::WAVE)
                              npart = new WavePart((WaveTrack*)track);
                            else
                            {
                              xml.skip("part");
                              return 0;
                            }  
                            
                            // Signify a new non-clone part was created.
                            // Even if the original part was itself a clone, clear this because the
                            //  attribute section did not create a clone from any matching part.
                            clone = false;
                            
                            // If an id or uuid was found, add the part to the clone list 
                            //  so that subsequent parts can look it up and clone from it...
                            if(id != -1)
                            {
                              ClonePart ncp(npart, id);
                              cloneList.push_back(ncp);
                            }
                            else  
                            if(uuidvalid)
                            {
                              ClonePart ncp(npart);
                              // New ClonePart creates its own uuid, but we need to replace it.
                              uuid_copy(ncp.uuid, uuid);
                              cloneList.push_back(ncp);
                            }
                          }
                        }  
                        
                        if (tag == "name")
                              npart->setName(xml.parse1());
                        else if (tag == "poslen") {
                              ((PosLen*)npart)->read(xml, "poslen");
                              }
                        else if (tag == "pos") {
                              Pos pos;
                              pos.read(xml, "pos");  // obsolete
                              npart->setTick(pos.tick());
                              }
                        else if (tag == "len") {
                              Pos len;
                              len.read(xml, "len");  // obsolete
                              npart->setLenTick(len.tick());
                              }
                        else if (tag == "selected")
                              npart->setSelected(xml.parseInt());
                        else if (tag == "color")
                              npart->setColorIndex(xml.parseInt());
                        else if (tag == "mute")
                              npart->setMute(xml.parseInt());
                        else if (tag == "event") 
                        {
                              // If a new non-clone part was created, accept the events...
                              if(!clone)
                              {
                                EventType type = Wave;
                                if(track->isMidiTrack())
                                  type = Note;
                                Event e(type);
                                e.read(xml);
                                // stored tickpos for event has absolute value. However internally
                                // tickpos is relative to start of part, we substract tick().
                                // TODO: better handling for wave event
                                e.move( -npart->tick() );
                                int tick = e.tick();  
                                
                                // Do not discard events belonging to clone parts,
                                //  at least not yet. A later clone might have a longer, 
                                //  fully accommodating part length!
                                //if ((tick < 0) || (tick >= (int) lenTick())) {
                                //if ((tick < 0) || ( id == -1 && !clone && (tick >= (int)lenTick()) )) 
                                // No way to tell at the moment whether there will be clones referencing this...
                                // No choice but to accept all events past 0.
                                if(tick < 0) 
                                {
                                  //printf("readClone: warning: event not in part: %d - %d -%d, discarded\n",
                                  printf("readClone: warning: event at tick:%d not in part:%s, discarded\n",
                                    tick, npart->name().latin1());
                                }
                                else 
                                {
                                  npart->events()->add(e);
                                }      
                              }
                              else
                                // ...Otherwise a clone was created, so we don't need the events.
                                xml.skip(tag);
                        }
                        else
                              xml.unknown("readXmlPart");
                        break;
                  case Xml::Attribut:
                        if (tag == "type")
                        {
                          if(xml.s2() == "wave")
                            wave = true;
                        }
                        else if (tag == "cloneId")
                        {
                          id = xml.s2().toInt();
                          //if(id != -1)
                          //{
                          //  for(iClone i = cloneList.begin(); i != cloneList.end(); ++i) 
                          //  {
                              // Is a matching part found in the clone list?
                          //    if(i->id == id) 
                          //    {
                                // If it's a regular paste (not paste clone), and the original part is
                                //  not a clone, defer so that a new copy is created in TagStart above.
                                //if(!doClone && i->cp->cevents()->arefCount() <= 1)
                                //if(!doClone && !isclone)
                                //  break;
                                  
                                // This makes a clone, chains the part, and increases ref counts.
                          //      npart = track->newPart((Part*)i->cp, true);
                          //      break;
                          //    }
                          //  }
                          //}  
                        }      
                        else if (tag == "uuid")
                        {
                          uuid_parse(xml.s2().latin1(), uuid);
                          if(!uuid_is_null(uuid))
                          {
                            uuidvalid = true;
                            /*
                            for(iClone i = cloneList.begin(); i != cloneList.end(); ++i) 
                            {
                              // Is a matching part found in the clone list?
                              if(uuid_compare(uuid, i->uuid) == 0) 
                              {
                                Track* cpt = i->cp->track();
                                // If we want to paste to the given track...
                                if(toTrack)
                                {
                                  // If the given track type is not the same as the part's 
                                  //  original track type, we can't continue. Just return.
                                  if(!track || cpt->type() != track->type())
                                  {
                                    xml.skip("part");
                                    return 0;
                                  }  
                                }
                                else
                                // ...else we want to paste to the part's original track.
                                {
                                  // Make sure the track exists (has not been deleted).
                                  if((cpt->isMidiTrack() && song->midis()->find(cpt) != song->midis()->end()) || 
                                     (cpt->type() == Track::WAVE && song->waves()->find(cpt) != song->waves()->end()))
                                    track = cpt;   
                                  else
                                  // Track was not found. Try pasting to the given track, as above...
                                  {
                                    if(!track || cpt->type() != track->type())
                                    {
                                      // No luck. Just return.
                                      xml.skip("part");
                                      return 0;
                                    }  
                                  }
                                }
                                
                                // If it's a regular paste (not paste clone), and the original part is
                                //  not a clone, defer so that a new copy is created in TagStart above.
                                //if(!doClone && i->cp->cevents()->arefCount() <= 1)
                                if(!doClone && !isclone)
                                  break;
                                  
                                // This makes a clone, chains the part, and increases ref counts.
                                npart = track->newPart((Part*)i->cp, true);
                                break;
                              }
                            }
                            */  
                          }
                        }      
                        else if(tag == "isclone")        
                          isclone = xml.s2().toInt();
                        break;
                  case Xml::TagEnd:
                        if (tag == "part") 
                          return npart;
                  default:
                        break;
                  }
            }
  return npart;            
}

//---------------------------------------------------------
//   Part::write
//   If isCopy is true, write the xml differently so that 
//    we can have 'Paste Clone' feature. 
//---------------------------------------------------------

//void Part::write(int level, Xml& xml) const
void Part::write(int level, Xml& xml, bool isCopy, bool forceWavePaths) const
      {
      const EventList* el = cevents();
      int id              = -1;
      uuid_t uuid; 
      uuid_clear(uuid);
      bool dumpEvents     = true;
      bool wave = _track->type() == Track::WAVE;
      
      if(isCopy)
      {
        //for(iClone i = copyCloneList.begin(); i != copyCloneList.end(); ++i) 
        for(iClone i = cloneList.begin(); i != cloneList.end(); ++i) 
        {
          //if(i->el == el) {
          if(i->cp->cevents() == el) 
          {
            //id = i->id;
            uuid_copy(uuid, i->uuid);
            dumpEvents = false;
            break;
          }
        }
        //if(id == -1) 
        if(uuid_is_null(uuid)) 
        {
          //id = copyCloneList.size();
          //id = cloneList.size();
          //ClonePart cp(el, id);
          //ClonePart cp(this, id);
          ClonePart cp(this);
          uuid_copy(uuid, cp.uuid);
          //copyCloneList.push_back(cp);
          cloneList.push_back(cp);
        }
      }  
      else
      {
        if (el->arefCount() > 1) 
        {
          for (iClone i = cloneList.begin(); i != cloneList.end(); ++i) 
          {
            //if (i->el == el) {
            if (i->cp->cevents() == el) 
            {
              id = i->id;
              //uuid_copy(id, i->uid);
              dumpEvents = false;
              break;
            }
          }
          if (id == -1) 
          //if(uuid_is_null(id)) 
          {
            id = cloneList.size();
            //ClonePart cp(el, id);
            ClonePart cp(this, id);
            //ClonePart cp(this);
            cloneList.push_back(cp);
          }
        }
      }  

      // Special markers if this is a copy operation and the 
      //  part is a clone.
      if(isCopy)
      {
        char sid[40]; // uuid string is 36 chars. Try 40 for good luck.
        sid[0] = 0;
        uuid_unparse_lower(uuid, sid);
        //if(midi)
        //  xml.nput(level, "<midipart uuid=\"%s\"", sid);
        //else  
        if(wave)
          xml.nput(level, "<part type=\"wave\" uuid=\"%s\"", sid);
        else  
          xml.nput(level, "<part uuid=\"%s\"", sid);
        
        if(el->arefCount() > 1)
          xml.nput(" isclone=\"1\"");
        xml.put(">");
        level++;  
      }
      else
      if (id != -1)
      //if(!uuid_is_null(id))
      {
        xml.tag(level++, "part cloneId=\"%d\"", id);
        //char sid[40]; // uuid string is 36 chars. Try 40 for good luck.
        //sid[0] = 0;
        //uuid_unparse_lower(id, sid);
        //xml.tag(level++, "part cloneId=\"%s\"", sid);
      }      
      else
        xml.tag(level++, "part");
      
      xml.strTag(level, "name", _name);

//      PosLen poslen(*this);
//      int tickpos = tick();
//      poslen.setTick(tickpos);
      PosLen::write(level, xml, "poslen");
      xml.intTag(level, "selected", _selected);
      xml.intTag(level, "color", _colorIndex);
      if (_mute)
            xml.intTag(level, "mute", _mute);
      if (dumpEvents) {
            for (ciEvent e = el->begin(); e != el->end(); ++e)
                  //e->second.write(level, xml, *this);
                  e->second.write(level, xml, *this, forceWavePaths);
            }
      xml.etag(level, "part");
      }

/*
//---------------------------------------------------------
//   Part::read
//---------------------------------------------------------

void Part::read(Xml& xml, int, bool toTrack)    // int newPartOffset
      {
      int id = -1;
      bool containsEvents = false;
      uuid_t uuid; 
      uuid_clear(uuid);
      bool uuidvalid = false;
      bool clone = false;

      for (;;) {
            Xml::Token token = xml.parse();
            const QString& tag = xml.s1();
            switch (token) {
                  case Xml::Error:
                  case Xml::End:
                        return;
                  case Xml::TagStart:
                        if (tag == "name")
                              _name = xml.parse1();
                        else if (tag == "poslen") {
                              PosLen::read(xml, "poslen");
                              }
                        else if (tag == "pos") {
                              Pos pos;
                              pos.read(xml, "pos");  // obsolete
                              setTick(pos.tick());
                              }
                        else if (tag == "len") {
                              Pos len;
                              len.read(xml, "len");  // obsolete
                              setLenTick(len.tick());
                              }
                        else if (tag == "selected")
                              _selected = xml.parseInt();
                        else if (tag == "color")
                              _colorIndex = xml.parseInt();
                        else if (tag == "mute")
                              _mute = xml.parseInt();
                        else if (tag == "event") {
                              containsEvents = true;
                              EventType type = Wave;
                              if (_track->isMidiTrack())
                                    type = Note;
                              Event e(type);
                              e.read(xml);
                              // stored tickpos for event has absolute value. However internally
                              // tickpos is relative to start of part, we substract tick().
                              // TODO: better handling for wave event
                              e.move(-tick());
                              int tick = e.tick();  
                              
                              // Changed by T356. Do not discard events belonging to clone parts,
                              //  at least not yet. A later clone might have a longer, 
                              //  fully accommodating part length!
                              //if ((tick < 0) || (tick >= (int) lenTick())) {
                              if ((tick < 0) || ( id == -1 && !clone && (tick >= (int)lenTick()) )) 
                              {
                                    //printf("Part::read: warning: event not in part: %d - %d -%d, discarded\n",
                                    printf("Part::read: warning: event at tick:%d not in part:%s, discarded\n",
                                       tick, name().latin1());
                              }
                              else {
                                    _events->add(e);
*/                                    
                                    
                                    
                                    /*
                                    // TODO: This should NOT be done here since the event list
                                    //  might be deleted below. Since after reading a part it
                                    //  likely (always?) that (msg)AddPart() or (msg)ChangePart()
                                    //  will be called (must check if they're ever called BEFORE 
                                    //  Part::read), then those routines will take care of it,
                                    //  they are already coded to do so. 
                                    // Note the redundancy of doing it here AND (msg)Add/ChangePart !
                                    // Try to eliminate this section altogether by verifying that
                                    //  (msg)Add/ChangePart (or one of the other routines which add port
                                    //  controller values) is always called after Part::read...
                                    if (e.type() == Controller) {
                                          MidiTrack* mt = (MidiTrack*)_track;
                                          int channel = mt->outChannel();
                                          MidiPort* mp = &midiPorts[mt->outPort()];
                                          // tick is relative to part, controller needs an absolute value hence
                                          // part offset is added. If newPartOffset was given we use that instead of
                                          // the recorded offset!
                                          if (!newPartOffset)
                                            newPartOffset=this->tick();

                                          int ctl = e.dataA();
                                          if(mt->type() == Track::DRUM)
                                          {
                                            // Is it a drum controller event, according to the track port's instrument?
                                            MidiController* mc = mp->drumController(ctl);
                                            if(mc)
                                            {
                                              int note = ctl & 0x7f;
                                              ctl &= ~0xff;
                                              channel = drumMap[note].channel;
                                              mp = &midiPorts[drumMap[note].port];
                                              ctl |= drumMap[note].anote;
                                            }
                                          }  
                                          
                                          // Removed by T356
                                          // check if controller exists
                                          //if (mp->hwCtrlState(channel, e.dataA()) == CTRL_VAL_UNKNOWN) {
                                          //      mp->addManagedController(channel, e.dataA());
                                          //      }
                                          
                                          // Changed by T356
                                          // add controller value
                                          //mp->setCtrl(channel, tick+newPartOffset, e.dataA(), e.dataB());
                                          mp->setControllerVal(channel, tick+newPartOffset, ctl, e.dataB(), this);
                                          }
                                      */
/*                                    
                                    }
                              }
                        else
                              xml.unknown("Part::read");
                        break;
                  case Xml::Attribut:
                        if (tag == "cloneId")
                              id = xml.s2().toInt();
                        else if (tag == "uuid")
                              {
                                uuid_parse(xml.s2().latin1(), uuid);
                                if(!uuid_is_null(uuid))
                                  uuidvalid = true;
                              }  
                        else if (tag == "isclone")
                              clone = xml.s2().toInt();
                        break;
                  case Xml::TagEnd:
                        if (tag == "part") 
                        {
*/                              
                              /*
                              if (id != -1) 
                              {
                                    
                                    // clone part
                                    if (containsEvents) {
                                          // add to cloneList:
                                          //ClonePart cp(_events, id);
                                          ClonePart cp(this, id);
                                          cloneList.push_back(cp);
                                          }
                                    else {
                                          // replace event list with clone event
                                          // list
                                          for (iClone i = cloneList.begin();
                                             i != cloneList.end(); ++i) {
                                                if (i->id == id) {
                                                      delete _events;
                                                      //_events = (EventList*)(i->el);
                                                      _events = (EventList*)(i->cp->cevents());
                                                      _events->incRef(1);
                                                      _events->incARef(1);
                                                      //i->cp->chainClone(this);
                                                      chainClone((Part*)i->cp, this);
                                                      break;
                                                      }
                                                }
                                          }
                                */          
                                          
/*                          
                          if(id != -1) 
                          {
                            // See if the part exists in the clone list. 
                            // The clone list is also the copy/paste clone list.
                            // Care must be taken to ensure the list is ALWAYS EMPTY 
                            //  before loading or dropping parts INTO muse, because the
                            //  current song parts are NOT the same as when the imported parts 
                            //  were created, (even if they were created from the current session,
                            //  we should NOT look them up). Always back up the list, clear it,
                            //  read part(s), then restore the list so that paste works after.
                            Part* cp = 0;
                            for(iClone i = cloneList.begin(); i != cloneList.end(); ++i) 
                            {  
                              if(i->id == id) 
                              {
                                cp = (Part*)i->cp;
                                break;
                              }
                            }
                            // Was a matching part found in the clone list?
                            if(cp)
                            {
                              // Make this part a clone of that part. Use its event list...
                              delete _events;
                              _events = (EventList*)(cp->cevents());
                              _events->incRef(1);
                              _events->incARef(1);
                              chainClone(cp, this);
                            }
                            else                                
                            {
                              // No matching part to clone was found in the clone list. 
                              // Does the part contain some events?
                              //if(containsEvents) 
                              {
                                // Add the part to the clone list so that subsequent parts
                                //  can look it up and clone from it...
                                ClonePart ncp(this, id);
                                cloneList.push_back(ncp);
                              }
                              // Otherwise this part has no matching part in the clone list
                              //  and no events of its own. Nothing left to do, we now have 
                              //  a blank part with the original offset, colour etc.
                            }
                          }
                          else
                          // If a uuid was found, do the same as above. Using uuids  
                          //  allows foolproof rejection of copied parts not found 
                          //  in the clone list, particularly when copying parts from 
                          //  another instance of muse.
                          if(uuidvalid) 
                          {
                            Part* cp = 0;
                            for(iClone i = cloneList.begin(); i != cloneList.end(); ++i) 
                            {  
                              if(uuid_compare(uuid, i->uuid) == 0) 
                              {
                                cp = (Part*)i->cp;
                                break;
                              }
                            }
                            // If a matching part was found, and we want to paste to the original track...
                            if(cp && !toTrack)
                            {
                              // Make sure the track exists (has not been deleted).
                              if((cp->track()->isMidiTrack() && song->midis()->find(cp->track()) != song->midis()->end()) || 
                                 (cp->track()->type() == Track::WAVE && song->waves()->find(cp->track()) != song->waves()->end()))
                                setTrack(cp->track());
                            }  
                            // Was a matching part found in the clone list, and was it 
                            //  originally a clone part?
                            if(cp && clone)
                            {
                              // Make this part a clone of that part. Use its event list...
                              delete _events;
                              _events = (EventList*)(cp->cevents());
                              _events->incRef(1);
                              _events->incARef(1);
                              // Chain the clone.
                              // Use the slower function which makes sure it chains to a part
                              //  within a valid (non-deleted) track.
                              //chainClone(cp, this);
                              chainClone(this);
                            }
                            else                                
                            {
                              // No matching part to clone was found in the clone list. 
                              // Does the part contain some events?
                              //if(containsEvents) 
                              {
                                // Add the part to the clone list so that subsequent parts
                                //  can look it up and clone from it...
                                ClonePart ncp(this);
                                // New ClonePart creates its own uuid, but we need to replace it.
                                uuid_copy(ncp.uuid, uuid);
                                cloneList.push_back(ncp);
                              }
                            }
                          }
                          return;
                        }
                  default:
                        break;
                  }
            }
      }
*/

//---------------------------------------------------------
//   writeFont
//---------------------------------------------------------

void Song::writeFont(int level, Xml& xml, const char* name,
   const QFont& font) const
      {
      xml.nput(level, "<%s family=\"%s\" size=\"%d\"",
         //name, font.family().latin1(), font.pointSize());
         name, Xml::xmlString(font.family()).latin1(), font.pointSize());
      if (font.weight() != QFont::Normal)
            xml.nput(" weight=\"%d\"", font.weight());
      if (font.italic())
            xml.nput(" italic=\"1\"");
      xml.nput(" />\n");
      }

//---------------------------------------------------------
//   readFont
//---------------------------------------------------------

QFont Song::readFont(Xml& xml, const char* name)
      {
      QFont f;
      for (;;) {
            Xml::Token token = xml.parse();
            switch (token) {
                  case Xml::Error:
                  case Xml::End:
                        return f;
                  case Xml::TagStart:
                        xml.unknown("readFont");
                        break;
                  case Xml::Attribut:
                        if (xml.s1() == "family")
                              f.setFamily(xml.s2());
                        else if (xml.s1() == "size")
                              f.setPointSize(xml.s2().toInt());
                        else if (xml.s1() == "weight")
                              f.setWeight(xml.s2().toInt());
                        else if (xml.s1() == "italic")
                              f.setItalic(xml.s2().toInt());
                        break;
                  case Xml::TagEnd:
                        if (xml.s1() == name)
                              return f;
                  default:
                        break;
                  }
            }
      return f;
      }

//---------------------------------------------------------
//   readPart
//---------------------------------------------------------

Part* MusE::readPart(Xml& xml)
      {
      Part* part = 0;
      for (;;) {
            Xml::Token token = xml.parse();
            const QString& tag = xml.s1();
            switch (token) {
                  case Xml::Error:
                  case Xml::End:
                        return part;
                  case Xml::Text:
                        {
                        int trackIdx, partIdx;
                        sscanf(tag.latin1(), "%d:%d", &trackIdx, &partIdx);
                        Track* track = song->tracks()->index(trackIdx);
                        if (track)
                              part = track->parts()->find(partIdx);
                        }
                        break;
                  case Xml::TagStart:
                        xml.unknown("readPart");
                        break;
                  case Xml::TagEnd:
                        if (tag == "part")
                              return part;
                  default:
                        break;
                  }
            }
      }

//---------------------------------------------------------
//   readToplevels
//---------------------------------------------------------

void MusE::readToplevels(Xml& xml)
      {
      PartList* pl = new PartList;
      for (;;) {
            Xml::Token token = xml.parse();
            const QString& tag = xml.s1();
            switch (token) {
                  case Xml::Error:
                  case Xml::End:
                        return;
                  case Xml::TagStart:
                        if (tag == "part") {
                              Part* part = readPart(xml);
                              if (part)
                                    pl->add(part);
                              }
                        else if (tag == "pianoroll") {
                              // p3.3.34
                              // Do not open if there are no parts.
                              // Had bogus '-1' part index for list edit in med file,
                              //  causing list edit to segfault on song load.
                              // Somehow that -1 was put there on write, because the
                              //  current part didn't exist anymore, so no index number
                              //  could be found for it on write. Watching... may be fixed.
                              // But for now be safe for all the top levels...
                              if(!pl->empty())
                              {
                                
                                startPianoroll(pl);
                                toplevels.back().cobject()->readStatus(xml);
                                pl = new PartList;
                              }  
                              }
                        else if (tag == "drumedit") {
                              if(!pl->empty())
                              {
                                startDrumEditor(pl);
                                toplevels.back().cobject()->readStatus(xml);
                                pl = new PartList;
                              }  
                              }
                        else if (tag == "listeditor") {
                              if(!pl->empty())
                              {
                                startListEditor(pl);
                                toplevels.back().cobject()->readStatus(xml);
                                pl = new PartList;
                              }  
                              }
                        else if (tag == "master") {
                              startMasterEditor();
                              toplevels.back().cobject()->readStatus(xml);
                              }
                        else if (tag == "lmaster") {
                              startLMasterEditor();
                              toplevels.back().cobject()->readStatus(xml);
                              }
                        else if (tag == "marker") {
                              showMarker(true);
                              toplevels.back().cobject()->readStatus(xml);
                              }
                        else if (tag == "waveedit") {
                              if(!pl->empty())
                              {
                                startWaveEditor(pl);
                                toplevels.back().cobject()->readStatus(xml);
                                pl = new PartList;
                              }  
                              }
                        else if (tag == "cliplist") {
                              startClipList();
                              toplevels.back().cobject()->readStatus(xml);
                              }
                        else
                              xml.unknown("MusE");
                        break;
                  case Xml::Attribut:
                        break;
                  case Xml::TagEnd:
                        if (tag == "toplevels") {
                              delete pl;
                              return;
                              }
                  default:
                        break;
                  }
            }
      }

//---------------------------------------------------------
//   readCtrl
//---------------------------------------------------------

void MusE::readCtrl(Xml&, int /*prt*/, int /*channel*/)
      {
#if 0
      ChannelState* iState = midiPorts[prt].iState(channel);

      int idx = 0;
      int val = -1;

      for (;;) {
            Xml::Token token = xml.parse();
            switch (token) {
                  case Xml::Error:
                  case Xml::End:
                        return;
                  case Xml::TagStart:
                        xml.unknown("readCtrl");
                        break;
                  case Xml::Attribut:
                        if (xml.s1() == "idx")
                              idx = xml.s2().toInt();
                        else if (xml.s1() == "val")
                              val = xml.s2().toInt();
                        break;
                  case Xml::TagEnd:
                        if (xml.s1() == "ctrl") {
                              iState->controller[idx] = val;
// printf("%d %d ctrl %d val %d\n", prt, channel, idx, val);
                              return;
                              }
                  default:
                        break;
                  }
            }
#endif
      }

//---------------------------------------------------------
//   readMidichannel
//---------------------------------------------------------

void MusE::readMidichannel(Xml& xml, int prt)
      {
      int channel = 0;
//      MidiPort* port = &midiPorts[prt];

      for (;;) {
            Xml::Token token = xml.parse();
            const QString& tag = xml.s1();
            switch (token) {
                  case Xml::Error:
                  case Xml::End:
                        return;
                  case Xml::TagStart:
                        if (tag == "pitch") {
//TODO                              port->setCtrl(channel, 0, CTRL_PITCH, xml.parseInt());
                              }
                        else if (tag == "program") {
//TODO                              port->setCtrl(channel, 0, CTRL_PROGRAM, xml.parseInt());
                              }
                        else if (tag == "ctrl")
                              readCtrl(xml, prt, channel);
                        else {
                              xml.unknown("readMidichannel");
                              }
                        break;
                  case Xml::Attribut:
                        if (tag == "ch") {
                              channel = xml.s2().toInt();
                              }
                        break;
                  case Xml::TagEnd:
                        if (tag == "midichannel")
                              return;
                  default:
                        break;
                  }
            }
      }

//---------------------------------------------------------
//   readMidiport
//---------------------------------------------------------

void MusE::readMidiport(Xml& xml)
      {
      int port = 0;
      for (;;) {
            Xml::Token token = xml.parse();
            const QString& tag = xml.s1();
            switch (token) {
                  case Xml::Error:
                  case Xml::End:
                        return;
                  case Xml::TagStart:
                        if (tag == "midichannel")
                              readMidichannel(xml, port);
                        else {
                              xml.unknown("readMidiport");
                              }
                        break;
                  case Xml::Attribut:
                        if (tag == "port") {
                              port = xml.s2().toInt();
                              }
                        break;
                  case Xml::TagEnd:
                        if (tag == "midiport") {
                              return;
                              }
                  default:
                        break;
                  }
            }
      }

//---------------------------------------------------------
//   readMarker
//---------------------------------------------------------

void Song::readMarker(Xml& xml)
      {
      Marker m;
      m.read(xml);
      _markerList->add(m);
      }

//---------------------------------------------------------
//   read
//---------------------------------------------------------

void Song::read(Xml& xml)
      {
      cloneList.clear();
      for (;;) {
            Xml::Token token;
            token = xml.parse();
            const QString& tag = xml.s1();
            switch (token) {
                  case Xml::Error:
                  case Xml::End:
                        return;
                  case Xml::TagStart:
                        if (tag == "master")
                              setMasterFlag(xml.parseInt());
                        else if (tag == "info")
                              songInfoStr = xml.parse1();
                        else if (tag == "loop")
                              setLoop(xml.parseInt());
                        else if (tag == "punchin")
                              setPunchin(xml.parseInt());
                        else if (tag == "punchout")
                              setPunchout(xml.parseInt());
                        else if (tag == "record")
                              setRecord(xml.parseInt());
                        else if (tag == "solo")
                              soloFlag = xml.parseInt();
                        else if (tag == "type")
                              _mtype  = MType(xml.parseInt());
                        else if (tag == "recmode")
                              _recMode  = xml.parseInt();
                        else if (tag == "cycle")
                              _cycleMode  = xml.parseInt();
                        else if (tag == "click")
                              setClick(xml.parseInt());
                        else if (tag == "quantize")
                              _quantize  = xml.parseInt();
                        else if (tag == "len")
                              _len  = xml.parseInt();
                        else if (tag == "follow")
                              _follow  = FollowMode(xml.parseInt());
                        else if (tag == "tempolist") {
                              tempomap.read(xml);
                              }
                        else if (tag == "siglist")
                              sigmap.read(xml);
                        else if (tag == "miditrack") {
                              MidiTrack* track = new MidiTrack();
                              track->read(xml);
                              insertTrack0(track, -1);
                              }
                        else if (tag == "drumtrack") {
                              MidiTrack* track = new MidiTrack();
                              track->setType(Track::DRUM);
                              track->read(xml);
                              insertTrack0(track, -1);
                              }
                        else if (tag == "wavetrack") {
                              WaveTrack* track = new WaveTrack();
                              track->read(xml);
                              insertTrack0(track,-1);
                              // Now that the track has been added to the lists in insertTrack2(), 
                              //  OSC can find the track and its plugins, and start their native guis if required...
                              track->showPendingPluginNativeGuis();
                              }
                        else if (tag == "AudioInput") {
                              AudioInput* track = new AudioInput();
                              track->read(xml);
                              insertTrack0(track,-1);
                              track->showPendingPluginNativeGuis();
                              }
                        else if (tag == "AudioOutput") {
                              AudioOutput* track = new AudioOutput();
                              track->read(xml);
                              insertTrack0(track,-1);
                              track->showPendingPluginNativeGuis();
                              }
                        else if (tag == "AudioGroup") {
                              AudioGroup* track = new AudioGroup();
                              track->read(xml);
                              insertTrack0(track,-1);
                              track->showPendingPluginNativeGuis();
                              }
                        else if (tag == "AudioAux") {
                              AudioAux* track = new AudioAux();
                              track->read(xml);
                              insertTrack0(track,-1);
                              track->showPendingPluginNativeGuis();
                              }
                        else if (tag == "SynthI") {
                              SynthI* track = new SynthI();
                              track->read(xml);
                              // Done in SynthI::read()
                              // insertTrack(track,-1);
                              //track->showPendingPluginNativeGuis();
                              }
                        else if (tag == "Route") {
                              readRoute(xml);
                              }
                        else if (tag == "marker")
                              readMarker(xml);
                        else if (tag == "globalPitchShift")
                              _globalPitchShift = xml.parseInt();
                        else if (tag == "automation")
                              automation = xml.parseInt();
                        else if (tag == "cpos") {
                              int pos = xml.parseInt();
                              Pos p(pos, true);
                              setPos(Song::CPOS, p, false, false, false);
                              }
                        else if (tag == "lpos") {
                              int pos = xml.parseInt();
                              Pos p(pos, true);
                              setPos(Song::LPOS, p, false, false, false);
                              }
                        else if (tag == "rpos") {
                              int pos = xml.parseInt();
                              Pos p(pos, true);
                              setPos(Song::RPOS, p, false, false, false);
                              }
                        else if (tag == "drummap")
                              readDrumMap(xml, false);
                        else
                              xml.unknown("Song");
                        break;
                  case Xml::Attribut:
                        break;
                  case Xml::TagEnd:
                        if (tag == "song") {
                              return;
                              }
                  default:
                        break;
                  }
            }
      dirty = false;
      
      // Since cloneList is also used for copy/paste operations, 
      //  clear the copy clone list again.
      cloneList.clear();
      }

//---------------------------------------------------------
//   read
//    read song
//---------------------------------------------------------

void MusE::read(Xml& xml, bool skipConfig)
      {
      bool skipmode = true;
      for (;;) {
            Xml::Token token = xml.parse();
            const QString& tag = xml.s1();
            switch (token) {
                  case Xml::Error:
                  case Xml::End:
                        return;
                  case Xml::TagStart:
                        if (skipmode && tag == "muse")
                              skipmode = false;
                        else if (skipmode)
                              break;
                        else if (tag == "configuration")
                              if (skipConfig)
                                    //xml.skip(tag);
                                    readConfiguration(xml,true /* only read sequencer settings */);
                              else
                                    readConfiguration(xml, false);
                        else if (tag == "song")
                        {
                              song->read(xml);
                              audio->msgUpdateSoloStates();
                        }      
                        else if (tag == "midiport")
                              readMidiport(xml);
                        else if (tag == "Controller") {  // obsolete
                              MidiController* ctrl = new MidiController;
                              ctrl->read(xml);
                              delete ctrl;
                              }
                        else if (tag == "mplugin")
                              readStatusMidiInputTransformPlugin(xml);
                        else if (tag == "toplevels")
                              readToplevels(xml);
                        else
                              xml.unknown("muse");
                        break;
                  case Xml::Attribut:
                        if (tag == "version") {
                              int major = xml.s2().section('.', 0, 0).toInt();
                              int minor = xml.s2().section('.', 1, 1).toInt();
                              xml.setVersion(major, minor);
                              }
                        break;
                  case Xml::TagEnd:
                        if (!skipmode && tag == "muse")
                              return;
                  default:
                        break;
                  }
            }
      }

//---------------------------------------------------------
//   write
//---------------------------------------------------------

void Song::write(int level, Xml& xml) const
      {
      xml.tag(level++, "song");
      xml.strTag(level, "info", songInfoStr);
      xml.intTag(level, "automation", automation);
      xml.intTag(level, "cpos", song->cpos());
      xml.intTag(level, "rpos", song->rpos());
      xml.intTag(level, "lpos", song->lpos());
      xml.intTag(level, "master", _masterFlag);
      xml.intTag(level, "loop", loopFlag);
      xml.intTag(level, "punchin", punchinFlag);
      xml.intTag(level, "punchout", punchoutFlag);
      xml.intTag(level, "record", recordFlag);
      xml.intTag(level, "solo", soloFlag);
      xml.intTag(level, "type", _mtype);
      xml.intTag(level, "recmode", _recMode);
      xml.intTag(level, "cycle", _cycleMode);
      xml.intTag(level, "click", _click);
      xml.intTag(level, "quantize", _quantize);
      xml.intTag(level, "len", _len);
      xml.intTag(level, "follow", _follow);
      if (_globalPitchShift)
            xml.intTag(level, "globalPitchShift", _globalPitchShift);

      // Make a backup of the current clone list, to retain any 'copy' items,
      //  so that pasting works properly after.
      CloneList copyCloneList = cloneList;
      cloneList.clear();

      // write tracks
      for (ciTrack i = _tracks.begin(); i != _tracks.end(); ++i)
            (*i)->write(level, xml);

      // write routing
      for (ciTrack i = _tracks.begin(); i != _tracks.end(); ++i) {
            
            // p3.3.38 Changed
            //if ((*i)->isMidiTrack())
            //      continue;
            //WaveTrack* track = (WaveTrack*)(*i);
            //track->writeRouting(level, xml);
            
            (*i)->writeRouting(level, xml);
            }

      // Write Jack midi routing.
      for (iMidiDevice i = midiDevices.begin(); i != midiDevices.end(); ++i) {
            //MidiJackDevice* mjd = dynamic_cast<MidiJackDevice*>(*i);
            //if (!mjd)
            //  continue;
            //mjd->writeRouting(level, xml);
            (*i)->writeRouting(level, xml);
            }
      
      tempomap.write(level, xml);
      sigmap.write(level, xml);
      _markerList->write(level, xml);

      writeDrumMap(level, xml, false);
      xml.tag(level, "/song");
      
      // Restore backup of the clone list, to retain any 'copy' items,
      //  so that pasting works properly after.
      cloneList.clear();
      cloneList = copyCloneList;
      }

//---------------------------------------------------------
//   write
//    write song
//---------------------------------------------------------

void MusE::write(Xml& xml) const
      {
      xml.header();

      int level = 0;
      xml.tag(level++, "muse version=\"2.0\"");
      writeConfiguration(level, xml);

      writeStatusMidiInputTransformPlugins(level, xml);

      song->write(level, xml);

      if (!toplevels.empty()) {
            xml.tag(level++, "toplevels");
            for (ciToplevel i = toplevels.begin(); i != toplevels.end(); ++i) {
                  if (i->cobject()->isVisible())
                        i->cobject()->writeStatus(level, xml);
                  }
            xml.tag(level--, "/toplevels");
            }

      xml.tag(level, "/muse");
      }

