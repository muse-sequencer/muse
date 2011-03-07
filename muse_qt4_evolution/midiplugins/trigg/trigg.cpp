//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: filter.cpp,v 1.10 2005/11/06 17:49:34 wschweer Exp $
//
//    filter  - simple midi filter
//
//  (C) Copyright 2005 Werner Schweer (ws@seh.de)
//=========================================================

#include "trigggui.h"
#include "trigg.h"
#include "midi.h"
#include "midievent.h"

//---------------------------------------------------------
//   Trigg
//---------------------------------------------------------

Trigg::Trigg(const char* name, const MempiHost* h)
   : Mempi(name, h)
      {
      data.note=30;    // allow any events
      data.velocity=127;
      gui = 0;
      }

//---------------------------------------------------------
//   Trigg
//---------------------------------------------------------

Trigg::~Trigg()
      {
      if (gui)
            delete gui;
      }

//---------------------------------------------------------
//   init
//---------------------------------------------------------

bool Trigg::init()
      {
      gui = new TriggGui(this, 0);
      gui->setWindowTitle("MusE: "+QString(name()));
      gui->show();
      return false;
      }

//---------------------------------------------------------
//   getGeometry
//---------------------------------------------------------

void Trigg::getGeometry(int* x, int* y, int* w, int* h) const
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

void Trigg::setGeometry(int x, int y, int w, int h)
      {
      gui->resize(QSize(w, h));
      gui->move(QPoint(x, y));
      }

//---------------------------------------------------------
//   process
//---------------------------------------------------------

void Trigg::process(unsigned , unsigned , MidiEventList* il, MidiEventList* ol)
      {

      for (iMidiEvent i = il->begin(); i != il->end(); ++i) {
            MidiEvent temp=*i;
            if (temp.isNote() || temp.isNoteOff())
                    {
                    // for each event modify note and velocity
                    printf("a=%d b=%d isNote=%d isNoteOff=%d\n",temp.dataA(),temp.dataB(),temp.isNote(),temp.isNoteOff());
                    temp.setA(data.note);
                    if (!temp.isNoteOff()) 
                        temp.setB(data.velocity);
                    printf("AFTER a=%d b=%d\n",temp.dataA(),temp.dataB());
                    }
            ol->insert(temp);
            }
      }

void Trigg::getInitData(int* n, const unsigned char** p) const
      {
      *n = sizeof(data);
      *p = (unsigned char*)&data;
      printf("::getInitData note=%d vel=%d\n",data.note,data.velocity);
      }

void Trigg::setInitData(int n, const unsigned char* p)
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
      return new Trigg(name, h);
      }

extern "C" {
      static MEMPI descriptor = {
            "Trigg",
            "Any note triggers a specified note with specified velocity",
            "1.0",                  // filter version string
            MEMPI_FILTER,           // plugin type
            MEMPI_MAJOR_VERSION, MEMPI_MINOR_VERSION,
            instantiate
            };

      const MEMPI* mempi_descriptor() { return &descriptor; }
      }

