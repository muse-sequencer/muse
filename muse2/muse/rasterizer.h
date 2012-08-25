//=========================================================
//  MusE
//  Linux Music Editor
//    rasterizer.h
//  (C) Copyright 2012 Tim E. Real (terminator356 on users dot sourceforge dot net)
//
//  Based on former rasterizers in midieditor.cpp
//  (C) Copyright 1999 Werner Schweer (ws@seh.de)
//   and others.
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

#ifndef __RASTERIZER_H__
#define __RASTERIZER_H__

//#include "type_defs.h"
#include "al/sig.h"
#include "pos.h"
//#include "pos.h"
//#include "cobject.h"
//#include "pos.h"

//#include <set>

namespace MusECore {

class Rasterizer {
      int _raster;            // Current 'snap' setting
      Pos::TType _timeType;   // Current raster type
      bool _formatted;        // Formatted (BBT or MSF etc) or raw.
      
  public:
      Rasterizer(unsigned raster = 0, Pos::TType time_type = Pos::TType::TICKS, bool formatted = true);
    
      int raster() const              { return _raster; }
      void setRaster(int val)         { _raster = val; }
      Pos::TType timeType() const     { return _timeType; }
      void setTimeType(Pos::TType tt) { _timeType = tt; }
      int formatted() const           { return _formatted; }
      void setFormatted(bool val)     { _formatted = val; }

      // REMOVE Tim. When conversion to all Pos is done.
      int rasterStep(unsigned tick) const   { return AL::sigmap.rasterStep(tick, _raster); }  
      unsigned rasterVal(unsigned v)  const { return AL::sigmap.raster(v, _raster);  }
      unsigned rasterVal1(unsigned v) const { return AL::sigmap.raster1(v, _raster); }
      unsigned rasterVal2(unsigned v) const { return AL::sigmap.raster2(v, _raster); }
      
      // Newer versions:
      Pos rasterStep(Pos pos) const;  
      Pos rasterVal(Pos pos)  const; 
      Pos rasterVal1(Pos pos) const;
      Pos rasterVal2(Pos pos) const;
      Pos rasterSnapUp(Pos pos) const;
      Pos rasterSnapDown(Pos pos) const;
      Pos rasterUpNoSnap(Pos pos) const;
      Pos rasterDownNoSnap(Pos pos) const;
};

} // namespace MusECore

#endif
