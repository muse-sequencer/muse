//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: siglabel.h,v 1.1.1.1 2003/10/27 18:54:56 wschweer Exp $
//  (C) Copyright 1999 Werner Schweer (ws@seh.de)
//=========================================================

#ifndef __SIGLABEL_H__
#define __SIGLABEL_H__

#include <qlabel.h>
//Added by qt3to4:
#include <QWheelEvent>
#include <QMouseEvent>

//---------------------------------------------------------
//   SigLabel
//    show/edit time signature
//---------------------------------------------------------

class SigLabel : public QLabel {
      Q_OBJECT
      virtual void mousePressEvent(QMouseEvent*);
      virtual void wheelEvent(QWheelEvent*);
      void incValue(bool zaehler, bool inc, int&, int&);

   protected:
      int z, n;

   signals:
      void valueChanged(int, int);

   public slots:
      virtual void setValue(int, int);

   public:
      SigLabel(int z, int n, QWidget*);
      void value(int& a, int& b) const { a = z; b = n; }
      void setFrame(bool);
      };
#endif

