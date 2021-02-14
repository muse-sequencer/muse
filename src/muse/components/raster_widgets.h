//=========================================================
//  MusE
//  Linux Music Editor
//
//  raster_widgets.h
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

#ifndef __RASTER_WIDGETS_H__
#define __RASTER_WIDGETS_H__

#include <QListView>
#include <QTableView>
#include <QTreeView>

#include "lcombo.h"
#include "rasterizer.h"

class QModelIndex;

namespace MusEGui {

class RasterizerListView : public QListView
{
  public:
    RasterizerListView(QWidget *parent = nullptr);
};


//===================================================


class RasterizerTableView : public QTableView
{
  private:
    Q_OBJECT

  public slots:
    void reset() override;
    
  public:
    RasterizerTableView(QWidget *parent = nullptr);
};


//===================================================


class RasterizerTreeView : public QTreeView
{
  public:
    RasterizerTreeView(QWidget *parent = nullptr);
};


//===================================================


class RasterLabelCombo : public LabelCombo
{
  private:
    Q_OBJECT

  public:
    enum RasterComboType { ListView, TableView, TreeView };

  private slots:
    void rasterActivated(const QModelIndex&);
    
  private:
    QAbstractItemView *rlist;
    RasterizerModel *_rlistModel;

  signals:
    void rasterChanged(int raster);

  public:
    RasterLabelCombo(RasterComboType type, RasterizerModel *model, QWidget* parent, const char* name=0);

    const Rasterizer *rasterizer() const;
    RasterizerModel *rasterizerModel() const;
    void setRasterizerModel(RasterizerModel *model);
};

} // namespace MusEGui

#endif
