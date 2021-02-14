//=========================================================
//  MusE
//  Linux Music Editor
//
//  custom_widget_actions.h
//  (C) Copyright 2011-2015 Tim E. Real (terminator356 on users.sourceforge.net)
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
class QSize;

namespace MusEGui {

class PixmapButton;  

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
  // The switch's value.
  bool _value;
  // Accompanying text beside the switch.
  QString _text;
  // Rectangle of the swich portion.
  QRect _rect;
  RouteChannelArrayItem() { _value = false; }
};

struct RouteChannelArrayHeaderItem
{
  // The header item text.
  QString _text;
  // Rectangle of the header item text.
  QRect _rect;
};

class RouteChannelArray
{
  private:
    // The number of columns.
    int _cols;
    // Whether the columns are exclusive.
    bool _colsExclusive;
    // Whether toggling a column is allowed even when columns are exclusive.
    bool _exclusiveToggle;
    // Whether the header items above the array of switches are visible.
    bool _headerVisible;
    // Current column the mouse is over. -1 == none.
    int _activeCol;
    // Current column being pressed. -1 == none.
    int _pressedCol;
    // The array of switches.
    RouteChannelArrayItem* _array;
    // Header items above the array of switches.
    RouteChannelArrayHeaderItem* _header;
    // Title header item above the checkbox portion. 
    RouteChannelArrayHeaderItem _checkBoxTitleItem;
    // Title header item above the item text portion.
    RouteChannelArrayHeaderItem _headerTitleItem;
    // Title header item above the switch array portion.
    RouteChannelArrayHeaderItem _arrayTitleItem;
    
    void init();
    
  public:
    RouteChannelArray(int cols = 0);
    virtual ~RouteChannelArray();
    RouteChannelArray& operator=(const RouteChannelArray&);
    // Returns the number of columns in the array.
    int columns() const { return _cols; }
    // Sets the number of columns in the array.
    void setColumns(int cols);
    // Whether the given column number is out of range.
    bool invalidColumn(int col) const { return col < 0 || col >= _cols; }

    // Whether the columns are exclusive.
    bool columnsExclusive() const    { return _colsExclusive; }
    // Sets whether the columns are exclusive.
    void setColumnsExclusive(bool v) { _colsExclusive = v; }
    // Whether toggling a column is allowed even when columns are exclusive.
    bool exclusiveToggle() const     { return _exclusiveToggle; }
    // Sets whether toggling a column is allowed even when columns are exclusive.
    void setExclusiveToggle(bool v)  { _exclusiveToggle = v; }
    // Current column mouse is over. -1 == none.
    int activeColumn() const         { return _activeCol; }
    // Set current column mouse is over. -1 == none.
    void setActiveColumn(int col)    
      { if((col == -1 || !invalidColumn(col)) && _activeCol != col) _activeCol = col; }
    // Current column being pressed. -1 == none.
    int pressedColumn() const { return _pressedCol; }
    // Set current column being pressed. -1 == none.
    // Returns true if any channel was changed (for redrawing).
    bool setPressedColumn(int col)
      { if((col != -1 && invalidColumn(col)) || _pressedCol == col) return false; _pressedCol = col; return true; }
    QString checkBoxTitle() const
      { return _checkBoxTitleItem._text; }
    void setCheckBoxTitle(const QString& str)
      { _checkBoxTitleItem._text = str; }
    QRect checkBoxTitleRect() const
      { return _checkBoxTitleItem._rect; }
    void setCheckBoxTitleRect(const QRect& r)
      { _checkBoxTitleItem._rect = r; }
      
    void setValues(int col, bool value, bool exclusive_cols = false, bool exclusive_toggle = false);
    bool value(int col) const
      { if(invalidColumn(col)) return false; return _array[col]._value; }
    void setValue(int col, bool value)
      { setValues(col, value, _colsExclusive, _exclusiveToggle); }
    QRect rect(int col) const
      { if(invalidColumn(col)) return QRect(); return _array[col]._rect; }
    void setRect(int col, const QRect& r)
      { if(invalidColumn(col)) return; _array[col]._rect = r; }
    QString text(int col) const
      { if(invalidColumn(col)) return QString(); return _array[col]._text; }
    void setText(int col, const QString& s)
      { if(invalidColumn(col)) return; _array[col]._text = s; }
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
      { if(invalidColumn(col)) return QRect(); return _header[col]._rect; }
    void headerSetRect(int col, const QRect& rect)
      { if(invalidColumn(col)) return; _header[col]._rect = rect; }
    QString headerText(int col) const
      { if(invalidColumn(col)) return QString(); return _header[col]._text; }
    void headerSetText(int col, const QString& str)
      { if(invalidColumn(col)) return; _header[col]._text = str; }
    QString headerTitle() const
      { return _headerTitleItem._text; }
    void headerSetTitle(const QString& str)
      { _headerTitleItem._text = str; }
    QRect headerTitleRect() const
      { return _headerTitleItem._rect; }
    void headerSetTitleRect(const QRect& r)
      { _headerTitleItem._rect = r; }
};

//---------------------------------------------------------
// RoutePopupHit
// Structure for action hit tests
//---------------------------------------------------------

struct RoutePopupHit
{
  enum HitTestType { HitTestHover, HitTestClick };
  enum HitType { HitNone, HitSpace, HitMenuItem, HitChannelBar, HitChannel };
  HitType _type;    
  QAction* _action; // Action where the hit occurred.
  int _value;   // Channel number for HitChannel.

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
      
  protected:
    QSize sizeHint() const;
    void paintEvent(QPaintEvent*);
    
  public:
    MenuItemControlWidget(RoutingMatrixWidgetAction* action, QWidget* parent = 0);
    // Returns, in the passed rectangle pointers, the rectangles of the checkbox, and/or item text label.
    void elementRect(QRect* checkbox_rect = 0, QRect* label_rect = 0) const;
};
      
//---------------------------------------------------------
//   SwitchBarActionWidget
//   The switch bar portion of the custom widget action
//---------------------------------------------------------

class SwitchBarActionWidget : public QWidget {
    Q_OBJECT
      
  private:
    RoutingMatrixWidgetAction* _action;
    
  protected:
    QSize sizeHint() const;
    void paintEvent(QPaintEvent*);
    
  public:
    SwitchBarActionWidget(RoutingMatrixWidgetAction* action, QWidget* parent = 0);
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
    // The part containing a checkbox, and the item text, together.
    MenuItemControlWidget* _menuItemControlWidget;
    // The array of switches.
    SwitchBarActionWidget* _switchWidget;
      
  protected:
    // For resizing properly on text changes.
    void actionEvent(QActionEvent*);
    
  public:
    RoutingMatrixActionWidget(RoutingMatrixWidgetAction* action, QWidget* parent = 0);
    // Performs a hit test on the contents of the widget at the given point, according to the requested hit test type.
    RoutePopupHit hitTest(const QPoint&, RoutePopupHit::HitTestType);
};
      
//---------------------------------------------------------
//   RoutingMatrixWidgetAction
//   The custom widget action
//---------------------------------------------------------

class RoutingMatrixWidgetAction : public QWidgetAction 
{
  Q_OBJECT
  private:
    // The switch (channel) array.
    RouteChannelArray _array;
    // Pixmap used for 'on' indicator.
    QPixmap* _onPixmap;
    // Pixmap used for 'off' indicator.
    QPixmap* _offPixmap;
    // A smaller font (about half size) than the action's font.
    QFont _smallFont;
    // Maximum dimensions of the on/off pixmaps.
    QSize _maxPixmapGeometry;
    // NOTE: _hasCheckBox is used instead of QAction::isCheckable()/setCheckable().
    bool _hasCheckBox;
    // Whether the checkbox is currently checked or not.
    bool _checkBoxChecked;
    // Whether the label and checkbox area is currently pressed or not.
    bool _menuItemPressed;
    // Whether clicking the array closes the menu or not.
    bool _arrayStayOpen;
    // Whether the action is highlighted (hovered) or not.
    bool _isSelected;
    // NOTE: _actionText is used instead of QAction::text()/setText().
    QString _actionText;
      
  protected:
    // Override
    QWidget* createWidget(QWidget* parent);
    // Sends ActionChanged events to created and associated widgets. Emits changed().
    // For resizing properly on text changes.
    void sendActionChanged();
      
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

    // Access to the switch (channel) array.
    RouteChannelArray* array() { return &_array; }
    // Updates the structure and/or cached rectangles of the channel array.
    // When array header text is changed this should also be called.
    void updateChannelArray();
    // Updates (redraws) created widgets.
    void updateCreatedWidgets();

    // A smaller font (about half size) than the action's font.
    QFont smallFont() const { return _smallFont; }
    // Pixmap used for 'on' indicator.
    QPixmap* onPixmap() const { return _onPixmap; }
    // Pixmap used for 'off' indicator.
    QPixmap* offPixmap() const { return _offPixmap; }
    // Maximum dimensions of the on/off pixmaps.
    QSize maxPixmapGeometry() const { return _maxPixmapGeometry; }
    
    // NOTE: Use hasCheckBox() instead of QAction::isCheckable().
    bool hasCheckBox() const { return _hasCheckBox; }
    // NOTE: Use setHasCheckBox() instead of QAction::setCheckable().
    void setHasCheckBox(bool v) { _hasCheckBox = v; }
    
    // Whether the checkbox is currently checked or not.
    bool checkBoxChecked() const { return _checkBoxChecked; }
    // Sets whether the checkbox is currently checked or not.
    void setCheckBoxChecked(bool v) { _checkBoxChecked = v; }
    
    // Whether the label and checkbox area is currently pressed or not.
    bool menuItemPressed() const { return _menuItemPressed; }
    // Sets whether the label and checkbox area is currently pressed or not.
    // Returns true if the menu item section was changed (for redrawing).
    bool setMenuItemPressed(bool v) { if(_menuItemPressed == v) return false; _menuItemPressed = v; return true; }
    
    // Whether clicking the array closes the menu or not.
    bool arrayStayOpen() const { return _arrayStayOpen; }
    // Sets whether clicking the array closes the menu or not.
    void setArrayStayOpen(bool v)  { _arrayStayOpen = v; }
    
    // Whether the action is highlighted (hovered) or not.
    bool isSelected() const { return _isSelected; }
    // Sets whether the action is highlighted (hovered) or not.
    void setSelected(bool v)  { _isSelected = v; }
  
    // NOTE: Use setActionText() instead of QAction::setText().
    void setActionText(const QString& s);
    // NOTE: Use actionText() instead of QAction::text().
    QString actionText() const { return _actionText; }
    //QString actionText() const { return text(); }
    
    // Does a hit test of type HitTestType, returning a RoutePopupHit structure describing what was hit.
    RoutePopupHit hitTest(const QPoint&, RoutePopupHit::HitTestType);
    // Returns the previous hittable item (ie. for left key movement).
    RoutePopupHit previousHit(const RoutePopupHit& fromHit);
    // Returns the next hittable item (ie. for right key movement).
    RoutePopupHit nextHit(const RoutePopupHit& fromHit);
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
      QString _checkBoxLabel;
      QString _itemLabel;
      QString _arrayLabel;
      
   protected:
      QWidget* createWidget(QWidget* parent);
     
   public:
      RoutingMatrixHeaderWidgetAction(const QString& checkbox_label, const QString& item_label, const QString& array_label, QWidget* parent = 0);
      };
      
} // namespace MusEGui

#endif  // __CUSTOM_WIDGET_ACTIONS_H__
