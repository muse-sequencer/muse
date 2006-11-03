//=============================================================================
//  MusE
//  Linux Music Editor
//  $Id:$
//
//  Copyright (C) 2002-2006 by Werner Schweer and others
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License version 2.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//=============================================================================

#ifndef __GUI_H__
#define __GUI_H__

//
// GUI constants
//	central point of tweaking the gui
//

// size of horizontal and vertical splitter
//
static const int splitWidth = 6;

// arranger:
static const int trackRowHeight = 24;
static const int minTrackHeight = trackRowHeight + splitWidth + 1;
static const int defaultTrackHeight = minTrackHeight;
static const int infoHeight     = 20;
static const int infoWidth      = 140;
static const int trackSeparator = 1;
static const int yTrackOffset   = -4;

//  mixer:
static const int STRIP_WIDTH  = 66+4;
static const int LABEL_HEIGHT = 20;

static const int ICON_WIDTH = 18;
static const QSize ICON_SIZE(ICON_WIDTH, ICON_WIDTH);

#endif



