#ifndef TRACKAUTOMATIONVIEW_H
#define TRACKAUTOMATIONVIEW_H

class QPaintEvent;
class QWidget;

#include "track.h"

class TrackAutomationView : public QWidget
{
    Track *_t;
    void paintEvent(QPaintEvent *e);
    std::map<int,int> automationList;
public:
    TrackAutomationView(QWidget *parent, Track *t);
    Track *track() { return _t; }
    void collectAutomationData();
};

#endif // TRACKAUTOMATIONVIEW_H
