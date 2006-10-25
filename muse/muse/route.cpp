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

Route::Route()
      {
      track   = 0;
      channel = -1;
      type    = TRACK;
      disconnected = false;
      }

Route::Route(Port p, int ch, RouteType t)
      {
      port    = p;
      channel = ch;
      type    = t;
      disconnected = false;
      }

Route::Route(Port p, RouteType t)
      {
      port    = p;
      channel = -1;
      type    = t;
      disconnected = false;
      }

Route::Route(Track* tr)
      {
      track   = tr;
      channel = -1;
      type    = TRACK;
      disconnected = false;
      }

Route::Route(AuxPluginIF* p)
      {
      plugin  = p;
      channel = -1;
      type    = AUXPLUGIN;
      disconnected = false;
      }

Route::Route(Track* tr, int ch, RouteType t)
      {
      track   = tr;
      channel = ch;
      type    = t;
      disconnected = false;
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
            if (inRoutes->indexOf(src) != -1) {
                  printf("  route already there 1\n");
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

            if (outRoutes->indexOf(dst) != -1) {
                  printf("  route already there 2\n");
                  return true;
                  }
            outRoutes->push_back(dst);
            }
      else if (src.type == Route::AUXPLUGIN) {
            RouteList* inRoutes = dst.track->inRoutes();
            inRoutes->insert(inRoutes->begin(), src);
            }
      else {
            if (src.track->outRoutes()->indexOf(dst) != -1) {
                  printf("  route already there 3\n");
                  return true;
                  }
            src.track->outRoutes()->push_back(dst);
            dst.track->inRoutes()->push_back(src);
            }
      return true;
      }

//---------------------------------------------------------
//   removeRoute
//---------------------------------------------------------

void removeRoute(Route src, Route dst)
      {
//      printf("removeRoute %s.%d:<%s> %s.%d:<%s>\n",
//         src.tname(), src.channel, src.name().toLatin1().data(),
//         dst.tname(), dst.channel, dst.name().toLatin1().data());
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
            case JACKMIDIPORT:
                  if (port.isZero())
                        return QString("0");
                  return audioDriver->portName(port);
            case MIDIPORT:
                  if (port.isZero())
                        return QString("0");
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
      if (!s.isValid()) {   // source port not found?
            printf("Song::readRoute(): invalid source port: %s.%d:<%s> - %s.%d:<%s>\n",
              s.tname(), s.channel, s.name().toLatin1().data(),
              d.tname(), d.channel, d.name().toLatin1().data());
            return;
            }
      if (!d.isValid()) {    // destination port not found?
            printf("Song::readRoute(): invalid destination port: %s.%d:<%s> - %s.%d:<%s>\n",
              s.tname(), s.channel, s.name().toLatin1().data(),
              d.tname(), d.channel, d.name().toLatin1().data());
            return;
            }
      
      if (s.type == Route::AUDIOPORT)
            s.channel = d.channel;
      if (d.type == Route::AUDIOPORT)
            d.channel = s.channel;
      if (s.type == Route::TRACK && s.track->type() == Track::MIDI_IN)
            d.channel = s.channel;
      s.disconnected = true;
      d.disconnected = true;
      addRoute(s, d);
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
            case JACKMIDIPORT:
                  return port == a.port;
            case AUDIOPORT:
                  return (channel == a.channel) && (port == a.port);
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
            if (track == 0)
                  printf("Route::read(): track <%s> not found\n", s.toLatin1().data());
            }
      else if (st == "AUDIOPORT") {
            type = Route::AUDIOPORT;
            port = audioDriver->findPort(s);
            if (port.isZero())
                  printf("Route::read(): audioport <%s> not found\n", s.toLatin1().data());
            }
      else if (st == "JACKMIDIPORT") {
            type = Route::JACKMIDIPORT;
            port = audioDriver->findPort(s);
            if (port.isZero())
                  printf("Route::read(): jack midiport <%s> not found\n", s.toLatin1().data());
            }
      else if (st == "MIDIPORT") {
            type = Route::MIDIPORT;
            port = midiDriver->findPort(s);
            if (port.isZero())
                  printf("Route::read(): midiport <%s> not found\n", s.toLatin1().data());
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
            if (plugin == 0)
                  printf("Route::read(): plugin <%s> not found\n", s.toLatin1().data());
            }
      else {
            printf("Route::read(): unknown type <%s>\n", st.toLatin1().data());
            type = Route::TRACK;
            }
      }

