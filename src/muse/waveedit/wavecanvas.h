//=========================================================
//  MusE
//  Linux Music Editor
//    wavecanvas.h
//  (C) Copyright 2012 Tim E. Real (terminator356 on users dot sourceforge dot net)
//
//  Based on WaveView.cpp and PianoCanvas.cpp
//  (C) Copyright 2000 Werner Schweer (ws@seh.de)
//   and others.
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

#ifndef __WAVECANVAS_H__
#define __WAVECANVAS_H__

#include "type_defs.h"
#include "ecanvas.h"
#include <QList>
#include <QRect>
#include <QMenu>
#include <QPoint>

#include <map>
#include "muse_time.h"
#include "time_stretch.h"
#include "wave.h"


// Forward declarations:
class QDragEnterEvent;
class QDropEvent;
class QMouseEvent;
class QDragMoveEvent;
class QDragLeaveEvent;
class QWheelEvent;
class QResizeEvent;
class QPainter;

namespace MusECore {
class WavePart;
class WaveTrack;

struct WaveEventSelection {
      Event event;         
      unsigned startframe;
      unsigned endframe;
      };

typedef std::list<WaveEventSelection> WaveSelectionList;
typedef std::list<WaveEventSelection>::iterator iWaveSelection;
}

namespace MusEGui {

//---------------------------------------------------------
//   WEvent
//    ''visual'' Wave Event
//---------------------------------------------------------

class WEvent : public EItem {
   public:
      WEvent(const MusECore::Event& e, MusECore::Part* p, int height);

// REMOVE Tim. wave. Added.
      // ctrl is used for 'drag events with border' for example.
      // left is whether we are resizing the right or left border.
//       void horizResize(int newPos, bool left) override;
// REMOVE Tim. wave. Added.
//       void initItemTempValues();
      };

      
struct StretchSelectedItem
{
  MusECore::StretchListItem::StretchEventType _type;
  MusECore::SndFileR _sndFile;
  
// REMOVE Tim. wave. Changed.
//   StretchSelectedItem(MusECore::StretchListItem::StretchEventType type,
//                       MusECore::SndFileR sndFile = MusECore::SndFileR())
//   {
//     _type = type;
//     _sndFile = sndFile;
//   }
  StretchSelectedItem(MusECore::StretchListItem::StretchEventType type,
                      MusECore::SndFileR sndFile = MusECore::SndFileR());
};

typedef std::multimap<MusECore::MuseFrame_t, StretchSelectedItem, std::less<MusECore::MuseFrame_t> > StretchSelectedList_t;
typedef StretchSelectedList_t::iterator iStretchSelectedItem;
typedef StretchSelectedList_t::const_iterator ciStretchSelectedItem;
typedef StretchSelectedList_t::reverse_iterator riStretchSelectedItem;
typedef StretchSelectedList_t::const_reverse_iterator criStretchSelectedItem;

typedef std::pair<iStretchSelectedItem, iStretchSelectedItem> iStretchSelectedItemPair;
typedef std::pair<ciStretchSelectedItem, ciStretchSelectedItem> ciStretchSelectedItemPair;

typedef std::pair<MusECore::MuseFrame_t, StretchSelectedItem> StretchSelectedItemInsertPair_t;

enum StretchControllerVals { stretchDoNothing, stretchStartMove, stretchMovingController, stretchAddNewController };
struct StretchAutomationObject {
  StretchSelectedList_t _stretchSelectedList;
  StretchControllerVals _controllerState;
  bool _moveController;
  QPoint _startMovePoint;
  
  
  
//   QPoint startMovePoint;
//   QList<MusECore::MuseFrame_t> currentCtrlFrameList;
//   bool currentCtrlValid;
//   //MusECore::CtrlList *currentCtrlList;
//   MusECore::StretchList* currentCtrlList;
//   MusECore::Track *currentTrack;
//   bool moveController;
//   StretchControllerVals controllerState;
//   QString currentText;
//   bool breakUndoCombo;
  //QRect currentTextRect;
  //QRect currentVertexRect;
  //int currentTick;
  //int currentYNorm;
  
  StretchAutomationObject()
  {
    _controllerState = stretchDoNothing;
    _moveController = false;
  }
};

//---------------------------------------------------------
//   WaveCanvas
//---------------------------------------------------------

class WaveCanvas : public EventCanvas {
      Q_OBJECT
    
      enum { NORMAL, DRAG } mode;
      enum { MUTE = 0, NORMALIZE, FADE_IN, FADE_OUT, REVERSE, GAIN, EDIT_EXTERNAL, CUT, COPY, PASTE }; //!< Modify operations
      
      static const int _stretchAutomationPointDetectDist;
      static const int _stretchAutomationPointWidthUnsel;
      static const int _stretchAutomationPointWidthSel;
      
      int yScale;
      int button;
      unsigned startSample;
      unsigned endSample;
      int colorMode;
      int selectionStart, selectionStop, dragstartx;
      int lastGainvalue; //!< Stores the last used gainvalue when specifying gain value in the editgain dialog
      QString copiedPart;

      StretchAutomationObject _stretchAutomation;
      
      //bool getUniqueTmpfileName(QString& newFilename); //!< Generates unique filename for temporary SndFile
      MusECore::WaveSelectionList getSelection(unsigned startpos, unsigned stoppos);
      void modifySelection(int operation, unsigned startpos, unsigned stoppos, double paramA); //!< Modifies selection
      void muteSelection(unsigned channels, float** data, unsigned length); //!< Mutes selection
      void normalizeSelection(unsigned channels, float** data, unsigned length); //!< Normalizes selection
      void fadeInSelection(unsigned channels, float** data, unsigned length); //!< Linear fade in of selection
      void fadeOutSelection(unsigned channels, float** data, unsigned length); //!< Linear fade out of selection
      void reverseSelection(unsigned channels, float** data, unsigned length); //!< Reverse selection
      void applyGain(unsigned channels, float** data, unsigned length, double gain); //!< Apply gain to selection
      void copySelection(unsigned file_channels, float** tmpdata, unsigned tmpdatalen, bool blankData, unsigned format, unsigned sampleRate);
      void editExternal(unsigned file_format, unsigned file_samplerate, unsigned channels, float** data, unsigned length);
      //void applyLadspa(unsigned channels, float** data, unsigned length); //!< Apply LADSPA plugin on selection

      void drawStretchAutomation(QPainter& p, const QRect& r, WEvent* wevent) const;
      MusECore::iStretchListItem stretchListHitTest(int types, QPoint pt, WEvent* wevent);
      void setStretchAutomationCursor(QPoint pt);
      void setRangeToSelection() override;

   protected:
      virtual QPoint raster(const QPoint&) const override;
      void drawParts(QPainter&, bool /*do_cur_part*/, const QRect&, const QRegion& = QRegion()) override;
  
      virtual void viewMouseDoubleClickEvent(QMouseEvent*) override;
      virtual void wheelEvent(QWheelEvent*) override;
      virtual bool mousePress(QMouseEvent*) override;
      virtual void mouseMove(QMouseEvent* event) override;
      virtual void mouseRelease(QMouseEvent*) override;
      virtual void drawItem(QPainter&, const CItem*, const QRect&, const QRegion& = QRegion()) override;
      void drawMarkers(QPainter& p, const QRect& mr, const QRegion& mrg = QRegion()) override;
      
      void drawTopItem(QPainter& p, const QRect& rect, const QRegion& = QRegion()) override;
      virtual void drawMoving(QPainter&, const CItem*, const QRect&, const QRegion& = QRegion()) override;
      virtual MusECore::Undo moveCanvasItems(CItemMap&, int, int, DragType, bool rasterize = true) override;
      virtual bool moveItem(MusECore::Undo&, CItem*, const QPoint&, DragType, bool rasterize = true) override;
      virtual CItem* newItem(const QPoint&, int) override;
      // REMOVE Tim. wave. Added.
//       virtual void adjustItemSize(CItem* item, int pos, bool left, bool noSnap=false, bool ctrl=false);
      // REMOVE Tim. wave. Added.
//       virtual void initItemTempValues(CItem*);
      // REMOVE Tim. wave. Added.
      //virtual void adjustItemTempValues(CItem*, int pos, bool noSnap, bool ctrl, bool alt);
      virtual void adjustItemSize(CItem* item, int pos, bool left, bool noSnap=false, bool ctrl=false, bool alt = false, QRegion * = nullptr) override;
      virtual void resizeItem(CItem*, bool noSnap, bool ctrl) override;
      virtual void newItem(CItem*, bool noSnap) override;
      virtual bool deleteItem(CItem*) override;
      virtual void startDrag(CItem* item, DragType) override;
      virtual void dragEnterEvent(QDragEnterEvent* event) override;
      virtual void dragMoveEvent(QDragMoveEvent*) override;
      virtual void dragLeaveEvent(QDragLeaveEvent*) override;
      virtual CItem* addItem(MusECore::Part*, const MusECore::Event&) override;

      int y2pitch(int) const override;
      int pitch2y(int) const override;
      inline int y2height(int) const override { return height(); }
      inline int yItemOffset() const override { return 0; }
      virtual void drawCanvas(QPainter&, const QRect&, const QRegion& = QRegion()) override;
      virtual void curPartChanged() override;
      virtual void resizeEvent(QResizeEvent*) override;
      void adjustWaveOffset(); 
      
      virtual QMenu* genItemPopup(CItem*) override;
      virtual void itemPopup(CItem*, int, const QPoint&) override;
      
   private slots:
      void setPos(int idx, unsigned val, bool adjustScrollbar) override;

   signals:
      void quantChanged(int);
      void rasterChanged(int);
      void newWidth(int);
      void mouseWheelMoved(int);

   public slots:
      void setYScale(int);
      void waveCmd(int);

   public:
     
      enum { CMD_MUTE=0, CMD_NORMALIZE, CMD_FADE_IN, CMD_FADE_OUT, CMD_REVERSE,
             CMD_GAIN_FREE, CMD_GAIN_200, CMD_GAIN_150, CMD_GAIN_75, CMD_GAIN_50, CMD_GAIN_25,
             CMD_EDIT_COPY, CMD_EDIT_CUT, CMD_EDIT_PASTE, CMD_PASTE_DIALOG, CMD_DEL,
             CMD_CREATE_PART_REGION,
             CMD_EDIT_EXTERNAL,
             CMD_QUANTIZE,
             CMD_SELECT_ALL, CMD_SELECT_NONE, CMD_SELECT_INVERT, 
             CMD_SELECT_ILOOP, CMD_SELECT_OLOOP,
             CMD_SELECT_PREV_PART, CMD_SELECT_NEXT_PART,
             CMD_ERASE_MEASURE, CMD_DELETE_MEASURE, CMD_CREATE_MEASURE,
             CMD_ADJUST_WAVE_OFFSET , CMD_RANGE_TO_SELECTION
           };
             
      WaveCanvas(MidiEditor*, QWidget*, int, int);
      virtual ~WaveCanvas();
      MusECore::WaveTrack* track() const;
      void cmd(int cmd);
// REMOVE Tim. wave. Changed.
//       void setColorMode(int mode) {
//             colorMode = mode;
//             redraw();
//             }
      void setColorMode(int mode);
//      QString getCaption() const;
      void songChanged(MusECore::SongChangedStruct_t) override;
      void range(int* s, int* e) const override { *s = startSample; *e = endSample; }
      void selectAtTick(unsigned int tick) override;
      void selectAtFrame(unsigned int frame);
      void modifySelected(NoteInfo::ValType type, int val, bool delta_mode = true) override;
      void keyPress(QKeyEvent*) override;
      void keyRelease(QKeyEvent* event) override;
      void updateItems() override;

      static int eventBorderWidth;
      };

} // namespace MusEGui

#endif

