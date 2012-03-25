//=========================================================
//  MusE
//  Linux Music Editor
//  songpos_toolbar.h
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

#ifndef __SONGPOS_TOOLBAR_H__
#define __SONGPOS_TOOLBAR_H__

#include "mtscale.h"

namespace MusEGui
{
	class SongPosToolbarWidget : public MTScale
	{
		Q_OBJECT
		
		private:
			int _raster;
    protected:
      virtual void paintEvent(QPaintEvent* ev);

		public:
			SongPosToolbarWidget(QWidget* parent);
			
			virtual QSize	sizeHint() const;
			virtual void resizeEvent(QResizeEvent*);
		
		private slots:
			void song_changed(int);
		
	};
}

#endif
