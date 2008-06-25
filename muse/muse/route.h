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
class AuxPluginIF;

namespace AL {
      class Xml;
      };
using AL::Xml;

#include "driver.h"

//---------------------------------------------------------
//   RouteNode
//    this describes one endpoint of a route
//       Track
//       Track/Channel
//       AuxPlugin
//       Port
//---------------------------------------------------------

struct RouteNode {
      enum RouteNodeType {
            TRACK, AUDIOPORT, JACKMIDIPORT, AUXPLUGIN
            };

      Port   port;
      union {
            Track* track;
            AuxPluginIF* plugin;
            };
      int channel;
      RouteNodeType type;

      RouteNode();
      RouteNode(Port, int, RouteNodeType);
      RouteNode(Port, RouteNodeType);
      RouteNode(Track*);
      RouteNode(Track*, int, RouteNodeType t = TRACK);
      RouteNode(AuxPluginIF*);

      bool isPortType() const {
            return type==AUDIOPORT || type == JACKMIDIPORT;
            }
      bool isValid() const {
            return (isPortType() && !port.isZero())
               || ((type == TRACK) && track)
               || ((type == AUXPLUGIN) && plugin);
            }
      QString name() const;
      void read(QDomNode node);
      void write(Xml&, const char* name) const;

      bool operator==(const RouteNode& a) const;
      void dump() const;
      const char* tname() const;
      static const char* tname(RouteNodeType);
      };

//---------------------------------------------------------
//    Route
//---------------------------------------------------------

struct Route {
      RouteNode src;
      RouteNode dst;
      bool disconnected;      // if true, do not remove route in graphChanged()
                              // or removeConnection()

      Route() { disconnected = false;}
      Route(const RouteNode& s, const RouteNode& d) : src(s), dst(d) { disconnected = false;}
      bool operator==(const Route& a) const {
            return (src==a.src) && (dst==a.dst);
            }
      };

Q_DECLARE_METATYPE(struct Route);

typedef QList<Route> RouteList;
typedef RouteList::iterator iRoute;
typedef RouteList::const_iterator ciRoute;

extern bool addRoute(const Route&);
extern void removeRoute(const Route&);

#endif

