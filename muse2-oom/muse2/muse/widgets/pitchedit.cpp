//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: pitchedit.cpp,v 1.2 2004/01/09 17:12:54 wschweer Exp $
//  (C) Copyright 2001 Werner Schweer (ws@seh.de)
//=========================================================

#include <stdio.h>
#include "pitchedit.h"
#include "helper.h"

//---------------------------------------------------------
//   PitchEdit
//---------------------------------------------------------

PitchEdit::PitchEdit(QWidget* parent)
  : QSpinBox(parent)
      {
      setMinimum(0);
      setMaximum(127);
      setSingleStep(1);
      deltaMode = false;
      }

//---------------------------------------------------------
//   mapValueToText
//---------------------------------------------------------

QString PitchEdit::mapValueToText(int v)
      {
      if (deltaMode) {
            QString s;
            s.setNum(v);
            return s;
            }
      else
            return pitch2string(v);
      }

//---------------------------------------------------------
//   mapTextToValue
//---------------------------------------------------------

int PitchEdit::mapTextToValue(bool* ok)
      {
      printf("PitchEdit: mapTextToValue: not impl.\n");
      if (ok)
            *ok = false;
      return 0;
      }

//---------------------------------------------------------
//   setDeltaMode
//---------------------------------------------------------

void PitchEdit::setDeltaMode(bool val)
      {
      deltaMode = val;
      if (deltaMode)
            setRange(-127, 127);
      else
            setRange(0, 127);
      }

