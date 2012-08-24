//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: drange.cpp,v 1.2.2.1 2009/03/09 02:05:18 terminator356 Exp $
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

#include <cmath>

#include <QtGlobal>

#include "mmath.h"
#include "drange.h"

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
      d_prevValue = 0.0;
      d_exactPrevValue = 0.0;
      d_exactValue = 0.0;
      d_value = 0.0;
      d_step  = 0.1;
      d_periodic = FALSE;
      }

//---------------------------------------------------------
//   setNewValue
//---------------------------------------------------------

void DoubleRange::setNewValue(double x, bool align)
      {
      d_prevValue = d_value;

      double vmin = MusECore::qwtMin(d_minValue, d_maxValue);
      double vmax = MusECore::qwtMax(d_minValue, d_maxValue);

      // Range check

      if (x < vmin) {
            if ((d_periodic) && (vmin != vmax))
                  d_value = x + ceil((vmin - x) / (vmax - vmin))
                     * (vmax - vmin);
            else
                  d_value = vmin;
            }
      else if (x > vmax) {
            if ((d_periodic) && (vmin != vmax))
                  d_value = x - ceil( ( x - vmax) / (vmax - vmin ))
                     * (vmax - vmin);
            else
                  d_value = vmax;
            }
      else
            d_value = x;

      d_exactPrevValue = d_exactValue;
      d_exactValue = d_value;

      // align to grid
      if (align) {
            if (d_step != 0.0)
                  d_value = d_minValue + rint((d_value - d_minValue) / d_step ) * d_step;
            else
                  d_value = d_minValue;
	
            // correct rounding error at the border
            if (fabs(d_value - d_maxValue) < MinEps * MusECore::qwtAbs(d_step))
                  d_value = d_maxValue;

            // correct rounding error if value = 0
            if (fabs(d_value) < MinEps * MusECore::qwtAbs(d_step))
                  d_value = 0.0;
            }
      if (d_prevValue != d_value)
            valueChange();
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
      bool rchg = ((d_maxValue != vmax) || (d_minValue != vmin));

      if(!rchg && vstep == d_step && pageSize == d_pageSize)    // p4.0.45
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
      d_pageSize = MusECore::qwtLim(pageSize,0, int(MusECore::qwtAbs((d_maxValue - d_minValue) / d_step)));

      //
      // If the value lies out of the range, it
      // will be changed. Note that it will not be adjusted to
      // the new step width.
      setNewValue(d_value, false);

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
      setNewValue(d_value + double(nSteps) * d_step, true);
      }

//---------------------------------------------------------
//   incPages
//    Increment the value by a specified number of pages
//---------------------------------------------------------

void DoubleRange::incPages(int nPages)
      {
      setNewValue(d_value + double(nPages) * double(d_pageSize)
         * d_step, true);
      }

//---------------------------------------------------------
//   step
//---------------------------------------------------------

double DoubleRange::step() const
      {
      return MusECore::qwtAbs(d_step);
      }

} // namespace MusEGui
