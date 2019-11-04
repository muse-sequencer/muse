//=========================================================
//  MusE
//  Linux Music Editor
//
//  lock_free_data_buffer.h
//  (C) Copyright 2019 Tim E. Real (terminator356 on users dot sourceforge dot net)
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

#ifndef __LOCK_FREE_DATA_BUFFER_H__
#define __LOCK_FREE_DATA_BUFFER_H__

#include <cstdint>
#include <cstring>
#include <atomic>

namespace MusECore {

//---------------------------------------------------------
//   LockFreeDataRingBuffer
//   Variable sized data items.
//   No 'split' buffer or extra copying required when reading,
//    read() and peek() return linear addresses.
//   64KBytes maximum capacity.
//---------------------------------------------------------

class LockFreeDataRingBuffer
{
      uint16_t _capacity;
      uint8_t *_fifo;
      std::atomic<uint16_t> _size;
      std::atomic<uint16_t> _wPos;
      std::atomic<uint16_t> _rPos;
      uint16_t _sizeSnapshot;

   public:
      LockFreeDataRingBuffer(uint16_t capacity)
      {
        _capacity = capacity;
        _fifo = new uint8_t[_capacity];
        clear();
      }

      ~LockFreeDataRingBuffer()
      {
        if(_fifo)
          delete[] _fifo;
      }

      void setCapacity(uint16_t capacity)
      {
        if(_fifo)
          delete _fifo;
        _fifo = 0;
        _capacity = capacity;
        _fifo = new uint8_t[_capacity];
      }

      // This is only for the writer.
      // Returns true on success, false on fifo overflow or other error.
      bool put(const void* data, size_t data_size)
      {
        // Nothing to save?
        if(data_size == 0)
          return false;

        // The size of the data size value.
        const uint16_t count_sz = sizeof(uint16_t);

        // Data too big?
        if(data_size >= 65536 - count_sz)
          return false;

        // The total size required.
        const uint16_t total_sz = data_size + count_sz;
        // The current read position.
        // TODO: Hm, watch out for this. No choice but to read it here in put().
        const uint16_t cur_rpos = _rPos.load();
        // The current write position.
        uint16_t cur_wpos = _wPos.load();

        // If the read pointer is after the write pointer, make sure
        //  the write pointer doesn't get ahead of the read pointer.
        if(cur_rpos > cur_wpos)
        {
          if(cur_wpos + total_sz >= cur_rpos)
            return false;
        }
        // Is there space at the end of the buffer?
        else if(cur_wpos + total_sz >= _capacity)
        {
          // No space at end of buffer. Is there space at the start?
          if(cur_rpos < total_sz)
            return false;
          // There is space at the start of the buffer.
          // If there is room, store zero as the current data size value,
          //  meaning 'end of buffer' reader should jump back to the start.
          if(_capacity - cur_wpos >= count_sz)
            *((uint16_t*)&_fifo[cur_wpos]) = 0;
          // Reset the write pointer.
          cur_wpos = 0;
        }

        // Store the data size value.
        *((uint16_t*)&_fifo[cur_wpos]) = data_size;
        // Increment to the start of the data position.
        cur_wpos += count_sz;
        // Store the data.
        ::memcpy(&_fifo[cur_wpos], data, data_size);
        // Increment to the start of the next item position.
        // Don't bother checking and resetting write pos if it passes capacity.
        // That needs to be done safely in the next call to put().
        cur_wpos += data_size;

        // Now safely store the new write position.
        _wPos.store(cur_wpos);
        // Now safely increment the size.
        _size++;

        //fprintf(stderr, "put SIZE:%d\n", _size.load());

        // Success.
        return true;
      }

      // This is only for the reader.
      // Returns true on success, false if nothing to read or other error.
      // NOTE: This is not multi-reader safe. Yet.
      bool get(void* data, size_t* data_size)
      {
        // Nothing to read?
        if(_size.load() == 0)
          return false;

        // The size of the data size value.
        const uint16_t count_sz = sizeof(uint16_t);
        // Safely read the current read position.
        uint16_t cur_rpos = _rPos.load();

        // If there is no room for the data size value, we've reached the end of the buffer,
        //  reader should jump back to the start.
        // Or if the data size value is zero meaning 'end of buffer',
        //  reader should jump back to the start.
        if((_capacity - cur_rpos < count_sz) || (*((uint16_t*)&_fifo[cur_rpos]) == 0))
          cur_rpos = 0;

        // Get the data size value.
        const uint16_t dat_sz = *((uint16_t*)&_fifo[cur_rpos]);
        // Increment to the start of the data position.
        cur_rpos += count_sz;

        // Return the data size.
        *data_size = dat_sz;
        // Is there data? Return it.
        if(dat_sz > 0)
          ::memcpy(data, &_fifo[cur_rpos], dat_sz);
        // Increment to the start of the next item position.
        cur_rpos += dat_sz;
        
        // Now safely store the new read position.
        _rPos.store(cur_rpos);
        // Now safely decrement the size.
        _size--;
        // Success.
        return true;
      }

      // This is only for the reader.
      // Returns true on success, false if nothing to read or other error.
      // NOTE: This is not multi-reader safe. Yet.
      bool peek(void** data, size_t* data_size)
      {
        // Nothing to read?
        if(_size.load() == 0)
          return false;

        // The size of the data size value.
        const uint16_t count_sz = sizeof(uint16_t);
        // Safely read the current read position.
        uint16_t cur_rpos = _rPos.load();

        // If there is no room for the data size value, we've reached the end of the buffer,
        //  reader should jump back to the start.
        // Or if the data size value is zero meaning 'end of buffer',
        //  reader should jump back to the start.
        if((_capacity - cur_rpos < count_sz) || (*((uint16_t*)&_fifo[cur_rpos]) == 0))
          cur_rpos = 0;

        // Get the data size value.
        const uint16_t dat_sz = *((uint16_t*)&_fifo[cur_rpos]);
        // Increment to the start of the data position.
        cur_rpos += count_sz;

        // Return the data size.
        *data_size = dat_sz;
        // Is there data? Return it.
        if(dat_sz > 0)
          *data = &_fifo[cur_rpos];
        
        // Success.
        return true;
      }

      // This is only for the reader.
      // Returns true on success or false if nothing to remove or other error.
      bool remove()
      {
        // Nothing to read?
        if(_size.load() == 0)
          return false;

        // The size of the data size value.
        const uint16_t count_sz = sizeof(uint16_t);
        // Safely read the current read position.
        uint16_t cur_rpos = _rPos.load();

        // If there is no room for the data size value, we've reached the end of the buffer,
        //  reader should jump back to the start.
        // Or if the data size value is zero meaning 'end of buffer',
        //  reader should jump back to the start.
        if((_capacity - cur_rpos < count_sz) || (*((uint16_t*)&_fifo[cur_rpos]) == 0))
          cur_rpos = 0;

        // Get the data size value.
        const uint16_t dat_sz = *((uint16_t*)&_fifo[cur_rpos]);
        // Increment to the start of the next item position.
        cur_rpos += count_sz + dat_sz;

        // Now safely store the new read position.
        _rPos.store(cur_rpos);
        // Now safely decrement the size.
        _size--;

        //fprintf(stderr, "remove SIZE:%d\n", _size.load());

        // Success.
        return true;
      }

      // This is only for the reader.
      // Returns the number of items in the buffer.
      // If NOT requesting the size snapshot, this conveniently stores a snapshot (cached) version 
      //  of the size for consistent behaviour later. If requesting the size snapshot, it does not 
      //  update the snapshot itself.
      unsigned int getSize(bool useSizeSnapshot/* = false*/)
      { 
        const unsigned int sz = useSizeSnapshot ? _sizeSnapshot : _size.load();
        if(!useSizeSnapshot)
          _sizeSnapshot = sz; 
        return sz;
      }
      // This is only for the reader.
      bool isEmpty(bool useSizeSnapshot/* = false*/) const { return useSizeSnapshot ? _sizeSnapshot == 0 : _size.load() == 0; }
      // This is not thread safe, call it only when it is safe to do so.
      void clear() { _size.store(0); _sizeSnapshot = 0; _wPos.store(0); _rPos.store(0); }
      // This is only for the reader.
      // Clear the 'read' side of the ring buffer, which also clears the size.
      // NOTE: A corresponding clearWrite() is not provided because it is dangerous to reset 
      //  the size from the sender side - the receiver might cache the size, briefly. 
      // The sender should only grow the size while the receiver should only shrink it.
      //void clearRead() { _size = 0; _sizeSnapshot = 0; _rIndex = _wIndex; }
      void clearRead() { _size.store(0); _sizeSnapshot = 0; _rPos.store(_wPos); }
};


//---------------------------------------------------------
//   LockFreeMPSCDataRingBuffer
//---------------------------------------------------------

class LockFreeMPSCDataRingBuffer
{
      unsigned int _capacity;
      size_t _itemSize;
      uint8_t* *_fifo;
      std::atomic<unsigned int> _size;
      std::atomic<unsigned int> _wIndex;
      std::atomic<unsigned int> _rIndex;
      unsigned int _capacityMask;
      unsigned int _sizeSnapshot;

      // Rounds to the nearest or equal power of 2.
      // For 0, 1, and 2, always returns 2.
      unsigned int roundCapacity(unsigned int reqCap) const
      {
        unsigned int i;
        for(i = 1; (1U << i) < reqCap; i++);
        return 1U << i;
      }

   public:
      // Start simple with just 2, like a flipping buffer for example.
      LockFreeMPSCDataRingBuffer(size_t data_size, unsigned int capacity = 2)
      {
        // The size of the data size value.
        const int count_sz = sizeof(_itemSize);
        _itemSize = count_sz + data_size;
        _capacity = roundCapacity(capacity);
        _capacityMask = _capacity - 1;
        _fifo = new uint8_t*[_capacity];
        for(unsigned int i = 0; i < _capacity; ++i)
          _fifo[i] = new uint8_t[_itemSize];
        clear();
      }

      ~LockFreeMPSCDataRingBuffer()
      {
        if(_fifo)
        {
          for(unsigned int i = 0; i < _capacity; ++i)
            delete[] _fifo[i];
          delete[] _fifo;
        }
      }

      void setCapacity(size_t data_size, unsigned int capacity = 2)
      {
        if(_fifo)
        {
          for(unsigned int i = 0; i < _capacity; ++i)
            delete[] _fifo[i];
          delete[] _fifo;
        }
        _fifo = 0;
        // The size of the data size value.
        const int count_sz = sizeof(_itemSize);
        _itemSize = count_sz + data_size;
        _capacity = roundCapacity(capacity);
        _capacityMask = _capacity - 1;
        _fifo = new uint8_t*[_capacity];
        for(unsigned int i = 0; i < _capacity; ++i)
          _fifo[i] = new uint8_t[_itemSize];
      }

      // This is only for the writer.
      // Returns true on success, false on fifo overflow or other error.
      bool put(const void* data, size_t data_size)
      {
        // Buffer full? Overflow condition.
        if(_size.load() >= _capacity) 
          return false;
        // The size of the data size value.
        const int count_sz = sizeof(_itemSize);
        // Data too big?
        if(data_size >= _itemSize - count_sz)
          return false;

        // Safely read, then increment, the current write position.
        //std::atomic<unsigned int> pos = _wIndex++;
        unsigned int pos = _wIndex++;
        // Mask the position for a circular effect.
        pos &= _capacityMask;
        // Pointer to the data item.
        uint8_t* p_data = _fifo[pos];
        // Store the data size value.
        *((size_t*)p_data) = data_size;
        // Increment to the start of the data position.
        p_data += count_sz;
        // Store the data.
        std::memcpy(p_data, data, data_size);
        // Now safely increment the size.
        _size++;

        //fprintf(stderr, "put SIZE:%d\n", _size.load());

        // Success.
        return true;
      }

      // This is only for the reader.
      // Returns true on success, false if nothing to read or other error.
      // NOTE: This is not multi-reader safe. Yet.
      bool get(void* data, size_t* data_size)
      {
        // Nothing to read?
        if(_size.load() == 0)
          return false;

        // The size of the data size value.
        const int count_sz = sizeof(_itemSize);

        // Safely read, then increment, the current read position.
        //std::atomic<unsigned int> pos = _rIndex++;
        unsigned int pos = _rIndex++;
        // Mask the position for a circular effect.
        pos &= _capacityMask;
        // Pointer to the data item.
        uint8_t* p_data = _fifo[pos];
        // Get the data size value.
        const size_t sz = *((size_t*)p_data);
        // Return the data size.
        *data_size = sz;
        if(sz > 0)
        {
          // Increment to the start of the data position.
          p_data += count_sz;
          // Return the data.
          std::memcpy(data, p_data, sz);
        }
        // Now safely decrement the size.
        _size--;
        // Success.
        return true;
      }

      // This is only for the reader.
      // Returns true on success, false if nothing to read or other error.
      // NOTE: This is not multi-reader safe. Yet.
      bool peek(void** data, size_t* data_size, unsigned int n = 0)
      {
        // The size of the data size value.
        const int count_sz = sizeof(_itemSize);

        // Safely read the current read position.
        //std::atomic<unsigned int> pos = _rIndex.load();
        unsigned int pos = _rIndex.load();
        // Add the desired position.
        pos += n;
        // Mask the position for a circular effect.
        pos &= _capacityMask;
        // Pointer to the data item.
        uint8_t* p_data = _fifo[pos];
        // Get the data size value.
        const size_t sz = *((size_t*)p_data);
        // Return the data size.
        *data_size = sz;
        // Increment to the start of the data position.
        p_data += count_sz;
        // Return the data pointer.
        *data = p_data;
        // Success.
        return true;
      }

//       // This is only for the reader.
//       // A non-constant version of peek so that we can modify the items in-place.
//       T& peekNonConst(int n = 0)
//       {
//         const int idx = (_rIndex + n) % _capacity;
//         return _fifo[idx];
//       }

      // This is only for the reader.
      // Returns true on success or false if nothing to remove or other error.
      bool remove()
      {
        // Nothing to read?
        if(_size.load() == 0)
          return false;

        // Safely increment the current read position.
        _rIndex++;
        // Now safely decrement the size.
        _size--;

        //fprintf(stderr, "remove SIZE:%d\n", _size.load());

        // Success.
        return true;
      }

      // This is only for the reader.
      // Returns the number of items in the buffer.
      // If NOT requesting the size snapshot, this conveniently stores a snapshot (cached) version 
      //  of the size for consistent behaviour later. If requesting the size snapshot, it does not 
      //  update the snapshot itself.
      unsigned int getSize(bool useSizeSnapshot/* = false*/)
      { 
        const unsigned int sz = useSizeSnapshot ? _sizeSnapshot : _size.load();
        if(!useSizeSnapshot)
          _sizeSnapshot = sz; 
        return sz;
      }
      // This is only for the reader.
      bool isEmpty(bool useSizeSnapshot/* = false*/) const { return useSizeSnapshot ? _sizeSnapshot == 0 : _size.load() == 0; }
      // This is not thread safe, call it only when it is safe to do so.
      void clear() { _size.store(0); _sizeSnapshot = 0; _wIndex.store(0); _rIndex.store(0); }
      // This is only for the reader.
      // Clear the 'read' side of the ring buffer, which also clears the size.
      // NOTE: A corresponding clearWrite() is not provided because it is dangerous to reset 
      //  the size from the sender side - the receiver might cache the size, briefly. 
      // The sender should only grow the size while the receiver should only shrink it.
      //void clearRead() { _size = 0; _sizeSnapshot = 0; _rIndex = _wIndex; }
      void clearRead() { _size.store(0); _sizeSnapshot = 0; _rIndex.store(_wIndex); }
};



} // namespace MusECore

#endif
