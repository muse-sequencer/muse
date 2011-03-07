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

#include "trackdrag.h"

class Track;
const char TrackDrag::type[] = "application/muse/track";

//---------------------------------------------------------
//   TrackDrag
//---------------------------------------------------------

TrackDrag::TrackDrag(Track* track, QWidget* src)
   : QDrag(src)
      {
      QByteArray a((char*)&track, sizeof(track));
      QMimeData* mimeData = new QMimeData;
      mimeData->setData(type, a);
      setMimeData(mimeData);
      }

//---------------------------------------------------------
//   canDecode
//---------------------------------------------------------

bool TrackDrag::canDecode(const QMimeSource* s)
      {
      return !strcmp(s->format(0), type);
      }

//---------------------------------------------------------
//   decode
//---------------------------------------------------------

bool TrackDrag::decode(const QMimeSource* s, Track*& p)
      {
      QByteArray a = s->encodedData(type);
      p = (Track*)((a[0] & 0xff)
          | (a[1] & 0xff) << 8
          | (a[2] & 0xff) << 16
          | (a[3] & 0xff) << 24);
      return true;
      }

