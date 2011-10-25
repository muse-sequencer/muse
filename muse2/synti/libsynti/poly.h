//=========================================================
//  MusE
//  Linux Music Editor
//    software synthesizer helper library
//    $Id: poly.h,v 1.2 2004/04/15 13:46:18 wschweer Exp $
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

#ifndef __SYNTH_POLY_H__
#define __SYNTH_POLY_H

#include <list>
#include "mess.h"

//---------------------------------------------------------
//   MessPoly
//    implements some functions for monophone
//    synthesizer
//---------------------------------------------------------

class MessPoly : public Mess {
      float volume;
      float expression;

      // cached values:
      float mainLevel;

   protected:
      virtual bool playNote(int channel, int pitch, int velo);
      virtual bool setController(int, int, int);

   public:
      MessPoly() : Mess(1) {}
      virtual ~MessPoly() {}
      };

#endif

