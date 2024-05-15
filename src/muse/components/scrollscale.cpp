//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: scrollscale.cpp,v 1.2.2.2 2009/11/04 17:43:25 lunar_shuttle Exp $
//  (C) Copyright 1999 Werner Schweer (ws@seh.de)
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

#include <stdio.h>
#include "muse_math.h"

#include <QBoxLayout>
#include <QLabel>
#include <QResizeEvent>
#include <QScrollBar>
#include <QSlider>
#include <QToolButton>
#include <QToolTip>
#include <QStyle>

#include "scrollscale.h"
#include "icons.h"

namespace MusEGui {


//---------------------------------------------------------
//   stepScale
//    increase/decrease scale by single step
//---------------------------------------------------------
void ScrollScale::stepScale ( bool up )
{
    setMag(scale2mag(up ? scaleVal + 1 : scaleVal - 1));
}

//---------------------------------------------------------
//   setScale
//    "val" - slider value in range 0-convertQuickZoomLevelToMag(zoomLevels-1)
//---------------------------------------------------------

void ScrollScale::setScale ( int val, int pos_offset )
{
	int off = offset();
	int old_scale_val = scaleVal;

	scaleVal = mag2scale(val);

	//fprintf(stderr, "scaleMin %d scaleMax %d val=%d emit scaleVal=%d\n", scaleMin, scaleMax, val, scaleVal);
	emit scaleChanged ( scaleVal );
	if ( !noScale )
		setRange ( minVal, maxVal );

	int i = ( scroll->orientation() == Qt::Horizontal ) ? width() : height();
	int pos, pmax;
// REMOVE Tim. wave. Changed.
// 	if ( scaleVal < 1 )
	if ( scaleVal < 0 )
	{
		pos = ( off-scaleVal/2 ) / ( -scaleVal );
		pmax = ( maxVal-scaleVal-1 ) / ( -scaleVal ) - i;
	}
	else
	{
		pos = off * scaleVal;
		pmax = maxVal * scaleVal - i;
	}
	
	// Zoom at cursor support...
	if(pos_offset != 0)
	{
		double oscale = old_scale_val;
		double nscale = scaleVal;
// REMOVE Tim. wave. Changed.
// 		if(old_scale_val < 1)
		if(old_scale_val < 0)
			oscale = 1.0 / -old_scale_val;
// REMOVE Tim. wave. Changed.
// 		if(scaleVal < 1)
		if(scaleVal < 0)
			nscale = 1.0 / -scaleVal;
		double scale_fact = nscale / oscale;
		int pos_diff = (int)((double)pos_offset * scale_fact - (double)pos_offset + 0.5);  // 0.5 for round-off
		pos += pos_diff;
	}
	
	if(pos > pmax)
		pos = pmax;
	setPos(pos);
}

//---------------------------------------------------------
//   setMag
//---------------------------------------------------------

void ScrollScale::setMag ( int cs, int pos_offset )
{
	scale->blockSignals(true);
	scale->setValue ( cs );
	scale->blockSignals(false);
	setScale ( cs, pos_offset );
}

//---------------------------------------------------------
//   setRange
//    min,max  ticks
//---------------------------------------------------------

void ScrollScale::setRange ( int min, int max )
{
//      if ((min != minVal) && (max != maxVal))
//            return;
	minVal = min;
	maxVal = max;
	int i = ( scroll->orientation() == Qt::Horizontal ) ? width() : height();

	if ( !noScale )
	{
// REMOVE Tim. wave. Changed.
// 		if ( scaleVal < 1 )
		if ( scaleVal < 0 )
		{
			min = minVal / ( -scaleVal );
			max = ( maxVal-scaleVal-1 ) / ( -scaleVal ) - i;
		}
		else
		{
			min = minVal * scaleVal;
			max = maxVal * scaleVal - i;
		}
	}
	else
		max -= i;
	if ( max < 0 )
		max = 0;
	if ( min < 0 )
		min = 0;
	if ( min > max )
		max = min;
  
	scroll->setRange ( min, max );

	// qt doesn't check this...
	if ( scroll->value() < min )
		scroll->setValue ( min );
	if ( scroll->value() > max )
		scroll->setValue ( max );
	scroll->setSingleStep(20);
	scroll->setPageStep(i);
}

//---------------------------------------------------------
//   setPos
//    pos in pixel
//---------------------------------------------------------

void ScrollScale::setPos ( unsigned pos )
{
	scroll->setValue ( pos );
}

//---------------------------------------------------------
//   setPosNoLimit
//    pos in pixel
//---------------------------------------------------------

void ScrollScale::setPosNoLimit ( unsigned pos )
{
	//printf ( "ScrollScale::setPosNoLimit pos:%d scaleVal:%d offset ticks:%d\n", pos, scaleVal, pos2offset ( pos ) );

	if((int)pos > scroll->maximum())
		scroll->setMaximum(pos);
	scroll->setValue(pos);
}

//---------------------------------------------------------
//   resizeEvent
//---------------------------------------------------------

void ScrollScale::resizeEvent ( QResizeEvent* ev)
{
	QWidget::resizeEvent(ev);
	emit scaleChanged ( scaleVal );
	if ( !noScale )
		setRange ( minVal, maxVal );
}

//---------------------------------------------------------
//   ScrollScale
//---------------------------------------------------------

ScrollScale::ScrollScale ( int s1, int s2, int cs, int max_, Qt::Orientation o,
                           QWidget* parent, int min_, bool inv, double bas )
		: QWidget ( parent )
{
	noScale     = false;
	_page        = 0;
	_pages       = 1;
    pageButtons = false;
	showMagFlag = true;
	scaleMin    = s1;
	scaleMax    = s2;
	minVal      = min_;
	maxVal      = max_;
    up          = nullptr;
    down        = nullptr;
	logbase     = bas;
	invers      = inv;
	scaleVal    = 0;

	scaleVal = cs;
	const int cur = scale2mag(cs);
  
	//fprintf(stderr, "ScrollScale: cs:%d cur:%f\n", cs, cur);
	scale  = new QSlider (o);
    scale->setObjectName("ScrollScaleZoomSlider");
	// Added by Tim. For some reason focus was on. 
	// It messes up tabbing, and really should have a shortcut instead.
	scale->setFocusPolicy(Qt::NoFocus);  
	scale->setMinimum(0);
	scale->setMaximum(convertQuickZoomLevelToMag(zoomLevels-1));
	scale->setPageStep(1);
	scale->setValue(cur);	

	scroll = new QScrollBar ( o );
	//scroll->setFocusPolicy(Qt::NoFocus);  // Tim.

	emit scaleChanged ( scaleVal );
	if ( !noScale )
		setRange ( minVal, maxVal );

	if ( o == Qt::Horizontal )
	{
		box = new QBoxLayout ( QBoxLayout::LeftToRight);
		scale->setMaximumWidth ( 70 );
		scroll->setMinimumWidth ( 50 );
	}
	else
	{
		box = new QBoxLayout ( QBoxLayout::TopToBottom);
		scroll->setMinimumHeight ( 50 );
		scale->setMaximumHeight ( 70 );
	}
    box->setContentsMargins(0, 0, 0, 0);
    box->setSpacing(0);
    box->addWidget ( scroll, 10 );

    int w = style()->pixelMetric(QStyle::PM_ScrollBarExtent);;
    scaleUp = new QToolButton;
    scaleUp->setObjectName("ScrollScaleZoomButton");
    scaleUp->setFocusPolicy(Qt::NoFocus);
    scaleUp->setMaximumSize(w, w);
    scaleUp->setIcon (*plusSVGIcon);
    scaleUp->setToolTip(tr("Increase zoom level"));
    connect(scaleUp, &QToolButton::clicked, this, [this](){ stepScale(true); });
    scaleDown = new QToolButton;
    scaleDown->setFocusPolicy(Qt::NoFocus);
    scaleDown->setObjectName("ScrollScaleZoomButton");
    scaleDown->setMaximumSize(w, w);
    scaleDown->setIcon (*minusSVGIcon);
    scaleDown->setToolTip(tr("Decrease zoom level"));
    connect(scaleDown, &QToolButton::clicked, this, [this](){ stepScale(false); });

    box->addSpacing(4);
    box->addWidget(scaleDown);
	box->addWidget ( scale, 5 );
    box->addWidget(scaleUp);

    setLayout(box);
	connect ( scale, SIGNAL ( valueChanged ( int ) ), SLOT ( setScale ( int ) ) );
	connect ( scroll, SIGNAL ( valueChanged ( int ) ), SIGNAL ( scrollChanged ( int ) ) );
}

//---------------------------------------------------------
//   setPageButtons
//---------------------------------------------------------

void ScrollScale::setPageButtons ( bool flag )
{
	if ( flag == pageButtons )
		return;

	if ( flag )
	{
		if ( up == nullptr )
		{
			up = new QToolButton;
			up->setIcon ( QIcon(":/svg/up_vee.svg") );
			down = new QToolButton;
			down->setIcon ( QIcon(":/svg/down_vee.svg") );
			pageNo = new QLabel;
			QString s;
			s.setNum ( _page+1 );
			pageNo->setText ( s );
			down->setToolTip(tr ( "next page" ) );
			up->setToolTip(tr ( "previous page" ) );
			pageNo->setToolTip(tr ( "current page number" ) );
			box->insertWidget ( 1, up );
			box->insertWidget ( 2, down );
			box->insertSpacing ( 3, 5 );
			box->insertWidget ( 4, pageNo );
			box->insertSpacing ( 5, 5 );
			connect ( up, SIGNAL ( clicked() ), SLOT ( pageUp() ) );
			connect ( down, SIGNAL ( clicked() ), SLOT ( pageDown() ) );
		}
		up->show();
		down->show();
		pageNo->show();
		if ( _page == ( _pages-1 ) )
			down->setEnabled ( false );
		if ( _page == 0 )
			up->setEnabled ( false );
	}
	else
	{
		up->hide();
		down->hide();
	}
	pageButtons = flag;
}

//---------------------------------------------------------
//   showMag
//---------------------------------------------------------

void ScrollScale::showMag ( bool flag )
{
	showMagFlag = flag;
	if ( flag )
		scale->show();
	else
		scale->hide();
	box->activate();
}

//---------------------------------------------------------
//   offset
//---------------------------------------------------------
int ScrollScale::offset() const
{
	return pos2offset ( scroll->value() );
}

//---------------------------------------------------------
//   pos2offset
//---------------------------------------------------------
int ScrollScale::pos2offset ( int pos ) const
{
// REMOVE Tim. wave. Changed.
	if ( scaleVal < 1 )
	//if ( scaleVal < 0 )
		return pos * ( -scaleVal ) + scaleVal/2;
	else
		return pos / scaleVal;
}

//---------------------------------------------------------
//   offset2pos
//---------------------------------------------------------

int ScrollScale::offset2pos ( int off ) const
{
// REMOVE Tim. wave. Changed.
// 	if ( scaleVal < 1 )
	if ( scaleVal < 0 )
		return ( off-scaleVal/2 ) / ( -scaleVal );
	else
		return off * scaleVal;
}

//---------------------------------------------------------
//   mag2scale
//---------------------------------------------------------

int ScrollScale::mag2scale(int mag) const
{
	int mag_max = convertQuickZoomLevelToMag(zoomLevels-1);
	if(mag < 0)
	  mag = 0;
	else if(mag > mag_max)
	  mag = mag_max;
	if ( invers )
		mag = mag_max - mag;
	double min, max;
	if ( scaleMin < 0 )
		min = 1.0/ ( -scaleMin );
	else
		min = double ( scaleMin );

	if ( scaleMax < 0 )
		max = 1.0/ ( -scaleMax );
	else
		max = double ( scaleMax );

	double diff = max-min;
	double fkt  = double ( mag ) /double(mag_max);
	double v = ( pow ( logbase, fkt )-1 ) / ( logbase-1 );
	double scale;
	if ( invers )
		scale = max - v * diff;
	else
		scale = min + v * diff;

#if 0
	if ( scaleMax > scaleMin )
	{
		if ( scale < scaleMin )
			scale = scaleMin;
		else if ( scale > scaleMax )
			scale = scaleMax;
	}
	else
	{
		if ( scale < scaleMax )
			scale = scaleMax;
		else if ( scale > scaleMin )
			scale = scaleMin;
	}
#endif

	int scale_val;
// REMOVE Tim. wave. Changed.
	if ( scale < 1.0 )
// 	if ( scale < 0.0 )
		// Floor, rather than simply casting 1.0/scale as a negative int,
		//  was required here due to the unique nature of our negative numbers,
		//  so that the reciprocal scale2mag() matches this mag2scale().
		// Tested OK so far, loading and saving, with newly opened windows as well. Tim.
		scale_val = floor ( 1.0 / ( -scale ) );
	else
		scale_val = int ( scale );
	if ( scale_val == -1 )   // nur so
		scale_val = 1;
	return scale_val;
}

//---------------------------------------------------------
//   scale2mag
//---------------------------------------------------------

int ScrollScale::scale2mag(int scale) const
{
	double min, max;
	if ( scaleMin < 0 )
		min = 1.0/ ( -scaleMin );
	else
		min = double ( scaleMin );

	if ( scaleMax < 0 )
		max = 1.0/ ( -scaleMax );
	else
		max = double ( scaleMax );

	double cmag = ( scale < 0 ) ? ( 1.0/ ( -scale ) ) : double ( scale );
	double diff = max-min;

	const int mag_max = convertQuickZoomLevelToMag(zoomLevels-1);

	// Do a log in the given logbase (see Change of Base Formula).
	// Choice of 'log()' base (10, 2 natural etc.) is not supposed to matter.
	const double fkt = log10( (cmag - min) * (logbase - 1) / diff + 1 ) / log10(logbase);
// 	const double fkt = log( (cmag - min) * (logbase - 1) / diff + 1 ) / log(logbase);
	// Round up so that the reciprocal function scale2mag() matches.
	const double cur = ceil( fkt * mag_max );

	return cur;
}

//---------------------------------------------------------
//   setOffset
//    val in tick
//---------------------------------------------------------

void ScrollScale::setOffset ( int val )
{
	int i = ( scroll->orientation() == Qt::Horizontal ) ? width() : height();
	int pos, max;

// REMOVE Tim. wave. Changed.
// 	if ( scaleVal < 1 )
	if ( scaleVal < 0 )
	{
		pos = ( val-scaleVal/2 ) / ( -scaleVal );
		max = ( maxVal-scaleVal-1 ) / ( -scaleVal ) - i;
	}
	else
	{
		pos = val * scaleVal;
		max = maxVal * scaleVal - i;
	}
	if ( pos > max )
	{
		int min;
// REMOVE Tim. wave. Changed.
// 		if ( scaleVal < 1 )
		if ( scaleVal < 0 )
		{
			maxVal  = ( pos + width() ) * ( -scaleVal );
			min = ( minVal-scaleVal/2 ) / ( -scaleVal );
			max     = ( maxVal-scaleVal/2 ) / ( -scaleVal ) - i;
		}
		else
		{
			maxVal  = ( pos + width() + scaleVal/2 ) /scaleVal;
			min = minVal * scaleVal;
			max     = maxVal * scaleVal - i;
		}

		if ( max < 0 )
			max = 0;
		if ( min < 0 )
			min = 0;
		if ( min > max )
			max = min;
		scroll->setRange ( min, max );
	}

	setPos ( pos );
}

//---------------------------------------------------------
//   pageUp
//    goto previous page
//---------------------------------------------------------

void ScrollScale::pageUp()
{
	if ( _page )
	{
		--_page;
		emit newPage ( _page );
		QString s;
		s.setNum ( _page+1 );
		pageNo->setText ( s );
		if ( _page == 0 )
			up->setEnabled ( false );
		if ( _page == ( _pages-2 ) )
			down->setEnabled ( true );
	}
}

//---------------------------------------------------------
//   pageDown
//    goto next page
//---------------------------------------------------------

void ScrollScale::pageDown()
{
	if ( _page + 1 < _pages )
	{
		++_page;
		emit newPage ( _page );
		QString s;
		s.setNum ( _page+1 );
		pageNo->setText ( s );
		if ( _page == ( _pages-1 ) )
			down->setEnabled ( false );
		if ( _page == 1 )
			up->setEnabled ( true );
	}
}

//---------------------------------------------------------
//   setPages
//---------------------------------------------------------

void ScrollScale::setPages ( int n )
{
	_pages = n;
	if ( _page >= _pages )
	{
		_page = _pages-1;
		emit newPage ( _page );
		QString s;
		s.setNum ( _page+1 );
		pageNo->setText ( s );
	}
	up->setEnabled ( _page );
	down->setEnabled ( _page < ( _pages-1 ) );
}

int ScrollScale::pos() const
{
	return scroll->value();
}

int ScrollScale::mag() const
{
	return scale->value();
}

/**
 * Hardcoded hackish function that corresponds to the values used for the scrollscales in PianoRoll and DrumEditor
 * since I couldn't easily create any inverse function from the [0,1024]-range to detect where a zoom actually occurs
 * (mg)
 */
int ScrollScale::getQuickZoomLevel(int mag)
{
	if (mag == 0)
		return 0;

	for (int i=0; i<zoomLevels-1; i++) {
		int val1 = ScrollScale::convertQuickZoomLevelToMag(i);
		int val2 = ScrollScale::convertQuickZoomLevelToMag(i + 1);
		if (mag > val1 && mag <= val2)
			return i + 1;
	}

	return -1; 
}

/**
 * Function returning the boundary values for a zoom change, hardcoded corresponding to the values used in PianoRoll 
 * and DrumEditor
 */
int ScrollScale::convertQuickZoomLevelToMag(int zoomlevel)
{
	int vals[] = {
		0,   1,   15,  30,  46,  62,  80,  99,  119, 140,
		163, 187, 214, 242, 274, 308, 346, 388, 436, 491,
		555, 631, 726, 849, 1024, 1200, 1300, 1400, 1500, 1600, 1700, 1800,    
		1900, 2100, 2200, 2300, 2400, 2500 };  

	return vals[zoomlevel];
}

int ScrollScale::scaleMinimum() const { return scaleMin; }
int ScrollScale::scaleMaximum() const { return scaleMax; }

void ScrollScale::setScaleMinimum(int min)
{
  if(scaleMin == min)
    return;
  scaleMin = min;
  
  if(scaleVal < scaleMin)
  {
    scaleVal = scaleMin;
    emit scaleChanged ( scaleVal );
    if ( !noScale )
      setRange ( minVal, maxVal );
  }
  repaint();
}

void ScrollScale::setScaleMaximum(int max)
{
  if(scaleMax == max)
    return;
  scaleMax = max;
  
  if(scaleVal > scaleMax)
  {
    scaleVal = scaleMax;
    emit scaleChanged ( scaleVal );
    if ( !noScale )
      setRange ( minVal, maxVal );
  }
  repaint();
}

void ScrollScale::setScaleRange(int min, int max)
{
  if(scaleMin == min && scaleMax == max)
    return;
  scaleMin = min;
  scaleMax = max;
  
  if(scaleVal < scaleMin || scaleVal > scaleMax)
  {
    if(scaleVal < scaleMin)
      scaleVal = scaleMin;
    else if(scaleVal > scaleMax)
      scaleVal = scaleMax;
    emit scaleChanged ( scaleVal );
    if ( !noScale )
      setRange ( minVal, maxVal );
  }
  repaint();
}

} // namespace MusEGui
