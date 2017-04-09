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
class QIcon;
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


//---------------------------------------------------------
//   IconButton
//---------------------------------------------------------

class IconButton : public QWidget
{
      Q_OBJECT

      Q_PROPERTY(bool checkable READ isCheckable WRITE setCheckable)
      Q_PROPERTY(bool checked READ isChecked WRITE setChecked)
      //Q_PROPERTY(QPixmap* offPixmap READ offPixmap WRITE setOffPixmap)
      //Q_PROPERTY(QPixmap* onPixmap READ onPixmap WRITE setOnPixmap)
      Q_PROPERTY(int margin READ margin WRITE setMargin)

   private:
     QIcon* _onIcon;
     QIcon* _offIcon;
     QIcon* _onIconB;
     QIcon* _offIconB;
     bool _hasFixedIconSize;
     bool _drawFlat;
     QString _text;
     int _margin;

     QSize _iconSize;
     bool _checkable;
     bool _checked;
     bool _iconSetB;
     bool _blinkPhase;

  protected:
    virtual void paintEvent(QPaintEvent* );
    virtual void mousePressEvent(QMouseEvent* );
    virtual void mouseReleaseEvent(QMouseEvent* );
    virtual void mouseMoveEvent(QMouseEvent* );
    virtual void contextMenuEvent(QContextMenuEvent*);

   signals:
     void clicked(bool checked = false);
     void toggled(bool checked = false);
     void pressed();

   public:
      IconButton(QWidget* parent = 0, const char* name = 0);
      IconButton(QIcon* on_icon, QIcon* off_icon, QIcon* on_iconB, QIcon* off_iconB,
                 bool hasFixedIconSize = true, bool drawFlat = false,
                 const QString& text = QString(), int margin = 0, QWidget* parent = 0, const char* name = 0);
      virtual QSize minimumSizeHint () const;
      virtual bool margin() const { return _margin; }
      virtual void setMargin(int v);
      virtual bool isChecked() const { return _checked; }
      virtual void setChecked(bool);
      virtual bool isDown() const { return _checked; }
      virtual void setDown(bool);
      virtual bool isCheckable() const { return _checkable; }
      virtual void setCheckable(bool);
      virtual QIcon* offIcon() const { return _offIcon; }
      virtual void setOffIcon(QIcon*);
      virtual QIcon* onIcon() const { return _onIcon; }
      virtual void setOnIcon(QIcon*);
      virtual QIcon* offIconB() const { return _offIconB; }
      virtual void setOffIconB(QIcon*);
      virtual QIcon* onIconB() const { return _onIconB; }
      virtual void setOnIconB(QIcon*);
      virtual bool iconSetB() const { return _iconSetB; }
      virtual void setIconSetB(bool v);

      QString text() const { return _text; }
      void setText(QString txt);

      bool hasFixedIconSize() const { return _hasFixedIconSize; }
      void setHasFixedIconSize(bool v);

      QSize iconSize() const { return _iconSize; }
      void setIconSize(const QSize sz);

      bool drawFlat() const { return _drawFlat; }
      void setDrawFlat(bool v);

      // If _hasFixedIconSize is true, this relies on iconSize(). Be sure to set iconSize to the desired value.
      virtual QSize sizeHint() const;

      bool blinkPhase() const { return _blinkPhase; }
      void setBlinkPhase(bool v);
};


} // MusEGui  
#endif  // __PIXMAP_BUTTON_H__