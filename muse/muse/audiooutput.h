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

#ifndef __AUDIOOUTPUT_H__
#define __AUDIOOUTPUT_H__

#include "audiotrack.h"

//---------------------------------------------------------
//   AudioOutput
//---------------------------------------------------------

class AudioOutput : public AudioTrack {
      Q_OBJECT

      void* jackPorts[MAX_CHANNELS];
      float* buffer[MAX_CHANNELS];
      unsigned long _nframes;

      float* _monitorBuffer[MAX_CHANNELS];

   public:
      AudioOutput();
      virtual ~AudioOutput();
      virtual AudioOutput* newTrack() const { return new AudioOutput(); }

      virtual void activate1();
      virtual void activate2();
      virtual void deactivate();

      virtual void read(QDomNode);
      virtual void write(Xml&) const;
      virtual void setName(const QString& s);
      void* jackPort(int channel) { return jackPorts[channel]; }
      void setJackPort(int channel, void*p) { jackPorts[channel] = p; }
      virtual void setChannels(int n);

      virtual bool canRecord() const { return true; }
      virtual void stopRecording(const AL::Pos&, const AL::Pos&);

      float** monitorBuffer() { return _monitorBuffer; }
	void silence(unsigned);
      virtual void process();
      };

typedef tracklist<AudioOutput*>::iterator iAudioOutput;
typedef tracklist<AudioOutput*>::const_iterator ciAudioOutput;
typedef tracklist<AudioOutput*> OutputList;

#endif

