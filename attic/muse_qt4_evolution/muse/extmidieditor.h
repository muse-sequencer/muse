//=============================================================================
//  MusE
//  Linux Music Editor
//  $Id:$
//
//  Copyright (C) 2004 Mathias Lundgren <lunar_shuttle@users.sourceforge.net>
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

#ifndef __EXTMIDIEDITOR_H__
#define __EXTMIDIEDITOR_H__

#include "midiedit/midieditor.h"
#include "esettings.h"

namespace AL {
      class Pos;
      };

class EditToolBar;
class NoteInfo;
class PartList;
class CtrlEdit;
class Toolbar1;
class Part;

//---------------------------------------------------------
//   GraphMidiEditor
//
// Base class for graphical editors
// Currently used by WaveEdit and MasterEdit
//---------------------------------------------------------

class GraphMidiEditor : public MidiEditor {
      Q_OBJECT

   protected:
      virtual EditorSettings* newDefaultSettings(); //TODO: = 0
      virtual void initSettings();
      virtual void refreshSettings() const;
      QAction* srec;
      QAction* midiin;
      bool _followSong;
      EditToolBar* tools2;

   protected slots:
      void setRaster(int val);

   public:
      GraphMidiEditor(PartList*);
      virtual ~GraphMidiEditor();
      int rasterStep(unsigned tick) const;
      unsigned rasterVal(unsigned v)  const;
      unsigned rasterVal1(unsigned v) const;
      unsigned rasterVal2(unsigned v) const;
      int raster() const;
      bool followSong() const { return _followSong; }
      };


//---------------------------------------------------------
//   ExtMidiEditor
//
// Inherited by Drumeditor and Pianoroll
// Not instantiated directly - virtual
//---------------------------------------------------------

class ExtMidiEditor : public GraphMidiEditor   {
      Q_OBJECT
      int _curDrumInstrument;  // currently selected instrument if drum
                               // editor

   protected:
      std::list<CtrlEdit*> ctrlEditList;
      virtual EditorSettings* newDefaultSettings();
      virtual void initSettings();
      virtual void refreshSettings() const;
      Toolbar1* toolbar;
      Part* selPart;
      NoteInfo* info;
      QToolBar* tools;
      EventCanvas* canvas;
      WaveView* wview;

      void updateCtrlEdits() const;

   signals:
      void curDrumInstrumentChanged(int); //hACK!

   protected slots:
      void setQuant(int val);
      void follow(int pos);
      void removeCtrl(CtrlEdit* ctrl);
      virtual CtrlEdit* addCtrl();

   public slots:
      void setCurDrumInstrument(int instr); //hACK!

   public:
      ExtMidiEditor(PartList*);
      virtual ~ExtMidiEditor();
      int quantVal(int v) const;
      int quant() const;

      int curDrumInstrument() const  { return _curDrumInstrument; }
      };

#endif

