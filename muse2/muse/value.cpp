//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: value.cpp,v 1.2 2004/02/28 14:58:21 wschweer Exp $
//
//  (C) Copyright 2000 Werner Schweer (ws@seh.de)
//=========================================================

#include "value.h"
#include "xml.h"


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

