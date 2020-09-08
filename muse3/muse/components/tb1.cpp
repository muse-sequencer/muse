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

#include <QToolButton>

#include "tb1.h"
#include "poslabel.h"
#include "pitchlabel.h"
#include "gconfig.h"
#include "raster_widgets.h"

namespace MusEGui {

//---------------------------------------------------------
//   Toolbar1
//    solo time pitch raster
//---------------------------------------------------------

Toolbar1::Toolbar1(RasterizerModel *model, QWidget* parent, int r, bool sp)    
   : QToolBar(QString("Pos/Snap/Solo-tools"), parent)
      {
      setObjectName("Pos/Snap/Solo-tools");
      pitch = nullptr;
      showPitch = sp;

      solo = new QToolButton();    
      solo->setText(tr("Solo"));
      solo->setCheckable(true);
      solo->setFocusPolicy(Qt::NoFocus);
      //solo->setContentsMargins(0,0,0,0);  
      addWidget(solo);

      //---------------------------------------------------
      //  Cursor Position
      //---------------------------------------------------

      QLabel* label = new QLabel(tr("Cursor"));
      label->setIndent(3);
      //label->setContentsMargins(0,0,0,0);  
      addWidget(label);
      pos   = new PosLabel(nullptr, "PosLabel");
      addWidget(pos);
      if (showPitch) {
            pitch = new PitchLabel(nullptr, "PitchLabel");
            pitch->setEnabled(false);
            addWidget(pitch);
            }

      //---------------------------------------------------
      //  Raster
      //---------------------------------------------------

      raster = new RasterLabelCombo(RasterLabelCombo::TableView, model, nullptr, "RasterLabelCombo");
      raster->setFocusPolicy(Qt::TabFocus);

      setRaster(r);
      //setContentsMargins(0,0,0,0);  
      addWidget(raster);
      
      connect(raster, &RasterLabelCombo::rasterChanged, [this](int raster) { _rasterChanged(raster); } );
      connect(solo,   &QToolButton::toggled, [this](bool v) { soloChanged(v); } );
      pos->setEnabled(false);
      }

const Rasterizer *Toolbar1::rasterizer() const
{
  return raster->rasterizer();
}

void Toolbar1::setRasterizerModel(RasterizerModel *model)
{
  raster->setRasterizerModel(model);
}

//---------------------------------------------------------
//   rasterChanged
//---------------------------------------------------------

void Toolbar1::_rasterChanged(int raster)
      {
      emit rasterChanged(raster);
      }


//---------------------------------------------------------
//   currentRaster
//---------------------------------------------------------

int Toolbar1::currentRaster() const
      {
      const QModelIndex mdl_idx = raster->currentModelIndex();
      if(!mdl_idx.isValid())
        return 1;
      return mdl_idx.data(RasterizerModel::RasterValueRole).toInt();
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
      if (val == INT_MAX)
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
        changeRaster(val);
      }

//---------------------------------------------------------
//   changeRaster
//---------------------------------------------------------

int Toolbar1::changeRaster(int val)
      {
        const RasterizerModel* rast_mdl = raster->rasterizerModel();
        const int rast = rast_mdl->checkRaster(val);
        const QModelIndex mdl_idx = rast_mdl->modelIndexOfRaster(rast);
        if(mdl_idx.isValid())
          raster->setCurrentModelIndex(mdl_idx);
        else
          fprintf(stderr, "Toolbar1::changeRaster: rast %d not found in box!\n", rast);
        return rast;
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
