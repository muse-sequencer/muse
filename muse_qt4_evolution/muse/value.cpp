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

#include "value.h"
#include "al/xml.h"

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

void BValue::save(Xml& xml)
      {
      xml.intTag(objectName().toLatin1().data(), val);
      }

//---------------------------------------------------------
//   save
//---------------------------------------------------------

void IValue::save(Xml& xml)
      {
      xml.intTag(objectName().toLatin1().data(), val);
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

