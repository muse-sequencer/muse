//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: memory.cpp,v 1.1.1.1.2.2 2009/12/19 23:35:39 spamatica Exp $
//
//  (C) Copyright 2003 Werner Schweer (ws@seh.de)
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

#include "memory.h"

Pool audioRTmemoryPool;
Pool midiRTmemoryPool;

//---------------------------------------------------------
//   Pool
//---------------------------------------------------------

Pool::Pool()
      {
      for (int idx = 0; idx < dimension; ++idx) {
            head[idx]   = 0;
            chunks[idx] = 0;
            grow(idx);  // preallocate
            }
      }

//---------------------------------------------------------
//   ~Pool
//---------------------------------------------------------

Pool::~Pool()
      {
      for (int i = 0; i < dimension; ++i) {
            Chunk* n = chunks[i];
            while (n) {
                  Chunk* p = n;
                  n = n->next;
                  delete p;
                  }
            }
      }

//---------------------------------------------------------
//   grow
//---------------------------------------------------------

void Pool::grow(int idx)
      {
      int esize = (idx+1) * sizeof(unsigned long);

      Chunk* n    = new Chunk;
      n->next     = chunks[idx];
      chunks[idx] = n;

      const int nelem = Chunk::size / esize;
      char* start     = n->mem;
      char* last      = &start[(nelem-1) * esize];

      for (char* p = start; p < last; p += esize)
            reinterpret_cast<Verweis*>(p)->next =
               reinterpret_cast<Verweis*>(p + esize);
      reinterpret_cast<Verweis*>(last)->next = 0;
      head[idx] = reinterpret_cast<Verweis*>(start);
      }


#ifdef TEST
//=========================================================
//    TEST
//=========================================================

struct mops {
      char a, c;
      int b;
      mops(int x) : b(x) {}
      };

typedef std::list<struct mops, RTalloc<struct mops> > List;
typedef List::iterator iList;

//---------------------------------------------------------
//   main
//    2.8 s  normal                     0.7 vector
//    2.5 s  RTalloc
//    1.18    all optimisations (0.97)
//---------------------------------------------------------

int main()
      {
      List l;

      for (int i = 0; i < 10000000; ++i)
            l.push_back(mops(i));
      return 0;
      }
#endif

