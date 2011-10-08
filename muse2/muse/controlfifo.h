//=============================================================================
//  MusE
//  Linux Music Editor
//
//  Copyright (C) 1999-2011 by Werner Schweer and others
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License 
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
//=============================================================================

#ifndef __CONTROLFIFO_H__
#define __CONTROLFIFO_H__

#define CONTROL_FIFO_SIZE 8192

namespace MusECore {

//---------------------------------------------------------
//  ControlEvent
//  Item struct for ControlFifo. 
//---------------------------------------------------------

struct ControlEvent
{
  // Unique: Whether the item must not be skipped, even if it has the same 
  //  (possibly rounded) frame and index as the previous item. This is mainly for 
  //  dssi-vst guis, they require acknowledgment of every message.
  bool unique; 
  unsigned long idx;
  float value;
  unsigned long frame;    
};

//---------------------------------------------------------
//  ControlFifo
//---------------------------------------------------------

class ControlFifo
{
      ControlEvent fifo[CONTROL_FIFO_SIZE];
      volatile int size;
      int wIndex;
      int rIndex;

   public:
      ControlFifo()  { clear(); }
      bool put(const ControlEvent& event);   // returns true on fifo overflow
      ControlEvent get();
      const ControlEvent& peek(int n = 0);
      void remove();
      bool isEmpty() const { return size == 0; }
      void clear()         { size = 0, wIndex = 0, rIndex = 0; }
      int getSize() const  { return size; }
};

} // namespace MusECore

#endif
