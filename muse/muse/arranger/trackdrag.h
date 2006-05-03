//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: trackdrag.h,v 1.4 2005/09/26 18:26:20 wschweer Exp $
//
//  (C) Copyright 2004 Werner Schweer (ws@seh.de)
//=========================================================

#ifndef __TRACKDRAG_H__
#define __TRACKDRAG_H__

class Track;

//---------------------------------------------------------
//   TrackDrag
//---------------------------------------------------------

class TrackDrag : public QDrag {
      static const char type[];
      Q_OBJECT

   public:
      TrackDrag(Track*, QWidget* src);
      static bool canDecode(const QMimeSource*);
      static bool decode(const QMimeSource* s, Track*& p);
      };


#endif

