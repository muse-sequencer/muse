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

#include <QSpinBox>

class QEvent;
class QLineEdit;
class QMouseEvent;

//---------------------------------------------------------
//   SpinBox
//---------------------------------------------------------

class SpinBox : public QSpinBox {
      Q_OBJECT

      bool _clearFocus; 
      StepEnabled upEnabled;
      StepEnabled downEnabled;

   protected:
      bool eventFilter(QObject* obj, QEvent* ev);
      virtual void mousePressEvent ( QMouseEvent * event );
      virtual StepEnabled stepEnabled() const;      

   public slots:
      virtual void stepUp();
      virtual void stepDown();
   
   signals:
      void doubleClicked();
      void stepDownPressed();
      void stepUpPressed();

   public:
      SpinBox(QWidget* parent=0);
      SpinBox(int minValue, int maxValue, int step = 1, QWidget* parent=0);
      void setStepEnabled(bool up, bool down);
      int arrowWidth() const;
      void setEditor(QLineEdit* ed);
};

#endif

