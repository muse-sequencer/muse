//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: waveevent.cpp,v 1.9.2.6 2009/12/20 05:00:35 terminator356 Exp $
//
//  (C) Copyright 2000-2003 Werner Schweer (ws@seh.de)
//
//  This program is free software; you can redistribute it and/or
//  modify it under the terms of the GNU General Public License
//  as published by the Free Software Foundation; version 2 of
//  the License, or (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//
//=========================================================

#include "audio_convert/audio_converter_settings_group.h"
#include "audio_convert/audio_converter_plugin.h"
#include "gconfig.h"
#include "time_stretch.h"

#include "globals.h"
#include "waveevent.h"
#include "xml.h"
#include "wave.h"
#include "part.h"
#include "wave_helper.h"
#include "audio_fifo.h"

#include <iostream>
#include "muse_math.h"

#include <QDir>
#include <QFileInfo>
#include <QLatin1Char>

// For debugging output: Uncomment the fprintf section.
#define WAVEEVENT_DEBUG(dev, format, args...) // fprintf(dev, format, ##args)
#define WAVEEVENT_DEBUG_PRC(dev, format, args...) // fprintf(dev, format, ##args)

namespace MusECore {

//---------------------------------------------------------
//   WaveEvent
//---------------------------------------------------------

WaveEventBase::WaveEventBase(EventType t)
   : EventBase(t)
      {
      _spos = 0;
      _prefetchFifo = new Fifo();
      _prefetchWritePos = ~0;
      _lastSeekPos  = ~0;
      }

WaveEventBase::WaveEventBase(const WaveEventBase& ev, bool duplicate_not_clone)
   : EventBase(ev, duplicate_not_clone)
{
      _name = ev._name;
      _spos = ev._spos;

      // Make a new Fifo.
      _prefetchFifo = new Fifo();
      _prefetchWritePos = ~0;
      _lastSeekPos  = ~0;
      //*_audioConverterSettings = *ev._audioConverterSettings; // TODO? Or just keep the existing settings?

      // NOTE: It is necessary to create copies always. Unlike midi events, no shared data is allowed for 
      //        wave events because sndfile handles and audio stretchers etc. ABSOLUTELY need separate instances always. 
      //       So duplicate_not_clone is not used here. 
      if(!ev.f.isNull() && !ev.f.canonicalPath().isEmpty())
      {
        // Don't show error box, and assign the audio converter settings and stretch list.
        f = sndFileGetWave(ev.f.canonicalPath(), !ev.f.isWritable(), ev.f.isOpen(),
                    false, ev.f.audioConverterSettings(), ev.f.stretchList());
      }
}

WaveEventBase::~WaveEventBase()
{
  delete _prefetchFifo;
}

//---------------------------------------------------------
//   assign
//---------------------------------------------------------

void WaveEventBase::assign(const EventBase& ev)  
{
  if(ev.type() != type())
    return;
  EventBase::assign(ev);

  _name = ev.name();
  _spos = ev.spos();

  SndFileR sf = ev.sndFile();
  setSndFile(sf);
  
  _prefetchWritePos = ~0;
  _lastSeekPos  = ~0;
}

bool WaveEventBase::isSimilarTo(const EventBase& other_) const
{
	const WaveEventBase* other = dynamic_cast<const WaveEventBase*>(&other_);
	if (other==nullptr) // dynamic cast hsa failed: "other_" is not of type WaveEventBase.
		return false;
	
	return f.dirPath()==other->f.dirPath() && _spos==other->_spos && this->PosLen::operator==(*other);
}

//---------------------------------------------------------
//   WaveEvent::mid
//---------------------------------------------------------

EventBase* WaveEventBase::mid(unsigned b, unsigned e) const
      {
      // We want a duplicate with a new id always, not a clone here.
      WaveEventBase* ev = new WaveEventBase(*this, true);
      unsigned fr = frame();
      unsigned start = fr - b;
      if(b > fr)
      {
        start = 0;
        ev->setSpos(spos() + b - fr);
      }
      unsigned end = endFrame();

      if (e < end)
            end = e;

      ev->setFrame(start);
      ev->setLenFrame(end - b - start);
      return ev;
      }

//---------------------------------------------------------
//   dump
//---------------------------------------------------------

void WaveEventBase::dump(int n) const
      {
      EventBase::dump(n);
      }

//---------------------------------------------------------
//   WaveEventBase::read
//---------------------------------------------------------

void WaveEventBase::read(Xml& xml)
      {
      StretchList sl;
      AudioConverterSettingsGroup settings(true); // Local non-default settings.
      settings.populate(&MusEGlobal::audioConverterPluginList, true);  // Local non-default settings.
      QString filename;
      for (;;) {
            Xml::Token token = xml.parse();
            const QString& tag = xml.s1();
            switch (token) {
                  case Xml::Error:
                  case Xml::End:
                  case Xml::Attribut:
                        return;
                  case Xml::TagStart:
                        if (tag == "poslen")
                              PosLen::read(xml, "poslen");
                        else if (tag == "frame")
                              _spos = xml.parseInt();
                        else if (tag == "file") {
                              filename = xml.parse1();
                              }
                        else if (tag == "stretchlist")
                        {
                          sl.read(xml);
                        }
                        else if (tag == "audioConverterSettingsGroup")
                        {
                          settings.read(xml, &MusEGlobal::audioConverterPluginList);
                        }
                        
                        else
                              xml.unknown("Event");
                        break;
                  case Xml::TagEnd:
                        if (tag == "event") {
                              Pos::setType(FRAMES);   // DEBUG
                              
                              if(!filename.isEmpty())
                              {
                                SndFileR wf = sndFileGetWave(filename, true, true, true, &settings, &sl);
                                if(wf) 
                                  setSndFile(wf);
                              }
                              
                              return;
                              }
                  default:
                        break;
                  }
            }
      }

//---------------------------------------------------------
//   isSameOrSubDir
//    True if dir is base or a subdirectory of base. Both are
//    expected to be clean absolute paths. Compares as path
//    prefixes, not substrings.
//---------------------------------------------------------

static bool isSameOrSubDir(const QString& dir, const QString& base)
      {
      if (base.isEmpty())
            return false;
      if (dir == base)
            return true;
      // Avoid producing "//..." when base is the filesystem root.
      if (base == QLatin1String("/"))
            return dir.startsWith(base);
      return dir.startsWith(base + QLatin1Char('/'));
      }

//---------------------------------------------------------
//   write
//---------------------------------------------------------

void WaveEventBase::write(int level, Xml& xml, const Pos& offset, bool forcePath) const
      {
      if (f.isNull())
            return;
      xml.tag(level++, "event");
      PosLen wpos(*this);
      wpos += offset;
      wpos.write(level, xml, "poslen");
      xml.intTag(level, "frame", _spos);  // offset in wave file

      //
      // Waves in the project directory (or a subdirectory of it) are stored
      // with a relative path name, others with an absolute path.
      // Compare as path prefixes, not substrings, and also compare the
      // symlink-resolved forms, otherwise a project directory reached through
      // a symlink produces mangled paths (issue 1102).
      //
      QString fileName = f.path();
      if (!forcePath) {
            const QString projPath  = QDir::cleanPath(MusEGlobal::museProject);
            const QString projCanon = QFileInfo(MusEGlobal::museProject).canonicalFilePath();

            const QString absDir   = QDir::cleanPath(f.dirPath());
            const QString canonDir = f.canonicalDirPath();

            if (isSameOrSubDir(absDir, projPath))
                  fileName = QDir(projPath).relativeFilePath(f.path());
            else if (!projCanon.isEmpty() && !canonDir.isEmpty() && isSameOrSubDir(canonDir, projCanon))
                  fileName = QDir(projCanon).relativeFilePath(f.canonicalPath());
            }
      xml.strTag(level, "file", fileName);
      
      if(f.stretchList())
        f.stretchList()->write(level, xml);
      
      if(f.audioConverterSettings())
        f.audioConverterSettings()->write(level, xml, &MusEGlobal::audioConverterPluginList);
      
      xml.etag(--level, "event");
      }

//---------------------------------------------------------
//   seekAudio
//---------------------------------------------------------

void WaveEventBase::seekAudio(sf_count_t frame)
{
  WAVEEVENT_DEBUG_PRC(stderr, "WaveEventBase::seekAudio frame:%lu\n", frame);

  if(!f.isNull())
  {
    f.seekConverted(frame, SEEK_SET, _spos);
  }
}
      
void WaveEventBase::readAudio(unsigned frame, float** buffer, int channel, int n, bool /*doSeek*/, bool overwrite)
{
  WAVEEVENT_DEBUG_PRC(stderr, "WaveEventBase::readAudio frame:%u channel:%d n:%d overwrite:%d\n", frame, channel, n, overwrite);
  
  if(!f.isNull())
  {  
    f.readConverted(frame, channel, buffer, n, overwrite);
  }
  return;
}
      
//---------------------------------------------------------
//   prefetchAudio
//---------------------------------------------------------

void WaveEventBase::prefetchAudio(Part* part, sf_count_t frames)
{        
  Fifo* fifo = audioPrefetchFifo();
  
  if(!fifo)
    return;
  
  SndFileR sf = sndFile();
  if(sf.isNull())
    return;

  const sf_count_t p_spos  = part->frame();
  const sf_count_t p_epos  = p_spos + part->lenFrame();

  const sf_count_t e_spos  = frame() + p_spos;
  sf_count_t nn            = lenFrame();
  const sf_count_t e_epos  = e_spos + nn;
  
  if(_prefetchWritePos + frames >= p_spos && _prefetchWritePos < p_epos &&
     _prefetchWritePos + frames >= e_spos && _prefetchWritePos < e_epos)
  {  
    const sf_count_t offset = e_spos - _prefetchWritePos;

    if(offset > 0) 
    {
      nn = frames - offset;
    }
    else 
    {
      nn += offset;
      if(nn > frames)
        nn = frames;
    }
  
    const int chans = sf.channels();
    const sf_count_t  samples = chans * frames;
    
    float* bp;
    
    // Here we allocate ONE interleaved buffer which is fed with direct interleaved soundfile data.
    if(fifo->getWriteBuffer(1, samples, &bp, _prefetchWritePos))
      return;

    // Clear the buffer.  TODO: Optimize this by only clearing what's required, and merge with the denormal code below.
    memset(bp, 0, samples * sizeof(float));

    sf.readDirect(bp, nn);
    
    // Add denormal bias to outdata.
    if(MusEGlobal::config.useDenormalBias) 
    {
      for(sf_count_t i = 0; i < samples; ++i)
        bp[i] += MusEGlobal::denormalBias;
    }
    
    // Increment the prefetch buffer to a new position.
    fifo->add();
    _prefetchWritePos += nn;
  }
}

} // namespace MusECore
