//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: songfile.cpp,v 1.25.2.12 2009/11/04 15:06:07 spamatica Exp $
//
//  (C) Copyright 1999/2000 Werner Schweer (ws@seh.de)
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

#include <uuid/uuid.h>
#include <QProgressDialog>
#include <QMessageBox>

#include "app.h"
#include "song.h"
#include "arranger.h"
#include "arrangerview.h"
#include "cobject.h"
#include "drumedit.h"
#include "pianoroll.h"
#include "scoreedit.h"
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
#include "audiodev.h"
#include "conf.h"
#include "driver/jackmidi.h"
#include "keyevent.h"

namespace MusEGlobal {
MusECore::CloneList cloneList;
}

namespace MusECore {


/* DELETETHIS 42
//---------------------------------------------------------
//   updateCloneList
//---------------------------------------------------------

void updateCloneList(Part* oPart, Part* nPart)
{
  for(iClone i = MusEGlobal::cloneList.begin(); i != MusEGlobal::cloneList.end(); ++i) 
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
    for(iClone i = MusEGlobal::cloneList.begin(); i != MusEGlobal::cloneList.end(); ++i) 
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
  MusEGlobal::cloneList.clear();
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
                        if(!npart) // If the part has not been created yet...
                        {
                          if(id != -1) // If an id was found...
                          {
                            for(iClone i = MusEGlobal::cloneList.begin(); i != MusEGlobal::cloneList.end(); ++i) 
                            {
                              if(i->id == id) // Is a matching part found in the clone list?
                              {
                                // This makes a clone, chains the part, and increases ref counts.
                                npart = track->newPart((Part*)i->cp, true);
                                break;
                              }
                            }
                          }  
                          else if(uuidvalid) // If a uuid was found...
                          {
                            for(iClone i = MusEGlobal::cloneList.begin(); i != MusEGlobal::cloneList.end(); ++i) 
                            {
                              if(uuid_compare(uuid, i->uuid) == 0) // Is a matching part found in the clone list?
                              {
                                Track* cpt = i->cp->track();
                                if(toTrack) // If we want to paste to the given track...
                                {
                                  // If the given track type is not the same as the part's 
                                  //  original track type, we can't continue. Just return.
                                  if(!track || cpt->type() != track->type())
                                  {
                                    xml.skip("part");
                                    return 0;
                                  }  
                                }
                                else // ...else we want to paste to the part's original track.
                                {
                                  // Make sure the track exists (has not been deleted).
                                  if((cpt->isMidiTrack() && MusEGlobal::song->midis()->find(cpt) != MusEGlobal::song->midis()->end()) || 
                                      (cpt->type() == Track::WAVE && MusEGlobal::song->waves()->find(cpt) != MusEGlobal::song->waves()->end()))
                                    track = cpt;   
                                  else // Track was not found. Try pasting to the given track, as above...
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
                                if(!doClone && !isclone)
                                  break;
                                  
                                // This makes a clone, chains the part, and increases ref counts.
                                npart = track->newPart((Part*)i->cp, true);
                                break;
                              }
                            }  
                          }
                        
                          if(!npart) // If the part still has not been created yet...
                          {
                            
                            if(!track) // A clone was not created from any matching
                            {          // part. Create a non-clone part now.
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
                              npart = new MusECore::WavePart((MusECore::WaveTrack*)track);
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
                              MusEGlobal::cloneList.push_back(ncp);
                            }
                            else  
                            if(uuidvalid)
                            {
                              ClonePart ncp(npart);
                              // New ClonePart creates its own uuid, but we need to replace it.
                              uuid_copy(ncp.uuid, uuid);
                              MusEGlobal::cloneList.push_back(ncp);
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
                                
                                // DELETETHIS 7
                                // Do not discard events belonging to clone parts,
                                //  at least not yet. A later clone might have a longer, 
                                //  fully accommodating part length!
                                //if ((tick < 0) || (tick >= (int) lenTick())) {
                                //if ((tick < 0) || ( id == -1 && !clone && (tick >= (int)lenTick()) )) 
                                // No way to tell at the moment whether there will be clones referencing this...
                                // No choice but to accept all events past 0.
                                if(tick < 0) 
                                {
                                  printf("readClone: warning: event at tick:%d not in part:%s, discarded\n",
                                    tick, npart->name().toLatin1().constData());
                                }
                                else 
                                {
                                  npart->events()->add(e);
                                }      
                              }
                              else // ...Otherwise a clone was created, so we don't need the events.
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
                          //if(id != -1) DELETETHIS 19
                          //{
                          //  for(iClone i = MusEGlobal::cloneList.begin(); i != MusEGlobal::cloneList.end(); ++i) 
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
                          uuid_parse(xml.s2().toLatin1().constData(), uuid);
                          if(!uuid_is_null(uuid))
                          {
                            uuidvalid = true;
                            /* DELETETHIS 50
                            for(iClone i = MusEGlobal::cloneList.begin(); i != MusEGlobal::cloneList.end(); ++i) 
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
                                  if((cpt->isMidiTrack() && MusEGlobal::song->midis()->find(cpt) != MusEGlobal::song->midis()->end()) || 
                                     (cpt->type() == Track::WAVE && MusEGlobal::song->waves()->find(cpt) != MusEGlobal::song->waves()->end()))
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
        for(iClone i = MusEGlobal::cloneList.begin(); i != MusEGlobal::cloneList.end(); ++i) 
        {
          if(i->cp->cevents() == el) 
          {
            uuid_copy(uuid, i->uuid);
            dumpEvents = false;
            break;
          }
        }
        if(uuid_is_null(uuid)) 
        {
          ClonePart cp(this);
          uuid_copy(uuid, cp.uuid);
          MusEGlobal::cloneList.push_back(cp);
        }
      }  
      else
      {
        if (el->arefCount() > 1) 
        {
          for (iClone i = MusEGlobal::cloneList.begin(); i != MusEGlobal::cloneList.end(); ++i) 
          {
            if (i->cp->cevents() == el) 
            {
              id = i->id;
              dumpEvents = false;
              break;
            }
          }
          if (id == -1) 
          {
            id = MusEGlobal::cloneList.size();
            ClonePart cp(this, id);
            MusEGlobal::cloneList.push_back(cp);
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
      {
        xml.tag(level++, "part cloneId=\"%d\"", id);
      }      
      else
        xml.tag(level++, "part");
      
      xml.strTag(level, "name", _name);

      PosLen::write(level, xml, "poslen");
      xml.intTag(level, "selected", _selected);
      xml.intTag(level, "color", _colorIndex);
      if (_mute)
            xml.intTag(level, "mute", _mute);
      if (dumpEvents) {
            for (ciEvent e = el->begin(); e != el->end(); ++e)
                  e->second.write(level, xml, *this, forceWavePaths);
            }
      xml.etag(level, "part");
      }

// DELETETHIS 280! whoa!
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
                                       tick, name().toLatin1().constData());
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
                                          MidiPort* mp = &MusEGlobal::midiPorts[mt->outPort()];
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
                                              mp = &MusEGlobal::midiPorts[drumMap[note].port];
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
                                uuid_parse(xml.s2().toLatin1().constData(), uuid);
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
                                          // add to MusEGlobal::cloneList:
                                          //ClonePart cp(_events, id);
                                          ClonePart cp(this, id);
                                          MusEGlobal::cloneList.push_back(cp);
                                          }
                                    else {
                                          // replace event list with clone event
                                          // list
                                          for (iClone i = MusEGlobal::cloneList.begin();
                                             i != MusEGlobal::cloneList.end(); ++i) {
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
                            for(iClone i = MusEGlobal::cloneList.begin(); i != MusEGlobal::cloneList.end(); ++i) 
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
                                MusEGlobal::cloneList.push_back(ncp);
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
                            for(iClone i = MusEGlobal::cloneList.begin(); i != MusEGlobal::cloneList.end(); ++i) 
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
                              if((cp->track()->isMidiTrack() && MusEGlobal::song->midis()->find(cp->track()) != MusEGlobal::song->midis()->end()) || 
                                 (cp->track()->type() == Track::WAVE && MusEGlobal::song->waves()->find(cp->track()) != MusEGlobal::song->waves()->end()))
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
                                MusEGlobal::cloneList.push_back(ncp);
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
         name, Xml::xmlString(font.family()).toLatin1().constData(), font.pointSize());
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

void Song::read(Xml& xml, bool isTemplate)
      {
      MusEGlobal::cloneList.clear();
      for (;;) {
         if (MusEGlobal::muse->progress)
            MusEGlobal::muse->progress->setValue(MusEGlobal::muse->progress->value()+1);

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
                        else if (tag == "showinfo")
                              showSongInfo = xml.parseInt();
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
                        else if (tag == "sampleRate") {
                              int sRate  = xml.parseInt();
                              if (!isTemplate && MusEGlobal::audioDevice->deviceType() != AudioDevice::DUMMY_AUDIO && sRate != MusEGlobal::sampleRate)
                                QMessageBox::warning(MusEGlobal::muse,"Wrong sample rate", "The sample rate in this project and the current system setting differs, the project may not work as intended!");
                            }
                        else if (tag == "tempolist") {
                              MusEGlobal::tempomap.read(xml);
                              }
                        else if (tag == "siglist")
                              ///sigmap.read(xml);
                              AL::sigmap.read(xml);
                        else if (tag == "keylist") {
                              MusEGlobal::keymap.read(xml);
                              }
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
                              MusECore::WaveTrack* track = new MusECore::WaveTrack();
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
                              MusEGlobal::automation = xml.parseInt();
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
      MusEGlobal::cloneList.clear();
      }

//---------------------------------------------------------
//   write
//---------------------------------------------------------

void Song::write(int level, Xml& xml) const
      {
      xml.tag(level++, "song");
      xml.strTag(level, "info", songInfoStr);
      xml.intTag(level, "showinfo", showSongInfo);
      xml.intTag(level, "automation", MusEGlobal::automation);
      xml.intTag(level, "cpos", MusEGlobal::song->cpos());
      xml.intTag(level, "rpos", MusEGlobal::song->rpos());
      xml.intTag(level, "lpos", MusEGlobal::song->lpos());
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
      xml.intTag(level, "sampleRate", MusEGlobal::sampleRate);
      if (_globalPitchShift)
            xml.intTag(level, "globalPitchShift", _globalPitchShift);

      // Make a backup of the current clone list, to retain any 'copy' items,
      //  so that pasting works properly after.
      CloneList copyCloneList = MusEGlobal::cloneList;
      MusEGlobal::cloneList.clear();

      // write tracks
      for (ciTrack i = _tracks.begin(); i != _tracks.end(); ++i)
            (*i)->write(level, xml);

      // write routing
      for (ciTrack i = _tracks.begin(); i != _tracks.end(); ++i)
            (*i)->writeRouting(level, xml);

      // Write midi device routing.
      for (iMidiDevice i = MusEGlobal::midiDevices.begin(); i != MusEGlobal::midiDevices.end(); ++i)
            (*i)->writeRouting(level, xml);
      
      // Write midi port routing.
      for (int i = 0; i < MIDI_PORTS; ++i)
            MusEGlobal::midiPorts[i].writeRouting(level, xml);
      
      MusEGlobal::tempomap.write(level, xml);
      AL::sigmap.write(level, xml);
      MusEGlobal::keymap.write(level, xml);
      _markerList->write(level, xml);

      writeDrumMap(level, xml, false);
      xml.tag(level, "/song");
      
      // Restore backup of the clone list, to retain any 'copy' items,
      //  so that pasting works properly after.
      MusEGlobal::cloneList.clear();
      MusEGlobal::cloneList = copyCloneList;
      }


} // namespace MusECore

namespace MusEGui {

//---------------------------------------------------------
//   readPart
//---------------------------------------------------------

MusECore::Part* MusE::readPart(MusECore::Xml& xml)
      {
      MusECore::Part* part = 0;
      for (;;) {
            MusECore::Xml::Token token = xml.parse();
            const QString& tag = xml.s1();
            switch (token) {
                  case MusECore::Xml::Error:
                  case MusECore::Xml::End:
                        return part;
                  case MusECore::Xml::Text:
                        {
                        int trackIdx, partIdx;
                        sscanf(tag.toLatin1().constData(), "%d:%d", &trackIdx, &partIdx);
                        MusECore::Track* track = MusEGlobal::song->tracks()->index(trackIdx);
                        if (track)
                              part = track->parts()->find(partIdx);
                        }
                        break;
                  case MusECore::Xml::TagStart:
                        xml.unknown("readPart");
                        break;
                  case MusECore::Xml::TagEnd:
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

void MusE::readToplevels(MusECore::Xml& xml)
      {
      MusECore::PartList* pl = new MusECore::PartList;
      for (;;) {
            MusECore::Xml::Token token = xml.parse();
            const QString& tag = xml.s1();
            switch (token) {
                  case MusECore::Xml::Error:
                  case MusECore::Xml::End:
                        return;
                  case MusECore::Xml::TagStart:
                        if (tag == "part") {
                              MusECore::Part* part = readPart(xml);
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
                                toplevels.back()->readStatus(xml);
                                pl = new MusECore::PartList;
                              }  
                              }
                        else if (tag == "scoreedit") {
                                MusEGui::ScoreEdit* score = new MusEGui::ScoreEdit(this, 0, _arranger->cursorValue());
                                toplevels.push_back(score);
                                connect(score, SIGNAL(isDeleting(MusEGui::TopWin*)), SLOT(toplevelDeleting(MusEGui::TopWin*)));
                                connect(score, SIGNAL(name_changed()), arrangerView, SLOT(scoreNamingChanged()));
                                score->show();
                                score->readStatus(xml);
                              }
                        else if (tag == "drumedit") {
                              if(!pl->empty())
                              {
                                startDrumEditor(pl);
                                toplevels.back()->readStatus(xml);
                                pl = new MusECore::PartList;
                              }  
                              }
                        else if (tag == "listeditor") {
                              if(!pl->empty())
                              {
                                startListEditor(pl);
                                toplevels.back()->readStatus(xml);
                                pl = new MusECore::PartList;
                              }  
                              }
                        else if (tag == "master") {
                              startMasterEditor();
                              toplevels.back()->readStatus(xml);
                              }
                        else if (tag == "lmaster") {
                              startLMasterEditor();
                              toplevels.back()->readStatus(xml);
                              }
                        else if (tag == "marker") {
                              showMarker(true);
                              TopWin* tw = toplevels.findType(TopWin::MARKER);
                              if(!tw)
                                xml.skip("marker");
                              else
                                tw->readStatus(xml);
                              }
                        else if (tag == "arrangerview") {
                              showArranger(true);
                              TopWin* tw = toplevels.findType(TopWin::ARRANGER);
                              if(!tw)
                                xml.skip("arrangerview");
                              else
                                tw->readStatus(xml);
                              }
                        else if (tag == "waveedit") {
                              if(!pl->empty())
                              {
                                startWaveEditor(pl);
                                toplevels.back()->readStatus(xml);
                                pl = new MusECore::PartList;
                              }  
                              }
                        else if (tag == "cliplist") {
                              startClipList(true);
                              TopWin* tw = toplevels.findType(TopWin::CLIPLIST);
                              if(!tw)
                                xml.skip("cliplist");
                              else
                                tw->readStatus(xml);
                              }
                        else
                              xml.unknown("MusE");
                        break;
                  case MusECore::Xml::Attribut:
                        break;
                  case MusECore::Xml::TagEnd:
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

void MusE::readCtrl(MusECore::Xml&, int /*prt*/, int /*channel*/)
      {
#if 0 // DELETETHIS 30. delete the whole function?
      ChannelState* iState = MusEGlobal::midiPorts[prt].iState(channel);

      int idx = 0;
      int val = -1;

      for (;;) {
            MusECore::Xml::Token token = xml.parse();
            switch (token) {
                  case MusECore::Xml::Error:
                  case MusECore::Xml::End:
                        return;
                  case MusECore::Xml::TagStart:
                        xml.unknown("readCtrl");
                        break;
                  case MusECore::Xml::Attribut:
                        if (xml.s1() == "idx")
                              idx = xml.s2().toInt();
                        else if (xml.s1() == "val")
                              val = xml.s2().toInt();
                        break;
                  case MusECore::Xml::TagEnd:
                        if (xml.s1() == "ctrl") {
                              iState->controller[idx] = val;
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

void MusE::readMidichannel(MusECore::Xml& xml, int prt)
      {
      int channel = 0;

      for (;;) {
            MusECore::Xml::Token token = xml.parse();
            const QString& tag = xml.s1();
            switch (token) {
                  case MusECore::Xml::Error:
                  case MusECore::Xml::End:
                        return;
                  case MusECore::Xml::TagStart:
                        if (tag == "pitch") {
//TODO                              port->setCtrl(channel, 0, CTRL_PITCH, xml.parseInt()); DELETETHIS? and below
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
                  case MusECore::Xml::Attribut:
                        if (tag == "ch") {
                              channel = xml.s2().toInt();
                              }
                        break;
                  case MusECore::Xml::TagEnd:
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

void MusE::readMidiport(MusECore::Xml& xml)
      {
      int port = 0;
      for (;;) {
            MusECore::Xml::Token token = xml.parse();
            const QString& tag = xml.s1();
            switch (token) {
                  case MusECore::Xml::Error:
                  case MusECore::Xml::End:
                        return;
                  case MusECore::Xml::TagStart:
                        if (tag == "midichannel")
                              readMidichannel(xml, port);
                        else {
                              xml.unknown("readMidiport");
                              }
                        break;
                  case MusECore::Xml::Attribut:
                        if (tag == "port") {
                              port = xml.s2().toInt();
                              }
                        break;
                  case MusECore::Xml::TagEnd:
                        if (tag == "midiport") {
                              return;
                              }
                  default:
                        break;
                  }
            }
      }

//---------------------------------------------------------
//   read
//    read song
//---------------------------------------------------------

void MusE::read(MusECore::Xml& xml, bool doReadMidiPorts, bool isTemplate)
      {
      bool skipmode = true;
      
      writeTopwinState=true;
      
      for (;;) {
            if (progress)
                progress->setValue(progress->value()+1);
            MusECore::Xml::Token token = xml.parse();
            const QString& tag = xml.s1();
            switch (token) {
                  case MusECore::Xml::Error:
                  case MusECore::Xml::End:
                        return;
                  case MusECore::Xml::TagStart:
                        if (skipmode && tag == "muse")
                              skipmode = false;
                        else if (skipmode)
                              break;
                        else if (tag == "configuration")
                              readConfiguration(xml, doReadMidiPorts, false /* do NOT read global settings, see below */);
                        /* Explanation for why "do NOT read global settings":
                         * if you would use true here, then muse would overwrite certain global config stuff
                         * by the settings stored in the song. but you don't want this. imagine that you
                         * send a friend a .med file. your friend opens it and baaam, his configuration is
                         * garbled. why? well, because these readConfigurations here would have overwritten
                         * parts (but not all) of his global config (like MDI/SDI, toolbar states etc.)
                         * with the data stored in the song. (it IS stored there. dunny why, i find it pretty
                         * senseless.)
                         * 
                         * If you've a problem which seems to be solved by replacing "false" with "true", i've
                         * a better solution for you: go into conf.cpp, in void readConfiguration(Xml& xml, bool readOnlySequencer, bool doReadGlobalConfig)
                         * (around line 525), look for a comment like this:
                         * "Global and/or per-song config stuff ends here" (alternatively just search for
                         * "----"). Your problem is probably that some non-global setting should be loaded but
                         * is not. Fix it by either placing the else if (foo)... clause responsible for that
                         * setting to be loaded into the first part, that is, before "else if (!doReadGlobalConfig)"
                         * or (if the settings actually IS global and not per-song), ensure that the routine
                         * which writes the global (and not the song-)configuration really writes that setting.
                         * (it might happen that it formerly worked because it was written to the song instead
                         *  of the global config by mistake, and now isn't loaded anymore. write it to the
                         *  correct location.)
                         * 
                         *                                                                                -- flo93
                         */
                        else if (tag == "song")
                        {
                              MusEGlobal::song->read(xml, isTemplate);
                              MusEGlobal::audio->msgUpdateSoloStates();
                        }      
                        else if (tag == "midiport")
                              readMidiport(xml);
                        else if (tag == "Controller") {  // obsolete
                              MusECore::MidiController* ctrl = new MusECore::MidiController;
                              ctrl->read(xml);
                              delete ctrl;
                              }
                        else if (tag == "mplugin")
                              readStatusMidiInputTransformPlugin(xml);
                        else if (tag == "toplevels")
                              readToplevels(xml);
                        else if (tag == "no_toplevels")
                        {     
                              if (!isTemplate)
                                writeTopwinState=false;
                              
                              xml.skip("no_toplevels");
                        }
                              
                        else
                              xml.unknown("muse");
                        break;
                  case MusECore::Xml::Attribut:
                        if (tag == "version") {
                              int major = xml.s2().section('.', 0, 0).toInt();
                              int minor = xml.s2().section('.', 1, 1).toInt();
                              xml.setVersion(major, minor);
                              }
                        break;
                  case MusECore::Xml::TagEnd:
                        if (!skipmode && tag == "muse")
                              return;
                  default:
                        break;
                  }
            }
      }


//---------------------------------------------------------
//   write
//    write song
//---------------------------------------------------------

void MusE::write(MusECore::Xml& xml, bool writeTopwins) const
      {
      xml.header();

      int level = 0;
      xml.tag(level++, "muse version=\"2.0\"");
      writeConfiguration(level, xml);

      writeStatusMidiInputTransformPlugins(level, xml);

      MusEGlobal::song->write(level, xml);

      if (writeTopwins && !toplevels.empty()) {
            xml.tag(level++, "toplevels");
            for (MusEGui::ciToplevel i = toplevels.begin(); i != toplevels.end(); ++i) {
                  if ((*i)->isVisible())
                        (*i)->writeStatus(level, xml);
                  }
            xml.tag(level--, "/toplevels");
            }
      else if (!writeTopwins)
      {
            xml.tag(level, "no_toplevels");
            xml.etag(level, "no_toplevels");
      }

      xml.tag(level, "/muse");
      }

} // namespace MusEGui

