//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: gatetime.h,v 1.1.1.1.2.1 2008/01/19 13:33:47 wschweer Exp $
//  (C) Copyright 2001 Werner Schweer (ws@seh.de)
//=========================================================

#ifndef __GATETIME_H__
#define __GATETIME_H__

#include "ui_gatetimebase.h"

class QButtonGroup;
class Xml;

//---------------------------------------------------------
//   GateTime
//---------------------------------------------------------

class GateTime : public QDialog, public Ui::GateTimeBase {
 	Q_OBJECT
   private:
      
      QButtonGroup *rangeGroup;

   protected slots:
      void accept();
      void pullValues();

   public:
      GateTime(QWidget* parent=0);

      int range;
      int rateVal;
      int offsetVal;
      
      void read_configuration(Xml& xml);
      void write_configuration(int level, Xml& xml);

      
   public slots:
      int exec();
      };

#endif

