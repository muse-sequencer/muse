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

} // namespace MusEGui
