//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: masteredit.cpp,v 1.4.2.5 2009/07/01 22:14:56 spamatica Exp $
//  (C) Copyright 1999 Werner Schweer (ws@seh.de)
//=========================================================

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
#include "sigedit.h"
#include "globals.h"

#include <values.h>
#include <qtoolbar.h>
#include <qtoolbutton.h>
#include <qtooltip.h>
#include <qaccel.h>
#include <qlayout.h>
#include <qhbox.h>
#include <qsizegrip.h>
#include <qscrollbar.h>
#include <qlabel.h>
#include <qpushbutton.h>
#include <qradiobutton.h>
#include <qbuttongroup.h>
#include <qlistbox.h>
#include <qcombobox.h>
#include <qheader.h>
#include <qpopupmenu.h>
#include <qmenubar.h>
#include <qaction.h>

int MasterEdit::_rasterInit = 0;

//---------------------------------------------------------
//   closeEvent
//---------------------------------------------------------

void MasterEdit::closeEvent(QCloseEvent* e)
      {
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
            sigmap.timesig(song->cpos(), z, n);
            curSig->blockSignals(true);
            curSig->setValue(Sig(z, n));
            curSig->blockSignals(false);
            sign->redraw();
            }
      if (type & SC_MASTER) {
            enableButton->blockSignals(true);
            enableButton->setOn(song->masterFlag());
            enableButton->blockSignals(false);
            }
      }

//---------------------------------------------------------
//   MasterEdit
//---------------------------------------------------------

MasterEdit::MasterEdit()
   : MidiEditor(0, _rasterInit, 0)
      {
      setCaption(tr("MusE: Mastertrack"));
      _raster = 0;      // measure
      setMinimumSize(400, 300);
      resize(500, 350);

      //---------Pulldown Menu----------------------------
//      QPopupMenu* file = new QPopupMenu(this);
//      menuBar()->insertItem("&File", file);

      //---------ToolBar----------------------------------
      tools = new QToolBar(tr("edit tools"), this);
      undoRedo->addTo(tools);

      EditToolBar* tools2 = new EditToolBar(this,
         PointerTool | PencilTool | RubberTool);

      QToolBar* enableMaster = new QToolBar(tr("EnableMaster"), this);
      enableButton = new QToolButton(enableMaster);
      enableButton->setToggleButton(true);
      enableButton->setText(tr("Enable"));
      QToolTip::add(enableButton, tr("Enable usage of master track"));
      enableButton->setOn(song->masterFlag());
      connect(enableButton, SIGNAL(toggled(bool)), song, SLOT(setMasterFlag(bool)));

      QToolBar* info = new QToolBar(tr("Info"), this);
      QLabel* label  = new QLabel(tr("Cursor"), info);
      label->setAlignment(AlignRight|AlignVCenter);
      label->setIndent(3);

      cursorPos = new PosLabel(info);
      tempo = new TempoLabel(info);

      const char* rastval[] = {
            QT_TR_NOOP("Off"), "Bar", "1/2", "1/4", "1/8", "1/16"
            };
      rasterLabel = new LabelCombo(tr("Snap"), info);
	  rasterLabel->setFocusPolicy(QWidget::NoFocus);
      for (int i = 0; i < 6; i++)
            rasterLabel->insertItem(tr(rastval[i]), i);
      rasterLabel->setCurrentItem(1);
      connect(rasterLabel, SIGNAL(activated(int)), SLOT(_setRaster(int)));

      QToolTip::add(cursorPos, tr("time at cursor position"));
      QToolTip::add(tempo, tr("tempo at cursor position"));

      //---------values for current position---------------
      new QLabel(tr("CurPos "), info);
      curTempo = new TempoEdit(info);
      curSig   = new SigEdit(info);
      curSig->setValue(Sig(4, 4));
      QToolTip::add(curTempo, tr("tempo at current position"));
      QToolTip::add(curSig, tr("time signature at current position"));
      connect(curSig, SIGNAL(valueChanged(int,int)), song, SLOT(setSig(int,int)));
      connect(curTempo, SIGNAL(valueChanged(double)), song, SLOT(setTempo(double)));

      //---------------------------------------------------
      //    master
      //---------------------------------------------------

      int xscale = -20;
      int yscale = -500;
      hscroll   = new ScrollScale(-100, -2, xscale, song->len(), Horizontal, mainw);
      vscroll   = new ScrollScale(-1000, -100, yscale, 120000, Vertical, mainw);
      vscroll->setRange(30000, 250000);
      time1     = new MTScale(&_raster, mainw, xscale);
      sign      = new SigScale(&_raster, mainw, xscale);
//      thits     = new HitScale(&_raster, mainw, xscale);

      canvas    = new Master(this, mainw, xscale, yscale);

//      zhits     = new HitScale(&_raster, mainw, xscale);
      time2     = new MTScale(&_raster, mainw, xscale);
      tscale    = new TScale(mainw, yscale);
      time2->setBarLocator(true);

      //---------------------------------------------------
      //    Rest
      //---------------------------------------------------

//      QSizeGrip* corner   = new QSizeGrip(mainw);

      mainGrid->setRowStretch(5, 100);
      mainGrid->setColStretch(1, 100);

      mainGrid->addWidget(hLine(mainw),  0, 1);
      mainGrid->addWidget(time1,         1, 1);
      mainGrid->addWidget(hLine(mainw),  2, 1);
      mainGrid->addWidget(sign,          3, 1);
      mainGrid->addWidget(hLine(mainw),  4, 1);
//    mainGrid->addWidget(thits,         5, 1);
//    mainGrid->addWidget(hLine(mainw),  6, 1);
      mainGrid->addWidget(canvas,        5, 1);
      mainGrid->addWidget(tscale,        5, 0);
      mainGrid->addWidget(hLine(mainw),  6, 1);
//    mainGrid->addWidget(zhits,         9, 1);
//    mainGrid->addWidget(hLine(mainw),  7, 1);
      mainGrid->addWidget(time2,         7, 1);
      mainGrid->addWidget(hscroll,       8, 1);
      mainGrid->addMultiCellWidget(vscroll, 0, 9, 2, 2);
//      mainGrid->addWidget(corner,  9, 2, AlignBottom | AlignRight);

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
      }

//---------------------------------------------------------
//   ~MasterEdit
//---------------------------------------------------------

MasterEdit::~MasterEdit()
      {
      undoRedo->removeFrom(tools);
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
                              rasterLabel->setCurrentItem(item);
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
            sigmap.timesig(val, z, n);
            curTempo->blockSignals(true);
            curSig->blockSignals(true);

            curTempo->setValue(double(60000000.0/tempo));
            curSig->setValue(Sig(z, n));

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

