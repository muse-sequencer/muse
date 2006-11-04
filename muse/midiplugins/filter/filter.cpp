//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: filter.cpp,v 1.10 2005/11/06 17:49:34 wschweer Exp $
//
//    filter  - simple midi filter
//
//  (C) Copyright 2005 Werner Schweer (ws@seh.de)
//=========================================================

#include "filtergui.h"
#include "filter.h"
#include "midi.h"

//---------------------------------------------------------
//   Filter
//---------------------------------------------------------

Filter::Filter(const char* name, const MempiHost* h)
   : Mempi(name, h)
      {
      data.type = 0;    // allow any events
      for (int i = 0; i < 4; ++i)
            data.ctrl[i] = -1;
      gui = 0;
      }

//---------------------------------------------------------
//   Filter
//---------------------------------------------------------

Filter::~Filter()
      {
      if (gui)
            delete gui;
      }

//---------------------------------------------------------
//   init
//---------------------------------------------------------

bool Filter::init()
      {
      gui = new FilterGui(this, 0);
      gui->setWindowTitle("MusE: "+QString(name()));
      gui->show();
      return false;
      }

//---------------------------------------------------------
//   getGeometry
//---------------------------------------------------------

void Filter::getGeometry(int* x, int* y, int* w, int* h) const
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

void Filter::setGeometry(int x, int y, int w, int h)
      {
      gui->resize(QSize(w, h));
      gui->move(QPoint(x, y));
      }

//---------------------------------------------------------
//   process
//---------------------------------------------------------

void Filter::process(unsigned, unsigned, MidiEventList* il, MidiEventList* ol)
      {
      for (iMidiEvent i = il->begin(); i != il->end(); ++i) {
            if (!filterEvent(*i))
                  ol->insert(*i);
            }
      }

//---------------------------------------------------------
//   filterEvent
//    return true if event filtered
//---------------------------------------------------------

bool Filter::filterEvent(const MidiEvent& event)
      {
      switch(event.type()) {
            case ME_NOTEON:
            case ME_NOTEOFF:
                  if (data.type & MIDI_FILTER_NOTEON)
                        return true;
                  break;
            case ME_POLYAFTER:
                  if (data.type & MIDI_FILTER_POLYP)
                        return true;
                  break;
            case ME_CONTROLLER:
                  if (data.type & MIDI_FILTER_CTRL)
                        return true;
                  for (int i = 0; i < 4; ++i) {
                        if (data.ctrl[i] == event.dataA())
                              return true;
                        }
                  break;
            case ME_PROGRAM:
                  if (data.type & MIDI_FILTER_PROGRAM)
                        return true;
                  break;
            case ME_AFTERTOUCH:
                  if (data.type & MIDI_FILTER_AT)
                        return true;
                  break;
            case ME_PITCHBEND:
                  if (data.type & MIDI_FILTER_PITCH)
                        return true;
                  break;
            case ME_SYSEX:
                  if (data.type & MIDI_FILTER_SYSEX)
                        return true;
                  break;
            default:
                  break;
            }
      return false;
      }

void Filter::getInitData(int* n, const unsigned char** p) const
      {
      *n = sizeof(data);
      *p = (unsigned char*)&data;
      }

void Filter::setInitData(int n, const unsigned char* p)
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
      return new Filter(name, h);
      }

extern "C" {
      static MEMPI descriptor = {
            "Filter",
            "MusE Simple Midi Filter",
            "0.1",                  // filter version string
            MEMPI_FILTER,           // plugin type
            MEMPI_MAJOR_VERSION, MEMPI_MINOR_VERSION,
            instantiate
            };

      const MEMPI* mempi_descriptor() { return &descriptor; }
      }

