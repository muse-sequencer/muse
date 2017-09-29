//=========================================================
//  MusE
//  Linux Music Editor
//
//  lock_free_buffer.cpp
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

#include "lock_free_buffer.h"

namespace MusECore {

template <class T>
// Start simple with just 2, like a flipping buffer for example.
LockFreeBuffer<T>::LockFreeBuffer(int capacity, int id) 
  : _capacity(capacity), _id(id)
{
  _fifo = new T[_capacity];
  clear();
}

template <class T>
LockFreeBuffer<T>::~LockFreeBuffer()
{
  if(_fifo)
    delete[] _fifo;
}

template <class T>
void LockFreeBuffer<T>::setCapacity(int capacity)
{
  if(_fifo)
    delete _fifo;
  _fifo = 0;
  _capacity = capacity;
  _fifo = new T[_capacity];
}

template <class T>
bool LockFreeBuffer<T>::put(const T& item)   // returns true on fifo overflow
{
  if (_size < _capacity) {
        _fifo[_wIndex] = item;
        _wIndex = (_wIndex + 1) % _capacity;
        // q_atomic_increment(&_size);
        ++_size;
        return false;
        }
  return true;
}

template <class T>
T LockFreeBuffer<T>::get()
{
  T item(_fifo[_rIndex]);
  _rIndex = (_rIndex + 1) % _capacity;
  --_size;
  return item;
}

template <class T>
const T& LockFreeBuffer<T>::peek(int n)
{
  const int idx = (_rIndex + n) % _capacity;
  return _fifo[idx];
}

template <class T>
void LockFreeBuffer<T>::remove()
{
  _rIndex = (_rIndex + 1) % _capacity;
  --_size;
}

      
//--------------------------------------
//  LockFreeMultiBuffer
//--------------------------------------
      
template <class T>
LockFreeMultiBuffer<T>::~LockFreeMultiBuffer()
{
  for(iLockFreeMultiBuffer i = vlist::begin(); i != vlist::end(); ++i)
  {
    if(i->second)
      delete i->second;
  }
}

// Returns new buffer or zero if duplicate id or other error.
// Start simple with just 2, like a flipping buffer for example.
template <class T>
LockFreeBuffer<T>* LockFreeMultiBuffer<T>::createBuffer(int capacity, int id)
{
  LockFreeBuffer<T>* buf = new LockFreeBuffer<T>(capacity, id);
  std::pair < iLockFreeMultiBuffer, bool > res = 
    vlist::insert(std::pair < const int, LockFreeBuffer<T>* >(buf->id(), buf));
//       if(res.second)
//       {
//         const int c_id = _curId;
//         ++_curId;
//         return c_id;
//       }
//       return -1;
    
  if(res.second)
    return buf;
  
  delete buf;
  return 0;
}

    // Returns true on error.
template <class T>
bool LockFreeMultiBuffer<T>::deleteBuffer(int id)
{
  //if(id < 0)
  //  return true;
  iLockFreeMultiBuffer i = vlist::find(id);
  if(i == vlist::end())
    return true;
  if(i->second)
    delete i->second;
  vlist::erase(i);
  return false;
}
    
template <class T>
LockFreeBuffer<T>* LockFreeMultiBuffer<T>::findBuffer(int id)
{
  //if(id < 0)
  //  return 0;
  iLockFreeMultiBuffer i = vlist::find(id);
  if(i == vlist::end())
    return 0;
  return i->second;
}
    
// Returns true on invalid id.
template <class T>
bool LockFreeMultiBuffer<T>::setCapacity(int id, int capacity)
{
  //if(id < 0)
  //  return true;
  iLockFreeMultiBuffer i = vlist::find(id);
  if(i == vlist::end())
    return true;
  i->second->setCapacity(capacity);
  return false;
}

// This is only for the writer.
// Returns true on invalid id, or on fifo overflow of that id's buffer.
template <class T>
bool LockFreeMultiBuffer<T>::put(int id, const T& item)
{
  //if(id < 0)
  //  return true;
  iLockFreeMultiBuffer i = vlist::find(id);
  if(i == vlist::end())
    return true;
  return i->second->put(item);
}

// This is only for the reader.
template <class T>
T LockFreeMultiBuffer<T>::get(bool useSizeSnapshot)
{
  T temp_val;
  iLockFreeMultiBuffer least_i = vlist::end();
  bool is_first = true;
  for(iLockFreeMultiBuffer i = vlist::begin(); i != vlist::end(); ++i)
  {
    LockFreeBuffer<T>* buf = i->second;
    if(!buf || buf->isEmpty(useSizeSnapshot))
      continue;
    temp_val = buf->peek(useSizeSnapshot);
    if(is_first)
    {
      is_first = false;
      least_i = i;
      continue;
    }
    else if(temp_val < least_i->second)
      least_i = i;
  }

  if(least_i != vlist::end())
  {
    return least_i->second->get(useSizeSnapshot);
  }
  return T();
}

// This is only for the reader.
template <class T>
const T& LockFreeMultiBuffer<T>::peek(bool useSizeSnapshot, int n)
{
  T temp_val;
  iLockFreeMultiBuffer least_i = vlist::end();
  bool is_first = true;
  int buf_sz;
  for(int idx = 0; idx <= n; ++idx)  // Yes, that's <=
  {
    for(iLockFreeMultiBuffer i = vlist::begin(); i != vlist::end(); ++i)
    {
      const LockFreeBuffer<T>* buf = i->second;
      if(!buf)
        continue;
      buf_sz = buf->getSize(useSizeSnapshot);
      if(buf_sz == 0 || n >= buf_sz)
        continue;
      temp_val = buf->peek(useSizeSnapshot);
      if(is_first)
      {
        is_first = false;
        least_i = i;
        //if(idx == n)
        //  break;
        //++idx;
        continue;
      }
      else if(temp_val < least_i->second)
      {
        least_i = i;
        //if(idx == n)
        //  break;
        //++idx;
      }
    }
    if(idx == n)
      break;
    ++idx;
  }

  if(least_i != vlist::end())
    return least_i->second->peek(useSizeSnapshot);
  
  return T();
}
    
// This is only for the reader.
template <class T>
void LockFreeMultiBuffer<T>::remove(bool useSizeSnapshot)
{
  T temp_val;
  iLockFreeMultiBuffer least_i = vlist::end();
  bool is_first = true;
  for(iLockFreeMultiBuffer i = vlist::begin(); i != vlist::end(); ++i)
  {
    LockFreeBuffer<T>* buf = i->second;
    if(!buf || buf->isEmpty(useSizeSnapshot))
      continue;
    temp_val = buf->peek(useSizeSnapshot);
    if(is_first)
    {
      is_first = false;
      least_i = i;
      continue;
    }
    else if(temp_val < least_i->second)
      least_i = i;
  }

  if(least_i != vlist::end())
    least_i->second->remove(useSizeSnapshot);
}

// This is only for the reader.
template <class T>
int LockFreeMultiBuffer<T>::getSize(bool useSizeSnapshot) const
{
  int sz = 0;
  // Hm, maybe not so accurate, sizes may be susceptable to
  //  asynchronous change as we iterate here...
  for(ciLockFreeMultiBuffer i = vlist::begin(); i != vlist::end(); ++i)
  {
    if(const LockFreeBuffer<T>* buf = i->second)
      sz += buf->getSize(useSizeSnapshot);
  }
  return sz;
}
    
// This is only for the reader.
template <class T>
bool LockFreeMultiBuffer<T>::isEmpty(bool useSizeSnapshot) const
{ 
  // Hm, maybe not so accurate, sizes may be susceptable to
  //  asynchronous change as we iterate here...
  for(ciLockFreeMultiBuffer i = vlist::begin(); i != vlist::end(); ++i)
  {
    if(const LockFreeBuffer<T>* buf = i->second)
    {
      if(!buf->isEmpty(useSizeSnapshot))
        return false;
    }
  }
  return true;
}

// This is not thread safe, call it only when it is safe to do so.
template <class T>
void LockFreeMultiBuffer<T>::clear()
{ 
  for(iLockFreeMultiBuffer i = vlist::begin(); i != vlist::end(); ++i)
  {
    if(LockFreeBuffer<T>* buf = i->second)
      buf->clear();
  }
}
    
// Clear the 'read' side of the ring buffer, which also clears the size.
// NOTE: A corresponding clearWrite() is not provided because
//  it is dangerous to reset the size from the sender side -
//  the receiver might cache the size, briefly. The sender should 
//  only grow the size while the receiver should only shrink it.
template <class T>
void LockFreeMultiBuffer<T>::clearRead()
{
  for(iLockFreeMultiBuffer i = vlist::begin(); i != vlist::end(); ++i)
  {
    if(LockFreeBuffer<T>* buf = i->second)
      buf->clearRead();
  }
}
  
  
  
} // namespace MusECore
