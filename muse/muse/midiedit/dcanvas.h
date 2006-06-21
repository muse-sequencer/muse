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

#ifndef __DCANVAS_H__
#define __DCANVAS_H__

#include "ecanvas.h"

class MidiEditor;
class DrumMap;

//---------------------------------------------------------
//   DrumCanvas
//---------------------------------------------------------

class DrumCanvas : public EventCanvas {
      Q_OBJECT

      int singlePitch;
      virtual void mousePress(QMouseEvent*);
      virtual void paint(QPainter&, QRect);
      virtual void addItem(Part* part, const Event& event);
      virtual void moveItem(CItem*, DragType);
      virtual CItem* newItem(const QPoint&, int);
      virtual void resizeItem(CItem*, bool);
      virtual void newItem(CItem*, bool);
      virtual bool deleteItem(CItem*);
      virtual CItem* searchItem(const QPoint& p) const;

      void copy();
      void paste();
      void startDrag(CItem*, bool copymode);
      void dragEnterEvent(QDragEnterEvent* event);
      void dragMoveEvent(QDragMoveEvent*);
      void dragLeaveEvent(QDragLeaveEvent*);
      void viewDropEvent(QDropEvent* event);
      virtual void resizeEvent(QResizeEvent*);
      virtual void paintDrumList(QPainter&, QRect);
      virtual void selectLasso(bool toggle);

	DrumMap* drumMap() const;

   protected:
      virtual int y2pitch(int y) const;
      virtual int pitch2y(int pitch) const;

   signals:
      void newWidth(int);

   public slots:
      void keyPressed(int, bool);
      void keyReleased(int, bool);

   public:
      enum {
         CMD_CUT, CMD_COPY, CMD_PASTE,
         CMD_SELECT_ALL, CMD_SELECT_NONE, CMD_SELECT_INVERT,
         CMD_SELECT_ILOOP, CMD_SELECT_OLOOP, CMD_DEL,
         CMD_FIXED_LEN, CMD_RIGHT, CMD_LEFT, CMD_MODIFY_VELOCITY,
         CMD_COMMANDS
         };
      DrumCanvas(MidiEditor*);
      void cmd(int);
      virtual void modifySelected(NoteInfo::ValType type, int delta);
      };
#endif

