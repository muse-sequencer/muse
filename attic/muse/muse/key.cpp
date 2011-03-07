//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: key.cpp,v 1.1.1.1 2003/10/27 18:51:22 wschweer Exp $
//
//  (C) Copyright 1999/2000 Werner Schweer (ws@seh.de)
//=========================================================

#include <stdio.h>
#include "key.h"
#include "globals.h"

int NKey::offsets[14] = {
      0, 7, 14, -7,
      -(12),
      -19, -26, -10, -14, -2, -4, -6, -8, 0
      };

int NKey::width() const
      {
      return 25;
      }

//---------------------------------------------------------
//   Scale::width
//---------------------------------------------------------

int Scale::width() const
      {
      int i = val;
      if (i < 0)
           i = -i;
      return i * 7;
      }

