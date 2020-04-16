//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: midictrl.h,v 1.16.2.8 2009/11/25 09:09:43 terminator356 Exp $
//
//  (C) Copyright 1999-2003 Werner Schweer (ws@seh.de)
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

#ifndef __MIDI_CONTROLLER_H__
#define __MIDI_CONTROLLER_H__

#include <map>

#include <QString>

#include "midictrl_consts.h"
#include "xml.h"

//#define _MIDI_CTRL_DEBUG_
// For finding exactly who may be calling insert, erase clear etc. in
//  the controller list classes. (KDevelop 'Find uses'.)
//#define _MIDI_CTRL_METHODS_DEBUG_

namespace MusECore {

//---------------------------------------------------------
//   MidiController
//---------------------------------------------------------

class MidiController {
   public:
      //
      // mapping of midi controller types to
      // controller number:
      //
      enum ControllerType {
            Controller7,      // num values from 0 - 0x7f
            Controller14,     // values from 0x10000 - 0x12fff
            RPN,              // registered parameter 0x20000 -
            NRPN,             // non registered parameter 0x30000 -
            RPN14,            // registered parameter 0x50000
            NRPN14,           // non registered parameter 0x60000 -
            Pitch,            // num value = CTRL_PITCH
            Program,          // num value = CTRL_PROGRAM
            PolyAftertouch,   // num value = CTRL_POLYAFTER
            Aftertouch,       // num value = CTRL_AFTERTOUCH
            Velo              // not assigned
            };
            
      enum ShowInTrackType { ShowInDrum=1, ShowInMidi=2 };
      
   protected:
      QString _name;
      int _num;               // Controller Number
      int _minVal;            // controller value range (used in gui)
      int _maxVal;
      int _initVal;
      // Special for drum mode, for controllers such as program.
      int _drumInitVal;
      int _bias;
      int _showInTracks;
      void updateBias();

   public:
      MidiController();
      // If drumInit = -1, it means don't care - use the init val.
      MidiController(const QString& n, int num, int min, int max, int init, int drumInit, int show_in_track = (ShowInDrum | ShowInMidi));
      MidiController(const MidiController& mc);
      void copy(const MidiController &mc);
      MidiController& operator= (const MidiController &mc);

      // Convert given controller double value to integer.
      static inline int dValToInt(double v) {
        // TODO: Decide best choice here.
        //return int(round(v));
        //return lrint(v);
        return int(v);
      }

      // Whether the given integer value is CTRL_VAL_UNKNOWN.
      static inline bool iValIsUnknown(int v) { return v == CTRL_VAL_UNKNOWN; }
      // Whether the given double value is CTRL_VAL_UNKNOWN.
      static inline bool dValIsUnknown(double v) { return iValIsUnknown(dValToInt(v)); }

      const QString& name() const         { return _name;   }
      int num() const                     { return _num;    }
      ControllerType type() const;
      void setName(const QString& s)      { _name = s;      }
      void setNum(int v)                  { _num = v;       }
      void write(int level, Xml& xml) const;
      void read(Xml& xml);
      int minVal() const                  { return _minVal; }
      int maxVal() const                  { return _maxVal; }
      int initVal() const                 { return _initVal; }
      inline bool initValIsUnknown() const { return iValIsUnknown(_initVal); }
      void setInitVal(int val)            { _initVal = val; }
      int drumInitVal() const             { return _drumInitVal; }
      inline bool drumInitValIsUnknown() const { return iValIsUnknown(_drumInitVal); }
      void setDrumInitVal(int val)        { _drumInitVal = val; }
      void setMinVal(int val)             { _minVal = val; updateBias(); }
      void setMaxVal(int val)             { _maxVal = val; updateBias(); }
      int bias() const                    { return _bias; }
      int showInTracks() const            { return _showInTracks; }
      void setShowInTracks(int i)         { _showInTracks = i; }
      bool isPerNoteController() const;
      static int genNum(ControllerType, int, int);
      };


//---------------------------------------------------------
//   MidiControllerList
//    this is a list of used midi controllers created
//    - explicit by user
//    - implicit during import of a midi file
//---------------------------------------------------------

typedef std::map<int, MidiController*, std::less<int> > MidiControllerList_t;

class MidiControllerList : public MidiControllerList_t
{
      bool _RPN_Ctrls_Reserved; 
      
   public:
      MidiControllerList();
      MidiControllerList(const MidiControllerList& mcl);
      // NOTE: There is no destructor here, this container does not
      //        delete its own contents, that is done in ~MidiInstrument
      //        because some controls are shared (the ones in
      //        defaultMidiController and defaultManagedMidiController).
      //       However, an inheritor such as the class MidiNamCtrls
      //        found in the MidNam module DOES delete its own.

      // Like find() which finds a verbose ctl number, but this version also finds a per-note
      //  controller if there is one for the given ctl number, if no verbose one was found.
      // Returns null if no controller found. The ctl number can be the 'real' controller number,
      //  ie the low byte can be the actual note number and does not have to be 0xff.
      MidiController* findController(int ctl) const;
      // Returns per-note controller if there is one for the given ctl number.
      // Otherwise returns null. The ctl number can be the 'real' controller number,
      //  ie the low byte can be the actual note number and does not have to be 0xff.
      MidiController* perNoteController(int ctl) const;

      // Like 'find', finds a controller given fully qualified type + number. 
      // But it returns controller with highest priority if multiple controllers use the 
      //  given number such as {Controller7, num = 0x55} + {Controller14, num = 0x5544}.
      // Note if given number is one of the eight reserved General Midi (N)RPN controllers,
      //  this will only return Controller7 or Controller14, not anything (N)RPN related.
      // That is, it will not 'encode' (N)RPNs. Use a MidiEncoder instance for that. 
      iterator searchControllers(int ctl);
      // Check if either a per-note controller, or else a regular controller already exists.
      bool ctrlAvailable(int find_num, MidiController* ignore_this = 0);
      // Returns true if any of the EIGHT reserved General Midi (N)RPN control numbers are  
      //  ALREADY defined as Controller7 or part of Controller14. Cached, for speed.
      // Used (at least) by midi input encoders to quickly arbitrate new input.
      bool RPN_Ctrls_Reserved() { return _RPN_Ctrls_Reserved; }
      // Manual check and update of the flag. For convenience, returns the flag.
      bool update_RPN_Ctrls_Reserved();
      
      // NOTICE: If update is false or these are bypassed by using insert, erase, clear etc. for speed, 
      //          then BE SURE to call update_RPN_Ctrls_Reserved() later. 
      // Returns true if add is successful.
      bool add(MidiController* mc, bool update = true);
      void del(iterator ictl, bool update = true);
      size_type del(int num, bool update = true);
      void del(iterator first, iterator last, bool update = true);
      void clr();

#ifdef _MIDI_CTRL_METHODS_DEBUG_      
      // Need to catch all insert, erase, clear etc...
      void swap(MidiControllerList&);
      std::pair<iterator, bool> insert(const std::pair<int, MidiController*>& p);
      iterator insert(iterator ic, const std::pair<int, MidiController*>& p);
      void erase(iterator ictl);
      size_type erase(int num);
      void erase(iterator first, iterator last);
      void clear();
#endif       
      // Some IDEs won't "Find uses" of operators. So, no choice but to trust always catching it here.
      MidiControllerList& operator=(const MidiControllerList&);
};

typedef MidiControllerList::iterator iMidiController;
typedef MidiControllerList::const_iterator ciMidiController;
typedef std::pair<int /* number */, MidiController*> MidiControllerListPair;

extern MidiController::ControllerType midiControllerType(int num);
extern int midiCtrlTerms2Number(MidiController::ControllerType type, int ctrl = 0);
extern bool isPerNoteMidiController(int num);


extern const QString& int2ctrlType(int n);
extern MidiController::ControllerType ctrlType2Int(const QString& s);
extern QString midiCtrlName(int ctrl, bool fullyQualified = false);
extern QString midiCtrlNumString(int ctrl, bool fullyQualified = false);

typedef std::map<int, int, std::less<int> > MidiCtl2LadspaPortMap;
typedef MidiCtl2LadspaPortMap::iterator iMidiCtl2LadspaPort;
typedef MidiCtl2LadspaPortMap::const_iterator ciMidiCtl2LadspaPort;


// // REMOVE Tim. midnam. Added.
// // template<class Key, class T, class Compare = std::less<Key>,
// //          class Alloc = std::allocator<std::pair<const Key,T> > >
// //   class CompoundMidiControllerList_t : public MidiControllerList_t
//   class CompoundMidiControllerList_t : public MidiControllerList
// {
//   private:
// //     typedef std::multimap<Key, T, Compare, Alloc> vlist;
// //     typedef MidiControllerList_t vlist;
//     typedef MidiControllerList vlist;
//     //typedef typename std::list<T>::const_iterator cil_t;
//     
//     //MidiControllerList** _p_other;
//     //MidNamMIDIName* _midnamDocument;
// 
//   protected:
//     //Pos::TType _type;
//     MidiControllerList** _p_other;
// 
//   public:
//     typedef typename vlist::iterator iCompoundMidiControllerList_t;
//     typedef typename vlist::const_iterator ciCompoundMidiControllerList_t;
// //     typedef std::pair <ciMixedPosList_t, ciMixedPosList_t> cMixedPosListRange_t;
// //     typedef std::pair <Key, T> MixedPosListInsertPair_t;
//     typedef std::pair <int, MidiController*> CompoundMidiControllerListInsertPair_t;
// 
// 
//     class iterator : public vlist::iterator {
//         protected:
//           const MidiControllerList* _p_list;
//           MidiControllerList** _p_other;
//           bool _isFromOther;
// 
//         public:
//           iterator() : vlist::iterator(), _p_list(nullptr), _p_other(nullptr), _isFromOther(false) {}
//           iterator(vlist::iterator i) : vlist::iterator(i), _p_list(nullptr), _p_other(nullptr), _isFromOther(false) {}
// 
//           void setLists(const MidiControllerList* this_list, MidiControllerList** other, bool isFromOther)
//             { _p_list = this_list; _p_other = other; _isFromOther = isFromOther; }
// 
// //             MidiController* operator*() {
// //                   //return (MidiController*)(**((vlist::iterator*)this));
// //                   return (MidiController*)(**((vlist::iterator*)this->second));
// //                   }
//           iterator operator++(int) {
//                 return iterator ((*(vlist::iterator*)this).operator++(0));
//                 }
//           iterator& operator++() {
//                 vlist::iterator i_this = *this;
//                 i_this++;
//                 if(_p_other && !(*_p_other)->empty())
//                 {
//                   
//                 }
//                 
//                 (iterator&) ((*(vlist::iterator*)this).operator++())
// 
// 
//                 return (iterator&) ((*(vlist::iterator*)this).operator++());
//                 }
//           };
// 
//     class const_iterator : public vlist::const_iterator {
//         protected:
//           const MidiControllerList* _p_list;
//           MidiControllerList** _p_other;
//           bool _isFromOther;
// 
//         public:
//           const_iterator() : vlist::const_iterator(), _p_list(nullptr), _p_other(nullptr), _isFromOther(false) {}
//           const_iterator(vlist::const_iterator i)
//             : vlist::const_iterator(i), _p_list(nullptr), _p_other(nullptr), _isFromOther(false) {}
//           const_iterator(vlist::iterator i)
//             : vlist::const_iterator(i), _p_list(nullptr), _p_other(nullptr), _isFromOther(false) {}
// 
//           void setLists(const MidiControllerList* this_list, MidiControllerList** other, bool isFromOther)
//             { _p_list = this_list; _p_other = other; _isFromOther = isFromOther; }
// 
// //             const T operator*() const {
// //                   return (T)(**((vlist::const_iterator*)this));
// //                   }
//           std::pair <int, MidiController*> operator*() const {
//                 return **((vlist::const_iterator*)this);
//                 }
// //             const MidiController* operator->() const {
// //                   return (**((vlist::const_iterator*)this)->second);
// //                   }
//           };
// 
//     class reverse_iterator : public vlist::reverse_iterator {
//         protected:
//           const MidiControllerList* _p_list;
//           MidiControllerList** _p_other;
//           bool _isFromOther;
// 
//         public:
//           reverse_iterator() : vlist::reverse_iterator(), _p_list(nullptr), _p_other(nullptr), _isFromOther(false) {}
//           reverse_iterator(vlist::reverse_iterator i)
//             : vlist::reverse_iterator(i), _p_list(nullptr), _p_other(nullptr), _isFromOther(false) {}
// 
//           void setLists(const MidiControllerList* this_list, MidiControllerList** other, bool isFromOther)
//             { _p_list = this_list; _p_other = other; _isFromOther = isFromOther; }
// 
// //             T operator*() {
// //                   return (T)(**((vlist::reverse_iterator*)this));
// //                   }
//           std::pair <int, MidiController*> operator*() {
//                 return **((vlist::reverse_iterator*)this);
//                 }
//           };
// 
//     class const_reverse_iterator : public vlist::const_reverse_iterator {
//         protected:
//           const MidiControllerList* _p_list;
//           MidiControllerList** _p_other;
//           bool _isFromOther;
// 
//         public:
//           const_reverse_iterator() : vlist::const_reverse_iterator(), _p_list(nullptr), _p_other(nullptr), _isFromOther(false) {}
//           const_reverse_iterator(vlist::const_reverse_iterator i) 
//             : vlist::const_reverse_iterator(i), _p_list(nullptr), _p_other(nullptr), _isFromOther(false) {}
// 
//           void setLists(const MidiControllerList* this_list, MidiControllerList** other, bool isFromOther)
//             { _p_list = this_list; _p_other = other; _isFromOther = isFromOther; }
// 
// //             T operator*() {
// //                   return (T)(**((vlist::const_reverse_iterator*)this));
// //                   }
//           std::pair <int, MidiController*> operator*() {
//                 return **((vlist::const_reverse_iterator*)this);
//                 }
//           };
// 
//     
//     
//     
//     //CompoundMidiControllerList_t(Pos::TType type = Pos::TICKS) : vlist(), _type(type) {}
//     CompoundMidiControllerList_t(MidiControllerList** other = nullptr)
//       : vlist(), _p_other(other) { }
// 
//     virtual ~CompoundMidiControllerList_t() {}
// 
//     //inline Pos::TType type() const { return _type; }
//     //inline void setType(const Pos::TType& t) { _type = t; rebuild(); }
// 
//     iterator begin() noexcept
//     {
//       iterator i_this = vlist::begin();
//       i_this.setLists(this, _p_other, false);
//       if(_p_other && !(*_p_other)->empty())
//       {
//         iterator i_other = (*_p_other)->begin();
//         i_other.setLists(this, _p_other, true);
//         if(vlist::empty() || i_other->first < i_this->first)
//           return i_other;
//       }
//       return i_this;
//     }
// 
//     const_iterator begin() const noexcept
//     {
//       const_iterator i_this = vlist::begin();
//       i_this.setLists(this, _p_other, false);
//       if(_p_other && !(*_p_other)->empty())
//       {
//         const MidiControllerList* mcl_other = *_p_other;
//         const_iterator i_other = mcl_other->begin();
//         i_other.setLists(this, _p_other, true);
//         if(vlist::empty() || i_other->first < i_this->first)
//           return i_other;
//       }
//       return i_this;
//     }
//     
//     const_iterator cbegin() const noexcept
//     {
//       const_iterator i_this = vlist::cbegin();
//       i_this.setLists(this, _p_other, false);
//       if(_p_other && !(*_p_other)->empty())
//       {
//         const_iterator i_other = (*_p_other)->cbegin();
//         i_other.setLists(this, _p_other, true);
//         if(vlist::empty() || i_other->first < i_this->first)
//           return i_other;
//       }
//       return i_this;
//     }
//     
//     reverse_iterator rbegin() noexcept
//     {
//       reverse_iterator i_this = vlist::rbegin();
//       i_this.setLists(this, _p_other, false);
//       if(_p_other && !(*_p_other)->empty())
//       {
//         reverse_iterator i_other = (*_p_other)->rbegin();
//         i_other.setLists(this, _p_other, true);
//         if(vlist::empty() || i_other->first > i_this->first)
//           return i_other;
//       }
//       return i_this;
//     }
// 
//     const_reverse_iterator rbegin() const noexcept
//     {
//       const_reverse_iterator i_this = vlist::rbegin();
//       i_this.setLists(this, _p_other, false);
//       if(_p_other && !(*_p_other)->empty())
//       {
//         const MidiControllerList* mcl_other = *_p_other;
//         const_reverse_iterator i_other = mcl_other->rbegin();
//         i_other.setLists(this, _p_other, true);
//         if(vlist::empty() || i_other->first > i_this->first)
//           return i_other;
//       }
//       return i_this;
//     }
// 
//     const_reverse_iterator crbegin() const noexcept
//     {
//       const_reverse_iterator i_this = vlist::crbegin();
//       i_this.setLists(this, _p_other, false);
//       if(_p_other && !(*_p_other)->empty())
//       {
//         const_reverse_iterator i_other = (*_p_other)->crbegin();
//         i_other.setLists(this, _p_other, true);
//         if(vlist::empty() || i_other->first > i_this->first)
//           return i_other;
//       }
//       return i_this;
//     }
// 
// 
//     // -------------------------------------------------------------------
//     // With these end iterators, we have a choice of either container.
//     // We could ask which container's end is more relevant by comparing
//     //  the decremented end iterators, or we could simply just take the
//     //  first container's. Which ever way we choose, it MUST be
//     //  consistent with the way our operator++/-- work.
//     // -------------------------------------------------------------------
// 
//     iterator end() noexcept
//     {
//       iterator i_this = vlist::end();
//       i_this.setLists(this, _p_other, false);
// //       if(_p_other && !(*_p_other)->empty())
// //       {
// //         iterator i_other = (*_p_other)->end();
// //         i_other.setLists(this, _p_other, true);
// //         if(vlist::empty() || i_other->first > i_this->first)
// //           return i_other;
// //       }
//       return i_this;
//     }
// 
//     const_iterator end() const noexcept
//     {
//       const_iterator i_this = vlist::end();
//       i_this.setLists(this, _p_other, false);
// //       if(_p_other && !(*_p_other)->empty())
// //       {
// //         const_iterator i_other = (*_p_other)->end();
// //         i_other.setLists(this, _p_other, true);
// //         if(vlist::empty() || i_other->first > i_this->first)
// //           return i_other;
// //       }
//       return i_this;
//     }
//     
//     const_iterator cend() const noexcept
//     {
//       const_iterator i_this = vlist::cend();
//       i_this.setLists(this, _p_other, false);
// //       if(_p_other && !(*_p_other)->empty())
// //       {
// //         const_iterator i_other = (*_p_other)->cend();
// //         i_other.setLists(this, _p_other, true);
// //         if(vlist::empty() || i_other->first > i_this->first)
// //           return i_other;
// //       }
//       return i_this;
//     }
//     
//     reverse_iterator rend() noexcept
//     {
//       reverse_iterator i_this = vlist::rend();
//       i_this.setLists(this, _p_other, false);
// //       if(_p_other && !(*_p_other)->empty())
// //       {
// //         reverse_iterator i_other = (*_p_other)->rend();
// //         i_other.setOtherList(_p_other, true);
// //         if(vlist::empty() || i_other->first < i_this->first)
// //           return i_other;
// //       }
//       return i_this;
//     }
// 
//     const_reverse_iterator crend() const noexcept
//     {
//       const_reverse_iterator i_this = vlist::crend();
//       i_this.setLists(this, _p_other, false);
// //       if(_p_other && !(*_p_other)->empty())
// //       {
// //         const_reverse_iterator i_other = (*_p_other)->crend();
// //         i_other.setLists(this, _p_other, true);
// //         if(vlist::empty() || i_other->first < i_this->first)
// //           return i_other;
// //       }
//       return i_this;
//     }
// 
//     bool empty() const noexcept
//     {
//       return vlist::empty() && (!_p_other || (*_p_other)->empty());
//     }
// 
//     size_type size() const noexcept
//     {
//       return vlist::size() + (_p_other ? (*_p_other)->size() : 0);
//     }
// 
//     iterator find(const int& key)
//     {
//       iterator i_this = vlist::find(key);
//       i_this.setLists(this, _p_other, false);
//       if(i_this == vlist::end() && _p_other)
//       {
//         iterator i_other = (*_p_other)->find(key);
//         i_other.setLists(this, _p_other, true);
//         if(i_other != (*_p_other)->end())
//           return i_other;
//       }
//       return i_this;
//     }
// 
//     const_iterator find(const int& key) const
//     {
//       const_iterator i_this = vlist::find(key);
//       i_this.setLists(this, _p_other, false);
//       if(i_this == vlist::end() && _p_other)
//       {
//         const MidiControllerList* mcl_other = *_p_other;
//         const_iterator i_other = mcl_other->find(key);
//         i_other.setLists(this, _p_other, true);
//         if(i_other != mcl_other->end())
//           return i_other;
//       }
//       return i_this;
//     }
// 
//     iterator lower_bound(const int& key)
//     {
//       iterator i_this = vlist::lower_bound(key);
//       i_this.setLists(this, _p_other, false);
//       if(i_this == vlist::end() && _p_other && !(*_p_other)->empty())
//       {
//         iterator i_other = (*_p_other)->lower_bound(key);
//         i_other.setLists(this, _p_other, true);
//         if(i_other != (*_p_other)->end() && (vlist::empty() || i_other->first < i_this->first))
//           return i_other;
//       }
//       return i_this;
//     }
//     
//     const_iterator lower_bound(const int& key) const
//     {
//       const_iterator i_this = vlist::lower_bound(key);
//       i_this.setLists(this, _p_other, false);
//       if(i_this == vlist::end() && _p_other && !(*_p_other)->empty())
//       {
//         const MidiControllerList* mcl_other = *_p_other;
//         const_iterator i_other = mcl_other->lower_bound(key);
//         i_other.setLists(this, _p_other, true);
//         if(i_other != mcl_other->end() && (vlist::empty() || i_other->first < i_this->first))
//           return i_other;
//       }
//       return i_this;
//     }
//     
//     iterator upper_bound(const int& key)
//     {
//       iterator i_this = vlist::upper_bound(key);
//       i_this.setLists(this, _p_other, false);
//       if(i_this == vlist::end() && _p_other && !(*_p_other)->empty())
//       {
//         iterator i_other = (*_p_other)->upper_bound(key);
//         i_other.setLists(this, _p_other, true);
//         if(i_other != (*_p_other)->end() && (vlist::empty() || i_other->first < i_this->first))
//           return i_other;
//       }
//       return i_this;
//     }
//     
//     const_iterator upper_bound(const int& key) const
//     {
//       const_iterator i_this = vlist::upper_bound(key);
//       i_this.setLists(this, _p_other, false);
//       if(i_this == vlist::end() && _p_other && !(*_p_other)->empty())
//       {
//         const MidiControllerList* mcl_other = *_p_other;
//         const_iterator i_other = mcl_other->upper_bound(key);
//         i_other.setLists(this, _p_other, true);
//         if(i_other != mcl_other->end() && (vlist::empty() || i_other->first < i_this->first))
//           return i_other;
//       }
//       return i_this;
//     }
// 
//     
//     
//     
// //     // Returns an iterator that points to the inserted event.
// //     // Note that the pair's key part is IGNORED.
// //     // All information is gathered from the pair's value type T.
// //     // Returns end() if an error occurred.
// //     iCompoundMidiControllerList_t insert (const CompoundMidiControllerListInsertPair_t& v)
// //     {
// //       return add(v.second);
// //     }
// // 
// //     // TODO:
// //     // template <class P> iMixedPosList insert (P&& v)  { return vlist::insert(v); }
// //     // iMixedPosList insert (ciMixedPosList pos, const T& v) { return vlist::insert(pos, v); }
// //     // template <class P> iMixedPosList insert (ciMixedPosList pos, P&& v) { return vlist::insert(pos, v); }
// //     // template <class InputIterator>
// //     // void insert (InputIterator first, InputIterator last) { return vlist::insert(first, last); }
// //     // void insert (std::initializer_list<T> il) { return vlist::insert(il); }
// // 
// //     // Returns an iterator that points to the added event.
// //     // Returns end() if an error occurred.
// //     iMixedPosList_t add(const T& v)
// //     {
// //       const unsigned v_frame = v.frame();
// //       const unsigned v_tick = v.tick();
// //       ciMixedPosList_t pos = vlist::end();
// //       cMixedPosListRange_t r;
// // 
// //       // If list type is ticks, compare frame. If list type is frames, compare tick.
// //       switch(type())
// //       {
// //         case Pos::TICKS:
// //           r = vlist::equal_range(v_tick);
// //           for(pos = r.first; pos != r.second; ++pos)
// //             if(v_frame < pos->second.frame())
// //               break;
// //           return vlist::insert(pos, MixedPosListInsertPair_t(v_tick, v));
// //         break;
// // 
// //         case Pos::FRAMES:
// //           r = vlist::equal_range(v_frame);
// //           for(pos = r.first; pos != r.second; ++pos)
// //             if(v_tick < pos->second.tick())
// //               break;
// //           return vlist::insert(pos, MixedPosListInsertPair_t(v_frame, v));
// //         break;
// //       }
// //       return vlist::end();
// //     }
//     
// 
// };

} // namespace MusECore

#endif

