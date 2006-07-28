//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: waveedit.cpp,v 1.33 2006/02/10 16:40:59 wschweer Exp $
//  (C) Copyright 2000 Werner Schweer (ws@seh.de)
//=========================================================

#include "waveedit.h"
#include "waveview.h"
#include "song.h"
#include "awl/poslabel.h"
#include "esettings.h"
#include "al/tempo.h"
#include "icons.h"
#include "shortcuts.h"
#include "wave.h"
#include "part.h"
#include "muse.h"

int WaveEdit::initWidth  = WaveEdit::INIT_WIDTH;
int WaveEdit::initHeight = WaveEdit::INIT_HEIGHT;

//---------------------------------------------------------
//   WaveEdit
//---------------------------------------------------------

WaveEdit::WaveEdit(PartList* pl, bool init)
   : Editor()
      {
      _parts = pl;
      //---------Pulldown Menu----------------------------
      QMenuBar* mb = menuBar();
      QAction* a;

      QMenu* menuFile = mb->addMenu(tr("&File"));
      QMenu* menuEdit = mb->addMenu(tr("&Edit"));
      menuFunctions   = mb->addMenu(tr("Func&tions"));

      menuGain = menuFunctions->addMenu(tr("&Gain"));
      a = menuGain->addAction(tr("200%"));
      a->setData(CMD_GAIN_200);
      a = menuGain->addAction(tr("150%"));
      a->setData(CMD_GAIN_150);
      a = menuGain->addAction(tr("75%"));
      a->setData(CMD_GAIN_75);
      a = menuGain->addAction(tr("50%"));
      a->setData(CMD_GAIN_50);
      a = menuGain->addAction(tr("25%"));
      a->setData(CMD_GAIN_25);
      a = menuGain->addAction(tr("Other"));
      a->setData(CMD_GAIN_FREE);
      a = menuFunctions->addSeparator();

      a = menuEdit->addAction(tr("Edit in E&xternal Editor"));
      a->setData(CMD_EDIT_EXTERNAL);
      a = menuFunctions->addAction(tr("Mute Selection"));
      a->setData(CMD_MUTE);
      a = menuFunctions->addAction(tr("Normalize Selection"));
      a->setData(CMD_NORMALIZE);
      a = menuFunctions->addAction(tr("Fade In Selection"));
      a->setData(CMD_FADE_IN);
      a = menuFunctions->addAction(tr("Fade Out Selection"));
      a->setData(CMD_FADE_OUT);
      a = menuFunctions->addAction(tr("Reverse Selection"));
      a->setData(CMD_REVERSE);

      select = menuEdit->addMenu(QIcon(*selectIcon), tr("Select"));
      a = select->addAction(QIcon(*select_allIcon), tr("Select &All"));
      a->setData(CMD_SELECT_ALL);
      a = select->addAction(QIcon(*select_deselect_allIcon), tr("&Deselect All"));
      a->setData(CMD_SELECT_NONE);


      connect(menuFunctions, SIGNAL(triggered(QAction*)), SLOT(cmd(QAction*)));
      connect(menuFile,      SIGNAL(triggered(QAction*)), SLOT(cmd(QAction*)));
      connect(select,        SIGNAL(triggered(QAction*)), SLOT(cmd(QAction*)));
      connect(menuGain,      SIGNAL(triggered(QAction*)), SLOT(cmd(QAction*)));
      connect(menuEdit,      SIGNAL(triggered(QAction*)), SLOT(cmd(QAction*)));

      //---------ToolBar----------------------------------
      tools = addToolBar(tr("waveedit-tools"));
      tools->addAction(undoAction);
      tools->addAction(redoAction);

      connect(muse, SIGNAL(configChanged()), SLOT(configChanged()));

      //--------------------------------------------------
      //    Transport Bar
      QToolBar* transport = addToolBar(tr("Transport"));
      muse->setupTransportToolbar(transport);

      //--------------------------------------------------
      //    ToolBar:   Solo  Cursor1 Cursor2

      addToolBarBreak();
      tb1 = addToolBar(tr("pianoroll-tools"));
      solo = tb1->addAction(tr("Solo"));
      solo->setCheckable(true);
      connect(solo,  SIGNAL(toggled(bool)), SLOT(soloChanged(bool)));

      tb1->addWidget(new QLabel(tr("Cursor")));
      pos1 = new PosLabel;
      pos2 = new PosLabel;
      pos2->setSmpte(true);
      tb1->addWidget(pos1);
      tb1->addWidget(pos2);

      //---------------------------------------------------
      //    Rest
      //---------------------------------------------------

//      if (!parts()->empty()) { // Roughly match total size of part
//            Part* firstPart = parts()->begin()->second;
//            xscale = 0 - firstPart->lenFrame()/_widthInit;
//            }

      view = new WaveView(this);
      view->setRaster(0);
      view->setFollow(INIT_FOLLOW);

      connect(song, SIGNAL(posChanged(int,const AL::Pos&,bool)), view, SLOT(setLocatorPos(int,const AL::Pos&,bool)));
      view->setLocatorPos(0, song->cpos(), true);
      view->setLocatorPos(1, song->lpos(), false);
      view->setLocatorPos(2, song->rpos(), false);
//      connect(view, SIGNAL(cursorPos(unsigned)), SIGNAL(setTime(unsigned)));
      connect(view, SIGNAL(posChanged(int,const AL::Pos&)), song, SLOT(setPos(int,const AL::Pos&)));

      setCentralWidget(view);
      view->setCornerWidget(new QSizeGrip(view));
      setWindowTitle(view->getCaption());

      Pos p1(0, AL::FRAMES), p2(0, AL::FRAMES);
      view->range(p1, p2);
      p2 += AL::sigmap.ticksMeasure(p2.tick());  // show one more measure
      view->setTimeRange(p1, p2);

      configChanged();
//      initSettings();

      if (init)
            ;  // initFromPart();
      else {
	      resize(initWidth, initHeight);
            }
      }

//---------------------------------------------------------
//   configChanged
//---------------------------------------------------------

void WaveEdit::configChanged()
      {
//      view->setBg(config.waveEditBackgroundColor);
//TD      select->setShortcut(shortcuts[SHRT_SELECT_ALL].key, CMD_SELECT_ALL);
//      select->setShortcut(shortcuts[SHRT_SELECT_NONE].key, CMD_SELECT_NONE);
      }

//---------------------------------------------------------
//   setTime
//---------------------------------------------------------

void WaveEdit::setTime(unsigned samplepos)
      {
//    printf("setTime %d %x\n", samplepos, samplepos);
      unsigned tick = AL::tempomap.frame2tick(samplepos);
      pos1->setValue(tick, true);
      pos2->setValue(tick, true);
//      time->setPos(3, tick, false);
      }

//---------------------------------------------------------
//   ~WaveEdit
//---------------------------------------------------------

WaveEdit::~WaveEdit()
      {
//      undoRedo->removeFrom(tools);
      }

//---------------------------------------------------------
//   cmd
//---------------------------------------------------------

void WaveEdit::cmd(QAction* a)
      {
      view->cmd(a->data().toInt());
      }

//---------------------------------------------------------
//   soloChanged
//    signal from "song"
//---------------------------------------------------------

void WaveEdit::soloChanged(SNode*/* s*/)
      {
      Part* part = parts()->begin()->second;
      solo->setChecked(part->track()->solo());
      }

//---------------------------------------------------------
//   soloChanged
//    signal from solo button
//---------------------------------------------------------

void WaveEdit::soloChanged(bool flag)
      {
      Part* part = parts()->begin()->second;
      song->setSolo(part->track(), flag);
      }

//---------------------------------------------------------
//   viewKeyPressEvent
//---------------------------------------------------------

void WaveEdit::keyPressEvent(QKeyEvent* event)
      {
      int key = event->key();
      if (key == Qt::Key_Escape) {
            close();
            return;
            }
      else {
            event->ignore();
            }
      }


