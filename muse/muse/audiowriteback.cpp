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

#include <poll.h>

#include "audiowriteback.h"
#include "globals.h"
#include "audio.h"
#include "song.h"

AudioWriteback* audioWriteback;

//---------------------------------------------------------
//   AudioWriteback
//---------------------------------------------------------

AudioWriteback::AudioWriteback(const char* name)
   : Thread(name)
      {
      counter = 0;
      }

//---------------------------------------------------------
//   readMsg
//---------------------------------------------------------

static void readMsgP(void* p, void*)
      {
      AudioWriteback* at = (AudioWriteback*)p;
      at->readMsg1(1);
      }

//---------------------------------------------------------
//   start
//---------------------------------------------------------

void AudioWriteback::start(int priority)
      {
      clearPollFd();
      addPollFd(toThreadFdr, POLLIN, ::readMsgP, this, 0);
      Thread::start(priority);
      }

//---------------------------------------------------------
//   processMsg
//---------------------------------------------------------

void AudioWriteback::processMsg1(const void* m)
      {
      while (counter) {
            q_atomic_decrement(&counter);
      	OutputList* ol = song->outputs();
      	if (!ol->empty()) {
            	AudioOutput* ao = ol->front();
	            if (ao->recordFlag())
      	            ao->record();
	            }
      	WaveTrackList* tl = song->waves();
	      for (iWaveTrack t = tl->begin(); t != tl->end(); ++t) {
      	      WaveTrack* track = *t;
            	if (track->recordFlag())
                  	track->record();
	            }
      	}
      }

//---------------------------------------------------------
//   trigger
//    trigger audio writeback loop
//---------------------------------------------------------

void AudioWriteback::trigger()
	{
	q_atomic_increment(&counter);
      if (counter < 2) {
      	if (sendMsg1("0", 1)) {
            	printf("AudioWriteback::msgTick(): send failed!\n");
                  }
            }
	}

