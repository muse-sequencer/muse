//=============================================================================
//  MusE
//  Linux Music Editor
//  custom_widget_actions.cpp
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
//=============================================================================

#include <QLabel>
#include <QHBoxLayout>
#include <QSignalMapper>
#include <QPainter>
#include <QPaintEvent>
//#include <QPainterPath>
#include <QString>
#include <QList>
#include <QApplication>
#include <QStyleOptionButton>
#include <QStyle>
#include <QMenu>
#include <QGraphicsWidget>
#include <QMouseEvent>

#include "icons.h"
#include "pixmap_button.h"
#include "custom_widget_actions.h"

namespace MusEGui {

//---------------------------------------------------------
//   PixmapButtonsHeaderWidgetAction
//---------------------------------------------------------

PixmapButtonsHeaderWidgetAction::PixmapButtonsHeaderWidgetAction(const QString& text, QPixmap* ref_pixmap, int channels, QWidget* parent)
  : QWidgetAction(parent)
{
  _refPixmap = ref_pixmap;
  _channels = channels;
  _text = text;
  // Just to be safe, set to -1 instead of default 0.
  setData(-1);
}

QWidget* PixmapButtonsHeaderWidgetAction::createWidget(QWidget* parent)
{
  QWidget* lw = new QWidget(parent);
  QHBoxLayout* layout = new QHBoxLayout(lw);

  layout->setSpacing(0);
  
  QLabel* lbl = new QLabel(_text, lw);
  lbl->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);
  lbl->setAlignment(Qt::AlignCenter);
  lbl->setAutoFillBackground(true);
  //QPalette palette;
  //palette.setColor(label->backgroundRole(), c);
  //lbl->setPalette(palette);
  lbl->setBackgroundRole(QPalette::Dark);
  layout->addWidget(lbl); 
  
  layout->addSpacing(8);
  //layout->addStretch();
      
  QSignalMapper* mapper = new QSignalMapper(this);

  PixmapButton* pb = new PixmapButton(toggle_small_Icon, toggle_small_Icon, 2, lw, QString("T"));  // Margin  = 2
  //mapper->setMapping(pb, _channels);  // Set to one past end.
  layout->addWidget(pb); 
  layout->addSpacing(6);
  //connect(pb, SIGNAL(clicked(bool)), mapper, SLOT(map()));
  
  for(int i = 0; i < _channels; ++i)
  {
    PixmapButton* b = new PixmapButton(_refPixmap, _refPixmap, 2, lw, QString::number(i + 1));  // Margin  = 2
    mapper->setMapping(b, i);
    connect(b, SIGNAL(pressed()), mapper, SLOT(map()));
    if((i != 0) && (i % 4 == 0))
      layout->addSpacing(6);
    layout->addWidget(b); 
  }

  connect(mapper, SIGNAL(mapped(int)), this, SLOT(chanClickMap(int)));
  
  return lw;
}
  
void PixmapButtonsHeaderWidgetAction::chanClickMap(int /*idx*/)
{
  // TODO: Toggle vertical columns...   p4.0.42 
  
  //trigger();  // REMOVE Tim. Persistent routes. Removed. Already triggered by widget, causes double activation!
}

  
//---------------------------------------------------------
//   PixmapButtonsWidgetAction
//---------------------------------------------------------

PixmapButtonsWidgetAction::PixmapButtonsWidgetAction(const QString& text, QPixmap* on_pixmap, QPixmap* off_pixmap, const QBitArray& initial, QWidget* parent)
  : QWidgetAction(parent)
      {
        _onPixmap = on_pixmap;
        _offPixmap = off_pixmap;
        _current = initial;
        _text = text;
        // Just to be safe, set to -1 instead of default 0.
        setData(-1);
      }

QWidget* PixmapButtonsWidgetAction::createWidget(QWidget *parent)
{
  const int channels = _current.size();
  QWidget* lw = new QWidget(parent);
  QHBoxLayout* layout = new QHBoxLayout(lw);

  layout->setSpacing(0);
  
  QLabel* lbl = new QLabel(_text, lw);
  lbl->setAlignment(Qt::AlignCenter);
  //lbl->setAutoFillBackground(true);
  //QPalette palette;
  //palette.setColor(label->backgroundRole(), c);
  //lbl->setPalette(palette);
  //lbl->setBackgroundRole(QPalette::Dark);
  layout->addWidget(lbl); 
  
  layout->addSpacing(8);
  layout->addStretch();
      
  QSignalMapper* mapper = new QSignalMapper(this);

  PixmapButton* pb = new PixmapButton(toggle_small_Icon, toggle_small_Icon, 2, lw);  // Margin  = 2
  mapper->setMapping(pb, channels);  // Set to one past end.
  layout->addWidget(pb); 
  layout->addSpacing(6);
  connect(pb, SIGNAL(pressed()), mapper, SLOT(map()));
  
  for(int i = 0; i < channels; ++i)
  {
    bool set = _current.at(i);
    PixmapButton* b = new PixmapButton(_onPixmap, _offPixmap, 2, lw);  // Margin  = 2
    _chan_buttons.append(b);
    b->setCheckable(true);
    b->setDown(set);
    mapper->setMapping(b, i);
    connect(b, SIGNAL(toggled(bool)), mapper, SLOT(map()));
    if((i != 0) && (i % 4 == 0))
      layout->addSpacing(6);
    layout->addWidget(b); 
  }

  connect(mapper, SIGNAL(mapped(int)), this, SLOT(chanClickMap(int)));
  
  return lw;
}

void PixmapButtonsWidgetAction::chanClickMap(int idx)
{
  const int channels = _current.size();
  const int buttons_sz = _chan_buttons.size();
  if(idx == channels)  // One past end = Toggle all button.
  {
    int allch = 0;
    for(; allch < channels; ++allch)
    {
      if(!_current.at(allch))
        break;
    }
    if(allch == channels)
    {
      //fprintf(stderr, "PixmapButtonsWidgetAction::chanClickMap: filling bit array with false\n"); // REMOVE Tim. Persistent routes. Added. 
      _current.fill(false);
    }
    else
    {
      //fprintf(stderr, "PixmapButtonsWidgetAction::chanClickMap: filling bit array with true\n"); // REMOVE Tim. Persistent routes. Added. 
      _current.fill(true);
    }
    
    // Set and redraw the buttons.
    for(int i = 0; i < buttons_sz; ++i)
      _chan_buttons.at(i)->setDown(allch != channels);
  }
  else
  {
    for(int i = 0; i < channels && i < buttons_sz; ++i)
    {
      if(_chan_buttons.at(i)->isChecked())
      {
        //fprintf(stderr, "PixmapButtonsWidgetAction::chanClickMap: setting bit:%d\n", i); // REMOVE Tim. Persistent routes. Added. 
        _current.setBit(i);
      }
      else
      {
        //fprintf(stderr, "PixmapButtonsWidgetAction::chanClickMap: clearing bit:%d\n", i); // REMOVE Tim. Persistent routes. Added. 
        _current.clearBit(i);
      }
    }
  }
  
  //trigger();  // REMOVE Tim. Persistent routes. Removed. Already triggered by widget, causes double activation!
}

void PixmapButtonsWidgetAction::setCurrentState(const QBitArray& state)
{
    _current = state;
    const int channels = _current.size();
    const int buttons_sz = _chan_buttons.size();
    // Set and redraw the buttons.
    for(int i = 0; i < channels && i < buttons_sz; ++i)
      _chan_buttons.at(i)->setDown(_current.at(i));
}


// //---------------------------------------------------------
// //   RouteChannelArray
// //---------------------------------------------------------
// 
// RouteChannelArray::RouteChannelArray(int rows, int cols)
// {
//   _array = 0;
//   _header = 0;
//   _cols = cols;
//   _rows = rows;
//   //_visible_cols = cols;
//   //_visible_rows = rows;
//   _rowsExclusive = false;
//   _colsExclusive = false;
//   _exclusiveToggle = false;
//   init();
// }
// 
// RouteChannelArray::~RouteChannelArray()
// {
//   if(_header)
//   {
//     delete[] _header;
//     _header = 0;
//   }
//   if(_array)
//   {
//     delete[] _array;
//     _array = 0;
//   }
// }
// 
// void RouteChannelArray::init()
// {
//   if(_header)
//   {
//     delete[] _header;
//     _header = 0;
//   }
//   if(_array)
//   {
//     delete[] _array;
//     _array = 0;
//   }
//   const int sz = itemCount();
//   if(sz == 0)
//     return;
//   _array = new RouteChannelArrayItem[sz];
//   
//   const int hsz = headerItemCount();
//   if(hsz == 0)
//     return;
//   _header = new RouteChannelArrayHeaderItem[hsz];
// }
// 
// RouteChannelArray& RouteChannelArray::operator=(const RouteChannelArray& a)
// {
//   if(a._cols != _cols || a._rows != _rows)
//   {
//     _cols = a._cols;
//     _rows = a._rows;
//     //_visible_cols = a._visible_cols;
//     //_visible_rows = a._visible_rows;
//     init();
//   }
//   _arrayTitleItem = a._arrayTitleItem;
//   _headerTitleItem = a._headerTitleItem;
//   _rowsExclusive = a._rowsExclusive;
//   _colsExclusive = a._colsExclusive;
//   _exclusiveToggle = a._exclusiveToggle;
//   
//   const int sz = itemCount();
//   if(sz == 0)
//     return *this;
//   for(int i = 0; i < sz; ++i)
//     _array[i] = a._array[i];
// 
//   const int hsz = headerItemCount();
//   if(hsz == 0)
//     return *this;
//   for(int i = 0; i < hsz; ++i)
//     _header[i] = a._header[i];
//   return *this;
// }
//     
// void RouteChannelArray::setSize(int rows, int cols)
// {
//   if(cols == _cols && rows == _rows)
//     return;
//   _cols = cols;
//   _rows = rows;
//   //_visible_cols = cols;
//   //_visible_rows = rows;
//   init();
// }
// 
// bool RouteChannelArray::headerInvalidIndex(int row, int col) const 
// { 
//   if(row == -1)
//     return col == -1 || col >= _cols;
//   if(col == -1)
//     return row >= _rows;
//   return true;
// }
// 
// void RouteChannelArray::setValues(int row, int col, bool value, bool exclusive_rows, bool exclusive_cols, bool exclusive_toggle)
// { 
//   if(invalidIndex(row, col)) 
//     return; 
//   
//   const bool v = (!exclusive_toggle || (exclusive_toggle && value));
//   if(exclusive_rows && exclusive_cols)
//   {
//     for(int r = 0; r < _rows; ++r)
//     {
//       for(int c = 0; c < _cols; ++c)
//         _array[itemIndex(r, c)]._value = (r == row && c == col && v);
//     }
//     return;
//   }
//   
//   if(exclusive_rows)
//   {
//     for(int r = 0; r < _rows; ++r)
//       _array[itemIndex(r, col)]._value = (r == row && v);
//     return;
//   }
//   
//   if(exclusive_cols)
//   {
//     for(int c = 0; c < _cols; ++c)
//       _array[itemIndex(row, c)]._value = (c == col && v); 
//     return;
//   }
//   
//   _array[itemIndex(row, col)]._value = value; 
// }
// 
// void RouteChannelArray::headerSetValues(int row, int col, bool value, bool exclusive_rows, bool exclusive_cols, bool exclusive_toggle)
// { 
//   if(headerInvalidIndex(row, col)) 
//     return; 
//   const bool v = (!exclusive_toggle || (exclusive_toggle && value));
//   if(row != -1 && exclusive_rows)
//   {
//     for(int r = 0; r < _rows; ++r)
//       _header[headerItemIndex(r, -1)]._value = (r == row && v);
//     return;
//   }
//   if(col != -1 && exclusive_cols)
//   {
//     for(int c = 0; c < _cols; ++c)
//       _header[headerItemIndex(-1, c)]._value = (c == col && v); 
//     return;
//   }
//   _header[headerItemIndex(row, col)]._value = value; 
// }
// 
// // void RouteChannelArray::setValue(int row, int col, bool value)
// // { 
// //   setValues(row, col, value, _rowsExclusive, _colsExclusive, _exclusiveToggle);
// // }  
// //   if(invalidIndex(row, col)) 
// //     return; 
//   //_array[itemIndex(row, col)]._value = value; 
// 
// 
// //   if(_rowsExclusive && _colsExclusive)
// //   {
// //     for(int r = 0; r < _rows; ++r)
// //     {
// //       for(int c = 0; c < _cols; ++c)
// //       {
// //         _array[itemIndex(r, col)]._value = (r == row); 
// //       }
// //     }
// //    
// //     return;
// //   }
// //   
// //   if(_rowsExclusive)
// //   {
// //     for(int r = 0; r < _rows; ++r)
// //       _array[itemIndex(r, col)]._value = (r == row); 
// //   }
// //   if(_colsExclusive)
// //   {
// //     for(int c = 0; c < _cols; ++c)
// //       _array[itemIndex(row, c)]._value = (c == col); 
// //   }
// //   if(!_rowsExclusive && !_colsExclusive)
// //     _array[itemIndex(row, col)]._value = value; 
// // }
// 
// // void RouteChannelArray::headerSetValue(int row, int col, bool value)
// // { 
// //   headerSetValues(row, col, value, _rowsExclusive, _colsExclusive, _exclusiveToggle);
// // }  
// 
// 
// // //---------------------------------------------------------
// // //   RouteChannelArrayHeader
// // //---------------------------------------------------------
// // 
// // void RouteChannelArrayHeader::init()
// // {
// //   if(_array)
// //   {
// //     delete[] _array;
// //     _array = 0;
// //   }
// //   const int sz = itemCount();
// //   if(sz == 0)
// //     return;
// //   _array = new RouteChannelArrayItem[sz];
// // }
// // 
// // bool RouteChannelArrayHeader::invalidIndex(int row, int col) const 
// // { 
// //   if(row == -1)
// //     return col == -1 || col >= _cols;
// //   if(col == -1)
// //     return row >= _rows;
// //   return true;
// // }
// // 
// // void RouteChannelArrayHeader::setValues(int row, int col, bool value, bool exclusive_rows, bool exclusive_cols, bool exclusive_toggle)
// // { 
// //   if(invalidIndex(row, col)) 
// //     return; 
// //   const bool v = (!exclusive_toggle || (exclusive_toggle && value));
// //   if(row != -1 && exclusive_rows)
// //   {
// //     for(int r = 0; r < _rows; ++r)
// //       _array[itemIndex(r, -1)]._value = (r == row && v);
// //     return;
// //   }
// //   if(col != -1 && exclusive_cols)
// //   {
// //     for(int c = 0; c < _cols; ++c)
// //       _array[itemIndex(-1, c)]._value = (c == col && v); 
// //     return;
// //   }
// //   _array[itemIndex(row, col)]._value = value; 
// // }
// 
// //---------------------------------------------------------
// //   RoutingMatrixWidget
// //---------------------------------------------------------
// 
// RoutingMatrixWidget::RoutingMatrixWidget(RoutingMatrixWidgetAction* action, QWidget* parent)
//   : QWidget(parent)
// {
//   _action = action;
// }
// 
// QSize RoutingMatrixWidget::sizeHint() const
// {
// //   const int cellW = _action->cellGeometry().width();
// //   const int cellH = _action->cellGeometry().height();
// //   const int c_groups = _action->array()->columns() / _action->itemsPerGroup;
// //   const int r_groups = _action->array()->rows() / _action->itemsPerGroup;
// //   const int w = (_action->array()->columns() + 1) * (cellW + _action->itemHSpacing) +  // 1 extra for the vertical header column.
// //                 c_groups * _action->groupSpacing + 
// //                 2 * _action->margin;
// //   const int h = (_action->array()->rows() + 1) * (cellH + _action->itemVSpacing) +    // 1 extra for the horizontal header row.
// //                 r_groups * _action->groupSpacing + 
// //                 2 * _action->margin;
//   
// /*                
//   int w = 0;
//   int h = 0;
//   const int rows = _action->array()->rows();
//   const int cols = _action->array()->columns();
//   for(int row = 0; row < rows; ++row)
//   {
//     for(int col = 0; col < cols; ++col)
//     {
//       const QString str = _action->array()->text(row, col);
//       if(str.isEmpty())
//       {
//         w += cellW + _action->itemHSpacing;
//         h += cellH + _action->itemVSpacing;
//       }
//       else
//       {
//         QFont fnt = font();
//         //fnt.setPointSize(6);
//         const QFontMetrics fm(fnt);
//         const QRect r = fm.boundingRect(str);
//         int r_w = r.width();
//         int r_h = r.height();
//         if(_action->onPixmap() && r_w < _action->onPixmap()->width())
//           r_w = _action->onPixmap()->width();
//         if(_action->onPixmap() && r_h < _action->onPixmap()->height())
//           r_h = _action->onPixmap()->height();
//         w += r_w + _action->itemHSpacing;
//         h += r_h + _action->itemVSpacing;
//       }
//     }
//   }
//   
//   const int h_rows = _action->header()->rows();
//   const int h_cols = _action->header()->columns();
//   int max_hdr_w = 0;
//   for(int row = 0; row < h_rows; ++row)
//   {
//     const QString str = _action->header()->text(row, -1);
//     if(str.isEmpty())
//       h += cellH + _action->itemVSpacing;
//     else
//     {
//       QFont fnt = font();
//       //fnt.setPointSize(6);
//       const QFontMetrics fm(fnt);
//       const QRect r = fm.boundingRect(str);
//       int r_w = r.width();
//       if(_action->onPixmap() && r_w < _action->onPixmap()->width())
//         r_w = _action->onPixmap()->width();
//       if(r_w > max_hdr_w)
//         max_hdr_w = r_w;
//       int r_h = r.height();
//       if(_action->onPixmap() && r_h < _action->onPixmap()->height())
//         r_h = _action->onPixmap()->height();
//       h += r_h + _action->itemVSpacing;
//     }
//   }
//   int max_hdr_h = 0;
//   for(int col = 0; col < h_cols; ++col)
//   {
//     const QString str = _action->header()->text(-1, col);
//     if(str.isEmpty())
//       w += cellW + _action->itemHSpacing;
//     else
//     {
//       QFont fnt = font();
//       //fnt.setPointSize(6);
//       const QFontMetrics fm(fnt);
//       const QRect r = fm.boundingRect(str);
//       int r_w = r.width();
//       if(_action->onPixmap() && r_w < _action->onPixmap()->width())
//         r_w = _action->onPixmap()->width();
//       int r_h = r.height();
//       if(_action->onPixmap() && r_h < _action->onPixmap()->height())
//         r_h = _action->onPixmap()->height();
//       if(r_h > max_hdr_h)
//         max_hdr_h = r_h;
//       w += r_w + _action->itemHSpacing;
//     }
//   }
//   w += max_hdr_w + c_groups * _action->groupSpacing + 2 * _action->margin;
//   h += max_hdr_h + r_groups * _action->groupSpacing + 2 * _action->margin;
//   */
//      
//   const int rows = _action->array()->rows();
//   const int cols = _action->array()->columns();
//   //const int h_rows = _action->header()->rows();
//   //const int h_cols = _action->header()->columns();
//   const QRect last_array_v_cell = _action->array()->rect(rows - 1, 0);
//   const QRect last_array_h_cell = _action->array()->rect(0, cols - 1);
//   // Take the height of any horizontal header column - they're all the same.
//   const int h = //_action->header()->rect(-1, 0).height() + 
//                 last_array_v_cell.y() + last_array_v_cell.height() +
//                 2 * _action->margin;
//   const QString array_title_str = _action->array()->arrayTitle();
//   const QRect array_title_r = _action->array()->arrayTitleRect();
//   // Take the width of any vertical header row - they're all the same.
//   const int w = (array_title_str.isEmpty() ? (last_array_h_cell.x() + last_array_h_cell.width()) : (array_title_r.x() + array_title_r.width())) +
//                 2 * _action->margin; // +
//                 // Take the width of any vertical header row checkbox - they're all the same. Not necessary - already taken into account.
//                 //_action->array()->headerCheckBoxRect(0).width();
//                 //_action->isCheckable() ? cb_rect.width() : 0;
//   //const int w = //_action->header()->rect(0, -1).width() +
//   //              last_array_h_cell.x() + last_array_h_cell.width() +
//   //              2 * _action->margin;
// //   for(int row = 0; row < h_rows; ++row)
// //     h += _action->header()->rect(row, -1).height();
// //   for(int col = 0; col < h_cols; ++col)
// //     w += _action->header()->rect(-1, col).width();
//   
// //   w += 2 * _action->margin;
// //   h += 2 * _action->margin;
//   
//   return QSize(w, h);
// }
// 
// void RoutingMatrixWidget::resizeEvent(QResizeEvent* e)
// {
//   fprintf(stderr, "RoutingMatrixWidget::resizeEvent\n");
//   QWidget::resizeEvent(e);
// }
// 
// 
// void RoutingMatrixWidget::drawGrid(QPainter& p)
// {
//   const int rows = _action->array()->rows();
//   const int cols = _action->array()->columns();
//   const int cellW = _action->maxPixmapGeometry().width();
//   const int cellH = _action->maxPixmapGeometry().height();
//   const int c_groups = _action->array()->columns() / _action->itemsPerGroup;
//   const int r_groups = _action->array()->rows() / _action->itemsPerGroup;
//   const int w = _action->margin + _action->array()->columns() * (cellW + _action->itemHSpacing) + 
//                 c_groups * _action->groupSpacing +
//                 cellW; // One more for the last line, minus the itemSpacing.
//   const int h = _action->margin + _action->array()->rows() * (cellH + _action->itemVSpacing) + 
//                 r_groups * _action->groupSpacing +
//                 cellH; // One more for the last line, minus the itemSpacing.
//   const int x0 = _action->margin + cellW;
//   const int x1 = w;
//   const int y0 = _action->margin + cellH;
//   const int y1 = h;
//   
//   int y = _action->margin + cellH;
//   for(int row = 0; row <= rows; ++row) // Using <= to get the last line.
//   {
//     int line_y;
//     if(row != 0 && ((row % _action->itemsPerGroup) == 0))
//     {
//       line_y = y + (_action->itemVSpacing + _action->groupSpacing) / 2;
//       y += _action->groupSpacing;
//     }
//     else
//       line_y = y + _action->itemVSpacing / 2;
//     p.drawLine(x0, line_y, x1, line_y);
//     y += cellH + _action->itemVSpacing;
//   }
//   
//   int x = _action->margin + cellW;
//   for(int col = 0; col <= cols; ++col) // Using <= to get the last line.
//   {
//     int line_x;
//     if(col != 0 && ((col % _action->itemsPerGroup) == 0))
//     {
//       line_x = x + (_action->itemHSpacing + _action->groupSpacing) / 2;
//       x += _action->groupSpacing;
//     }
//     else
//       line_x = x + _action->itemHSpacing / 2;
//     p.drawLine(line_x, y0, line_x, y1);
//     x += cellW + _action->itemHSpacing;
//   }
// }
// 
// void RoutingMatrixWidget::paintEvent(QPaintEvent* /*event*/)
// {
//   QPainter p(this);
// 
//   // Not used - too cluttered, like looking through a screen, too hard to distinguish the squares and the
//   //  fact that with a grid, 'off' means no pixmap or drawing at all, so only the grid shows so it's hard
//   //  to pick the right box to click on. And the added group spacing makes it look distorted and confusing.
//   //drawGrid(p); 
//   
//   const int rows = _action->array()->rows();
//   const int cols = _action->array()->columns();
//   p.setFont(_action->smallFont());
//   for(int row = 0; row < rows; ++row)
//   {
//     for(int col = 0; col < cols; ++col)
//     {
//       const QPixmap& pm = _action->array()->value(row, col) ? *_action->onPixmap() : *_action->offPixmap();
//       const int pm_w = pm.width();
//       const int pm_h = pm.height();
//       const QRect r = _action->array()->rect(row, col);
//       int x = r.x();
//       if(r.width() > pm_w)
//         x += (r.width() - pm_w) / 2;
//       int y = r.y();
//       if(r.height() > pm_h)
//         y += (r.height() - pm_h) / 2;
//       p.drawPixmap(x, y, pm);
//     }
//   }
//   
//   //p.setFont(_action->font());
//   //const int h_rows = _action->header()->rows();
//   //const int h_cols = _action->header()->columns();
//   //const int vis_h_rows = _action->header()->visibleRows();
//   //const int vis_h_cols = _action->header()->visibleColumns();
//   const int vis_h_rows = rows;
//   const int vis_h_cols = cols;
//   for(int row = 0; row < vis_h_rows; ++row)
//   {
//     if(_action->isCheckable())
//     {
//       QStyle* st = style() ? style() : QApplication::style();
//       if(st)
//       {
//         QStyleOptionButton option;
//         option.state = QStyle::State_Active | QStyle::State_Enabled | //QStyle::State_HasFocus | // TODO
//                       (_action->array()->headerValue(row, -1) ? QStyle::State_On : QStyle::State_Off);
//         option.rect = _action->array()->headerCheckBoxRect(row);
// 
//          // REMOVE Tim. Persistent routes. Added.
//         fprintf(stderr, "RoutingMatrixWidgetAction::paintEvent rect x:%d y:%d w:%d h:%d\n", 
//                 option.rect.x(), option.rect.y(), 
//                 option.rect.width(), option.rect.height());
// 
//         st->drawPrimitive(QStyle::PE_IndicatorItemViewItemCheck, &option, &p);
//         
//         //if(cb_rect.width() > max_v_hdr_w)
//         //  max_v_hdr_w = cb_rect.width();
//       }
//     }  
//     
//     
//     
//     //const QRect r = _action->header()->rect(row, -1);
//     //const QString str = _action->header()->text(row, -1);
//     const QRect r = _action->array()->headerRect(row, -1);
//     const QString str = _action->array()->headerText(row, -1);
//     if(str.isEmpty())
//     {
//       p.setFont(_action->smallFont());
//       p.drawText(r, Qt::AlignRight | Qt::AlignVCenter, QString::number(row + 1));
//     }
//     else
//     {
//       p.setFont(_action->font());
//       p.drawText(r, Qt::AlignLeft | Qt::AlignVCenter, str);
//     }
//   }
//   for(int col = 0; col < vis_h_cols; ++col)
//   {
//     //const QRect r = _action->header()->rect(-1, col);
//     //const QString str = _action->header()->text(-1, col);
//     const QRect r = _action->array()->headerRect(-1, col);
//     const QString str = _action->array()->headerText(-1, col);
//     if(str.isEmpty())
//     {
//       p.setFont(_action->smallFont());
//       p.drawText(r, Qt::AlignCenter, QString::number(col + 1));
//     }
//     else
//     {
//       p.setFont(_action->font());
//       p.drawText(r, Qt::AlignCenter, str);
//     }
//   }
//   
//   const QString header_title_str = _action->array()->headerTitle();
//   if(!header_title_str.isEmpty())
//   {
//     p.setFont(_action->font());
//     p.drawText(_action->array()->headerTitleRect(), Qt::AlignHCenter | Qt::AlignVCenter, header_title_str);
//   }
//   
//   const QString array_title_str = _action->array()->arrayTitle();
//   if(!array_title_str.isEmpty())
//   {
//     p.setFont(_action->font());
//     p.drawText(_action->array()->arrayTitleRect(), Qt::AlignHCenter | Qt::AlignVCenter, array_title_str);
//   }
// }
// 
// void RoutingMatrixWidget::mousePressEvent(QMouseEvent* ev)
// {
//   // REMOVE Tim. Persistent routes. Added.
//   fprintf(stderr, "RoutingMatrixWidget::mousePressEvent\n");
//   const int rows = _action->array()->rows();
//   const int cols = _action->array()->columns();
//   const QPoint pt = ev->pos();
//   bool changed = false;
//   for(int row = 0; row < rows; ++row)
//   {
//     if(_action->isCheckable())
//     {
//       const QRect cb_rect = _action->array()->headerCheckBoxRect(row);
//       if(cb_rect.contains(pt))
//       {
//         _action->array()->headerSetValue(row, -1, !_action->array()->headerValue(row, -1));  // TODO: Add a toggleValue.
//         changed = true;
//         break;
//       }
//     }
//     
//     for(int col = 0; col < cols; ++col)
//     {
//       const QRect rect = _action->array()->rect(row, col);
//       if(rect.contains(pt))
//       {
//         _action->array()->setValue(row, col, !_action->array()->value(row, col));  // TODO: Add a toggleValue.
//         changed = true;
//         break;
//       }
//     }
//     if(changed)
//       break;
//   }
// 
// //   if(changed)
// //   {
// //     //ev->accept();
// //     update();  // Redraw the indicators.
// //     //return;
// //   }
// //   
// //   ev->ignore();  // Don't accept. Let the menu close if neccessary.
//   
//   
//   if(changed)
//   {
//     //ev->accept();
//     update();  // Redraw the indicators.
//     ev->ignore();  // Don't accept. Let the menu close if neccessary.
//     //return;
//   }
//   else
//     // Nothing clicked? Stay open.
//     ev->accept();
// }
// 
// void RoutingMatrixWidget::mouseReleaseEvent(QMouseEvent* ev)
// {
//   // REMOVE Tim. Persistent routes. Added.
//   fprintf(stderr, "RoutingMatrixWidget::mouseReleaseEvent\n");
//   ev->ignore();
// }
// 
// void RoutingMatrixWidget::mouseDoubleClickEvent(QMouseEvent* ev)
// {
//   // REMOVE Tim. Persistent routes. Added.
//   fprintf(stderr, "RoutingMatrixWidget::mouseDoubleClickEvent\n");
//   ev->accept(); 
// }
// 
// //---------------------------------------------------------
// //   RoutingMatrixWidgetAction
// //---------------------------------------------------------
// 
// const int RoutingMatrixWidgetAction::margin = 1;
// const int RoutingMatrixWidgetAction::itemHSpacing = 1;
// const int RoutingMatrixWidgetAction::itemVSpacing = 3;
// const int RoutingMatrixWidgetAction::groupSpacing = 4;
// const int RoutingMatrixWidgetAction::itemsPerGroup = 4;
// 
// RoutingMatrixWidgetAction::RoutingMatrixWidgetAction(int rows, int cols, 
//                                                      QPixmap* on_pixmap, QPixmap* off_pixmap, 
//                                                      QWidget* parent)
//                                                      //int visible_rows, int visible_cols)
//   : QWidgetAction(parent)
// {
//   // Just to be safe, set to -1 instead of default 0.
//   //setData(-1);
//   _onPixmap = on_pixmap;
//   _offPixmap = off_pixmap;
//   //_header.setSize(rows, cols);
//   //if(visible_rows != -1)
//   //  _header.setVisibleRows(visible_rows);
//   //if(visible_cols != -1)
//   //  _header.setVisibleColums(visible_cols);
//   _array.setSize(rows, cols);
//   _smallFont = font();
//   //_smallFont.setPointSize(6);
//   _smallFont.setPointSize(_smallFont.pointSize() / 2 + 1);
//   
//   // REMOVE Tim. Just a test.
//   setCheckable(true);
//   setChecked(true);
//    
// //   const QFontMetrics sfm(_smallFont);
// //   //_cellGeometry = fm.boundingRect("888");
// //   const QRect sbrect = sfm.boundingRect("888");
// //   const QFontMetrics fm(font());
// //   const QRect brect = fm.boundingRect("jT");
// 
// //   if(_cellGeometry.width() < brect->width())
// //     _cellGeometry.setWidth(brect->width());
// //   if(_cellGeometry.height() < _onPixmap->height())
// //     _cellGeometry.setHeight(_onPixmap->height());
//   
//   if(_maxPixmapGeometry.width() < _onPixmap->width())
//     _maxPixmapGeometry.setWidth(_onPixmap->width());
//   if(_maxPixmapGeometry.height() < _onPixmap->height())
//     _maxPixmapGeometry.setHeight(_onPixmap->height());
//   
//   if(_maxPixmapGeometry.width() < _offPixmap->width())
//     _maxPixmapGeometry.setWidth(_offPixmap->width());
//   if(_maxPixmapGeometry.height() < _offPixmap->height())
//     _maxPixmapGeometry.setHeight(_offPixmap->height());
//   updateChannelArray();
// }
// 
// void RoutingMatrixWidgetAction::updateChannelArray()
// {
//   const int rows = _array.rows();
//   const int cols = _array.columns();
//   const int cellW = _maxPixmapGeometry.width();
//   const int cellH = _maxPixmapGeometry.height();
//   
//   int menu_h_margin = 8;
//   //int menu_v_margin = 0;
//   QRect cb_rect; // Combo box rectangle, if enabled.
//   
// //   int y = margin + cellH + itemVSpacing;
// //   for(int row = 0; row < rows; ++row)
// //   {
// //     if(row != 0 && ((row % itemsPerGroup) == 0))
// //       y += groupSpacing;
// //     int x = margin + cellW + itemHSpacing;
// //     for(int col = 0; col < cols; ++col)
// //     {
// //       if(col != 0 && ((col % itemsPerGroup) == 0))
// //         x += groupSpacing;
// //       
// //       const QRect r(x, y, cellW, cellH);
// //       _array.setRect(row, col, r);
// //       
// //       x += cellW + itemHSpacing;
// //     }
// //     y += cellH + itemVSpacing;
// //   }
// 
//   //const int h_rows = _header.rows();
//   //const int h_cols = _header.columns();
//   const int h_rows = _array.rows();
//   const int h_cols = _array.columns();
//   
//   // Determine the maximum vertical header width.
//   int max_v_hdr_w = 0;
//   //if(_header.visibleRows() != 0)
//   //{
//     for(int row = 0; row < h_rows; ++row)
//     {
//       //const QString str = _header.text(row, -1);
//       const QString str = _array.headerText(row, -1);
// //       if(str.isEmpty())
// //       {
// //         const int cell_w = cellW + itemHSpacing;
// //         if(cell_w > max_v_hdr_w)
// //           max_v_hdr_w = cell_w;
// //       }
// //       else
// //       {
// //         QFont fnt = font();
// //         //fnt.setPointSize(6);
// //         const QFontMetrics fm(fnt);
// //         const QRect r = fm.boundingRect(str);
// //         const int r_w = r.width();
// //         if(r_w > max_v_hdr_w)
// //           max_v_hdr_w = r_w;
// //       }
//       const QFontMetrics fm(str.isEmpty() ? smallFont() : font());
//       const QRect brect = fm.boundingRect(str.isEmpty() ? "888" : str);
//       const int r_w = brect.width();
//       if(r_w > max_v_hdr_w)
//         max_v_hdr_w = r_w;
//     }
//   //}
//   
//   QStyle* style = parentWidget() ? parentWidget()->style() : QApplication::style();
//   if(style)
//   {
//     {
//       //QStyleOptionButton option;
//       //option.state = QStyle::State_Active | QStyle::State_Enabled | QStyle::State_HasFocus |
//       //              QStyle::State_On;
//       //option.rect = QRect(0, 0, 100, 100);              
//       //menu_h_margin = style->pixelMetric(QStyle::PM_MenuHMargin, &option, parentWidget());
//       // FIXME: Returns zero. May need to pass menu. How to find the menu this action is part of?
//       //menu_h_margin = style->pixelMetric(QStyle::PM_MenuHMargin, 0, parentWidget());
//       // REMOVE Tim. Persistent routes. Added.
//       //fprintf(stderr, "RoutingMatrixWidgetAction::updateChannelArray menu_h_margin:%d\n", menu_h_margin);
//     }
// 
//     
//     QStyleOptionMenuItem option;
//     option.checkType = QStyleOptionMenuItem::NonExclusive;
//     option.checked = true;
//     option.text = QString("testing one two three");
//     //option.state = QStyle::State_Active | QStyle::State_Enabled | QStyle::State_HasFocus |
//     //            QStyle::State_On;
//     //option.rect = QRect(0, 0, 100, 100);
//     QSize sz = style->sizeFromContents(QStyle::CT_MenuItem, &option, QSize(10, 10)); //, q);
//     // REMOVE Tim. Persistent routes. Added.
//     fprintf(stderr, "RoutingMatrixWidgetAction::updateChannelArray sz w:%d h:%d\n", sz.width(), sz.height());
//     QSize sz2 = style->sizeFromContents(QStyle::CT_MenuItem, &option, QSize(100, 100)); //, q);
//     // REMOVE Tim. Persistent routes. Added.
//     fprintf(stderr, "RoutingMatrixWidgetAction::updateChannelArray sz2 w:%d h:%d\n", sz2.width(), sz2.height());
//     QSize sz3 = style->sizeFromContents(QStyle::CT_MenuItem, &option, QSize(1000, 700)); //, q);
//     // REMOVE Tim. Persistent routes. Added.
//     fprintf(stderr, "RoutingMatrixWidgetAction::updateChannelArray sz3 w:%d h:%d\n", sz3.width(), sz3.height());
// 
//     //menu_v_margin = style->pixelMetric(QStyle::PM_MenuVMargin);
//     
//     // Account for the check box rectangle, if enabled.
//     if(isCheckable())
//     {
//       QStyleOptionButton option;
//       option.state = QStyle::State_Active | QStyle::State_Enabled | QStyle::State_HasFocus |
//                     QStyle::State_On;
//       cb_rect = style->subElementRect(QStyle::SE_CheckBoxClickRect, &option);
//       //if(cb_rect.width() > max_v_hdr_w)
//       //  max_v_hdr_w = cb_rect.width();
//     }
//   }  
//   
// 
//   // Determine the maximum horizontal header height.
//   int max_h_hdr_h = 0;
//   //if(_header.visibleColumns() != 0)
//   //{
//     for(int col = 0; col < h_cols; ++col)
//     {
//       //const QString str = _header.text(-1, col);
//       const QString str = _array.headerText(-1, col);
// //       if(str.isEmpty())
// //       {
// //         const QFontMetrics fm(smallFont());
// //         const QRect brect = fm.boundingRect("8");
// //         //const int cell_h = cellH + itemVSpacing;
// //         const int cell_h = brect.height() + itemVSpacing;
// //         if(cell_h > max_h_hdr_h)
// //           max_h_hdr_h = cell_h;
// //       }
// //       else
// //       {
// //         //QFont fnt = font();
// //         //fnt.setPointSize(6);
// //         //const QFontMetrics fm(fnt);
// //         const QFontMetrics fm(font());
// //         const QRect brect = fm.boundingRect(str);
// //         const int r_h = brect.height();
// //         if(r_h > max_h_hdr_h)
// //           max_h_hdr_h = r_h;
// //       }
//       const QFontMetrics fm(str.isEmpty() ? smallFont() : font());
//       const QRect brect = fm.boundingRect(str.isEmpty() ? "8" : str);
//       const int r_h = brect.height();
//       if(r_h > max_h_hdr_h)
//         max_h_hdr_h = r_h;
//     }
//   //}
//   
//   int max_title_h = 0;
//   const QString h_ttl = _array.headerTitle();
//   if(!h_ttl.isEmpty())
//   {
//     const QFontMetrics fm(font());
//     const QRect r = fm.boundingRect(h_ttl);
//     const int r_w = r.width();
//     const int r_h = r.height();
//     if(r_w > max_v_hdr_w)
//       max_v_hdr_w = r_w;
//     //if(r_h > max_h_hdr_h)
//     //  max_h_hdr_h = r_h;
//     if(r_h > max_title_h)
//       max_title_h = r_h;
//     //_array.headerSetTitleRect(QRect(0, 0, max_v_hdr_w, max_h_hdr_h));
//   }
//   
//   int array_title_txt_w = 0;
//   const QString a_ttl = _array.arrayTitle();
//   if(!a_ttl.isEmpty())
//   {
//     const QFontMetrics fm(font());
//     const QRect r = fm.boundingRect(a_ttl);
//     //const int r_w = r.width();
//     array_title_txt_w = r.width();
//     const int r_h = r.height();
//     //if(r_w > max_v_hdr_w)
//     //  max_v_hdr_w = r_w;
//     //if(r_h > max_h_hdr_h)
//     //  max_h_hdr_h = r_h;
//     if(r_h > max_title_h)
//       max_title_h = r_h;
//     //_array.setArrayTitleRect(QRect(max_v_hdr_w, 0, r_w, max_h_hdr_h));
//   }
// 
//   //max_h_hdr_h += max_title_h;
//   
//   // Set the vertical header rectangles.
//   int x = margin + menu_h_margin + cb_rect.width() + menu_h_margin;
//   //y = margin + cellH + itemVSpacing;
//   int y = margin + max_h_hdr_h + max_title_h + itemVSpacing;
//   for(int row = 0; row < h_rows; ++row)
//   {
//     if(row != 0 && ((row % itemsPerGroup) == 0))
//       y += groupSpacing;
// 
//     //int cell_h;
//     //const QString str = _header.text(row, -1);
//     const QString str = _array.headerText(row, -1);
// //     if(str.isEmpty())
// //       cell_h = cellH + itemVSpacing;
// //     else
// //     {
// //       QFont fnt = font();
// //       //fnt.setPointSize(6);
// //       const QFontMetrics fm(fnt);
// //       const QRect r = fm.boundingRect(str);
// //       int r_h = r.height();
// //       if(onPixmap() && r_h < onPixmap()->height())
// //         r_h = onPixmap()->height();
// //       cell_h = r_h + itemVSpacing;
// //     }
//     const QFontMetrics fm(str.isEmpty() ? smallFont() : font());
//     const QRect brect = fm.boundingRect(str.isEmpty() ? "8" : str);
//     int r_h = brect.height();
//     if(r_h < cellH)
//       r_h = cellH;
//     if(cb_rect.height() > r_h)
//       r_h = cb_rect.height();
//     const int cell_h = r_h + itemVSpacing;
// 
//     //if(r_w > max_v_hdr_w)
//     //  max_v_hdr_w = r_w;
//     
//     //const QRect r(x, y, cellW, cellH);
//     const QRect r(x, y, max_v_hdr_w, cell_h);
//     //_header.setRect(row, -1, r);
//     _array.headerSetRect(row, -1, r);
//     const int ofs = (r_h > cb_rect.height()) ? (r_h - cb_rect.height()) / 2 : 0;
//     const QRect cbr(margin + menu_h_margin, y + ofs, cb_rect.width(), cb_rect.height());
//     _array.headerSetCheckBoxRect(row, cbr);
//     //y += cellH + itemVSpacing;
//     y += cell_h;
//   }
//   
//   // Set the horizontal header rectangles.
//   //x = margin + cellW + itemHSpacing;
//   int tot_array_w = 0;
//   x = margin + menu_h_margin + cb_rect.width() + menu_h_margin + max_v_hdr_w + itemHSpacing;
//   y = margin + max_title_h;
//   for(int col = 0; col < h_cols; ++col)
//   {
//     if(col != 0 && ((col % itemsPerGroup) == 0))
//     {
//       x += groupSpacing;
//       tot_array_w += groupSpacing;
//     }
//     
//     //int cell_w;
//     //const QString str = _header.text(-1, col);
//     const QString str = _array.headerText(-1, col);
// //     if(str.isEmpty())
// //       cell_w = cellW + itemHSpacing;
// //     else
// //     {
// //       QFont fnt = font();
// //       //fnt.setPointSize(6);
// //       const QFontMetrics fm(fnt);
// //       const QRect r = fm.boundingRect(str);
// //       int r_w = r.width();
// //       if(onPixmap() && r_w < onPixmap()->width())
// //         r_w = onPixmap()->width();
// //       cell_w = r_w + itemHSpacing;
// //     }
//     const QFontMetrics fm(str.isEmpty() ? smallFont() : font());
//     const QRect brect = fm.boundingRect(str.isEmpty() ? "888" : str);
//     int r_w = brect.width();
//     if(r_w < cellW)
//       r_w = cellW;
//     const int cell_w = r_w + itemHSpacing;
//     //const QRect r(x, y, cellW, cellH);
//     const QRect r(x, y, cell_w, max_h_hdr_h);
//     //_header.setRect(-1, col, r);
//     _array.headerSetRect(-1, col, r);
//     //x += cellW + itemHSpacing;
//     x += cell_w;
//     tot_array_w += cell_w;
//   }
//   
//   const int array_title_w = array_title_txt_w > tot_array_w ? array_title_txt_w : tot_array_w;
//   const QRect array_title_r(margin + max_v_hdr_w + itemHSpacing, margin, array_title_w, max_title_h);
//   _array.setArrayTitleRect(array_title_r);
//   const QRect header_title_r(margin, margin, max_v_hdr_w, max_title_h);
//   _array.headerSetTitleRect(header_title_r);
//   
//   // Set the array rectangles.
//   //int y = margin + cellH + itemVSpacing;
//   y = margin + max_h_hdr_h + max_title_h + itemVSpacing;
//   for(int row = 0; row < rows; ++row)
//   {
//     if(row != 0 && ((row % itemsPerGroup) == 0))
//       y += groupSpacing;
//     //const int cell_h = _header.rect(row, -1).height();
//     const int cell_h = _array.headerRect(row, -1).height();
//     //int x = margin + cellW + itemHSpacing;
//     x = margin + menu_h_margin + cb_rect.width() + menu_h_margin + max_v_hdr_w + itemHSpacing;
//     for(int col = 0; col < cols; ++col)
//     {
//       if(col != 0 && ((col % itemsPerGroup) == 0))
//         x += groupSpacing;
//       
//       //const int cell_w = _header.rect(-1, col).width();
//       const int cell_w = _array.headerRect(-1, col).width();
//       
//       //const QRect r(x, y, cellW, cellH);
//       const QRect r(x, y, cell_w, cell_h);
//       _array.setRect(row, col, r);
//       
//       //x += cellW + itemHSpacing;
//       x += cell_w;
//     }
//     //y += cellH + itemVSpacing;
//     y += cell_h;
//   }
//   
// //   QList<QWidget*> created_widgets(createdWidgets());
// //   //Q_Q(QAction);
// //   QActionEvent e(QEvent::ActionChanged, this);
// //   //QActionEvent e(QEvent::ActionChanged, q);
// //   if(!created_widgets.isEmpty())
// //   {
// //     QWidget* w = created_widgets.front();
// //     fprintf(stderr, "RoutingMatrixWidgetAction::updateChannelArray widget:%p\n", w); // REMOVE Tim. Persistent routes. Added. 
// //     //w->adjustSize();
// //     //w->update();
// //     QApplication::sendEvent(w, &e);
// //   }
// //   QApplication::sendEvent(this, &e);
// //   //QApplication::sendEvent(q, &e);
// //   
// // //   Q_Q(QAction);
// // //   QActionEvent e(QEvent::ActionChanged, q);
// // //   for (int i = 0; i < widgets.size(); ++i) {
// // //     QWidget *w = widgets.at(i);
// // //     QApplication::sendEvent(w, &e);
// // //   }
// // //   QApplication::sendEvent(q, &e);
// // 
// //   //emit q->changed();
// //   emit changed();
// 
//   // Force it to resize. This is one of only a few methods we can call 
//   //  to trigger it which don't check to see if the value is already set.
//   setMenu(menu());
// 
//   
// //   // Determine the maximum row height and column width.
// //   //int y = margin + cellH + itemVSpacing;
// //   int y = margin + max_h_hdr_h + itemVSpacing;
// //   int max_row_h = 0;
// //   int max_col_w = 0;
// //   for(int row = 0; row < rows; ++row)
// //   {
// //     if(row != 0 && ((row % itemsPerGroup) == 0))
// //       y += groupSpacing;
// //     //int x = margin + cellW + itemHSpacing;
// //     int x = margin + max_v_hdr_w + itemHSpacing;
// //     for(int col = 0; col < cols; ++col)
// //     {
// //       if(col != 0 && ((col % itemsPerGroup) == 0))
// //         x += groupSpacing;
// // 
// // 
// //       const QString str = _array.text(row, col);
// //       if(str.isEmpty())
// //       {
// //         
// //       }
// //       else
// //       {
// //         QFont fnt = font();
// //         //fnt.setPointSize(6);
// //         const QFontMetrics fm(fnt);
// //         const QRect r = fm.boundingRect(str);
// //         const int r_w = r.width();
// //         const int r_h = r.height();
// //         if(r_h > max_h_hdr_h)
// //           max_h_hdr_h = r_h;
// //       }
// //       
// //       const QRect r(x, y, cellW, cellH);
// //       _array.setRect(row, col, r);
// //       
// //       x += cellW + itemHSpacing;
// //     }
// //     y += cellH + itemVSpacing;
// //   }
//   
// }
// 
// QWidget* RoutingMatrixWidgetAction::createWidget(QWidget *parent)
// {
//   RoutingMatrixWidget* widget = new RoutingMatrixWidget(this, parent);
//   fprintf(stderr, "RoutingMatrixWidgetAction::createWidget widget:%p\n", widget); // REMOVE Tim. Persistent routes. Added. 
//   return widget;
// }
// 
// void RoutingMatrixWidgetAction::deleteWidget(QWidget* widget)
// {
//   fprintf(stderr, "RoutingMatrixWidgetAction::deleteWidget widget:%p\n", widget); // REMOVE Tim. Persistent routes. Added. 
//   QWidgetAction::deleteWidget(widget);
// }  
// 
// void RoutingMatrixWidgetAction::activate(ActionEvent event)
// {
//   fprintf(stderr, "RoutingMatrixWidgetAction::activate event:%d\n", event); // REMOVE Tim. Persistent routes. Added. 
//   QWidgetAction::activate(event);
// }


//---------------------------------------------------------
//   RouteChannelArray
//---------------------------------------------------------

RouteChannelArray::RouteChannelArray(int cols)
{
  _array = 0;
  _headerVisible = true;
  _header = 0;
  _cols = cols;
  _colsExclusive = false;
  _exclusiveToggle = false;
  _activeCol = -1;
  init();
}

RouteChannelArray::~RouteChannelArray()
{
  if(_header)
  {
    delete[] _header;
    _header = 0;
  }
  if(_array)
  {
    delete[] _array;
    _array = 0;
  }
}

void RouteChannelArray::init()
{
  if(_header)
  {
    delete[] _header;
    _header = 0;
  }
  if(_array)
  {
    delete[] _array;
    _array = 0;
  }
  const int sz = itemCount();
  if(sz == 0)
    return;
  _array = new RouteChannelArrayItem[sz];
  
  const int hsz = headerItemCount();
  if(hsz == 0)
    return;
  _header = new RouteChannelArrayHeaderItem[hsz];
}

RouteChannelArray& RouteChannelArray::operator=(const RouteChannelArray& a)
{
  if(a._cols != _cols)
  {
    _cols = a._cols;
    init();
  }
  _headerVisible = a._headerVisible;
  _arrayTitleItem = a._arrayTitleItem;
  _headerTitleItem = a._headerTitleItem;
  _colsExclusive = a._colsExclusive;
  _exclusiveToggle = a._exclusiveToggle;
  
  const int sz = itemCount();
  if(sz == 0)
    return *this;
  for(int i = 0; i < sz; ++i)
    _array[i] = a._array[i];

  const int hsz = headerItemCount();
  if(hsz == 0)
    return *this;
  for(int i = 0; i < hsz; ++i)
    _header[i] = a._header[i];
  return *this;
}
    
void RouteChannelArray::setSize(int cols)
{
  if(cols == _cols)
    return;
  _cols = cols;
  init();
}

bool RouteChannelArray::headerInvalidIndex(int col) const 
{ 
  return col == -1 || col >= _cols;
}

void RouteChannelArray::setValues(int col, bool value, bool exclusive_cols, bool exclusive_toggle)
{ 
  // REMOVE Tim. Persistent routes. Added.
  //fprintf(stderr, "RouteChannelArray::setValues col:%d val:%d exc_cols:%d exc_tog:%d\n", col, value, exclusive_cols, exclusive_toggle);
  if(invalidIndex(col)) 
    return; 
  
  const bool v = (!exclusive_toggle || (exclusive_toggle && value));
  if(exclusive_cols)
  {
    for(int c = 0; c < _cols; ++c)
      _array[itemIndex(c)]._value = (c == col && v); 
    return;
  }
  
  _array[itemIndex(col)]._value = value; 
}

// void RouteChannelArray::headerSetValues(int col, bool value, bool exclusive_cols, bool exclusive_toggle)
// { 
//   if(headerInvalidIndex(col)) 
//     return; 
//   const bool v = (!exclusive_toggle || (exclusive_toggle && value));
//   if(col != -1 && exclusive_cols)
//   {
//     for(int c = 0; c < _cols; ++c)
//       _header[headerItemIndex(c)]._value = (c == col && v); 
//     return;
//   }
//   _header[headerItemIndex(col)]._value = value; 
// }

// void RouteChannelArray::setValue(int row, int col, bool value)
// { 
//   setValues(row, col, value, _rowsExclusive, _colsExclusive, _exclusiveToggle);
// }  
//   if(invalidIndex(row, col)) 
//     return; 
  //_array[itemIndex(row, col)]._value = value; 


//   if(_rowsExclusive && _colsExclusive)
//   {
//     for(int r = 0; r < _rows; ++r)
//     {
//       for(int c = 0; c < _cols; ++c)
//       {
//         _array[itemIndex(r, col)]._value = (r == row); 
//       }
//     }
//    
//     return;
//   }
//   
//   if(_rowsExclusive)
//   {
//     for(int r = 0; r < _rows; ++r)
//       _array[itemIndex(r, col)]._value = (r == row); 
//   }
//   if(_colsExclusive)
//   {
//     for(int c = 0; c < _cols; ++c)
//       _array[itemIndex(row, c)]._value = (c == col); 
//   }
//   if(!_rowsExclusive && !_colsExclusive)
//     _array[itemIndex(row, col)]._value = value; 
// }

// void RouteChannelArray::headerSetValue(int row, int col, bool value)
// { 
//   headerSetValues(row, col, value, _rowsExclusive, _colsExclusive, _exclusiveToggle);
// }  


// //---------------------------------------------------------
// //   RouteChannelArrayHeader
// //---------------------------------------------------------
// 
// void RouteChannelArrayHeader::init()
// {
//   if(_array)
//   {
//     delete[] _array;
//     _array = 0;
//   }
//   const int sz = itemCount();
//   if(sz == 0)
//     return;
//   _array = new RouteChannelArrayItem[sz];
// }
// 
// bool RouteChannelArrayHeader::invalidIndex(int row, int col) const 
// { 
//   if(row == -1)
//     return col == -1 || col >= _cols;
//   if(col == -1)
//     return row >= _rows;
//   return true;
// }
// 
// void RouteChannelArrayHeader::setValues(int row, int col, bool value, bool exclusive_rows, bool exclusive_cols, bool exclusive_toggle)
// { 
//   if(invalidIndex(row, col)) 
//     return; 
//   const bool v = (!exclusive_toggle || (exclusive_toggle && value));
//   if(row != -1 && exclusive_rows)
//   {
//     for(int r = 0; r < _rows; ++r)
//       _array[itemIndex(r, -1)]._value = (r == row && v);
//     return;
//   }
//   if(col != -1 && exclusive_cols)
//   {
//     for(int c = 0; c < _cols; ++c)
//       _array[itemIndex(-1, c)]._value = (c == col && v); 
//     return;
//   }
//   _array[itemIndex(row, col)]._value = value; 
// }

MenuItemControlWidget::MenuItemControlWidget(RoutingMatrixWidgetAction* action, QWidget* parent)
             : QWidget(parent)
{
  _action = action;
  //_isSelected = false;
  setMouseTracking(true);
}

void MenuItemControlWidget::elementRect(QRect* checkbox_rect, QRect* label_rect) const
{
  QSize checkbox_sz(0, 0);
  //QSize txt_sz(0, 0);
//   QRect cb_rect;
  //QRect txt_rect();
  
  if(_action->hasCheckBox())
  {
    QStyle* st = style() ? style() : QApplication::style();
    if(st)
    {
//       QStyleOptionMenuItem option;
//       option.checkType = QStyleOptionMenuItem::NonExclusive;
//       option.menuHasCheckableItems = _action->hasCheckBox();
//       option.checked = _action->checkBoxChecked();
//       option.menuItemType = QStyleOptionMenuItem::Normal;
//       //option.menuRect = menuItemControlRect();
//       option.text = _action->actionText();

      QStyleOptionButton option;
      option.state = QStyle::State_Active | QStyle::State_Enabled | QStyle::State_HasFocus |
                      (_action->checkBoxChecked() ? QStyle::State_On : QStyle::State_Off);
      //if(_isSelected)
      //if(_action->isSelected())
      //  option.state |= QStyle::State_Selected;
      //option.rect = rect();
      
      //option.text = _action->actionText();
      //option.fontMetrics = ;
      
//       if(_action->hasCheckBox())
        checkbox_sz = st->sizeFromContents(QStyle::CT_CheckBox, &option, QSize(0, 0)); //, q);

      //if(_action->hasCheckBox())
//         cb_rect = st->subElementRect(QStyle::SE_CheckBoxIndicator, &option); //, q);
      
//       if(_action->hasCheckBox())
//         cb_rect = st->subElementRect(QStyle::SE_CheckBoxIndicator, &option);
//       
//       txt_rect = st->subElementRect(QStyle::SE_CheckBoxContents, &option);
      
      //QRect QStyle::itemTextRect(const QFontMetrics & metrics, const QRect & rectangle, int alignment, bool enabled, const QString & text) const      
//       const QFontMetrics txt_fm(_action->font());
//       const QRect txt_geo = geometry();
      //txt_rect = st->itemTextRect(txt_fm, txt_geo, Qt::AlignLeft | Qt::AlignVCenter, true, _action->actionText());
//       txt_rect = txt_fm.boundingRect(_action->actionText());
      
      //txt_sz = st->sizeFromContents(QStyle::CT_CheckBoxLabel, &option, QSize(txt_rect.width(), txt_rect.height())); //, q);
      
    }
  }

  const QFontMetrics txt_fm(_action->font());
  const QSize txt_sz = txt_fm.size(Qt::TextSingleLine, _action->actionText().isEmpty() ? "8" : _action->actionText());
//   const QRect txt_rect = txt_fm.boundingRect(geometry(), 
//                                              Qt::TextSingleLine | Qt::AlignLeft | Qt::AlignVCenter, 
//                                              _action->actionText().isEmpty() ? "8" : _action->actionText());

  const int menu_item_h = txt_sz.height() > checkbox_sz.height() ? txt_sz.height() : checkbox_sz.height();
//   const int menu_item_h = txt_rect.height() > checkbox_sz.height() ? txt_rect.height() : checkbox_sz.height();
//   const int menu_item_h = txt_sz.height() > cb_rect.height() ? txt_sz.height() : cb_rect.height();
  if(checkbox_rect)
//     *checkbox_rect = QRect(_action->actionHMargin, 0, checkbox_sz.width() + _action->actionHMargin, menu_item_h);
//     *checkbox_rect = QRect(_action->actionHMargin, 0, cb_rect.width() + _action->actionHMargin, menu_item_h);
//     *checkbox_rect = QRect(_action->actionHMargin, 0, cb_rect.width(), cb_rect.height());
//     *checkbox_rect = cb_rect;
//     *checkbox_rect = QRect(0, 0, checkbox_sz.width(), checkbox_sz.height());
    *checkbox_rect = QRect(0, 0, checkbox_sz.width(), menu_item_h);
  if(label_rect)
//     *label_rect = QRect(_action->actionHMargin + checkbox_sz.width() + _action->actionHMargin, 0, txt_sz.width(), menu_item_h);
//     *label_rect = QRect(txt_rect.x(), txt_rect.y(), txt_rect.width(), menu_item_h);
//     *label_rect = QRect(_action->actionHMargin + cb_rect.width() + _action->actionHMargin, 0, txt_sz.width(), menu_item_h);
//     *label_rect = QRect(_action->actionHMargin + cb_rect.width() + _action->actionHMargin, 0, txt_sz.width(), txt_sz.height());
//     *label_rect = txt_rect;
//     *label_rect = QRect(0, 0, txt_sz.width(), txt_sz.height());
    *label_rect = QRect(0, 0, txt_sz.width(), menu_item_h);
  
//   const int menu_item_h = txt_rect.height() > cb_rect.height() ? txt_rect.height() : cb_rect.height();
//   if(checkbox_rect)
//     *checkbox_rect = QRect(cb_rect.x(), cb_rect.y(), cb_rect.width(), menu_item_h);
//   if(label_rect)
//     *label_rect = QRect(txt_rect.x(), txt_rect.y(), txt_rect.width(), menu_item_h);
  
}

QSize MenuItemControlWidget::sizeHint() const
{
  QRect cb_ctrl_rect;
  QRect lbl_ctrl_rect;
  
  elementRect(&cb_ctrl_rect, &lbl_ctrl_rect);
  
  // REMOVE Tim. Persistent routes. Added.
//   fprintf(stderr, "MenuItemControlWidget::sizeHint menu item checkbox w:%d h:%d label w:%d h:%d\n", 
//           cb_ctrl_rect.width(), cb_ctrl_rect.height(), lbl_ctrl_rect.width(), lbl_ctrl_rect.height());

  const int cb_w = _action->hasCheckBox() ? (_action->actionHMargin + cb_ctrl_rect.x() + cb_ctrl_rect.width()) : 0;
  const int l_w = cb_w + _action->actionHMargin + lbl_ctrl_rect.x() + lbl_ctrl_rect.width();
  
  const int cb_h = cb_ctrl_rect.y() + cb_ctrl_rect.height();
  const int l_h = lbl_ctrl_rect.y() + lbl_ctrl_rect.height();
  const int h = l_h > cb_h ? l_h : cb_h;
//   return QSize(lbl_ctrl_rect.x() + lbl_ctrl_rect.width(), h);
  return QSize(cb_w + l_w, h);


/*  
    QSize item_sz(0, 0);
    QStyle* st = style() ? style() : QApplication::style();
    if(st)
    {
//       QStyleOptionMenuItem option;
//       option.checkType = QStyleOptionMenuItem::NonExclusive;
//       option.menuHasCheckableItems = _action->hasCheckBox();
//       option.checked = _action->checkBoxChecked();
//       option.menuItemType = QStyleOptionMenuItem::Normal;
//       //option.menuRect = menuItemControlRect();
//       option.text = _action->actionText();

      QStyleOptionViewItem option;
      //option.state = QStyle::State_Active | QStyle::State_Enabled | QStyle::State_HasFocus | QStyle::State_On;
      option.state = QStyle::State_Active | QStyle::State_Enabled; // | QStyle::State_HasFocus;
      if(_isSelected)
        option.state |= QStyle::State_Selected;
      
      //option.features = QStyleOptionViewItem::None;
      option.features = QStyleOptionViewItem::HasDisplay;
      if(_action->hasCheckBox())
      {
        option.features |= QStyleOptionViewItem::HasCheckIndicator;
        option.checkState = _action->checkBoxChecked() ? Qt::Checked : Qt::Unchecked;
      }
      option.text = _action->actionText();
      option.font = _action->font();
      
      QFontMetrics fm(_action->font());
      const QRect b_rect = fm.tightBoundingRect(_action->actionText());
      
      item_sz = st->sizeFromContents(QStyle::CT_ItemViewItem, &option, QSize(b_rect.width(), b_rect.height())); //, q);
    }
  return item_sz;  */
}

void MenuItemControlWidget::paintEvent(QPaintEvent*)
{
  QPainter p(this);

  QRect cb_ctrl_rect;
  QRect lbl_ctrl_rect;
  
  elementRect(&cb_ctrl_rect, &lbl_ctrl_rect);
   
  //if(_isSelected)
  if(_action->isSelected())
    p.fillRect(rect(), palette().highlight());
  
//   p.fillRect(0, 0, 500, 500, Qt::lightGray);
  
//   if(_action->text().isEmpty())
//   {
//     p.setFont(_action->smallFont());
//   }
//   else
//   {
//   p.setFont(_action->font());
//   }
  
  if(_action->hasCheckBox())
  {
    QStyle* st = style() ? style() : QApplication::style();
    if(st)
    {
//       QStyleOptionMenuItem option;
//       option.checkType = QStyleOptionMenuItem::NonExclusive;
//       option.checked = _action->isChecked();
//       option.menuHasCheckableItems = true;
//       option.menuItemType = QStyleOptionMenuItem::Normal;
//       //option.menuRect = _action->menuItemControlRect();
//       option.rect = _action->menuItemControlRect();
//       option.text = _action->text();
//       option.font = _action->font();

      
      //if(_action->hasCheckBox())
      //{
        QStyleOptionButton option;
        option.state = QStyle::State_Active | QStyle::State_Enabled | QStyle::State_HasFocus | // QStyle::State_Selected |
                      (_action->checkBoxChecked() ? QStyle::State_On : QStyle::State_Off);
//         if(_isSelected)
//           option.state |= QStyle::State_Selected;
//         option.rect = _action->checkBoxControlRect();
        //option.rect = cb_ctrl_rect;
        option.rect = QRect(_action->actionHMargin + cb_ctrl_rect.x(), 
                            cb_ctrl_rect.y(), 
                            cb_ctrl_rect.width(), 
                            cb_ctrl_rect.height());
        option.palette = palette();
        //option.text = _action->text();
        //option.fontMetrics = QFontMetrics(_action->font());
          
        //st->drawControl(QStyle::CE_MenuItem, &option, &p);
        st->drawControl(QStyle::CE_CheckBox, &option, &p);
//         st->drawPrimitive(QStyle::PE_IndicatorCheckBox, &option, &p);
      //}
      
//       if(!_action->actionText().isEmpty())
//       {
//         //p.setPen(_isSelected ? palette().highlightedText().color() : palette().text().color());
//         //st->drawItemText(&p, lbl_ctrl_rect, Qt::AlignLeft | Qt::AlignVCenter, palette(), true, _action->actionText());
//         st->drawControl(QStyle::CE_CheckBoxLabel, &option, &p);
//       }
    }
  }  
//   else
//   {
//     //if(_action->text().isEmpty())
//     //  p.drawText(_action->menuItemControlRect(), Qt::AlignRight | Qt::AlignVCenter, QString::number(col + 1));
//     //else
//       p.drawText(_action->menuItemControlRect(), Qt::AlignVCenter, _action->text());
//   }

//   if(!_action->itemLabelText().isEmpty())
//     p.drawText(_action->labelControlRect(), Qt::AlignLeft | Qt::AlignVCenter, _action->itemLabelText());
  if(!_action->actionText().isEmpty())
  {
//     p.drawText(_action->labelControlRect(), Qt::AlignLeft | Qt::AlignVCenter, _action->text());
    //p.setBrush(_isSelected ? palette().highlightedText() : palette().text());
    //p.setPen(_isSelected ? palette().highlightedText().color() : palette().text().color());
    p.setPen(_action->isSelected() ? palette().highlightedText().color() : palette().text().color());
    p.setFont(_action->font());
    const int l_x = _action->actionHMargin + (_action->hasCheckBox() ? (_action->actionHMargin + cb_ctrl_rect.x() + cb_ctrl_rect.width()) : 0);
    const QRect l_r(l_x, lbl_ctrl_rect.y(), 
                    lbl_ctrl_rect.width(), lbl_ctrl_rect.height());
    p.drawText(l_r, Qt::AlignLeft | Qt::AlignVCenter, _action->actionText());
  }
  
  
  
//     QStyle* st = style() ? style() : QApplication::style();
//     if(st)
//     {
//       QStyleOptionViewItem option;
//       //option.state = QStyle::State_Active | QStyle::State_Enabled | QStyle::State_HasFocus | QStyle::State_On;
//       option.state = QStyle::State_Active | QStyle::State_Enabled; // | QStyle::State_HasFocus;
//       if(_isSelected)
//         option.state |= QStyle::State_Selected;
//       //option.features = QStyleOptionViewItem::None;
//       option.features = QStyleOptionViewItem::HasDisplay;
//       if(_action->hasCheckBox())
//       {
//         option.features |= QStyleOptionViewItem::HasCheckIndicator;
//         option.checkState = _action->checkBoxChecked() ? Qt::Checked : Qt::Unchecked;
//       }
//       option.text = _action->actionText();
//       option.font = _action->font();
//       option.rect = geometry();
//       
//       //item_sz = st->sizeFromContents(QStyle::CT_ItemViewItem, &option, QSize(b_rect.width(), b_rect.height())); //, q);
//       st->drawControl(QStyle::CE_ItemViewItem, &option, &p);
//     }  
}

void MenuItemControlWidget::mousePressEvent(QMouseEvent* e)
{
  // REMOVE Tim. Persistent routes. Added.
  fprintf(stderr, "MenuItemControlWidget::mousePressEvent\n");
  
  if(_action->hasCheckBox())
  {
    _action->setCheckBoxChecked(!_action->checkBoxChecked());
    update();  // Redraw the indicators.
    e->ignore();  // Don't accept. Let the menu close if neccessary.
    _action->setIsChanged(true); // For when the container widget gets the event.
  }
  else
  {
    e->accept();  // Not checkable? Eat it up - the text was clicked on.
    _action->setIsChanged(false);
  }
}

void MenuItemControlWidget::mouseReleaseEvent(QMouseEvent* e)
{
  // REMOVE Tim. Persistent routes. Added.
  fprintf(stderr, "MenuItemControlWidget::mouseReleaseEvent\n");
  if(_action->hasCheckBox())
  {
    e->ignore();  // Don't accept. Let the menu close if neccessary.
    //_action->setChanged(true); // For when the container widget gets the event.
  }
  else
  {
    e->accept();  // Not checkable? Eat it up - the text was clicked on.
    _action->setIsChanged(false);
  }
}

void MenuItemControlWidget::mouseDoubleClickEvent(QMouseEvent* e)
{
  // REMOVE Tim. Persistent routes. Added.
  fprintf(stderr, "MenuItemControlWidget::mouseDoubleClickEvent\n");
  e->accept(); 
  _action->setIsChanged(false);
}

void MenuItemControlWidget::contextMenuEvent(QContextMenuEvent* e)
{
  e->accept();
  _action->setIsChanged(false);
}

// void MenuItemControlWidget::actionEvent(QActionEvent* e)
// {
//   // REMOVE Tim. Persistent routes. Added.
//   fprintf(stderr, "MenuItemControlWidget::actionEvent\n");
//   QWidget::actionEvent(e);
// }

// bool MenuItemControlWidget::event(QEvent* e)
// {
//   fprintf(stderr, "MenuItemControlWidget::event type:%d\n", e->type()); // REMOVE Tim. Persistent routes. Added.
//   return QWidget::event(e);
// }


//---------------------------------------------------------
//   SwitchBarActionWidget
//---------------------------------------------------------

SwitchBarActionWidget::SwitchBarActionWidget(RoutingMatrixWidgetAction* action, QWidget* parent)
  : QWidget(parent)
{
  _action = action;
  //_isSelected = false;
  setMouseTracking(true);
}

QSize SwitchBarActionWidget::sizeHint() const
{
//   const int cellW = _action->cellGeometry().width();
//   const int cellH = _action->cellGeometry().height();
//   const int c_groups = _action->array()->columns() / _action->itemsPerGroup;
//   const int r_groups = _action->array()->rows() / _action->itemsPerGroup;
//   const int w = (_action->array()->columns() + 1) * (cellW + _action->itemHSpacing) +  // 1 extra for the vertical header column.
//                 c_groups * _action->groupSpacing + 
//                 2 * _action->margin;
//   const int h = (_action->array()->rows() + 1) * (cellH + _action->itemVSpacing) +    // 1 extra for the horizontal header row.
//                 r_groups * _action->groupSpacing + 
//                 2 * _action->margin;
  
/*                
  int w = 0;
  int h = 0;
  const int rows = _action->array()->rows();
  const int cols = _action->array()->columns();
  for(int row = 0; row < rows; ++row)
  {
    for(int col = 0; col < cols; ++col)
    {
      const QString str = _action->array()->text(row, col);
      if(str.isEmpty())
      {
        w += cellW + _action->itemHSpacing;
        h += cellH + _action->itemVSpacing;
      }
      else
      {
        QFont fnt = font();
        //fnt.setPointSize(6);
        const QFontMetrics fm(fnt);
        const QRect r = fm.boundingRect(str);
        int r_w = r.width();
        int r_h = r.height();
        if(_action->onPixmap() && r_w < _action->onPixmap()->width())
          r_w = _action->onPixmap()->width();
        if(_action->onPixmap() && r_h < _action->onPixmap()->height())
          r_h = _action->onPixmap()->height();
        w += r_w + _action->itemHSpacing;
        h += r_h + _action->itemVSpacing;
      }
    }
  }
  
  const int h_rows = _action->header()->rows();
  const int h_cols = _action->header()->columns();
  int max_hdr_w = 0;
  for(int row = 0; row < h_rows; ++row)
  {
    const QString str = _action->header()->text(row, -1);
    if(str.isEmpty())
      h += cellH + _action->itemVSpacing;
    else
    {
      QFont fnt = font();
      //fnt.setPointSize(6);
      const QFontMetrics fm(fnt);
      const QRect r = fm.boundingRect(str);
      int r_w = r.width();
      if(_action->onPixmap() && r_w < _action->onPixmap()->width())
        r_w = _action->onPixmap()->width();
      if(r_w > max_hdr_w)
        max_hdr_w = r_w;
      int r_h = r.height();
      if(_action->onPixmap() && r_h < _action->onPixmap()->height())
        r_h = _action->onPixmap()->height();
      h += r_h + _action->itemVSpacing;
    }
  }
  int max_hdr_h = 0;
  for(int col = 0; col < h_cols; ++col)
  {
    const QString str = _action->header()->text(-1, col);
    if(str.isEmpty())
      w += cellW + _action->itemHSpacing;
    else
    {
      QFont fnt = font();
      //fnt.setPointSize(6);
      const QFontMetrics fm(fnt);
      const QRect r = fm.boundingRect(str);
      int r_w = r.width();
      if(_action->onPixmap() && r_w < _action->onPixmap()->width())
        r_w = _action->onPixmap()->width();
      int r_h = r.height();
      if(_action->onPixmap() && r_h < _action->onPixmap()->height())
        r_h = _action->onPixmap()->height();
      if(r_h > max_hdr_h)
        max_hdr_h = r_h;
      w += r_w + _action->itemHSpacing;
    }
  }
  w += max_hdr_w + c_groups * _action->groupSpacing + 2 * _action->margin;
  h += max_hdr_h + r_groups * _action->groupSpacing + 2 * _action->margin;
  */
     
//   const int rows = _action->array()->rows();
  const int cols = _action->array()->columns();
  //const int h_rows = _action->header()->rows();
  //const int h_cols = _action->header()->columns();
//   const QRect last_array_v_cell = _action->array()->rect(rows - 1, 0);
//   const QRect last_array_h_cell = _action->array()->rect(0, cols - 1);
  const QRect last_array_h_cell = _action->array()->rect(cols - 1);
  // Take the height of any horizontal header column - they're all the same.
//   const int h = //_action->header()->rect(-1, 0).height() + 
//                 last_array_v_cell.y() + last_array_v_cell.height() +
//                 2 * _action->margin;
  const int hdr_h = _action->array()->headerVisible() ? _action->array()->headerRect(0).height() : 0;
  const int h = //_action->menuItemControlRect().y() +
                //_action->menuItemControlRect().height() + 
                hdr_h + 
                last_array_h_cell.height() +
                _action->itemVSpacing + 
                2 * _action->margin;
  //const QString array_title_str = _action->array()->arrayTitle();
  //const QRect array_title_r = _action->array()->arrayTitleRect();
  // Take the width of any vertical header row - they're all the same.
  //const int w = (array_title_str.isEmpty() ? (last_array_h_cell.x() + last_array_h_cell.width()) : (array_title_r.x() + array_title_r.width())) +
  const int w = last_array_h_cell.x() + last_array_h_cell.width() +
                2 * _action->margin; // +
                // Take the width of any vertical header row checkbox - they're all the same. Not necessary - already taken into account.
                //_action->array()->headerCheckBoxRect(0).width();
                //_action->isCheckable() ? cb_rect.width() : 0;
  //const int w = //_action->header()->rect(0, -1).width() +
  //              last_array_h_cell.x() + last_array_h_cell.width() +
  //              2 * _action->margin;
//   for(int row = 0; row < h_rows; ++row)
//     h += _action->header()->rect(row, -1).height();
//   for(int col = 0; col < h_cols; ++col)
//     w += _action->header()->rect(-1, col).width();
  
//   w += 2 * _action->margin;
//   h += 2 * _action->margin;
  
  return QSize(w, h);
}

void SwitchBarActionWidget::resizeEvent(QResizeEvent* e)
{
  fprintf(stderr, "SwitchBarActionWidget::resizeEvent\n");
  QWidget::resizeEvent(e);
}


// void RoutingMatrixWidget::drawGrid(QPainter& p)
// {
//   const int rows = _action->array()->rows();
//   const int cols = _action->array()->columns();
//   const int cellW = _action->maxPixmapGeometry().width();
//   const int cellH = _action->maxPixmapGeometry().height();
//   const int c_groups = _action->array()->columns() / _action->itemsPerGroup;
//   const int r_groups = _action->array()->rows() / _action->itemsPerGroup;
//   const int w = _action->margin + _action->array()->columns() * (cellW + _action->itemHSpacing) + 
//                 c_groups * _action->groupSpacing +
//                 cellW; // One more for the last line, minus the itemSpacing.
//   const int h = _action->margin + _action->array()->rows() * (cellH + _action->itemVSpacing) + 
//                 r_groups * _action->groupSpacing +
//                 cellH; // One more for the last line, minus the itemSpacing.
//   const int x0 = _action->margin + cellW;
//   const int x1 = w;
//   const int y0 = _action->margin + cellH;
//   const int y1 = h;
//   
//   int y = _action->margin + cellH;
//   for(int row = 0; row <= rows; ++row) // Using <= to get the last line.
//   {
//     int line_y;
//     if(row != 0 && ((row % _action->itemsPerGroup) == 0))
//     {
//       line_y = y + (_action->itemVSpacing + _action->groupSpacing) / 2;
//       y += _action->groupSpacing;
//     }
//     else
//       line_y = y + _action->itemVSpacing / 2;
//     p.drawLine(x0, line_y, x1, line_y);
//     y += cellH + _action->itemVSpacing;
//   }
//   
//   int x = _action->margin + cellW;
//   for(int col = 0; col <= cols; ++col) // Using <= to get the last line.
//   {
//     int line_x;
//     if(col != 0 && ((col % _action->itemsPerGroup) == 0))
//     {
//       line_x = x + (_action->itemHSpacing + _action->groupSpacing) / 2;
//       x += _action->groupSpacing;
//     }
//     else
//       line_x = x + _action->itemHSpacing / 2;
//     p.drawLine(line_x, y0, line_x, y1);
//     x += cellW + _action->itemHSpacing;
//   }
// }

// void RoutingMatrixWidget::paintEvent(QPaintEvent* /*event*/)
// {
//   QPainter p(this);
// 
//   // Not used - too cluttered, like looking through a screen, too hard to distinguish the squares and the
//   //  fact that with a grid, 'off' means no pixmap or drawing at all, so only the grid shows so it's hard
//   //  to pick the right box to click on. And the added group spacing makes it look distorted and confusing.
//   //drawGrid(p); 
// 
//   const int cols = _action->array()->columns();
// 
//   const QRect last_array_h_cell = _action->array()->rect(cols - 1);
//   const QString array_title_str = _action->array()->arrayTitle();
//   const QRect array_title_r = _action->array()->arrayTitleRect();
//   // Take the width of any vertical header row - they're all the same.
//   const int w = (array_title_str.isEmpty() ? (last_array_h_cell.x() + last_array_h_cell.width()) : (array_title_r.x() + array_title_r.width())) +
//                 2 * _action->margin; // +
//   const int array_offset = (width() > w) ? (width() - w) : 0;
//                 
//   p.setFont(_action->smallFont());
//   for(int col = 0; col < cols; ++col)
//   {
//     const QPixmap& pm = _action->array()->value(col) ? *_action->onPixmap() : *_action->offPixmap();
//     const int pm_w = pm.width();
//     const int pm_h = pm.height();
//     const QRect r = _action->array()->rect(col);
//     int x = r.x();
//     if(r.width() > pm_w)
//       x += (r.width() - pm_w) / 2;
//     int y = r.y();
//     if(r.height() > pm_h)
//       y += (r.height() - pm_h) / 2;
//     x += array_offset;
//     p.drawPixmap(x, y, pm);
//   }
// 
// //   if(_action->text().isEmpty())
// //   {
// //     p.setFont(_action->smallFont());
// //   }
// //   else
// //   {
//   p.setFont(_action->font());
// //   }
//   if(_action->isCheckable())
//   {
//     QStyle* st = style() ? style() : QApplication::style();
//     if(st)
//     {
//       QStyleOptionMenuItem option;
//       option.checkType = QStyleOptionMenuItem::NonExclusive;
//       option.checked = _action->isChecked();
//       option.menuHasCheckableItems = true;
//       option.menuItemType = QStyleOptionMenuItem::Normal;
//       //option.menuRect = _action->menuItemControlRect();
//       option.rect = _action->menuItemControlRect();
//       option.text = _action->text();
//       st->drawControl(QStyle::CE_MenuItem, &option, &p);
//     }
//   }  
//   else
//   {
//     //if(_action->text().isEmpty())
//     //  p.drawText(_action->menuItemControlRect(), Qt::AlignRight | Qt::AlignVCenter, QString::number(col + 1));
//     //else
//       p.drawText(_action->menuItemControlRect(), Qt::AlignVCenter, _action->text());
//   }
//   
//   //p.setFont(_action->font());
//   //const int h_rows = _action->header()->rows();
//   //const int h_cols = _action->header()->columns();
//   //const int vis_h_rows = _action->header()->visibleRows();
// //   const int vis_h_cols = _action->header()->visibleColumns();
//   const int vis_h_cols = cols;
// //   for(int row = 0; row < vis_h_rows; ++row)
// //   {
// //     if(_action->isCheckable())
// //     {
// // //       QStyle* st = style() ? style() : QApplication::style();
// //       if(st)
// //       {
// //         QStyleOptionButton option;
// //         option.state = QStyle::State_Active | QStyle::State_Enabled | //QStyle::State_HasFocus | // TODO
// //                       (_action->array()->headerValue(row, -1) ? QStyle::State_On : QStyle::State_Off);
// //         option.rect = _action->array()->headerCheckBoxRect(row);
// // 
// //          // REMOVE Tim. Persistent routes. Added.
// //         fprintf(stderr, "RoutingMatrixWidgetAction::paintEvent rect x:%d y:%d w:%d h:%d\n", 
// //                 option.rect.x(), option.rect.y(), 
// //                 option.rect.width(), option.rect.height());
// // 
// //         st->drawPrimitive(QStyle::PE_IndicatorItemViewItemCheck, &option, &p);
// //         
// //         //if(cb_rect.width() > max_v_hdr_w)
// //         //  max_v_hdr_w = cb_rect.width();
// //       }
// //     }  
// //     
// /*    
//     
//     //const QRect r = _action->header()->rect(row, -1);
//     //const QString str = _action->header()->text(row, -1);
//     const QRect r = _action->array()->headerRect(row, -1);
//     const QString str = _action->array()->headerText(row, -1);
//     if(str.isEmpty())
//     {
//       p.setFont(_action->smallFont());
//       p.drawText(r, Qt::AlignRight | Qt::AlignVCenter, QString::number(row + 1));
//     }
//     else
//     {
//       p.setFont(_action->font());
//       p.drawText(r, Qt::AlignLeft | Qt::AlignVCenter, str);
//     }
//   }*/
//   
//   if(_action->array()->headerVisible())
//   {
//     for(int col = 0; col < vis_h_cols; ++col)
//     {
//       //const QRect r = _action->header()->rect(-1, col);
//       //const QString str = _action->header()->text(-1, col);
//       const QRect r = _action->array()->headerRect(col);
//       const QString str = _action->array()->headerText(col);
//       if(str.isEmpty())
//       {
//         p.setFont(_action->smallFont());
//         p.drawText(r, Qt::AlignCenter, QString::number(col + 1));
//       }
//       else
//       {
//         p.setFont(_action->font());
//         p.drawText(r, Qt::AlignCenter, str);
//       }
//     }
//   }
//   
//   const QString header_title_str = _action->array()->headerTitle();
//   if(!header_title_str.isEmpty())
//   {
//     p.setFont(_action->font());
//     p.drawText(_action->array()->headerTitleRect(), Qt::AlignHCenter | Qt::AlignVCenter, header_title_str);
//   }
//   
//   //const QString array_title_str = _action->array()->arrayTitle();
//   if(!array_title_str.isEmpty())
//   {
//     p.setFont(_action->font());
//     p.drawText(_action->array()->arrayTitleRect(), Qt::AlignHCenter | Qt::AlignVCenter, array_title_str);
//   }
// }

void SwitchBarActionWidget::paintEvent(QPaintEvent* /*event*/)
{
  QPainter p(this);

  // Not used - too cluttered, like looking through a screen, too hard to distinguish the squares and the
  //  fact that with a grid, 'off' means no pixmap or drawing at all, so only the grid shows so it's hard
  //  to pick the right box to click on. And the added group spacing makes it look distorted and confusing.
  //drawGrid(p); 

  const int cols = _action->array()->columns();

  //p.fillRect(0, 0, 500, 500, Qt::lightGray);
  
  p.setFont(_action->smallFont());
  for(int col = 0; col < cols; ++col)
  {
    const QRect r = _action->array()->rect(col);
    if(_action->isSelected() && col == _action->array()->activeColumn())
      p.fillRect(r, palette().highlight());
    const QPixmap& pm = _action->array()->value(col) ? *_action->onPixmap() : *_action->offPixmap();
    const int pm_w = pm.width();
    const int pm_h = pm.height();
    int x = r.x();
    if(r.width() > pm_w)
      x += (r.width() - pm_w) / 2;
    int y = r.y();
    if(r.height() > pm_h)
      y += (r.height() - pm_h) / 2;
//     // REMOVE Tim. Persistent routes. Added.
//     fprintf(stderr, "RoutingMatrixWidget::paintEvent array rect x:%d y:%d w:%d h:%d pm x%d y:%d\n", 
//             r.x(), r.y(), r.width(), r.height(), x, y);
    p.drawPixmap(x, y, pm);
  }

  if(_action->array()->headerVisible())
  {
    for(int col = 0; col < cols; ++col)
    {
      const QRect r = _action->array()->headerRect(col);
      const QString str = _action->array()->headerText(col);
      if(str.isEmpty())
      {
        p.setFont(_action->smallFont());
        p.drawText(r, Qt::AlignCenter, QString::number(col + 1));
      }
      else
      {
        p.setFont(_action->font());
        p.drawText(r, Qt::AlignCenter, str);
      }
    }
  }
}

void SwitchBarActionWidget::mousePressEvent(QMouseEvent* ev)
{
  // REMOVE Tim. Persistent routes. Added.
  fprintf(stderr, "SwitchBarActionWidget::mousePressEvent\n");
  const int cols = _action->array()->columns();
  const QPoint pt = ev->pos();
  bool changed = false;
  //for(int row = 0; row < rows; ++row)
  {
//     if(_action->isCheckable())
//     {
//       const QRect mi_rect = _action->menuItemControlRect();
//       if(mi_rect.contains(pt))
//       {
//         _action->toggle();
//         changed = true;
// //         break;
//       }
//     }
    
    for(int col = 0; col < cols; ++col)
    {
      const QRect rect = _action->array()->rect(col);
      if(rect.contains(pt))
      {
        _action->array()->setValue(col, !_action->array()->value(col));  // TODO: Add a toggleValue.
        changed = true;
        break;
      }
    }
//     if(changed)
//       break;
  }

  // Reset any other switch bars besides this one which are part of a QActionGroup.
  // Since they are all part of an action group, force them to be exclusive regardless of their exclusivity settings.
//   if(changed)
//   {
//     QActionGroup* act_group = _action->actionGroup();
//     if(act_group && act_group->isExclusive())
//     {
//       for(int i = 0; i < act_group->actions().size(); ++i) 
//       {
//         RoutingMatrixWidgetAction* act = dynamic_cast<RoutingMatrixWidgetAction*>(act_group->actions().at(i));
//         if(act && act != _action)
//           // Set any column to false, and exclusiveColumns and exclusiveToggle to true which will reset all columns.
//           act->array()->setValues(0, false, true, true);
//       }  
//     }
//   }
//   if(changed)
//   {
//     //ev->accept();
//     update();  // Redraw the indicators.
//     //return;
//   }
//   
//   ev->ignore();  // Don't accept. Let the menu close if neccessary.
  
  
  if(changed)
  {
    // Reset any other switch bars besides this one which are part of a QActionGroup.
    // Since they are all part of an action group, force them to be exclusive regardless of their exclusivity settings.
    QActionGroup* act_group = _action->actionGroup();
    if(act_group && act_group->isExclusive())
    {
      for(int i = 0; i < act_group->actions().size(); ++i) 
      {
        RoutingMatrixWidgetAction* act = dynamic_cast<RoutingMatrixWidgetAction*>(act_group->actions().at(i));
        if(act && act != _action)
          // Set any column to false, and exclusiveColumns and exclusiveToggle to true which will reset all columns.
          act->array()->setValues(0, false, true, true);
      }  
    }
    
    update();  // Redraw the indicators.
    ev->ignore();  // Don't accept. Let the menu close if neccessary.
    _action->setIsChanged(true); // For when the container widget gets the event.
  }
  else
  {
    // Nothing clicked? Stay open.
    ev->accept();
    _action->setIsChanged(false); // For when the container widget gets the event.
  }
}

void SwitchBarActionWidget::mouseReleaseEvent(QMouseEvent* ev)
{
  // REMOVE Tim. Persistent routes. Added.
  fprintf(stderr, "SwitchBarActionWidget::mouseReleaseEvent\n");
  ev->ignore();
  //_action->setChanged(false); // For when the container widget gets the event.
}

void SwitchBarActionWidget::mouseDoubleClickEvent(QMouseEvent* ev)
{
  // REMOVE Tim. Persistent routes. Added.
  fprintf(stderr, "SwitchBarActionWidget::mouseDoubleClickEvent\n");
  ev->accept(); 
  _action->setIsChanged(false);
}

void SwitchBarActionWidget::mouseMoveEvent(QMouseEvent* ev)
{
  const int cols = _action->array()->columns();
  const QPoint pt = ev->pos();
  int a_col = -1;
  for(int col = 0; col < cols; ++col)
  {
    const QRect rect = _action->array()->rect(col);
    if(rect.contains(pt))
    {
      a_col = col;
      break;
    }
  }
  if(a_col != _action->array()->activeColumn())
  {
    _action->array()->setActiveColumn(a_col);
    update();
  }
  ev->ignore();
  //QWidget::mouseMoveEvent(ev);
}

void SwitchBarActionWidget::contextMenuEvent(QContextMenuEvent* ev)
{
  ev->accept();
  _action->setIsChanged(false);
}


//---------------------------------------------------------
//   RoutingMatrixActionWidget
//---------------------------------------------------------

RoutingMatrixActionWidget::RoutingMatrixActionWidget(RoutingMatrixWidgetAction* action, QWidget* parent)
             : QWidget(parent)
{
  _action = action;
  //_isSelected = false;

  setMouseTracking(true);
  setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
  
  //QWidget* lw = new QWidget(parent);
  
  const int layout_m_l = 0, layout_m_r = 0, layout_m_t = 1, layout_m_b = 1;
  
  QHBoxLayout* h_layout = new QHBoxLayout(this);
  h_layout->setSpacing(0);
  h_layout->setContentsMargins(layout_m_l, layout_m_t, layout_m_r, layout_m_b);

  // Remove Tim. Just a test.
//   h_layout->addWidget(_itemLabel);
//   h_layout->addStretch();
//   h_layout->addWidget(new QLabel("TestLabel", parent));
  
  QVBoxLayout* left_v_layout = new QVBoxLayout();
  QVBoxLayout* right_v_layout = new QVBoxLayout();
  left_v_layout->setSpacing(0);
  right_v_layout->setSpacing(0);
//   left_v_layout->setContentsMargins(layout_m_l, layout_m_t, layout_m_r, layout_m_b);
//   right_v_layout->setContentsMargins(layout_m_l, layout_m_t, layout_m_r, layout_m_b);
  left_v_layout->setContentsMargins(0, 0, 0, 0);
  right_v_layout->setContentsMargins(0, 0, 0, 0);
  
//   if(!_array.headerTitle().isEmpty())
//   {
//     QLabel* lbl = new QLabel(_array.headerTitle(), parent);
//     lbl->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
//     lbl->setAlignment(Qt::AlignCenter);
//     lbl->setAutoFillBackground(true);
//     //QPalette palette;
//     //palette.setColor(label->backgroundRole(), c);
//     //lbl->setPalette(palette);
//     lbl->setBackgroundRole(QPalette::Dark);
//     left_v_layout->addWidget(lbl);
//   }
  if(!_action->array()->headerTitle().isEmpty() || !_action->array()->checkBoxTitle().isEmpty())
  {
    QHBoxLayout* left_title_layout = new QHBoxLayout();
    left_title_layout->setSpacing(0);
    //left_title_layout->setContentsMargins(layout_m_l, layout_m_t, layout_m_r, layout_m_b);
    left_title_layout->setContentsMargins(0, 0, 0, 0); // Zero because we're already inside a layout.
    if(!_action->array()->checkBoxTitle().isEmpty())
    {
      QLabel* cb_lbl = new QLabel(_action->array()->checkBoxTitle(), parent);
      cb_lbl->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
      cb_lbl->setAlignment(Qt::AlignCenter);
      cb_lbl->setAutoFillBackground(true);
      cb_lbl->setBackgroundRole(QPalette::Dark);
      left_title_layout->addWidget(cb_lbl);
    }
    //left_title_layout->addSpacing(actionHMargin);
    if(!_action->array()->headerTitle().isEmpty())
    {
      QLabel* hdr_lbl = new QLabel(_action->array()->headerTitle(), parent);
      hdr_lbl->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
      hdr_lbl->setAlignment(Qt::AlignCenter);
      hdr_lbl->setAutoFillBackground(true);
      hdr_lbl->setBackgroundRole(QPalette::Dark);
      left_title_layout->addWidget(hdr_lbl);
    }
    left_v_layout->addLayout(left_title_layout);
  }
  left_v_layout->addStretch();

  //MenuItemControlWidget* micw = new MenuItemControlWidget(this, parent);
  //left_v_layout->addWidget(micw);
//   QHBoxLayout* mic_h_layout = new QHBoxLayout();
//   mic_h_layout->setSpacing(0);
//   mic_h_layout->setContentsMargins(0, 0, 0, 0);
  
  //MenuItemControlWidget* micw = new MenuItemControlWidget(this, parent);
  //micw->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);
  _menuItemControlWidget = new MenuItemControlWidget(_action, parent);
  _menuItemControlWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
  
//   micw->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
//   mic_h_layout->addWidget(micw);
//   mic_h_layout->addWidget(_itemLabel);
  //mic_h_layout->addStretch();
//   left_v_layout->addLayout(mic_h_layout);
  
  //left_v_layout->addWidget(micw);
  left_v_layout->addWidget(_menuItemControlWidget);

  // Remove Tim. Just a test.
  //left_v_layout->addWidget(_itemLabel);
  //right_v_layout->addWidget(new QLabel("TestLabel", parent));
  //left_v_layout->addWidget(new QLabel("TestLabel", parent));
  //right_v_layout->addWidget(_itemLabel);

  
  if(_action->array()->arrayTitle().isEmpty())
  {
    right_v_layout->addStretch();
//     RoutingMatrixWidget* switch_widget = new RoutingMatrixWidget(this, parent);
//     switch_widget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);
//     right_v_layout->addWidget(switch_widget);
  }
  else
//   if(!_action->array()->arrayTitle().isEmpty())
  {
    QLabel* lbl = new QLabel(_action->array()->arrayTitle(), parent);
    //lbl->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
    lbl->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
    lbl->setAlignment(Qt::AlignCenter);
    lbl->setAutoFillBackground(true);
    //QPalette palette;
    //palette.setColor(label->backgroundRole(), c);
    //lbl->setPalette(palette);
    lbl->setBackgroundRole(QPalette::Dark);
    right_v_layout->addWidget(lbl);
//     QHBoxLayout* sw_h_layout = new QHBoxLayout();
//     sw_h_layout->setSpacing(0);
//     sw_h_layout->setContentsMargins(0, 0, 0, 0);
//     RoutingMatrixWidget* switch_widget = new RoutingMatrixWidget(this, parent);
//     switch_widget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);
//     sw_h_layout->addStretch();
//     sw_h_layout->addWidget(switch_widget);
//     right_v_layout->addLayout(sw_h_layout);
  }
//   right_v_layout->addStretch();
  
  QHBoxLayout* sw_h_layout = new QHBoxLayout();
  sw_h_layout->setSpacing(0);
//   sw_h_layout->setContentsMargins(layout_m_l, layout_m_t, layout_m_r, layout_m_b);
  sw_h_layout->setContentsMargins(0, 0, 0, 0);
  sw_h_layout->addStretch();
  _switchWidget = new SwitchBarActionWidget(_action, parent);
  _switchWidget->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
  sw_h_layout->addWidget(_switchWidget);
  right_v_layout->addLayout(sw_h_layout);

  h_layout->addLayout(left_v_layout);
//   h_layout->addStretch();
  h_layout->addLayout(right_v_layout);
  //h_layout->addLayout(sw_h_layout);
  
//   QSignalMapper* mapper = new QSignalMapper(this);
// 
//   PixmapButton* pb = new PixmapButton(toggle_small_Icon, toggle_small_Icon, 2, lw, QString("T"));  // Margin  = 2
//   //mapper->setMapping(pb, _channels);  // Set to one past end.
//   h_layout->addWidget(pb); 
//   h_layout->addSpacing(6);
//   //connect(pb, SIGNAL(clicked(bool)), mapper, SLOT(map()));
//   
//   for(int i = 0; i < _channels; ++i)
//   {
//     PixmapButton* b = new PixmapButton(_refPixmap, _refPixmap, 2, lw, QString::number(i + 1));  // Margin  = 2
//     mapper->setMapping(b, i);
//     connect(b, SIGNAL(pressed()), mapper, SLOT(map()));
//     if((i != 0) && (i % 4 == 0))
//       h_layout->addSpacing(6);
//     h_layout->addWidget(b); 
//   }
// 
//   connect(mapper, SIGNAL(mapped(int)), this, SLOT(chanClickMap(int)));
  
//   return lw;
}

void RoutingMatrixActionWidget::mousePressEvent(QMouseEvent* e)
{
  // REMOVE Tim. Persistent routes. Added.
  fprintf(stderr, "RoutingMatrixActionWidget::mousePressEvent\n");
  //_action->setItemLabelText("fdshjkskfhks dauiasd sajkldjkla wqjelwqkjewl");
  if(_action->isChanged())
    e->ignore();  // Don't accept. Let the menu close if neccessary.
  else
    e->accept();  // Eat it up and don't close - it's whitespace.
  //_action->setChanged(false);
}

void RoutingMatrixActionWidget::mouseReleaseEvent(QMouseEvent* e)
{
  // REMOVE Tim. Persistent routes. Added.
  fprintf(stderr, "RoutingMatrixActionWidget::mouseReleaseEvent\n");
  if(_action->isChanged())
    e->ignore();  // Don't accept. Let the menu close if neccessary.
  else
    e->accept();  // Eat it up and don't close - it's whitespace.
  _action->setIsChanged(false);
}

void RoutingMatrixActionWidget::mouseDoubleClickEvent(QMouseEvent* e)
{
  // REMOVE Tim. Persistent routes. Added.
  fprintf(stderr, "RoutingMatrixActionWidget::mouseDoubleClickEvent\n");
  e->accept();  // Eat it up and don't close - it's whitespace.
}

void RoutingMatrixActionWidget::contextMenuEvent(QContextMenuEvent* e)
{
  e->accept();
  _action->setIsChanged(false);
}

void RoutingMatrixActionWidget::actionEvent(QActionEvent* e)
{
  // REMOVE Tim. Persistent routes. Added.
  fprintf(stderr, "RoutingMatrixActionWidget::actionEvent\n");
  if(e->type() == QEvent::ActionChanged && e->action() == _action)
  {
    _menuItemControlWidget->updateGeometry();
    if(layout())
    {
      fprintf(stderr, "    layout...\n"); // REMOVE Tim. Persistent routes. Added. 
      //layout()->invalidate();
      //layout()->update();
      layout()->activate();
    }
  }
  e->ignore();
}

// bool RoutingMatrixActionWidget::event(QEvent* e)
// {
//   fprintf(stderr, "RoutingMatrixActionWidget::event type:%d hasMouseTracking:%d\n", e->type(), hasMouseTracking()); // REMOVE Tim. Persistent routes. Added.
// //   switch(e->type())
// //   {
// //     case QEvent::MouseMove:
// //     {
// //       QMouseEvent* m_e = static_cast<QMouseEvent*>(e);
// //       //if(_isSelected != true && geometry().contains(m_e->pos()))
// //       if(_isSelected != true && rect().contains(m_e->pos()))
// //       {
// //         _isSelected = true;
// //         _menuItemControlWidget->setSelected(true);
// //         _switchWidget->setSelected(true);
// //         //e->accept();
// //         e->ignore();
// //         update();
// //         //return true;
// //         //return false;
// //       }
// //     }
// //     break;
// //     
// //     case QEvent::Enter:
// //       if(_isSelected != true)
// //       {
// //         _isSelected = true;
// //         _menuItemControlWidget->setSelected(true);
// //         _switchWidget->setSelected(true);
// //         //e->accept();
// //         e->ignore();
// //         update();
// //         //return true;
// //         //return false;
// //       }
// //     break;
// // 
// //     case QEvent::Leave:
// //       if(_isSelected != false)
// //       {
// //         _isSelected = false;
// //         _menuItemControlWidget->setSelected(false);
// //         _switchWidget->setSelected(false);
// //         //e->accept();
// //         e->ignore();
// //         update();
// //         //return true;
// //         //return false;
// //       }
// //     break;
// // 
// //     default:
// //     break;
// //   }
//   return QWidget::event(e);
// }
// 
// void RoutingMatrixActionWidget::mouseMoveEvent(QMouseEvent* e)
// {
//   //fprintf(stderr, "RoutingMatrixActionWidget::mouseMoveEvent pos x:%d y:%d rect x:%d y:%d w:%d h:%d\n", 
//   //        e->pos().x(), e->pos().y(), rect().x(), rect().y(), rect().width(), rect().height()); // REMOVE Tim. Persistent routes. Added.
//   //fprintf(stderr, "RoutingMatrixActionWidget::mouseMoveEvent\n"); 
//   //if(_isSelected != true && geometry().contains(m_e->pos()))
//   if(_isSelected != true && rect().contains(e->pos()))
//   {
//     //fprintf(stderr, "   selecting:%s\n", _action->actionText().toLatin1().constData()); 
//     _isSelected = true;
//     _menuItemControlWidget->setSelected(true);
//     _switchWidget->setSelected(true);
//     //e->accept();
//     e->ignore();
//     update();
//   }
// }

// void RoutingMatrixActionWidget::enterEvent(QEvent* e)
// {
//   fprintf(stderr, "RoutingMatrixActionWidget::enterEvent\n"); // REMOVE Tim. Persistent routes. Added.
//   if(_isSelected != true)
//   {
//     _isSelected = true;
//     _menuItemControlWidget->setSelected(true);
//     _switchWidget->setSelected(true);
//     //e->accept();
//     e->ignore();
//     update();
//   }
// }
// 
// void RoutingMatrixActionWidget::leaveEvent(QEvent* e)
// {
//   fprintf(stderr, "RoutingMatrixActionWidget::leaveEvent\n"); // REMOVE Tim. Persistent routes. Added.
//   if(_isSelected != false)
//   {
//     _isSelected = false;
//     _menuItemControlWidget->setSelected(false);
//     _switchWidget->setSelected(false);
//     //e->accept();
//     e->ignore();
//     update();
//   }
// }

//---------------------------------------------------------
//   RoutingMatrixWidgetAction
//---------------------------------------------------------

const int RoutingMatrixWidgetAction::margin = 1;
const int RoutingMatrixWidgetAction::itemHSpacing = 1;
const int RoutingMatrixWidgetAction::itemVSpacing = 3;
const int RoutingMatrixWidgetAction::groupSpacing = 4;
const int RoutingMatrixWidgetAction::itemsPerGroup = 4;
const int RoutingMatrixWidgetAction::actionHMargin = 8;

RoutingMatrixWidgetAction::RoutingMatrixWidgetAction(int cols, 
                                                     QPixmap* on_pixmap, QPixmap* off_pixmap, 
                                                     QWidget* parent, const QString& action_text)
                                                     //int visible_rows, int visible_cols)
  : QWidgetAction(parent)
{
//   _itemLabel = new QLabel(itemLabelText, parent);
//   _itemLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);
  //_itemLabel = 0;
  _labelText = action_text;
//   setText(action_text);
  // Just to be safe, set to -1 instead of default 0.
  //setData(-1);
  _isChanged = false;
  _hasCheckBox = false;
  _checkBoxChecked = false;
  _isSelected = false;
  _onPixmap = on_pixmap;
  _offPixmap = off_pixmap;
  //_header.setSize(rows, cols);
  //if(visible_rows != -1)
  //  _header.setVisibleRows(visible_rows);
  //if(visible_cols != -1)
  //  _header.setVisibleColums(visible_cols);
  _array.setSize(cols);
  _smallFont = font();
  //_smallFont.setPointSize(6);
  _smallFont.setPointSize(_smallFont.pointSize() / 2 + 1);
  
  // REMOVE Tim. Just a test.
//   setCheckable(true);
//   setChecked(true);
   
//   const QFontMetrics sfm(_smallFont);
//   //_cellGeometry = fm.boundingRect("888");
//   const QRect sbrect = sfm.boundingRect("888");
//   const QFontMetrics fm(font());
//   const QRect brect = fm.boundingRect("jT");

//   if(_cellGeometry.width() < brect->width())
//     _cellGeometry.setWidth(brect->width());
//   if(_cellGeometry.height() < _onPixmap->height())
//     _cellGeometry.setHeight(_onPixmap->height());
  
  if(_maxPixmapGeometry.width() < _onPixmap->width())
    _maxPixmapGeometry.setWidth(_onPixmap->width());
  if(_maxPixmapGeometry.height() < _onPixmap->height())
    _maxPixmapGeometry.setHeight(_onPixmap->height());
  
  if(_maxPixmapGeometry.width() < _offPixmap->width())
    _maxPixmapGeometry.setWidth(_offPixmap->width());
  if(_maxPixmapGeometry.height() < _offPixmap->height())
    _maxPixmapGeometry.setHeight(_offPixmap->height());
  updateChannelArray();

  //connect(this, SIGNAL(hovered()), this, SLOT(actionHovered()));
}

// void RoutingMatrixWidgetAction::updateChannelArray()
// {
//   const int cols = _array.columns();
//   const int cellW = _maxPixmapGeometry.width();
// //   const int cellH = _maxPixmapGeometry.height();
//   
//   int menu_h_margin = 8;
//   //int menu_v_margin = 0;
// //   QRect cb_rect; // Combo box rectangle, if enabled.
//   
// //   int y = margin + cellH + itemVSpacing;
// //   for(int row = 0; row < rows; ++row)
// //   {
// //     if(row != 0 && ((row % itemsPerGroup) == 0))
// //       y += groupSpacing;
// //     int x = margin + cellW + itemHSpacing;
// //     for(int col = 0; col < cols; ++col)
// //     {
// //       if(col != 0 && ((col % itemsPerGroup) == 0))
// //         x += groupSpacing;
// //       
// //       const QRect r(x, y, cellW, cellH);
// //       _array.setRect(row, col, r);
// //       
// //       x += cellW + itemHSpacing;
// //     }
// //     y += cellH + itemVSpacing;
// //   }
// 
//   //const int h_rows = _header.rows();
//   //const int h_cols = _header.columns();
//   const int h_cols = _array.columns();
//   
//   // Determine the maximum vertical header width.
// //   int max_v_hdr_w = 0;
// //   //if(_header.visibleRows() != 0)
// //   //{
// //     //for(int row = 0; row < h_rows; ++row)
// //     {
// //       //const QString str = _header.text(row, -1);
// //       const QString str = _array.headerText(-1);
// // //       if(str.isEmpty())
// // //       {
// // //         const int cell_w = cellW + itemHSpacing;
// // //         if(cell_w > max_v_hdr_w)
// // //           max_v_hdr_w = cell_w;
// // //       }
// // //       else
// // //       {
// // //         QFont fnt = font();
// // //         //fnt.setPointSize(6);
// // //         const QFontMetrics fm(fnt);
// // //         const QRect r = fm.boundingRect(str);
// // //         const int r_w = r.width();
// // //         if(r_w > max_v_hdr_w)
// // //           max_v_hdr_w = r_w;
// // //       }
// //       const QFontMetrics fm(str.isEmpty() ? smallFont() : font());
// //       const QRect brect = fm.boundingRect(str.isEmpty() ? "888" : str);
// //       const int r_w = brect.width();
// //       if(r_w > max_v_hdr_w)
// //         max_v_hdr_w = r_w;
// //     }
// //   //}
//   
//   QSize menu_item_sz;
//   const QFontMetrics text_fm(text().isEmpty() ? smallFont() : font());
//   //const QRect text_brect = text_fm.boundingRect(text().isEmpty() ? "8" : text());
//   const QSize text_sz = text_fm.size(Qt::TextSingleLine, text().isEmpty() ? "8" : text());
//   if(isCheckable())
//   {
//     QStyle* style = parentWidget() ? parentWidget()->style() : QApplication::style();
//     if(style)
//     {
//       QStyleOptionMenuItem option;
//       option.checkType = QStyleOptionMenuItem::NonExclusive;
//       option.checked = isChecked();
//       option.menuHasCheckableItems = true;
//       option.menuItemType = QStyleOptionMenuItem::Normal;
//       //option.menuRect = menuItemControlRect();
//       option.text = text();
// 
// //       const QFontMetrics fm(option.text.isEmpty() ? smallFont() : font());
// //       const QRect brect = fm.boundingRect(option.text.isEmpty() ? "8" : option.text);
//   //     const int r_h = brect.height();
//   //     if(r_h > max_h_hdr_h)
//   //       max_h_hdr_h = r_h;
//       
//       //menu_item_sz = style->sizeFromContents(QStyle::CT_MenuItem, &option, QSize(text_brect.width(), text_brect.height())); //, q);
//       menu_item_sz = style->sizeFromContents(QStyle::CT_MenuItem, &option, text_sz); //, q);
//       // REMOVE Tim. Persistent routes. Added.
//       fprintf(stderr, "RoutingMatrixWidgetAction::updateChannelArray text width:%d br w:%d h:%d sz w:%d h:%d\n", 
//               text_fm.width(text()), text_sz.width(), text_sz.height(), menu_item_sz.width(), menu_item_sz.height());
//     }
//   }
//   else
//     //menu_item_sz = QSize(text_brect.width() + menu_h_margin, text_brect.height());
//     menu_item_sz = QSize(text_sz.width() + menu_h_margin, text_sz.height());
//     
//   
// //   QStyle* style = parentWidget() ? parentWidget()->style() : QApplication::style();
// //   if(style)
// //   {
// //     QStyleOptionMenuItem option;
// //     option.checkType = isCheckable() ? QStyleOptionMenuItem::NonExclusive : QStyleOptionMenuItem::NotCheckable;
// //     option.checked = isChecked();
// //     option.menuHasCheckableItems = isCheckable();
// //     option.menuItemType = QStyleOptionMenuItem::Normal;
// //     //option.menuRect = menuItemControlRect();
// //     option.text = text();
// // 
// //     const QFontMetrics fm(option.text.isEmpty() ? smallFont() : font());
// //     const QRect brect = fm.boundingRect(option.text.isEmpty() ? "8" : option.text);
// // //     const int r_h = brect.height();
// // //     if(r_h > max_h_hdr_h)
// // //       max_h_hdr_h = r_h;
// //     
// //     menu_item_sz = style->sizeFromContents(QStyle::CT_MenuItem, &option, QSize(brect.width(), brect.height())); //, q);
// //     // REMOVE Tim. Persistent routes. Added.
// //     fprintf(stderr, "RoutingMatrixWidgetAction::updateChannelArray isCheckable:%d sz w:%d h:%d\n", isCheckable(), menu_item_sz.width(), menu_item_sz.height());
// // 
// //     
// //     //menu_v_margin = style->pixelMetric(QStyle::PM_MenuVMargin);
// //     
// //     // Account for the check box rectangle, if enabled.
// // //     if(isCheckable())
// // //     {
// // //       QStyleOptionButton option;
// // //       option.state = QStyle::State_Active | QStyle::State_Enabled | QStyle::State_HasFocus |
// // //                     QStyle::State_On;
// // //       cb_rect = style->subElementRect(QStyle::SE_CheckBoxClickRect, &option);
// // //       //if(cb_rect.width() > max_v_hdr_w)
// // //       //  max_v_hdr_w = cb_rect.width();
// // //     }
// //   }  
//   
// 
//   // Determine the maximum horizontal header height.
//   int max_h_hdr_h = 0;
//   if(_array.headerVisible())
//   {
//     for(int col = 0; col < h_cols; ++col)
//     {
//       //const QString str = _header.text(-1, col);
//       const QString str = _array.headerText(col);
// //       if(str.isEmpty())
// //       {
// //         const QFontMetrics fm(smallFont());
// //         const QRect brect = fm.boundingRect("8");
// //         //const int cell_h = cellH + itemVSpacing;
// //         const int cell_h = brect.height() + itemVSpacing;
// //         if(cell_h > max_h_hdr_h)
// //           max_h_hdr_h = cell_h;
// //       }
// //       else
// //       {
// //         //QFont fnt = font();
// //         //fnt.setPointSize(6);
// //         //const QFontMetrics fm(fnt);
// //         const QFontMetrics fm(font());
// //         const QRect brect = fm.boundingRect(str);
// //         const int r_h = brect.height();
// //         if(r_h > max_h_hdr_h)
// //           max_h_hdr_h = r_h;
// //       }
//       const QFontMetrics fm(str.isEmpty() ? smallFont() : font());
//       const QRect brect = fm.boundingRect(str.isEmpty() ? "8" : str);
//       const int r_h = brect.height();
//       if(r_h > max_h_hdr_h)
//         max_h_hdr_h = r_h;
//     }
//   }
//   
//   int max_title_h = 0;
//   const QString h_ttl = _array.headerTitle();
//   if(!h_ttl.isEmpty())
//   {
//     const QFontMetrics fm(font());
//     const QRect r = fm.boundingRect(h_ttl);
// //     const int r_w = r.width();
//     const int r_h = r.height();
// //     if(r_w > max_v_hdr_w)
// //       max_v_hdr_w = r_w;
//     //if(r_h > max_h_hdr_h)
//     //  max_h_hdr_h = r_h;
//     if(r_h > max_title_h)
//       max_title_h = r_h;
//     //_array.headerSetTitleRect(QRect(0, 0, max_v_hdr_w, max_h_hdr_h));
//   }
//   
//   int array_title_txt_w = 0;
//   const QString a_ttl = _array.arrayTitle();
//   if(!a_ttl.isEmpty())
//   {
//     const QFontMetrics fm(font());
//     const QRect r = fm.boundingRect(a_ttl);
//     //const int r_w = r.width();
//     array_title_txt_w = r.width();
//     const int r_h = r.height();
//     //if(r_w > max_v_hdr_w)
//     //  max_v_hdr_w = r_w;
//     //if(r_h > max_h_hdr_h)
//     //  max_h_hdr_h = r_h;
//     if(r_h > max_title_h)
//       max_title_h = r_h;
//     //_array.setArrayTitleRect(QRect(max_v_hdr_w, 0, r_w, max_h_hdr_h));
//   }
// 
//   _menuItemControlRect = QRect(margin, margin + max_h_hdr_h + max_title_h + itemVSpacing, menu_item_sz.width(), menu_item_sz.height());
//   
//   //max_h_hdr_h += max_title_h;
//   
// //   // Set the vertical header rectangles.
// //   int x = margin + menu_h_margin + cb_rect.width() + menu_h_margin;
// //   //y = margin + cellH + itemVSpacing;
// //   int y = margin + max_h_hdr_h + max_title_h + itemVSpacing;
// //   for(int row = 0; row < h_rows; ++row)
// //   {
// //     if(row != 0 && ((row % itemsPerGroup) == 0))
// //       y += groupSpacing;
// // 
// //     //int cell_h;
// //     //const QString str = _header.text(row, -1);
// //     const QString str = _array.headerText(row, -1);
// // //     if(str.isEmpty())
// // //       cell_h = cellH + itemVSpacing;
// // //     else
// // //     {
// // //       QFont fnt = font();
// // //       //fnt.setPointSize(6);
// // //       const QFontMetrics fm(fnt);
// // //       const QRect r = fm.boundingRect(str);
// // //       int r_h = r.height();
// // //       if(onPixmap() && r_h < onPixmap()->height())
// // //         r_h = onPixmap()->height();
// // //       cell_h = r_h + itemVSpacing;
// // //     }
// //     const QFontMetrics fm(str.isEmpty() ? smallFont() : font());
// //     const QRect brect = fm.boundingRect(str.isEmpty() ? "8" : str);
// //     int r_h = brect.height();
// //     if(r_h < cellH)
// //       r_h = cellH;
// //     if(cb_rect.height() > r_h)
// //       r_h = cb_rect.height();
// //     const int cell_h = r_h + itemVSpacing;
// // 
// //     //if(r_w > max_v_hdr_w)
// //     //  max_v_hdr_w = r_w;
// //     
// //     //const QRect r(x, y, cellW, cellH);
// //     const QRect r(x, y, max_v_hdr_w, cell_h);
// //     //_header.setRect(row, -1, r);
// //     _array.headerSetRect(row, -1, r);
// //     const int ofs = (r_h > cb_rect.height()) ? (r_h - cb_rect.height()) / 2 : 0;
// //     const QRect cbr(margin + menu_h_margin, y + ofs, cb_rect.width(), cb_rect.height());
// //     _array.headerSetCheckBoxRect(row, cbr);
// //     //y += cellH + itemVSpacing;
// //     y += cell_h;
// //   }
//   
//   // Set the horizontal header rectangles.
//   //x = margin + cellW + itemHSpacing;
//   int tot_array_w = 0;
//   int x = margin + menu_item_sz.width() + itemHSpacing;
//   int y = margin + max_title_h;
//   for(int col = 0; col < h_cols; ++col)
//   {
//     if(col != 0 && ((col % itemsPerGroup) == 0))
//     {
//       x += groupSpacing;
//       tot_array_w += groupSpacing;
//     }
//     
//     //int cell_w;
//     //const QString str = _header.text(-1, col);
//     const QString str = _array.headerText(col);
// //     if(str.isEmpty())
// //       cell_w = cellW + itemHSpacing;
// //     else
// //     {
// //       QFont fnt = font();
// //       //fnt.setPointSize(6);
// //       const QFontMetrics fm(fnt);
// //       const QRect r = fm.boundingRect(str);
// //       int r_w = r.width();
// //       if(onPixmap() && r_w < onPixmap()->width())
// //         r_w = onPixmap()->width();
// //       cell_w = r_w + itemHSpacing;
// //     }
//     const QFontMetrics fm(str.isEmpty() ? smallFont() : font());
//     const QRect brect = fm.boundingRect(str.isEmpty() ? "888" : str);
//     int r_w = brect.width();
//     if(r_w < cellW)
//       r_w = cellW;
//     const int cell_w = r_w + itemHSpacing;
//     //const QRect r(x, y, cellW, cellH);
//     const QRect r(x, y, cell_w, max_h_hdr_h);
//     //_header.setRect(-1, col, r);
//     _array.headerSetRect(col, r);
//     //x += cellW + itemHSpacing;
//     x += cell_w;
//     tot_array_w += cell_w;
//   }
//   
//   const int array_title_w = array_title_txt_w > tot_array_w ? array_title_txt_w : tot_array_w;
//   const QRect array_title_r(margin + menu_item_sz.width() + itemHSpacing, margin, array_title_w, max_title_h);
//   _array.setArrayTitleRect(array_title_r);
//   const QRect header_title_r(margin, margin, menu_item_sz.width(), max_title_h);
//   _array.headerSetTitleRect(header_title_r);
//   
//   // Set the array rectangles.
//   //int y = margin + cellH + itemVSpacing;
//   y = margin + max_h_hdr_h + max_title_h + itemVSpacing;
//   //for(int row = 0; row < rows; ++row)
//   {
// //     if(row != 0 && ((row % itemsPerGroup) == 0))
// //       y += groupSpacing;
// //     const int cell_h = _array.headerRect(row, -1).height();
//     const int cell_h = menu_item_sz.height();
//     //int x = margin + cellW + itemHSpacing;
//     x = margin + menu_item_sz.width() + itemHSpacing;
//     for(int col = 0; col < cols; ++col)
//     {
//       if(col != 0 && ((col % itemsPerGroup) == 0))
//         x += groupSpacing;
//       
//       const int cell_w = _array.headerRect(col).width();
//       
//       const QRect r(x, y, cell_w, cell_h);
//       _array.setRect(col, r);
//       
//       //x += cellW + itemHSpacing;
//       x += cell_w;
//     }
//     //y += cellH + itemVSpacing;
//     y += cell_h;
//   }
//   
// //   QList<QWidget*> created_widgets(createdWidgets());
// //   //Q_Q(QAction);
// //   QActionEvent e(QEvent::ActionChanged, this);
// //   //QActionEvent e(QEvent::ActionChanged, q);
// //   if(!created_widgets.isEmpty())
// //   {
// //     QWidget* w = created_widgets.front();
// //     fprintf(stderr, "RoutingMatrixWidgetAction::updateChannelArray widget:%p\n", w); // REMOVE Tim. Persistent routes. Added. 
// //     //w->adjustSize();
// //     //w->update();
// //     QApplication::sendEvent(w, &e);
// //   }
// //   QApplication::sendEvent(this, &e);
// //   //QApplication::sendEvent(q, &e);
// //   
// // //   Q_Q(QAction);
// // //   QActionEvent e(QEvent::ActionChanged, q);
// // //   for (int i = 0; i < widgets.size(); ++i) {
// // //     QWidget *w = widgets.at(i);
// // //     QApplication::sendEvent(w, &e);
// // //   }
// // //   QApplication::sendEvent(q, &e);
// // 
// //   //emit q->changed();
// //   emit changed();
// 
//   // Force it to resize. This is one of only a few methods we can call 
//   //  to trigger it which don't check to see if the value is already set.
//   setMenu(menu());
// 
//   
// //   // Determine the maximum row height and column width.
// //   //int y = margin + cellH + itemVSpacing;
// //   int y = margin + max_h_hdr_h + itemVSpacing;
// //   int max_row_h = 0;
// //   int max_col_w = 0;
// //   for(int row = 0; row < rows; ++row)
// //   {
// //     if(row != 0 && ((row % itemsPerGroup) == 0))
// //       y += groupSpacing;
// //     //int x = margin + cellW + itemHSpacing;
// //     int x = margin + max_v_hdr_w + itemHSpacing;
// //     for(int col = 0; col < cols; ++col)
// //     {
// //       if(col != 0 && ((col % itemsPerGroup) == 0))
// //         x += groupSpacing;
// // 
// // 
// //       const QString str = _array.text(row, col);
// //       if(str.isEmpty())
// //       {
// //         
// //       }
// //       else
// //       {
// //         QFont fnt = font();
// //         //fnt.setPointSize(6);
// //         const QFontMetrics fm(fnt);
// //         const QRect r = fm.boundingRect(str);
// //         const int r_w = r.width();
// //         const int r_h = r.height();
// //         if(r_h > max_h_hdr_h)
// //           max_h_hdr_h = r_h;
// //       }
// //       
// //       const QRect r(x, y, cellW, cellH);
// //       _array.setRect(row, col, r);
// //       
// //       x += cellW + itemHSpacing;
// //     }
// //     y += cellH + itemVSpacing;
// //   }
//   
// }

void RoutingMatrixWidgetAction::updateChannelArray()
{
  const int cols = _array.columns();
  const int cellW = _maxPixmapGeometry.width();
  const int cellH = _maxPixmapGeometry.height();

//   //const int menu_h_margin = 8;
//   QSize checkbox_sz(0, 0);
// //   const QFontMetrics text_fm(text().isEmpty() ? smallFont() : font());
// //   //const QRect text_brect = text_fm.boundingRect(text().isEmpty() ? "8" : text());
// //   const QSize text_sz = text_fm.size(Qt::TextSingleLine, text().isEmpty() ? "8" : text());
//   if(hasCheckBox())
//   {
//     QStyle* st = parentWidget() ? parentWidget()->style() : QApplication::style();
//     if(st)
//     {
// //       QStyleOptionMenuItem option;
// //       option.checkType = QStyleOptionMenuItem::NonExclusive;
// //       option.checked = isChecked();
// //       option.menuHasCheckableItems = true;
// //       option.menuItemType = QStyleOptionMenuItem::Normal;
// //       //option.menuRect = menuItemControlRect();
// //       option.text = text();
// 
//       QStyleOptionButton option;
//       option.state = QStyle::State_Active | QStyle::State_Enabled | QStyle::State_HasFocus | QStyle::State_On;
//       //option.text = text();
//       
//       //menu_item_sz = st->sizeFromContents(QStyle::CT_MenuItem, &option, text_sz); //, q);
//       checkbox_sz = st->sizeFromContents(QStyle::CT_CheckBox, &option, QSize(0, 0)); //, q);
//     }
//   }
// //   else
// //     //menu_item_sz = QSize(text_brect.width() + menu_h_margin, text_brect.height());
// //     menu_item_sz = QSize(text_sz.width() + menu_h_margin, text_sz.height());
// 
//   //const QFontMetrics text_fm(text().isEmpty() ? smallFont() : font());
//   const QFontMetrics text_fm(font());
//   //const QRect text_brect = text_fm.boundingRect(text().isEmpty() ? "8" : text());
//   //const QSize text_sz = text_fm.size(Qt::TextSingleLine, _labelText.isEmpty() ? "8" : _labelText);
//   const QSize text_sz = text_fm.size(Qt::TextSingleLine, text().isEmpty() ? "8" : text());
//   //QSize label_sz(0, 0);
// //   if(st)
// //   {
// // //       QStyleOptionMenuItem option;
// // //       option.checkType = QStyleOptionMenuItem::NonExclusive;
// // //       option.checked = isChecked();
// // //       option.menuHasCheckableItems = true;
// // //       option.menuItemType = QStyleOptionMenuItem::Normal;
// // //       //option.menuRect = menuItemControlRect();
// // //       option.text = text();
// // 
// //     QStyleOptionButton option;
// //     option.state = QStyle::State_Active | QStyle::State_Enabled | QStyle::State_HasFocus | QStyle::State_On;
// //     //option.text = text();
// //     
// //     //menu_item_sz = st->sizeFromContents(QStyle::CT_MenuItem, &option, text_sz); //, q);
// //     label_sz = st->sizeFromContents(QStyle::CT_CheckBox, &option, QSize(0, 0)); //, q);
// //   }
// 
// 
//   //_menuItemControlRect = menu_item_sz;
//   //_menuItemControlRect = QRect(margin, margin + max_h_hdr_h + max_title_h + itemVSpacing, menu_item_sz.width(), menu_item_sz.height());
//   //_menuItemControlRect = QRect(actionHMargin, 0, checkbox_sz.width() + actionHMargin, checkbox_sz.height());
//   const int menu_item_h = text_sz.height() > checkbox_sz.height() ? text_sz.height() : checkbox_sz.height();
//   _checkBoxControlRect = QRect(actionHMargin,
//                                0,
//                                checkbox_sz.width() + actionHMargin,
//                                menu_item_h);
//   _labelControlRect = QRect(actionHMargin + checkbox_sz.width() + actionHMargin, 
//                             0, 
//                             text_sz.width(),
//                             menu_item_h);
//   
//   // REMOVE Tim. Persistent routes. Added.
//   //fprintf(stderr, "RoutingMatrixWidgetAction::updateChannelArray text width:%d br w:%d h:%d sz w:%d h:%d\n", 
//   //        text_fm.width(text()), text_sz.width(), text_sz.height(), menu_item_sz.width(), menu_item_sz.height());
//   fprintf(stderr, "RoutingMatrixWidgetAction::updateChannelArray menu item checkbox w:%d h:%d label w:%d h:%d\n", 
//           checkbox_sz.width(), checkbox_sz.height(), text_sz.width(), text_sz.height());
  
  
  // Determine the maximum horizontal header height.
  int max_h_hdr_h = 0;
  if(_array.headerVisible())
  {
    for(int col = 0; col < cols; ++col)
    {
      const QString str = _array.headerText(col);
      const QFontMetrics fm(str.isEmpty() ? smallFont() : font());
      const QRect brect = fm.boundingRect(str.isEmpty() ? "8" : str);
      const int r_h = brect.height();
      if(r_h > max_h_hdr_h)
        max_h_hdr_h = r_h;
    }
  
    // Set the horizontal header rectangles.
    //x = margin + cellW + itemHSpacing;
    //int tot_array_w = 0;
    int x = margin + itemHSpacing;
    int y = margin;
    for(int col = 0; col < cols; ++col)
    {
      if(col != 0 && ((col % itemsPerGroup) == 0))
      {
        x += groupSpacing;
        //tot_array_w += groupSpacing;
      }
      
      const QString str = _array.headerText(col);
      const QFontMetrics fm(str.isEmpty() ? smallFont() : font());
      const QRect brect = fm.boundingRect(str.isEmpty() ? "888" : str);
      int r_w = brect.width();
      if(r_w < cellW)
        r_w = cellW;
      const int cell_w = r_w + itemHSpacing;
      const QRect r(x, y, cell_w, max_h_hdr_h);
      _array.headerSetRect(col, r);
      x += cell_w;
      //tot_array_w += cell_w;
    }
  }
  
  // Set the array rectangles.
  int y = margin + max_h_hdr_h + itemVSpacing;
  int x = margin + itemHSpacing;
  for(int col = 0; col < cols; ++col)
  {
    if(col != 0 && ((col % itemsPerGroup) == 0))
      x += groupSpacing;
    const int hdr_w = _array.headerRect(col).width();
    const int cell_w = (hdr_w > cellW) ? hdr_w : cellW;
    const QRect r(x, y, cell_w, cellH);
    _array.setRect(col, r);
    x += cell_w;
  }
  
  // Force it to resize. This is one of only a few methods we can call 
  //  to trigger it which don't check to see if the value is already set.
  //setMenu(menu());
}

// void RoutingMatrixWidgetAction::actionHovered()
// {  
//   fprintf(stderr, "RoutingMatrixWidgetAction::actionHovered action text:%s\n", text().toLatin1().constData()); // REMOVE Tim. Persistent routes. Added.
//   const int m_sz = associatedWidgets().size();
//   for(int k = 0; k < m_sz; ++k)
//   {
//     fprintf(stderr, "   assoc widget:%d\n", k); // REMOVE Tim. Persistent routes. Added.
//     if(QMenu* m = qobject_cast< QMenu* >(associatedWidgets().at(k)))
//     {
//       fprintf(stderr, "   menu:%p\n", m); // REMOVE Tim. Persistent routes. Added.
//       const int sz = m->actions().size();
//       for(int i = 0; i < sz; ++i)
//       {
//         fprintf(stderr, "   action:%d text:%s\n", i, m->actions().at(i)->text().toLatin1().constData()); // REMOVE Tim. Persistent routes. Added.
//         if(RoutingMatrixWidgetAction* wa = qobject_cast< RoutingMatrixWidgetAction* >(m->actions().at(i)))
//         {
//           fprintf(stderr, "   matrix\n"); // REMOVE Tim. Persistent routes. Added.
//           const bool sel = (this == wa);
//           if(wa->isSelected() != sel)
//           {
//             fprintf(stderr, "   sel:%d\n", sel); // REMOVE Tim. Persistent routes. Added.
//             wa->setSelected(sel);
//             wa->updateCreatedWidgets();
//           }
//         }
//       }
//     }
//   }
// }

void RoutingMatrixWidgetAction::updateCreatedWidgets()
{
  const int sz = createdWidgets().size();
  for(int i = 0; i < sz; ++i)
    createdWidgets().at(i)->update();
}

QWidget* RoutingMatrixWidgetAction::createWidget(QWidget *parent)
{
//   RoutingMatrixWidget* widget = new RoutingMatrixWidget(this, parent);
//   fprintf(stderr, "RoutingMatrixWidgetAction::createWidget widget:%p\n", widget); // REMOVE Tim. Persistent routes. Added. 
//   return widget;
  
  RoutingMatrixActionWidget* lw = new RoutingMatrixActionWidget(this, parent);
//   lw->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
//   
//   //QWidget* lw = new QWidget(parent);
//   
//   QHBoxLayout* h_layout = new QHBoxLayout(lw);
//   h_layout->setSpacing(0);
//   h_layout->setContentsMargins(0, 0, 0, 0);
// 
//   // Remove Tim. Just a test.
// //   h_layout->addWidget(_itemLabel);
// //   h_layout->addStretch();
// //   h_layout->addWidget(new QLabel("TestLabel", parent));
//   
//   QVBoxLayout* left_v_layout = new QVBoxLayout();
//   QVBoxLayout* right_v_layout = new QVBoxLayout();
//   left_v_layout->setSpacing(0);
//   right_v_layout->setSpacing(0);
//   left_v_layout->setContentsMargins(0, 0, 0, 0);
//   right_v_layout->setContentsMargins(0, 0, 0, 0);
//   
// //   if(!_array.headerTitle().isEmpty())
// //   {
// //     QLabel* lbl = new QLabel(_array.headerTitle(), parent);
// //     lbl->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
// //     lbl->setAlignment(Qt::AlignCenter);
// //     lbl->setAutoFillBackground(true);
// //     //QPalette palette;
// //     //palette.setColor(label->backgroundRole(), c);
// //     //lbl->setPalette(palette);
// //     lbl->setBackgroundRole(QPalette::Dark);
// //     left_v_layout->addWidget(lbl);
// //   }
//   if(!_array.headerTitle().isEmpty() || !_array.checkBoxTitle().isEmpty())
//   {
//     QHBoxLayout* left_title_layout = new QHBoxLayout();
//     left_title_layout->setSpacing(0);
//     left_title_layout->setContentsMargins(0, 0, 0, 0);
//     if(!_array.checkBoxTitle().isEmpty())
//     {
//       QLabel* lbl = new QLabel(_array.checkBoxTitle(), parent);
//       lbl->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
//       lbl->setAlignment(Qt::AlignCenter);
//       lbl->setAutoFillBackground(true);
//       lbl->setBackgroundRole(QPalette::Dark);
//       left_title_layout->addWidget(lbl);
//     }
//     //left_title_layout->addSpacing(actionHMargin);
//     if(!_array.headerTitle().isEmpty())
//     {
//       QLabel* lbl = new QLabel(_array.headerTitle(), parent);
//       lbl->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
//       lbl->setAlignment(Qt::AlignCenter);
//       lbl->setAutoFillBackground(true);
//       lbl->setBackgroundRole(QPalette::Dark);
//       left_title_layout->addWidget(lbl);
//     }
//     left_v_layout->addLayout(left_title_layout);
//   }
//   left_v_layout->addStretch();
// 
//   //MenuItemControlWidget* micw = new MenuItemControlWidget(this, parent);
//   //left_v_layout->addWidget(micw);
// //   QHBoxLayout* mic_h_layout = new QHBoxLayout();
// //   mic_h_layout->setSpacing(0);
// //   mic_h_layout->setContentsMargins(0, 0, 0, 0);
//   MenuItemControlWidget* micw = new MenuItemControlWidget(this, parent);
//   micw->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);
// //   micw->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
// //   mic_h_layout->addWidget(micw);
// //   mic_h_layout->addWidget(_itemLabel);
//   //mic_h_layout->addStretch();
// //   left_v_layout->addLayout(mic_h_layout);
//   left_v_layout->addWidget(micw);
// 
//   // Remove Tim. Just a test.
//   //left_v_layout->addWidget(_itemLabel);
//   //right_v_layout->addWidget(new QLabel("TestLabel", parent));
//   //left_v_layout->addWidget(new QLabel("TestLabel", parent));
//   //right_v_layout->addWidget(_itemLabel);
// 
//   
//   if(_array.arrayTitle().isEmpty())
//   {
//     right_v_layout->addStretch();
// //     RoutingMatrixWidget* switch_widget = new RoutingMatrixWidget(this, parent);
// //     switch_widget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);
// //     right_v_layout->addWidget(switch_widget);
//   }
//   else
//   {
//     QLabel* lbl = new QLabel(_array.arrayTitle(), parent);
//     //lbl->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
//     lbl->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
//     lbl->setAlignment(Qt::AlignCenter);
//     lbl->setAutoFillBackground(true);
//     //QPalette palette;
//     //palette.setColor(label->backgroundRole(), c);
//     //lbl->setPalette(palette);
//     lbl->setBackgroundRole(QPalette::Dark);
//     right_v_layout->addWidget(lbl);
// //     QHBoxLayout* sw_h_layout = new QHBoxLayout();
// //     sw_h_layout->setSpacing(0);
// //     sw_h_layout->setContentsMargins(0, 0, 0, 0);
// //     RoutingMatrixWidget* switch_widget = new RoutingMatrixWidget(this, parent);
// //     switch_widget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);
// //     sw_h_layout->addStretch();
// //     sw_h_layout->addWidget(switch_widget);
// //     right_v_layout->addLayout(sw_h_layout);
//   }
//   
//   QHBoxLayout* sw_h_layout = new QHBoxLayout();
//   sw_h_layout->setSpacing(0);
//   sw_h_layout->setContentsMargins(0, 0, 0, 0);
//   sw_h_layout->addStretch();
//   RoutingMatrixWidget* switch_widget = new RoutingMatrixWidget(this, parent);
//   switch_widget->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
//   sw_h_layout->addWidget(switch_widget);
//   right_v_layout->addLayout(sw_h_layout);
// 
//   h_layout->addLayout(left_v_layout);
//   h_layout->addStretch();
//   h_layout->addLayout(right_v_layout);
//   //h_layout->addLayout(sw_h_layout);
//   
// //   QSignalMapper* mapper = new QSignalMapper(this);
// // 
// //   PixmapButton* pb = new PixmapButton(toggle_small_Icon, toggle_small_Icon, 2, lw, QString("T"));  // Margin  = 2
// //   //mapper->setMapping(pb, _channels);  // Set to one past end.
// //   h_layout->addWidget(pb); 
// //   h_layout->addSpacing(6);
// //   //connect(pb, SIGNAL(clicked(bool)), mapper, SLOT(map()));
// //   
// //   for(int i = 0; i < _channels; ++i)
// //   {
// //     PixmapButton* b = new PixmapButton(_refPixmap, _refPixmap, 2, lw, QString::number(i + 1));  // Margin  = 2
// //     mapper->setMapping(b, i);
// //     connect(b, SIGNAL(pressed()), mapper, SLOT(map()));
// //     if((i != 0) && (i % 4 == 0))
// //       h_layout->addSpacing(6);
// //     h_layout->addWidget(b); 
// //   }
// // 
// //   connect(mapper, SIGNAL(mapped(int)), this, SLOT(chanClickMap(int)));
  
  return lw;
}

void RoutingMatrixWidgetAction::deleteWidget(QWidget* widget)
{
  fprintf(stderr, "RoutingMatrixWidgetAction::deleteWidget widget:%p\n", widget); // REMOVE Tim. Persistent routes. Added. 
  QWidgetAction::deleteWidget(widget);
}  

// void RoutingMatrixWidgetAction::activate(ActionEvent event)
// {
//   fprintf(stderr, "RoutingMatrixWidgetAction::activate event:%d\n", event); // REMOVE Tim. Persistent routes. Added. 
//   QWidgetAction::activate(event);
// }

void RoutingMatrixWidgetAction::sendActionChanged()
{
  QActionEvent e(QEvent::ActionChanged, this);
  
  // This code copied and modified from QActionPrivate::sendDataChanged() source.
  // Using QAction::setText() calls QActionPrivate::sendDataChanged() which doesn't
  //  send to the created widgets so fails to resize properly on text changes.
  
  // Update the created widgets first.
  for (int i = 0; i < createdWidgets().size(); ++i) {
      QWidget *w = createdWidgets().at(i);
      //fprintf(stderr, "RoutingMatrixWidgetAction::sendActionChanged created widget:%s\n", w->metaObject()->className()); // REMOVE Tim. Persistent routes. Added. 
      qApp->sendEvent(w, &e);
  }

  // Now update the associated widgets and graphics widgets (popup menus, widgets etc. containing this action)...
  
#ifndef QT_NO_GRAPHICSVIEW
  for (int i = 0; i < associatedGraphicsWidgets().size(); ++i) {
      //fprintf(stderr, "RoutingMatrixWidgetAction::sendActionChanged associated graphics widget\n"); // REMOVE Tim. Persistent routes. Added. 
      QGraphicsWidget *w = associatedGraphicsWidgets().at(i);
      qApp->sendEvent(w, &e);
  }
#endif

  for (int i = 0; i < associatedWidgets().size(); ++i) {
      QWidget *w = associatedWidgets().at(i);
      //fprintf(stderr, "RoutingMatrixWidgetAction::sendActionChanged associated widget:%s \n", w->metaObject()->className()); // REMOVE Tim. Persistent routes. Added. 
      qApp->sendEvent(w, &e);
  }
  emit changed();
}

// void RoutingMatrixWidgetAction::setItemLabelText(const QString& s) 
// { 
//   //_itemLabel->setText(s); 
//   _labelText = s;
//   // Need to update the rectangle.
//   updateChannelArray();
//   // Need to send it twice to take effect
//   sendActionChanged(); 
//   //sendActionChanged(); 
// }

void RoutingMatrixWidgetAction::setActionText(const QString& s) 
{ 
  //fprintf(stderr, "RoutingMatrixWidgetAction::setActionText\n"); // REMOVE Tim. Persistent routes. Added. 
//   QWidgetAction::setText(s);
  //_itemLabel->setText(s); 
  _labelText = s;
  // Need to update the rectangle.
  //updateChannelArray();
  
  // Need to send ActionChanged twice to take effect
  // setText already sends ActionChanged, but we need it twice AFTER the text change, 
  //  so that the resizing can work fully.
  // NOTE: FIXME This is a big speed hit sending ActionChanged three times !
  // The solution is to keep our own QString and not use QWidgetAction::setText (done), 
  //  but that's still TWO ActionChanged sends...
  // A better way might be tell the widget to updateGeometry and so on before a single ActionChanged message.
  sendActionChanged(); 
}

bool RoutingMatrixWidgetAction::event(QEvent* e)
{
  fprintf(stderr, "RoutingMatrixWidgetAction::event type:%d\n", e->type()); // REMOVE Tim. Persistent routes. Added. 
  return QWidgetAction::event(e);
}

bool RoutingMatrixWidgetAction::eventFilter(QObject* obj, QEvent* e)
{
  fprintf(stderr, "RoutingMatrixWidgetAction::eventFilter type:%d\n", e->type()); // REMOVE Tim. Persistent routes. Added. 
  return QWidgetAction::eventFilter(obj, e);
}


//---------------------------------------------------------
//   RoutingMatrixHeaderWidgetAction
//---------------------------------------------------------

RoutingMatrixHeaderWidgetAction::RoutingMatrixHeaderWidgetAction(const QString& checkbox_label, const QString& item_label, const QString& array_label, QWidget* parent)
  : QWidgetAction(parent)
{
  setEnabled(false);
  _checkBoxLabel = new QLabel(checkbox_label, parent);
  _checkBoxLabel->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
  _checkBoxLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
  _checkBoxLabel->setAutoFillBackground(true);
  _checkBoxLabel->setBackgroundRole(QPalette::Dark);
  
  _itemLabel = new QLabel(item_label, parent);
  _itemLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
  _itemLabel->setAlignment(Qt::AlignCenter);
  //_itemLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
  _itemLabel->setAutoFillBackground(true);
  _itemLabel->setBackgroundRole(QPalette::Dark);
  
  _arrayLabel = new QLabel(array_label, parent);
  _arrayLabel->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
  _arrayLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
  _arrayLabel->setAutoFillBackground(true);
  _arrayLabel->setBackgroundRole(QPalette::Dark);
}

QWidget* RoutingMatrixHeaderWidgetAction::createWidget(QWidget *parent)
{
  QWidget* lw = new QWidget(parent);
  lw->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
  
  QHBoxLayout* h_layout = new QHBoxLayout(lw);
  h_layout->setSpacing(0);
  h_layout->setContentsMargins(0, 0, 0, 0);

  if(!_checkBoxLabel->text().isEmpty())
    h_layout->addWidget(_checkBoxLabel);
  //h_layout->addSpacing(15);
  if(!_itemLabel->text().isEmpty())
    h_layout->addWidget(_itemLabel);
  if(!_arrayLabel->text().isEmpty())
    h_layout->addWidget(_arrayLabel);
  return lw;
}

void RoutingMatrixHeaderWidgetAction::deleteWidget(QWidget* widget)
{
  fprintf(stderr, "RoutingMatrixHeaderWidgetAction::deleteWidget widget:%p\n", widget); // REMOVE Tim. Persistent routes. Added. 
  QWidgetAction::deleteWidget(widget);
}  

} // namespace MusEGui
