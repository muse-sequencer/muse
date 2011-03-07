//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: swidget.cpp,v 1.1.1.1 2003/10/27 18:54:27 wschweer Exp $
//  (C) Copyright 1999 Werner Schweer (ws@seh.de)
//=========================================================

#include "swidget.h"

#include <QResizeEvent>

//---------------------------------------------------------
//    resizeEvent
//---------------------------------------------------------

void SWidget::resizeEvent(QResizeEvent* ev)
      {
      emit heightChanged(ev->size().height());
      }

