//=========================================================
//  MusE
//  Linux Music Editor
//    software synthesizer helper library
//    $Id: mono.h,v 1.4 2004/04/15 13:46:18 wschweer Exp $
//
//  (C) Copyright 2004 Werner Schweer (ws@seh.de)
//=========================================================

#ifndef __SYNTH_MONO_H__
#define __SYNTH_MONO_H

#include <list>
#include "mess.h"

//---------------------------------------------------------
//   PitchVelo
//---------------------------------------------------------

struct PitchVelo {
      signed char channel;
      signed char pitch;
      signed char velo;
      PitchVelo(signed char a, signed char b, signed char c)
         : channel(a), pitch(b), velo(c) {}
      };

//---------------------------------------------------------
//   MessMono
//    implements some functions for monophone
//    synthesizer
//---------------------------------------------------------

class MessMono : public Mess {
      std::list<PitchVelo> pitchStack;

   protected:
      virtual bool playNote(int channel, int pitch, int velo);
      virtual void note(int channel, int pitch, int velo) = 0;

   public:
      MessMono() : Mess(1) {}
      virtual ~MessMono() {}
      };

#endif

