//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: panknob.h,v 1.3 2003/11/08 15:10:18 wschweer Exp $
//
//  (C) Copyright 2000 Werner Schweer (ws@seh.de)
//=========================================================

#ifndef __PANKNOB_H__
#define __PANKNOB_H__

#include "knob.h"

class AudioTrack;

//---------------------------------------------------------
//   PanKnob
//---------------------------------------------------------

class PanKnob : public Knob {
      Q_OBJECT
      AudioTrack* src;

   private slots:
      void valueChanged(double);

   public:
      PanKnob(QWidget* parent, AudioTrack*);
      };

#endif

