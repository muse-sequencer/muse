//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: spinbox.h,v 1.1.2.2 2009/02/02 21:38:01 terminator356 Exp $
//    (C) Copyright 2001 Werner Schweer (ws@seh.de)
//=========================================================

// SpinBox:
// Click up/down, or mousewheel, or hit enter with un-modified text (which means enter TWICE for modified text), 
// and the control will give up focus, thereby allowing you to use global shortcut keys afterwards. 
// Up/down keys still keep the focus.
#ifndef __SPINBOX_H__
#define __SPINBOX_H__

#include <qspinbox.h>

//---------------------------------------------------------
//   SpinBox
//---------------------------------------------------------

class SpinBox : public QSpinBox {
      Q_OBJECT

      bool _clearFocus; 

   protected:
      bool eventFilter(QObject* obj, QEvent* ev);
      
   public slots:
      virtual void stepUp();
      virtual void stepDown();
   
   signals:
      void doubleClicked();

   public:
      SpinBox(QWidget* parent=0, const char* name = 0);
      SpinBox(int minValue, int maxValue, int step = 1, QWidget* parent=0, const char* name = 0);
};

#endif

