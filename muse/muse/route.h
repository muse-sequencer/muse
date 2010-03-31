//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: route.h,v 1.5.2.1 2008/05/21 00:28:52 terminator356 Exp $
//
//  (C) Copyright 2001 Werner Schweer (ws@seh.de)
//=========================================================

#ifndef __ROUTE_H__
#define __ROUTE_H__

//#include <alsa/asoundlib.h>
#include <vector>
#include <map>

class QString;
//class AudioTrack;
class Track;
//class MidiJackDevice;
class MidiDevice;
class Xml;

//---------------------------------------------------------
//   Route
//---------------------------------------------------------

//enum { TRACK_ROUTE=0, JACK_ROUTE=1 };

struct Route {
      //enum { TRACK_ROUTE=0, JACK_ROUTE=1, JACK_MIDI_ROUTE=2, ALSA_MIDI_ROUTE=3 };
      enum { TRACK_ROUTE=0, JACK_ROUTE=1, MIDI_DEVICE_ROUTE=2 };
      
      union {
            //AudioTrack* track;
            Track* track;
            //MidiJackDevice* device;
            MidiDevice* device;
            void* jackPort;
            };
      
      //snd_seq_addr_t alsaAdr;
      
      // Starting source channel (of the owner of this route). Normally zero for mono or stereo tracks, higher for multi-channel tracks. 
      int channel;
      // Number of channels being routed. 
      int channels;
      
      // Allow for multi-channel syntis to feed to/from regular tracks, and to feed one to another. 
      // If a synti is feeding to/from a regular track, remoteChannel is the 'starting' channel of this multi-channel synti.
      // If a synti is feeding to/from another synti, this is not used and individual channels are routed using channel instead.
      int remoteChannel;
      
      unsigned char type;     // 0 - track, 1 - jackPort, 2 - jack midi device, 3 - alsa midi device

      Route(void* t, int ch=-1);
      //Route(AudioTrack* t, int ch);
      //Route(Track* t, int ch);
      Route(Track* t, int ch = -1, int chans = -1);
      //Route(Track* t, int ch = -1);
      
      //Route(MidiJackDevice* d);
      Route(MidiDevice* d, int ch);
      //Route(const QString&, bool dst, int ch);
      Route(const QString&, bool dst, int ch, int rtype = -1);
      Route();
      QString name() const;
      bool operator==(const Route& a) const;
      bool isValid() const {
            //return ((type == 0) && (track != 0)) || ((type == 1) && (jackPort != 0));
            return ((type == TRACK_ROUTE) && (track != 0)) || ((type == JACK_ROUTE) && (jackPort != 0)) || 
                   //(((type == JACK_MIDI_ROUTE) || (type == ALSA_MIDI_ROUTE)) && (device != 0));
                   ((type == MIDI_DEVICE_ROUTE) && (device != 0));
            }
      void read(Xml& xml);
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
//extern Route name2route(const QString&, bool dst);
extern Route name2route(const QString&, bool dst, int rtype = -1);
extern bool checkRoute(const QString&, const QString&);

//---------------------------------------------------------
//   RouteMenuMap
//---------------------------------------------------------

//struct TRouteMenuMap{
//       Route r;
//       };
//typedef std::map<int, TRouteMenuMap, std::less<int> >::iterator iRouteMenuMap;
//typedef std::map<int, TRouteMenuMap, std::less<int> >::const_iterator ciRouteMenuMap;
//typedef std::map<int, TRouteMenuMap, std::less<int> > RouteMenuMap;
typedef std::map<int, Route, std::less<int> >::iterator iRouteMenuMap;
typedef std::map<int, Route, std::less<int> >::const_iterator ciRouteMenuMap;
typedef std::map<int, Route, std::less<int> > RouteMenuMap;
typedef std::pair<int, Route> pRouteMenuMap;
typedef std::pair<iRouteMenuMap, bool > rpRouteMenuMap;

#endif

