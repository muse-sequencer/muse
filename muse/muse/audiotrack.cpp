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
#include "auxplugin.h"
#include "pipeline.h"
#include "driver/audiodev.h"
#include "gconfig.h"

//---------------------------------------------------------
//   AudioTrack
//---------------------------------------------------------

AudioTrack::AudioTrack()
   : Track()
      {
      _tt       = AL::FRAMES;
      _prefader = false;
      _prePipe  = new Pipeline();
      _postPipe = new Pipeline();
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
      c->setRange(pow(10.0f, config.minSlider*0.05f), pow(10.0f, config.maxSlider*0.05f));

      addController(c);
      c = new Ctrl(AC_PAN, "Pan");
      c->setRange(-1.0f, +1.0f);
      addController(c);

      for (int i = 0; i < MAX_CHANNELS; ++i)
            buffer[i] = new float[segmentSize];
      }

//---------------------------------------------------------
//   ~AudioTrack
//---------------------------------------------------------

AudioTrack::~AudioTrack()
      {
      foreach(PluginI* plugin, *_prePipe)
            delete plugin;
      foreach(PluginI* plugin, *_postPipe)
            delete plugin;
      delete _prePipe;
      delete _postPipe;
      for (int i = 0; i < MAX_CHANNELS; ++i) {
            if (buffer[i])
            	delete[] buffer[i];
            }
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
//    idx = -1     append
//    plugin = 0   remove plugin
//---------------------------------------------------------

void AudioTrack::addPlugin(PluginI* plugin, int idx, bool pre)
      {
      Pipeline* pipe = pre ? _prePipe : _postPipe;
      if (plugin == 0) {
            PluginI* oldPlugin = (*pipe)[idx];
            if (oldPlugin) {
                  int controller = oldPlugin->plugin()->parameter();
                  for (int i = 0; i < controller; ++i) {
                        int id = genACnum(idx, i, pre);
                        removeController(id);
                        }
                  pipe->removeAt(idx);
                  }
            }
      else {
            if (idx == -1)
                  idx = pipe->size();
            pipe->insert(idx, plugin);
            int ncontroller = plugin->plugin()->parameter();
            for (int i = 0; i < ncontroller; ++i) {
                  int id = genACnum(idx, i, pre);
                  QString name(plugin->getParameterName(i));
                  double min, max;
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
                  plugin->setParam(i, cl->curVal().f);
                  plugin->setControllerList(cl);
                  }
            }
      _preAux.clear();
      _postAux.clear();
      foreach(PluginI* pi, *_prePipe) {
            if (pi->plugin() == auxPlugin)
                  _preAux.append((AuxPluginIF*)(pi->pluginIF(0)));
            }
      foreach(PluginI* pi, *_postPipe) {
            if (pi->plugin() == auxPlugin)
                  _postAux.append((AuxPluginIF*)(pi->pluginIF(0)));
            }
      }

//---------------------------------------------------------
//   plugin
//---------------------------------------------------------

PluginI* AudioTrack::plugin(int idx, bool prefader) const
      {
      Pipeline* pipe = prefader ? _prePipe : _postPipe;
      return (*pipe)[idx];
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
            ctrl->setRange(pow(10.0f, config.minSlider*0.05f), pow(10.0f, config.maxSlider*0.05f));
            addController(ctrl);

            c = getController(AC_AUX_PAN + i);
            if (c)
                  continue;
            s = ("AuxSendPan-");
            s += QString("%1").arg(i+1);
            ctrl = new Ctrl(AC_AUX_PAN + i, s);
            ctrl->setRange(-1.0f, +1.0f);
            addController(ctrl);
            }
      }

//---------------------------------------------------------
//   AudioTrack::writeProperties
//---------------------------------------------------------

void AudioTrack::writeProperties(Xml& xml) const
      {
      Track::writeProperties(xml);
      foreach (PluginI* plugin, *_prePipe)
            plugin->writeConfiguration(xml, true);
      foreach (PluginI* plugin, *_postPipe)
            plugin->writeConfiguration(xml, false);
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
            bool prefader;
            if (pi->readConfiguration(node, &prefader)) {
                  delete pi;
                  }
            else {
                  // insert plugin into first free slot
                  // of plugin rack
                  addPlugin(pi, -1, prefader);
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
      if (fifo.put(channels, n, bp, audio->seqTime()->pos.frame())) {
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
      if (_prePipe)
            _prePipe->setChannels(n);
      if (_postPipe)
            _postPipe->setChannels(n);
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
      bufferEmpty = false;
      if (_off) {
            bufferEmpty = true;
            return;
            }
      collectInputData();
      _prePipe->apply(channels(), segmentSize, buffer);

      if (_prefader) {
		for (int i = 0; i < channels(); ++i) {
      		float* p = buffer[i];
	            float meter = 0.0;
      	      for (unsigned k = 0; k < segmentSize; ++k) {
            		double f = fabs(*p++);
	                  if (f > meter)
      	            	meter = f;
      	            }
	         	setMeter(i, meter);
      	      }
            }

      //
      // TODO: we can only handle 1 or 2 channels
      //
      double vol[2];
      double _volume = _mute ? 0.0 : ctrlVal(AC_VOLUME).f;
      double _pan    = ctrlVal(AC_PAN).f;
      vol[0]         = _volume * (1.0 - _pan);
	vol[1]         = _volume * (1.0 + _pan);

      for (int i = 0; i < channels(); ++i) {
            float* p = buffer[i];
            for (unsigned k = 0; k < segmentSize; ++k)
                  *p++ *= vol[i];
            }
      _postPipe->apply(channels(), segmentSize, buffer);

    	if (!_prefader) {
		for (int i = 0; i < channels(); ++i) {
      		float* p = buffer[i];
	            float meter = 0.0;
      	      for (unsigned k = 0; k < segmentSize; ++k) {
            		double f = fabs(*p);
	                  if (f > meter)
      	            	meter = f;
				++p;
      	            }
	         	setMeter(i, meter);
      	      }
	      }
      }

//---------------------------------------------------------
//   add
//    add audio buffer to track buffer
//---------------------------------------------------------

void AudioTrack::add(int srcChannels, float** srcBuffer)
	{
      int dstChannels   = channels();
      float** dstBuffer = buffer;

      if (srcChannels == dstChannels) {
            for (int c = 0; c < dstChannels; ++c) {
                  float* sp = srcBuffer[c];
                  float* dp = dstBuffer[c];
                  for (unsigned k = 0; k < segmentSize; ++k)
                        dp[k] += sp[k];
                  }
            }
      //
      // mix mono to stereo
      //
      else if (srcChannels == 1 && dstChannels == 2) {
            float* dp1 = dstBuffer[0];
            float* dp2 = dstBuffer[1];
            float* sp  = srcBuffer[0];
            for (unsigned k = 0; k < segmentSize; ++k) {
                  dp1[k] += sp[k];
                  dp2[k] += sp[k];
                  }
            }
      //
      // downmix stereo to mono
      //
      else if (srcChannels == 2 && dstChannels == 1) {
            float* sp1 = srcBuffer[0];
            float* sp2 = srcBuffer[1];
            float* dp  = dstBuffer[0];
            for (unsigned k = 0; k < segmentSize; ++k)
                  dp[k] += sp1[k] + sp2[k];
            }
      }

//---------------------------------------------------------
//   copy
//    add audio buffer to track buffer
//---------------------------------------------------------

bool AudioTrack::copy(int srcChannels, float** srcBuffer)
	{
      int dstChannels   = channels();
      float** dstBuffer = buffer;

      if (srcChannels == dstChannels) {
            for (int c = 0; c < dstChannels; ++c) {
                  float* sp = srcBuffer[c];
                  float* dp = dstBuffer[c];
                  for (unsigned k = 0; k < segmentSize; ++k)
                        *dp++ = *sp++;
                  }
            }
      else if (srcChannels == 1 && dstChannels == 2) {
            float* sp = srcBuffer[0];
            for (unsigned k = 0; k < segmentSize; ++k) {
                  float val = *sp++;
                  *(dstBuffer[0] + k) = val;
                  *(dstBuffer[1] + k) = val;
                  }
            }
      else if (srcChannels == 2 && dstChannels == 1) {
            float* sp1 = srcBuffer[0];
            float* sp2 = srcBuffer[1];
            float* dp = dstBuffer[0];
            for (unsigned k = 0; k < segmentSize; ++k)
                  dp[k] = sp1[k] + sp2[k];
            }
      return true;
      }

//---------------------------------------------------------
//   collectInputData
//    if buffer contains silence, set bufferEmpty to true
//---------------------------------------------------------

void AudioTrack::collectInputData()
      {
      bufferEmpty = false;
      bool copyFlag = true;
      foreach (const Route& r, _inRoutes) {
            float** ptr;
            int ch;
            if (r.src.type == RouteNode::TRACK) {
                  AudioTrack* track = (AudioTrack*)r.src.track;
                  if (track->off() || song->bounceTrack == track)
                        continue;
                  ptr = track->buffer;
                  ch  = track->channels();
                  }
            else if (r.src.type == RouteNode::AUXPLUGIN) {
                  ch  = r.src.plugin->channel();
                  ptr = r.src.plugin->buffer();
                  }
            else
                  printf("AudioTrack::collectInputRoutes(): bad route type\n");
            if (copyFlag) {
                  copy(ch, ptr);
                  copyFlag = false;
                  }
            else
	            add(ch, ptr);
            }
      if (copyFlag) {
            //
            // no input,
            // fill with silence
            //
            for (int i = 0; i < channels(); ++i)
                  memset(buffer[i], 0, sizeof(float) * segmentSize);
            bufferEmpty = true;
	      }
      }

