//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: midictrl.h,v 1.16.2.8 2009/11/25 09:09:43 terminator356 Exp $
//
//  (C) Copyright 1999-2003 Werner Schweer (ws@seh.de)
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

namespace MusECore {

class Xml;
class Part;

const int CTRL_HBANK = 0x00;
const int CTRL_LBANK = 0x20;

const int CTRL_HDATA = 0x06;
const int CTRL_LDATA = 0x26;

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
const int CTRL_PROGRAM  = CTRL_INTERNAL_OFFSET      + 1;
const int CTRL_VELOCITY = CTRL_INTERNAL_OFFSET      + 2;
const int CTRL_MASTER_VOLUME = CTRL_INTERNAL_OFFSET + 3;

const int CTRL_VAL_UNKNOWN   = 0x10000000; // used as unknown hwVal

const int CTRL_14_OFFSET     = 0x10000;
const int CTRL_RPN_OFFSET    = 0x20000;
const int CTRL_NRPN_OFFSET   = 0x30000;
const int CTRL_RPN14_OFFSET  = 0x50000;
const int CTRL_NRPN14_OFFSET = 0x60000;
const int CTRL_NONE_OFFSET   = 0x70000;

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
            Velo              // not assigned
            };
   private:
      QString _name;
      int _num;               // Controller Number
      int _minVal;            // controller value range (used in gui)
      int _maxVal;
      int _initVal;
      int _bias;
      void updateBias();

   public:
      MidiController();
      MidiController(const QString& n, int num, int min, int max, int init);
      MidiController(const MidiController& mc);
      void copy(const MidiController &mc);
      MidiController& operator= (const MidiController &mc);

      const QString& name() const         { return _name;   }
      int num() const                     { return _num;    }
      void setName(const QString& s)      { _name = s;      }
      void setNum(int v)                  { _num = v;       }
      void write(int level, Xml& xml) const;
      void read(Xml& xml);
      int minVal() const                  { return _minVal; }
      int maxVal() const                  { return _maxVal; }
      int initVal() const                 { return _initVal; }
      void setInitVal(int val)            { _initVal = val; }
      void setMinVal(int val)             { _minVal = val; updateBias(); }
      void setMaxVal(int val)             { _maxVal = val; updateBias(); }
      int bias() const                    { return _bias; }
      static int genNum(ControllerType, int, int);
      };

// Added by T356.
struct MidiCtrlVal
{
  // The part containing the event which this value came from. Used for searching and deleting.
  Part* part;
  // The stored value.
  int val;
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
      
      int ctrlNum;
      int _lastValidHWVal;
      int _hwVal;       // current set value in midi hardware
                        // can be CTRL_VAL_UNKNOWN
      
      // Hide built-in finds.
      iMidiCtrlVal find(const int&) { return end(); };
      ciMidiCtrlVal find(const int&) const { return end(); };

   public:
      MidiCtrlValList(int num);
      
      Part* partAtTick(int tick) const;
      
      iMidiCtrlVal iValue(int tick);
      int value(int tick) const;
      int value(int tick, Part* part) const;
      bool addMCtlVal(int tick, int value, Part* part);
      void delMCtlVal(int tick, Part* part);
      
      iMidiCtrlVal findMCtlVal(int tick, Part* part);
      
      int hwVal() const       { return _hwVal;   }
      bool setHwVal(const int v);
      bool setHwVals(const int v, const int lastv);
      int num() const         { return ctrlNum;  }
      int lastValidHWVal() const          { return _lastValidHWVal; }
      };

//---------------------------------------------------------
//   MidiCtrlValListList
//    List of midi controller value lists.
//    This list represents the controller state of a
//    midi port.
//          index = (channelNumber << 24) + ctrlNumber
//---------------------------------------------------------

typedef std::map<int, MidiCtrlValList*, std::less<int> >::iterator iMidiCtrlValList;
typedef std::map<int, MidiCtrlValList*, std::less<int> >::const_iterator ciMidiCtrlValList;

class MidiCtrlValListList : public std::map<int, MidiCtrlValList*, std::less<int> > {
   public:
      void add(int channel, MidiCtrlValList* vl) {
            insert(std::pair<const int, MidiCtrlValList*>((channel << 24) + vl->num(), vl));
            }
      iMidiCtrlValList find(int channel, int ctrl) {
            return std::map<int, MidiCtrlValList*, std::less<int> >::find((channel << 24) + ctrl);
            }
      void clearDelete(bool deleteLists);      
      };

//---------------------------------------------------------
//   MidiControllerList
//    this is a list of used midi controllers created
//    - excplicit by user
//    - implicit during import of a midi file
//---------------------------------------------------------

class MidiControllerList : public std::map<int, MidiController*, std::less<int> > 
{
   public:
      MidiControllerList() {}
      MidiControllerList(const MidiControllerList& mcl);
      
      void add(MidiController* mc) { insert(std::pair<int, MidiController*>(mc->num(), mc)); }
};

typedef MidiControllerList::iterator iMidiController;
typedef MidiControllerList::const_iterator ciMidiController;
typedef MidiControllerList MidiControllerList;

extern MidiControllerList defaultMidiController;
extern void initMidiController();
extern MidiController::ControllerType midiControllerType(int num);
extern int midiCtrlTerms2Number(int type_num, int ctrl);


extern const QString& int2ctrlType(int n);
extern MidiController::ControllerType ctrlType2Int(const QString& s);
extern QString midiCtrlName(int ctrl, bool fullyQualified = false);
extern QString midiCtrlNumString(int ctrl, bool fullyQualified = false);
extern MidiController veloCtrl;


typedef std::map<int, int, std::less<int> > MidiCtl2LadspaPortMap;
typedef MidiCtl2LadspaPortMap::iterator iMidiCtl2LadspaPort;
typedef MidiCtl2LadspaPortMap::const_iterator ciMidiCtl2LadspaPort;

} // namespace MusECore

#endif

