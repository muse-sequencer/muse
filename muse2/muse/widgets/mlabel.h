//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: mlabel.h,v 1.1.1.1 2003/10/27 18:55:03 wschweer Exp $
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

#ifndef __MLABEL_H__
#define __MLABEL_H__

#include <QLabel>

namespace MusEGui {

//---------------------------------------------------------
//   MLabel
//    label widged which sends signal mousePressed
//    on mousePressEvent
//---------------------------------------------------------

class MLabel : public QLabel {

      Q_OBJECT

   protected:
      virtual void mousePressEvent(QMouseEvent*);

   signals:
      void mousePressed();

   public:
      MLabel(const QString& txt, QWidget* parent, const char* name = 0)
         : QLabel(txt, parent) {setObjectName(name);};

      MLabel(QWidget* parent, const char* name = 0)
         : QLabel(parent) {setObjectName(name);};
      };

} // namespace MusEGui

#endif

