//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: tb1.cpp,v 1.3.2.2 2007/01/04 00:35:17 terminator356 Exp $
//  (C) Copyright 1999 Werner Schweer (ws@seh.de)
//=========================================================

#include <assert.h>
#include <stdio.h>
#include <values.h>

#include <qlabel.h>
#include <qlayout.h>
#include <q3listbox.h>
#include <qtoolbutton.h>
#include <QTableWidget>    // p4.0.3

#include "config.h"
#include "lcombo.h"
#include "tb1.h"
#include "globals.h"
#include "poslabel.h"
#include "pitchlabel.h"

static int rasterTable[] = {
      //------                8    4     2
      1, 4,  8, 16, 32,  64, 128, 256,  512, 1024,
      1, 6, 12, 24, 48,  96, 192, 384,  768, 1536,
      1, 9, 18, 36, 72, 144, 288, 576, 1152, 2304
      };

static const char* rasterStrings[] = {
      QT_TR_NOOP("Off"), "2pp", "5pp", "64T", "32T", "16T", "8T", "4T", "2T", "1T",
      QT_TR_NOOP("Off"), "3pp", "6pp", "64",  "32",  "16",  "8",  "4",  "2",  "1",
      QT_TR_NOOP("Off"), "4pp", "7pp", "64.", "32.", "16.", "8.", "4.", "2.", "1."
      };

static int quantTable[] = {
      1, 16, 32,  64, 128, 256,  512, 1024,
      1, 24, 48,  96, 192, 384,  768, 1536,
      1, 36, 72, 144, 288, 576, 1152, 2304
      };

static const char* quantStrings[] = {
      QT_TR_NOOP("Off"), "64T", "32T", "16T", "8T", "4T", "2T", "1T",
      QT_TR_NOOP("Off"), "64",  "32",  "16",  "8",  "4",  "2",  "1",
      QT_TR_NOOP("Off"), "64.", "32.", "16.", "8.", "4.", "2.", "1."
      };

//---------------------------------------------------------
//   genToolbar
//    solo time pitch raster quant
//---------------------------------------------------------

Toolbar1::Toolbar1(Q3MainWindow* parent, int r, int q, bool sp)
   : Q3ToolBar(QString("Qant'n'Snap-tools"), parent)
      {
      showPitch = sp;
      setHorizontalStretchable(false);

      solo = new QToolButton(this);
      solo->setText(tr("Solo"));
      solo->setToggleButton(true);

      //---------------------------------------------------
      //  Cursor Position
      //---------------------------------------------------

      QLabel* label = new QLabel(tr("Cursor"), this, "Cursor");
      label->setAlignment(Qt::AlignRight|Qt::AlignVCenter);
      label->setIndent(3);
      pos   = new PosLabel(this, "pos");
      if (showPitch) {
            pitch = new PitchLabel(this);
            pitch->setEnabled(false);
            }

      //---------------------------------------------------
      //  Raster, Quant.
      //---------------------------------------------------

      raster = new LabelCombo(tr("Snap"), this);
      quant  = new LabelCombo(tr("Quantize"), this);

      //Q3ListBox* rlist = new Q3ListBox(this);
      //Q3ListBox* qlist = new Q3ListBox(this);
      QTableWidget* rlist = new QTableWidget(10, 3, this);    // p4.0.3
      QTableWidget* qlist = new QTableWidget(8, 3, this);    //
      rlist->setMinimumWidth(95);
      qlist->setMinimumWidth(95);
      //raster->setListBox(rlist); ddskrjo
      //quant->setListBox(qlist); ddskrjo
      raster->setView(rlist);              // p4.0.3
      quant->setView(qlist);               //
      
      //rlist->setColumnMode(3);  
      //qlist->setColumnMode(3);  
      //for (int i = 0; i < 30; i++)
      //      rlist->insertItem(tr(rasterStrings[i]), i);
      //for (int i = 0; i < 24; i++)
      //      qlist->insertItem(tr(quantStrings[i]), i);
      // p4.0.3
      for (int j = 0; j < 3; j++)                                                 
        for (int i = 0; i < 10; i++)
          rlist->setItem(i, j, new QTableWidgetItem(tr(rasterStrings[i + j * 10])));
      for (int j = 0; j < 3; j++)                           
        for (int i = 0; i < 8; i++)
          qlist->setItem(i, j, new QTableWidgetItem(tr(quantStrings[i + j * 8])));
       
      setRaster(r);
      setQuant(q);

      //---------------------------------------------------
      //  To Menu
      //---------------------------------------------------

      LabelCombo* to = new LabelCombo(tr("To"), this);
//       Q3ListBox* toList = new Q3ListBox(this);
//       //to->setListBox(toList); ddskrjo
//       toList->insertItem(tr("All Events"), 0);
//       toList->insertItem(tr("Looped Ev."),   CMD_RANGE_LOOP);
//       toList->insertItem(tr("Selected Ev."), CMD_RANGE_SELECTED);
//       toList->insertItem(tr("Looped+Sel."),  CMD_RANGE_LOOP | CMD_RANGE_SELECTED);

      connect(raster, SIGNAL(activated(int)), SLOT(_rasterChanged(int)));
      connect(quant,  SIGNAL(activated(int)), SLOT(_quantChanged(int)));
      //connect(raster, SIGNAL(cellActivated(int, int)), SLOT(_rasterChanged(int, int)));  // p4.0.3
      //connect(quant,  SIGNAL(cellActivated(int, int)), SLOT(_quantChanged(int, int)));   //
      connect(to,     SIGNAL(activated(int)), SIGNAL(toChanged(int)));
      connect(solo,   SIGNAL(toggled(bool)), SIGNAL(soloChanged(bool)));
      pos->setEnabled(false);
      }

//---------------------------------------------------------
//   rasterChanged
//---------------------------------------------------------

void Toolbar1::_rasterChanged(int index)
//void Toolbar1::_rasterChanged(int r, int c)    // p4.0.3
      {
      emit rasterChanged(rasterTable[index]);
      //emit rasterChanged(rasterTable[r + c * 10]);    // p4.0.3
      }

//---------------------------------------------------------
//   quantChanged
//---------------------------------------------------------

void Toolbar1::_quantChanged(int index)
//void Toolbar1::_quantChanged(int r, int c)     // p4.0.3
      {
      emit quantChanged(quantTable[index]);
      //emit quantChanged(quantTable[r + c * 10]);
      }

//---------------------------------------------------------
//   setPitch
//---------------------------------------------------------

void Toolbar1::setPitch(int val)
      {
      if (showPitch) {
            pitch->setEnabled(val != -1);
            pitch->setPitch(val);
            }
      }

void Toolbar1::setInt(int val)
      {
      if (showPitch) {
            pitch->setEnabled(val != -1);
            pitch->setInt(val);
            }
      }

//---------------------------------------------------------
//   setTime
//---------------------------------------------------------

void Toolbar1::setTime(unsigned val)
      {
      if (!pos->isVisible()) {
            printf("NOT visible\n");
            return;
            }
      if (val == MAXINT)
            pos->setEnabled(false);
      else {
            pos->setEnabled(true);
            pos->setValue(val);
            }
      }

//---------------------------------------------------------
//   setRaster
//---------------------------------------------------------

void Toolbar1::setRaster(int val)
      {
      for (unsigned i = 0; i < sizeof(rasterTable)/sizeof(*rasterTable); i++) {
            if (val == rasterTable[i]) {
                  raster->setCurrentItem(i);
                  return;
                  }
            }
      printf("setRaster(%d) not defined\n", val);
      raster->setCurrentItem(0);
      }

//---------------------------------------------------------
//   setQuant
//---------------------------------------------------------

void Toolbar1::setQuant(int val)
      {
      for (unsigned i = 0; i < sizeof(quantTable)/sizeof(*quantTable); i++) {
            if (val == quantTable[i]) {
                  quant->setCurrentItem(i);
                  return;
                  }
            }
      printf("setQuant(%d) not defined\n", val);
      quant->setCurrentItem(0);
      }

//---------------------------------------------------------
//   setSolo
//---------------------------------------------------------

void Toolbar1::setSolo(bool flag)
      {
      solo->blockSignals(true);
      solo->setOn(flag);
      solo->blockSignals(false);
      }

//---------------------------------------------------------
//   setPitchMode
//---------------------------------------------------------

void Toolbar1::setPitchMode(bool /*flag*/)
      {
//      pitch->setPitchMode(flag);
      }

