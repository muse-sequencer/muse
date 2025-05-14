//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: midifile.h,v 1.3 2004/01/04 18:24:43 wschweer Exp $
//
//  (C) Copyright 1999-2004 Werner Schweer (ws@seh.de)
//  (C) Copyright 2012 Tim E. Real (terminator356 on users dot sourceforge dot net)
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

#ifndef __MIDIFILE_H__
#define __MIDIFILE_H__

#include <QtGlobal>
#include <QString>

#include <stdio.h>
#include <list>

#include "globaldefs.h"
#include "mpevent.h"
#include "libs/file/file.h"

namespace MusECore {

class MPEventList;
class MidiPlayEvent;
class MidiInstrument;

//---------------------------------------------------------
//   MidiFileTrack
//---------------------------------------------------------

struct MidiFilePort {
  bool _isStandardDrums; 
  MType _midiType;
  QString _instrName;
  QString _subst4DevName;
  MidiFilePort() {
    _midiType = MT_UNKNOWN;
    _isStandardDrums = false;
  }
};


typedef std::map<int, MidiFilePort> MidiFilePortMap;
typedef MidiFilePortMap::iterator iMidiFilePort;
typedef MidiFilePortMap::const_iterator ciMidiFilePort;

//---------------------------------------------------------
//   MidiFileTrack
//---------------------------------------------------------

struct MidiFileTrack {
      MPEventList events;
      bool _isDrumTrack;
      MidiFileTrack() {
            _isDrumTrack = false;
            }
      };

class MidiFileTrackList : public std::list<MidiFileTrack*>
{
  public:
    void clearDelete();
};
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
      // False: division is standard ticks based musical time. True: division is SMPTE/MTC linear time.
      bool _divisionIsLinearTime;
      //MType _mtype;
      MidiFileTrackList* _tracks;

      int status, click;
      int sstatus;
      int lastport, lastchannel;
      MType lastMtype;
      //QString lastMtypeInstrument;
      QString lastInstrName;
      QString lastDeviceName;
      //MidiInstrument* def_instr;
      MidiFilePortMap* _usedPortMap;
      MusEFile::File *fp;
      int curPos;

      bool read(char*, qint64);
      bool write(const char*, qint64);
      void put(char c);
      bool skip(qint64);

      int readShort();
      bool writeShort(int);
      int readLong();
      bool writeLong(int);
      int getvl();
      void putvl(unsigned);

      bool readTrack(MidiFileTrack*);
      bool writeTrack(const MidiFileTrack*);

      // returns:
      //  3    OK
      //  0    End of track
      // -1    Event filtered
      // -2    Error
      int readEvent(MidiPlayEvent*, MidiFileTrack*);
      void writeEvent(const MidiPlayEvent*);

   public:
      MidiFile(MusEFile::File* f);
      ~MidiFile();
      bool read();
      bool write();
      QString error();
      MidiFilePortMap* usedPortMap() { return _usedPortMap; }
      MidiFileTrackList* trackList()  { return _tracks; }
      int tracks() const              { return ntracks; }
      // Takes ownership of list and its contents.
      void setTrackList(MidiFileTrackList* tr, int n);
      void setDivision(int d)         { _division = d; }
      int division() const            { return _division; }
      bool divisionIsLinearTime() const { return _divisionIsLinearTime; }
      };

} // namespace MusECore

#define XCHG_SHORT(x) ((((x)&0xFF)<<8) | (((x)>>8)&0xFF))
#ifdef Q_PROCESSOR_X86
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

#if Q_BYTE_ORDER == Q_LITTLE_ENDIAN
#define BE_SHORT(x) XCHG_SHORT(x)
#define BE_LONG(x) XCHG_LONG(x)
#else
#define BE_SHORT(x) x
#define BE_LONG(x) x
#endif


#endif

