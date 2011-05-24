//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: prcanvas.h,v 1.5.2.6 2009/11/16 11:29:33 lunar_shuttle Exp $
//  (C) Copyright 1999 Werner Schweer (ws@seh.de)
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

#define KH        13

//---------------------------------------------------------
//   NEvent
//    ''visual'' Note Event
//---------------------------------------------------------

class NEvent : public CItem {
   public:
      NEvent(Event& e, Part* p, int y);
      };

class ScrollScale;
class PianoRoll;
class QRect;

//---------------------------------------------------------
//   PianoCanvas
//---------------------------------------------------------

class PianoCanvas : public EventCanvas {
      int colorMode;
      int playedPitch;
      
      QTimer* chordTimer;
      unsigned chordTimer_setToTick;

      bool _steprec;

      Q_OBJECT
      virtual void viewMouseDoubleClickEvent(QMouseEvent*);
      virtual void drawItem(QPainter&, const CItem*, const QRect&);
      void drawTopItem(QPainter &p, const QRect &rect);
      virtual void drawMoving(QPainter&, const CItem*, const QRect&);
      virtual void moveCanvasItems(CItemList&, int, int, DragType, int*);
      // Changed by T356. 
      //virtual bool moveItem(CItem*, const QPoint&, DragType, int*);
      virtual bool moveItem(CItem*, const QPoint&, DragType);
      virtual CItem* newItem(const QPoint&, int);
      virtual void resizeItem(CItem*, bool noSnap);
      virtual void newItem(CItem*, bool noSnap);
      virtual bool deleteItem(CItem*);
      virtual void startDrag(CItem* item, bool copymode);
      virtual void dragEnterEvent(QDragEnterEvent* event);
      virtual void dragMoveEvent(QDragMoveEvent*);
      virtual void dragLeaveEvent(QDragLeaveEvent*);
      virtual void addItem(Part*, Event&);

      int y2pitch(int) const;
      int pitch2y(int) const;
      virtual void drawCanvas(QPainter&, const QRect&);
      void copy();
      void paste();
      virtual void itemPressed(const CItem*);
      virtual void itemReleased(const CItem*, const QPoint&);
      virtual void itemMoved(const CItem*, const QPoint&);
      virtual void curPartChanged();
      virtual void resizeEvent(QResizeEvent*);

   private slots:
      void midiNote(int pitch, int velo);
      void chordTimerTimedOut();

   signals:
      void quantChanged(int);
      void rasterChanged(int);
      void newWidth(int);

   public slots:
      void pianoCmd(int);
      void pianoPressed(int pitch, int velocity, bool shift);
      void pianoReleased(int pitch, bool);
      void setSteprec(bool f) { _steprec = f; }

   public:
      enum {
         CMD_CUT, CMD_COPY, CMD_PASTE, CMD_DEL,
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
      void cmd(int cmd);
      void setColorMode(int mode) {
            colorMode = mode;
            redraw();
            }
      virtual void modifySelected(NoteInfo::ValType type, int delta);

      bool steprec() const    { return _steprec; }
      };
#endif

