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

#ifndef __CONFIG_TRACK_H__
#define __CONFIG_TRACK_H__

#include "ui_configtrack.h"
#include "track.h"
#include "trelement.h"

struct TrElement;
class TrGroupList;

//---------------------------------------------------------
//   ConfigTrackList
//---------------------------------------------------------

class ConfigTrackList : public QDialog, Ui::ConfigTrackListBase {
      Q_OBJECT

      TrGroupList list[Track::TRACK_TYPES];
      void init();
      bool dirty;
      void saveTrackType();
      int curType;

   private slots:
      void trackTypeChanged(int);
      void availableSelected();
      void configuredSelected();
      void upClicked();
      void downClicked();
      void addItemClicked();
      void removeItemClicked();
      virtual void done(int);

   signals:
      void trackConfigChanged();

   public:
      ConfigTrackList(QWidget*);
      };

#endif
