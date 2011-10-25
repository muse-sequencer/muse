//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: ./muse/arranger/trackautomationview.h $
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
#ifndef TRACKAUTOMATIONVIEW_H
#define TRACKAUTOMATIONVIEW_H

class QPaintEvent;
class QWidget;

#include "track.h"

namespace MusEGui { 

class TrackAutomationView : public QWidget
{
    MusECore::Track *_t;
    virtual void paintEvent(QPaintEvent *e);
    std::map<int,int> automationList;
public:
    TrackAutomationView(QWidget *parent, MusECore::Track *t);
    MusECore::Track *track() { return _t; }
    void collectAutomationData();
};

} // namespace MusEGui

#endif // TRACKAUTOMATIONVIEW_H
