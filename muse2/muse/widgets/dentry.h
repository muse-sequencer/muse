//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: dentry.h,v 1.1.1.1.2.3 2008/08/18 00:15:26 terminator356 Exp $
//  (C) Copyright 1999 Werner Schweer (ws@seh.de)
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

#ifndef __DENTRY_H__
#define __DENTRY_H__

#include <QLineEdit>

#include "sliderbase.h"

namespace MusEGui {

//---------------------------------------------------------
//   Dentry
//---------------------------------------------------------

class Dentry : public QLineEdit {
      Q_OBJECT

      Q_PROPERTY( int id READ id WRITE setId )
      Q_PROPERTY( double value READ value WRITE setValue )
      Q_PROPERTY( bool frame READ frame WRITE setFrame )

      SliderBase* _slider;
      int button;
      int starty;
      bool drawFrame;
      QTimer* timer;
      double evx;
      int timecount;

      virtual void wheelEvent(QWheelEvent*);
      virtual void mousePressEvent(QMouseEvent*);
      virtual void mouseMoveEvent(QMouseEvent*);
      virtual void mouseDoubleClickEvent(QMouseEvent*);
      virtual void mouseReleaseEvent(QMouseEvent*);
      void contextMenuEvent(QContextMenuEvent*);

   protected:
      int _id;
      double val;

      virtual void incValue(double x) = 0;
      virtual void decValue(double x) = 0;
      virtual bool setString(double) = 0;
      virtual bool setSValue(const QString&) = 0;

   private slots:
      void repeat();

   protected slots:
      void endEdit();

   signals:
      void valueChanged(double, int);
      void doubleClicked(int);
      void ctrlDoubleClicked(int);

   public slots:
      virtual void setValue(double);

   public:
      Dentry(QWidget*, const char* name=0);
      double value() const { return val; }
      bool frame() const { return drawFrame; }
      void setFrame(bool);
      int id() const    { return _id; }
      void setId(int i) { _id = i; }
      SliderBase* slider() const            { return _slider; }
      void setSlider(SliderBase* s)         { _slider = s; }
      };

} // namespace MusEGui

#endif
