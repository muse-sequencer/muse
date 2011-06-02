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
class Xml;

//---------------------------------------------------------
//   Velocity
//---------------------------------------------------------

class Velocity : public QDialog, public Ui::VelocityBase {
      Q_OBJECT
   private:
      
      QButtonGroup* rangeGroup;

   protected slots:
      void accept();
      void pullValues();

   public:
      Velocity(QWidget* parent = 0);

      int range;
      int rateVal;
      int offsetVal;
      
      void read_configuration(Xml& xml);
      void write_configuration(int level, Xml& xml);
      
      
   public slots:
      int exec();
      };

#endif

