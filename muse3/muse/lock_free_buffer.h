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


namespace MusECore {


//---------------------------------------------------------
//   LockFreeBuffer
//---------------------------------------------------------

template <class T>

class LockFreeBuffer
{
      int _capacity;
      T *_fifo;
      volatile int _size;
      int _wIndex;
      int _rIndex;

   public:
      // Start simple with just 2, like a flipping buffer for example.
      LockFreeBuffer(int capacity = 2)
      {
        _capacity = capacity;
        _fifo = new T[_capacity];
        clear();
      }
      ~LockFreeBuffer()
      {
        if(_fifo)
          delete[] _fifo;
      }

      void setCapacity(int capacity = 2)
      {
        if(_fifo)
          delete _fifo;
        _fifo = 0;
        _capacity = capacity;
        _fifo = new T[_capacity];
      }

      bool put(const T& item)   // returns true on fifo overflow
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

      T get()
      {
        T item(_fifo[_rIndex]);
        _rIndex = (_rIndex + 1) % _capacity;
        --_size;
        return item;
      }

      const T& peek(int n = 0)
      {
        const int idx = (_rIndex + n) % _capacity;
        return _fifo[idx];
      }
      void remove()
      {
        _rIndex = (_rIndex + 1) % _capacity;
        --_size;
      }

      bool isEmpty() const { return _size == 0; }
      void clear()         { _size = 0, _wIndex = 0, _rIndex = 0; }
      int getSize() const  { return _size; }
};


} // namespace MusECore

#endif

