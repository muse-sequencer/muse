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
// REMOVE Tim. citem. Added.
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

// REMOVE Tim. citem. Changed.
// class CEvent {
class CEvent : public CItem {
   private:
// REMOVE Tim. citem. Removed.
      MusECore::Event _event;
      int       _val;
// REMOVE Tim. citem. Removed.
//       MusECore::MidiPart* _part;
      MusECore::Part* _part;
      int ex;
      //bool _isSelected;

   public:
//       CEvent(MusECore::Event e, MusECore::MidiPart* part, int v);
      CEvent(const MusECore::Event&, MusECore::Part*, int v);
      CEvent();
// REMOVE Tim. citem. Removed.
//       MusECore::Event event() const          { return _event; }
//       void setEvent(MusECore::Event& ev)     { _event = ev; }
      
// REMOVE Tim. citem. Changed.
//       bool isSelected() const { return _event.selected(); }
//       void setSelected(bool v);
      //bool isSelected() const { return _isSelected; }
      //void setSelected(bool f) { _isSelected = f; }
//       bool isObjectTagged() const { return _event.tagged(); }
//       void setObjectTagged(bool v);
      bool isObjectInRange(const MusECore::Pos&, const MusECore::Pos&) const;
      bool objectIsSelected() const { return _event.selected(); }
      
      int val() const              { return _val;   }
      void setVal(int v)           { _val = v; }
      void setEX(int v)            { ex = v; }
// REMOVE Tim. citem. Removed.
//       MusECore::MidiPart* part() const       { return _part;  }
      bool containsPoint(const MusECore::MidiController* mc, const QPoint& p, const int tickstep, const int wh) const;
      bool containsXRange(int x1, int x2) const;
      bool intersectsController(const MusECore::MidiController*, const QRect&, const int tickstep, const int windowHeight) const;
      int EX()                      { return ex; }

// REMOVE Tim. citem. Added.
      MusECore::Event event() const         { return _event;  }
      // HACK This returns a clone of the event with the length set to the visual length.
      //      It should only be used for temporary things like copy/paste and the length
      //       value should be reset to zero after it has been used.
      MusECore::Event eventWithLength() const;
      void setEvent(const MusECore::Event& e)     { _event = e;     }
      MusECore::Part* part() const  { return _part;  }
      void setPart(MusECore::Part* p)       { _part = p; }
      };

// REMOVE Tim. citem. Removed.
// typedef std::list<CEvent*>::iterator iCEvent;
// typedef std::list<CEvent*>::const_iterator ciCEvent;

//---------------------------------------------------------
//   CEventList
//    Controller Item List
//---------------------------------------------------------

// REMOVE Tim. citem. Changed.
// class CEventList: public std::list<CEvent*> {
//    public:
//       void add(CEvent* item) { push_back(item); }
//       
//       void clearDelete();
//       };
      
// class CEventList: public CItemList {
//    public:
//       void clearDelete();
//       };

//---------------------------------------------------------
//   CtrlCanvas
//---------------------------------------------------------

class CtrlCanvas : public MusEGui::View {
      Q_OBJECT
    
      MidiEditor* editor;
      MusECore::MidiTrack* curTrack;
      MusECore::MidiPart* curPart;
      MusECore::MidiCtrlValList* ctrl;
      MusECore::MidiController* _controller;
      CtrlPanel* _panel;
      int _cnum;
      int _dnum; // Current real drum controller number (anote).
      int _didx; // Current real drum controller index.
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
      //bool _rasterizeDrag;
      //Qt::CursorShape _cursorShape;

      void viewMousePressEvent(QMouseEvent* event);
      void viewMouseMoveEvent(QMouseEvent*);
      void viewMouseReleaseEvent(QMouseEvent*);
      virtual void wheelEvent(QWheelEvent*);

// REMOVE Tim. citem. Changed.
//       virtual void draw(QPainter& p, const QRect& rect);
      virtual void draw(QPainter&, const QRect& rect, const QRegion& = QRegion());
// REMOVE Tim. citem. Changed.
//       virtual void pdraw(QPainter&, const QRect&);
      virtual void pdraw(QPainter&, const QRect&, const QRegion& = QRegion());
// REMOVE Tim. citem. Changed.
//       virtual void drawOverlay(QPainter&)
      virtual void drawOverlay(QPainter&, const QRect&, const QRegion& = QRegion());
      virtual QRect overlayRect() const;

      void changeValRamp(int x1, int x2, int y1, int y2);
      void newValRamp(int x1, int y1, int x2, int y2);
      void changeVal(int x1, int x2, int y);
      void newVal(int x1, int y);
      void newVal(int x1, int y1, int x2, int y2);
      void deleteVal(int x1, int x2, int y);

      bool setCurTrackAndPart();
//       void drawMoving(QPainter&, const CItem*, const QRect&, const QRegion& = QRegion());
      void drawMoving(QPainter& p, const QRect& rect, const QRegion& region, const MusECore::MidiPart* part);
      void pdrawItems(QPainter& p, const QRect& rect, const MusECore::MidiPart* part, bool velo, bool fg);
      void pFillBackgrounds(QPainter& p, const QRect& rect, const MusECore::MidiPart* part);
      void pdrawExtraDrumCtrlItems(QPainter& p, const QRect& rect, const MusECore::MidiPart* part, int drum_ctl);
      void partControllers(const MusECore::MidiPart*, int, int*, int*, MusECore::MidiController**, MusECore::MidiCtrlValList**);
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

      CItemList items;
      // To avoid working directly with a potentially huge number of items
      //  in the item list, these 'indexing' lists are used instead.
      CItemList selection;
      CItemList moving;
      //CItemSet  adding;
      //CItemSet  modifying;
      //CItemSet  deleting;
      
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
//       int button;
      
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
// REMOVE Tim. citem. Added.
//       // Checks the item indexing lists for events which should be created, deleted, or modified,
//       //  and executes any necessary corresponding operations.
//       void evaluateItems();
// REMOVE Tim. citem. Removed.
//       void updateSelections();
      // REMOVE Tim. citem. Added.
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
      //void endMoveItems(const QPoint&, DragType, int dir, bool rasterize = true);
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
      // REMOVE Tim. citem. Added.
      bool itemsAreSelected() const { return !selection.empty(); }
      //const CEventList& selectedItems() const { return selection; }
      // Adds all selected items to the given list. Does not clear the list first.
      // Checks for duplicates, employing the 'tagged' features.
      //void getAllSelectedItems(CItemSet&) const;
      // Tags all selected item objects. Checks for duplicates, employing the 'tagged' features.
//       void tagItems(bool tagAllItems = false, bool tagAllParts = false, bool range = false,
//         const MusECore::Pos& = MusECore::Pos(), const MusECore::Pos& = MusECore::Pos()) const;
      // Appends given tag list with item objects according to options. Avoids duplicate events or clone events.
      // Special: We 'abuse' a controller event's length, normally 0, to indicate visual item length.
      void tagItems(MusECore::TagEventList* list, const MusECore::EventTagOptionsStruct& options) const;
      };

} // namespace MusEGui

#endif

