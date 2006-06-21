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

#ifndef __ECANVAS_H__
#define __ECANVAS_H__

#include "awl/tcanvas.h"
#include "widgets/noteinfo.h"
#include "citem.h"
#include "ctrledit.h"
#include "widgets/tb1.h"

class MidiTrack;
class MidiEditor;
class MidiEvent;
class Part;
class Event;
class CtrlEdit;

//---------------------------------------------------------
//   EventCanvas
//---------------------------------------------------------

class EventCanvas : public TimeCanvas {
      Q_OBJECT

      int keyDown;
      int playedPitch;

      void noteOn(int pitch, int velocity, bool shift);
      void noteOff(int pitch);
      virtual void layout();
	virtual void magChanged();

   protected:
      enum DragMode {
            DRAG_OFF, DRAG_NEW,
            DRAG_MOVE_START, DRAG_MOVE,
            DRAG_COPY_START, DRAG_COPY,
            DRAGX_MOVE, DRAGY_MOVE,
            DRAGX_COPY, DRAGY_COPY,
            DRAG_DELETE,
            DRAG_RESIZE, DRAG_LASSO_START, DRAG_LASSO,
            };

      enum DragType {
            MOVE_MOVE, MOVE_COPY
            };

      CtrlEditList ctrlEditList;
      MidiEditor* editor;
      CItemList items;
      CItemList moving;
      CItem* curItem;
      Part* curPart;
      int canvasTools;
      int curSplitter;  // -1 mouse not in splitter
      bool dragSplitter;
      int splitterY;

      DragMode drag;
      QRect lasso;
      QPoint start;
      QMenu* itemPopupMenu;
      QMenu* canvasPopupMenu;

      unsigned startTick, endTick;
      int curVelo;

      virtual void mousePress(QMouseEvent*);
      virtual void mouseDoubleClick(QMouseEvent*);
      virtual void mouseMove(QPoint);
      virtual void enterB();
      virtual void leaveB();
      virtual void mouseRelease(QMouseEvent*);
      void layoutPanelB(CtrlEdit* c);

      void updateSelection();
      virtual CItem* searchItem(const QPoint& p) const = 0;
      virtual void addItem(Part*, const Event&) = 0;
      virtual void moveItem(CItem*, DragType) = 0;
      virtual CItem* newItem(const QPoint&, int state) = 0;
      virtual void newItem(CItem*, bool noSnap=false) = 0;
      virtual bool deleteItem(CItem*) = 0;
      virtual void resizeItem(CItem*, bool noSnap=false) = 0;

      virtual void startDrag(CItem*, bool) {}
      virtual QMenu* genItemPopup(CItem*) { return 0; }
      virtual void itemPopup(CItem*, int, const QPoint&) {}
      virtual void itemReleased() {}
      virtual void itemPressed(const CItem*) {}
      virtual void itemMoved(const CItem*) {}

      virtual void addController();

      void endMoveItems(DragType);
      virtual void selectItem(CItem* e, bool);
      virtual void deselectAll();
      QMenu* genCanvasPopup();
      void canvasPopup(int);
      virtual void startUndo(DragType);
      virtual void endUndo(DragType);
      virtual void selectLasso(bool) {}
      void setCursor();
      virtual void deleteItem(const QPoint&);
      void moveItems(const QPoint& pos, int dir);
      void mousePressCanvasA(QPoint pos);
      void mousePressCanvasA(QMouseEvent*);
      void mouseMoveCanvasA(QPoint pos);
      void mouseReleaseCanvasA(QMouseEvent*);

      virtual void paintControllerCanvas(QPainter&, QRect);
	virtual void paintControllerPanel(QPainter&, QRect);

      void pasteAt(const QString& pt, unsigned pos);
      void updatePartControllerList();

   private slots:
      void removeController(QAction*);
      void midiNote(const MidiEvent&);

   public slots:
      void setQuant(int)      { update(); }
      void songChanged(int);

   signals:
      void selectionChanged(int, Event&, Part*);
      void enterCanvas();
      void yChanged(int);  // emitted from mouseMove in controller canvas

   public:
      EventCanvas(MidiEditor*, TimeCanvasType);
      MidiTrack* track() const;
      Part* part() const       	{ return curPart; }
      QString getCaption() const;
      void range(int* s, int* e) const;
      void selectFirst();
      virtual void modifySelected(NoteInfo::ValType, int) {}
      virtual void keyPress(QKeyEvent*);
      int selectionSize() const;
      void setCurPart(Part*);
      QMimeData* getTextDrag();
      CItemList* getItems() { return &items; }
      CtrlEditList* getCtrlEditors() { return &ctrlEditList; }
      const CtrlEditList* getCtrlEditors() const { return &ctrlEditList; }
      void addController(int id, int h);
      void layout1();
      };

extern void paintVLine(QPainter& p, int y1, int y2, int x);

#endif

