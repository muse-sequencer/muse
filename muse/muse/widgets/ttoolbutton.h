//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: ttoolbutton.h,v 1.1 2004/02/21 16:53:51 wschweer Exp $
//  (C) Copyright 2004 Werner Schweer (ws@seh.de)
//=========================================================

#ifndef __TTOOLBUTTON_H__
#define __TTOOLBUTTON_H__

#include <qtoolbutton.h>

//---------------------------------------------------------
//   TransparentToolButton
//---------------------------------------------------------

class TransparentToolButton : public QToolButton {
      Q_OBJECT

      virtual void drawButton(QPainter*);

   public:
      TransparentToolButton(QWidget* parent, const char* name = 0)
         : QToolButton(parent, name) {}
      };

#endif

