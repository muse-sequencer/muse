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

#ifndef __MIDIFILE_H__
#define __MIDIFILE_H__

#include "globaldefs.h"
#include "midievent.h"

struct MPEventList;
class MidiEvent;

//---------------------------------------------------------
//   MidiFileTrack
//---------------------------------------------------------

struct MidiFileTrack {
      MPEventList events;
      bool isDrumTrack;
      MidiFileTrack() {
            isDrumTrack = false;
            }
      };

typedef std::list<MidiFileTrack*> MidiFileTrackList;
typedef MidiFileTrackList::iterator iMidiFileTrack;
typedef MidiFileTrackList::const_iterator ciMidiFileTrack;

//---------------------------------------------------------
//   MidiFile
//---------------------------------------------------------

class MidiFile {
      int _error;
      int format;       // smf file format
      int ntracks;      // number of midi tracks
      int _division;
      MidiInstrumentType _midiType;
      MidiFileTrackList* _tracks;

      int status, click;
      int sstatus;
      int lastport, lastchannel;
      QFile* fp;
      int curPos;

      bool read(void*, size_t);
      bool write(const void*, size_t);
      void put(unsigned char c) { write(&c, 1); }
      bool skip(size_t);
      int readShort();
      bool writeShort(int);
      int readLong();
      bool writeLong(int);
      int getvl();
      void putvl(unsigned);

      bool readTrack(MidiFileTrack*);
      bool writeTrack(const MidiFileTrack*);

      int readEvent(MidiEvent*, MidiFileTrack*);
      void writeEvent(const MidiEvent*);

   public:
      MidiFile();
      ~MidiFile();
      bool read(QFile* f);
      bool write(QFile* f);
      QString error();
      MidiFileTrackList* trackList()  { return _tracks; }
      int tracks() const              { return ntracks; }
      void setTrackList(MidiFileTrackList* tr) {
            _tracks = tr;
            }
      void setDivision(int d)             { _division = d;    }
      int division() const                { return _division; }
      void setFormat(int val)             { format = val;     }
      MidiInstrumentType midiType() const { return _midiType; }
      };

#define XCHG_SHORT(x) ((((x)&0xFF)<<8) | (((x)>>8)&0xFF))
#ifdef __i486__
#define XCHG_LONG(x) \
     ({ int __value; \
        asm ("bswap %1; movl %1,%0" : "=g" (__value) : "r" (x)); \
       __value; })
#else
#define XCHG_LONG(x) ((((x)&0xFF)<<24) | \
		      (((x)&0xFF00)<<8) | \
		      (((x)&0xFF0000)>>8) | \
		      (((x)>>24)&0xFF))
#endif

#if __BYTE_ORDER == __LITTLE_ENDIAN
#define BE_SHORT(x) XCHG_SHORT(x)
#define BE_LONG(x) XCHG_LONG(x)
#else
#define BE_SHORT(x) x
#define BE_LONG(x) x
#endif


#endif

