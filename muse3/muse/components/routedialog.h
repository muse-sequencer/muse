//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: routedialog.h,v 1.2 2004/01/31 17:31:49 wschweer Exp $
//
//  (C) Copyright 2004 Werner Schweer (ws@seh.de)
//  (C) Copyright 2015 Tim E. Real (terminator356 on sourceforge)
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

#ifndef __ROUTEDIALOG_H__
#define __ROUTEDIALOG_H__

#include <list>
#include <QFrame>
#include <QWidget>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QDialog>
#include <QStyledItemDelegate>
#include <QString>
#include <QItemSelectionModel>
#include <QColor>
#include <QList>
#include <QVector>

#include "type_defs.h"
#include "route.h"

// NOTE: To cure circular dependencies, of which there are many, these are
//        forward referenced and the corresponding headers included further down here.
class QCloseEvent;
class QMouseEvent;
class QWheelEvent;
class QPainter;
class QResizeEvent;
class QPaintEvent;
class QContextMenuEvent;

namespace MusEGui {
class RouteDialog;

typedef QList <QTreeWidgetItem*> RouteTreeItemList;

//---------------------------------------------------------
//   RouteTreeWidgetItem
//---------------------------------------------------------

struct RouteChannelsStruct
{
  // Whether the channel is selected (highlight colour).
  bool _selected;
  // Whether the channel is selected as part of one or more complete routes (yellow colour).
  bool _routeSelected;
  // Whether the channel is connected to any other route (whether to draw a header connector line).
  bool _connected;
  // Rectangle of the channel.
  QRect _buttonRect;
  // Contains pre-computed handy y value for drawing connection lines.
  int _lineY;
  
  RouteChannelsStruct() : _selected(false), _routeSelected(false), _connected(false), _lineY(-1) { }
  void toggleSelected()      { _selected = !_selected; }
  void toggleRouteSelected() { _routeSelected = !_routeSelected; }
  void toggleConnected()     { _connected = !_connected; }
};

class RouteChannelsList : public QVector<RouteChannelsStruct>
{
  public:
    // Returns true if any channel was changed.
    bool fillSelected(bool v) { 
      bool changed = false; const int sz = size(); 
      for(int i = 0; i < sz; ++i) { 
        bool& sel = operator[](i)._selected; 
        if(sel != v) changed = true; 
        sel = v;
      } 
      return changed; 
    }
    bool selected(int c) const       { if(c >= size()) return false; return at(c)._selected; }
    void select(int c, bool v)       { if(c >= size()) return; operator[](c)._selected = v; }
    void toggleSelected(int c)       { if(c >= size()) return; operator[](c).toggleSelected(); }

    // Returns true if any channel was changed.
    bool fillRouteSelected(bool v) {
      bool changed = false; const int sz = size(); 
      for(int i = 0; i < sz; ++i) {
        bool& sel = operator[](i)._routeSelected; 
        if(sel != v) changed = true; 
        sel = v;
      }
      return changed; 
    }
    bool routeSelected(int c) const  { if(c >= size()) return false; return at(c)._routeSelected; }
    void routeSelect(int c, bool v)  { if(c >= size()) return; operator[](c)._routeSelected = v; }
    void toggleRouteSelected(int c)  { if(c >= size()) return; operator[](c).toggleRouteSelected(); }
    
    // Returns true if any channel was changed.
    bool fillConnected(bool v) {
      bool changed = false; const int sz = size();
      for(int i = 0; i < sz; ++i) {
        bool& sel = operator[](i)._connected; 
        if(sel != v) changed = true; 
        sel = v;
      }
      return changed; 
    }
    bool connected(int c) const      { if(c >= size()) return false; return at(c)._connected; }
    void setConnected(int c, bool v) { if(c >= size()) return; operator[](c)._connected = v; }
    void toggleConnected(int c)      { if(c >= size()) return; operator[](c).toggleConnected(); }

    // Pre-computed handy y value for drawing connection lines.
    int  lineY(int c) const          { if(c >= size()) return -1; return at(c)._lineY; }
    // Set pre-computed handy y value for drawing connection lines.
    void setLineY(int c, int v)      { if(c >= size()) return; operator[](c)._lineY = v; }
    
    // How many channels are connected.
    int connectedChannels() const;
    // Returns the smallest width that can fit a channel bar.
    static int minimumWidthHint();
    // Returns the minimum width of the array that will fit into the given width constraint.
    int widthHint(int width) const;
    // Returns the minimum height of the array that will fit into the given width constraint.
    int heightHint(int width) const;
    // Returns a suitable size based on the given width constraint.
    QSize sizeHint(int width) const { return QSize(widthHint(width), heightHint(width)); }
    // How many channels fit into the width.
    static int channelsPerWidth(int width);
    // How many groups accommodate the given number of channels, all on a single bar.
    static int groupsPerChannels(int channels);
    // How many bars accommodate the item's total channels, for the given number of maximum number of channels on a single bar.
    int barsPerColChannels(int cc) const;
};

class RouteTreeWidgetItem : public QTreeWidgetItem
{
  public:
        enum ItemType { NormalItem = Type, CategoryItem = UserType, RouteItem = UserType + 1, ChannelsItem = UserType + 2};
        enum ItemMode { NormalMode, ExclusiveMode };
        // A data role to pass the item type from item to delegate.
        //enum ItemDataRole { TypeRole = Qt::UserRole};
        
  private:
        bool _isInput;
        MusECore::Route _route;
        RouteChannelsList _channels;
        ItemMode _itemMode;
        int _curChannel;

        void init();
  
  public:
        // Overrides for QTreeWidgetItem constructor...
        RouteTreeWidgetItem(int type = NormalItem, bool isInput = false, const MusECore::Route& route = MusECore::Route(), ItemMode mode = NormalMode)
                            : QTreeWidgetItem(type), _isInput(isInput), _route(route), _itemMode(mode) { init(); }
                            
        RouteTreeWidgetItem(const QStringList& strings, int type = NormalItem,
                            bool isInput = false, const MusECore::Route& route = MusECore::Route(), ItemMode mode = NormalMode)
                            : QTreeWidgetItem(strings, type), _isInput(isInput), _route(route), _itemMode(mode) { init(); }
                            
        RouteTreeWidgetItem(QTreeWidget* parent, int type = NormalItem,
                            bool isInput = false, const MusECore::Route& route = MusECore::Route(), ItemMode mode = NormalMode)
                            : QTreeWidgetItem(parent, type), _isInput(isInput), _route(route), _itemMode(mode) { init(); }
                            
        RouteTreeWidgetItem(QTreeWidget* parent, const QStringList& strings, int type = NormalItem,
                            bool isInput = false, const MusECore::Route& route = MusECore::Route(), ItemMode mode = NormalMode)
                            : QTreeWidgetItem(parent, strings, type), _isInput(isInput), _route(route), _itemMode(mode) { init(); }
                            
        RouteTreeWidgetItem(QTreeWidget* parent, QTreeWidgetItem* preceding, int type = NormalItem,
                            bool isInput = false, const MusECore::Route& route = MusECore::Route(), ItemMode mode = NormalMode)
                            : QTreeWidgetItem(parent, preceding, type), _isInput(isInput), _route(route), _itemMode(mode) { init(); }
                            
        RouteTreeWidgetItem(QTreeWidgetItem* parent, int type = NormalItem,
                            bool isInput = false, const MusECore::Route& route = MusECore::Route(), ItemMode mode = NormalMode)
                            : QTreeWidgetItem(parent, type), _isInput(isInput), _route(route), _itemMode(mode) { init(); }
                            
        RouteTreeWidgetItem(QTreeWidgetItem* parent, const QStringList& strings, int type = NormalItem,
                            bool isInput = false, const MusECore::Route& route = MusECore::Route(), ItemMode mode = NormalMode)
                            : QTreeWidgetItem(parent, strings, type), _isInput(isInput), _route(route), _itemMode(mode) { init(); }
                            
        RouteTreeWidgetItem(QTreeWidgetItem* parent, QTreeWidgetItem* preceding, int type = NormalItem,
                            bool isInput = false, const MusECore::Route& route = MusECore::Route(), ItemMode mode = NormalMode)
                            : QTreeWidgetItem(parent, preceding, type), _isInput(isInput), _route(route), _itemMode(mode) { init(); }
                            
        MusECore::Route& route()          { return _route; }
        // Whether this item should exist or not, based on _route and the item type.
        bool routeNodeExists();
        // Fills a list of routes with selected items' routes.
        void getSelectedRoutes(MusECore::RouteList& routes);
        
        // Returns item exclusive mode setting.
        ItemMode itemMode() const         { return _itemMode; }
        // Sets item exclusive mode setting.
        void setItemMode(ItemMode mode)   { _itemMode = mode; }
        
        // Automatically sets the number of channels. Returns true if channel count was changed.
        bool setChannels();
        // Returns the number of channels.
        int channelCount() const               { return _channels.size(); }
        // Sets the number of channels.
        void setChannelCount(int c)            { _channels.resize(c); }
        // Returns true if the channel is selected.
        bool channelSelected(int c) const      { return _channels.selected(c); }
        // Selects the channel.
        void selectChannel(int c, bool v)      { _channels.select(c, v); }
        // Toggles the channel selection.
        void toggleChannel(int c)              { _channels.toggleSelected(c); }
        // Sets all channels' selected state. Returns true if any channel was changed.
        bool fillSelectedChannels(bool v)      { return _channels.fillSelected(v); }
        // Returns true if the channel is route-selected.
        bool channelRouteSelected(int c) const { return _channels.routeSelected(c); }
        // Route-selects the channel.
        void routeSelectChannel(int c, bool v) { _channels.routeSelect(c, v); }
        // Toggles the channel route-selection.
        void toggleChannelRouteSelect(int c)   { _channels.toggleRouteSelected(c); }
        // Sets all channels' route-selected state. Returns true if any channel was changed.
        bool fillChannelsRouteSelected(bool v) { return _channels.fillRouteSelected(v); }
        // Returns the channel, based at rect y, whose rectangle contains pt.
        int channelAt(const QPoint& pt, const QRect& rect) const;

        // For drawing channel lines:
        int channelYValue(int c) const    { return _channels.lineY(c); }
        // Computes, and caches, channel y values. May be slow. Don't call on each draw, only when routes change. Use channelYValue() to draw.
        void computeChannelYValues(int col_width = -1); 
        
        // Returns current channel. (Unlike being selected.)
        int curChannel() const            { return _curChannel; }
        // Sets the current channel. (Unlike being selected.)
        void setCurChannel(int c)         { _curChannel = c; }

        // Handles mouse press events.
        bool mousePressHandler(QMouseEvent* e, const QRect& rect); 
        // Handles mouse move events.
        bool mouseMoveHandler(QMouseEvent* e, const QRect& rect); 
        // Handles painting. Returns true if the painting was handled.
        bool paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const;
        // Returns suggested size of item, that will fit into the given width while expanding vertically.
        // If width is -1, it uses the view width.
        QSize getSizeHint(int column, int width = -1) const;
        // Returns true if the item should be re-laid out (ie. with a tree's scheduleDelayedItemsLayout() or a delegate's sizeHintChanged()).
        // For channel items, it first adjusts the channel bar width to fit the new width.
        bool testForRelayout(int column, int old_width, int new_width);
};

//---------------------------------------------------------
//   ConnectionsView
//---------------------------------------------------------

class ConnectionsView : public QFrame
{
        Q_OBJECT
        
  private:
        RouteDialog* _routeDialog;
        int lastY;
        int itemY(RouteTreeWidgetItem* item, bool is_input, int channel = -1) const;
        void drawItem(QPainter* pPainter, QTreeWidgetItem* routesItem, const QColor& col);
        void drawConnectionLine(QPainter* pPainter,
                int x1, int y1, int x2, int y2, int h1, int h2);

  protected:
        virtual void paintEvent(QPaintEvent*);
        virtual void mousePressEvent(QMouseEvent*);
        virtual void mouseMoveEvent(QMouseEvent*);
        virtual void wheelEvent(QWheelEvent*);
        virtual void contextMenuEvent(QContextMenuEvent*);

      signals:
        void scrollBy(int, int);
        
     
  public:
        ConnectionsView(QWidget* parent = 0, RouteDialog* d = 0);
        virtual ~ConnectionsView();
        void setRouteDialog(RouteDialog* d) { _routeDialog = d; }
};

//---------------------------------------------------------
//   RouteTreeWidget
//---------------------------------------------------------

class RouteTreeWidget : public QTreeWidget
{
  
        Q_OBJECT
        
        Q_PROPERTY(bool isInput READ isInput WRITE setIsInput)
        // An extra property required to support stylesheets (not enough colours).
        Q_PROPERTY(QColor categoryColor READ categoryColor WRITE setCategoryColor)

private:
        bool _isInput;
        bool _channelWrap;
        QColor _categoryColor;
        
private slots:
        void headerSectionResized(int logicalIndex, int oldSize, int newSize);

protected:
        virtual void mousePressEvent(QMouseEvent*);
        virtual void mouseMoveEvent(QMouseEvent*);
        virtual QItemSelectionModel::SelectionFlags selectionCommand(const QModelIndex& index, const QEvent* event = 0) const;
        virtual void resizeEvent(QResizeEvent*);
        
protected slots:
        virtual void selectionChanged(const QItemSelection& selected, const QItemSelection& deselected);
        
public slots:
        void scrollBy(int dx, int dy);
        
public:
        RouteTreeWidget(QWidget* parent = 0, bool is_input = false);
        virtual ~RouteTreeWidget();

        void computeChannelYValues();
        
        bool isInput() { return _isInput; }
        void setIsInput(bool v) { _isInput = v; }
        
        bool channelWrap() { return _channelWrap; }
        void setChannelWrap(bool v) { _channelWrap = v; }
        
        RouteTreeWidgetItem* itemFromIndex(const QModelIndex& index) const;
        RouteTreeWidgetItem* findItem(const MusECore::Route&, int itemType = -1);
        RouteTreeWidgetItem* findCategoryItem(const QString&);
        int channelAt(RouteTreeWidgetItem* item, const QPoint& pt);
        //void clearChannels();
        
        void getSelectedRoutes(MusECore::RouteList& routes);
        void getItemsToDelete(QVector<QTreeWidgetItem*>& items_to_remove, bool showAllMidiPorts = false);
        //void scheduleDelayedLayout() { scheduleDelayedItemsLayout(); }  // Just to make it public.
        void selectRoutes(const QList<QTreeWidgetItem*>& routes, bool doNormalSelections);

        QColor categoryColor() const { return _categoryColor; }
        void setCategoryColor(const QColor& c) { _categoryColor = c; }
};


} // namespace MusEGui

#include "ui_routedialogbase.h"

namespace MusEGui {

//-----------------------------------
//   RoutingItemDelegate
//-----------------------------------

class RoutingItemDelegate : public QStyledItemDelegate
{
    Q_OBJECT

  private:
    RouteTreeWidget* _tree;
    bool _isInput;
    QStyle::SubElement _currentSubElement; // Set in mouse press, checked in release to prevent unwanted editor opening.
    // Need this. For some reason when using CurrentChanged trigger, createEditor is called upon opening the dialog, yet nothing is selected.
    bool _firstPress; 

    QRect getItemRectangle(const QStyleOptionViewItem& option, const QModelIndex& index, QStyle::SubElement subElement, QWidget* editor = NULL) const;
    bool subElementHitTest(const QPoint& point, const QStyleOptionViewItem& option, const QModelIndex& index, QStyle::SubElement* subElement, QWidget* editor = NULL) const;
    
  public:
    RoutingItemDelegate(bool is_input, RouteTreeWidget* tree, QWidget *parent = 0);

    virtual void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const;
    virtual QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const;
    virtual void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const;
    // Exposed as public from protected, so that it may be called from the tree widget.                  
    virtual void initStyleOption(QStyleOptionViewItem* option, const QModelIndex& index) const 
      { QStyledItemDelegate::initStyleOption(option, index); }
      
    // Emits the required sizeHintChanged(index) signal, to notify the tree to relayout the item.
    virtual void emitSizeHintChanged(const QModelIndex &index) { emit sizeHintChanged(index); }
    
  protected:
    bool editorEvent(QEvent* event, QAbstractItemModel* model, const QStyleOptionViewItem& option, const QModelIndex& index);
    bool eventFilter(QObject* editor, QEvent* event);
};

//---------------------------------------------------------
//   RouteDialog
//---------------------------------------------------------

class RouteDialog : public QDialog, public Ui::RouteDialogBase {
      Q_OBJECT

      MusECore::RouteList tmpSrcList;
      MusECore::RouteList tmpDstList;
      MusECore::RouteList tmpRoutesSrcList;
      MusECore::RouteList tmpRoutesDstList;
      
      RoutingItemDelegate* srcItemDelegate;
      RoutingItemDelegate* dstItemDelegate;
      
      RouteTreeItemList _srcFilterItems;
      RouteTreeItemList _dstFilterItems;
      
      virtual void closeEvent(QCloseEvent*);
      void removeItems();
      void addItems();
      void getRoutesToDelete(QTreeWidget* routesTree, QVector<QTreeWidgetItem*>& items_to_remove);
      void selectRoutes(bool doNormalSelections);

   private slots:
      void routeSelectionChanged();
      void disconnectClicked();
      void connectClicked();
      void srcSelectionChanged();
      void dstSelectionChanged();
      void songChanged(MusECore::SongChangedStruct_t);

      void srcTreeScrollValueChanged(int value);
      void dstTreeScrollValueChanged(int value);
      void srcScrollBarValueChanged(int value);
      void dstScrollBarValueChanged(int value);

      void filterSrcClicked(bool v);
      void filterDstClicked(bool v);
      
      void filterSrcRoutesClicked(bool v);
      void filterDstRoutesClicked(bool v);
      
      void allMidiPortsClicked(bool v);
      void preferredRouteAliasChanged(int);
      void verticalLayoutClicked(bool);
      
   signals:
      void closed();

   public:
      RouteDialog(QWidget* parent=0);
      QTreeWidgetItem* findRoutesItem(const MusECore::Route&, const MusECore::Route&);
      
      // Hide all items in the source or destination tree except the filter items, 
      //  and hide any items in the route tree whose source or destination route data 
      //  matches matches the filter items' routes.
      // If filter items is empty show all items, in both the source or destination tree 
      //  and the route tree.
      // Hiding items does not disturb the open state of an entire tree.
      void filter(const RouteTreeItemList& srcFilterItems, 
                  const RouteTreeItemList& dstFilterItems,
                  bool filterSrc, 
                  bool filterDst);
      
      enum { ROUTE_NAME_COL = 0 }; //, ROUTE_TYPE_COL };
      enum { ROUTE_SRC_COL = 0, ROUTE_DST_COL };
      enum  RoutingRoles { RouteRole = Qt::UserRole}; //, ChannelsRole = Qt::UserRole + 1 };
      
      static const QString tracksCat;
      static const QString midiPortsCat;
      static const QString midiDevicesCat;
      static const QString jackCat;
      static const QString jackMidiCat;

      static const int channelDotDiameter;
      static const int channelDotSpacing;
      static const int channelDotsPerGroup;
      static const int channelDotGroupSpacing;
      static const int channelDotsMargin;
      static const int channelBarHeight;
      static const int channelLineWidth;
      static const int channelLinesSpacing;
      static const int channelLinesMargin;
      };


} // namespace MusEGui

#endif

