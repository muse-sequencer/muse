//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: vscale.h,v 1.1.1.1.2.1 2008/01/19 13:33:47 wschweer Exp $
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

#ifndef __VSCALE_H__
#define __VSCALE_H__

#include <QWidget>

class QPaintEvent;

namespace MusEGui {

//---------------------------------------------------------
//   VScale
//---------------------------------------------------------

class VScale : public QWidget {
      Q_OBJECT

      virtual void paintEvent(QPaintEvent*);

   public:
      VScale(QWidget* parent=0) : QWidget(parent) {setFixedWidth(18);}
      };

} // namespace MusEGui

#endif

