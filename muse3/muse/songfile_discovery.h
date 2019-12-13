//=========================================================
//  MusE
//  Linux Music Editor
//
//  songfile_discovery.h
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

#ifndef __SONGFILE_DISCOVERY_H__
#define __SONGFILE_DISCOVERY_H__

#include <map>
#include <list>

#include <QString>

#include "sndfile.h"
#include "xml.h"

namespace MusECore {

struct SongfileDiscoveryWaveItem
{
  QString _filename;
  SF_INFO _sfinfo;
  bool _valid;
  SongfileDiscoveryWaveItem(const QString& filename);
};

typedef std::map<int /* samplerate */, int /* count */> SongSamplerates_t;
typedef std::pair<int /* samplerate */, int /* count */> SongSampleratesIns_t;
typedef std::pair<SongSamplerates_t::iterator, bool /* result */> SongSampleratesInsRes_t;
typedef std::pair<SongSamplerates_t::iterator, SongSamplerates_t::iterator> SongSampleratesRange_t;

class SongfileDiscoveryWaveList : public std::list<SongfileDiscoveryWaveItem>
{
  public:
//     SongfileDiscoveryWaveList() : _projectSampleRate(0), _projectSampleRateValid(false) {}
//     
    int _projectSampleRate;
    bool _projectSampleRateValid;
    SongSamplerates_t _samplerates;

    void setProjectSampleRate(int r) { _projectSampleRate = r; _projectSampleRateValid = true; }
// 
//     void readWaveEvent(Xml& xml);
//     void readWavePart(Xml& xml);
//     void readWaveTrack(Xml&);
//     void readSong(Xml&);
//     void readSongfile(Xml& xml);
//     
    int getMostCommonSamplerate() const;
};

class SongfileDiscovery
{
  private:
    QString _projectPath;

  public:
    SongfileDiscovery(const QString& projectPath) : _projectPath(projectPath) {}

    SongfileDiscoveryWaveList _waveList;

    void readWaveEvent(Xml& xml);
    void readWavePart(Xml& xml);
    void readWaveTrack(Xml&);
    void readSong(Xml&);
    void readSongfile(Xml& xml);
};

} // namespace MusECore

#endif
