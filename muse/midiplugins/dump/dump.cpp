//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: filter.cpp,v 1.10 2005/11/06 17:49:34 wschweer Exp $
//
//    dump- simple midi event dump for testing purposes
//
//  (C) Copyright 2006 Werner Schweer (ws@seh.de)
//=========================================================

#include "dump.h"
#include "midi.h"

//---------------------------------------------------------
//   process
//---------------------------------------------------------

void Dump::process(unsigned, unsigned, MidiEventList* il, MidiEventList* ol)
      {
      for (iMidiEvent i = il->begin(); i != il->end(); ++i) {
            printf("Event %6d ch:%2d type:%2d 0x%02x 0x%02x\n", i->time(), i->channel(), 
               i->type(), i->dataA(), i->dataB());
            ol->insert(*i);
            }
      }

//---------------------------------------------------------
//   inst
//---------------------------------------------------------

static Mempi* instantiate(const char* name, const MempiHost* h)
      {
      return new Dump(name, h);
      }

extern "C" {
      static MEMPI descriptor = {
            "Dump",
            "MusE Simple Midi Event Dump",
            "0.1",                  // version string
            MEMPI_FILTER,           // plugin type
            MEMPI_MAJOR_VERSION, MEMPI_MINOR_VERSION,
            instantiate
            };

      const MEMPI* mempi_descriptor() { return &descriptor; }
      }

