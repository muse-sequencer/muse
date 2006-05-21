//=============================================================================
//  MusE
//  Linux Music Editor
//  $Id:$
//
//  Copyright (C) 2002-2006 by Werner Schweer and others
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License version 2.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//=============================================================================

#ifndef __MIDICTRL_H__
#define __MIDICTRL_H__

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
const int CTRL_PITCH         = 0x40000;
const int CTRL_PROGRAM       = 0x40001;
const int CTRL_VELOCITY      = 0x40002;
const int CTRL_MASTER_VOLUME = 0x40003;
const int CTRL_OTHER         = 0x40004;
const int CTRL_SVELOCITY     = 0x40005;	// single velocity, used for drum editor
							// to show only velocity for current instrument
const int CTRL_NO_CTRL       = 0x40006;

const int CTRL_VAL_UNKNOWN   = -1;        // used as unknown hwVal

const int CTRL_RPN_OFFSET    = 0x20000;
const int CTRL_NRPN_OFFSET   = 0x30000;
const int CTRL_RPN14_OFFSET  = 0x50000;
const int CTRL_NRPN14_OFFSET = 0x60000;

namespace AL {
      class Xml;
      };
using AL::Xml;

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

   public:
      MidiController();
      MidiController(const QString& n, int num, int min, int max, int init);
      const QString& name() const         { return _name;   }
      int num() const                     { return _num;    }
      void setName(const QString& s)      { _name = s;      }
      void setNum(int v)                  { _num = v;       }
      void write(Xml& xml) const;
      void read(QDomNode);
      int minVal() const                  { return _minVal; }
      int maxVal() const                  { return _maxVal; }
      int initVal() const                 { return _initVal; }
      void setInitVal(int val)            { _initVal = val; }
      void setMinVal(int val)             { _minVal = val;  }
      void setMaxVal(int val)             { _maxVal = val;  }
      };

//---------------------------------------------------------
//   MidiControllerList
//    this is a list of used midi controllers created
//    - excplicit by user
//    - implicit during import of a midi file
//---------------------------------------------------------

class MidiControllerList : public std::list<MidiController*> {
   public:
      MidiControllerList() {}
      };

typedef MidiControllerList::iterator iMidiController;
typedef MidiControllerList::const_iterator ciMidiController;
typedef MidiControllerList MidiControllerList;

extern MidiControllerList defaultMidiController;
extern void initMidiController();
extern MidiController::ControllerType midiControllerType(int num);

extern void configMidiController();
extern const QString& int2ctrlType(int n);
extern MidiController::ControllerType ctrlType2Int(const QString& s);
extern QString midiCtrlName(int ctrl);
extern MidiController veloCtrl;

#endif

