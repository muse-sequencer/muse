//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: siglabel.h,v 1.5 2005/10/10 19:34:09 wschweer Exp $
//  (C) Copyright 1999 Werner Schweer (ws@seh.de)
//=========================================================

#ifndef __SIGLABEL_H__
#define __SIGLABEL_H__

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
      SigLabel(QWidget* parent = 0);
      void value(int& a, int& b) const { a = z; b = n; }
      void setFrame(bool);
      };
#endif

