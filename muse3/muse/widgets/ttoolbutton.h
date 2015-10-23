//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: ttoolbutton.h,v 1.1 2004/02/21 16:53:51 wschweer Exp $
//  (C) Copyright 2004 Werner Schweer (ws@seh.de)
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

#ifndef __TTOOLBUTTON_H__
#define __TTOOLBUTTON_H__

#include <QToolButton>

namespace MusEGui {

//---------------------------------------------------------
//   TransparentToolButton
//---------------------------------------------------------

class TransparentToolButton : public QToolButton {
      Q_OBJECT

      virtual void drawButton(QPainter*);

   public:
      TransparentToolButton(QWidget* parent, const char* name = 0)
         : QToolButton(parent) {setObjectName(name);}
      };

} // namespace MusEGui

#endif

