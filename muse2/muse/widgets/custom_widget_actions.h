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

class QMouseEvent;
class QPainter;
class QPaintEvent;
class QPixmap;
class QString;

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
  bool _value;
  RouteChannelArrayItem() { _value = false; }
};

class RouteChannelArray
{
  protected:
    int _cols;
    int _rows;
    RouteChannelArrayItem* _array;
    
    virtual void init();
    virtual int itemCount() const { return _rows * _cols; }
    virtual bool invalidIndex(int row, int col) const { return col >= _cols || row >= _rows; }
    // Row and col must be valid.
    virtual int itemIndex(int row, int col) const { return _cols * row + col; }
    
  public:
    RouteChannelArray(int rows = 0, int cols = 0);
    virtual ~RouteChannelArray();
    RouteChannelArray& operator=(const RouteChannelArray&);
    int columns() const { return _cols; }
    int rows() const { return _rows; }
    void setSize(int rows, int cols);

    bool value(int row, int col) const                  
      { if(invalidIndex(row, col)) return false; return _array[itemIndex(row, col)]._value; }
    void setValue(int row, int col, bool value)
      { if(invalidIndex(row, col)) return; _array[itemIndex(row, col)]._value = value; }
    QRect rect(int row, int col) const                  
      { if(invalidIndex(row, col)) return QRect(); return _array[itemIndex(row, col)]._rect; }
    void setRect(int row, int col, const QRect r)
      { if(invalidIndex(row, col)) return; _array[itemIndex(row, col)]._rect = r; }
};

class RouteChannelArrayHeader : public RouteChannelArray {
  protected:
    virtual void init();
    virtual int itemCount() const { return _rows + _cols; }
    virtual bool invalidIndex(int row, int col) const;
    // Row and col must be valid. Row or col can be -1, but not both.
    virtual int itemIndex(int row, int col) const { if(row == -1) return col; return _cols + row; }
};

class RoutingMatrixWidget : public QWidget {
      Q_OBJECT
  private:
      RouteChannelArrayHeader _header;
      RouteChannelArray _current;
      QPixmap* _onPixmap;
      QPixmap* _offPixmap;
      QFont _cellFont;
      QRect _cellGeometry;

      void updateChannelArray();
      
  protected:
    virtual QSize sizeHint() const;
    virtual void drawGrid(QPainter&);
    virtual void paintEvent(QPaintEvent*);
    
  public:
    RoutingMatrixWidget(int rows, int cols, QPixmap* onPixmap, QPixmap* offPixmap, QWidget* parent = 0);
    void setSize(int rows, int cols) { _header.setSize(rows, cols); _current.setSize(rows, cols); updateChannelArray(); }
    bool value(int row, int col) const { return _current.value(row, col); }
    void setValue(int row, int col, bool value) { _current.setValue(row, col, value); }

    static const int margin;
    static const int itemHSpacing;
    static const int itemVSpacing;
    static const int groupSpacing;
    static const int itemsPerGroup;
};
      
class RoutingMatrixWidgetAction : public QWidgetAction { 
      Q_OBJECT
   private:
      
      //RouteChannelArray _current;
      //QString _text;
      //QBitArray _current;
      int _cols;
      int _rows;
      RoutingMatrixWidget* _widget;
      QPixmap* _onPixmap;
      QPixmap* _offPixmap;
      //QList<PixmapButton*> _chan_buttons;
      
   //private slots:
   //  void chanClickMap(int);
     
   public:
      RoutingMatrixWidgetAction(//const QString& text, 
                              int rows, int cols,  
                              QPixmap* on_pixmap, QPixmap* off_pixmap, 
                              //const RouteChannelArray& initial,
                              QWidget* parent = 0);
      
      QWidget* createWidget(QWidget* parent);
      //RouteChannelArray currentState() const { return _current; }
      //void setCurrentState(const QBitArray& state); 
      void setSize(int rows, int cols) { _rows = rows; _cols = cols; }
      bool value(int row, int col) const { return _widget->value(row, col); }
      void setValue(int row, int col, bool value) const { _widget->setValue(row, col, value); }
      };

} // namespace MusEGui
#endif  // __CUSTOM_WIDGET_ACTIONS_H__
