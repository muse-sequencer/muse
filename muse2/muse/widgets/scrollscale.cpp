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
#include <cmath>

#include <QBoxLayout>
#include <QLabel>
#include <QResizeEvent>
#include <QScrollBar>
#include <QSlider>
#include <QToolButton>
#include <QToolTip>

// #include "globals.h"
#include "scrollscale.h"
#include "icons.h"

namespace MusEGui {

//---------------------------------------------------------
//   setScale
//    "val" - slider value in range 0-1024
//---------------------------------------------------------

void ScrollScale::setScale ( int val )
{
	int off = offset();
	if ( invers )
		val = 1024 - val;
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
	double fkt  = double ( val ) /1024.0;
	double v = ( pow ( logbase, fkt )-1 ) / ( logbase-1 );
	double scale;
	if ( invers )
		scale = max - v * diff;
	else
		scale = min + v * diff;

	if ( scale < 1.0 )
		scaleVal = - ( int ( 1.0 / scale ) );
	else
		scaleVal = int ( scale );
	if ( scaleVal == -1 )   // nur so
		scaleVal = 1;

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

	emit scaleChanged ( scaleVal );
	if ( !noScale )
		setRange ( minVal, maxVal );

  int i = ( scroll->orientation() == Qt::Horizontal ) ? width() : height();
  int pos, pmax;
  if ( scaleVal < 1 )
  {
    pos = ( off-scaleVal/2 ) / ( -scaleVal );
    pmax = ( maxVal-scaleVal-1 ) / ( -scaleVal ) - i;
  }
  else
  {
    pos = off * scaleVal;
    pmax = maxVal * scaleVal - i;
  }
  if(pos > pmax)
    pos = pmax;
  setPos(pos);
}

//---------------------------------------------------------
//   setMag
//---------------------------------------------------------

void ScrollScale::setMag ( int cs )
{
	scale->setValue ( cs );
	setScale ( cs );
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
		if ( scaleVal < 1 )
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
  setScale ( scale->value() );
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
	up          = 0;
	down        = 0;
	logbase     = bas;
	invers      = inv;
        scaleVal    = 0;

	double min, max;
	if ( scaleMin < 0 )
		min = 1.0/ ( -scaleMin );
	else
		min = double ( scaleMin );

	if ( scaleMax < 0 )
		max = 1.0/ ( -scaleMax );
	else
		max = double ( scaleMax );

	double cmag = ( cs < 0 ) ? ( 1.0/ ( -cs ) ) : double ( cs );
	double diff = max-min;

	//
	// search initial value for slider
	//
	int cur   = 512;
	int delta = 256;
	for ( int i = 0; i < 8; ++i )
	{
		int tryVal   = invers ? 1025 - cur : cur;
		double fkt   = double ( tryVal ) /1024.0;
		double v     = ( pow ( logbase, fkt )-1 ) / ( logbase-1 );
		double scale = invers ? ( max - v * diff ) : ( min + v * diff );
		if ( scale == cmag ) // not very likely
			break;
 //printf("iteration %d invers:%d soll %f(cur:%d) - ist %f\n", i, invers, scale, cur, cmag);
		int dd = invers ? -delta : delta;
		cur += ( scale < cmag ) ? dd : -dd;
		delta/=2;
	}

	scale  = new QSlider (o);
        // Added by Tim. For some reason focus was on. 
        // It messes up tabbing, and really should have a shortcut instead.
        scale->setFocusPolicy(Qt::NoFocus);  
        scale->setMinimum(0);
        scale->setMaximum(1024);
	scale->setPageStep(1);
	scale->setValue(cur);	

	scroll = new QScrollBar ( o );
        //scroll->setFocusPolicy(Qt::NoFocus);  // Tim.
	setScale ( cur );

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
	box->addWidget ( scale, 5 );
	setLayout(box);
	connect ( scale, SIGNAL ( valueChanged ( int ) ), SLOT ( setScale ( int ) ) );
	///connect ( scale, SIGNAL ( valueChanged ( int ) ), SIGNAL ( lscaleChanged ( int ) ) );  // ??
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
		if ( up == 0 )
		{
			up = new QToolButton;
			up->setIcon ( QIcon(*upIcon) );
			down = new QToolButton;
			down->setIcon ( QIcon(*downIcon) );
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
int ScrollScale::offset()
{
	return pos2offset ( scroll->value() );
}

//---------------------------------------------------------
//   pos2offset
//---------------------------------------------------------
int ScrollScale::pos2offset ( int pos )
{
	if ( scaleVal < 1 )
		return pos * ( -scaleVal ) + scaleVal/2;
	else
		return pos / scaleVal;
}

//---------------------------------------------------------
//   setOffset
//    val in tick
//---------------------------------------------------------

void ScrollScale::setOffset ( int val )
{
	int i = ( scroll->orientation() == Qt::Horizontal ) ? width() : height();
	int pos, max;

	if ( scaleVal < 1 )
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
		if ( scaleVal < 1 )
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

      for (int i=0; i<24; i++) {
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
      int vals[] = { 0, 1, 15, 30, 46, 62, 80, 99, 119, 140, 163, 
            187, 214, 242, 274, 308, 346, 388, 436, 491, 555, 631, 
            726, 849, 1024 };

      return vals[zoomlevel];
}

} // namespace MusEGui
