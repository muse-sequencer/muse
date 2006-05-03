//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: dcanvas.h,v 1.30 2006/01/27 21:12:10 wschweer Exp $
//  (C) Copyright 1999-2006 Werner Schweer (ws@seh.de)
//=========================================================

#ifndef __DCANVAS_H__
#define __DCANVAS_H__

#include "ecanvas.h"

class MidiEditor;
class DrumMap;

//---------------------------------------------------------
//   DrumCanvas
//---------------------------------------------------------

class DrumCanvas : public EventCanvas {
      Q_OBJECT

      int singlePitch;
      virtual void mousePress(QMouseEvent*);
      virtual void paint(QPainter&, QRect);
      virtual void addItem(Part* part, const Event& event);
      virtual void moveItem(CItem*, DragType);
      virtual CItem* newItem(const QPoint&, int);
      virtual void resizeItem(CItem*, bool);
      virtual void newItem(CItem*, bool);
      virtual bool deleteItem(CItem*);
      virtual CItem* searchItem(const QPoint& p) const;

      void copy();
      void paste();
      void startDrag(CItem*, bool copymode);
      void dragEnterEvent(QDragEnterEvent* event);
      void dragMoveEvent(QDragMoveEvent*);
      void dragLeaveEvent(QDragLeaveEvent*);
      void viewDropEvent(QDropEvent* event);
      virtual void resizeEvent(QResizeEvent*);
      virtual void paintDrumList(QPainter&, QRect);
      virtual void selectLasso(bool toggle);

	DrumMap* drumMap() const;

   protected:
      virtual int y2pitch(int y) const;
      virtual int pitch2y(int pitch) const;

   signals:
      void newWidth(int);

   public slots:
      void keyPressed(int, bool);
      void keyReleased(int, bool);

   public:
      enum {
         CMD_CUT, CMD_COPY, CMD_PASTE,
         CMD_SELECT_ALL, CMD_SELECT_NONE, CMD_SELECT_INVERT,
         CMD_SELECT_ILOOP, CMD_SELECT_OLOOP, CMD_DEL,
         CMD_FIXED_LEN, CMD_RIGHT, CMD_LEFT, CMD_MODIFY_VELOCITY,
         CMD_COMMANDS
         };
      DrumCanvas(MidiEditor*);
      void cmd(int);
      virtual void modifySelected(NoteInfo::ValType type, int delta);
      };
#endif

