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

MEvent::MEvent(unsigned t, int port, int tpe, const unsigned char* data, int len)
      {
      _time = t;
      _port = port;
      edata.setData(data, len);
      _type = tpe;
      _loopNum = 0;
      setChannel(0);
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
        case CTRL_PROGRAM:
          return 21;  
        default:
          return 24;
      }
    case ME_PROGRAM:
      return 20;  
      
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

      // play note off events first to prevent overlapping
      // notes

      if (channel() == e.channel())
        return sortingWeight() < e.sortingWeight();

      int map[16] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 0, 10, 11, 12, 13, 14, 15 };
      return map[channel()] < map[e.channel()];
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

//---------------------------------------------------------
//   add
//    Optimize to eliminate duplicate events at the SAME time.
//    It will not handle duplicate events at DIFFERENT times.
//    Replaces event if it already exists.
//---------------------------------------------------------

void MPEventList::add(const MidiPlayEvent& ev)
{
  MPEventListRangePair_t range = equal_range(ev);

  for(iMPEvent impe = range.first; impe != range.second; ++impe)
  {
    // Note that (multi)set iterators are constant and can't be modified.
    // The only option is to erase the old item(s), then insert a new item.
    const MidiPlayEvent& l_ev = *impe;

    // The type, time, port, and channel should already be equal, according to the operator< method.
    switch(ev.type())
    {
      case ME_NOTEON:
      case ME_NOTEOFF:
      case ME_CONTROLLER:
      case ME_POLYAFTER:
        // Are the notes or controller numbers the same?
        if(l_ev.dataA() == ev.dataA())
        {
          // If the velocities or values are the same, just ignore.
          if(l_ev.dataB() == ev.dataB())
            return;
          // Erase the item, and insert the replacement.
          erase(impe);
          insert(ev);
          return;
        }
      break;

      case ME_PROGRAM:
      case ME_AFTERTOUCH:
      case ME_PITCHBEND:
      case ME_SONGPOS:
      case ME_MTC_QUARTER:
      case ME_SONGSEL:
          // If the values are the same, just ignore.
          if(l_ev.dataA() == ev.dataA())
            return;
          // Erase the item, and insert the replacement.
          erase(impe);
          insert(ev);
          return;
      break;

      case ME_SYSEX:
      {
        const int len = ev.len();
        // If length is zero there's no point in adding this sysex. Just return.
        if(len == 0)
          return;
      }
      break;

      case ME_CLOCK:
      case ME_START:
      case ME_CONTINUE:
      case ME_STOP:
      case ME_SYSEX_END:
      case ME_TUNE_REQ:
      case ME_TICK:
      case ME_SENSE:
        // Event already exists. Ignore the event to be added.
        return;
      break;

      case ME_META: // TODO: This could be reset, or might be a meta, depending on MPEventList usage.
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
  SeqMPEventListRangePair_t range = equal_range(ev);

  for(iSeqMPEvent impe = range.first; impe != range.second; ++impe)
  {
    // Note that (multi)set iterators are constant and can't be modified.
    // The only option is to erase the old item(s), then insert a new item.
    const MidiPlayEvent& l_ev = *impe;

    // The type, time, port, and channel should already be equal, according to the operator< method.
    switch(ev.type())
    {
      case ME_NOTEON:
      case ME_NOTEOFF:
      case ME_CONTROLLER:
      case ME_POLYAFTER:
        // Are the notes or controller numbers the same?
        if(l_ev.dataA() == ev.dataA())
        {
          // If the velocities or values are the same, just ignore.
          if(l_ev.dataB() == ev.dataB())
            return;
          // Erase the item, and insert the replacement.
          erase(impe);
          insert(ev);
          return;
        }
      break;

      case ME_PROGRAM:
      case ME_AFTERTOUCH:
      case ME_PITCHBEND:
      case ME_SONGPOS:
      case ME_MTC_QUARTER:
      case ME_SONGSEL:
          // If the values are the same, just ignore.
          if(l_ev.dataA() == ev.dataA())
            return;
          // Erase the item, and insert the replacement.
          erase(impe);
          insert(ev);
          return;
      break;

      case ME_SYSEX:
      {
        const int len = ev.len();
        // If length is zero there's no point in adding this sysex. Just return.
        if(len == 0)
          return;
      }
      break;

      case ME_CLOCK:
      case ME_START:
      case ME_CONTINUE:
      case ME_STOP:
      case ME_SYSEX_END:
      case ME_TUNE_REQ:
      case ME_TICK:
      case ME_SENSE:
        // Event already exists. Ignore the event to be added.
        return;
      break;

      case ME_META: // TODO: This could be reset, or might be a meta, depending on MPEventList usage.
      break;
    }
  }
  insert(ev);
}

//---------------------------------------------------------
//   put
//    return true on fifo overflow
//---------------------------------------------------------

bool MidiRecFifo::put(const MidiRecordEvent& event)
      {
      if (size < MIDI_REC_FIFO_SIZE) {
            fifo[wIndex] = event;
            wIndex = (wIndex + 1) % MIDI_REC_FIFO_SIZE;
            ++size;
            return false;
            }
      return true;
      }

//---------------------------------------------------------
//   get
//---------------------------------------------------------

MidiRecordEvent MidiRecFifo::get()
      {
      MidiRecordEvent event(fifo[rIndex]);
      rIndex = (rIndex + 1) % MIDI_REC_FIFO_SIZE;
      --size;
      return event;
      }

//---------------------------------------------------------
//   peek
//---------------------------------------------------------

const MidiRecordEvent& MidiRecFifo::peek(int n)
      {
      int idx = (rIndex + n) % MIDI_REC_FIFO_SIZE;
      return fifo[idx];
      }

//---------------------------------------------------------
//   remove
//---------------------------------------------------------

void MidiRecFifo::remove()
      {
      rIndex = (rIndex + 1) % MIDI_REC_FIFO_SIZE;
      --size;
      }
      
} // namespace MusECore
