//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: metronom.cpp,v 1.3 2005/11/16 17:55:59 wschweer Exp $
//
//    metronom  - midi metronom
//
//  (C) Copyright 2005 Werner Schweer (ws@seh.de)
//=========================================================

#include "metronomgui.h"
#include "metronom.h"
#include "midi.h"
#include "midievent.h"

//---------------------------------------------------------
//   Metronom
//---------------------------------------------------------

Metronom::Metronom(const char* name, const MempiHost* h)
   : Mempi(name, h)
      {
      gui = 0;
      }

//---------------------------------------------------------
//   Metronom
//---------------------------------------------------------

Metronom::~Metronom()
      {
      if (gui)
            delete gui;
      }

//---------------------------------------------------------
//   init
//---------------------------------------------------------

bool Metronom::init()
      {
      nextTick         = 0;
      lastTo           = 0;
      data.measureNote = 63;
      data.measureVelo = 127;
      data.beatNote    = 63;
      data.beatVelo    = 70;

      gui = new MetronomGui(this, 0);
      gui->hide();
      gui->setWindowTitle(QString(name()));

      return false;
      }

//---------------------------------------------------------
//   getGeometry
//---------------------------------------------------------

void Metronom::getGeometry(int* x, int* y, int* w, int* h) const
      {
      QPoint pos(gui->pos());
      QSize size(gui->size());
      *x = pos.x();
      *y = pos.y();
      *w = size.width();
      *h = size.height();
      }

//---------------------------------------------------------
//   setGeometry
//---------------------------------------------------------

void Metronom::setGeometry(int x, int y, int w, int h)
      {
      gui->resize(QSize(w, h));
      gui->move(QPoint(x, y));
      }

//---------------------------------------------------------
//   process
//---------------------------------------------------------

void Metronom::process(unsigned from, unsigned to, MPEventList* /*il*/, MPEventList* ol)
      {
      if (from == to) {
            nextTick = 0;
            return;
            }
      if (lastTo != from) {    // seek?
//            printf("  seek? %d-%d\n", lastTo, from);
            nextTick = 0;
            }
      lastTo = to;
      if (nextTick > to)
            return;
      while (nextTick < to) {
            int bar, beat;
            unsigned tick;
            if (nextTick < from) {
                  host->bar(from, &bar, &beat, &tick);
                  if (tick)
                        nextTick = host->bar2tick(bar, beat+1, 0);
                  else
                        nextTick = from;
                  }
            host->bar(nextTick, &bar, &beat, &tick);
            bool isMeasure = beat == 0;

            MidiEvent ev(nextTick, 0, ME_NOTEON, data.beatNote, data.beatVelo);
            if (isMeasure) {
                  ev.setA(data.measureNote);
                  ev.setB(data.measureVelo);
                  }
            ol->insert(ev);   // insert note on
            ev.setB(0);
            ev.setTime(nextTick + 10);
            ev.setB(0);
            ol->insert(ev);   // insert note off

            nextTick = host->bar2tick(bar, beat+1, 0);
            }
      }

//---------------------------------------------------------
//   getInitData
//---------------------------------------------------------

void Metronom::getInitData(int* n, const unsigned char** p) const
      {
      *n = sizeof(data);
      *p = (unsigned char*)&data;
      }

//---------------------------------------------------------
//   setInitData
//---------------------------------------------------------

void Metronom::setInitData(int n, const unsigned char* p)
      {
      memcpy((void*)&data, p, n);
      if (gui)
            gui->init();
      }

//---------------------------------------------------------
//   inst
//---------------------------------------------------------

static Mempi* instantiate(const char* name, const MempiHost* h)
      {
      return new Metronom(name, h);
      }

extern "C" {
      static MEMPI descriptor = {
            "Metronom",
            "MusE Simple Midi Metronom",
            "0.1",      // version string
            MEMPI_GENERATOR,
            MEMPI_MAJOR_VERSION, MEMPI_MINOR_VERSION,
            instantiate
            };

      const MEMPI* mempi_descriptor() { return &descriptor; }
      }

