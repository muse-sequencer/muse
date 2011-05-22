//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: ecanvas.h,v 1.5.2.4 2009/02/02 21:38:00 terminator356 Exp $
//  (C) Copyright 2001 Werner Schweer (ws@seh.de)
//=========================================================

#ifndef __ECANVAS_H__
#define __ECANVAS_H__

#include "canvas.h"
#include "noteinfo.h"
#include <QEvent>
#include <QKeyEvent>

class MidiPart;
class MidiTrack;
class MidiEditor;
class Part;
class QMimeData;
class QDrag;
class QString;
class QDropEvent;

struct PartToChange
{
  Part* npart;
  int   xdiff;
}; 
typedef std::map<Part*, PartToChange> PartsToChangeMap;
typedef std::map<Part*, PartToChange>::iterator iPartToChange;
  
//---------------------------------------------------------
//   EventCanvas
//---------------------------------------------------------

class EventCanvas : public Canvas {
      Q_OBJECT
      virtual void leaveEvent(QEvent*e);
      virtual void enterEvent(QEvent*e);
      
      virtual void mouseMove(QMouseEvent* event);

   protected:
      bool _playEvents;
      MidiEditor* editor;
      unsigned start_tick, end_tick;
      int curVelo;
      bool _steprec;
      bool _midiin;

      void updateSelection();
      virtual void addItem(Part*, Event&) = 0;
      // Added by T356.
      virtual QPoint raster(const QPoint&) const;
      virtual Undo moveCanvasItems(CItemList&, int, int, DragType, int*) = 0;
      virtual UndoOp moveItem(CItem*, const QPoint&, DragType) = 0;
      virtual void endMoveItems(const QPoint&, DragType, int dir);
      virtual void updateSong(DragType, int flags = 0);

   public slots:
      void redrawGrid()       { redraw(); }
      void setSteprec(bool f) { _steprec = f; }
      void setMidiin(bool f)  { _midiin = f; }

   signals:
      void pitchChanged(int);       // current cursor position
      void timeChanged(unsigned);
      void selectionChanged(int, Event&, Part*);
      void enterCanvas();

   public:
      EventCanvas(MidiEditor*, QWidget*, int, int, const char* name = 0);
      MidiTrack* track() const;
      unsigned start() const       { return start_tick; }
      unsigned end() const         { return end_tick; }
      bool midiin() const     { return _midiin; }
      bool steprec() const    { return _steprec; }
      QString getCaption() const;
      void songChanged(int);
      void range(int* s, int* e) const { *s = start_tick; *e = end_tick; }
      void playEvents(bool flag) { _playEvents = flag; }
      void selectAtTick(unsigned int tick);
      //QDrag* getTextDrag(QWidget* parent);
      QMimeData* getTextDrag();
      void pasteAt(const QString& pt, int pos);
      void viewDropEvent(QDropEvent* event);
      virtual void modifySelected(NoteInfo::ValType, int) {}
      virtual void keyPress(QKeyEvent*);
      };

#endif

