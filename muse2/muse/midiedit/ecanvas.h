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

#include "type_defs.h"
#include "canvas.h"
#include "noteinfo.h"
#include <QEvent>
#include <QKeyEvent>

class QMimeData;
class QDrag;
class QString;
class QDropEvent;
class QPainter;

namespace MusECore {
class MidiPart;
class MidiTrack;
class Part;
class Pos;

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
      MidiEditor* _editor;
      //unsigned start_tick, end_tick;   // REMOVE Tim.
      MusECore::Pos _startPos, _endPos;
      int curVelo;
      bool _steprec;
      bool _midiin;
      bool _setCurPartIfOnlyOneEventIsSelected;

      void updateSelection();
      virtual CItem* addItem(MusECore::Part*, MusECore::Event&) = 0;
      virtual void draw(QPainter&, const QRect&);  
      virtual QPoint rasterPoint(const QPoint&) const;
      virtual MusECore::Undo moveCanvasItems(CItemList&, int, int, DragType) = 0;
      virtual bool moveItem(MusECore::Undo&, CItem*, const QPoint&, DragType) = 0;
      virtual void endMoveItems(const QPoint&, DragType, int dir);

   public slots:
      void redrawGrid()       { redraw(); }
      void setSteprec(bool f) { _steprec = f; }
      void setMidiin(bool f)  { _midiin = f; }
      void setPos(int idx, const MusECore::Pos& pos, bool adjustScrollbar);

   signals:
      void pitchChanged(int);       // current cursor position
      void timeChanged(unsigned);
      void timeChanged(const MusECore::Pos&);
      void selectionChanged(int /*tick or frame*/ , MusECore::Event&, MusECore::Part*, bool /*update*/);
      void enterCanvas();

   public:
      EventCanvas(MidiEditor*, QWidget*, int, int, const char* name = 0); 
      //EventCanvas(MidiEditor*, QWidget*, int, int, const char* name = 0, 
      //            MusECore::Pos::TType time_type = MusECore::Pos::TICKS,
      //            bool formatted = true, int raster = 1);
      MusECore::MidiTrack* track() const;
      //virtual unsigned start() const       { return start_tick; }  // REMOVE Tim.
      //virtual unsigned end() const         { return end_tick; }
      virtual MusECore::Pos start() const       { return _startPos; }
      virtual MusECore::Pos end() const         { return _endPos; }
      bool midiin() const     { return _midiin; }
      bool steprec() const    { return _steprec; }
      virtual QString getCaption() const;
      virtual void songChanged(MusECore::SongChangedFlags_t);
      //virtual void range(int* s, int* e) const { *s = start_tick; *e = end_tick; }
      virtual void range(MusECore::Pos* s, MusECore::Pos* e) const { *s = _startPos; *e = _endPos; }
      void playEvents(bool flag) { _playEvents = flag; }
      virtual void selectAtTick(unsigned int tick);
      virtual void viewDropEvent(QDropEvent* event);
      virtual void modifySelected(NoteInfo::ValType, int /*val*/, bool /*delta_mode*/ = true) {}
      virtual void keyPress(QKeyEvent*);
      };

} // namespace MusEGui

#endif

