//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: value.h,v 1.1.1.1 2003/10/27 18:51:53 wschweer Exp $
//
//  (C) Copyright 2000 Werner Schweer (ws@seh.de)
//=========================================================

#ifndef __VALUE_H__
#define __VALUE_H__

#include <QObject>

class Xml;

//---------------------------------------------------------
//   IValue
//---------------------------------------------------------

class IValue : public QObject {
      int val;

      Q_OBJECT

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
      bool val;

      Q_OBJECT

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

#endif

