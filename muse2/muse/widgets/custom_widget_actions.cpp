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
    
//     if(_array)
//       delete[] _array;
//     _array = 0;
//     if(_cols == 0 || _rows == 0)
//       return *this;
//     _array = new RouteChannelArrayItem[_rows * _cols];
  }
  
  //const int sz = _rows * _cols;
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
  
//   if(_array)
//     delete[] _array;
//   _array = 0;
//   _cols = cols;
//   _rows = rows;
//   if(_cols == 0 || _rows == 0)
//     return;
//   _array = new RouteChannelArrayItem[_rows * _cols];
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
  //if(_cols == 0 || _rows == 0)
  //if((_cols == 0 && _rows == 0) || (_cols != 0 && _rows != 0))
  const int sz = itemCount();
  if(sz == 0)
    return;
  //_array = new RouteChannelArrayItem[_rows * _cols];
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

const int RoutingMatrixWidget::margin = 1;
const int RoutingMatrixWidget::itemHSpacing = 1;
const int RoutingMatrixWidget::itemVSpacing = 3;
const int RoutingMatrixWidget::groupSpacing = 4;
const int RoutingMatrixWidget::itemsPerGroup = 4;

RoutingMatrixWidget::RoutingMatrixWidget(int rows, int cols, QPixmap* onPixmap, QPixmap* offPixmap, QWidget* parent)
  : QWidget(parent)
{
  _header.setSize(rows, cols);
  _current.setSize(rows, cols);
  _onPixmap = onPixmap;
  _offPixmap = offPixmap;
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

void RoutingMatrixWidget::updateChannelArray()
{
  const int rows = _current.rows();
  const int cols = _current.columns();
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
      //int x = margin + vertHeaderWidth + col * (_onPixmap->width() + itemSpacing);
      //int x = margin + (col + 1) * (cellW + itemSpacing);
      //if(col != 0 && ((col % itemsPerGroup) == 0))
      //  x += groupSpacing;
      //const int y = margin + horizHeaderHeight + row * (_onPixmap->height() + itemSpacing);
      //const int y = margin + (row + 1) * (cellH + itemSpacing);
      //const QRect r(x, y, _onPixmap->width(), _onPixmap->height());
      
      const QRect r(x, y, cellW, cellH);
      _current.setRect(row, col, r);
      
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
    //const int y = margin + horizHeaderHeight + row * (_onPixmap->height() + itemSpacing);
    //const int y = margin + (row + 1) * (cellH + itemSpacing);
    //const QRect r(x, y, vertHeaderWidth, _onPixmap->height());
    const QRect r(x, y, cellW, cellH);
    _header.setRect(row, -1, r);
    y += cellH + itemVSpacing;
  }
  x = margin + cellW + itemHSpacing;
  y = margin;
  for(int col = 0; col < h_cols; ++col)
  {
    //int x = margin + vertHeaderWidth + col * (_onPixmap->width() + itemSpacing);
    //int x = margin + (col + 1) * (cellW + itemSpacing);
    if(col != 0 && ((col % itemsPerGroup) == 0))
      x += groupSpacing;
    //const int y = margin;
    //const QRect r(x, y, _onPixmap->width(), horizHeaderHeight);
    const QRect r(x, y, cellW, cellH);
    _header.setRect(-1, col, r);
    x += cellW + itemHSpacing;
  }
}

QSize RoutingMatrixWidget::sizeHint() const
{
  const int cellW = _cellGeometry.width();
  const int cellH = _cellGeometry.height();
  const int c_groups = _current.columns() / itemsPerGroup;
  const int r_groups = _current.rows() / itemsPerGroup;
  //const int w = _current.columns() * (_onPixmap->width() + itemSpacing) + 
  const int w = (_current.columns() + 1) * (cellW + itemHSpacing) + 
                c_groups * groupSpacing + 
                //vertHeaderWidth + 
                2 * margin;
  //const int h = _current.rows() * (_onPixmap->height() + itemSpacing) + 
  const int h = (_current.rows() + 1) * (cellH + itemVSpacing) + 
                r_groups * groupSpacing + 
                //horizHeaderHeight + 
                2 * margin;
  return QSize(w, h);
}

void RoutingMatrixWidget::drawGrid(QPainter& p)
{
  const int rows = _current.rows();
  const int cols = _current.columns();
  const int cellW = _cellGeometry.width();
  const int cellH = _cellGeometry.height();
  const int c_groups = _current.columns() / itemsPerGroup;
  const int r_groups = _current.rows() / itemsPerGroup;
  const int w = margin + _current.columns() * (cellW + itemHSpacing) + 
                c_groups * groupSpacing +
                cellW; // One more for the last line, minus the itemSpacing.
  const int h = margin + _current.rows() * (cellH + itemVSpacing) + 
                r_groups * groupSpacing +
                cellH; // One more for the last line, minus the itemSpacing.
  const int x0 = margin + cellW;
  const int x1 = w;
  const int y0 = margin + cellH;
  const int y1 = h;
  
  int y = margin + cellH;
  for(int row = 0; row <= rows; ++row) // <= to get the last line.
  {
    int line_y;
    if(row != 0 && ((row % itemsPerGroup) == 0))
    {
      line_y = y + (itemVSpacing + groupSpacing) / 2;
      y += groupSpacing;
    }
    else
      line_y = y + itemVSpacing / 2;
    p.drawLine(x0, line_y, x1, line_y);
    y += cellH + itemVSpacing;
  }
  
  int x = margin + cellW;
  for(int col = 0; col <= cols; ++col) // <= to get the last line.
  {
    int line_x;
    if(col != 0 && ((col % itemsPerGroup) == 0))
    {
      line_x = x + (itemHSpacing + groupSpacing) / 2;
      x += groupSpacing;
    }
    else
      line_x = x + itemHSpacing / 2;
    p.drawLine(line_x, y0, line_x, y1);
    x += cellW + itemHSpacing;
  }
}

void RoutingMatrixWidget::paintEvent(QPaintEvent* /*event*/)
{
  QPainter p(this);

  //drawGrid(p);
  
  //const QRect geo = geometry();
  const int rows = _current.rows();
  const int cols = _current.columns();
  for(int row = 0; row < rows; ++row)
  {
    for(int col = 0; col < cols; ++col)
    {
      const QPixmap& pm = _current.value(row, col) ? *_onPixmap : *_offPixmap;
      const int pm_w = pm.width();
      const int pm_h = pm.height();
      //fprintf(stderr, "RoutingMatrixWidget::paintEvent pm_w:%d pm_h:%d\n", pm_w, pm_h); // REMOVE Tim. Persistent routes. Added. 
//       // Only draw if on.
//       if(!_current.value(row, col))
//         continue;
      const QRect r = _current.rect(row, col);
      //int x = r.x() + geo.x();
      int x = r.x();
      if(r.width() > pm_w)
        x += (r.width() - pm_w) / 2;
      //int y = r.y() + geo.y();
      int y = r.y();
      if(r.height() > pm_h)
        y += (r.height() - pm_h) / 2;
      p.drawPixmap(x, y, pm);
      
      //p.fillRect(r.x(), r.y(), r.width(), r.height(), _current.value(row, col) ? Qt::green : Qt::red);
      //p.fillRect(r.x(), r.y(), r.width(), r.height(), Qt::green);
    }
  }
  
  p.setFont(_cellFont);
  const int h_rows = _header.rows();
  const int h_cols = _header.columns();
  for(int row = 0; row < h_rows; ++row)
  {
    QRect r = _header.rect(row, -1);
    //const int x = r.x() + geo.x();
    //const int y = r.y() + geo.y();
    //const int x = r.x();
    //const int y = r.y();
    //fprintf(stderr, "RoutingMatrixWidget::paintEvent vheader x:%d y:%d\n", x, y); // REMOVE Tim. Persistent routes. Added. 
    //p.drawText(x, y, QString::number(row));
    //p.drawText(QRect(x, y, r.width(), r.height()), Qt::AlignRight | Qt::AlignVCenter, QString::number(row));
    p.drawText(r, Qt::AlignRight | Qt::AlignVCenter, QString::number(row + 1));
  }
  for(int col = 0; col < h_cols; ++col)
  {
    QRect r = _header.rect(-1, col);
    //const int x = r.x() + geo.x();
    //const int y = r.y() + geo.y();
    //const int x = r.x();
    //const int y = r.y();
    //fprintf(stderr, "RoutingMatrixWidget::paintEvent hheader x:%d y:%d\n", x, y); // REMOVE Tim. Persistent routes. Added. 
    //p.drawText(QRect(x, y, r.width(), r.height()), Qt::AlignCenter, QString::number(col));
    p.drawText(r, Qt::AlignCenter, QString::number(col + 1));
  }
}
    
//---------------------------------------------------------
//   RoutingMatrixWidgetAction
//---------------------------------------------------------

RoutingMatrixWidgetAction::RoutingMatrixWidgetAction(//const QString& text, 
                                                     int rows, int cols, 
                                                     QPixmap* on_pixmap, QPixmap* off_pixmap, 
                                                     //const RouteChannelArray& initial, 
                                                     QWidget* parent)
  : QWidgetAction(parent)
{
  _rows = rows;
  _cols = cols;
  _onPixmap = on_pixmap;
  _offPixmap = off_pixmap;
  //_current = initial;
  //_text = text;
  // Just to be safe, set to -1 instead of default 0.
  setData(-1);
}

QWidget* RoutingMatrixWidgetAction::createWidget(QWidget *parent)
{
  fprintf(stderr, "RoutingMatrixWidgetAction::createWidget\n"); // REMOVE Tim. Persistent routes. Added. 
  RoutingMatrixWidget* _widget = new RoutingMatrixWidget(_rows, _cols, _onPixmap, _offPixmap, parent);
  return _widget;
  
//   const int channels = _current.size();
//   QWidget* lw = new QWidget(parent);
//   QHBoxLayout* layout = new QHBoxLayout(lw);
// 
//   layout->setSpacing(0);
//   
//   QLabel* lbl = new QLabel(_text, lw);
//   lbl->setAlignment(Qt::AlignCenter);
//   //lbl->setAutoFillBackground(true);
//   //QPalette palette;
//   //palette.setColor(label->backgroundRole(), c);
//   //lbl->setPalette(palette);
//   //lbl->setBackgroundRole(QPalette::Dark);
//   layout->addWidget(lbl); 
//   
//   layout->addSpacing(8);
//   layout->addStretch();
//       
//   QSignalMapper* mapper = new QSignalMapper(this);
// 
//   PixmapButton* pb = new PixmapButton(toggle_small_Icon, toggle_small_Icon, 2, lw);  // Margin  = 2
//   mapper->setMapping(pb, channels);  // Set to one past end.
//   layout->addWidget(pb); 
//   layout->addSpacing(6);
//   connect(pb, SIGNAL(pressed()), mapper, SLOT(map()));
//   
//   for(int i = 0; i < channels; ++i)
//   {
//     bool set = _current.at(i);
//     PixmapButton* b = new PixmapButton(_onPixmap, _offPixmap, 2, lw);  // Margin  = 2
//     _chan_buttons.append(b);
//     b->setCheckable(true);
//     b->setDown(set);
//     mapper->setMapping(b, i);
//     connect(b, SIGNAL(toggled(bool)), mapper, SLOT(map()));
//     if((i != 0) && (i % 4 == 0))
//       layout->addSpacing(6);
//     layout->addWidget(b); 
//   }
// 
//   connect(mapper, SIGNAL(mapped(int)), this, SLOT(chanClickMap(int)));
//   
//   return lw;
}


} // namespace MusEGui
