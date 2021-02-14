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
#include "muse_math.h"

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
      insert(std::pair<const unsigned, KeyEvent> (MAX_TICK+1, KeyEvent(DEFAULT_KEY, 0, false)));
      }

//---------------------------------------------------------
//   copy
//---------------------------------------------------------

void KeyList::copy(const KeyList& src)
{
  // Clear the existing destination list.
  KEYLIST::clear();

  for (ciKeyEvent i = src.cbegin(); i != src.cend(); ++i)
  {
    KeyEvent new_e = KeyEvent(i->second);
    std::pair<iKeyEvent, bool> res = insert(std::pair<const unsigned, KeyEvent> (i->first, new_e));
    if(!res.second)
    {
      fprintf(stderr, "KeyList::copy insert failed: keylist:%p key:%d tick:%d minor:%d\n", 
                        this, new_e.key, new_e.tick, new_e.minor);
    }
  }
}

//---------------------------------------------------------
//   add
//---------------------------------------------------------

void KeyList::add(unsigned tick, key_enum key, bool isMinor)
      {
      if (tick > MAX_TICK)
            tick = MAX_TICK;
      iKeyEvent e = upper_bound(tick);

      if (tick == e->second.tick)
      {
            e->second.key = key;
            e->second.minor = isMinor;
      }
      else {
            KeyEvent& ne = e->second;
            KeyEvent ev = KeyEvent(ne.key, ne.tick, ne.minor);
            ne.key  = key;
            ne.tick   = tick;
            ne.minor   = isMinor;
            insert(std::pair<const unsigned, KeyEvent> (tick, ev));
            }
      }

void KeyList::add(KeyEvent e)
{
  int tick = e.tick;
  key_enum k = e.key;
  bool is_minor = e.minor;
  std::pair<iKeyEvent, bool> res = insert(std::pair<const unsigned, KeyEvent> (tick, e));
  if(!res.second)
  {
    fprintf(stderr, "KeyList::add insert failed: keylist:%p key:%d tick:%d minor:%d\n", 
                      this, e.key, e.tick, e.minor);
  }
  else
  {
    iKeyEvent ike = res.first;
    ++ike; // There is always a 'next' key event - there is always one at index MAX_TICK.
    KeyEvent& ne = ike->second;
    
    // Swap the values. (This is how the key list works.)
    e.key = ne.key;
    e.tick = ne.tick;
    e.minor = ne.minor;
    ne.key = k;
    ne.tick = tick;
    ne.minor = is_minor;
  }
}

//---------------------------------------------------------
//   KeyList::dump
//---------------------------------------------------------

void KeyList::dump() const
      {
      printf("\nKeyList:\n");
      for (ciKeyEvent i = begin(); i != end(); ++i) {
            printf("%6d %06d key %6d minor:%d\n",
               i->first, i->second.tick, i->second.key, i->second.minor);
            }
      }


//---------------------------------------------------------
//   clear
//---------------------------------------------------------

void KeyList::clear()
      {
      KEYLIST::clear();
      insert(std::pair<const unsigned, KeyEvent> (MAX_TICK+1, KeyEvent(DEFAULT_KEY, 0, false)));
      }

//---------------------------------------------------------
//   keyAtTick
//---------------------------------------------------------

KeyEvent KeyList::keyAtTick(unsigned tick) const
      {
            ciKeyEvent i = upper_bound(tick);
            if (i == end()) {
                  printf("no key at tick %d,0x%x\n", tick, tick);
                  return KeyEvent();
                  }
            return i->second;
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
      ne->second.minor = e->second.minor;
      erase(e);
      }

//---------------------------------------------------------
//   addKey
//---------------------------------------------------------

void KeyList::addKey(unsigned t, key_enum key, bool isMinor)
      {
      add(t, key, isMinor);
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

KeyEvent::KeyEvent()
{
  key = DEFAULT_KEY;
  tick = 0;
  minor = false;
}

KeyEvent::KeyEvent(key_enum k, unsigned tk, bool isMinor)
{
  key = k;
  tick  = tk;
  minor = isMinor;
}

//---------------------------------------------------------
//   KeyEvent::write
//---------------------------------------------------------

void KeyEvent::write(int level, Xml& xml, int at) const
      {
      xml.tag(level++, "key at=\"%d\"", at);
      xml.intTag(level, "tick", tick);
      xml.intTag(level, "val", key);
      xml.intTag(level, "minor", minor);
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
                        else if (tag == "minor")
                              minor = xml.parseInt();
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

//don't remove or insert new elements in keyStrs.
//only renaming (keeping the semantic sense) is allowed! (flo)
// Static
const QStringList KeyEvent::keyStrs = QStringList()
                      << "C (sharps)" << "G" << "D" << "A"<< "E" << "B" << "F#"
                      << "C (flats)" << "F"<< "Bb" << "Eb"<< "Ab"<< "Db"<< "Gb"
                      << "Am (sharps)" << "Em" << "Bm" << "F#m" << "C#m" << "G#m" << "D#m"
                      << "Am (flats)" << "Dm" << "Gm" << "Cm" << "Fm" << "Bbm" << "Ebm";

//don't change this function (except when renaming stuff)
// Static
KeyEvent KeyEvent::stringToKey(QString input) //flo
{
	int index = keyStrs.indexOf(input);
	KeyEvent map[]={
  {KEY_C, 0, false}, {KEY_G, 0, false}, {KEY_D, 0, false}, {KEY_A, 0, false},
  {KEY_E, 0, false}, {KEY_B, 0, false}, {KEY_FIS, 0, false},
  {KEY_C_B, 0, false}, {KEY_F, 0, false}, {KEY_BES, 0, false}, {KEY_ES, 0, false},
  {KEY_AS, 0, false}, {KEY_DES, 0, false}, {KEY_GES, 0, false},
    
  {KEY_C, 0, true}, {KEY_G, 0, true}, {KEY_D, 0, true}, {KEY_A, 0, true},
  {KEY_E, 0, true}, {KEY_B, 0, true}, {KEY_FIS, 0, true},
  {KEY_C_B, 0, true}, {KEY_F, 0, true}, {KEY_BES, 0, true}, {KEY_ES, 0, true},
  {KEY_AS, 0, true}, {KEY_DES, 0, true}, {KEY_GES, 0, true}
  };
	return map[index];
}

//don't change the below two functions (except when renaming stuff)
// Static
int KeyEvent::keyToIndex(key_enum key, bool isMinor)
{
  int index=0;
	switch(key)
	{
    case KEY_C:   index= isMinor ? 14 : 0; break;
    case KEY_G:   index= isMinor ? 15 : 1; break;
		case KEY_D:   index= isMinor ? 16 : 2; break;
		case KEY_A:   index= isMinor ? 17 : 3; break;
		case KEY_E:   index= isMinor ? 18 : 4; break;
		case KEY_B:   index= isMinor ? 19 : 5; break;
		case KEY_FIS: index= isMinor ? 20 : 6; break;
		case KEY_C_B: index= isMinor ? 21 : 7; break;
		case KEY_F:   index= isMinor ? 22 : 8; break;
		case KEY_BES: index= isMinor ? 23 : 9; break;
		case KEY_ES:  index= isMinor ? 24 : 10; break;
		case KEY_AS:  index= isMinor ? 25 : 11; break;
		case KEY_DES: index= isMinor ? 26 : 12; break;
		case KEY_GES: index= isMinor ? 27 : 13; break;

		case KEY_SHARP_BEGIN:
		case KEY_SHARP_END:
		case KEY_B_BEGIN:
		case KEY_B_END:
			printf("ILLEGAL FUNCTION CALL: keyToIndex called with key_sharp_begin etc.\n");
      return 0;
			break;
		
		default:
			printf("ILLEGAL FUNCTION CALL: keyToIndex called with illegal key value (not in enum)\n");
      return 0;
	}
	return index;
}

// Static
QString KeyEvent::keyToString(key_enum key, bool isMinor)
{
	return keyStrs[keyToIndex(key, isMinor)];
}

int KeyEvent::keyIndex() const
{
  return keyToIndex(key, minor);
}

QString KeyEvent::keyString() const
{
  return keyToString(key, minor);
}


} // namespace MusECore


