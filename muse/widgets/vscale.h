//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: vscale.h,v 1.1.1.1.2.1 2008/01/19 13:33:47 wschweer Exp $
//  (C) Copyright 1999 Werner Schweer (ws@seh.de)
//=========================================================

#ifndef __VSCALE_H__
#define __VSCALE_H__

#include <QWidget>

class QPaintEvent;

//---------------------------------------------------------
//   VScale
//---------------------------------------------------------

class VScale : public QWidget {
      Q_OBJECT

      virtual void paintEvent(QPaintEvent*);

   public:
      VScale(QWidget* parent=0) : QWidget(parent) {setFixedWidth(18);}
      };

#endif

