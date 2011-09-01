//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: panknob.cpp,v 1.5 2004/01/23 08:41:38 wschweer Exp $
//
//  (C) Copyright 2000 Werner Schweer (ws@seh.de)
//=========================================================

#include "../audio.h"
#include "panknob.h"
#include "track.h"

//---------------------------------------------------------
//   PanKnob
//---------------------------------------------------------

PanKnob::PanKnob(QWidget* parent, AudioTrack* s)
   : Knob(parent, "pan")
      {
      src = s;
      connect(this, SIGNAL(valueChanged(double,int)), SLOT(valueChanged(double)));
      }

//---------------------------------------------------------
//   panChanged
//---------------------------------------------------------

void PanKnob::valueChanged(double val)
      {
      audio->msgSetPan(src, val);
      // p4.0.21 audio->msgXXX waits. Do we really need to?
      //src->setPan(val);
      }


