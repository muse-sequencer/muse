//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: ticksynth.h,v 1.1.2.3 2009/12/06 10:05:00 terminator356 Exp $
//  (C) Copyright 2004 Werner Schweer (ws@seh.de)
//=========================================================

#ifndef __TICKSYNTH_H__
#define __TICKSYNTH_H__

#include "synth.h"
extern void initMetronome();
extern void exitMetronome();
class MetronomeSynthI : public SynthI
{
   virtual bool hasAuxSend() const  { return false; }

};
extern MetronomeSynthI* metronome;

#endif

