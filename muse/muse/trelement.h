//=============================================================================
//  MusE
//  Linux Music Editor
//  $Id:$
//
//  Copyright (C) 2002-2006 by Werner Schweer and others
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License version 2.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//=============================================================================

#ifndef __TR_ELEMENT_H__
#define __TR_ELEMENT_H__

#include "track.h"

//---------------------------------------------------------
//   TrElement
//    describes a configurable gui element of the
//    track list
//---------------------------------------------------------

enum {
      TR_NAME, TR_MUTE, TR_OFF, TR_SOLO, TR_RECORD, TR_AREAD, TR_AWRITE,
      TR_OCHANNEL, TR_MONITOR, TR_DRUMMAP, TR_INSTRUMENT, TR_PATCH
      };

struct TrElement {
      int id;
      int grp;          // default group
      const char* name;
      int trackMask;

      TrElement(int i, int g, const char* s, int m)
        : id(i), grp(g), name(s), trackMask(m) {}
      };

//---------------------------------------------------------
//   TrGroup
//    TrElements are grouped
//---------------------------------------------------------

typedef std::list<const TrElement*> TrElementList;
typedef TrElementList::iterator iTrElement;

class TrGroupList : public std::list<TrElementList> {
      };
typedef TrGroupList::iterator iTrGroup;

extern const TrElement trElements[];
extern const int nTrElements;
extern TrGroupList glist[Track::TRACK_TYPES];

#endif
