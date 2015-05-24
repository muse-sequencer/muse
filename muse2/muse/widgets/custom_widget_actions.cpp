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
  init();
}
    
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
  const int cellW = _action->cellGeometry().width();
  const int cellH = _action->cellGeometry().height();
  const int c_groups = _action->array()->columns() / _action->itemsPerGroup;
  const int r_groups = _action->array()->rows() / _action->itemsPerGroup;
  const int w = (_action->array()->columns() + 1) * (cellW + _action->itemHSpacing) +  // 1 extra for the vertical header column.
                c_groups * _action->groupSpacing + 
                2 * _action->margin;
  const int h = (_action->array()->rows() + 1) * (cellH + _action->itemVSpacing) +    // 1 extra for the horizontal header row.
                r_groups * _action->groupSpacing + 
                2 * _action->margin;
  return QSize(w, h);
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
  
  p.setFont(_action->cellFont());
  const int h_rows = _action->header()->rows();
  const int h_cols = _action->header()->columns();
  for(int row = 0; row < h_rows; ++row)
  {
    QRect r = _action->header()->rect(row, -1);
    p.drawText(r, Qt::AlignRight | Qt::AlignVCenter, QString::number(row + 1));
  }
  for(int col = 0; col < h_cols; ++col)
  {
    QRect r = _action->header()->rect(-1, col);
    p.drawText(r, Qt::AlignCenter, QString::number(col + 1));
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
                                                     QWidget* parent)
  : QWidgetAction(parent)
{
  // Just to be safe, set to -1 instead of default 0.
  //setData(-1);
  _onPixmap = on_pixmap;
  _offPixmap = off_pixmap;
  _header.setSize(rows, cols);
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
  int y = margin + cellH + itemVSpacing;
  for(int row = 0; row < rows; ++row)
  {
    if(row != 0 && ((row % itemsPerGroup) == 0))
      y += groupSpacing;
    int x = margin + cellW + itemHSpacing;
    for(int col = 0; col < cols; ++col)
    {
      if(col != 0 && ((col % itemsPerGroup) == 0))
        x += groupSpacing;
      
      const QRect r(x, y, cellW, cellH);
      _array.setRect(row, col, r);
      
      x += cellW + itemHSpacing;
    }
    y += cellH + itemVSpacing;
  }

  const int h_rows = _header.rows();
  const int h_cols = _header.columns();
  int x = margin;
  y = margin + cellH + itemVSpacing;
  for(int row = 0; row < h_rows; ++row)
  {
    if(row != 0 && ((row % itemsPerGroup) == 0))
      y += groupSpacing;
    const QRect r(x, y, cellW, cellH);
    _header.setRect(row, -1, r);
    y += cellH + itemVSpacing;
  }
  x = margin + cellW + itemHSpacing;
  y = margin;
  for(int col = 0; col < h_cols; ++col)
  {
    if(col != 0 && ((col % itemsPerGroup) == 0))
      x += groupSpacing;
    const QRect r(x, y, cellW, cellH);
    _header.setRect(-1, col, r);
    x += cellW + itemHSpacing;
  }
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
