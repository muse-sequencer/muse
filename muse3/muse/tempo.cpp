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
#include <errno.h>
#include <cmath>

#include "tempo.h"
#include "globals.h"
#include "gconfig.h"
#include "xml.h"

#include <stdint.h>
#include "large_int.h"

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
//   add
//---------------------------------------------------------

void TempoList::add(unsigned tick, int tempo, bool do_normalize)
      {
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
    ++_tempoSN;
}
      
//---------------------------------------------------------
//   tempo
//---------------------------------------------------------

int TempoList::tempo(unsigned tick) const
      {
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

//---------------------------------------------------------
//   tempo
//   Bypass the useList flag and read from the list
//---------------------------------------------------------

int TempoList::tempoAt(unsigned tick) const
      {
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
      iTEvent e = find(tick);
      if (e == end()) {
            printf("TempoList::del(%d): not found\n", tick);
            return;
            }
      del(e, do_normalize);
      ++_tempoSN;
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
      ++_tempoSN;
      }

//---------------------------------------------------------
//   setTempo
//    called from transport window
//    & slave mode tempo changes
//---------------------------------------------------------

void TempoList::setTempo(unsigned tick, int newTempo)
      {
      if (useList)
            add(tick, newTempo);
      else
            _tempo = newTempo;
      ++_tempoSN;
      }

//---------------------------------------------------------
//   setGlobalTempo
//---------------------------------------------------------

void TempoList::setGlobalTempo(int val)
      {
      _globalTempo = val;
      ++_tempoSN;
      normalize();
      }

//---------------------------------------------------------
//   addTempo
//---------------------------------------------------------

void TempoList::addTempo(unsigned t, int tempo, bool do_normalize)
      {
      add(t, tempo, do_normalize);
      ++_tempoSN;
      }

//---------------------------------------------------------
//   delTempo
//---------------------------------------------------------

void TempoList::delTempo(unsigned tick, bool do_normalize)
      {
      del(tick, do_normalize);
      ++_tempoSN;
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

unsigned TempoList::ticks2frames(unsigned ticks, unsigned tempoTick) const
{
  // Tick resolution is less than frame resolution. 
  // Round up so that the reciprocal function (frame to tick) matches value for value.
  return muse_multiply_64_div_64_to_64(
    (uint64_t)MusEGlobal::sampleRate * (uint64_t)tempo(tempoTick), ticks,
    (uint64_t)MusEGlobal::config.division * (uint64_t)_globalTempo * 10000UL, LargeIntRoundUp);
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

unsigned TempoList::tick2frame(unsigned tick, unsigned frame, int* sn) const
      {
      return (*sn == _tempoSN) ? frame : tick2frame(tick, sn);
      }

//---------------------------------------------------------
//   tick2frame
//---------------------------------------------------------

unsigned TempoList::tick2frame(unsigned tick, int* sn) const
      {
      unsigned f;
      const uint64_t numer = (uint64_t)MusEGlobal::sampleRate;
      const uint64_t denom = (uint64_t)MusEGlobal::config.division * (uint64_t)_globalTempo * 10000UL;
      if (useList) {
            ciTEvent i = upper_bound(tick);
            if (i == end()) {
                  printf("tick2frame(%d,0x%x): not found\n", tick, tick);
                  return 0;
                  }
            // Tick resolution is less than frame resolution. 
            // Round up so that the reciprocal function (frame to tick) matches value for value.
            f = i->second->frame + muse_multiply_64_div_64_to_64(
              numer * (uint64_t)i->second->tempo, tick - i->second->tick, denom, LargeIntRoundUp);
            }
      else {
            // Tick resolution is less than frame resolution. 
            // Round up so that the reciprocal function (frame to tick) matches value for value.
            f = muse_multiply_64_div_64_to_64(numer * (uint64_t)_tempo, tick, denom, LargeIntRoundUp);
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

unsigned TempoList::deltaTick2frame(unsigned tick1, unsigned tick2, int* sn) const
      {
      unsigned int f1, f2;
      const uint64_t numer = (uint64_t)MusEGlobal::sampleRate;
      const uint64_t denom = (uint64_t)MusEGlobal::config.division * (uint64_t)_globalTempo * 10000UL;
      if (useList) {
            ciTEvent i = upper_bound(tick1);
            if (i == end()) {
                  printf("TempoList::deltaTick2frame: tick1:%d not found\n", tick1);
                  // abort();
                  return 0;
                  }
            // Tick resolution is less than frame resolution. 
            // Round up so that the reciprocal function (frame to tick) matches value for value.
            f1 = i->second->frame + muse_multiply_64_div_64_to_64(
              numer * (uint64_t)i->second->tempo, tick1 - i->second->tick, denom, LargeIntRoundUp);

            i = upper_bound(tick2);
            if (i == end()) {
                  return 0;
                  }
            // Tick resolution is less than frame resolution. 
            // Round up so that the reciprocal function (frame to tick) matches value for value.
            f2 = i->second->frame + muse_multiply_64_div_64_to_64(
              numer * (uint64_t)i->second->tempo, tick2 - i->second->tick, denom, LargeIntRoundUp);
            }
      else {
            // Tick resolution is less than frame resolution. 
            // Round up so that the reciprocal function (frame to tick) matches value for value.
            f1 = muse_multiply_64_div_64_to_64(numer * (uint64_t)_tempo, tick1, denom, LargeIntRoundUp);
            // Tick resolution is less than frame resolution. 
            // Round up so that the reciprocal function (frame to tick) matches value for value.
            f2 = muse_multiply_64_div_64_to_64(numer * (uint64_t)_tempo, tick2, denom, LargeIntRoundUp);
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
      xml.tag(level, "/tempolist");
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
                              ++_tempoSN;
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
      xml.tag(level, "/tempo");
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

