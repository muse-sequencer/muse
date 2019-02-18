//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: ctrlcanvas.h,v 1.7.2.4 2009/06/01 20:15:53 spamatica Exp $
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

#ifndef __CTRLCANVAS_H__
#define __CTRLCANVAS_H__

#include <set>

#include "type_defs.h"
#include "view.h"
#include "tools.h"
#include "midictrl.h"
#include "event.h"
#include "citem.h"
#include "undo.h"
#include "event_tag_list.h"

class QWheelEvent;
class QMouseEvent;
class QEvent;
class QWidget;

namespace MusECore {
class Event;
class MidiPart;
class MidiTrack;
class PartList;
}

namespace MusEGui {

class CtrlPanel;
class MidiEditor;
class PopupMenu;

//---------------------------------------------------------
//   CEvent
//    ''visual'' Controller Event
//---------------------------------------------------------

class CEvent : public CItem {
   private:
      MusECore::Event _event;
      int _val;
      MusECore::Part* _part;
      int ex;

   public:
      CEvent(const MusECore::Event&, MusECore::Part*, int v);
      CEvent();
      bool isObjectInRange(const MusECore::Pos&, const MusECore::Pos&) const;
      bool objectIsSelected() const { return _event.selected(); }
      
      int val() const              { return _val;   }
      void setVal(int v)           { _val = v; }
      void setEX(int v)            { ex = v; }
      bool containsPoint(const MusECore::MidiController* mc, const QPoint& p, const int tickstep, const int wh) const;
      bool containsXRange(int x1, int x2) const;
      bool intersectsController(const MusECore::MidiController*, const QRect&, const int tickstep, const int windowHeight) const;
      int EX()                      { return ex; }

      MusECore::Event event() const         { return _event;  }
      // HACK This returns a clone of the event with the length set to the visual length.
      //      It should only be used for temporary things like copy/paste and the length
      //       value should be reset to zero after usage.
      //      Normally an event's length is ALWAYS zero for all controller events.
      MusECore::Event eventWithLength() const;
      void setEvent(const MusECore::Event& e)     { _event = e;     }
      MusECore::Part* part() const  { return _part;  }
      void setPart(MusECore::Part* p)       { _part = p; }
      };

//---------------------------------------------------------
//   CtrlCanvas
//---------------------------------------------------------

class CtrlCanvas : public MusEGui::View {
      Q_OBJECT

    public:

      //---------------------------------------------------------
      //   CtrlCanvasInfoStruct
      //    Structure for returning info from CtrlCanvas::getCtrlInfo()
      //---------------------------------------------------------

      struct CtrlCanvasInfoStruct
      {
        int fin_ctrl_num;
        bool is_drum_ctl;
        bool is_newdrum_ctl;
        int min;
        int max;
        int bias;
        
        CtrlCanvasInfoStruct() : fin_ctrl_num(0), is_drum_ctl(false), is_newdrum_ctl(false), min(0), max(127), bias(0) {}
      };

    private:
      MidiEditor* editor;
      MusECore::MidiTrack* curTrack;
      MusECore::MidiPart* curPart;
      MusECore::MidiCtrlValList* ctrl;
      MusECore::MidiController* _controller;
      CtrlPanel* _panel;
      int _cnum;
      int _dnum; // Current real drum controller number (anote).
      int _didx; // Current real drum controller index.
      // For current part.
      CtrlCanvasInfoStruct _ctrlInfo;

      int line1x;
      int line1y;
      int line2x;
      int line2y;
      bool drawLineMode;
      bool noEvents;
      bool filterTrack;
      // Whether we have grabbed the mouse.
      bool _mouseGrabbed;
      // The number of times we have called QApplication::setOverrideCursor().
      // This should always be one or zero, anything else is an error, but unforeseen 
      //  events might cause us to miss a decrement with QApplication::restoreOverrideCursor().
      int _cursorOverrideCount;

      QPoint _curDragOffset;
      unsigned int _dragFirstXPos;
      //Qt::CursorShape _cursorShape;

      void applyYOffset(MusECore::Event& e, int yoffset) const;

      void viewMousePressEvent(QMouseEvent* event);
      void viewMouseMoveEvent(QMouseEvent*);
      void viewMouseReleaseEvent(QMouseEvent*);
      virtual void wheelEvent(QWheelEvent*);

      virtual void draw(QPainter&, const QRect& rect, const QRegion& = QRegion());
      virtual void pdraw(QPainter&, const QRect&, const QRegion& = QRegion());
      virtual void drawOverlay(QPainter&, const QRect&, const QRegion& = QRegion());
      virtual QRect overlayRect() const;

      void changeValRamp(int x1, int x2, int y1, int y2);
      void newValRamp(int x1, int y1, int x2, int y2);
      void changeVal(int x1, int x2, int y);
      void newVal(int x1, int y);
      void newVal(int x1, int y1, int x2, int y2);
      void deleteVal(int x1, int x2, int y);

      bool setCurTrackAndPart();
      void drawMoving(QPainter& p, const QRect& rect, const QRegion& region, const MusECore::MidiPart* part);
      void pdrawItems(QPainter& p, const QRect& rect, const MusECore::MidiPart* part, bool velo, bool fg);
      void pFillBackgrounds(QPainter& p, const QRect& rect, const MusECore::MidiPart* part);
      void pdrawExtraDrumCtrlItems(QPainter& p, const QRect& rect, const MusECore::MidiPart* part, int drum_ctl);
      void partControllers(
        const MusECore::MidiPart* part, int num,
        int* dnum, int* didx,
        MusECore::MidiController** mc, MusECore::MidiCtrlValList** mcvl,
        CtrlCanvasInfoStruct* ctrlInfo);
      // Checks if the current drum pitch requires setting the midi controller and rebuilding the items.
      // Returns whether setMidiController() and updateItems() were in fact called.
      bool drumPitchChanged();
      CEvent* findCurrentItem(const QPoint& p, const int tickstep, const int h);
      // If show is true, calls QApplication::restoreOverrideCursor() until _cursorOverrideCount-- is <= 0.
      // If show is false, calls QApplication::setOverrideCursor with a blank cursor.
      void showCursor(bool show = true);
      // Sets or resets the _mouseGrabbed flag and grabs or releases the mouse.
      void setMouseGrab(bool grabbed = false);

   protected:
      enum DragMode { DRAG_OFF, DRAG_NEW, DRAG_MOVE_START, DRAG_MOVE,
            DRAG_DELETE, DRAG_COPY_START, DRAG_COPY,
            DRAGX_MOVE, DRAGY_MOVE,
            DRAGX_COPY, DRAGY_COPY,
            DRAG_RESIZE, DRAG_LASSO_START, DRAG_LASSO,
            DRAG_PAN, DRAG_ZOOM
            };

      enum DragType {
            MOVE_MOVE, MOVE_COPY
            };

// REMOVE Tim. clip. Changed.
//       static const int contextIdCancelDrag;
//       static const int contextIdMerge;
//       static const int contextIdMergeCopy;
//       static const int contextIdErase;
//       static const int contextIdEraseWysiwyg;
//       static const int contextIdEraseInclusive;

      enum ContextIds {
        ContextIdCancelDrag = 0x01,
        ContextIdMerge = 0x02,
        ContextIdMergeCopy = 0x04,
        ContextIdErase = 0x08,
        ContextIdEraseWysiwyg = 0x10,
        ContextIdEraseInclusive = 0x20
      };
      
      CItemList items;
      // To avoid working directly with a potentially huge number of items
      //  in the item list, these 'indexing' lists are used instead.
      CItemList selection;
      CItemList moving;
      
      CEvent* curItem;
      CEvent* _movingItemUnderCursor;

      DragMode drag;
      DragType _dragType;
      QRect lasso;
      QPoint start;
      QPoint _mouseDist;
      MusEGui::Tool tool;
      unsigned pos[3];
      int curDrumPitch;    //Used by the drum-editor to view velocity of only one key (one drum)
      bool _perNoteVeloMode;
      
      // Accumulated operations during drawing etc.
      MusECore::Undo _operations;

      void setCursor();
      void keyPressEvent(QKeyEvent *event);
      void keyReleaseEvent(QKeyEvent *event);
      void enterEvent(QEvent*e);
      void leaveEvent(QEvent*e);
      QPoint raster(const QPoint&) const;

      // selection
      bool isSingleSelection()  { return selection.size() == 1; }
      void deselectAll();
      void selectItem(CEvent* e);
      void deselectItem(CEvent* e);
      void removeSelection(CEvent* e);

      void setMidiController(int);
      void updateItems();
      // Inform the app if local items have changed and their corresponding
      //  objects need to be updated synchronously in the audio thread.
      // Returns true if anything changed (or will change).
      // Uses an internal undo operations list or optionally with a supplied list
      //  so operations can be chained.
      bool itemSelectionsChanged(MusECore::Undo* operations = 0, bool deselectAll = false);
      void updateItemSelections();

      // moving
      void startMoving(const QPoint&, int dir, bool rasterize = true);
      void moveItems(const QPoint&, int dir = 0, bool rasterize = true);
      void endMoveItems();
      MusECore::Undo moveCanvasItems(CItemList&, int, int, DragType, bool rasterize = true);
      bool moveItem(MusECore::Undo&, CItem*, const QPoint&, DragType, bool rasterize = true);
      // Resets the moving flag of all items in moving list, then clears the list.
      // Returns true if anything was changed.
      bool clearMoving();
      // Resets all mouse operations if detecting missed mouseRelease event (which DOES happen).
      // Returns true if reset was actually done.
      bool cancelMouseOps();

      // Populates a popup menu with items related to drag/drop merging.
      void populateMergeOptions(PopupMenu* menu);
      
      // Merges any dragged items. Merges copies of items if 'copy' is true. Otherwise moves the items.
      // Returns true if items were merged, and it was successful. False if no items were moving, or error.
      bool mergeDraggedItems(bool copy);
     
   private slots:
      void songChanged(MusECore::SongChangedStruct_t type);
      void configChanged();    
      // Returns whether setMidiController() and updateItems() were in fact called, via drumPitchChanged().
      bool setCurDrumPitch(int);

   public slots:
      void setTool(int t);
      void setPos(int, unsigned, bool adjustScrollbar);
      void setController(int ctrl);
      void curPartHasChanged(MusECore::Part*);

   signals:
      void followEvent(int);
      void xposChanged(unsigned);
      void yposChanged(int);
      void redirectWheelEvent(QWheelEvent*);

   public:
      CtrlCanvas(MidiEditor*, QWidget* parent, int,
         const char* name = 0, CtrlPanel* pnl = 0);
      ~CtrlCanvas();   
      void setPanel(CtrlPanel* pnl);
      MusECore::MidiCtrlValList* ctrlValList() { return ctrl; }
      MusECore::MidiController* controller() { return _controller; }
      MusECore::MidiTrack* track() const { return curTrack; }
      int getCurDrumPitch() const { return curDrumPitch; }
      bool perNoteVeloMode() const { return _perNoteVeloMode; }
      void setPerNoteVeloMode(bool);
      bool itemsAreSelected() const { return !selection.empty(); }
      // Appends given tag list with item objects according to options. Avoids duplicate events or clone events.
      // Special: We 'abuse' a controller event's length, normally 0, to indicate visual item length.
      void tagItems(MusECore::TagEventList* tag_list, const MusECore::EventTagOptionsStruct& options) const;
      };

} // namespace MusEGui

#endif

