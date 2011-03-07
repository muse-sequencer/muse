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

#ifndef __AUDIOWRITEBACK_H__
#define __AUDIOWRITEBACK_H__

#include "thread.h"

//---------------------------------------------------------
//   AudioWriteback
//---------------------------------------------------------

class AudioWriteback : public Thread {
      virtual void processMsg1(const void*);
      volatile int counter;

   public:
      AudioWriteback(const char* name);
      ~AudioWriteback() {}
      virtual void start(int);
      void trigger();
      bool active() const { return counter > 0; }
      };

extern AudioWriteback* audioWriteback;

#endif
