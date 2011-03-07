//=========================================================
//  MusE
//  Linux Music Editor
//    software synthesizer helper library
//    $Id: poly.h,v 1.2 2004/04/15 13:46:18 wschweer Exp $
//
//  (C) Copyright 2004 Werner Schweer (ws@seh.de)
//=========================================================

#ifndef __SYNTH_POLY_H__
#define __SYNTH_POLY_H

#include <list>
#include "mess.h"

//---------------------------------------------------------
//   MessPoly
//    implements some functions for monophone
//    synthesizer
//---------------------------------------------------------

class MessPoly : public Mess {
      float volume;
      float expression;

      // cached values:
      float mainLevel;

   protected:
      virtual bool playNote(int channel, int pitch, int velo);
      virtual bool setController(int, int, int);

   public:
      MessPoly() : Mess(1) {}
      virtual ~MessPoly() {}
      };

#endif

