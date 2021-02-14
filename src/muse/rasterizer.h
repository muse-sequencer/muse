//=========================================================
//  MusE
//  Linux Music Editor
//
//  rasterizer.h
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

#ifndef __RASTERIZER_H__
#define __RASTERIZER_H__

#include <QAbstractTableModel>
#include <QModelIndex>
#include <QVariant>
#include <QString>
#include <QObject>
#include <QMetaObject>
#include <QList>
#include <QMap>
#include <QSize>

namespace MusEGui {
  
class Rasterizer : public QObject {
  private:
        Q_OBJECT

  public:
      enum Column { 
        InvalidColumn = -1,
        TripletColumn =  0,
        NormalColumn  =  1,
        DottedColumn  =  2
        };

      enum CommonRasters {
         CommonRasterBar, CommonRasterOff, CommonRaster1, CommonRaster2, CommonRaster4,
         CommonRaster8, CommonRaster16, CommonRaster32, CommonRaster64
        };

  private:
      int _division;          // System midi division (ticks per quarter note) setting.
      int _rows;              // Current number of rows in the raster array.
      int *_rasterArray;      // Two-dimensional array of raster values. -1 = invalid. 0 = Snap to bar. 1 = 'off'.

      void updateColumn(Column col);

  signals:
      // This signal is emitted just before the data will be rebuilt.
      void dataAboutToBeReset();
      // This signal is emitted just after the data has been rebuilt.
      void dataReset();

  public:
      Rasterizer(int division, QObject *parent = nullptr);
      ~Rasterizer();

      int division() const { return _division; }
      void setDivision(int div);

      int rowCount() const { return _rows; }

      // Returns the number of columns.
      int columnCount() const;

      // Rebuilds the raster array.
      void updateRasterizer();

      // Returns the given raster, or returns 1 division (1 quarter note) if no suitable raster could be found.
      int checkRaster(int val) const;

      // Returns the index into the array of the given raster value, or -1 if raster not found.
      int indexOf(int val) const;
      // Returns raster value at given row and column, or -1 if no raster
      //  at that location, or row or column are out of range.
      int rasterAt(int row, int col) const;

      // Returns true if the raster at the given row and column is 0 (snap to bar).
      bool isBarRaster(int row, int col) const;
      // Returns true if the raster at the given row and column is 1 ('off').
      bool isOffRaster(int row, int col) const;
      // Returns the row number of the 'bar' row.
      int barRow() const;
      // Returns the row number of the 'off' row.
      int offRow() const;
      // Returns the raster value of some often-used denominator values.
      // Returns -1 if the raster is not available.
      int commonRaster(CommonRasters commonRast) const;
      // Returns true if the raster at the given row and column is less than
      //  a triple, normal, or dotted version of the given normal raster value.
      bool isLessThanNormalRaster(int row, int col, int normalRaster) const;
      // Returns a denominator value for the row, suitable for display (1 2 4 8 16 32 etc.)
      // Returns zero if the row is a 'bar' row.
      int rasterDenomAt(int row) const;
};

class RasterizerModel : public QAbstractTableModel
{
  private:
    Q_OBJECT

  public:
    enum Roles { RasterTextRole = Qt::DisplayRole, RasterValueRole = Qt::UserRole};
    enum DisplayFormat { FractionFormat, DenominatorFormat };
    enum RasterPick { NoPick,
      ToggleTriple, ToggleDotted, ToggleHigherDotted,
      GotoBar, GotoOff, Goto1, Goto2, Goto4, Goto8, Goto16, Goto32, Goto64 };

  private:
    // The external rasterizer array used in this model.
    const Rasterizer *_rasterizer;
    // Maximum number of rows. If set to -1, all rows are included.
    int _maxRows = 0;
    // How text is displayed.
    DisplayFormat _displayFormat;

    // Lookup lists for model <> raster rows and columns.
    QList<int /*rasterRow*/> _modelToRasterRowList;
    QMap<int /*rasterRow*/, int /*modelRow*/> _rasterToModelRowMap;
    QList<Rasterizer::Column> _modelToRasterColumnList;
    QMap<Rasterizer::Column, int /*modelColumn*/> _rasterToModelColumnMap;

    QMetaObject::Connection _dataAboutToBeResetConnection;
    QMetaObject::Connection _dataResetConnection;

    void updateRows();
    
    // Converts model row to rasterizer row. Returns -1 if row out of bounds.
    int modelToRasterRow(int row) const;
    // Converts model column to rasterizer column.
    // Returns Rasterizer::InvalidColumn if column out of bounds.
    Rasterizer::Column modelToRasterCol(int col) const;

    // Returns raster text at given row and column, or empty string if no raster
    //  at that location, or row or column are out of range.
    QString textAt(int row, int col) const;
    // Returns raster value at given row and column, or -1 if no raster
    //  at that location, or row or column are out of range.
    int rasterAt(int row, int col) const;

  protected slots:
    void endResetModelHandler();

  public:
    RasterizerModel(
      Rasterizer *rasterizer, QObject *parent = nullptr, int max_rows = -1, 
      QList<Rasterizer::Column> visible_columns = QList<Rasterizer::Column>(),
      DisplayFormat displayFormat = DenominatorFormat);
    virtual ~RasterizerModel();

    // Required overrides for QAbstractTableModel.
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    // Returns the external Rasterizer array used in this model.
    const Rasterizer *rasterizer() { return _rasterizer; }
    // Sets the external Rasterizer array used in this model.
    void setRasterizer(const Rasterizer *r);

    // Returns the system midi division that the rasterizer is using.
    int division() const;
    // Returns maximum number of rows. If -1, all rows are included.
    int maxRows() const;
    // Sets maximum number of rows. If set to -1, all rows are included.
    void setMaxRows(int rows);
    // Returns a model-to-raster row list, ie. which raster rows are included in the model.
    QList<int /*rasterRow*/> visibleRows() const;
    // Returns the model-to-raster column list, ie. which raster columns are included in the model.
    QList<Rasterizer::Column> visibleColumns() const;
    // Sets the model-to-raster column list, ie. which raster columns are included in the model.
    void setVisibleColumns(const QList<Rasterizer::Column>& cols);
    // How text is displayed.
    DisplayFormat displayFormat() const;
    // Sets how text is displayed.
    void setDisplayFormat(DisplayFormat format);

    // Returns the model row number of the 'bar' row.
    int barRow() const;
    // Returns the model row number of the 'off' row.
    int offRow() const;
    // Returns true if the raster at the given row and column is 0 (snap to bar).
    bool isBarRaster(int row, int col) const;
    // Returns true if the raster at the given row and column is 1 ('off').
    bool isOffRaster(int row, int col) const;
    // Returns the raster value of some often-used denominator values.
    // Returns -1 if the raster is not available.
    int commonRaster(Rasterizer::CommonRasters commonRast) const;
    // Returns the index into the model of the given raster value, or -1 if raster not found.
    int indexOfRaster(int val) const;
    // Returns the model index of the given raster value, or invalid model index if raster not found.
    QModelIndex modelIndexOfRaster(int val) const;
    // Returns the given raster, or returns 1 division (1 quarter note) if no suitable raster could be found.
    int checkRaster(int val) const;
    // Given a raster, picks another raster based on the RasterPick, for example toggle the triple or dotted
    //  version of raster, or pick a raster from the same column that raster is in.
    int pickRaster(int raster, RasterPick pick) const;
};

} // namespace MusEGui

#endif
