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

#include <set>
#include "evdata.h"
#include "memory.h"
#include <cstddef>

namespace MusECore {

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
      MEvent();
      MEvent(const MEvent& e);
      MEvent(unsigned tm, int p, int c, int t, int a, int b);
      MEvent(unsigned t, int p, int type, const unsigned char* data, int len);
      MEvent(unsigned t, int p, int tpe, EvData d);

      virtual ~MEvent();

      MEvent& operator=(const MEvent& ed);
      
      int sortingWeight() const;
      
      int port()    const;
      int channel() const;
      int type()    const;
      int dataA()   const;
      int dataB()   const;
      unsigned time() const;
      int loopNum() const;

      void setPort(int val);
      void setChannel(int val);
      void setType(int val);
      void setA(int val);
      void setB(int val);
      void setTime(unsigned val);
      void setLoopNum(int n);

      const EvData& eventData() const;
      unsigned char* data();
      const unsigned char* constData() const;
      int len() const;
      void setData(const EvData& e);
      void setData(const unsigned char* p, int len);

      bool isNote() const;
      bool isNoteOff() const;
      bool operator<(const MEvent&) const;
      bool isValid() const;
      // Returns true if the event is any of the EIGHT standard General Midi RPN controllers.
      bool isStandardRPN() const;
      // Returns true if the event is one of our own native compound RPN controller ie. (N)RPN, (N)RPN14.
      bool isNativeRPN() const;

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
      MidiRecordEvent();
      MidiRecordEvent(const MidiRecordEvent& e);
      MidiRecordEvent(const MEvent& e);
      MidiRecordEvent(unsigned tm, int p, int c, int t, int a, int b);
      MidiRecordEvent(unsigned t, int p, int tpe, const unsigned char* data, int len);
      MidiRecordEvent(unsigned t, int p, int type, EvData data);
      virtual ~MidiRecordEvent();

      MidiRecordEvent& operator=(const MidiRecordEvent& e);

      unsigned int tick();
      void setTick(unsigned int tick);
      };

//---------------------------------------------------------
//   MidiPlayEvent
//    allocated and deleted in audio thread context
//---------------------------------------------------------

class MidiPlayEvent : public MEvent {
   private:
      // This latency value can be used by the 'stuck notes' mechanism,
      //  where time is in ticks and the events are note-offs to be played later and
      //  the driver computes the frame later given the tempo AT THAT MOMENT but also
      //  needs to know what the the original note-on latency was when it was scheduled.
      int _latency;
   public:
      MidiPlayEvent();
      MidiPlayEvent(const MidiPlayEvent& e);
      MidiPlayEvent(const MEvent& e);
      MidiPlayEvent(unsigned tm, int p, int c, int t, int a, int b);
      MidiPlayEvent(unsigned t, int p, int type, const unsigned char* data, int len);
      MidiPlayEvent(unsigned t, int p, int type, EvData data);
      virtual ~MidiPlayEvent();

      MidiPlayEvent& operator=(const MidiPlayEvent& e);

      int latency();
      void setLatency(int latency);
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

    audioMPEventRTalloc() { } 
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

//---------------------------------------------------------
//   MPEventList
//    memory allocation in audio thread domain
//---------------------------------------------------------

typedef std::multiset<MidiPlayEvent, std::less<MidiPlayEvent>, audioMPEventRTalloc<MidiPlayEvent> > MPEL;

class MPEventList : public MPEL {
  public:
      // Optimize to eliminate duplicate events at the SAME time.
      // It will not handle duplicate events at DIFFERENT times.
      // Replaces event if it already exists.
      void add(const MidiPlayEvent& ev);
      // Optimize to replace duplicate events at ANY time starting from the end of the list.
      // Replaces event if it already exists.
      // This looks backwards for the first occurrence of a similar event and replaces it if found.
      // This is mainly designed for a MidiDevice's queue buffer when the device is 'off' to avoid
      //  large backlog of events waiting to be sent when the device comes back on. Some event types
      //  are ignored or handled differently.
      // Note this will NOT eliminate any FURTHER duplicates that may have already existed. Only the last one found.
      // If RPNControllersReserved is false, it will optimize the EIGHT RPN standard controllers in a special way.
      // If RPNControllersReserved is true, it will treat such controllers as ordinary generic controllers.
      void addExclusive(const MidiPlayEvent& ev, bool RPNControllersReserved = false);
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
      // Optimize to replace duplicate events at ANY time starting from the end of the list.
      // Replaces event if it already exists.
      // This looks backwards for the first occurrence of a similar event and replaces it if found.
      // This is mainly designed for a MidiDevice's queue buffer when the device is 'off' to avoid
      //  large backlog of events waiting to be sent when the device comes back on. Some event types
      //  are ignored or handled differently.
      // Note this will NOT eliminate any FURTHER duplicates that may have already existed. Only the last one found.
      // If RPNControllersReserved is false, it will optimize the EIGHT RPN standard controllers in a special way.
      // If RPNControllersReserved is true, it will treat such controllers as ordinary generic controllers.
      void addExclusive(const MidiPlayEvent& ev, bool RPNControllersReserved = false);
};

typedef SeqMPEventList::iterator iSeqMPEvent;
typedef SeqMPEventList::const_iterator ciSeqMPEvent;
typedef std::pair<iSeqMPEvent, iSeqMPEvent> SeqMPEventListRangePair_t;


} // namespace MusECore

#endif

