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

#ifndef __ROUTE_H__
#define __ROUTE_H__

class Track;
namespace AL {
      class Xml;
      };
using AL::Xml;


typedef void* Port;

//---------------------------------------------------------
//   Route
//---------------------------------------------------------

struct Route {
      enum RouteType { TRACK, AUDIOPORT, MIDIPORT, SYNTIPORT};

      union {
            Track* track;
            Port   port;
            };
      int channel;      // route to/from JACK can specify a channel to connect to
      int stream;       // 0 - main, 1-n - aux send
      RouteType type;

      Route();
      Route(const QString&, int ch, RouteType);
      Route(Port, RouteType);
      Route(Track*);
      Route(Track*, RouteType);
      Route(Track*, int, RouteType);
      QString name() const;
      void read(QDomNode node);
      void write(Xml&, const char* name) const;
      static void write(Xml&, const char* name, const Track*);

      bool operator==(const Route& a) const;
      bool isValid() const { return track != 0; }
      void dump() const;
      const char* tname() const;
      static const char* tname(RouteType);
      };


//---------------------------------------------------------
//   RouteList
//---------------------------------------------------------

class RouteList : public std::vector<Route> {
   public:
      void removeRoute(const Route& r);
      };

typedef RouteList::iterator iRoute;
typedef RouteList::const_iterator ciRoute;

extern bool addRoute(Route, Route);
extern void removeRoute(Route, Route);
extern bool checkRoute(const QString&, const QString&);

#endif

