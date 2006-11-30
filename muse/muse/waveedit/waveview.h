//=============================================================================
//  MusE
//  Linux Music Editor
//  $Id:$
//
//  Copyright (C) 2002-2006 by Werner Schweer and others
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License version 2.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//=============================================================================

#ifndef WAVE_VIEW_H
#define WAVE_VIEW_H

#include "al/pos.h"
#include "wave.h"
#include "awl/tcanvas.h"

class PartList;
class QPainter;
class QRect;
class Part;
class WaveEdit;
class GraphMidiEditor;

//---------------------------------------------------------
//   WaveEventSelection
//---------------------------------------------------------

struct WaveEventSelection {
      SndFileR file;
      unsigned startframe;
      unsigned endframe;
      };

typedef std::list<WaveEventSelection> WaveSelectionList;
typedef std::list<WaveEventSelection>::iterator iWaveSelection;

//---------------------------------------------------------
//   WaveView
//---------------------------------------------------------

class WaveView : public TimeCanvas {
      WaveEdit* editor;

      int startFrame;
      int endFrame;

      Part* curPart;

      enum { NORMAL, DRAG } mode;
      enum { MUTE = 0, NORMALIZE, FADE_IN, FADE_OUT, REVERSE, GAIN, EDIT_EXTERNAL }; //!< Modify operations

      unsigned selectionStart, selectionStop, dragstartx;

      Q_OBJECT
      virtual void paint(QPainter&, QRect);

//      virtual void pdraw(QPainter&, const QRect&);
//      virtual void draw(QPainter&, const QRect&);

      virtual void viewMousePressEvent(QMouseEvent*);
      virtual void viewMouseMoveEvent(QMouseEvent*);
      virtual void viewMouseReleaseEvent(QMouseEvent*);

      bool getUniqueTmpfileName(QString& newFilename); //!< Generates unique filename for temporary SndFile
      WaveSelectionList getSelection(unsigned startpos, unsigned stoppos);

      int lastGainvalue; //!< Stores the last used gainvalue when specifiying gain value in the editgain dialog
      void modifySelection(int operation, unsigned startpos, unsigned stoppos, double paramA); //!< Modifies selection

      void muteSelection(unsigned channels, float** data, unsigned length); //!< Mutes selection
      void normalizeSelection(unsigned channels, float** data, unsigned length); //!< Normalizes selection
      void fadeInSelection(unsigned channels, float** data, unsigned length); //!< Linear fade in of selection
      void fadeOutSelection(unsigned channels, float** data, unsigned length); //!< Linear fade out of selection
      void reverseSelection(unsigned channels, float** data, unsigned length); //!< Reverse selection
      void applyGain(unsigned channels, float** data, unsigned length, double gain); //!< Apply gain to selection

      void editExternal(unsigned file_format, unsigned file_samplerate, unsigned channels, float** data, unsigned length);

      //void applyLadspa(unsigned channels, float** data, unsigned length); //!< Apply LADSPA plugin on selection

      void drawWavePart(QPainter& p, Part* wp, int y0, int th, int from, int to);

   public slots:
      void songChanged(int type);

   public:
      WaveView(WaveEdit*);
      QString getCaption() const;
      void cmd(const QString&);
      void range(AL::Pos&, AL::Pos&) const;
      };

#endif

