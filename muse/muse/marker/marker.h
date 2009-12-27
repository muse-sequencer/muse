//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: marker.h,v 1.2 2003/12/15 11:41:00 wschweer Exp $
//  (C) Copyright 2000 Werner Schweer (ws@seh.de)
//=========================================================

#ifndef __MARKER_H__
#define __MARKER_H__

#include <map>
#include <qstring.h>
#include "xml.h"
#include "pos.h"

//---------------------------------------------------------
//   Marker
//---------------------------------------------------------

class Marker : public Pos {
      QString _name;
      bool _current;

   public:
      Marker() : _current(false) {}
      Marker(const QString& s, bool cur = false)
         : _name(s), _current(cur) {}
      void read(Xml&);
      const QString name() const     { return _name; }
      void setName(const QString& s) { _name = s;    }
      bool current() const           { return _current; }
      void setCurrent(bool f)        { _current = f; }
      };

//---------------------------------------------------------
//   MarkerList
//---------------------------------------------------------

class MarkerList : public std::multimap<unsigned, Marker, std::less<unsigned> > {
   public:
      Marker* add(const Marker& m);
      Marker* add(const QString& s, int t, bool lck);
      void write(int, Xml&) const;
      void remove(Marker*);
      };

typedef std::multimap<unsigned, Marker, std::less<unsigned> >::iterator iMarker;
typedef std::multimap<unsigned, Marker, std::less<unsigned> >::const_iterator ciMarker;

#endif

