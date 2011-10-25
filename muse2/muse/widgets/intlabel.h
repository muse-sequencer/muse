//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: intlabel.h,v 1.1.1.1.2.2 2008/08/18 00:15:26 terminator356 Exp $
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

#ifndef __INTLABEL_H__
#define __INTLABEL_H__

#include <limits.h>
#include "nentry.h"

class QString;

namespace MusEGui {

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

} // namespace MusEGui

#endif
