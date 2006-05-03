//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: fluid.h,v 1.8 2005/11/23 13:55:32 wschweer Exp $
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
//=========================================================

#ifndef _SYNTH_H
#define _SYNTH_H

#include <fluidsynth.h>
#include "libsynti/mess.h"

enum SfOp { SF_REPLACE = 1, SF_ADD, SF_REMOVE };
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

      virtual void process(float**, int, int);
      virtual bool playNote(int channel, int pitch, int velo);
	virtual bool setController(int, int, int);
      virtual bool sysex(int len, const unsigned char* p);
	virtual const char* getPatchName (int, int, int) const;
	virtual const MidiPatch* getPatchInfo(int, const MidiPatch *) const;
      virtual void getInitData(int*, const unsigned char**);

      virtual bool guiVisible() const;
      virtual void showGui(bool);
      virtual bool hasGui() const { return true; }

   public:
      ISynth();
      ~ISynth();

      fluid_synth_t* synth() { return _fluidsynth; }
      const fluid_synth_t* synth() const { return _fluidsynth; }
      char* getFont() const  { return sfont; }
      void setFontId(int id)  { fontId = id; }
      int getFontId() const   { return fontId; }
      bool init(const char* name);
      void noRTHelper();
      };

#endif  /* _SYNTH_H */
