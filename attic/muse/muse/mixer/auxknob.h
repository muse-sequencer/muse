//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: auxknob.h,v 1.3 2003/11/08 15:10:18 wschweer Exp $
//
//  (C) Copyright 2000 Werner Schweer (ws@seh.de)
//=========================================================

#ifndef __AUXKNOB_H__
#define __AUXKNOB_H__

#include "knob.h"

//---------------------------------------------------------
//   AuxKnob
//---------------------------------------------------------

class AuxKnob : public Knob {
      Q_OBJECT
      int idx;

   private slots:
      void valueChanged(double v);

   signals:
      void auxChanged(int, double);

   public:
      AuxKnob(QWidget* parent, int idx);
      };

#endif

