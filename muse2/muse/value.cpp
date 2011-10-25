//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: value.cpp,v 1.2 2004/02/28 14:58:21 wschweer Exp $
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

#include "value.h"
#include "xml.h"

namespace MusECore {

IValue::IValue(QObject* parent, const char* name)
   : QObject(parent)
      {
      setObjectName(name);
      }
BValue::BValue(QObject* parent, const char* name)
   : QObject(parent)
      {
      setObjectName(name);
      }

//---------------------------------------------------------
//   save
//---------------------------------------------------------

void BValue::save(int level, Xml& xml)
      {
      xml.intTag(level, objectName().toLatin1().constData(), val);
      }

//---------------------------------------------------------
//   save
//---------------------------------------------------------

void IValue::save(int level, Xml& xml)
      {
      xml.intTag(level, objectName().toLatin1().constData(), val);
      }

//---------------------------------------------------------
//   setValue
//---------------------------------------------------------

void BValue::setValue(bool v)
      {
      if (val != v) {
            val = v;
            emit valueChanged(val);
            emit valueChanged(int(val));
            }
      }

void IValue::setValue(int v)
      {
      if (val != v) {
            val = v;
            emit valueChanged(val);
            }
      }

} // namespace MusECore
