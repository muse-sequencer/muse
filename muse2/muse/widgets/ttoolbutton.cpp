//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: ttoolbutton.cpp,v 1.1 2004/02/21 16:53:50 wschweer Exp $
//  (C) Copyright 2004 Werner Schweer (ws@seh.de)
//=========================================================

#include <QPainter>
#include <QPixmap>
#include <QStyle>

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
      QIcon::Mode mode = isEnabled() ? QIcon::Normal : QIcon::Disabled;
      QIcon::State state = isChecked() ? QIcon::On : QIcon::Off;
      const QPixmap pm(icon().pixmap(style()->pixelMetric(QStyle::PM_SmallIconSize), mode, state));
      p->drawPixmap(QPoint((w - pm.width())/2, (h - pm.height())/2), pm);
      }


