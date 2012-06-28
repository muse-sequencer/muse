//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: tempo.h,v 1.2.2.1 2006/09/19 19:07:09 spamatica Exp $
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

#ifndef __TEMPO_H__
#define __TEMPO_H__

#include <map>
#include <vector>

#ifndef MAX_TICK
#define MAX_TICK (0x7fffffff/100)
#endif

// Tempo ring buffer size
#define TEMPO_FIFO_SIZE    1024


namespace MusECore {

class Xml;

//---------------------------------------------------------
//   Tempo Event
//---------------------------------------------------------

struct TEvent {
      int tempo;
      unsigned tick;    // new tempo at tick
      unsigned frame;   // precomputed time for tick in sec

      int read(Xml&);
      void write(int, Xml&, int) const;

      TEvent() { }
      TEvent(unsigned t, unsigned tk) {
            tempo = t;
            tick  = tk;
            frame = 0;
            }
      };

//---------------------------------------------------------
//   TempoList
//---------------------------------------------------------

typedef std::map<unsigned, TEvent*, std::less<unsigned> > TEMPOLIST;
typedef TEMPOLIST::iterator iTEvent;
typedef TEMPOLIST::const_iterator ciTEvent;
typedef TEMPOLIST::reverse_iterator riTEvent;
typedef TEMPOLIST::const_reverse_iterator criTEvent;

class TempoList : public TEMPOLIST {
      int _tempoSN;           // serial no to track tempo changes
      bool useList;
      int _tempo;             // tempo if not using tempo list
      int _globalTempo;       // %percent 50-200%

      void add(unsigned tick, int tempo, bool do_normalize = true);
      void change(unsigned tick, int newTempo);
      void del(iTEvent);
      void del(unsigned tick);

   public:
      TempoList();
      ~TempoList();
      void normalize();
      void clear();
      void eraseRange(unsigned stick, unsigned etick);

      void read(Xml&);
      void write(int, Xml&) const;
      void dump() const;

      int tempo(unsigned tick) const;
      int tempoAt(unsigned tick) const;
      unsigned tick2frame(unsigned tick, unsigned frame, int* sn) const;
      unsigned tick2frame(unsigned tick, int* sn = 0) const;
      unsigned frame2tick(unsigned frame, int* sn = 0) const;
      unsigned frame2tick(unsigned frame, unsigned tick, int* sn) const;
      unsigned deltaTick2frame(unsigned tick1, unsigned tick2, int* sn = 0) const;
      unsigned deltaFrame2tick(unsigned frame1, unsigned frame2, int* sn = 0) const;
      
      int tempoSN() const { return _tempoSN; }
      void setTempo(unsigned tick, int newTempo);
      void addTempo(unsigned t, int tempo, bool do_normalize = true);
      void delTempo(unsigned tick);
      void changeTempo(unsigned tick, int newTempo);
      bool masterFlag() const { return useList; }
      bool setMasterFlag(unsigned tick, bool val);
      int globalTempo() const           { return _globalTempo; }
      void setGlobalTempo(int val);
      };

//---------------------------------------------------------
//   Tempo Record Event
//---------------------------------------------------------

struct TempoRecEvent {
      int tempo;
      unsigned tick;    
      TempoRecEvent() { }
      TempoRecEvent(unsigned tk, unsigned t) {
            tick  = tk;
            tempo = t;
            }
      };

class TempoRecList : public std::vector<TempoRecEvent >
{
  public:
    void addTempo(int tick, int tempo)    { push_back(TempoRecEvent(tick, tempo)); }
    void addTempo(const TempoRecEvent& e) { push_back(e); }
};

//---------------------------------------------------------
//   TempoFifo
//---------------------------------------------------------

class TempoFifo {
      TempoRecEvent fifo[TEMPO_FIFO_SIZE];
      volatile int size;
      int wIndex;
      int rIndex;

   public:
      TempoFifo()  { clear(); }
      bool put(const TempoRecEvent& event);   // returns true on fifo overflow
      TempoRecEvent get();
      const TempoRecEvent& peek(int = 0);
      void remove();
      bool isEmpty() const { return size == 0; }
      void clear()         { size = 0, wIndex = 0, rIndex = 0; }
      int getSize() const  { return size; }
      };
      
} // namespace MusECore

namespace MusEGlobal {
extern MusECore::TempoList tempomap;
extern MusECore::TempoRecList tempo_rec_list;
}

#endif
