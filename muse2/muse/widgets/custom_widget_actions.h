//=========================================================
//  MusE
//  Linux Music Editor
//  custom_widget_actions.h
//  (C) Copyright 2011 Tim E. Real (terminator356 on users.sourceforge.net)
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

#ifndef __CUSTOM_WIDGET_ACTIONS_H__
#define __CUSTOM_WIDGET_ACTIONS_H__

#include <QWidget>
#include <QWidgetAction>
#include <QList>
#include <QBitArray>

#include "route.h"

class QMouseEvent;
class QPainter;
class QPaintEvent;
class QPixmap;
class QString;
class QResizeEvent;

namespace MusEGui {

class PixmapButton;  

//---------------------------------------------------------
//   PixmapButtonsHeaderWidgetAction
//---------------------------------------------------------

class PixmapButtonsHeaderWidgetAction : public QWidgetAction { 
      Q_OBJECT
   private:
      
      QPixmap* _refPixmap;
      QString _text;
      int _channels;
      
   private slots:
     void chanClickMap(int);
     
   public:
      PixmapButtonsHeaderWidgetAction (const QString& text, QPixmap* ref_pixmap, int channels, QWidget* parent = 0);
      QWidget* createWidget(QWidget* parent);
      };
      
//---------------------------------------------------------
//   PixmapButtonsWidgetAction
//---------------------------------------------------------

class PixmapButtonsWidgetAction : public QWidgetAction { 
      Q_OBJECT
   private:
      
      QString _text;
      QBitArray _current;
      QPixmap* _onPixmap;
      QPixmap* _offPixmap;
      QList<PixmapButton*> _chan_buttons;
      
   private slots:
     void chanClickMap(int);
     
   public:
      PixmapButtonsWidgetAction(const QString& text, 
                              QPixmap* on_pixmap, QPixmap* off_pixmap, 
                              const QBitArray& initial,
                              QWidget* parent = 0);
      
      QWidget* createWidget(QWidget* parent);
      QBitArray currentState() const { return _current; }
      void setCurrentState(const QBitArray& state); 
      };

      
//---------------------------------------------------------
//   RoutingMatrixWidgetAction
//---------------------------------------------------------

struct RouteChannelArrayItem
{
  QRect _rect;
  QString _text;
  bool _value;
  RouteChannelArrayItem() { _value = false; }
};

struct RouteChannelArrayHeaderItem
{
  QRect _rect;
  QString _text;
  bool _value;
  MusECore::Route _route;
  RouteChannelArrayHeaderItem() { _value = false; }
};

class RouteChannelArray
{
  protected:
    int _cols;
    int _rows;
    //int _visible_cols;
    //int _visible_rows;
    bool _rowsExclusive;
    bool _colsExclusive;
    bool _exclusiveToggle;
    RouteChannelArrayItem* _array;
    RouteChannelArrayHeaderItem* _header;
    RouteChannelArrayItem _arrayTitleItem;
    RouteChannelArrayItem _headerTitleItem;
    
    virtual void init();
    virtual int itemCount() const { return _rows * _cols; }
    virtual bool invalidIndex(int row, int col) const { return col >= _cols || row >= _rows; }
    // Row and col must be valid.
    virtual int itemIndex(int row, int col) const { return _cols * row + col; }

    virtual int headerItemCount() const { return _rows + _cols; }
    virtual bool headerInvalidIndex(int row, int col) const;
    // Row and col must be valid. Row or col can be -1, but not both.
    virtual int headerItemIndex(int row, int col) const { if(row == -1) return col; return _cols + row; }
    
  public:
    RouteChannelArray(int rows = 0, int cols = 0);
    virtual ~RouteChannelArray();
    RouteChannelArray& operator=(const RouteChannelArray&);
    int columns() const { return _cols; }
    int rows() const { return _rows; }
    void setSize(int rows, int cols);

    virtual void setValues(int row, int col, bool value, bool exclusive_rows = false, bool exclusive_cols = false, bool exclusive_toggle = false);
    virtual void headerSetValues(int row, int col, bool value, bool exclusive_rows = false, bool exclusive_cols = false, bool exclusive_toggle = false);
    
    bool value(int row, int col) const
      { if(invalidIndex(row, col)) return false; return _array[itemIndex(row, col)]._value; }
    void setValue(int row, int col, bool value)
      { setValues(row, col, value, _rowsExclusive, _colsExclusive, _exclusiveToggle); }
    QRect rect(int row, int col) const
      { if(invalidIndex(row, col)) return QRect(); return _array[itemIndex(row, col)]._rect; }
    void setRect(int row, int col, const QRect& r)
      { if(invalidIndex(row, col)) return; _array[itemIndex(row, col)]._rect = r; }
    QString text(int row, int col) const
      { if(invalidIndex(row, col)) return QString(); return _array[itemIndex(row, col)]._text; }
    void setText(int row, int col, const QString& s)
      { if(invalidIndex(row, col)) return; _array[itemIndex(row, col)]._text = s; }
    QString arrayTitle() const
      { return _arrayTitleItem._text; }
    void setArrayTitle(const QString& str)
      { _arrayTitleItem._text = str; }
    QRect arrayTitleRect() const
      { return _arrayTitleItem._rect; }
    void setArrayTitleRect(const QRect& r)
      { _arrayTitleItem._rect = r; }
      
    bool headerValue(int row, int col) const
      { if(headerInvalidIndex(row, col)) return false; return _header[headerItemIndex(row, col)]._value; }
    void headerSetValue(int row, int col, bool value)
      { headerSetValues(row, col, value, _rowsExclusive, _colsExclusive, _exclusiveToggle); }
    QRect headerRect(int row, int col) const
      { if(headerInvalidIndex(row, col)) return QRect(); return _header[headerItemIndex(row, col)]._rect; }
    void headerSetRect(int row, int col, const QRect& rect)
      { if(headerInvalidIndex(row, col)) return; _header[headerItemIndex(row, col)]._rect = rect; }
    QString headerText(int row, int col) const
      { if(headerInvalidIndex(row, col)) return QString(); return _header[headerItemIndex(row, col)]._text; }
    void headerSetText(int row, int col, const QString& str)
      { if(headerInvalidIndex(row, col)) return; _header[headerItemIndex(row, col)]._text = str; }
    MusECore::Route headerRoute(int row, int col) const
      { if(headerInvalidIndex(row, col)) return MusECore::Route(); return _header[headerItemIndex(row, col)]._route; }
    void headerSetRoute(int row, int col, const MusECore::Route& route)
      { if(headerInvalidIndex(row, col)) return; _header[headerItemIndex(row, col)]._route = route; }
    QString headerTitle() const
      { return _headerTitleItem._text; }
    void headerSetTitle(const QString& str)
      { _headerTitleItem._text = str; }
    QRect headerTitleRect() const
      { return _headerTitleItem._rect; }
    void headerSetTitleRect(const QRect& r)
      { _headerTitleItem._rect = r; }
      
    //int visibleColumns() const { return _visible_cols; }
    //int visibleRows() const { return _visible_rows; }
    //void setVisibleColums(int cols) { _visible_cols = (cols > _cols) ? _cols : cols; }
    //void setVisibleRows(int rows) { _visible_rows = (rows > _rows) ? _rows : rows; }
    
    bool rowsExclusive() const       { return _rowsExclusive; }
    bool columnsExclusive() const    { return _colsExclusive; }
    void setRowsExclusive(bool v)    { _rowsExclusive = v; }
    void setColumnsExclusive(bool v) { _colsExclusive = v; }
    bool exclusiveToggle() const     { return _exclusiveToggle; }
    void setExclusiveToggle(bool v)  { _exclusiveToggle = v; }
};

// class RouteChannelArrayHeader : public RouteChannelArray {
//   protected:
//     virtual void init();
//     virtual int itemCount() const { return _rows + _cols; }
//     virtual bool invalidIndex(int row, int col) const;
//     // Row and col must be valid. Row or col can be -1, but not both.
//     virtual int itemIndex(int row, int col) const { if(row == -1) return col; return _cols + row; }
//     
//   public:
//     virtual void setValues(int row, int col, bool value, bool exclusive_rows = false, bool exclusive_cols = false, bool exclusive_toggle = false);
// };

class RoutingMatrixWidgetAction;
class RoutingMatrixWidget : public QWidget {
      Q_OBJECT
  private:
    RoutingMatrixWidgetAction* _action;
    
  protected:
    virtual QSize sizeHint() const;
    virtual void drawGrid(QPainter&);
    virtual void paintEvent(QPaintEvent*);
    virtual void mousePressEvent(QMouseEvent*);
    virtual void resizeEvent(QResizeEvent* e);
    
  public:
    RoutingMatrixWidget(RoutingMatrixWidgetAction* action, QWidget* parent = 0);
};
      
class RoutingMatrixWidgetAction : public QWidgetAction { 
      Q_OBJECT
   private:
      
      //RouteChannelArrayHeader _header;
      RouteChannelArray _array;
      QPixmap* _onPixmap;
      QPixmap* _offPixmap;
      QFont _smallFont;
      QRect _maxPixmapGeometry;
      
   protected:
      virtual QWidget* createWidget(QWidget* parent);
      virtual void deleteWidget(QWidget* widget);
     
   public:
      static const int margin;
      static const int itemHSpacing;
      static const int itemVSpacing;
      static const int groupSpacing;
      static const int itemsPerGroup;
      
      RoutingMatrixWidgetAction(int rows, int cols,  
                                QPixmap* on_pixmap, QPixmap* off_pixmap, 
                                QWidget* parent = 0);
                                //int visible_rows = -1, int visible_cols = -1);

      void updateChannelArray();
      
      //RouteChannelArrayHeader* header() { return &_header; }
      RouteChannelArray* array()        { return &_array; }

      QFont smallFont() const     { return _smallFont; }
      QRect maxPixmapGeometry() const { return _maxPixmapGeometry; }

      QPixmap* onPixmap() const  { return _onPixmap; }
      QPixmap* offPixmap() const { return _offPixmap; }
      };

} // namespace MusEGui

#endif  // __CUSTOM_WIDGET_ACTIONS_H__
