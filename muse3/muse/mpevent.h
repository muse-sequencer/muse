//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: mpevent.h,v 1.8.2.5 2009/11/25 09:09:43 terminator356 Exp $
//
//  (C) Copyright 1999-2002 Werner Schweer (ws@seh.de)
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

#ifndef __MPEVENT_H__
#define __MPEVENT_H__

// #include <set>
#include <set>
//#include <iterator>
//#include <memory>
//#include <list>
#include "evdata.h"
#include "memory.h"
// REMOVE Tim. autoconnect. Added.
//#include "sysex_processor.h"
//#include <stdio.h>
//#include <stdlib.h>
#include <cstddef>
// #include <map>

// Play events ring buffer size
#define MIDI_FIFO_SIZE    4096         

// Record events ring buffer size
#define MIDI_REC_FIFO_SIZE  256

namespace MusECore {

class Event;
class EvData;

//---------------------------------------------------------
//   MEvent
//    baseclass for MidiPlayEvent and MidiRecordEvent
//---------------------------------------------------------

//---------------------------------------------------------
//   MEvent
//---------------------------------------------------------

class MEvent {
      unsigned _time;
      EvData edata;
      unsigned char _port, _channel, _type;
      int _a, _b;
      int _loopNum; // The loop count when the note was recorded.

   public:
      MEvent() : _time(0), _port(0), _channel(0), _type(0), _a(0), _b(0), _loopNum(0) { }
      MEvent(unsigned tm, int p, int c, int t, int a, int b)
        : _time(tm), _port(p), _channel(c & 0xf), _type(t), _a(a), _b(b), _loopNum(0) { }
      MEvent(unsigned t, int p, int type, const unsigned char* data, int len);
      MEvent(unsigned t, int p, int tpe, EvData d) : _time(t), edata(d), _port(p), _type(tpe), _loopNum(0) { }
      MEvent(unsigned t, int port, int channel, const Event& e);

      virtual ~MEvent()         {}

      MEvent& operator=(const MEvent& ed) {
            _time    = ed._time;
            edata    = ed.edata;
            _port    = ed._port;
            _channel = ed._channel;
            _type    = ed._type;
            _a       = ed._a;
            _b       = ed._b;
            _loopNum = ed._loopNum;
            return *this;
            }
      
      int sortingWeight() const;
      
      int port()    const      { return _port;    }
      int channel() const      { return _channel; }
      int type()    const      { return _type;    }
      int dataA()   const      { return _a;       }
      int dataB()   const      { return _b;       }
      unsigned time() const    { return _time;    }
      int loopNum() const      { return _loopNum; }

      void setPort(int val)    { _port = val;     }
      void setChannel(int val) { _channel = val;  }
      void setType(int val)    { _type = val;     }
      void setA(int val)       { _a = val;        }
      void setB(int val)       { _b = val;        }
      void setTime(unsigned val) { _time = val;   }
      void setLoopNum(int n)   { _loopNum = n;    }

      const EvData& eventData() const { return edata; }
      unsigned char* data() const     { return edata.data; }
      int len() const                 { return edata.dataLen; }
      void setData(const EvData& e)   { edata = e; }
      void setData(const unsigned char* p, int len) { edata.setData(p, len); }
      // REMOVE Tim. autoconnect. Added.
      //void setData(const SysExInputProcessor* p) { edata.setData(p); }
      
      void dump() const;
      bool isNote() const      { return _type == 0x90; }
      bool isNoteOff() const   { return (_type == 0x80)||(_type == 0x90 && _b == 0); }
      bool operator<(const MEvent&) const;
      bool isValid() const { return _type != 0; }
      
      // Returns a valid source controller number (above zero), 
      //  translated from the event to proper internal control type.
      // For example 
      //  ME_CONTROLLER + Data(A = CTRL_HBANK) = CTRL_PROGRAM
      //  ME_CONTROLLER + Data(A = CTRL_LBANK) = CTRL_PROGRAM
      //  ME_PROGRAM                           = CTRL_PROGRAM
      //  ME_PITCHBEND                         = CTRL_PITCH
      //  ME_CONTROLLER + Data(A = ctrl)       = ctrl
      // Otherwise returns -1 if the event is not translatable to a controller, 
      //  or an error occurred.
      int translateCtrlNum() const;
      };

//---------------------------------------------------------
//   MidiRecordEvent
//    allocated and deleted in midiseq thread context
//---------------------------------------------------------

class MidiRecordEvent : public MEvent {
   private:
      unsigned int _tick; // To store tick when external sync is on, required besides frame.
   public:
      MidiRecordEvent() : MEvent() {}
      MidiRecordEvent(const MEvent& e) : MEvent(e) {}
      MidiRecordEvent(unsigned tm, int p, int c, int t, int a, int b)
        : MEvent(tm, p, c, t, a, b) {}
      MidiRecordEvent(unsigned t, int p, int tpe, const unsigned char* data, int len)
        : MEvent(t, p, tpe, data, len) {}
      MidiRecordEvent(unsigned t, int p, int type, EvData data)
        : MEvent(t, p, type, data) {}
      virtual ~MidiRecordEvent() {}
      
      unsigned int tick() {return _tick;}
      void setTick(unsigned int tick) {_tick = tick;}
      };

//---------------------------------------------------------
//   MidiPlayEvent
//    allocated and deleted in audio thread context
//---------------------------------------------------------

class MidiPlayEvent : public MEvent {
   public:
      MidiPlayEvent() : MEvent() {}
      MidiPlayEvent(const MEvent& e) : MEvent(e) {}
      MidiPlayEvent(unsigned tm, int p, int c, int t, int a, int b)
        : MEvent(tm, p, c, t, a, b) {}
      MidiPlayEvent(unsigned t, int p, int type, const unsigned char* data, int len)
        : MEvent(t, p, type, data, len) {}
      MidiPlayEvent(unsigned t, int p, int type, EvData data)
        : MEvent(t, p, type, data) {}
      MidiPlayEvent(unsigned t, int port, int channel, const Event& e)
        : MEvent(t, port, channel, e) {}
      virtual ~MidiPlayEvent() {}
      };

// REMOVE Tim. autoconnect. Removed. Moved below.
// //---------------------------------------------------------
// //   MPEventList
// //    memory allocation in audio thread domain
// //---------------------------------------------------------
// 
// typedef std::multiset<MidiPlayEvent, std::less<MidiPlayEvent>, audioRTalloc<MidiPlayEvent> > MPEL;
// 
// struct MPEventList : public MPEL {
//   public:
//       // Optimize to eliminate duplicate events at the SAME time.
//       // It will not handle duplicate events at DIFFERENT times.
//       // Replaces event if it already exists.
//       void add(const MidiPlayEvent& ev);
// };
// 
// typedef MPEventList::iterator iMPEvent;
// typedef MPEventList::const_iterator ciMPEvent;
// typedef std::pair<iMPEvent, iMPEvent> MPEventListRangePair_t;

/* DELETETHIS 20 ??
//---------------------------------------------------------
//   MREventList
//    memory allocation in midi thread domain
//---------------------------------------------------------

// Changed by Tim. p3.3.8

// audioRTalloc? Surely this must have been a mistake?  
//typedef std::list<MidiRecordEvent, audioRTalloc<MidiRecordEvent> > MREL;
typedef std::list<MidiRecordEvent, midiRTalloc<MidiRecordEvent> > MREL;

struct MREventList : public MREL {
      void add(const MidiRecordEvent& ev) { MREL::push_back(ev); }
      };

typedef MREventList::iterator iMREvent;
typedef MREventList::const_iterator ciMREvent;
*/

// REMOVE Tim. autoconnect. Removed. Replaced with template-ized version.
// //---------------------------------------------------------
// //   MidiFifo
// //---------------------------------------------------------
// 
// class MidiFifo {
//       MidiPlayEvent fifo[MIDI_FIFO_SIZE];
//       volatile int size;
//       int wIndex;
//       int rIndex;
// 
//    public:
//       MidiFifo()  { clear(); }
//       bool put(const MidiPlayEvent& event);   // returns true on fifo overflow
//       MidiPlayEvent get();
//       const MidiPlayEvent& peek(int = 0);
//       void remove();
//       bool isEmpty() const { return size == 0; }
//       // This is not thread-safe.
//       void clear()         { size = 0, wIndex = 0, rIndex = 0; }
//       // Clear the 'read' side of the ring buffer, which also clears the size.
//       // NOTE: A corresponding clearWrite() is not provided because
//       //  it is dangerous to reset the size from the sender side -
//       //  the receiver might cache the size, briefly. The sender should 
//       //  only grow the size while the receiver should only shrink it.
//       void clearRead()     { size = 0; rIndex = wIndex; }
//       int getSize() const  { return size; }
//       };

//---------------------------------------------------------
//   MidiRecFifo
//---------------------------------------------------------

class MidiRecFifo {
      MidiRecordEvent fifo[MIDI_REC_FIFO_SIZE];
      volatile int size;
      int wIndex;
      int rIndex;

   public:
      MidiRecFifo()  { clear(); }
      bool put(const MidiRecordEvent& event);   // returns true on fifo overflow
      MidiRecordEvent get();
      const MidiRecordEvent& peek(int = 0);
      void remove();
      bool isEmpty() const { return size == 0; }
      void clear()         { size = 0, wIndex = 0, rIndex = 0; }
      int getSize() const  { return size; }
      };

//---------------------------------------------------------
//   audioMPEventRTalloc
//---------------------------------------------------------

template <typename T> class audioMPEventRTalloc
{
  private:
    static TypedMemoryPool<T, 2048> pool;
    
  public:
    typedef T         value_type;
    typedef size_t    size_type;
    typedef ptrdiff_t difference_type;

    typedef T*        pointer;
    typedef const T*  const_pointer;

    typedef T&        reference;
    typedef const T&  const_reference;

    pointer address(reference x) const { return &x; }
    const_pointer address(const_reference x) const { return &x; }

    audioMPEventRTalloc() 
    { 
      // REMOVE Tim. autoconnect. Added.
      fprintf(stderr, "audioMPEventRTalloc ctor: sizeof T:%u\n", (unsigned int)sizeof(T));
    } 
    template <typename U> audioMPEventRTalloc(const audioMPEventRTalloc<U>&) {}
    ~audioMPEventRTalloc() {}

    pointer allocate(size_type n, void * = 0) { return static_cast<T*>(pool.alloc(n)); }
    void deallocate(pointer p, size_type n) { pool.free(p, n); }

    audioMPEventRTalloc<T>&  operator=(const audioMPEventRTalloc&) { return *this; }
    void construct(pointer p, const T& val) { new ((T*) p) T(val); }
    void destroy(pointer p) { p->~T(); }
    size_type max_size() const { return size_t(-1); }

    template <typename U> struct rebind { typedef audioMPEventRTalloc<U> other; };
    template <typename U> audioMPEventRTalloc& operator=(const audioMPEventRTalloc<U>&) { return *this; }
};

//---------------------------------------------------------
//   seqMPEventRTalloc
//---------------------------------------------------------

template <typename T> class seqMPEventRTalloc
{
  private:
    static TypedMemoryPool<T, 2048> pool;
    
  public:
    typedef T         value_type;
    typedef size_t    size_type;
    typedef ptrdiff_t difference_type;

    typedef T*        pointer;
    typedef const T*  const_pointer;

    typedef T&        reference;
    typedef const T&  const_reference;

    pointer address(reference x) const { return &x; }
    const_pointer address(const_reference x) const { return &x; }

    seqMPEventRTalloc() { }
    template <typename U> seqMPEventRTalloc(const seqMPEventRTalloc<U>&) {}
    ~seqMPEventRTalloc() {}

    pointer allocate(size_type n, void * = 0) { return static_cast<T*>(pool.alloc(n)); }
    void deallocate(pointer p, size_type n) { pool.free(p, n); }

    seqMPEventRTalloc<T>&  operator=(const seqMPEventRTalloc&) { return *this; }
    void construct(pointer p, const T& val) { new ((T*) p) T(val); }
    void destroy(pointer p) { p->~T(); }
    size_type max_size() const { return size_t(-1); }

    template <typename U> struct rebind { typedef seqMPEventRTalloc<U> other; };
    template <typename U> seqMPEventRTalloc& operator=(const seqMPEventRTalloc<U>&) { return *this; }
};


// // REMOVE. Nope, can't use this. Containers expect a standard allocator parameter.
// //---------------------------------------------------------
// //   MPEventRTalloc
// //---------------------------------------------------------
// 
// enum MPEventRTallocID { AudioRTAlloc = 0, SeqRTAlloc = 1 };
// 
// template <typename T, MPEventRTallocID aid> class MPEventRTalloc
// {
//   private:
//     static TypedMemoryPool<T, 2048> audio_pool;
//     static TypedMemoryPool<T, 2048> seq_pool;
//     
//   public:
//     typedef T         value_type;
//     typedef size_t    size_type;
//     typedef ptrdiff_t difference_type;
// 
//     typedef T*        pointer;
//     typedef const T*  const_pointer;
// 
//     typedef T&        reference;
//     typedef const T&  const_reference;
// 
//     pointer address(reference x) const { return &x; }
//     const_pointer address(const_reference x) const { return &x; }
// 
//     MPEventRTalloc() 
//     { 
//       // REMOVE Tim. autoconnect. Added.
//       fprintf(stderr, "MPEventRTalloc ctor: sizeof T:%u\n", (unsigned int)sizeof(T));
//     } 
//     template <typename U, MPEventRTallocID aaid> MPEventRTalloc(const MPEventRTalloc<U, aaid>&) {}
//     ~MPEventRTalloc() {}
// 
//     pointer allocate(size_type n, void * = 0) 
//     { 
//       switch(aid)
//       {
//         case AudioRTAlloc:
//           return static_cast<T*>(audio_pool.alloc(n)); 
//         break;
//         case SeqRTAlloc:
//           return static_cast<T*>(seq_pool.alloc(n)); 
//         break;
//       }
//       return 0;
//     }
// 
//     void deallocate(pointer p, size_type n) 
//     { 
//       switch(aid)
//       {
//         case AudioRTAlloc:
//           audio_pool.free(p, n); 
//         break;
//         case SeqRTAlloc:
//           seq_pool.free(p, n); 
//         break;
//       }
//     }
// 
//     MPEventRTalloc<T, aid>&  operator=(const MPEventRTalloc&) { return *this; }
//     void construct(pointer p, const T& val) { new ((T*) p) T(val); }
//     void destroy(pointer p) { p->~T(); }
//     size_type max_size() const { return size_t(-1); }
// 
//     template <typename U, MPEventRTallocID aaid> struct rebind { typedef MPEventRTalloc<U, aaid> other; };
//     template <typename U, MPEventRTallocID aaid> MPEventRTalloc& operator=(const MPEventRTalloc<U, aaid>&) { return *this; }
// };
// 
// //---------------------------------------------------------
// //   MPEventsList
// //    memory allocation in audio thread domain
// //---------------------------------------------------------
// 
// //typedef std::multiset<MidiPlayEvent, std::less<MidiPlayEvent>, audioMPEventRTalloc<MidiPlayEvent> > MPEL;
// 
// template <MPEventRTallocID aid> 
// //class MPEventsList : public std::multiset<MidiPlayEvent, std::less<MidiPlayEvent>, MPEventRTalloc<MidiPlayEvent, aid> > {
// class MPEventsList : public std::multiset<MidiPlayEvent, std::less<MidiPlayEvent>, AudioMPEventRTalloc<MidiPlayEvent> > {
//   public:
//       // Optimize to eliminate duplicate events at the SAME time.
//       // It will not handle duplicate events at DIFFERENT times.
//       // Replaces event if it already exists.
//       void add(const MidiPlayEvent& ev);
// };
// 
// typedef MPEventsList::iterator iMPEvents;
// typedef MPEventsList::const_iterator ciMPEvents;
// typedef std::pair<iMPEvents, iMPEvents> MPEventsListRangePair_t;

//---------------------------------------------------------
//   MPEventList
//    memory allocation in audio thread domain
//---------------------------------------------------------

typedef std::multiset<MidiPlayEvent, std::less<MidiPlayEvent>, audioMPEventRTalloc<MidiPlayEvent> > MPEL;
//typedef std::multiset<MidiPlayEvent, std::less<MidiPlayEvent>, std::allocator<MidiPlayEvent> > MPEL;

class MPEventList : public MPEL {
  public:
      // Optimize to eliminate duplicate events at the SAME time.
      // It will not handle duplicate events at DIFFERENT times.
      // Replaces event if it already exists.
      void add(const MidiPlayEvent& ev);
};

typedef MPEventList::iterator iMPEvent;
typedef MPEventList::const_iterator ciMPEvent;
typedef std::pair<iMPEvent, iMPEvent> MPEventListRangePair_t;

//---------------------------------------------------------
//   SeqMPEventList
//    memory allocation in sequencer thread domain
//---------------------------------------------------------

typedef std::multiset<MidiPlayEvent, std::less<MidiPlayEvent>, seqMPEventRTalloc<MidiPlayEvent> > SMPEL;

class SeqMPEventList : public SMPEL {
  public:
      // Optimize to eliminate duplicate events at the SAME time.
      // It will not handle duplicate events at DIFFERENT times.
      // Replaces event if it already exists.
      void add(const MidiPlayEvent& ev);
};

typedef SeqMPEventList::iterator iSeqMPEvent;
typedef SeqMPEventList::const_iterator ciSeqMPEvent;
typedef std::pair<iSeqMPEvent, iSeqMPEvent> SeqMPEventListRangePair_t;


// //---------------------------------------------------------
// //   MPEventsList
// //    memory allocation in audio thread domain
// //---------------------------------------------------------
// 
// //typedef std::multiset<MidiPlayEvent, std::less<MidiPlayEvent>, audioMPEventRTalloc<MidiPlayEvent> > MPEL;
// 
// //template <typename _Alloc = std::allocator<MidiPlayEvent> > 
// //template < > <typename T, typename _Alloc = std::allocator<T> > 
// //template < > 
// //template <std::allocator _Alloc > 
// //class MPEventsList : public std::multiset < MidiPlayEvent, std::less<MidiPlayEvent>, _Alloc > {
// template < typename T >
// class MPEventsList<T> : public std::multiset < T, std::less<T> > {
// //class MPEventsList : public std::multiset < T, std::less<T>, std::allocator<T> > {
// //class MPEventsList : public MPEventList<MidiPlayEvent, std::less<MidiPlayEvent>, SeqMPEventRTalloc<MidiPlayEvent> > {
//   public:
//       typedef std::iterator<T> iMPEvents;
// //       typedef const_iterator ciMPEvents;
// //       typedef std::pair<iMPEvents, iMPEvents> MPEventsListRangePair_t;
//       
//       // Optimize to eliminate duplicate events at the SAME time.
//       // It will not handle duplicate events at DIFFERENT times.
//       // Replaces event if it already exists.
//       void add(const MidiPlayEvent& ev);
// };

//typedef MPEventsList::iterator iMPEvents;
// typedef MPEventsList::const_iterator ciMPEvents;
// typedef std::pair<iMPEvents, iMPEvents> MPEventsListRangePair_t;

} // namespace MusECore

#endif

