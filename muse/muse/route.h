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

#include "driver/driver.h"

// Routing Types:
//
// Audio:
//    Port - Track::Channel         Audio Input
//    Track::Channel - Port         Audio Ouput
//    Track - Track
//    Aux   - Track                 Audio Aux Send
// Midi:
//    Port  - Track                 Midi Input
//    Track - Port                  Midi Output
//    Track - Track
//
// A software synthesizer is somewhat special as it has
//    a midi input and an audio output

//---------------------------------------------------------
//   Route
//    this describes one endpoint of a route
//       Track
//       Track/Channel
//       AuxPlugin
//       Port
//       SYNTI
//---------------------------------------------------------

struct Route {
      enum RouteType { TRACK, AUDIOPORT, MIDIPORT, JACKMIDIPORT,
         SYNTIPORT, AUXPLUGIN};

      Port   port;
      union {
            Track* track;
            AuxPluginIF* plugin;
            };
      int channel;            // route to/from JACK can specify a channel to connect to
      bool disconnected;      // if true, do not remove route in graphChanged()
                              // or removeConnection()

      RouteType type;

      Route();
      Route(Port, int, RouteType);
      Route(Port, RouteType);
      Route(Track*);
      Route(Track*, int, RouteType t = TRACK);
      Route(AuxPluginIF*);

      bool isPortType() const {
            return type==AUDIOPORT || type == MIDIPORT || type == JACKMIDIPORT;
            }
      bool isValid() const {
            return (isPortType() && !port.isZero())
               || ((type == TRACK || type == SYNTIPORT) && track)
               || ((type == AUXPLUGIN) && plugin);
            }
      QString name() const;
      void read(QDomNode node);
      void write(Xml&, const char* name) const;

      bool operator==(const Route& a) const;
      void dump() const;
      const char* tname() const;
      static const char* tname(RouteType);
      };

Q_DECLARE_METATYPE(struct Route);

typedef QList<Route> RouteList;
typedef RouteList::iterator iRoute;
typedef RouteList::const_iterator ciRoute;

extern bool addRoute(Route, Route);
extern void removeRoute(Route, Route);
extern bool checkRoute(const QString&, const QString&);

#endif

