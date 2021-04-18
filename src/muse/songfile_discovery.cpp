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
    _valid = sf != nullptr;
    if(sf)
      sf_close(sf);
  }
}

//---------------------------------------------------------
//   readWaveEvent
//---------------------------------------------------------

void SongfileDiscovery::readWaveEvent(Xml& xml)
      {
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
                        if (tag == "file") {
                              filename = xml.parse1();
                              }
                        else
                              // Same as unknown() without the error message.
                              xml.parse1();
                        break;
                  case Xml::TagEnd:
                        if (tag == "event") {
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
      for (;;) {
            Xml::Token token = xml.parse();
            const QString& tag = xml.s1();
            switch (token) {
                  case Xml::Error:
                  case Xml::End:
                        return;
                  case Xml::TagStart:
                        if (tag == "event")
                        {
                          readWaveEvent(xml);
                        }
                        else
                              // Same as unknown() without the error message.
                              xml.parse1();
                        break;
                  case Xml::Attribut:
                        break;
                  case Xml::TagEnd:
                        if (tag == "part")
                          return;
                  default:
                        break;
                  }
            }
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
                              readWavePart(xml);
                              }
                        else
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
                        else if (tag == "wavetrack") {
                              readWaveTrack(xml);
                              }
                        else
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
                        else if (tag == "song")
                        {
                              readSong(xml);
                        }
                        else
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

