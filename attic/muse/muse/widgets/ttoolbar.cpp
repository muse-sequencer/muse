//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: ttoolbar.cpp,v 1.1.1.1 2003/10/27 18:54:46 wschweer Exp $
//  (C) Copyright 1999 Werner Schweer (ws@seh.de)
//=========================================================

#include <qaction.h>
#include "globals.h"

//---------------------------------------------------------
//   syncChanged
//---------------------------------------------------------

void syncChanged(bool flag)
      {
      startAction->setEnabled(!flag);
      forwardAction->setEnabled(!flag);
      rewindAction->setEnabled(!flag);
      stopAction->setEnabled(!flag);
      playAction->setEnabled(!flag);
      }


