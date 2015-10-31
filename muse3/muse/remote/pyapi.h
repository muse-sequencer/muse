//=========================================================
//  MusE
//  Linux Music Editor
//  (C) Copyright 2009 Mathias Gyllengahm (lunar_shuttle@users.sf.net)
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
#ifndef PYAPI_H
#define PYAPI_H

#include <QEvent>

namespace MusECore {

class QPybridgeEvent : public QEvent
{
public:
      enum EventType { SONG_UPDATE=0, SONGLEN_CHANGE, SONG_POSCHANGE, SONG_SETPLAY, SONG_SETSTOP, SONG_REWIND, SONG_SETMUTE,
             SONG_SETCTRL, SONG_SETAUDIOVOL, SONG_IMPORT_PART, SONG_TOGGLE_EFFECT, SONG_ADD_TRACK, SONG_CHANGE_TRACKNAME,
             SONG_DELETE_TRACK };
      QPybridgeEvent( QPybridgeEvent::EventType _type, int _p1=0, int _p2=0);
      EventType getType() { return type; }
      int getP1() { return p1; }
      int getP2() { return p2; }
      void setS1(QString in) { s1 = in; }
      void setS2(QString in) { s2 = in; }
      const QString& getS1() { return s1; }
      const QString& getS2() { return s2; }
      double getD1() { return d1; }
      void setD1(double _d1) { d1 = _d1; }

private:
      EventType type;
      int p1, p2;
      double d1;
      QString s1;
      QString s2;

};

bool initPythonBridge();

} // namespace MusECore

#endif

