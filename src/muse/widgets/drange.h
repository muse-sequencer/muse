//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: drange.h,v 1.1.1.1.2.1 2007/01/27 14:52:43 spamatica Exp $
//
//    Copyright (C) 1997  Josef Wilgen
//    (C) Copyright 2000 Werner Schweer (ws@seh.de)
//  (C) Copyright 2016 Tim E. Real (terminator356 on sourceforge)
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
      double d_prevValue;
      double d_exactPrevValue;
      bool d_periodic;
      bool d_log;
      bool d_integer;
      bool d_logCanZero;
      double d_dBFactor, d_dBFactorInv, d_logFactor;

      // In log mode, returns the log factor, otherwise returns 1.0.
      //inline double realLogFactor() const { return d_log ? d_logFactor : 1.0; }

      void setNewValue(double x, bool align = false);

   public:
     enum ConversionMode { ConvertNone, ConvertDefault, ConvertInt, ConvertLog };
      
   protected:
      double internalMaxValue(ConversionMode mode = ConvertNone) const;
      double internalMinValue(ConversionMode mode = ConvertNone) const;
      // Returns the current internal value which is never below minimum, even in log mode. See value().
      double internalValue(ConversionMode mode = ConvertNone) const;
      void setInternalValue(double x, ConversionMode mode = ConvertNone);
      void internalFitValue(double x, ConversionMode mode = ConvertNone);
      double convertFrom(double x, ConversionMode mode = ConvertNone) const;
      double convertTo(double x, ConversionMode mode = ConvertNone) const;
      double exactValue(ConversionMode mode = ConvertNone) const;
      double exactPrevValue(ConversionMode mode = ConvertNone) const;
      double prevValue(ConversionMode mode = ConvertNone) const;
      virtual void valueChange();
      virtual void stepChange();
      virtual void rangeChange();

   public:
      DoubleRange();
      virtual ~DoubleRange();

      // Returns the current value.
      // In log mode, when the value is equal to or less than the minimum AND logCanOff is true,
      //  this will return 0.0 (-inf dB). See internalValue().
      double value() const;
      virtual void setValue(double x);

      virtual void fitValue(double x);
      virtual void incValue(int);
      virtual void incPages(int);
      void setPeriodic(bool tf);
      void setRange   (double vmin, double vmax, double vstep = 0.0, int pagesize = 1);
      void setStep(double);

      double maxValue() const;
      double minValue() const;
      bool periodic()  const;
      int pageSize() const;
      double step() const;
      
      bool log() const;
      void setLog(bool v);
      bool integer() const;
      void setInteger(bool v);
      // Sets whether a log value jumps to zero (-inf dB) when it reaches the given minimum which in most cases
      //  will be set to something above zero (the app's minimum slider dB setting) - even if the controller
      //  itself goes all the way to zero. Use with valueWithLogOff().
      void setLogCanZero(bool v);
      // In log mode, sets the dB factor when conversions are done.
      // For example 20 * log10() for signals, 10 * log10() for power, and 40 * log10() for MIDI volume.
      void setDBFactor(double v = 20.0);
      // Sets the scale of a log range. For example a MIDI volume control can set a logFactor = 127
      //  so that the range can conveniently be set to 0-127. (With MIDI volume, dBFactor would be
      //  set to 40.0, as per MMA specs.) Min, max, off, input and output values are all scaled by this factor,
      //  but step is not.
      void setLogFactor(double v = 1.0);
      };

} // namespace MusEGui

#endif
