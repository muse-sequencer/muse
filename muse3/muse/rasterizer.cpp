//=========================================================
//  MusE
//  Linux Music Editor
//
//  rasterizer.cpp
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

#include "sig.h"
#include "rasterizer.h"
#include "globals.h"

namespace MusEGui {

Rasterizer::Rasterizer(int division, QObject *parent) 
  : QObject(parent), _division(division)
{
  _rows = 0;
  _rasterArray = nullptr;
  updateRasterizer();
}

Rasterizer::~Rasterizer()
{
  if(_rasterArray)
    delete [] _rasterArray;
  _rasterArray = nullptr;
}

int Rasterizer::columnCount() const
{
  return 3;
}

void Rasterizer::updateColumn(Column col)
{
  int div, row, d;

  const int col_num = col;
  const int col_offset = col_num * _rows;
  // For the 'off' row.
  _rasterArray[col_offset] = 1;
  // For the 'bar' row.
  _rasterArray[col_offset + _rows - 1] = 0;

  if(col == TripletColumn) {
    d = _division * 4 * 2;
    // Can't use it if it has a remainder.
    if(d % 3 != 0)
      return;
    div = d / 3;
    }
  else if(col == NormalColumn)
    div = _division * 4;
  else if(col == DottedColumn) {
    d = _division * 4 * 3;
    // Can't use it if it has a remainder.
    if(d % 2 != 0)
      return;
    div = d / 2;
    }
    
  // Set all rows, except 'off' and 'bar' which are already done above.
  for(row = _rows - 2; row >= 1; --row)
  {
    _rasterArray[col_offset + row] = div;
    // Can't use it if it has a remainder.
    if(div % 2 != 0)
      break;
    div /= 2;
  }
}

void Rasterizer::updateRasterizer()
{
  emit dataAboutToBeReset();

  if(_rasterArray)
    delete [] _rasterArray;
  _rasterArray = nullptr;
  _rows = 0;

  int num_cols = columnCount();
  if(num_cols > 0)
  {
    // The actual number of rows displayed depends on the division value.
    // The actual number and the maximum will always be at least 5: Bar, off, whole, half,
    //  and quarter notes are always available, regardless of division value.
    // The other two columns, triplet (x2/3) and dotted (x3/2), may contain up to the
    //  same rows as the centre, but some values may be missing if they have a remainder -
    //  values are unusable if they have a remainder after the divisions.

    int div = _division;
    int rows = 5;
    while(1)
    {
      // Must be integral values, no remainder.
      if(div % 2 != 0)
        break;
      div /= 2;
      // Can't go any lower than 6 due to the triplet (x2/3) column.
      if(div < 6)
        break;
      ++rows;
    }
    _rows = rows;

    const int num_rasters = _rows * num_cols;
    _rasterArray = new int[num_rasters];
    // Initalize the array elements to -1 (invalid).
    for(int i = 0; i < num_rasters; ++i)
      _rasterArray[i] = -1;

    updateColumn(TripletColumn);
    updateColumn(NormalColumn);
    updateColumn(DottedColumn);
  }
  
  emit dataReset();
}

void Rasterizer::setDivision(int div)
{
  if(_division == div)
    return;
  _division = div;
  updateRasterizer();
}

//---------------------------------------------------------
//   indexOf
//---------------------------------------------------------

int Rasterizer::indexOf(int val) const
{
  int idx;
  const int cols = columnCount();
  for(int row = 0; row < _rows; ++row)
  {
    for(int col = 0; col < cols; ++col)
    {
      idx = row + col * _rows;
      if(_rasterArray[idx] == val)
        return idx;
    }
  }
  // Raster was not found. Return -1.
  return -1;
}

int Rasterizer::checkRaster(int val) const
{
  int rast;
  const int cols = columnCount();
  for(int row = 0; row < _rows; ++row)
  {
    for(int col = 0; col < cols; ++col)
    {
      rast = _rasterArray[row + col * _rows];
      if(rast == val)
        return rast;
    }
  }
  // No suitable raster was found. Just return 1 division (1 quarter note).
  return _division;
}

int Rasterizer::rasterAt(int row, int col) const
{
  if(row >= _rows || col >= columnCount() || !_rasterArray)
    return -1;
  return _rasterArray[row + col * _rows];
}

bool Rasterizer::isBarRaster(int row, int col) const 
{ 
  return rasterAt(row, col) == 0;
}

bool Rasterizer::isOffRaster(int row, int col) const 
{ 
  return rasterAt(row, col) == 1;
}

int Rasterizer::barRow() const
{
  return rowCount() - 1;
}

int Rasterizer::offRow() const
{
  return 0;
}

int Rasterizer::commonRaster(CommonRasters commonRast) const
{
  const int rows = rowCount();
  int inv_row = 0;
  switch(commonRast)
  {
    case CommonRasterBar:
      return 0;
    break;
    case CommonRasterOff:
      return 1;
    break;
    case CommonRaster1:
      inv_row = 2;
    break;
    case CommonRaster2:
      inv_row = 3;
    break;
    case CommonRaster4:
      inv_row = 4;
    break;
    case CommonRaster8:
      inv_row = 5;
    break;
    case CommonRaster16:
      inv_row = 6;
    break;
    case CommonRaster32:
      inv_row = 7;
    break;
    case CommonRaster64:
      inv_row = 8;
    break;
  }
  const int row = rows - inv_row;
  if(row >= 0 && row != offRow() && row != barRow())
    return rasterAt(row, NormalColumn);
  return -1;
}

bool Rasterizer::isLessThanNormalRaster(int row, int col, int normalRaster) const 
{ 
  const int rast = rasterAt(row, col);
  if(rast < 0)
    return true;
  // The last row is for 'bar'.
  if(rast == 0)
    return false;

  switch(col)
  {
    case TripletColumn:
      return rast < (normalRaster * 2) / 3;
    break;

    case NormalColumn:
      return rast < normalRaster;
    break;

    case DottedColumn:
      return rast < (normalRaster * 3) / 2;
    break;
  }
  
  return true;
}

int Rasterizer::rasterDenomAt(int row) const
{
  // The first row is for 'off'.
  if(row == 0)
    return 0;
  const int row_count = rowCount();
  // The last row is for 'bar'.
  if(row == row_count - 1)
    return 0;
  return 1 << (row_count - row - 2);
}


// =========================================================================



RasterizerModel::RasterizerModel(
  Rasterizer *rasterizer, QObject *parent, int max_rows,
  QList<Rasterizer::Column> visible_columns, DisplayFormat displayFormat)
  : QAbstractTableModel(parent), _rasterizer(rasterizer), _displayFormat(displayFormat)
{
  setVisibleColumns(visible_columns);
  setMaxRows(max_rows);

  // Receive signals to satisfy Qt's model requirement of being informed
  //  when the data is about to change and has changed.
  _dataAboutToBeResetConnection = 
    connect(_rasterizer, &Rasterizer::dataAboutToBeReset, [this]() { beginResetModel(); } );
  _dataResetConnection = 
    connect(_rasterizer, &Rasterizer::dataReset, [this]() { endResetModelHandler(); } );
}

RasterizerModel::~RasterizerModel()
{
  disconnect(_dataAboutToBeResetConnection);
  disconnect(_dataResetConnection);
}

void RasterizerModel::endResetModelHandler()
{
  //endResetModel(); ???
  updateRows();
  endResetModel();
}

void RasterizerModel::updateRows()
{
  _modelToRasterRowList.clear();
  _rasterToModelRowMap.clear();
  const int mdl_row_count = rowCount();
  const int rast_row_count = _rasterizer->rowCount();
  if(mdl_row_count <= 0 || rast_row_count <= 0)
    return;

  // There is always a row zero - the 'off' row.
  _modelToRasterRowList.append(0);
  _rasterToModelRowMap.insert(0, 0);

  int rast_row = rast_row_count - mdl_row_count + 1;
  for(int mdl_row = 1; mdl_row < mdl_row_count; ++mdl_row, ++rast_row)
  {
    _modelToRasterRowList.append(rast_row);
    _rasterToModelRowMap.insert(rast_row, mdl_row);
  }
}

int RasterizerModel::modelToRasterRow(int row) const
{
  if(row >= _modelToRasterRowList.size())
    return -1;
  return _modelToRasterRowList.at(row);
}

Rasterizer::Column RasterizerModel::modelToRasterCol(int col) const
{
  if(col >= _modelToRasterColumnList.size())
    return Rasterizer::InvalidColumn;
  return _modelToRasterColumnList.at(col);
}

QString RasterizerModel::textAt(int row, int col) const
{
  const int rast_row = modelToRasterRow(row);
  if(rast_row < 0)
    return QString();

  const Rasterizer::Column rast_col = modelToRasterCol(col);
  if(rast_col == Rasterizer::InvalidColumn)
    return QString();

  // All columns of row zero say 'off'.
  if(_rasterizer->isOffRaster(rast_row, rast_col))
    return QT_TRANSLATE_NOOP("MusECore::RasterizerModel", "Off");
  
  // All columns of the last row say 'Bar'.
  if(_rasterizer->isBarRaster(rast_row, rast_col))
    return QT_TRANSLATE_NOOP("MusECore::RasterizerModel", "Bar");
  
  const int rast = _rasterizer->rasterAt(rast_row, rast_col);
  if(rast < 0)
    return QString();

  // Is the raster value length less than a triplet, normal, or dotted 64th note?
  const bool show_tick = _rasterizer->isLessThanNormalRaster(rast_row, rast_col, _rasterizer->division() / 16);

  if(show_tick)
    return QString("%1tk").arg(rast);
  else
  {
    const int denom_num = _rasterizer->rasterDenomAt(rast_row);
    QString s;
    switch(_displayFormat)
    {
      case FractionFormat:
          s += QString("1/%1").arg(denom_num);
      break;

      case DenominatorFormat:
        s += QString("%1").arg(denom_num);
      break;
    }
    if(rast_col == Rasterizer::TripletColumn)
      s += QString("T");
    else if(rast_col == Rasterizer::DottedColumn)
      s += QString(".");
    return s;
  }
}

int RasterizerModel::rasterAt(int row, int col) const
{
  const int rast_row = modelToRasterRow(row);
  if(rast_row < 0)
    return -1;

  Rasterizer::Column rast_col = modelToRasterCol(col);
  if(rast_col == Rasterizer::InvalidColumn)
    return -1;

  return _rasterizer->rasterAt(rast_row, rast_col);
}

void RasterizerModel::setRasterizer(const Rasterizer *r)
{
  if(_rasterizer == r)
    return;

  disconnect(_dataAboutToBeResetConnection);
  disconnect(_dataResetConnection);

  beginResetModel();
  _rasterizer = r;
  updateRows();
  endResetModel();

  // Receive signals to satisfy Qt's model requirement of being informed
  //  when the data is about to change and has changed.
  _dataAboutToBeResetConnection = 
    connect(_rasterizer, &Rasterizer::dataAboutToBeReset, [this]() { beginResetModel(); } );
  _dataResetConnection = 
    connect(_rasterizer, &Rasterizer::dataReset, [this]() { endResetModelHandler(); } );
}

int RasterizerModel::division() const
{ 
  return _rasterizer->division();
}

int RasterizerModel::maxRows() const
{
  return _maxRows;
}

void RasterizerModel::setMaxRows(int rows)
{
  if(_maxRows == rows)
    return;
  beginResetModel();
  _maxRows = rows;
  updateRows();
  endResetModel();
}

QList<int /*rasterRow*/> RasterizerModel::visibleRows() const
{
  return _modelToRasterRowList;
}

QList<Rasterizer::Column> RasterizerModel::visibleColumns() const
{
  return _modelToRasterColumnList;
}

void RasterizerModel::setVisibleColumns(const QList<Rasterizer::Column>& cols)
{
  beginResetModel();

  _modelToRasterColumnList = cols;

  _rasterToModelColumnMap.clear();
  const int sz = _modelToRasterColumnList.size();
  for(int i = 0; i < sz; ++i)
    _rasterToModelColumnMap.insert(_modelToRasterColumnList.at(i), i);

  updateRows();
  endResetModel();
}

RasterizerModel::DisplayFormat RasterizerModel::displayFormat() const
{
 return _displayFormat;  
}

void RasterizerModel::setDisplayFormat(DisplayFormat format)
{
  beginResetModel();
  _displayFormat = format;
  endResetModel();
}

int RasterizerModel::indexOfRaster(int val) const
{
  const int mdl_rows = _modelToRasterRowList.size();
  const int mdl_cols = _modelToRasterColumnList.size();
  int rast_row, rast_col;
  for(int mdl_row = 0; mdl_row < mdl_rows; ++mdl_row)
  {
    rast_row = _modelToRasterRowList.at(mdl_row);
    for(int mdl_col = 0; mdl_col < mdl_cols; ++mdl_col)
    {
      rast_col = _modelToRasterColumnList.at(mdl_col);
      if(_rasterizer->rasterAt(rast_row, rast_col) == val)
        return mdl_row + mdl_col * mdl_rows;
    }
  }
  return -1;
}

QModelIndex RasterizerModel::modelIndexOfRaster(int val) const
{
  const int mdl_rows = _modelToRasterRowList.size();
  const int mdl_cols = _modelToRasterColumnList.size();
  int rast_row, rast_col;
  for(int mdl_row = 0; mdl_row < mdl_rows; ++mdl_row)
  {
    rast_row = _modelToRasterRowList.at(mdl_row);
    for(int mdl_col = 0; mdl_col < mdl_cols; ++mdl_col)
    {
      rast_col = _modelToRasterColumnList.at(mdl_col);
      if(_rasterizer->rasterAt(rast_row, rast_col) == val)
        return index(mdl_row, mdl_col);
    }
  }
  return QModelIndex();
}

int RasterizerModel::checkRaster(int val) const
{
  const int mdl_rows = _modelToRasterRowList.size();
  const int mdl_cols = _modelToRasterColumnList.size();
  int rast, rast_row, rast_col;
  for(int mdl_row = 0; mdl_row < mdl_rows; ++mdl_row)
  {
    rast_row = _modelToRasterRowList.at(mdl_row);
    for(int mdl_col = 0; mdl_col < mdl_cols; ++mdl_col)
    {
      rast_col = _modelToRasterColumnList.at(mdl_col);
      rast = _rasterizer->rasterAt(rast_row, rast_col);
      if(rast == val)
        return rast;
    }
  }
  // No suitable raster was found. Just return 1 division (1 quarter note).
  return _rasterizer->division();
}

int RasterizerModel::barRow() const
{
  const int rast_bar_row = _rasterizer->barRow();
  QMap<int /*rasterRow*/, int /*modelRow*/>::const_iterator imbr =
    _rasterToModelRowMap.find(rast_bar_row);
  if(imbr == _rasterToModelRowMap.constEnd())
    return -1;
  return imbr.value();
}

int RasterizerModel::offRow() const
{
  const int rast_off_row = _rasterizer->offRow();
  QMap<int /*rasterRow*/, int /*modelRow*/>::const_iterator imbr =
    _rasterToModelRowMap.find(rast_off_row);
  if(imbr == _rasterToModelRowMap.constEnd())
    return -1;
  return imbr.value();
}

bool RasterizerModel::isBarRaster(int row, int col) const 
{ 
  const int rast_row = modelToRasterRow(row);
  if(rast_row < 0)
    return false;

  const Rasterizer::Column rast_col = modelToRasterCol(col);
  if(rast_col == Rasterizer::InvalidColumn)
    return false;

  return _rasterizer->isBarRaster(rast_row, rast_col);
}

bool RasterizerModel::isOffRaster(int row, int col) const 
{ 
  const int rast_row = modelToRasterRow(row);
  if(rast_row < 0)
    return false;

  const Rasterizer::Column rast_col = modelToRasterCol(col);
  if(rast_col == Rasterizer::InvalidColumn)
    return false;

  return _rasterizer->isOffRaster(rast_row, rast_col);
}

int RasterizerModel::commonRaster(Rasterizer::CommonRasters commonRast) const
{
  const int rows = rowCount();
  int inv_row = 0;
  switch(commonRast)
  {
    case Rasterizer::CommonRasterBar:
      return 0;
    break;
    case Rasterizer::CommonRasterOff:
      return 1;
    break;
    case Rasterizer::CommonRaster1:
      inv_row = 2;
    break;
    case Rasterizer::CommonRaster2:
      inv_row = 3;
    break;
    case Rasterizer::CommonRaster4:
      inv_row = 4;
    break;
    case Rasterizer::CommonRaster8:
      inv_row = 5;
    break;
    case Rasterizer::CommonRaster16:
      inv_row = 6;
    break;
    case Rasterizer::CommonRaster32:
      inv_row = 7;
    break;
    case Rasterizer::CommonRaster64:
      inv_row = 8;
    break;
  }
  const int row = rows - inv_row;
  if(row >= 0 && row != offRow() && row != barRow())
    return rasterAt(row, Rasterizer::NormalColumn);
  return -1;
}

int RasterizerModel::pickRaster(int raster, RasterPick pick) const
{
  const QModelIndex mdl_idx = modelIndexOfRaster(raster);
  if(!mdl_idx.isValid())
    return raster;

  const int mdl_row = mdl_idx.row();
  const int mdl_col = mdl_idx.column();

  const int mdl_rows = rowCount();

  const bool has_triple_col =
    _rasterToModelColumnMap.find(Rasterizer::TripletColumn) != _rasterToModelColumnMap.constEnd();
  const bool has_normal_col =
    _rasterToModelColumnMap.find(Rasterizer::NormalColumn) != _rasterToModelColumnMap.constEnd();
  const bool has_dotted_col =
    _rasterToModelColumnMap.find(Rasterizer::DottedColumn) != _rasterToModelColumnMap.constEnd();

  const bool is_off = isOffRaster(mdl_row, mdl_col);
  const bool is_bar = isBarRaster(mdl_row, mdl_col);
  const int off_row = offRow();
  const int bar_row = barRow();

  int new_mdl_row = mdl_row;
  int new_mdl_col = mdl_col;
  int new_raster = -1;
  switch(pick)
  {
    case ToggleTriple:
      // Special for 'off' and 'bar' rows.
      if(is_off || is_bar)
        return raster;
      if(mdl_col == Rasterizer::TripletColumn && has_normal_col)
        new_mdl_col = Rasterizer::NormalColumn;
      else if(has_triple_col) 
        new_mdl_col = Rasterizer::TripletColumn;
    break;

    case ToggleDotted:
      if(is_off || is_bar)
        return raster;
      if(mdl_col == Rasterizer::DottedColumn && has_normal_col)
        new_mdl_col = Rasterizer::NormalColumn;
      else if(has_dotted_col) 
        new_mdl_col = Rasterizer::DottedColumn;
    break;

    case ToggleHigherDotted:
      if(is_off || is_bar)
        return raster;
      if(mdl_col == Rasterizer::DottedColumn && has_normal_col)
      {
        // Exclude 'off' and 'bar' rows.
        const int nrow = new_mdl_row - 1;
        if(nrow >= 0 && nrow != off_row && nrow != bar_row)
        {
          --new_mdl_row;
          new_mdl_col = Rasterizer::NormalColumn;
        }
      }
      else if(has_dotted_col)
      {
        const int nrow = new_mdl_row + 1;
        if(nrow < mdl_rows && nrow != off_row && nrow != bar_row)
        {
          new_mdl_col = Rasterizer::DottedColumn;
          ++new_mdl_row;
        }
      }
    break;

    case GotoBar:
      new_raster = _rasterizer->commonRaster(Rasterizer::CommonRasterBar);
    break;

    case GotoOff:
      new_raster = _rasterizer->commonRaster(Rasterizer::CommonRasterOff);
    break;

    case Goto1:
      new_raster = _rasterizer->commonRaster(Rasterizer::CommonRaster1);
    break;

    case Goto2:
      new_raster = _rasterizer->commonRaster(Rasterizer::CommonRaster2);
    break;

    case Goto4:
      new_raster = _rasterizer->commonRaster(Rasterizer::CommonRaster4);
    break;

    case Goto8:
      new_raster = _rasterizer->commonRaster(Rasterizer::CommonRaster8);
    break;

    case Goto16:
      new_raster = _rasterizer->commonRaster(Rasterizer::CommonRaster16);
    break;

    case Goto32:
      new_raster = _rasterizer->commonRaster(Rasterizer::CommonRaster32);
    break;

    case Goto64:
      new_raster = _rasterizer->commonRaster(Rasterizer::CommonRaster64);
    break;

    case NoPick:
    break;
  }

  // Was there a new raster?
  if(new_raster >= 0)
    return new_raster;
  // Otherwise was there no new model row and column?
  if(new_mdl_row == mdl_row && new_mdl_col == mdl_col)
    return raster;
  // Return the raster at the new model row and column
  return rasterAt(new_mdl_row, new_mdl_col);
}

int RasterizerModel::rowCount(const QModelIndex &/*parent*/) const
{
  const int rast_rows = _rasterizer->rowCount();
  // Ignore _maxRows if -1.
  if(_maxRows >= 0 && rast_rows > _maxRows)
    return _maxRows;
  return rast_rows;
}

int RasterizerModel::columnCount(const QModelIndex &/*parent*/) const
{
  return _modelToRasterColumnList.size();
}

QVariant RasterizerModel::data(const QModelIndex &index, int role) const
{
  const int row = index.row();
  const int col = index.column();
  if(role == RasterTextRole)
    return textAt(row, col);
  else if(role == RasterValueRole)
    return rasterAt(row, col);
  else if(role == Qt::TextAlignmentRole)
  {
    // Special for 'off' and 'bar' rows: In the view we set them to span all columns.
    // Don't bother aligning if there is only one column (it just looks a little weird).
    if(columnCount() > 1 && (row == offRow() || row == barRow()))
      return Qt::AlignCenter;
  }
  
  return QVariant();
}

} // namespace MusEGui
