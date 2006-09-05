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
   : AudioTrack(AUDIO_INPUT)
      {
      // set Default for Input Ports:
      _mute = true;
//TODO      setVolume(0.0);
      for (int i = 0; i < MAX_CHANNELS; ++i)
            jackPorts[i] = 0;
      _channels = 0;
      setChannels(2);
      //
      // buffer pointer are filled in collectInputData()
      //  (pointer to jack buffer)

      for (int i = 0; i < MAX_CHANNELS; ++i) {
		delete[] buffer[i];
            buffer[i] = 0;
            }
      }

//---------------------------------------------------------
//   ~AudioInput
//---------------------------------------------------------

AudioInput::~AudioInput()
      {
      for (int i = 0; i < _channels; ++i)
            audioDriver->unregisterPort(jackPorts[i]);
      //
      // buffers belong to JACK: zero pointer so they are
      // not free'd by AudioTrack destructor
      //
      for (int i = 0; i < MAX_CHANNELS; ++i)
            buffer[i] = 0;
      }

//---------------------------------------------------------
//   write
//---------------------------------------------------------

void AudioInput::write(Xml& xml) const
      {
      xml.tag("AudioInput");
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
//   activate1
//    register jack port for every channel of this track
//---------------------------------------------------------

void AudioInput::activate1()
      {
      for (int i = 0; i < channels(); ++i) {
            char buffer[128];
            snprintf(buffer, 128, "%s-%d", _name.toAscii().data(), i);
            if (jackPorts[i])
                  printf("AudioInput::activate(): already active!\n");
            else
                  jackPorts[i] = audioDriver->registerInPort(buffer);
            }
      }

//---------------------------------------------------------
//   activate2
//    connect all routes to jack; can only be done if
//    jack is activ running
//---------------------------------------------------------

void AudioInput::activate2()
      {
      if (audioState != AUDIO_RUNNING) {
            printf("AudioInput::activate2(): no audio running !\n");
            abort();
            }
      for (iRoute i = _inRoutes.begin(); i != _inRoutes.end(); ++i) {
            audioDriver->connect(i->port, jackPorts[i->channel]);
            }
      }

//---------------------------------------------------------
//   deactivate
//---------------------------------------------------------

void AudioInput::deactivate()
      {
      for (iRoute i = _inRoutes.begin(); i != _inRoutes.end(); ++i)
            audioDriver->disconnect(i->port, jackPorts[i->channel]);
      for (int i = 0; i < channels(); ++i) {
            if (jackPorts[i]) {
                  audioDriver->unregisterPort(jackPorts[i]);
                  jackPorts[i] = 0;
                  }
            else
                  printf("AudioOutput::deactivate(): not active!\n");
            }
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
            if (jackPorts[i]) {
                  char buffer[128];
                  snprintf(buffer, 128, "%s-%d", _name.toAscii().data(), i);
                  if (jackPorts[i])
                        audioDriver->setPortName(jackPorts[i], buffer);
                  }
            }
      }

//---------------------------------------------------------
//   collectInputData
//    return true if data
//---------------------------------------------------------

void AudioInput::collectInputData()
      {
      bufferEmpty = false;
      for (int ch = 0; ch < channels(); ++ch) {
            void* jackPort = jackPorts[ch];
            if (jackPort)
                  buffer[ch] = audioDriver->getBuffer(jackPort, segmentSize);
            else {
                  //TODO: this should crash
                  printf("NO JACK PORT\n");
                  memset(buffer[ch], 0, segmentSize * sizeof(float));
                  }
            }
      }

