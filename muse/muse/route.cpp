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

#include "song.h"
#include "route.h"
#include "audio.h"
#include "track.h"
#include "synth.h"
#include "driver/audiodev.h"
#include "driver/mididev.h"
#include "al/xml.h"

//---------------------------------------------------------
//   name2route
//---------------------------------------------------------

static Route name2route(const QString& rn, Route::RouteType t)
      {
      int channel = -1;
      
      // mainly for debugging purposes, it happens that name2route is called without an entry
      // possibly this should be removed if it's no longer the case.
      if (rn.isEmpty()) {
            static const char* names[] = {
                    "TRACK", "AUDIOPORT", "MIDIPORT", "SYNTIPORT"
                    };
            printf("!!!!!!!!!!!!!!!!!!!!!! empty route!!!!!!!!!!!!!!name2route: %s: <%s> not found\n", names[t], rn.toLatin1().data());
            return Route((Track*) 0, channel, Route::TRACK);
            }
      QString s(rn);

      if (rn[0].isNumber() && rn[1]==':') {
            int c = rn[0].toLatin1();
            channel = c - '1';
            s = rn.mid(2);
            }
      else if (rn[0].isNumber() && rn[1].isNumber() && rn[2]==':') {
            channel = (rn[0].toLatin1() - '0') * 10 + (rn[1].toLatin1() - '0') - 1;
            s = rn.mid(3);
            }
      switch (t) {
            case Route::TRACK:
                  {
                  TrackList* tl = song->tracks();
                  for (iTrack i = tl->begin(); i != tl->end(); ++i) {
                        Track* track = *i;
                        if (track->name() == s)
                              return Route(track, channel, Route::TRACK);
                        }
                  MidiChannelList* mc = song->midiChannel();
                  for (iMidiChannel i = mc->begin(); i != mc->end(); ++i) {
                        MidiChannel* t = *i;
                        if (t->name() == s)
                              return Route(t, channel, Route::TRACK);
                        }
                  }
                  break;
            case Route::AUDIOPORT:
                  {
                  Port p = audioDriver->findPort(s);
                  if (p)
                        return Route(p, Route::AUDIOPORT);
                  }
                  break;
            case Route::MIDIPORT:
                  {
                  Port p = midiDriver->findPort(s);
                  if (p)
                        return Route(p, Route::MIDIPORT);
                  }
                  break;
            case Route::SYNTIPORT:
                  {
                  SynthIList* tl = song->syntis();
                  for (iSynthI i = tl->begin(); i != tl->end(); ++i) {
                        SynthI* track = *i;
                        if (track->name() == s)
                              return Route(track, channel, Route::SYNTIPORT);
                        }
                  }

            }
      static const char* names[] = {
            "TRACK", "AUDIOPORT", "MIDIPORT", "SYNTIPORT"
            };
      printf("name2route: %s: <%s> not found\n", names[t], rn.toLatin1().data());
      return Route((Track*) 0, channel, Route::TRACK);
      }

//---------------------------------------------------------
//   Route
//---------------------------------------------------------

Route::Route(Port p, RouteType t)
      {
      port    = p;
      channel = -1;
      stream  = 0;
      type    = t;
      }

Route::Route(Track* tr, RouteType t)
      {
      track   = tr;
      channel = -1;
      stream  = 0;
      type    = t;
      }

Route::Route(Track* tr)
      {
      track   = tr;
      channel = -1;
      stream  = 0;
      type    = TRACK;
      }

Route::Route(Track* tr, int ch, RouteType t)
      {
      track   = tr;
      channel = ch;
      stream  = 0;
      type    = t;
      }

Route::Route(const QString& s, int ch, RouteType t)
      {
      Route node(name2route(s, t));
      if (node.isValid()) {
            channel = node.channel;
            if (channel == -1)
                  channel = ch;
            if (node.type == TRACK)
                  track = node.track;
            else
                  port = node.port;
            type = t;
            }
      }

Route::Route()
      {
      track   = 0;
      channel = -1;
      stream  = 0;
      type    = TRACK;
      }

//---------------------------------------------------------
//   addRoute
//    return false, if route invalid or cannot be found
//---------------------------------------------------------

bool addRoute(Route src, Route dst)
      {
      if (!src.isValid() || !dst.isValid())
            return false;

// printf("addRoute %s.%d:<%s> %s.%d:<%s>\n",
//         src.tname(), src.channel, src.name().toLatin1().data(),
//         dst.tname(), dst.channel, dst.name().toLatin1().data());

      if (src.type == Route::AUDIOPORT || src.type == Route::MIDIPORT) {
            if (dst.type != Route::TRACK) {
                  fprintf(stderr, "addRoute: bad route 1\n");
                  return false;
                  }
            if (dst.track->type() != Track::AUDIO_INPUT && dst.track->type() != Track::MIDI_IN) {
                  fprintf(stderr, "addRoute: bad route 2\n");
                  return false;
                  }
            src.channel = dst.channel;
            RouteList* inRoutes = dst.track->inRoutes();
            for (iRoute i = inRoutes->begin(); i != inRoutes->end(); ++i) {
                  if (*i == src)    // route already there
                        return true;
                  }
            inRoutes->push_back(src);
            }
      else if (dst.type == Route::AUDIOPORT || dst.type == Route::MIDIPORT) {
//         || dst.type == Route::SYNTIPORT) {
            if (src.type != Route::TRACK) {
                  fprintf(stderr, "addRoute: bad route 3\n");
                  return false;
                  }
            if (src.track->type() != Track::AUDIO_OUTPUT && src.track->type() != Track::MIDI_OUT) {
                  fprintf(stderr, "addRoute: bad route 4, tracktype %s\n", src.track->cname().toLatin1().data());
                  return false;
                  }
            RouteList* outRoutes = src.track->outRoutes();
            dst.channel = src.channel;

            for (iRoute i = outRoutes->begin(); i != outRoutes->end(); ++i) {
                  if (*i == dst)    // route already there
                        return true;
                  }
            outRoutes->push_back(dst);
            }
      else {
            RouteList* outRoutes = src.track->outRoutes();
            for (iRoute i = outRoutes->begin(); i != outRoutes->end(); ++i) {
                  if (*i == dst) {    // route already there
                        printf("  route already there\n");
                        return true;
                        }
                  }
            outRoutes->push_back(dst);
            RouteList* inRoutes = dst.track->inRoutes();
            inRoutes->insert(inRoutes->begin(), src);
            }
      return true;
      }

//---------------------------------------------------------
//   removeRoute
//---------------------------------------------------------

void removeRoute(Route src, Route dst)
      {
//    printf("removeRoute %s.%d:<%s> %s.%d:<%s>\n",
//         src.tname(), src.channel, src.name().toLatin1().data(),
//         dst.tname(), dst.channel, dst.name().toLatin1().data());
      if (src.type == Route::AUDIOPORT || src.type == Route::MIDIPORT) {
            if (dst.type != Route::TRACK && dst.type != Route::SYNTIPORT) {
                  fprintf(stderr, "removeRoute: bad route 1\n");
                  goto error;
                  }
            if (dst.track->type() != Track::AUDIO_INPUT
               && dst.track->type() != Track::AUDIO_SOFTSYNTH
               && dst.track->type() != Track::MIDI_IN) {
                  fprintf(stderr, "removeRoute: bad route 2\n");
                  goto error;
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
      else if (dst.type == Route::AUDIOPORT || dst.type == Route::MIDIPORT) {
//         | dst.type == Route::SYNTIPORT) {
            if (src.type != Route::TRACK) {
                  fprintf(stderr, "removeRoute: bad route 3\n");
                  goto error;
                  }
            if (src.track->type() != Track::AUDIO_OUTPUT && src.track->type() != Track::MIDI_OUT) {
                  fprintf(stderr, "removeRoute: bad route 4\n");
                  goto error;
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
      return;
error:
      printf("removeRoute %s.%d:<%s> %s.%d:<%s>\n",
         src.tname(), src.channel, src.name().toLatin1().data(),
         dst.tname(), dst.channel, dst.name().toLatin1().data());
      abort();
      }

//---------------------------------------------------------
//   track2name
//    create string name representation for audio node
//---------------------------------------------------------

static QString track2name(const Track* n)
      {
      return n ? n->name() : "None";
      }

//---------------------------------------------------------
//   name
//    create string name representation for audio node
//---------------------------------------------------------

QString Route::name() const
      {
      switch (type) {
            case TRACK:
            case SYNTIPORT:
                  return track2name(track);
            case AUDIOPORT:
                  return audioDriver->portName(port);
            case MIDIPORT:
                  return midiDriver->portName(port);
            }
      return QString("?");
      }

//---------------------------------------------------------
//   checkRoute
//    return true if route is valid
//---------------------------------------------------------

bool checkRoute(const QString& /*s*/, const QString& /*d*/)
      {
      return true;
      }

//---------------------------------------------------------
//   read
//---------------------------------------------------------

void Song::readRoute(QDomNode n)
      {
      Route s, d;
      for (QDomNode node = n.firstChild(); !node.isNull(); node = node.nextSibling()) {
            QDomElement e = node.toElement();
            if (e.tagName() == "src")
                  s.read(node);
            else if (e.tagName() == "dst")
                  d.read(node);
            else
                  printf("MusE:readRoute: unknown tag %s\n", e.tagName().toLatin1().data());
            }
      addRoute(s, d);
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
      printf("RouteList::internal error: cannot remove Route\n  ");
      r.dump();
      printf("  found:\n");
      for (iRoute i = begin(); i != end(); ++i) {
            printf("  ");
            i->dump();
            }
      printf("  -----\n");
      }

//---------------------------------------------------------
//   dump
//---------------------------------------------------------

void Route::dump() const
      {
      printf("Route %p dump: <%s> channel %d, type %s\n",
         track, name().toLatin1().data(), channel, tname());
      }

//---------------------------------------------------------
//   operator==
//---------------------------------------------------------

bool Route::operator==(const Route& a) const
      {
      if (type == a.type) {
            if (type == TRACK || type == SYNTIPORT) {
                  return channel == a.channel && track == a.track;
                  }
            else if (type == MIDIPORT) {
                  return midiDriver->equal(port, a.port);
                  }
            else if (type == AUDIOPORT) {
                  return channel == a.channel && audioDriver->equal(port, a.port);
                  }
            }
      return false;
      }

//---------------------------------------------------------
//   tname
//---------------------------------------------------------

const char* Route::tname(RouteType t)
      {
      static const char* names[] = {
            "TRACK", "AUDIOPORT", "MIDIPORT", "SYNTIPORT"
            };
      if (t > 3)
            return "???";
      return names[t];
      }

const char* Route::tname() const
      {
      return tname(type);
      }

//---------------------------------------------------------
//   write
//---------------------------------------------------------

void Route::write(Xml& xml, const char* label) const
      {
      xml.put("<%s type=\"%s\" channel=\"%d\" stream=\"%d\" name=\"%s\"/>", 
         label, tname(), channel + 1, stream,  name().toUtf8().data());
      }

//---------------------------------------------------------
//   write
//---------------------------------------------------------

void Route::write(Xml& xml, const char* label, const Track* track)
      {
      xml.put("<%s type=\"TRACK\" channel=\"0\" stream=\"0\" name=\"%s\"/>", 
         label, track->name().toUtf8().data());
      }

//---------------------------------------------------------
//   read
//---------------------------------------------------------

void Route::read(QDomNode node)
      {
      QDomElement e = node.toElement();
      channel       = e.attribute("channel","0").toInt() - 1;
      stream        = e.attribute("stream", "0").toInt();
      QString s     = e.attribute("name");
      QString st    = e.attribute("type", "TRACK");

      if (st == "TRACK") {
            type = Route::TRACK;
            track = 0;
            TrackList* tl = song->tracks();
            for (iTrack i = tl->begin(); i != tl->end(); ++i) {
                  Track* t = *i;
                  if (t->name() == s) {
                        track = t;
                        break;
                        }
                  }
            if (track == 0) {
                  MidiChannelList* mc = song->midiChannel();
                  for (iMidiChannel i = mc->begin(); i != mc->end(); ++i) {
                        MidiChannel* t = *i;
                        if (t->name() == s) {
                              track = t;
                              break;
                              }
                        }
                  }
            }
      else if (st == "AUDIOPORT") {
            type = Route::AUDIOPORT;
            port = audioDriver->findPort(s);
            if (port == 0)
                  printf("Route::read(): audioport not found\n");
            }
      else if (st == "MIDIPORT") {
            type = Route::MIDIPORT;
            port = midiDriver->findPort(s);
            if (port == 0)
                  printf("Route::read(): midiport not found\n");
            }
      else if (st == "SYNTIPORT") {
            type = Route::SYNTIPORT;
            SynthIList* tl = song->syntis();
            for (iSynthI i = tl->begin(); i != tl->end(); ++i) {
                  SynthI* t = *i;
                  if (t->name() == s) {
                        track = t;
                        break;
                        }
                  }
            }
      else {
            printf("Route::read(): unknown type <%s>\n", st.toLatin1().data());
            type = Route::TRACK;
            }
      }

