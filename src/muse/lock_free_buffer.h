//=========================================================
//  MusE
//  Linux Music Editor
//
//  lock_free_buffer.h
//  (C) Copyright 1999-2002 Werner Schweer (ws@seh.de)
//  (C) Copyright 2012, 2017 Tim E. Real (terminator356 on users dot sourceforge dot net)
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

#ifndef __LOCK_FREE_BUFFER_H__
#define __LOCK_FREE_BUFFER_H__

//#include <map>
#include <atomic>

namespace MusECore {


//---------------------------------------------------------
//   LockFreeBuffer
//---------------------------------------------------------

template <class T>

class LockFreeBuffer
{
      int _capacity;
      int _id; // Optional ID value.
      T *_fifo;
      volatile int _size;
      int _wIndex;
      int _rIndex;
      int _sizeSnapshot;
      T _dummyRetValue;

   public:
      // Start simple with just 2, like a flipping buffer for example.
      LockFreeBuffer(int capacity = 2, int id = 0)
      : _capacity(capacity), _id(id)
      {
        _dummyRetValue = T();
        _fifo = new T[_capacity];
        clear();
      }
      
      ~LockFreeBuffer()
      {
        if(_fifo)
          delete[] _fifo;
      }

      int id() const { return _id; }
      
      void setCapacity(int capacity = 2)
      {
        if(_fifo)
          delete _fifo;
        _fifo = 0;
        _capacity = capacity;
        _fifo = new T[_capacity];
      }

      // This is only for the writer.
      // Returns true on fifo overflow
      bool put(const T& item)
      {
        if (_size < _capacity) 
        {
          _fifo[_wIndex] = item;
          _wIndex = (_wIndex + 1) % _capacity;
          // q_atomic_increment(&_size);
          ++_size;
          return false;
        }
        return true;
      }

      // This is only for the reader.
      T get()
      {
        if(_size <= 0)
          return _dummyRetValue;
        T item(_fifo[_rIndex]);
        _rIndex = (_rIndex + 1) % _capacity;
        --_size;
        return item;
      }

      // This is only for the reader.
      const T& peek(int n = 0)
      {
        const int idx = (_rIndex + n) % _capacity;
        return _fifo[idx];
      }
      
//       // This is only for the reader.
//       // A non-constant version of peek so that we can modify the items in-place.
//       T& peekNonConst(int n = 0)
//       {
//         const int idx = (_rIndex + n) % _capacity;
//         return _fifo[idx];
//       }
      
      // This is only for the reader.
      // Returns true if error (nothing to remove).
      bool remove()
      {
        if(_size <= 0)
          return true;
        _rIndex = (_rIndex + 1) % _capacity;
        --_size;
        return false;
      }

      // This is only for the reader.
      // Returns the number of items in the buffer.
      // If NOT requesting the size snapshot, this conveniently stores a snapshot (cached) version 
      //  of the size for consistent behaviour later. If requesting the size snapshot, it does not 
      //  update the snapshot itself.
      int getSize(bool useSizeSnapshot/* = false*/)
      { 
        const int sz = useSizeSnapshot ? _sizeSnapshot : _size; 
        if(!useSizeSnapshot)
          _sizeSnapshot = sz; 
        return sz;
      }
      // This is only for the reader.
      bool isEmpty(bool useSizeSnapshot/* = false*/) const { return useSizeSnapshot ? _sizeSnapshot == 0 : _size == 0; }
      // This is not thread safe, call it only when it is safe to do so.
      void clear()         { _size = 0; _sizeSnapshot = 0; _wIndex = 0; _rIndex = 0; }
      // Clear the 'read' side of the ring buffer, which also clears the size.
      // NOTE: A corresponding clearWrite() is not provided because
      //  it is dangerous to reset the size from the sender side -
      //  the receiver might cache the size, briefly. The sender should 
      //  only grow the size while the receiver should only shrink it.
      void clearRead()     { _size = 0; _sizeSnapshot = 0; _rIndex = _wIndex; }
};

// // template <class T>
// // class LockFreeMultiBuffer
// // {
// //       int _listCapacity;
// //       LockFreeBuffer<T> *_list;
// //       //volatile int _size;
// //       //int _wIndex;
// //       //int _rIndex;
// // 
// //    public:
// //       // Start simple with just 2, like a flipping buffer for example.
// //       LockFreeMultiBuffer(int listCapacity = 1)
// //       {
// //         _listCapacity = listCapacity;
// //         _list = new LockFreeBuffer<T>[_listCapacity];
// //         //clear();
// //       }
// //       ~LockFreeMultiBuffer()
// //       {
// //         if(_list)
// //           delete[] _list;
// //       }
// //       
// //       void setListCapacity(int listCapacity = 1)
// //       {
// //         if(_list)
// //           delete _list;
// //         _list = 0;
// //         _listCapacity = listCapacity;
// //         _list = new LockFreeBuffer<T>[_listCapacity];
// //       }
// // 
// // };

// template <class T>
// class LockFreeMultiBuffer : public std::map<int, LockFreeBuffer<T>*, std::less<int> >
// {
//   public:
//     typedef typename std::map<int, LockFreeBuffer<T>*, std::less<int> > vlist;
//     typedef typename vlist::iterator iLockFreeMultiBuffer;
//     typedef typename vlist::const_iterator ciLockFreeMultiBuffer;
//     
//   private:
// //     int _curId;
//     T _dummyRetValue;
// 
//   public:
//     //LockFreeMultiBuffer() : _curId(0) { }
//     LockFreeMultiBuffer() { _dummyRetValue = T(); }
//     ~LockFreeMultiBuffer()
//     {
//       for(iLockFreeMultiBuffer i = vlist::begin(); i != vlist::end(); ++i)
//       {
//         if(i->second)
//           delete i->second;
//       }
//     }
// 
//     // Returns new buffer or zero if duplicate id or other error.
//     // Start simple with just 2, like a flipping buffer for example.
//     LockFreeBuffer<T>* createBuffer(int id, int capacity = 2)
//     {
//       LockFreeBuffer<T>* buf = new LockFreeBuffer<T>(capacity, id);
//       std::pair < iLockFreeMultiBuffer, bool > res = 
//         vlist::insert(std::pair < const int, LockFreeBuffer<T>* >(buf->id(), buf));
// //       if(res.second)
// //       {
// //         const int c_id = _curId;
// //         ++_curId;
// //         return c_id;
// //       }
// //       return -1;
//         
//       if(res.second)
//         return buf;
//       
//       delete buf;
//       return 0;
//     }
// 
//     // Returns true on error.
//     bool deleteBuffer(int id)
//     {
//       //if(id < 0)
//       //  return true;
//       iLockFreeMultiBuffer i = vlist::find(id);
//       if(i == vlist::end())
//         return true;
//       if(i->second)
//         delete i->second;
//       vlist::erase(i);
//       return false;
//     }
//     
//     LockFreeBuffer<T>* findBuffer(int id)
//     {
//       //if(id < 0)
//       //  return 0;
//       iLockFreeMultiBuffer i = vlist::find(id);
//       if(i == vlist::end())
//         return 0;
//       return i->second;
//     }
//     
//     // Returns true on invalid id.
//     bool setCapacity(int id, int capacity = 2)
//     {
//       //if(id < 0)
//       //  return true;
//       iLockFreeMultiBuffer i = vlist::find(id);
//       if(i == vlist::end())
//         return true;
//       i->second->setCapacity(capacity);
//       return false;
//     }
// 
//     // This is only for the writer.
//     // Returns true on invalid id, or on fifo overflow of that id's buffer.
//     bool put(int id, const T& item)
//     {
//       //if(id < 0)
//       //  return true;
//       iLockFreeMultiBuffer i = vlist::find(id);
//       if(i == vlist::end())
//         return true;
//       return i->second->put(item);
//     }
// 
//     // This is only for the reader.
//     T get(bool useSizeSnapshot/* = false*/)
//     {
//       iLockFreeMultiBuffer least_i = vlist::end();
//       bool is_first = true;
//       for(iLockFreeMultiBuffer i = vlist::begin(); i != vlist::end(); ++i)
//       {
//         LockFreeBuffer<T>* buf = i->second;
//         if(!buf || buf->isEmpty(useSizeSnapshot))
//           continue;
//         const T& temp_val = buf->peek();
//         if(is_first)
//         {
//           is_first = false;
//           least_i = i;
//           //least_t = temp_val;
//           continue;
//         }
//         else if(temp_val < least_i->second->peek())
//           least_i = i;
//       }
//       
//       if(least_i != vlist::end())
//         return least_i->second->get();
//       
//       return _dummyRetValue;
//     }
// 
//     // This is only for the reader.
//     const T& peek(bool useSizeSnapshot/* = false*/, int n = 0) // const
//     {
//       iLockFreeMultiBuffer least_i = vlist::end();
//       bool is_first = true;
//       int buf_sz;
//       for(int idx = 0; idx <= n; ++idx)  // Yes, that's <=
//       {
//         for(iLockFreeMultiBuffer i = vlist::begin(); i != vlist::end(); ++i)
//         {
//           LockFreeBuffer<T>* buf = i->second;
//           if(!buf)
//             continue;
//           buf_sz = buf->getSize(useSizeSnapshot);
//           if(buf_sz == 0 || n >= buf_sz)
//             continue;
//           const T& temp_val = buf->peek();
//           if(is_first)
//           {
//             is_first = false;
//             least_i = i;
//             
//             //if(idx == n)
//             //  break;
//             //++idx;
//             continue;
//           }
//           else if(temp_val < least_i->second->peek())
//           {
//             least_i = i;
//             
//             //if(idx == n)
//             //  break;
//             //++idx;
//           }
//         }
//         if(idx == n)
//           break;
//         ++idx;
//       }
// 
//       if(least_i != vlist::end())
//         return least_i->second->peek();
//       
//       return _dummyRetValue;
//     }
//     
// //     // This is only for the reader.
// //     // A non-constant version of peek so that we can modify the items in-place.
// //     T& peekNonConst(bool useSizeSnapshot/* = false*/, int n = 0) // const
// //     {
// //       iLockFreeMultiBuffer least_i = vlist::end();
// //       bool is_first = true;
// //       int buf_sz;
// //       for(int idx = 0; idx <= n; ++idx)  // Yes, that's <=
// //       {
// //         for(iLockFreeMultiBuffer i = vlist::begin(); i != vlist::end(); ++i)
// //         {
// //           LockFreeBuffer<T>* buf = i->second;
// //           if(!buf)
// //             continue;
// //           buf_sz = buf->getSize(useSizeSnapshot);
// //           if(buf_sz == 0 || n >= buf_sz)
// //             continue;
// //           T& temp_val = buf->peekNonConst();
// //           if(is_first)
// //           {
// //             is_first = false;
// //             least_i = i;
// //             
// //             //if(idx == n)
// //             //  break;
// //             //++idx;
// //             continue;
// //           }
// //           else if(temp_val < least_i->second->peekNonConst())
// //           {
// //             least_i = i;
// //             
// //             //if(idx == n)
// //             //  break;
// //             //++idx;
// //           }
// //         }
// //         if(idx == n)
// //           break;
// //         ++idx;
// //       }
// // 
// //       if(least_i != vlist::end())
// //         return least_i->second->peekNonConst();
// //       
// //       return _dummyRetValue;
// //     }
//     
//     // This is only for the reader.
//     // Returns true if error (nothing to remove).
//     bool remove(bool useSizeSnapshot/* = false*/)
//     {
//       iLockFreeMultiBuffer least_i = vlist::end();
//       bool is_first = true;
//       for(iLockFreeMultiBuffer i = vlist::begin(); i != vlist::end(); ++i)
//       {
//         LockFreeBuffer<T>* buf = i->second;
//         if(!buf || buf->isEmpty(useSizeSnapshot))
//           continue;
//         const T& temp_val = buf->peek();
//         if(is_first)
//         {
//           is_first = false;
//           least_i = i;
//           continue;
//         }
//         else if(temp_val < least_i->second->peek())
//           least_i = i;
//       }
// 
//       if(least_i != vlist::end())
//         return least_i->second->remove();
//       
//       return true;
//     }
// 
//     // This is only for the reader.
//     // Returns the total number of items in the buffers.
//     // Also conveniently stores a cached version of the size for consistent behaviour later.
//     int getSize(bool useSizeSnapshot/* = false*/) const
//     {
//       int sz = 0;
//       // Hm, maybe not so accurate, sizes may be susceptable to
//       //  asynchronous change as we iterate here...
//       for(ciLockFreeMultiBuffer i = vlist::begin(); i != vlist::end(); ++i)
//       {
//         if(LockFreeBuffer<T>* buf = i->second)
//           sz += buf->getSize(useSizeSnapshot);
//       }
//       return sz;
//     }
//     
//     // This is only for the reader.
//     bool isEmpty(bool useSizeSnapshot/* = false*/) const
//     { 
//       // Hm, maybe not so accurate, sizes may be susceptable to
//       //  asynchronous change as we iterate here...
//       for(ciLockFreeMultiBuffer i = vlist::begin(); i != vlist::end(); ++i)
//       {
//         if(const LockFreeBuffer<T>* buf = i->second)
//         {
//           if(!buf->isEmpty(useSizeSnapshot))
//             return false;
//         }
//       }
//       return true;
//     }
// 
//     // This is not thread safe, call it only when it is safe to do so.
//     void clear()
//     { 
//       for(iLockFreeMultiBuffer i = vlist::begin(); i != vlist::end(); ++i)
//       {
//         if(LockFreeBuffer<T>* buf = i->second)
//           buf->clear();
//       }
//     }
//     
//     // Clear the 'read' side of the ring buffer, which also clears the size.
//     // NOTE: A corresponding clearWrite() is not provided because
//     //  it is dangerous to reset the size from the sender side -
//     //  the receiver might cache the size, briefly. The sender should 
//     //  only grow the size while the receiver should only shrink it.
//     void clearRead()
//     {
//       for(iLockFreeMultiBuffer i = vlist::begin(); i != vlist::end(); ++i)
//       {
//         if(LockFreeBuffer<T>* buf = i->second)
//           buf->clearRead();
//       }
//     }
// };

//---------------------------------------------------------
//   LockFreeMPSCBuffer
//   A lock-free Multi-Producer Single-Consumer buffer.
//   Similar to a FIFO or Ring Buffer, but uses a fixed number of 'bins'.
//   There are no position counters or modulo operations.
//   There is no size or peek method.
//   It is intended to be fully consumed at reading time.
//---------------------------------------------------------

template <class T, unsigned int capacity>
class LockFreeMPSCBuffer
{
      T _array[capacity];
      std::atomic<bool> _inUse[capacity];
      std::atomic<bool> _hasData[capacity];

   public:
      LockFreeMPSCBuffer() { clear(); }
      
      // Returns the buffer capacity.
      unsigned int bufferCapacity() const { return capacity; }
      
      // This is only for the writer.
      // Returns true on success, false if buffer overflow.
      bool put(const T& item)
      {
        bool expected;
        for(unsigned int i = 0; i < capacity; ++i)
        {
          // Expecting a not-in-use bin. Must reset expected each time.
          expected = false;
          // Safely check and set the bin's inUse flag.
          if(_inUse[i].compare_exchange_strong(expected, true))
          {
            // Bin was not in use, now safely marked as in use. Now set the item.
            _array[i] = item;
            // Safely set the hasData flag for the reader to examine.
            _hasData[i].store(true);
            // Success.
            return true;
          }
        }
        // Sorry, all bins were full. A buffer overflow condition.
        return false;
      }

      // This is only for the reader.
      // Returns true on success, false if there was no data 
      //  available at the bin index or other error.
      bool get(T& dst, unsigned int index)
      {
        if(index >= capacity)
          return false;
        
        // Expecting hasData true.
        bool expected = true;
        // Safely check if there is data in the bin, and reset the 
        //  bin's hasData and inUse flags. Clear the hasData flag first !!!
        if(_hasData[index].compare_exchange_strong(expected, false))
        {
          // It is safe to store the value in the destination.
          dst = _array[index];
          // Now clear the inUse flag !!!
          _inUse[index].store(false);
          // Success.
          return true;
        }
        // Sorry, there was no data available in that bin.
        return false;
      }

      // This is only for the reader.
      // Returns true on success.
      bool remove(unsigned int index)
      {
        if(index >= capacity)
          return false;
        
        // Expecting hasData true.
        bool expected = true;
        // Safely check and reset the bin's hasData and inUse flags.
        if(_hasData[index].compare_exchange_strong(expected, false))
        {
          _inUse[index].store(false); 
          // Success.
          return true;
        }
        // Sorry, there was no data available in that bin.
        return false;
      }
      
      // Not thread safe. Only call when safe to do so,
      //  like constructor etc.
      void clear() 
      { 
        for(unsigned int i = 0; i < capacity; ++i) 
        { 
          // Clear the hasData flag first !!!
          _hasData[i].store(false);
          // Now clear the inUse flag !!!
          _inUse[i].store(false);
        } 
      }
      
      // This is only for the reader.
      void clearRead() 
      { 
        bool expected;
        for(unsigned int i = 0; i < capacity; ++i) 
        { 
          // Expecting hasData true. Must reset expected each time.
          expected = true;
          // Safely check and reset the bin's hasData and inUse flags.
          if(_hasData[i].compare_exchange_strong(expected, false))
            _inUse[i].store(false); 
        } 
      }
};

//---------------------------------------------------------
//   LockFreeMPSCRingBuffer
//---------------------------------------------------------

template <class T>

class LockFreeMPSCRingBuffer
{
      unsigned int _capacity;
      T *_fifo;
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
      LockFreeMPSCRingBuffer(unsigned int capacity = 2)
      {
        _capacity = roundCapacity(capacity);
        _capacityMask = _capacity - 1;
        _fifo = new T[_capacity];
        clear();
      }
      
      ~LockFreeMPSCRingBuffer()
      {
        if(_fifo)
          delete[] _fifo;
      }

      void setCapacity(unsigned int capacity = 2)
      {
        if(_fifo)
          delete[] _fifo;
        _fifo = 0;
        _capacity = roundCapacity(capacity);
        _capacityMask = _capacity - 1;
        _fifo = new T[_capacity];
      }

      // This is only for the writer.
      // Returns true on success, false on fifo overflow or other error.
      bool put(const T& item)
      {
        // Buffer full? Overflow condition.
        if(_size.load() >= _capacity) 
          return false;
        
        // Safely read, then increment, the current write position.
        //std::atomic<unsigned int> pos = _wIndex++;
        unsigned int pos = _wIndex++;
        // Mask the position for a circular effect.
        pos &= _capacityMask;
        // Store the item in that position.
        _fifo[pos] = item;
        // Now safely increment the size.
        _size++;
        // Success.
        return true;
      }

      // This is only for the reader.
      // Returns true on success, false if nothing to read or other error.
      // NOTE: This is not multi-reader safe. Yet.
      bool get(T& dst)
      {
        // Nothing to read?
        if(_size.load() == 0)
          return false;
        
        // Safely read, then increment, the current read position.
        //std::atomic<unsigned int> pos = _rIndex++;
        unsigned int pos = _rIndex++;
        // Mask the position for a circular effect.
        pos &= _capacityMask;
        // Store the item in that position into the destination.
        dst = _fifo[pos];
        // Now safely decrement the size.
        _size--;
        // Success.
        return true;
      }

      // This is only for the reader.
      // NOTE: This is not multi-reader safe. Yet.
      const T& peek(unsigned int n = 0)
      {
        // Safely read the current read position.
        //std::atomic<unsigned int> pos = _rIndex.load();
        unsigned int pos = _rIndex.load();
        // Add the desired position.
        pos += n;
        // Mask the position for a circular effect.
        pos &= _capacityMask;
        return _fifo[pos];
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

