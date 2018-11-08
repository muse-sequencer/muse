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

#include <QUuid>
#include <QProgressDialog>
#include <QMessageBox>
#include <QCheckBox>
#include <QString>

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
//   Part::readFromXml
//---------------------------------------------------------

Part* Part::readFromXml(Xml& xml, Track* track, bool doClone, bool toTrack)
      {
      int id = -1;
      Part* npart = 0;
      QUuid uuid;
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
                              if(i->is_deleted) // Is the clone item marked as deleted? Ignore it.
                                continue;
                              if(i->id == id) // Is a matching part found in the clone list?
                              {
                                // Create a clone. It must still be added later in a operationgroup
                                npart = track->newPart((Part*)i->cp, true);
                                break;
                              }
                            }
                          }
                          else if(uuidvalid) // If a uuid was found...
                          {
                            for(iClone i = MusEGlobal::cloneList.begin(); i != MusEGlobal::cloneList.end(); ++i)
                            {
                              if(uuid == i->_uuid) // Is a matching part found in the clone list?
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

                                if(i->is_deleted) // Is the clone item marked as deleted? Don't create a clone, create a copy.
                                  break;

                                // If it's a regular paste (not paste clone), and the original part is
                                //  not a clone, defer so that a new copy is created in TagStart above.
                                if(!doClone && !isclone)
                                  break;

                                // Create a clone. It must still be added later in a operationgroup
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
                              ncp._uuid = uuid; // OK for non-windows?
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

                                if(tick < 0)
                                {
                                  printf("readClone: warning: event at tick:%d not in part:%s, discarded\n",
                                    tick, npart->name().toLatin1().constData());
                                }
                                else
                                {
                                  npart->addEvent(e);
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
                        }
                        else if (tag == "uuid")
                        {
                          uuid = QUuid(xml.s2());
                          if(!uuid.isNull())
                          {
                            uuidvalid = true;
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
      int id              = -1;
      QUuid uuid;
      bool dumpEvents     = true;
      bool wave = _track->type() == Track::WAVE;

      // NOTE ::write() should never be called on a deleted part, so no checking here for cloneList items marked as deleted.
      //      Checking is awkward anyway and doesn't fit well here.

      if(isCopy)
      {
        for(iClone i = MusEGlobal::cloneList.begin(); i != MusEGlobal::cloneList.end(); ++i)
        {
          if(i->cp->isCloneOf(this))
          {
            uuid = i->_uuid;
            dumpEvents = false;
            break;
          }
        }
        if(uuid.isNull())
        {
          ClonePart cp(this);
          uuid = cp._uuid;
          MusEGlobal::cloneList.push_back(cp);
        }
      }
      else
      {
        if (this->hasClones())
        {
          for (iClone i = MusEGlobal::cloneList.begin(); i != MusEGlobal::cloneList.end(); ++i)
          {
            if (i->cp->isCloneOf(this))
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
        if(wave)
          xml.nput(level, "<part type=\"wave\" uuid=\"%s\"", uuid.toByteArray().constData());
        else
          xml.nput(level, "<part uuid=\"%s\"", uuid.toByteArray().constData());

        if(hasClones())
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
            for (ciEvent e = events().begin(); e != events().end(); ++e)
                  e->second.write(level, xml, *this, forceWavePaths);
            }
      xml.etag(level, "part");
      }


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
         if (MusEGlobal::muse->progress) {
            MusEGlobal::muse->progress->setValue(MusEGlobal::muse->progress->value()+1);
         }

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
                        else if (tag == "type")          // Obsolete.
                              xml.parseInt();
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
                              if (!isTemplate && MusEGlobal::audioDevice->deviceType() != AudioDevice::DUMMY_AUDIO && sRate != MusEGlobal::sampleRate) {

                                if (MusEGlobal::audioDevice->deviceType() == AudioDevice::RTAUDIO_AUDIO) {
                                  // restart audio with selected sample rate
                                  MusEGlobal::sampleRate = sRate;
                                  MusEGlobal::audioDevice->stop();
                                  MusEGlobal::audioDevice->start(0);
                                  if (MusEGlobal::sampleRate == sRate) {
                                    QMessageBox::warning(MusEGlobal::muse,"Sample rate", "Changing sample rate to song setting " + QString::number(sRate) + "!");
                                  } else {
                                    QMessageBox::warning(MusEGlobal::muse,"Sample rate", "Tried changing sample rate to song setting " +
                                                         QString::number(sRate) + " but driver set it to " + QString::number(MusEGlobal::sampleRate) + "!");
                                  }
                                } else {
                                  QMessageBox::warning(MusEGlobal::muse,"Wrong sample rate", "The sample rate in this project and the current system setting differs, the project may not work as intended!");
                                }
                              }
                            }
                        else if (tag == "tempolist") {
                              MusEGlobal::tempomap.read(xml);
                              }
                        else if (tag == "siglist")
                              ///MusEGlobal::sigmap.read(xml);
                              MusEGlobal::sigmap.read(xml);
                        else if (tag == "keylist") {
                              MusEGlobal::keymap.read(xml);
                              }
                        else if (tag == "miditrack") {
                              MidiTrack* track = new MidiTrack();
                              track->read(xml);
                              insertTrack0(track, -1);
                              }
                        else if (tag == "drumtrack") { // Old drumtrack is obsolete.
                              MidiTrack* track = new MidiTrack();
                              track->setType(Track::NEW_DRUM);
                              track->read(xml);
                              track->convertToType(Track::NEW_DRUM); // Convert the notes and controllers.
                              insertTrack0(track, -1);
                              }
                        else if (tag == "newdrumtrack") {
                              MidiTrack* track = new MidiTrack();
                              track->setType(Track::NEW_DRUM);
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
                        else if (tag == "drum_ordering")
                              MusEGlobal::global_drum_ordering.read(xml);
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
      for (int i = 0; i < MusECore::MIDI_PORTS; ++i)
            MusEGlobal::midiPorts[i].writeRouting(level, xml);

      MusEGlobal::tempomap.write(level, xml);
      MusEGlobal::sigmap.write(level, xml);
      MusEGlobal::keymap.write(level, xml);
      _markerList->write(level, xml);

      writeDrumMap(level, xml, false);
      MusEGlobal::global_drum_ordering.write(level, xml);
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
                        MusECore::Track* track = NULL;
                        //check if track index is in bounds before getting it (danvd)
                        if(trackIdx < (int)MusEGlobal::song->tracks()->size())
                        {
                            track = MusEGlobal::song->tracks()->index(trackIdx);
                        }
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
                              // Inform the rest of the app that the song (may) have changed, using these flags.
                              // After this function is called, the caller can do a general Song::update() MINUS these flags,
                              //  like in MusE::loadProjectFile1() - the only place calling so far, as of this writing.
                              // Some existing windows need this, like arranger, some don't which are dynamically created after this.
                              MusEGlobal::song->update(SC_TRACK_INSERTED);
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
                        if(!xml.isVersionEqualToLatest())
                        {
                          fprintf(stderr, "\n***WARNING***\nLoaded file version is %d.%d\nCurrent version is %d.%d\n"
                                  "Conversions may be applied if file is saved!\n\n",
                                  xml.majorVersion(), xml.minorVersion(),
                                  xml.latestMajorVersion(), xml.latestMinorVersion());
                          // Cannot construct QWidgets until QApplication created!
                          // Check MusEGlobal::muse which is created shortly after the application...
                          if(MusEGlobal::muse && MusEGlobal::config.warnOnFileVersions)
                          {
                            QString txt = tr("File version is %1.%2\nCurrent version is %3.%4\n"
                                             "Conversions may be applied if file is saved!")
                                            .arg(xml.majorVersion()).arg(xml.minorVersion())
                                            .arg(xml.latestMajorVersion()).arg(xml.latestMinorVersion());
                            QMessageBox* mb = new QMessageBox(QMessageBox::Warning,
                                                              tr("Opening file"),
                                                              txt,
                                                              QMessageBox::Ok, MusEGlobal::muse);
                            QCheckBox* cb = new QCheckBox(tr("Do not warn again"));
                            cb->setChecked(!MusEGlobal::config.warnOnFileVersions);
                            mb->setCheckBox(cb);
                            mb->exec();
                            if(!mb->checkBox()->isChecked() != MusEGlobal::config.warnOnFileVersions)
                            {
                              MusEGlobal::config.warnOnFileVersions = !mb->checkBox()->isChecked();
                              // Save settings. Use simple version - do NOT set style or stylesheet, this has nothing to do with that.
                              //MusEGlobal::muse->changeConfig(true);  // Save settings? No, wait till close.
                            }
                            delete mb;
                          }
                        }
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
      xml.nput(level++, "<muse version=\"%d.%d\">\n", xml.latestMajorVersion(), xml.latestMinorVersion());

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

