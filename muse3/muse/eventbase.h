//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: eventbase.h,v 1.3.2.3 2009/12/20 05:00:35 terminator356 Exp $
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

#ifndef __EVENTBASE_H__
#define __EVENTBASE_H__

#include <sys/types.h>
#include <sndfile.h>

#include "type_defs.h"
#include "pos.h"
#include "event.h"

namespace MusECore {
// REMOVE Tim. samplerate. Changed.
//class WavePart;
class Part;
// REMOVE Tim. samplerate. Added.
class Fifo;

//---------------------------------------------------------
//   EventBase
//---------------------------------------------------------

class EventBase : public PosLen {
      EventType _type;
      static EventID_t idGen;
      // An always unique id.
      EventID_t _uniqueId; 
      // Can be either _uniqueId or the same _uniqueId as other clone 'group' events. De-cloning restores it to _uniqueId.
      EventID_t _id;       

   protected:
      int refCount;
      bool _selected;

   public:
      EventBase(EventType t);
      // Creates a non-shared clone with same id, or duplicate with unique id, and 0 ref count and invalid Pos sn. 
      EventBase(const EventBase& ev, bool duplicate_not_clone = false); 

      virtual ~EventBase() { }

      int getRefCount() const    { return refCount; }

      EventID_t id() const       { return _id; }
      EventID_t newId()          { return idGen++; }
      void shareId(const EventBase* ev) { _id = ev->_id; } // Makes id same as given event's. Effectively makes the events non-shared clones.
      virtual void assign(const EventBase& ev);            // Assigns to this event, excluding the _id. 
      
      EventType type() const     { return _type;  }
      void setType(EventType t)  { _type = t;  }
      bool selected() const      { return _selected; }
      void setSelected(bool val) { _selected = val; }

      void move(int offset);
      
      virtual bool isSimilarTo(const EventBase& other) const = 0;

      virtual void read(Xml&) = 0;
      virtual void write(int, Xml&, const Pos& offset, bool forcePath = false) const = 0;
      virtual void dump(int n = 0) const;
      virtual EventBase* mid(unsigned, unsigned) const = 0;
      friend class Event;

      virtual bool isNote() const                   { return false; }
      virtual bool isNoteOff() const                { return false; }
      virtual bool isNoteOff(const Event&) const    { return false; }
      virtual int pitch() const                     { return 0;      }
      virtual int program() const                   { return 0;      }
      virtual int cntrl() const                     { return 0;      }
      virtual int dataA() const                     { return 0;      }
      virtual void setA(int)                        { }
      virtual void setPitch(int)                    { }

      virtual int cntrlVal() const                  { return 0;      }
      virtual int dataB() const                     { return 0;      }
      virtual int velo() const                      { return 0;      }
      virtual void setB(int)                        { }
      virtual void setVelo(int)                     { }

      virtual int veloOff() const                   { return 0;      }
      virtual int dataC() const                     { return 0;      }
      virtual void setC(int)                        { }
      virtual void setVeloOff(int)                  { }

      virtual const unsigned char* data() const     { return 0; }
      virtual int dataLen() const                   { return 0; }
      virtual void setData(const unsigned char*, int) { }
      virtual const EvData eventData() const        { return EvData(); }

      virtual const QString name() const            { return QString("?");  }
      virtual void setName(const QString&)          { }
      virtual int spos() const                      { return 0;  }
      virtual void setSpos(int)                     { }
      virtual SndFileR sndFile() const              { return 0;      }
      virtual void setSndFile(SndFileR&)            { }
      // Creates a non-shared clone, having the same 'group' _id.
      // NOTE: Certain pointer members may still be SHARED. Such as the sysex MidiEventBase::edata.
      //       Be aware when iterating or modifying clones.
      virtual EventBase* clone() const = 0; 
      
      // Restores _id to _uniqueId, removing the event from any clone 'group'. 
      virtual void deClone() { _id = _uniqueId; } 
      // Creates a copy of the event base, excluding the 'group' _id. 
      virtual EventBase* duplicate() const = 0; 
      
// REMOVE Tim. samplerate. Changed.
//       virtual void readAudio(WavePart* /*part*/, unsigned /*offset*/, 
      virtual void readAudio(unsigned /*frame*/, float** /*bpp*/, int /*channels*/, int /*nn*/, bool /*doSeek*/, bool /*overwrite*/) { }
// REMOVE Tim. samplerate. Added.
      virtual void seekAudio(sf_count_t /*frame*/) { }
      //virtual void clearAudioPrefetchFifo() { }
      virtual Fifo* audioPrefetchFifo()     { return 0; }
      virtual void prefetchAudio(Part* /*part*/, sf_count_t /*frames*/) { }
      //virtual void fetchAudioData(WavePart* /*part*/, sf_count_t /*pos*/, int /*channels*/, bool /*off*/, 
      //                            sf_count_t /*frames*/, float** /*bp*/, bool /*doSeek*/, bool /*overwrite*/) { }
      //virtual bool getAudioPrefetchBuffer(int segs, unsigned long samples, float** dst, unsigned* pos) { }
      };

} // namespace MusECore

#endif

