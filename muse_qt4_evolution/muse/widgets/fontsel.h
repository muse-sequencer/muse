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

#ifndef __FONTSEL_H__
#define __FONTSEL_H__

#include "song.h"

class QFont;
class QSpinBox;
class QToolButton;
class QComboBox;

//---------------------------------------------------------
//   FontSel
//---------------------------------------------------------

class FontSel : public QWidget {
      QFont _font;
      QSpinBox* s1;
      QToolButton* fcb1;
      QToolButton* fcb2;
      QToolButton* fcb3;
      QComboBox* cb;

      Q_OBJECT

      void setFont();

   private slots:
      void fontSelect();

   public:
      FontSel(QWidget* parent, const QFont&, const QString&);
      const QFont& font();
      };


#endif

