//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: evdata.h,v 1.2.2.2 2008/08/18 00:15:23 terminator356 Exp $
//
//  (C) Copyright 1999-2003 Werner Schweer (ws@seh.de)
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

#ifndef __EVDATA_H__
#define __EVDATA_H__

#include <string.h>

namespace MusECore {

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
                  if(data)
                    delete[] data;
                }
            data     = ed.data;
            dataLen  = ed.dataLen;
            refCount = ed.refCount;
            (*refCount)++;
            return *this;
            }

      ~EvData() {
            if (--(*refCount) == 0) {
                  if(data)
                  {  
                    delete[] data;
                    data = 0;
                  }  
                  if(refCount)
                  {  
                    delete refCount;
                    refCount = 0;
                  } 
                }
            }
      void setData(const unsigned char* p, int l) {
            if(data)
              delete[] data;                  // p4.0.27
            data = new unsigned char[l];
            memcpy(data, p, l);
            dataLen = l;
            }
      };

} // namespace MusECore

#endif

