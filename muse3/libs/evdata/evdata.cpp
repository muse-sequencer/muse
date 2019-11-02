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
#include "sysex_helper.h"

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
          // Reset or clear the queue (prefer simple reset but memory may grow - clear later).
          //_q.reset();
          _q.clear();

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
        // Reset or clear the queue (prefer simple reset but memory may grow - clear later).
        //_q.reset();
        _q.clear();
      }
      else if((*(src + len - 1) == ME_SYSEX_END))
      {
        // Finish what we got. Don't include the end byte - 
        //  our EvData is designed to strip the start/end bytes out.
        if(len >= 2)
          _q.add(src, len - 1);
        _state = Finished;
        dst->setData(this);
        // Reset or clear the queue (prefer simple reset but memory may grow - clear later).
        //_q.reset();
        _q.clear();
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
//   SysExOutputProcessor
//    Special processing of system exclusive chunks.
//---------------------------------------------------------

size_t SysExOutputProcessor::dataSize() const
{
  return _evData.dataLen;
}

size_t SysExOutputProcessor::curChunkSize() const
{
  switch(_state)
  {
    case Clear:
    case Finished:
      fprintf(stderr, "SysExOutputProcessor: curChunkSize called while State is not Sending.\n");
      return 0;
    break;

    case Sending:
    {
      // The remaining number of data bytes (minus any start/end byte).
      size_t sz = 0;
      if((int)_curPos < _evData.dataLen)
        sz = _evData.dataLen - _curPos;

      // Are we on the first chunk? Leave room for the start byte.
      if(_curPos == 0)
        ++sz;
      
      // Should there be more chunks? That is, will the data so far -
      //  plus an end byte - not fit into a chunk?
      if(sz > (_chunkSize - 1))
        // Limit the final size.
        sz = _chunkSize;
      else
        // Leave room for the end byte.
        ++sz;
      
      return sz;
    }
    break;
  }
  
  return 0;
}

void SysExOutputProcessor::clear()
{
  // Release any reference to the data.
  _evData = EvData();
  _state = Clear;
  _curPos = 0;
}

void SysExOutputProcessor::reset()
{
  _state = Clear;
  _curPos = 0;
}

SysExOutputProcessor::State SysExOutputProcessor::setEvData(const EvData& src, size_t frame)
{
  if(!src.data || src.dataLen == 0)
    return _state;

  switch(_state)
  {
    case Clear:
    case Finished:
      // Keep a reference to the data so that it doesn't disappear between calls,
      //  so that we can step through the data block.
      _evData = src;
      
      _curPos = 0;
      
      // Mark the starting frame as the given frame.
      _curChunkFrame = frame;
      
      _state = Sending;
    break;

    case Sending:
      fprintf(stderr, "SysExOutputProcessor: processOutput called while State is Sending.\n");
    break;
  }
  
  return _state;
}

bool SysExOutputProcessor::getCurChunk(unsigned char* dst, int sampleRate)
{
  if(!dst)
    return false;
  
  switch(_state)
  {
    case Clear:
    case Finished:
      fprintf(stderr, "SysExOutputProcessor: getCurChunk called while State is not Sending.\n");
      return false;
    break;

    case Sending:
    {
      unsigned char* p = dst;
      bool is_chunk = false;
      
      // The remaining number of data bytes (minus any start/end byte).
      size_t sz = 0;
      if((int)_curPos < _evData.dataLen)
        sz = _evData.dataLen - _curPos;

      // Are we on the first chunk? Leave room for the start byte.
      if(_curPos == 0)
        ++sz;
      
      // Should there be more chunks? That is, will the data so far -
      //  plus an end byte - not fit into a chunk?
      if(sz > (_chunkSize - 1))
      {
        // Limit the final size.
        sz = _chunkSize;
        is_chunk = true;
      }
        
      // Are we on the first chunk?
      if(_curPos == 0)
      {
        // Add the start byte.
        *p++ = ME_SYSEX;
        --sz;
      }
      
      // Besides any start byte, is there any actual data to copy?
      if(sz != 0)
      {
        // Copy the data to the destination.
        memcpy(p, _evData.data + _curPos, sz);
        // Advance the pointer.
        p += sz;
        // Advance the current position.
        _curPos += sz;
      }
      
      // Are there no more chunks to follow?
      if(!is_chunk)
      {
        // Add the end byte.
        *p = ME_SYSEX_END;
        // We are finished.
        _state = Finished;
        // Release any reference to the data.
        _evData = EvData();
      }
      
      // Estimate the number of audio frames it should take (or took) to transmit the current midi chunk.
      // Advance the current chunk frame so that the driver can schedule the next chunk. 
      // Do it even if the state has Finished, so the driver can wait until the last chunk is done
      //  before calling Clear() or Reset() (setting the state to Clear).
      _curChunkFrame += sysexDuration(sz, sampleRate);
    }
    break;
  }
  
  return true;
}

size_t SysExOutputProcessor::stageEvData(const EvData& evData, unsigned int frame)
{
  // Cannot do if already sending.
  if(_state == SysExOutputProcessor::Sending)
    return 0;
  // Set the event data, and proceed only if state changed to Sending.
  if(setEvData(evData, frame) != SysExOutputProcessor::Sending)
    return 0;
  // Return the current (first) chunk size.
  return curChunkSize();
}

//---------------------------------------------------------
//   EvData
//    variable len event data (sysex, meta etc.)
//---------------------------------------------------------

void EvData::setRawData(unsigned char* p, int l) 
{
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
        // Set the data pointer.
        data = p;
        
        // Setting the data destroys any reference. Create a new reference now.
        refCount = new int(1);
      }
      dataLen = l;
}
            
void EvData::setData(const unsigned char* p, int l) 
{
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
        
        // Setting the data destroys any reference. Create a new reference now.
        refCount = new int(1);
      }
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

