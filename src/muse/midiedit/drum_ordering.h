//=========================================================
//  MusE
//  Linux Music Editor
//
//  (C) Copyright 1999/2000 Werner Schweer (ws@seh.de)
//  drum_ordering.h
//  Moved out of drummap.h by Tim.
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

#ifndef __DRUM_ORDERING_H__
#define __DRUM_ORDERING_H__

#include <QString>
#include <QList>
#include <utility>

#include "xml.h"
#include "track.h"

namespace MusEGlobal {

class global_drum_ordering_t : public QList< std::pair<MusECore::MidiTrack*,int> >
{
  public:
    void cleanup();
    void write(int level, MusECore::Xml& xml);
    void read(MusECore::Xml& xml);
  
  private:
    typedef std::pair<MusECore::MidiTrack*,int> entry_t;
    
    void write_single(int level, MusECore::Xml& xml, const entry_t& entry);
    entry_t read_item(MusECore::Xml& xml);
    // OBSOLETE. Keep for backwards compatibility.
    entry_t read_single(MusECore::Xml& xml);
};

extern global_drum_ordering_t global_drum_ordering;

}

#endif

