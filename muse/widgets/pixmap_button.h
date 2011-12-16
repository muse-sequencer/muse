//=========================================================
//  MusE
//  Linux Music Editor
//  pixmap_button.h
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

#ifndef __PIXMAP_BUTTON_H__
#define __PIXMAP_BUTTON_H__

#include <QWidget>

class QMouseEvent;
class QPaintEvent;
class QPixmap;
class QString;

namespace MusEGui {

//---------------------------------------------------------
//   PixmapButton
//---------------------------------------------------------

class PixmapButton : public QWidget 
{ 
      Q_OBJECT
   private:
     QString _text;
     int _margin;
     bool _checkable; 
     bool _checked; 
     QPixmap* _onPixmap;
     QPixmap* _offPixmap;
      
  protected:
    virtual void paintEvent(QPaintEvent* );
    virtual void mousePressEvent(QMouseEvent* );
    
   signals:
     void clicked(bool checked = false);
     
   public:
      PixmapButton(QPixmap* on_pixmap, QPixmap* off_pixmap, int margin, QWidget* parent = 0, const QString& text = QString());
      virtual bool isChecked() const { return _checked; }
      virtual void setChecked(bool);
      virtual bool isCheckable() const { return _checkable; }
      virtual void setCheckable(bool);
};
      

} // MusEGui  
#endif  // __PIXMAP_BUTTON_H__