//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: prcanvas.h,v 1.36 2006/02/08 17:33:41 wschweer Exp $
//  (C) Copyright 1999-2006 Werner Schweer (ws@seh.de)
//=========================================================

#ifndef __PRCANVAS_H__
#define __PRCANVAS_H__

#include "ecanvas.h"

class CItem;

//---------------------------------------------------------
//   PianoCanvas
//---------------------------------------------------------

class PianoCanvas : public EventCanvas {
      Q_OBJECT

      int colorMode;
      int playedPitch;

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
      enum {
         CMD_CUT, CMD_COPY, CMD_PASTE,
         CMD_DEL,
         CMD_OVER_QUANTIZE, CMD_ON_QUANTIZE, CMD_ONOFF_QUANTIZE,
         CMD_ITERATIVE_QUANTIZE,
         CMD_SELECT_ALL, CMD_SELECT_NONE, CMD_SELECT_INVERT,
         CMD_SELECT_ILOOP, CMD_SELECT_OLOOP,
         CMD_MODIFY_GATE_TIME, CMD_MODIFY_VELOCITY,
         CMD_CRESCENDO, CMD_TRANSPOSE, CMD_THIN_OUT, CMD_ERASE_EVENT,
         CMD_NOTE_SHIFT, CMD_MOVE_CLOCK, CMD_COPY_MEASURE,
         CMD_ERASE_MEASURE, CMD_DELETE_MEASURE, CMD_CREATE_MEASURE,
         CMD_END
         };

      PianoCanvas(MidiEditor*);
      void cmd(int, int, int, bool);
      void setColorMode(int mode);
      virtual void modifySelected(NoteInfo::ValType type, int delta);
      };
#endif

