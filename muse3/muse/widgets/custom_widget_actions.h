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

#include <QWidgetAction>
#include <QList>

class QMouseEvent;
class QPaintEvent;
class QPixmap;

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
      int _channels;
      int _current;
      QPixmap* _onPixmap;
      QPixmap* _offPixmap;
      QList<PixmapButton*> _chan_buttons;
      
   private slots:
     void chanClickMap(int);
     
   public:
      PixmapButtonsWidgetAction(const QString& text, 
                              QPixmap* on_pixmap, QPixmap* off_pixmap, 
                              int channels, int initial, 
                              QWidget* parent = 0);
      
      QWidget* createWidget(QWidget* parent);
      int currentState() const { return _current; }
      void setCurrentState(int state); 
      };

} // namespace MusEGui
#endif  // __CUSTOM_WIDGET_ACTIONS_H__
