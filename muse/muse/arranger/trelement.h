//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: trelement.h,v 1.6 2005/05/17 15:07:57 wschweer Exp $
//
//  (C) Copyright 2004 Werner Schweer (ws@seh.de)
//=========================================================

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
      char* name;
      int trackMask;

      TrElement(int i, int g, char* s, int m)
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

