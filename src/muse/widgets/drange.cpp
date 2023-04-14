//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: drange.cpp,v 1.2.2.1 2009/03/09 02:05:18 terminator356 Exp $
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

//#include <QtGlobal>

#include "mmath.h"
#include "drange.h"

// For debugging output: Uncomment the fprintf section.
//#include <stdio.h>
#define DEBUG_DRANGE(dev, format, args...) // fprintf(dev, format, ##args);


namespace MusEGui {

const double DoubleRange::MinRelStep = 1.0e-10;
const double DoubleRange::DefaultRelStep = 1.0e-2;
const double DoubleRange::MinEps = 1.0e-10;

//-----------------------------------------------------------
//	This class is useful as a base class or a member for sliders.
//	It represents an interval of type double within which a value can
//	be moved. The value can be either an arbitrary point inside
//	the interval (see @DoubleRange::setValue@), or it can be fitted
//	into a step raster (see @DoubleRange::fitValue@ and
//	@DoubleRange::incValue@).
//
//	As a special case, a DoubleRange can be periodic, which means that
//	a value outside the interval will be mapped to a value inside the
//	interval when @DoubleRange::setValue@, @DoubleRange::fitValue@,
//	@DoubleRange::incValue@ or @DoubleRange::incPages@ are called.
//------------------------------------------------------------

//---------------------------------------------------------
//   doubleRange
//---------------------------------------------------------

DoubleRange::DoubleRange()
      {
      d_minValue = 0;
      d_maxValue = 100.0;
      d_exactPrevValue = 0.0;
      d_exactValue = 0.0;
      d_prevValue = d_value = 0.0;
      d_step  = 0.1;
      d_periodic = false;
      d_log = false;
      d_integer = false;
      d_logCanZero = false;
      d_dBFactor = 20.0;
      d_dBFactorInv = 1.0/d_dBFactor;
      d_logFactor = 1.0;
      }

DoubleRange::~DoubleRange(){}

//---------------------------------------------------------
//   convertTo
//---------------------------------------------------------

double DoubleRange::convertTo(double x, ConversionMode mode) const
{
  switch(mode)
  {
    case ConvertNone:
      return x;

    case ConvertDefault:
    {
      double rv = x;
      if(d_log)
        rv = d_logFactor * museDbToVal(rv, d_dBFactorInv);
      return rv;
    }
    break;

    case ConvertInt:
      return rint(x);
    break;

    case ConvertLog:
      return d_logFactor * museDbToVal(x, d_dBFactorInv);
    break;
  }
  return x;
}

//---------------------------------------------------------
//   convertTo
//---------------------------------------------------------

double DoubleRange::convertFrom(double x, ConversionMode mode) const
{
  switch(mode)
  {
    case ConvertNone:
      return x;

    case ConvertDefault:
    {
      double rv = x;
      if(d_log)
      {
        if(rv <= 0.0)
        {
          if(d_integer)
            rv = rint(d_minValue);
          else
            rv = museValToDb(d_minValue / d_logFactor, d_dBFactor);
        }
        else
          rv = museValToDb(rv / d_logFactor, d_dBFactor);
      }
      return rv;
    }
    break;

    case ConvertInt:
      return rint(x);
    break;

    case ConvertLog:
      if(x <= 0.0)
        x = museValToDb(d_minValue / d_logFactor, d_dBFactor);
      else
        x = museValToDb(x / d_logFactor, d_dBFactor);
    break;
  }
  return x;
}

//---------------------------------------------------------
//   setNewValue
//---------------------------------------------------------

void DoubleRange::setNewValue(double x, bool align)
      {
      DEBUG_DRANGE(stderr, "DoubleRange::setNewValue TOP val:%.20f d_value:%.20f\n", x, d_value);

      const double vmin = MusECore::qwtMin(d_minValue, d_maxValue);
      const double vmax = MusECore::qwtMax(d_minValue, d_maxValue);
      d_prevValue = d_value;

      // Range check

      if (x < vmin) {
            if ((d_periodic) && (vmin != vmax))
                  d_value = x + ceil((vmin - x) / (vmax - vmin))
                     * (vmax - vmin);
            else if(d_value == vmin)
                  return;
            d_value = vmin;
            }
      else if (x > vmax) {
            if ((d_periodic) && (vmin != vmax))
                  d_value = x - ceil( ( x - vmax) / (vmax - vmin ))
                     * (vmax - vmin);
            else if(d_value == vmax)
                  return;
            d_value = vmax;
            }
      else
            d_value = x;

      if(d_value == d_prevValue)
        return;

      d_exactPrevValue = d_exactValue;
      d_exactValue = d_value;

      // align to grid
      if (align) {
            // If it's log but not integer.
            if(d_log && !d_integer)
            {
              if (d_step != 0.0)
              {
                  // Don't touch the value if it's already at a boundary, to avoid introducing unwanted error.
                  if(d_value > vmin && d_value < vmax)
                  {
                    const double mn = museValToDb(vmin / d_logFactor, d_dBFactor);
                    double v = museValToDb(d_value / d_logFactor, d_dBFactor);
                    d_value = d_logFactor * museDbToVal(mn + rint((v - mn) / d_step ) * d_step, d_dBFactorInv);
                  }
              }
              else
              {
                    d_value = vmin;
              }
            }
            // It's integer or log integer, or none.
            else
            {
              // Don't touch the value if it's already at a boundary, to avoid introducing unwanted error.
              if(d_value > vmin && d_value < vmax)
              {
                  if (d_step != 0.0)
                        d_value = vmin + rint((d_value - vmin) / d_step ) * d_step;
                  else
                        d_value = vmin;

                  // correct rounding error at the border
                  if (fabs(d_value - vmax) < MinEps * MusECore::qwtAbs(d_step))
                        d_value = vmax;

                  // correct rounding error if value = 0
                  if (fabs(d_value) < MinEps * MusECore::qwtAbs(d_step))
                        d_value = 0.0;
              }
            }
            }
      DEBUG_DRANGE(stderr, "                         BOTTOM val:%.20f d_value:%.20f\n", x, d_value);
      if (d_prevValue != d_value)
      {
            DEBUG_DRANGE(stderr, "  not equal, calling valueChange\n");
            valueChange();
      }
      }


//---------------------------------------------------------
//   fitValue
//    Adjust the value to the closest point in the step
//	raster.
//	The value is clipped when it lies outside the range.
//	When the range is @DoubleRange::periodic@, it will
//	be mapped to a point in the interval such that
//---------------------------------------------------------

void DoubleRange::fitValue(double x)
      {
      setNewValue(x, true);
      }

//---------------------------------------------------------
//   setValue
//    Set a new value without adjusting to the step raster
//    The value is clipped when it lies outside the range.
//	When the range is @DoubleRange::periodic@, it will
//	be mapped to a point in the interval such that
//
//		new value := x + n * (max. value - min. value)
//
//    with an integer number n.
//---------------------------------------------------------

void DoubleRange::setValue(double x)
      {
      setNewValue(x, false);
      }

//---------------------------------------------------------
//   setRange
//    Specify  range and step size
//	- A change of the range changes the value if it lies outside the
//	  new range. The current value
//	  will *not* be adjusted to the new step raster.
//	- vmax < vmin is allowed.
//	- If the step size is left out or set to zero, it will be
//	  set to 1/100 of the interval length.
//	- If the step size has an absurd value, it will be corrected
//	  to a better one.
//---------------------------------------------------------

void DoubleRange::setRange(double vmin, double vmax, double vstep, int pageSize)
      {
      // If it's integer or log integer.
      if(d_integer)
      {
        vmin = rint(vmin);
        vmax = rint(vmax);
      }

      // If it's log or log integer.
      if(d_log)
      {
        if(d_integer)
        {
          // Force a hard lower limit of integer 1.
          if(vmin <= 0.0)
            vmin = 1.0;
          if(vmax <= 0.0)
            vmax = 1.0;
        }
        else
        {
          // Force a hard lower limit of -120 dB.
          if(vmin <= 0.0)
            vmin = 0.000001;
          if(vmax <= 0.0)
            vmax = 0.000001;
        }
      }

      bool rchg = ((d_maxValue != vmax) || (d_minValue != vmin));

      if(!rchg && vstep == d_step && pageSize == d_pageSize)
        return;

      if (rchg) {
            d_minValue = vmin;
            d_maxValue = vmax;
            }

      //
      // look if the step width has an acceptable
      // value or otherwise change it.
      //
      setStep(vstep);

      //
      // limit page size
      //
      d_pageSize = MusECore::qwtLim(
        pageSize,0,
        int(MusECore::qwtAbs((d_maxValue - d_minValue) / (d_step * (d_log ? d_logFactor : 1.0)))));

      //
      // If the value lies out of the range, it
      // will be changed. Note that it will not be adjusted to
      // the new step width.
      setNewValue(d_value / (d_log ? d_logFactor : 1.0), false);

      // call notifier after the step width has been
      // adjusted.
      if (rchg)
            rangeChange();
      }

//---------------------------------------------------------
//   setStep
//    Change the step raster 	
//
//    The value will *not* be adjusted to the new step raster.
//---------------------------------------------------------

void DoubleRange::setStep(double vstep)
      {
      double newStep,intv;

      // If it's log but not integer.
      if(d_log && !d_integer)
        intv = museValToDb(d_maxValue / d_logFactor, d_dBFactor) - museValToDb(d_minValue / d_logFactor, d_dBFactor);
      // It's integer or log integer, or none.
      else
        intv = d_maxValue - d_minValue;

      if (vstep == 0.0)
            newStep = intv * DefaultRelStep;
      else {
         if (((intv > 0) && (vstep < 0)) || ((intv < 0) && (vstep > 0)))
                  newStep = -vstep;
            else
                  newStep = vstep;

            if ( fabs(newStep) < fabs(MinRelStep * intv) )
                  newStep = MinRelStep * intv;
            }

      if (newStep != d_step) {
            d_step = newStep;
            DEBUG_DRANGE(stderr, "DoubleRange::setStep vstep:%.20f d_step:%.20f\n", vstep, d_step);
            stepChange();
            }
      }


//---------------------------------------------------------
//   setPeriodic
//    Make the range periodic
//
//    When the range is periodic, the value will be set to a point
//    inside the interval such that
//
//  	point = value + n * width
//
//    if the user tries to set a new value which is outside the range.
//    If the range is nonperiodic (the default), values outside the
//    range will be clipped.
//---------------------------------------------------------

void DoubleRange::setPeriodic(bool tf)
      {
      d_periodic = tf;
      }

//------------------------------------------------------------
//  incValue
//	Increment the value by a specified number of steps
//
//    As a result of this operation, the new value will always be
//	adjusted to the step raster.
//------------------------------------------------------------

void DoubleRange::incValue(int nSteps)
      {
      // If it's log but not integer.
      if(d_log && !d_integer)
        setNewValue(d_logFactor * museDbToVal((museValToDb(d_value / d_logFactor, d_dBFactor) +
          double(nSteps) * d_step), d_dBFactorInv), true);
      // It's integer or log integer, or none.
      else
        setNewValue(d_value + double(nSteps) * d_step, true);
      }

//---------------------------------------------------------
//   incPages
//    Increment the value by a specified number of pages
//---------------------------------------------------------

void DoubleRange::incPages(int nPages)
      {
      // If it's log but not integer.
      if(d_log && !d_integer)
        setNewValue(d_logFactor * museDbToVal(museValToDb(d_value / d_logFactor, d_dBFactor) +
          double(nPages) * double(d_pageSize) * d_step, d_dBFactorInv), true);
      // It's integer or log integer, or none.
      else
        setNewValue(d_value + double(nPages) * double(d_pageSize) * d_step, true);
      }

//---------------------------------------------------------
//   step
//---------------------------------------------------------

double DoubleRange::step() const
      {
      return MusECore::qwtAbs(d_step);
      }

double DoubleRange::internalMaxValue(ConversionMode mode) const { return convertFrom(d_maxValue, mode); }
double DoubleRange::internalMinValue(ConversionMode mode) const { return convertFrom(d_minValue, mode); }

//---------------------------------------------------------
//   internalValue
//    Returns the current internal value which is never below minimum,
//     even in log mode. See value().
//---------------------------------------------------------

double DoubleRange::internalValue(ConversionMode mode) const
      {
      return convertFrom(d_value, mode);
      }

//---------------------------------------------------------
//   setInternalValue
//    Set a new value without adjusting to the step raster
//    The value is clipped when it lies outside the range.
//	When the range is @DoubleRange::periodic@, it will
//	be mapped to a point in the interval such that
//
//		new value := x + n * (max. value - min. value)
//
//    with an integer number n.
//---------------------------------------------------------

void DoubleRange::setInternalValue(double x, ConversionMode mode)
      {
      setNewValue(convertTo(x, mode), false);
      }

//---------------------------------------------------------
//   value
//    Returns the current value.
//    In log mode, when the value is equal to or less than the minimum
//     AND logCanOff is true, this will return 0.0 (-inf dB). See internalValue().
//---------------------------------------------------------

double DoubleRange::value() const
{
  // In log mode, when the value equals min, return 0.0 (-inf dB).
  if(d_log && d_logCanZero && d_value <= d_minValue)
    return 0.0;
  return d_value;
}

void DoubleRange::internalFitValue(double x, ConversionMode mode)
      {
      setNewValue(convertTo(x, mode), true);
      }

double DoubleRange::exactValue(ConversionMode mode) const { return convertFrom(d_exactValue, mode); }
double DoubleRange::exactPrevValue(ConversionMode mode) const { return convertFrom(d_exactPrevValue, mode); }
double DoubleRange::prevValue(ConversionMode mode) const { return convertFrom(d_prevValue, mode); }
void DoubleRange::valueChange() {}
void DoubleRange::stepChange()  {}
void DoubleRange::rangeChange() {}
double DoubleRange::maxValue() const { return d_maxValue; }
double DoubleRange::minValue() const { return d_minValue; }
bool DoubleRange::periodic()  const  { return d_periodic; }
int DoubleRange::pageSize() const    { return d_pageSize; }
bool DoubleRange::log() const        { return d_log; }
void DoubleRange::setLog(bool v)     { d_log = v; }
bool DoubleRange::integer() const    { return d_integer; }
void DoubleRange::setInteger(bool v) { d_integer = v; }
void DoubleRange::setLogCanZero(bool v) { d_logCanZero = v; }
void DoubleRange::setDBFactor(double v) { d_dBFactor = v; d_dBFactorInv = 1.0/d_dBFactor; }

void DoubleRange::setLogFactor(double v)
{
  if(d_log)
  {
    // Set the new factor.
    d_logFactor = v;
    // Reset the value.
    DoubleRange::setValue(d_value / d_logFactor);
    return;
  }

  // Set the new factor.
  d_logFactor = v;
}

} // namespace MusEGui
