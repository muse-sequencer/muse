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
#include <QWidget>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QDialog>
#include <QStyledItemDelegate>
//#include <QItemSelectionModel>
#include <QBitArray>

//#include "ui_routedialogbase.h"
#include "type_defs.h"
#include "route.h"


class QCloseEvent;
class QMouseEvent;
class QWheelEvent;
class QString;
class QMouseEvent;
class QItemSelectionModel;
class QPainter;
class QColor;

namespace MusEGui {

  
typedef QList <QTreeWidgetItem*> RouteTreeItemList;

//---------------------------------------------------------
//   RouteTreeWidgetItem
//---------------------------------------------------------

struct RouteChannelsStruct
{
  bool _selected;
  //// Contains pre-computed handy y value for drawing connection lines. -1 means not connected.
  // Contains pre-computed handy y value for drawing connection lines.
  int _lineY;
  bool _connected;
  QRect _buttonRect;
  RouteChannelsStruct() { _selected = false; _lineY = -1; _connected = false; }
  void toggleSelected() { _selected = !_selected; }
};

class RouteChannelsList : public QVector<RouteChannelsStruct>
{
  public:
    void fillSelectedChannels(bool v) { const int sz = size(); for(int i = 0; i < sz; ++i) operator[](i)._selected = v; }
    bool channelSelected(int c) const { if(c >= size()) return false; return at(c)._selected; }
    void selectChannel(int c, bool v) { if(c >= size()) return; operator[](c)._selected = v; }
    void toggleChannel(int c)         { if(c >= size()) return; operator[](c)._selected = !at(c)._selected; }
    int  lineY(int c) const           { if(c >= size()) return -1; return at(c)._lineY; }
    void setLineY(int c, int v)       { if(c >= size()) return; operator[](c)._lineY = v; }
    //void fillYValues(bool v)          { const int sz = size(); for(int i = 0; i < sz; ++i) at(i)._lineY = v; }
    //void computeChannelYValues(); // May be slow. Don't call every draw time, only when routes change. Use channelYValue() to draw.
    bool connected(int c) const       { if(c >= size()) return false; return at(c)._connected; }
    void setConnected(int c, bool v)  { if(c >= size()) return; operator[](c)._connected = v; }
    void fillConnected(bool v)        { const int sz = size(); for(int i = 0; i < sz; ++i) operator[](i)._connected = v; }
};

class RouteTreeWidgetItem : public QTreeWidgetItem
{
  private:
        MusECore::Route _route;
        RouteChannelsList _channels;
        //QBitArray _channels;
        //QVector<int> _channelYValues; // Useful for drawing channel lines.
        
        int _curChannel;
        bool _isInput;
        void init();
  
  public:
        enum ItemType { NormalItem = Type, CategoryItem = UserType, RouteItem = UserType + 1, ChannelsItem = UserType + 2};
    
        RouteTreeWidgetItem(int type = NormalItem, bool isInput = false, const MusECore::Route& route = MusECore::Route())
                            : QTreeWidgetItem(type), _route(route), _isInput(isInput) { init(); }
        RouteTreeWidgetItem(const QStringList& strings, int type = NormalItem, bool isInput = false, const MusECore::Route& route = MusECore::Route())
                            : QTreeWidgetItem(strings, type), _route(route), _isInput(isInput) { init(); }
        RouteTreeWidgetItem(QTreeWidget* parent, int type = NormalItem, bool isInput = false, const MusECore::Route& route = MusECore::Route())
                            : QTreeWidgetItem(parent, type), _route(route), _isInput(isInput) { init(); }
        RouteTreeWidgetItem(QTreeWidget* parent, const QStringList& strings, int type = NormalItem, bool isInput = false, const MusECore::Route& route = MusECore::Route())
                            : QTreeWidgetItem(parent, strings, type), _route(route), _isInput(isInput) { init(); }
        RouteTreeWidgetItem(QTreeWidget* parent, QTreeWidgetItem* preceding, int type = NormalItem, bool isInput = false, const MusECore::Route& route = MusECore::Route())
                            : QTreeWidgetItem(parent, preceding, type), _route(route), _isInput(isInput) { init(); }
        RouteTreeWidgetItem(QTreeWidgetItem* parent, int type = NormalItem, bool isInput = false, const MusECore::Route& route = MusECore::Route())
                            : QTreeWidgetItem(parent, type), _route(route), _isInput(isInput) { init(); }
        RouteTreeWidgetItem(QTreeWidgetItem* parent, const QStringList& strings, int type = NormalItem, bool isInput = false, const MusECore::Route& route = MusECore::Route())
                            : QTreeWidgetItem(parent, strings, type), _route(route), _isInput(isInput) { init(); }
        RouteTreeWidgetItem(QTreeWidgetItem* parent, QTreeWidgetItem* preceding, int type = NormalItem, bool isInput = false, const MusECore::Route& route = MusECore::Route())
                            : QTreeWidgetItem(parent, preceding, type), _route(route), _isInput(isInput) { init(); }
        //RouteTreeWidgetItem(const RouteTreeWidgetItem& other);
        
        MusECore::Route& route()          { return _route; }
        bool routeNodeExists();
        void getSelectedRoutes(MusECore::RouteList& routes);
        
        int channelCount() const          { return _channels.size(); }
        //void setChannelCount(int c)       { _channels.resize(c); _channelYValues.resize(c); _channelYValues.fill(-1); }
        void setChannelCount(int c)       { _channels.resize(c); }
        //bool channelSelected(int c) const { if(c >= _channels.size()) return false; return _channels.testBit(c); }
        bool channelSelected(int c) const { return _channels.channelSelected(c); }
        //void selectChannel(int c, bool v) { if(c >= _channels.size()) return; v ? _channels.setBit(c) : _channels.clearBit(c); }
        void selectChannel(int c, bool v) { _channels.selectChannel(c, v); }
        //void toggleChannel(int c)         { if(c >= _channels.size()) return; _channels.toggleBit(c); }
        void toggleChannel(int c)         { _channels.toggleChannel(c); }
        //void fillSelectedChannels(bool v)         { _channels.fill(v); }
        void fillSelectedChannels(bool v)         { _channels.fillSelectedChannels(v); }
        int channelAt(const QPoint& pt, const QRect& rect) const;
        // How many non-omni channels are connected. For speed, it looks in the channel y values list, which must be current.
        int connectedChannels() const;
        // How many channels fit into the column. If w is -1, it uses the width of the first tree column.
        int channelsPerWidth(int w = -1) const;
        // How many groups accommodate the given number of channels, all on a single bar.
        int groupsPerChannels(int c) const;
        // How many bars accommodate the item's total channels, for the given number of maximum number of channels on a single bar.
        int barsPerColChannels(int cc) const;

        // For drawing channel lines:
        // Dual use: -1 means channel is not routed anywhere. Values >= 0 are y values for drawing channel lines.
        //int channelYValue(int c) const    { if(c >= _channelYValues.size()) return -1; return _channelYValues.at(c); }
        // For drawing channel lines:
        int channelYValue(int c) const    { return _channels.lineY(c); }
        // May be slow. Don't call every draw time, only when routes change. Use channelYValue() to draw.
        void computeChannelYValues(int col_width = -1); 
        
        int curChannel() const            { return _curChannel; }
        void setCurChannel(int c)         { _curChannel = c; }
        
        bool mousePressHandler(QMouseEvent* e, const QRect& rect); 
        bool paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const;
        QSize getSizeHint(int col, int col_width = -1) const;
        QSize getSizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const;
        bool testForRelayout(int col, int old_width, int new_width) const;
        
        //void columnSizeChanged(int logicalIndex, int oldSize, int newSize);
};

//---------------------------------------------------------
//   ConnectionsView
//---------------------------------------------------------

class RouteDialog;
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
private:
        bool _isInput;
        
private slots:
        void headerSectionResized(int logicalIndex, int oldSize, int newSize);

        void scrollRangeChanged(int min, int max);
        void scrollSliderMoved(int value);
        void scrollValueChanged(int value);

protected:
        virtual void mousePressEvent(QMouseEvent*);
        virtual QItemSelectionModel::SelectionFlags selectionCommand(const QModelIndex& index, const QEvent* event = 0) const;

protected slots:
        virtual void selectionChanged(const QItemSelection& selected, const QItemSelection& deselected);
        
public slots:
        void scrollBy(int dx, int dy);
        
public:
        RouteTreeWidget(QWidget* parent = 0, bool is_input = false);
        virtual ~RouteTreeWidget();

        RouteTreeWidgetItem* itemFromIndex(const QModelIndex& index) const;

        bool isInput() { return _isInput; }
        void setIsInput(bool v) { _isInput = v; }
        
        RouteTreeWidgetItem* findItem(const MusECore::Route&, int itemType = -1);
        RouteTreeWidgetItem* findCategoryItem(const QString&);
        int channelAt(RouteTreeWidgetItem* item, const QPoint& pt);
        //void clearChannels();
        
        void getSelectedRoutes(MusECore::RouteList& routes);
        void getItemsToDelete(QVector<QTreeWidgetItem*>& items_to_remove);
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

    void paint(QPainter *painter, const QStyleOptionViewItem &option,
              const QModelIndex &index) const;
    virtual QSize sizeHint(const QStyleOptionViewItem &option,
                  const QModelIndex &index) const;
    //QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option,
    //                      const QModelIndex &index) const;
    //void setEditorData(QWidget *editor, const QModelIndex &index) const;
    void setModelData(QWidget *editor, QAbstractItemModel *model,
                      const QModelIndex &index) const;

  protected:
    bool editorEvent(QEvent* event, QAbstractItemModel* model, const QStyleOptionViewItem& option, const QModelIndex& index);
    bool eventFilter(QObject* editor, QEvent* event);
    //void updateEditorGeometry(QWidget* editor, const QStyleOptionViewItem& option, const QModelIndex& index ) const;
    
  //private slots:
  //  void editorChanged();
    ////void commitAndCloseEditor();
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
      
      //RouteTreeWidgetItem* _srcFilterItem;
      //RouteTreeWidgetItem* _dstFilterItem;
      RouteTreeItemList _srcFilterItems;
      RouteTreeItemList _dstFilterItems;
      //RouteTreeItemList _srcFilterRouteItems;
      //RouteTreeItemList _dstFilterRouteItems;
      
      virtual void closeEvent(QCloseEvent*);
      void routingChanged();
      void removeItems();
      void addItems();
      //bool routeNodeExists(const MusECore::Route&);
      //void getItemsToDelete(QTreeWidget* tree, QVector<QTreeWidgetItem*>& items_to_remove);
      void getRoutesToDelete(QTreeWidget* routesTree, QVector<QTreeWidgetItem*>& items_to_remove);

   private slots:
      void routeSelectionChanged();
      void removeRoute();
      void addRoute();
      void srcSelectionChanged();
      void dstSelectionChanged();
      void songChanged(MusECore::SongChangedFlags_t);

      void srcTreeScrollValueChanged(int value);
      void dstTreeScrollValueChanged(int value);
      void srcScrollBarValueChanged(int value);
      void dstScrollBarValueChanged(int value);
      
      void filterSrcClicked(bool v);
      void filterDstClicked(bool v);
      
      void filterSrcRoutesClicked(bool v);
      void filterDstRoutesClicked(bool v);
      
   signals:
      void closed();

   public:
      RouteDialog(QWidget* parent=0);
      //RouteTreeWidgetItem* findSrcItem(const MusECore::Route&);
      //RouteTreeWidgetItem* findDstItem(const MusECore::Route&);
      //RouteTreeWidgetItem* findCategoryItem(QTreeWidget*, const QString&);
      QTreeWidgetItem* findRoutesItem(const MusECore::Route&, const MusECore::Route&);
      //void getSelectedRoutes(QTreeWidget* tree, MusECore::RouteList& routes);
      
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
      static const QString midiDevicesCat;
      static const QString jackCat;
      static const QString jackMidiCat;

//       static const QString trackLabel;
//       static const QString midiDeviceLabel;
//       static const QString jackLabel;
//       static const QString jackMidiLabel;
      
      static const int channelDotDiameter;
      static const int channelDotSpacing;
      static const int channelDotsPerGroup;
      static const int channelDotGroupSpacing;
      static const int channelDotsMargin;
      static const int channelBarHeight;
      static const int channelLinesSpacing;
      };


} // namespace MusEGui

#endif

