//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: velocity.h,v 1.1.1.1 2003/10/27 18:54:51 wschweer Exp $
//  (C) Copyright 2001 Werner Schweer (ws@seh.de)
//=========================================================

#ifndef __VELOCITY_H__
#define __VELOCITY_H__

#include "ui_velocitybase.h"

class QButtonGroup;

//---------------------------------------------------------
//   VelocityBaseWidget
//   Wrapper around Ui::VelocityBase
//---------------------------------------------------------

class VelocityBaseWidget : public QDialog, public Ui::VelocityBase
{
      Q_OBJECT

   public:
      VelocityBaseWidget(QDialog *parent = 0)
           : QDialog(parent)
           { setupUi(this); }
};


//---------------------------------------------------------
//   Velocity
//---------------------------------------------------------

class Velocity : public VelocityBaseWidget {
      int _range;
      int _rateVal;
      int _offsetVal;

      Q_OBJECT
      QButtonGroup* rangeGroup;

   protected slots:
      void accept();

   public:
      Velocity(QDialog* parent = 0);
      void setRange(int id);
      int range() const     { return _range; }
      int rateVal() const   { return _rateVal; }
      int offsetVal() const { return _offsetVal; }
      };

#endif

