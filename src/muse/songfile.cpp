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

#include <map>
#include <set>

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
#include "waveedit.h"
#include "master/masteredit.h"
#include "scoreedit.h"
#include "globals.h"
#include "xml.h"
#include "drummap.h"
#include "drum_ordering.h"
#include "event.h"
#include "marker/marker.h"
#include "midiport.h"
#include "audio.h"
#include "mitplugin.h"
#include "wave.h"
#include "midictrl.h"
#include "audiodev.h"
#include "conf.h"
#include "keyevent.h"
#include "gconfig.h"
#include "config.h"
#include "missing_plugins.h"

// Forwards from header:
#include "xml_statistics.h"

// For debugging loading, saving, copy, paste, clone: Uncomment the fprintf section.
#define DEBUG_SONGFILE(dev, format, args...) // fprintf(dev, format, ##args);

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

Part* Part::readFromXml(Xml& xml, Track* track, XmlReadStatistics* stats, bool doClone, bool trackIsParent)
      {
      int cloneId = -1;
      QUuid cloneUuid;
      QUuid trackUuid;
      Track* origTrack = nullptr;
      Track* t = nullptr;
      Track* fin_track = nullptr;
      Part* npart = nullptr;
      Track::TrackType xmlTrackType = Track::MIDI;
      bool xmlTrackTypeValid = false;
      bool clone = true;
      bool xmlPartWasClone = false;
      bool wave = false;
      const TrackList* tl = MusEGlobal::song->tracks();
      const ciTrack itlend = tl->cend();

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
                          if(!trackUuid.isNull()) // If a track serial number was found...
                            origTrack = tl->findUuid(trackUuid);

                          // Track given? Use it.
                          if(track)
                            fin_track = track;
                          // Original track found? Use it.
                          else if(origTrack)
                            fin_track = origTrack;
                          // Else no track to paste to! Return.
                          else
                          {
                            xml.skip("part");
                            return nullptr;
                          }

                          bool uuidFoundInTracks = false;

                          DEBUG_SONGFILE(stderr, "Part::readFromXml: fin_track:%p %s stats:%p doClone:%d\n",
                            fin_track, fin_track->name().toLocal8Bit().constData(), stats, doClone);

                          if(!cloneUuid.isNull()) // If a clone uuid was found...
                          {
                            DEBUG_SONGFILE(stderr, " cloneUuid valid:%s\n",
                              cloneUuid.toString().toLocal8Bit().constData());

                            // The original track type and the destination track type must be the same.
                            if(!xmlTrackTypeValid || xmlTrackType != fin_track->type())
                            {
                              xml.skip("part");
                              return nullptr;
                            }

                            // Find a clone part in the stats list with the same clonemaster id.
                            if(stats)
                            {
                              Part* part = stats->findClonemasterPart(cloneUuid);
                              if(part)
                              {
                                npart = fin_track->newPart(part, true);

                                DEBUG_SONGFILE(stderr, " found in stats: part:%p npart:%p muuid:%s\n",
                                  part,
                                  npart, npart->clonemaster_uuid().toString().toLocal8Bit().constData());

                                break;
                              }
                            }

                            // Find any existing track clone part with the same clonemaster id.
                            for(ciTrack it = tl->cbegin(); it != itlend; ++it)
                            {
                              t = *it;

                              // The found track type and the destination track type must be the same.
                              if(t->type() != fin_track->type())
                                continue;

                              Part* part = t->parts()->findCloneMaster(cloneUuid);
                              if(part)
                              {
                                uuidFoundInTracks = true;

                                DEBUG_SONGFILE(stderr, " found in track:%p %s part:%p hasClones:%d\n",
                                  t, t->name().toLocal8Bit().constData(), part, part->hasClones());

                                // Is the found part a clone?
                                const bool isclone = part->hasClones();

                                if(
                                  // If the found part is a clone, then clone it. Always.
                                  isclone ||
                                  // The found part is not a clone. Do we WANT to clone it?
                                  (doClone &&
                                  // Only if the xml part was NOT a clone.
                                  // Do not clone the found part if the xml part was a clone
                                  //  and the found part is not. That is possible: Copy two clones,
                                  //  then de-clone one of them, then paste. This will handle it smoothly,
                                  //  ensuring two pasted clones WITHOUT chaining to the existing (now) non-clone.
                                  !xmlPartWasClone))
                                {
                                  // Create a clone. It must still be added later in a operationgroup
                                  // The track (t) that we happened to find a cloneable part on
                                  //  is NOT the track we want to paste to. We want the given track.
                                  npart = fin_track->newPart(part, true);

                                  DEBUG_SONGFILE(stderr, " npart:%p muuid:%s\n",
                                    npart, npart->clonemaster_uuid().toString().toLocal8Bit().constData());
                                }
                                break;
                              }
                            }
                          }

                          // If the part has not been created and an id was found...
                          if(!npart && cloneId != -1)
                          {
                            DEBUG_SONGFILE(stderr, " cloneId valid:%d\n", cloneId);

                            // Find a clone part in the stats list with the same clone id.
                            if(stats)
                            {
                              Part* part = stats->findCloneNum(cloneId);
                              if(part)
                              {
                                npart = fin_track->newPart(part, true);

                                DEBUG_SONGFILE(stderr, " found in stats: part:%p npart:%p muuid:%s\n",
                                  part,
                                  npart, npart->clonemaster_uuid().toString().toLocal8Bit().constData());

                                break;
                              }
                            }
                          }

                          if(!npart) // If the part still has not been created yet...
                          {
                            // If we're pasting to selected track and the 'wave'
                            //  variable is valid, check for mismatch...
                            if(wave || !trackIsParent)
                            {
                              // If both the part and track are not midi or wave...
                              if((wave && fin_track->isMidiTrack()) ||
                                (!wave && fin_track->type() == Track::WAVE))
                              {
                                xml.skip("part");
                                return nullptr;
                              }
                            }

                            if (fin_track->isMidiTrack())
                              npart = new MidiPart((MidiTrack*)fin_track);
                            else if (fin_track->type() == Track::WAVE)
                              npart = new WavePart((WaveTrack*)fin_track);
                            else
                            {
                              xml.skip("part");
                              return nullptr;
                            }

                            // Signify a new non-clone part was created.
                            // Even if the original part was itself a clone, clear this because the
                            //  attribute section did not create a clone from any matching part.
                            clone = false;

                            DEBUG_SONGFILE(stderr, " new npart:%p muuid:%s\n",
                              npart, npart->clonemaster_uuid().toString().toLocal8Bit().constData());

                            // New part creates its own clone master uuid, but we need to replace it.
                            // ONLY if the uuid was not already found in tracks.
                            // That handles the situation where parts are copied, then erased, then pasted.
                            if(!cloneUuid.isNull() && !uuidFoundInTracks)
                            {
                              npart->setClonemasterUuid(cloneUuid);
                              DEBUG_SONGFILE(stderr, " set npart muuid:%s\n",
                                npart->clonemaster_uuid().toString().toLocal8Bit().constData());
                            }

                            // If an id or uuid was found, add the part to the clone list
                            //  so that subsequent parts can look it up and clone from it...
                            // Include the given uuid since the new part's uuid may be different (see above).
                            if(stats && (!cloneUuid.isNull() || cloneId != -1))
                            {
                              DEBUG_SONGFILE(stderr, " adding to stats: cloneId:%d\n", cloneId);
                              stats->_parts.push_back(XmlReadStatsStruct(npart, cloneUuid, cloneId));
                            }
                          }
                        }

                        if (tag == "name")
                              npart->setName(xml.parse1());
                        else if (tag == "viewState") {
                              npart->viewState().read(xml);
                              }
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
                                if(npart->partType() == Part::MidiPartType)
                                  type = Note;
                                Event e(type);
                                e.read(xml);
                                // stored pos for event has absolute value. However internally
                                // pos is relative to start of part, we substract part pos.
                                // In case the event and part pos types differ, the event dominates.
                                e.setPosValue(e.posValue() - npart->posValue(e.pos().type()));
#ifdef ALLOW_LEFT_HIDDEN_EVENTS
                                npart->addEvent(e);
#else
                                const int posval = e.posValue();
                                if(posval < 0)
                                {
                                  printf("readClone: warning: event at posval:%d not in part:%s, discarded\n",
                                    posval, npart->name().toLocal8Bit().constData());
                                }
#endif
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
                        else if (tag == "uuidSn")
                        {
                          // Not used yet.
                          //uuidSn = QUuid(xml.s2());
                        }
                        else if (tag == "cloneId")
                        {
                          cloneId = xml.s2().toInt();
                        }
                        else if (tag == "uuid")
                        {
                          cloneUuid = QUuid(xml.s2());
                        }
                        else if(tag == "isclone")
                        {
                          xmlPartWasClone = xml.s2().toInt();
                        }
                        else if (tag == "trackUuid")
                        {
                          trackUuid = QUuid(xml.s2());
                        }
                        else if (tag == "trackType")
                        {
                          xmlTrackType = Track::TrackType(xml.s2().toUInt());
                          xmlTrackTypeValid = true;
                        }
                        else
                          fprintf(stderr, "Part::readFromXml unknown tag %s\n", tag.toLocal8Bit().constData());
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

void Part::write(int level, Xml& xml, bool isCopy, bool forceWavePaths, XmlWriteStatistics* stats) const
      {
      // Whether to save track and part uuids inside the song file.
      // If false, uuids will be written ONLY to copy/paste/clipboard/save operations, but not song files.
      // If true, uuids will be saved in song files. FOR FUTURE USE...
      // TODO: Nagging reservations about saving uuids or even local uids in a song file.
      //       Yes, it could be useful for say, a persistent cliplist between sessions.
      //       And it's actually the 'right' thing to do - everything gets a truly global identifier.
      //       But it's ugly, detracts from hand-edited files, and adds another path for corruption
      //        in case two uuids are the same in a song file for some reason.
      //       Yet it would provide a mechanism to link to some external muse file.
      //       Persistent saved clips that are specific 'project aware' for example.
      const bool saveUuidsInSong = false;

      bool midi = partType() == Part::MidiPartType;

      int clonemasterID = -1;
      if(stats)
        clonemasterID = stats->cloneID(clonemaster_uuid());
      const bool clonemasterIDFound = clonemasterID != -1;

      const bool hasclones = hasClones();

      xml.nput(level, "<part");

      // Special markers if this is a copy operation.
      if(isCopy && !midi)
        xml.nput(level, " type=\"wave\"");

      // Not used yet.
      //if(isCopy || saveUuidsInSong)
      //  xml.nput(" uuidSn=\"%s\"", uuid().toString().toUtf8().constData());

      if(hasclones)
      {
        // If this is the first time writing a clone part with this clonemaster serial number,
        //  remember the part and grab its clone ID counter after adding.
        if(stats && !clonemasterIDFound)
          clonemasterID = stats->addPart(const_cast<Part*>(this));
      }

      if(isCopy || saveUuidsInSong)
      {
        // Write the full globally unique clonemaster identifier.
        xml.nput(" uuid=\"%s\"", clonemaster_uuid().toString().toUtf8().constData());
      }
      else if(hasclones)
      {
        // Write a 'clonemaster counter' using the increasing parts container size.
        // These numbers look nicer than a clonemaster serial number or uuid.
        // They conveniently start at zero so users can easily read the various
        //  clone chains in a song file.
        if(stats)
          xml.nput(" cloneId=\"%d\"", clonemasterID);
      }

      // If this is a copy operation, not a song save operation, include whether the part is a clone.
      if(isCopy && hasclones)
        xml.nput(" isclone=\"1\"");

      // If this is a copy operation, not a song save operation, include
      //  information about the original track to help with pasting or
      //  importing from a clipboard saved to a file.
      // (In a song save operation, the part is already listed under a track tag.)
      if(isCopy && track())
        xml.nput(" trackUuid=\"%s\" trackType=\"%d\"", track()->uuid().toString().toUtf8().constData(), (unsigned) track()->type());

      xml.put(">");
      level++;

      xml.strTag(level, "name", _name);

      // This won't bother writing if the state is invalid.
      viewState().write(level, xml);

      PosLen::write(level, xml, "poslen");
      xml.intTag(level, "selected", _selected);
      xml.intTag(level, "color", _colorIndex);
      if (_mute)
            xml.intTag(level, "mute", _mute);

      // If this is the first time writing a part with this clonemaster serial number,
      //  write the events, even if this part itself is a clone.
      // Otherwise another part with that clonemaster serial number has already written
      //  its events. Don't bother writing this part's events since they would be redundant.
      if ( !clonemasterIDFound ) {
            for (ciEvent e = events().begin(); e != events().end(); ++e)
                  e->second.write(level, xml, *this, forceWavePaths);
            }
      xml.etag(--level, "part");
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
//   checkSongSampleRate
//   Called by gui thread.
//---------------------------------------------------------

void Song::checkSongSampleRate()
{
  std::map<int, int> waveRates;
  
  for(ciWaveTrack iwt = waves()->begin(); iwt != waves()->end(); ++iwt)
  {
    WaveTrack* wt = *iwt;
    for(ciPart ipt = wt->parts()->begin(); ipt != wt->parts()->end(); ++ipt)
    {
      Part* pt = ipt->second;
      for(ciEvent ie = pt->events().begin(); ie != pt->events().end(); ++ie)
      {
        const Event e(ie->second);
        if(e.sndFile().isOpen())
        {
          const int sr = e.sndFile().samplerate();
          std::map<int, int>::iterator iwr = waveRates.find(sr);
          if(iwr == waveRates.end())
          {
            waveRates.insert(std::pair<int, int>(sr, 1));
          }
          else
          {
            ++iwr->second;
          }
        }
      }
    }
  }
  
  for(std::map<int, int>::const_iterator iwr = waveRates.cbegin(); iwr != waveRates.cend(); ++iwr)
  {
    
  }
}

//---------------------------------------------------------
//   read
//---------------------------------------------------------

void Song::read(Xml& xml, bool /*isTemplate*/)
      {
      XmlReadStatistics stats;

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
                        goto song_read_end;
                  case Xml::TagStart:
                        if (tag == "master")
                        {
                              // Avoid emitting songChanged.
                              // Tick parameter is not used.
                              MusEGlobal::tempomap.setMasterFlag(0, xml.parseInt());
                        }
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
                        // This doesn't work as there are no tracks yet at this point and this is checked
                        // in setRecord. So better make it clear and explicit.
                        // (Using the default autoRecEnable==true would seem wrong too at this point.)
                        // setRecord(xml.parseInt());
                              setRecord(false);
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
                              _songLenTicks  = xml.parseInt();
                        else if (tag == "follow")
                              _follow  = FollowMode(xml.parseInt());
                        else if (tag == "midiDivision") {
                              // TODO: Compare with current global setting and convert the
                              //  song if required - similar to how the song vs. global
                              //  sample rate ratio is handled. Ignore for now.
                              xml.parseInt();
                            }
                        else if (tag == "sampleRate") {
                              // Ignore. Sample rate setting is handled by the
                              //  song discovery mechanism (in MusE::loadProjectFile1()).
                              xml.parseInt();
                            }
                        else if (tag == "tempolist") {
                              MusEGlobal::tempomap.read(xml);
                              }
                        else if (tag == "siglist")
                              MusEGlobal::sigmap.read(xml);
                        else if (tag == "keylist") {
                              MusEGlobal::keymap.read(xml);
                              }
                        else if (tag == "miditrack") {
                              MidiTrack* track = new MidiTrack();
                              track->read(xml, &stats);
                              insertTrack0(track, -1);
                              }
                        else if (tag == "drumtrack") { // Old drumtrack is obsolete.
                              MidiTrack* track = new MidiTrack();
                              track->setType(Track::DRUM);
                              track->read(xml, &stats);
                              track->convertToType(Track::DRUM); // Convert the notes and controllers.
                              insertTrack0(track, -1);
                              }
                        else if (tag == "newdrumtrack") {
                              MidiTrack* track = new MidiTrack();
                              track->setType(Track::DRUM);
                              track->read(xml, &stats);
                              insertTrack0(track, -1);
                              }
                        else if (tag == "wavetrack") {
                              MusECore::WaveTrack* track = new MusECore::WaveTrack();
                              track->read(xml, &stats);
                              insertTrack0(track,-1);
                              track->showPendingPluginGuis();
                              }
                        else if (tag == "AudioInput") {
                              AudioInput* track = new AudioInput();
                              track->read(xml, &stats);
                              insertTrack0(track,-1);
                              track->showPendingPluginGuis();
                              }
                        else if (tag == "AudioOutput") {
                              AudioOutput* track = new AudioOutput();
                              track->read(xml, &stats);
                              insertTrack0(track,-1);
                              track->showPendingPluginGuis();
                              }
                        else if (tag == "AudioGroup") {
                              AudioGroup* track = new AudioGroup();
                              track->read(xml, &stats);
                              insertTrack0(track,-1);
                              track->showPendingPluginGuis();
                              }
                        else if (tag == "AudioAux") {
                              AudioAux* track = new AudioAux();
                              track->read(xml, &stats);
                              insertTrack0(track,-1);
                              track->showPendingPluginGuis();
                              }
                        else if (tag == "SynthI") {
                              SynthI* track = new SynthI();
                              track->read(xml, &stats);
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
                        // REMOVE Tim. automation. Removed.
                        // Deprecated. MusEGlobal::automation is now fixed TRUE
                        //  for now until we decide what to do with it.
                        else if (tag == "automation")
                        //      MusEGlobal::automation = xml.parseInt();
                              xml.parseInt();
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
                        else if (tag == "midiAssign")
                              // Any assignments read here will have no track.
                              _midiAssignments.read(xml, nullptr);
                        else
                              xml.unknown("Song");
                        break;
                  case Xml::Attribut:
                        break;
                  case Xml::TagEnd:
                        if (tag == "song") {
                              goto song_read_end;
                              }
                  default:
                        break;
                  }
            }
            
song_read_end:
      dirty = false;
      }

//---------------------------------------------------------
//   write
//---------------------------------------------------------

void Song::write(int level, Xml& xml) const
      {
      xml.tag(level++, "song");

      xml.strTag(level, "info", songInfoStr);
      xml.intTag(level, "showinfo", showSongInfo);
// REMOVE Tim. automation. Removed.
// Deprecated. MusEGlobal::automation is now fixed TRUE
//   for now until we decide what to do with it.
//       xml.intTag(level, "automation", MusEGlobal::automation);
      xml.intTag(level, "cpos", MusEGlobal::song->cpos());
      xml.intTag(level, "rpos", MusEGlobal::song->rpos());
      xml.intTag(level, "lpos", MusEGlobal::song->lpos());
      xml.intTag(level, "master", MusEGlobal::tempomap.masterFlag());
      xml.intTag(level, "loop", loopFlag);
      xml.intTag(level, "punchin", punchinFlag);
      xml.intTag(level, "punchout", punchoutFlag);
      xml.intTag(level, "record", recordFlag);
      xml.intTag(level, "solo", soloFlag);
      xml.intTag(level, "recmode", _recMode);
      xml.intTag(level, "cycle", _cycleMode);
      xml.intTag(level, "click", _click);
      xml.intTag(level, "quantize", _quantize);
      xml.intTag(level, "len", _songLenTicks);
      xml.intTag(level, "follow", _follow);
      // Save the current global midi division as well as current sample rate
      //  so the song can be scaled properly if the values differ on reload.
      xml.intTag(level, "midiDivision", MusEGlobal::config.division);
      xml.intTag(level, "sampleRate", MusEGlobal::sampleRate);
      if (_globalPitchShift)
            xml.intTag(level, "globalPitchShift", _globalPitchShift);

      {
        XmlWriteStatistics xmlStats;

        // write tracks
        for (ciTrack i = _tracks.begin(); i != _tracks.end(); ++i)
              (*i)->write(level, xml, &xmlStats);
      }

      // Write only the assignments which have no track.
      _midiAssignments.write(level, xml, nullptr);

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
      xml.etag(--level, "song");
      }

//--------------------------------
//   resolveInstrumentReferences
//--------------------------------

static void resolveInstrumentReferences()
{
  for(int i = 0; i < MIDI_PORTS; ++i)
  {
    MidiPort* mp = &MusEGlobal::midiPorts[i];
    const QString& name = mp->tmpInstrRef();
    const int idx = mp->tmpTrackRef();
    if(idx >= 0)
    {
      Track* track = MusEGlobal::song->tracks()->index(idx);
      if(track && track->isSynthTrack())
      {
        SynthI* si = static_cast<SynthI*>(track);
        mp->changeInstrument(si);
      }
    }
    else if(!name.isEmpty())
    {
      mp->changeInstrument(registerMidiInstrument(name));
    }

    // Done with temporary file references. Clear them.
    mp->clearTmpFileRefs();
  }
}

//--------------------------------
//   resolveStripReferences
//--------------------------------

static void resolveStripReferences(MusEGlobal::MixerConfig* mconfig)
{
  MusECore::TrackList* tl = MusEGlobal::song->tracks();
  MusECore::Track* track;
  MusEGlobal::StripConfigList_t& scl = mconfig->stripConfigList;
  if(!scl.empty())
  {
    for(MusEGlobal::iStripConfigList isc = scl.begin(); isc != scl.end(); )
    {
      MusEGlobal::StripConfig& sc = *isc;

      // Is it a dud? Delete it.
      if(sc.isNull() && sc._tmpFileIdx < 0)
      {
        isc = scl.erase(isc);
        continue;
      }

      // Does it have a temporary track index?
      if(sc._tmpFileIdx >= 0)
      {
        // Find an existing track at that index.
        track = tl->index(sc._tmpFileIdx);
        // No corresponding track found? Delete the strip config.
        if(!track)
        {
          isc = scl.erase(isc);
          continue;
        }
        // Link the strip config to the track, via uuid.
        sc._uuid = track->uuid();
        // Done with temporary index. Reset it.
        sc._tmpFileIdx = -1;
      }

      // As for other configs with a serial and no _tmpFileIdx,
      //  leave them alone. They may already be valid. If not,
      //  they may need to persist for the undo system to restore from.
      // Ultimately when saving, the strip config write() function removes duds anyway.

      ++isc;
    }
  }
}

//--------------------------------
//   resolveSongfileReferences
//--------------------------------

void Song::resolveSongfileReferences()
{
  //-----------------------------------------------
  // Resolve instrument references:
  //-----------------------------------------------
  resolveInstrumentReferences();

  //-----------------------------------------------
  // Resolve mixer strip configuration references:
  //-----------------------------------------------
  resolveStripReferences(&MusEGlobal::config.mixer1);
  resolveStripReferences(&MusEGlobal::config.mixer2);
}

// REMOVE Tim. samplerate. Added. TODO
#if 0
//---------------------------------------------------------
//   convertProjectSampleRate
//---------------------------------------------------------

void Song::convertProjectSampleRate(int newRate)
{
  double sratio = double(newRate) / double(MusEGlobal::sampleRate);
  
  for(iTrack i = _tracks.begin(); i != _tracks.end(); ++i)
  {
    Track* track = *i;
    //if(track->isMidiTrack)
    //  continue;
    
    PartList* parts = track->parts();
    
    for(iPart ip = parts->begin(); i != parts->end(); ++i)
    {
      Part* part = *ip;
      
      EventList& el = part->nonconst_events();
      
      for(iEvent ie = el.begin(); ie != el.end(); ++ie)
      {
        Event& e = *ie;
      }
      
      if(part->type() == Part::FRAMES)
      {
        part->setFrame(double(part->frame()) * sratio);
        part->setLenFrame(double(part->lenFrame()) * sratio);
      }
    }
    
    if(track->type == Track::WAVE)
    {
      
    }

    
  }
}
#endif 
      
} // namespace MusECore

namespace MusEGui {

//---------------------------------------------------------
//   readPart
//---------------------------------------------------------

MusECore::Part* MusE::readPart(MusECore::Xml& xml)
      {
      MusECore::Part* part = nullptr;
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
                        sscanf(tag.toLocal8Bit().constData(), "%d:%d", &trackIdx, &partIdx);
                        MusECore::Track* track = nullptr;
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
            delete pl;
            return;
        case MusECore::Xml::TagStart:
            if (tag == "part") {
                MusECore::Part* part = readPart(xml);
                if (part)
                    pl->add(part);
            }
            else if (tag == "pianoroll") {
                // Do not open if there are no parts.
                // Had bogus '-1' part index for list edit in med file,
                //  causing list edit to segfault on song load.
                // Somehow that -1 was put there on write, because the
                //  current part didn't exist anymore, so no index number
                //  could be found for it on write. Watching... may be fixed.
                // But for now be safe for all the top levels...
                if(pl->empty())
                  xml.skip(tag);
                else
                {
                  bool createdNotFound = false;
                  MusEGui::PianoRoll *pr = startPianoroll(pl, false, false, &createdNotFound);
                  if(pr && createdNotFound)
                  {
                    pr->readStatus(xml);
                    pl = new MusECore::PartList;
                  }
                  else
                  {
                    // Should not really happen.
                    fprintf(stderr, "WARNING: MusE::read: pianoroll already exists or was not created\n");
                    pl->clear();
                    xml.skip(tag);
                  }
                }
            }
            else if (tag == "scoreedit") {
                MusEGui::ScoreEdit* score = new MusEGui::ScoreEdit(this, 0, _arranger->cursorValue());
                toplevels.push_back(score);
                connect(score, SIGNAL(isDeleting(MusEGui::TopWin*)), SLOT(toplevelDeleting(MusEGui::TopWin*)));
                connect(score, SIGNAL(name_changed()), arrangerView, SLOT(scoreNamingChanged()));
                score->show();
                score->readStatus(xml);
                // Part list is not used.
                pl->clear();
            }
            else if (tag == "drumedit") {
                if(pl->empty())
                  xml.skip(tag);
                else
                {
                  bool createdNotFound = false;
                  MusEGui::DrumEdit *de = startDrumEditor(pl, false, false, &createdNotFound);
                  if(de && createdNotFound)
                  {
                    de->readStatus(xml);
                    pl = new MusECore::PartList;
                  }
                  else
                  {
                    // Should not really happen.
                    fprintf(stderr, "WARNING: MusE::read: drum editor already exists or was not created\n");
                    pl->clear();
                    xml.skip(tag);
                  }
                }
            }
            else if (tag == "master") {
                bool createdNotFound = false;
                MusEGui::MasterEdit *me = startMasterEditor(&createdNotFound);
                if(me && createdNotFound)
                  me->readStatus(xml);
                else
                {
                  // Should not really happen.
                  fprintf(stderr, "WARNING: MusE::read: master editor already exists or was not created\n");
                  xml.skip(tag);
                }
                // Part list is not used.
                pl->clear();
            }
            else if (tag == "arrangerview") {
                TopWin* tw = toplevels.findType(TopWin::ARRANGER);
                tw->readStatus(xml);
                tw->showMaximized();
                // Part list is not used.
                pl->clear();
            }
            else if (tag == "waveedit") {
                if(pl->empty())
                  xml.skip(tag);
                else
                {
                  bool createdNotFound = false;
                  MusEGui::WaveEdit *we = startWaveEditor(pl, false, &createdNotFound);
                  if(we && createdNotFound)
                  {
                    we->readStatus(xml);
                    pl = new MusECore::PartList;
                  }
                  else
                  {
                    // Should not really happen.
                    fprintf(stderr, "WARNING: MusE::read: wave editor already exists or was not created\n");
                    pl->clear();
                    xml.skip(tag);
                  }
                }
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
                        // {
                        //   // REMOVE Tim. qt6. Added. Diagnostics.
                        //   QString t1(tag);
                        //   QString t2("muse");
                        //   const int ts1 = t1.size();
                        //   const int ts2 = t2.size();
                        //   fprintf(stderr, "t1:\n");
                        //   for(int i = 0; i < ts1; ++i)
                        //   {
                        //     fprintf(stderr, " pos:%d value:%X\n", i, t1.at(i).unicode());
                        //   }
                        //   fprintf(stderr, "\nt2:\n");
                        //   for(int i = 0; i < ts2; ++i)
                        //   {
                        //     fprintf(stderr, " pos:%d value:%X\n", i, t2.at(i).unicode());
                        //   }
                        //
                        // }

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
                              // Before reading the song, clear the list of missing plugins.
                              MusEGlobal::missingPlugins.clear();

                              MusEGlobal::song->read(xml, isTemplate);

                              // Now that the song file has been fully loaded, resolve any references in the file.
                              MusEGlobal::song->resolveSongfileReferences();

                              // Now that all track and instrument references have been resolved,
                              //  it is safe to add all the midi controller cache values.
                              MusEGlobal::song->changeMidiCtrlCacheEvents(true, true, true, true, true);

                              MusEGlobal::audio->msgUpdateSoloStates();
                              // Inform the rest of the app that the song (may) have changed, using these flags.
                              // After this function is called, the caller can do a general Song::update() MINUS these flags,
                              //  like in MusE::loadProjectFile1() - the only place calling so far, as of this writing.
                              // Some existing windows need this, like arranger, some don't which are dynamically created after this.
                              MusEGlobal::song->update(SC_TRACK_INSERTED);
                        }
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
                        if(!MusEGlobal::missingPlugins.empty())
                        {
                          MusEGui::MissingPluginsDialog mpd =
                           MusEGui::MissingPluginsDialog(MusEGlobal::missingPlugins, this);
                          mpd.exec();
                        }

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
        for (const auto& i : toplevels) {
            if (i->isVisible())
                i->writeStatus(level, xml);
        }
        xml.etag(--level, "toplevels");
    }
    else if (!writeTopwins)
    {
        xml.tag(level, "no_toplevels");
        xml.etag(level, "no_toplevels");
    }

    xml.etag(--level, "muse");
}

} // namespace MusEGui

