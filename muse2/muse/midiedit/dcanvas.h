//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: dcanvas.h,v 1.8.2.2 2009/02/02 21:38:00 terminator356 Exp $
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

#ifndef __DCANVAS_H__
#define __DCANVAS_H__

#include "ecanvas.h"
#include "song.h"
#include "steprec.h"
#include <map>
#include <QList>
#include <QSet>

#define TH 18


class QResizeEvent;
class QDragEnterEvent;
class QDropEvent;
class QDragMoveEvent;
class QDragLeaveEvent;

class DrumMap;
class MidiEditor;

//---------------------------------------------------------
//   DEvent
//    ''visual'' Drum Event
//---------------------------------------------------------

class DEvent : public MusEWidget::CItem {
   public:
      DEvent(Event e, Part* p, int instr);
      };

class ScrollScale;
class PianoRoll;


struct instrument_number_mapping_t
{
  QSet<Track*> tracks;
  int pitch;
  
  instrument_number_mapping_t()
  {
    pitch=-1;
    tracks.clear();
  }
  
  instrument_number_mapping_t(const QSet<Track*>& tr, int p)
  {
    tracks=tr;
    pitch=p;
  }
};

//---------------------------------------------------------
//   DrumCanvas
//---------------------------------------------------------

class DrumCanvas : public EventCanvas {
      Q_OBJECT
      
      bool old_style_drummap_mode;
      DrumMap* ourDrumMap;
      bool must_delete_our_drum_map; //FINDMICH really delete it!
      QVector<instrument_number_mapping_t> instrument_map;
      
      StepRec* steprec;
      
      // Cursor tool position
      QPoint cursorPos;
      int _stepSize;

      
      virtual void drawCanvas(QPainter&, const QRect&);
      virtual void drawItem(QPainter&, const MusEWidget::CItem*, const QRect&);
      void drawTopItem(QPainter& p, const QRect& rect);
      virtual void drawMoving(QPainter&, const MusEWidget::CItem*, const QRect&);
      virtual Undo moveCanvasItems(MusEWidget::CItemList&, int, int, DragType);
      virtual UndoOp moveItem(MusEWidget::CItem*, const QPoint&, DragType);
      virtual MusEWidget::CItem* newItem(const QPoint&, int);
      virtual void resizeItem(MusEWidget::CItem*, bool, bool);
      virtual void newItem(MusEWidget::CItem*, bool);
      virtual void newItem(MusEWidget::CItem*, bool, bool replace );
      virtual bool deleteItem(MusEWidget::CItem*);
      MusEWidget::CItem* newItem(int tick, int instrument, int velocity);

      int y2pitch(int y) const;
      int pitch2y(int pitch) const;
      void startDrag(MusEWidget::CItem*, bool copymode);
      void dragEnterEvent(QDragEnterEvent* event);
      void dragMoveEvent(QDragMoveEvent*);
      void dragLeaveEvent(QDragLeaveEvent*);
      virtual void addItem(Part*, Event&);
      virtual void resizeEvent(QResizeEvent*);
      virtual void curPartChanged();
      int getNextStep(unsigned int pos, int basicStep, int stepSize=1);
      
      /* FINDMICH OBSOLETE
      int parts_first_instrument(Part* p);
      int tracks_first_instrument(Track* t);
      bool is_track_of_instrument(Track* t, int instr);
      QSet<Track*> tracks_of_instrument(int instr);
      */

   signals:
      void newWidth(int);

   private slots:
      void midiNote(int pitch, int velo);
      
   public slots:
      void mapChanged(int, int);
      void keyPressed(int, int);
      void keyReleased(int, bool);
      void setTool2(int);
      void setCurDrumInstrument(int);
      virtual void setStep(int);
      void moveAwayUnused();

   public:
      enum {
         CMD_CUT, CMD_COPY, CMD_COPY_RANGE, CMD_PASTE, CMD_PASTE_DIALOG, CMD_SAVE, CMD_LOAD, CMD_RESET,
         CMD_SELECT_ALL, CMD_SELECT_NONE, CMD_SELECT_INVERT,
         CMD_SELECT_ILOOP, CMD_SELECT_OLOOP, CMD_SELECT_PREV_PART, CMD_SELECT_NEXT_PART, 
         CMD_DEL, CMD_FIXED_LEN, CMD_RIGHT, CMD_LEFT, CMD_RIGHT_NOSNAP, CMD_LEFT_NOSNAP, CMD_MODIFY_VELOCITY, CMD_CRESCENDO,
         CMD_QUANTIZE, CMD_ERASE_EVENT, CMD_NOTE_SHIFT, CMD_DELETE_OVERLAPS, CMD_REORDER_LIST
         };
      DrumCanvas(MidiEditor*, QWidget*, int, int,
         const char* name = 0);
      void cmd(int);
      virtual void modifySelected(MusEWidget::NoteInfo::ValType type, int delta);
      virtual void keyPress(QKeyEvent* event);
      Event *getEventAtCursorPos();
      void selectCursorEvent(Event *ev);
      int drum_map_size() { return instrument_map.size(); }
      int pitch_and_track_to_instrument(int pitch, Track* track);
      DrumMap* getOurDrumMap() { return ourDrumMap; } //FINDMICH UGLY
      int getOurDrumMapSize() { return instrument_map.size(); } //FINDMICH UGLY
      };
#endif

