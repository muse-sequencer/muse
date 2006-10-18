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
#include "auxplugin.h"
#include "midichannel.h"

//---------------------------------------------------------
//   Route
//---------------------------------------------------------

Route::Route(Port p, int ch, RouteType t)
      {
      port    = p;
      channel = ch;
      type    = t;
      }

Route::Route(Port p, RouteType t)
      {
      port    = p;
      channel = -1;
      type    = t;
      }

Route::Route(Track* tr)
      {
      track   = tr;
      channel = -1;
      type    = TRACK;
      }

Route::Route(AuxPluginIF* p)
      {
      plugin  = p;
      channel = -1;
      type    = AUXPLUGIN;
      }

Route::Route(Track* tr, int ch, RouteType t)
      {
      track   = tr;
      channel = ch;
      type    = t;
      }

Route::Route()
      {
      track   = 0;
      channel = -1;
      type    = TRACK;
      }

//---------------------------------------------------------
//   addRoute
//    return false, if route invalid or cannot be found
//---------------------------------------------------------

bool addRoute(Route src, Route dst)
      {
// printf("addRoute %s.%d:<%s> %s.%d:<%s>\n",
//         src.tname(), src.channel, src.name().toLatin1().data(),
//         dst.tname(), dst.channel, dst.name().toLatin1().data());

      if (src.type == Route::AUDIOPORT || src.type == Route::MIDIPORT 
         || src.type == Route::JACKMIDIPORT) {
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
      else if (dst.type == Route::AUDIOPORT || dst.type == Route::MIDIPORT
         || dst.type == Route::JACKMIDIPORT) {
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
      else if (src.type == Route::AUXPLUGIN) {
            RouteList* inRoutes = dst.track->inRoutes();
            inRoutes->insert(inRoutes->begin(), src);
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
      if (src.type == Route::AUDIOPORT || src.type == Route::MIDIPORT
         || src.type == Route::JACKMIDIPORT) {
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
      else if (dst.type == Route::AUDIOPORT || dst.type == Route::MIDIPORT
         || dst.type == Route::JACKMIDIPORT) {
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
      else if (src.type == Route::AUXPLUGIN) {
            if (dst.type != Route::TRACK) {
                  fprintf(stderr, "removeRoute: bad route 5\n");
                  goto error;
                  }
            RouteList* inRoutes = dst.track->inRoutes();
            for (iRoute i = inRoutes->begin(); i != inRoutes->end(); ++i) {
                  if (*i == src) {
                        inRoutes->erase(i);
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
            case JACKMIDIPORT:
                  return audioDriver->portName(port);
            case MIDIPORT:
                  return midiDriver->portName(port);
            case AUXPLUGIN:
                  return plugin->pluginInstance()->name();
            }
      return QString("?");
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
      if (s.type == Route::AUDIOPORT)
            s.channel = d.channel;
      if (d.type == Route::AUDIOPORT)
            d.channel = s.channel;
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
      if (type != a.type)
            return false;
      switch(type) {
            case TRACK:
            case SYNTIPORT:
                  return channel == a.channel && track == a.track;
            case MIDIPORT:
                  return midiDriver->equal(port, a.port);
            case JACKMIDIPORT:
                  return audioDriver->equal(port, a.port);
            case AUDIOPORT:
                  return channel == a.channel && audioDriver->equal(port, a.port);
            case AUXPLUGIN:
                  return plugin == a.plugin;
            }
      return false;
      }

//---------------------------------------------------------
//   tname
//---------------------------------------------------------

const char* Route::tname(RouteType t)
      {
      static const char* names[] = {
            "TRACK", "AUDIOPORT", "MIDIPORT", "JACKMIDIPORT", 
            "SYNTIPORT", "AUX"
            };
      if (t > (int)(sizeof(names)/sizeof(*names)))
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
      switch (type) {
            case AUDIOPORT:
            case JACKMIDIPORT:
            case MIDIPORT:
            case AUXPLUGIN:
                  xml.put("<%s type=\"%s\" name=\"%s\"/>", 
                     label, tname(), name().toUtf8().data());
                  break;
            case TRACK:
            case SYNTIPORT:
                  if (channel != -1)
                        xml.put("<%s type=\"%s\" channel=\"%d\" name=\"%s\"/>", 
                              label, tname(), channel + 1, name().toUtf8().data());
                  else
                        xml.put("<%s type=\"%s\" name=\"%s\"/>", 
                              label, tname(), name().toUtf8().data());

                  break;
            }
      }

//---------------------------------------------------------
//   read
//---------------------------------------------------------

void Route::read(QDomNode node)
      {
      QDomElement e = node.toElement();
      channel       = e.attribute("channel","0").toInt() - 1;
//      int stream    = e.attribute("stream", "0").toInt();
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
      else if (st == "JACKMIDIPORT") {
            type = Route::JACKMIDIPORT;
            port = audioDriver->findPort(s);
            if (port == 0)
                  printf("Route::read(): jack midiport not found\n");
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
      else if (st == "AUX") {
            type = Route::AUXPLUGIN;
            plugin = 0;
            TrackList* tl = song->tracks();
            for (iTrack i = tl->begin(); i != tl->end(); ++i) {
                  if ((*i)->type() == Track::WAVE || (*i)->type() == Track::AUDIO_INPUT) {
                        AudioTrack* t = (AudioTrack*)*i;
                        QList<AuxPluginIF*> pl = t->preAux();
                        foreach(AuxPluginIF* p, pl) {
                              if (p->pluginInstance()->name() == s) {
                                    plugin = p;
                                    break;
                                    }
                              }
                        if (plugin)
                              break;
                        pl = t->postAux();
                        foreach(AuxPluginIF* p, pl) {
                              if (p->pluginInstance()->name() == s) {
                                    plugin = p;
                                    break;
                                    }
                              }
                        if (plugin)
                              break;
                        }
                  }
            }
      else {
            printf("Route::read(): unknown type <%s>\n", st.toLatin1().data());
            type = Route::TRACK;
            }
      }

//---------------------------------------------------------
//   findTrack
//---------------------------------------------------------

Track* Song::findTrack(const QString& s) const
      {
      TrackList* tl = song->tracks();
      for (iTrack i = tl->begin(); i != tl->end(); ++i) {
            Track* track = *i;
            if (track->name() == s)
                  return track;
            }
      MidiChannelList* mc = song->midiChannel();
      for (iMidiChannel i = mc->begin(); i != mc->end(); ++i) {
            MidiChannel* t = *i;
            if (t->name() == s)
                  return t;
            }
      printf("track <%s> not found\n", s.toLatin1().data());
      return 0;
      }
