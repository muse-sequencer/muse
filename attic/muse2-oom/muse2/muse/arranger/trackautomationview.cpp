#include "trackautomationview.h"
#include "math.h"

#include <QPainter>
#include <QPaintEvent>

#include "track.h"

TrackAutomationView::TrackAutomationView(QWidget *parent, Track *t) : QWidget(parent)
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
