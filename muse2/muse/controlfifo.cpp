//=============================================================================
//  MusE
//  Linux Music Editor
//
//  Copyright (C) 1999-2010 by Werner Schweer and others
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License version 2.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//=============================================================================

#include "controlfifo.h"

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
            // q_atomic_increment(&size);
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
      // q_atomic_decrement(&size);
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
      // q_atomic_decrement(&size);
      --size;
      }



