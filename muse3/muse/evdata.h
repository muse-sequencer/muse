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

//#include <string.h>
// REMOVE Tim. autoconnect. Added.
//#include "sysex_processor.h"
#include "memory.h"

namespace MusECore {

class EvData;

//---------------------------------------------------------
//   SysExInputProcessor
//    Special processing of system exclusive chunks.
//---------------------------------------------------------

class SysExInputProcessor
{
  public:
    enum State { Clear = 0, Filling = 1, Finished = 2 };
    
  private:
    MemoryQueue _q;
    State _state;
    size_t _startFrame;

  public:
    SysExInputProcessor() : _state(Clear), _startFrame(0) { }
    // Returns the state of the queue.
    State state() const { return _state; }
    // Returns the frame that the sysex started at.
    size_t startFrame() const { return _startFrame; }
    // Returns the size of the queue.
    size_t size() const { return _q.curSize(); }
    // Clears the queue.
    void clear() { _q.clear(); _state = Clear; }
    // Resets the queue.
    void reset() { _q.reset(); _state = Clear; }
    // Return true if successful.
    State processInput(EvData* dst, const unsigned char* src, size_t len, size_t frame);
    // Copies the queue to a character buffer.
    // Returns number of bytes copied.
    size_t copy(unsigned char* dst, size_t len) const { return _q.copy(dst, len); }
};

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
// REMOVE Tim. autoconnect. Changed.
//             refCount = new int(1);
            refCount = 0;
            }
      EvData(const EvData& ed) {
            data     = ed.data;
            dataLen  = ed.dataLen;
            refCount = ed.refCount;
// REMOVE Tim. autoconnect. Added.
            //if(!refCount)
            //  refCount = new int(1);
            if(refCount)
              
            (*refCount)++;
            }

      EvData& operator=(const EvData& ed) {
            if (data == ed.data)
                  return *this;
// REMOVE Tim. autoconnect. Changed.
//             if (--(*refCount) == 0) {
//                   delete refCount;
//                   if(data)
//                     delete[] data;
//                 }
            if (refCount && (--(*refCount) == 0)) 
            {
              delete refCount;
              if(data)
                delete[] data;
            }
            
            data     = ed.data;
            dataLen  = ed.dataLen;
            refCount = ed.refCount;
// REMOVE Tim. autoconnect. Added.
            if(refCount)
              
            (*refCount)++;
            return *this;
            }

      ~EvData() {
// REMOVE Tim. autoconnect. Changed.
//             if (--(*refCount) == 0) {
            if (refCount && (--(*refCount) == 0)) {
                  if(data)
                  {  
                    delete[] data;
                    data = 0;
                  }  
// REMOVE Tim. autoconnect. Removed.
//                   if(refCount)
//                   {  
                    delete refCount;
                    refCount = 0;
//                   } 
                }
            }
      void setData(const unsigned char* p, int l);
//       {
// // REMOVE Tim. autoconnect. Removed. Moved below.
// //             if(data)
// //               delete[] data;
// //             data = 0;  
//             
// // REMOVE Tim. autoconnect. Added.
//             // Setting the data destroys any reference. Dereference now.
//             // The data may still be shared. Destroy it only if no more references.
//             if (refCount && (--(*refCount) == 0)) 
//             {
//               delete refCount;
//               refCount = 0;
//               
//               if(data)
//                 delete[] data;
//             }
//             // Clear the data variable.
//             data = 0;  
//               
//             if(l > 0) 
//             {
//               data = new unsigned char[l];
//               memcpy(data, p, l);
//               
// // REMOVE Tim. autoconnect. Added.
//               // Setting the data destroys any reference. Create a new reference now.
//               refCount = new int(1);
//             }
// // REMOVE Tim. autoconnect. Added.
// //             else if(refCount)
// //             {  
// //               delete refCount;
// //               refCount = 0;
// //             } 
//             
//             dataLen = l;
//             }
            
      void setData(const SysExInputProcessor* q);
//       {
//             // Let's not risk unterminated data: Accept a queue with a Finished state only.
//             if(q->state() != SysExInputProcessor::Finished)
//               return;
//             // Setting the data destroys any reference. Dereference now.
//             // The data may still be shared. Destroy it only if no more references.
//             if (refCount && (--(*refCount) == 0)) 
//             {
//               delete refCount;
//               refCount = 0;
//               
//               if(data)
//                 delete[] data;
//             }
//             // Clear the data variable.
//             data = 0;  
//               
//             const size_t l = q->size();
//             if(l > 0) 
//             {
//               // Create a contiguous memory block to hold the data.
//               data = new unsigned char[l];
//               // Copy the non-contiguous chunks of data to the contiguous data.
//               q->copy(data, l);
//               // Setting the data destroys any reference. Create a new reference now.
//               refCount = new int(1);
//             }
//             dataLen = l;
//             }
      };
      
} // namespace MusECore

#endif

