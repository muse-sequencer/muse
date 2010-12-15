//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: drange.h,v 1.1.1.1.2.1 2007/01/27 14:52:43 spamatica Exp $
//
//    Copyright (C) 1997  Josef Wilgen
//	This program is free software; you can redistribute it and/or modify
//	it under the terms of the GNU General Public License, version 2,
//	as published by	the Free Software Foundation.
//
//    (C) Copyright 2000 Werner Schweer (ws@seh.de)
//=========================================================

#ifndef __DOUBLE_RANGE_H__
#define __DOUBLE_RANGE_H__

//---------------------------------------------------------
//   DoubleRange
//---------------------------------------------------------

class DoubleRange
      {
      static const double MinRelStep;
      static const double DefaultRelStep;
      static const double MinEps;

      double d_minValue;
      double d_maxValue;
      double d_step;
      int d_pageSize;
      double d_value;
      double d_exactValue;
      double d_exactPrevValue;
      double d_prevValue;
      bool d_periodic;

      void setNewValue(double x, bool align = false);

   protected:
      double exactValue() const { return d_exactValue; }
      double exactPrevValue() const { return d_exactPrevValue; }
      double prevValue() const { return d_prevValue; }
      virtual void valueChange() {}
      virtual void stepChange()  {}
      virtual void rangeChange() {}

   public:
      DoubleRange();
      virtual ~DoubleRange(){};

      double value() const    { return d_value; }
      virtual void setValue(double);

      virtual void fitValue(double);
      virtual void incValue(int);
      virtual void incPages(int);
      void setPeriodic(bool tf);
      void setRange(double vmin, double vmax, double vstep = 0.0,
         int pagesize = 1);
      void setStep(double);

      double maxValue() const { return d_maxValue; }
      double minValue() const { return d_minValue; }
      bool periodic()  const  { return d_periodic; }
      int pageSize() const    { return d_pageSize; }
      double step() const;
      };

#endif
