//=============================================================================
//  MusE
//  Linux Music Editor
//  $Id:$
//
//  Copyright (C) 2002-2006 by Werner Schweer and others
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

#ifndef __AUDIOPREFETCH_H__
#define __AUDIOPREFETCH_H__

#include "thread.h"
#include "fifo.h"

//---------------------------------------------------------
//   AudioPrefetch
//---------------------------------------------------------

class AudioPrefetch : public Thread {
      Fifo1 fifo;
      unsigned writePos;
      unsigned seekPos; // remember last seek to optimize seeks

      virtual void processMsg1(const void*);
      void prefetch(bool seekFlag);
      void seek(unsigned pos);
      volatile int seekCount;

   public:
      AudioPrefetch(const char* name);
      ~AudioPrefetch();
      virtual void start(int);

      void msgTick();
      void msgSeek(unsigned pos);
      bool seekDone() const { return seekCount == 0; }
      Fifo1* getFifo() { return &fifo; }
      };

extern AudioPrefetch* audioPrefetch;

#endif
