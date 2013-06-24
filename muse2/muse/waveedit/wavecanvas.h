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

#include "xtick.h"
#include "type_defs.h"
#include "ecanvas.h"
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMouseEvent>
#include <QDragMoveEvent>
#include <QDragLeaveEvent>
#include <QWheelEvent>
#include <QResizeEvent>
#include <QTimer>

class QRect;

namespace MusECore {
class WavePart;
class WaveTrack;

struct WaveEventSelection {
      Event event;         
      unsigned startframe; // that's the frame in the raw file. inclusive
      unsigned endframe; // exclusive
      };

typedef std::list<WaveEventSelection> WaveSelectionList;
typedef std::list<WaveEventSelection>::iterator iWaveSelection;
}

namespace MusEGui {
using MusECore::XTick;

//---------------------------------------------------------
//   WEvent
//    ''visual'' Wave Event
//---------------------------------------------------------

class WEvent : public CItem {
   public:
      WEvent(MusECore::Event& e, MusECore::Part* p, int height);
      };

//---------------------------------------------------------
//   WaveCanvas
//---------------------------------------------------------

class WaveCanvas : public EventCanvas {
      Q_OBJECT
    
      enum { NORMAL, DRAG } mode;
      enum { MUTE = 0, NORMALIZE, FADE_IN, FADE_OUT, REVERSE, GAIN, EDIT_EXTERNAL, CUT, COPY, PASTE }; //!< Modify operations
      
      int yScale;
      int button;
      unsigned startSample;
      unsigned endSample;
      int colorMode;
      XTick selectionStart, selectionStop, dragstartx;
      int lastGainvalue; //!< Stores the last used gainvalue when specifiying gain value in the editgain dialog
      QString copiedPart;
      
      //bool getUniqueTmpfileName(QString& newFilename); //!< Generates unique filename for temporary SndFile
      MusECore::WaveSelectionList getSelection(XTick startpos, XTick stoppos);
      void modifySelection(int operation, XTick startpos, XTick stoppos, double paramA); //!< Modifies selection
      void muteSelection(unsigned channels, float** data, unsigned length); //!< Mutes selection
      void normalizeSelection(unsigned channels, float** data, unsigned length); //!< Normalizes selection
      void fadeInSelection(unsigned channels, float** data, unsigned length); //!< Linear fade in of selection
      void fadeOutSelection(unsigned channels, float** data, unsigned length); //!< Linear fade out of selection
      void reverseSelection(unsigned channels, float** data, unsigned length); //!< Reverse selection
      void applyGain(unsigned channels, float** data, unsigned length, double gain); //!< Apply gain to selection
      void copySelection(unsigned file_channels, float** tmpdata, unsigned tmpdatalen, bool blankData, unsigned format, unsigned sampleRate);
      void editExternal(unsigned file_format, unsigned file_samplerate, unsigned channels, float** data, unsigned length);
      //void applyLadspa(unsigned channels, float** data, unsigned length); //!< Apply LADSPA plugin on selection

      
   protected:
      virtual QPoint raster(const QPoint&) const;
      void drawTickRaster(QPainter& p, int x, int y, int w, int h, int raster);
      void drawParts(QPainter&, const QRect&, bool do_cur_part);
  
      virtual void draw(QPainter&, const QRect&);
      virtual void viewMouseDoubleClickEvent(QMouseEvent*);
      virtual void wheelEvent(QWheelEvent*);
      virtual bool mousePress(QMouseEvent*);
      virtual void mouseMove(QMouseEvent* event);
      virtual void mouseRelease(const QPoint&);
      virtual void drawItem(QPainter&, const CItem*, const QRect&);
      void drawTopItem(QPainter &p, const QRect &rect);
      virtual void drawMoving(QPainter&, const CItem*, const QRect&);
      virtual MusECore::Undo moveCanvasItems(CItemList&, int, int, DragType, bool rasterize = true);
      virtual bool moveItem(MusECore::Undo&, CItem*, const QPoint&, DragType, bool rasterize = true);
      virtual CItem* newItem(const QPoint&, int);
      virtual void resizeItem(CItem*, bool noSnap, bool);
      virtual void newItem(CItem*, bool noSnap);
      virtual bool deleteItem(CItem*);
      virtual void startDrag(CItem* item, bool copymode);
      virtual void dragEnterEvent(QDragEnterEvent* event);
      virtual void dragMoveEvent(QDragMoveEvent*);
      virtual void dragLeaveEvent(QDragLeaveEvent*);
      virtual CItem* addItem(MusECore::Part*, MusECore::Event&);

      int y2pitch(int) const;
      int pitch2y(int) const;
      inline int y2height(int) const { return height(); }
      inline int yItemOffset() const { return 0; }
      virtual void drawCanvas(QPainter&, const QRect&);
      virtual void itemPressed(const CItem*);
      virtual void itemReleased(const CItem*, const QPoint&);
      virtual void itemMoved(const CItem*, const QPoint&);
      virtual void curPartChanged();
      virtual void resizeEvent(QResizeEvent*);
      void adjustWaveOffset(); 
      
   private slots:
      void setPos(int idx, unsigned val, bool adjustScrollbar);

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
             CMD_SELECT_ILOOP, CMD_SELECT_OLOOP, CMD_SELECT_PREV_PART, CMD_SELECT_NEXT_PART, 
             CMD_ERASE_MEASURE, CMD_DELETE_MEASURE, CMD_CREATE_MEASURE,
             CMD_ADJUST_WAVE_OFFSET
           };
             
      WaveCanvas(MidiEditor*, QWidget*, int, int);
      virtual ~WaveCanvas();
      MusECore::WaveTrack* track() const;
      void cmd(int cmd);
      void setColorMode(int mode) {
            colorMode = mode;
            redraw();
            }
      QString getCaption() const;
      void songChanged(MusECore::SongChangedFlags_t);
      void range(int* s, int* e) const { *s = startSample; *e = endSample; }
      void selectAtTick(unsigned int tick);
      void selectAtFrame(unsigned int frame);
      void modifySelected(NoteInfo::ValType type, int val, bool delta_mode = true);
      void keyPress(QKeyEvent*);
      };

} // namespace MusEGui

#endif

