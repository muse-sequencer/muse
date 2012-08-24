//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: ./muse/widgets/mmath.cpp $
//
//  Copyright (C) 1999-2011 by Werner Schweer and others
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
#include "mmath.h"

//	QwtMath - a set of mathematical routines
//
//	qwtGetMin -- Find the smallest value in an array
//	qwtGetMax -- Find the largest value in an array
//	qwtTwistArray -- invert the order of an array
//	qwtFloor125 -- Find the largest value fitting in a 1-2-5 pattern
//	qwtCeil125 -- Find the smallest value fitting in a 1-2-5 pattern
//	qwtChkMono -- Check for monotony
//	qwtLinSpace -- construct an array of equally spaced values
//	qwtLogSpace -- construct an array of logarithmically equally spaced values
//	qwtMax	-- Return the largest of two values
//	qwtMin -- Return the smallest of two values
//	qwtAbs -- return the absolute value
//	qwtSign -- Return the sign of a number
//	qwtSqr -- Return the square of a number
//	qwtCopyArray -- Copy an array into another
//	qwtShiftArray -- Shift an array
//	qwtSwap --	Swap two values
//	qwtSort (1) -- Sort two values
//	qwtSort (2) -- Sort two values
//	qwtInt -- Return nearest integer
//	qwtLim -- Limit a values

namespace MusECore {

//------------------------------------------------------------
//.F	qwtGetMin
//	Find the smallest value in an array
//
//.u	Syntax
//.f	double qwtGetMin(double *array, int size)
//
//.u	Parameters
//.p	double *array, int size
//
//------------------------------------------------------------

double qwtGetMin(double *array, int size)
{
    double rv;
    int i;

    if (size > 0)
    {
	rv = array[0];
	for (i=1; i< size; i++)
	   rv = qwtMin(rv, array[i]);
	return rv;
    }
    else
       return 0.0;

}


//------------------------------------------------------------
//
//.F	qwtGetMax
//	Find the largest value in an array
//
//.u	Syntax
//.f	double qwtGetMax(double *array, int size)
//
//.u	Parameters
//.p	double *array, int size
//
//------------------------------------------------------------
double qwtGetMax(double *array, int size)
{
    double rv;
    int i;

    if (size > 0)
    {
	rv = array[0];
	for (i=1; i< size; i++)
	   rv = qwtMax(rv, array[i]);
	return rv;
    }
    else
       return 0.0;

}


//------------------------------------------------------------
//
//.F	qwtCeil125
//	Find the smallest value out of {1,2,5}*10^n with an integer number n
//	which is greater than or equal to x
//
//.u	Syntax
//.f	double qwtCeil125(double x)
//
//.u	Parameters
//.p	double x
//
//------------------------------------------------------------
double qwtCeil125( double x)
{
    double lx, rv;
    double p10, fr;
    double sign = ( x > 0) ? 1.0 : -1.0;

    if (x == 0.0) return 0.0;

    lx = log10(fabs(x));
    p10 = floor(lx);
    fr = pow(10.0,lx - p10);
    if (fr <=1.0)
       fr = 1.0;
    else if (fr <= 2.0)
       fr = 2.0;
    else if (fr <= 5.0)
       fr = 5.0;
    else
       fr = 10.0;
    rv = fr * pow(10.0,p10);
    return sign * rv;
}


//------------------------------------------------------------
//
//.F	qwtFloor125
//	Find the largest value out of {1,2,5}*10^n with an integer number n
//	which is smaller than or equal to x
//
//.u	Syntax
//.f	double qwtFloor125(double x)
//
//.u	Parameters
//.p	double x
//
//------------------------------------------------------------
double qwtFloor125( double x)
{
    double lx, rv;
    double p10, fr;
    double sign = ( x > 0) ? 1.0 : -1.0;

    if (x == 0.0) return 0.0;

    lx = log10(fabs(x));
    p10 = floor(lx);
    fr = pow(10.0,lx - p10);
    if (fr >= 10.0)
       fr = 10.0;
    else if (fr >= 5.0)
       fr = 5.0;
    else if (fr >= 2.0)
       fr = 2.0;
    else
       fr = 1.0;
    rv = fr * pow(10.0,p10);
    return sign * rv;
}


//------------------------------------------------------------
//
//.F	qwtChkMono
//	  Checks if an array is a strictly monotonic sequence
//
//.u	Syntax
//.f	int qwtChkMono(double *array, int size)
//
//.u	Parameters
//.p	double *array	-- pointer to a double array
//	int size	-- size of the array
//
//.u	Return Value
//.t      0 -- sequence is not strictly monotonic
//      1 -- sequence is strictly monotonically increasing
//      -1 --  sequence is strictly monotonically decreasing
//
//------------------------------------------------------------
int qwtChkMono(double *array, int size)
{
    int rv, i;

    if (size < 2) return 0;

    rv = qwtSign(array[1] - array[0]);
    for (i=1;i<size-1;i++)
    {
	if ( qwtSign(array[i+1] - array[i]) != rv )
	{
	    rv = 0;
	    break;
	}
    }
    return rv;

}

//------------------------------------------------------------
//
//.F	qwtTwistArray
//	Invert the order of array elements
//
//.u	Syntax
//.f	void qwtTwistArray(double *array, int size)
//
//.u	Parameters
//.p	double *array, int size
//
//------------------------------------------------------------
void qwtTwistArray(double *array, int size)
{
    int itmp;
    int i, s2;
    double dtmp;

    s2 = size / 2;

    for (i=0; i < s2; i++)
    {
	itmp = size - 1 - i;
	dtmp = array[i];
	array[i] = array[itmp];
	array[itmp] = dtmp;
    }

}


//------------------------------------------------------------
//
//.F	qwtLinSpace
//	Create an array of equally spaced values
//
//.u Syntax
//.f void qwtLinSpace(double *array, int size, double xmin, double xmax)	
//
//.u Parameters
//.p	double *array	--	where to put the values
//	int size	-- size of the array
//	double xmin	-- value associated with index 0
//	double xmax	-- value associated with index (size-1)
//
//------------------------------------------------------------
void qwtLinSpace(double *array, int size, double xmin, double xmax)
{
    int i, imax;
    imax = size -1;
    double step;

    if (size > 0)
    {
	array[0] = xmin;
	array[imax] = xmax;
	step = (xmax - xmin) / double(imax);

	for (i=1;i<imax;i++)
	   array[i] = xmin + double(i) * step;
    }

}


//------------------------------------------------------------
//
//.F	qwtLogSpace
//	Create an array of logarithmically equally spaced values
//
//.u	Syntax
//.f	void qwtLogSpace(double *array, int size, double xmin, double xmax)
//
//.u Parameters
//.p	double *array	--	where to put the values
//	int size	-- size of the array
//	double xmin	-- value associated with index 0
//	double xmax	-- value associated with index (size-1)
//------------------------------------------------------------
void qwtLogSpace(double *array, int size, double xmin, double xmax)
{
    int i, imax;

    double lxmin,lxmax;
    double lstep;

    imax = size -1;

    if ((xmin <= 0.0) || (xmax <= 0.0) || (size <= 0))
       return;

    array[0] = xmin;
    array[imax] = xmax;
    lxmin = log(xmin);
    lxmax = log(xmax);

    lstep = (lxmax - lxmin) / double(imax);

    for (i=1; i<imax;i++)
       array[i] = exp(lxmin + double(i) * lstep);

}

} // namespace MusECore
