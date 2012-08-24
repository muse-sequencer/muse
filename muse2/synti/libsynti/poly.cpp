//=========================================================
//  MusE
//  Linux Music Editor
//    software synthesizer helper library
//    $Id: poly.cpp,v 1.3 2004/06/01 14:25:50 wschweer Exp $
//
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

#include "poly.h"
#include "muse/midictrl.h"

//---------------------------------------------------------
//   playNote
//---------------------------------------------------------

bool MessPoly::playNote(int /*channel*/, int /*pitch*/, int /*velo*/)
      {
      return false;
      }

//---------------------------------------------------------
//   setController
//---------------------------------------------------------

bool MessPoly::setController(int /*channel*/, int num, int /*val*/)
      {
      switch(num) {
            case MusECore::CTRL_VOLUME:
            case MusECore::CTRL_EXPRESSION:
                  break;
            }
      return false;
      }

