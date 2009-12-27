//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: route.h,v 1.5.2.1 2008/05/21 00:28:52 terminator356 Exp $
//
//  (C) Copyright 2001 Werner Schweer (ws@seh.de)
//=========================================================

#ifndef __ROUTE_H__
#define __ROUTE_H__

#include <vector>

class QString;
class AudioTrack;

//---------------------------------------------------------
//   Route
//---------------------------------------------------------

enum { TRACK_ROUTE=0, JACK_ROUTE=1 };

struct Route {
      union {
            AudioTrack* track;
            void* jackPort;
            };
      int channel;
      unsigned char type;     // 0 - track, 1 - jackPort

      Route(void* t, int ch=-1);
      Route(AudioTrack* t, int ch);
      Route(const QString&, bool dst, int ch);
      Route();
      QString name() const;
      bool operator==(const Route& a) const;
      bool isValid() const {
            return ((type == 0) && (track != 0)) || ((type == 1) && (jackPort != 0));
            }
      void dump() const;
      };


//---------------------------------------------------------
//   RouteList
//---------------------------------------------------------

struct RouteList : public std::vector<Route> {
      void removeRoute(const Route& r);
      };

typedef RouteList::iterator iRoute;
typedef RouteList::const_iterator ciRoute;

extern void addRoute(Route, Route);
extern void removeRoute(Route, Route);
extern Route name2route(const QString&, bool dst);
extern bool checkRoute(const QString&, const QString&);

#endif

