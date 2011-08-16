//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: waveedit.h,v 1.3.2.8 2008/01/26 07:23:21 terminator356 Exp $
//  (C) Copyright 2000 Werner Schweer (ws@seh.de)
//=========================================================

#ifndef __WAVE_EDIT_H__
#define __WAVE_EDIT_H__

#include <QMenu>

#include <QWidget>
#include <QResizeEvent>
#include <QKeyEvent>
#include <QCloseEvent>
#include <QByteArray>

#include "midieditor.h"

class QToolButton;
class PartList;
class WaveView;
class ScrollScale;
class QSlider;
class PosLabel;
class QResizeEvent;
class SNode;
class QAction;

//---------------------------------------------------------
//   WaveEdit
//---------------------------------------------------------

class WaveEdit : public MidiEditor {
      Q_OBJECT
    
      WaveView* view;
      QSlider* ymag;
      QToolBar* tools;
      QToolBar* tb1;
      QToolButton* solo;
      PosLabel* pos1;
      PosLabel* pos2;
      QAction* selectAllAction;
      QAction* selectNoneAction;
      QAction* cutAction;
      QAction* copyAction;
      QAction* pasteAction;
      

      
      virtual void closeEvent(QCloseEvent*);
      virtual void keyPressEvent(QKeyEvent*);

      QMenu* menuFunctions, *select, *menuGain;

      void initShortcuts();

   private slots:
      void cmd(int);
      void setTime(unsigned t);
      void songChanged1(int);
      void soloChanged(bool flag);
      void moveVerticalSlider(int val);

   public slots:
      void configChanged();
   
      virtual void updateHScrollRange();
      void horizontalZoomIn();
      void horizontalZoomOut();


   signals:
      void deleted(TopWin*);

   public:
      WaveEdit(PartList*);
      ~WaveEdit();
      virtual void readStatus(Xml&);
      virtual void writeStatus(int, Xml&) const;
      static void readConfiguration(Xml&);
      static void writeConfiguration(int, Xml&);

      enum { CMD_MUTE=0, CMD_NORMALIZE, CMD_FADE_IN, CMD_FADE_OUT, CMD_REVERSE,
             CMD_GAIN_FREE, CMD_GAIN_200, CMD_GAIN_150, CMD_GAIN_75, CMD_GAIN_50, CMD_GAIN_25,
             CMD_EDIT_COPY, CMD_EDIT_CUT, CMD_EDIT_PASTE,
             CMD_EDIT_EXTERNAL,
             CMD_SELECT_ALL, CMD_SELECT_NONE };
      };

#endif

