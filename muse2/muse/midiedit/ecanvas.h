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

// Item layer indices:
#define _ECANVAS_EVENT_ITEMS_            0

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
      MusECore::Part* _curPart;
      int _curPartId;
      bool _setCurPartIfOnlyOneEventIsSelected;

      void updateSelection();
      virtual CItem* addItem(MusECore::Part*, MusECore::Event&) = 0;
      virtual void deleteItemAtPoint(const QPoint&);
      virtual void selectLasso(bool);
      virtual void selectItemRow(bool);
      virtual QPoint raster(const QPoint&) const;
      virtual MusECore::Undo moveCanvasItems(CItemList&, int, int, DragType) = 0;
      virtual bool moveItem(MusECore::Undo&, CItem*, const QPoint&, DragType) = 0;
      virtual void endMoveItems(const QPoint&, DragType, int dir);
      virtual void curItemChanged();
      virtual void curPartChanged();  
      //virtual void sortLayerItem(CItem* item);
      virtual void drawItemLayer(QPainter& p, const QRect& r, int layer); 

   public slots:
      void redrawGrid()       { redraw(); }
      void setSteprec(bool f) { _steprec = f; }
      void setMidiin(bool f)  { _midiin = f; }

   signals:
      void pitchChanged(int);       // current cursor position
      void timeChanged(unsigned);
      void selectionChanged(int /*tick*/ , MusECore::Event&, MusECore::Part*, bool /*update*/);
      void enterCanvas();
      void curPartHasChanged(MusECore::Part*);

   public:
      EventCanvas(MidiEditor*, QWidget*, int, int, const char* name = 0);
      MusECore::MidiTrack* track() const;
      unsigned start() const       { return start_tick; }
      unsigned end() const         { return end_tick; }
      bool midiin() const     { return _midiin; }
      bool steprec() const    { return _steprec; }
      QString getCaption() const;
      void songChanged(MusECore::SongChangedFlags_t);
      void range(int* s, int* e) const { *s = start_tick; *e = end_tick; }
      void playEvents(bool flag) { _playEvents = flag; }
      void selectAtTick(unsigned int tick);
      void viewDropEvent(QDropEvent* event);
      virtual void modifySelected(NoteInfo::ValType, int /*val*/, bool /*delta_mode*/ = true) {}
      virtual void keyPress(QKeyEvent*);
      MusECore::Part* part() const { return _curPart; }
      void setCurrentPart(MusECore::Part*); 
      };

} // namespace MusEGui

#endif

