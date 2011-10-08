//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: ./muse/arranger/trackautomationview.cpp $
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
#include "trackautomationview.h"
#include "math.h"

#include <QPainter>
#include <QPaintEvent>

#include "track.h"

namespace MusEGui {

TrackAutomationView::TrackAutomationView(QWidget *parent, MusECore::Track *t) : QWidget(parent)
{
  printf("created trackautomationview\n");
  _t = t;
  //show();
}

void TrackAutomationView::paintEvent(QPaintEvent* e)
{
    QPainter p(this);
    const QRect &r = e->rect();

    // temporary solution, audio track drawing moved here.
    // best would be to get transparency to work correctly
    p.setPen(QPen(Qt::black, 2, Qt::SolidLine));
    p.setBrush(Qt::gray);
    p.drawRect(r);

     int height=r.bottom()-r.top();
     if( _t->type()>1) { // audio type
       double volume = ((AudioTrack*)_t)->volume();
       double dbvolume = (20.0*log10(volume)+60) /70.0; // represent volume between 0 and 1
       if (dbvolume < 0) dbvolume =0.0;
       printf("height=%d volume=%f dbvolume=%f\n", height, volume, dbvolume);
       p.setPen(QPen(Qt::yellow,1,Qt::SolidLine));
       p.drawLine(r.left(),r.bottom()-dbvolume*height,r.right(),r.bottom()-dbvolume*height);

   }



    printf("paintEvent\n");
}

void TrackAutomationView::collectAutomationData()
{
  // here we should collect all automation data that is currently selected for viewing and
  // prepare an event list that is easy to draw in paintEvent
  // the main reason being that the event list in it's entirety likely contains too much data to
  // be processed in the paintEvent. Better to preprocess.

//  CtrlListList cll =((AudioTrack*)_t)->controller();
//  cll.count()
}

} // namespace MusEGui
