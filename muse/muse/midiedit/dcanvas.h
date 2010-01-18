//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: dcanvas.h,v 1.8.2.2 2009/02/02 21:38:00 terminator356 Exp $
//  (C) Copyright 1999 Werner Schweer (ws@seh.de)
//=========================================================

#ifndef __DCANVAS_H__
#define __DCANVAS_H__

#include "ecanvas.h"
#include "song.h"

#define TH 18

class MidiEditor;

//---------------------------------------------------------
//   DEvent
//    ''visual'' Drum Event
//---------------------------------------------------------

class DEvent : public CItem {
   public:
      DEvent(Event e, Part* p);
      };

class ScrollScale;
class PianoRoll;
class QTextDrag;

//---------------------------------------------------------
//   DrumCanvas
//---------------------------------------------------------

class DrumCanvas : public EventCanvas {

      Q_OBJECT
      virtual void drawCanvas(QPainter&, const QRect&);
      virtual void drawItem(QPainter&, const CItem*, const QRect&);
      virtual void drawMoving(QPainter&, const CItem*, const QRect&);
      virtual void moveCanvasItems(CItemList&, int, int, DragType, int*);
      // Changed by T356. 
      //virtual bool moveItem(CItem*, const QPoint&, DragType, int*);
      virtual bool moveItem(CItem*, const QPoint&, DragType);
      virtual CItem* newItem(const QPoint&, int);
      virtual void resizeItem(CItem*, bool);
      virtual void newItem(CItem*, bool);
      virtual bool deleteItem(CItem*);

      int y2pitch(int y) const;
      int pitch2y(int pitch) const;
      QTextDrag* getTextDrag(QWidget* parent);
      void copy();
      int pasteAt(const QString& pt, int pos);
      void paste();
      void startDrag(CItem*, bool copymode);
      void dragEnterEvent(QDragEnterEvent* event);
      void dragMoveEvent(QDragMoveEvent*);
      void dragLeaveEvent(QDragLeaveEvent*);
      void viewDropEvent(QDropEvent* event);
      virtual void addItem(Part*, Event&);
      virtual void resizeEvent(QResizeEvent*);
      virtual void curPartChanged();

   signals:
      void newWidth(int);

   public slots:
      void mapChanged(int, int);
      void keyPressed(int, bool);
      void keyReleased(int, bool);

   public:
      enum {
         CMD_CUT, CMD_COPY, CMD_PASTE, CMD_SAVE, CMD_LOAD, CMD_RESET,
         CMD_SELECT_ALL, CMD_SELECT_NONE, CMD_SELECT_INVERT,
         CMD_SELECT_ILOOP, CMD_SELECT_OLOOP, CMD_SELECT_PREV_PART, CMD_SELECT_NEXT_PART, 
         CMD_DEL, CMD_FIXED_LEN, CMD_RIGHT, CMD_LEFT, CMD_MODIFY_VELOCITY
         };
      DrumCanvas(MidiEditor*, QWidget*, int, int,
         const char* name = 0);
      void cmd(int);
      virtual void modifySelected(NoteInfo::ValType type, int delta);
      };
#endif

