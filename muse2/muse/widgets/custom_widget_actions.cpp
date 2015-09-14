//=============================================================================
//  MusE
//  Linux Music Editor
//
//  custom_widget_actions.cpp
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
//=============================================================================

#include <QLabel>
#include <QHBoxLayout>
#include <QSignalMapper>
#include <QPainter>
#include <QPaintEvent>
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
  lbl->setBackgroundRole(QPalette::Dark);
  layout->addWidget(lbl); 
  layout->addSpacing(8);
  QSignalMapper* mapper = new QSignalMapper(this);

  PixmapButton* pb = new PixmapButton(toggle_small_Icon, toggle_small_Icon, 2, lw, QString("T"));  // Margin  = 2
  layout->addWidget(pb); 
  layout->addSpacing(6);
  
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
  
// void PixmapButtonsHeaderWidgetAction::chanClickMap(int /*idx*/)
// {
//   // TODO: Toggle vertical columns...   p4.0.42 
//   //trigger();  // REMOVE Tim. Persistent routes. Removed. Already triggered by widget, causes double activation!
// }

  
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
      _current.fill(false);
    else
      _current.fill(true);
    
    // Set and redraw the buttons.
    for(int i = 0; i < buttons_sz; ++i)
      _chan_buttons.at(i)->setDown(allch != channels);
  }
  else
  {
    for(int i = 0; i < channels && i < buttons_sz; ++i)
    {
      if(_chan_buttons.at(i)->isChecked())
        _current.setBit(i);
      else
        _current.clearBit(i);
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

RouteChannelArray::RouteChannelArray(int cols)
{
  _array = 0;
  _headerVisible = true;
  _header = 0;
  _cols = cols;
  _colsExclusive = false;
  _exclusiveToggle = false;
  _activeCol = -1;
  _pressedCol = -1;
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
  const int sz = columns();
  if(sz == 0)
    return;
  _array = new RouteChannelArrayItem[sz];
  _header = new RouteChannelArrayHeaderItem[sz];
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
  
  const int sz = columns();
  if(sz == 0)
    return *this;
  for(int i = 0; i < sz; ++i)
  {
    _array[i] = a._array[i];
    _header[i] = a._header[i];
  }
  return *this;
}
    
void RouteChannelArray::setColumns(int cols)
{
  if(cols == _cols)
    return;
  _cols = cols;
  init();
}

void RouteChannelArray::setValues(int col, bool value, bool exclusive_cols, bool exclusive_toggle)
{ 
  if(invalidColumn(col)) 
    return; 
  
  const bool v = (!exclusive_toggle || (exclusive_toggle && value));
  if(exclusive_cols)
  {
    for(int c = 0; c < _cols; ++c)
      _array[c]._value = (c == col && v); 
    return;
  }
  
  _array[col]._value = value; 
}

MenuItemControlWidget::MenuItemControlWidget(RoutingMatrixWidgetAction* action, QWidget* parent)
             : QWidget(parent)
{
  _action = action;
  setMouseTracking(true);
}

void MenuItemControlWidget::elementRect(QRect* checkbox_rect, QRect* label_rect) const
{
  QSize checkbox_sz(0, 0);
  
  if(_action->hasCheckBox())
  {
    QStyle* st = style() ? style() : QApplication::style();
    if(st)
    {
      QStyleOptionButton option;
      option.state = QStyle::State_Active | QStyle::State_Enabled | QStyle::State_HasFocus |
                      (_action->checkBoxChecked() ? QStyle::State_On : QStyle::State_Off);
      checkbox_sz = st->sizeFromContents(QStyle::CT_CheckBox, &option, QSize(0, 0)); //, q);
    }
  }

  const QFontMetrics txt_fm(_action->font());
  const QSize txt_sz = txt_fm.size(Qt::TextSingleLine, _action->actionText().isEmpty() ? "8" : _action->actionText());
  const int menu_item_h = txt_sz.height() > checkbox_sz.height() ? txt_sz.height() : checkbox_sz.height();
  if(checkbox_rect)
    *checkbox_rect = QRect(0, 0, checkbox_sz.width(), menu_item_h);
  if(label_rect)
    *label_rect = QRect(0, 0, txt_sz.width(), menu_item_h);
}

QSize MenuItemControlWidget::sizeHint() const
{
  QRect cb_ctrl_rect;
  QRect lbl_ctrl_rect;
  
  elementRect(&cb_ctrl_rect, &lbl_ctrl_rect);
  const int cb_w = _action->hasCheckBox() ? (_action->actionHMargin + cb_ctrl_rect.x() + cb_ctrl_rect.width()) : 0;
  const int l_w = cb_w + _action->actionHMargin + lbl_ctrl_rect.x() + lbl_ctrl_rect.width();
  
  const int cb_h = cb_ctrl_rect.y() + cb_ctrl_rect.height();
  const int l_h = lbl_ctrl_rect.y() + lbl_ctrl_rect.height();
  const int h = l_h > cb_h ? l_h : cb_h;
  return QSize(cb_w + l_w, h);
}

void MenuItemControlWidget::paintEvent(QPaintEvent*)
{
  QPainter p(this);

  QRect cb_ctrl_rect;
  QRect lbl_ctrl_rect;
  
  elementRect(&cb_ctrl_rect, &lbl_ctrl_rect);
   
  if(_action->isSelected())
    p.fillRect(rect(), palette().highlight());
  
  if(_action->hasCheckBox())
  {
    QStyle* st = style() ? style() : QApplication::style();
    if(st)
    {
        QStyleOptionButton option;
        option.state = QStyle::State_Active | QStyle::State_HasFocus | 
                      (_action->isEnabled() ? QStyle::State_Enabled : QStyle::State_ReadOnly) |  // Or State_None?
                      (_action->checkBoxChecked() ? QStyle::State_On : QStyle::State_Off) |
                      (_action->menuItemPressed() ? QStyle::State_Sunken : QStyle::State_None);
        option.rect = QRect(_action->actionHMargin + cb_ctrl_rect.x(), 
                            cb_ctrl_rect.y(), 
                            cb_ctrl_rect.width(), 
                            cb_ctrl_rect.height());
        option.palette = palette();
        st->drawControl(QStyle::CE_CheckBox, &option, &p);
    }
  }  
  
  if(!_action->actionText().isEmpty())
  {
    QPalette pal = palette();
    pal.setCurrentColorGroup(_action->isEnabled() ? QPalette::Active : QPalette::Disabled);
    p.setPen(_action->isSelected() ? pal.highlightedText().color() : pal.text().color());
    p.setFont(_action->font());
    const int l_x = _action->actionHMargin + (_action->hasCheckBox() ? (_action->actionHMargin + cb_ctrl_rect.x() + cb_ctrl_rect.width()) : 0);
    const QRect l_r(l_x, lbl_ctrl_rect.y(), 
                    lbl_ctrl_rect.width(), lbl_ctrl_rect.height());
    p.drawText(l_r, Qt::AlignLeft | Qt::AlignVCenter, _action->actionText());
  }
}


//---------------------------------------------------------
//   SwitchBarActionWidget
//---------------------------------------------------------

SwitchBarActionWidget::SwitchBarActionWidget(RoutingMatrixWidgetAction* action, QWidget* parent)
  : QWidget(parent)
{
  _action = action;
  setMouseTracking(true);
}

QSize SwitchBarActionWidget::sizeHint() const
{
  const int cols = _action->array()->columns();
  const QRect last_array_h_cell = _action->array()->rect(cols - 1);
  // Take the height of any horizontal header column - they're all the same.
  const int hdr_h = _action->array()->headerVisible() ? _action->array()->headerRect(0).height() : 0;
  const int h = hdr_h + 
                last_array_h_cell.height() +
                _action->itemVSpacing + 
                2 * _action->margin;
  // Take the width of any vertical header row - they're all the same.
  const int w = last_array_h_cell.x() + last_array_h_cell.width() +
                2 * _action->margin;
  return QSize(w, h);
}

void SwitchBarActionWidget::paintEvent(QPaintEvent* /*event*/)
{
  QPainter p(this);
  const int cols = _action->array()->columns();
  p.setFont(_action->font());
  QString a_txt;
  for(int col = 0; col < cols; ++col)
  {
    const QRect r = _action->array()->rect(col);
    if(col == _action->array()->pressedColumn())
      p.fillRect(r, palette().dark());
    else if(col == _action->array()->activeColumn())
      p.fillRect(r, palette().highlight());
    const QPixmap& pm = _action->array()->value(col) ? *_action->onPixmap() : *_action->offPixmap();
    const int pm_w = pm.width();
    const int pm_h = pm.height();
    
    int x;
    a_txt = _action->array()->text(col);
    if(!a_txt.isEmpty())
    {
      p.drawText(r, Qt::AlignLeft | Qt::AlignVCenter, a_txt);
      x = r.x() + r.width() - pm_w - _action->groupSpacing; // Leave some space at the end.
    }
    else
    {
      x = r.x();
      if(r.width() > pm_w)
        x += (r.width() - pm_w) / 2;
    }
    
    int y = r.y();
    if(r.height() > pm_h)
      y += (r.height() - pm_h) / 2;
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

//---------------------------------------------------------
//   RoutingMatrixActionWidget
//---------------------------------------------------------

RoutingMatrixActionWidget::RoutingMatrixActionWidget(RoutingMatrixWidgetAction* action, QWidget* parent)
             : QWidget(parent)
{
  _action = action;

  setMouseTracking(true);
  setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
  
  const int layout_m_l = 0, layout_m_r = 0, layout_m_t = 1, layout_m_b = 1;
  
  QHBoxLayout* h_layout = new QHBoxLayout(this);
  h_layout->setSpacing(0);
  h_layout->setContentsMargins(layout_m_l, layout_m_t, layout_m_r, layout_m_b);

  QVBoxLayout* left_v_layout = new QVBoxLayout();
  QVBoxLayout* right_v_layout = new QVBoxLayout();
  left_v_layout->setSpacing(0);
  right_v_layout->setSpacing(0);
  left_v_layout->setContentsMargins(0, 0, 0, 0);
  right_v_layout->setContentsMargins(0, 0, 0, 0);
  
  if(!_action->array()->headerTitle().isEmpty() || !_action->array()->checkBoxTitle().isEmpty())
  {
    QHBoxLayout* left_title_layout = new QHBoxLayout();
    left_title_layout->setSpacing(0);
    left_title_layout->setContentsMargins(0, 0, 0, 0); // Zero because we're already inside a layout.
    if(!_action->array()->checkBoxTitle().isEmpty())
    {
      QLabel* cb_lbl = new QLabel(_action->array()->checkBoxTitle(), parent);
      cb_lbl->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
      cb_lbl->setAlignment(Qt::AlignCenter);
      cb_lbl->setAutoFillBackground(true);
      cb_lbl->setBackgroundRole(QPalette::Dark);
      left_title_layout->addWidget(cb_lbl);
      left_title_layout->addSpacing(4);
    }
    if(!_action->array()->headerTitle().isEmpty())
    {
      QLabel* hdr_lbl = new QLabel(_action->array()->headerTitle(), parent);
      hdr_lbl->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
      hdr_lbl->setAlignment(Qt::AlignCenter);
      hdr_lbl->setAutoFillBackground(true);
      hdr_lbl->setBackgroundRole(QPalette::Dark);
      left_title_layout->addWidget(hdr_lbl);
      left_title_layout->addSpacing(4);
    }
    left_v_layout->addLayout(left_title_layout);
  }
  left_v_layout->addStretch();

  _menuItemControlWidget = new MenuItemControlWidget(_action, parent);
  _menuItemControlWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
  left_v_layout->addWidget(_menuItemControlWidget);

  if(_action->array()->arrayTitle().isEmpty())
  {
    right_v_layout->addStretch();
  }
  else
  {
    QLabel* lbl = new QLabel(_action->array()->arrayTitle(), parent);
    lbl->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
    lbl->setAlignment(Qt::AlignCenter);
    lbl->setAutoFillBackground(true);
    lbl->setBackgroundRole(QPalette::Dark);
    right_v_layout->addWidget(lbl);
  }
  
  QHBoxLayout* sw_h_layout = new QHBoxLayout();
  sw_h_layout->setSpacing(0);
  sw_h_layout->setContentsMargins(0, 0, 0, 0);
  sw_h_layout->addStretch();
  _switchWidget = new SwitchBarActionWidget(_action, parent);
  _switchWidget->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
  sw_h_layout->addWidget(_switchWidget);
  right_v_layout->addLayout(sw_h_layout);

  h_layout->addLayout(left_v_layout);
  h_layout->addLayout(right_v_layout);
}

RoutePopupHit RoutingMatrixActionWidget::hitTest(const QPoint& p, RoutePopupHit::HitTestType test_type)
{
  if(_action->isEnabled())
  {
    // The point is relative to this widget.
    // Check if we hit the left hand portion (the checkbox and text area).
    if(_menuItemControlWidget->geometry().contains(p))
    {
      switch(test_type)
      {
        case RoutePopupHit::HitTestClick:
        case RoutePopupHit::HitTestHover:
          if(_action->hasCheckBox())
            return RoutePopupHit(_action, RoutePopupHit::HitMenuItem);
        break;
      }
      return RoutePopupHit(_action, RoutePopupHit::HitSpace);
    }
    
    // Check if we hit one of the channel bar channels.
    const QPoint cb_p(p.x() - _switchWidget->x(), p.y() - _switchWidget->y()); 
    const int cols = _action->array()->columns();
    for(int col = 0; col < cols; ++col)
    {
      const QRect rect = _action->array()->rect(col);
      if(rect.contains(cb_p))
        return RoutePopupHit(_action, RoutePopupHit::HitChannel, col);
    }

    // Check if we hit the channel bar itself.
    if(_switchWidget->geometry().contains(p))
      return RoutePopupHit(_action, RoutePopupHit::HitChannelBar);
    
    // Check if we hit this widget.
    if(rect().contains(p))
      return RoutePopupHit(_action, RoutePopupHit::HitSpace);
  }
  
  return RoutePopupHit(_action, RoutePopupHit::HitNone);
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
  QWidget::actionEvent(e);
}


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
  : QWidgetAction(parent)
{
  _actionText = action_text;
  _hasCheckBox = false;
  _checkBoxChecked = false;
  _menuItemPressed = false;
  _arrayStayOpen = false;
  _isSelected = false;
  _onPixmap = on_pixmap;
  _offPixmap = off_pixmap;
  _array.setColumns(cols);
  _smallFont = font();
  _smallFont.setPointSize(_smallFont.pointSize() / 2 + 1);
  
  if(_maxPixmapGeometry.width() < _onPixmap->width())
    _maxPixmapGeometry.setWidth(_onPixmap->width());
  if(_maxPixmapGeometry.height() < _onPixmap->height())
    _maxPixmapGeometry.setHeight(_onPixmap->height());
  
  if(_maxPixmapGeometry.width() < _offPixmap->width())
    _maxPixmapGeometry.setWidth(_offPixmap->width());
  if(_maxPixmapGeometry.height() < _offPixmap->height())
    _maxPixmapGeometry.setHeight(_offPixmap->height());
  updateChannelArray();
}

void RoutingMatrixWidgetAction::updateChannelArray()
{
  const int cols = _array.columns();
  const int cellW = _maxPixmapGeometry.width();
  const int cellH = _maxPixmapGeometry.height();
  
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
    int x = margin + itemHSpacing;
    int y = margin;
    for(int col = 0; col < cols; ++col)
    {
      if(col != 0 && ((col % itemsPerGroup) == 0))
        x += groupSpacing;
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
    }
  }
  
  // Set the array rectangles.
  int y = margin + max_h_hdr_h + itemVSpacing;
  int x = margin + itemHSpacing;
  const QFontMetrics a_fm(font());
  for(int col = 0; col < cols; ++col)
  {
    if(col != 0 && ((col % itemsPerGroup) == 0))
      x += groupSpacing;
    const int hdr_w = _array.headerRect(col).width();
    int txt_w = 0;
    int txt_h = 0;
    if(!_array.text(col).isEmpty())
    {
      const QRect b_rect = a_fm.boundingRect(_array.text(col));
      txt_w = b_rect.width() + margin + groupSpacing; // Add a wee bit of space.
      txt_h = b_rect.height();
    }
    int cell_w = txt_w + cellW;
    if(hdr_w > cell_w)
      cell_w = hdr_w;
    const int cell_h = txt_h > cellH ? txt_h : cellH;
    
    const QRect r(x, y, cell_w, cell_h);
    _array.setRect(col, r);
    x += cell_w;
  }
}

void RoutingMatrixWidgetAction::updateCreatedWidgets()
{
  const int sz = createdWidgets().size();
  for(int i = 0; i < sz; ++i)
    createdWidgets().at(i)->update();
}

QWidget* RoutingMatrixWidgetAction::createWidget(QWidget *parent)
{
  RoutingMatrixActionWidget* lw = new RoutingMatrixActionWidget(this, parent);
  return lw;
}

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

void RoutingMatrixWidgetAction::setActionText(const QString& s) 
{ 
  //fprintf(stderr, "RoutingMatrixWidgetAction::setActionText\n"); // REMOVE Tim. Persistent routes. Added. 
  _actionText = s;
  sendActionChanged(); 
}

RoutePopupHit RoutingMatrixWidgetAction::hitTest(const QPoint& p, RoutePopupHit::HitTestType test_type)
{
  for(int i = 0; i < createdWidgets().size(); ++i)
  {
    QWidget* w = createdWidgets().at(i);
    if(RoutingMatrixActionWidget* maw = qobject_cast< RoutingMatrixActionWidget* >(w))
    {
      // The point is relative to the menu. Translate the point to reference our created container widget.
      RoutePopupHit hit = maw->hitTest(QPoint(p.x() - maw->x(), p.y() - maw->y()), test_type);
      if(hit._type != RoutePopupHit::HitNone)
        return hit;
    }
  }  
  return RoutePopupHit(this, RoutePopupHit::HitNone);
}

RoutePopupHit RoutingMatrixWidgetAction::previousHit(const RoutePopupHit& fromHit)
{
  RoutePopupHit retHit = fromHit;
  retHit._action = this;
  const int cols = array()->columns();
  switch(fromHit._type)
  {
    case RoutePopupHit::HitChannel:
    {
      if(cols == 0)
      {
        if(hasCheckBox())
        {
          retHit._type = RoutePopupHit::HitMenuItem;
          retHit._value = 0;
        }
      }
      else
      {
        int col = fromHit._value; // The column.
        if(col > cols) // Greater than. Let the decrement work.
          col = cols;
        --col;
        if(col == -1)
        {
          if(hasCheckBox())
          {
            retHit._type = RoutePopupHit::HitMenuItem;
            retHit._value = 0;
          }
          else
            col = cols - 1;  // Wrap around.
        }
        if(col != -1)
          retHit._value = col; // Adjust the current 'last' column setting.
      }
    }
    break;
    
    case RoutePopupHit::HitMenuItem:
    {
      if(cols != 0)
      {
        const int col = cols - 1; // Wrap around.
        retHit._type = RoutePopupHit::HitChannel;
        retHit._value = col;
      }
    }
    break; 
    
    case RoutePopupHit::HitChannelBar:
    case RoutePopupHit::HitSpace:
    case RoutePopupHit::HitNone:
      // If it has a checkbox (or there is no channel bar) select the checkbox/text area.
      if(hasCheckBox() || array()->columns() == 0)
      {
        retHit._type = RoutePopupHit::HitMenuItem;
        retHit._value = 0;
      }
      // Otherwise select the first available channel bar column.
      else
      {
        retHit._type = RoutePopupHit::HitChannel;
        retHit._value = 0;
      }
    break;
  }
  
  return retHit;
}

RoutePopupHit RoutingMatrixWidgetAction::nextHit(const RoutePopupHit& fromHit)
{
  RoutePopupHit retHit = fromHit;
  const int cols = array()->columns();
  switch(fromHit._type)
  {
    case RoutePopupHit::HitChannel:
    {
      if(cols == 0)
      {
        if(hasCheckBox())
        {
          retHit._type = RoutePopupHit::HitMenuItem;
          retHit._action = this;
          retHit._value = 0;
        }
      }
      else
      {
        int col = fromHit._value; // The column.
        ++col;
        if(col >= cols)
        {
          if(hasCheckBox())
          {
            retHit._type = RoutePopupHit::HitMenuItem;
            retHit._action = this;
            retHit._value = 0;
            col = -1;
          }
          else
            col = 0;  // Wrap around.
        }
        if(col != -1)
          retHit._value = col; // Adjust the current 'last' column setting.
      }
    }
    break;
    
    case RoutePopupHit::HitMenuItem:
    {
      if(cols != 0)
      {
        const int col = 0; // Select the first available channel.
        retHit._type = RoutePopupHit::HitChannel;
        retHit._action = this;
        retHit._value = col;
      }
    }
    break; 
    
    case RoutePopupHit::HitChannelBar:
    case RoutePopupHit::HitSpace:
    case RoutePopupHit::HitNone:
      // If it has a checkbox (or there is no channel bar) select the checkbox/text area.
      if(hasCheckBox() || array()->columns() == 0)
      {
        retHit._type = RoutePopupHit::HitMenuItem;
        retHit._action = this;
        retHit._value = 0;
      }
      // Otherwise select the first available channel bar column.
      else
      {
        retHit._type = RoutePopupHit::HitChannel;
        retHit._action = this;
        retHit._value = 0;
      }
    break;
  }
  
  return retHit;
}


//---------------------------------------------------------
//   RoutingMatrixHeaderWidgetAction
//---------------------------------------------------------

RoutingMatrixHeaderWidgetAction::RoutingMatrixHeaderWidgetAction(const QString& checkbox_label, const QString& item_label, const QString& array_label, QWidget* parent)
  : QWidgetAction(parent), _checkBoxLabel(checkbox_label), _itemLabel(item_label), _arrayLabel(array_label)
{
  setEnabled(false);
}

QWidget* RoutingMatrixHeaderWidgetAction::createWidget(QWidget *parent)
{
  QWidget* lw = new QWidget(parent);
  lw->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
  
  QHBoxLayout* h_layout = new QHBoxLayout(lw);
  h_layout->setSpacing(0);
  h_layout->setContentsMargins(0, 0, 0, 0);

  if(!_checkBoxLabel.isEmpty())
  {
    QLabel* lbl = new QLabel(_checkBoxLabel, parent);
    lbl->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
    lbl->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    lbl->setAutoFillBackground(true);
    lbl->setBackgroundRole(QPalette::Dark);
    h_layout->addWidget(lbl);
  }
  
  if(!_itemLabel.isEmpty())
  {
    QLabel* lbl = new QLabel(_itemLabel, parent);
    lbl->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
    lbl->setAlignment(Qt::AlignCenter);
    lbl->setAutoFillBackground(true);
    lbl->setBackgroundRole(QPalette::Dark);
    h_layout->addSpacing(4);
    h_layout->addWidget(lbl);
  }
  
  if(!_arrayLabel.isEmpty())
  {
    QLabel* lbl = new QLabel(_arrayLabel, parent);
    lbl->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
    lbl->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    lbl->setAutoFillBackground(true);
    lbl->setBackgroundRole(QPalette::Dark);
    h_layout->addSpacing(4);
    h_layout->addWidget(lbl);
  }
  
  return lw;
}

} // namespace MusEGui
