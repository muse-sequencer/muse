//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: gatetime.h,v 1.3 2006/01/25 16:24:33 wschweer Exp $
//  (C) Copyright 2001 Werner Schweer (ws@seh.de)
//=========================================================

#ifndef __GATETIME_H__
#define __GATETIME_H__

#include "ui_gatetime.h"

//---------------------------------------------------------
//   GateTime
//---------------------------------------------------------

class GateTime : public QDialog, public Ui::GateTimeBase {
      Q_OBJECT

      QButtonGroup* rangeGroup;
      int _range;
      int _rateVal;
      int _offsetVal;

   protected slots:
      void accept();

   public:
      GateTime(QWidget* parent = 0);
      void setRange(int id);
      int range() const     { return _range; }
      int rateVal() const   { return _rateVal; }
      int offsetVal() const { return _offsetVal; }
      };

#endif

