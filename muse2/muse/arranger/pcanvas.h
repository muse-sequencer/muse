//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: pcanvas.h,v 1.11.2.4 2009/05/24 21:43:44 terminator356 Exp $
//  (C) Copyright 1999 Werner Schweer (ws@seh.de)
//  (C) Copyright 2011 Tim E. Real (terminator356 on users DOT sourceforge DOT net)
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

#ifndef __PCANVAS_H__
#define __PCANVAS_H__

#include <QVector>
#include <set>

#include "song.h"
#include "canvas.h"
#include "trackautomationview.h"

class QDropEvent;
class QMouseEvent;
class QKeyEvent;
class QEvent;
class QDragEnterEvent;
class QLineEdit;

class MidiEditor;
class QMenu;
class Xml;
class CtrlVal;

#define beats     4

namespace MusEArranger {

//---------------------------------------------------------
//   NPart
//    ''visual'' Part
//    wraps Parts with additional information needed
//    for displaying
//---------------------------------------------------------

class NPart : public MusEWidget::CItem {
   public:
      NPart(Part* e);
      const QString name() const     { return part()->name(); }
      void setName(const QString& s) { part()->setName(s); }
      Track* track() const           { return part()->track(); }
      
      bool leftBorderTouches;  // Whether the borders touch other part borders. 
      bool rightBorderTouches;
      };

enum ControllerVals { doNothing, movingController, addNewController };
struct AutomationObject {
  //CtrlVal *currentCtrl;
  int currentCtrlFrame;
  bool currentCtrlValid;
  CtrlList *currentCtrlList;
  Track *currentTrack;
  bool moveController;
  ControllerVals controllerState;
};

//---------------------------------------------------------
//   PartCanvas
//---------------------------------------------------------

class PartCanvas : public MusEWidget::Canvas {
      Q_OBJECT
      int* _raster;
      TrackList* tracks;

      Part* resizePart;
      QLineEdit* lineEditor;
      NPart* editPart;
      int curColorIndex;
      bool editMode;

      AutomationObject automation;

      //std::vector<TrackAutomationView*> automationViews;
      
      virtual void keyPress(QKeyEvent*);
      virtual void mousePress(QMouseEvent*);
      virtual void mouseMove(QMouseEvent* event);
      virtual void mouseRelease(const QPoint&);
      virtual void viewMouseDoubleClickEvent(QMouseEvent*);
      virtual void leaveEvent(QEvent*e);
      virtual void drawItem(QPainter&, const MusEWidget::CItem*, const QRect&);
      virtual void drawMoving(QPainter&, const MusEWidget::CItem*, const QRect&);
      virtual void updateSelection();
      virtual QPoint raster(const QPoint&) const;
      virtual int y2pitch(int y) const;
      virtual int pitch2y(int p) const;
      
      virtual MusEWidget::CItem* newItem(const QPoint&, int);
      virtual void resizeItem(MusEWidget::CItem*,bool, bool ctrl);
      virtual void newItem(MusEWidget::CItem*,bool);
      virtual bool deleteItem(MusEWidget::CItem*);
      virtual void moveCanvasItems(MusEWidget::CItemList&, int, int, DragType);
      virtual UndoOp moveItem(MusEWidget::CItem*, const QPoint&, DragType);

      virtual void updateSong(DragType, int);
      virtual void startDrag(MusEWidget::CItem*, DragType);
      virtual void dragEnterEvent(QDragEnterEvent*);
      virtual void viewDropEvent(QDropEvent*);

      virtual QMenu* genItemPopup(MusEWidget::CItem*);
      virtual void itemPopup(MusEWidget::CItem*, int, const QPoint&);

      void glueItem(MusEWidget::CItem* item);
      void splitItem(MusEWidget::CItem* item, const QPoint&);

      void copy(PartList*);
      void copy_in_range(PartList*);
      enum paste_mode_t { PASTEMODE_MIX, PASTEMODE_MOVEALL, PASTEMODE_MOVESOME };
      void paste(bool clone = false, paste_mode_t paste_mode = PASTEMODE_MIX, bool to_single_track=false, int amount=1, int raster=1536);
      Undo pasteAt(const QString&, Track*, unsigned int, bool clone = false, bool toTrack = true, int* finalPosPtr = NULL, std::set<Track*>* affected_tracks = NULL);
      //Part* readClone(Xml&, Track*, bool toTrack = true);
      void drawWavePart(QPainter&, const QRect&, WavePart*, const QRect&);
      //void drawMidiPart(QPainter&, const QRect& rect, EventList* events, MidiTrack*mt, const QRect& r, int pTick, int from, int to);
      void drawMidiPart(QPainter&, const QRect& rect, EventList* events, MidiTrack*mt, MidiPart*pt, const QRect& r, int pTick, int from, int to);
      Track* y2Track(int) const;
      void drawAudioTrack(QPainter& p, const QRect& r, const QRect& bbox, AudioTrack* track);
      void drawAutomation(QPainter& p, const QRect& r, AudioTrack* track);
      void drawTopItem(QPainter& p, const QRect& rect);

      void checkAutomation(Track * t, const QPoint& pointer, bool addNewCtrl);
      void processAutomationMovements(QPoint pos, bool addPoint);
      double dbToVal(double inDb);
      double valToDb(double inV);

   protected:
      virtual void drawCanvas(QPainter&, const QRect&);
      virtual void endMoveItems(const QPoint&, DragType, int dir);

   signals:
      void timeChanged(unsigned);
      void tracklistChanged();
      void dclickPart(Track*);
      void selectionChanged();
      void dropSongFile(const QString&);
      void dropMidiFile(const QString&);
      void setUsedTool(int);
      void trackChanged(Track*);
      void selectTrackAbove();
      void selectTrackBelow();

      void startEditor(PartList*, int);

   private slots:
      void returnPressed();

   public:
      enum { CMD_CUT_PART, CMD_COPY_PART, CMD_COPY_PART_IN_RANGE, CMD_PASTE_PART, CMD_PASTE_CLONE_PART,
             CMD_PASTE_DIALOG, CMD_PASTE_CLONE_DIALOG, CMD_INSERT_EMPTYMEAS };

      PartCanvas(int* raster, QWidget* parent, int, int);
      virtual ~PartCanvas();
      void partsChanged();
      void cmd(int);
   public slots:
   void redirKeypress(QKeyEvent* e) { keyPress(e); }
   void controllerChanged(Track *t);
};

} // namespace MusEArranger

#endif

