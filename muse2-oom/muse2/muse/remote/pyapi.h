//=========================================================
//  MusE
//  Linux Music Editor
//  (C) Copyright 2009 Mathias Gyllengahm (lunar_shuttle@users.sf.net)
//=========================================================
#ifndef PYAPI_H
#define PYAPI_H

#include <QEvent>

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

#endif

