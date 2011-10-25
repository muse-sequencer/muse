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

#ifndef __AWLPITCHLABEL_H__
#define __AWLPITCHLABEL_H__

#include <QLabel>

namespace Awl {

//---------------------------------------------------------
//   PitchLabel
//---------------------------------------------------------

class PitchLabel : public QLabel {
      Q_OBJECT
    
      bool _pitchMode;
      int _value;
      

   protected:
      QSize sizeHint() const;

   public slots:
      void setValue(int);
      void setInt(int);
      void setPitch(int);

   public:
      PitchLabel();
      int value() const { return _value; }
      void setPitchMode(bool val);
      bool pitchMode() const { return _pitchMode; }
      };
}

#endif
