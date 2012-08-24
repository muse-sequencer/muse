//=============================================================================
//  Awl
//  Audio Widget Library
//  $Id:$
//
//  Copyright (C) 1999-2011 by Werner Schweer and others
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License
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
//=============================================================================

#ifndef __AWLPOSLABEL_H__
#define __AWLPOSLABEL_H__

#include "al/pos.h"

namespace Awl {

//---------------------------------------------------------
//   PosLabel
//---------------------------------------------------------

class PosLabel : public QLabel {
      bool _smpte;
      AL::Pos pos;
      Q_OBJECT

      void updateValue();

   protected:
      QSize sizeHint() const;

   public slots:
      void setValue(const AL::Pos&, bool);

   public:
      PosLabel(QWidget* parent = 0);
      AL::Pos value() const { return pos; }

      void setSmpte(bool);
      bool smpte() const { return _smpte; }
      };

}

#endif

