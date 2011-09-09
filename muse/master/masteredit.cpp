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
///#include "sigedit.h"
#include "globals.h"

#include <values.h>

#include <QActionGroup>
#include <QCloseEvent>
#include <QGridLayout>
#include <QLabel>
#include <QToolBar>
#include <QToolButton>
#include <QSettings>

int MasterEdit::_rasterInit = 0;
int MasterEdit::_widthInit = 600;
int MasterEdit::_heightInit = 400;
QByteArray MasterEdit::_toolbarInit;

//---------------------------------------------------------
//   closeEvent
//---------------------------------------------------------

void MasterEdit::closeEvent(QCloseEvent* e)
      {
      QSettings settings("MusE", "MusE-qt");
      //settings.setValue("MasterEdit/geometry", saveGeometry());
      settings.setValue("MasterEdit/windowState", saveState());
      
      emit deleted((unsigned long)this);
      e->accept();
      }

//---------------------------------------------------------
//   songChanged
//---------------------------------------------------------

void MasterEdit::songChanged(int type)
      {
      if (type & SC_TEMPO) {
            int tempo = tempomap.tempo(song->cpos());
            curTempo->blockSignals(true);
            curTempo->setValue(double(60000000.0/tempo));
            
            curTempo->blockSignals(false);
            }
      if (type & SC_SIG) {
            int z, n;
            AL::sigmap.timesig(song->cpos(), z, n);
            curSig->blockSignals(true);
            curSig->setValue(AL::TimeSignature(z, n));
            curSig->blockSignals(false);
            sign->redraw();
            }
      if (type & SC_MASTER) {
            enableButton->blockSignals(true);
            enableButton->setChecked(song->masterFlag());
            enableButton->blockSignals(false);
            }
      }

//---------------------------------------------------------
//   MasterEdit
//---------------------------------------------------------

MasterEdit::MasterEdit()
   : MidiEditor(_rasterInit, 0)
      {
      setWindowTitle(tr("MusE: Mastertrack"));
      _raster = 0;      // measure
      setMinimumSize(400, 300);
      resize(_widthInit, _heightInit);

      //---------Pulldown Menu----------------------------
//      QPopupMenu* file = new QPopupMenu(this);
//      menuBar()->insertItem("&File", file);

      //---------ToolBar----------------------------------
      
      tools = addToolBar(tr("Master tools"));
      tools->setObjectName("Master tools");
      tools->addActions(MusEGlobal::undoRedo->actions());

      MusEWidget::EditToolBar* tools2 = new MusEWidget::EditToolBar(this, MusEWidget::PointerTool | MusEWidget::PencilTool | MusEWidget::RubberTool);
      addToolBar(tools2);

      QToolBar* enableMaster = addToolBar(tr("Enable master"));
      enableMaster->setObjectName("Enable master");
      enableButton = new QToolButton();
      enableButton->setCheckable(true);
      enableButton->setText(tr("Enable"));
      enableButton->setToolTip(tr("Enable usage of master track"));
      enableButton->setChecked(song->masterFlag());
      enableMaster->addWidget(enableButton);
      connect(enableButton, SIGNAL(toggled(bool)), song, SLOT(setMasterFlag(bool)));

      QToolBar* info = addToolBar(tr("Info"));
      info->setObjectName("Info");
      QLabel* label  = new QLabel(tr("Cursor"));
      label->setAlignment(Qt::AlignRight|Qt::AlignVCenter);
      label->setIndent(3);
      info->addWidget(label);

      cursorPos = new MusEWidget::PosLabel(0);
      cursorPos->setFixedHeight(22);
      cursorPos->setToolTip(tr("time at cursor position"));
      info->addWidget(cursorPos);
      tempo = new MusEWidget::TempoLabel(0);
      tempo->setFixedHeight(22);
      tempo->setToolTip(tr("tempo at cursor position"));
      info->addWidget(tempo);

      const char* rastval[] = {
            QT_TRANSLATE_NOOP("@default", "Off"), "Bar", "1/2", "1/4", "1/8", "1/16"
            };
      rasterLabel = new MusEWidget::LabelCombo(tr("Snap"), 0);
      rasterLabel->setFocusPolicy(Qt::NoFocus);
      for (int i = 0; i < 6; i++)
            rasterLabel->insertItem(i, tr(rastval[i]));
      rasterLabel->setCurrentIndex(1);
      info->addWidget(rasterLabel);
      connect(rasterLabel, SIGNAL(activated(int)), SLOT(_setRaster(int)));

      //---------values for current position---------------
      info->addWidget(new QLabel(tr("CurPos ")));
      curTempo = new MusEWidget::TempoEdit(0);
      curSig   = new SigEdit(0);
      curSig->setValue(AL::TimeSignature(4, 4));
      curTempo->setToolTip(tr("tempo at current position"));
      curSig->setToolTip(tr("time signature at current position"));
      info->addWidget(curTempo);
      info->addWidget(curSig);
      ///connect(curSig, SIGNAL(valueChanged(int,int)), song, SLOT(setSig(int,int)));
      connect(curSig, SIGNAL(valueChanged(const AL::TimeSignature&)), song, SLOT(setSig(const AL::TimeSignature&)));
      
      ///connect(curTempo, SIGNAL(valueChanged(double)), song, SLOT(setTempo(double)));
      connect(curTempo, SIGNAL(tempoChanged(double)), song, SLOT(setTempo(double)));
                                                                                    
      //---------------------------------------------------
      //    master
      //---------------------------------------------------

      int xscale = -20;
      int yscale = -500;
      hscroll   = new MusEWidget::ScrollScale(-100, -2, xscale, song->len(), Qt::Horizontal, mainw);
      vscroll   = new MusEWidget::ScrollScale(-1000, -100, yscale, 120000, Qt::Vertical, mainw);
      vscroll->setRange(30000, 250000);
      time1     = new MusEWidget::MTScale(&_raster, mainw, xscale);
      sign      = new MusEWidget::SigScale(&_raster, mainw, xscale);
//      thits     = new MusEWidget::HitScale(&_raster, mainw, xscale);

      canvas    = new Master(this, mainw, xscale, yscale);

//      zhits     = new MusEWidget::HitScale(&_raster, mainw, xscale);
      time2     = new MusEWidget::MTScale(&_raster, mainw, xscale);
      tscale    = new TScale(mainw, yscale);
      time2->setBarLocator(true);

      //---------------------------------------------------
      //    Rest
      //---------------------------------------------------

//      QSizeGrip* corner   = new QSizeGrip(mainw);

      mainGrid->setRowStretch(5, 100);
      mainGrid->setColumnStretch(1, 100);

      mainGrid->addWidget(MusEUtil::hLine(mainw),  0, 1);
      mainGrid->addWidget(time1,         1, 1);
      mainGrid->addWidget(MusEUtil::hLine(mainw),  2, 1);
      mainGrid->addWidget(sign,          3, 1);
      mainGrid->addWidget(MusEUtil::hLine(mainw),  4, 1);
//    mainGrid->addWidget(thits,         5, 1);
//    mainGrid->addWidget(MusEUtil::hLine(mainw),  6, 1);
      mainGrid->addWidget(canvas,        5, 1);
      mainGrid->addWidget(tscale,        5, 0);
      mainGrid->addWidget(MusEUtil::hLine(mainw),  6, 1);
//    mainGrid->addWidget(zhits,         9, 1);
//    mainGrid->addWidget(MusEUtil::hLine(mainw),  7, 1);
      mainGrid->addWidget(time2,         7, 1);
      mainGrid->addWidget(hscroll,       8, 1);
      mainGrid->addWidget(vscroll, 0, 2, 10, 1);
//      mainGrid->addWidget(corner,  9, 2, AlignBottom | AlignRight);

      canvas->setFocus(); // Tim.

      connect(tools2, SIGNAL(toolChanged(int)), canvas, SLOT(setTool(int)));
      connect(vscroll, SIGNAL(scrollChanged(int)),   canvas, SLOT(setYPos(int)));
      connect(vscroll, SIGNAL(scaleChanged(int)), canvas, SLOT(setYMag(int)));

      connect(vscroll, SIGNAL(scrollChanged(int)),   tscale, SLOT(setYPos(int)));
      connect(vscroll, SIGNAL(scaleChanged(int)), tscale, SLOT(setYMag(int)));

      connect(hscroll, SIGNAL(scrollChanged(int)), time1,  SLOT(setXPos(int)));
      connect(hscroll, SIGNAL(scrollChanged(int)), sign,   SLOT(setXPos(int)));
//      connect(hscroll, SIGNAL(scrollChanged(int)), thits,  SLOT(setXPos(int)));
      connect(hscroll, SIGNAL(scrollChanged(int)), canvas, SLOT(setXPos(int)));
//      connect(hscroll, SIGNAL(scrollChanged(int)), zhits,  SLOT(setXPos(int)));
      connect(hscroll, SIGNAL(scrollChanged(int)), time2,  SLOT(setXPos(int)));

      connect(hscroll, SIGNAL(scaleChanged(int)), time1,  SLOT(setXMag(int)));
      connect(hscroll, SIGNAL(scaleChanged(int)), sign,   SLOT(setXMag(int)));
//      connect(hscroll, SIGNAL(scaleChanged(int)), thits,  SLOT(setXMag(int)));
      connect(hscroll, SIGNAL(scaleChanged(int)), canvas, SLOT(setXMag(int)));
//      connect(hscroll, SIGNAL(scaleChanged(int)), zhits,  SLOT(setXMag(int)));
      connect(hscroll, SIGNAL(scaleChanged(int)), time2,  SLOT(setXMag(int)));

      connect(time1,  SIGNAL(timeChanged(unsigned)), SLOT(setTime(unsigned)));
//      connect(sign,   SIGNAL(timeChanged(unsigned)), pos, SLOT(setValue(unsigned)));
//      connect(thits,  SIGNAL(timeChanged(unsigned)), pos, SLOT(setValue(unsigned)));
//      connect(canvas, SIGNAL(timeChanged(unsigned)), pos, SLOT(setValue(unsigned)));
//      connect(zhits,  SIGNAL(timeChanged(unsigned)), pos, SLOT(setValue(unsigned)));
      connect(time2,  SIGNAL(timeChanged(unsigned)), SLOT(setTime(unsigned)));

      connect(tscale, SIGNAL(tempoChanged(int)), SLOT(setTempo(int)));
      connect(canvas, SIGNAL(tempoChanged(int)), SLOT(setTempo(int)));
      connect(song, SIGNAL(songChanged(int)), SLOT(songChanged(int)));
      connect(song, SIGNAL(posChanged(int,unsigned,bool)), SLOT(posChanged(int,unsigned,bool)));

      connect(canvas, SIGNAL(followEvent(int)), hscroll, SLOT(setOffset(int)));
      connect(canvas, SIGNAL(timeChanged(unsigned)),   SLOT(setTime(unsigned)));

      if (!_toolbarInit.isEmpty())
            restoreState(_toolbarInit);      
      
      QSettings settings("MusE", "MusE-qt");
      //restoreGeometry(settings.value("MasterEdit/geometry").toByteArray());
      restoreState(settings.value("MasterEdit/windowState").toByteArray());
      }

//---------------------------------------------------------
//   ~MasterEdit
//---------------------------------------------------------

MasterEdit::~MasterEdit()
      {
      //undoRedo->removeFrom(tools);  // p4.0.6 Removed
      }

//---------------------------------------------------------
//   readStatus
//---------------------------------------------------------

void MasterEdit::readStatus(Xml& xml)
      {
      for (;;) {
            Xml::Token token = xml.parse();
            const QString& tag = xml.s1();
            switch (token) {
                  case Xml::Error:
                  case Xml::End:
                        return;
                  case Xml::TagStart:
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
                  case Xml::TagEnd:
                        if (tag == "master") {
                              // raster setzen
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

void MasterEdit::writeStatus(int level, Xml& xml) const
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

void MasterEdit::readConfiguration(Xml& xml)
      {
      for (;;) {
            Xml::Token token = xml.parse();
            const QString& tag = xml.s1();
            switch (token) {
                  case Xml::Error:
                  case Xml::End:
                        return;
                  case Xml::TagStart:
                        if (tag == "raster")
                              _rasterInit = xml.parseInt();
                        else if (tag == "width")
                              _widthInit = xml.parseInt();
                        else if (tag == "height")
                              _heightInit = xml.parseInt();
                        else if (tag == "toolbars")
                              _toolbarInit = QByteArray::fromHex(xml.parse1().toAscii());
                        else
                              xml.unknown("MasterEdit");
                        break;
                  case Xml::TagEnd:
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

void MasterEdit::writeConfiguration(int level, Xml& xml)
      {
      xml.tag(level++, "masteredit");
      xml.intTag(level, "raster", _rasterInit);
      xml.intTag(level, "width", _widthInit);
      xml.intTag(level, "height", _heightInit);
      xml.strTag(level, "toolbars", _toolbarInit.toHex().data());
      xml.tag(level, "/masteredit");
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
      }

//---------------------------------------------------------
//   posChanged
//---------------------------------------------------------

void MasterEdit::posChanged(int idx, unsigned val, bool)
      {
      if (idx == 0) {
            int z, n;
            int tempo = tempomap.tempo(val);
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
      if (tick == MAXINT)
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


//---------------------------------------------------------
//   resizeEvent
//---------------------------------------------------------

void MasterEdit::resizeEvent(QResizeEvent* ev)
      {
      QWidget::resizeEvent(ev);
      storeInitialState();
      }

//---------------------------------------------------------
//   focusOutEvent
//---------------------------------------------------------

void MasterEdit::focusOutEvent(QFocusEvent* ev)
      {
      QWidget::focusOutEvent(ev);
      storeInitialState();
      }


//---------------------------------------------------------
//   storeInitialState
//---------------------------------------------------------

void MasterEdit::storeInitialState()
      {
      _widthInit = width();
      _heightInit = height();
      _toolbarInit=saveState();
      }
