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
#include <list>
// #include <initializer_list>

#include "pos.h"

namespace MusECore {

//---------------------------------------------------------
//   MixedPosList_t
//     A C++11 multimap template that can store any type of value
//      as long as it has three methods: type(), tick(), and frame().
//     BOTH ticks and frame based values are allowed.
//     For example, both midi events and wave events can be stored.
//     They can be locked/unlocked by switching their time type
//      from/to ticks to/from frames.
//---------------------------------------------------------

template<class Key, class T, class Compare = std::less<Key>,
         class Alloc = std::allocator<std::pair<const Key,T> > >
  class MixedPosList_t : public std::multimap<Key, T, Compare, Alloc >
{
  private:
    typedef std::multimap<Key, T, Compare, Alloc> vlist;
    typedef typename std::list<T>::const_iterator cil_t;

  protected:
    Pos::TType _type;

  public:
    typedef typename vlist::iterator iMixedPosList_t;
    typedef typename vlist::const_iterator ciMixedPosList_t;
    typedef std::pair <ciMixedPosList_t, ciMixedPosList_t> cMixedPosListRange_t;
    typedef std::pair <Key, T> MixedPosListInsertPair_t;

    MixedPosList_t(Pos::TType type = Pos::TICKS) : vlist(), _type(type) {}
    virtual ~MixedPosList_t() {}

    inline Pos::TType type() const { return _type; }
    inline void setType(Pos::TType t) { _type = t; rebuild(); }

    // Note that the pair's key part is IGNORED.
    // All information is gathered from the pair's value type T.
    iMixedPosList_t insert (MixedPosListInsertPair_t v)
    {
      return add(v.second);
    }

    // TODO:
    // template <class P> iMixedPosList insert (P&& v)  { return vlist::insert(v); }
    // iMixedPosList insert (ciMixedPosList pos, const T& v) { return vlist::insert(pos, v); }
    // template <class P> iMixedPosList insert (ciMixedPosList pos, P&& v) { return vlist::insert(pos, v); }
    // template <class InputIterator>
    // void insert (InputIterator first, InputIterator last) { return vlist::insert(first, last); }
    // void insert (std::initializer_list<T> il) { return vlist::insert(il); }

    // Returns an iterator that points to the inserted event.
    iMixedPosList_t add(T v)
    {
      const unsigned v_frame = v.frame();
      const unsigned v_tick = v.tick();
      ciMixedPosList_t pos = vlist::end();
      cMixedPosListRange_t r;

      // If list type is ticks, compare frame. If list type is frames, compare tick.
      switch(type())
      {
        case Pos::TICKS:
          r = vlist::equal_range(v_tick);
          for(pos = r.first; pos != r.second; ++pos)
            if(v_frame <= pos->second.frame())
              break;
          return vlist::insert(pos, MixedPosListInsertPair_t(v_tick, v));
        break;

        case Pos::FRAMES:
          r = vlist::equal_range(v_frame);
          for(pos = r.first; pos != r.second; ++pos)
            if(v_tick <= pos->second.tick())
              break;
          return vlist::insert(pos, MixedPosListInsertPair_t(v_frame, v));
        break;
      }
      //return vlist::end();
    }
    
    // After any tempo changes, it is essential to rebuild the list
    //  so that any 'locked' items are re-sorted properly by tick.
    // Returns true if any items were rebuilt.
    bool rebuild()
    {
      std::list<T> to_be_added;
      for(ciMixedPosList_t i = vlist::begin(); i != vlist::end(); )
      {
        const T& m = i->second;
        if((type() == Pos::TICKS && m.type() == Pos::FRAMES) ||
           (type() == Pos::FRAMES && m.type() == Pos::TICKS))
        {
          to_be_added.push_back(m);
          i = erase(i);
        }
        else
          ++i;
      }

      for(cil_t ai = to_be_added.begin(); ai != to_be_added.end(); ++ai)
        add(*ai);

      return !to_be_added.empty();
    }

};


} // namespace MusECore

#endif

