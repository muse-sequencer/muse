//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: sclif.cpp,v 1.1.1.1 2003/10/27 18:55:10 wschweer Exp $

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

#include "sclif.h"

namespace MusEGui {

//  ScaleIf - An interface class for widgets containing a scale
//
//	This interface class is used to provide classes
//	with a protected ScaleDraw member and a public
//	interface to access that scale.
//
//	The primary purpose of this class is to define
//	a common interface for classes which are supposed to
//	contain a ScaleDraw class. It provides a protected
//	ScaleDraw member
//      called d_scale and a couple of public member functions
//      which allow direct but restricted access
//	to this scale by the user.
//	Widgets derived from this class have
//	to implement the member function scaleChange(),
//	which is called to notify changes of the
//	scale parameters and usually requires repainting or
//	resizing respectively.
//	In general, a class derived from ScaleIf is
//      expected to manage the division and the position of its scale internally
//	when no user-defined scale is set. It should take the d_maxMinor
//	and d_maxMajor members into account, which can be set by the user.
//	An implementation can check if a user-defined scale is set by calling the
//	@ScaleIf::hasUserScale@ member.

//------------------------------------------------------------
//	ScaleIf::ScaleIf
//  	Construct a ScaleIf instance
//
//	Syntax
//	 ScaleIf::ScaleIf()
//------------------------------------------------------------

ScaleIf::ScaleIf()
      {
      d_userScale = FALSE;
      d_maxMajor = 5;
      d_maxMinor = 3;
      d_scale.setScale(0.0,100.0,d_maxMajor, d_maxMinor);
      }

//------------------------------------------------------------
//	ScaleIf::setScale (1)
//  	Specify a user-defined scale.
//
//	Syntax
//	void ScaleIf::setScale(double vmin, double vmax, int logarithmic)
//
//	Parameters
//	double vmin, double vmax	-- boundary values
//  int logarithmic	--	If != 0, Build a logarithmic scale
//
//	Description
//  	By default, the widget is supposed to control the range of its scale
//  	automatically,	but sometimes it is desirable to have a user-defined
//  	scale which is not in sync with
//  	the widget's range, e.g. if a logarithmic scale is needed
//  	(sliders don't support that) or if the scale is required
//  	to have a fixed range (say 0...100%), independent of the
//  	widget's range.
//
//	See also
//  @ScaleIf::autoScale@
//------------------------------------------------------------

void ScaleIf::setScale(double vmin, double vmax, int logarithmic)
      {
      setScale(vmin,vmax,0.0,logarithmic);
      }

//------------------------------------------------------------
//	ScaleIf::setScale (2)
//  	Specify a user-defined scale.
//
//	Syntax
//	void ScaleIf::setScale(double vmin, double vmax, int logarithmic)
//
//	Parameters
//	double vmin, double vmax	-- interval boundaries
//    int step			-- major step size
//    int logarithmic			-- If != 0, build a logarithmic scale
//
//	Description
//  	By default, the widget is supposed to control the range of its scale
//  	automatically,	but sometimes it is desirable to have a user-defined
//  	scale which is not in sync with
//  	the widget's range, e.g. if a logarithmic scale is needed
//  	(sliders don't support that) or if the scale is required
//  	to have a fixed range (say 0...100%), independent of the
//  	widget's range.
//------------------------------------------------------------

void ScaleIf::setScale(double vmin, double vmax, double step, int logarithmic)
      {
      ScaleDiv oldscl(d_scale.scaleDiv());

      d_scale.setScale(vmin, vmax, d_maxMajor, d_maxMinor, step, logarithmic);
      d_userScale = TRUE;
      if (oldscl != d_scale.scaleDiv())
            scaleChange();
      }

//------------------------------------------------------------
//	Scale::setScale
//  Assign a user-defined scale division
//
//	Syntax
//	void Scale::setScale(const ScaleDiv &s)
//
//	Parameters
//	const ScaleDiv &s -- scale division
//------------------------------------------------------------

void ScaleIf::setScale(const ScaleDiv &s)
      {
      d_scale.setScale(s);
      scaleChange();
      }

//------------------------------------------------------------
//	ScaleIf::autoScale
//  	Advise the widget to control the scale range
//  	internally.
//	Syntax
//	void ScaleIf::autoScale
//
//	Description
//  	Autoscaling is on by default.
//------------------------------------------------------------

void ScaleIf::autoScale()
      {
      if (!d_userScale) {
            d_userScale = FALSE;
            scaleChange();
            }
      }

//------------------------------------------------------------
//	ScaleIf::setScaleMaxMajor
//  	Set the maximum number of major tick intervals.
//
//	Syntax
//	void ScaleIf::setScaleMaxMajor(int ticks)
//
//	Parameters
//	int ticks		--		maximal number of major ticks.
//
//	Description
//  	The scale's major ticks are calculated automatically such that
//  	the number of major intervals does not exceed <ticks>.
//  	The default value is 5.
//------------------------------------------------------------

void ScaleIf::setScaleMaxMajor(int ticks)
      {
      if (ticks != d_maxMajor) {
            d_maxMajor = ticks;
            d_scale.setScale(d_scale.scaleDiv().lBound(), d_scale.scaleDiv().hBound(),
               d_maxMajor, d_maxMinor, 0.0,d_scale.scaleDiv().logScale());
            scaleChange();
            }
      }

//------------------------------------------------------------
//  ScaleIf::setScaleMaxMinor
//    Set the maximum number of minor tick intervals
//
//	Syntax
//	void ScaleIf::setScaleMaxMinor(int ticks)
//
//	Parameters
//	int ticks
//
//	Description
//  	The scale's minor ticks are calculated automatically such that
//  	the number of minor intervals does not exceed <ticks>.
//  	The default value is 3.
//------------------------------------------------------------

void ScaleIf::setScaleMaxMinor(int ticks)
      {
      if ( ticks != d_maxMinor) {
            d_maxMinor = ticks;
            d_scale.setScale(d_scale.scaleDiv().lBound(), d_scale.scaleDiv().hBound(),
               d_maxMajor, d_maxMinor, 0.0, d_scale.scaleDiv().logScale());
            scaleChange();
            }
      }

} // namespace MusEGui
