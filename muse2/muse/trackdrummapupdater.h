//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: trackdrummapupdater.h,v 1.59.2.52 2011/12/27 20:25:58 flo93 Exp $
//
//  (C) Copyright 2011 Florian Jung (florian.a.jung (at) web.de)
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

#ifndef __TRACKDRUMMAPUPDATER_H__
#define __TRACKDRUMMAPUPDATER_H__

#include <QObject>
#include "type_defs.h"

namespace MusECore {

class TrackDrummapUpdater : public QObject
{
  Q_OBJECT

  public:
    TrackDrummapUpdater(QObject* parent = 0);
    
  private slots:
    void songChanged(MusECore::SongChangedFlags_t flags);
};

} //namespace MusECore
#endif
