//=============================================================================
//  MusE
//  Linux Music Editor
//  (C) Copyright 1999-2001 Werner Schweer (ws@seh.de)
//       midi_widget_action.cpp
//  (C) Copyright 2011 Time E. Real (terminator356 on users.sourceforge.net)
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
#include <QCheckBox>
#include <QBoxLayout>

#include "midi_widget_action.h"

namespace MusEGui {

//---------------------------------------------------------
//   MidiChanSelWidgetAction
//---------------------------------------------------------

MidiChanSelWidgetAction::PixmapButtonsWidgetAction(const QString& ss, int channels, QWidget* parent)
  : QWidgetAction(parent)
      {
        _channels = channels;
        s = ss;
        // Don't allow to click on it.
        //setEnabled(false);
        // Just to be safe, set to -1 instead of default 0.
        setData(-1);
      }

QWidget* MidiChanSelWidgetAction::createWidget(QWidget *parent)
{
  QWidget* lw = new QWidget(parent);
  QBoxLayout* layout = new QBoxLayout(QBoxLayout::LeftToRight, lw);

  QLabel* lbl = new QLabel(s, lw);
  lbl->setAlignment(Qt::AlignCenter);
  lbl->setAutoFillBackground(true);
  //QPalette palette;
  //palette.setColor(label->backgroundRole(), c);
  //lbl->setPalette(palette);
  lbl->setBackgroundRole(QPalette::Dark);
  layout->addWidget(lbl); 
  
  
  for(int i = 0; i < _channels; ++i)
  {
    QCheckBox* cb = new QCheckBox(lw);
    layout->addWidget(cb); 
  }

  return lw;
}

} // namespace MusEGui
