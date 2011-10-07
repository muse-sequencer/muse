//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: scldiv.cpp,v 1.1.1.1 2003/10/27 18:54:32 wschweer Exp $
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
#include "scldiv.h"
#include "mmath.h"

namespace MusEGui {

//	ScaleDiv - A class for building  scale divisions
//
//	The ScaleDiv class can build
//	linear and logarithmic scale divisions for specified
//	intervals. It uses an adjustable algorithm to
//	generate the major and minor step widths automatically.
//	A scale division has a minimum value, a maximum value,
//	a vector of major marks, and a vector of minor marks.
//	
//	ScaleDiv uses implicit sharing for the mark vectors.
//
//	Build a logarithmic scale division from 0.01 to 1000
//	and print out the major and minor marks.
//.c
//      #include <scldiv.h>
//      #include <iostream.h>
//
//      main()
//      {
//          int i,k;
//          ScaleDiv sd;
//
//          sd.rebuild(0.01, 100, 10, 10, TRUE, 0.0);
//
//          k=0;
//          for (i=0;i<sd.majCnt();i++)
//          {
//              while(k < sd.minCnt())
//              {
//                  if(sd.minMark(k) < sd.majMark(i))
//                  {
//                      cout << " - " << sd.minMark(i) << "\n";
//                      k++;
//                  }
//                  else
//                     break;
//              }
//              cout << "-- " << sd.majMark(i) << "\n";
//          }
//          while(k < sd.minCnt())
//          {
//              cout << " - " << sd.minMark(i) << "\n";
//              k++;
//          }
//      }
//
//------------------------------------------------------------

static const double step_eps = 1.0e-3;
static const double border_eps = 1.0e-10;

static bool limRange(double &val, double v1, double v2, double eps_rel = 0.0,
   double eps_abs = 0.0)
      {

    bool rv = TRUE;
    double vmin = MusECore::qwtMin(v1, v2);
    double vmax = MusECore::qwtMax(v1, v2);
    double delta_min = MusECore::qwtMax(MusECore::qwtAbs(eps_rel * vmin), MusECore::qwtAbs(eps_abs));
    double delta_max = MusECore::qwtMax(MusECore::qwtAbs(eps_rel * vmax), MusECore::qwtAbs(eps_abs));

    if (val < vmin)
    {
	if (val < vmin - delta_min) rv = FALSE;
	val = vmin;
    }
    else if (val > vmax)
    {
	if (val > vmax + delta_max) rv = FALSE;
	val = vmax;
    }
    return rv;

}

//------------------------------------------------------------
//.F	ScaleDiv::ScaleDiv
//	Construct a ScaleDiv instance.
//
//.u	Syntax
//.f	 ScaleDiv::ScaleDiv()
//------------------------------------------------------------

ScaleDiv::ScaleDiv()
      {
      d_lBound = 0.0;
      d_hBound = 0.0;
      d_majStep = 0.0;
      d_log = FALSE;
      }

//------------------------------------------------------------
//.F	ScaleDiv::~ScaleDiv
//	Destroy a ScaleDiv instance.
//
//.u	Syntax
//.f	 ScaleDiv::~ScaleDiv()
//------------------------------------------------------------

ScaleDiv::~ScaleDiv()
      {
      }

//------------------------------------------------------------
//.F	ScaleDiv::ScaleDiv
//      Copy Constructor
//
//.u	Syntax
//.f	 ScaleDiv::ScaleDiv(const ScaleDiv &s)
//
//.u	Parameters
//.p	const ScaleDiv &s -- scale division to be copied
//------------------------------------------------------------

ScaleDiv::ScaleDiv(const ScaleDiv &s)
      {
      copy(s);
      }

//------------------------------------------------------------
//.F	ScaleDiv::operator=
//	Assignment operator
//
//.u	Syntax
//.f	ScaleDiv & ScaleDiv::operator=(const ScaleDiv &s)
//
//.u	Parameters
//.p	const ScaleDiv &s -- scale divison to be assigned
//------------------------------------------------------------

ScaleDiv& ScaleDiv::operator=(const ScaleDiv &s)
      {
      copy(s);
      return *this;
      }

//------------------------------------------------------------
//.F	ScaleDiv::copy
//	Copy member data from another ScaleDiv instance.
//
//.u	Syntax
//.f	void ScaleDiv::copy(const ScaleDiv &s)
//
//.u	Parameters
//.p	const ScaleDiv &s
//------------------------------------------------------------

void ScaleDiv::copy(const ScaleDiv &s)
      {
      d_lBound = s.d_lBound;
      d_hBound = s.d_hBound;
      d_log = s.d_log;
      d_majStep = s.d_majStep;
      d_minMarks = s.d_minMarks;
      d_majMarks = s.d_majMarks;
      }

//------------------------------------------------------------
//.F	ScaleDiv::rebuild
//	Build a scale width major and minor divisions
//
//.p
//	double x1	--	first boundary value
//	double x2	--	second boundary value
//	int maxMajSteps	--	max. number of major step intervals
//	int maxMinSteps	--	max. number of minor step intervals
//      bool log	--	logarithmic division (TRUE/FALSE)
//	double step 	--	fixed major step width. Defaults to 0.0.
//	bool ascend	--	if TRUE, sort in ascending order from min(x1, x2)
//				to max(x1, x2). If FALSE, sort in the direction
//				from x1 to x2.	Defaults to TRUE.
//
//.u Return Value
//	True if the arrays have been allocated successfully.
//
//.u Description
//	If no fixed step width is specified or if it is set to 0, the
//	major step width will be calculated automatically according to the
//	the value of maxMajSteps. The maxMajSteps parameter has no effect
//	if a fixed step size is specified. The minor step width is always
//	calculated automatically.
//	If the step width is to be calculated automatically, the algorithm
//	tries to find reasonable values fitting into the scheme {1,2,5}*10^n
//	with an integer number n for linear scales.
//	For logarithmic scales, there are three different cases:
//.i
//	-- If the major step width is one decade, the minor marks
//		will fit into one of the schemes {1,2,...9}, {2,4,6,8}, {2,5} or {5},
//		depending on the maxMinSteps parameter.
//	-- If the major step size spans
//		more than one decade, the minor step size will be {1,2,5}*10^n decades
//		with a natural number n.
//	-- If the whole range is less than one decade, a linear scale
//		division will be built
//	
//.u Note
//	For logarithmic scales, the step width is measured in decades.
//------------------------------------------------------------

bool ScaleDiv::rebuild(double x1, double x2, int maxMajSteps, int maxMinSteps,
   bool log, double step, bool ascend)
{

  int rv;

  d_lBound = MusECore::qwtMin(x1, x2);
  d_hBound = MusECore::qwtMax(x1, x2);
  d_log = log;

  if (d_log)
      rv = buildLogDiv(maxMajSteps,maxMinSteps,step);
  else
      rv = buildLinDiv(maxMajSteps, maxMinSteps, step);

  if ((!ascend) && (x2 < x1))
  {
      d_lBound = x1;
      d_hBound = x2;
      MusECore::qwtTwistArray(d_majMarks.data(), d_majMarks.size());
      MusECore::qwtTwistArray(d_minMarks.data(), d_minMarks.size());
  }

  return rv;

}

//------------------------------------------------------------
//.F	ScaleDiv::buildLinDiv
//	Build a linear scale division in ascending order
//
//.u	Syntax
//.f	bool ScaleDiv::buildLinDiv(int majSteps, int minSteps, double step)
//
//.u	Parameters
//.p	int maxSteps -- max. number of step intervals
//	double step -- fixed step width
//
//.u	Return Value
//		TRUE if array has been successfully resized
//
//.u	Description
//		If the 'step' parameter is set to 0.0, this function
//		cal[culates the step width automatically according to
//		the value of 'maxSteps'. MaxSteps must be greater than or
//		equal to 2. It will be guessed if an invalid value is specified.
//		The maximum possible number of steps is	limited to 10000.
//		The maxSteps parameter has no effect if a fixed step width is
//		specified.
//
//.u	Note
//	This function uses the data members d_lBound and d_hBound and assumes
//	that d_hBound > d_lBound.
//------------------------------------------------------------

bool ScaleDiv::buildLinDiv(int maxMajSteps, int maxMinSteps, double step)
      {

    int nMaj, nMin, minSize, i0,i,k;
    double val, mval;
    double firstTick, lastTick;
    double minStep;
    QVector<double> buffer;
    bool rv = TRUE;

    // parameter range check
    maxMajSteps = MusECore::qwtMax(1, maxMajSteps);
    maxMinSteps = MusECore::qwtMax(0, maxMinSteps);
    step = MusECore::qwtAbs(step);

    // reset vectors
    d_minMarks.resize(0);
    d_majMarks.resize(0);

    if (d_lBound == d_hBound) return TRUE;

    //
    // Set up major divisions
    //
    if (step == 0.0)
       d_majStep = MusECore::qwtCeil125(MusECore::qwtAbs(d_hBound - d_lBound) * 0.999999
			      / double(maxMajSteps));
    else
       d_majStep = step;

    if (d_majStep == 0.0) return TRUE;

    firstTick = ceil( (d_lBound - step_eps * d_majStep) / d_majStep) * d_majStep;
    lastTick = floor( (d_hBound + step_eps * d_majStep) / d_majStep) * d_majStep;

    nMaj = MusECore::qwtMin(10000, int(rint((lastTick - firstTick) / d_majStep)) + 1);
    
    d_majMarks.resize(nMaj);
    MusECore::qwtLinSpace(d_majMarks.data(), d_majMarks.size(), firstTick, lastTick);

    //
    // Set up minor divisions
    //
    if (maxMinSteps < 1) // no minor divs
       return TRUE;

    minStep = MusECore::qwtCeil125( d_majStep  /  double(maxMinSteps) );

    if (minStep == 0.0) return TRUE;

    nMin = MusECore::qwtAbs(int(rint(d_majStep / minStep))) - 1; // # minor steps per interval

    // Do the minor steps fit into the interval?
    if ( MusECore::qwtAbs(double(nMin +  1) * minStep - d_majStep) >  step_eps * d_majStep)
    {
	nMin = 1;
	minStep = d_majStep * 0.5;
    }

    // Are there minor ticks below the first major tick?
    if (d_majMarks[0] > d_lBound )
       i0 = -1;	
    else
       i0 = 0;

    // resize buffer to the maximum possible number of minor ticks
    buffer.resize(nMin * (nMaj + 1));

    // calculate minor ticks
    if (rv)
    {
	minSize = 0;
	for (i = i0; i < (int)d_majMarks.size(); i++)
	{
	    if (i >= 0)
	       val = d_majMarks[i];
	    else
	       val = d_majMarks[0] - d_majStep;

	    for (k=0; k< nMin; k++)
	    {
		mval = (val += minStep);
		if (limRange(mval, d_lBound, d_hBound, border_eps))
		{
		    buffer[minSize] = mval;
		    minSize++;
		}
	    }
	}
        //d_minMarks.duplicate(buffer.data(), minSize);
        d_minMarks.resize(minSize);
        qCopy(buffer.data(), buffer.data() + minSize, d_minMarks.begin());
    }

    return rv;
      }

//------------------------------------------------------------
//.F	ScaleDiv::buildLogDiv
//	Build a logarithmic scale division
//
//.u	Syntax
//.f	bool ScaleDiv::buildLogDiv(int maxMajSteps, int maxMinSteps, int majStep)
//
//.u	Parameters
//.p	int maxMajSteps, int maxMinSteps, int majStep
//
//.u	Return Value
//		True if memory has been successfully allocated
//
//.u	Note
//	This function uses the data members d_lBound and d_hBound and assumes
//	that d_hBound > d_lBound.
//------------------------------------------------------------

bool ScaleDiv::buildLogDiv(int maxMajSteps, int maxMinSteps, double majStep)
      {
    double firstTick, lastTick;
    double lFirst, lLast;
    double val, sval, minStep, minFactor;
    int nMaj, nMin, minSize, i, k, k0, kstep, kmax, i0;
    int rv = TRUE;
    double width;

    QVector<double> buffer;


    // Parameter range check
    maxMajSteps = MusECore::qwtMax(1, MusECore::qwtAbs(maxMajSteps));
    maxMinSteps = MusECore::qwtMax(0, MusECore::qwtAbs(maxMinSteps));
    majStep = MusECore::qwtAbs(majStep);

    // boundary check
    limRange(d_hBound, LOG_MIN, LOG_MAX);
    limRange(d_lBound, LOG_MIN, LOG_MAX);

    // reset vectors
    d_minMarks.resize(0);
    d_majMarks.resize(0);

    if (d_lBound == d_hBound) return TRUE;

    // scale width in decades
    width = log10(d_hBound) - log10(d_lBound);

    // scale width is less than one decade -> build linear scale
    if (width < 1.0)
    {
	rv = buildLinDiv(maxMajSteps, maxMinSteps, 0.0);
	// convert step width to decades
	if (d_majStep > 0)
	   d_majStep = log10(d_majStep);

	return rv;
    }

    //
    //  Set up major scale divisions
    //
    if (majStep == 0.0)
       d_majStep = MusECore::qwtCeil125( width * 0.999999 / double(maxMajSteps));
    else
       d_majStep = majStep;

    // major step must be >= 1 decade
    d_majStep = MusECore::qwtMax(d_majStep, 1.0);


    lFirst = ceil((log10(d_lBound) - step_eps * d_majStep) / d_majStep) * d_majStep;
    lLast = floor((log10(d_hBound) + step_eps * d_majStep) / d_majStep) * d_majStep;

    firstTick = pow(10.0, lFirst);
    lastTick = pow(10.0, lLast);

    nMaj = MusECore::qwtMin(10000, int(rint(MusECore::qwtAbs(lLast - lFirst) / d_majStep)) + 1);

    d_majMarks.resize(nMaj);
    MusECore::qwtLogSpace(d_majMarks.data(), d_majMarks.size(), firstTick, lastTick);


    //
    // Set up minor scale divisions
    //

    if ((d_majMarks.size() < 1) || (maxMinSteps < 1)) return TRUE; // no minor marks

    if (d_majStep < 1.1)			// major step width is one decade
    {
	if (maxMinSteps >= 8)
	{
	    k0 = 2;
	    kmax = 9;
	    kstep = 1;
	    minSize = (d_majMarks.size() + 1) * 8;
	}
	else if (maxMinSteps >= 4)
	{
	    k0 = 2;
	    kmax = 8;
	    kstep = 2;
	    minSize = (d_majMarks.size() + 1) * 4;
	}
	else if (maxMinSteps >= 2)
	{
	    k0 = 2;
	    kmax = 5;
	    kstep = 3;
	    minSize = (d_majMarks.size() + 1) * 2;
	}
	else
	{
	    k0 = 5;
	    kmax = 5;
	    kstep = 1;
	    minSize = (d_majMarks.size() + 1);
	}
	
	// resize buffer to the max. possible number of minor marks
	buffer.resize(minSize);

	// Are there minor ticks below the first major tick?
	if ( d_lBound < firstTick )
	    i0 = -1;
	else
	   i0 = 0;
	
	minSize = 0;
	for (i = i0; i< (int)d_majMarks.size(); i++)
	{
	    if (i >= 0)
	       val = d_majMarks[i];
	    else
	       val = d_majMarks[0] / pow(10.0, d_majStep);
	
	    for (k=k0; k<= kmax; k+=kstep)
	    {
		sval = val * double(k);
		if (limRange(sval, d_lBound, d_hBound, border_eps))
		{
		    buffer[minSize] = sval;
		    minSize++;
		}
	    }
	}

	// copy values into the minMarks array
	//d_minMarks.duplicate(buffer.data(), minSize);
        d_minMarks.resize(minSize);
        qCopy(buffer.data(), buffer.data() + minSize, d_minMarks.begin());


    }
    else				// major step > one decade
    {
	
	// substep width in decades, at least one decade
	minStep = MusECore::qwtCeil125( (d_majStep - step_eps * (d_majStep / double(maxMinSteps)))
			 /  double(maxMinSteps) );
	minStep = MusECore::qwtMax(1.0, minStep);

	// # subticks per interval
	nMin = int(rint(d_majStep / minStep)) - 1;

	// Do the minor steps fit into the interval?
	if ( MusECore::qwtAbs( double(nMin + 1) * minStep - d_majStep)  >  step_eps * d_majStep)
	    nMin = 0;

	if (nMin < 1) return TRUE;		// no subticks

	// resize buffer to max. possible number of subticks
	buffer.resize((d_majMarks.size() + 1) * nMin );
	
	// substep factor = 10^substeps
	minFactor = MusECore::qwtMax(pow(10,minStep), 10.0);

	// Are there minor ticks below the first major tick?
	if ( d_lBound < firstTick )
	    i0 = -1;
	else
	   i0 = 0;
	
	minSize = 0;
	for (i = i0; i< (int)d_majMarks.size(); i++)
	{
	    if (i >= 0)
	       val = d_majMarks[i];
	    else
	       val = firstTick / pow(10.0, d_majStep);
	
	    for (k=0; k< nMin; k++)
	    {
		sval = (val *= minFactor);
		if (limRange(sval, d_lBound, d_hBound, border_eps))
		{
		    buffer[minSize] = sval;
		    minSize++;
		}
	    }
	}
	//d_minMarks.duplicate(buffer.data(), minSize);
        d_minMarks.resize(minSize);
        qCopy(buffer.data(), buffer.data() + minSize, d_minMarks.begin());

    }

    return rv;
}

//------------------------------------------------------------
//.F	ScaleDiv::operator==
//	Equality operator
//
//.u	Syntax
//.f	int ScaleDiv::operator==(const ScaleDiv &s)
//
//.u	Parameters
//.p	const ScaleDiv &s
//
//.u	Return Value
//		TRUE if this instance is equal to s
//------------------------------------------------------------

int ScaleDiv::operator==(const ScaleDiv &s) const
      {
      if (d_lBound != s.d_lBound)
            return 0;
      if (d_hBound != s.d_hBound)
            return 0;
      if (d_log != s.d_log)
            return 0;
      if (d_majStep != s.d_majStep)
            return 0;
      if (d_majMarks != s.d_majMarks)
            return 0;
      return (d_minMarks == s.d_minMarks);
      }

//------------------------------------------------------------
//.F	ScaleDiv::operator!=
//	Inequality
//
//.u	Syntax
//.f	int ScaleDiv::operator!=(const ScaleDiv &s)
//
//.u	Parameters
//.p	const ScaleDiv &s
//
//.u	Return Value
//		TRUE if this instance is not equal to s
//------------------------------------------------------------

int ScaleDiv::operator!=(const ScaleDiv &s) const
      {
      return (!(*this == s));
      }

//------------------------------------------------------------
//.F	ScaleDiv::reset
//	Detach the shared data and set everything to zero.
//
//.u	Syntax
//.f	void ScaleDiv::reset()
//------------------------------------------------------------

void ScaleDiv::reset()
      {
      // reset vectors
      d_minMarks.resize(0);
      d_majMarks.resize(0);


      d_lBound = 0.0;
      d_hBound = 0.0;
      d_majStep = 0.0;
      d_log = FALSE;
      }

} // namespace MusEGui






