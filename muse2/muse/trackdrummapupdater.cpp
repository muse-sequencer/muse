//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: trackdrummapupdater.cpp,v 1.59.2.52 2011/12/27 20:25:58 flo93 Exp $
//
//  (C) Copyright 2011 Florian Jung (florian.a.jung (at) web.de)
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

#include "trackdrummapupdater.h"
#include "song.h"
#include "globals.h"

namespace MusECore {

using MusEGlobal::song;

TrackDrummapUpdater::TrackDrummapUpdater()
{
  connect(song,SIGNAL(songChanged(int, int)), this, SLOT(songChanged(int)));
}

void TrackDrummapUpdater::songChanged(int flags)
{
  if (flags & (SC_TRACK_INSERTED | SC_TRACK_REMOVED | SC_TRACK_MODIFIED |
               SC_PART_INSERTED  | SC_PART_REMOVED  | SC_PART_MODIFIED  |
               SC_EVENT_INSERTED | SC_EVENT_REMOVED | SC_EVENT_MODIFIED ) )
  {
    bool changed=false;
    for (iTrack t=song->tracks()->begin(); t!=song->tracks()->end(); t++)
    {
      MidiTrack* track=dynamic_cast<MidiTrack*>(*t);
      if (track && track->auto_update_drummap())
        changed=true;
    }
    
    if (changed)
    {
      // allow recursion. there will be no more recursion, because this
      // is only executed when something other than SC_DRUMMAP happens
      song->update(SC_DRUMMAP, 0, true); 
    }
    
  }
}

} //namespace MusECore
