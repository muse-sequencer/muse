//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: mlabel.cpp,v 1.1.1.1 2003/10/27 18:55:03 wschweer Exp $
//  (C) Copyright 2000 Werner Schweer (ws@seh.de)
//=========================================================

#include "mlabel.h"
//Added by qt3to4:
#include <QMouseEvent>


void MLabel::mousePressEvent(QMouseEvent*)
      {
      emit mousePressed();
      }

