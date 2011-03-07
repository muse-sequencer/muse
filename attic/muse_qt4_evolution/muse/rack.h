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

#ifndef __RACK_H__
#define __RACK_H__

#include <al/xml.h>

class AudioTrack;
class Plugin;

//---------------------------------------------------------
//   EffectRack
//---------------------------------------------------------

class EffectRack : public QListWidget {
      Q_OBJECT

      bool prefader;
      AudioTrack* track;
      
      QPoint dragPos;

      virtual void contextMenuEvent(QContextMenuEvent*);
      virtual void mouseDoubleClickEvent(QMouseEvent*);
      void selectNew();
      
      void startDrag(int idx);
      void initPlugin(QDomNode &node, int idx);
      void addPlugin(Plugin* plugin);

   private slots:
      void doubleClicked(QListWidgetItem*);
      void songChanged(int);

   protected:
      void dropEvent(QDropEvent *event);
      void dragEnterEvent(QDragEnterEvent *event);
      void mousePressEvent(QMouseEvent *event);
      void mouseMoveEvent(QMouseEvent *event);
      void dragMoveEvent(QDragMoveEvent *event);

   public:
      EffectRack(QWidget*, AudioTrack*, bool);
      QSize sizeHint() const;
      };

#endif

