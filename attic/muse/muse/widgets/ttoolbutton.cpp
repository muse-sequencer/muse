//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: ttoolbutton.cpp,v 1.1 2004/02/21 16:53:50 wschweer Exp $
//  (C) Copyright 2004 Werner Schweer (ws@seh.de)
//=========================================================

#include <qpainter.h>

#include "ttoolbutton.h"
#include "gconfig.h"
#include "icons.h"

//---------------------------------------------------------
//   drawButton
//---------------------------------------------------------

void TransparentToolButton::drawButton(QPainter* p)
      {
      int w = width();
      int h = height();
      QIconSet::Mode mode = isEnabled() ? QIconSet::Normal : QIconSet::Disabled;
      QIconSet::State state = isOn() ? QIconSet::On : QIconSet::Off;
      const QPixmap pm(iconSet().pixmap(QIconSet::Automatic, mode, state));
      p->drawPixmap(QPoint((w - pm.width())/2, (h - pm.height())/2), pm);
      }


