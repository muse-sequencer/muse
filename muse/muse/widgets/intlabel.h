//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: intlabel.h,v 1.2 2004/09/14 18:17:47 wschweer Exp $
//  (C) Copyright 1999 Werner Schweer (ws@seh.de)
//=========================================================

#ifndef __INTLABEL_H__
#define __INTLABEL_H__

#include "nentry.h"

class QString;

//---------------------------------------------------------
//   IntLabel
//---------------------------------------------------------

class IntLabel : public Nentry {
      int min, max, off;
      QString suffix;
      QString specialValue;
      Q_OBJECT

      void init();

      virtual bool setSValue(const QString&);
      virtual bool setString(int val, bool editable = false);
      virtual void incValue(int);
      virtual void decValue(int);

   signals:
      void valueChanged(int);

   public:
      IntLabel(int, int, int, QWidget*, int _off = MAXINT,
         const QString& = QString(""), int lpos = 0);
      void setOff(int v);
      void setSuffix(const QString& s) { suffix = s; }
      void setSpecialValueText(const QString& s);
      };

#endif
