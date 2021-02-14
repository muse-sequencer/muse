//=========================================================
//  MusE
//  Linux Music Editor
//
//  name_factory.h
//  (C) Copyright 2020 Tim E. Real (terminator356 on sourceforge.net)
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

#ifndef __NAME_FACTORY_H__
#define __NAME_FACTORY_H__

#include <QList>
#include <QStringList>

#include "track.h"

namespace MusECore {
  
//---------------------------------
// NameFactoryBase
//---------------------------------

class NameFactoryBase : public QStringList
{
  protected:
    QStringList _usedNames;

  public:
    NameFactoryBase();
};
  
//----------------------------------------------------------------------------
// TrackNameFactory
// This convenience class generates unique track names from the given type
//  and base name, for a given number of copies.
// To look for existing track names, the song's track list is searched first,
//  followed by searching an internal accumulated 'used names' list.
// This is useful for creating multiple tracks BEFORE they are added to
//  the song's track list. Each generated name is added to the 'used names' list.
//----------------------------------------------------------------------------

class TrackNameFactory : public NameFactoryBase
{
  public:
    TrackNameFactory();
    TrackNameFactory(Track::TrackType type, QString base = QString(), int copies = 1);

    // Returns true on success.
    bool genUniqueNames(
      Track::TrackType type, QString base = QString(), int copies = 1);
};
  
} // namespace MusECore

#endif


