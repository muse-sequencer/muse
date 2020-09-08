//=========================================================
//  MusE
//  Linux Music Editor
//
//  raster_widgets.cpp
//  Copyright (C) 2020 Tim E. Real (terminator356 on users dot sourceforge dot net)
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

#include "raster_widgets.h"

#include <QHeaderView>
#include <QModelIndex>

namespace MusEGui {

RasterizerTableView::RasterizerTableView(QWidget *parent)
  : QTableView(parent)
{
  verticalHeader()->setDefaultSectionSize(22);
  horizontalHeader()->setDefaultSectionSize(32);
  setSelectionMode(QAbstractItemView::SingleSelection);
  verticalHeader()->hide();
  horizontalHeader()->hide();
}

void RasterizerTableView::reset()
{
  QTableView::reset();

  const QAbstractItemModel *mdl = model();
  if(!mdl)
    return;
  const int cols = mdl->columnCount();
  // Special for row zero ('off'): Span all columns.
  setSpan(0, 0, 1, cols);
  int w = 0;
  for(int i = 0; i < cols; ++i)
  {
    if(isColumnHidden(i))
      continue;
    w += columnWidth(i);
  }
  setMinimumWidth(w);

  //QTableView::reset();
}


//=====================================================================


RasterizerListView::RasterizerListView(QWidget *parent)
  : QListView(parent)
{
  setSelectionMode(QAbstractItemView::SingleSelection);
}


//=====================================================================


RasterizerTreeView::RasterizerTreeView(QWidget *parent)
  : QTreeView(parent)
{
  setSelectionMode(QAbstractItemView::SingleSelection);
}


//=====================================================================


RasterLabelCombo::RasterLabelCombo(RasterComboType type, RasterizerModel *model, QWidget* parent, const char* name)
  : LabelCombo(tr("Snap"), parent, name)
{
//   setFocusPolicy(Qt::TabFocus);
  //setContentsMargins(0,0,0,0);

  // Arbitrarily high value to show everything.
  setMaxVisibleItems(50);
  setSizeAdjustPolicy(QComboBox::AdjustToContents);

  switch(type)
  {
    case ListView:
      rlist = new RasterizerListView();
    break;

    case TableView:
      rlist = new RasterizerTableView();
    break;

    case TreeView:
      rlist = new RasterizerTreeView();
    break;
  }
  
  _rlistModel = model;
  rlist->setModel(_rlistModel);
  setView(rlist);

  connect(this, QOverload<const QModelIndex&>::of(&LabelCombo::activated),
          [=](const QModelIndex& mdl_idx) { rasterActivated(mdl_idx); } );
}

const Rasterizer *RasterLabelCombo::rasterizer() const
{
  return _rlistModel->rasterizer();
}

RasterizerModel *RasterLabelCombo::rasterizerModel() const
{
  return _rlistModel;
}

void RasterLabelCombo::setRasterizerModel(RasterizerModel *model)
{
  if(_rlistModel == model)
    return;
  _rlistModel = model;
  rlist->setModel(_rlistModel);
}

void RasterLabelCombo::rasterActivated(const QModelIndex& mdl_idx)
{
  const int raster = _rlistModel->data(mdl_idx, RasterizerModel::RasterValueRole).toInt();
  if(raster < 0)
    return;
  emit rasterChanged(raster);
}

} // namespace MusEGui
