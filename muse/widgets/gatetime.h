//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: gatetime.h,v 1.1.1.1.2.1 2008/01/19 13:33:47 wschweer Exp $
//  (C) Copyright 2001 Werner Schweer (ws@seh.de)
//=========================================================

#ifndef __GATETIME_H__
#define __GATETIME_H__

#include "gatetimebase.h"

//---------------------------------------------------------
//   GateTime
//---------------------------------------------------------

class GateTime : public GateTimeBase {
      Q_OBJECT

      int _range;
      int _rateVal;
      int _offsetVal;

   protected slots:
      void accept();

   public:
      GateTime(QWidget* parent, const char* name = 0);
      void setRange(int id);
      int range() const { return _range; }
      int rateVal() const { return _rateVal; }
      int offsetVal() const { return _offsetVal; }
      };

#endif

