//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: pianoroll.h,v 1.5.2.4 2009/11/16 11:29:33 lunar_shuttle Exp $
//  (C) Copyright 1999 Werner Schweer (ws@seh.de)
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
class Splitter;
class PartList;
class Toolbar1;
class Xml;
class QuantConfig;
class ScrollScale;
class Part;
class SNode;
class QMenu;
class QAction;
class QWidget;
class QScrollBar;
class MidiTrackInfo;
class QScrollArea;

//---------------------------------------------------------
//   PianoRoll
//---------------------------------------------------------

class PianoRoll : public MidiEditor {
      Event selEvent;
      MidiPart* selPart;
      int selTick;

      //enum { CMD_EVENT_COLOR, CMD_CONFIG_QUANT, CMD_LAST };
      //int menu_ids[CMD_LAST];
      //Q3PopupMenu *menuEdit, *menuFunctions, *menuSelect, *menuConfig, *menuPlugins;

      
      QMenu *menuEdit, *menuFunctions, *menuSelect, *menuConfig, *eventColor, *menuPlugins;
      MidiTrackInfo *midiTrackInfo;
      Track* selected;
      
      QAction* editCutAction; 
      QAction* editCopyAction; 
      QAction* editPasteAction; 
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
      
      QAction* funcOverQuantAction;
      QAction* funcNoteOnQuantAction;
      QAction* funcNoteOnOffQuantAction;
      QAction* funcIterQuantAction;
      QAction* funcConfigQuantAction;
      QAction* funcGateTimeAction;
      QAction* funcModVelAction;
      QAction* funcCrescendoAction;
      QAction* funcTransposeAction;
      QAction* funcThinOutAction;
      QAction* funcEraseEventAction;
      QAction* funcNoteShiftAction;
      QAction* funcMoveClockAction;
      QAction* funcCopyMeasureAction;
      QAction* funcEraseMeasureAction;
      QAction* funcDelMeasureAction;
      QAction* funcCreateMeasureAction;
      QAction* funcSetFixedLenAction;
      QAction* funcDelOverlapsAction;
      
      
      int tickOffset;
      int lenOffset;
      int pitchOffset;
      int veloOnOffset;
      int veloOffOffset;
      bool deltaMode;

      NoteInfo* info;
      QToolButton* srec;
      QToolButton* midiin;

      Toolbar1* toolbar;
      Splitter* splitter;
      Splitter* hsplitter;
      Splitter* ctrlLane;

      QToolButton* speaker;
      QToolBar* tools;
      EditToolBar* tools2;

      int colorMode;

      static int _quantInit, _rasterInit;
      static int _widthInit, _heightInit;

      static int _quantStrengthInit;
      static int _quantLimitInit;
      static bool _quantLenInit;
      static int _toInit;
      static int colorModeInit;

      int _quantStrength;
      int _quantLimit;
      int _to;
      bool _quantLen;
      QuantConfig* quantConfig;
      bool _playEvents;

      //QScrollBar* infoScroll;
      QScrollArea* infoScroll;

      Q_OBJECT
      void initShortcuts();
      void setEventColorMode(int);
      QWidget* genToolbar(QWidget* parent);
      virtual void closeEvent(QCloseEvent*);
      virtual void keyPressEvent(QKeyEvent*);
      virtual void resizeEvent(QResizeEvent*);

   private slots:
      void setSelection(int, Event&, Part*);
      void noteinfoChanged(NoteInfo::ValType, int);
      CtrlEdit* addCtrl();
      void removeCtrl(CtrlEdit* ctrl);
      void soloChanged(bool flag);
      //void trackInfoScroll(int);
      void setRaster(int);
      void setQuant(int);
      void configQuant();
      void setQuantStrength(int val) { _quantStrength = val; }
      void setQuantLimit(int val)    { _quantLimit = val; }
      void setQuantLen(bool val)     { _quantLen = val; }
      void cmd(int);
      void setSteprec(bool);
      void setTo(int val)            { _to = val; }
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
      void deleted(unsigned long);
   
   public slots:
      virtual void updateHScrollRange();
      void execDeliveredScript(int id);
      void execUserScript(int id);
   public:
      PianoRoll(PartList*, QWidget* parent = 0, const char* name = 0, unsigned initPos = MAXINT);
      ~PianoRoll();
      virtual void readStatus(Xml&);
      virtual void writeStatus(int, Xml&) const;
      static void readConfiguration(Xml&);
      static void writeConfiguration(int, Xml&);
      };

#endif

