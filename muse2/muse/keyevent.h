//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: tempo.h,v 1.2.2.1 2006/09/19 19:07:09 spamatica Exp $
//
//  (C) Copyright 1999/2000 Werner Schweer (ws@seh.de)
//=========================================================
#ifndef KEYEVENT_H
#define KEYEVENT_H

#include <map>

#ifndef MAX_TICK
#define MAX_TICK (0x7fffffff/100)
#endif

class Xml;

//---------------------------------------------------------
//   Tempo Event
//---------------------------------------------------------

struct KeyEvent {
      int key;
      unsigned tick;    // new tempo at tick
      //unsigned frame;   // precomputed time for tick in sec

      int read(Xml&);
      void write(int, Xml&, int) const;

      KeyEvent() { }
      KeyEvent(unsigned k, unsigned tk) {
            key = k;
            tick  = tk;
            //frame = 0;
            }
      };

//---------------------------------------------------------
//   TempoList
//---------------------------------------------------------

typedef std::map<unsigned, KeyEvent*, std::less<unsigned> > KEYLIST;
typedef KEYLIST::iterator iKeyEvent;
typedef KEYLIST::const_iterator ciKeyEvent;
typedef KEYLIST::reverse_iterator riKeyEvent;
typedef KEYLIST::const_reverse_iterator criKeyEvent;

class KeyList : public KEYLIST {
//      int _keySN;           // serial no to track key changes
      bool useList;
      int _key;             // key if not using key list

      void add(unsigned tick, int tempo);
      void change(unsigned tick, int newKey);
      void del(iKeyEvent);
      void del(unsigned tick);

   public:

      enum keyList {
        keyC,
        keyCis,
        keyD,
        keyDis,
        keyE,
        keyF,
        keyFis,
        keyG,
        keyGis,
        keyA,
        keyB,
        keyBes,
        };


      KeyList();
      void clear();

      void read(Xml&);
      void write(int, Xml&) const;
      void dump() const;

      int key(unsigned tick) const;

      //int keySN() const { return _keySN; }
//      void setKey(unsigned tick, int newKey);
      void addKey(unsigned t, int newKey);
      void delKey(unsigned tick);
//      void changeKey(unsigned tick, int newKey);
      bool setMasterFlag(unsigned tick, bool val);
      };

extern KeyList keymap;


#endif // KEYEVENT_H
