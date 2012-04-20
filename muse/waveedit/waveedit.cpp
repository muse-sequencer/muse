//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: waveedit.cpp,v 1.5.2.12 2009/04/06 01:24:54 terminator356 Exp $
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

#include <QMenu>
#include <QSignalMapper>
#include <QToolBar>
#include <QToolButton>
#include <QLayout>
#include <QSizeGrip>
#include <QScrollBar>
#include <QLabel>
#include <QSlider>
#include <QMenuBar>
#include <QAction>
#include <QCloseEvent>
#include <QResizeEvent>
#include <QKeyEvent>
#include <QSettings>

#include "app.h"
#include "xml.h"
#include "waveedit.h"
#include "mtscale.h"
#include "scrollscale.h"
#include "waveview.h"
#include "ttoolbar.h"
#include "globals.h"
#include "audio.h"
#include "utils.h"
#include "song.h"
#include "poslabel.h"
#include "gconfig.h"
#include "icons.h"
#include "shortcuts.h"

namespace MusECore {
extern QColor readColor(MusECore::Xml& xml);
}

namespace MusEGui {

//---------------------------------------------------------
//   closeEvent
//---------------------------------------------------------

void WaveEdit::closeEvent(QCloseEvent* e)
      {
      _isDeleting = true;  // Set flag so certain signals like songChanged, which may cause crash during delete, can be ignored.
      
      QSettings settings("MusE", "MusE-qt");
      //settings.setValue("Waveedit/geometry", saveGeometry());
      settings.setValue("Waveedit/windowState", saveState());
      emit isDeleting(static_cast<TopWin*>(this));
      e->accept();
      }

//---------------------------------------------------------
//   WaveEdit
//---------------------------------------------------------

WaveEdit::WaveEdit(MusECore::PartList* pl)
   : MidiEditor(TopWin::WAVE, 1, pl)
      {
      setFocusPolicy(Qt::NoFocus);

      QSignalMapper* mapper = new QSignalMapper(this);
      QAction* act;
      
      //---------Pulldown Menu----------------------------
      // We probably don't need an empty menu - Orcan
      //QMenu* menuFile = menuBar()->addMenu(tr("&File"));
      QMenu* menuEdit = menuBar()->addMenu(tr("&Edit"));
      
      menuFunctions = menuBar()->addMenu(tr("Func&tions"));

      menuGain = menuFunctions->addMenu(tr("&Gain"));
      
      act = menuGain->addAction("200%");
      mapper->setMapping(act, CMD_GAIN_200);
      connect(act, SIGNAL(triggered()), mapper, SLOT(map()));
      
      act = menuGain->addAction("150%");
      mapper->setMapping(act, CMD_GAIN_150);
      connect(act, SIGNAL(triggered()), mapper, SLOT(map()));
      
      act = menuGain->addAction("75%");
      mapper->setMapping(act, CMD_GAIN_75);
      connect(act, SIGNAL(triggered()), mapper, SLOT(map()));
      
      act = menuGain->addAction("50%");
      mapper->setMapping(act, CMD_GAIN_50);
      connect(act, SIGNAL(triggered()), mapper, SLOT(map()));
      
      act = menuGain->addAction("25%");
      mapper->setMapping(act, CMD_GAIN_25);
      connect(act, SIGNAL(triggered()), mapper, SLOT(map()));
      
      act = menuGain->addAction(tr("Other"));
      mapper->setMapping(act, CMD_GAIN_FREE);
      connect(act, SIGNAL(triggered()), mapper, SLOT(map()));
      
      connect(mapper, SIGNAL(mapped(int)), this, SLOT(cmd(int)));
      
      menuFunctions->addSeparator();

      copyAction = menuEdit->addAction(tr("&Copy"));
      mapper->setMapping(copyAction, CMD_EDIT_COPY);
      connect(copyAction, SIGNAL(triggered()), mapper, SLOT(map()));

      cutAction = menuEdit->addAction(tr("C&ut"));
      mapper->setMapping(cutAction, CMD_EDIT_CUT);
      connect(cutAction, SIGNAL(triggered()), mapper, SLOT(map()));

      pasteAction = menuEdit->addAction(tr("&Paste"));
      mapper->setMapping(pasteAction, CMD_EDIT_PASTE);
      connect(pasteAction, SIGNAL(triggered()), mapper, SLOT(map()));


      act = menuEdit->addAction(tr("Edit in E&xternal Editor"));
      mapper->setMapping(act, CMD_EDIT_EXTERNAL);
      connect(act, SIGNAL(triggered()), mapper, SLOT(map()));
      
      act = menuFunctions->addAction(tr("Mute Selection"));
      mapper->setMapping(act, CMD_MUTE);
      connect(act, SIGNAL(triggered()), mapper, SLOT(map()));
      
      act = menuFunctions->addAction(tr("Normalize Selection"));
      mapper->setMapping(act, CMD_NORMALIZE);
      connect(act, SIGNAL(triggered()), mapper, SLOT(map()));
      
      act = menuFunctions->addAction(tr("Fade In Selection"));
      mapper->setMapping(act, CMD_FADE_IN);
      connect(act, SIGNAL(triggered()), mapper, SLOT(map()));
      
      act = menuFunctions->addAction(tr("Fade Out Selection"));
      mapper->setMapping(act, CMD_FADE_OUT);
      connect(act, SIGNAL(triggered()), mapper, SLOT(map()));
      
      act = menuFunctions->addAction(tr("Reverse Selection"));
      mapper->setMapping(act, CMD_REVERSE);
      connect(act, SIGNAL(triggered()), mapper, SLOT(map()));
      
      select = menuEdit->addMenu(QIcon(*selectIcon), tr("Select"));
      
      selectAllAction = select->addAction(QIcon(*select_allIcon), tr("Select &All"));
      mapper->setMapping(selectAllAction, CMD_SELECT_ALL);
      connect(selectAllAction, SIGNAL(triggered()), mapper, SLOT(map()));
      
      selectNoneAction = select->addAction(QIcon(*select_allIcon), tr("&Deselect All"));
      mapper->setMapping(selectNoneAction, CMD_SELECT_NONE);
      connect(selectNoneAction, SIGNAL(triggered()), mapper, SLOT(map()));
      
      
      QMenu* settingsMenu = menuBar()->addMenu(tr("Window &Config"));
      settingsMenu->addAction(subwinAction);
      settingsMenu->addAction(shareAction);
      settingsMenu->addAction(fullscreenAction);


      connect(MusEGlobal::muse, SIGNAL(configChanged()), SLOT(configChanged()));


      //--------------------------------------------------
      //    ToolBar:   Solo  Cursor1 Cursor2

      addToolBarBreak();
      tb1 = addToolBar(tr("WaveEdit tools"));
      tb1->setObjectName("WaveEdit tools");

      //tb1->setLabel(tr("weTools"));
      solo = new QToolButton();
      solo->setText(tr("Solo"));
      solo->setCheckable(true);
      solo->setFocusPolicy(Qt::NoFocus);
      tb1->addWidget(solo);
      connect(solo,  SIGNAL(toggled(bool)), SLOT(soloChanged(bool)));
      
      QLabel* label = new QLabel(tr("Cursor"));
      tb1->addWidget(label);
      label->setAlignment(Qt::AlignRight|Qt::AlignVCenter);
      label->setIndent(3);
      pos1 = new PosLabel(0);
      pos1->setFixedHeight(22);
      tb1->addWidget(pos1);
      pos2 = new PosLabel(0);
      pos2->setFixedHeight(22);
      pos2->setSmpte(true);
      tb1->addWidget(pos2);

      //---------------------------------------------------
      //    Rest
      //---------------------------------------------------

      int yscale = 256;
      int xscale;

      if (!parts()->empty()) { // Roughly match total size of part
            MusECore::Part* firstPart = parts()->begin()->second;
            xscale = 0 - firstPart->lenFrame()/_widthInit[_type];
            }
      else {
            xscale = -8000;
            }

      hscroll = new ScrollScale(1, -32768, xscale, 10000, Qt::Horizontal, mainw, 0, true, 10000.0);
      view    = new WaveView(this, mainw, xscale, yscale);
      wview   = view;   // HACK!

      QSizeGrip* corner    = new QSizeGrip(mainw);
      ymag                 = new QSlider(Qt::Vertical, mainw);
      ymag->setMinimum(1);
      ymag->setMaximum(256);
      ymag->setPageStep(256);
      ymag->setValue(yscale);
      ymag->setFocusPolicy(Qt::NoFocus);

      time                 = new MTScale(&_raster, mainw, xscale, true);
      ymag->setFixedWidth(16);
      connect(view, SIGNAL(mouseWheelMoved(int)), this, SLOT(moveVerticalSlider(int)));
      connect(ymag, SIGNAL(valueChanged(int)), view, SLOT(setYScale(int)));
      connect(view, SIGNAL(horizontalZoomIn()), SLOT(horizontalZoomIn()));
      connect(view, SIGNAL(horizontalZoomOut()), SLOT(horizontalZoomOut()));

      time->setOrigin(0, 0);

      mainGrid->setRowStretch(0, 100);
      mainGrid->setColumnStretch(0, 100);

      mainGrid->addWidget(time,   0, 0, 1, 2);
      mainGrid->addWidget(MusECore::hLine(mainw),    1, 0, 1, 2);
      mainGrid->addWidget(view,    2, 0);
      mainGrid->addWidget(ymag,    2, 1);
      mainGrid->addWidget(hscroll, 3, 0);
      mainGrid->addWidget(corner,  3, 1, Qt::AlignBottom | Qt::AlignRight);

      view->setFocus();  
      
      connect(hscroll, SIGNAL(scrollChanged(int)), view, SLOT(setXPos(int)));
      connect(hscroll, SIGNAL(scaleChanged(int)),  view, SLOT(setXMag(int)));
      setWindowTitle(view->getCaption());
      connect(view, SIGNAL(followEvent(int)), hscroll, SLOT(setOffset(int)));

      connect(hscroll, SIGNAL(scrollChanged(int)), time,  SLOT(setXPos(int)));
      connect(hscroll, SIGNAL(scaleChanged(int)),  time,  SLOT(setXMag(int)));
//      connect(time,    SIGNAL(timeChanged(unsigned)),  SLOT(setTime(unsigned)));
      connect(view,    SIGNAL(timeChanged(unsigned)),  SLOT(setTime(unsigned)));

      connect(view,  SIGNAL(horizontalScroll(unsigned)),hscroll, SLOT(setPos(unsigned)));

      connect(hscroll, SIGNAL(scaleChanged(int)),  SLOT(updateHScrollRange()));
      connect(MusEGlobal::song, SIGNAL(songChanged(int)), SLOT(songChanged1(int)));

      initShortcuts();
      
      updateHScrollRange();
      configChanged();
      
      if(!parts()->empty())
      {
        MusECore::WavePart* part = (MusECore::WavePart*)(parts()->begin()->second);
        solo->setChecked(part->track()->solo());
      }

      initTopwinState();
      finalizeInit();
      }

void WaveEdit::initShortcuts()
      {
      cutAction->setShortcut(shortcuts[SHRT_CUT].key);
      copyAction->setShortcut(shortcuts[SHRT_COPY].key);
      pasteAction->setShortcut(shortcuts[SHRT_PASTE].key);
      selectAllAction->setShortcut(shortcuts[SHRT_SELECT_ALL].key);
      selectNoneAction->setShortcut(shortcuts[SHRT_SELECT_NONE].key);
      }

//---------------------------------------------------------
//   configChanged
//---------------------------------------------------------

void WaveEdit::configChanged()
      {
      view->setBg(MusEGlobal::config.waveEditBackgroundColor);
      selectAllAction->setShortcut(shortcuts[SHRT_SELECT_ALL].key);
      selectNoneAction->setShortcut(shortcuts[SHRT_SELECT_NONE].key);
      }

//---------------------------------------------------------
//   updateHScrollRange
//---------------------------------------------------------
void WaveEdit::updateHScrollRange()
{
      int s, e;
      wview->range(&s, &e);
      // Show one more measure.
      e += AL::sigmap.ticksMeasure(e);  
      // Show another quarter measure due to imprecise drawing at canvas end point.
      e += AL::sigmap.ticksMeasure(e) / 4;
      // Compensate for the vscroll width. 
      //e += wview->rmapxDev(-vscroll->width()); 
      int s1, e1;
      hscroll->range(&s1, &e1);
      if(s != s1 || e != e1) 
        hscroll->setRange(s, e);
}

//---------------------------------------------------------
//   setTime
//---------------------------------------------------------

void WaveEdit::setTime(unsigned samplepos)
      {
//    printf("setTime %d %x\n", samplepos, samplepos);
      unsigned tick = MusEGlobal::tempomap.frame2tick(samplepos);
      pos1->setValue(tick);
      //pos2->setValue(tick);
      pos2->setValue(samplepos);
      time->setPos(3, tick, false);
      }

//---------------------------------------------------------
//   ~WaveEdit
//---------------------------------------------------------

WaveEdit::~WaveEdit()
      {
      // MusEGlobal::undoRedo->removeFrom(tools); // p4.0.6 Removed
      }

//---------------------------------------------------------
//   cmd
//---------------------------------------------------------

void WaveEdit::cmd(int n)
      {
      view->cmd(n);
      }

//---------------------------------------------------------
//   loadConfiguration
//---------------------------------------------------------

void WaveEdit::readConfiguration(MusECore::Xml& xml)
      {
      for (;;) {
            MusECore::Xml::Token token = xml.parse();
            const QString& tag = xml.s1();
            switch (token) {
                  case MusECore::Xml::TagStart:
                        if (tag == "bgcolor")
                              MusEGlobal::config.waveEditBackgroundColor = readColor(xml);
                        else if (tag == "topwin")
                              TopWin::readConfiguration(WAVE, xml);
                        else
                              xml.unknown("WaveEdit");
                        break;
                  case MusECore::Xml::TagEnd:
                        if (tag == "waveedit")
                              return;
                  default:
                        break;
                  case MusECore::Xml::Error:
                  case MusECore::Xml::End:
                        return;
                  }
            }
      }

//---------------------------------------------------------
//   saveConfiguration
//---------------------------------------------------------

void WaveEdit::writeConfiguration(int level, MusECore::Xml& xml)
      {
      xml.tag(level++, "waveedit");
      xml.colorTag(level, "bgcolor", MusEGlobal::config.waveEditBackgroundColor);
      TopWin::writeConfiguration(WAVE, level,xml);
      xml.tag(level, "/waveedit");
      }

//---------------------------------------------------------
//   writeStatus
//---------------------------------------------------------

void WaveEdit::writeStatus(int level, MusECore::Xml& xml) const
      {
      writePartList(level, xml);
      xml.tag(level++, "waveedit");
      MidiEditor::writeStatus(level, xml);
      xml.intTag(level, "xpos", hscroll->pos());
      xml.intTag(level, "xmag", hscroll->mag());
      xml.intTag(level, "ymag", ymag->value());
      xml.tag(level, "/waveedit");
      }

//---------------------------------------------------------
//   readStatus
//---------------------------------------------------------

void WaveEdit::readStatus(MusECore::Xml& xml)
      {
      for (;;) {
            MusECore::Xml::Token token = xml.parse();
            if (token == MusECore::Xml::Error || token == MusECore::Xml::End)
                  break;
            QString tag = xml.s1();
            switch (token) {
                  case MusECore::Xml::TagStart:
                        if (tag == "midieditor")
                              MidiEditor::readStatus(xml);
                        else if (tag == "xmag")
                              hscroll->setMag(xml.parseInt());
                        else if (tag == "ymag")
                              ymag->setValue(xml.parseInt());
                        else if (tag == "xpos")
                              hscroll->setPos(xml.parseInt());
                        else
                              xml.unknown("WaveEdit");
                        break;
                  case MusECore::Xml::TagEnd:
                        if (tag == "waveedit")
                              return;
                  default:
                        break;
                  }
            }
      }


//---------------------------------------------------------
//   songChanged1
//    signal from "song"
//---------------------------------------------------------

void WaveEdit::songChanged1(int bits)
      {
        if(_isDeleting)  // Ignore while while deleting to prevent crash.
          return;

        if (bits & SC_SOLO)
        {
          MusECore::WavePart* part = (MusECore::WavePart*)(parts()->begin()->second);
          solo->blockSignals(true);
          solo->setChecked(part->track()->solo());
          solo->blockSignals(false);
        }  
        
        songChanged(bits);
      }


//---------------------------------------------------------
//   soloChanged
//    signal from solo button
//---------------------------------------------------------

void WaveEdit::soloChanged(bool flag)
      {
      MusECore::WavePart* part = (MusECore::WavePart*)(parts()->begin()->second);
      MusEGlobal::audio->msgSetSolo(part->track(), flag);
      MusEGlobal::song->update(SC_SOLO);
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
//   moveVerticalSlider
//---------------------------------------------------------

void WaveEdit::moveVerticalSlider(int val)
      {
      ymag->setValue(ymag->value() + val);
      }


void WaveEdit::horizontalZoomIn()
{
  int mag = hscroll->mag();
  int zoomlvl = ScrollScale::getQuickZoomLevel(mag);
  if (zoomlvl < 23)
        zoomlvl++;

  int newmag = ScrollScale::convertQuickZoomLevelToMag(zoomlvl);

  hscroll->setMag(newmag);

}

void WaveEdit::horizontalZoomOut()
{
  int mag = hscroll->mag();
  int zoomlvl = ScrollScale::getQuickZoomLevel(mag);
  if (zoomlvl > 1)
        zoomlvl--;

  int newmag = ScrollScale::convertQuickZoomLevelToMag(zoomlvl);

  hscroll->setMag(newmag);

}

//---------------------------------------------------------
//   focusCanvas
//---------------------------------------------------------

void WaveEdit::focusCanvas()
{
  if(MusEGlobal::config.smartFocus)
  {
    view->setFocus();
    view->activateWindow();
  }
}

} // namespace MusEGui
