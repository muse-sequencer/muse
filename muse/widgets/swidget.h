//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: swidget.h,v 1.1.1.1 2003/10/27 18:54:49 wschweer Exp $
//  (C) Copyright 1999 Werner Schweer (ws@seh.de)
//=========================================================

#ifndef __SWIDGET_H__
#define __SWIDGET_H__

#include <QWidget>

//---------------------------------------------------------
//   SWidget
//    a simple widget which emits a heighChanged signal
//    on received ResizeEvent´s
//---------------------------------------------------------

class SWidget : public QWidget {
      virtual void resizeEvent(QResizeEvent*);
      Q_OBJECT

   signals:
      void heightChanged(int);

   public:
      SWidget(QWidget* parent) : QWidget(parent) {}
      };

#endif

