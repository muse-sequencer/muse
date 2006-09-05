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

#include "al/al.h"
#include "track.h"
#include "event.h"
#include "song.h"
#include "audio.h"
#include "wave.h"
#include "al/xml.h"
#include "plugin.h"
#include "driver/audiodev.h"
#include "gconfig.h"

//---------------------------------------------------------
//   AudioTrack
//---------------------------------------------------------

AudioTrack::AudioTrack(TrackType t)
   : Track(t)
      {
      _tt       = AL::FRAMES;
      _prefader = false;
      _efxPipe  = new Pipeline();
      _recFile  = 0;
      _channels = 0;
      bufferEmpty = false;
      setChannels(1);

      //
      // add two managed standard controller:
      //    volume and pan
      //
      Ctrl* c;
      c = new Ctrl(AC_VOLUME, "Volume");
      c->setType(Ctrl::INTERPOLATE | Ctrl::LOG);
      c->setRange(pow(10.0f, config.minSlider*0.05f), 2.0f);

      addController(c);
      c = new Ctrl(AC_PAN, "Pan");
      c->setRange(-1.0f, +1.0f);
      addController(c);

      buffer = new float*[MAX_CHANNELS];
      for (int i = 0; i < MAX_CHANNELS; ++i)
            buffer[i] = new float[segmentSize];
      }

//---------------------------------------------------------
//   ~AudioTrack
//---------------------------------------------------------

AudioTrack::~AudioTrack()
      {
      delete _efxPipe;
      for (int i = 0; i < MAX_CHANNELS; ++i) {
            if (buffer[i])
            	delete[] buffer[i];
            }
      delete[] buffer;
      }

//---------------------------------------------------------
//   newPart
//---------------------------------------------------------

Part* AudioTrack::newPart(Part*, bool /*clone*/)
      {
      return 0;
      }

//---------------------------------------------------------
//   addPlugin
//    idx = -1   insert into first free slot
//---------------------------------------------------------

void AudioTrack::addPlugin(PluginI* plugin, int idx)
      {
      if (plugin == 0) {
            PluginI* oldPlugin = (*_efxPipe)[idx];
            if (oldPlugin) {
                  int controller = oldPlugin->plugin()->parameter();
                  for (int i = 0; i < controller; ++i) {
                        int id = (idx + 1) * 0x1000 + i;
                        removeController(id);
                        }
                        _efxPipe->removeAt(idx);
                  }
            }
      if (idx == -1)
            idx = _efxPipe->size();

      if (plugin) {
            efxPipe()->insert(idx, plugin);
            int ncontroller = plugin->plugin()->parameter();
            for (int i = 0; i < ncontroller; ++i) {
                  int id = (idx + 1) * 0x1000 + i;
                  QString name(plugin->getParameterName(i));
                  float min, max;
                  plugin->range(i, &min, &max);
                  Ctrl* cl = getController(id);
                  //printf("Plugin name: %s id:%d\n",name.toAscii().data(), id);
                  if (cl == 0) {
                        cl = new Ctrl(id, name);
                        cl->setRange(min, max);
                        float defaultValue = plugin->defaultValue(i);
                        cl->setDefault(defaultValue);
                        cl->setCurVal(defaultValue);
                        addController(cl);
                        }
                  cl->setRange(min, max);
                  cl->setName(name);
                  plugin->setParam(i, cl->schedVal().f);
                  plugin->setControllerList(cl);
                  }
            }
      }

//---------------------------------------------------------
//   plugin
//---------------------------------------------------------

PluginI* AudioTrack::plugin(int idx) const
      {
      return (*_efxPipe)[idx];
      }

//---------------------------------------------------------
//   addAuxSend
//---------------------------------------------------------

void AudioTrack::addAuxSend(int n)
      {
      if (n >= NUM_AUX) {
            printf("too many aux sends (>%d)\n", n);
            n = NUM_AUX;
            }
      for (int i = 0; i < n; ++i) {
            Ctrl* c = getController(AC_AUX + i);
            if (c)
                  continue;
            QString s("AuxSend-");
            s += QString("%1").arg(i+1);
            Ctrl* ctrl = new Ctrl(AC_AUX + i, s);
            addController(ctrl);
            }
      }

//---------------------------------------------------------
//   AudioTrack::writeProperties
//---------------------------------------------------------

void AudioTrack::writeProperties(Xml& xml) const
      {
      Track::writeProperties(xml);
      xml.intTag("prefader", prefader());
      for (ciPluginI ip = _efxPipe->begin(); ip != _efxPipe->end(); ++ip) {
            if (*ip)
                  (*ip)->writeConfiguration(xml);
            }
      }

//---------------------------------------------------------
//   AudioTrack::readProperties
//---------------------------------------------------------

bool AudioTrack::readProperties(QDomNode node)
      {
      QDomElement e = node.toElement();
      QString tag(e.tagName());
      if (tag == "plugin") {
            PluginI* pi = new PluginI(this);
            if (pi->readConfiguration(node)) {
                  delete pi;
                  }
            else {
                  // insert plugin into first free slot
                  // of plugin rack
                  addPlugin(pi, -1);
                  }
            }
      else if (tag == "prefader")
            _prefader = e.text().toInt();
      else if (tag == "recfile")
            readRecfile(node.firstChild());
      else
            return Track::readProperties(node);
      return false;
      }

//---------------------------------------------------------
//   setAutoRead
//---------------------------------------------------------

void AudioTrack::setAutoRead(bool val)
      {
      if (_autoRead != val) {
            _autoRead = val;
            emit autoReadChanged(_autoRead);
            }
      }

//---------------------------------------------------------
//   setAutoWrite
//---------------------------------------------------------

void AudioTrack::setAutoWrite(bool val)
      {
      if (_autoWrite != val) {
            _autoWrite = val;
            emit autoWriteChanged(_autoWrite);
            }
      }

//---------------------------------------------------------
//   isMute
//---------------------------------------------------------

bool AudioTrack::isMute() const
      {
      if (_solo)
            return false;
      if (song->solo())
            return true;
      return _mute;
      }

//---------------------------------------------------------
//   setSolo
//---------------------------------------------------------

bool AudioTrack::setSolo(bool val)
      {
      if (Track::setSolo(val)) {
            if (mute())
                  resetMeter();
            return true;
            }
      return false;
      }

//---------------------------------------------------------
//   readRecfile
//---------------------------------------------------------

void AudioTrack::readRecfile(QDomNode node)
      {
      QString path;
      int channels = 2;
      int format   = SF_FORMAT_WAV | SF_FORMAT_PCM_16;
      while (!node.isNull()) {
            QDomElement e = node.toElement();
            QString tag(e.nodeName());
            if (tag == "path")
                  path = e.text();
            else if (tag == "channels")
                  channels = e.text().toInt();
            else if (tag == "format")
                  format = e.text().toInt();
            else
                  printf("MusE:readRecfile: unknown tag %s\n", e.tagName().toAscii().data());
            node = node.nextSibling();
            }
      if (QFile::exists(path)) {
            setRecFile(SndFile::getWave(path, true));
            }
      else {
            setRecFile(new SndFile(path));
            recFile()->setFormat(format, channels, AL::sampleRate);
            if (recFile()->openWrite()) {
                  fprintf(stderr, "create wave file(%s) failed: %s\n",
                     path.toAscii().data(), recFile()->strerror().toAscii().data());
                  delete _recFile;
                  _recFile = 0;
                  }
            }
      }

//---------------------------------------------------------
//   putFifo
//---------------------------------------------------------

void AudioTrack::putFifo(int channels, unsigned long n, float** bp)
      {
      if (fifo.put(channels, n, bp, audio->pos().frame())) {
            printf("AudioTrack(%s)::putFifo(): overrun\n", name().toAscii().data());
            }
      }

//---------------------------------------------------------
//   setMute
//---------------------------------------------------------

bool AudioTrack::setMute(bool f)
      {
      if (Track::setMute(f)) {
            resetAllMeter();
            return true;
            }
      return false;
      }

//---------------------------------------------------------
//   setOff
//---------------------------------------------------------

bool AudioTrack::setOff(bool val)
      {
      if (Track::setOff(val)) {
            resetAllMeter();
            return true;
            }
      return false;
      }

//---------------------------------------------------------
//   setPrefader
//---------------------------------------------------------

void AudioTrack::setPrefader(bool val)
      {
      _prefader = val;
      if (!_prefader && isMute())
            resetAllMeter();
      }

//---------------------------------------------------------
//   record
//	called from audio writeback task
//---------------------------------------------------------

void AudioTrack::record()
      {
      float* recBuffer[_channels];

      if (fifo.get(_channels, segmentSize, recBuffer)) {
            printf("AudioTrack(%s)::record():: fifo underflow\n",
               name().toAscii().data());
            return;
            }
      if (_recFile) {
            _recFile->write(_channels, recBuffer, segmentSize);
            }
      else {
            printf("AudioTrack(%s)::record(): no recFile\n",
               name().toAscii().data());
            }
      }

//---------------------------------------------------------
//   setChannels
//---------------------------------------------------------

void AudioTrack::setChannels(int n)
      {
      if (n > MAX_CHANNELS) {
            fprintf(stderr, "too many channels!\n");
            abort();
            }
      Track::setChannels(n);
      if (_efxPipe)
            _efxPipe->setChannels(n);
      }

//---------------------------------------------------------
//   startRecording
//---------------------------------------------------------

void AudioTrack::startRecording()
      {
	if (!recordFlag())
      	return;
      if (!_recFile)
            _recFile = SndFile::createRecFile(_channels);
      _recFile->openWrite();
      if (debugMsg)
            printf("AudioTrack::startRecording: create internal file %s\n",
               _recFile->finfo()->filePath().toAscii().data());
      }

//---------------------------------------------------------
//   process
//	this is called only once per cycle
//---------------------------------------------------------

void AudioTrack::process()
      {
      if (_off) {
            bufferEmpty = true;
            return;
            }
      collectInputData();

      //---------------------------------------------------
      // apply plugin chain
      //---------------------------------------------------

      _efxPipe->apply(channels(), segmentSize, buffer);

      //---------------------------------------------------
      //    metering
      //---------------------------------------------------

      double vol[2];
      double _volume = ctrlVal(AC_VOLUME).f;
    	if (_volume == 0.0 || bufferEmpty) {
            for (int i = 0; i < channels(); ++i)
                  setMeter(i, 0.0);
            }
      else {
	      double _pan = ctrlVal(AC_PAN).f;
      	vol[0]      = _volume * (1.0 - _pan);
	      vol[1]      = _volume * (1.0 + _pan);

		for (int i = 0; i < channels(); ++i) {
      		float* p = buffer[i];
	            float meter = 0.0;
      	      for (unsigned k = 0; k < segmentSize; ++k) {
            		double f = fabs(*p);
	                  if (f > meter)
      	            	meter = f;
				++p;
      	            }
	      	if (!_prefader)
      	            meter *= (vol[i] * (_mute ? 0.0 : 1.0));
	         	setMeter(i, meter);
      	      }
	      }
      }

//---------------------------------------------------------
//   multiplyAdd
//---------------------------------------------------------

void AudioTrack::multiplyAdd(int dstChannels, float** dstBuffer, int ctrl)
	{
      double _volume = ctrlVal(ctrl).f;
      if (_mute || bufferEmpty || _volume == 0.0)
            return;
      int srcChannels = channels();
      double vol[2];
      double _pan = ctrlVal(AC_PAN).f;
      vol[0] = _volume * (1.0 - _pan);
      vol[1] = _volume * (1.0 + _pan);

      if (srcChannels == dstChannels) {
            for (int c = 0; c < dstChannels; ++c) {
                  float* sp = buffer[c];
                  float* dp = dstBuffer[c];
                  float  v  = vol[c];
                  for (unsigned k = 0; k < segmentSize; ++k)
                        dp[k] += (sp[k] * v);
                  }
            }
      else if (srcChannels == 1 && dstChannels == 2) {
            for (int c = 0; c < dstChannels; ++c) {
                  float* dp = dstBuffer[c];
                  float* sp = buffer[0];
                  float v   = vol[c];
                  for (unsigned k = 0; k < segmentSize; ++k)
                        dp[k] += (sp[k] * v);
                  }
            }
      else if (srcChannels == 2 && dstChannels == 1) {
            float* sp1 = buffer[0];
            float* sp2 = buffer[1];
            float* dp = dstBuffer[0];
            for (unsigned k = 0; k < segmentSize; ++k)
                  dp[k] += (sp1[k] * vol[0] + sp2[k] * vol[1]);
            }
      }

//---------------------------------------------------------
//   multiplyCopy
//	return false if multiply by zero
//---------------------------------------------------------

bool AudioTrack::multiplyCopy(int dstChannels, float** dstBuffer, int ctrl)
	{
      double _volume = ctrlVal(ctrl).f;
      if (_mute || bufferEmpty || _volume == 0.0)
            return false;
      int srcChannels = channels();
      float vol[2];
      float _pan = ctrlVal(AC_PAN).f;
      vol[0] = _volume * (1.0 - _pan);
      vol[1] = _volume * (1.0 + _pan);

      if (srcChannels == dstChannels) {
            for (int c = 0; c < dstChannels; ++c) {
                  float* sp = buffer[c];
                  float* dp = dstBuffer[c];
                  for (unsigned k = 0; k < segmentSize; ++k) {
                        *dp++ = *sp++ * vol[c];
                        }
                  }
            }
      else if (srcChannels == 1 && dstChannels == 2) {
            float* sp = buffer[0];
            for (unsigned k = 0; k < segmentSize; ++k) {
                  float val = *sp++;
                  *(dstBuffer[0] + k) = val * vol[0];
                  *(dstBuffer[1] + k) = val * vol[1];
                  }
            }
      else if (srcChannels == 2 && dstChannels == 1) {
            float* sp1 = buffer[0];
            float* sp2 = buffer[1];
            float* dp = dstBuffer[0];
            for (unsigned k = 0; k < segmentSize; ++k) {
                  float val1 = *sp1++ * vol[0];
                  float val2 = *sp2++ * vol[1];
                  *dp++ = (val1 + val2);
                  }
            }
      return true;
      }

//---------------------------------------------------------
//   collectInputData
//---------------------------------------------------------

void AudioTrack::collectInputData()
      {
      bufferEmpty = false;
      RouteList* rl = inRoutes();
      bool copy = true;
      for (iRoute ir = rl->begin(); ir != rl->end(); ++ir) {
            AudioTrack* track = (AudioTrack*)ir->track;
            if (track->off() || song->bounceTrack == track)
                  continue;
            if (copy)
      		copy = !track->multiplyCopy(channels(), buffer);
            else
	            track->multiplyAdd(channels(), buffer);
            }
      if (copy) {
            //
            // no input,
            // fill with silence
            //
            for (int i = 0; i < channels(); ++i)
                  memset(buffer[i], 0, sizeof(float) * segmentSize);
            bufferEmpty = true;
	      }
      }

