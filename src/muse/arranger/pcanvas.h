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

#include <set>
#include <list>
#include <map>
#include <QElapsedTimer>
#include <QList>
#include <QMap>
#include <QUuid>

#include "type_defs.h"
#include "canvas.h"
#include "undo.h"
#include "track.h"
#include "ctrl.h"
#include "citem.h"
#include "pos.h"


// Forward declarations:
class QDropEvent;
class QMouseEvent;
class QKeyEvent;
class QEvent;
class QDragEnterEvent;
class QLineEdit;
class QMenu;
class QPainter;

namespace MusECore {
class Xml;
class Track;
class MidiTrack;
class AudioTrack;
class Part;
class WavePart;
class MidiPart;
class PartList;
}

namespace MusEGui {

//---------------------------------------------------------
//   NPart
//    ''visual'' Part
//    wraps Parts with additional information needed
//    for displaying
//---------------------------------------------------------

class NPart : public PItem {
   protected:
      QUuid _serial;
   
   public:
      NPart(MusECore::Part*);
      NPart() { }
      const QString name() const     { return part()->name(); }
      void setName(const QString& s) { part()->setName(s); }
      MusECore::Track* track() const           { return part()->track(); }
      QUuid serial() { return _serial; }
      
      bool leftBorderTouches;  // Whether the borders touch other part borders. 
      bool rightBorderTouches;
      };

enum ControllerVals { doNothing, addNewController };

struct AutomationObject {
  // List of controller items that are SELECTED only.
  MusECore::AudioAutomationItemTrackMap currentCtrlFrameList;
  unsigned int currentFrame;
  double currentVal;
  // Whether the currentFrame and currentVal are valid.
  bool currentCtrlValid;
  // Current controller list. May be null.
  MusECore::CtrlList *currentCtrlList;
  // Current track. May be null.
  MusECore::Track *currentTrack;
  ControllerVals controllerState;
  QString currentText;
  bool breakUndoCombo;

  AutomationObject();
  void clear();
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

enum AudioAutomationOperations {
  // Canvas::TOOLS_ID_BASE is 10000 (but it's private), so start this at 20000.
  AUTO_OP_BASE_ENUM = 20000,
  AUTO_OP_REMOVE = AUTO_OP_BASE_ENUM,
  AUTO_OP_NO_ERASE_MODE = AUTO_OP_BASE_ENUM + 1,
  AUTO_OP_ERASE_MODE = AUTO_OP_BASE_ENUM + 2,
  AUTO_OP_ERASE_RANGE_MODE = AUTO_OP_BASE_ENUM + 3,
  AUTO_OP_END_MOVE_MODE = AUTO_OP_BASE_ENUM + 4,
  AUTO_OP_END_ENUM = AUTO_OP_END_MOVE_MODE
};

class PartCanvas : public Canvas {
      Q_OBJECT
      int* _raster;
      MusECore::TrackList* tracks;

      QLineEdit* lineEditor;
      NPart* editPart;
      int curColorIndex;
      bool editMode;
      
      static const int _automationTopMargin;
      static const int _automationBottomMargin;
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
      virtual void moveCanvasItems(CItemMap&, int, int, DragType, MusECore::Undo&, bool rasterize = true);

      virtual void startDrag(CItem*, DragType);
      virtual void dragEnterEvent(QDragEnterEvent*);
      virtual void viewDropEvent(QDropEvent*);

      virtual QMenu* genItemPopup(CItem*);
      virtual void itemPopup(CItem*, int, const QPoint&);

      virtual QMenu* genAutomationPopup(QMenu* menu = nullptr);
      virtual void automationPopup(int);

      void glueItem(CItem* item);
      void splitItem(CItem* item, const QPoint&);

      void copy(MusECore::PartList*);
      void copy_in_range(MusECore::PartList*);
      // Copies audio automation controller graphs to the given xml object at the given level.
      // If useRange is false, uses only selected items from all tracks.
      // If useRange is true, all items from position p0 to p1 are used regardless of selection state,
      //  and if useAllTracks is true, uses all tracks, otherwise if specificTrack is valid, uses only
      //  that track, otherwise uses only selected tracks.
      // Returns true if anything was written to the xml.
      bool copyAudioAutomation(
        int level, MusECore::Xml& xml,
        bool useAllTracks,
        bool useRange,
        const MusECore::Track* specificTrack = nullptr,
        const MusECore::Pos& p0 = MusECore::Pos(), const MusECore::Pos& p1 = MusECore::Pos());

      enum paste_mode_t { PASTEMODE_MIX, PASTEMODE_MOVEALL, PASTEMODE_MOVESOME };

      void paste(bool clone = false, paste_mode_t paste_mode = PASTEMODE_MIX,
                 bool to_single_track=false, int amount=1, int raster=1536);
      void pasteAt(MusECore::Undo& operations, const QString&, MusECore::Track*, unsigned int, bool clone = false,
                             bool toTrack = true, unsigned int* finalPosPtr = nullptr,
                             std::set<MusECore::Track*>* affected_tracks = nullptr);
      void drawWaveSndFile(QPainter &p, MusECore::SndFileR &f, int samplePos, unsigned rootFrame,
                           unsigned startFrame, unsigned lengthFrames, int startY, int startX, int endX, int rectHeight, bool selected);
      void drawWavePart(QPainter&, const QRect&, MusECore::WavePart*, const QRect&, bool selected);
      void drawMidiPart(QPainter&, const QRect& rect, const MusECore::EventList& events,
                        MusECore::MidiTrack* mt, MusECore::MidiPart* midipart,
                        const QRect& r, int pTick, int from, int to, bool selected);
	    void drawMidiPart(QPainter&, const QRect& rect, MusECore::MidiPart* midipart,
                        const QRect& r, int from, int to, bool selected);
      MusECore::Track* y2Track(int) const;
      void drawAudioTrack(QPainter& p, const QRect& mr, const QRegion& vrg, const ViewRect& vbbox, MusECore::AudioTrack* track);
      void drawAutomationFills(QPainter& p, const QRect& r, MusECore::AudioTrack* track);
      void drawAutomation(QPainter& p, const QRect& r, MusECore::AudioTrack* track);
      void drawAutomationPoints(QPainter& p, const QRect& r, MusECore::AudioTrack* track);
      // Returns false if the point is beyond rr.right(), true otherwise.
      // currentFrame and newFrame can be different in the case of moving points. Otherwise they are the same.
      bool drawAutomationPoint(
        QPainter& p, const QRect& rr, const QPen& currentPen, const QPen& nonCurrentPen, int pointWidth,
        const MusECore::AudioTrack* t, const MusECore::CtrlList* cl, unsigned int currentFrame, unsigned int newFrame, double value);
      // Returns false if the point is beyond rr.right(), true otherwise.
      // currentFrame and newFrame can be different in the case of moving points. Otherwise they are the same.
      bool fillAutomationPoint(
        QPainter& p, const QRect& rr, const QColor& currentColor, const QColor& nonCurrentColor, int pointWidth,
        const MusECore::AudioTrack* t, const MusECore::CtrlList* cl, unsigned int currentFrame, unsigned int newFrame, double value);
      void drawAutomationText(QPainter& p, const QRect& r, MusECore::AudioTrack* track);
      void drawTopItem(QPainter& p, const QRect& rect, const QRegion& = QRegion());

      void checkAutomation(const QPoint& pointer);
      //    dir = 0     move in all directions
      //          1     move only horizontal
      //          2     move only vertical
      void processAutomationMovements(QPoint pos, int dir, bool rasterize);
      // Uses fast_log10() instead of log10().
      double logToVal(double inLog, double min, double max);
      // Uses fast_log10() instead of log10().
      double valToLog(double inV, double min, double max);
      // Uses log10() instead of fast_log10().
      double deltaValToLog(double inLog, double inLinDeltaNormalized, double min, double max);
      // Given a frame, finds the corresponding item and returns its value, and returns
      //  the minimum and maximum frame that the given frame can be moved to.
      // Returns true on success, false if no item was found for the given frame.
      bool getMovementRange(
        const MusECore::CtrlList*, unsigned int frame, double* value, unsigned int* minPrevFrame,
        unsigned int* maxNextFrame, bool* maxNextFrameValid);
      void unselectAllAutomation(MusECore::Undo& undo);
      void deleteSelectedAutomation(MusECore::Undo& undo);
      // Returns true if successful.
      bool newAutomationVertex(QPoint inPos, MusECore::Undo& undo, bool snap = false);
      // The isCopy argument means whether to copy the items or just move them.
      // Returns true is anything was added to undo. False if not or error.
      bool commitAutomationChanges(MusECore::Undo& undo, bool isCopy);
      void showStatusTip(QMouseEvent *event);


   protected:
      virtual void drawCanvas(QPainter&, const QRect&, const QRegion& = QRegion());
      virtual void endMoveItems(const QPoint&, DragType, int dir, bool rasterize = true);

      void startMoving(const QPoint&, int dir, DragType, bool rasterize = true);
      //    dir = 0     move in all directions
      //          1     move only horizontal
      //          2     move only vertical
      void moveItems(const QPoint&, int dir = 0, bool rasterize = true);
      // Returns true if anything was selected.
      bool selectLasso(bool toggle, MusECore::Undo* undo = nullptr);
      void deselectAll(MusECore::Undo* undo = nullptr);
      void setCursor();

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
      void volumeSelectedTracks(int);
      void panSelectedTracks(int);
      void curPartColorIndexChanged(int);

      void startEditor(MusECore::PartList*, int);

   private slots:
      void returnPressed();

   public:
      enum { CMD_DELETE,
             CMD_CUT_PART, CMD_COPY_PART, CMD_COPY_PART_IN_RANGE, CMD_PASTE_PART, CMD_PASTE_CLONE_PART,
             CMD_PASTE_PART_TO_TRACK, CMD_PASTE_CLONE_PART_TO_TRACK, CMD_PASTE_DIALOG, CMD_INSERT_EMPTYMEAS };

      PartCanvas(int* raster, QWidget* parent, int, int);
      virtual ~PartCanvas();
      void updateItems();
      void updateAudioAutomation();
      void cmd(int);
      void songIsClearing();
      void setRangeToSelection();
      int currentPartColorIndex() const;
      bool isSingleAudioAutomationSelection() const;
      int audioAutomationSelectionSize() const;
      bool audioAutomationItemsAreSelected() const;
      // Appends given tag list with item objects according to options. Avoids duplicate events or clone events.
      // Special: We 'abuse' a controller event's length, normally 0, to indicate visual item length.
      void tagItems(MusECore::TagEventList* tag_list, const MusECore::EventTagOptionsStruct& options) const;

   public slots:
      void redirKeypress(QKeyEvent* e) { keyPress(e); }
      // Redraws the track. CtrlId (unused so far) would be ignored if -1.
      void controllerChanged(const MusECore::Track *t, int CtrlId = -1);
      void setPartColor(int idx);
      void setCurrentColorIndex(int idx);
};

} // namespace MusEGui

#endif

