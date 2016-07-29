//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: waveevent.h,v 1.6.2.4 2009/12/20 05:00:35 terminator356 Exp $
//
//  (C) Copyright 1999-2004 Werner Schweer (ws@seh.de)
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

#ifndef __WAVE_EVENT_H__
#define __WAVE_EVENT_H__

#include <sys/types.h>

#include "eventbase.h"

// REMOVE Tim. samplerate. Added.
#include "muse_time.h"

// REMOVE Tim. samplerate. Added.
//class AudioConverter;

namespace MusECore {

// REMOVE Tim. samplerate. Changed.
//class WavePart;
class Part;
// REMOVE Tim. samplerate. Added.
class Fifo;
//class AudioConverterSettingsGroup;

//---------------------------------------------------------
//   WaveEvent
//---------------------------------------------------------

class WaveEventBase : public EventBase {
      QString _name;
      SndFileR f;
      int _spos;            // start sample position in WaveFile
      // REMOVE Tim. samplerate. Added.
      //AudioConverter* _audConv;
      //StretchList* _stretchList;
      Fifo* _prefetchFifo;            // Prefetch Fifo
      MuseCount_t _prefetchWritePos;  // Current fifo write position.
      MuseCount_t _lastSeekPos;       // Remember last seek to optimize seeks.
      //AudioConverterSettingsGroup* _audioConverterSettings;

      // Creates a non-shared clone (copies event base), including the same 'group' id.
      virtual EventBase* clone() const { return new WaveEventBase(*this); }
      // Creates a copy of the event base, excluding the 'group' _id. 
      virtual EventBase* duplicate() const { return new WaveEventBase(*this, true); } 

   public:
      WaveEventBase(EventType t);
      // Creates a non-shared clone with same id, or duplicate with unique id, and 0 ref count and invalid Pos sn. 
      WaveEventBase(const WaveEventBase& ev, bool duplicate_not_clone = false);
// REMOVE Tim. samplerate. Changed.
//       virtual ~WaveEventBase() {}
      virtual ~WaveEventBase();
      
      virtual void assign(const EventBase& ev); // Assigns to this event, excluding the _id. 
      
      virtual bool isSimilarTo(const EventBase& other) const;

      virtual void read(Xml&);
      virtual void write(int, Xml&, const Pos& offset, bool forcePath = false) const;
      virtual EventBase* mid(unsigned, unsigned) const;
      
      virtual void dump(int n = 0) const;

      virtual const QString name() const       { return _name;  }
      virtual void setName(const QString& s)   { _name = s;     }
      virtual int spos() const                 { return _spos;  }
      virtual void setSpos(int s)              { _spos = s;     }
      virtual SndFileR sndFile() const         { return f;      }
// REMOVE Tim. samplerate. Changed.
      virtual void setSndFile(SndFileR& sf)    { f = sf;        }
      //virtual void setSndFile(SndFileR& sf);
      
// REMOVE Tim. samplerate. Changed.
//       virtual void readAudio(WavePart* part, unsigned offset, 
      virtual void readAudio(unsigned offset, float** bpp, int channels, int nn, bool doSeek, bool overwrite);
// REMOVE Tim. samplerate. Added.
      virtual void seekAudio(sf_count_t offset);
      //virtual void clearPrefetchFifo();
      virtual Fifo* audioPrefetchFifo()        { return _prefetchFifo; }
      //virtual void prefetchAudio(WavePart* part, sf_count_t writePos, int channels, bool off, sf_count_t frames);
      virtual void prefetchAudio(Part* part, sf_count_t frames);
      //virtual void fetchAudioData(WavePart* part, sf_count_t pos, int channels, bool off, sf_count_t frames, float** bp, bool doSeek, bool overwrite);
      //virtual bool getAudioPrefetchBuffer(int segs, unsigned long samples, float** dst, unsigned* pos);
      //virtual StretchList* stretchList() const { return _stretchList; }
      };
      
} // namespace MusECore

#endif

