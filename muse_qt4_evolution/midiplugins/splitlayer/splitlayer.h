//=============================================================================
//  MusE
//  Linux Music Editor
//  $Id:$
//
//  Copyright (C) 2006 by Werner Schweer
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License version 2.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//=============================================================================

#ifndef __METRONOM_H__
#define __METRONOM_H__

#include "../libmidiplugin/mempi.h"

static const int MIDI_CHANNELS = 16;

//---------------------------------------------------------
//   SplitLayer   - splits and layers for midi input
//---------------------------------------------------------

class SplitLayer : public Mempi {

   protected:
      struct InitData {
            int startVelo[MIDI_CHANNELS];
            int endVelo[MIDI_CHANNELS];
            int startPitch[MIDI_CHANNELS];
            int endPitch[MIDI_CHANNELS];
            int pitchOffset[MIDI_CHANNELS];
            int veloOffset[MIDI_CHANNELS];
            } data;
      int notes[128];   // bitmapped note-on/channel values
      bool learnMode;
      int learnChannel;
      bool learnStartPitch;

      SplitLayerGui* gui;
      friend class SplitLayerGui;

      virtual void process(unsigned, unsigned, MidiEventList*, MidiEventList*);

   public:
      SplitLayer(const char* name, const MempiHost*);
      ~SplitLayer();
      virtual bool init();

      virtual bool hasGui() const      { return true;             }
      virtual bool guiVisible() const  { return gui->isVisible(); }
      virtual void showGui(bool val)   { gui->setShown(val);      }
      virtual void getGeometry(int* x, int* y, int* w, int* h) const;
      virtual void setGeometry(int, int, int, int);

      virtual void getInitData(int*, const unsigned char**) const;
      virtual void setInitData(int, const unsigned char*);
      };

#endif

