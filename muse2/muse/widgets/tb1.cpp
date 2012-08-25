//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: tb1.cpp,v 1.3.2.2 2007/01/04 00:35:17 terminator356 Exp $
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

#include <stdio.h>
#include <limits.h>

#include <QHeaderView>
#include <QTableWidget>    
#include <QToolButton>

#include "config.h"
#include "lcombo.h"
#include "tb1.h"
#include "globals.h"
#include "poslabel.h"
#include "pitchlabel.h"

namespace MusEGui {

static int rasterTable[] = {
      //------                8    4     2
      1, 4,  8, 16, 32,  64, 128, 256,  512, 1024,
      1, 6, 12, 24, 48,  96, 192, 384,  768, 1536,
      1, 9, 18, 36, 72, 144, 288, 576, 1152, 2304
      };

static const char* rasterStrings[] = {
      QT_TRANSLATE_NOOP("MusEGui::Toolbar1", "Off"), "2pp", "5pp", "64T", "32T", "16T", "8T", "4T", "2T", "1T",
      QT_TRANSLATE_NOOP("MusEGui::Toolbar1", "Off"), "3pp", "6pp", "64",  "32",  "16",  "8",  "4",  "2",  "1",
      QT_TRANSLATE_NOOP("MusEGui::Toolbar1", "Off"), "4pp", "7pp", "64.", "32.", "16.", "8.", "4.", "2.", "1."
      };

static int frameRasterTable[] = {
      1, 2, 4, 8, 16, 32, 64, 128, 256, 512, 1024, 2048, 4096, 
      };
static const char* frameRasterStrings[] = {
      QT_TRANSLATE_NOOP("MusEGui::Toolbar1", "Off"), "2", "4", "8", "16", "32", "64", "128", "256", "512", "1024", "2048", "4096"
      };

// // These need to also be adjustable as well as having preset numbers
// // TODO Minutes, seconds, frames? SMPTE code? 
// static int smpteRasterTable[] = {
//       1, 2, 3, 4, 5, 6, 7, 8   // etc. Simple enumeration, code must determine actual frame
//       };
// static const char* smpteRasterStrings[] = {
//       QT_TRANSLATE_NOOP("MusEGui::Toolbar1", "Off"), "2", "4", "8", "16", "32", "64", "128", "256", "512", "1024", "2048", "4096"
//       };
// // TODO Ticks?
// static int tickRasterTable[] = {
//       1, 2, 3, 4, 5, 6, 7, 8   // etc. Simple enumeration, code must determine actual frame
//       };
// static const char* tickRasterStrings[] = {
//       QT_TRANSLATE_NOOP("MusEGui::Toolbar1", "Off"), "2", "4", "8", "16", "32", "64", "128", "256", "512", "1024", "2048", "4096"
//       //QT_TRANSLATE_NOOP("MusEGui::Toolbar1", "Off"), "15", "24", "25", "29", "30", "50", "60", "90", "100"
//       };
// 

//---------------------------------------------------------
//   genToolbar
//    solo time pitch raster
//---------------------------------------------------------

Toolbar1::Toolbar1(QWidget* parent, int r, bool sp, MusECore::Pos::TType time_type)    
   : QToolBar(QString("Pos/Snap/Solo-tools"), parent)
      {
      setObjectName("Pos/Snap/Solo-tools");
      
      pitch = 0;
      showPitch = sp;
      // ORCAN - FIXME: Check this:
      //setHorizontalStretchable(false);
      //setHorizontalPolicy(QSizePolicy::Minimum);
      //setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Preferred);

      solo = new QToolButton();    
      solo->setText(tr("Solo"));
      solo->setCheckable(true);
      solo->setFocusPolicy(Qt::NoFocus);
      addWidget(solo);

      //---------------------------------------------------
      //  Cursor Position
      //---------------------------------------------------

      QLabel* label = new QLabel(tr("Cursor"));
      label->setAlignment(Qt::AlignRight|Qt::AlignVCenter);
      label->setIndent(3);
      addWidget(label);
      pos   = new PosLabel(0, "pos");
      ///pos->setFixedHeight(22);
      addWidget(pos);
      if (showPitch) {
            pitch = new PitchLabel(0);
            pitch->setEnabled(false);
            ///pitch->setFixedHeight(22);
            addWidget(pitch);
            }

      //---------------------------------------------------
      //  Raster
      //---------------------------------------------------

      raster = new LabelCombo(tr("Snap"), 0);
      raster->setFocusPolicy(Qt::TabFocus);

      rlist = new QTableWidget(10, 3);    
      ///rlist->verticalHeader()->setDefaultSectionSize(22);                      
      ///rlist->horizontalHeader()->setDefaultSectionSize(32);                      
      rlist->setSelectionMode(QAbstractItemView::SingleSelection);                      
      rlist->verticalHeader()->hide();                        
      rlist->horizontalHeader()->hide();                      
      
      rlist->setMinimumWidth(96);
      
      raster->setView(rlist);              
      
      ///for (int j = 0; j < 3; j++)                                                 
      ///  for (int i = 0; i < 10; i++)
      ///    rlist->setItem(i, j, new QTableWidgetItem(tr(rasterStrings[i + j * 10])));
       
      setTimeType(time_type);
      
      setRaster(r);

      addWidget(raster);
      
      // FIXME: Not working right.
      ///raster->setFixedHeight(38);
      
      connect(raster, SIGNAL(activated(int)), SLOT(_rasterChanged(int)));
      connect(solo,   SIGNAL(toggled(bool)), SIGNAL(soloChanged(bool)));
      pos->setEnabled(false);
      }

      
//---------------------------------------------------------
//   setRasterTimeType
//---------------------------------------------------------

void Toolbar1::setRasterTimeType()
{
  rlist->clear();
  switch(_timeType) 
  {
    case MusECore::Pos::FRAMES:
      rlist->setColumnCount(1);
      rlist->setRowCount(13);
      rlist->verticalHeader()->setDefaultSectionSize(22);
      rlist->horizontalHeader()->setDefaultSectionSize(32);
      
      for (int j = 0; j < 1; j++)
        for (int i = 0; i < 12; i++)
          rlist->setItem(i, j, new QTableWidgetItem(tr(frameRasterStrings[i + j * 13])));
      break;    
    case MusECore::Pos::TICKS:
      rlist->clear();
      rlist->setColumnCount(3);
      rlist->setRowCount(10);
      rlist->verticalHeader()->setDefaultSectionSize(22);                      
      rlist->horizontalHeader()->setDefaultSectionSize(32);                      
      
      for (int j = 0; j < 3; j++)                                                 
        for (int i = 0; i < 10; i++)
          rlist->setItem(i, j, new QTableWidgetItem(tr(rasterStrings[i + j * 10])));
      break;    
  }
}

//---------------------------------------------------------
//   setTimeType
//---------------------------------------------------------

void Toolbar1::setTimeType(MusECore::Pos::TType tt)
{
  _timeType = tt;
  pos->setTimeType(_timeType);
  setRasterTimeType();
}
      
//---------------------------------------------------------
//   rasterChanged
//---------------------------------------------------------

void Toolbar1::_rasterChanged(int /*i*/)
//void Toolbar1::_rasterChanged(int r, int c)
      {
      // REMOVE Tim.  
      ///emit rasterChanged(rasterTable[rlist->currentRow() + rlist->currentColumn() * 10]);
      
      switch(_timeType)
      { 
        case MusECore::Pos::FRAMES:
          emit rasterChanged(frameRasterTable[rlist->currentRow() + rlist->currentColumn() * 13]);
        break;  
        case MusECore::Pos::TICKS:
          emit rasterChanged(rasterTable[rlist->currentRow() + rlist->currentColumn() * 10]);
        break;  
      }
      
      //parentWidget()->setFocus();
      //emit rasterChanged(rasterTable[r + c * 10]);
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

//void Toolbar1::setTime(unsigned val)   // REMOVE Tim.
void Toolbar1::setTime(const MusECore::Pos& val)
      {
      if (!pos->isVisible()) {
            //printf("NOT visible\n");
            return;
            }
      //if (val == INT_MAX)
      if (val.nullFlag())
            pos->setEnabled(false);
      else {
            pos->setEnabled(true);
            pos->setValue(val);
            //pos->setValue(val.pos(_timeType));
            }
      }

//---------------------------------------------------------
//   setRaster
//---------------------------------------------------------

void Toolbar1::setRaster(int val)
      {
      switch(_timeType)
      {
        case MusECore::Pos::FRAMES:
          for (unsigned i = 0; i < sizeof(frameRasterTable)/sizeof(*frameRasterTable); i++) {
                if (val == frameRasterTable[i]) {
                      raster->setCurrentIndex(i);
                      return;
                      }
                }
          break;      
        case MusECore::Pos::TICKS:
          for (unsigned i = 0; i < sizeof(rasterTable)/sizeof(*rasterTable); i++) {
                if (val == rasterTable[i]) {
                      raster->setCurrentIndex(i);
                      return;
                      }
                }
          break;      
      }        
      printf("setRaster(%d) not defined\n", val);
      raster->setCurrentIndex(0);
      }

//---------------------------------------------------------
//   setSolo
//---------------------------------------------------------

void Toolbar1::setSolo(bool flag)
      {
      solo->blockSignals(true);
      solo->setChecked(flag);
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

} // namespace MusEGui
