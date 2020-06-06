//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: rack.h,v 1.5.2.3 2006/09/24 19:32:31 terminator356 Exp $
//
//  (C) Copyright 2000 Werner Schweer (ws@seh.de)
//  (C) Copyright 2016 Tim E. Real (terminator356 on sourceforge)
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

#ifndef __RACK_H__
#define __RACK_H__

#include <QListWidget>
#include "type_defs.h"

#include <QDragEnterEvent>
#include <QDragLeaveEvent>
#include <QDropEvent>
#include <QMouseEvent>
#include <QEvent>
#include <QColor>

#include "track.h"
#include "xml.h"
#include "background_painter.h"

namespace MusEGui {

//---------------------------------------------------------
//   EffectRack
//---------------------------------------------------------

class EffectRack : public QListWidget {
      Q_OBJECT
    
      Q_PROPERTY( bool style3d READ style3d WRITE setStyle3d )
      Q_PROPERTY( int radius READ radius WRITE setRadius )
      Q_PROPERTY( bool customScrollbar READ customScrollbar WRITE setCustomScrollbar )

      bool _style3d;
      int _radius;
      bool _customScrollbar;
    
      MusECore::AudioTrack* track;
      int itemheight;
      QColor _activeColor;
      ItemBackgroundPainter* _bkgPainter;

      virtual QSize minimumSizeHint() const;
      virtual QSize sizeHint() const;
      
      void startDragItem(int idx);
      void initPlugin(MusECore::Xml xml, int idx);
      QPoint dragPos;
      void savePreset(int idx);
      void choosePlugin(QListWidgetItem* item, bool replace = false);

   private slots:
      void menuRequested(QListWidgetItem*);
      void doubleClicked(QListWidgetItem*);
      void songChanged(MusECore::SongChangedStruct_t);
      void updateContents();

   protected:
      void dropEvent(QDropEvent *event);
      void dragEnterEvent(QDragEnterEvent *event);
      void mousePressEvent(QMouseEvent *event);
      void mouseMoveEvent(QMouseEvent *event);
      void enterEvent(QEvent *event);
      void leaveEvent(QEvent *event);

      QStringList mimeTypes() const;
      Qt::DropActions supportedDropActions () const;
   
   public:
      EffectRack(QWidget*, MusECore::AudioTrack* t);

      MusECore::AudioTrack* getTrack() const { return track; } 
      QPoint getDragPos() const { return dragPos; }
      ItemBackgroundPainter* getBkgPainter() const { return _bkgPainter; }

      bool style3d() const { return _style3d; }
      void setStyle3d(const bool style3d) { _style3d = style3d; }
      int radius() const { return _radius; }
      void setRadius(const int radius) { _radius = radius; }
      bool customScrollbar() const { return _customScrollbar; }
      void setCustomScrollbar(const bool customScrollbar) { _customScrollbar = customScrollbar; }
      };

} // namespace MusEGui

#endif

