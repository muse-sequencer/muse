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

#include "globals.h"
#include "audioaux.h"
#include "al/xml.h"
#include "song.h"

//---------------------------------------------------------
//   AudioAux
//---------------------------------------------------------

AudioAux::AudioAux()
   : AudioTrack(AUDIO_AUX)
      {
      _channels = 0;
      setChannels(2);
      }

//---------------------------------------------------------
//   AudioAux
//---------------------------------------------------------

AudioAux::~AudioAux()
      {
      }

//---------------------------------------------------------
//   read
//---------------------------------------------------------

void AudioAux::read(QDomNode node)
      {
      while (!node.isNull()) {
            AudioTrack::readProperties(node);
            node = node.nextSibling();
            }
      }

//---------------------------------------------------------
//   write
//---------------------------------------------------------

void AudioAux::write(Xml& xml) const
      {
      xml.tag("AudioAux");
      AudioTrack::writeProperties(xml);
      xml.etag("AudioAux");
      }

//---------------------------------------------------------
//   setChannels
//---------------------------------------------------------

void AudioAux::setChannels(int n)
      {
      AudioTrack::setChannels(n);
      }

//---------------------------------------------------------
//   collectInputData
//---------------------------------------------------------

void AudioAux::collectInputData()
	{
      bufferEmpty = false;
      int ctrl = AC_AUX;
      AuxList* al = song->auxs();          // aux sends
      int idx = 0;
      for (iAudioAux i = al->begin(); i != al->end(); ++i, ++idx) {
            if (*i == this) {
            	ctrl += idx;
                  break;
                  }
            }
      RouteList* rl = inRoutes();
      bool copy = true;

      TrackList* tl = song->tracks();
      for (iTrack i = tl->begin(); i != tl->end(); ++i) {
            if ((*i)->isMidiTrack())
                  continue;
            AudioTrack* track = (AudioTrack*)(*i);
            if (!track->hasAuxSend() || track->off() || song->bounceTrack == track)
                  continue;
            if (copy) {
      		copy = !track->multiplyCopy(channels(), buffer, ctrl);
                  }
            else
	            track->multiplyAdd(channels(), buffer, ctrl);
            }
      if (copy) {
      	for (int i = 0; i < channels(); ++i)
            	memset(buffer[i], 0, sizeof(float) * segmentSize);
            bufferEmpty = true;
            }
      }

