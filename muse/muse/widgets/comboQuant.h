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

#ifndef __COMBOQUANT_H__
#define __COMBOQUANT_H__

//---------------------------------------------------------
//   ComboQuant
//---------------------------------------------------------

class ComboQuant : public QComboBox {
      Q_OBJECT

   private slots:
      void activated(int);

   signals:
      void valueChanged(int);

   public:
      ComboQuant(QWidget* parent = 0);
      void setValue(int val);
      };

#endif

