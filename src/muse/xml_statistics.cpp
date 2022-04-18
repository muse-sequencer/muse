//=========================================================
//  MusE
//  Linux Music Editor
//
//  xml_statistics.cpp
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

#include "xml_statistics.h"

// Forwards from header:
#include "track.h"
#include "part.h"

namespace MusECore {

Part* XmlWriteStatistics::findClonemasterPart(const QUuid& cloneUuid) const
{
  for(std::set<Part*>::const_iterator ip = _parts.cbegin(); ip != _parts.cend(); ++ip)
  {
    if((*ip)->clonemaster_uuid() == cloneUuid)
      return *ip;
  }
  return nullptr;
}

bool XmlWriteStatistics::clonemasterPartExists(const QUuid& cloneUuid) const
{
  for(std::set<Part*>::const_iterator ip = _parts.cbegin(); ip != _parts.cend(); ++ip)
  {
    if((*ip)->clonemaster_uuid() == cloneUuid)
      return true;
  }
  return false;
}

int XmlWriteStatistics::cloneIDCount() const
{
  // We want the clone id count to start at 0 if empty and 0 with one item,
  //  1 with two items, 2 with 3 items etc.
  if(_parts.empty())
    return 0;
  return _parts.size() - 1;
}


XmlReadStatsStruct::XmlReadStatsStruct(Part* part, int cloneNum)
  : _part(part), _cloneNum(cloneNum)
{
}

Part* XmlReadStatistics::findClonemasterPart(const QUuid& cloneUuid) const
{
  for(std::vector<XmlReadStatsStruct>::const_iterator ip = _parts.cbegin(); ip != _parts.cend(); ++ip)
  {
    if(ip->_part->clonemaster_uuid() == cloneUuid)
      return ip->_part;
  }
  return nullptr;
}

bool XmlReadStatistics::clonemasterPartExists(const QUuid& cloneUuid) const
{
  for(std::vector<XmlReadStatsStruct>::const_iterator ip = _parts.cbegin(); ip != _parts.cend(); ++ip)
  {
    if(ip->_part->clonemaster_uuid() == cloneUuid)
      return true;
  }
  return false;
}

Part* XmlReadStatistics::findCloneNum(int cloneNum) const
{
  for(std::vector<XmlReadStatsStruct>::const_iterator ip = _parts.cbegin(); ip != _parts.cend(); ++ip)
  {
    if(ip->_cloneNum == cloneNum)
      return ip->_part;
  }
  return nullptr;
}

bool XmlReadStatistics::cloneNumExists(int cloneNum) const
{
  for(std::vector<XmlReadStatsStruct>::const_iterator ip = _parts.cbegin(); ip != _parts.cend(); ++ip)
  {
    if(ip->_cloneNum == cloneNum)
      return true;
  }
  return false;
}


} // namespace MusECore
