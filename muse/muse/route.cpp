//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: route.cpp,v 1.18.2.3 2008/05/21 00:28:52 terminator356 Exp $
//
//  (C) Copyright 2003-2004 Werner Schweer (ws@seh.de)
//=========================================================

#include <qwidget.h>

#include "song.h"
#include "route.h"
#include "node.h"
#include "audio.h"
#include "track.h"
#include "synth.h"
#include "audiodev.h"
#include "xml.h"

//---------------------------------------------------------
//   Route
//---------------------------------------------------------

Route::Route(void* t, int ch)
      {
      jackPort = t;
      channel  = ch;
      type     = JACK_ROUTE;
      }

Route::Route(AudioTrack* t, int ch)
      {
      channel = ch;
      track   = t;
      type    = TRACK_ROUTE;
      }

Route::Route(const QString& s, bool dst, int ch)
      {
      Route node(name2route(s, dst));
      channel = node.channel;
      if (channel == -1)
            channel = ch;
      type = node.type;
      if (type == TRACK_ROUTE)
            track = node.track;
      else
            jackPort = node.jackPort;
      }

Route::Route()
      {
      track   = 0;
      channel = -1;
      type    = TRACK_ROUTE;
      }

//---------------------------------------------------------
//   addRoute
//---------------------------------------------------------

void addRoute(Route src, Route dst)
      {
      if (!src.isValid() || !dst.isValid())
            return;

//      printf("addRoute %d.%d:<%s> %d.%d:<%s>\n",
//         src.type, src.channel, src.name().latin1(),
//         dst.type, dst.channel, dst.name().latin1());
      if (src.type == JACK_ROUTE) {
            if (dst.type != TRACK_ROUTE) {
                  fprintf(stderr, "addRoute: bad route 1\n");
                  // exit(-1);
                  return;
                  }
            if (dst.track->type() != Track::AUDIO_INPUT) {
                  fprintf(stderr, "addRoute: bad route 2\n");
                  exit(-1);
                  }
            src.channel = dst.channel;
            RouteList* inRoutes = dst.track->inRoutes();
            for (iRoute i = inRoutes->begin(); i != inRoutes->end(); ++i) {
                  if (*i == src)    // route already there
                        return;
                  }
            inRoutes->push_back(src);
            }
      else if (dst.type == JACK_ROUTE) {
            if (src.type != TRACK_ROUTE) {
                  fprintf(stderr, "addRoute: bad route 3\n");
                  // exit(-1);
                  return;
                  }
            if (src.track->type() != Track::AUDIO_OUTPUT) {
                  fprintf(stderr, "addRoute: bad route 4\n");
                  // exit(-1);
                  return;
                  }
            RouteList* outRoutes = src.track->outRoutes();
            dst.channel = src.channel;
            for (iRoute i = outRoutes->begin(); i != outRoutes->end(); ++i) {
                  if (*i == dst)    // route already there
                        return;
                  }
            outRoutes->push_back(dst);
            }
      else {
            RouteList* outRoutes = src.track->outRoutes();
            for (iRoute i = outRoutes->begin(); i != outRoutes->end(); ++i) {
                  if (*i == dst)    // route already there
                        return;
                  }
            outRoutes->push_back(dst);
            RouteList* inRoutes = dst.track->inRoutes();
            //
            // make sure AUDIO_AUX is processed last
            //
            if (src.track->type() == Track::AUDIO_AUX)
                  inRoutes->push_back(src);
            else
                  inRoutes->insert(inRoutes->begin(), src);
            }
      }

//---------------------------------------------------------
//   removeRoute
//---------------------------------------------------------

void removeRoute(Route src, Route dst)
      {
    //printf("removeRoute %d.%d:<%s> %d.%d:<%s>\n",
    //     src.type, src.channel, src.name().latin1(),
    //     dst.type, dst.channel, dst.name().latin1());
         
      if (src.type == JACK_ROUTE) {
            if (dst.type != TRACK_ROUTE) {
                  fprintf(stderr, "removeRoute: bad route 1\n");
                  // exit(-1);
                  return;
                  }
            if (dst.track->type() != Track::AUDIO_INPUT) {
                  fprintf(stderr, "removeRoute: bad route 2\n");
                  // exit(-1);
                  return;
                  }
            RouteList* inRoutes = dst.track->inRoutes();
            iRoute i;
            for (i = inRoutes->begin(); i != inRoutes->end(); ++i) {
                  if (*i == src) {
                        inRoutes->erase(i);
                        break;
                        }
                  }
            }
      else if (dst.type == JACK_ROUTE) {
            if (src.type != TRACK_ROUTE) {
                  fprintf(stderr, "removeRoute: bad route 3\n");
                  // exit(-1);
                  return;
                  }
            if (src.track->type() != Track::AUDIO_OUTPUT) {
                  fprintf(stderr, "removeRoute: bad route 4\n");
                  // exit(-1);
                  return;
                  }
            RouteList* outRoutes = src.track->outRoutes();
            iRoute i;
            for (i = outRoutes->begin(); i != outRoutes->end(); ++i) {
                  if (*i == dst) {
                        outRoutes->erase(i);
                        break;
                        }
                  }
            }
      else {
            RouteList* outRoutes = src.track->outRoutes();
            for (iRoute i = outRoutes->begin(); i != outRoutes->end(); ++i) {
                  if (*i == dst) {
                        outRoutes->erase(i);
                        break;
                        }
                  }
            RouteList* inRoutes = dst.track->inRoutes();
            for (iRoute i = inRoutes->begin(); i != inRoutes->end(); ++i) {
                  if (*i == src) {
                        inRoutes->erase(i);
                        break;
                        }
                  }
            }
      }

//---------------------------------------------------------
//   track2name
//    create string name representation for audio node
//---------------------------------------------------------

static QString track2name(const Track* n)
      {
      if (n == 0)
            return QWidget::tr("None");
      return n->name();
      }

//---------------------------------------------------------
//   name
//    create string name representation for audio node
//---------------------------------------------------------

QString Route::name() const
      {
      QString s;
      if ((type == TRACK_ROUTE) && (channel != -1)) {
//      if (channel != -1) {
            QString c;
            c.setNum(channel+1);
            s = c + ":";
            }
      if (type == JACK_ROUTE) {
            if (!checkAudioDevice()) return "";
            return s + audioDevice->portName(jackPort);
            }
      else
            return s + track2name(track);
      }

//---------------------------------------------------------
//   name2route
//---------------------------------------------------------

Route name2route(const QString& rn, bool dst)
      {
// printf("name2route %s\n", rn.latin1());
      int channel = -1;
      QString s(rn);
      if (rn[0].isNumber() && rn[1]==':') {
            channel = rn[0] - '1';
            s = rn.mid(2);
            }
      if (dst) {
            TrackList* tl = song->tracks();
            for (iTrack i = tl->begin(); i != tl->end(); ++i) {
                  if ((*i)->isMidiTrack())
                        continue;
                  AudioTrack* track = (AudioTrack*)*i;
                  if (track->name() == s)
                        return Route(track, channel);
                  }
            if (!checkAudioDevice()) return Route((AudioTrack*)NULL,0);
            void* p = audioDevice->findPort(s.latin1());
            if (p)
                  return Route(p, channel);
            }
      else {
            if (!checkAudioDevice()) return Route((AudioTrack*)NULL,0);
            void* p = audioDevice->findPort(s.latin1());
            if (p)
                  return Route(p, channel);
            TrackList* tl = song->tracks();
            for (iTrack i = tl->begin(); i != tl->end(); ++i) {
                  if ((*i)->isMidiTrack())
                        continue;
                  AudioTrack* track = (AudioTrack*)*i;
                  if (track->name() == s)
                        return Route(track, channel);
                  }
            }
      printf("  name2route: <%s> not found\n", rn.latin1());
      return Route((Track*) 0, channel);
      }

//---------------------------------------------------------
//   checkRoute
//    return true if route is valid
//---------------------------------------------------------

bool checkRoute(const QString& s, const QString& d)
      {
      Route src(s, false, -1);
      Route dst(d, true, -1);

      if (!(src.isValid() && dst.isValid()) || (src == dst))
            return false;
      if (src.type == JACK_ROUTE) {
            if (dst.type != TRACK_ROUTE) {
                  return false;
                  }
            if (dst.track->type() != Track::AUDIO_INPUT) {
                  return false;
                  }
            src.channel = dst.channel;
            RouteList* inRoutes = dst.track->inRoutes();
            for (iRoute i = inRoutes->begin(); i != inRoutes->end(); ++i) {
                  if (*i == src) {   // route already there
                        return false;
                        }
                  }
            }
      else if (dst.type == JACK_ROUTE) {
            if (src.type != TRACK_ROUTE) {
                  return false;
                  }
            if (src.track->type() != Track::AUDIO_OUTPUT) {
                  return false;
                  }
            RouteList* outRoutes = src.track->outRoutes();
            dst.channel = src.channel;
            for (iRoute i = outRoutes->begin(); i != outRoutes->end(); ++i) {
                  if (*i == dst) {   // route already there
                        return false;
                        }
                  }
            }
      else {
            RouteList* outRoutes = src.track->outRoutes();
            for (iRoute i = outRoutes->begin(); i != outRoutes->end(); ++i) {
                  if (*i == dst) {   // route already there
                        return false;
                        }
                  }
            }
      return true;
      }

//---------------------------------------------------------
//   read
//---------------------------------------------------------

void Song::readRoute(Xml& xml)
      {
      QString src;
      QString dst;

      for (;;) {
            const QString& tag = xml.s1();
            Xml::Token token = xml.parse();
            switch (token) {
                  case Xml::Error:
                  case Xml::End:
                        return;
                  case Xml::TagStart:
                        if (tag == "srcNode")
                              src = xml.parse1();
                        else if (tag == "dstNode")
                              dst = xml.parse1();
                        else
                              xml.unknown("readRoute");
                        break;
                  case Xml::TagEnd:
                        if (xml.s1() == "Route") {
                              
                              if(!src.isEmpty() && !dst.isEmpty())
                              {
                                
                                Route s = name2route(src, false);
                                Route d = name2route(dst, true);
                                addRoute(s, d);
                              
                              }  
                              else
                                printf("  Warning - route name missing. Ignoring route!\n");
                              
                              return;
                              }
                  default:
                        break;
                  }
            }
      }

//---------------------------------------------------------
//   removeRoute
//---------------------------------------------------------

void RouteList::removeRoute(const Route& r)
      {
      for (iRoute i = begin(); i != end(); ++i) {
            if (r == *i) {
                  erase(i);
                  return;
                  }
            }
      printf("internal error: cannot remove Route\n");
      }

//---------------------------------------------------------
//   dump
//---------------------------------------------------------

void Route::dump() const
      {
      if (type == 0)
            printf("Route dump: track <%s> channel %d\n", track->name().latin1(), channel);
      else {
            if (!checkAudioDevice()) return;
            printf("Route dump: jPort <%s> channel %d\n",
               audioDevice->portName(jackPort).latin1(), channel);
            }
      }

//---------------------------------------------------------
//   operator==
//---------------------------------------------------------

bool Route::operator==(const Route& a) const
      {
      if ((type == a.type) && (channel == a.channel)) {
            if (type == 0)
                  return track == a.track;
            else {
                  if (!checkAudioDevice()) return false;
                  return audioDevice->portName(jackPort) == audioDevice->portName(a.jackPort);
                  }
            }
      return false;
      }

