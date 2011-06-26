//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: evdata.h,v 1.2.2.2 2008/08/18 00:15:23 terminator356 Exp $
//
//  (C) Copyright 1999-2003 Werner Schweer (ws@seh.de)
//=========================================================

#ifndef __EVDATA_H__
#define __EVDATA_H__

#include <string.h>
// #include <memory.h>

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

      ~EvData() {
            if (--(*refCount) == 0) {
                  delete[] data;
                  delete refCount;
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

#endif

