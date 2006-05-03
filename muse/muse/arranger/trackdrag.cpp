//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: trackdrag.cpp,v 1.6 2005/11/09 09:03:51 wschweer Exp $
//
//  (C) Copyright 2004 Werner Schweer (ws@seh.de)
//=========================================================

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

