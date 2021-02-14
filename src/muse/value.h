//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: value.h,v 1.1.1.1 2003/10/27 18:51:53 wschweer Exp $
//
//  (C) Copyright 2000 Werner Schweer (ws@seh.de)
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

#ifndef __VALUE_H__
#define __VALUE_H__

#include <QObject>

namespace MusECore {

class Xml;

//---------------------------------------------------------
//   IValue
//---------------------------------------------------------

class IValue : public QObject {
      Q_OBJECT
    
      int val;

   signals:
      void valueChanged(int);

   public slots:
      void setValue(int v);

   public:
      IValue(QObject* parent=0, const char* name=0);
      int value() const    { return val; }
      void save(int level, Xml& xml);
      };

//---------------------------------------------------------
//   BValue
//---------------------------------------------------------

class BValue : public QObject {
      Q_OBJECT
    
      bool val;

      

   signals:
      void valueChanged(bool);
      void valueChanged(int);

   public slots:
      void setValue(bool v);
      void setValue(int v) { setValue(bool(v)); }

   public:
      BValue(QObject* parent=0, const char* name=0);
      bool value() const    { return val; }
      void save(int level, Xml& xml);
      };

} // namespace MusECore

#endif

