//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: velocity.h,v 1.3 2006/01/25 16:24:33 wschweer Exp $
//  (C) Copyright 2001 Werner Schweer (ws@seh.de)
//=========================================================

#ifndef __VELOCITY_H__
#define __VELOCITY_H__

#include "ui_velocity.h"

//---------------------------------------------------------
//   Velocity
//---------------------------------------------------------

class Velocity : public QDialog, public Ui::VelocityBase {
      Q_OBJECT

      QButtonGroup* rangeGroup;
      int _range;
      int _rateVal;
      int _offsetVal;

   protected slots:
      void accept();

   public:
      Velocity(QWidget* parent = 0);
      void setRange(int id);
      int range() const     { return _range; }
      int rateVal() const   { return _rateVal; }
      int offsetVal() const { return _offsetVal; }
      };

#endif

