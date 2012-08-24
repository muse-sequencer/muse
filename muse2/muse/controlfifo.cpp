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

#include "controlfifo.h"

namespace MusECore {

//---------------------------------------------------------
//   ControlFifo
//    put
//    return true on fifo overflow
//---------------------------------------------------------

bool ControlFifo::put(const ControlEvent& event)
      {
      if (size < CONTROL_FIFO_SIZE) {
            fifo[wIndex] = event;
            wIndex = (wIndex + 1) % CONTROL_FIFO_SIZE;
            ++size;
            return false;
            }
      return true;
      }

//---------------------------------------------------------
//   get
//---------------------------------------------------------

ControlEvent ControlFifo::get()
      {
      ControlEvent event(fifo[rIndex]);
      rIndex = (rIndex + 1) % CONTROL_FIFO_SIZE;
      --size;
      return event;
      }

//---------------------------------------------------------
//   peek
//---------------------------------------------------------

const ControlEvent& ControlFifo::peek(int n)
      {
      int idx = (rIndex + n) % CONTROL_FIFO_SIZE;
      return fifo[idx];
      }

//---------------------------------------------------------
//   remove
//---------------------------------------------------------

void ControlFifo::remove()
      {
      rIndex = (rIndex + 1) % CONTROL_FIFO_SIZE;
      --size;
      }

} // namespace MusECore


