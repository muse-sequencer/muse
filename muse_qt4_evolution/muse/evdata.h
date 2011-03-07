//=============================================================================
//  MusE
//  Linux Music Editor
//  $Id:$
//
//  Copyright (C) 2002-2006 by Werner Schweer and others
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License version 2.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//=============================================================================

#ifndef __EVDATA_H__
#define __EVDATA_H__

#include <string.h>

//---------------------------------------------------------
//   EvData
//    variable len event data (sysex, meta etc.)
//---------------------------------------------------------

class EvData {
      int* refCount;

   public:
      unsigned char* data;
      int dataLen;

      EvData()  {
            data     = 0;
            dataLen  = 0;
            refCount = new int(1);
            }
      EvData(const EvData& ed) {
            data     = ed.data;
            dataLen  = ed.dataLen;
            refCount = ed.refCount;
            (*refCount)++;
            }

      EvData& operator=(const EvData& ed) {
            if (data == ed.data)
                  return *this;
            if (--(*refCount) == 0) {
                  delete refCount;
                  delete[] data;
                  }
            data     = ed.data;
            dataLen  = ed.dataLen;
            refCount = ed.refCount;
            (*refCount)++;
            return *this;
            }

      bool operator==(const EvData& ed) const {
	    if(dataLen==ed.dataLen) {
	      return memcmp(data, ed.data, sizeof(unsigned char) * dataLen)==0;
	      }
	    else return false;
            }

      ~EvData() {
            if (--(*refCount) == 0) {
                  delete[] data;
                  delete refCount;
                  }
            }
      void setData(const unsigned char* p, int l) {
            data = new unsigned char[l];
            memcpy(data, p, l);
            dataLen = l;
            }
      };

#endif

