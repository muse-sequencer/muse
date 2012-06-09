//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: midictrl.cpp,v 1.17.2.10 2009/06/10 00:34:59 terminator356 Exp $
//
//  (C) Copyright 2001-2004 Werner Schweer (ws@seh.de)
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

#include "midictrl.h"
#include "xml.h"
#include "globals.h"

namespace MusECore {

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
// Removed p3.3.37 Re-added p4.0.15
static MidiController mastervolCtrl("MasterVolume", CTRL_MASTER_VOLUME, 0, 0x3fff, 0x3000);
static MidiController volumeCtrl("MainVolume",      CTRL_VOLUME,        0, 127, 100);
static MidiController panCtrl("Pan",                CTRL_PANPOT,      -64, 63,    0);


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
      { MidiController::RPN14, QString("RPN14") },
      { MidiController::NRPN14, QString("NRPN14") },
      { MidiController::Pitch, QString("Pitch") },
      { MidiController::Program, QString("Program") },
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
      defaultMidiController.add(&veloCtrl);
      defaultMidiController.add(&pitchCtrl);
      defaultMidiController.add(&programCtrl);
      defaultMidiController.add(&mastervolCtrl);
      defaultMidiController.add(&volumeCtrl);
      defaultMidiController.add(&panCtrl);
      }

//---------------------------------------------------------
//   midiCtrlNumString
//---------------------------------------------------------

QString midiCtrlNumString(int ctrl, bool fullyQualified)
{
      int h = (ctrl >> 8) & 0xff;
      int l = ctrl & 0xff;
      QString s1 = QString("%1").arg(h);
      QString s2 = ( l == 0xff ? QString("* ") : QString("%1 ").arg(l) );
      MidiController::ControllerType type = midiControllerType(ctrl);
      switch (type)
      {
        case MidiController::Controller7:
          if(fullyQualified)
            return s2;
          else  
            return QString();
        case MidiController::Controller14:
          return s1 + QString("CF") + s2;
        case MidiController::RPN:
          return s1 + QString("R") + s2;
        case MidiController::NRPN:
          return s1 + QString("N") + s2;
        case MidiController::Pitch:    // Don't show internal controller numbers. 
          return QString();
        case MidiController::Program:
          return QString();
        case MidiController::Velo:
          return QString();
        case MidiController::RPN14:
          return s1 + QString("RF") + s2;
        case MidiController::NRPN14:
          return s1 + QString("NF") + s2;
      }
      return s1 + QString("?") + s2;
}

//---------------------------------------------------------
//   midiCtrlName
//---------------------------------------------------------

QString midiCtrlName(int ctrl, bool fullyQualified)
{
      int h = (ctrl >> 8) & 0xff;
      int l = ctrl & 0xff;
      QString s1 = QString("%1").arg(h);
      QString s2 = ( l == 0xff ? QString("*") : QString("%1").arg(l) );
      MidiController::ControllerType type = midiControllerType(ctrl);
      switch (type)
      {
        case MidiController::Controller7:
          if(fullyQualified)
            return s2 + QString(" ") + QString(ctrlName[l]);
          else  
            return QString(ctrlName[l]);
        case MidiController::Controller14:
          return s1 + QString("CF") + s2;
        case MidiController::RPN:
          return s1 + QString("R") + s2;
        case MidiController::NRPN:
          return s1 + QString("N") + s2;
        case MidiController::Pitch:
          return QString("Pitch");
        case MidiController::Program:
          return QString("Program");
        case MidiController::Velo:
          return QString("Velocity");
        case MidiController::RPN14:
          return s1 + QString("RF") + s2;
        case MidiController::NRPN14:
          return s1 + QString("NF") + s2;
      }
      return s1 + QString("?") + s2;
}

//---------------------------------------------------------
//   MidiController
//---------------------------------------------------------

MidiController::MidiController()
   : _name(QString("Velocity"))
      {
      _num     = CTRL_VELOCITY;
      _minVal  = 0;
      _maxVal  = 127;
      _initVal = 0;
      updateBias();
      }

MidiController::MidiController(const QString& s, int n, int min, int max, int init)
   : _name(s), _num(n), _minVal(min), _maxVal(max), _initVal(init)
      {
      updateBias();
      }

MidiController::MidiController(const MidiController& mc)
{
  copy(mc);
}

//---------------------------------------------------------
//   copy
//---------------------------------------------------------

void MidiController::copy(const MidiController &mc)
{
      _name    = mc._name;
      _num     = mc._num;
      _minVal  = mc._minVal;
      _maxVal  = mc._maxVal;
      _initVal = mc._initVal;
      _bias    = mc._bias;
}

//---------------------------------------------------------
//   operator =
//---------------------------------------------------------

MidiController& MidiController::operator=(const MidiController &mc)
{
  copy(mc);
  return *this;
}

//---------------------------------------------------------
//   type
//---------------------------------------------------------

MidiController::ControllerType midiControllerType(int num)
      {
      if (num < CTRL_14_OFFSET)
            return MidiController::Controller7;
      if (num < CTRL_RPN_OFFSET)
            return MidiController::Controller14;
      if (num < CTRL_NRPN_OFFSET)
            return MidiController::RPN;
      if (num < CTRL_INTERNAL_OFFSET)
            return MidiController::NRPN;
      if (num == CTRL_PITCH)
            return MidiController::Pitch;
      if (num == CTRL_PROGRAM)
            return MidiController::Program;
      if (num == CTRL_VELOCITY)
            return MidiController::Velo;
      if (num < CTRL_NRPN14_OFFSET)
            return MidiController::RPN14;
      if (num < CTRL_NONE_OFFSET)
            return MidiController::NRPN14;
      return MidiController::Controller7;
      }

//---------------------------------------------------------
//   midiCtrlTerms2Number
//---------------------------------------------------------

int midiCtrlTerms2Number(int type_num, int ctrl)
{
  ctrl &= 0xffff;
  switch(type_num)
  {
    case MidiController::Controller7:
      return ctrl & 0xff;
    case MidiController::Controller14:
      return CTRL_14_OFFSET + ctrl;
    case MidiController::RPN:
      return CTRL_RPN_OFFSET + ctrl;
    case MidiController::NRPN:
      return CTRL_NRPN_OFFSET  + ctrl;
    case MidiController::Pitch:
      return CTRL_PITCH;
    case MidiController::Program:
      return CTRL_PROGRAM;
    case MidiController::Velo:
      return CTRL_VELOCITY;
    case MidiController::RPN14:
      return CTRL_RPN14_OFFSET + ctrl;
    case MidiController::NRPN14:
      return CTRL_NRPN14_OFFSET + ctrl;
    default:
      printf("MusE: unknown ctrl type in midiCtrTerms2Number()\n");
      return ctrl;
  }
}

//---------------------------------------------------------
//   updateBias
//---------------------------------------------------------

void MidiController::updateBias()
{
  // If the specified minimum value is negative, we will 
  //  translate to a positive-biased range.
  // Cue: A controller like 'coarse tuning', is meant to be displayed
  //  as -24/+24, but really is centered on 64 and goes from 40 to 88.
  int b;
  int mn, mx;
  ControllerType t = midiControllerType(_num);
  switch (t) 
  {
    case RPN:
    case NRPN:
    case Controller7:
          b = 64;
          mn = 0;
          mx = 127;
          break;
    case Controller14:
    case RPN14:
    case NRPN14:
          b = 8192;
          mn = 0;
          mx = 16383;
          break;
    case Program:
          b =  0x800000;
          mn = 0;
          mx = 0xffffff;
          break;
    case Pitch:
          b = 0;
          mn = -8192;
          mx = 8191;
          break;
    //case Velo:        // cannot happen
    default:
          b = 64;
          mn = 0;
          mx = 127;
          break;
  }
  
  // TODO: Limit _minVal and _maxVal to range.
  
  if(_minVal >= 0)
    _bias = 0;
  else
  {
    _bias = b;
    
    if(t != Program && t != Pitch)
    {
      // Adjust bias to fit desired range.
      if(_minVal + _bias < mn)
        _bias += mn - _minVal + _bias;
      else
      if(_maxVal + _bias > mx)
        _bias -= _maxVal + _bias - mx;
    }  
  }  
}


//---------------------------------------------------------
//   MidiController::write
//---------------------------------------------------------

void MidiController::write(int level, Xml& xml) const
{
      ControllerType t = midiControllerType(_num);
      if(t == Velo)
        return;
        
      QString type(int2ctrlType(t));

      int h = (_num >> 8) & 0x7f;
      int l = _num & 0x7f;

      QString sl;
      if ((_num & 0xff) == 0xff)
            sl = "pitch";
      else
            sl.setNum(l);

      xml.nput(level, "<Controller name=\"%s\"", Xml::xmlString(_name).toLatin1().constData());
      if(t != Controller7)
        xml.nput(" type=\"%s\"", type.toLatin1().constData());
      
      int mn = 0;
      int mx = 0; 
      switch (t) 
      {
            case RPN:
            case NRPN:
                  xml.nput(" h=\"%d\"", h);
                  xml.nput(" l=\"%s\"", sl.toLatin1().constData());
                  mx = 127;
                  break;
            case Controller7:
                  xml.nput(" l=\"%s\"", sl.toLatin1().constData());
                  mx = 127;
                  break;
            case Controller14:
            case RPN14:
            case NRPN14:
                  xml.nput(" h=\"%d\"", h);
                  xml.nput(" l=\"%s\"", sl.toLatin1().constData());
                  mx = 16383;
                  break;
            case Pitch:
                  mn = -8192;
                  mx = 8191;
                  break;
            case Program:
            case Velo:        // Cannot happen
                  break;
      }
      
      if(t == Program)
      {
        if(_initVal != CTRL_VAL_UNKNOWN && _initVal != 0xffffff)
          xml.nput(" init=\"0x%x\"", _initVal);
      }
      else
      {
        if(_minVal != mn)     
          xml.nput(" min=\"%d\"", _minVal);
        if(_maxVal != mx)     
          xml.nput(" max=\"%d\"", _maxVal);
        
        if(_initVal != CTRL_VAL_UNKNOWN)     
          xml.nput(" init=\"%d\"", _initVal);
      }
      xml.put(" />");
}

//---------------------------------------------------------
//   MidiController::read
//---------------------------------------------------------

void MidiController::read(Xml& xml)
      {
      ControllerType t = Controller7;
      _initVal = CTRL_VAL_UNKNOWN;
      static const int NOT_SET = 0x100000;
      _minVal  = NOT_SET;
      _maxVal  = NOT_SET;
      int h    = 0;
      int l    = 0;
      bool     ok;
      int base = 10;

      for (;;) {
            Xml::Token token = xml.parse();
            const QString& tag = xml.s1();
            switch (token) {
                  case Xml::Error:
                  case Xml::End:
                        return;
                  case Xml::Attribut:
                        {
                        QString s = xml.s2();
                        if (s.left(2) == "0x")
                              base = 16;
                        if (tag == "name")
                              _name = xml.s2();
                        else if (tag == "type")
                              t = ctrlType2Int(xml.s2());
                        else if (tag == "h")
                              h = xml.s2().toInt(&ok, base);
                        else if (tag == "l") {
                              // By T356 08/16/08. Changed wildcard to '*'. 
                              // Changed back to 'pitch' again.
                              // Support instrument files with '*' as wildcard.
                              if ((xml.s2() == "*") || (xml.s2() == "pitch"))
                                    l = 0xff;
                              else
                                    l = xml.s2().toInt(&ok, base);
                              }
                        else if (tag == "min")
                              _minVal = xml.s2().toInt(&ok, base);
                        else if (tag == "max")
                              _maxVal = xml.s2().toInt(&ok, base);
                        else if (tag == "init")
                              _initVal = xml.s2().toInt(&ok, base);
                        }
                        break;
                  case Xml::TagStart:
                        xml.unknown("MidiController");
                        break;
                  case Xml::TagEnd:
                        if (tag == "Controller") {
                              _num = (h << 8) + l;
                              switch (t) {
                                    case RPN:
                                          if (_maxVal == NOT_SET)
                                                _maxVal = 127;
                                          _num |= CTRL_RPN_OFFSET;
                                          break;
                                    case NRPN:
                                          if (_maxVal == NOT_SET)
                                                _maxVal = 127;
                                          _num |= CTRL_NRPN_OFFSET;
                                          break;
                                    case Controller7:
                                          if (_maxVal == NOT_SET)
                                                _maxVal = 127;
                                          break;
                                    case Controller14:
                                          _num |= CTRL_14_OFFSET;
                                          if (_maxVal == NOT_SET)
                                                _maxVal = 16383;
                                          break;
                                    case RPN14:
                                          if (_maxVal == NOT_SET)
                                                _maxVal = 16383;
                                          _num |= CTRL_RPN14_OFFSET;
                                          break;
                                    case NRPN14:
                                          if (_maxVal == NOT_SET)
                                                _maxVal = 16383;
                                          _num |= CTRL_NRPN14_OFFSET;
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
                                                _maxVal = 0xffffff;
                                          _num = CTRL_PROGRAM;
                                          break;
                                    case Velo:        // cannot happen
                                          break;
                                    }
                              if (_minVal == NOT_SET)
                                    _minVal = 0;
                                    
                              updateBias();
                              return;
                              }
                  default:
                        break;
                  }
            }
      }

//---------------------------------------------------------
//   genNum
//---------------------------------------------------------

int MidiController::genNum(MidiController::ControllerType t, int h, int l)
      {
      int val = (h << 8) + l;
      switch(t) {
            case Controller7:
                  return l;
            case Controller14:
                  return val + CTRL_14_OFFSET;
            case RPN:
                  return val + CTRL_RPN_OFFSET;
            case NRPN:
                  return val + CTRL_NRPN_OFFSET;
            case RPN14:
                  return val + CTRL_RPN14_OFFSET;
            case NRPN14:
                  return val + CTRL_NRPN14_OFFSET;
            case Pitch:
                  return CTRL_PITCH;
            case Program:
                  return CTRL_PROGRAM;
            default:
                  return -1;
            }      
      }

//---------------------------------------------------------
//   MidiCtrlValList
//---------------------------------------------------------

MidiCtrlValList::MidiCtrlValList(int c)
      {
      ctrlNum = c;
      _hwVal = CTRL_VAL_UNKNOWN;
      _lastValidHWVal = CTRL_VAL_UNKNOWN;
      }

//---------------------------------------------------------
//   clearDelete
//---------------------------------------------------------

void MidiCtrlValListList::clearDelete(bool deleteLists)
{
  for(iMidiCtrlValList imcvl = begin(); imcvl != end(); ++imcvl)
  {
    if(imcvl->second)
    {
      imcvl->second->clear();
      if(deleteLists)
        delete imcvl->second;
    }  
  }
  if(deleteLists)
    clear();
}
      
//---------------------------------------------------------
//   setHwVal
//   Returns false if value is already equal, true if value is changed.
//---------------------------------------------------------
      
bool MidiCtrlValList::setHwVal(const int v)    
{ 
  if(_hwVal == v)
    return false;
  
  _hwVal = v;
  if(_hwVal != CTRL_VAL_UNKNOWN)
    _lastValidHWVal = _hwVal;
    
  return true;  
}

//---------------------------------------------------------
//   setHwVals
//   Sets current and last HW values.
//   Handy for forcing labels to show 'off' and knobs to show specific values 
//    without having to send two messages.
//   Returns false if both values are already set, true if either value is changed.
//---------------------------------------------------------

bool MidiCtrlValList::setHwVals(const int v, int const lastv)
{
  if(_hwVal == v && _lastValidHWVal == lastv)
    return false;
  
  _hwVal = v;
  // Don't want to break our own rules - _lastValidHWVal can't be unknown while _hwVal is valid...
  // But _hwVal can be unknown while _lastValidHWVal is valid...
  if(lastv == CTRL_VAL_UNKNOWN)
    _lastValidHWVal = _hwVal;
  else  
    _lastValidHWVal = lastv;
    
  return true;  
}

//---------------------------------------------------------
//   partAtTick
//---------------------------------------------------------

Part* MidiCtrlValList::partAtTick(int tick) const
{
      // Determine (first) part at tick. Return 0 if none found.
      
      ciMidiCtrlVal i = lower_bound(tick);
      if (i == end() || i->first != tick) {
            if (i == begin())
                  return 0;
            --i;
            }
      return i->second.part;
}

//---------------------------------------------------------
//   iValue
//---------------------------------------------------------

iMidiCtrlVal MidiCtrlValList::iValue(int tick) 
{
      // Determine value at tick, using values stored by ANY part...
      
      iMidiCtrlVal i = lower_bound(tick);
      if (i == end() || i->first != tick) {
            if (i == begin())
                  return end();
            --i;
            }
      return i;
}

//---------------------------------------------------------
//   value
//---------------------------------------------------------

int MidiCtrlValList::value(int tick) const
{
      // Determine value at tick, using values stored by ANY part...
      
      ciMidiCtrlVal i = lower_bound(tick);
      if (i == end() || i->first != tick) {
            if (i == begin())
                  return CTRL_VAL_UNKNOWN;
            --i;
            }
      return i->second.val;
}

int MidiCtrlValList::value(int tick, Part* part) const
{
  // Determine value at tick, using values stored by the SPECIFIC part...
  
  // Get the first value found with with a tick equal or greater than specified tick.
  ciMidiCtrlVal i = lower_bound(tick);
  // Since values from different parts can have the same tick, scan for part in all values at that tick.
  for(ciMidiCtrlVal j = i; j != end() && j->first == tick; ++j)
  {
    if(j->second.part == part)
      return j->second.val;
  }
  // Scan for part in all previous values, regardless of tick.
  while(i != begin())
  {
    --i;  
    if(i->second.part == part)
      return i->second.val;
  }
  // No previous values were found belonging to the specified part. 
  return CTRL_VAL_UNKNOWN;
}

//---------------------------------------------------------
//   add
//    return true if new controller value added or replaced
//---------------------------------------------------------

bool MidiCtrlValList::addMCtlVal(int tick, int val, Part* part)
      {
      iMidiCtrlVal e = findMCtlVal(tick, part);
      
      if (e != end()) {
            if(e->second.val != val)
            {
              e->second.val = val;
              return true;
            }  
            return false;
          }
            
      MidiCtrlVal v;
      v.val = val;
      v.part = part;
      insert(std::pair<const int, MidiCtrlVal> (tick, v));
      return true;
      }

//---------------------------------------------------------
//   del
//---------------------------------------------------------

void MidiCtrlValList::delMCtlVal(int tick, Part* part)
{
      iMidiCtrlVal e = findMCtlVal(tick, part);
      if (e == end()) {
	if(MusEGlobal::debugMsg)
              printf("MidiCtrlValList::delMCtlVal(%d): not found (size %zd)\n", tick, size());
            return;
            }
      erase(e);
}

//---------------------------------------------------------
//   find
//---------------------------------------------------------

iMidiCtrlVal MidiCtrlValList::findMCtlVal(int tick, Part* part)
{
  MidiCtrlValRange range = equal_range(tick);
  for(iMidiCtrlVal i = range.first; i != range.second; ++i) 
  {
    if(i->second.part == part)
      return i;
  }
  return end();
}
      
//---------------------------------------------------------
//   MidiControllerList
//---------------------------------------------------------

MidiControllerList::MidiControllerList(const MidiControllerList& mcl) : std::map<int, MidiController*>()
{
  for(ciMidiController i = mcl.begin(); i != mcl.end(); ++i)
  {
    MidiController* mc = i->second;
    add(new MidiController(*mc));
  }  
}

} // namespace MusECore
