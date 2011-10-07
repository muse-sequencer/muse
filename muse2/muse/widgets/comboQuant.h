//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: comboQuant.h,v 1.1.1.1 2003/10/27 18:54:30 wschweer Exp $
//  (C) Copyright 2001 Werner Schweer (ws@seh.de)
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

#ifndef __COMBOQUANT_H__
#define __COMBOQUANT_H__

#include <QComboBox>
//#include <QWidget>
//#include <QTableWidget>

class QWidget;
class QTableWidget;

namespace MusEGui {

//---------------------------------------------------------
//   ComboQuant
//---------------------------------------------------------

class ComboQuant : public QComboBox {
      Q_OBJECT

      QTableWidget* qlist;
       
   private slots:
      void activated(int);

   signals:
      void valueChanged(int);

   public:
      ComboQuant(QWidget* parent = 0);
      void setValue(int val);
      };

}

#endif

