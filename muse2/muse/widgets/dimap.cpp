//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: dimap.cpp,v 1.1.1.1 2003/10/27 18:55:11 wschweer Exp $

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
#include "dimap.h"
#include "mmath.h"

namespace MusEGui {

const double DiMap::LogMin = 1.0e-150;
const double DiMap::LogMax = 1.0e150;

//	DiMap - Map a double interval into an integer interval
//
//	The DiMap class maps an interval of type double into an interval of
//	type integer. It consists
//	of two intervals D = [d1, d2] (double) and I = [i1, i2] (int), which are
//	specified with the @DiMap::setDblRange@ and @DiMap::setIntRange@
//	members. The point d1 is mapped to the point i1, and d2 is mapped to i2.
//	Any point inside or outside D can be mapped to a point inside or outside
//	I using @DiMap::transform@ or @DiMap::limTransform@ or vice versa
//	using @QwtPlot::invTransform@. D can be scaled linearly or
//	logarithmically, as specified with @DiMap::setDblRange@.

//------------------------------------------------------------
//.F	DiMap::DiMap (1)
//	Construct a DiMap instance.
//
//.u	Syntax
//.f	 DiMap::DiMap()
//
//.u	Description
//	The double and integer intervals are both set to [0,1].
//------------------------------------------------------------

DiMap::DiMap()
      {
      d_x1 = 0.0;
      d_x2 = 1.0;
      d_y1 = 0;
      d_y2 = 1;
      d_cnv = 1.0;
      }

//------------------------------------------------------------
//.F	DiMap::DiMap (2)
//	Construct a DiMap instance with initial integer
//	and double intervals
//
//.u	Syntax
//.f	 DiMap::DiMap(int i1, int i2, double d1, double d2, bool logarithmic)
//
//.u	Parameters
//.p	int i1		--	first border of integer interval
//	int i2		--	second border of integer interval
//	double d1	--	first border of double interval
//	double d2	--	second border of double interval
//	bool logarithmic  -- logarithmic mapping, TRUE or FALSE. Defaults
//					to FALSE.
//------------------------------------------------------------

DiMap::DiMap(int i1, int i2, double d1, double d2, bool logarithmic)
      {
      d_log = logarithmic;
      setIntRange(i1,i2);
      setDblRange(d1, d2);
      }

//------------------------------------------------------------
//.F	DiMap::~DiMap
//	Destroy a DiMap instance.
//
//.u	Syntax
//.f	 DiMap::~DiMap()
//------------------------------------------------------------

DiMap::~DiMap()
      {
      }

//------------------------------------------------------------
//.F	DiMap::contains (1)
//	Returns TRUE if a value x lies inside or at the border of the
//	map's double range.
//
//.u	Syntax
//.f	bool DiMap::contains(double x)
//
//.u	Parameters
//.p	double x -- value
//------------------------------------------------------------

bool DiMap::contains(double x) const
      {
      return ( (x >= MusECore::qwtMin(d_x1, d_x1)) && (x <= MusECore::qwtMax(d_x1, d_x2)));
      }

//------------------------------------------------------------
//.F	DiMap::contains (2)
//	Returns TRUE if a value x lies inside or at the border of the
//	map's integer range
//
//.u	Syntax
//.f	bool DiMap::contains(int x)
//
//.u	Parameters
//.p	int x -- value
//------------------------------------------------------------

bool DiMap::contains(int x) const
      {
      return ( (x >= MusECore::qwtMin(d_y1, d_y1)) && (x <= MusECore::qwtMax(d_y1, d_y2)));
      }

//------------------------------------------------------------
//.F	DiMap::setDblRange
//	Specify the borders of the double interval
//
//.u	Syntax
//.f	void DiMap::setDblRange(double d1, double d2, bool lg = FALSE)
//
//.u	Parameters
//.p	double d1	--	first border
//	double d2	--	second border
//	bool lg		--	logarithmic (TRUE) or linear (FALSE)
//				scaling. Defaults to FALSE.
//------------------------------------------------------------

void DiMap::setDblRange(double d1, double d2, bool lg)
      {
      if (lg) {
            d_log = true;
            if (d1 < LogMin)
                  d1 = LogMin;
            else if (d1 > LogMax)
                  d1 = LogMax;
      	
            if (d2 < LogMin)
                  d2 = LogMin;
            else if (d2 > LogMax)
                  d2 = LogMax;
      	
            d_x1 = log(d1);
            d_x2 = log(d2);
            }
      else {
            d_log = FALSE;
            d_x1 = d1;
            d_x2 = d2;
            }
      newFactor();
      }

//------------------------------------------------------------
//.F	DiMap::setIntRange
//	Specify the borders of the integer interval
//
//.u	Syntax
//.f	void DiMap::setIntRange(int i1, int i2)
//
//.u	Parameters
//.p	int i1	--	first border
//	int i2  --	second border
//------------------------------------------------------------

void DiMap::setIntRange(int i1, int i2)
      {
      d_y1 = i1;
      d_y2 = i2;
      newFactor();
      }

//------------------------------------------------------------
//.F	DiMap::transform
//	Transform a point in double interval into an point in the
//	integer interval
//
//.u	Syntax
//.f	int DiMap::transform(double x)
//
//.u	Parameters
//.p	double x
//
//.u	Return Value
//.t
//	linear mapping:	-- rint(i1 + (i2 - i1) / (d2 - d1) * (x - d1))
//	logarithmic mapping: -- rint(i1 + (i2 - i1) / log(d2 / d1) * log(x / d1))
//
//.u    Note
//	The specified point is allowed to lie outside the intervals. If you
//	want to limit the returned value, use @DiMap::limTransform@.
//------------------------------------------------------------

int DiMap::transform(double x) const
      {
      if (d_log)
            return (d_y1 + int(rint( (log(x) - d_x1) * d_cnv )));
      else
            return (d_y1 + int(rint( (x - d_x1) * d_cnv )));
      }

//------------------------------------------------------------
//.F	DiMap::invTransform
//	Transform an integer value into a double value
//
//.u	Syntax
//.f	double DiMap::invTransform(int y)
//
//.u	Parameters
//.p	int y	--	integer value to be transformed
//
//.u	Return Value
//.t
//	linear mapping:	-- d1 + (d2 - d1) / (i2 - i1) * (y - i1)
//	logarithmic mapping: -- d1 + (d2 - d1) / log(i2 / i1) * log(y / i1)
//------------------------------------------------------------

double DiMap::invTransform(int y) const
      {
      if (d_cnv == 0.0)
            return 0.0;
      else {
            if (d_log)
                  return exp(d_x1 + double(y - d_y1) / d_cnv );
            else
                  return ( d_x1 + double(y - d_y1) / d_cnv );	
            }
      }

//------------------------------------------------------------
//.F	DiMap::limTransform
//	Transform and limit
//
//.u	Syntax
//.f	int DiMap::limTransform(double x)
//
//.u	Parameters
//.p	double x
//
//.u	Return Value
//		transformed value
//
//.u	Description
//	The function is similar to @DiMap::transform@, but limits the input value
//	to the nearest border of the map's double interval if it lies outside
//	that interval.
//------------------------------------------------------------

int DiMap::limTransform(double x) const
      {
      if ( x > MusECore::qwtMax(d_x1, d_x2) )
            x = MusECore::qwtMax(d_x1, d_x2);
      else if ( x < MusECore::qwtMin(d_x1, d_x2))
            x = MusECore::qwtMin(d_x1, d_x2);
      return transform(x);
      }

//------------------------------------------------------------
//.F	DiMap::xTransform
//	Exact transformation
//
//.u	Syntax
//.f	double DiMap::dTransform(double x)
//
//.u	Parameters
//.p		double x	-- value to be transformed
//
//.u	Return Value
//.t
//	linear mapping:	-- i1 + (i2 - i1) / (d2 - d1) * (x - d1)
//	logarithmic mapping: -- i1 + (i2 - i1) / log(d2 / d1) * log(x / d1)
//
//.u	Description
//	This function is similar to @DiMap::transform@, but
//	makes the integer interval appear to be double.
//------------------------------------------------------------

double DiMap::xTransform(double x) const
      {
      double rv;

      if (d_log)
            rv = double(d_y1) + (log(x) - d_x1) * d_cnv;
      else
            rv = double(d_y1) + (x - d_x1) * d_cnv;
      return rv;
      }

//------------------------------------------------------------
//.F	DiMap::newFactor
//	Re-calculate the conversion factor.
//------------------------------------------------------------

void DiMap::newFactor()
      {
      if (d_x2 != d_x1)
            d_cnv = double(d_y2 - d_y1) / (d_x2 - d_x1);
      else
            d_cnv = 0.0;
      }

} // namespace MusEGui
