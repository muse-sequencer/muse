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
#include <QLabel>
#include <QList>
#include <QBitArray>

#include "route.h"

class QMouseEvent;
class QPainter;
class QPaintEvent;
class QPixmap;
class QString;
class QResizeEvent;
class QMouseEvent;
class QEnterEvent;
class QLeaveEvent;

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

// struct RouteChannelArrayHeaderItem
// {
//   QRect _rect;
//   QRect _checkBoxRect;
//   QString _text;
//   bool _value;
//   MusECore::Route _route;
//   RouteChannelArrayHeaderItem() { _value = false; }
// };

struct RouteChannelArrayHeaderItem
{
  QRect _rect;
  QString _text;
};

// class RouteChannelArray
// {
//   protected:
//     int _cols;
//     int _rows;
//     //int _visible_cols;
//     //int _visible_rows;
//     bool _rowsExclusive;
//     bool _colsExclusive;
//     bool _exclusiveToggle;
//     RouteChannelArrayItem* _array;
//     RouteChannelArrayHeaderItem* _header;
//     RouteChannelArrayItem _arrayTitleItem;
//     RouteChannelArrayItem _headerTitleItem;
//     
//     virtual void init();
//     virtual int itemCount() const { return _rows * _cols; }
//     virtual bool invalidIndex(int row, int col) const { return col >= _cols || row >= _rows; }
//     // Row and col must be valid.
//     virtual int itemIndex(int row, int col) const { return _cols * row + col; }
// 
//     virtual int headerItemCount() const { return _rows + _cols; }
//     virtual bool headerInvalidIndex(int row, int col) const;
//     // Row and col must be valid. Row or col can be -1, but not both.
//     virtual int headerItemIndex(int row, int col) const { if(row == -1) return col; return _cols + row; }
//     
//   public:
//     RouteChannelArray(int rows = 0, int cols = 0);
//     virtual ~RouteChannelArray();
//     RouteChannelArray& operator=(const RouteChannelArray&);
//     int columns() const { return _cols; }
//     int rows() const { return _rows; }
//     void setSize(int rows, int cols);
// 
//     virtual void setValues(int row, int col, bool value, bool exclusive_rows = false, bool exclusive_cols = false, bool exclusive_toggle = false);
//     virtual void headerSetValues(int row, int col, bool value, bool exclusive_rows = false, bool exclusive_cols = false, bool exclusive_toggle = false);
//     
//     bool value(int row, int col) const
//       { if(invalidIndex(row, col)) return false; return _array[itemIndex(row, col)]._value; }
//     void setValue(int row, int col, bool value)
//       { setValues(row, col, value, _rowsExclusive, _colsExclusive, _exclusiveToggle); }
//     QRect rect(int row, int col) const
//       { if(invalidIndex(row, col)) return QRect(); return _array[itemIndex(row, col)]._rect; }
//     void setRect(int row, int col, const QRect& r)
//       { if(invalidIndex(row, col)) return; _array[itemIndex(row, col)]._rect = r; }
//     QString text(int row, int col) const
//       { if(invalidIndex(row, col)) return QString(); return _array[itemIndex(row, col)]._text; }
//     void setText(int row, int col, const QString& s)
//       { if(invalidIndex(row, col)) return; _array[itemIndex(row, col)]._text = s; }
//     QString arrayTitle() const
//       { return _arrayTitleItem._text; }
//     void setArrayTitle(const QString& str)
//       { _arrayTitleItem._text = str; }
//     QRect arrayTitleRect() const
//       { return _arrayTitleItem._rect; }
//     void setArrayTitleRect(const QRect& r)
//       { _arrayTitleItem._rect = r; }
//       
//     bool headerValue(int row, int col) const
//       { if(headerInvalidIndex(row, col)) return false; return _header[headerItemIndex(row, col)]._value; }
//     void headerSetValue(int row, int col, bool value)
//       { headerSetValues(row, col, value, _rowsExclusive, _colsExclusive, _exclusiveToggle); }
//     QRect headerRect(int row, int col) const
//       { if(headerInvalidIndex(row, col)) return QRect(); return _header[headerItemIndex(row, col)]._rect; }
//     void headerSetRect(int row, int col, const QRect& rect)
//       { if(headerInvalidIndex(row, col)) return; _header[headerItemIndex(row, col)]._rect = rect; }
//     QString headerText(int row, int col) const
//       { if(headerInvalidIndex(row, col)) return QString(); return _header[headerItemIndex(row, col)]._text; }
//     void headerSetText(int row, int col, const QString& str)
//       { if(headerInvalidIndex(row, col)) return; _header[headerItemIndex(row, col)]._text = str; }
//     MusECore::Route headerRoute(int row, int col) const
//       { if(headerInvalidIndex(row, col)) return MusECore::Route(); return _header[headerItemIndex(row, col)]._route; }
//     void headerSetRoute(int row, int col, const MusECore::Route& route)
//       { if(headerInvalidIndex(row, col)) return; _header[headerItemIndex(row, col)]._route = route; }
//     QString headerTitle() const
//       { return _headerTitleItem._text; }
//     void headerSetTitle(const QString& str)
//       { _headerTitleItem._text = str; }
//     QRect headerTitleRect() const
//       { return _headerTitleItem._rect; }
//     void headerSetTitleRect(const QRect& r)
//       { _headerTitleItem._rect = r; }
//     QRect headerCheckBoxRect(int row) const
//       { if(headerInvalidIndex(row, -1)) return QRect(); return _header[headerItemIndex(row, -1)]._checkBoxRect; }
//     void headerSetCheckBoxRect(int row, const QRect& rect)
//       { if(headerInvalidIndex(row, -1)) return; _header[headerItemIndex(row, -1)]._checkBoxRect = rect; }
//       
//     //int visibleColumns() const { return _visible_cols; }
//     //int visibleRows() const { return _visible_rows; }
//     //void setVisibleColums(int cols) { _visible_cols = (cols > _cols) ? _cols : cols; }
//     //void setVisibleRows(int rows) { _visible_rows = (rows > _rows) ? _rows : rows; }
//     
//     bool rowsExclusive() const       { return _rowsExclusive; }
//     bool columnsExclusive() const    { return _colsExclusive; }
//     void setRowsExclusive(bool v)    { _rowsExclusive = v; }
//     void setColumnsExclusive(bool v) { _colsExclusive = v; }
//     bool exclusiveToggle() const     { return _exclusiveToggle; }
//     void setExclusiveToggle(bool v)  { _exclusiveToggle = v; }
// };

class RouteChannelArray
{
  private:
    int _cols;
    bool _colsExclusive;
    bool _exclusiveToggle;
    bool _headerVisible;
    int _activeCol;  // -1 == none.
    RouteChannelArrayItem* _array;
    RouteChannelArrayHeaderItem* _header;
    RouteChannelArrayHeaderItem _checkBoxTitleItem;
    RouteChannelArrayHeaderItem _headerTitleItem;
    RouteChannelArrayHeaderItem _arrayTitleItem;
    
    void init();
    int itemCount() const { return _cols; }
    bool invalidIndex(int col) const { return col >= _cols; }
    int itemIndex(int col) const { return col; }

    int headerItemCount() const { return _cols; }
    bool headerInvalidIndex(int col) const;
    int headerItemIndex(int col) const { return col; }
    
  public:
    RouteChannelArray(int cols = 0);
    virtual ~RouteChannelArray();
    RouteChannelArray& operator=(const RouteChannelArray&);
    int columns() const { return _cols; }
    void setSize(int cols);

    void setValues(int col, bool value, bool exclusive_cols = false, bool exclusive_toggle = false);
    
    QString checkBoxTitle() const
      { return _checkBoxTitleItem._text; }
    void setCheckBoxTitle(const QString& str)
      { _checkBoxTitleItem._text = str; }
    QRect checkBoxTitleRect() const
      { return _checkBoxTitleItem._rect; }
    void setCheckBoxTitleRect(const QRect& r)
      { _checkBoxTitleItem._rect = r; }
      
    bool value(int col) const
      { if(invalidIndex(col)) return false; return _array[itemIndex(col)]._value; }
    void setValue(int col, bool value)
      { setValues(col, value, _colsExclusive, _exclusiveToggle); }
    QRect rect(int col) const
      { if(invalidIndex(col)) return QRect(); return _array[itemIndex(col)]._rect; }
    void setRect(int col, const QRect& r)
      { if(invalidIndex(col)) return; _array[itemIndex(col)]._rect = r; }
    QString text(int col) const
      { if(invalidIndex(col)) return QString(); return _array[itemIndex(col)]._text; }
    void setText(int col, const QString& s)
      { if(invalidIndex(col)) return; _array[itemIndex(col)]._text = s; }
    QString arrayTitle() const
      { return _arrayTitleItem._text; }
    void setArrayTitle(const QString& str)
      { _arrayTitleItem._text = str; }
    QRect arrayTitleRect() const
      { return _arrayTitleItem._rect; }
    void setArrayTitleRect(const QRect& r)
      { _arrayTitleItem._rect = r; }
      
    bool headerVisible() const
      { return _headerVisible; }
    void headerSetVisible(bool v)
      { _headerVisible = v; }
    QRect headerRect(int col) const
      { if(headerInvalidIndex(col)) return QRect(); return _header[headerItemIndex(col)]._rect; }
    void headerSetRect(int col, const QRect& rect)
      { if(headerInvalidIndex(col)) return; _header[headerItemIndex(col)]._rect = rect; }
    QString headerText(int col) const
      { if(headerInvalidIndex(col)) return QString(); return _header[headerItemIndex(col)]._text; }
    void headerSetText(int col, const QString& str)
      { if(headerInvalidIndex(col)) return; _header[headerItemIndex(col)]._text = str; }
    QString headerTitle() const
      { return _headerTitleItem._text; }
    void headerSetTitle(const QString& str)
      { _headerTitleItem._text = str; }
    QRect headerTitleRect() const
      { return _headerTitleItem._rect; }
    void headerSetTitleRect(const QRect& r)
      { _headerTitleItem._rect = r; }
      
    bool columnsExclusive() const    { return _colsExclusive; }
    void setColumnsExclusive(bool v) { _colsExclusive = v; }
    bool exclusiveToggle() const     { return _exclusiveToggle; }
    void setExclusiveToggle(bool v)  { _exclusiveToggle = v; }
    int activeColumn() const         { return _activeCol; }
    void setActiveColumn(int col)    { _activeCol = col; }
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


//---------------------------------------------------------
// RoutePopupHit
// Structure for action hit tests
//---------------------------------------------------------

struct RoutePopupHit
{
  enum HitTestType { HitTestHover, HitTestClick };
  enum HitType { HitNone, HitMenuItem, HitChannelBar };
  QAction* _action; // Action where the hit occurred.
  HitType _type;    
  int _value;   // Channel number for channel bar.

  RoutePopupHit() { _action = 0; _type = HitNone; _value = 0; }
  RoutePopupHit(QAction* action, HitType ht, int val = 0) { _action = action; _type = ht; _value = val; }
  bool operator==(const RoutePopupHit& hit) { return _action == hit._action && _type == hit._type && _value == hit._value; }
};


class RoutingMatrixActionWidget;
class RoutingMatrixWidgetAction;

//---------------------------------------------------------
//   MenuItemControlWidget
//   The checkable menu item-like portion of the custom widget action, with text portion as well
//---------------------------------------------------------

class MenuItemControlWidget : public QWidget 
{ 
  Q_OBJECT
      
  private:
    RoutingMatrixWidgetAction* _action;
    //bool _isSelected;
      
  protected:
    QSize sizeHint() const;
    void paintEvent(QPaintEvent*);
    void mousePressEvent(QMouseEvent*);
    void mouseReleaseEvent(QMouseEvent*);
    void mouseDoubleClickEvent(QMouseEvent*);
    void contextMenuEvent(QContextMenuEvent*);
    //void actionEvent(QActionEvent*);
    //bool event(QEvent*);
    
  public:
    MenuItemControlWidget(RoutingMatrixWidgetAction* action, QWidget* parent = 0);
    void elementRect(QRect* checkbox_rect = 0, QRect* label_rect = 0) const;
    //bool isSelected() const { return _isSelected; }
    //void setSelected(bool v)  { _isSelected = v; }
};
      
//---------------------------------------------------------
//   SwitchBarActionWidget
//   The switch bar portion of the custom widget action
//---------------------------------------------------------

class SwitchBarActionWidget : public QWidget {
    Q_OBJECT
      
  private:
    RoutingMatrixWidgetAction* _action;
    //bool _isSelected;
    
  protected:
    QSize sizeHint() const;
    //void drawGrid(QPainter&);
    void paintEvent(QPaintEvent*);
    void mousePressEvent(QMouseEvent*);
    void mouseReleaseEvent(QMouseEvent*);
    void mouseDoubleClickEvent(QMouseEvent*);
    void mouseMoveEvent(QMouseEvent*);
    void resizeEvent(QResizeEvent*);
    void contextMenuEvent(QContextMenuEvent*);
    
  public:
    SwitchBarActionWidget(RoutingMatrixWidgetAction* action, QWidget* parent = 0);
    //bool isSelected() const { return _isSelected; }
    //void setSelected(bool v)  { _isSelected = v; }
};
      
//---------------------------------------------------------
//   RoutingMatrixActionWidget
//   Container widget holds checkable menu item-like widget with text portion,
//    and switch bar widget, for the custom widget action
//---------------------------------------------------------

class RoutingMatrixActionWidget : public QWidget 
{ 
    Q_OBJECT
      
  private:
    RoutingMatrixWidgetAction* _action;
    MenuItemControlWidget* _menuItemControlWidget;
    SwitchBarActionWidget* _switchWidget;
    //bool _isSelected;
      
  protected:
    void mousePressEvent(QMouseEvent*);
    void mouseReleaseEvent(QMouseEvent*);
    void mouseDoubleClickEvent(QMouseEvent*);
    void contextMenuEvent(QContextMenuEvent*);
    void actionEvent(QActionEvent*);
//     bool event(QEvent*);
//     void mouseMoveEvent(QMouseEvent*);
//     void enterEvent(QEvent*);
//     void leaveEvent(QEvent*);
    
   public:
      RoutingMatrixActionWidget(RoutingMatrixWidgetAction* action, QWidget* parent = 0);
      RoutePopupHit hitTest(const QPoint&, RoutePopupHit::HitTestType);
};
      
//---------------------------------------------------------
//   RoutingMatrixWidgetAction
//   The custom widget action
//---------------------------------------------------------

class RoutingMatrixWidgetAction : public QWidgetAction { 
      Q_OBJECT
   private:
      //QLabel* _itemLabel;
      RouteChannelArray _array;
      QPixmap* _onPixmap;
      QPixmap* _offPixmap;
      QFont _smallFont;
      QRect _maxPixmapGeometry;
      //QRect _checkBoxControlRect;
      //QRect _labelControlRect;
      bool _isChanged;
      bool _hasCheckBox;
      bool _checkBoxChecked;
      // Whether clicking the array closes the menu or not.
      bool _stayOpen;
      bool _isSelected;
      QString _labelText;
      
   //private slots:
   //   void actionHovered();
     
   protected:
      QWidget* createWidget(QWidget* parent);
      void deleteWidget(QWidget* widget);
      bool event(QEvent*);
      bool eventFilter(QObject*, QEvent*);
      
   public:
      static const int margin;
      static const int itemHSpacing;
      static const int itemVSpacing;
      static const int groupSpacing;
      static const int itemsPerGroup;
      static const int actionHMargin; // Empty area left and right of checkbox and text label.
      
      RoutingMatrixWidgetAction(int cols,  
                                QPixmap* on_pixmap, QPixmap* off_pixmap, 
                                QWidget* parent = 0, const QString& action_text = QString());

      void updateCreatedWidgets();
      void updateChannelArray();
      void sendActionChanged();
      
      RouteChannelArray* array()        { return &_array; }

      QFont smallFont() const     { return _smallFont; }
      QRect maxPixmapGeometry() const { return _maxPixmapGeometry; }

      QPixmap* onPixmap() const  { return _onPixmap; }
      QPixmap* offPixmap() const { return _offPixmap; }
      
      //QRect checkBoxControlRect() const { return _checkBoxControlRect; }
      //QRect labelControlRect() const { return _labelControlRect; }
      
      //void activate(ActionEvent event);
      
      bool isChanged() const { return _isChanged; }
      void setIsChanged(bool v) { _isChanged = v; }
      
      bool hasCheckBox() const { return _hasCheckBox; }
      void setHasCheckBox(bool v) { _hasCheckBox = v; }
      
      bool checkBoxChecked() const { return _checkBoxChecked; }
      void setCheckBoxChecked(bool v) { _checkBoxChecked = v; }
      
      bool stayOpen() const { return _stayOpen; }
      void setStayOpen(bool v)  { _stayOpen = v; }
      
      bool isSelected() const { return _isSelected; }
      void setSelected(bool v)  { _isSelected = v; }
    
      // NOTE: Use setActionText instead of QAction::setText().
      void setActionText(const QString& s);
      // NOTE: Use actionText instead of QAction::text().
      QString actionText() const { return _labelText; }
      //QString actionText() const { return text(); }
      
      RoutePopupHit hitTest(const QPoint&, RoutePopupHit::HitTestType);
      };

//---------------------------------------------------------
//   RoutingMatrixHeaderWidgetAction
//   A header action suitable for non-RoutingMatrixWidgetAction items.
//   NOTE: This is a separate action which can be used to head regular QAction 
//     items instead of the RoutingMatrixWidgetAction's own array headers.
//---------------------------------------------------------

class RoutingMatrixHeaderWidgetAction : public QWidgetAction { 
      Q_OBJECT
   private:
      QLabel* _checkBoxLabel;
      QLabel* _itemLabel;
      QLabel* _arrayLabel;
      
   protected:
      QWidget* createWidget(QWidget* parent);
      void deleteWidget(QWidget* widget);
     
   public:
      RoutingMatrixHeaderWidgetAction(const QString& checkbox_label, const QString& item_label, const QString& array_label, QWidget* parent = 0);
      };
      
} // namespace MusEGui

#endif  // __CUSTOM_WIDGET_ACTIONS_H__
