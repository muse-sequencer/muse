//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: intlabel.h,v 1.1.1.1.2.2 2008/08/18 00:15:26 terminator356 Exp $
//  (C) Copyright 1999 Werner Schweer (ws@seh.de)
//=========================================================

#ifndef __INTLABEL_H__
#define __INTLABEL_H__

#include <limits.h>
#include "nentry.h"

class QString;

//---------------------------------------------------------
//   IntLabel
//---------------------------------------------------------

class IntLabel : public Nentry {
      Q_OBJECT

      int min, max, off;
      QString suffix;
      QString specialValue;

      void init();

      virtual bool setSValue(const QString&);
      virtual bool setString(int val, bool editable = false);
      virtual void incValue(int);
      virtual void decValue(int);

   signals:
      void valueChanged(int);

   public:
      IntLabel(int, int, int, QWidget*, int _off = INT_MAX,
         const QString& = QString(""), int lpos = 0);
      void setOff(int v);
      void setSuffix(const QString& s) { suffix = s; }
      void setSpecialValueText(const QString& s);
      void setRange(int, int);
      };

#endif
