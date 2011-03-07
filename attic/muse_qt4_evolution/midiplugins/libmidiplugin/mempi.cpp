//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: mempi.cpp,v 1.4 2005/05/24 15:27:48 wschweer Exp $
//  (C) Copyright 2005 Werner Schweer (ws@seh.de)
//=========================================================

#include "mempi.h"

static const int FIFO_SIZE = 128;

//---------------------------------------------------------
//   MidiEvent
//---------------------------------------------------------

MidiEvent::MidiEvent(unsigned t, int tpe, const unsigned char* data, int len)
      {
      _time = t;
      edata.setData(data, len);
      _type = tpe;
      }

//---------------------------------------------------------
//   operator <
//---------------------------------------------------------

bool MidiEvent::operator<(const MidiEvent& e) const
      {
      if (time() != e.time())
            return time() < e.time();

      // play note off events first to prevent overlapping
      // notes

      if (channel() == e.channel())
            return type() == 0x80
               || (type() == 0x90 && dataB() == 0);

      int map[16] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 0, 10, 11, 12, 13, 14, 15 };
      return map[channel()] < map[e.channel()];
      }

//---------------------------------------------------------
//   MempiP
//    Mempi private data structure
//---------------------------------------------------------

struct MempiP {
      int dummy;
      };

//---------------------------------------------------------
//   Mempi
//---------------------------------------------------------

Mempi::Mempi(const char* n, const MempiHost* h)
      {
      _name = strdup(n);
      host  = h;
      d     = new MempiP;
      }

Mempi::~Mempi()
      {
      delete _name;
      delete d;
      }

//---------------------------------------------------------
//   getGeometry
//    dummy
//---------------------------------------------------------

void Mempi::getGeometry(int* x, int* y, int* w, int* h) const
      {
      x = 0;
      y = 0;
      w = 0;
      h = 0;
      }

