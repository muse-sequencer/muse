//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: memory.h,v 1.4.2.3 2009/12/15 22:08:50 spamatica Exp $
//
//  (C) Copyright 2003-2004 Werner Schweer (ws@seh.de)
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

#ifndef __MEMORY_H__
#define __MEMORY_H__

#include <stddef.h>
// REMOVE Tim. autoconnect. Removed.
// #include <stdio.h>
// #include <stdlib.h>
// #include <cstddef>
// #include <map>

// REMOVE Tim. autoconnect. Removed.
// // most of the following code is based on examples
// // from Bjarne Stroustrup: "Die C++ Programmiersprache"
// 
// //---------------------------------------------------------
// //   Pool
// //---------------------------------------------------------
// 
// class Pool {
//       struct Verweis {
//             Verweis* next;
//             };
//       struct Chunk {
// // REMOVE Tim. autoconnect. Changed.
//             // Gives about 160 bytes maximum request @ 8 bytes item size.
// //             enum { size = 4 * 1024 };
//             enum { size = 4 * 2048 };
//             Chunk* next;
//             char mem[size];
//             };
// // REMOVE Tim. autoconnect. Changed.
//       // Gives about 160 bytes maximum request @ 8 bytes item size.
// //       enum { dimension = 21 };
//       // Gives about 300 bytes maximum request @ 8 bytes item size.
//       enum { dimension = 40 };
//       Chunk* chunks[dimension];
//       Verweis* head[dimension];
//       Pool(Pool&);
//       void operator=(Pool&);
//       void grow(int idx);
// 
//    public:
//       Pool();
//       ~Pool();
//       void* alloc(size_t n);
//       void free(void* b, size_t n);
//       };
// 
// //---------------------------------------------------------
// //   alloc
// //---------------------------------------------------------
// 
// inline void* Pool::alloc(size_t n)
//       {
//       if (n == 0)
//             return 0;
//       int idx = ((n + sizeof(unsigned long) - 1) / sizeof(unsigned long)) - 1;
//       if (idx >= dimension) {
//             printf("panic: alloc %zd %d %d\n", n, idx, dimension);
//             exit(-1);
//             }
//       if (head[idx] == 0)
//             grow(idx);
//       Verweis* p = head[idx];
//       head[idx] = p->next;
//       return p;
//       }
// 
// //---------------------------------------------------------
// //   free
// //---------------------------------------------------------
// 
// inline void Pool::free(void* b, size_t n)
//       {
//       if (b == 0 || n == 0)
//             return;
//       int idx = ((n + sizeof(unsigned long) - 1) / sizeof(unsigned long)) - 1;
//       if (idx >= dimension) {
//             printf("panic: free %zd %d %d\n", n, idx, dimension);
//             exit(-1);
//             }
//       Verweis* p = static_cast<Verweis*>(b);
//       p->next = head[idx];
//       head[idx] = p;
//       }
// 
// extern Pool audioRTmemoryPool;
// extern Pool midiRTmemoryPool;
// 
// //---------------------------------------------------------
// //   audioRTalloc
// //---------------------------------------------------------
// 
// template <class T> class audioRTalloc
//       {
//    public:
//       typedef T         value_type;
//       typedef size_t    size_type;
//       typedef ptrdiff_t difference_type;
// 
//       typedef T*        pointer;
//       typedef const T*  const_pointer;
// 
//       typedef T&        reference;
//       typedef const T&  const_reference;
// 
//       pointer address(reference x) const { return &x; }
//       const_pointer address(const_reference x) const { return &x; }
// 
//       audioRTalloc();
//       template <class U> audioRTalloc(const audioRTalloc<U>&) {}
//       ~audioRTalloc() {}
// 
//       pointer allocate(size_type n, void * = 0) {
//             return static_cast<T*>(audioRTmemoryPool.alloc(n * sizeof(T)));
//             }
//       void deallocate(pointer p, size_type n) {
//             audioRTmemoryPool.free(p, n * sizeof(T));
//             }
// 
//       audioRTalloc<T>&  operator=(const audioRTalloc&) { return *this; }
//       void construct(pointer p, const T& val) {
//             new ((T*) p) T(val);
//             }
//       void destroy(pointer p) {
//             p->~T();
//             }
//       size_type max_size() const { return size_t(-1); }
// 
//       template <class U> struct rebind { typedef audioRTalloc<U> other; };
//       template <class U> audioRTalloc& operator=(const audioRTalloc<U>&) { return *this; }
//       };
// 
// template <class T> audioRTalloc<T>::audioRTalloc() {}
// 
// //---------------------------------------------------------
// //   midiRTalloc
// //---------------------------------------------------------
// 
// template <class T> class midiRTalloc
//       {
//    public:
//       typedef T         value_type;
//       typedef size_t    size_type;
//       typedef ptrdiff_t difference_type;
// 
//       typedef T*        pointer;
//       typedef const T*  const_pointer;
// 
//       typedef T&        reference;
//       typedef const T&  const_reference;
// 
//       pointer address(reference x) const { return &x; }
//       const_pointer address(const_reference x) const { return &x; }
// 
//       midiRTalloc();
//       template <class U> midiRTalloc(const midiRTalloc<U>&) {}
//       ~midiRTalloc() {}
// 
//       pointer allocate(size_type n, void * = 0) {
//             return static_cast<T*>(midiRTmemoryPool.alloc(n * sizeof(T)));
//             }
//       void deallocate(pointer p, size_type n) {
//             midiRTmemoryPool.free(p, n * sizeof(T));
//             }
// 
//       midiRTalloc<T>&  operator=(const midiRTalloc&) { return *this; }
//       void construct(pointer p, const T& val) {
//             new ((T*) p) T(val);
//             }
//       void destroy(pointer p) {
//             p->~T();
//             }
//       size_type max_size() const { return size_t(-1); }
// 
//       template <class U> struct rebind { typedef midiRTalloc<U> other; };
//       template <class U> midiRTalloc& operator=(const midiRTalloc<U>&) { return *this; }
//       };
// 
// template <class T> midiRTalloc<T>::midiRTalloc() {}




// REMOVE Tim. autoconnect. Added.

//---------------------------------------------------------
//   MemoryQueue
//   An efficient queue which grows by fixed chunk sizes,
//    for single threads only.
//---------------------------------------------------------

class MemoryQueue {
      struct Chunk
      {
        //size_t _size;
        enum { ChunkSize = 8 * 1024 };
        Chunk* _next;
        char _mem[ChunkSize];
        //char* _mem;
        // TODO: Hm, will this cause a double call to new if new Chunk() is called?
        // Maybe have to go back to static enum...
        //Chunk(size_t size) { _size = size; _mem = new char[_size]; }
      };
      //size_t _chunkSize;
      Chunk* _startChunk;
      Chunk* _endChunk;
      Chunk* _curWriteChunk;
      size_t _curSize;
      size_t _curOffest;
      
      MemoryQueue(MemoryQueue&);
      void operator=(MemoryQueue&);
      void grow();

   public:
      MemoryQueue();
      //MemoryQueue(size_t chunkSize);
      ~MemoryQueue();

      // Static. Returns whether the given length in bytes needs to be chunked.
      static bool chunkable(size_t len) { return len > Chunk::ChunkSize; }
      
      // Returns capacity in bytes.
      //size_t capacity() const { return _chunkSize; }
      // Returns current size in bytes.
      size_t curSize() const { return _curSize; }
      // Deletes all chunks except the first (to avoid a preallocation), and calls reset.
      void clear();
      // Resets the size and current write position, but does not clear.
      // Existing chunks will be used, and new ones will be created (allocated) if required.
      // This saves having to clear (which deletes) before every use, at the expense
      //  of keeping what could be an ever increasing memory block alive. 
      void reset();
      // Return true if successful.
      bool add(const unsigned char* src, size_t len);
      // Copies the queue to a character buffer.
      // Returns number of bytes copied.
      size_t copy(unsigned char* dst, size_t len) const;
      };

#endif

