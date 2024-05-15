//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: pianoroll.h,v 1.5.2.4 2009/11/16 11:29:33 lunar_shuttle Exp $
//  (C) Copyright 1999 Werner Schweer (ws@seh.de)
//  (C) Copyright 2012-2016 Tim E. Real (terminator356 on users dot sourceforge dot net)
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

#ifndef __PIANOROLL_H__
#define __PIANOROLL_H__

#include <limits.h>
#include "type_defs.h"
#include "globaldefs.h"
#include "part.h"
#include "midieditor.h"
#include "ecanvas.h"
#include "noteinfo.h"
#include "midictrl_consts.h"
#include "scripts.h"

#include <QMetaObject>

// Forward declarations:
class QAction;
class QMenu;
class QToolBar;
class QToolButton;
class QWidget;
class QPoint;
class QCloseEvent;
class QKeyEvent;

namespace MusECore {
class Event;
class Xml;
}

namespace MusEGui {

class TopWin;
class CtrlEdit;
class PitchLabel;
class Splitter;
class Toolbar1;
class Piano;
class PopupMenu;
class EditToolBar;

//---------------------------------------------------------
//   PianoRoll
//---------------------------------------------------------

class PianoRoll : public MidiEditor {
      Q_OBJECT

    
    Q_PROPERTY(int pianoWidth READ pianoWidth WRITE setPianoWidth)

    int _pianoWidth;

      QMenu *menuEdit, *menuFunctions, *menuSelect, *menuConfig, *eventColor, *menuScripts;
      PopupMenu* addControllerMenu;
      
      QAction* editCutAction; 
      QAction* editCopyAction; 
      QAction* editCopyRangeAction; 
      QAction* editPasteAction; 
      QAction* editPasteToCurPartAction;
      QAction* editPasteDialogAction; 
      QAction* editDelEventsAction;

      QAction* editUseLastEditEventAction;

      QAction* selectAllAction; 
      QAction* selectNoneAction;
      QAction* selectInvertAction;
      QAction* selectInsideLoopAction;
      QAction* selectOutsideLoopAction;
      QAction* selectPrevPartAction;
      QAction* selectNextPartAction;
      QAction *startListEditAction;
      QAction* selectRangeToSelectionAction;

      QAction* evColorBlueAction;
      QAction* evColorPitchAction;
      QAction* evColorVelAction;
      
      QAction* funcQuantizeAction;
      QAction* funcGateTimeAction;
      QAction* funcModVelAction;
      QAction* funcCrescAction;
      QAction* funcTransposeAction;
      QAction* funcEraseEventAction;
      QAction* funcNoteShiftAction;
      QAction* funcSetFixedLenAction;
      QAction* funcDelOverlapsAction;
      
      QAction* speakerSingleNote;
      QAction* speakerChords;

      QAction *addControllerAction;

      int tickValue;
      int lenValue;
      int pitchValue;
      int veloOnValue;
      int veloOffValue;
      bool firstValueSet;
      int tickOffset;
      int lenOffset;
      int pitchOffset;
      int veloOnOffset;
      int veloOffOffset;
      bool deltaMode;
      int lastSelections;

      MusEGui::NoteInfo* info;
      QToolButton* addctrl;
      QToolButton* srec;
      QToolButton* midiin;

      Piano* piano;
      MusEGui::Toolbar1* toolbar;
      MusEGui::Splitter* splitter;
      MusEGui::Splitter* hsplitter;
      
      QToolButton* speaker;
      QToolBar* tools;
      MusEGui::EditToolBar* tools2;

      MidiEventColorMode colorMode;

      QMetaObject::Connection _configChangedConnection;
      QMetaObject::Connection _clipboardConnection;

      static int _rasterInit;
      static int _trackInfoWidthInit;
      static int _canvasWidthInit;
      static MidiEventColorMode colorModeInit;

      // Initial view state.
      MusECore::MidiPartViewState _viewState;
      MusECore::Scripts scripts;
      
      bool _playEvents;
      EventCanvas::PlayEventsMode _playEventsMode;

      QMetaObject::Connection _deliveredScriptReceivedMetaConn;
      QMetaObject::Connection _userScriptReceivedMetaConn;

      void initShortcuts();
      void setupNewCtrl(CtrlEdit* ctrlEdit);
      void setEventColorMode(MidiEventColorMode);
      QWidget* genToolbar(QWidget* parent);

      virtual void closeEvent(QCloseEvent*) override;
      virtual void keyPressEvent(QKeyEvent*) override;
      
      void setSpeakerMode(EventCanvas::PlayEventsMode mode);
      
      // Sets up a reasonable zoom minimum and/or maximum based on
      //  the current global midi division (ticks per quarter note)
      //  which has a very wide range (48 - 12288).
      // Also sets the canvas and time scale offsets accordingly.
      void setupHZoomRange();

   private slots:
      void setSelection(int /*tick*/, MusECore::Event&, MusECore::Part*, bool /*update*/);
      void noteinfoChanged(MusEGui::NoteInfo::ValType, int);
      void removeCtrl(CtrlEdit* ctrl);
      void soloChanged(bool flag);
      void gridOnChanged(bool flag);
      void setRaster(int) override;
      void cmd(int);
      void setSteprec(bool);
      void eventColorModeChanged(MidiEventColorMode);
      void clipboardChanged(); // enable/disable "Paste"
      void selectionChanged(); // enable/disable "Copy" & "Paste"
      void setSpeaker(bool);
      void setSpeakerSingleNoteMode(bool);
      void setSpeakerChordMode(bool);
      void setTime(unsigned);
      void follow(int pos);
      void songChanged1(MusECore::SongChangedStruct_t);
      void configChanged();
      void newCanvasWidth(int);
      void deltaModeChanged(bool);
      void addCtrlClicked();
      void ctrlPopupTriggered(QAction* act);
      void ctrlMenuAboutToShow();
      void ctrlMenuAboutToHide();

   signals:
      void isDeleting(MusEGui::TopWin*);
   
   public slots:
      void horizontalZoom(bool zoom_in, const QPoint& glob_pos);
      void horizontalZoom(int mag, const QPoint& glob_pos);
      void updateHScrollRange() override;
      void execDeliveredScript(int id);
      void execUserScript(int id);
      void focusCanvas() override;
      void storeInitialViewState() const override;
      
   public:
      PianoRoll(MusECore::PartList*, QWidget* parent = nullptr, const char* name = nullptr,
                unsigned initPos = INT_MAX, bool showDefaultControls = false);
      virtual ~PianoRoll();
      void readStatus(MusECore::Xml&) override;
      void writeStatus(int, MusECore::Xml&) const override;
      static void readConfiguration(MusECore::Xml&);
      static void writeConfiguration(int, MusECore::Xml&);
      void storeSettings() override;
      CtrlEdit* addCtrl(int ctl_num = MusECore::CTRL_VELOCITY);
      MusECore::MidiPartViewState getViewState() const;

      int pianoWidth() const { return _pianoWidth; }
      void setPianoWidth(int w) { _pianoWidth = w; }

      int changeRaster(int val);
      };

} // namespace MusEGui

#endif

