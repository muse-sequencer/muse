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

#include <list>


#include "view.h"
#include "tools.h"
#include "midictrl.h"
#include "event.h"

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

//---------------------------------------------------------
//   CEvent
//    ''visual'' Controller Event
//---------------------------------------------------------

class CEvent {
   private:
      MusECore::Event _event;
      int       _val;
      MusECore::MidiPart* _part;
      int ex;

   public:
      CEvent(MusECore::Event e, MusECore::MidiPart* part, int v);
      MusECore::Event event() const          { return _event; }
      void setEvent(MusECore::Event& ev)     { _event = ev; }
      bool selected() const { return !_event.empty() && _event.selected(); }
      void setSelected(bool v) { if(!_event.empty()) _event.setSelected(v); }
      int val() const              { return _val;   }
      void setVal(int v)           { _val = v; }
      void setEX(int v)            { ex = v; }
      MusECore::MidiPart* part() const       { return _part;  }
      bool contains(int x1, int x2) const;
      bool intersects(const MusECore::MidiController*, const QRect&, const int tickstep, const int windowHeight) const;
      int x()                      { return ex; }
      };

typedef std::list<CEvent*>::iterator iCEvent;
typedef std::list<CEvent*>::const_iterator ciCEvent;

//---------------------------------------------------------
//   CEventList
//    Controller Item List
//---------------------------------------------------------

class CEventList: public std::list<CEvent*> {
   public:
      void add(CEvent* item) { push_back(item); }
      
      void clearDelete();
      };


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

      void viewMousePressEvent(QMouseEvent* event);
      void viewMouseMoveEvent(QMouseEvent*);
      void viewMouseReleaseEvent(QMouseEvent*);

      virtual void draw(QPainter&, const QRect&);
      virtual void pdraw(QPainter&, const QRect&);
      virtual void drawOverlay(QPainter& p);
      virtual QRect overlayRect() const;

      void changeValRamp(int x1, int x2, int y1, int y2);
      void newValRamp(int x1, int y1, int x2, int y2);
      void changeVal(int x1, int x2, int y);
      void newVal(int x1, int y);
      void newVal(int x1, int y1, int x2, int y2);
      void deleteVal(int x1, int x2, int y);

      bool setCurTrackAndPart();
      void pdrawItems(QPainter&, const QRect&, const MusECore::MidiPart*, bool, bool);
      void partControllers(const MusECore::MidiPart*, int, int*, int*, MusECore::MidiController**, MusECore::MidiCtrlValList**);
      
      

   protected:
      enum DragMode { DRAG_OFF, DRAG_NEW, DRAG_MOVE_START, DRAG_MOVE,
            DRAG_DELETE, DRAG_COPY_START, DRAG_COPY,
            DRAG_RESIZE, DRAG_LASSO_START, DRAG_LASSO
            };

      CEventList items;
      CEventList selection;
      CEventList moving;
      CEvent* curItem;

      DragMode drag;
      QRect lasso;
      QPoint start;
      MusEGui::Tool tool;
      unsigned pos[3];
      int curDrumInstrument;    //Used by the drum-editor to view velocity of only one key (one drum)
      
      void leaveEvent(QEvent*e);
      QPoint raster(const QPoint&) const;

      // selection
      bool isSingleSelection()  { return selection.size() == 1; }
      void deselectAll();
      void selectItem(CEvent* e);
      void deselectItem(CEvent* e);

      void setMidiController(int);
      void updateItems();
      void updateSelections();
      
   private slots:
      void songChanged(int type);
      void configChanged();    
      void setCurDrumInstrument(int);

   public slots:
      void setTool(int t);
      void setPos(int, unsigned, bool adjustScrollbar);
      void setController(int ctrl);

   signals:
      void followEvent(int);
      void xposChanged(unsigned);
      void yposChanged(int);

   public:
      CtrlCanvas(MidiEditor*, QWidget* parent, int,
         const char* name = 0, CtrlPanel* pnl = 0);
      ~CtrlCanvas();   
      void setPanel(CtrlPanel* pnl) { _panel = pnl; }
      MusECore::MidiCtrlValList* ctrlValList() { return ctrl; }
      MusECore::MidiController* controller() { return _controller; }
      MusECore::MidiTrack* track() const { return curTrack; }
      };

} // namespace MusEGui

#endif

