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

#ifndef __PRCANVAS_H__
#define __PRCANVAS_H__

#include "ecanvas.h"

class CItem;
class MidiCmd;

//---------------------------------------------------------
//   PianoCanvas
//---------------------------------------------------------

class PianoCanvas : public EventCanvas {
      Q_OBJECT

      int colorMode;
      int playedPitch;

      MidiCmd* cmdModifyGateTime;
      MidiCmd* cmdModifyVelocity;

      virtual void paint(QPainter&, QRect);
      virtual CItem* searchItem(const QPoint& p) const;
      virtual void addItem(Part* part, const Event& event);
      virtual void viewMouseDoubleClickEvent(QMouseEvent*);
      virtual void moveItem(CItem*, DragType);
      virtual CItem* newItem(const QPoint&, int);
      virtual void resizeItem(CItem*, bool noSnap);
      virtual void newItem(CItem*, bool noSnap);
      virtual bool deleteItem(CItem*);
      virtual void startDrag(CItem* item, bool copymode);
      virtual void dragEnterEvent(QDragEnterEvent* event);
      virtual void dragMoveEvent(QDragMoveEvent*);
      virtual void dragLeaveEvent(QDragLeaveEvent*);
      virtual void viewDropEvent(QDropEvent* event);
      virtual void selectLasso(bool toggle);
      virtual void timeTypeChanged();

      void quantize(int, int, bool);
      void paste();
      virtual void itemPressed(const CItem*);
      virtual void itemReleased();
      virtual void itemMoved(const CItem*);

   public slots:
      void pianoCmd(int);

   public:
      PianoCanvas(MidiEditor*);
      void cmd(QAction*, int, int, bool);
      void setColorMode(int mode);
      virtual void modifySelected(NoteInfo::ValType type, int delta);
      };
#endif

