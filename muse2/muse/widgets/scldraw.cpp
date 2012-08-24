//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: scldraw.cpp,v 1.1.1.1 2003/10/27 18:54:36 wschweer Exp $
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

#include <QPainter>

#include "mmath.h"
#include "scldraw.h"


namespace MusEGui {

int const ScaleDraw::minLen = 10;

const double step_eps = 1.0e-6;
static const double WorstCase = -8.8888888888888888888888e-88;

//------------------------------------------------------------
//.H ScaleDraw | 3 | 30/08/97 | Qwt Widget Library | Qwt Programmer's Manual
//.I scldraw Different Scales
//.U NAME
//	ScaleDraw - A class for drawing scales
//
//.U SYNOPSIS
//	#include <qwt_scldraw.h>
//
//.U DESCRIPTION
//	ScaleDraw can be used to draw linear or logarithmic scales.
//	A scale has an origin,
//	an orientation and a length, which all can be specified with
//	@ScaleDraw::setGeometry@.
//	After a scale division has been specified as a @^QwtScaleDiv@ object
//	using @ScaleDraw::setScale (1)@
//	or determined internally using @ScaleDraw::setScale (2)@,
//	the scale can be drawn with the @QwtScaleDiv::draw@ member.
//
//.U INHERITED CLASSES
//	@QwtDiMap@
//
//.U PUBLIC MEMBERS
//.R
//     ScaleDraw::ScaleDraw -- constructor
//     ScaleDraw::setScale (1)  -- set scale using QwtScaleDiv
//     ScaleDraw::setScale (2)  -- set scale directly
//     ScaleDraw::setGeometry	  -- specify geometry
//     ScaleDraw::setAngleRange	-- specify angle range for round scales
//     ScaleDraw::setLabelFormat -- set number format
//     ScaleDraw::scalediv -- return scale division
//     ScaleDraw::orientation -- return orientation
//     ScaleDraw::maxBoundingRect -- return maximum bounding rectangle
//     ScaleDraw::maxWidth	--	return maximum width
//     ScaleDraw::maxHeight -- return maximum height
//     ScaleDraw::maxLabelWidth -- return maximum width of the number labels
//     ScaleDraw::draw	-- draw the scale
//
//.U STATIC DATA MEMBERS
//.t
//	enum ScaleDraw::Orientation { Left, Right, Top, Bottom, Round } --
//		Scale orientation
//------------------------------------------------------------

//------------------------------------------------------------
//.U MEMBER FUNCTION DESCRIPTION
//------------------------------------------------------------

//------------------------------------------------------------
//
//.F ScaleDraw::ScaleDraw
//
// Constructor
//.u Description
//	The range of the scale is initialized to [0, 100],
//	the angle range is set to [-135, 135], the geometry
//	is initialized such that the origin is at (0,0), the
//	length is 100, and the orientation is ScaleDraw::Bottom.
//
//------------------------------------------------------------
ScaleDraw::ScaleDraw()
{
/*    d_hpad   = 6;
    d_vpad   = 3;
    d_majLen = 8;
    d_medLen = 6;
    d_minLen = 4;
  */

    d_hpad   = 3;
    d_vpad   = 1;
    d_majLen = 4;
    d_medLen = 3;
    d_minLen = 2;

    d_minAngle = -135 * 16;
    d_maxAngle = 135 * 16;
    d_fmt = 'g';
    d_prec = 4;

    // initialize scale and geometry
    setGeometry(0,0,100,Bottom);
    setScale(0,100,0,0,10);
}


//------------------------------------------------------------
//
//.F	ScaleDraw::setScale (1)
//	Adjust the range of the scale
//
//.u	Syntax
//.f	void ScaleDraw::setScale(double x1, double x2, double step, int logscale)
//
//.u	Parameters
//.p	double x1	--	value at the left/low endpoint of the scale
//	double x2	--	value at the right/high endpoint of the scale
//	double step	--	step size (default : 0.0)
//	int logscale	--	logarithmic scale (default : 0)
//
//.u	Description
//	If step == 0.0, the step width is calculated automatically
//	dependent on the maximal number of scale ticks.
//	
//------------------------------------------------------------
void ScaleDraw::setScale(double x1, double x2, int maxMajIntv,
			    int maxMinIntv, double step, int logscale)
{
    d_scldiv.rebuild( x1, x2, maxMajIntv, maxMinIntv, logscale, step, FALSE );
    setDblRange( d_scldiv.lBound(), d_scldiv.hBound(), d_scldiv.logScale());
}


//------------------------------------------------------------
//
//.F	ScaleDraw::setScale (2)
//	Change the scale division
//
//.u	Syntax
//.f	void ScaleDraw::setScale(QwtAutoScale &as)
//
//.u	Parameters
//.p	const QwtScaleDiv& sd -- new scale division
//
//------------------------------------------------------------

void ScaleDraw::setScale(const ScaleDiv &s)
{
    d_scldiv = s;
    setDblRange(d_scldiv.lBound(),d_scldiv.hBound(),d_scldiv.logScale());
}



//------------------------------------------------------------
//.F ScaleDraw::draw
//	Draw the scale
//.u Parameters
//.p	QPainter *p  -- the  painter
//------------------------------------------------------------

void ScaleDraw::draw(QPainter *p) const
      {
      double val,hval,majTick;

      int i,k,kmax;

    for (i=0; i< d_scldiv.majCnt(); i++)
    {
	val = d_scldiv.majMark(i);
	drawTick(p, val, d_majLen);
	drawLabel(p, val);
    }

    if (d_scldiv.logScale())
    {
	for (i=0; i< d_scldiv.minCnt(); i++)
	{
	    drawTick(p,d_scldiv.minMark(i),d_minLen);
	}
    }
    else
    {
	k = 0;
	kmax = d_scldiv.majCnt() - 1;
	if (kmax > 0)
	{
	   majTick = d_scldiv.majMark(0);
	   hval = majTick - 0.5 * d_scldiv.majStep();

	   for (i=0; i< d_scldiv.minCnt(); i++)
	   {
	       val = d_scldiv.minMark(i);
	       if  (val > majTick)
	       {
		   if (k < kmax)
		   {
		       k++;
		       majTick = d_scldiv.majMark(k);
		   }
		   else
		   {
		       majTick += d_scldiv.majMark(kmax) + d_scldiv.majStep();
		   }
		   hval = majTick - 0.5 * d_scldiv.majStep();
		
	       }
	       if (MusECore::qwtAbs(val-hval) < step_eps * d_scldiv.majStep())
		  drawTick(p, val, d_medLen);
	       else
		  drawTick(p, val, d_minLen);
	   }
	}
    }

    //
    // draw backbone
    //
    //if (d_baseEnabled)
       drawBackbone(p);

}


//------------------------------------------------------------
//.F ScaleDraw::drawTick
//	Draws a singls scale tick
//
//.u Parameters
//.p  QPainter *p, double val, int len
//------------------------------------------------------------

void ScaleDraw::drawTick(QPainter *p, double val, int len) const
      {
      int tval = transform(val);
      double arc;
  int x1, x2, y1, y2;

  switch(d_orient)
  {
  case Right:

      p->drawLine(d_xorg, tval, d_xorg + len, tval);
      break;

  case Bottom:

      p->drawLine(tval, d_yorg, tval, d_yorg + len);
      break;

  case Left:

      p->drawLine(d_xorg, tval, d_xorg - len, tval);
      break;

  case Round:

      if ((tval <= d_minAngle + 359 * 16) || (tval >= d_minAngle - 359 * 16))
      {
	  arc = double(tval) / 16.0 * M_PI / 180.0;
	  x1 = MusECore::qwtInt(d_xCenter + sin(arc) * d_radius);
	  x2 = MusECore::qwtInt(d_xCenter + sin(arc) * (d_radius + double(len)));
	  y1 = MusECore::qwtInt(d_yCenter - cos(arc) * d_radius);
	  y2 = MusECore::qwtInt(d_yCenter - cos(arc) * (d_radius + double(len)));
	  p->drawLine(x1, y1, x2, y2);
      }
      break;

  case Top:
  default:

      p->drawLine(tval, d_yorg, tval, d_yorg - len);
      break;


  }

}




//------------------------------------------------------------
//.-
//.F ScaleDraw::drawLabel
//	Draws the number label for a major scale tick
//
//.u Parameters
//.p  QPainter *p,  double val
//
//------------------------------------------------------------
void ScaleDraw::drawLabel(QPainter *p, double val) const
{

    static QString label;
    static double pi_4 = M_PI * 0.25;
    static double pi_75 = M_PI * 0.75;

    double arc;
    int xpos, ypos;
    int tval;

    QFontMetrics fm = p->fontMetrics();

    tval = transform(val);

    // correct rounding errors if val = 0
    if ((!d_scldiv.logScale()) && (MusECore::qwtAbs(val) < MusECore::qwtAbs(step_eps * d_scldiv.majStep())))
       val = 0.0;

    label.setNum(val, d_fmt, d_prec);

    switch(d_orient)
    {
    case Right:
	p->drawText(d_xorg + d_majLen + d_hpad,
		    tval + (fm.ascent()-1) / 2,
		    label);
	break;
    case Left:
	p->drawText(d_xorg - d_majLen - d_hpad - fm.width(label),
		    tval + (fm.ascent() -1) / 2,
		    label);
	break;
    case Bottom:
	p->drawText(tval - (fm.width(label)-1) / 2, d_yorg + d_majLen + d_vpad + fm.ascent(), label);
	break;
    case Round:

	if ((tval > d_minAngle + 359 * 16) || (tval < d_minAngle - 359 * 16))
	   break;
	
	arc = double(tval) / 16.0 * M_PI / 180.0;
	
	// Map arc into the interval -pi <= arc <= pi
	if ((arc < -M_PI) || (arc > M_PI))
	   arc -= floor((arc + M_PI) / M_PI * 0.5) * 2.0 * M_PI;
	
	xpos = 1 + MusECore::qwtInt(d_xCenter + (d_radius + double(d_majLen + d_vpad)) * sin(arc));
	ypos = MusECore::qwtInt(d_yCenter - (d_radius + double(d_majLen + d_vpad)) * cos(arc));
	
	if (arc < -pi_75)
	{
	    p->drawText(xpos - MusECore::qwtInt(double(fm.width(label))
				      * (1.0 + (arc + pi_75) * M_2_PI) ),
			ypos + fm.ascent() - 1,
			label);
	}
	else if (arc < -M_PI_4)
	{
	    p->drawText(xpos - fm.width(label),
			
			
			ypos - MusECore::qwtInt(double(fm.ascent() - 1)
				      * (arc + M_PI_4) * M_2_PI),
			label);
	}
	else if (arc < pi_4)
	{
	    p->drawText(xpos + MusECore::qwtInt(double(fm.width(label))
				      * ( arc - M_PI_4 ) * M_2_PI ),
			ypos,
			label);
	}
	else if (arc < pi_75)
	{
	    p->drawText(xpos,
			ypos + MusECore::qwtInt(double(fm.ascent() - 1)
				      * (arc - M_PI_4) * M_2_PI),
			label);
	}
	else
	{
	    p->drawText(xpos - MusECore::qwtInt(double(fm.width(label))
				      * ( arc - pi_75) * M_2_PI ),
			ypos + fm.ascent() - 1,
			label);
	}
	break;
    case Top:
    default:
	p->drawText(tval - (fm.width(label)-1) / 2, d_yorg - d_majLen - d_vpad, label);
	break;
    }



}

//------------------------------------------------------------
//.-
//.F ScaleDraw::drawBackbone
//	Draws the baseline of the scale
//
//
//.u Parameters
//.p  QPainter *p
//
//------------------------------------------------------------
void ScaleDraw::drawBackbone(QPainter *p) const
{
    int bw2;
    int a1, a2;
    bw2 = p->pen().width() / 2;


    switch(d_orient)
    {
    case Left:
	p->drawLine(d_xorg - bw2, d_yorg, d_xorg - bw2, d_yorg + d_len - 1);
	break;
    case Right:
	p->drawLine(d_xorg + bw2, d_yorg, d_xorg + bw2, d_yorg + d_len - 1);
	break;
    case Round:
	
	a1 = MusECore::qwtMin(i1(), i2()) - 90 * 16;
	a2 = MusECore::qwtMax(i1(), i2()) - 90 * 16;
	
	p->drawArc(d_xorg, d_yorg, d_len,
		   d_len,
		   -a2, a2 - a1 + 1);		// counterclockwise
	
	break;
	
    case Top:
	p->drawLine(d_xorg, d_yorg - bw2, d_xorg + d_len - 1, d_yorg-bw2);
	break;
    case Bottom:
	p->drawLine(d_xorg, d_yorg+bw2, d_xorg + d_len - 1, d_yorg+bw2);
	break;
    default:
	p->drawLine(d_xorg, d_yorg, d_xorg + d_len - 1, d_yorg);
	break;
    }

}


//------------------------------------------------------------
//
//.F ScaleDraw::setGeometry
//	Specify the geometry of the scale
//
//
//.u Parameters
//.p	int xorigin	-- x coordinate of the origin
//	int yorigin	-- y coordinate of the origin
//	int length	-- length or diameter of the scale
//	Orientation o	-- The orientation
//
//.u Description
//
//	The parameters xorigin, yorigin and length have different meanings,
//	dependent on the
//	orientation:
//.t
//	ScaleDraw::Left -- The origin is the topmost point of the
//		baseline. The baseline is a vertical line with the
//		specified length. Scale marks and labels are drawn
//		at the left of the baseline.
//	
//	ScaleDraw::Right -- The origin is the topmost point of the
//		baseline. The baseline is a vertical line with the
//		specified length. Scale marks and labels are drawn
//		at the right of the baseline.
//	
//	ScaleDraw::Top -- The origin is the leftmost point of the
//		baseline. The baseline is a horizontal line with the
//		specified length. Scale marks and labels are drawn
//		above the baseline.
//
//	ScaleDraw::Bottom -- The origin is the leftmost point of the
//		baseline. The baseline is a horizontal line with the
//		specified length. Scale marks and labels are drawn
//		below the baseline.
//
//	ScaleDraw::Round -- The origin is the top left corner of the
//		bounding rectangle of the baseline circle. The baseline
//		is the segment of a circle with a diameter of the specified length.
//		Scale marks and labels are drawn outside the baseline
//		circle.
//
//------------------------------------------------------------
void ScaleDraw::setGeometry(int xorigin, int yorigin, int length, OrientationX o)
{

    d_xorg = xorigin;
    d_yorg = yorigin;
    d_radius = double(length) * 0.5;
    d_xCenter = double(xorigin) + double(length) * 0.5;
    d_yCenter = double(yorigin) + double(length) * 0.5;

    if (length > minLen)
       d_len = length;
    else
       d_len = minLen;

    d_orient = o;

    switch(d_orient)
    {
    case Left:
    case Right:
	setIntRange(d_yorg + d_len - 1, d_yorg);
	break;
    case Round:
	setIntRange(d_minAngle, d_maxAngle);
	break;
    case Top:
    case Bottom:
    default:
	setIntRange(d_xorg, d_xorg + d_len - 1);
	break;
    }
}



//------------------------------------------------------------
//
//.F	ScaleDraw::maxWidth
//	Return the maximum width of the scale for a specified QPainter
//
//.u	Syntax
//.f	int ScaleDraw::maxWidth(QPainter *p)
//
//.u	Parameters
//.p	QPainter *p -- painter
//	bool worst -- if TRUE, assume the worst possible case. If FALSE,
//			calculate the real maximum width, which is more
//			CPU intensive.
//
//------------------------------------------------------------
int ScaleDraw::maxWidth(QPainter *p, bool worst) const
{
    int rv = 0;
    int bw = p->pen().width();

    QString s;

    QFontMetrics fm = p->fontMetrics();

    rv = maxLabelWidth(p,worst);

    switch (d_orient)
    {
    case Left:
    case Right:
	rv += (bw + d_hpad + d_majLen);
	break;
    case Round:
	rv += (bw + d_vpad + d_majLen);
	break;
    case Top:
    case Bottom:
    default:
	rv += d_len;
    }

    return rv;

}

//------------------------------------------------------------
//
//.F	ScaleDraw::maxHeight
//	Return the maximum height of the scale for the
//	specified painter
//
//.u	Syntax
//.f	int ScaleDraw::maxHeight(QPainter *p)
//
//.u	Parameters
//.p	QPainter *p
//
//------------------------------------------------------------
int ScaleDraw::maxHeight(QPainter *p) const
{

    int rv = 0;
    int bw = p->pen().width();

    p->save();
    QFontMetrics fm = p->fontMetrics();

    switch (d_orient)
    {
    case Top:
    case Bottom:
    case Round:
	rv = bw + d_vpad + d_majLen + fm.height();
	break;
    case Left:
    case Right:
    default:
	rv = d_len + ((fm.height() + 1) / 2);
    }

    return rv;

}

//------------------------------------------------------------
//
//.F ScaleDraw:maxBoundingRect
//	Return the maximum bounding rectangle of the scale
//	for a specified painter
//
//.u Parameters
//.p  QPainter *p -- painter
//
//.u Description
//	The bounding rectangle is not very exact for round scales
//	with strange angle ranges.
//
//------------------------------------------------------------
QRect ScaleDraw::maxBoundingRect(QPainter *p) const
{
    int i, wl; //,wmax;
    int a, ar, amin, amax;
    double arc;

    QRect r;

    QFontMetrics fm = p->fontMetrics();

    wl = maxLabelWidth(p, TRUE);

    switch(d_orient)
    {
    case Left:
	
	r = QRect( d_xorg - d_hpad - d_majLen - wl,
		  d_yorg - fm.ascent(),
		  d_majLen + d_hpad + wl,
		  d_len + fm.height());
	break;
	
    case Right:
	
	r = QRect( d_xorg,
		  d_yorg - fm.ascent(),
		  d_majLen + d_hpad + wl,
		  d_len + fm.height());
	break;
	
    case Top:
	
	r = QRect ( d_xorg - wl / 2,
		   d_yorg - d_majLen - fm.ascent(),
		   d_len + wl,
		   d_majLen + d_vpad + fm.ascent());
	break;
	
    case Bottom:
	
	r = QRect ( d_xorg - wl / 2,
		   d_yorg,
		   d_len + wl,
		   d_majLen + d_vpad + fm.height());
	break;
	
    case Round:

	amin = 2880;
	amax = 0;
	ar = 0;

	for (i=0; i< d_scldiv.majCnt(); i++)
	{
	    a = transform(d_scldiv.majMark(i));
	
	    while (a > 2880) a -= 5760;
	    while (a < - 2880) a += 5760;

	    ar = MusECore::qwtAbs(a);

	    if (ar < amin) amin = ar;
	    if (ar > amax) amax = ar;

	}

	for (i=0; i< d_scldiv.minCnt(); i++)
	{
	    a = transform(d_scldiv.majMark(i));
	
	    while (a > 2880) a -= 5760;
	    while (a < - 2880) a += 5760;

	    ar = MusECore::qwtAbs(a);

	    if (ar < amin) amin = ar;
	    if (ar > amax) amax = ar;
	}

	arc = double(amin) / 16.0 * M_PI / 180.0;
	r.setTop(MusECore::qwtInt(d_yCenter - (d_radius + double(d_majLen + d_vpad)) * cos(arc))
		 + fm.ascent() );

	arc = double(amax) / 16.0 * M_PI / 180.0;
	r.setBottom(MusECore::qwtInt(d_yCenter - (d_radius + double(d_majLen + d_vpad)) * cos(arc))
		    + fm.height() );

	//wmax = d_len + d_majLen + d_hpad + wl; DELETETHIS

	r.setLeft(d_xorg - d_majLen - d_hpad - wl);
	r.setWidth(d_len + 2*(d_majLen + d_hpad + wl));
	break;
    }

    return r;

}

//------------------------------------------------------------
//
//.F	ScaleDraw::setAngleRange
//	Adjust the baseline circle segment for round scales.
//
//.u	Syntax
//.f	void ScaleDraw::setAngleRange(double angle1, double angle2)
//
//.u	Parameters
//.p	double angle1, double angle2
//			boundaries of the angle interval in degrees.
//
//.u	Description
//	The baseline will be drawn from min(angle1,angle2) to max(angle1, angle2).
//	The settings have no effect if the scale orientation is not set to
//	ScaleDraw::Round. The default setting is [ -135, 135 ].
//	An angle of 0 degrees corresponds to the 12 o'clock position,
//	and positive angles count in a clockwise direction.
//
//.u    Note
//.i
//	-- The angle range is limited to [-360, 360] degrees. Angles exceeding
//		this range will be clipped.
//	-- For angles more than 359 degrees above or below min(angle1, angle2),
//		scale marks will not be drawn.
//	-- If you need a counterclockwise scale, use @QwtScaleDiv::setRange (1)@
//		or @QwtScaleDiv::setRange (2)@.
//------------------------------------------------------------
void ScaleDraw::setAngleRange(double angle1, double angle2)
{
    int amin, amax;

    angle1 = MusECore::qwtLim(angle1, -360.0, 360.0);
    angle2 = MusECore::qwtLim(angle2, -360.0, 360.0);
    amin = int(rint(MusECore::qwtMin(angle1, angle2) * 16.0));
    amax = int(rint(MusECore::qwtMax(angle1, angle2) * 16.0));

    if (amin == amax)
    {
	amin -= 1;
	amax += 1;
    }

    d_minAngle = amin;
    d_maxAngle = amax;
    setIntRange(d_minAngle, d_maxAngle);

}


//------------------------------------------------------------
//
//.F	ScaleDraw::setLabelFormat
//	Set the number format for the major scale labels
//
//.u	Syntax
//.f	void ScaleDraw::setLabelFormat(char f, int prec)
//
//.u	Parameters
//.p	char f  -- format character
//	int prec -- precision
//
//.u	Description
//	 Format character and precision have the same meaning as for the
//	 QString class.
//
//.u	See also
//	QString::setNum in the Qt manual
//
//------------------------------------------------------------
void ScaleDraw::setLabelFormat(char f, int prec)
{
    d_fmt = f;
    d_prec = prec;
}

//------------------------------------------------------------
//
//.F	ScaleDraw::maxLabelWidth
//	Return the maximum width of a label
//
//.u	Syntax
//.f	int ScaleDraw::maxLabelWidth(QPainter *p, int worst)
//
//.u	Parameters
//.p	QPainter *p -- painter
//	int worst -- If TRUE, take the worst case. If FALSE, take
//			the actual width of the largest label.
//
//------------------------------------------------------------
int ScaleDraw::maxLabelWidth(QPainter *p, int worst) const
{

    int i,rv = 0;
    double val;
    QString s;


    QFontMetrics fm = p->fontMetrics();

    if (worst)			// worst case
    {
	s.setNum(WorstCase, d_fmt, d_prec);
	rv = fm.width(s);
    }
    else				// actual width
    {
	for (i=0;i<d_scldiv.majCnt(); i++)
	{
      val = d_scldiv.majMark(i);
	    // correct rounding errors if val = 0
	    if ((!d_scldiv.logScale()) && (MusECore::qwtAbs(val) < step_eps * MusECore::qwtAbs(d_scldiv.majStep())))
	       val = 0.0;
	    s.setNum(val, d_fmt, d_prec);
	    rv = MusECore::qwtMax(rv,fm.width(s));
	}
    }


    return rv;

}


//------------------------------------------------------------
//
//.F	ScaleDraw::scaleDiv
//	Return the scale division
//
//.u	Syntax
//.f	const QwtScaleDiv & ScaleDraw::scaleDiv() const
//
//.u	See also
//	@^QwtScaleDiv@
//------------------------------------------------------------

//------------------------------------------------------------
//
//.F	ScaleDraw::orientation
//	Return the orientation
//
//.u	Syntax
//.f	int ScaleDraw::orientation() const
//
//.u    See also
//		@ScaleDraw::setGeometry@
//
//------------------------------------------------------------

} // namespace MusEGui









