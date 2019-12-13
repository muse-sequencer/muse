//=========================================================
//  MusE
//  Linux Music Editor
//
//  songfile_discovery.cpp
//  Copyright (C) 2019 Tim E. Real (terminator356 at users dot sourceforge dot net)
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

#include "songfile_discovery.h"

#include <QFile>
#include <QFileInfo>

namespace MusECore {

SongfileDiscoveryWaveItem::SongfileDiscoveryWaveItem(const QString& filename)
  : _filename(filename)
{
  _valid = false;
  if(!_filename.isEmpty() && QFile::exists(_filename))
  {
    _sfinfo.format = 0;
    SNDFILE* sf = sf_open(_filename.toLocal8Bit().constData(), SFM_READ, &_sfinfo);
    _valid = sf != NULL;
    if(sf)
      sf_close(sf);
  }
}

//---------------------------------------------------------
//   readWaveEvent
//---------------------------------------------------------

void SongfileDiscovery::readWaveEvent(Xml& xml)
      {
//       StretchList sl;
//       AudioConverterSettingsGroup settings(true); // Local non-default settings.
//       settings.populate(&MusEGlobal::audioConverterPluginList, true);  // Local non-default settings.
      QString filename;
      for (;;) {
            Xml::Token token = xml.parse();
            const QString& tag = xml.s1();
            switch (token) {
                  case Xml::Error:
                  case Xml::End:
                  case Xml::Attribut:
                        return;
                  case Xml::TagStart:
//                         if (tag == "poslen")
//                               PosLen::read(xml, "poslen");
//                         else if (tag == "frame")
//                               _spos = xml.parseInt();
                        /*else*/ if (tag == "file") {
                              filename = xml.parse1();
                              }
                              
//                         else if (tag == "stretchlist")
//                         {
// //                           if(f.stretchList())
// //                             f.stretchList()->read(xml);
//                           sl.read(xml);
//                         }
//                         else if (tag == "audioConverterSettingsGroup")
//                         {
// //                           if(f.audioConverterSettings())
// //                             f.audioConverterSettings()->read(xml);
//                           settings.read(xml);
//                         }

                        else
                              //xml.unknown("Event");
                              // Same as unknown() without the error message.
                              xml.parse1();
                        break;
                  case Xml::TagEnd:
                        if (tag == "event") {
//                               Pos::setType(FRAMES);   // DEBUG

                              if(!filename.isEmpty())
                              {
                                QString name = filename;
                                if (QFileInfo(name).isRelative()) {
                                      name = _projectPath + QString("/") + name;
                                      }
                                else {
                                      if (!QFile::exists(name)) {
                                            if (QFile::exists(_projectPath + QString("/") + name)) {
                                                  name = _projectPath + QString("/") + name;
                                                  }
                                            }
                                      }

                                SongfileDiscoveryWaveItem item(name);
                                if(item._valid)
                                {
                                  _waveList.push_back(item);
                                  SongSampleratesInsRes_t res =
                                    _waveList._samplerates.insert(SongSampleratesIns_t(item._sfinfo.samplerate, 0));
                                  ++res.first->second;
                                }
//                                 SndFileR wf = getWave(filename, true, true, true, &settings, &sl);
//                                 if(wf) 
//                                   setSndFile(wf);
                              }
                              
                              return;
                              }
                  default:
                        break;
                  }
            }
      }

//---------------------------------------------------------
//   readWavePart
//---------------------------------------------------------

void SongfileDiscovery::readWavePart(Xml& xml)
      {
//       int id = -1;
//       Part* npart = 0;
//       QUuid uuid;
//       bool uuidvalid = false;
//       bool clone = true;
//       bool wave = false;
//       bool isclone = false;

      for (;;) {
            Xml::Token token = xml.parse();
            const QString& tag = xml.s1();
            switch (token) {
                  case Xml::Error:
                  case Xml::End:
//                         return npart;
                        return;
                  case Xml::TagStart:
//                         if(!npart) // If the part has not been created yet...
//                         {
//                           if(id != -1) // If an id was found...
//                           {
//                             for(iClone i = MusEGlobal::cloneList.begin(); i != MusEGlobal::cloneList.end(); ++i)
//                             {
//                               if(i->is_deleted) // Is the clone item marked as deleted? Ignore it.
//                                 continue;
//                               if(i->id == id) // Is a matching part found in the clone list?
//                               {
//                                 // Create a clone. It must still be added later in a operationgroup
//                                 npart = track->newPart((Part*)i->cp, true);
//                                 break;
//                               }
//                             }
//                           }
//                           else if(uuidvalid) // If a uuid was found...
//                           {
//                             for(iClone i = MusEGlobal::cloneList.begin(); i != MusEGlobal::cloneList.end(); ++i)
//                             {
//                               if(uuid == i->_uuid) // Is a matching part found in the clone list?
//                               {
//                                 Track* cpt = i->cp->track();
//                                 if(toTrack) // If we want to paste to the given track...
//                                 {
//                                   // If the given track type is not the same as the part's
//                                   //  original track type, we can't continue. Just return.
//                                   if(!track || cpt->type() != track->type())
//                                   {
//                                     xml.skip("part");
//                                     return 0;
//                                   }
//                                 }
//                                 else // ...else we want to paste to the part's original track.
//                                 {
//                                   // Make sure the track exists (has not been deleted).
//                                   if((cpt->isMidiTrack() && MusEGlobal::song->midis()->find(cpt) != MusEGlobal::song->midis()->end()) ||
//                                       (cpt->type() == Track::WAVE && MusEGlobal::song->waves()->find(cpt) != MusEGlobal::song->waves()->end()))
//                                     track = cpt;
//                                   else // Track was not found. Try pasting to the given track, as above...
//                                   {
//                                     if(!track || cpt->type() != track->type())
//                                     {
//                                       // No luck. Just return.
//                                       xml.skip("part");
//                                       return 0;
//                                     }
//                                   }
//                                 }
// 
//                                 if(i->is_deleted) // Is the clone item marked as deleted? Don't create a clone, create a copy.
//                                   break;
// 
//                                 // If it's a regular paste (not paste clone), and the original part is
//                                 //  not a clone, defer so that a new copy is created in TagStart above.
//                                 if(!doClone && !isclone)
//                                   break;
// 
//                                 // Create a clone. It must still be added later in a operationgroup
//                                 npart = track->newPart((Part*)i->cp, true);
//                                 break;
//                               }
//                             }
//                           }
// 
//                           if(!npart) // If the part still has not been created yet...
//                           {
// 
//                             if(!track) // A clone was not created from any matching
//                             {          // part. Create a non-clone part now.
//                               xml.skip("part");
//                               return 0;
//                             }
//                             // If we're pasting to selected track and the 'wave'
//                             //  variable is valid, check for mismatch...
//                             if(toTrack && uuidvalid)
//                             {
//                               // If both the part and track are not midi or wave...
//                               if((wave && track->isMidiTrack()) ||
//                                 (!wave && track->type() == Track::WAVE))
//                               {
//                                 xml.skip("part");
//                                 return 0;
//                               }
//                             }
// 
//                             if (track->isMidiTrack())
//                               npart = new MidiPart((MidiTrack*)track);
//                             else if (track->type() == Track::WAVE)
//                               npart = new MusECore::WavePart((MusECore::WaveTrack*)track);
//                             else
//                             {
//                               xml.skip("part");
//                               return 0;
//                             }
// 
//                             // Signify a new non-clone part was created.
//                             // Even if the original part was itself a clone, clear this because the
//                             //  attribute section did not create a clone from any matching part.
//                             clone = false;
// 
//                             // If an id or uuid was found, add the part to the clone list
//                             //  so that subsequent parts can look it up and clone from it...
//                             if(id != -1)
//                             {
//                               ClonePart ncp(npart, id);
//                               MusEGlobal::cloneList.push_back(ncp);
//                             }
//                             else
//                             if(uuidvalid)
//                             {
//                               ClonePart ncp(npart);
//                               // New ClonePart creates its own uuid, but we need to replace it.
//                               ncp._uuid = uuid; // OK for non-windows?
//                               MusEGlobal::cloneList.push_back(ncp);
//                             }
//                           }
//                         }

//                         if (tag == "name")
//                               npart->setName(xml.parse1());
//                         else if (tag == "viewState") {
//                               npart->viewState().read(xml);
//                               }
//                         else if (tag == "poslen") {
//                               ((PosLen*)npart)->read(xml, "poslen");
//                               }
//                         else if (tag == "pos") {
//                               Pos pos;
//                               pos.read(xml, "pos");  // obsolete
//                               npart->setTick(pos.tick());
//                               }
//                         else if (tag == "len") {
//                               Pos len;
//                               len.read(xml, "len");  // obsolete
//                               npart->setLenTick(len.tick());
//                               }
//                         else if (tag == "selected")
//                               npart->setSelected(xml.parseInt());
//                         else if (tag == "color")
//                               npart->setColorIndex(xml.parseInt());
//                         else if (tag == "mute")
//                               npart->setMute(xml.parseInt());
                        /*else*/
                        if (tag == "event")
                        {
//                               // If a new non-clone part was created, accept the events...
//                               if(!clone)
//                               {
//                                 EventType type = Wave;
//                                 if(track->isMidiTrack())
//                                   type = Note;
//                                 Event e(type);
//                                 e.read(xml);
//                                 // stored tickpos for event has absolute value. However internally
//                                 // tickpos is relative to start of part, we substract tick().
//                                 // TODO: better handling for wave event
//                                 e.move( -npart->tick() );
//                                 int tick = e.tick();
// 
//                                 if(tick < 0)
//                                 {
//                                   printf("readClone: warning: event at tick:%d not in part:%s, discarded\n",
//                                     tick, npart->name().toLatin1().constData());
//                                 }
//                                 else
//                                 {
//                                   npart->addEvent(e);
//                                 }
//                               }
//                               else // ...Otherwise a clone was created, so we don't need the events.
//                                 xml.skip(tag);
                              
                              readWaveEvent(xml);
                        }
                        else
                              //xml.unknown("readXmlPart");
                              // Same as unknown() without the error message.
                              xml.parse1();
                        break;
                  case Xml::Attribut:
//                         if (tag == "type")
//                         {
//                           if(xml.s2() == "wave")
//                             wave = true;
//                         }
//                         else if (tag == "cloneId")
//                         {
//                           id = xml.s2().toInt();
//                         }
//                         else if (tag == "uuid")
//                         {
//                           uuid = QUuid(xml.s2());
//                           if(!uuid.isNull())
//                           {
//                             uuidvalid = true;
//                           }
//                         }
//                         else if(tag == "isclone")
//                           isclone = xml.s2().toInt();
                        break;
                  case Xml::TagEnd:
                        if (tag == "part")
                          return;
                  default:
                        break;
                  }
            }
//   return npart;
  return;
}

//---------------------------------------------------------
//   readWaveTrack
//---------------------------------------------------------

void SongfileDiscovery::readWaveTrack(Xml& xml)
      {
      for (;;) {
            Xml::Token token = xml.parse();
            const QString& tag = xml.s1();
            switch (token) {
                  case Xml::Error:
                  case Xml::End:
                        return;
                  case Xml::TagStart:
                        if (tag == "part") {
//                               Part* p = 0;
//                               p = Part::readFromXml(xml, this);
//                               if(p)
//                                 parts()->add(p);
                              readWavePart(xml);
                              }
                        else // if (AudioTrack::readProperties(xml, tag))
                              //xml.unknown("WaveTrack");
                              // Same as unknown() without the error message.
                              xml.parse1();
                        break;
                  case Xml::Attribut:
                        break;
                  case Xml::TagEnd:
                        if (tag == "wavetrack") {
                              return;
                              }
                  default:
                        break;
                  }
            }
      }

//---------------------------------------------------------
//   readSong
//---------------------------------------------------------

void SongfileDiscovery::readSong(Xml& xml)
      {
      for (;;) {

            Xml::Token token;
            token = xml.parse();
            const QString& tag = xml.s1();
            switch (token) {
                  case Xml::Error:
                  case Xml::End:
                        return;
                  case Xml::TagStart:
                        if (tag == "sampleRate")
                              _waveList.setProjectSampleRate(xml.parseInt());
//                         else if (tag == "tempolist") {
//                               MusEGlobal::tempomap.read(xml);
//                               }
//                         else if (tag == "siglist")
//                               ///MusEGlobal::sigmap.read(xml);
//                               MusEGlobal::sigmap.read(xml);
//                         else if (tag == "keylist") {
//                               MusEGlobal::keymap.read(xml);
//                               }
                        else if (tag == "wavetrack") {
//                               MusECore::WaveTrack* track = new MusECore::WaveTrack();
//                               track->read(xml);
                              readWaveTrack(xml);
                              }
                        else
                              //xml.unknown("Song");
                              // Same as unknown() without the error message.
                              xml.parse1();
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
      }

//---------------------------------------------------------
//   readSongfile
//---------------------------------------------------------

void SongfileDiscovery::readSongfile(Xml& xml)
      {
      // Start with resetting.
      _waveList._projectSampleRate = 0;
      _waveList._projectSampleRateValid = false;

      bool skipmode = true;

//       writeTopwinState=true;

      for (;;) {
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
//                         else if (tag == "configuration")
//                               readConfiguration(xml, doReadMidiPorts, false /* do NOT read global settings, see below */);
//                         /* Explanation for why "do NOT read global settings":
//                          * if you would use true here, then muse would overwrite certain global config stuff
//                          * by the settings stored in the song. but you don't want this. imagine that you
//                          * send a friend a .med file. your friend opens it and baaam, his configuration is
//                          * garbled. why? well, because these readConfigurations here would have overwritten
//                          * parts (but not all) of his global config (like MDI/SDI, toolbar states etc.)
//                          * with the data stored in the song. (it IS stored there. dunny why, i find it pretty
//                          * senseless.)
//                          *
//                          * If you've a problem which seems to be solved by replacing "false" with "true",
//                          *  i've a better solution for you: go into conf.cpp, in
//                          *  void readConfiguration(Xml& xml, bool readOnlySequencer, bool doReadGlobalConfig)
//                          * (around line 525), look for a comment like this:
//                          * "Global and/or per-song config stuff ends here" (alternatively just search for
//                          * "----"). Your problem is probably that some non-global setting should be loaded but
//                          * is not. Fix it by either placing the else if (foo)... clause responsible for that
//                          * setting to be loaded into the first part, that is, before "else if (!doReadGlobalConfig)"
//                          * or (if the settings actually IS global and not per-song), ensure that the routine
//                          * which writes the global (and not the song-)configuration really writes that setting.
//                          * (it might happen that it formerly worked because it was written to the song instead
//                          *  of the global config by mistake, and now isn't loaded anymore. write it to the
//                          *  correct location.)
//                          *
//                          *                                                                                -- flo93
//                          */
                        else if (tag == "song")
                        {
//                               MusEGlobal::song->read(xml, isTemplate);
                              readSong(xml);

                              // Now that the song file has been fully loaded, resolve any references in the file.
//                               MusEGlobal::song->resolveSongfileReferences();
                        }
                        else
                              //xml.unknown("muse");
                              // Same as unknown() without the error message.
                              xml.parse1();
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
                          fprintf(stderr, "Songfile discovery: Loaded file version is %d.%d\nCurrent version is %d.%d\n",
                                  xml.majorVersion(), xml.minorVersion(),
                                  xml.latestMajorVersion(), xml.latestMinorVersion());
//                           // Cannot construct QWidgets until QApplication created!
//                           // Check MusEGlobal::muse which is created shortly after the application...
//                           if(MusEGlobal::muse && MusEGlobal::config.warnOnFileVersions)
//                           {
//                             QString txt = tr("File version is %1.%2\nCurrent version is %3.%4\n"
//                                              "Conversions may be applied if file is saved!")
//                                             .arg(xml.majorVersion()).arg(xml.minorVersion())
//                                             .arg(xml.latestMajorVersion()).arg(xml.latestMinorVersion());
//                             QMessageBox* mb = new QMessageBox(QMessageBox::Warning,
//                                                               tr("Opening file"),
//                                                               txt,
//                                                               QMessageBox::Ok, MusEGlobal::muse);
//                             QCheckBox* cb = new QCheckBox(tr("Do not warn again"));
//                             cb->setChecked(!MusEGlobal::config.warnOnFileVersions);
//                             mb->setCheckBox(cb);
//                             mb->exec();
//                             if(!mb->checkBox()->isChecked() != MusEGlobal::config.warnOnFileVersions)
//                             {
//                               MusEGlobal::config.warnOnFileVersions = !mb->checkBox()->isChecked();
//                               // Save settings. Use simple version - do NOT set style or stylesheet,
//                               //  this has nothing to do with that.
//                               //MusEGlobal::muse->changeConfig(true);  // Save settings? No, wait till close.
//                             }
//                             delete mb;
//                           }
                        }
                        if (!skipmode && tag == "muse")
                              return;
                  default:
                        break;
                  }
            }
      }

int SongfileDiscoveryWaveList::getMostCommonSamplerate() const
{
  int rate = 0;
  int count = 0;
  for(SongSamplerates_t::const_iterator it = _samplerates.cbegin(); it != _samplerates.cend(); ++it)
  {
    const int& c = it->second;
    if(c > count)
    {
      count = c;
      rate = it->first;
    }
  }
  return rate;
}

} // namespace MusECore

