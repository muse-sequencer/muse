//=========================================================
//  MusE
//  Linux Music Editor
//    software synthesizer helper library
//    $Id: poly.cpp,v 1.3 2004/06/01 14:25:50 wschweer Exp $
//
//  (C) Copyright 2004 Werner Schweer (ws@seh.de)
//=========================================================

#include "poly.h"
#include "muse/midictrl.h"

//---------------------------------------------------------
//   playNote
//---------------------------------------------------------

bool MessPoly::playNote(int /*channel*/, int /*pitch*/, int /*velo*/)
      {
      return false;
      }

//---------------------------------------------------------
//   setController
//---------------------------------------------------------

bool MessPoly::setController(int /*channel*/, int num, int /*val*/)
      {
      switch(num) {
            case CTRL_VOLUME:
            case CTRL_EXPRESSION:
                  break;
            }
      return false;
      }

