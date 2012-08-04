// THIS FILE IS ORPHANED: nothing uses its functions

//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: sig.h,v 1.2 2004/01/11 18:55:34 wschweer Exp $
//
//  (C) Copyright 2000 Werner Schweer (ws@seh.de)
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

#ifndef __SIG_H__
#define __SIG_H__

#include <map>

#ifndef MAX_TICK
#define MAX_TICK (0x7fffffff/100)
#endif

namespace MusECore {

class Xml;

//---------------------------------------------------------
//   Signature Event
//---------------------------------------------------------

struct SigEvent {
      int z, n;            // beat signature
      unsigned tick;       // valid from this position
      int bar;             // precomputed

      int read(Xml&);
      void write(int, Xml&, int) const;

      SigEvent() { }
      SigEvent(int Z, int N, unsigned tk) {
            z = Z;
            n = N;
            tick = tk;
            bar = 0;
            }
      };

//---------------------------------------------------------
//   SigList
//---------------------------------------------------------

typedef std::map<unsigned, SigEvent*, std::less<unsigned> > SIGLIST;
typedef SIGLIST::iterator iSigEvent;
typedef SIGLIST::const_iterator ciSigEvent;
typedef SIGLIST::reverse_iterator riSigEvent;
typedef SIGLIST::const_reverse_iterator criSigEvent;

class SigList : public SIGLIST {
      int ticks_beat(int N) const;
      void normalize();
      int ticksMeasure(int z, int n) const;

   public:
      SigList();
      void clear();
      void add(unsigned tick, int z, int n);
      void del(unsigned tick);

      void read(Xml&);
      void write(int, Xml&) const;
      void dump() const;

      void timesig(unsigned tick, int& z, int& n) const;
      void tickValues(unsigned t, int* bar, int* beat, unsigned* tick) const;
      unsigned bar2tick(int bar, int beat, unsigned tick) const;

      int ticksMeasure(unsigned tick) const;
      int ticksBeat(unsigned tick) const;
      unsigned raster(unsigned tick, int raster) const;
      unsigned raster1(unsigned tick, int raster) const;
      unsigned raster2(unsigned tick, int raster) const;
      int rasterStep(unsigned tick, int raster) const;
      };

} // namespace MusECore

namespace MusEGlobal {
extern MusECore::SigList sigmap;
}

#endif
