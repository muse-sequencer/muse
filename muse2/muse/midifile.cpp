//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: midifile.cpp,v 1.17 2004/06/18 08:36:43 wschweer Exp $
//
//  (C) Copyright 1999-2003 Werner Schweer (ws@seh.de)
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

#include <errno.h>

#include "song.h"
#include "midi.h"
#include "midifile.h"
#include "drummap.h"
#include "event.h"
#include "globals.h"
#include "midictrl.h"
#include "marker/marker.h"
#include "midiport.h"
#include "midictrl.h"
#include "mpevent.h"
#include "gconfig.h"

namespace MusECore {

const char* errString[] = {
      "no Error",
      "unexpected EOF",
      "read Error",
      "write Error",
      "bad midifile: 'MTrk' expected",
      "bad midifile: 'MThd' expected",
      "bad midi fileformat",
      };

enum ERROR {
      MF_NO_ERROR,
      MF_EOF,
      MF_READ,
      MF_WRITE,
      MF_MTRK,
      MF_MTHD,
      MF_FORMAT,
      };

//---------------------------------------------------------
//   error
//---------------------------------------------------------

QString MidiFile::error()
      {
      return QString(errString[_error]);
      }

//---------------------------------------------------------
//   MidiFile
//---------------------------------------------------------

MidiFile::MidiFile(FILE* f)
      {
      fp        = f;
      curPos    = 0;
      _mtype    = MT_GM; // MT_UNKNOWN;
      _error    = MF_NO_ERROR;
      _tracks   = new MidiFileTrackList;
      }

MidiFile::~MidiFile()
      {
      delete _tracks;
      }

//---------------------------------------------------------
//   read
//    return true on error
//---------------------------------------------------------

bool MidiFile::read(void* p, size_t len)
      {
      for (;;) {
            curPos += len;
            size_t rv = fread(p, 1, len, fp);
            if (rv == len)
                  return false;
            if (feof(fp)) {
                  _error = MF_EOF;
                  return true;
                  }
            _error = MF_READ;
            return true;
            }
      return false;
      }

//---------------------------------------------------------
//   write
//    return true on error
//---------------------------------------------------------

bool MidiFile::write(const void* p, size_t len)
      {
      size_t rv = fwrite(p, 1, len, fp);
      if (rv == len)
            return false;
      _error = MF_WRITE;
      return true;
      }

//---------------------------------------------------------
//   writeShort
//    return true on error
//---------------------------------------------------------

bool MidiFile::writeShort(int i)
      {
      short format = BE_SHORT(i);
      return write(&format, 2);
      }

//---------------------------------------------------------
//   writeLong
//    return true on error
//---------------------------------------------------------

bool MidiFile::writeLong(int i)
      {
      int format = BE_LONG(i);
      return write(&format, 4);
      }

//---------------------------------------------------------
//   readShort
//---------------------------------------------------------

int MidiFile::readShort()
      {
      short format;
      read(&format, 2);
      return BE_SHORT(format);
      }

//---------------------------------------------------------
//   readLong
//   writeLong
//---------------------------------------------------------

int MidiFile::readLong()
      {
      int format;
      read(&format, 4);
      return BE_LONG(format);
      }

/*---------------------------------------------------------
 *    skip
 *    This is meant for skipping a few bytes in a
 *    file or fifo.
 *---------------------------------------------------------*/

bool MidiFile::skip(size_t len)
      {
      char tmp[len];
      return read(tmp, len);
      }

/*---------------------------------------------------------
 *    getvl
 *    Read variable-length number (7 bits per byte, MSB first)
 *---------------------------------------------------------*/

int MidiFile::getvl()
      {
      int l = 0;
      for (int i = 0; i < 16; i++) {
            uchar c;
            if (read(&c, 1))
                  return -1;
            l += (c & 0x7f);
            if (!(c & 0x80))
                  return l;
            l <<= 7;
            }
      return -1;
      }

/*---------------------------------------------------------
 *    putvl
 *    Write variable-length number (7 bits per byte, MSB first)
 *---------------------------------------------------------*/

void MidiFile::putvl(unsigned val)
      {
      unsigned long buf = val & 0x7f;
      while ((val >>= 7) > 0) {
            buf <<= 8;
            buf |= 0x80;
            buf += (val & 0x7f);
            }
      for (;;) {
            put(buf);
            if (buf & 0x80)
                  buf >>= 8;
            else
                  break;
            }
      }

//---------------------------------------------------------
//   readTrack
//    return true on error
//---------------------------------------------------------

bool MidiFile::readTrack(MidiFileTrack* t)
      {
      MPEventList* el = &(t->events);
      char tmp[4];
      if (read(tmp, 4))
            return true;
      if (memcmp(tmp, "MTrk", 4)) {
            _error = MF_MTRK;
            return true;
            }
      int len    = readLong();       // len
      int endPos = curPos + len;
      status     = -1;
      sstatus    = -1;     // running status, not reset scanning meta or sysex
      click      = 0;

      int port    = 0;
      int channel = 0;

      for (;;) {
            MidiPlayEvent event;
            lastport    = -1;
            lastchannel = -1;

            int rv = readEvent(&event, t);
            if (lastport != -1) {
                  port = lastport;
                  if (port >= MIDI_PORTS) {
                        printf("port %d >= %d, reset to 0\n", port, MIDI_PORTS);
                        port = 0;
                        }
                  }
            if (lastchannel != -1) {
                  channel = lastchannel;
                  if (channel >= MIDI_CHANNELS) {
                        printf("channel %d >= %d, reset to 0\n", port, MIDI_CHANNELS);
                        channel = 0;
                        }
                  }
            if (rv == 0)
                  break;
            else if (rv == -1)
                  continue;
            else if (rv == -2)          // error
                  return true;

            event.setPort(port);
            if (event.type() == ME_SYSEX || event.type() == ME_META)
                  event.setChannel(channel);
            else
                  channel = event.channel();
            el->add(event);
            }
      int end = curPos;
      if (end != endPos) {
            printf("MidiFile::readTrack(): TRACKLEN does not fit %d+%d != %d, %d too much\n",
               endPos-len, len, end, endPos-end);
            if (end < endPos)
                  skip(endPos - end);
            }
      return false;
      }

//---------------------------------------------------------
//   readEvent
//    returns:
//          0     End of track
//          -1    Event filtered
//          -2    Error
//---------------------------------------------------------

int MidiFile::readEvent(MidiPlayEvent* event, MidiFileTrack* t)
      {
      uchar me, type, a, b;

      int nclick = getvl();
      if (nclick == -1) {
            printf("readEvent: error 1\n");
            return 0;
            }
      click += nclick;
      for (;;) {
            if (read(&me, 1)) {
                  printf("readEvent: error 2\n");
                  return 0;
                  }
            if (me >= 0xf8 && me <= 0xfe)
                  printf("Midi: Real Time Message 0x%02x??\n", me & 0xff);
            else
                  break;
            }

      event->setTime(click);
      int len;
      unsigned char* buffer;

      if ((me & 0xf0) == 0xf0) {
            if (me == 0xf0 || me == 0xf7) {
                  //
                  //    SYSEX
                  //
                  status = -1;                  // no running status
                  len = getvl();
                  if (len == -1) {
                        printf("readEvent: error 3\n");
                        return -2;
                        }
                  buffer = new unsigned char[len];
                  if (read(buffer, len)) {
                        printf("readEvent: error 4\n");
                        delete[] buffer;
                        return -2;
                        }
                  if (buffer[len-1] != 0xf7) {
                        printf("SYSEX doesn't end with 0xf7!\n");
                        // to be continued?
                        }
                  else
                        --len;      // don't count 0xf7
                  event->setType(ME_SYSEX);
                  event->setData(buffer, len);
                  if (((unsigned)len == gmOnMsgLen) && memcmp(buffer, gmOnMsg, gmOnMsgLen) == 0) {
                        setMType(MT_GM);
                        return -1;
                        }
                  if (((unsigned)len == gsOnMsgLen) && memcmp(buffer, gsOnMsg, gsOnMsgLen) == 0) {
                        setMType(MT_GS);
                        return -1;
                        }
                  if (((unsigned)len == xgOnMsgLen) && memcmp(buffer, xgOnMsg, xgOnMsgLen) == 0) {
                        setMType(MT_XG);
                        return -1;
                        }
                  if (buffer[0] == 0x41) {   // Roland
                        if (mtype() != MT_UNKNOWN)
                              setMType(MT_GS);
                        }
                  else if (buffer[0] == 0x43) {    // Yamaha
                        if (mtype() == MT_UNKNOWN || mtype() == MT_GM)
                              setMType(MT_XG);
                        int type   = buffer[1] & 0xf0;
                        switch (type) {
                              case 0x00:  // bulk dump
                                    buffer[1] = 0;
                                    break;
                              case 0x10:
                                    if (buffer[1] != 0x10) {
                                          buffer[1] = 0x10;    // fix to Device 1
                                          }
                                    if (len == 7 && buffer[2] == 0x4c && buffer[3] == 0x08 && buffer[5] == 7) {
                                          // part mode
                                          // 0 - normal
                                          // 1 - DRUM
                                          // 2 - DRUM 1
                                          // 3 - DRUM 2
                                          // 4 - DRUM 3
                                          // 5 - DRUM 4
                                          printf("xg set part mode channel %d to %d\n", buffer[4]+1, buffer[6]);
                                          if (buffer[6] != 0)
                                                t->isDrumTrack = true;
                                          }
                                    break;
                              case 0x20:
                                    printf("YAMAHA DUMP REQUEST\n");
                                    return -1;
                              case 0x30:
                                    printf("YAMAHA PARAMETER REQUEST\n");
                                    return -1;
                              default:
                                    printf("YAMAHA unknown SYSEX: data[2]=%02x\n", buffer[1]);
                                    return -1;
                              }
                        }
                  return 3;
                  }
            if (me == 0xff) {
                  //
                  //    META
                  //
                  status = -1;                  // no running status
                  if (read(&type, 1)) {         // read type
                        printf("readEvent: error 5\n");
                        return -2;
                        }
                  len = getvl();                // read len
                  if (len == -1) {
                        printf("readEvent: error 6\n");
                        return -2;
                        }
                  buffer = new unsigned char[len+1];
                  if (len) {
                        if (read(buffer, len)) {
                              printf("readEvent: error 7\n");
                              delete[] buffer;
                              return -2;
                              }
                        }
                  buffer[len] = 0;
                  switch(type) {
                        case 0x21:        // switch port
                              lastport = buffer[0];
                              delete[] buffer;
                              return -1;
                        case 0x20:        // switch channel
                              lastchannel = buffer[0];
                              delete[] buffer;
                              return -1;
                        case 0x2f:        // End of Track
                              delete[] buffer;
                              return 0;
                        default:
                              event->setType(ME_META);
                              event->setData(buffer, len+1);
                              event->setA(type);
                              return 3;
                        }
                  }
            else {
                  printf("Midi: unknown Message 0x%02x\n", me & 0xff);
                  return -1;
                  }
            }

      if (me & 0x80) {                     // status byte
            status   = me;
            sstatus  = status;
            if (read(&a, 1)) {
                  printf("readEvent: error 9\n");
                  return -2;
                  }
            a &= 0x7F;
            }
      else {
            if (status == -1) {
                  printf("readEvent: no running status, read 0x%02x sstatus %x\n", me, sstatus);
                  if (sstatus == -1)
                        return -1;
                  status = sstatus;
                  }
            a = me;
            }
      b = 0;
      switch (status & 0xf0) {
            case ME_NOTEOFF:
            case ME_NOTEON:
            case ME_POLYAFTER:
            case ME_CONTROLLER:
            case ME_PITCHBEND:
                  if (read(&b, 1)) {
                        printf("readEvent: error 15\n");
                        return -2;
                        }
                  event->setB(b & 0x80 ? 0 : b);
                  break;
            case ME_PROGRAM:
            case ME_AFTERTOUCH:
                  break;
            default:          // f1 f2 f3 f4 f5 f6 f7 f8 f9
                  printf("BAD STATUS 0x%02x, me 0x%02x\n", status, me);
                  return -2;
            }
      event->setA(a & 0x7f);
      event->setType(status & 0xf0);
      event->setChannel(status & 0xf);
      if ((a & 0x80) || (b & 0x80)) {
            printf("8'tes Bit in Daten(%02x %02x): tick %d read 0x%02x  status:0x%02x\n",
               a & 0xff, b & 0xff, click, me, status);
            printf("readEvent: error 16\n");
            if (b & 0x80) {
                  // Try to fix: interpret as channel byte
                  status   = b & 0xf0;
                  sstatus  = status;
                  return 3;
                  }
            return -1;
            }
      if (event->type() == ME_PITCHBEND) {
            int val = (event->dataB() << 7) + event->dataA();
            val -= 8192;
            event->setA(val);
            }
      return 3;
      }

//---------------------------------------------------------
//   writeTrack
//---------------------------------------------------------

bool MidiFile::writeTrack(const MidiFileTrack* t)
      {
      //FIXME: By T356 01/19/2010
      // If saving as a compressed file (gz or bz2),
      //  the file is a pipe, and pipes can't seek !
      // This results in a corrupted midi file. 
      // So exporting compressed midi has been disabled (elsewhere)
      //  for now...
      
      const MPEventList* events = &(t->events);
      write("MTrk", 4);
      int lenpos = ftell(fp);
      writeLong(0);                 // dummy len

      status = -1;
      int tick = 0;
      for (iMPEvent i = events->begin(); i != events->end(); ++i) {
            int ntick = i->time();
            if (ntick < tick) {
                  printf("MidiFile::writeTrack: ntick %d < tick %d\n", ntick, tick);
                  ntick = tick;
                  }
            putvl(((ntick - tick) * MusEGlobal::config.midiDivision + MusEGlobal::config.division/2)/MusEGlobal::config.division);
            tick = ntick;
            writeEvent(&(*i));
            }

      //---------------------------------------------------
      //    write "End Of Track" Meta
      //    write Track Len
      //

      putvl(0);
      put(0xff);        // Meta
      put(0x2f);        // EOT
      putvl(0);         // len 0
      int endpos = ftell(fp);
      fseek(fp, lenpos, SEEK_SET);
      writeLong(endpos-lenpos-4);   // tracklen
      fseek(fp, endpos, SEEK_SET);
      return false;
      }

//---------------------------------------------------------
//   writeEvent
//---------------------------------------------------------

void MidiFile::writeEvent(const MidiPlayEvent* event)
      {
      int c     = event->channel();
      int nstat = event->type();

      // we dont save meta data into smf type 0 files: DELETETHIS 4 ???
      // Oct 16, 2011: Apparently it is legal to do that. Part of fix for bug tracker 3293339.
      //if (MusEGlobal::config.smfFormat == 0 && nstat == ME_META)
      //      return;

      nstat |= c;
      //
      //  running status; except for Sysex- and Meta Events
      //
      if (((nstat & 0xf0) != 0xf0) && (nstat != status)) {
            status = nstat;
            put(nstat);
            }
      switch (event->type()) {
            case ME_NOTEOFF:
            case ME_NOTEON:
            case ME_POLYAFTER:
            case ME_CONTROLLER:
            case ME_PITCHBEND:
                  put(event->dataA());
                  put(event->dataB());
                  break;
            case ME_PROGRAM:        // Program Change
            case ME_AFTERTOUCH:     // Channel Aftertouch
                  put(event->dataA());
                  break;
            case ME_SYSEX:
                  put(0xf0);
                  putvl(event->len() + 1);  // including 0xf7
                  write(event->data(), event->len());
                  put(0xf7);
                  status = -1;      // invalidate running status
                  break;
            case ME_META:
                  put(0xff);
                  put(event->dataA());
                  putvl(event->len());
                  write(event->data(), event->len());
                  status = -1;
                  break;
            }
      }

//---------------------------------------------------------
//   write
//    returns true on error
//---------------------------------------------------------

bool MidiFile::write()
      {
      write("MThd", 4);
      writeLong(6);                 // header len
      writeShort(MusEGlobal::config.smfFormat);
      if (MusEGlobal::config.smfFormat == 0) {
            // DELETETHIS 30
            /*
            //writeShort(1);    // Removed. Bug tracker 3293339
            MidiFileTrack dst;
            for (iMidiFileTrack i = _tracks->begin(); i != _tracks->end(); ++i) {
                  MPEventList* sl = &((*i)->events);
                  for (iMPEvent ie = sl->begin(); ie != sl->end(); ++ie)
                  {  
                        // ALERT: Observed a problem here, apparently some of the events are being added too fast. 
                        //        The dump below tells me some of the events (sysex/meta) are missing from the list! 
                        //        Apparently it's a timing problem. Very puzzling. 
                        //        Attempting wild-guess fix now to eliminate multiple MidiFileTracks in MusE::exportMidi()...
                        //        Nope. Didn't help. Now that it's a single MidiFileTrack, try skipping this section altogether...
                        //        Yes that appears to have fixed it. Weird. What's the difference - the local 'dst' variable ?
                        //        Or are there still lurking problems, or something more fundamentally wrong with Event or MPEvent?
                        printf("MidiFile::write adding event to dst:\n"); // REMOVE Tim.
                        ie->dump();  // REMOVE Tim.
                        dst.events.add(*ie);
                  }      
                  }
            writeShort(1);
            writeShort(_division);
            writeTrack(&dst);
            */
            
            writeShort(1);
            //writeShort(_division); DELETETHIS 3
            //if(!_tracks->empty())
            //  writeTrack(*(_tracks->begin()));
            
            }
      else {
        
        
            writeShort(ntracks);
      }
            writeShort(_division);
            for (ciMidiFileTrack i = _tracks->begin(); i != _tracks->end(); ++i)
                  writeTrack(*i);

      return (ferror(fp) != 0);
      }

//---------------------------------------------------------
//   readMidi
//    returns true on error
//---------------------------------------------------------

bool MidiFile::read()
      {
      _error = MF_NO_ERROR;
      int i;
      char tmp[4];

      if (read(tmp, 4))
            return true;
      int len = readLong();
      if (memcmp(tmp, "MThd", 4) || len < 6) {
            _error = MF_MTHD;
            return true;
            }
      format   = readShort();
      ntracks  = readShort();
      _division = readShort();

      if (_division < 0)
            _division = (-(_division/256)) * (_division & 0xff);
      if (len > 6)
            skip(len-6); // skip excess bytes

      switch (format) {
            case 0:
                  {
                  MidiFileTrack* t = new MidiFileTrack;
                  _tracks->push_back(t);
                  if (readTrack(t))
                        return true;
                  }
                  break;
            case 1:
                  for (i = 0; i < ntracks; i++) {
                        MidiFileTrack* t = new MidiFileTrack;
                        _tracks->push_back(t);
                        if (readTrack(t))
                              return true;
                        }
                  break;
            default:
                  _error = MF_FORMAT;
                  return true;
            }
      return false;
      }

} // namespace MusECore
