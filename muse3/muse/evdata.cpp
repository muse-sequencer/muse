//=========================================================
//  MusE
//  Linux Music Editor
//
//  sysex_processor.cpp
//  (C) Copyright 2017 Tim E. Real (terminator356 on users dot sourceforge dot net)
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

#include "evdata.h"
#include "midi.h"

#include <stdio.h>
#include <string.h>

namespace MusECore {

//---------------------------------------------------------
//   SysExInputProcessor
//    Special processing of system exclusive chunks.
//---------------------------------------------------------

SysExInputProcessor::State SysExInputProcessor::processInput(EvData* dst, const unsigned char* src, size_t len, size_t frame)
{
  if(!src || len == 0)
    return _state;
  
  switch(_state)
  {
    case Clear:
    case Finished:
      if(*src == ME_SYSEX)
      {
        // Reset or clear the queue (prefer simple reset but memory may grow - clear later).
        _q.reset();
        //_q.clear();

        // Mark the starting frame as the given frame.
        _startFrame = frame;
        
        // Is this block of data a single chunk?
        if((*(src + len - 1) == ME_SYSEX_END))
        {
          // Aside from the start and end bytes, is there any useful data?
          if(len >= 3)
          {
            // It is a single chunk sysex - no need to queue, set the EvData directly.
            _state = Finished;
            dst->setData(src + 1, len - 2);
          }
          else
            // It's a useless chunk, no point in storing it.
            _state = Clear;
        }
        else
        {
          _state = Filling;
          // It's one chunk of others, queue it. Don't include the start byte - 
          //  our EvData is designed to strip the start/end bytes out.
          if(len >= 2)
            _q.add(src + 1, len - 1);
        }
      }
      else
      {
        _state = Clear;
        fprintf(stderr, "SysExInputProcessor: State is Clear or Finished:%d but chunk start is not ME_SYSEX\n", _state);
      }
    break;

    case Filling:
      if(*src == ME_SYSEX)
      {
        fprintf(stderr, "SysExInputProcessor: State is Filling but chunk start is ME_SYSEX. Finishing sysex.\n");
        // Finish what we got.
        _state = Finished;
        dst->setData(this);
      }
      else if((*(src + len - 1) == ME_SYSEX_END))
      {
        // Finish what we got. Don't include the end byte - 
        //  our EvData is designed to strip the start/end bytes out.
        if(len >= 2)
          _q.add(src, len - 1);
        _state = Finished;
        dst->setData(this);
      }
      else
      {
        // Add what we got and continue in Filling state.
        _q.add(src, len);
      }
    break;
  }
  
  return _state;
}


//---------------------------------------------------------
//   EvData
//    variable len event data (sysex, meta etc.)
//---------------------------------------------------------

void EvData::setData(const unsigned char* p, int l) 
{
// REMOVE Tim. autoconnect. Removed. Moved below.
//             if(data)
//               delete[] data;
//             data = 0;  
      
// REMOVE Tim. autoconnect. Added.
      // Setting the data destroys any reference. Dereference now.
      // The data may still be shared. Destroy it only if no more references.
      if (refCount && (--(*refCount) == 0)) 
      {
        delete refCount;
        refCount = 0;
        
        if(data)
          delete[] data;
      }
      // Clear the data variable.
      data = 0;  
        
      if(l > 0) 
      {
        data = new unsigned char[l];
        memcpy(data, p, l);
        
// REMOVE Tim. autoconnect. Added.
        // Setting the data destroys any reference. Create a new reference now.
        refCount = new int(1);
      }
// REMOVE Tim. autoconnect. Added.
//             else if(refCount)
//             {  
//               delete refCount;
//               refCount = 0;
//             } 
      
      dataLen = l;
}
            
void EvData::setData(const SysExInputProcessor* q) 
{
      // Let's not risk unterminated data: Accept a queue with a Finished state only.
      if(q->state() != SysExInputProcessor::Finished)
        return;
      // Setting the data destroys any reference. Dereference now.
      // The data may still be shared. Destroy it only if no more references.
      if (refCount && (--(*refCount) == 0)) 
      {
        delete refCount;
        refCount = 0;
        
        if(data)
          delete[] data;
      }
      // Clear the data variable.
      data = 0;  
        
      const size_t l = q->size();
      if(l > 0) 
      {
        // Create a contiguous memory block to hold the data.
        data = new unsigned char[l];
        // Copy the non-contiguous chunks of data to the contiguous data.
        q->copy(data, l);
        // Setting the data destroys any reference. Create a new reference now.
        refCount = new int(1);
      }
      dataLen = l;
}

} // namespace MusECore

