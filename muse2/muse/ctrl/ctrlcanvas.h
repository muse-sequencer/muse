//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: ctrlcanvas.h,v 1.7.2.4 2009/06/01 20:15:53 spamatica Exp $
//  (C) Copyright 1999 Werner Schweer (ws@seh.de)
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

class Event;
class MidiPart;
class PartList;
class MidiTrack;
class MidiEditor;
class CtrlPanel;

//---------------------------------------------------------
//   CEvent
//    ''visual'' Controller Event
//---------------------------------------------------------

class CEvent {
   public:
      //enum State { Normal, Selected };
   
   private:
      Event _event;
      //State _state;
      int       _val;
      MidiPart* _part;
      int ex;

   public:
      CEvent(Event e, MidiPart* part, int v);
      Event event() const          { return _event; }
      void setEvent(Event& ev)     { _event = ev; }
      //State state() { return _state; }
      //void setState(State s) { _state = s; }
      //bool isSelected() { return _state == Selected; }
      bool selected() const { return !_event.empty() && _event.selected(); }
      void setSelected(bool v) { if(!_event.empty()) _event.setSelected(v); }
      int val() const              { return _val;   }
      void setVal(int v)           { _val = v; }
      void setEX(int v)            { ex = v; }
      MidiPart* part() const       { return _part;  }
      bool contains(int /*x1*/, int /*x2*/) const;
      bool intersects(const MidiController*, const QRect&, const int /*windowHeight*/) const;
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

class CtrlCanvas : public View {
      MidiEditor* editor;
      MidiTrack* curTrack;
      MidiPart* curPart;
      MidiCtrlValList* ctrl;
      MidiController* _controller;
      CtrlPanel* _panel;
      int _cnum;
      // Current real drum controller number (anote).
      int _dnum;
      // Current real drum controller index.
      int _didx;
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
      //void newVal(int x1, int x2, int y);
      void newVal(int x1, int y);
      void newVal(int x1, int y1, int x2, int y2);
      void deleteVal(int x1, int x2, int y);

      bool setCurTrackAndPart();
      void pdrawItems(QPainter&, const QRect&, const MidiPart*, bool, bool);
      void partControllers(const MidiPart*, int, int*, int*, MidiController**, MidiCtrlValList**);
      
      Q_OBJECT

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
      Tool tool;
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
      MidiCtrlValList* ctrlValList() { return ctrl; }
      MidiController* controller() { return _controller; }
      MidiTrack* track() const { return curTrack; }
      };
#endif

