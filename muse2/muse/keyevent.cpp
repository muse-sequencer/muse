//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: key.cpp,v 1.7.2.7 2008/05/21 00:28:52 terminator356 Exp $
//
//  (C) Copyright 1999/2000 Werner Schweer (ws@seh.de)
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

#include <stdio.h>
#include <errno.h>
#include <cmath>

#include "key.h"
#include "globals.h"
#include "gconfig.h"
#include "xml.h"
#include "keyevent.h"

namespace MusEGlobal {
  MusECore::KeyList keymap;
}

namespace MusECore {

#define DEFAULT_KEY KEY_C

//---------------------------------------------------------
//   KeyList
//---------------------------------------------------------

KeyList::KeyList()
      {
      insert(std::pair<const unsigned, KeyEvent> (MAX_TICK+1, KeyEvent(DEFAULT_KEY, 0)));
      }

//---------------------------------------------------------
//   add
//---------------------------------------------------------

void KeyList::add(unsigned tick, key_enum key)
      {
      if (tick > MAX_TICK)
            tick = MAX_TICK;
      iKeyEvent e = upper_bound(tick);

      if (tick == e->second.tick)
            e->second.key = key;
      else {
            KeyEvent& ne = e->second;
            KeyEvent ev = KeyEvent(ne.key, ne.tick);
            ne.key  = key;
            ne.tick   = tick;
            insert(std::pair<const unsigned, KeyEvent> (tick, ev));
            }
      }

//---------------------------------------------------------
//   KeyList::dump
//---------------------------------------------------------

void KeyList::dump() const
      {
      printf("\nKeyList:\n");
      for (ciKeyEvent i = begin(); i != end(); ++i) {
            printf("%6d %06d key %6d\n",
               i->first, i->second.tick, i->second.key);
            }
      }


//---------------------------------------------------------
//   clear
//---------------------------------------------------------

void KeyList::clear()
      {
      KEYLIST::clear();
      insert(std::pair<const unsigned, KeyEvent> (MAX_TICK+1, KeyEvent(DEFAULT_KEY, 0)));
      }

//---------------------------------------------------------
//   keyAtTick
//---------------------------------------------------------

key_enum KeyList::keyAtTick(unsigned tick) const
      {
            ciKeyEvent i = upper_bound(tick);
            if (i == end()) {
                  printf("no key at tick %d,0x%x\n", tick, tick);
                  return DEFAULT_KEY;
                  }
            return i->second.key;
      }

//---------------------------------------------------------
//   del
//---------------------------------------------------------

void KeyList::del(unsigned tick)
      {
      iKeyEvent e = find(tick);
      if (e == end()) {
            printf("KeyList::del(%d): not found\n", tick);
            return;
            }
      del(e);
      }

void KeyList::del(iKeyEvent e)
      {
      iKeyEvent ne = e;
      ++ne;
      if (ne == end()) {
            printf("KeyList::del() HALLO\n");
            return;
            }
      ne->second.key = e->second.key;
      ne->second.tick  = e->second.tick;
      erase(e);
      }

//---------------------------------------------------------
//   change
//---------------------------------------------------------

void KeyList::change(unsigned tick, key_enum newkey)
      {
      iKeyEvent e = find(tick);
      e->second.key = newkey;
      }


//---------------------------------------------------------
//   addKey
//---------------------------------------------------------

void KeyList::addKey(unsigned t, key_enum key)
      {
      add(t, key);
      }

//---------------------------------------------------------
//   delKey
//---------------------------------------------------------

void KeyList::delKey(unsigned tick)
      {
      del(tick);
      }



//---------------------------------------------------------
//   KeyList::write
//---------------------------------------------------------

void KeyList::write(int level, Xml& xml) const
      {
      xml.tag(level, "keylist");
      for (ciKeyEvent i = begin(); i != end(); ++i)
            i->second.write(level, xml, i->first);
      xml.tag(level, "/keylist");
      }

//---------------------------------------------------------
//   KeyList::read
//---------------------------------------------------------

void KeyList::read(Xml& xml)
      {
      for (;;) {
            Xml::Token token = xml.parse();
            const QString& tag = xml.s1();
            switch (token) {
                  case Xml::Error:
                  case Xml::End:
                        return;
                  case Xml::TagStart:
                        if (tag == "key") {
                              KeyEvent t;
                              unsigned tick = t.read(xml);
                              iKeyEvent pos = find(tick);
                              if (pos != end())
                                    erase(pos);
                              insert(std::pair<const int, KeyEvent> (tick, t));
                              }
                        else
                              xml.unknown("keyList");
                        break;
                  case Xml::Attribut:
                        break;
                  case Xml::TagEnd:
                        if (tag == "keylist") {
                              return;
                              }
                  default:
                        break;
                  }
            }
      }

//---------------------------------------------------------
//   KeyEvent::write
//---------------------------------------------------------

void KeyEvent::write(int level, Xml& xml, int at) const
      {
      xml.tag(level++, "key at=\"%d\"", at);
      xml.intTag(level, "tick", tick);
      xml.intTag(level, "val", key);
      xml.tag(level, "/key");
      }

//---------------------------------------------------------
//   KeyEvent::read
//---------------------------------------------------------

int KeyEvent::read(Xml& xml)
      {
      int at = 0;
      for (;;) {
            Xml::Token token = xml.parse();
            const QString& tag = xml.s1();
            switch (token) {
                  case Xml::Error:
                  case Xml::End:
                        return 0;
                  case Xml::TagStart:
                        if (tag == "tick")
                              tick = xml.parseInt();
                        else if (tag == "val")
                              key = key_enum(xml.parseInt());
                        else
                              xml.unknown("KeyEvent");
                        break;
                  case Xml::Attribut:
                        if (tag == "at")
                              at = xml.s2().toInt();
                        break;
                  case Xml::TagEnd:
                        if (tag == "key") {
                              return at;
                              }
                  default:
                        break;
                  }
            }
      return 0;
      }

} // namespace MusECore


