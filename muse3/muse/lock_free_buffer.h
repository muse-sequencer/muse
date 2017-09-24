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

#include <map>

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

   public:
      // Start simple with just 2, like a flipping buffer for example.
      LockFreeBuffer(int capacity = 2, int id = 0);
//       : _capacity(capacity), _id(id);
//       //LockFreeBuffer(int capacity = 2) : _capacity(capacity)
//       {
//         _fifo = new T[_capacity];
//         clear();
//       }
      ~LockFreeBuffer();
//       {
//         if(_fifo)
//           delete[] _fifo;
//       }

      int id() const { return _id; }
      
      void setCapacity(int capacity = 2);
//       {
//         if(_fifo)
//           delete _fifo;
//         _fifo = 0;
//         _capacity = capacity;
//         _fifo = new T[_capacity];
//       }

      bool put(const T& item);   // returns true on fifo overflow
//       {
//         if (_size < _capacity) {
//               _fifo[_wIndex] = item;
//               _wIndex = (_wIndex + 1) % _capacity;
//               // q_atomic_increment(&_size);
//               ++_size;
//               return false;
//               }
//         return true;
//       }

      T get();
//       {
//         T item(_fifo[_rIndex]);
//         _rIndex = (_rIndex + 1) % _capacity;
//         --_size;
//         return item;
//       }

      const T& peek(int n = 0);
//       {
//         const int idx = (_rIndex + n) % _capacity;
//         return _fifo[idx];
//       }
      void remove();
//       {
//         _rIndex = (_rIndex + 1) % _capacity;
//         --_size;
//       }

      // This is only for the reader.
      int  getSize() const { return _size; }
      // This is only for the reader.
      bool isEmpty() const { return _size == 0; }
      // This is not thread safe, call it only when it is safe to do so.
      void clear()         { _size = 0; _wIndex = 0; _rIndex = 0; }
      // Clear the 'read' side of the ring buffer, which also clears the size.
      // NOTE: A corresponding clearWrite() is not provided because
      //  it is dangerous to reset the size from the sender side -
      //  the receiver might cache the size, briefly. The sender should 
      //  only grow the size while the receiver should only shrink it.
      void clearRead()     { _size = 0; _rIndex = _wIndex; }
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

template <class T>
class LockFreeMultiBuffer : public std::map<int, LockFreeBuffer<T>*, std::less<int> >
{
  public:
    typedef typename std::map<int, LockFreeBuffer<T>*, std::less<int> > vlist;
    typedef typename vlist::iterator iLockFreeMultiBuffer;
    typedef typename vlist::const_iterator ciLockFreeMultiBuffer;
    
//   private:
//     int _curId;

  public:
    //LockFreeMultiBuffer() : _curId(0) { }
    LockFreeMultiBuffer() { }
    ~LockFreeMultiBuffer();
//     {
//       for(iLockFreeMultiBuffer i = vlist::begin(); i != vlist::end(); ++i)
//       {
//         if(i->second)
//           delete i->second;
//       }
//     }

    // Returns new buffer or zero if duplicate id or other error.
    // Start simple with just 2, like a flipping buffer for example.
    LockFreeBuffer<T>* createBuffer(int capacity = 2, int id = 0);
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

    // Returns true on error.
    bool deleteBuffer(int id);
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
    
    LockFreeBuffer<T>* findBuffer(int id);
//     {
//       //if(id < 0)
//       //  return 0;
//       iLockFreeMultiBuffer i = vlist::find(id);
//       if(i == vlist::end())
//         return 0;
//       return i->second;
//     }
    
    // Returns true on invalid id.
    bool setCapacity(int id, int capacity = 2);
//     {
//       //if(id < 0)
//       //  return true;
//       iLockFreeMultiBuffer i = vlist::find(id);
//       if(i == vlist::end())
//         return true;
//       i->second->setCapacity(capacity);
//       return false;
//     }

    // This is only for the writer.
    // Returns true on invalid id, or on fifo overflow of that id's buffer.
    bool put(int id, const T& item);
//     {
//       //if(id < 0)
//       //  return true;
//       iLockFreeMultiBuffer i = vlist::find(id);
//       if(i == vlist::end())
//         return true;
//       return i->second->put(item);
//     }

    // This is only for the reader.
    T get();
//     {
//       T temp_val;
//       iLockFreeMultiBuffer least_i = vlist::end();
//       bool is_first = true;
//       for(iLockFreeMultiBuffer i = vlist::begin(); i != vlist::end(); ++i)
//       {
//         LockFreeBuffer<T>* buf = i->second;
//         if(!buf || buf->isEmpty())
//           continue;
//         temp_val = buf->peek();
//         if(is_first)
//         {
//           is_first = false;
//           least_i = i;
//           continue;
//         }
//         else if(temp_val < least_i->second)
//           least_i = i;
//       }
// 
//       if(least_i != vlist::end())
//       {
//         return least_i->second->get();
//       }
//       return T();
//     }

    // This is only for the reader.
    const T& peek(int n = 0);
//     {
//       T temp_val;
//       iLockFreeMultiBuffer least_i = vlist::end();
//       bool is_first = true;
//       int buf_sz;
//       for(int idx = 0; idx <= n; ++idx)  // Yes, that's <=
//       {
//         for(iLockFreeMultiBuffer i = vlist::begin(); i != vlist::end(); ++i)
//         {
//           const LockFreeBuffer<T>* buf = i->second;
//           if(!buf)
//             continue;
//           buf_sz = buf->getSize();
//           if(buf_sz == 0 || n >= buf_sz)
//             continue;
//           temp_val = buf->peek();
//           if(is_first)
//           {
//             is_first = false;
//             least_i = i;
//             //if(idx == n)
//             //  break;
//             //++idx;
//             continue;
//           }
//           else if(temp_val < least_i->second)
//           {
//             least_i = i;
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
//       return T();
//     }
    
    // This is only for the reader.
    void remove();
//     {
//       T temp_val;
//       iLockFreeMultiBuffer least_i = vlist::end();
//       bool is_first = true;
//       for(iLockFreeMultiBuffer i = vlist::begin(); i != vlist::end(); ++i)
//       {
//         LockFreeBuffer<T>* buf = i->second;
//         if(!buf || buf->isEmpty())
//           continue;
//         temp_val = buf->peek();
//         if(is_first)
//         {
//           is_first = false;
//           least_i = i;
//           continue;
//         }
//         else if(temp_val < least_i->second)
//           least_i = i;
//       }
// 
//       if(least_i != vlist::end())
//         least_i->second->remove();
//     }

    // This is only for the reader.
    int getSize() const;
//     {
//       int sz = 0;
//       // Hm, maybe not so accurate, sizes may be susceptable to
//       //  asynchronous change as we iterate here...
//       for(ciLockFreeMultiBuffer i = vlist::begin(); i != vlist::end(); ++i)
//       {
//         if(const LockFreeBuffer<T>* buf = i->second)
//           sz += buf->getSize();
//       }
//       return sz;
//     }
    
    // This is only for the reader.
    bool isEmpty() const;
//     { 
//       // Hm, maybe not so accurate, sizes may be susceptable to
//       //  asynchronous change as we iterate here...
//       for(ciLockFreeMultiBuffer i = vlist::begin(); i != vlist::end(); ++i)
//       {
//         if(const LockFreeBuffer<T>* buf = i->second)
//         {
//           if(buf->getSize() > 0)
//             return false;
//         }
//       }
//       return true;
//     }

    // This is not thread safe, call it only when it is safe to do so.
    void clear();
//     { 
//       for(iLockFreeMultiBuffer i = vlist::begin(); i != vlist::end(); ++i)
//       {
//         if(LockFreeBuffer<T>* buf = i->second)
//           buf->clear();
//       }
//     }
    
    // Clear the 'read' side of the ring buffer, which also clears the size.
    // NOTE: A corresponding clearWrite() is not provided because
    //  it is dangerous to reset the size from the sender side -
    //  the receiver might cache the size, briefly. The sender should 
    //  only grow the size while the receiver should only shrink it.
    void clearRead();
//     {
//       for(iLockFreeMultiBuffer i = vlist::begin(); i != vlist::end(); ++i)
//       {
//         if(LockFreeBuffer<T>* buf = i->second)
//           buf->clearRead();
//       }
//     }
};


} // namespace MusECore

#endif

