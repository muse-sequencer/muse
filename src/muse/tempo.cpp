//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: tempo.cpp,v 1.7.2.7 2008/05/21 00:28:52 terminator356 Exp $
//
//  (C) Copyright 1999/2000 Werner Schweer (ws@seh.de)
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

#include <stdio.h>
//#include <errno.h>
#include "muse_math.h"

#include "tempo.h"
#include "globals.h"
#include "gconfig.h"
#include "xml.h"
#include "config.h"

#include <stdint.h>

namespace MusEGlobal {
MusECore::TempoList tempomap;
MusECore::TempoRecList tempo_rec_list;
}

namespace MusECore {

//---------------------------------------------------------
//   TempoList
//---------------------------------------------------------

TempoList::TempoList()
      {
      _tempo   = 500000;
      insert(std::pair<const unsigned, TEvent*> (MAX_TICK+1, new TEvent(_tempo, 0)));
      _tempoSN     = 1;
      _globalTempo = 100;
      useList      = true;
      }

TempoList::~TempoList()
      {
      for (iTEvent i = begin(); i != end(); ++i)
            delete i->second;
      }

//---------------------------------------------------------
//   copy
//---------------------------------------------------------

void TempoList::copy(const TempoList& src)
{
  // Clear and delete the existing destination list.
  for (iTEvent i = begin(); i != end(); ++i)
    delete i->second;
  TEMPOLIST::clear();

  for (ciTEvent i = src.cbegin(); i != src.cend(); ++i)
  {
    TEvent* new_e = new TEvent(*i->second);
    std::pair<iTEvent, bool> res = insert(std::pair<const unsigned, TEvent*> (i->first, new_e));
    if(!res.second)
    {
      fprintf(stderr, "TempoList::copy insert failed: tempolist:%p tempo:%p %d tick:%d\n", 
                        this, new_e, new_e->tempo, new_e->tick);
    }
  }
}

//---------------------------------------------------------
//   add
//---------------------------------------------------------

void TempoList::add(unsigned tick, int tempo, bool do_normalize)
      {
#ifdef ALLOW_LEFT_HIDDEN_EVENTS
      if ((int)tick < 0)
            tick = 0;
#endif
      if (tick > MAX_TICK)
            tick = MAX_TICK;
      iTEvent e = upper_bound(tick);

      if (tick == e->second->tick)
            e->second->tempo = tempo;
      else {
            TEvent* ne = e->second;
            TEvent* ev = new TEvent(ne->tempo, ne->tick);
            ne->tempo  = tempo;
            ne->tick   = tick;
            insert(std::pair<const unsigned, TEvent*> (tick, ev));
            }
      if(do_normalize)      
        normalize();
      }


void TempoList::add(unsigned tick, TEvent* e, bool do_normalize)
{
#ifdef ALLOW_LEFT_HIDDEN_EVENTS
      if ((int)tick < 0)
            tick = 0;
#endif
  int tempo = e->tempo;
  std::pair<iTEvent, bool> res = insert(std::pair<const unsigned, TEvent*> (tick, e));
  if(!res.second)
  {
    fprintf(stderr, "TempoList::add insert failed: tempolist:%p tempo:%p %d tick:%d\n", 
                      this, e, tempo, e->tick);
  }
  else
  {
    iTEvent ine = res.first;
    ++ine; // There is always a 'next' tempo event - there is always one at index MAX_TICK + 1.
    TEvent* ne = ine->second;
    
    // Swap the values. (This is how the tempo list works.)
    e->tempo = ne->tempo;
    e->tick = ne->tick;
    ne->tempo = tempo;
    ne->tick = tick;
    
    if(do_normalize)      
      normalize();
  }
}

//---------------------------------------------------------
//   TempoList::normalize
//---------------------------------------------------------

void TempoList::normalize()
      {
      unsigned frame = 0;
      const uint64_t numer = (uint64_t)MusEGlobal::sampleRate;
      const uint64_t denom = (uint64_t)MusEGlobal::config.division * (uint64_t)_globalTempo * 10000UL;
      
      for (iTEvent e = begin(); e != end(); ++e) {
            e->second->frame = frame;
            // Tick resolution is less than frame resolution. 
            // Round up so that the reciprocal function (frame to tick) matches value for value.
            frame += muse_multiply_64_div_64_to_64(
              numer * (uint64_t)e->second->tempo, 
              e->first - e->second->tick,
              denom, LargeIntRoundUp);
            }
      // Invalidate all cached frame or tick values used in the program, such as in class Pos.
      // On the very next call of tick2frame() or frame2tick(), serial numbers are compared
      //  and if they are the same a cached value is returned.
      // Otherwise if the serial numbers are not the same the value is recalculated.
      ++_tempoSN;
      }

//---------------------------------------------------------
//   TempoList::dump
//---------------------------------------------------------

void TempoList::dump() const
      {
      printf("\nTempoList:\n");
      for (ciTEvent i = begin(); i != end(); ++i) {
            printf("%6d %06d Tempo %6d Frame %d\n",
               i->first, i->second->tick, i->second->tempo,
               i->second->frame);
            }
      }

//---------------------------------------------------------
//   clear
//---------------------------------------------------------

void TempoList::clear()
      {
      for (iTEvent i = begin(); i != end(); ++i)
            delete i->second;
      TEMPOLIST::clear();
      insert(std::pair<const unsigned, TEvent*> (MAX_TICK+1, new TEvent(500000, 0)));
      ++_tempoSN;
      }

//---------------------------------------------------------
//   eraseRange
//---------------------------------------------------------

void TempoList::eraseRange(unsigned stick, unsigned etick)
{
#ifdef ALLOW_LEFT_HIDDEN_EVENTS
      if ((int)stick < 0)
            stick = 0;
      if ((int)etick < 0)
            etick = 0;
#endif
    if(stick >= etick || stick > MAX_TICK)
      return;
    if(etick > MAX_TICK)
      etick = MAX_TICK;
    
    iTEvent se = MusEGlobal::tempomap.upper_bound(stick);
    if(se == end() || (se->first == MAX_TICK+1))
      return;

    iTEvent ee = MusEGlobal::tempomap.upper_bound(etick);

    ee->second->tempo = se->second->tempo;
    ee->second->tick = se->second->tick;

    for(iTEvent ite = se; ite != ee; ++ite)
      delete ite->second;
    erase(se, ee); // Erase range does NOT include the last element.
    normalize();
}
      
//---------------------------------------------------------
//   tempo
//---------------------------------------------------------

int TempoList::tempo(unsigned tick) const
      {
#ifdef ALLOW_LEFT_HIDDEN_EVENTS
      if ((int)tick < 0)
            tick = 0;
#endif
      if (useList) {
            ciTEvent i = upper_bound(tick);
            if (i == end()) {
                  printf("no TEMPO at tick %d,0x%x\n", tick, tick);
                  return 1000;
                  }
            return i->second->tempo;
            }
      else
            return _tempo;
      }

float TempoList::bpm(unsigned tick) const
      {
#ifdef ALLOW_LEFT_HIDDEN_EVENTS
      if ((int)tick < 0)
            tick = 0;
#endif
        return (float)globalTempo() * 600000.0f / (float)tempo(tick);
      }

float TempoList::bpmAt(unsigned tick) const
      {
#ifdef ALLOW_LEFT_HIDDEN_EVENTS
      if ((int)tick < 0)
            tick = 0;
#endif
        return (float)globalTempo() * 600000.0f / (float)tempoAt(tick);
      }

//---------------------------------------------------------
//   tempoAt
//   Bypass the useList flag and read from the list
//---------------------------------------------------------

int TempoList::tempoAt(unsigned tick) const
      {
#ifdef ALLOW_LEFT_HIDDEN_EVENTS
      if ((int)tick < 0)
            tick = 0;
#endif
            ciTEvent i = upper_bound(tick);
            if (i == end()) {
                  printf("tempoAt: no TEMPO at tick %d,0x%x\n", tick, tick);
                  return 1000;
                  }
            return i->second->tempo;
      }

//---------------------------------------------------------
//   del
//---------------------------------------------------------

void TempoList::del(unsigned tick, bool do_normalize)
      {
#ifdef ALLOW_LEFT_HIDDEN_EVENTS
      if ((int)tick < 0)
            tick = 0;
#endif
      iTEvent e = find(tick);
      if (e == end()) {
            printf("TempoList::del(%d): not found\n", tick);
            return;
            }
      del(e, do_normalize);
      }

void TempoList::del(iTEvent e, bool do_normalize)
      {
      iTEvent ne = e;
      ++ne;
      if (ne == end()) {
            printf("TempoList::del() HALLO\n");
            return;
            }
      ne->second->tempo = e->second->tempo;
      ne->second->tick  = e->second->tick;
      erase(e);
      if(do_normalize)
        normalize();
      }

//---------------------------------------------------------
//   setTempo
//    called from transport window
//    & slave mode tempo changes
//---------------------------------------------------------

void TempoList::setTempo(unsigned tick, int newTempo)
      {
#ifdef ALLOW_LEFT_HIDDEN_EVENTS
      if ((int)tick < 0)
            tick = 0;
#endif
      if (useList)
            add(tick, newTempo, true);
      else
      {
        setStaticTempo(newTempo);
      }
      }

//---------------------------------------------------------
//   setGlobalTempo
//---------------------------------------------------------

void TempoList::setGlobalTempo(int val)
      {
      _globalTempo = val;
      normalize();
      }

//---------------------------------------------------------
//   addTempo
//---------------------------------------------------------

void TempoList::addTempo(unsigned t, int tempo, bool do_normalize)
      {
#ifdef ALLOW_LEFT_HIDDEN_EVENTS
      if ((int)t < 0)
            t = 0;
#endif
      add(t, tempo, do_normalize);
      }

//---------------------------------------------------------
//   delTempo
//---------------------------------------------------------

void TempoList::delTempo(unsigned tick, bool do_normalize)
      {
#ifdef ALLOW_LEFT_HIDDEN_EVENTS
      if ((int)tick < 0)
            tick = 0;
#endif
      del(tick, do_normalize);
      }

//---------------------------------------------------------
//   setStaticTempo
//---------------------------------------------------------

void TempoList::setStaticTempo(int newTempo)
      {
      _tempo = newTempo;
      ++_tempoSN;
      }

//---------------------------------------------------------
//   setMasterFlag
//---------------------------------------------------------

bool TempoList::setMasterFlag(unsigned /*tick*/, bool val)
      {
      if (useList != val) {
            useList = val;
            ++_tempoSN;
            return true;
            }
      return false;
      }

//---------------------------------------------------------
//   ticks2frames
//---------------------------------------------------------

unsigned TempoList::ticks2frames(unsigned ticks, unsigned tempoTick, LargeIntRoundMode round_mode) const
{
  // Tick resolution is less than frame resolution. 
  // Round up so that the reciprocal function (frame to tick) matches value for value.
  return muse_multiply_64_div_64_to_64(
    (uint64_t)MusEGlobal::sampleRate * (uint64_t)tempo(tempoTick), ticks,
    (uint64_t)MusEGlobal::config.division * (uint64_t)_globalTempo * 10000UL, round_mode);
}

// TODO
// //---------------------------------------------------------
// //   frames2ticks
// //---------------------------------------------------------
// 
// unsigned TempoList::frames2ticks(unsigned frames, unsigned tempoTick) const
// {
//   const uint64_t numer = (uint64_t)MusEGlobal::config.division * (uint64_t)_globalTempo * 10000UL;
//   const uint64_t denom = (uint64_t)MusEGlobal::sampleRate;
//   return muse_multiply_64_div_64_to_64(numer, frames, denom * (uint64_t)tempo(tempoTick));
// }

//---------------------------------------------------------
//   tick2frame
//---------------------------------------------------------

unsigned TempoList::tick2frame(unsigned tick, unsigned frame, int* sn, LargeIntRoundMode round_mode) const
      {
      return (*sn == _tempoSN) ? frame : tick2frame(tick, sn, round_mode);
      }

//---------------------------------------------------------
//   tick2frame
//---------------------------------------------------------

unsigned TempoList::tick2frame(unsigned tick, int* sn, LargeIntRoundMode round_mode) const
      {
// REMOVE Tim. wave. Changed.
//       unsigned t = tick;
// #ifdef ALLOW_LEFT_HIDDEN_EVENTS
//       if ((int)t < 0)
//             t = 0;
// #endif
#ifdef ALLOW_LEFT_HIDDEN_EVENTS
      if ((int)tick < 0)
            tick = 0;
#endif
      unsigned f;
      const uint64_t numer = (uint64_t)MusEGlobal::sampleRate;
      const uint64_t denom = (uint64_t)MusEGlobal::config.division * (uint64_t)_globalTempo * 10000UL;
      if (useList) {
// REMOVE Tim. wave. Changed.
//             ciTEvent i = upper_bound(t);
            ciTEvent i = upper_bound(tick);
            if (i == end()) {
// REMOVE Tim. wave. Changed.
//                   printf("tick2frame(%d,0x%x): not found\n", t, t);
                  fprintf(stderr, "tick2frame(%d,0x%x): not found\n", tick, tick);
                  return 0;
                  }
            // Tick resolution is less than frame resolution. 
            // Round up so that the reciprocal function (frame to tick) matches value for value.
            f = i->second->frame + muse_multiply_64_div_64_to_64(
              numer * (uint64_t)i->second->tempo, tick - i->second->tick, denom, round_mode);
            }
      else {
            // Tick resolution is less than frame resolution. 
            // Round up so that the reciprocal function (frame to tick) matches value for value.
            f = muse_multiply_64_div_64_to_64(numer * (uint64_t)_tempo, tick, denom, round_mode);
            }
      if (sn)
            *sn = _tempoSN;
      return f;
      }

//---------------------------------------------------------
//   frame2tick
//    return cached value t if list did not change
//---------------------------------------------------------

unsigned TempoList::frame2tick(unsigned frame, unsigned t, int* sn, LargeIntRoundMode round_mode) const
      {
      return (*sn == _tempoSN) ? t : frame2tick(frame, sn, round_mode);
      }

//---------------------------------------------------------
//   frame2tick
//---------------------------------------------------------

unsigned TempoList::frame2tick(unsigned frame, int* sn, LargeIntRoundMode round_mode) const
      {
// REMOVE Tim. wave. Changed.
//       unsigned f = frame;
// #ifdef ALLOW_LEFT_HIDDEN_EVENTS
//       if ((int)f < 0)
//             f = 0;
// #endif
#ifdef ALLOW_LEFT_HIDDEN_EVENTS
      if ((int)frame < 0)
            frame = 0;
#endif
      unsigned tick;
      const uint64_t numer = (uint64_t)MusEGlobal::config.division * (uint64_t)_globalTempo * 10000UL;
      const uint64_t denom = (uint64_t)MusEGlobal::sampleRate;
      if (useList) {
            ciTEvent e;
            for (e = begin(); e != end();) {
                  ciTEvent ee = e;
                  ++ee;
                  if (ee == end())
                        break;
// REMOVE Tim. wave. Changed.
//                   if (f < ee->second->frame)
                  if (frame < ee->second->frame)
                        break;
                  e = ee;
                  }
            // Normally do not round up here since (audio) frame resolution is higher than tick resolution.
            tick = e->second->tick + muse_multiply_64_div_64_to_64(
              numer, frame - e->second->frame, denom * (uint64_t)e->second->tempo, round_mode);
            }
      else
            // Normally do not round up here since (audio) frame resolution is higher than tick resolution.
            tick = muse_multiply_64_div_64_to_64(numer, frame, denom * (uint64_t)_tempo, round_mode);
      if (sn)
            *sn = _tempoSN;
      return tick;
      }

//---------------------------------------------------------
//   deltaTick2frame
//---------------------------------------------------------

unsigned TempoList::deltaTick2frame(unsigned tick1, unsigned tick2, int* sn, LargeIntRoundMode round_mode) const
      {
// REMOVE Tim. wave. Changed.
//       unsigned t1 = tick1;
//       unsigned t2 = tick2;
// #ifdef ALLOW_LEFT_HIDDEN_EVENTS
//       if ((int)t1 < 0)
//             t1 = 0;
//       if ((int)t2 < 0)
//             t2 = 0;
// #endif
#ifdef ALLOW_LEFT_HIDDEN_EVENTS
      if ((int)tick1 < 0)
            tick1 = 0;
      if ((int)tick2 < 0)
            tick2 = 0;
#endif
      unsigned int f1, f2;
      const uint64_t numer = (uint64_t)MusEGlobal::sampleRate;
      const uint64_t denom = (uint64_t)MusEGlobal::config.division * (uint64_t)_globalTempo * 10000UL;
      if (useList) {
// REMOVE Tim. wave. Changed.
//             ciTEvent i = upper_bound(t1);
            ciTEvent i = upper_bound(tick1);
            if (i == end()) {
// REMOVE Tim. wave. Changed.
//                   printf("TempoList::deltaTick2frame: t1:%d not found\n", t1);
                  fprintf(stderr, "TempoList::deltaTick2frame: tick1:%d not found\n", tick1);
                  // abort();
                  return 0;
                  }
            // Tick resolution is less than frame resolution. 
            // Round up so that the reciprocal function (frame to tick) matches value for value.
            f1 = i->second->frame + muse_multiply_64_div_64_to_64(
              numer * (uint64_t)i->second->tempo, tick1 - i->second->tick, denom, round_mode);

// REMOVE Tim. wave. Changed.
//             i = upper_bound(t2);
            i = upper_bound(tick2);
            if (i == end()) {
                  fprintf(stderr, "TempoList::deltaTick2frame: tick2:%d not found\n", tick2);
                  return 0;
                  }
            // Tick resolution is less than frame resolution. 
            // Round up so that the reciprocal function (frame to tick) matches value for value.
            f2 = i->second->frame + muse_multiply_64_div_64_to_64(
              numer * (uint64_t)i->second->tempo, tick2 - i->second->tick, denom, round_mode);
            }
      else {
            // Tick resolution is less than frame resolution. 
            // Round up so that the reciprocal function (frame to tick) matches value for value.
            f1 = muse_multiply_64_div_64_to_64(numer * (uint64_t)_tempo, tick1, denom, round_mode);
            // Tick resolution is less than frame resolution. 
            // Round up so that the reciprocal function (frame to tick) matches value for value.
            f2 = muse_multiply_64_div_64_to_64(numer * (uint64_t)_tempo, tick2, denom, round_mode);
            }
      if (sn)
            *sn = _tempoSN;
      // FIXME: Caution: This should be rounded off properly somehow, but how to do that? 
      //                 But it seems to work so far.
      return f2 - f1;
      }


//---------------------------------------------------------
//   deltaFrame2tick
//---------------------------------------------------------

unsigned TempoList::deltaFrame2tick(unsigned frame1, unsigned frame2, int* sn, LargeIntRoundMode round_mode) const
      {
// REMOVE Tim. wave. Changed.
//       unsigned f1 = frame1;
//       unsigned f2 = frame2;
// #ifdef ALLOW_LEFT_HIDDEN_EVENTS
//       if ((int)f1 < 0)
//             f1 = 0;
//       if ((int)f2 < 0)
//             f2 = 0;
// #endif
#ifdef ALLOW_LEFT_HIDDEN_EVENTS
      if ((int)frame1 < 0)
            frame1 = 0;
      if ((int)frame2 < 0)
            frame2 = 0;
#endif
      unsigned tick1, tick2;
      const uint64_t numer = (uint64_t)MusEGlobal::config.division * (uint64_t)_globalTempo * 10000UL;
      const uint64_t denom = (uint64_t)MusEGlobal::sampleRate;
      if (useList) {
            ciTEvent e;
            for (e = begin(); e != end();) {
                  ciTEvent ee = e;
                  ++ee;
                  if (ee == end())
                        break;
// REMOVE Tim. wave. Changed.
//                   if (f1 < ee->second->frame)
                  if (frame1 < ee->second->frame)
                        break;
                  e = ee;
                  }
            // Normally do not round up here since (audio) frame resolution is higher than tick resolution.
            tick1 = e->second->tick + muse_multiply_64_div_64_to_64(
              numer, frame1 - e->second->frame, denom * (uint64_t)e->second->tempo, round_mode);
            
            for (e = begin(); e != end();) {
                  ciTEvent ee = e;
                  ++ee;
                  if (ee == end())
                        break;
// REMOVE Tim. wave. Changed.
//                   if (f2 < ee->second->frame)
                  if (frame2 < ee->second->frame)
                        break;
                  e = ee;
                  }
            // Normally do not round up here since (audio) frame resolution is higher than tick resolution.
            tick2 = e->second->tick + muse_multiply_64_div_64_to_64(
              numer, frame2 - e->second->frame, denom * (uint64_t)e->second->tempo, round_mode);
            }
      else
      {
            // Normally do not round up here since (audio) frame resolution is higher than tick resolution.
            tick1 = muse_multiply_64_div_64_to_64(numer, frame1, denom * (uint64_t)_tempo, round_mode);
            tick2 = muse_multiply_64_div_64_to_64(numer, frame2, denom * (uint64_t)_tempo, round_mode);
      }
      if (sn)
            *sn = _tempoSN;
      // FIXME: Caution: This should be rounded off properly somehow, but how to do that? 
      //                 But it seems to work so far.
      return tick2 - tick1;
      }
    
//---------------------------------------------------------
//   TempoList::write
//---------------------------------------------------------

void TempoList::write(int level, Xml& xml) const
      {
      xml.put(level++, "<tempolist fix=\"%d\">", _tempo);
      if (_globalTempo != 100)
            xml.intTag(level, "globalTempo", _globalTempo);
      for (ciTEvent i = begin(); i != end(); ++i)
            i->second->write(level, xml, i->first);
      xml.etag(--level, "tempolist");
      }

//---------------------------------------------------------
//   TempoList::read
//---------------------------------------------------------

void TempoList::read(Xml& xml)
      {
      for (;;) {
            Xml::Token token = xml.parse();
            const QString& tag = xml.s1();
            switch (token) {
                  case Xml::Error:
                  case Xml::End:
                        return;
                  case Xml::TagStart:
                        if (tag == "tempo") {
                              TEvent* t = new TEvent();
                              unsigned tick = t->read(xml);
                              iTEvent pos = find(tick);
                              if (pos != end())
                                    erase(pos);
                              insert(std::pair<const int, TEvent*> (tick, t));
                              }
                        else if (tag == "globalTempo")
                              _globalTempo = xml.parseInt();
                        else
                              xml.unknown("TempoList");
                        break;
                  case Xml::Attribut:
                        if (tag == "fix")
                              _tempo = xml.s2().toInt();
                        break;
                  case Xml::TagEnd:
                        if (tag == "tempolist") {
                              normalize();
                              return;
                              }
                  default:
                        break;
                  }
            }
      }

//---------------------------------------------------------
//   TEvent::write
//---------------------------------------------------------

void TEvent::write(int level, Xml& xml, int at) const
      {
      xml.tag(level++, "tempo at=\"%d\"", at);
      xml.intTag(level, "tick", tick);
      xml.intTag(level, "val", tempo);
      xml.etag(--level, "tempo");
      }

//---------------------------------------------------------
//   TEvent::read
//---------------------------------------------------------

int TEvent::read(Xml& xml)
      {
      int at = 0;
      for (;;) {
            Xml::Token token = xml.parse();
            const QString& tag = xml.s1();
            switch (token) {
                  case Xml::Error:
                  case Xml::End:
                        return 0;
                  case Xml::TagStart:
                        if (tag == "tick")
                              tick = xml.parseInt();
                        else if (tag == "val")
                              tempo = xml.parseInt();
                        else
                              xml.unknown("TEvent");
                        break;
                  case Xml::Attribut:
                        if (tag == "at")
                              at = xml.s2().toInt();
                        break;
                  case Xml::TagEnd:
                        if (tag == "tempo") {
                              return at;
                              }
                  default:
                        break;
                  }
            }
      return 0;
      }

//---------------------------------------------------------
//   put
//    return true on fifo overflow
//---------------------------------------------------------

bool TempoFifo::put(const TempoRecEvent& event)
      {
      if (size < TEMPO_FIFO_SIZE) {
            fifo[wIndex] = event;
            wIndex = (wIndex + 1) % TEMPO_FIFO_SIZE;
            // q_atomic_increment(&size);
            ++size;
            return false;
            }
      return true;
      }

//---------------------------------------------------------
//   get
//---------------------------------------------------------

TempoRecEvent TempoFifo::get()
      {
      TempoRecEvent event(fifo[rIndex]);
      rIndex = (rIndex + 1) % TEMPO_FIFO_SIZE;
      --size;
      return event;
      }

//---------------------------------------------------------
//   peek
//---------------------------------------------------------

const TempoRecEvent& TempoFifo::peek(int n)
      {
      int idx = (rIndex + n) % TEMPO_FIFO_SIZE;
      return fifo[idx];
      }

//---------------------------------------------------------
//   remove
//---------------------------------------------------------

void TempoFifo::remove()
      {
      rIndex = (rIndex + 1) % TEMPO_FIFO_SIZE;
      --size;
      }
      
} // namespace MusECore

