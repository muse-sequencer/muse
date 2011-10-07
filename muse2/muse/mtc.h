//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: mtc.h,v 1.1.1.1 2003/10/27 18:51:25 wschweer Exp $
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

#ifndef __MTC_H__
#define __MTC_H__

namespace MusECore {

//---------------------------------------------------------
//   MTC
//---------------------------------------------------------

class MTC {
      unsigned char _h, _m, _s, _f, _sf;

   public:
      MTC(int h, int m, int s, int f, int sf=0) {
            _h  = h;
            _m  = m;
            _s  = s;
            _f  = f;
            _sf = sf;
            }
      MTC() {
            _h = _m = _s = _f = _sf = 0;
            }
      MTC(double, int type = -1);
      void set(int h, int m, int s, int f, int sf=0) {
            _h  = h;
            _m  = m;
            _s  = s;
            _f  = f;
            _sf = sf;
            }
      void incQuarter(int type = -1);
      void setH(int val)  { _h = val; }
      void setM(int val)  { _m = val; }
      void setS(int val)  { _s = val; }
      void setF(int val)  { _f = val; }
      void setSf(int val) { _sf = val; }

      int h() const  { return _h; }
      int m() const  { return _m; }
      int s() const  { return _s; }
      int f() const  { return _f; }
      int sf() const { return _sf; }
      double time(int type = -1) const;
      void print() const;
      };

} // namespace MusECore

#endif

