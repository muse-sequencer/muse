//=========================================================
//  MusE
//  Linux Music Editor
//  songpos_toolbar.cpp
//  (C) Copyright 2012 Florian Jung (flo93@users.sourceforge.net)
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


#include "songpos_toolbar.h"
#include "song.h"
#include <QPainter>
#include <QPaintEvent>

namespace MusEGui
{
	SongPosToolbarWidget::SongPosToolbarWidget(QWidget* p)
	      : MTScale(&_raster, p, -100 /* some random scale, will be overwritten immediately */)
	{
		_raster=0;
		setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
		connect(MusEGlobal::song, SIGNAL(songChanged(int)), this, SLOT(song_changed(int)));
		song_changed(0);
	}
	
	void SongPosToolbarWidget::song_changed(int)
	{
		if (width()!=0)
			setXMag(-(MusEGlobal::song->len()/width()));
	}
	
	QSize	SongPosToolbarWidget::sizeHint() const
	{
		return QSize(100, minimumSize().height());
	}
	
	void SongPosToolbarWidget::resizeEvent(QResizeEvent* ev)
	{
		song_changed(0);
		MTScale::resizeEvent(ev);
	}
  void SongPosToolbarWidget::paintEvent(QPaintEvent* ev)
  {
    View::paintEvent(ev);
    QPainter p;
    p.begin(this);
    p.setPen(Qt::darkGray);
    p.drawRect(0, 0, width()-1, height()-1);
    p.setPen(Qt::lightGray);
    p.drawRect(1, 1, width()-1, height()-1);
    p.end();
  }


}
