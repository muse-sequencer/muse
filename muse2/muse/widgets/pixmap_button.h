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
      
      Q_PROPERTY(bool checkable READ isCheckable WRITE setCheckable)
      Q_PROPERTY(bool checked READ isChecked WRITE setChecked)
      //Q_PROPERTY(QPixmap* offPixmap READ offPixmap WRITE setOffPixmap)
      //Q_PROPERTY(QPixmap* onPixmap READ onPixmap WRITE setOnPixmap)
      Q_PROPERTY(int margin READ margin WRITE setMargin)
      
   private:
     QString _text;
     bool _checkable;
     bool _checked;
     int _margin;
     QPixmap* _onPixmap;
     QPixmap* _offPixmap;
      
  protected:
    virtual void paintEvent(QPaintEvent* );
    virtual void mousePressEvent(QMouseEvent* );
    virtual void mouseReleaseEvent(QMouseEvent* );
    void contextMenuEvent(QContextMenuEvent*);
    
   signals:
     void clicked(bool checked = false);
     void toggled(bool checked = false);
     void pressed();
     
   public:
      PixmapButton(QWidget* parent = 0);
      PixmapButton(QPixmap* on_pixmap, QPixmap* off_pixmap, int margin, QWidget* parent = 0, const QString& text = QString());
      virtual QSize minimumSizeHint () const;
      virtual bool margin() const { return _margin; }
      virtual void setMargin(int v); 
      virtual bool isChecked() const { return _checked; }
      virtual void setChecked(bool);
      virtual bool isDown() const { return _checked; }
      virtual void setDown(bool);
      virtual bool isCheckable() const { return _checkable; }
      virtual void setCheckable(bool);
      virtual QPixmap* offPixmap() const { return _offPixmap; }
      virtual void setOffPixmap(QPixmap*);
      virtual QPixmap* onPixmap() const { return _onPixmap; }
      virtual void setOnPixmap(QPixmap*);
};
      

} // MusEGui  
#endif  // __PIXMAP_BUTTON_H__