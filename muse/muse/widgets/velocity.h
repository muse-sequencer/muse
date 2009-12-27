//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: velocity.h,v 1.1.1.1 2003/10/27 18:54:51 wschweer Exp $
//  (C) Copyright 2001 Werner Schweer (ws@seh.de)
//=========================================================

#ifndef __VELOCITY_H__
#define __VELOCITY_H__

#include "velocitybase.h"

//---------------------------------------------------------
//   Velocity
//---------------------------------------------------------

class Velocity : public VelocityBase {
      int _range;
      int _rateVal;
      int _offsetVal;

      Q_OBJECT

   protected slots:
      void accept();

   public:
      Velocity(QWidget* parent, const char* name = 0);
      void setRange(int id);
      int range() const     { return _range; }
      int rateVal() const   { return _rateVal; }
      int offsetVal() const { return _offsetVal; }
      };

#endif

