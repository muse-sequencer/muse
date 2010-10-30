//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: tb1.cpp,v 1.3.2.2 2007/01/04 00:35:17 terminator356 Exp $
//  (C) Copyright 1999 Werner Schweer (ws@seh.de)
//=========================================================

#include <assert.h>
#include <stdio.h>
#include <values.h>

//#include <qlabel.h>
//#include <qlayout.h>
//#include <q3listbox.h>
//#include <qtoolbutton.h>
// p4.0.3
#include <QLabel>          
#include <QLayout>
#include <QToolButton>
#include <QTableWidget>    
#include <QListWidget>      

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

//Toolbar1::Toolbar1(QMainWindow* parent, int r, int q, bool sp)
Toolbar1::Toolbar1(QWidget* parent, int r, int q, bool sp)    
   : QToolBar(QString("Quant'n'Snap-tools"), parent)
   //: QToolBar(QString("Qant'n'Snap-tools"), parent)             
      {
      pitch = 0;
      showPitch = sp;
      // ORCAN - FIXME: Check this:
      //setHorizontalStretchable(false);
      //setHorizontalPolicy(QSizePolicy::Minimum);
      //setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Preferred);

      solo = new QToolButton();    
      solo->setText(tr("Solo"));
      solo->setCheckable(true);
      addWidget(solo);

      //---------------------------------------------------
      //  Cursor Position
      //---------------------------------------------------

      //QLabel* label = new QLabel(tr("Cursor"), this, "Cursor");
      QLabel* label = new QLabel(tr("Cursor"));
      label->setAlignment(Qt::AlignRight|Qt::AlignVCenter);
      label->setIndent(3);
      addWidget(label);
      //pos   = new PosLabel(this, "pos");
      pos   = new PosLabel(0, "pos");
      pos->setFixedHeight(22);
      addWidget(pos);
      if (showPitch) {
            //pitch = new PitchLabel(this);
            pitch = new PitchLabel(0);
            pitch->setEnabled(false);
            pitch->setFixedHeight(22);
            addWidget(pitch);
            }

      //---------------------------------------------------
      //  Raster, Quant.
      //---------------------------------------------------

      //raster = new LabelCombo(tr("Snap"), this);
      //quant  = new LabelCombo(tr("Quantize"), this);
      raster = new LabelCombo(tr("Snap"), 0);
      quant  = new LabelCombo(tr("Quantize"), 0);
      //addWidget(raster);
      //addWidget(quant);

      //Q3ListBox* rlist = new Q3ListBox(this);
      //Q3ListBox* qlist = new Q3ListBox(this);
      // p4.0.3
      //QTableWidget* rlist = new QTableWidget(10, 3, this);    
      //QTableWidget* qlist = new QTableWidget(8, 3, this);     
      rlist = new QTableWidget(10, 3);    
      qlist = new QTableWidget(8, 3);     
      //addWidget(rlist);
      //addWidget(qlist);
      rlist->verticalHeader()->setDefaultSectionSize(22);                      
      rlist->horizontalHeader()->setDefaultSectionSize(32);                      
      rlist->setSelectionMode(QAbstractItemView::SingleSelection);                      
      rlist->verticalHeader()->hide();                        
      rlist->horizontalHeader()->hide();                      
      qlist->verticalHeader()->setDefaultSectionSize(22);                      
      qlist->horizontalHeader()->setDefaultSectionSize(32);                      
      qlist->setSelectionMode(QAbstractItemView::SingleSelection);                      
      qlist->verticalHeader()->hide();                        
      qlist->horizontalHeader()->hide();                      
      
      rlist->setMinimumWidth(96);
      qlist->setMinimumWidth(96);
      
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

      addWidget(raster);
      addWidget(quant);
      
      // FIXME: Not working right.
      raster->setFixedHeight(38);
      quant->setFixedHeight(38);
      
      //---------------------------------------------------
      //  To Menu
      //---------------------------------------------------

      /*      
      //LabelCombo* to = new LabelCombo(tr("To"), this);
      LabelCombo* to = new LabelCombo(tr("To"), 0);
      addWidget(to);
//       Q3ListBox* toList = new Q3ListBox(this);
      //QListWidget* toList = new QListWidget(this);       // p4.0.4
      QListWidget* toList = new QListWidget();             
      toList->setFixedHeight(24);
      //addWidget(toList);
//       //to->setListBox(toList); ddskrjo
      to->setView(toList);                               // p4.0.4

//       toList->insertItem(tr("All Events"), 0);
//       toList->insertItem(tr("Looped Ev."),   CMD_RANGE_LOOP);
//       toList->insertItem(tr("Selected Ev."), CMD_RANGE_SELECTED);
//       toList->insertItem(tr("Looped+Sel."),  CMD_RANGE_LOOP | CMD_RANGE_SELECTED);
      // p4.0.4
      toList->insertItem(0, tr("All Events"));
      toList->insertItem(CMD_RANGE_LOOP, tr("Looped Ev."));
      toList->insertItem(CMD_RANGE_SELECTED, tr("Selected Ev."));
      toList->insertItem(CMD_RANGE_LOOP | CMD_RANGE_SELECTED, tr("Looped+Sel."));
      */

      addWidget(new QLabel(tr("To")));
      QComboBox* toList = new QComboBox;
      toList->setFixedHeight(22);
      toList->insertItem(0, tr("All Events"));
      toList->insertItem(CMD_RANGE_LOOP, tr("Looped Ev."));
      toList->insertItem(CMD_RANGE_SELECTED, tr("Selected Ev."));
      toList->insertItem(CMD_RANGE_LOOP | CMD_RANGE_SELECTED, tr("Looped+Sel."));
      addWidget(toList);

      connect(raster, SIGNAL(activated(int)), SLOT(_rasterChanged(int)));
      connect(quant,  SIGNAL(activated(int)), SLOT(_quantChanged(int)));
      //connect(rlist, SIGNAL(cellClicked(int,int)), SLOT(_rasterChanged(int, int)));
      //connect(qlist,  SIGNAL(cellClicked(int,int)), SLOT(_quantChanged(int,int)));
      //connect(to,     SIGNAL(activated(int)), SIGNAL(toChanged(int)));
      connect(toList,     SIGNAL(activated(int)), SIGNAL(toChanged(int)));
      connect(solo,   SIGNAL(toggled(bool)), SIGNAL(soloChanged(bool)));
      pos->setEnabled(false);
      }

//---------------------------------------------------------
//   rasterChanged
//---------------------------------------------------------

void Toolbar1::_rasterChanged(int i)
//void Toolbar1::_rasterChanged(int r, int c)
      {
      emit rasterChanged(rasterTable[rlist->currentRow() + rlist->currentColumn() * 10]);
      //emit rasterChanged(rasterTable[r + c * 10]);
      }

//---------------------------------------------------------
//   quantChanged
//---------------------------------------------------------

void Toolbar1::_quantChanged(int i)
//void Toolbar1::_quantChanged(int r, int c)
      {
      emit quantChanged(quantTable[qlist->currentRow() + qlist->currentColumn() * 8]);
      //emit quantChanged(quantTable[r + c * 8]);
      }

//---------------------------------------------------------
//   setPitch
//---------------------------------------------------------

void Toolbar1::setPitch(int val)
      {
      if (pitch && showPitch) {
            pitch->setEnabled(val != -1);
            pitch->setPitch(val);
            }
      }

void Toolbar1::setInt(int val)
      {
      if (pitch && showPitch) {
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
            //printf("NOT visible\n");
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
                  raster->setCurrentIndex(i);
                  //raster->setModelColumn(i / 10);
                  //raster->setCurrentIndex(i % 10);
                  //rlist->setCurrentCell(i % 10, i / 10);
                  return;
                  }
            }
      printf("setRaster(%d) not defined\n", val);
      //raster->setCurrentIndex(0);
      rlist->setCurrentCell(0, 0);
      }

//---------------------------------------------------------
//   setQuant
//---------------------------------------------------------

void Toolbar1::setQuant(int val)
      {
      for (unsigned i = 0; i < sizeof(quantTable)/sizeof(*quantTable); i++) {
            if (val == quantTable[i]) {
                  quant->setCurrentIndex(i);
                  //quant->setModelColumn(i / 8);
                  //quant->setCurrentIndex(i % 8);
                  //qlist->setCurrentCell(i % 8, i / 8);
                  return;
                  }
            }
      printf("setQuant(%d) not defined\n", val);
      //quant->setCurrentIndex(0);
      qlist->setCurrentCell(0, 0);
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
     // if(pitch)
//        pitch->setPitchMode(flag);
      }

