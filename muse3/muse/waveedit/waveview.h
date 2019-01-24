//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: waveview.h,v 1.3.2.6 2009/02/02 21:38:01 terminator356 Exp $
//  (C) Copyright 2000 Werner Schweer (ws@seh.de)
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

#ifndef WAVE_VIEW_H
#define WAVE_VIEW_H

#include "type_defs.h"
#include "view.h"
#include <QWidget>
#include <QMouseEvent>
#include "wave.h"

class QPainter;
class QRect;

namespace MusECore {
class PartList;
class SndFileR;
class WavePart;

struct WaveEventSelection {
      SndFileR file;
      unsigned startframe;
      unsigned endframe;
      };

typedef std::list<WaveEventSelection> WaveSelectionList;
typedef std::list<WaveEventSelection>::iterator iWaveSelection;

} // namespace MusECore

namespace MusEGui {

class MidiEditor;

//---------------------------------------------------------
//   WaveView
//---------------------------------------------------------

class WaveView : public View {
      Q_OBJECT
    
      MidiEditor* editor;
      unsigned pos[3];
      int yScale;
      int button;
      int startSample;
      int endSample;

      MusECore::WavePart* curPart;
      QString copiedPart;
      int curPartId;

      enum { NORMAL, DRAG } mode;
      enum { MUTE = 0, NORMALIZE, FADE_IN, FADE_OUT, REVERSE, GAIN, EDIT_EXTERNAL, CUT, COPY, PASTE }; //!< Modify operations

      unsigned selectionStart, selectionStop, dragstartx;

      virtual void pdraw(QPainter&, const QRect&, const QRegion& = QRegion());
      virtual void draw(QPainter&, const QRect&, const QRegion& = QRegion());
      virtual void viewMousePressEvent(QMouseEvent*);
      virtual void viewMouseMoveEvent(QMouseEvent*);
      virtual void viewMouseReleaseEvent(QMouseEvent*);
      virtual void wheelEvent(QWheelEvent*);

      //bool getUniqueTmpfileName(QString& newFilename); //!< Generates unique filename for temporary SndFile
      MusECore::WaveSelectionList getSelection(unsigned startpos, unsigned stoppos);

      int lastGainvalue; //!< Stores the last used gainvalue when specifying gain value in the editgain dialog
   private slots:
      void setPos(int idx, unsigned val, bool adjustScrollbar);

   public slots:
      void setYScale(int);
      void songChanged(MusECore::SongChangedStruct_t type);

   signals:
      void followEvent(int);
      void timeChanged(unsigned);
      void mouseWheelMoved(int);
      void horizontalScroll(unsigned);
      void horizontalZoomIn();
      void horizontalZoomOut();

   public:
      WaveView(MidiEditor*, QWidget* parent, int xscale, int yscale);
      QString getCaption() const;
      void range(int*, int*);
      MusECore::WavePart* part() { return curPart; }
      };

} // namespace MusEGui

#endif

