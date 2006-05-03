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

#ifndef __WAVETRACK_H__
#define __WAVETRACK_H__

#include "audiotrack.h"

namespace AL {
      class Pos;
      };

class Part;

//---------------------------------------------------------
//   WaveTrack
//---------------------------------------------------------

class WaveTrack : public AudioTrack {
      Q_OBJECT

      float** readBuffer[FIFO_BUFFER];

      Part* recordPart;      // part we are recording into
      bool partCreated;

	void getDummyInput(int, unsigned);

   protected:
      virtual void collectInputData();

   public:
      static bool firstWaveTrack;

      WaveTrack();
      ~WaveTrack();
      void clone(WaveTrack*);

      virtual WaveTrack* newTrack() const { return new WaveTrack(); }
      virtual Part* newPart(Part*p=0, bool clone=false);

      virtual void read(QDomNode);
      virtual void write(Xml&) const;

      virtual void fetchData(unsigned pos, unsigned frames, int);

      virtual void setChannels(int n);
      virtual bool hasAuxSend() const 	{ return true; }
      bool canEnableRecord() const;
      virtual bool canRecord() const 	{ return true; }

      virtual void startRecording();
      void recordBeat();
      virtual void stopRecording(const AL::Pos&, const AL::Pos&);
	virtual void process();
      };


typedef tracklist<WaveTrack*>::iterator iWaveTrack;
typedef tracklist<WaveTrack*>::const_iterator ciWaveTrack;
typedef tracklist<WaveTrack*> WaveTrackList;

#endif



