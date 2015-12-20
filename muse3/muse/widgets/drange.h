//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: drange.h,v 1.1.1.1.2.1 2007/01/27 14:52:43 spamatica Exp $
//
//    Copyright (C) 1997  Josef Wilgen
//    (C) Copyright 2000 Werner Schweer (ws@seh.de)
//
//  This program is free software; you can redistribute it and/or
//  modify it under the terms of the GNU General Public License
//  as published by the Free Software Foundation; version 2 of
//  the License, or (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//
//=========================================================

#ifndef __DOUBLE_RANGE_H__
#define __DOUBLE_RANGE_H__

namespace MusEGui {

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
      bool d_log;
      bool d_integer;

      void setNewValue(double x, bool align = false);

   public:
     enum ConversionMode { ConvertNone, ConvertDefault, ConvertInt, ConvertLog };
      
   protected:
      double convertFrom(double x, ConversionMode mode = ConvertDefault) const;
      double convertTo(double x, ConversionMode mode = ConvertDefault) const;
      double exactValue(ConversionMode mode = ConvertDefault) const { return convertTo(d_exactValue, mode); }
      double exactPrevValue(ConversionMode mode = ConvertDefault) const { return convertTo(d_exactPrevValue, mode); }
      double prevValue(ConversionMode mode = ConvertDefault) const { return convertTo(d_prevValue, mode); }
      bool valHasChanged() const { return d_value != d_prevValue; }
      virtual void valueChange() {}
      virtual void stepChange()  {}
      virtual void rangeChange() {}

   public:
      DoubleRange();
      virtual ~DoubleRange(){}

//       double value() const    { return d_value; }
      double value(ConversionMode mode = ConvertDefault) const;
      virtual void setValue(double x, ConversionMode mode = ConvertDefault);

      virtual void fitValue(double x, ConversionMode mode = ConvertDefault);
      virtual void incValue(int);
      virtual void incPages(int);
      void setPeriodic(bool tf);
      void setRange   (double vmin, double vmax, double vstep = 0.0, int pagesize = 1, ConversionMode mode = ConvertDefault);
      void setLogRange(double vmin, double vmax, double vstep = 0.0, int pagesize = 1);
      void setStep(double);

      double maxValue(ConversionMode mode = ConvertDefault) const { return convertTo(d_maxValue, mode); }
      void setMaxLogValue(double v);
      double minValue(ConversionMode mode = ConvertDefault) const { return convertTo(d_minValue, mode); }
      void setMinLogValue(double v);
      bool periodic()  const  { return d_periodic; }
      int pageSize() const    { return d_pageSize; }
      double step() const;
      
      bool log() const        { return d_log; }
      void setLog(bool v)     { d_log = v; }
      bool integer() const    { return d_integer; }
      void setInteger(bool v) { d_integer = v; }
      };

} // namespace MusEGui

#endif
