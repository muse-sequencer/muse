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

#include "midictrl.h"
#include "al/xml.h"

static const char* ctrlName[] = {
       "BankSelMSB",  "Modulation",  "BreathCtrl",  "Control 3",  "Foot Ctrl",
       "Porta Time",  "DataEntMSB",  "MainVolume",  "Balance",    "Control 9",
/*10*/ "Pan",         "Expression",  "Control 12",  "Control 13", "Control 14",
       "Control 15",  "Gen.Purp.1",  "Gen.Purp.2",  "Gen.Purp.3", "Gen.Purp.4",
/*20*/ "Control 20",  "Control 21",  "Control 22",  "Control 23", "Control 24",
       "Control 25",  "Control 26",  "Control 27",  "Control 28", "Control 29",
/*30*/ "Control 30",  "Control 31",  "BankSelLSB",  "Modul. LSB", "BrthCt.LSB",
       "Control 35", "FootCt.LSB",   "Port.T LSB",  "DataEntLSB", "MainVolLSB",
       "BalanceLSB",  "Control 41",  "Pan LSB",     "Expr. LSB",  "Control 44",
       "Control 45",  "Control 46",  "Control 47",  "Gen.P.1LSB", "Gen.P.2LSB",
/*50*/ "Gen.P.3LSB",  "Gen.P.4LSB", "Control 52",   "Control 53",  "Control 54",
       "Control 55", "Control 56",  "Control 57",   "Control 58",  "Control 59",
       "Control 60",  "Control 61",  "Control 62",  "Control 63", "Sustain",
       "Porta Ped",   "Sostenuto",  "Soft Pedal", "Control 68",  "Hold 2",
       "Control 70",  "HarmonicCo", "ReleaseTime", "Attack Time", "Brightness",
       "Control 75", "Control 76",  "Control 77",  "Control 78",  "Control 79",
       "Gen.Purp.5",  "Gen.Purp.6",  "Gen.Purp.7", "Gen.Purp.8", "Porta Ctrl",
       "Control 85",  "Control 86",  "Control 87", "Control 88",  "Control 89",
       "Control 90",  "Effect1Dep", "Effect2Dep",  "Effect3Dep",  "Effect4Dep",
       "Phaser Dep", "Data Incr",   "Data Decr",   "NRPN LSB",    "NRPN MSB",
/*100*/ "RPN LSB",     "RPN MSB",     "Control102", "Control103", "Control104",
       "Control105",  "Control106",  "Control107", "Control108",  "Control109",
       "Control110",  "Control111", "Control112",  "Control113",  "Control114",
       "Control115", "Control116",  "Control117",  "Control118",  "Control119",
       "AllSndOff",   "Reset Ctrl",  "Local Ctrl", "AllNoteOff", "OmniModOff",
       "OmniModeOn",  "MonoModeOn",  "PolyModeOn"
      };

#if 0
static const char* ctrl14Name[] = {
     "BankSel",    "Modulation",  "BreathCtrl",
     "Control 3",  "Foot Ctrl",   "Porta Time",  "DataEntry",
     "MainVolume", "Balance",     "Control 9",   "Pan",
     "Expression", "Control 12",  "Control 13",  "Control 14",
     "Control 15", "Gen.Purp.1",  "Gen.Purp.2",  "Gen.Purp.3",
     "Gen.Purp.4", "Control 20",  "Control 21",  "Control 22",
     "Control 23", "Control 24",  "Control 25",  "Control 26",
     "Control 27", "Control 28",  "Control 29",  "Control 30",
     "Control 31",
     };
#endif

enum {
      COL_NAME = 0, COL_TYPE,
      COL_HNUM, COL_LNUM, COL_MIN, COL_MAX
      };

MidiControllerList defaultMidiController;
//
// some global controller which are always available:
//
MidiController veloCtrl("Velocity",                 CTRL_VELOCITY,      0, 127,   0);
static MidiController pitchCtrl("PitchBend",        CTRL_PITCH,     -8192, +8191, 0);
static MidiController programCtrl("Program",        CTRL_PROGRAM,       0, 0xffffff, 0);
static MidiController mastervolCtrl("MasterVolume", CTRL_MASTER_VOLUME, 0, 0x3fff, 0x3000);
static MidiController volumeCtrl("MainVolume",      CTRL_VOLUME,        0, 127, 100);
static MidiController panCtrl("Pan",                CTRL_PANPOT,        0, 127,  64);
static MidiController reverbCtrl("ReverbSend",      CTRL_REVERB_SEND,   0, 127,  0);
static MidiController chorusCtrl("ChorusSend",      CTRL_CHORUS_SEND,   0, 127,  0);
static MidiController variationCtrl("VariationSend",CTRL_VARIATION_SEND,0, 127,  0);

//---------------------------------------------------------
//   ctrlType2Int
//   int2ctrlType
//---------------------------------------------------------

static struct {
      MidiController::ControllerType type;
      QString name;
      }  ctrlTypes[] = {
      { MidiController::Controller7, QString("Control7") },
      { MidiController::Controller14, QString("Control14") },
      { MidiController::RPN, QString("RPN") },
      { MidiController::NRPN, QString("NRPN") },
      { MidiController::Pitch, QString("Pitch") },
      { MidiController::Program, QString("Program") },
      { MidiController::RPN14, QString("RPN14") },
      { MidiController::NRPN14, QString("NRPN14") },
      { MidiController::Controller7, QString("Control") },    // alias
      };

//---------------------------------------------------------
//   ctrlType2Int
//---------------------------------------------------------

MidiController::ControllerType ctrlType2Int(const QString& s)
      {
      int n = sizeof(ctrlTypes)/sizeof(*ctrlTypes);
      for (int i = 0; i < n; ++i) {
            if (ctrlTypes[i].name == s)
                  return ctrlTypes[i].type;
            }
      return MidiController::ControllerType(0);
      }

//---------------------------------------------------------
//   int2ctrlType
//---------------------------------------------------------

const QString& int2ctrlType(int n)
      {
      static QString dontKnow("?T?");
      int size = sizeof(ctrlTypes)/sizeof(*ctrlTypes);
      for (int i = 0; i < size; ++i) {
            if (ctrlTypes[i].type == n)
                  return ctrlTypes[i].name;
            }
      return dontKnow;
      }

//---------------------------------------------------------
//   initMidiController
//---------------------------------------------------------

void initMidiController()
      {
      static bool initialized = false;
      if (initialized)
            return;
      defaultMidiController.push_back(&veloCtrl);
      defaultMidiController.push_back(&pitchCtrl);
      defaultMidiController.push_back(&programCtrl);
      defaultMidiController.push_back(&mastervolCtrl);
      defaultMidiController.push_back(&volumeCtrl);
      defaultMidiController.push_back(&panCtrl);
      defaultMidiController.push_back(&reverbCtrl);
      defaultMidiController.push_back(&chorusCtrl);
      defaultMidiController.push_back(&variationCtrl);
      initialized = true;
      }

//---------------------------------------------------------
//   midiCtrlName
//---------------------------------------------------------

QString midiCtrlName(int ctrl)
      {
      if (ctrl < 0x10000)
            return QString(ctrlName[ctrl]);
      QString s("?N?");
      switch(ctrl) {
            case CTRL_PITCH:
                  s = "Pitch";
                  break;
            case CTRL_PROGRAM:
                  s = "ProgramChange";
                  break;
            case CTRL_VELOCITY:
                  s = "Velocity";
                  break;
            case CTRL_MASTER_VOLUME:
                  s = "MasterVolume";
                  break;
            default:
                  printf("midiCtrlName unknown %x\n", ctrl);
                  break;
            }
      return s;
      }

//---------------------------------------------------------
//   MidiController
//---------------------------------------------------------

MidiController::MidiController()
   : _name(QString(QT_TR_NOOP("Velocity")))
      {
      _num     = CTRL_VELOCITY;
      _minVal  = 0;
      _maxVal  = 127;
      _initVal = 0;
      }

MidiController::MidiController(const QString& s, int n, int min, int max, int init)
   : _name(s), _num(n), _minVal(min), _maxVal(max), _initVal(init)
      {
      }

//---------------------------------------------------------
//   type
//---------------------------------------------------------

MidiController::ControllerType midiControllerType(int num)
      {
      if (num < 0x10000)
            return MidiController::Controller7;
      if (num < 0x20000)
            return MidiController::Controller14;
      if (num < 0x30000)
            return MidiController::RPN;
      if (num < 0x40000)
            return MidiController::NRPN;
      if (num == CTRL_PITCH)
            return MidiController::Pitch;
      if (num == CTRL_PROGRAM)
            return MidiController::Program;
      if (num == CTRL_VELOCITY || num == CTRL_SVELOCITY)
            return MidiController::Velo;
      if (num < 0x60000)
            return MidiController::RPN14;
      if (num < 0x70000)
            return MidiController::NRPN14;
      return MidiController::Controller7;
      }

//---------------------------------------------------------
//   MidiController::write
//---------------------------------------------------------

void MidiController::write(Xml& xml) const
      {
      ControllerType t = midiControllerType(_num);
      QString type(int2ctrlType(t));

      int h = (_num >> 8) & 0x7f;
      int l = _num & 0x7f;

      QString sl;
      if (_num & 0xff == 0xff)
            sl = "Pitch";
      else
            sl.setNum(l);

      xml.tagE("Controller name=\"%s\" type=\"%s\" h=\"%d\" l=\"%s\" min=\"%d\" max\"%d\" init=\"%d\"",
         _name.toLatin1().data(), type.toLatin1().data(), h, sl.toLatin1().data(), _minVal, _maxVal, _initVal);
      }

//---------------------------------------------------------
//   MidiController::read
//---------------------------------------------------------

void MidiController::read(QDomNode node)
      {
      ControllerType t = Controller7;
      static const int NOT_SET = 0x100000;
      int l    = 0;

      QDomElement e = node.toElement();
      _name = e.attribute("name");
      t = ctrlType2Int(e.attribute("type"));
      int h = e.attribute("h","0").toInt(0,0);
      QString s = e.attribute("l");
      if (s == "pitch")
            l = 0xff;
      else
            l  = s.toInt(0,0);
      _minVal  = e.attribute("min",  "0").toInt(0,0);
      _maxVal  = e.attribute("max",  "0x100000").toInt(0,0);
      _initVal = e.attribute("init", "-1").toInt(0,0);

      _num = (h << 8) + l;
      switch (t) {
            case RPN:
                  if (_maxVal == NOT_SET)
                        _maxVal = 127;
                  _num |= 0x20000;
                  break;
            case NRPN:
                  if (_maxVal == NOT_SET)
                        _maxVal = 127;
                  _num |= 0x30000;
                  break;
            case Controller7:
                  if (_maxVal == NOT_SET)
                        _maxVal = 127;
                  break;
            case Controller14:
                  _num |= 0x10000;
                  if (_maxVal == NOT_SET)
                        _maxVal = 127 * 127 * 127;
                  break;
            case RPN14:
                  if (_maxVal == NOT_SET)
                        _maxVal = 127 * 127 * 127;
                  _num |= 0x50000;
                  break;
            case NRPN14:
                  if (_maxVal == NOT_SET)
                        _maxVal = 127 * 127 * 127;
                  _num |= 0x60000;
                  break;
            case Pitch:
                  if (_maxVal == NOT_SET)
                        _maxVal = 8191;
                  if (_minVal == NOT_SET)
                        _minVal = -8192;
                  _num = CTRL_PITCH;
                  break;
            case Program:
                  if (_maxVal == NOT_SET)
                        _maxVal = 127 * 127 * 127;
                  _num = CTRL_PROGRAM;
                  break;
            case Velo:        // cannot happen
                  break;
            }
      return;
      }

