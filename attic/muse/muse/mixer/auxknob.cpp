//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: auxknob.cpp,v 1.7 2004/07/11 16:26:46 wschweer Exp $
//
//  (C) Copyright 2000-2004 Werner Schweer (ws@seh.de)
//=========================================================

#include <cmath>
#include "auxknob.h"
#include "gconfig.h"

//---------------------------------------------------------
//   Aux
//---------------------------------------------------------

AuxKnob::AuxKnob(QWidget* parent, int i)
   : Knob(parent, "aux")
      {
      idx = i;
      setRange(config.minSlider-0.1, 10.0);
      connect(this, SIGNAL(valueChanged(double,int)), SLOT(valueChanged(double)));
      }

//---------------------------------------------------------
//   panChanged
//---------------------------------------------------------

void AuxKnob::valueChanged(double val)
      {
      double vol;
      if (val <= config.minSlider) {
            vol = 0.0;
            val -= 1.0; // display special value "off"
            }
      else
            vol = pow(10.0, val/20.0);
      emit auxChanged(idx, vol);
      }

