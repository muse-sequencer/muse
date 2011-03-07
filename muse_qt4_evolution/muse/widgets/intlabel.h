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
