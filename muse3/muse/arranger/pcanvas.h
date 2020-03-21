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
#include <QElapsedTimer>

#include "type_defs.h"
#include "song.h"
#include "canvas.h"
#include "trackautomationview.h"

class QDropEvent;
class QMouseEvent;
class QKeyEvent;
class QEvent;
class QDragEnterEvent;
class QLineEdit;
class QMenu;

namespace MusECore {
struct CtrlVal;
class Xml;
class Undo;
class Part;
}

namespace MusEGui {

class MidiEditor;

//---------------------------------------------------------
//   NPart
//    ''visual'' Part
//    wraps Parts with additional information needed
//    for displaying
//---------------------------------------------------------

class NPart : public PItem {
   protected:
      int _serial;
   
   public:
      NPart(MusECore::Part*);
      NPart() { }
      const QString name() const     { return part()->name(); }
      void setName(const QString& s) { part()->setName(s); }
      MusECore::Track* track() const           { return part()->track(); }
      int serial() { return _serial; }
      
      bool leftBorderTouches;  // Whether the borders touch other part borders. 
      bool rightBorderTouches;
      };

enum ControllerVals { doNothing, movingController, addNewController };
struct AutomationObject {
  QPoint startMovePoint;
  QList<int> currentCtrlFrameList;
  bool currentCtrlValid;
  MusECore::CtrlList *currentCtrlList;
  MusECore::Track *currentTrack;
  bool moveController;
  ControllerVals controllerState;
  QString currentText;
  bool breakUndoCombo;
  //QRect currentTextRect;
  //QRect currentVertexRect;
  //int currentTick;
  //int currentYNorm;
};

//---------------------------------------------------------
//   PartCanvas
//---------------------------------------------------------

enum PartOperations {
  OP_RENAME = 0,
  OP_DELETE = 1,
  OP_SPLIT = 2,
  OP_GLUE = 3,
  OP_CUT = 4,
  OP_COPY = 5,
  OP_GLUESELECTION = 6,

  OP_WAVEEDIT = 14,
  OP_DECLONE = 15,
  OP_SAVEPARTTODISK = 16,
  OP_FILEINFO = 17,
  OP_SELECT_CLONES = 18,
  OP_NORMALIZE = 19,
  OP_PARTCOLORBASE = 20,
};

class PartCanvas : public Canvas {
      Q_OBJECT
      int* _raster;
      MusECore::TrackList* tracks;

      QLineEdit* lineEditor;
      NPart* editPart;
      int curColorIndex;
      bool editMode;
      
      static const int _automationPointDetectDist;
      static const int _automationPointWidthUnsel;
      static const int _automationPointWidthSel;
      
      QElapsedTimer editingFinishedTime;

      AutomationObject automation;

      void updateSelectedItem(CItem* newItem, bool add, bool singleSelection);
      virtual void keyPress(QKeyEvent*);
      virtual void keyRelease(QKeyEvent* event);
      virtual bool mousePress(QMouseEvent*);
      virtual void mouseMove(QMouseEvent* event);
      virtual void mouseRelease(QMouseEvent*);
      virtual void viewMouseDoubleClickEvent(QMouseEvent*);
      virtual void leaveEvent(QEvent*e);
      virtual void drawItem(QPainter&, const CItem*, const QRect&, const QRegion& = QRegion());
      virtual void drawMoving(QPainter&, const CItem*, const QRect&, const QRegion& = QRegion());
      virtual bool itemSelectionsChanged(MusECore::Undo* operations = 0, bool deselectAll = false);
      virtual QPoint raster(const QPoint&) const;
      virtual int y2pitch(int y) const;
      virtual int pitch2y(int p) const;
      virtual int y2height(int) const; 
      virtual inline int yItemOffset() const { return 0; }
      
      virtual CItem* newItem(const QPoint&, int);
      virtual void resizeItem(CItem*,bool, bool ctrl);
      virtual void newItem(CItem*,bool);
      virtual bool deleteItem(CItem*);
      virtual void renameItem(CItem*);
      virtual void moveCanvasItems(CItemMap&, int, int, DragType, bool rasterize = true);
      virtual bool moveItem(MusECore::Undo& operations, CItem*, const QPoint&, DragType);

      virtual void startDrag(CItem*, DragType);
      virtual void dragEnterEvent(QDragEnterEvent*);
      virtual void viewDropEvent(QDropEvent*);

      virtual QMenu* genItemPopup(CItem*);
      virtual void itemPopup(CItem*, int, const QPoint&);

      void glueItem(CItem* item);
      void splitItem(CItem* item, const QPoint&);

      void copy(MusECore::PartList*);
      void copy_in_range(MusECore::PartList*);
      enum paste_mode_t { PASTEMODE_MIX, PASTEMODE_MOVEALL, PASTEMODE_MOVESOME };
      void paste(bool clone = false, paste_mode_t paste_mode = PASTEMODE_MIX,
                 bool to_single_track=false, int amount=1, int raster=1536);
      MusECore::Undo pasteAt(const QString&, MusECore::Track*, unsigned int, bool clone = false,
                             bool toTrack = true, unsigned int* finalPosPtr = NULL,
                             std::set<MusECore::Track*>* affected_tracks = NULL);
      void drawWaveSndFile(QPainter &p, MusECore::SndFileR &f, int samplePos, unsigned rootFrame,
                           unsigned startFrame, unsigned lengthFrames, int startY, int startX, int endX, int rectHeight);
      void drawWavePart(QPainter&, const QRect&, MusECore::WavePart*, const QRect&);
      void drawMidiPart(QPainter&, const QRect& rect, const MusECore::EventList& events,
                        MusECore::MidiTrack* mt, MusECore::MidiPart* midipart,
                        const QRect& r, int pTick, int from, int to, bool selected);
	    void drawMidiPart(QPainter&, const QRect& rect, MusECore::MidiPart* midipart,
                        const QRect& r, int from, int to, bool selected);
      MusECore::Track* y2Track(int) const;
      void drawAudioTrack(QPainter& p, const QRect& mr, const QRegion& vrg, const ViewRect& vbbox, MusECore::AudioTrack* track);
      void drawAutomation(QPainter& p, const QRect& r, MusECore::AudioTrack* track);
      void drawAutomationPoints(QPainter& p, const QRect& r, MusECore::AudioTrack* track);
      void drawAutomationText(QPainter& p, const QRect& r, MusECore::AudioTrack* track);
      void drawTopItem(QPainter& p, const QRect& rect, const QRegion& = QRegion());

      void checkAutomation(MusECore::Track * t, const QPoint& pointer, bool addNewCtrl);
      void processAutomationMovements(QPoint pos, bool slowMotion);
      double logToVal(double inLog, double min, double max);
      double valToLog(double inV, double min, double max);
      void newAutomationVertex(QPoint inPos);

   protected:
      virtual void drawCanvas(QPainter&, const QRect&, const QRegion& = QRegion());
      virtual void endMoveItems(const QPoint&, DragType, int dir, bool rasterize = true);

   signals:
      void timeChanged(unsigned);
      void tracklistChanged();
      void dclickPart(MusECore::Track*);
      void dropSongFile(const QString&);
      void dropMidiFile(const QString&);
      void setUsedTool(int);
      void trackChanged(MusECore::Track*);
      void selectTrackAbove();
      void selectTrackBelow();
      void editTrackNameSig();
      void muteSelectedTracks();
      void soloSelectedTracks();

      void startEditor(MusECore::PartList*, int);

   private slots:
      void returnPressed();

   public:
      enum { CMD_CUT_PART, CMD_COPY_PART, CMD_COPY_PART_IN_RANGE, CMD_PASTE_PART, CMD_PASTE_CLONE_PART,
             CMD_PASTE_PART_TO_TRACK, CMD_PASTE_CLONE_PART_TO_TRACK, CMD_PASTE_DIALOG, CMD_INSERT_EMPTYMEAS };

      PartCanvas(int* raster, QWidget* parent, int, int);
      virtual ~PartCanvas();
      void updateItems();
      void cmd(int);
      void songIsClearing();
      
   public slots:
      void redirKeypress(QKeyEvent* e) { keyPress(e); }
      void controllerChanged(MusECore::Track *t, int CtrlId);
};

} // namespace MusEGui

#endif

