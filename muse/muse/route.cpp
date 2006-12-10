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

//---------------------------------------------------------
//   RouteNode
//---------------------------------------------------------

RouteNode::RouteNode()
      {
      track   = 0;
      channel = -1;
      type    = TRACK;
      }

RouteNode::RouteNode(Port p, int ch, RouteNodeType t)
      {
      port    = p;
      channel = ch;
      type    = t;
      }

RouteNode::RouteNode(Port p, RouteNodeType t)
      {
      port    = p;
      channel = -1;
      type    = t;
      }

RouteNode::RouteNode(Track* tr)
      {
      track   = tr;
      channel = -1;
      type    = TRACK;
      }

RouteNode::RouteNode(AuxPluginIF* p)
      {
      plugin  = p;
      channel = -1;
      type    = AUXPLUGIN;
      }

RouteNode::RouteNode(Track* tr, int ch, RouteNodeType t)
      {
      track   = tr;
      channel = ch;
      type    = t;
      }

//---------------------------------------------------------
//   addRoute
//    return false, if route invalid or cannot be found
//---------------------------------------------------------

bool addRoute(const Route& r)
      {
// printf("addRoute %s.%d:<%s> %s.%d:<%s>\n",
//         r.src.tname(), r.src.channel, r.src.name().toLatin1().data(),
//         r.dst.tname(), r.dst.channel, r.dst.name().toLatin1().data());

      if (r.src.type == RouteNode::AUDIOPORT || r.src.type == RouteNode::MIDIPORT 
         || r.src.type == RouteNode::JACKMIDIPORT) {
            if (r.dst.type != RouteNode::TRACK) {
                  fprintf(stderr, "addRoute: bad route 1\n");
                  return false;
                  }
            if (r.dst.track->type() != Track::AUDIO_INPUT && r.dst.track->type() != Track::MIDI_IN) {
                  fprintf(stderr, "addRoute: bad route 2\n");
                  return false;
                  }
            RouteList* inRoutes = r.dst.track->inRoutes();
            if (inRoutes->indexOf(r) != -1) {
                  printf("  route already there 1\n");
                  return true;
                  }
            inRoutes->push_back(r);
            }
      else if (r.dst.type == RouteNode::AUDIOPORT || r.dst.type == RouteNode::MIDIPORT
         || r.dst.type == RouteNode::JACKMIDIPORT) {
            if (r.src.type != RouteNode::TRACK) {
                  fprintf(stderr, "addRoute: bad route 3\n");
                  return false;
                  }
            if (r.src.track->type() != Track::AUDIO_OUTPUT && r.src.track->type() != Track::MIDI_OUT) {
                  fprintf(stderr, "addRoute: bad route 4, tracktype %s\n", r.src.track->cname().toLatin1().data());
                  return false;
                  }
            RouteList* outRoutes = r.src.track->outRoutes();
            if (outRoutes->indexOf(r) != -1) {
                  printf("  route already there 2\n");
                  return true;
                  }
            outRoutes->push_back(r);
            }
      else if (r.src.type == RouteNode::AUXPLUGIN) {
            RouteList* inRoutes = r.dst.track->inRoutes();
            inRoutes->insert(inRoutes->begin(), r);
            }
      else {
            if (r.src.track->outRoutes()->indexOf(r) != -1) {
                  printf("  route already there 3\n");
                  return true;
                  }
            r.src.track->outRoutes()->push_back(r);
            r.dst.track->inRoutes()->push_back(r);
            }
      return true;
      }

//---------------------------------------------------------
//   removeRoute
//---------------------------------------------------------

void removeRoute(const Route& r)
      {
//      printf("removeRoute %s.%d:<%s> %s.%d:<%s>\n",
//         r.src.tname(), r.src.channel, r.src.name().toLatin1().data(),
//         r.dst.tname(), r.dst.channel, r.dst.name().toLatin1().data());
      if (r.src.type == RouteNode::AUDIOPORT || r.src.type == RouteNode::MIDIPORT
         || r.src.type == RouteNode::JACKMIDIPORT) {
            if (r.dst.type != RouteNode::TRACK) {
                  fprintf(stderr, "removeRoute: bad route 1\n");
                  goto error;
                  }
            if (r.dst.track->type() != Track::AUDIO_INPUT
               && r.dst.track->type() != Track::AUDIO_SOFTSYNTH
               && r.dst.track->type() != Track::MIDI_IN) {
                  fprintf(stderr, "removeRoute: bad route 2\n");
                  goto error;
                  }
            RouteList* inRoutes = r.dst.track->inRoutes();
            iRoute i;
            for (i = inRoutes->begin(); i != inRoutes->end(); ++i) {
                  if (*i == r) {
                        inRoutes->erase(i);
                        break;
                        }
                  }
            }
      else if (r.dst.type == RouteNode::AUDIOPORT || r.dst.type == RouteNode::MIDIPORT
         || r.dst.type == RouteNode::JACKMIDIPORT) {
            if (r.src.type != RouteNode::TRACK) {
                  fprintf(stderr, "removeRoute: bad route 3\n");
                  goto error;
                  }
            if (r.src.track->type() != Track::AUDIO_OUTPUT && r.src.track->type() != Track::MIDI_OUT) {
                  fprintf(stderr, "removeRoute: bad route 4\n");
                  goto error;
                  }
            RouteList* outRoutes = r.src.track->outRoutes();
            iRoute i;
            for (i = outRoutes->begin(); i != outRoutes->end(); ++i) {
                  if (*i == r) {
                        outRoutes->erase(i);
                        break;
                        }
                  }
            }
      else if (r.src.type == RouteNode::AUXPLUGIN) {
            if (r.dst.type != RouteNode::TRACK) {
                  fprintf(stderr, "removeRoute: bad route 5\n");
                  goto error;
                  }
            RouteList* inRoutes = r.dst.track->inRoutes();
            for (iRoute i = inRoutes->begin(); i != inRoutes->end(); ++i) {
                  if (*i == r) {
                        inRoutes->erase(i);
                        break;
                        }
                  }
            }
      else {
            RouteList* outRoutes = r.src.track->outRoutes();
            for (iRoute i = outRoutes->begin(); i != outRoutes->end(); ++i) {
                  if (*i == r) {
                        outRoutes->erase(i);
                        break;
                        }
                  }
            RouteList* inRoutes = r.dst.track->inRoutes();
            for (iRoute i = inRoutes->begin(); i != inRoutes->end(); ++i) {
                  if (*i == r) {
                        inRoutes->erase(i);
                        break;
                        }
                  }
            }
      return;
error:
      printf("removeRoute %s.%d:<%s> %s.%d:<%s>\n",
         r.src.tname(), r.src.channel, r.src.name().toLatin1().data(),
         r.dst.tname(), r.dst.channel, r.dst.name().toLatin1().data());
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

QString RouteNode::name() const
      {
      switch (type) {
            case TRACK:
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
      RouteNode s, d;
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
      Route r(s, d);
      r.disconnected = true;
      addRoute(r);
      }

//---------------------------------------------------------
//   dump
//---------------------------------------------------------

void RouteNode::dump() const
      {
      printf("RouteNode %p dump: <%s> channel %d, type %s\n",
         track, name().toLatin1().data(), channel, tname());
      }

//---------------------------------------------------------
//   operator==
//---------------------------------------------------------

bool RouteNode::operator==(const RouteNode& a) const
      {
      if (type != a.type)
            return false;
      switch(type) {
            case TRACK:
                  return (channel == a.channel) && (track == a.track);
            case MIDIPORT:
            case JACKMIDIPORT:
            case AUDIOPORT:
                  return port == a.port;
            case AUXPLUGIN:
                  return plugin == a.plugin;
            }
      return false;
      }

//---------------------------------------------------------
//   tname
//---------------------------------------------------------

const char* RouteNode::tname(RouteNodeType t)
      {
      static const char* names[] = {
            "TRACK", "AUDIOPORT", "MIDIPORT", "JACKMIDIPORT", "AUX"
            };
      if (t > (int)(sizeof(names)/sizeof(*names)))
            return "???";
      return names[t];
      }

const char* RouteNode::tname() const
      {
      return tname(type);
      }

//---------------------------------------------------------
//   write
//---------------------------------------------------------

void RouteNode::write(Xml& xml, const char* label) const
      {
      if (channel != -1)
            xml.tagE(QString("%1 type=\"%2\" channel=\"%3\" name=\"%4\"")
               .arg(label).arg(tname()).arg(channel + 1).arg(name()));
      else
            xml.tagE(QString("%1 type=\"%2\" name=\"%3\"")
               .arg(label).arg(tname()).arg(name()));
      }

//---------------------------------------------------------
//   read
//---------------------------------------------------------

void RouteNode::read(QDomNode node)
      {
      QDomElement e = node.toElement();
      channel       = e.attribute("channel","0").toInt() - 1;
      QString s     = e.attribute("name");
      QString st    = e.attribute("type", "TRACK");

      if (st == "TRACK") {
            type = RouteNode::TRACK;
            track = 0;
            TrackList* tl = song->tracks();
            for (iTrack i = tl->begin(); i != tl->end(); ++i) {
                  Track* t = *i;
                  if (t->name() == s) {
                        track = t;
                        break;
                        }
                  }
            if (track == 0)
                  printf("Route::read(): track <%s> not found\n", s.toLatin1().data());
            }
      else if (st == "AUDIOPORT") {
            type = RouteNode::AUDIOPORT;
            port = audioDriver->findPort(s);
            if (port.isZero())
                  printf("Route::read(): audioport <%s> not found\n", s.toLatin1().data());
            }
      else if (st == "JACKMIDIPORT") {
            type = RouteNode::JACKMIDIPORT;
            port = audioDriver->findPort(s);
            if (port.isZero())
                  printf("Route::read(): jack midiport <%s> not found\n", s.toLatin1().data());
            }
      else if (st == "MIDIPORT") {
            type = RouteNode::MIDIPORT;
            port = midiDriver->findPort(s);
            if (port.isZero())
                  printf("Route::read(): midiport <%s> not found\n", s.toLatin1().data());
            }
      else if (st == "AUX") {
            type = RouteNode::AUXPLUGIN;
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
            type = RouteNode::TRACK;
            }
      }

