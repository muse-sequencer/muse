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

#include "wavetrack.h"
#include "event.h"
#include "audio.h"
#include "wave.h"
#include "al/xml.h"
#include "song.h"
#include "globals.h"
#include "part.h"
#include "audiowriteback.h"
#include "muse.h"
#include "audioprefetch.h"

bool WaveTrack::firstWaveTrack = true;

//---------------------------------------------------------
//   WaveTrack
//---------------------------------------------------------

WaveTrack::WaveTrack()
   : AudioTrack()
      {
      //
      // allocate prefetch buffer
      //
      float* p   = new float[FIFO_BUFFER * segmentSize * MAX_CHANNELS];
      float** pp = new float*[FIFO_BUFFER * MAX_CHANNELS];

      for (int i = 0; i < FIFO_BUFFER; ++i) {
        	readBuffer[i] = pp;
            for (int k = 0; k < MAX_CHANNELS; ++k) {
            	*pp = p;
                  p += segmentSize;
                  ++pp;
                  }
            }

      recordPart = 0;      // part we are recording into
      }

//---------------------------------------------------------
//   WaveTrack
//---------------------------------------------------------

WaveTrack::~WaveTrack()
      {
	delete[] *readBuffer[0];
	delete[] readBuffer[0];
      }

//---------------------------------------------------------
//   fetchData
//    called from prefetch thread
//---------------------------------------------------------

void WaveTrack::fetchData(unsigned pos, unsigned samples, int widx)
      {
      float** bp = readBuffer[widx];
      for (int i = 0; i < channels(); ++i)
            memset(bp[i], 0, samples * sizeof(float));

      PartList* pl = parts();
      for (iPart ip = pl->begin(); ip != pl->end(); ++ip) {
            Part* part = ip->second;

            if (part->mute())
                  continue;
            unsigned p_spos = part->frame();
//            if (pos + samples < p_spos)
            if (pos + samples <= p_spos)
                  break;
            unsigned p_epos = p_spos + part->lenFrame();
            if (pos >= p_epos)
                  continue;

            EventList* events = part->events();
            for (iEvent ie = events->begin(); ie != events->end(); ++ie) {
                  Event& event    = ie->second;

                  unsigned e_spos = event.frame() + p_spos;
                  // if (pos + samples < e_spos)
                  if (pos + samples <= e_spos)
                        break;
                  unsigned nn     = event.lenFrame();
                  unsigned e_epos = e_spos + nn;
                  if (pos >= e_epos)
                        continue;

                  int offset = e_spos - pos;

                  unsigned srcOffset, dstOffset;
                  if (offset > 0) {
                        nn = samples - offset;
                        srcOffset = 0;
                        dstOffset = offset;
                        }
                  else {
                        srcOffset = -offset;
                        dstOffset = 0;
                        nn -= offset;
                        if (nn > samples)
                              nn = samples;
                        }
                  if (dstOffset) {
                        float* bpp[channels()];
                        for (int i = 0; i < channels(); ++i)
                              bpp[i] = bp[i] + dstOffset;
                        event.read(srcOffset, bpp, channels(), nn);
                        }
                  else
                        event.read(srcOffset, bp, channels(), nn);
                  }
            }
      }

//---------------------------------------------------------
//   write
//---------------------------------------------------------

void WaveTrack::write(Xml& xml) const
      {
      xml.stag("wavetrack");
      AudioTrack::writeProperties(xml);
      const PartList* pl = parts();
      for (ciPart p = pl->begin(); p != pl->end(); ++p)
            p->second->write(xml);
      xml.etag("wavetrack");
      }

//---------------------------------------------------------
//   read
//---------------------------------------------------------

void WaveTrack::read(QDomNode node)
      {
      for (; !node.isNull(); node = node.nextSibling()) {
            QDomElement e = node.toElement();
            if (e.tagName() == "part") {
                  Part* p = newPart();
                  p->ref();
                  p->read(node);
                  parts()->add(p);
                  }
            else if (AudioTrack::readProperties(node))
                  printf("MusE:WaveTrack: unknown tag %s\n", e.tagName().toLatin1().data());
            }
      }

//---------------------------------------------------------
//   newPart
//---------------------------------------------------------

Part* WaveTrack::newPart(Part*p, bool clone)
      {
      Part* part = new Part(this);
      if (p) {
            if (clone)
                  part->clone(p->events());
            part->setName(p->name());
            part->setColorIndex(p->colorIndex());

            *(AL::PosLen*)part = *(AL::PosLen*)p;
            part->setMute(p->mute());
            }
      return part;
      }

//---------------------------------------------------------
//   setChannels
//---------------------------------------------------------

void WaveTrack::setChannels(int n)
      {
      AudioTrack::setChannels(n);
      SndFile* sf = recFile();
      if (sf) {
            if (sf->samples() == 0) {
                  sf->remove();
                  sf->setFormat(sf->format(), _channels,
                     sf->samplerate());
                  sf->openWrite();
                  }
            }
      }

//---------------------------------------------------------
//   startRecording
//    gui context
//---------------------------------------------------------

void WaveTrack::startRecording()
      {
      AudioTrack::startRecording();	// create file
      partCreated = false;
      AL::Pos start = song->punchin() ? song->lPos() : song->cPos();

      // search for recordPart
      recordPart = 0;
      for (iPart ip = parts()->begin(); ip != parts()->end(); ++ip) {
            Part* part = ip->second;
            unsigned partStart = part->tick();
            unsigned partEnd   = partStart + part->lenTick();
            if (start.tick() >= partStart && start.tick() < partEnd) {
                  recordPart = part;
                  break;
                  }
            }
      if (recordPart == 0) {
            //
            // create new part for recording
            //
            recordPart    = new Part(this);
            recordPart->setType(AL::FRAMES);
            Pos spos(start.downSnaped(muse->raster()));
            Pos epos(start.upSnaped(muse->raster()));
            recordPart->setPos(spos);
            recordPart->setLenTick(epos.tick() - spos.tick());
            recordPart->setName(name());
            song->addPart(recordPart);
            partCreated = true;
            emit partsChanged();
            }
      }

//---------------------------------------------------------
//   recordBeat
//    gui context
//    update current recording
//---------------------------------------------------------

void WaveTrack::recordBeat()
      {
      if (partCreated) {
            int cpos  = song->cpos();
            int ptick = recordPart->tick();
            recordPart->setLenTick(cpos - ptick);
            song->addUpdateFlags(SC_PART_MODIFIED);
            }
      }

//---------------------------------------------------------
//   stopRecording
//    gui context
//---------------------------------------------------------

void WaveTrack::stopRecording(const Pos& s, const Pos& e)
      {
      int updateFlags = 0;

	while(audioWriteback->active())
            sleep(1);
      _recFile->update();

      // create Event
      Event event(Wave);
      SndFileR sf(_recFile);
      event.setSndFile(sf);
      event.setPos(s - *recordPart);
      event.setSpos(0);
      event.setLenFrame(e.frame() - s.frame());
      // recordPart->addEvent(event);
      audio->msgAddEvent(event, recordPart, false);

      _recFile = 0;

      //
      // adjust part len && song len
      //
      unsigned etick = e.upSnaped(muse->raster()).tick();
      unsigned len   = etick - recordPart->tick();

      if (recordPart->lenTick() < len) {
            recordPart->setLenTick(len);
            updateFlags |= SC_PART_MODIFIED;
            }

      if (song->len() < etick)
            song->setLen(etick);
      song->update(updateFlags);
      }

//---------------------------------------------------------
//   canEnableRecord
//---------------------------------------------------------

bool WaveTrack::canEnableRecord() const
      {
      return  (!noInRoute() || (this == song->bounceTrack));
      }

//---------------------------------------------------------
//   collectInputData
//---------------------------------------------------------

void WaveTrack::collectInputData()
	{
      bufferEmpty = false;
      if (recordFlag() && (audio->isRecording() || !audio->isPlaying())) {
      	if (song->bounceTrack == this && audio->isPlaying()) {
      		OutputList* ol = song->outputs();
            	if (!ol->empty()) {
//TODO                        AudioOutput* out = ol->front();
                        }
            	}
            else
      		AudioTrack::collectInputData();
            return;
            }
      if (!audio->isPlaying()) {
		for (int i = 0; i < channels(); ++i)
      		memset(buffer[i], 0, sizeof(float) * segmentSize);
            return;
            }

      if (audio->freewheel()) {
      	// when freewheeling, read data direct from file:
            // TODO: fetchData(framePos, segmentSize, buffer);
            }
	else {
            int idx = audio->curReadIndex();
            if (idx == -1) {
      	      for (int i = 0; i < channels(); ++i)
            	      memset(buffer[i], 0, sizeof(float) * segmentSize);
                  }
            else {
	            float** bpp = readBuffer[idx];
      	      for (int i = 0; i < channels(); ++i)
            	      memcpy(buffer[i], bpp[i], sizeof(float) * segmentSize);
                  }
            }
      }

//---------------------------------------------------------
//   process
//---------------------------------------------------------

void WaveTrack::process()
      {
	AudioTrack::process();

      //
      // record collected data for track
      //
	if (recordFlag() && audio->isRecording() && recFile()) {
		unsigned framePos = audio->pos().frame();
      	if (audio->freewheel()) {
                  // write data directly to file
                  _recFile->write(channels(), buffer, framePos);
                  }
		else {
			if (fifo.put(channels(), segmentSize, buffer, framePos)) {
                  	printf("WaveTrack(%s)::getData: fifo overrun\n",
                        name().toLatin1().data());
                        }
                  }
            if (!_monitor) {
                  //
                  // end data processing here if monitor is not on
                  //
                  bufferEmpty = true;
                  }
            return;
            }
      if (!audio->isPlaying() && !_monitor)
            bufferEmpty = true;
      }

//---------------------------------------------------------
//   clone
//---------------------------------------------------------

void WaveTrack::clone(WaveTrack* t)
      {
      QString name;
      for (int i = 1; ; ++i) {
            name.sprintf("%s-%d", t->name().toLatin1().data(), i);
            TrackList* tl = song->tracks();
            bool found = false;
            for (iTrack it = tl->begin(); it != tl->end(); ++it) {
                  if ((*it)->name() == name) {
                        found = true;
                        break;
                        }
                  }
            if (!found)
                  break;
            }
      setName(name);

      _recordFlag   = t->_recordFlag;
      _mute         = t->_mute;
      _solo         = t->_solo;
      _off          = t->_off;
      _monitor      = t->_monitor;
      _channels     = t->_channels;
      _locked       = t->_locked;
      _inRoutes     = t->_inRoutes;
      _outRoutes    = t->_outRoutes;
      _controller   = t->_controller;
      _autoRead     = t->_autoRead;
      _autoWrite    = t->_autoWrite;
      setPrefader(t->prefader());
      }


