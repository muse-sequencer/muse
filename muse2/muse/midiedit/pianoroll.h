//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: pianoroll.h,v 1.5.2.4 2009/11/16 11:29:33 lunar_shuttle Exp $
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

#ifndef __PIANOROLL_H__
#define __PIANOROLL_H__

#include <QCloseEvent>
#include <QResizeEvent>
#include <QLabel>
#include <QKeyEvent>

#include <values.h>
#include "noteinfo.h"
#include "cobject.h"
#include "midieditor.h"
#include "tools.h"
#include "event.h"

class MidiPart;
class TimeLabel;
class PitchLabel;
class QLabel;
class PianoCanvas;
class MTScale;
class Track;
class QToolButton;
class QToolBar;
class QPushButton;
class CtrlEdit;
class PartList;
class Xml;
class ScrollScale;
class Part;
class SNode;
class QMenu;
class QAction;
class QWidget;
class QScrollBar;
class QScrollArea;

namespace MusEWidget {
class MidiTrackInfo;
class Splitter;
class Toolbar1;
}

//---------------------------------------------------------
//   PianoRoll
//---------------------------------------------------------

class PianoRoll : public MidiEditor {
      Q_OBJECT
    
      Event selEvent;
      MidiPart* selPart;
      int selTick;

      //enum { CMD_EVENT_COLOR, CMD_CONFIG_QUANT, CMD_LAST };
      //int menu_ids[CMD_LAST];
      //Q3PopupMenu *menuEdit, *menuFunctions, *menuSelect, *menuConfig, *menuPlugins;

      
      QMenu *menuEdit, *menuFunctions, *menuSelect, *menuConfig, *eventColor, *menuPlugins;
      MusEWidget::MidiTrackInfo *midiTrackInfo;
      Track* selected;
      
      QAction* editCutAction; 
      QAction* editCopyAction; 
      QAction* editPasteAction; 
      QAction* editPasteDialogAction; 
      QAction* editDelEventsAction;
      
      QAction* selectAllAction; 
      QAction* selectNoneAction;
      QAction* selectInvertAction;
      QAction* selectInsideLoopAction;
      QAction* selectOutsideLoopAction;
      QAction* selectPrevPartAction;
      QAction* selectNextPartAction;
      
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
      
      
      int tickOffset;
      int lenOffset;
      int pitchOffset;
      int veloOnOffset;
      int veloOffOffset;
      bool deltaMode;

      MusEWidget::NoteInfo* info;
      QToolButton* srec;
      QToolButton* midiin;

      MusEWidget::Toolbar1* toolbar;
      MusEWidget::Splitter* splitter;
      MusEWidget::Splitter* hsplitter;
      MusEWidget::Splitter* ctrlLane;

      QToolButton* speaker;
      QToolBar* tools;
      MusEWidget::EditToolBar* tools2;

      int colorMode;

      static int _rasterInit;

      static int colorModeInit;

      bool _playEvents;

      //QScrollBar* infoScroll;
      QScrollArea* infoScroll;

      
      void initShortcuts();
      void setEventColorMode(int);
      QWidget* genToolbar(QWidget* parent);
      virtual void closeEvent(QCloseEvent*);
      virtual void keyPressEvent(QKeyEvent*);

   private slots:
      void setSelection(int, Event&, Part*);
      void noteinfoChanged(MusEWidget::NoteInfo::ValType, int);
      //CtrlEdit* addCtrl();
      void removeCtrl(CtrlEdit* ctrl);
      void soloChanged(bool flag);
      //void trackInfoScroll(int);
      void setRaster(int);
      void cmd(int);
      void setSteprec(bool);
      void eventColorModeChanged(int);
      void clipboardChanged(); // enable/disable "Paste"
      void selectionChanged(); // enable/disable "Copy" & "Paste"
      void setSpeaker(bool);
      void setTime(unsigned);
      void follow(int pos);
      void songChanged1(int);
      void configChanged();
      void newCanvasWidth(int);
      void toggleTrackInfo();
      void updateTrackInfo();

   signals:
      void deleted(TopWin*);
   
   public slots:
      virtual void updateHScrollRange();
      void execDeliveredScript(int id);
      void execUserScript(int id);
      CtrlEdit* addCtrl();
      
   public:
      PianoRoll(PartList*, QWidget* parent = 0, const char* name = 0, unsigned initPos = MAXINT);
      ~PianoRoll();
      virtual void readStatus(Xml&);
      virtual void writeStatus(int, Xml&) const;
      static void readConfiguration(Xml&);
      static void writeConfiguration(int, Xml&);
      };

#endif

