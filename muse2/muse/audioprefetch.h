//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: audioprefetch.h,v 1.3.2.2 2009/12/20 05:00:35 terminator356 Exp $
//
//  (C) Copyright 2001 Werner Schweer (ws@seh.de)
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

#ifndef __AUDIOPREFETCH_H__
#define __AUDIOPREFETCH_H__

#include "thread.h"

namespace MusECore {

//---------------------------------------------------------
//   AudioPrefetch
//---------------------------------------------------------

class AudioPrefetch : public Thread {
      unsigned writePos;
      unsigned seekPos; // remember last seek to optimize seeks

      virtual void processMsg1(const void*);
      void prefetch(bool doSeek);
      void seek(unsigned pos);

      volatile int seekCount;
      
   public:
      AudioPrefetch(const char* name);
      
      ~AudioPrefetch();
      virtual void start(int);

      void msgTick();
      void msgSeek(unsigned samplePos, bool force=false);
      
      bool seekDone() const { return seekCount == 0; }
      };

} // namespace MusECore

namespace MusEGlobal {
extern MusECore::AudioPrefetch* audioPrefetch;
}

#endif
