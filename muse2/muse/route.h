//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: route.h,v 1.5.2.1 2008/05/21 00:28:52 terminator356 Exp $
//
//  (C) Copyright 2001 Werner Schweer (ws@seh.de)
//  (C) Copyright 2011 Tim E. Real (terminator356 on sourceforge)
//
//  This program is free software; you can redistribute it and/or
//  modify it under the terms of the GNU General Public License
//  as published by the Free Software Foundation; version 2 of
//  the License, or (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//
//=========================================================

#ifndef __ROUTE_H__
#define __ROUTE_H__

#include <QMetaType>

#include <vector>
#include <map>
#include "globaldefs.h"

#define ROUTE_PERSISTENT_NAME_SIZE 256    // Size of char array Route::persistentName, including the terminating null character.

class QString;

namespace MusECore {

class Track;
class MidiDevice;
class Xml;
class PendingOperationList;

//---------------------------------------------------------
//   Route
//---------------------------------------------------------

struct Route {
      enum RouteType { TRACK_ROUTE=0, JACK_ROUTE=1, MIDI_DEVICE_ROUTE=2, MIDI_PORT_ROUTE=3 }; 
      
      union {
            Track* track;
            MidiDevice* device;      
            void* jackPort;
	    void* voidPointer;
            };
      
      int midiPort;              // Midi port number. Best not to put this in the union to avoid problems?
      
      //snd_seq_addr_t alsaAdr;
      
      // Starting source channel (of the owner of this route). Normally zero for mono or stereo tracks, higher for multi-channel tracks. 
      // NOTICE: channel is now a bit-wise channel mask, for MidiPort <-> MidiTrack routes. 
      // This saves many routes: Instead of one route per channel as before, there can now be only one single route with a channel mask, 
      //  for each MidiPort <-> MidiTrack combination.
      int channel;                             
      // Number of (audio) channels being routed. 
      int channels;
      
      // Allow for multi-channel syntis to feed to/from regular tracks, and to feed one to another. 
      // If a synti is feeding to/from a regular track, remoteChannel is the 'starting' channel of this multi-channel synti.
      // If a synti is feeding to/from another synti, this is not used and individual channels are routed using channel instead.
      int remoteChannel;
      
      RouteType type;

      // REMOVE Tim. Persistent routes. 
      //QString persistentJackPortName; // Always same as the port name. When connection disappears, this holds on to the name.
      // Always same as the port name. When connection disappears, this holds on to the name.
      char persistentJackPortName[ROUTE_PERSISTENT_NAME_SIZE]; 
      
      Route(void* t, int ch=-1);
      Route(Track* t, int ch = -1, int chans = -1);
      Route(MidiDevice* d, int ch);  
      Route(int port, int ch);         
      Route(const QString&, bool dst, int ch, int rtype = -1);
      Route();
      Route(const Route&); // Copy constructor
      // Useful for generic complete construction.
      //Route(RouteType type_, int midi_port_num_, void* void_pointer_, int channel_, int channels_, int remote_channel_, const QString& name_);
      Route(RouteType type_, int midi_port_num_, void* void_pointer_, int channel_, int channels_, int remote_channel_, const char* name_);
      
      QString name() const;
      bool operator==(const Route&) const;
      Route& operator=(const Route&);
      bool isValid() const {
            return ((type == TRACK_ROUTE) && (track != 0)) || 
                   //((type == JACK_ROUTE) && (jackPort != 0)) || 
                   (type == JACK_ROUTE) ||  // Persistent Jack ports: NULL jackPort is actually valid.
                   ((type == MIDI_DEVICE_ROUTE) && (device != 0)) ||
                   ((type == MIDI_PORT_ROUTE) && (midiPort >= 0) && (midiPort < MIDI_PORTS));   
            }
      void read(Xml& xml);
      void dump() const;
      };

//---------------------------------------------------------
//   RouteList
//---------------------------------------------------------

struct RouteList : public std::vector<Route> {
      void removeRoute(const Route& r);
      iterator find(const Route& r);
      };

typedef RouteList::iterator iRoute;
typedef RouteList::const_iterator ciRoute;

extern void addRoute(Route, Route);
// REMOVE Tim. Persistent routes. Added.
//extern void addRouteOperation(Route src, Route dst, PendingOperationList& ops);
extern void removeRoute(Route, Route);
// REMOVE Tim. Persistent routes. Added.
//extern void removeRouteOperation(Route src, Route dst, PendingOperationList& ops);
extern void removeAllRoutes(Route, Route);
extern Route name2route(const QString&, bool dst, int rtype = -1);
extern bool checkRoute(const QString&, const QString&);

} // namespace MusECore

// Allow Routes to be a QVariant
Q_DECLARE_METATYPE(MusECore::Route) ;

#endif

