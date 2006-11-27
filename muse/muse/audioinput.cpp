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
#include "audioinput.h"
#include "driver/jackaudio.h"
#include "audio.h"

//---------------------------------------------------------
//   AudioInput
//---------------------------------------------------------

AudioInput::AudioInput()
   : AudioTrack()
      {
      // set Default for Input Ports:
      _mute = muteDefault();
      _channels = 0;
      setChannels(2);
      //
      // buffers are allocated from AudioTrack()
      // and not needed by AudioInput which uses
      // the JACK supplied buffers
      for (int i = 0; i < MAX_CHANNELS; ++i) {
            if (buffer[i]) {
            	delete[] buffer[i];
                  buffer[i] = 0;
                  }
            }
      }

//---------------------------------------------------------
//   ~AudioInput
//---------------------------------------------------------

AudioInput::~AudioInput()
      {
      for (int i = 0; i < _channels; ++i) {
            if (!jackPort(i).isZero())
                  audioDriver->unregisterPort(jackPort(i));
            }
      // AudioInput does not own buffers (they are from JACK)
      // make sure ~AudioTrack() does not delete them:
      for (int i = 0; i < MAX_CHANNELS; ++i)
            buffer[i] = 0;
      }

//---------------------------------------------------------
//   write
//---------------------------------------------------------

void AudioInput::write(Xml& xml) const
      {
      xml.stag("AudioInput");
      AudioTrack::writeProperties(xml);
      xml.etag("AudioInput");
      }

//---------------------------------------------------------
//   read
//---------------------------------------------------------

void AudioInput::read(QDomNode node)
      {
      for (;!node.isNull(); node = node.nextSibling()) {
            AudioTrack::readProperties(node);
            }
      setName(name());  // allocate jack ports
      }

//---------------------------------------------------------
//   setChannels
//---------------------------------------------------------

void AudioInput::setChannels(int n)
      {
      if (n == _channels)
            return;
      AudioTrack::setChannels(n);
      }

//---------------------------------------------------------
//   setName
//---------------------------------------------------------

void AudioInput::setName(const QString& s)
      {
      Track::setName(s);
      for (int i = 0; i < channels(); ++i) {
            if (!jackPort(i).isZero()) {
                  char buffer[128];
                  snprintf(buffer, 128, "%s-%d", _name.toAscii().data(), i);
                  if (!jackPort(i).isZero())
                        audioDriver->setPortName(jackPort(i), buffer);
                  }
            }
      }

//---------------------------------------------------------
//   collectInputData
//    if buffer contains silence, set bufferEmpty to true
//---------------------------------------------------------

void AudioInput::collectInputData()
      {
      bufferEmpty = false;
      for (int ch = 0; ch < channels(); ++ch) {
            Port port = jackPort(ch);
            if (!port.isZero())
                  buffer[ch] = audioDriver->getBuffer(port, segmentSize);
            }
      }


