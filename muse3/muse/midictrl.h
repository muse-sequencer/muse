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

//#define _MIDI_CTRL_DEBUG_
// For finding exactly who may be calling insert, erase clear etc. in
//  the controller list classes. (KDevelop 'Find uses'.)
//#define _MIDI_CTRL_METHODS_DEBUG_

namespace MusECore {

class Xml;
class Part;
class MidiRecordEvent;

const int CTRL_HBANK = 0x00;
const int CTRL_LBANK = 0x20;

const int CTRL_HDATA = 0x06;
const int CTRL_LDATA = 0x26;

const int CTRL_DATA_INC = 0x60;
const int CTRL_DATA_DEC = 0x61;

const int CTRL_HNRPN = 0x63;
const int CTRL_LNRPN = 0x62;

const int CTRL_HRPN  = 0x65;
const int CTRL_LRPN  = 0x64;

const int CTRL_MODULATION         = 0x01;
const int CTRL_PORTAMENTO_TIME    = 0x05;
const int CTRL_VOLUME             = 0x07;
const int CTRL_PANPOT             = 0x0a;
const int CTRL_EXPRESSION         = 0x0b;
const int CTRL_SUSTAIN            = 0x40;
const int CTRL_PORTAMENTO         = 0x41;
const int CTRL_SOSTENUTO          = 0x42;
const int CTRL_SOFT_PEDAL         = 0x43;
const int CTRL_HARMONIC_CONTENT   = 0x47;
const int CTRL_RELEASE_TIME       = 0x48;
const int CTRL_ATTACK_TIME        = 0x49;

const int CTRL_BRIGHTNESS         = 0x4a;
const int CTRL_PORTAMENTO_CONTROL = 0x54;
const int CTRL_REVERB_SEND        = 0x5b;
const int CTRL_CHORUS_SEND        = 0x5d;
const int CTRL_VARIATION_SEND     = 0x5e;

const int CTRL_ALL_SOUNDS_OFF     = 0x78; // 120
const int CTRL_RESET_ALL_CTRL     = 0x79; // 121
const int CTRL_LOCAL_OFF          = 0x7a; // 122

// controller types 0x10000 - 0x1ffff are 14 bit controller with
//    0x1xxyy
//      xx - MSB controller
//      yy - LSB controller

// RPN  - registered parameter numbers 0x20000 -
// NRPN - non registered parameter numbers 0x30000 -

// internal controller types:
const int CTRL_INTERNAL_OFFSET = 0x40000;

const int CTRL_PITCH    = CTRL_INTERNAL_OFFSET;
const int CTRL_PROGRAM  = CTRL_INTERNAL_OFFSET      + 0x01;
const int CTRL_VELOCITY = CTRL_INTERNAL_OFFSET      + 0x02;
const int CTRL_MASTER_VOLUME = CTRL_INTERNAL_OFFSET + 0x03;
const int CTRL_AFTERTOUCH = CTRL_INTERNAL_OFFSET    + 0x04;
// NOTE: The range from CTRL_INTERNAL_OFFSET + 0x100 to CTRL_INTERNAL_OFFSET + 0x1ff is reserved 
//        for this control. (The low byte is reserved because this is a per-note control.) 
const int CTRL_POLYAFTER = CTRL_INTERNAL_OFFSET     + 0x1FF;  // 100 to 1FF !

const int CTRL_VAL_UNKNOWN   = 0x10000000; // used as unknown hwVal
const int CTRL_PROGRAM_VAL_DONT_CARE = 0xffffff; // High-bank, low-bank, and program are all 0xff don't care.

const int CTRL_7_OFFSET      = 0x00000;
const int CTRL_14_OFFSET     = 0x10000;
const int CTRL_RPN_OFFSET    = 0x20000;
const int CTRL_NRPN_OFFSET   = 0x30000;
const int CTRL_RPN14_OFFSET  = 0x50000;
const int CTRL_NRPN14_OFFSET = 0x60000;
const int CTRL_NONE_OFFSET   = 0x70000;

const int CTRL_OFFSET_MASK   = 0xf0000;

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
      static int dValToInt(double v);
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

typedef std::multimap<int, MidiCtrlVal, std::less<int> >::iterator iMidiCtrlVal;
typedef std::multimap<int, MidiCtrlVal, std::less<int> >::const_iterator ciMidiCtrlVal;

typedef std::pair <iMidiCtrlVal, iMidiCtrlVal> MidiCtrlValRange;
class MidiCtrlValList : public std::multimap<int, MidiCtrlVal, std::less<int> > {
      
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
      iMidiCtrlVal find(const int&) { return end(); };
      ciMidiCtrlVal find(const int&) const { return end(); };

   public:
      MidiCtrlValList(int num);
      
      Part* partAtTick(int tick) const;
      
      // Determine value at tick, using values stored by ANY part.
      iMidiCtrlVal iValue(int tick);
      // Determine value at tick, using values stored by ANY part.
      int value(int tick) const;
      // Determine value at tick, using values stored by the SPECIFIC part.
      int value(int tick, Part* part) const;
      // Determine value at tick, using values stored by ANY part,
      //  ignoring values that are OUTSIDE of their parts, or muted or off parts or tracks.
      int visibleValue(unsigned int tick, bool inclMutedParts, bool inclMutedTracks, bool inclOffTracks) const;
      // Determine value at tick, using values stored by the SPECIFIC part,
      //  ignoring values that are OUTSIDE of the part, or muted or off part or track.
      int visibleValue(unsigned int tick, Part* part, bool inclMutedParts, bool inclMutedTracks, bool inclOffTracks) const;
      bool addMCtlVal(int tick, int value, Part* part);
      void delMCtlVal(int tick, Part* part);
      
      iMidiCtrlVal findMCtlVal(int tick, Part* part);

      // Current set value in midi hardware. Can be CTRL_VAL_UNKNOWN.
      inline int hwVal() const { return MidiController::dValToInt(_hwVal); }

      double hwDVal() const { return _hwVal; }
      inline bool hwValIsUnknown() const { return MidiController::iValIsUnknown(MidiController::dValToInt(_hwVal)); }

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
//    - excplicit by user
//    - implicit during import of a midi file
//---------------------------------------------------------

typedef std::map<int, MidiController*, std::less<int> >::iterator iMidiController;
typedef std::map<int, MidiController*, std::less<int> >::const_iterator ciMidiController;

class MidiControllerList : public std::map<int, MidiController*, std::less<int> > 
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
      iMidiController searchControllers(int ctl);
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
      void del(iMidiController ictl, bool update = true);
      size_type del(int num, bool update = true);
      void del(iMidiController first, iMidiController last, bool update = true);
      void clr();

#ifdef _MIDI_CTRL_METHODS_DEBUG_      
      // Need to catch all insert, erase, clear etc...
      void swap(MidiControllerList&);
      std::pair<iMidiController, bool> insert(const std::pair<int, MidiController*>& p);
      iMidiController insert(iMidiController ic, const std::pair<int, MidiController*>& p);
      void erase(iMidiController ictl);
      size_type erase(int num);
      void erase(iMidiController first, iMidiController last);
      void clear();
#endif       
      // Some IDEs won't "Find uses" of operators. So, no choice but to trust always catching it here.
      MidiControllerList& operator=(const MidiControllerList&);
};

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

