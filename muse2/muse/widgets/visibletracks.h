//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: tools.h,v 1.1.1.1 2003/10/27 18:54:49 wschweer Exp $
//  (C) Copyright 1999 Werner Schweer (ws@seh.de)
//  (C) Copyright 2011 Robert Jonsson (rj@spamatica.se)
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
#ifndef VISIBLETRACKS_H
#define VISIBLETRACKS_H

#include <QToolBar>

class QAction;
class QPixmap;
class QWidget;

namespace MusEGui {

class Action;

struct VisibleToolB {
      QPixmap** icon;
      const char* tip;
      const char* ltip;
      };

extern VisibleToolB visTrackList[];

//---------------------------------------------------------
//   EditToolBar
//---------------------------------------------------------

class VisibleTracks : public QToolBar {
      Q_OBJECT
      Action** actions;
      int nactions;

   private slots:
      void visibilityChanged(QAction* action);
   public slots:
      void updateVisibleTracksButtons();

   signals:
      void visibilityChanged();

   public:
      VisibleTracks(QWidget* /*parent*/, const char* name = 0);  // Needs a parent !
      ~VisibleTracks();
      };

} // namespace MusEGui

#endif // VISIBLETRACKS_H
