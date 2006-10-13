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

#include "audiooutput.h"
#include "driver/jackaudio.h"
#include "audio.h"
#include "globals.h"
#include "song.h"
#include "ticksynth.h"

//---------------------------------------------------------
//   AudioOutput
//---------------------------------------------------------

AudioOutput::AudioOutput()
   : AudioTrack(AUDIO_OUTPUT)
      {
      for (int i = 0; i < MAX_CHANNELS; ++i)
            jackPorts[i] = 0;
      _channels = 0;
      setChannels(2);

      //
      // buffers are allocated from AudioTrack()
      // and not needed by AudioOutput which uses
      // the JACK supplied buffers

      for (int i = 0; i < MAX_CHANNELS; ++i) {
            if (buffer[i]) {
            	delete[] buffer[i];
                  buffer[i] = 0;
                  }
            }
      }

//---------------------------------------------------------
//   ~AudioOutput
//---------------------------------------------------------

AudioOutput::~AudioOutput()
      {
      for (int i = 0; i < _channels; ++i) {
            if (jackPorts[i])
                  audioDriver->unregisterPort(jackPorts[i]);
            }
      // AudioOutput does not own buffers (they are from JACK)
      // make sure ~AudioTrack() does not delete them:
      for (int i = 0; i < MAX_CHANNELS; ++i)
            buffer[i] = 0;
      }

//---------------------------------------------------------
//   write
//---------------------------------------------------------

void AudioOutput::write(Xml& xml) const
      {
      xml.tag("AudioOutput");
      AudioTrack::writeProperties(xml);
      xml.etag("AudioOutput");
      }

//---------------------------------------------------------
//   read
//---------------------------------------------------------

void AudioOutput::read(QDomNode node)
      {
      while (!node.isNull()) {
            AudioTrack::readProperties(node);
            node = node.nextSibling();
            }
      setName(name());  // allocate jack ports
      }

//---------------------------------------------------------
//   setChannels
//---------------------------------------------------------

void AudioOutput::setChannels(int n)
      {
      if (n == _channels)
            return;
      AudioTrack::setChannels(n);
      }

//---------------------------------------------------------
//   silence
//---------------------------------------------------------

void AudioOutput::silence(unsigned n)
      {
      for (int i = 0; i < channels(); ++i) {
            if (jackPorts[i])
                  buffer[i] = audioDriver->getBuffer(jackPorts[i], n);
            else {
                  printf("PANIC: silence(): no buffer from audio driver\n");
                  abort();
                  return;
                  }
            }
      for (int i = 0; i < channels(); ++i)
            memset(buffer[i], 0, n * sizeof(float));
      }

//---------------------------------------------------------
//   setName
//---------------------------------------------------------

void AudioOutput::setName(const QString& s)
      {
      Track::setName(s);
      for (int i = 0; i < channels(); ++i) {
            if (jackPorts[i]) {
                  char buffer[128];
                  snprintf(buffer, 128, "%s-%d", _name.toAscii().data(), i);
                  audioDriver->setPortName(jackPorts[i], buffer);
                  }
            }
      }

//---------------------------------------------------------
//   activate1
//---------------------------------------------------------

void AudioOutput::activate1()
      {
      for (int i = 0; i < channels(); ++i) {
            char buffer[128];
            snprintf(buffer, 128, "%s-%d", _name.toAscii().data(), i);
            if (jackPorts[i]) {
                  printf("AudioOutput::activate(): already active!\n");
                  }
            else
                  jackPorts[i] = audioDriver->registerOutPort(QString(buffer));
            }
      }

//---------------------------------------------------------
//   activate2
//---------------------------------------------------------

void AudioOutput::activate2()
      {
      if (audioState != AUDIO_RUNNING) {
            printf("AudioOutput::activate2(): no audio running !\n");
            abort();
            }
      for (iRoute i = _outRoutes.begin(); i != _outRoutes.end(); ++i) {
            i->port = audioDriver->findPort(i->name());
            audioDriver->connect(jackPorts[i->channel], i->port);
            }
      }

//---------------------------------------------------------
//   deactivate
//---------------------------------------------------------

void AudioOutput::deactivate()
      {
      for (iRoute i = _outRoutes.begin(); i != _outRoutes.end(); ++i) {
            Route r = *i;
            audioDriver->disconnect(jackPorts[i->channel], i->port);
            }
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
//   stopRecording
//    gui context
//---------------------------------------------------------

void AudioOutput::stopRecording(const Pos& /*s*/, const Pos& /*e*/)
      {
      SndFile* sf = recFile();
      if (sf)
         delete sf;              // close
      setRecFile(0);
      setRecordFlag(false);
      }

//---------------------------------------------------------
//   process
//    synthesize "n" frames at buffer offset "offset"
//    current frame position is "pos"
//---------------------------------------------------------

void AudioOutput::process()
      {
      for (int c = 0; c < channels(); ++c)
            buffer[c] = audioDriver->getBuffer(jackPorts[c], segmentSize);

      AudioTrack::process();

      int n = segmentSize;
      if (audio->isRecording() && recordFlag() && _recFile) {
            // bounce to file
            if (audio->freewheel())
                  _recFile->write(channels(), buffer, n);
            else
                  putFifo(channels(), n, buffer);
            }

#if 0
      if (audioClickFlag && song->click() && metronome) {
            float b[n];
            float* bp[1];
            bp[0] = b;
            metronome->getData(pos, 1, n, bp);
            for (unsigned i = 0; i < n; ++i) {
                  for (int k = 0; k < channels(); ++k)
                        buffer[k][i] += b[i] * audioClickVolume;
                  }
            }
#endif
      }

