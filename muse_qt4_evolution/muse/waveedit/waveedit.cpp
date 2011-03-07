//=============================================================================
//  MusE
//  Linux Music Editor
//  $Id:$
//
//  Copyright (C) 2000-2006 by Werner Schweer and others
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
      _parts  = pl;
      selPart = 0;

      //---------Pulldown Menu----------------------------
      QMenuBar* mb = menuBar();
      QAction* a;

      QMenu* menuFile = mb->addMenu(tr("&File"));
      QMenu* menuEdit = mb->addMenu(tr("&Edit"));
      menuFunctions   = mb->addMenu(tr("Func&tions"));

      menuGain = menuFunctions->addMenu(tr("&Gain"));
      a = menuGain->addAction(tr("200%"));
      a->setData("gain200");
      a = menuGain->addAction(tr("150%"));
      a->setData("gain150");
      a = menuGain->addAction(tr("75%"));
      a->setData("gain75");
      a = menuGain->addAction(tr("50%"));
      a->setData("gain50");
      a = menuGain->addAction(tr("25%"));
      a->setData("gain25");
      a = menuGain->addAction(tr("Other"));
      a->setData("gain_free");
      a = menuFunctions->addSeparator();

      a = menuEdit->addAction(tr("Edit in E&xternal Editor"));
      a->setData("exit_external");
      a = menuFunctions->addAction(tr("Mute Selection"));
      a->setData("mute");
      a = menuFunctions->addAction(tr("Normalize Selection"));
      a->setData("normalize");
      a = menuFunctions->addAction(tr("Fade In Selection"));
      a->setData("fade_in");
      a = menuFunctions->addAction(tr("Fade Out Selection"));
      a->setData("fade_out");
      a = menuFunctions->addAction(tr("Reverse Selection"));
      a->setData("reverse");

      select = menuEdit->addMenu(QIcon(*selectIcon), tr("Select"));
      select->addAction(getAction("sel_all", this));
      select->addAction(getAction("sel_none", this));

      connect(menuFunctions, SIGNAL(triggered(QAction*)), SLOT(cmd(QAction*)));
      connect(menuFile,      SIGNAL(triggered(QAction*)), SLOT(cmd(QAction*)));
      connect(select,        SIGNAL(triggered(QAction*)), SLOT(cmd(QAction*)));
      connect(menuGain,      SIGNAL(triggered(QAction*)), SLOT(cmd(QAction*)));
      connect(menuEdit,      SIGNAL(triggered(QAction*)), SLOT(cmd(QAction*)));

      //---------ToolBar----------------------------------
      tools = addToolBar(tr("waveedit-tools"));
      tools->addAction(undoAction);
      tools->addAction(redoAction);

      const int waveeditTools = PointerTool | PencilTool 
         | RubberTool | DrawTool;
      EditToolBar* tools2 = new EditToolBar(this, waveeditTools);
      addToolBar(tools2);

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

      connect(muse, SIGNAL(configChanged()), SLOT(configChanged()));
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

      connect(view, SIGNAL(toolChanged(int)), tools2, SLOT(set(int)));
      connect(tools2, SIGNAL(toolChanged(int)), view, SLOT(setTool(int)));

//      view->selectFirst();
      configChanged();
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
      view->cmd(a->data().toString());
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

//---------------------------------------------------------
//   read
//---------------------------------------------------------

void WaveEdit::read(QDomNode node)
      {
      for (node = node.firstChild(); !node.isNull(); node = node.nextSibling()) {
            QDomElement e = node.toElement();
            QString tag(e.tagName());
            if (tag == "CtrlEdit") {
                  int id = e.attribute("id","0").toInt();
                  int h = e.attribute("h","50").toInt();
                  view->addController(id, h);
                  }
            else
                  AL::readProperties(this, node);
            }
      view->layout1();
      }


//---------------------------------------------------------
//   write
//---------------------------------------------------------

void WaveEdit::write(Xml& xml) const
      {
      for (ciPart p = _parts->begin(); p != _parts->end(); ++p) {
            Part* part   = p->second;
            Track* track = part->track();
            int trkIdx   = song->tracks()->indexOf(track);
            int partIdx  = track->parts()->index(part);
            xml.stag("part");
            xml.put(QString("%1:%2").arg(trkIdx).arg(partIdx));
            xml.etag("part");
            }
      xml.stag(metaObject()->className());
      xml.writeProperties(this);
      const CtrlEditList* el = view->getCtrlEditors();
      for (ciCtrlEdit i = el->begin(); i != el->end(); ++i) {
            xml.tagE(QString("CtrlEdit h=\"%1\" id=\"%2\"")
               .arg((*i)->height()).arg((*i)->ctrl()->id()));
            }
      xml.etag(metaObject()->className());
      }


