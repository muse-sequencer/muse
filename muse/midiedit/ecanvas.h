//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: ecanvas.h,v 1.5.2.4 2009/02/02 21:38:00 terminator356 Exp $
//  (C) Copyright 2001 Werner Schweer (ws@seh.de)
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

#ifndef __ECANVAS_H__
#define __ECANVAS_H__

#include "canvas.h"
#include "noteinfo.h"
#include <QEvent>
#include <QKeyEvent>

class QMimeData;
class QDrag;
class QString;
class QDropEvent;

namespace MusECore {
class MidiPart;
class MidiTrack;
class Part;

struct PartToChange
{
  Part* npart;
  int   xdiff;
}; 
typedef std::map<Part*, PartToChange> PartsToChangeMap;
typedef std::map<Part*, PartToChange>::iterator iPartToChange;
  
}

namespace MusEGui {

class MidiEditor;
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
      virtual CItem* addItem(MusECore::Part*, MusECore::Event&) = 0;
      virtual QPoint raster(const QPoint&) const;
      virtual MusECore::Undo moveCanvasItems(CItemList&, int, int, DragType) = 0;
      virtual MusECore::UndoOp moveItem(CItem*, const QPoint&, DragType) = 0;
      virtual void endMoveItems(const QPoint&, DragType, int dir);

   public slots:
      void redrawGrid()       { redraw(); }
      void setSteprec(bool f) { _steprec = f; }
      void setMidiin(bool f)  { _midiin = f; }

   signals:
      void pitchChanged(int);       // current cursor position
      void timeChanged(unsigned);
      void selectionChanged(int /*tick*/ , MusECore::Event&, MusECore::Part*, bool /*update*/);
      void enterCanvas();

   public:
      EventCanvas(MidiEditor*, QWidget*, int, int, const char* name = 0);
      MusECore::MidiTrack* track() const;
      unsigned start() const       { return start_tick; }
      unsigned end() const         { return end_tick; }
      bool midiin() const     { return _midiin; }
      bool steprec() const    { return _steprec; }
      QString getCaption() const;
      void songChanged(int);
      void range(int* s, int* e) const { *s = start_tick; *e = end_tick; }
      void playEvents(bool flag) { _playEvents = flag; }
      void selectAtTick(unsigned int tick);
      void viewDropEvent(QDropEvent* event);
      virtual void modifySelected(NoteInfo::ValType, int /*val*/, bool /*delta_mode*/ = true) {}
      virtual void keyPress(QKeyEvent*);
      };

} // namespace MusEGui

#endif

