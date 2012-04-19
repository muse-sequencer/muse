//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: masteredit.cpp,v 1.4.2.5 2009/07/01 22:14:56 spamatica Exp $
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

#include "awl/sigedit.h"

#include "masteredit.h"
#include "mtscale.h"
#include "hitscale.h"
#include "sigscale.h"
#include "scrollscale.h"
#include "poslabel.h"
#include "master.h"
#include "utils.h"
#include "tscale.h"
#include "tempolabel.h"
#include "xml.h"
#include "lcombo.h"
#include "doublelabel.h"
#include "globals.h"
#include "app.h"
#include "gconfig.h"
#include "audio.h"

#include <limits.h>

#include <QActionGroup>
#include <QCloseEvent>
#include <QGridLayout>
#include <QLabel>
#include <QToolBar>
#include <QToolButton>
#include <QMenuBar>
#include <QMenu>

namespace MusEGui {

int MasterEdit::_rasterInit = 0;

//---------------------------------------------------------
//   closeEvent
//---------------------------------------------------------

void MasterEdit::closeEvent(QCloseEvent* e)
      {
      _isDeleting = true;  // Set flag so certain signals like songChanged, which may cause crash during delete, can be ignored.

      emit isDeleting(static_cast<TopWin*>(this));
      e->accept();
      }

//---------------------------------------------------------
//   songChanged
//---------------------------------------------------------

void MasterEdit::songChanged(int type)
      {
      if(_isDeleting)  // Ignore while while deleting to prevent crash.
        return;
        
      if (type & SC_TEMPO) {
            int tempo = MusEGlobal::tempomap.tempoAt(MusEGlobal::song->cpos());  // Bypass the useList flag and read from the list.
            curTempo->blockSignals(true);
            curTempo->setValue(double(60000000.0/tempo));
            
            curTempo->blockSignals(false);
            }
      if (type & SC_SIG) {
            int z, n;
            AL::sigmap.timesig(MusEGlobal::song->cpos(), z, n);
            curSig->blockSignals(true);
            curSig->setValue(AL::TimeSignature(z, n));
            curSig->blockSignals(false);
            sign->redraw();
            }
      if (type & SC_MASTER) {
            enableButton->blockSignals(true);
            enableButton->setChecked(MusEGlobal::song->masterFlag());
            enableButton->blockSignals(false);
            }
      }

//---------------------------------------------------------
//   MasterEdit
//---------------------------------------------------------

MasterEdit::MasterEdit()
   : MidiEditor(TopWin::MASTER, _rasterInit, 0)
      {
      setWindowTitle(tr("MusE: Mastertrack"));
      setFocusPolicy(Qt::NoFocus);
      _raster = 0;      // measure

      //---------Pulldown Menu----------------------------
      QMenu* settingsMenu = menuBar()->addMenu(tr("Window &Config"));
      settingsMenu->addAction(subwinAction);
      settingsMenu->addAction(shareAction);
      settingsMenu->addAction(fullscreenAction);

      // Toolbars ---------------------------------------------------------
      MusEGui::EditToolBar* tools2 = new MusEGui::EditToolBar(this, MusEGui::PointerTool | MusEGui::PencilTool | MusEGui::RubberTool);
      addToolBar(tools2);

      QToolBar* enableMaster = addToolBar(tr("Enable master"));
      enableMaster->setObjectName("Enable master");
      enableButton = new QToolButton();
      enableButton->setFocusPolicy(Qt::NoFocus);
      enableButton->setCheckable(true);
      enableButton->setText(tr("Enable"));
      enableButton->setToolTip(tr("Enable usage of master track"));
      enableButton->setChecked(MusEGlobal::song->masterFlag());
      enableMaster->addWidget(enableButton);
      connect(enableButton, SIGNAL(toggled(bool)), MusEGlobal::song, SLOT(setMasterFlag(bool)));

      QToolBar* info = addToolBar(tr("Info"));
      info->setObjectName("Info");
      QLabel* label  = new QLabel(tr("Cursor"));
      label->setAlignment(Qt::AlignRight|Qt::AlignVCenter);
      label->setIndent(3);
      info->addWidget(label);

      cursorPos = new MusEGui::PosLabel(0);
      cursorPos->setFixedHeight(22);
      cursorPos->setToolTip(tr("time at cursor position"));
      info->addWidget(cursorPos);
      tempo = new MusEGui::TempoLabel(0);
      tempo->setFixedHeight(22);
      tempo->setToolTip(tr("tempo at cursor position"));
      info->addWidget(tempo);

      const char* rastval[] = {
            QT_TRANSLATE_NOOP("MusEGui::MasterEdit", "Off"), QT_TRANSLATE_NOOP("MusEGui::MasterEdit", "Bar"), "1/2", "1/4", "1/8", "1/16"
            };
      rasterLabel = new MusEGui::LabelCombo(tr("Snap"), 0);
      rasterLabel->setFocusPolicy(Qt::TabFocus);
      for (int i = 0; i < 6; i++)
            rasterLabel->insertItem(i, tr(rastval[i]));
      rasterLabel->setCurrentIndex(1);
      info->addWidget(rasterLabel);
      connect(rasterLabel, SIGNAL(activated(int)), SLOT(_setRaster(int)));

      //---------values for current position---------------
      info->addWidget(new QLabel(tr("CurPos ")));
      curTempo = new MusEGui::TempoEdit(0);
      curSig   = new SigEdit(0);  // SigEdit is already StrongFocus.
      curTempo->setFocusPolicy(Qt::StrongFocus);
      curSig->setValue(AL::TimeSignature(4, 4));
      curTempo->setToolTip(tr("tempo at current position"));
      curSig->setToolTip(tr("time signature at current position"));
      info->addWidget(curTempo);
      info->addWidget(curSig);

      connect(curSig, SIGNAL(valueChanged(const AL::TimeSignature&)), SLOT(sigChange(const AL::TimeSignature&)));
      connect(curTempo, SIGNAL(tempoChanged(double)), SLOT(tempoChange(double)));
                                                                                    
      //---------------------------------------------------
      //    master
      //---------------------------------------------------

      int xscale = -20;
      int yscale = -500;
      hscroll   = new MusEGui::ScrollScale(-100, -2, xscale, MusEGlobal::song->len(), Qt::Horizontal, mainw);
      vscroll   = new MusEGui::ScrollScale(-1000, -100, yscale, 120000, Qt::Vertical, mainw);
      vscroll->setRange(30000, 250000);
      time1     = new MusEGui::MTScale(&_raster, mainw, xscale);
      sign      = new MusEGui::SigScale(&_raster, mainw, xscale);
//      thits     = new MusEGui::HitScale(&_raster, mainw, xscale); DELETETHIS what IS this? delete zhits as well

      canvas    = new Master(this, mainw, xscale, yscale);

//      zhits     = new MusEGui::HitScale(&_raster, mainw, xscale);
      time2     = new MusEGui::MTScale(&_raster, mainw, xscale);
      tscale    = new TScale(mainw, yscale);
      time2->setBarLocator(true);

      //---------------------------------------------------
      //    Rest
      //---------------------------------------------------

      mainGrid->setRowStretch(5, 100);
      mainGrid->setColumnStretch(1, 100);

      mainGrid->addWidget(MusECore::hLine(mainw),  0, 1);
      mainGrid->addWidget(time1,         1, 1);
      mainGrid->addWidget(MusECore::hLine(mainw),  2, 1);
      mainGrid->addWidget(sign,          3, 1);
      mainGrid->addWidget(MusECore::hLine(mainw),  4, 1);
//    mainGrid->addWidget(thits,         5, 1); DELETETHIS
//    mainGrid->addWidget(MusECore::hLine(mainw),  6, 1);
      mainGrid->addWidget(canvas,        5, 1);
      mainGrid->addWidget(tscale,        5, 0);
      mainGrid->addWidget(MusECore::hLine(mainw),  6, 1);
//    mainGrid->addWidget(zhits,         9, 1); DELETETHIS
//    mainGrid->addWidget(MusECore::hLine(mainw),  7, 1);
      mainGrid->addWidget(time2,         7, 1);
      mainGrid->addWidget(hscroll,       8, 1);
      mainGrid->addWidget(vscroll, 0, 2, 10, 1);

      canvas->setFocus(); 

      connect(tools2, SIGNAL(toolChanged(int)), canvas, SLOT(setTool(int)));
      connect(vscroll, SIGNAL(scrollChanged(int)),   canvas, SLOT(setYPos(int)));
      connect(vscroll, SIGNAL(scaleChanged(int)), canvas, SLOT(setYMag(int)));

      connect(vscroll, SIGNAL(scrollChanged(int)),   tscale, SLOT(setYPos(int)));
      connect(vscroll, SIGNAL(scaleChanged(int)), tscale, SLOT(setYMag(int)));

      connect(hscroll, SIGNAL(scrollChanged(int)), time1,  SLOT(setXPos(int)));
      connect(hscroll, SIGNAL(scrollChanged(int)), sign,   SLOT(setXPos(int)));
//      connect(hscroll, SIGNAL(scrollChanged(int)), thits,  SLOT(setXPos(int))); DELETETHIS
      connect(hscroll, SIGNAL(scrollChanged(int)), canvas, SLOT(setXPos(int)));
//      connect(hscroll, SIGNAL(scrollChanged(int)), zhits,  SLOT(setXPos(int)));DELETETHIS
      connect(hscroll, SIGNAL(scrollChanged(int)), time2,  SLOT(setXPos(int)));

      connect(hscroll, SIGNAL(scaleChanged(int)), time1,  SLOT(setXMag(int)));
      connect(hscroll, SIGNAL(scaleChanged(int)), sign,   SLOT(setXMag(int)));
//      connect(hscroll, SIGNAL(scaleChanged(int)), thits,  SLOT(setXMag(int)));DELETETHIS
      connect(hscroll, SIGNAL(scaleChanged(int)), canvas, SLOT(setXMag(int)));
//      connect(hscroll, SIGNAL(scaleChanged(int)), zhits,  SLOT(setXMag(int))); DELETETHIS
      connect(hscroll, SIGNAL(scaleChanged(int)), time2,  SLOT(setXMag(int)));

      connect(time1,  SIGNAL(timeChanged(unsigned)), SLOT(setTime(unsigned)));
//      connect(sign,   SIGNAL(timeChanged(unsigned)), pos, SLOT(setValue(unsigned))); DELETETHIS
//      connect(thits,  SIGNAL(timeChanged(unsigned)), pos, SLOT(setValue(unsigned)));
//      connect(canvas, SIGNAL(timeChanged(unsigned)), pos, SLOT(setValue(unsigned)));
//      connect(zhits,  SIGNAL(timeChanged(unsigned)), pos, SLOT(setValue(unsigned)));
      connect(time2,  SIGNAL(timeChanged(unsigned)), SLOT(setTime(unsigned)));

      connect(tscale, SIGNAL(tempoChanged(int)), SLOT(setTempo(int)));
      connect(canvas, SIGNAL(tempoChanged(int)), SLOT(setTempo(int)));
      connect(MusEGlobal::song, SIGNAL(songChanged(int)), SLOT(songChanged(int)));
      connect(MusEGlobal::song, SIGNAL(posChanged(int,unsigned,bool)), SLOT(posChanged(int,unsigned,bool)));

      connect(canvas, SIGNAL(followEvent(int)), hscroll, SLOT(setOffset(int)));
      connect(canvas, SIGNAL(timeChanged(unsigned)),   SLOT(setTime(unsigned)));

      connect(curSig,   SIGNAL(returnPressed()), SLOT(focusCanvas()));
      connect(curSig,   SIGNAL(escapePressed()), SLOT(focusCanvas()));
      connect(curTempo, SIGNAL(returnPressed()), SLOT(focusCanvas()));
      connect(curTempo, SIGNAL(escapePressed()), SLOT(focusCanvas()));
      
      initTopwinState();
      finalizeInit();
      }

//---------------------------------------------------------
//   ~MasterEdit
//---------------------------------------------------------

MasterEdit::~MasterEdit()
      {
      }

//---------------------------------------------------------
//   readStatus
//---------------------------------------------------------

void MasterEdit::readStatus(MusECore::Xml& xml)
      {
      for (;;) {
            MusECore::Xml::Token token = xml.parse();
            const QString& tag = xml.s1();
            switch (token) {
                  case MusECore::Xml::Error:
                  case MusECore::Xml::End:
                        return;
                  case MusECore::Xml::TagStart:
                        if (tag == "midieditor")
                              MidiEditor::readStatus(xml);
                        else if (tag == "ypos")
                              vscroll->setPos(xml.parseInt());
                        else if (tag == "ymag") {
                              // vscroll->setMag(xml.parseInt());
                              int mag = xml.parseInt();
                              vscroll->setMag(mag);
                              }
                        else
                              xml.unknown("MasterEdit");
                        break;
                  case MusECore::Xml::TagEnd:
                        if (tag == "master") {
                              // set raster
                              int item = 0;
                              switch(_raster) {
                                    case 1:   item = 0; break;
                                    case 0:   item = 1; break;
                                    case 768: item = 2; break;
                                    case 384: item = 3; break;
                                    case 192: item = 4; break;
                                    case 96:  item = 5; break;
                                    }
                              _rasterInit = _raster;
                              rasterLabel->setCurrentIndex(item);
                              return;
                              }
                  default:
                        break;
                  }
            }
      }

//---------------------------------------------------------
//   writeStatus
//---------------------------------------------------------

void MasterEdit::writeStatus(int level, MusECore::Xml& xml) const
      {
      xml.tag(level++, "master");
      xml.intTag(level, "ypos", vscroll->pos());
      xml.intTag(level, "ymag", vscroll->mag());
      MidiEditor::writeStatus(level, xml);
      xml.tag(level, "/master");
      }

//---------------------------------------------------------
//   readConfiguration
//---------------------------------------------------------

void MasterEdit::readConfiguration(MusECore::Xml& xml)
      {
      for (;;) {
            MusECore::Xml::Token token = xml.parse();
            const QString& tag = xml.s1();
            switch (token) {
                  case MusECore::Xml::Error:
                  case MusECore::Xml::End:
                        return;
                  case MusECore::Xml::TagStart:
                        if (tag == "raster")
                              _rasterInit = xml.parseInt();
                        else if (tag == "topwin")
                              TopWin::readConfiguration(MASTER, xml);
                        else
                              xml.unknown("MasterEdit");
                        break;
                  case MusECore::Xml::TagEnd:
                        if (tag == "masteredit")
                              return;
                  default:
                        break;
                  }
            }
      }

//---------------------------------------------------------
//   writeConfiguration
//---------------------------------------------------------

void MasterEdit::writeConfiguration(int level, MusECore::Xml& xml)
      {
      xml.tag(level++, "masteredit");
      xml.intTag(level, "raster", _rasterInit);
      TopWin::writeConfiguration(MASTER, level, xml);
      xml.tag(level, "/masteredit");
      }

//---------------------------------------------------------
//   focusCanvas
//---------------------------------------------------------

void MasterEdit::focusCanvas()
{
  if(MusEGlobal::config.smartFocus)
  {
    canvas->setFocus();
    canvas->activateWindow();
  } 
}

//---------------------------------------------------------
//   _setRaster
//---------------------------------------------------------

void MasterEdit::_setRaster(int index)
      {
      static int rasterTable[] = {
            1, 0, 768, 384, 192, 96
            };
      _raster = rasterTable[index];
      _rasterInit = _raster;
      focusCanvas();
      }

//---------------------------------------------------------
//   posChanged
//---------------------------------------------------------

void MasterEdit::posChanged(int idx, unsigned val, bool)
      {
      if (idx == 0) {
            int z, n;
            int tempo = MusEGlobal::tempomap.tempoAt(val); // Bypass the useList flag and read from the list.
            AL::sigmap.timesig(val, z, n);
            curTempo->blockSignals(true);
            curSig->blockSignals(true);

            curTempo->setValue(double(60000000.0/tempo));
            curSig->setValue(AL::TimeSignature(z, n));

            curTempo->blockSignals(false);
            curSig->blockSignals(false);
            }
      }

//---------------------------------------------------------
//   setTime
//---------------------------------------------------------

void MasterEdit::setTime(unsigned tick)
      {
      if (tick == INT_MAX)
            cursorPos->setEnabled(false);
      else {
            cursorPos->setEnabled(true);
            cursorPos->setValue(tick);
            time1->setPos(3, tick, false);
            time2->setPos(3, tick, false);
            }
      }

//---------------------------------------------------------
//   setTempo
//---------------------------------------------------------

void MasterEdit::setTempo(int val)
      {
      if (val == -1)
            tempo->setEnabled(false);
      else {
            tempo->setEnabled(true);
            tempo->setValue(val);
            }
      }
      
void MasterEdit::sigChange(const AL::TimeSignature& sig)
{
  // TODO: FIXME: Tempo/sig undo + redo broken here. Either fix tempo and sig, or finish something here...
  MusEGlobal::audio->msgAddSig(MusEGlobal::song->cPos().tick(), sig.z, sig.n);  // Add will replace if found. 
}

void MasterEdit::tempoChange(double t)
{
  if(int(t) == 0)
    return;
  
  // TODO: FIXME: Tempo/sig undo + redo broken here. Either fix tempo and sig, or finish something here... Also in transport.
  //MusEGlobal::song->startUndo();
  //iTEvent e = find(tick);
  //MusEGlobal::audio->msgDeleteTempo(it->first, it->second, false);
  MusEGlobal::audio->msgAddTempo(MusEGlobal::song->cPos().tick(), int(60000000.0/t), true);  // Add will replace if found. 
  //MusEGlobal::song->endUndo(SC_TEMPO);
}

} // namespace MusEGui
