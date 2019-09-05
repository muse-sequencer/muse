//=========================================================
//  MusE
//  Linux Music Editor
//
//  mixed_pos_list.h
//  (C) Copyright 2019 Tim E. Real (terminator356 on users dot sourceforge dot net)
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
#ifndef MIXED_POS_LIST_H
#define MIXED_POS_LIST_H

#include <map>
#include <set>
#include <initializer_list>

#include "pos.h"

namespace MusECore {

//---------------------------------------------------------
//   MixedPosList_t
//     A C++11 multimap template that can store any type of value
//      as long as it has two methods: tick() and frame().
//     BOTH ticks and frame based values are allowed.
//     For example, both midi events and wave events can be stored.
//     They can be locked/unlocked by switching their time type
//      from/to ticks to/from frames.
//---------------------------------------------------------

template<class key, class T> class MixedPosList_t : 
  public std::multimap<key, T, std::less<key> >
{
  private:
    typedef std::multimap<key, T, std::less<key>> vlist;

  protected:
    Pos::TType _type;

  public:
    typedef typename vlist::iterator iMixedPosList;
    typedef typename vlist::const_iterator ciMixedPosList;
    typedef std::pair <ciMixedPosList, ciMixedPosList> cMixedPosListRange;
    typedef std::pair <key, T> cMixedPosListInsert;

    MixedPosList_t(Pos::TType type = Pos::TICKS) : vlist(), _type(type) {}
    virtual ~MixedPosList_t() {}

    inline Pos::TType type() const { return _type; }

    iMixedPosList insert (cMixedPosListInsert v)
    {
      const unsigned v_frame = v.second.frame();
      const unsigned v_tick = v.second.tick();
      ciMixedPosList pos = vlist::end();
      cMixedPosListRange r;

      // If list type is ticks, compare frame. If list type is frames, compare tick.
      if(type() == Pos::TICKS)
      {
        r = vlist::equal_range(v_tick);
        for(pos = r.first; pos != r.second; ++pos)
          if(v_frame <= pos->second.frame())
            break;
        return vlist::insert(pos, cMixedPosListInsert(v_tick, v.second));
      }
      else //if(_type == Pos::FRAMES)
      {
        r = vlist::equal_range(v_frame);
        for(pos = r.first; pos != r.second; ++pos)
          if(v_tick <= pos->second.tick())
            break;
        return vlist::insert(pos, cMixedPosListInsert(v_frame, v.second));
      }
      // return vlist::end();
    }

// TODO
//     template <class P> iMixedPosList insert (P&& v)  { return vlist::insert(v); }
//     iMixedPosList insert (ciMixedPosList pos, const T& v) { return vlist::insert(pos, v); }
//     template <class P> iMixedPosList insert (ciMixedPosList pos, P&& v) { return vlist::insert(pos, v); }
//     template <class InputIterator>
//     void insert (InputIterator first, InputIterator last) { return vlist::insert(first, last); }
//     void insert (std::initializer_list<T> il) { return vlist::insert(il); }

    // Returns an iterator that points to the inserted event.
//     // Returns end() if an error occurred.
    iMixedPosList add(T v)
    {
      const unsigned v_frame = v.frame();
      const unsigned v_tick = v.tick();
      ciMixedPosList pos = vlist::end();
      cMixedPosListRange r;

      // If list type is ticks, compare frame. If list type is frames, compare tick.
      if(type() == Pos::TICKS)
      {
        r = vlist::equal_range(v_tick);
        for(pos = r.first; pos != r.second; ++pos)
          if(v_frame <= pos->second.frame())
            break;
        return vlist::insert(pos, cMixedPosListInsert(v_tick, v));
      }
      else //if(_type == Pos::FRAMES)
      {
        r = vlist::equal_range(v_frame);
        for(pos = r.first; pos != r.second; ++pos)
          if(v_tick <= pos->second.tick())
            break;
        return vlist::insert(pos, cMixedPosListInsert(v_frame, v));
      }
      // return vlist::end();
    }
};


} // namespace MusECore

#endif

