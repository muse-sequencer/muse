//=========================================================
//  MusE
//  Linux Music Editor
//    rasterizer.cpp
//  (C) Copyright 2012 Tim E. Real (terminator356 on users dot sourceforge dot net)
//
//  Based on former rasterizers in midieditor.h
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

#include "pos.h"
#include "rasterizer.h"

namespace MusECore {


Rasterizer::Rasterizer(unsigned raster, Pos::TType time_type, bool formatted) 
           : _raster(raster), _timeType(time_type), _formatted(formatted)
{

}

Pos Rasterizer::rasterStep(Pos pos) const 
{ 
  pos.setType(_timeType);
  switch(_timeType)
  {
    case Pos::FRAMES:
// REMOVE Tim.      
//       {
//         unsigned f = pos.frame();
//         if(_raster == 0)
//           return f;
//         unsigned r = f % _raster;
//         if(r >= _raster / 2)
//           return f - r + _raster;
//         else
//           return f - r;
//       }
      return Pos(_raster, _timeType);
      break;
    case Pos::TICKS:
      return Pos(AL::sigmap.rasterStep(pos.tick(), _raster), _timeType); 
      break;
  }  
  
  return pos;
}

Pos Rasterizer::rasterVal(Pos pos) const 
{ 
  pos.setType(_timeType);
  switch(_timeType)
  {
    case Pos::FRAMES:
      {
        if(_raster == 0 || _raster == 1)
          return pos;
        // FIXME: Probably incorrect. Fix proper snapping like sigmap.  // REMOVE Tim.
        unsigned f = pos.frame();
        unsigned r = f % _raster;
        if(r >= _raster / 2)
          return Pos(f - r + _raster, _timeType);
        else
          return Pos(f - r, _timeType);
      }
      break;
    case Pos::TICKS:
      return Pos(AL::sigmap.raster(pos.tick(), _raster), _timeType); 
      break;
  }  
  
  return pos;
}

Pos Rasterizer::rasterVal1(Pos pos) const
{ 
  pos.setType(_timeType);
  switch(_timeType)
  {
    case Pos::FRAMES:
      {
        if(_raster == 0 || _raster == 1)
          return pos;
        // FIXME: Probably incorrect. Fix proper snapping like sigmap.  // REMOVE Tim.
        unsigned f = pos.frame();
        unsigned r = f % _raster;
        if(r >= _raster / 2)
          return Pos(f - r + _raster, _timeType);
        else
          return Pos(f - r, _timeType);
      }
      break;
    case Pos::TICKS:
      return Pos(AL::sigmap.raster1(pos.tick(), _raster), _timeType);
      break;
  }  
  
  return pos;
}

Pos Rasterizer::rasterVal2(Pos pos) const
{ 
  pos.setType(_timeType);
  switch(_timeType)
  {
    case Pos::FRAMES:
      {
        if(_raster == 0 || _raster == 1)
          return pos;
        // FIXME: Probably incorrect. Fix proper snapping like sigmap.  // REMOVE Tim.
        unsigned f = pos.frame();
        unsigned r = f % _raster;
        if(r >= _raster / 2)
          return Pos(f - r + _raster, _timeType);
        else
          return Pos(f - r, _timeType);
      }
      break;
    case Pos::TICKS:
      return Pos(AL::sigmap.raster2(pos.tick(), _raster), _timeType); 
      break;
  }  
  
  return pos;
}

Pos Rasterizer::rasterSnapDown(Pos pos) const
{
  pos.setType(_timeType);
  switch(_timeType)
  {
    case Pos::FRAMES:
      {
        if(_raster == 0 || _raster == 1)
          return pos;
        // FIXME: Probably incorrect. Fix proper snapping like sigmap.  // REMOVE Tim.
        unsigned f = pos.frame();
        unsigned r = f % _raster;
        return Pos(f - r, _timeType);
      }
      break;
    case Pos::TICKS:
      {
        int spos = pos.tick();
        if(spos > 0) 
        {
          spos -= 1;     // Nudge by -1, then snap down with raster1.
          spos = AL::sigmap.raster1(spos, rasterStep(pos));
        }  
        if(spos < 0)
          spos = 0;
        return Pos(spos, _timeType);
      }
      break;
  }  
  
  return pos;
}

Pos Rasterizer::rasterSnapUp(Pos pos) const
{
  pos.setType(_timeType);
  switch(_timeType)
  {
    case Pos::FRAMES:
      {
        if(_raster == 0 || _raster == 1)
          return pos;
        // FIXME: Probably incorrect. Fix proper snapping like sigmap.  // REMOVE Tim.
        unsigned f = pos.frame();
        unsigned r = f % _raster;
        if(r >= _raster / 2)
          return Pos(f - r + _raster, _timeType);
        else
          return Pos(f - r, _timeType);
      }
      break;
    case Pos::TICKS:
      // Nudge by +1, then snap up with raster2.
      return Pos(AL::sigmap.raster2(pos.tick() + 1, rasterStep(pos)), _timeType); 
      break;
  }  
  
  return pos;
}
 
Pos Rasterizer::rasterDownNoSnap(Pos pos) const
{
  pos.setType(_timeType);
  Pos rstep = rasterStep(pos);
  if(pos <= rstep)
    return Pos(0, _timeType);
  return pos - rstep;
}
 
Pos Rasterizer::rasterUpNoSnap(Pos pos) const
{
  pos.setType(_timeType);
  return pos + rasterStep(pos);
}


