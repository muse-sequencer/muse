//=========================================================
//  MusE
//  Linux Music Editor
//
//  xml_statistics.h
//  Copyright (C) 2021 Tim E. Real (terminator356 on users dot sourceforge dot net)
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

#ifndef __XML_STATISTICS_H__
#define __XML_STATISTICS_H__

#include <QUuid>
#include <set>
#include <vector>

namespace MusECore {

class Track;
class Part;

struct XmlWriteStatistics
{
  std::set<Part*> _parts;

  Part* findClonemasterPart(const QUuid&) const;
  bool clonemasterPartExists(const QUuid&) const;
  int cloneIDCount() const;
};

struct XmlReadStatsStruct
{
  Part* _part;
  int _cloneNum;

  // CloneNum is only used for song files. Other operations use part uuids.
  XmlReadStatsStruct(Part* part, int cloneNum = -1);
};

struct XmlReadStatistics
{
  // The order in the list gives the clone group counter.
  std::vector<XmlReadStatsStruct> _parts;

  Part* findClonemasterPart(const QUuid&) const;
  bool clonemasterPartExists(const QUuid&) const;
  Part* findCloneNum(int cloneNum) const;
  bool cloneNumExists(int cloneNum) const;
};

}   // namespace MusECore

#endif
