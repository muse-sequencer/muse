//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: fluid.h,v 1.7.2.4 2009/11/19 04:20:33 terminator356 Exp $
//
//  This file is derived from fluid Synth and modified
//    for MusE.
//  Parts of fluid are derived from Smurf Sound Font Editor.
//  Parts of Smurf Sound Font Editor are derived from
//    awesfx utilities
//  Smurf:  Copyright (C) 1999-2000 Josh Green
//  fluid:   Copyright (C) 2001 Peter Hanappe
//  MusE:   Copyright (C) 2001 Werner Schweer
//  awesfx: Copyright (C) 1996-1999 Takashi Iwai
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

#ifndef _SYNTH_H
#define _SYNTH_H

#include <list>
#include <fluidsynth.h>
#include "libsynti/mess.h"
#include "common_defs.h"

//enum SfOp { SF_REPLACE = 1, SF_ADD, SF_REMOVE };
class FLUIDGui;

//---------------------------------------------------------
//   ISynth
//---------------------------------------------------------

class ISynth : public Mess {
      bool _busy;
      bool _gmMode;

      unsigned char* initBuffer;
      int initLen;

      fluid_synth_t* _fluidsynth;
      char* sfont;
      mutable fluid_sfont_t* fluid_font;
      int fontId;

      int readFd, writeFd;

      mutable MidiPatch patch;

      pthread_t helperThread;
      FLUIDGui* gui;

      void gmOn(bool);
      void sysexSoundFont(SfOp op, const char* data);

      void allNotesOff();
      void resetAllController(int);

      virtual void processMessages();
      virtual void process(float**, int, int);
      virtual bool playNote(int channel, int pitch, int velo);
	virtual bool setController(int, int, int);
      virtual bool sysex(int len, const unsigned char* p);
    
      virtual bool processEvent(const MusECore::MidiPlayEvent&);
      virtual const char* getPatchName (int, int, int, bool) const;
      virtual const MidiPatch* getPatchInfo(int, const MidiPatch *) const;
      virtual void getInitData(int*, const unsigned char**);

      //virtual bool guiVisible() const;
      //virtual void showGui(bool);
      //virtual bool hasGui() const { return true; }
      virtual bool nativeGuiVisible() const;
      virtual void showNativeGui(bool);
      virtual bool hasNativeGui() const { return true; }

   public:
      ISynth();
      virtual ~ISynth();

      // This is only a kludge required to support old songs' midistates. Do not use in any new synth.
      // Note for Fluid, do nothing because unlike other synths, Fluid already had correct sysex headers.
      //virtual int oldMidiStateHeader(const unsigned char** data) const;  
      
      fluid_synth_t* synth() { return _fluidsynth; }
      const fluid_synth_t* synth() const { return _fluidsynth; }
      char* getFont() const  { return sfont; }
      void setFontId(int id)  { fontId = id; }
      int getFontId() const   { return fontId; }
      bool init(const char* name);
      void noRTHelper();
      };

#endif  /* _SYNTH_H */
