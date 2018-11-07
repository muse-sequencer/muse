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
#include <string.h>

// NOTE: Keep this code in case we need a dimensioned pool!
#if 0
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

#endif

//---------------------------------------------------------
//   MemoryQueue
//---------------------------------------------------------

MemoryQueue::MemoryQueue()
{
  _endChunk = 0;
  _curSize = 0;
  _curOffest = 0;
  
  // Preallocate.
  grow();
  // Remember the very first chunk.
  _startChunk = _endChunk;
  // Start writing from the first chunk.
  _curWriteChunk = _startChunk;
}

//---------------------------------------------------------
//   ~MemoryQueue
//---------------------------------------------------------

MemoryQueue::~MemoryQueue()
{
  // Delete all chunks.
  Chunk* n = _startChunk;
  while(n) 
  {
    Chunk* p = n;
    n = n->_next;
    delete p;
  }
}

//---------------------------------------------------------
//   clear
//---------------------------------------------------------

void MemoryQueue::clear()
{
  // Delete all chunks except the first one, keep it around. That avoids an initial grow.
  if(_startChunk)
  {
    Chunk* n = _startChunk->_next;
    while(n) 
    {
      Chunk* p = n;
      n = n->_next;
      delete p;
    }
  }
  _endChunk = _startChunk;
  // Reset the writer.
  reset();
}

//---------------------------------------------------------
//   reset
//---------------------------------------------------------

void MemoryQueue::reset()
{
  // Start writing from the first chunk.
  _curWriteChunk = _startChunk;
  _curSize = 0;
  _curOffest = 0;
}

//---------------------------------------------------------
//   grow
//---------------------------------------------------------

void MemoryQueue::grow()
{
  Chunk* n = new Chunk();
  //Chunk* n = new Chunk(_chunkSize);
  n->_next = 0;
  if(_endChunk)
    _endChunk->_next = n;
  _endChunk = n;
}
      
//---------------------------------------------------------
//   add
//   Return true if successful.
//---------------------------------------------------------

bool MemoryQueue::add(const unsigned char* src, size_t len)
{
  if(!src || len == 0 || !_curWriteChunk)
    return false;
  const unsigned char* pp = src;
  size_t remain = len;
  size_t bytes; 
  while(true)
  {
    bytes = Chunk::ChunkSize - _curOffest;
    if(remain < bytes)
      bytes = remain;
    memcpy(_curWriteChunk->_mem + _curOffest, pp, bytes);
    _curSize += bytes;
    _curOffest += bytes;
    if(_curOffest == Chunk::ChunkSize)
    {
      _curOffest = 0;
      // Does the next chunk exist?
      if(_endChunk->_next)
      {
        // Advance the current write chunk to the next existing chunk.
        _curWriteChunk = _endChunk->_next;
      }
      else
      {
        // Create a new chunk.
        grow();
        // The _endChunk changed. Advance the current write chunk to it.
        _curWriteChunk = _endChunk;
      }
    }
    remain -= bytes;
    // No more remaining? Done.
    if(remain == 0)
      break;
    // Advance the source read pointer.
    pp += bytes;
  }
  return true;
}

//---------------------------------------------------------
//   copy
//   Return true if successful.
//---------------------------------------------------------

// Return true if successful.
size_t MemoryQueue::copy(unsigned char* dst, size_t len) const
{
  if(!dst || len == 0 || _curSize == 0 || !_startChunk)
    return 0;
  // Limit number of requested bytes to actual available size.
  if(len > _curSize)
    len = _curSize;
  unsigned char* pp = dst;
  size_t remain = len;
  size_t bytes; 
  // Start reading at the very first chunk.
  Chunk* read_chunk = _startChunk;
  while(true)
  {
    bytes = Chunk::ChunkSize;
    if(remain < bytes)
      bytes = remain;
    memcpy(pp, read_chunk->_mem, bytes);
    remain -= bytes;
    // No more remaining? Done.
    if(remain == 0)
      break;
    // The next chunk must already exist.
    if(!read_chunk->_next)
      break;
    // Advance the read chunk to the next existing chunk.
    read_chunk = read_chunk->_next;
    // Advance the destination write pointer.
    pp += bytes;
  }
  return len - remain;
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

