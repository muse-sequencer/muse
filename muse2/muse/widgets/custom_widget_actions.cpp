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
#include <QPainterPath>
#include <QString>
#include <QList>
#include <QApplication>

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
      fprintf(stderr, "PixmapButtonsWidgetAction::chanClickMap: filling bit array with false\n"); // REMOVE Tim. Persistent routes. Added. 
      _current.fill(false);
    }
    else
    {
      fprintf(stderr, "PixmapButtonsWidgetAction::chanClickMap: filling bit array with true\n"); // REMOVE Tim. Persistent routes. Added. 
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
        fprintf(stderr, "PixmapButtonsWidgetAction::chanClickMap: setting bit:%d\n", i); // REMOVE Tim. Persistent routes. Added. 
        _current.setBit(i);
      }
      else
      {
        fprintf(stderr, "PixmapButtonsWidgetAction::chanClickMap: clearing bit:%d\n", i); // REMOVE Tim. Persistent routes. Added. 
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


//---------------------------------------------------------
//   RouteChannelArray
//---------------------------------------------------------

RouteChannelArray::RouteChannelArray(int rows, int cols)
{
  _array = 0;
  _cols = cols;
  _rows = rows;
  _visible_cols = cols;
  _visible_rows = rows;
  _rowsExclusive = false;
  _colsExclusive = false;
  init();
}

RouteChannelArray::~RouteChannelArray()
{
  if(_array)
  {
    delete[] _array;
    _array = 0;
  }
}

void RouteChannelArray::init()
{
  if(_array)
  {
    delete[] _array;
    _array = 0;
  }
  const int sz = itemCount();
  if(sz == 0)
    return;
  _array = new RouteChannelArrayItem[sz];
}

RouteChannelArray& RouteChannelArray::operator=(const RouteChannelArray& a)
{
  if(a._cols != _cols || a._rows != _rows)
  {
    _cols = a._cols;
    _rows = a._rows;
    _visible_cols = a._visible_cols;
    _visible_rows = a._visible_rows;
    init();
  }
  const int sz = itemCount();
  if(sz == 0)
    return *this;
  for(int i = 0; i < sz; ++i)
    _array[i] = a._array[i];
  return *this;
}
    
void RouteChannelArray::setSize(int rows, int cols)
{
  if(cols == _cols && rows == _rows)
    return;
  _cols = cols;
  _rows = rows;
  _visible_cols = cols;
  _visible_rows = rows;
  init();
}

void RouteChannelArray::setValues(int row, int col, bool value, bool exclusive_rows, bool exclusive_cols, bool exclusive_toggle)
{ 
  if(invalidIndex(row, col)) 
    return; 
  
  const bool v = (!exclusive_toggle || (exclusive_toggle && value));
  if(exclusive_rows && exclusive_cols)
  {
    for(int r = 0; r < _rows; ++r)
    {
      for(int c = 0; c < _cols; ++c)
        _array[itemIndex(r, c)]._value = (r == row && c == col && v);
    }
    return;
  }
  
  if(exclusive_rows)
  {
    for(int r = 0; r < _rows; ++r)
      _array[itemIndex(r, col)]._value = (r == row && v);
    return;
  }
  
  if(exclusive_cols)
  {
    for(int c = 0; c < _cols; ++c)
      _array[itemIndex(row, c)]._value = (c == col && v); 
    return;
  }
  
  _array[itemIndex(row, col)]._value = value; 
}

void RouteChannelArray::setValue(int row, int col, bool value)
{ 
  setValues(row, col, value, _rowsExclusive, _colsExclusive, _exclusiveToggle);
}  
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



//---------------------------------------------------------
//   RouteChannelArrayHeader
//---------------------------------------------------------

void RouteChannelArrayHeader::init()
{
  if(_array)
  {
    delete[] _array;
    _array = 0;
  }
  const int sz = itemCount();
  if(sz == 0)
    return;
  _array = new RouteChannelArrayItem[sz];
}

bool RouteChannelArrayHeader::invalidIndex(int row, int col) const 
{ 
  if(row == -1)
    return col == -1 || col >= _cols;
  if(col == -1)
    return row >= _rows;
  return true;
}

void RouteChannelArrayHeader::setValues(int row, int col, bool value, bool exclusive_rows, bool exclusive_cols, bool exclusive_toggle)
{ 
  if(invalidIndex(row, col)) 
    return; 
  const bool v = (!exclusive_toggle || (exclusive_toggle && value));
  if(row != -1 && exclusive_rows)
  {
    for(int r = 0; r < _rows; ++r)
      _array[itemIndex(r, -1)]._value = (r == row && v);
    return;
  }
  if(col != -1 && exclusive_cols)
  {
    for(int c = 0; c < _cols; ++c)
      _array[itemIndex(-1, c)]._value = (c == col && v); 
    return;
  }
  _array[itemIndex(row, col)]._value = value; 
}

//---------------------------------------------------------
//   RoutingMatrixWidget
//---------------------------------------------------------

RoutingMatrixWidget::RoutingMatrixWidget(RoutingMatrixWidgetAction* action, QWidget* parent)
  : QWidget(parent)
{
  _action = action;
}

QSize RoutingMatrixWidget::sizeHint() const
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
     

  const int rows = _action->array()->rows();
  const int cols = _action->array()->columns();
  //const int h_rows = _action->header()->rows();
  //const int h_cols = _action->header()->columns();
  const QRect last_array_v_cell = _action->array()->rect(rows - 1, 0);
  const QRect last_array_h_cell = _action->array()->rect(0, cols - 1);
  // Take the height of any horizontal header column - they're all the same.
  const int h = //_action->header()->rect(-1, 0).height() + 
                last_array_v_cell.y() + last_array_v_cell.height() +
                2 * _action->margin;
  // Take the width of any vertical header row - they're all the same.
  const int w = //_action->header()->rect(0, -1).width() +
                last_array_h_cell.x() + last_array_h_cell.width() +
                2 * _action->margin;
//   for(int row = 0; row < h_rows; ++row)
//     h += _action->header()->rect(row, -1).height();
//   for(int col = 0; col < h_cols; ++col)
//     w += _action->header()->rect(-1, col).width();
  
//   w += 2 * _action->margin;
//   h += 2 * _action->margin;
  
  return QSize(w, h);
}

void RoutingMatrixWidget::resizeEvent(QResizeEvent* e)
{
  fprintf(stderr, "RoutingMatrixWidget::resizeEvent\n");
  QWidget::resizeEvent(e);
}


void RoutingMatrixWidget::drawGrid(QPainter& p)
{
  const int rows = _action->array()->rows();
  const int cols = _action->array()->columns();
  const int cellW = _action->cellGeometry().width();
  const int cellH = _action->cellGeometry().height();
  const int c_groups = _action->array()->columns() / _action->itemsPerGroup;
  const int r_groups = _action->array()->rows() / _action->itemsPerGroup;
  const int w = _action->margin + _action->array()->columns() * (cellW + _action->itemHSpacing) + 
                c_groups * _action->groupSpacing +
                cellW; // One more for the last line, minus the itemSpacing.
  const int h = _action->margin + _action->array()->rows() * (cellH + _action->itemVSpacing) + 
                r_groups * _action->groupSpacing +
                cellH; // One more for the last line, minus the itemSpacing.
  const int x0 = _action->margin + cellW;
  const int x1 = w;
  const int y0 = _action->margin + cellH;
  const int y1 = h;
  
  int y = _action->margin + cellH;
  for(int row = 0; row <= rows; ++row) // Using <= to get the last line.
  {
    int line_y;
    if(row != 0 && ((row % _action->itemsPerGroup) == 0))
    {
      line_y = y + (_action->itemVSpacing + _action->groupSpacing) / 2;
      y += _action->groupSpacing;
    }
    else
      line_y = y + _action->itemVSpacing / 2;
    p.drawLine(x0, line_y, x1, line_y);
    y += cellH + _action->itemVSpacing;
  }
  
  int x = _action->margin + cellW;
  for(int col = 0; col <= cols; ++col) // Using <= to get the last line.
  {
    int line_x;
    if(col != 0 && ((col % _action->itemsPerGroup) == 0))
    {
      line_x = x + (_action->itemHSpacing + _action->groupSpacing) / 2;
      x += _action->groupSpacing;
    }
    else
      line_x = x + _action->itemHSpacing / 2;
    p.drawLine(line_x, y0, line_x, y1);
    x += cellW + _action->itemHSpacing;
  }
}

void RoutingMatrixWidget::paintEvent(QPaintEvent* /*event*/)
{
  QPainter p(this);

  // Not used - too cluttered, like looking through a screen, too hard to distinguish the squares and the
  //  fact that with a grid, 'off' means no pixmap or drawing at all, so only the grid shows so it's hard
  //  to pick the right box to click on. And the added group spacing makes it look distorted and confusing.
  //drawGrid(p); 
  
  const int rows = _action->array()->rows();
  const int cols = _action->array()->columns();
  p.setFont(_action->cellFont());
  for(int row = 0; row < rows; ++row)
  {
    for(int col = 0; col < cols; ++col)
    {
      const QPixmap& pm = _action->array()->value(row, col) ? *_action->onPixmap() : *_action->offPixmap();
      const int pm_w = pm.width();
      const int pm_h = pm.height();
      const QRect r = _action->array()->rect(row, col);
      int x = r.x();
      if(r.width() > pm_w)
        x += (r.width() - pm_w) / 2;
      int y = r.y();
      if(r.height() > pm_h)
        y += (r.height() - pm_h) / 2;
      p.drawPixmap(x, y, pm);
    }
  }
  
  //p.setFont(_action->font());
  //const int h_rows = _action->header()->rows();
  //const int h_cols = _action->header()->columns();
  const int vis_h_rows = _action->header()->visibleRows();
  const int vis_h_cols = _action->header()->visibleColumns();
  for(int row = 0; row < vis_h_rows; ++row)
  {
    const QRect r = _action->header()->rect(row, -1);
    const QString str = _action->header()->text(row, -1);
    if(str.isEmpty())
    {
      p.setFont(_action->cellFont());
      p.drawText(r, Qt::AlignRight | Qt::AlignVCenter, QString::number(row + 1));
    }
    else
    {
      p.setFont(_action->font());
      p.drawText(r, Qt::AlignLeft | Qt::AlignVCenter, str);
    }
  }
  for(int col = 0; col < vis_h_cols; ++col)
  {
    const QRect r = _action->header()->rect(-1, col);
    const QString str = _action->header()->text(-1, col);
    if(str.isEmpty())
    {
      p.setFont(_action->cellFont());
      p.drawText(r, Qt::AlignCenter, QString::number(col + 1));
    }
    else
    {
      p.setFont(_action->font());
      p.drawText(r, Qt::AlignCenter, str);
    }
  }
}

void RoutingMatrixWidget::mousePressEvent(QMouseEvent* ev)
{
  const int rows = _action->array()->rows();
  const int cols = _action->array()->columns();
  const QPoint pt = ev->pos();
  bool changed = false;
  for(int row = 0; row < rows; ++row)
  {
    for(int col = 0; col < cols; ++col)
    {
      const QRect rect = _action->array()->rect(row, col);
      if(rect.contains(pt))
      {
        _action->array()->setValue(row, col, !_action->array()->value(row, col));  // TODO: Add a toggleValue.
        changed = true;
        break;
      }
    }
    if(changed)
      break;
  }

  if(changed)
  {
    //ev->accept();
    update();  // Redraw the indicators.
    //return;
  }
  
  ev->ignore();  // Don't accept. Let the menu close if neccessary.
}

    
//---------------------------------------------------------
//   RoutingMatrixWidgetAction
//---------------------------------------------------------

const int RoutingMatrixWidgetAction::margin = 1;
const int RoutingMatrixWidgetAction::itemHSpacing = 1;
const int RoutingMatrixWidgetAction::itemVSpacing = 3;
const int RoutingMatrixWidgetAction::groupSpacing = 4;
const int RoutingMatrixWidgetAction::itemsPerGroup = 4;

RoutingMatrixWidgetAction::RoutingMatrixWidgetAction(int rows, int cols, 
                                                     QPixmap* on_pixmap, QPixmap* off_pixmap, 
                                                     QWidget* parent,
                                                     int visible_rows, int visible_cols)
  : QWidgetAction(parent)
{
  // Just to be safe, set to -1 instead of default 0.
  //setData(-1);
  _onPixmap = on_pixmap;
  _offPixmap = off_pixmap;
  _header.setSize(rows, cols);
  if(visible_rows != -1)
    _header.setVisibleRows(visible_rows);
  if(visible_cols != -1)
    _header.setVisibleColums(visible_cols);
  _array.setSize(rows, cols);
  _cellFont = font();
  _cellFont.setPointSize(6);
  QFontMetrics fm(_cellFont);
  _cellGeometry = fm.boundingRect("888");
  if(_cellGeometry.width() < _onPixmap->width())
    _cellGeometry.setWidth(_onPixmap->width());
  if(_cellGeometry.height() < _onPixmap->height())
    _cellGeometry.setHeight(_onPixmap->height());
  updateChannelArray();
}

void RoutingMatrixWidgetAction::updateChannelArray()
{
  const int rows = _array.rows();
  const int cols = _array.columns();
  const int cellW = _cellGeometry.width();
  const int cellH = _cellGeometry.height();
  
  
  
//   int y = margin + cellH + itemVSpacing;
//   for(int row = 0; row < rows; ++row)
//   {
//     if(row != 0 && ((row % itemsPerGroup) == 0))
//       y += groupSpacing;
//     int x = margin + cellW + itemHSpacing;
//     for(int col = 0; col < cols; ++col)
//     {
//       if(col != 0 && ((col % itemsPerGroup) == 0))
//         x += groupSpacing;
//       
//       const QRect r(x, y, cellW, cellH);
//       _array.setRect(row, col, r);
//       
//       x += cellW + itemHSpacing;
//     }
//     y += cellH + itemVSpacing;
//   }

  const int h_rows = _header.rows();
  const int h_cols = _header.columns();
  
  // Determine the maximum vertical header width.
  int max_v_hdr_w = 0;
  if(_header.visibleRows() != 0)
  {
    for(int row = 0; row < h_rows; ++row)
    {
      const QString str = _header.text(row, -1);
      if(str.isEmpty())
      {
        const int cell_w = cellW + itemHSpacing;
        if(cell_w > max_v_hdr_w)
          max_v_hdr_w = cell_w;
      }
      else
      {
        QFont fnt = font();
        //fnt.setPointSize(6);
        const QFontMetrics fm(fnt);
        const QRect r = fm.boundingRect(str);
        const int r_w = r.width();
        if(r_w > max_v_hdr_w)
          max_v_hdr_w = r_w;
      }
    }
  }
  
  // Determine the maximum horizontal header height.
  int max_h_hdr_h = 0;
  if(_header.visibleColumns() != 0)
  {
    for(int col = 0; col < h_cols; ++col)
    {
      const QString str = _header.text(-1, col);
      if(str.isEmpty())
      {
        const int cell_h = cellH + itemVSpacing;
        if(cell_h > max_h_hdr_h)
          max_h_hdr_h = cell_h;
      }
      else
      {
        QFont fnt = font();
        //fnt.setPointSize(6);
        const QFontMetrics fm(fnt);
        const QRect r = fm.boundingRect(str);
        const int r_h = r.height();
        if(r_h > max_h_hdr_h)
          max_h_hdr_h = r_h;
      }
    }
  }

  // Set the vertical header rectangles.
  int x = margin;
  //y = margin + cellH + itemVSpacing;
  int y = margin + max_h_hdr_h + itemVSpacing;
  for(int row = 0; row < h_rows; ++row)
  {
    if(row != 0 && ((row % itemsPerGroup) == 0))
      y += groupSpacing;

    int cell_h;
    const QString str = _header.text(row, -1);
    if(str.isEmpty())
      cell_h = cellH + itemVSpacing;
    else
    {
      QFont fnt = font();
      //fnt.setPointSize(6);
      const QFontMetrics fm(fnt);
      const QRect r = fm.boundingRect(str);
      int r_h = r.height();
      if(onPixmap() && r_h < onPixmap()->height())
        r_h = onPixmap()->height();
      cell_h = r_h + itemVSpacing;
    }
    
    //const QRect r(x, y, cellW, cellH);
    const QRect r(x, y, max_v_hdr_w, cell_h);
    _header.setRect(row, -1, r);
    //y += cellH + itemVSpacing;
    y += cell_h;
  }
  
  // Set the horizontal header rectangles.
  //x = margin + cellW + itemHSpacing;
  x = margin + max_v_hdr_w + itemHSpacing;
  y = margin;
  for(int col = 0; col < h_cols; ++col)
  {
    if(col != 0 && ((col % itemsPerGroup) == 0))
      x += groupSpacing;
    
    int cell_w;
    const QString str = _header.text(-1, col);
    if(str.isEmpty())
      cell_w = cellW + itemHSpacing;
    else
    {
      QFont fnt = font();
      //fnt.setPointSize(6);
      const QFontMetrics fm(fnt);
      const QRect r = fm.boundingRect(str);
      int r_w = r.width();
      if(onPixmap() && r_w < onPixmap()->width())
        r_w = onPixmap()->width();
      cell_w = r_w + itemHSpacing;
    }
    
    //const QRect r(x, y, cellW, cellH);
    const QRect r(x, y, cell_w, max_h_hdr_h);
    _header.setRect(-1, col, r);
    //x += cellW + itemHSpacing;
    x += cell_w;
  }
  
  // Set the array rectangles.
  //int y = margin + cellH + itemVSpacing;
  y = margin + max_h_hdr_h + itemVSpacing;
  for(int row = 0; row < rows; ++row)
  {
    if(row != 0 && ((row % itemsPerGroup) == 0))
      y += groupSpacing;
    const int cell_h = _header.rect(row, -1).height();
    //int x = margin + cellW + itemHSpacing;
    x = margin + max_v_hdr_w + itemHSpacing;
    for(int col = 0; col < cols; ++col)
    {
      if(col != 0 && ((col % itemsPerGroup) == 0))
        x += groupSpacing;
      
      const int cell_w = _header.rect(-1, col).width();
      
      //const QRect r(x, y, cellW, cellH);
      const QRect r(x, y, cell_w, cell_h);
      _array.setRect(row, col, r);
      
      //x += cellW + itemHSpacing;
      x += cell_w;
    }
    //y += cellH + itemVSpacing;
    y += cell_h;
  }
  
//   QList<QWidget*> created_widgets(createdWidgets());
//   //Q_Q(QAction);
//   QActionEvent e(QEvent::ActionChanged, this);
//   //QActionEvent e(QEvent::ActionChanged, q);
//   if(!created_widgets.isEmpty())
//   {
//     QWidget* w = created_widgets.front();
//     fprintf(stderr, "RoutingMatrixWidgetAction::updateChannelArray widget:%p\n", w); // REMOVE Tim. Persistent routes. Added. 
//     //w->adjustSize();
//     //w->update();
//     QApplication::sendEvent(w, &e);
//   }
//   QApplication::sendEvent(this, &e);
//   //QApplication::sendEvent(q, &e);
//   
// //   Q_Q(QAction);
// //   QActionEvent e(QEvent::ActionChanged, q);
// //   for (int i = 0; i < widgets.size(); ++i) {
// //     QWidget *w = widgets.at(i);
// //     QApplication::sendEvent(w, &e);
// //   }
// //   QApplication::sendEvent(q, &e);
// 
//   //emit q->changed();
//   emit changed();

  // Force it to resize. This is one of only a few methods we can call 
  //  to trigger it which don't check to see if the value is already set.
  setMenu(menu());

  
//   // Determine the maximum row height and column width.
//   //int y = margin + cellH + itemVSpacing;
//   int y = margin + max_h_hdr_h + itemVSpacing;
//   int max_row_h = 0;
//   int max_col_w = 0;
//   for(int row = 0; row < rows; ++row)
//   {
//     if(row != 0 && ((row % itemsPerGroup) == 0))
//       y += groupSpacing;
//     //int x = margin + cellW + itemHSpacing;
//     int x = margin + max_v_hdr_w + itemHSpacing;
//     for(int col = 0; col < cols; ++col)
//     {
//       if(col != 0 && ((col % itemsPerGroup) == 0))
//         x += groupSpacing;
// 
// 
//       const QString str = _array.text(row, col);
//       if(str.isEmpty())
//       {
//         
//       }
//       else
//       {
//         QFont fnt = font();
//         //fnt.setPointSize(6);
//         const QFontMetrics fm(fnt);
//         const QRect r = fm.boundingRect(str);
//         const int r_w = r.width();
//         const int r_h = r.height();
//         if(r_h > max_h_hdr_h)
//           max_h_hdr_h = r_h;
//       }
//       
//       const QRect r(x, y, cellW, cellH);
//       _array.setRect(row, col, r);
//       
//       x += cellW + itemHSpacing;
//     }
//     y += cellH + itemVSpacing;
//   }
  
}

QWidget* RoutingMatrixWidgetAction::createWidget(QWidget *parent)
{
  RoutingMatrixWidget* widget = new RoutingMatrixWidget(this, parent);
  fprintf(stderr, "RoutingMatrixWidgetAction::createWidget widget:%p\n", widget); // REMOVE Tim. Persistent routes. Added. 
  return widget;
}

void RoutingMatrixWidgetAction::deleteWidget(QWidget* widget)
{
  fprintf(stderr, "RoutingMatrixWidgetAction::deleteWidget widget:%p\n", widget); // REMOVE Tim. Persistent routes. Added. 
  QWidgetAction::deleteWidget(widget);
}  

} // namespace MusEGui
