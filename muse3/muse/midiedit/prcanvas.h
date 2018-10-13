//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: prcanvas.h,v 1.5.2.6 2009/11/16 11:29:33 lunar_shuttle Exp $
//  (C) Copyright 1999 Werner Schweer (ws@seh.de)
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

#ifndef __PRCANVAS_H__
#define __PRCANVAS_H__

#include "ecanvas.h"
#include "pianoroll.h"
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMouseEvent>
#include <QDragMoveEvent>
#include <QDragLeaveEvent>
#include <QTimer>

#include "steprec.h"


class QRect;

namespace MusEGui {

//---------------------------------------------------------
//   NEvent
//    ''visual'' Note Event
//---------------------------------------------------------

// REMOVE Tim. citem. Changed.
//class NEvent : public CItem {
class NEvent : public EItem {
   public:
      NEvent(const MusECore::Event& e, MusECore::Part* p, int y);
      
      // REMOVE Tim. citem. Added.
      //bool objectIsSelected() const { return event().selected(); }
      };

class ScrollScale;
class PianoRoll;

//---------------------------------------------------------
//   PianoCanvas
//---------------------------------------------------------

class PianoCanvas : public EventCanvas {
      Q_OBJECT
    
      int colorMode;
      
      bool noteHeldDown[128];
      
      MusECore::StepRec* steprec;

      
      virtual void viewMouseDoubleClickEvent(QMouseEvent*);
// REMOVE Tim. citem. Changed.
//       virtual void drawItem(QPainter&, const CItem*, const QRect&);
      virtual void drawItem(QPainter&, const CItem*, const QRect&, const QRegion& = QRegion());
// REMOVE Tim. citem. Changed.
//       void drawTopItem(QPainter& p, const QRect& rect);
      void drawTopItem(QPainter& p, const QRect& rect, const QRegion& = QRegion());
// REMOVE Tim. citem. Changed.
//       virtual void drawMoving(QPainter&, const CItem*, const QRect&);
      virtual void drawMoving(QPainter&, const CItem*, const QRect&, const QRegion& = QRegion());
      virtual MusECore::Undo moveCanvasItems(CItemMap&, int, int, DragType, bool rasterize = true);
      virtual bool moveItem(MusECore::Undo&, CItem*, const QPoint&, DragType, bool rasterize = true);
      virtual CItem* newItem(const QPoint&, int);
      virtual void resizeItem(CItem*, bool noSnap, bool);
      virtual void newItem(CItem*, bool noSnap);
      virtual bool deleteItem(CItem*);
      virtual void startDrag(CItem* item, DragType t);
      virtual void dragEnterEvent(QDragEnterEvent* event);
      virtual void dragMoveEvent(QDragMoveEvent*);
      virtual void dragLeaveEvent(QDragLeaveEvent*);
      virtual CItem* addItem(MusECore::Part*, const MusECore::Event&);

      int y2pitch(int) const;
      int pitch2y(int) const;
      inline int y2height(int) const { return KH/2; }
      inline int yItemOffset() const { return KH/4; }
// REMOVE Tim. citem. Changed.
//       virtual void drawCanvas(QPainter&, const QRect&);
      virtual void drawCanvas(QPainter&, const QRect&, const QRegion& = QRegion());
      virtual void itemPressed(const CItem*);
      virtual void itemReleased(const CItem*, const QPoint&);
      virtual void itemMoved(const CItem*, const QPoint&);
      virtual void curPartChanged();
      virtual void resizeEvent(QResizeEvent*);

   private slots:
      void midiNote(int pitch, int velo);

   signals:
      void quantChanged(int);
      void rasterChanged(int);
      void newWidth(int);

   public slots:
      void pianoCmd(int);
      void pianoPressed(int pitch, int velocity, bool shift);
      void pianoReleased(int pitch, bool);

   public:
      enum {
         CMD_CUT, CMD_COPY, CMD_COPY_RANGE, CMD_PASTE, CMD_PASTE_TO_CUR_PART, CMD_PASTE_DIALOG, CMD_DEL,
         CMD_QUANTIZE,
         CMD_SELECT_ALL, CMD_SELECT_NONE, CMD_SELECT_INVERT,
         CMD_SELECT_ILOOP, CMD_SELECT_OLOOP, CMD_SELECT_PREV_PART, CMD_SELECT_NEXT_PART, 
         CMD_MODIFY_GATE_TIME, CMD_MODIFY_VELOCITY, CMD_CRESCENDO,
         CMD_TRANSPOSE, CMD_THIN_OUT, CMD_ERASE_EVENT,
         CMD_NOTE_SHIFT, CMD_MOVE_CLOCK, CMD_COPY_MEASURE,
         CMD_ERASE_MEASURE, CMD_DELETE_MEASURE, CMD_CREATE_MEASURE,
         CMD_FIXED_LEN, CMD_DELETE_OVERLAPS, CMD_LEGATO
         };

      PianoCanvas(MidiEditor*, QWidget*, int, int);
      virtual ~PianoCanvas();
      void cmd(int cmd);
      void setColorMode(int mode) {
            colorMode = mode;
            redraw();
            }
      virtual void modifySelected(NoteInfo::ValType type, int val, bool delta_mode = true);
      };

} // namespace MusEGui

#endif

