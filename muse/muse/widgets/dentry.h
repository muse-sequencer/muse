//=============================================================================
//  MusE
//  Linux Music Editor
//  $Id:$
//
//  Copyright (C) 2002-2006 by Werner Schweer and others
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License version 2.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//=============================================================================

#ifndef __DENTRY_H__
#define __DENTRY_H__

class QTimer;

//---------------------------------------------------------
//   Dentry
//---------------------------------------------------------

class Dentry : public QLineEdit {
      Q_OBJECT

      Q_PROPERTY( int id READ id WRITE setId )
      Q_PROPERTY( double value READ value WRITE setValue )
      Q_PROPERTY( bool frame READ frame WRITE setFrame )

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

   public slots:
      virtual void setValue(double);

   public:
      Dentry(QWidget* parent = 0);
      double value() const { return val; }
      bool frame() const { return drawFrame; }
      void setFrame(bool);
      int id() const    { return _id; }
      void setId(int i) { _id = i; }
      };
#endif
