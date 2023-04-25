//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: mpevent.cpp,v 1.6.2.2 2009/11/25 09:09:43 terminator356 Exp $
//
//  (C) Copyright 2002-2004 Werner Schweer (ws@seh.de)
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
//=========================================================

#include <stdio.h>
//#include <string.h>

#include "mpevent.h"

#include "midictrl_consts.h"
#include "muse/midi_consts.h"

namespace MusECore {


template <typename T> TypedMemoryPool<T, 2048> audioMPEventRTalloc<T>::pool;
template <typename T> TypedMemoryPool<T, 2048> seqMPEventRTalloc<T>::pool;

//template <typename T, MPEventRTallocID aid> TypedMemoryPool<T, 2048> MPEventRTalloc<T, aid>::audio_pool;
//template <typename T, MPEventRTallocID aid> TypedMemoryPool<T, 2048> MPEventRTalloc<T, aid>::seq_pool;

//---------------------------------------------------------
//   MEvent
//---------------------------------------------------------

MEvent::MEvent() : _time(0), _port(0), _channel(0), _type(0), _a(0), _b(0), _loopNum(0) { }
MEvent::MEvent(const MEvent& e) : _time(e._time), edata(e.edata), _port(e._port), _channel(e._channel),
        _type(e._type), _a(e._a), _b(e._b), _loopNum(e._loopNum) { }
MEvent::MEvent(unsigned tm, int p, int c, int t, int a, int b)
  : _time(tm), _port(p), _channel(c & 0xf), _type(t), _a(a), _b(b), _loopNum(0) { }
MEvent::MEvent(unsigned t, int p, int tpe, EvData d) :
  _time(t), edata(d), _port(p), _channel(0), _type(tpe), _a(0), _b(0), _loopNum(0) { }

MEvent::MEvent(unsigned t, int port, int tpe, const unsigned char* data, int len)
      {
      _time = t;
      _port = port;
      edata.setData(data, len);
      _type = tpe;
      _loopNum = 0;
      setChannel(0);
      _a = _b = 0;
      }

MEvent::~MEvent() {}

MEvent& MEvent::operator=(const MEvent& ed) {
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

//---------------------------------------------------------
//   sortingWeight
//---------------------------------------------------------

int MEvent::sortingWeight() const
{
  // Sorting weight initially worked out by Tim E. Real
  // Sorted here by most popular for quickest response.
  
  switch(_type)
  {
    case ME_NOTEON:
      if(_b == 0)  // Is it really a note off?
        return 7;  
      return 98;  
    case ME_NOTEOFF:
      return 7;
      
    case ME_PITCHBEND:
      return 25;  
    case ME_CONTROLLER:
      switch(_a)
      {
        case CTRL_HBANK:
        case CTRL_LBANK:
          return 20;

        case CTRL_PROGRAM:
          return 21;  
        default:
          return 24;
      }
    case ME_PROGRAM:
      return 21;

    case ME_CLOCK:
      return 0;  
    case ME_MTC_QUARTER:
      return 1;  
    case ME_TICK:
      return 2;  
    case ME_SENSE:
      return 3;  

    case ME_SYSEX_END:
      return 4;  
    case ME_AFTERTOUCH:
      return 5;  
    case ME_POLYAFTER:
      return 6;  
    case ME_STOP:
      return 8;  

    case ME_SONGSEL:
      return 9;  
    case ME_SYSEX:
      return 18;  
    case ME_META:
      switch(_a)
      {
        case ME_META_TEXT_2_COPYRIGHT:
          return 10;
        case ME_META_TEXT_1_COMMENT:
          return 11; 
        case ME_META_PORT_CHANGE:
          return 12; 
        case ME_META_TEXT_9_DEVICE_NAME:
          return 13; 
        case ME_META_CHANNEL_CHANGE:
          return 14;
          
        case ME_META_TEXT_3_TRACK_NAME:
          return 15; 
        case ME_META_TEXT_F_TRACK_COMMENT:  
          return 16; 
        case ME_META_TEXT_0_SEQUENCE_NUMBER:
          return 17; 

        case ME_META_TEXT_4_INSTRUMENT_NAME:
          return 19; 
        case ME_META_END_OF_TRACK:
          return 99; 
        default:  
          return 97;
      }

    case ME_TUNE_REQ:
      return 22;  
    case ME_SONGPOS:
      return 23;  

    case ME_START:
      return 26;  
    case ME_CONTINUE:
      return 27;  
  }
  
  fprintf(stderr, "FIXME: MEvent::sortingWeight: unknown event type:%d\n", _type);
  return 100;
}
      
//---------------------------------------------------------
//   operator <
//---------------------------------------------------------

bool MEvent::operator<(const MEvent& e) const
      {
      // Be careful about being any more specific than these checks.
      // Be sure to examine the add() method, which might be upset by it.

      if (time() != e.time())
            return time() < e.time();
      if (port() != e.port())
            return port() < e.port();

      // Do not compare channels if either event is a channel-less event.
      // In that case the sorting weight function will do all the work.
      switch(e.type())
      {
        case ME_SYSEX:
        case ME_SYSEX_END:
        case ME_META:
        case ME_START:
        case ME_STOP:
        case ME_CONTINUE:
        case ME_TUNE_REQ:
        case ME_SONGPOS:
        case ME_SONGSEL:
        case ME_CLOCK:
        case ME_MTC_QUARTER:
        case ME_TICK:
        case ME_SENSE:
          return sortingWeight() < e.sortingWeight();
        break;
        default:
        break;
      }
      switch(type())
      {
        case ME_SYSEX:
        case ME_SYSEX_END:
        case ME_META:
        case ME_START:
        case ME_STOP:
        case ME_CONTINUE:
        case ME_TUNE_REQ:
        case ME_SONGPOS:
        case ME_SONGSEL:
        case ME_CLOCK:
        case ME_MTC_QUARTER:
        case ME_TICK:
        case ME_SENSE:
          return sortingWeight() < e.sortingWeight();
        break;
        default:
        break;
      }

      // play note off events first to prevent overlapping
      // notes

      if (channel() == e.channel())
        return sortingWeight() < e.sortingWeight();

      int map[16] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 0, 10, 11, 12, 13, 14, 15 };
      return map[channel()] < map[e.channel()];
      }

int MEvent::port()    const      { return _port;    }
int MEvent::channel() const      { return _channel; }
int MEvent::type()    const      { return _type;    }
int MEvent::dataA()   const      { return _a;       }
int MEvent::dataB()   const      { return _b;       }
unsigned MEvent::time() const    { return _time;    }
int MEvent::loopNum() const      { return _loopNum; }

void MEvent::setPort(int val)    { _port = val;     }
void MEvent::setChannel(int val) { _channel = val;  }
void MEvent::setType(int val)    { _type = val;     }
void MEvent::setA(int val)       { _a = val;        }
void MEvent::setB(int val)       { _b = val;        }
void MEvent::setTime(unsigned val) { _time = val;   }
void MEvent::setLoopNum(int n)   { _loopNum = n;    }

const EvData& MEvent::eventData() const        { return edata; }
unsigned char* MEvent::data()                  { return edata.data(); }
const unsigned char* MEvent::constData() const { return edata.constData(); }
int MEvent::len() const                        { return edata.dataLen(); }
void MEvent::setData(const EvData& e)          { edata = e; }
void MEvent::setData(const unsigned char* p, int len) { edata.setData(p, len); }

bool MEvent::isNote() const                    { return _type == 0x90; }
bool MEvent::isNoteOff() const                 { return (_type == 0x80)||(_type == 0x90 && _b == 0); }
bool MEvent::isValid() const                   { return _type != 0; }

bool MEvent::isStandardRPN() const
{
  switch(type())
  {
    case ME_CONTROLLER:
    {
      const int da = dataA();
      switch(da)
      {
        case CTRL_HRPN:
        case CTRL_LRPN:
        case CTRL_HNRPN:
        case CTRL_LNRPN:
        case CTRL_HDATA:
        case CTRL_LDATA:
        case CTRL_DATA_DEC:
        case CTRL_DATA_INC:
          return true;
        break;
        default:
        break;
      }
    }
    break;
    default:
    break;
  }
  return false;
}

bool MEvent::isNativeRPN() const
{
  switch(type())
  {
    case ME_CONTROLLER:
    {
      const int offs = dataA() & CTRL_OFFSET_MASK;
      switch(offs)
      {
        case CTRL_RPN_OFFSET:
        case CTRL_NRPN_OFFSET:
        case CTRL_RPN14_OFFSET:
        case CTRL_NRPN14_OFFSET:
          return true;
        break;
        default:
        break;
      }
    }
    break;
    default:
    break;
  }
  return false;
}

//---------------------------------------------------------
//  translateCtrlNum
//---------------------------------------------------------

int MEvent::translateCtrlNum() const
{
  const int da = dataA();
  int ctrl = -1;

  switch(type())
  {
    case ME_CONTROLLER:
      switch(da)
      {
        case CTRL_HBANK:
          ctrl = CTRL_PROGRAM;
        break;

        case CTRL_LBANK:
          ctrl = CTRL_PROGRAM;
        break;

        case CTRL_PROGRAM:
          ctrl = CTRL_PROGRAM;
        break;
        
        default:
          ctrl = da;
        break;
      }
    break;
    
    case ME_POLYAFTER:
    {
      const int pitch = da & 0x7f;
      ctrl = (CTRL_POLYAFTER & ~0xff) | pitch;
    }
    break;
    
    case ME_AFTERTOUCH:
      ctrl = CTRL_AFTERTOUCH;
    break;
    
    case ME_PITCHBEND:
      ctrl = CTRL_PITCH;
    break;
    
    case ME_PROGRAM:
      ctrl = CTRL_PROGRAM;
    break;
    
    default:
    break;
  }
  
  return ctrl;
}

MidiRecordEvent::MidiRecordEvent() : MEvent(), _tick(0) {}
MidiRecordEvent::MidiRecordEvent(const MidiRecordEvent& e) : MEvent(e), _tick(e._tick) {}
MidiRecordEvent::MidiRecordEvent(const MEvent& e) : MEvent(e), _tick(0) {}
MidiRecordEvent::MidiRecordEvent(unsigned tm, int p, int c, int t, int a, int b)
  : MEvent(tm, p, c, t, a, b), _tick(0) {}
MidiRecordEvent::MidiRecordEvent(unsigned t, int p, int tpe, const unsigned char* data, int len)
  : MEvent(t, p, tpe, data, len), _tick(0) {}
MidiRecordEvent::MidiRecordEvent(unsigned t, int p, int type, EvData data)
  : MEvent(t, p, type, data), _tick(0) {}
MidiRecordEvent::~MidiRecordEvent() {}
MidiRecordEvent& MidiRecordEvent::operator=(const MidiRecordEvent& e) { MEvent::operator=(e); _tick = e._tick; return *this; }
unsigned int MidiRecordEvent::tick() {return _tick;}
void MidiRecordEvent::setTick(unsigned int tick) {_tick = tick;}


MidiPlayEvent::MidiPlayEvent() : MEvent(), _latency(0) {}
MidiPlayEvent::MidiPlayEvent(const MidiPlayEvent& e) : MEvent(e), _latency(e._latency) {}
MidiPlayEvent::MidiPlayEvent(const MEvent& e) : MEvent(e), _latency(0) {}
MidiPlayEvent::MidiPlayEvent(unsigned tm, int p, int c, int t, int a, int b)
  : MEvent(tm, p, c, t, a, b), _latency(0) {}
MidiPlayEvent::MidiPlayEvent(unsigned t, int p, int type, const unsigned char* data, int len)
  : MEvent(t, p, type, data, len), _latency(0) {}
MidiPlayEvent::MidiPlayEvent(unsigned t, int p, int type, EvData data)
  : MEvent(t, p, type, data), _latency(0) {}
MidiPlayEvent::~MidiPlayEvent() {}
MidiPlayEvent& MidiPlayEvent::operator=(const MidiPlayEvent& e) { MEvent::operator=(e); _latency = e._latency; return *this; }
int MidiPlayEvent::latency() {return _latency;}
void MidiPlayEvent::setLatency(int latency) {_latency = latency;}

//---------------------------------------------------------
//   add
//    Optimize to eliminate duplicate events at the SAME time.
//    It will not handle duplicate events at DIFFERENT times.
//    Replaces event if it already exists.
//---------------------------------------------------------

void MPEventList::add(const MidiPlayEvent& ev)
{
  switch((ME_EVENT_TYPE)ev.type())
  {
    case ME_CONTROLLER:
      switch(ev.dataA())
      {
        // Don't touch these, just insert normally.
        case CTRL_DATA_DEC:
        case CTRL_DATA_INC:
          insert(ev);
          return;
        break;
      }
    break;

    // These should be allowed to be added. Just insert normally.
    case ME_CLOCK:
    case ME_MTC_QUARTER:
    case ME_START:
    case ME_CONTINUE:
    case ME_STOP:
    case ME_SYSEX:
    case ME_SYSEX_END:
    case ME_TUNE_REQ:
    case ME_TICK:
    case ME_SENSE:
    case ME_META: // This could be reset, or might be a meta, depending on MPEventList usage.
      insert(ev);
      return;
    break;

    default:
    break;
  }

  bool rpnFound = false;
  bool dataFound = false;
  bool patchOrSysexFound = false;
  bool canOptimizePatch = true;

  MPEventListRangePair_t range = equal_range(ev);
  if(range.first != end())
  {
    iterator impe = range.second;
    while(impe != range.first)
    {
      --impe;

      // Note that (multi)set iterators are constant and can't be modified.
      // The only option is to erase the old item(s), then insert a new item.
      const MidiPlayEvent& mpe = *impe;

      // The port and channel should be equal.
      if(mpe.port() != ev.port() || mpe.channel() != ev.channel())
        continue;

      switch((ME_EVENT_TYPE)mpe.type())
      {
        case ME_CONTROLLER:
          switch(mpe.dataA())
          {
            case CTRL_HDATA:
            case CTRL_LDATA:
            case CTRL_DATA_DEC:
            case CTRL_DATA_INC:
              dataFound = true;
              canOptimizePatch = false;
            break;

            case CTRL_HRPN:
            case CTRL_LRPN:
            case CTRL_HNRPN:
            case CTRL_LNRPN:
              rpnFound = true;
              canOptimizePatch = false;
            break;

            case CTRL_HBANK:
            case CTRL_LBANK:
            case CTRL_PROGRAM:
              patchOrSysexFound = true;
            break;

            default:
              canOptimizePatch = false;
            break;
          }
        break;

        case ME_NOTEON:
        case ME_NOTEOFF:
        case ME_AFTERTOUCH:
        case ME_POLYAFTER:
        case ME_PITCHBEND:
          canOptimizePatch = false;
        break;

        case ME_META:
        case ME_SYSEX:
        case ME_SYSEX_END:
          canOptimizePatch = false;
          patchOrSysexFound = true;
        break;

        case ME_PROGRAM:
          patchOrSysexFound = true;
        break;

        default:
          canOptimizePatch = false;
        break;
      }

      // The type should be equal beyond this point.
      if(mpe.type() != ev.type())
        continue;

      switch((ME_EVENT_TYPE)ev.type())
      {
        case ME_CONTROLLER:
        {
          // Are the controller numbers the same?
          if(mpe.dataA() != ev.dataA())
            continue;
          // If the values are the same, just return.
          if(mpe.dataB() == ev.dataB())
            return;

          switch(ev.dataA())
          {
            case CTRL_HRPN:
            case CTRL_LRPN:
            case CTRL_HNRPN:
            case CTRL_LNRPN:
              // If a data controller or patch or sysex came after this, don't touch this, just insert normally.
              if(patchOrSysexFound || dataFound)
              {
                insert(ev);
                return;
              }
            break;

            case CTRL_HDATA:
            case CTRL_LDATA:
              // If an (N)RPN controller or patch or sysex came after this, don't touch this, just insert normally.
              if(patchOrSysexFound || rpnFound)
              {
                insert(ev);
                return;
              }
            break;

            case CTRL_HBANK:
            case CTRL_LBANK:
            case CTRL_PROGRAM:
              // If there are certain other events after this, don't touch this, just insert normally.
              if(!canOptimizePatch)
              {
                insert(ev);
                return;
              }
            break;

            default:
              // If a patch or sysex came after this, don't touch this, just insert normally.
              if(patchOrSysexFound)
              {
                insert(ev);
                return;
              }
            break;
          }

          // Erase the item, and insert the replacement.
          // Note this will NOT eliminate any FURTHER duplicates that may have already existed. Only the last one found.
          erase(impe);
          insert(ev);
          return;
        }
        break;

        case ME_NOTEON:
        case ME_NOTEOFF:
        case ME_POLYAFTER:
        {
          // Are the note numbers the same?
          if(mpe.dataA() != ev.dataA())
            continue;
          // If the values are the same, just return.
          if(mpe.dataB() == ev.dataB())
            return;
          // If a patch or sysex came after this, don't touch this, just insert normally.
          if(patchOrSysexFound)
          {
            insert(ev);
            return;
          }
          // Erase the item, and insert the replacement.
          // Note this will NOT eliminate any FURTHER duplicates that may have already existed. Only the last one found.
          erase(impe);
          insert(ev);
          return;
        }
        break;

        case ME_PROGRAM:
        case ME_AFTERTOUCH:
        case ME_PITCHBEND:
        case ME_SONGPOS:
        case ME_SONGSEL:
        {
            // If the values are the same, just return.
            if(mpe.dataA() == ev.dataA())
              return;
            // If this is ME_PROGRAM and there are certain other events after this, don't touch this, just insert normally.
            // Or if this is not ME_PROGRAM and a patch or sysex came after this, don't touch this, just insert normally.
            if((!canOptimizePatch && ev.type() == ME_PROGRAM) ||
              (patchOrSysexFound && ev.type() != ME_PROGRAM))
            {
              insert(ev);
              return;
            }
            // Erase the item, and insert the replacement.
            // Note this will NOT eliminate any FURTHER duplicates that may have already existed. Only the last one found.
            erase(impe);
            insert(ev);
            return;
        }
        break;

        default:
        break;
      }
    }
  }

  insert(ev);
}

void MPEventList::addExclusive(const MidiPlayEvent& ev, bool RPNControllersReserved)
{
  switch((ME_EVENT_TYPE)ev.type())
  {
    case ME_CONTROLLER:
      switch(ev.dataA())
      {
        // Don't touch these, just insert normally.
        case CTRL_DATA_DEC:
        case CTRL_DATA_INC:
          if(!RPNControllersReserved)
          {
            insert(ev);
            return;
          }
        break;
      }
    break;

    // Do not allow ANY note-ons to be added. This routine was designed for restrictive adding
    //  when the target is 'off' or 'inactive'. When the target becomes 'on' or 'active' again,
    //  we do not want a startling sudden flood of notes that were waiting in this list.
    case ME_NOTEON:
    // Do not allow start or continue to be added. They would be awkward when the device becomes active again.
    // Do not allow clocks, ticks or sense to be added. They could easily overflow the buffer.
    case ME_START:
    case ME_CONTINUE:
    case ME_CLOCK:
    case ME_TICK:
    case ME_MTC_QUARTER:
    case ME_SENSE:
      return;
    break;

    // These should be allowed to be added. Just insert normally.
    case ME_SYSEX:
    case ME_SYSEX_END:
    case ME_STOP:
    case ME_TUNE_REQ:
    case ME_META: // This could be reset, or might be a meta, depending on MPEventList usage.
      insert(ev);
      return;
    break;

    default:
    break;
  }

  bool rpnFound = false;
  bool dataFound = false;
  bool patchOrSysexFound = false;
  bool canOptimizePatch = true;
  for(reverse_iterator impe = rbegin(); impe != rend(); ++impe)
  {
    // Note that (multi)set iterators are constant and can't be modified.
    // The only option is to erase the old item(s), then insert a new item.
    const MidiPlayEvent& mpe = *impe;

    // The port and channel should be equal.
    if(mpe.port() != ev.port() || mpe.channel() != ev.channel())
      continue;

    switch((ME_EVENT_TYPE)mpe.type())
    {
      case ME_CONTROLLER:
        switch(mpe.dataA())
        {
          case CTRL_HDATA:
          case CTRL_LDATA:
          case CTRL_DATA_DEC:
          case CTRL_DATA_INC:
            if(!RPNControllersReserved)
              dataFound = true;
            canOptimizePatch = false;
          break;

          case CTRL_HRPN:
          case CTRL_LRPN:
          case CTRL_HNRPN:
          case CTRL_LNRPN:
            if(!RPNControllersReserved)
              rpnFound = true;
            canOptimizePatch = false;
          break;

          case CTRL_HBANK:
          case CTRL_LBANK:
          case CTRL_PROGRAM:
            patchOrSysexFound = true;
          break;

          default:
            canOptimizePatch = false;
          break;
        }
      break;

      case ME_NOTEOFF:
      case ME_AFTERTOUCH:
      case ME_POLYAFTER:
      case ME_PITCHBEND:
        canOptimizePatch = false;
      break;

      case ME_META:
      case ME_SYSEX:
      case ME_SYSEX_END:
        canOptimizePatch = false;
        patchOrSysexFound = true;
      break;

      case ME_PROGRAM:
        patchOrSysexFound = true;
      break;

      default:
        canOptimizePatch = false;
      break;
    }

    // The type should be equal beyond this point.
    if(mpe.type() != ev.type())
      continue;

    switch((ME_EVENT_TYPE)ev.type())
    {
      case ME_CONTROLLER:
      {
        // Are the controller numbers the same?
        if(mpe.dataA() != ev.dataA())
          continue;
        // If the existing event's time is greater, don't touch it, just return.
        // Or if the values and times are the same, just return.
        if(mpe.time() > ev.time() || (mpe.time() == ev.time() && mpe.dataB() == ev.dataB()))
          return;

        switch(ev.dataA())
        {
          case CTRL_HRPN:
          case CTRL_LRPN:
          case CTRL_HNRPN:
          case CTRL_LNRPN:
            // If a data controller or patch or sysex came after this, don't touch this, just insert normally.
            if(patchOrSysexFound || dataFound)
            {
              insert(ev);
              return;
            }
          break;

          case CTRL_HDATA:
          case CTRL_LDATA:
            // If an (N)RPN controller or patch or sysex came after this, don't touch this, just insert normally.
            if(patchOrSysexFound || rpnFound)
            {
              insert(ev);
              return;
            }
          break;

          case CTRL_HBANK:
          case CTRL_LBANK:
          case CTRL_PROGRAM:
            // If there are certain other events after this, don't touch this, just insert normally.
            if(!canOptimizePatch)
            {
              insert(ev);
              return;
            }
          break;

          default:
            // If a patch or sysex came after this, don't touch this, just insert normally.
            if(patchOrSysexFound)
            {
              insert(ev);
              return;
            }
          break;
        }

        // Erase the item, and insert the replacement.
        // Note this will NOT eliminate any FURTHER duplicates that may have already existed. Only the last one found.
        iterator base_impe = impe.base();
        --base_impe;
        erase(base_impe);
        insert(ev);
        return;
      }
      break;

      // Note-offs need to be allowed since there may have been notes playing when the device went off or inactive.
      // But optimize them to prevent redundancies.
      case ME_NOTEOFF:
      case ME_POLYAFTER:
      {
        // Are the note numbers the same?
        if(mpe.dataA() != ev.dataA())
          continue;
        // If the existing event's time is greater, don't touch it, just return.
        // Or if the values and times are the same, just return.
        if(mpe.time() > ev.time() || (mpe.time() == ev.time() && mpe.dataB() == ev.dataB()))
          return;
        // If a patch or sysex came after this, don't touch this, just insert normally.
        if(patchOrSysexFound)
        {
          insert(ev);
          return;
        }
        // Erase the item, and insert the replacement.
        // Note this will NOT eliminate any FURTHER duplicates that may have already existed. Only the last one found.
        iterator base_impe = impe.base();
        --base_impe;
        erase(base_impe);
        insert(ev);
        return;
      }
      break;

      case ME_PROGRAM:
      case ME_AFTERTOUCH:
      case ME_PITCHBEND:
      case ME_SONGPOS:
      case ME_SONGSEL:
      {
          // If the existing event's time is greater, don't touch it, just return.
          // Or if the values and times are the same, just return.
          if(mpe.time() > ev.time() || (mpe.time() == ev.time() && mpe.dataA() == ev.dataA()))
            return;
          // If this is ME_PROGRAM and there are certain other events after this, don't touch this, just insert normally.
          // Or if this is not ME_PROGRAM and a patch or sysex came after this, don't touch this, just insert normally.
          if((!canOptimizePatch && ev.type() == ME_PROGRAM) ||
             (patchOrSysexFound && ev.type() != ME_PROGRAM))
          {
            insert(ev);
            return;
          }
          // Erase the item, and insert the replacement.
          // Note this will NOT eliminate any FURTHER duplicates that may have already existed. Only the last one found.
          iterator base_impe = impe.base();
          --base_impe;
          erase(base_impe);
          insert(ev);
          return;
      }
      break;

      default:
      break;
    }
  }

  insert(ev);
}

//---------------------------------------------------------
//   add
//    Optimize to eliminate duplicate events at the SAME time.
//    It will not handle duplicate events at DIFFERENT times.
//    Replaces event if it already exists.
//---------------------------------------------------------

void SeqMPEventList::add(const MidiPlayEvent& ev)
{
  switch((ME_EVENT_TYPE)ev.type())
  {
    case ME_CONTROLLER:
      switch(ev.dataA())
      {
        // Don't touch these, just insert normally.
        case CTRL_DATA_DEC:
        case CTRL_DATA_INC:
          insert(ev);
          return;
        break;
      }
    break;

    // These should be allowed to be added. Just insert normally.
    case ME_CLOCK:
    case ME_MTC_QUARTER:
    case ME_START:
    case ME_CONTINUE:
    case ME_STOP:
    case ME_SYSEX:
    case ME_SYSEX_END:
    case ME_TUNE_REQ:
    case ME_TICK:
    case ME_SENSE:
    case ME_META: // This could be reset, or might be a meta, depending on MPEventList usage.
      insert(ev);
      return;
    break;

    default:
    break;
  }

  bool rpnFound = false;
  bool dataFound = false;
  bool patchOrSysexFound = false;
  bool canOptimizePatch = true;

  SeqMPEventListRangePair_t range = equal_range(ev);
  if(range.first != end())
  {
    iterator impe = range.second;
    while(impe != range.first)
    {
      --impe;

      // Note that (multi)set iterators are constant and can't be modified.
      // The only option is to erase the old item(s), then insert a new item.
      const MidiPlayEvent& mpe = *impe;

      // The port and channel should be equal.
      if(mpe.port() != ev.port() || mpe.channel() != ev.channel())
        continue;

      switch((ME_EVENT_TYPE)mpe.type())
      {
        case ME_CONTROLLER:
          switch(mpe.dataA())
          {
            case CTRL_HDATA:
            case CTRL_LDATA:
            case CTRL_DATA_DEC:
            case CTRL_DATA_INC:
              dataFound = true;
              canOptimizePatch = false;
            break;

            case CTRL_HRPN:
            case CTRL_LRPN:
            case CTRL_HNRPN:
            case CTRL_LNRPN:
              rpnFound = true;
              canOptimizePatch = false;
            break;

            case CTRL_HBANK:
            case CTRL_LBANK:
            case CTRL_PROGRAM:
              patchOrSysexFound = true;
            break;

            default:
              canOptimizePatch = false;
            break;
          }
        break;

        case ME_NOTEON:
        case ME_NOTEOFF:
        case ME_AFTERTOUCH:
        case ME_POLYAFTER:
        case ME_PITCHBEND:
          canOptimizePatch = false;
        break;

        case ME_META:
        case ME_SYSEX:
        case ME_SYSEX_END:
          canOptimizePatch = false;
          patchOrSysexFound = true;
        break;

        case ME_PROGRAM:
          patchOrSysexFound = true;
        break;

        default:
          canOptimizePatch = false;
        break;
      }

      // The type should be equal beyond this point.
      if(mpe.type() != ev.type())
        continue;

      switch((ME_EVENT_TYPE)ev.type())
      {
        case ME_CONTROLLER:
        {
          // Are the controller numbers the same?
          if(mpe.dataA() != ev.dataA())
            continue;
          // If the values are the same, just return.
          if(mpe.dataB() == ev.dataB())
            return;

          switch(ev.dataA())
          {
            case CTRL_HRPN:
            case CTRL_LRPN:
            case CTRL_HNRPN:
            case CTRL_LNRPN:
              // If a data controller or patch or sysex came after this, don't touch this, just insert normally.
              if(patchOrSysexFound || dataFound)
              {
                insert(ev);
                return;
              }
            break;

            case CTRL_HDATA:
            case CTRL_LDATA:
              // If an (N)RPN controller or patch or sysex came after this, don't touch this, just insert normally.
              if(patchOrSysexFound || rpnFound)
              {
                insert(ev);
                return;
              }
            break;

            case CTRL_HBANK:
            case CTRL_LBANK:
            case CTRL_PROGRAM:
              // If there are certain other events after this, don't touch this, just insert normally.
              if(!canOptimizePatch)
              {
                insert(ev);
                return;
              }
            break;

            default:
              // If a patch or sysex came after this, don't touch this, just insert normally.
              if(patchOrSysexFound)
              {
                insert(ev);
                return;
              }
            break;
          }

          // Erase the item, and insert the replacement.
          // Note this will NOT eliminate any FURTHER duplicates that may have already existed. Only the last one found.
          erase(impe);
          insert(ev);
          return;
        }
        break;

        case ME_NOTEON:
        case ME_NOTEOFF:
        case ME_POLYAFTER:
        {
          // Are the note numbers the same?
          if(mpe.dataA() != ev.dataA())
            continue;
          // If the values are the same, just return.
          if(mpe.dataB() == ev.dataB())
            return;
          // If a patch or sysex came after this, don't touch this, just insert normally.
          if(patchOrSysexFound)
          {
            insert(ev);
            return;
          }
          // Erase the item, and insert the replacement.
          // Note this will NOT eliminate any FURTHER duplicates that may have already existed. Only the last one found.
          erase(impe);
          insert(ev);
          return;
        }
        break;

        case ME_PROGRAM:
        case ME_AFTERTOUCH:
        case ME_PITCHBEND:
        case ME_SONGPOS:
        case ME_SONGSEL:
        {
            // If the values are the same, just return.
            if(mpe.dataA() == ev.dataA())
              return;
            // If this is ME_PROGRAM and there are certain other events after this, don't touch this, just insert normally.
            // Or if this is not ME_PROGRAM and a patch or sysex came after this, don't touch this, just insert normally.
            if((!canOptimizePatch && ev.type() == ME_PROGRAM) ||
              (patchOrSysexFound && ev.type() != ME_PROGRAM))
            {
              insert(ev);
              return;
            }
            // Erase the item, and insert the replacement.
            // Note this will NOT eliminate any FURTHER duplicates that may have already existed. Only the last one found.
            erase(impe);
            insert(ev);
            return;
        }
        break;

        default:
        break;
      }
    }
  }

  insert(ev);
}

void SeqMPEventList::addExclusive(const MidiPlayEvent& ev, bool RPNControllersReserved)
{
  switch((ME_EVENT_TYPE)ev.type())
  {
    case ME_CONTROLLER:
      switch(ev.dataA())
      {
        // Don't touch these, just insert normally.
        case CTRL_DATA_DEC:
        case CTRL_DATA_INC:
          if(!RPNControllersReserved)
          {
            insert(ev);
            return;
          }
        break;
      }
    break;

    // Do not allow ANY note-ons to be added. This routine was designed for restrictive adding
    //  when the target is 'off' or 'inactive'. When the target becomes 'on' or 'active' again,
    //  we do not want a startling sudden flood of notes that were waiting in this list.
    case ME_NOTEON:
    // Do not allow start or continue to be added. They would be awkward when the device becomes active again.
    // Do not allow clocks, ticks or sense to be added. They could easily overflow the buffer.
    case ME_START:
    case ME_CONTINUE:
    case ME_CLOCK:
    case ME_TICK:
    case ME_MTC_QUARTER:
    case ME_SENSE:
      return;
    break;

    // These should be allowed to be added. Just insert normally.
    case ME_SYSEX:
    case ME_SYSEX_END:
    case ME_STOP:
    case ME_TUNE_REQ:
    case ME_META: // This could be reset, or might be a meta, depending on MPEventList usage.
      insert(ev);
      return;
    break;

    default:
    break;
  }

  bool rpnFound = false;
  bool dataFound = false;
  bool patchOrSysexFound = false;
  bool canOptimizePatch = true;
  for(reverse_iterator impe = rbegin(); impe != rend(); ++impe)
  {
    // Note that (multi)set iterators are constant and can't be modified.
    // The only option is to erase the old item(s), then insert a new item.
    const MidiPlayEvent& mpe = *impe;

    // The port and channel should be equal.
    if(mpe.port() != ev.port() || mpe.channel() != ev.channel())
      continue;

    switch((ME_EVENT_TYPE)mpe.type())
    {
      case ME_CONTROLLER:
        switch(mpe.dataA())
        {
          case CTRL_HDATA:
          case CTRL_LDATA:
          case CTRL_DATA_DEC:
          case CTRL_DATA_INC:
            if(!RPNControllersReserved)
              dataFound = true;
            canOptimizePatch = false;
          break;

          case CTRL_HRPN:
          case CTRL_LRPN:
          case CTRL_HNRPN:
          case CTRL_LNRPN:
            if(!RPNControllersReserved)
              rpnFound = true;
            canOptimizePatch = false;
          break;

          case CTRL_HBANK:
          case CTRL_LBANK:
          case CTRL_PROGRAM:
            patchOrSysexFound = true;
          break;

          default:
            canOptimizePatch = false;
          break;
        }
      break;

      case ME_NOTEOFF:
      case ME_AFTERTOUCH:
      case ME_POLYAFTER:
      case ME_PITCHBEND:
        canOptimizePatch = false;
      break;

      case ME_META:
      case ME_SYSEX:
      case ME_SYSEX_END:
        canOptimizePatch = false;
        patchOrSysexFound = true;
      break;

      case ME_PROGRAM:
        patchOrSysexFound = true;
      break;

      default:
        canOptimizePatch = false;
      break;
    }

    // The type should be equal beyond this point.
    if(mpe.type() != ev.type())
      continue;

    switch((ME_EVENT_TYPE)ev.type())
    {
      case ME_CONTROLLER:
      {
        // Are the controller numbers the same?
        if(mpe.dataA() != ev.dataA())
          continue;
        // If the existing event's time is greater, don't touch it, just return.
        // Or if the values and times are the same, just return.
        if(mpe.time() > ev.time() || (mpe.time() == ev.time() && mpe.dataB() == ev.dataB()))
          return;

        switch(ev.dataA())
        {
          case CTRL_HRPN:
          case CTRL_LRPN:
          case CTRL_HNRPN:
          case CTRL_LNRPN:
            // If a data controller or patch or sysex came after this, don't touch this, just insert normally.
            if(patchOrSysexFound || dataFound)
            {
              insert(ev);
              return;
            }
          break;

          case CTRL_HDATA:
          case CTRL_LDATA:
            // If an (N)RPN controller or patch or sysex came after this, don't touch this, just insert normally.
            if(patchOrSysexFound || rpnFound)
            {
              insert(ev);
              return;
            }
          break;

          case CTRL_HBANK:
          case CTRL_LBANK:
          case CTRL_PROGRAM:
            // If there are certain other events after this, don't touch this, just insert normally.
            if(!canOptimizePatch)
            {
              insert(ev);
              return;
            }
          break;

          default:
            // If a patch or sysex came after this, don't touch this, just insert normally.
            if(patchOrSysexFound)
            {
              insert(ev);
              return;
            }
          break;
        }

        // Erase the item, and insert the replacement.
        // Note this will NOT eliminate any FURTHER duplicates that may have already existed. Only the last one found.
        iterator base_impe = impe.base();
        --base_impe;
        erase(base_impe);
        insert(ev);
        return;
      }
      break;

      // Note-offs need to be allowed since there may have been notes playing when the device went off or inactive.
      // But optimize them to prevent redundancies.
      case ME_NOTEOFF:
      case ME_POLYAFTER:
      {
        // Are the note numbers the same?
        if(mpe.dataA() != ev.dataA())
          continue;
        // If the existing event's time is greater, don't touch it, just return.
        // Or if the values and times are the same, just return.
        if(mpe.time() > ev.time() || (mpe.time() == ev.time() && mpe.dataB() == ev.dataB()))
          return;
        // If a patch or sysex came after this, don't touch this, just insert normally.
        if(patchOrSysexFound)
        {
          insert(ev);
          return;
        }
        // Erase the item, and insert the replacement.
        // Note this will NOT eliminate any FURTHER duplicates that may have already existed. Only the last one found.
        iterator base_impe = impe.base();
        --base_impe;
        erase(base_impe);
        insert(ev);
        return;
      }
      break;

      case ME_PROGRAM:
      case ME_AFTERTOUCH:
      case ME_PITCHBEND:
      case ME_SONGPOS:
      case ME_SONGSEL:
      {
          // If the existing event's time is greater, don't touch it, just return.
          // Or if the values and times are the same, just return.
          if(mpe.time() > ev.time() || (mpe.time() == ev.time() && mpe.dataA() == ev.dataA()))
            return;
          // If this is ME_PROGRAM and there are certain other events after this, don't touch this, just insert normally.
          // Or if this is not ME_PROGRAM and a patch or sysex came after this, don't touch this, just insert normally.
          if((!canOptimizePatch && ev.type() == ME_PROGRAM) ||
             (patchOrSysexFound && ev.type() != ME_PROGRAM))
          {
            insert(ev);
            return;
          }
          // Erase the item, and insert the replacement.
          // Note this will NOT eliminate any FURTHER duplicates that may have already existed. Only the last one found.
          iterator base_impe = impe.base();
          --base_impe;
          erase(base_impe);
          insert(ev);
          return;
      }
      break;

      default:
      break;
    }
  }

  insert(ev);
}

} // namespace MusECore
