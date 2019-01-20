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

#ifndef __MIDICTRL_H__
#define __MIDICTRL_H__

#include <list>
#include <map>

#include <QString>

#include "midictrl_consts.h"

//#define _MIDI_CTRL_DEBUG_
// For finding exactly who may be calling insert, erase clear etc. in
//  the controller list classes. (KDevelop 'Find uses'.)
//#define _MIDI_CTRL_METHODS_DEBUG_

namespace MusECore {

class Xml;
class Part;
class MidiRecordEvent;

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
      
   private:
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
      bool isPerNoteController() const    { return (_num & 0xff) == 0xff; }
      static int genNum(ControllerType, int, int);
      };

struct MidiCtrlVal
{
  // The part containing the event which this value came from. Used for searching and deleting.
  Part* part;
  // The stored value.
  int val;
  MidiCtrlVal(Part* p, int v) { part = p; val = v; }
  bool operator==(const MidiCtrlVal& mcv) { return part == mcv.part && val == mcv.val; }
};

//---------------------------------------------------------
//   MidiCtrlValList
//    arrange controller events of a specific type in a
//    list for easy retrieval
//---------------------------------------------------------

typedef std::pair<unsigned int, MidiCtrlVal> MidiCtrlValListInsertPair_t;
typedef std::multimap<unsigned int, MidiCtrlVal, std::less<unsigned int> > MidiCtrlValList_t;

class MidiCtrlValList : public MidiCtrlValList_t {
      
      // The controller number.
      int ctrlNum;
      // Current set value in midi hardware. Can be CTRL_VAL_UNKNOWN.
      double _hwVal;
      // The last value that was not CTRL_VAL_UNKNOWN. Can still be CTRL_VAL_UNKNOWN (typically at startup).
      // Note that in the case of PROGRAM for example, HBank/LBank bytes can still be 0xff (OFF).
      double _lastValidHWVal;
      // The last byte values that were not CTRL_VAL_UNKNOWN or 0xff (Off).
      // Can never be 0xff (OFF), but can still be CTRL_VAL_UNKNOWN (typically at startup).
      // Special for example PROGRAM controller, has 3 separate values: HBank, LBank and Program.
      int _lastValidByte2;
      int _lastValidByte1;
      int _lastValidByte0;

      // Hide built-in finds.
      iterator find(const unsigned int&) { return end(); };
      const_iterator find(const unsigned int&) const { return end(); };

   public:
      MidiCtrlValList(int num);
      
      Part* partAtTick(unsigned int tick) const;
      
      // Determine value at tick, using values stored by ANY part.
      iterator iValue(unsigned int tick);
      // Determine value at tick, using values stored by ANY part.
      int value(unsigned int tick) const;
      // Determine value at tick, using values stored by the SPECIFIC part.
      int value(unsigned int tick, Part* part) const;
      // Determine value at tick, using values stored by ANY part,
      //  ignoring values that are OUTSIDE of their parts, or muted or off parts or tracks.
      int visibleValue(unsigned int tick, bool inclMutedParts, bool inclMutedTracks, bool inclOffTracks) const;
      // Determine value at tick, using values stored by the SPECIFIC part,
      //  ignoring values that are OUTSIDE of the part, or muted or off part or track.
      int visibleValue(unsigned int tick, Part* part, bool inclMutedParts, bool inclMutedTracks, bool inclOffTracks) const;
      // Adds the new value. Accepts duplicate controller items at the same position, to accurately reflect
      //  what is really in the event lists. Mostly for the purpose of dragging and dropping
      //  controller events and allowing them to be on top of each other TEMPORARILY.
      // But ultimately once dropping is finished there must be only ONE value per controller
      //  per position per part.
      bool addMCtlVal(unsigned int tick, int value, Part* part);
      // If val is not -1 it will search for that value.
      void delMCtlVal(unsigned int tick, Part* part, int val/* = -1*/);
      
      // If val is not -1 it will search for that value.
      iterator findMCtlVal(unsigned int tick, Part* part, int val/* = -1*/);

      // Current set value in midi hardware. Can be CTRL_VAL_UNKNOWN.
      inline int hwVal() const { return MidiController::dValToInt(_hwVal); }

      double hwDVal() const { return _hwVal; }
      inline bool hwValIsUnknown() const { return MidiController::iValIsUnknown(MidiController::dValToInt(_hwVal)); }

      // Resets the current, and optionally the last, hardware value to CTRL_VAL_UNKNOWN.
      // Returns true if either value was changed.
      bool resetHwVal(bool doLastHwValue = false);
      
      // Set current value in midi hardware. Can be CTRL_VAL_UNKNOWN.
      // Returns false if value is already equal, true if value is changed.
      bool setHwVal(const double v);
      //   Sets current and last HW values.
      //   Handy for forcing labels to show 'off' and knobs to show specific values
      //    without having to send two messages.
      //   Returns false if both values are already set, true if either value is changed.
      bool setHwVals(const double v, const double lastv);
      // The controller number.
      int num() const { return ctrlNum; }
      // The last value that was not CTRL_VAL_UNKNOWN. Can still be CTRL_VAL_UNKNOWN (typically at startup).
      // Note that in the case of PROGRAM for example, HBank/LBank bytes can still be 0xff (OFF).
      inline int lastValidHWVal() const { return MidiController::dValToInt(_lastValidHWVal); }

      double lastValidHWDVal() const { return _lastValidHWVal; }
      inline bool lastHwValIsUnknown() const { return MidiController::iValIsUnknown(MidiController::dValToInt(_lastValidHWVal)); }

      // The last byte values that were not CTRL_VAL_UNKNOWN or 0xff (Off).
      // Can never be 0xff (OFF), but can still be CTRL_VAL_UNKNOWN (typically at startup).
      // Special for example PROGRAM controller, has 3 separate values: HBank, LBank and Program.
      int lastValidByte2() const          { return _lastValidByte2; }
      int lastValidByte1() const          { return _lastValidByte1; }
      int lastValidByte0() const          { return _lastValidByte0; }
      };

typedef MidiCtrlValList::iterator iMidiCtrlVal;
typedef MidiCtrlValList::const_iterator ciMidiCtrlVal;
typedef std::pair <iMidiCtrlVal, iMidiCtrlVal> MidiCtrlValRange;

//---------------------------------------------------------
//   MidiCtrlValListList
//    List of midi controller value lists.
//    This list represents the controller state of a
//    midi port.
//          index = (channelNumber << 24) + ctrlNumber
//---------------------------------------------------------

typedef std::map<int, MidiCtrlValList*, std::less<int> > MidiCtrlValListList_t;
typedef MidiCtrlValListList_t::iterator iMidiCtrlValList;
typedef MidiCtrlValListList_t::const_iterator ciMidiCtrlValList;

class MidiCtrlValListList : public MidiCtrlValListList_t {
      bool _RPN_Ctrls_Reserved; 
      
   public:
      MidiCtrlValListList();
      //MidiCtrlValListList(const MidiCtrlValListList&); // TODO
      
      iterator find(int channel, int ctrl) {
            return std::map<int, MidiCtrlValList*, std::less<int> >::find((channel << 24) + ctrl);
            }
      const_iterator find(int channel, int ctrl) const {
            return ((const MidiCtrlValListList_t*)this)->find((channel << 24) + ctrl);
            }
      void clearDelete(bool deleteLists);      
      // Like 'find', finds a controller given fully qualified type + number. 
      // But it returns controller with highest priority if multiple controllers use the 
      //  given number such as {Controller7, num = 0x55} + {Controller14, num = 0x5544}.
      // Note if given number is one of the eight reserved General Midi (N)RPN controllers,
      //  this will only return Controller7 or Controller14, not anything (N)RPN related.
      // That is, it will not 'encode' (N)RPNs. Use a MidiEncoder instance for that. 
      iMidiCtrlValList searchControllers(int channel, int ctl);
      // Returns true if any of the EIGHT reserved General Midi (N)RPN control numbers are ALREADY 
      //  defined as Controller7 or part of Controller14. Cached, for speed.
      // Used (at least) by midi input encoders to quickly arbitrate new input.
      bool RPN_Ctrls_Reserved() { return _RPN_Ctrls_Reserved; }
      // Manual check and update of the flag. For convenience, returns the flag.
      // Cost depends on types and number of list controllers, so it is good for deferring 
      //  an update until AFTER some lengthy list operation. JUST BE SURE to call this!
      bool update_RPN_Ctrls_Reserved();
      
      // NOTICE: If update is false or these are bypassed by using insert, erase, clear etc. for speed, 
      //          then BE SURE to call update_RPN_Ctrls_Reserved() later. 
      void add(int channel, MidiCtrlValList* vl, bool update = true);
      void del(iMidiCtrlValList ictl, bool update = true);
      size_type del(int num, bool update = true);
      void del(iMidiCtrlValList first, iMidiCtrlValList last, bool update = true);
      void clr();
      // Convenience method: Resets all current, and optionally the last, hardware controller values to CTRL_VAL_UNKNOWN.
      // Equivalent to calling resetAllHwVal() on each MidiCtrlValList.
      // Returns true if either value was changed in any controller.
      bool resetAllHwVals(bool doLastHwValue);
      
#ifdef _MIDI_CTRL_METHODS_DEBUG_      
      // Need to catch all insert, erase, clear etc...
      void swap(MidiCtrlValListList&);
      std::pair<iMidiCtrlValList, bool> insert(const std::pair<int, MidiCtrlValList*>& p);
      iMidiCtrlValList insert(iMidiCtrlValList ic, const std::pair<int, MidiCtrlValList*>& p);
      void erase(iMidiCtrlValList ictl);
      size_type erase(int num);
      void erase(iMidiCtrlValList first, iMidiCtrlValList last);
      void clear();
#endif       
      // Some IDEs won't "Find uses" of operators. So, no choice but to trust always catching it here.
      MidiCtrlValListList& operator=(const MidiCtrlValListList&);
      };
      
//---------------------------------------------------------
//   MidiEncoder
//---------------------------------------------------------

class MidiEncoder {
  public:
    enum Mode { EncIdle, EncCtrl14, EncDiscoverRPN, EncDiscoverNRPN, EncRPN, EncNRPN, EncRPN14, EncNRPN14 };
    enum ParamMode { ParamModeUnknown, ParamModeRPN, ParamModeNRPN };

  private:  
    Mode          _curMode;
    ParamMode     _curParamMode;
    unsigned int  _timer;        // 
    unsigned char _curCtrl;      // Ctl num of first event
    unsigned char _curData;      // Data of first event
    unsigned int  _curTime;      // Time of first event
    unsigned char _nextCtrl;     // Expected next event ctl num (for ctrl14 only)
    unsigned char _curRPNH;
    unsigned char _curRPNL;
    unsigned char _curNRPNH;
    unsigned char _curNRPNL;

  public:
    MidiEncoder();

    void encodeEvent(const MidiRecordEvent& ev, int port, int channel);
    void endCycle(unsigned int blockSize);
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
      void add(MidiController* mc, bool update = true);
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

extern MidiControllerList defaultMidiController;
extern void initMidiController();
extern MidiController::ControllerType midiControllerType(int num);
extern int midiCtrlTerms2Number(MidiController::ControllerType type, int ctrl = 0);


extern const QString& int2ctrlType(int n);
extern MidiController::ControllerType ctrlType2Int(const QString& s);
extern QString midiCtrlName(int ctrl, bool fullyQualified = false);
extern QString midiCtrlNumString(int ctrl, bool fullyQualified = false);
extern MidiController veloCtrl;
extern MidiController pitchCtrl;
extern MidiController programCtrl;
extern MidiController mastervolCtrl;
extern MidiController volumeCtrl;
extern MidiController panCtrl;
extern MidiController reverbSendCtrl;
extern MidiController chorusSendCtrl;
extern MidiController variationSendCtrl;

typedef std::map<int, int, std::less<int> > MidiCtl2LadspaPortMap;
typedef MidiCtl2LadspaPortMap::iterator iMidiCtl2LadspaPort;
typedef MidiCtl2LadspaPortMap::const_iterator ciMidiCtl2LadspaPort;

} // namespace MusECore

#endif

