//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: mtc.cpp,v 1.1.1.1 2003/10/27 18:51:48 wschweer Exp $
//
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

#include "mtc.h"
#include <stdio.h>

namespace MusEGlobal {
extern int mtcType;
}

namespace MusECore {

//---------------------------------------------------------
//   MTC::time
//    converts MTC Time to seconds according to
//    global mtcType
//---------------------------------------------------------

double MTC::time(int type) const
      {
      double time = _h * 3600 + _m * 60 + _s;
      double ft = 0.0;
      if(type == -1)
        type = MusEGlobal::mtcType;
      switch (type) {
            case 0:     // 24 frames sec
                  ft = 1.0/24.0;
                  break;
            case 1:     // 25
                  ft = 0.04;
                  break;
            case 2:     // 30 drop frame        TODO
            case 3:     // 30 non drop frame
            default:
                  ft = 1.0/30.0;
                  break;
            }
      return time + ft *_f + 0.01 * ft * _sf;
      }

//---------------------------------------------------------
//   MTC
//---------------------------------------------------------

MTC::MTC(double t, int type)
      {
      _h  = int(t/3600);
      t -= _h * 3600;
      _m  = int(t/60);
      t -= _m * 60;
      _s  = int(t);
      t -= _s;
      double ft = 1.0/24.0;
      if(type == -1)
        type = MusEGlobal::mtcType;
      switch (type) {
            case 0:     // 24 frames sec
                  ft = 1.0/24.0;
                  break;
            case 1:     // 25
                  ft = 0.04;
                  break;
            case 2:     // 30 drop frame
            case 3:     // 30 non drop frame
            default:
                  ft = 1.0/30.0;
                  break;
            }
      double frames = t / ft;
      _f  = int(frames);
      frames -= _f;
      _sf = int(frames * 100);
      }

//---------------------------------------------------------
//   incQuarter
//    increment MTC time one quarter frame time
//---------------------------------------------------------

void MTC::incQuarter(int type)
      {
      int frames = 24;
      if(type == -1)
        type = MusEGlobal::mtcType;
      switch (type) {
            case 0:
                  frames = 24;
                  break;
            case 1:
                  frames = 25;
                  break;
            case 2:
            case 3:
            default:
                  frames = 30;
                  break;
            }
      _sf += 25;
      if (_sf >= 100) {
            ++_f;
            _sf -= 100;
            }
      if (_f == frames) {
            ++_s;
            _f = 0;
            }
      if (_s == 60) {
            ++_m;
            _s = 0;
            }
      if (_m == 60) {
            ++_h;
            _m = 0;
            }
      if (_h == 24) {
            _h = 0;
            }
      }

//---------------------------------------------------------
//   print
//---------------------------------------------------------

void MTC::print() const
      {
      printf("%02d:%02d:%02d:%02d:%02d", _h, _m, _s, _f, _sf);
      }


} // namespace MusECore
