//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: midictrl.cpp,v 1.17.2.10 2009/06/10 00:34:59 terminator356 Exp $
//
//  (C) Copyright 2001-2004 Werner Schweer (ws@seh.de)
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

#include <cstdio>

#include "midi_controller.h"

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
      { MidiController::PolyAftertouch, QString("PolyAftertouch") },
      { MidiController::Aftertouch, QString("Aftertouch") },
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
        case MidiController::Program:
        case MidiController::Velo:
        case MidiController::PolyAftertouch:
        case MidiController::Aftertouch:
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
        case MidiController::PolyAftertouch:
          return QString("PolyAftertouch");
        case MidiController::Aftertouch:
          return QString("Aftertouch");
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
      // Zero note on vel is not allowed now.
      _minVal  = 1;
      _maxVal  = 127;
      _initVal = _drumInitVal = 0;
      _showInTracks = ShowInDrum | ShowInMidi;
      updateBias();
      }

MidiController::MidiController(const QString& s, int n, int min, int max, int init, int drumInit, int show_in_track)
   : _name(s), _num(n), _minVal(min), _maxVal(max), _initVal(init), _showInTracks(show_in_track)
      {
      // If drumInit was given, use it otherwise set it to the normal init val.
      if(drumInit != -1)
        _drumInitVal = drumInit;
      else
        _drumInitVal = _initVal;
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
      _name         = mc._name;
      _num          = mc._num;
      _minVal       = mc._minVal;
      _maxVal       = mc._maxVal;
      _initVal      = mc._initVal;
      _drumInitVal  = mc._drumInitVal;
      _bias         = mc._bias;
      _showInTracks = mc._showInTracks;
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
      if ((num | 0xff) == CTRL_POLYAFTER)
            return MidiController::PolyAftertouch;
      if (num == CTRL_AFTERTOUCH)
            return MidiController::Aftertouch;
      if (num < CTRL_NRPN14_OFFSET)
            return MidiController::RPN14;
      if (num < CTRL_NONE_OFFSET)
            return MidiController::NRPN14;
      return MidiController::Controller7;
      }

//---------------------------------------------------------
//   midiCtrlTerms2Number
//---------------------------------------------------------

int midiCtrlTerms2Number(MidiController::ControllerType type, int ctrl)
{
  ctrl &= 0xffff;
  switch(type)
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
    case MidiController::PolyAftertouch:
      return CTRL_POLYAFTER;
    case MidiController::Aftertouch:
      return CTRL_AFTERTOUCH;
    case MidiController::RPN14:
      return CTRL_RPN14_OFFSET + ctrl;
    case MidiController::NRPN14:
      return CTRL_NRPN14_OFFSET + ctrl;
    default:
      printf("MusE: unknown ctrl type in midiCtrTerms2Number()\n");
      return ctrl;
  }
}


// REMOVE Tim. midnam. Added.
//---------------------------------------------------------
//   isPerNoteMidiController
//---------------------------------------------------------

bool isPerNoteMidiController(int ctl)
{
  const int n = ctl & 0xff0000;
  return ctl == CTRL_POLYAFTER ||
     ((ctl & 0xff) == 0xff &&
      (n == CTRL_RPN_OFFSET ||
       n == CTRL_NRPN_OFFSET ||
       n == CTRL_RPN14_OFFSET ||
       n == CTRL_NRPN14_OFFSET));
}

// REMOVE Tim. midnam. Added.
//---------------------------------------------------------
//   isPerNoteController
//---------------------------------------------------------

bool MidiController::isPerNoteController() const
{
  return isPerNoteMidiController(num());
}

// REMOVE Tim. midnam. Added.
//---------------------------------------------------------
//   type
//---------------------------------------------------------

MidiController::ControllerType MidiController::type() const
{
  return midiControllerType(num());
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
    //case PolyAfter:
    //case After:      
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
      if (isPerNoteController())
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
            case PolyAftertouch:
                  mn = 0;
                  mx = 127;
                  break;
            case Aftertouch: 
                  mn = 0;
                  mx = 127;
                  break;
            case Program:
            case Velo:        // Cannot happen
                  break;
      }
      
      if(t == Program)
      {
        if(_initVal != CTRL_VAL_UNKNOWN && _initVal != 0xffffff)
          xml.nput(" init=\"0x%x\"", _initVal);
        if(_drumInitVal != CTRL_VAL_UNKNOWN && _drumInitVal != 0xffffff)
          xml.nput(" drumInit=\"0x%x\"", _drumInitVal);
      }
      else
      {
        if(_minVal != mn)     
          xml.nput(" min=\"%d\"", _minVal);
        if(_maxVal != mx)     
          xml.nput(" max=\"%d\"", _maxVal);
        
        if(_initVal != CTRL_VAL_UNKNOWN)     
          xml.nput(" init=\"%d\"", _initVal);
        if(_drumInitVal != CTRL_VAL_UNKNOWN)
          xml.nput(" drumInit=\"%d\"", _drumInitVal);
      }

      if(_showInTracks != (ShowInDrum | ShowInMidi))
          xml.nput(" showType=\"%d\"", _showInTracks);
        
      xml.put(" />");
}

//---------------------------------------------------------
//   MidiController::read
//---------------------------------------------------------

void MidiController::read(Xml& xml)
      {
      ControllerType t = Controller7;
      _initVal = CTRL_VAL_UNKNOWN;
      int drum_init = -1; //  -1 = Not set yet.
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
                        goto mc_read_end;
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
                              // Support instrument files with '*' or 'pitch' as wildcard.
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
                        else if (tag == "drumInit")
                              drum_init = xml.s2().toInt(&ok, base);
                        else if (tag == "showType")
                              _showInTracks = xml.s2().toInt(&ok, base);
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
                                    case PolyAftertouch:   
                                          if (_maxVal == NOT_SET)
                                                _maxVal = 127;
                                          if (_minVal == NOT_SET)
                                                _minVal = 0;
                                          _num = CTRL_POLYAFTER;
                                          break;
                                    case Aftertouch:       
                                          if (_maxVal == NOT_SET)
                                                _maxVal = 127;
                                          if (_minVal == NOT_SET)
                                                _minVal = 0;
                                          _num = CTRL_AFTERTOUCH;
                                          break;
                                    case Velo:        // cannot happen
                                          break;
                                    }
                              if (_minVal == NOT_SET)
                                    _minVal = 0;
                              // No drum init val given? Use normal init val.
                              if(drum_init == -1)
                                _drumInitVal = _initVal;
                              else
                                _drumInitVal = drum_init;
                              updateBias();
                              return;
                              }
                  default:
                        break;
                  }
            }

mc_read_end:
      _drumInitVal = _initVal;
      }

//---------------------------------------------------------
//   genNum
//---------------------------------------------------------

int MidiController::genNum(MidiController::ControllerType t, int h, int l)
      {
      int val = (h << 8) | (l & 0xff);
      switch(t) {
            case Controller7:
                  return l & 0xff;
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
            case PolyAftertouch:
                  return CTRL_POLYAFTER;
            case Aftertouch:
                  return CTRL_AFTERTOUCH;
            default:
                  return -1;
            }      
      }

//---------------------------------------------------------
//   MidiControllerList
//---------------------------------------------------------

MidiControllerList::MidiControllerList() 
{
  _RPN_Ctrls_Reserved = false;
}

MidiControllerList::MidiControllerList(const MidiControllerList& mcl) : std::map<int, MidiController*>()
{
  for(ciMidiController i = mcl.begin(); i != mcl.end(); ++i)
  {
    MidiController* mc = i->second;
    add(new MidiController(*mc));
  }  
  update_RPN_Ctrls_Reserved();
}

bool MidiControllerList::add(MidiController* mc, bool update) 
{ 
  const int num = mc->num();
  if(!_RPN_Ctrls_Reserved && update)
  {
    const bool isCtl7  = ((num & CTRL_OFFSET_MASK) == CTRL_7_OFFSET);
    const bool isCtl14 = ((num & CTRL_OFFSET_MASK) == CTRL_14_OFFSET);
    if(isCtl14 || isCtl7)
    {
      const int l = num & 0xff;
      if(l == CTRL_HDATA    ||
         l == CTRL_LDATA    ||
         l == CTRL_DATA_INC ||
         l == CTRL_DATA_DEC ||
         l == CTRL_HNRPN    ||
         l == CTRL_LNRPN    ||
         l == CTRL_HRPN     ||
         l == CTRL_LRPN)
        _RPN_Ctrls_Reserved = true;
    }
    if(!_RPN_Ctrls_Reserved && isCtl14)
    {    
      const int h = (num >> 8) & 0xff;
      if(h == CTRL_HDATA    ||
         h == CTRL_LDATA    ||
         h == CTRL_DATA_INC ||
         h == CTRL_DATA_DEC ||
         h == CTRL_HNRPN    ||
         h == CTRL_LNRPN    ||
         h == CTRL_HRPN     ||
         h == CTRL_LRPN)     
        _RPN_Ctrls_Reserved = true;
    }
  }
  return insert(MidiControllerListPair(num, mc)).second; 
}

void MidiControllerList::del(iMidiController ictl, bool update) 
{ 
  erase(ictl); 
  if(update)
    update_RPN_Ctrls_Reserved();
}

MidiControllerList::size_type MidiControllerList::del(int num, bool update) 
{ 
  MidiControllerList::size_type res = erase(num);
  if(update)
    update_RPN_Ctrls_Reserved();
  return res;
}

void MidiControllerList::del(iMidiController first, iMidiController last, bool update) 
{ 
  erase(first, last); 
  if(update)
    update_RPN_Ctrls_Reserved();
}

void MidiControllerList::clr() 
{ 
  clear(); 
  update_RPN_Ctrls_Reserved();
}

//---------------------------------------------------------
//   update_RPN_Ctrls_Reserved
//   Manual check and update of the flag. For convenience, returns the flag.
//   Cost depends on types and number of list controllers, so it is good for deferring 
//    an update until AFTER some lengthy list operation. JUST BE SURE to call this!
//---------------------------------------------------------

bool MidiControllerList::update_RPN_Ctrls_Reserved()
{
  if(find(CTRL_HDATA) != end() ||
     find(CTRL_LDATA) != end() ||
     find(CTRL_DATA_INC) != end() ||
     find(CTRL_DATA_DEC) != end() ||
     find(CTRL_HNRPN) != end() ||
     find(CTRL_LNRPN) != end() ||
     find(CTRL_HRPN) != end() ||
     find(CTRL_LRPN) != end())
  {
    _RPN_Ctrls_Reserved = true;
    return true;
  }
  
  int num, h, l;
  // Search: Get a head-start by taking lower bound.
  for(iMidiController imc = lower_bound(CTRL_14_OFFSET); imc != end(); ++imc)
  {
    num = imc->second->num();
    // Stop if we went beyond this ctrl14 block. 
    if((num & CTRL_OFFSET_MASK) != CTRL_14_OFFSET)
    {
      _RPN_Ctrls_Reserved = false;
      return false;
    }
    h = (num >> 8) & 0xff;
    l = num & 0xff;
    if(h == CTRL_HDATA    || l == CTRL_HDATA    ||
       h == CTRL_LDATA    || l == CTRL_LDATA    ||
       h == CTRL_DATA_INC || l == CTRL_DATA_INC ||
       h == CTRL_DATA_DEC || l == CTRL_DATA_DEC ||
       h == CTRL_HNRPN    || l == CTRL_HNRPN    ||
       h == CTRL_LNRPN    || l == CTRL_LNRPN    ||
       h == CTRL_HRPN     || l == CTRL_HRPN     ||
       h == CTRL_LRPN     || l == CTRL_LRPN)
    {
      _RPN_Ctrls_Reserved = true;
      return true;
    }
  }
  _RPN_Ctrls_Reserved = false;
  return false;
}

//---------------------------------------------------------
//     Catch all insert, erase, clear etc.
//---------------------------------------------------------

MidiControllerList& MidiControllerList::operator=(const MidiControllerList& cl)
{
#ifdef _MIDI_CTRL_DEBUG_
  printf("MidiControllerList::operator=\n");  
#endif
  _RPN_Ctrls_Reserved = cl._RPN_Ctrls_Reserved;
  
  // Let map copy the items.
  std::map<int, MidiController*, std::less<int> >::operator=(cl);
  return *this;
}

//=========================================================
#ifdef _MIDI_CTRL_METHODS_DEBUG_      

void MidiControllerList::swap(MidiControllerList& cl)
{
#ifdef _MIDI_CTRL_DEBUG_
  printf("MidiControllerList::swap\n");  
#endif
  std::map<int, MidiController*, std::less<int> >::swap(cl);
}

std::pair<iMidiController, bool> MidiControllerList::insert(const std::pair<int, MidiController*>& p)
{
#ifdef _MIDI_CTRL_DEBUG_
  printf("MidiControllerList::insert num:%d\n", p.second->num());  
#endif
  std::pair<iMidiController, bool> res = std::map<int, MidiController*, std::less<int> >::insert(p);
  return res;
}

iMidiController MidiControllerList::insert(iMidiController ic, const std::pair<int, MidiController*>& p)
{
#ifdef _MIDI_CTRL_DEBUG_
  printf("MidiControllerList::insertAt num:%d\n", p.second->num()); 
#endif
  iMidiController res = std::map<int, MidiController*, std::less<int> >::insert(ic, p);
  return res;
}

void MidiControllerList::erase(iMidiController ictl)
{
#ifdef _MIDI_CTRL_DEBUG_
  printf("MidiControllerList::erase iMidiController num:%d\n", ictl->second->num());  
#endif
  std::map<int, MidiController*, std::less<int> >::erase(ictl);
}

MidiControllerList::size_type MidiControllerList::erase(int num)
{
#ifdef _MIDI_CTRL_DEBUG_
  printf("MidiControllerList::erase num:%d\n", num);  
#endif
  size_type res = std::map<int, MidiController*, std::less<int> >::erase(num);
  return res;
}

void MidiControllerList::erase(iMidiController first, iMidiController last)
{
#ifdef _MIDI_CTRL_DEBUG_
  printf("MidiControllerList::erase range first num:%d second num:%d\n", 
         first->second->num(), last->second->num());  
#endif
  std::map<int, MidiController*, std::less<int> >::erase(first, last);
}

void MidiControllerList::clear()
{
#ifdef _MIDI_CTRL_DEBUG_
  printf("MidiControllerList::clear\n");  
#endif
  std::map<int, MidiController*, std::less<int> >::clear();
}

#endif
// =========================================================

// REMOVE Tim. midnam. Added.
//---------------------------------------------------------
//   perNoteController
//   Returns per-note controller if there is one for the given ctl number.
//   Otherwise returns null.
//---------------------------------------------------------

MidiController* MidiControllerList::perNoteController(int ctl) const
{
  const int n = ctl & 0xff0000;
  if(((ctl | 0xff) == CTRL_POLYAFTER) ||
      (n == CTRL_RPN_OFFSET ||
       n == CTRL_NRPN_OFFSET ||
       n == CTRL_RPN14_OFFSET ||
       n == CTRL_NRPN14_OFFSET))
  {  
    // Does the list have a per-note controller to match the controller number?
    const_iterator imc = find(ctl | 0xff);
    if(imc != cend())
      // Yes, it's a per-note controller. Return a pointer to it.
      return imc->second;
  }
  return nullptr;
}
            
MidiController* MidiControllerList::findController(int ctl) const
{
  // Find a controller for the verbose ctl number.
  const_iterator i = find(ctl);
  if(i != cend())
    return i->second;
  // Find a per-note controller for the ctl number.
  return perNoteController(ctl);
}

//---------------------------------------------------------
// searchControllers
//---------------------------------------------------------

iMidiController MidiControllerList::searchControllers(int ctl)
{
  const int type = ctl & CTRL_OFFSET_MASK;
  int n;
  
  // Looking for Controller7? See if any Controller14 contains the number and should be used instead.
  if(type == CTRL_7_OFFSET)
  {
    const int num = ctl & 0xff;
    for(iMidiController imc = lower_bound(CTRL_14_OFFSET); imc != end(); ++imc)
    {
      n = imc->second->num();
      // Stop if we went beyond this ctrl14 block. 
      if((n & CTRL_OFFSET_MASK) != CTRL_14_OFFSET)
        break;
      if(((n >> 8) & 0xff) == num || (n & 0xff) == num)
        return imc;
    }
  }
  // Looking for RPN? See if any RPN14 also uses the number and should be used instead.
  else if (type == CTRL_RPN_OFFSET)
  {
    const int num = ctl & 0xffff;
    for(iMidiController imc = lower_bound(CTRL_RPN14_OFFSET); imc != end(); ++imc)
    {
      n = imc->second->num();
      // Stop if we went beyond this RPN14 block. 
      if((n & CTRL_OFFSET_MASK) != CTRL_RPN14_OFFSET)
        break;
      if((n & 0xffff) == num)
        return imc;
    }
  }
  // Looking for NRPN? See if any NRPN14 also uses the number and should be used instead.
  else if (type == CTRL_NRPN_OFFSET)
  {
    const int num = ctl & 0xffff;
    for(iMidiController imc = lower_bound(CTRL_NRPN14_OFFSET); imc != end(); ++imc)
    {
      n = imc->second->num();
      // Stop if we went beyond this NRPN14 block. 
      if((n & CTRL_OFFSET_MASK) != CTRL_NRPN14_OFFSET)
        break;
      if((n & 0xffff) == num)
        return imc;
    }
  }
  
  // Looking for any other type? Do a regular find.
  return find(ctl);
}

//---------------------------------------------------------
//   ctrlAvailable 
//   Check if either a per-note controller, or else a regular controller already exists.
//---------------------------------------------------------

bool MidiControllerList::ctrlAvailable(int find_num, MidiController* ignore_this)
{
  MusECore::ciMidiController imc;
  for(imc = begin(); imc != end(); ++ imc)
  {
    // Ignore this controller.
    if(ignore_this && imc->second == ignore_this)
      continue;
    int n = imc->second->num();
    if(((find_num & 0xff) == 0xff) && ((n | 0xff) == find_num))
      break;
    if(imc->second->isPerNoteController() && ((find_num | 0xff) == n))
      break;
    if(find_num == n)
      break;
  }
  return imc == end();
}


} // namespace MusECore
