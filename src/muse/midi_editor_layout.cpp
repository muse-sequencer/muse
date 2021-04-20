//=========================================================
//  MusE
//  Linux Music Editor
//    midi_editor_layout.cpp
//  (C) Copyright 2020 Tim E. Real (terminator356 on sourceforge)
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

#include "midi_editor_layout.h"

#include <QSpacerItem>
//#include <QWidgetItem>

namespace MusEGui {

//---------------------------------------------------------
//   MidiEditorCanvasLayout
//---------------------------------------------------------

void MidiEditorCanvasLayout::setGeometry(const QRect &rect) 
{ 
  QGridLayout::setGeometry(rect);
  // Tell the hbox to update as well, as if it was part of this layout.
  if(_hBox)
  {
    //_hBox->activate();
    _hBox->update();
  }
}

//---------------------------------------------------------
//   MidiEditorHScrollLayout
//---------------------------------------------------------

MidiEditorHScrollLayout::MidiEditorHScrollLayout(QWidget *parent, 
                      QWidget* button1, 
                      QWidget* button2,
                      QWidget* sb,
                      QWidget* corner,
                      QWidget* editor) 
  : QHBoxLayout(parent),
    _button1(button1),
    _button2(button2),
    _sb(sb), 
    _corner(corner),
    _editor(editor),
    _button1Li(nullptr),
    _button2Li(nullptr),
    _cornerLi(nullptr)
{ 
  _spacerLi = new QSpacerItem(0, 0);
  if(_button1)
    _button1Li = new QWidgetItem(_button1);
  if(_button2)
    _button2Li = new QWidgetItem(_button2);
  _sbLi = new QWidgetItem(_sb);
  if(_corner)
    _cornerLi = new QWidgetItem(_corner);
  
  addItem(_spacerLi);
  if(_button1Li)
    addItem(_button1Li);
  if(_button2Li)
    addItem(_button2Li);
  addItem(_sbLi);
  if(_cornerLi)
  {
    addItem(_cornerLi);
    setAlignment(_corner, Qt::AlignBottom|Qt::AlignRight);
  }
};

void MidiEditorHScrollLayout::setGeometry(const QRect &rect) 
{ 
  if(!_editor)
  {
    QHBoxLayout::setGeometry(rect);
    return;
  }

//   const int ti_w = _button1Li->sizeHint().width() + spacing() + 
//                    (_button2Li ? (_button2Li->sizeHint().width() + spacing()) : 0);
                   
  // The buttons use a fixed width! sizeHint and minimumSizeHint are no help here!
  const int ti_w = (_button1 ? (_button1->width() + spacing()) : 0) + 
                   (_button2 ? (_button2->width() + spacing()) : 0);

  const int corner_w = (_corner ? (_corner->sizeHint().width() + spacing()) : 0);
  
  int x = _editor->x();
  if(x < ti_w)
    x = ti_w;
  
//   int b2x = x - (_button2 ? (_button2->sizeHint().width() + spacing()) : 0);
  int b2x = x - (_button2 ? (_button2->width() + spacing()) : 0);
  if(b2x < 0)
    b2x = 0;
  if(b2x > rect.width() - (_sb->minimumSizeHint().width() + corner_w))
    b2x = rect.width() - (_sb->minimumSizeHint().width() + corner_w);
  
  //fprintf(stderr, "spacing:%d x:%d button minimumSizeHint width:%d button sizeHint width:%d\n",
  //        spacing(), x, _button1->minimumSizeHint().width(), _button1->sizeHint().width());
  
  //int b1x = b2x - (_button1 ? (_button1->sizeHint().width() + spacing()) : 0);
  int b1x = b2x - (_button1 ? (_button1->width() + spacing()) : 0);
  if(b1x < 0)
    b1x = 0;
  if(b1x > rect.width() - (_sb->minimumSizeHint().width() + corner_w))
    b1x = rect.width() - (_sb->minimumSizeHint().width() + corner_w);
  
  if(_button1Li)
    _button1Li->setGeometry(
      QRect(
        b1x,
        rect.y(),
        _button1->width() + spacing(),
        rect.height()
      ));
  
  if(_button2Li)
    _button2Li->setGeometry(
      QRect(
        b2x,
        rect.y(), 
        _button2->width() + spacing(), 
        rect.height()
      ));

  if(_editor->width() > 0)
  {
    _sb->setVisible(true);
    
    int new_w = rect.width() - x - corner_w;
    
    if(new_w < _sb->minimumSizeHint().width() + corner_w)
    {
      new_w = _sb->minimumSizeHint().width() + corner_w;
      x = rect.width() - new_w;
    } 
    
    QRect r(x, rect.y(), new_w, rect.height());
    _sbLi->setGeometry(r);
    _spacerLi->setGeometry(QRect(0, rect.y(), b1x, rect.height()));
  }
  else
  {
    _sb->setVisible(false);
    _spacerLi->setGeometry(QRect(0, rect.y(), b1x, rect.height()));
  }

  if(_cornerLi)
    _cornerLi->setGeometry(
      QRect(
        rect.width() - _corner->sizeHint().width(),
        rect.y(),
        _corner->sizeHint().width(),
        rect.height()
      ));

}


} // namespace MusEGui

