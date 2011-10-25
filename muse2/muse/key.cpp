//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: key.cpp,v 1.1.1.1 2003/10/27 18:51:22 wschweer Exp $
//
//  (C) Copyright 1999/2000 Werner Schweer (ws@seh.de)
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

#include <stdio.h>
#include "key.h"
#include "globals.h"

namespace MusECore {

int NKey::offsets[14] = {
      0, 7, 14, -7,
      -(12),
      -19, -26, -10, -14, -2, -4, -6, -8, 0
      };

int NKey::width() const
      {
      return 25;
      }

//---------------------------------------------------------
//   Scale::width
//---------------------------------------------------------

int Scale::width() const
      {
      int i = val;
      if (i < 0)
           i = -i;
      return i * 7;
      }

} // namespace MusECore
