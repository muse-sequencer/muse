//=========================================================
//  MusE
//  Linux Music Editor
//    midi_editor_layout.h
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

#ifndef __MIDI_EDITOR_LAYOUT_H__
#define __MIDI_EDITOR_LAYOUT_H__

#include <QWidget>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QAbstractButton>
#include <QRect>

namespace MusEGui {

//---------------------------------------------------------
//   MidiEditorCanvasLayout
//   For laying out a canvas as a last splitter widget and
//    automatically adjusting the width of its corresponding
//    horizontal scrollbar which is in another layout.
//---------------------------------------------------------
      
class MidiEditorCanvasLayout : public QGridLayout
      {
      Q_OBJECT
      QHBoxLayout* _hBox;
      
    public:
      MidiEditorCanvasLayout(QWidget *parent, QHBoxLayout* hBox = nullptr) : QGridLayout(parent), _hBox(hBox) { };
      QHBoxLayout* hBox() { return _hBox; }
      void setHBox(QHBoxLayout* l) { _hBox = l; }
      virtual void setGeometry(const QRect &rect);
      };

//---------------------------------------------------------
//   MidiEditorHScrollLayout
//   For laying out the bottom buttons and hscroll in the arranger.
//---------------------------------------------------------
      
class MidiEditorHScrollLayout : public QHBoxLayout
      {
      Q_OBJECT
      QWidget* _button1;
      QWidget* _button2;
      QWidget* _sb;
      QWidget* _corner;
      
      // This is not actually in the layout, but used anyway.
      QWidget* _editor;
      
      QWidgetItem* _button1Li;
      QWidgetItem* _button2Li;
      QSpacerItem* _spacerLi;
      QWidgetItem* _sbLi;
      QWidgetItem* _cornerLi;
      
    public:
      MidiEditorHScrollLayout(QWidget *parent, 
                            QWidget* button1, 
                            QWidget* button2,
                            QWidget* sb,
                            QWidget* corner = nullptr,
                            QWidget* editor = nullptr);
      QWidget* editor() { return _editor; }
      void setEditor(QWidget* w) { _editor = w; }
      virtual void setGeometry(const QRect &rect);
      };

}

#endif


