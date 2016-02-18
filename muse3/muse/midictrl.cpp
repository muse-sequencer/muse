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

#include <stdio.h>

#include "globaldefs.h"
#include "midictrl.h"
#include "xml.h"
#include "globals.h"

// REMOVE Tim. If necessary.
#include "audio.h"
#include "midi.h"
#include "mpevent.h"
#include "midiport.h"
#include "minstrument.h"

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
// REMOVE Tim. Noteoff. Changed. Zero note on vel is not allowed now.
// MidiController veloCtrl("Velocity",               CTRL_VELOCITY,       0,      127,      0);
MidiController veloCtrl("Velocity",               CTRL_VELOCITY,       1,      127,      0);
MidiController pitchCtrl("PitchBend",             CTRL_PITCH,      -8192,    +8191,      0);
MidiController programCtrl("Program",             CTRL_PROGRAM,        0, 0xffffff,      0);
MidiController mastervolCtrl("MasterVolume",      CTRL_MASTER_VOLUME,  0,   0x3fff, 0x3000);
MidiController volumeCtrl("MainVolume",           CTRL_VOLUME,         0,      127,    100);
MidiController panCtrl("Pan",                     CTRL_PANPOT,       -64,       63,      0);
MidiController reverbSendCtrl("ReverbSend",       CTRL_REVERB_SEND,    0,      127,      0);
MidiController chorusSendCtrl("ChorusSend",       CTRL_CHORUS_SEND,    0,      127,      0);
MidiController variationSendCtrl("VariationSend", CTRL_VARIATION_SEND, 0,      127,      0);

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
      defaultMidiController.add(&reverbSendCtrl);
      defaultMidiController.add(&chorusSendCtrl);
      defaultMidiController.add(&variationSendCtrl);
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
      // REMOVE Tim. Noteoff. Changed. Zero note on vel is not allowed now.
//       _minVal  = 0;
      _minVal  = 1;
      _maxVal  = 127;
      _initVal = 0;
      _showInTracks = ShowInDrum | ShowInMidi;
      updateBias();
      }

MidiController::MidiController(const QString& s, int n, int min, int max, int init, int show_in_track)
   : _name(s), _num(n), _minVal(min), _maxVal(max), _initVal(init), _showInTracks(show_in_track)
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
//   MidiCtrlValList
//---------------------------------------------------------

MidiCtrlValList::MidiCtrlValList(int c)
      {
      ctrlNum = c;
      _hwVal = CTRL_VAL_UNKNOWN;
      _lastValidHWVal = CTRL_VAL_UNKNOWN;
      }

//---------------------------------------------------------
//   MidiCtrlValListList
//---------------------------------------------------------

MidiCtrlValListList::MidiCtrlValListList()
{
  _RPN_Ctrls_Reserved = false;
}

// TODO: Finish copy constructor, but first MidiCtrlValList might need one too ?
// MidiCtrlValListList::MidiCtrlValListList(const MidiCtrlValListList& mcvl) : std::map<int, MidiCtrlValList*>()
// {
//   for(ciMidiCtrlValList i = mcvl.begin(); i != mcvl.end(); ++i)
//   {
//     MidiCtrlValList* mcl = i->second;
//     add(new MidiCtrlValList(*mcl));
//   }  
//   update_RPN_Ctrls_Reserved();
// }

void MidiCtrlValListList::add(int channel, MidiCtrlValList* vl, bool update) 
{
  // TODO: If per-channel instruments are ever added to MusE, this routine would need changing.

  const int num = vl->num();
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
  insert(std::pair<const int, MidiCtrlValList*>((channel << 24) + num, vl));
}

void MidiCtrlValListList::del(iMidiCtrlValList ictl, bool update) 
{ 
  erase(ictl); 
  if(update)
    update_RPN_Ctrls_Reserved();
}

MidiCtrlValListList::size_type MidiCtrlValListList::del(int num, bool update) 
{ 
  MidiCtrlValListList::size_type res = erase(num);
  if(update)
    update_RPN_Ctrls_Reserved();
  return res;
}

void MidiCtrlValListList::del(iMidiCtrlValList first, iMidiCtrlValList last, bool update) 
{ 
  erase(first, last); 
  if(update)
    update_RPN_Ctrls_Reserved();
}

void MidiCtrlValListList::clr() 
{ 
  clear(); 
  update_RPN_Ctrls_Reserved();
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
    clr();
}

//---------------------------------------------------------
// searchControllers
//---------------------------------------------------------

iMidiCtrlValList MidiCtrlValListList::searchControllers(int channel, int ctl)
{
  const int type = ctl & CTRL_OFFSET_MASK;
  const unsigned ch_bits = (channel << 24);
  int n;
  
  // Looking for Controller7? See if any Controller14 contains the number and should be used instead.
  if(type == CTRL_7_OFFSET)
  {
    const int num = ctl & 0xff;
    for(iMidiCtrlValList imc = lower_bound(ch_bits | CTRL_14_OFFSET); imc != end(); ++imc)
    {
      // There is ->second->num(), but this is only way to get channel.
      n = imc->first; 
      // Stop if we went beyond this channel number or this ctrl14 block. 
      if((n & 0xff000000) != ch_bits || (n & CTRL_OFFSET_MASK) != CTRL_14_OFFSET)
        break;
      if(((n >> 8) & 0xff) == num || (n & 0xff) == num)
        return imc;
    }
  }
  // Looking for RPN? See if any RPN14 also uses the number and should be used instead.
  else if (type == CTRL_RPN_OFFSET)
  {
    const int num = ctl & 0xffff;
    for(iMidiCtrlValList imc = lower_bound(ch_bits | CTRL_RPN14_OFFSET); imc != end(); ++imc)
    {
      // There is ->second->num(), but this is only way to get channel.
      n = imc->first; 
      // Stop if we went beyond this channel number or this RPN14 block. 
      if((n & 0xff000000) != ch_bits || (n & CTRL_OFFSET_MASK) != CTRL_RPN14_OFFSET)
        break;
      if((n & 0xffff) == num)
        return imc;
    }
  }
  // Looking for NRPN? See if any NRPN14 also uses the number and should be used instead.
  else if (type == CTRL_NRPN_OFFSET)
  {
    const int num = ctl & 0xffff;
    for(iMidiCtrlValList imc = lower_bound(ch_bits | CTRL_NRPN14_OFFSET); imc != end(); ++imc)
    {
      // There is ->second->num(), but this is only way to get channel.
      n = imc->first; 
      // Stop if we went beyond this channel number or this NRPN14 block. 
      if((n & 0xff000000) != ch_bits || (n & CTRL_OFFSET_MASK) != CTRL_NRPN14_OFFSET)
        break;
      if((n & 0xffff) == num)
        return imc;
    }
  }
  
  // Looking for any other type? Do a regular find.
  return std::map<int, MidiCtrlValList*, std::less<int> >::find(ch_bits | ctl);
}

//---------------------------------------------------------
//   update_RPN_Ctrls_Reserved
//   Manual check and update of the flag. For convenience, returns the flag.
//   Cost depends on types and number of list controllers, so it is good for deferring 
//    an update until AFTER some lengthy list operation. JUST BE SURE to call this!
//---------------------------------------------------------

bool MidiCtrlValListList::update_RPN_Ctrls_Reserved()
{
  // TODO: If per-channel instruments are ever added to MusE, this routine would need changing.
  
  int num, h, l;
  for(int ch = 0; ch < MIDI_CHANNELS; ++ch)
  {
    const unsigned ch_bits = (ch << 24);
    
    if(find(ch, CTRL_HDATA) != end() ||
      find(ch, CTRL_LDATA) != end() ||
      find(ch, CTRL_DATA_INC) != end() ||
      find(ch, CTRL_DATA_DEC) != end() ||
      find(ch, CTRL_HNRPN) != end() ||
      find(ch, CTRL_LNRPN) != end() ||
      find(ch, CTRL_HRPN) != end() ||
      find(ch, CTRL_LRPN) != end())
    {
      _RPN_Ctrls_Reserved = true;
      return true;
    }
  
    // Search: Get a head-start by taking lower bound.
    for(iMidiCtrlValList imc = lower_bound(ch_bits | CTRL_14_OFFSET); imc != end(); ++imc)
    {
      // There is ->second->num(), but this is only way to get channel.
      num = imc->first; 
      // Stop if we went beyond this channel number or its ctrl14 block. 
      if((num & 0xff000000) != ch_bits || (num & CTRL_OFFSET_MASK) != CTRL_14_OFFSET)
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
  }
  
  _RPN_Ctrls_Reserved = false;
  return false;
}

//---------------------------------------------------------
//     Catch all insert, erase, clear etc.
//---------------------------------------------------------

MidiCtrlValListList& MidiCtrlValListList::operator=(const MidiCtrlValListList& cl)
{
#ifdef _MIDI_CTRL_DEBUG_
  printf("MidiCtrlValListList::operator=\n");  
#endif
  _RPN_Ctrls_Reserved = cl._RPN_Ctrls_Reserved;
  
  // Let map copy the items.
  std::map<int, MidiCtrlValList*, std::less<int> >::operator=(cl);
  return *this;
}

//=========================================================
#ifdef _MIDI_CTRL_METHODS_DEBUG_      

void MidiCtrlValListList::swap(MidiCtrlValListList& cl)
{
#ifdef _MIDI_CTRL_DEBUG_
  printf("MidiCtrlValListList::swap\n");  
#endif
  std::map<int, MidiCtrlValList*, std::less<int> >::swap(cl);
}

std::pair<iMidiCtrlValList, bool> MidiCtrlValListList::insert(const std::pair<int, MidiCtrlValList*>& p)
{
#ifdef _MIDI_CTRL_DEBUG_
  printf("MidiCtrlValListList::insert num:%d\n", p.second->num());  
#endif
  std::pair<iMidiCtrlValList, bool> res = std::map<int, MidiCtrlValList*, std::less<int> >::insert(p);
  return res;
}

iMidiCtrlValList MidiCtrlValListList::insert(iMidiCtrlValList ic, const std::pair<int, MidiCtrlValList*>& p)
{
#ifdef _MIDI_CTRL_DEBUG_
  printf("MidiCtrlValListList::insertAt num:%d\n", p.second->num()); 
#endif
  iMidiCtrlValList res = std::map<int, MidiCtrlValList*, std::less<int> >::insert(ic, p);
  return res;
}

void MidiCtrlValListList::erase(iMidiCtrlValList ictl)
{
#ifdef _MIDI_CTRL_DEBUG_
  printf("MidiCtrlValListList::erase iMidiCtrlValList num:%d\n", ictl->second->num());  
#endif
  std::map<int, MidiCtrlValList*, std::less<int> >::erase(ictl);
}

MidiCtrlValListList::size_type MidiCtrlValListList::erase(int num)
{
#ifdef _MIDI_CTRL_DEBUG_
  printf("MidiCtrlValListList::erase num:%d\n", num);  
#endif
  size_type res = std::map<int, MidiCtrlValList*, std::less<int> >::erase(num);
  return res;
}

void MidiCtrlValListList::erase(iMidiCtrlValList first, iMidiCtrlValList last)
{
#ifdef _MIDI_CTRL_DEBUG_
  printf("MidiCtrlValListList::erase range first num:%d second num:%d\n", 
         first->second->num(), last->second->num());  
#endif
  std::map<int, MidiCtrlValList*, std::less<int> >::erase(first, last);
}

void MidiCtrlValListList::clear()
{
#ifdef _MIDI_CTRL_DEBUG_
  printf("MidiCtrlValListList::clear\n");  
#endif
  std::map<int, MidiCtrlValList*, std::less<int> >::clear();
}

#endif
// =========================================================

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
            
      insert(std::pair<const int, MidiCtrlVal> (tick, MidiCtrlVal(part, val)));
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

void MidiControllerList::add(MidiController* mc, bool update) 
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
  insert(std::pair<int, MidiController*>(num, mc)); 
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

//---------------------------------------------------------
//   MidiCtrlState
//---------------------------------------------------------

MidiCtrlState::MidiCtrlState()
{
  ctrls  = new unsigned char[128];
  RPNH   = new unsigned char[16384];
  RPNL   = new unsigned char[16384];
  NRPNH  = new unsigned char[16384];
  NRPNL  = new unsigned char[16384];

  //data = new char[128 + 16384 + 16384 + 16384 + 16384];

  init();
}

MidiCtrlState::~MidiCtrlState()
{
  delete ctrls;
  delete RPNH;
  delete RPNL;
  delete NRPNH;
  delete NRPNL;

  //delete data;
}

void MidiCtrlState::init()
{
  memset(ctrls, 0, sizeof(unsigned char) * 128);
  memset(RPNH,  0, sizeof(unsigned char) * 16384);
  memset(RPNL,  0, sizeof(unsigned char) * 16384);
  memset(NRPNH, 0, sizeof(unsigned char) * 16384);
  memset(NRPNL, 0, sizeof(unsigned char) * 16384);
  modeIsNRP = false;
}

//---------------------------------------------------------
//   MidiEncoder
//---------------------------------------------------------

MidiEncoder::MidiEncoder()
{
  _curMode  = EncIdle;
  // Try starting with ParamModeUnknown. Requires either RPN or NRPN params at least once.
  // Possibly may want to start with ParamModeRPN or ParamModeNRPN,
  //  possibly make it depend on planned all-encompassing 'Optimizations' Setting,
  //  and provide reset with 'Panic' button, just as it now resets output optimizations.
  _curParamMode = ParamModeUnknown; 
  _curData  = 0;
  _curTime  = 0; 
  _timer    = 0;
  _curCtrl  = 0;
  _nextCtrl = 0;
  _curRPNH  = 0;
  _curRPNL  = 0;
  _curNRPNL = 0;
  _curNRPNH = 0;
}

//---------------------------------------------------------
//   encodeEvent
//---------------------------------------------------------

void MidiEncoder::encodeEvent(const MidiRecordEvent& ev, int port, int channel)
{
  const int type = ev.type();
  if(type != ME_PITCHBEND || type != ME_AFTERTOUCH || type != ME_POLYAFTER || type != ME_PROGRAM || type != ME_CONTROLLER)
    return;

  MidiPort* mp = &MusEGlobal::midiPorts[port];

  MidiCtrlValListList* mcvll = mp->controller();
  MidiInstrument*      instr = mp->instrument();
  MidiControllerList*    mcl = instr->controller();

  int num;
  int data;
  //int ctrlH;
  //int ctrlL;
  //const int ch_bits = channel << 24;

  if(_curMode != EncIdle)
  {
    if(_curMode == EncCtrl14)
      num = CTRL_14_OFFSET + ((_curCtrl << 8) | _nextCtrl);
    else if(_curMode == EncRPN14)
      num = CTRL_RPN14_OFFSET + ((_curRPNH << 8) | _curRPNL);
    else if(_curMode == EncNRPN14)
      num = CTRL_NRPN14_OFFSET + ((_curNRPNH << 8) | _curNRPNL);
    else
    {
      // Error
      _curMode = EncIdle;
      return;
    }

    iMidiCtrlValList imcvl = mcvll->find(channel, num);
    if(imcvl == mcvll->end())
    {
      // Error, controller should exist
      _curMode = EncIdle;
      return;                  
    }
    MidiCtrlValList* mcvl = imcvl->second;

    if(type == ME_CONTROLLER && ev.dataA() == _nextCtrl)
    {
      data = (_curData << 7) | (ev.dataB() & 0x7f);
      mcvl->setHwVal(data);
      _curMode = EncIdle;
      return;
    }
    else
    {
      data = (_curData << 7) | (mcvl->hwVal() & 0x7f);
      mcvl->setHwVal(data);
      //_curMode = EncIdle;
      //return;
    }

    //return;
  }

  if(type == ME_CONTROLLER)
  {
    num = ev.dataA();
    const int val = ev.dataB();
    // Is it one of the 8 reserved GM controllers for RPN/NRPN?
    if(num == CTRL_HDATA || num == CTRL_LDATA || num == CTRL_DATA_INC || num == CTRL_DATA_DEC ||
       num == CTRL_HNRPN || num == CTRL_LNRPN || num == CTRL_HRPN || num == CTRL_LRPN)
    {
      // Does the working controller list, and instrument, allow RPN/NRPN?
      // (If EITHER the working controller list or instrument defines ANY of these
      //  8 controllers as plain 7-bit, then we cannot accept this data as RPN/NRPN.)
      const bool rpn_reserved = mcvll->RPN_Ctrls_Reserved() | mcl->RPN_Ctrls_Reserved();
      if(!rpn_reserved)
      {
        switch(num)
        {
          case CTRL_HDATA:
          {
            _curData = val;
            switch(_curParamMode)
            {
              case ParamModeUnknown:
                return;              // Sorry, we shouldn't accept it without valid (N)RPN numbers.
              case ParamModeRPN:
              {
                const int param_num = (_curRPNH << 8) | _curRPNL; 
                iMidiCtrlValList imcvl = mcvll->searchControllers(channel, CTRL_RPN_OFFSET | param_num);
                if(imcvl == mcvll->end())
                {
                  // Set mode, _curTime, and _timer to wait for next event.
                  _curMode = EncDiscoverRPN;   
                  _curTime = MusEGlobal::audio->curFrame();  
                  _timer = 0;            
                  return;
                }
                else if((imcvl->first & CTRL_OFFSET_MASK) == CTRL_RPN_OFFSET)
                {
                  // Done. Take _curData and param_num and compose something to return,
                  //  and set the HWval...  TODO
                  _curMode = EncIdle;   
                  return;
                }
                else if((imcvl->first & CTRL_OFFSET_MASK) == CTRL_RPN14_OFFSET)
                {
                  // Set mode, _curTime, and _timer to wait for next event.
                  _curMode = EncRPN14;   
                  _curTime = MusEGlobal::audio->curFrame();  
                  _timer = 0;            
                  return;
                }
                else
                {
                  fprintf(stderr, "MidiEncoder::encodeEvent unknown type %d\n", imcvl->first & CTRL_OFFSET_MASK);
                  return;
                }
                
                break;  
              }
              case ParamModeNRPN:
                break;  
              default:
                fprintf(stderr, "MidiEncoder::encodeEvent unknown ParamMode %d\n", _curParamMode);
                return;
            }
            
            break;
          }

          case CTRL_LDATA:
            break;
          case CTRL_DATA_INC: 
            break;
          case CTRL_DATA_DEC:
            break;
          case CTRL_HRPN:
            _curRPNH = val;
            _curParamMode = ParamModeRPN;
            return;
          case CTRL_LRPN:
            _curRPNL = val;
            _curParamMode = ParamModeRPN;
            return;
          case CTRL_HNRPN:
            _curNRPNH = val;
            _curParamMode = ParamModeNRPN;
            return;
          case CTRL_LNRPN:
            _curNRPNL = val;
            _curParamMode = ParamModeNRPN;
            return;
          default:
            break;  
        }
      }
    }
    
//     for(iMidiCtrlValList imcvl = mcvll->begin(); imcvl != mcvll->end(); ++imcvl)
//     {
//       if(((imcvl->first >> 24) & 0xf) != channel)
//         continue;
//       MidiCtrlValList* mcvl = imcvl->second;
//     }
  }
  
  _curMode = EncIdle;
  return;

  
//   if(_curMode != EncIdle)
//   {
//     if(_curMode == EncCtrl14 && type == ME_CONTROLLER && ev.dataA() == _nextCtrl)
//     {
//       num = CTRL_14_OFFSET + ((_curCtrl << 8) | (_nextCtrl & 0x7f));
//       iMidiCtrlValList imcvl = mcvll->find(channel, num);
//       if(imcvl == mcvll->end())
//         return;                  // Error, controller should exist
//       MidiCtrlValList* mcvl = imcvl->second;
//       data = (_curData << 7) | (ev.dataB() & 0x7f);
//       mcvl->setHwVal(data);
//       //_timer = 0;
//       _curMode = EncIdle;
//       return;
//     }
//     else if(_curMode == EncRPN14 && type == ME_CONTROLLER && ev.dataA() == CTRL_LDATA)
//     {
// 
//     }
//   }
  
  //mcvl->find();

//   for(ciMidiCtrlValList imcvl = mcvl->begin(); imcvl != mcvl->end(); ++imcvl)
//   {
//     const int ch = imcvl->first >> 24;
//     if(ch != channel)
//       continue; 
//     MidiCtrlValList* mcvl = imcvl->second;
//     num = mcvl->num();
//     ctrlH = (num >> 8) & 0x7f;
//     ctrlL = num & 0xff;
//     if(type == ME_CONTROLLER)
//     {
//       const int ev_num = ev.dataA();
//       if(num < CTRL_14_OFFSET)           // 7-bit controller  0 - 0x10000
//       {
//         if(ev_num == num)
//         {
// 
//         }
//       }
//       else if(num < CTRL_RPN_OFFSET)     // 14-bit controller 0x10000 - 0x20000
//       {
// 
//       }
//     }
//   }

  
//   int num;
// 
//   switch(type)
//   {
//     // TODO
//     case ME_PITCHBEND:
//       num = CTRL_PITCH;
//     break;
//     case ME_AFTERTOUCH:
//       num = CTRL_AFTERTOUCH;
//     break;
//     case ME_POLYAFTER:
//       num = CTRL_POLYAFTER | (ev.dataA() & 0x7f);
//     break;
//     case ME_PROGRAM:
//       num = CTRL_PROGRAM;
//     break;
// 
//     case ME_CONTROLLER:
//     {
//       //num = CTRL_;
//     }
//     break;
//     
//     default:
//       return;
//   }

  
  
//   if(instr)
//   {
//     int num;
//     int ctrlH;
//     int ctrlL;
//     MidiControllerList* mcl = instr->controller();
//     MidiController* mc;
// 
//     if (_outputInstrument) {
//           MidiControllerList* mcl = _outputInstrument->controller();
//           for (iMidiController i = mcl->begin(); i != mcl->end(); ++i) {
//                 int cn = i->second->num();
//                 if (cn == num)
//                       return i->second;
//                 // wildcard?
//                 if (i->second->isPerNoteController() && ((cn & ~0xff) == (num & ~0xff)))
//                       return i->second;
//                 }
//           }
// 
//     for (iMidiController i = defaultMidiController.begin(); i != defaultMidiController.end(); ++i) {
//           int cn = i->second->num();
//           if (cn == num)
//                 return i->second;
//           // wildcard?
//           if (i->second->isPerNoteController() && ((cn & ~0xff) == (num & ~0xff)))
//                 return i->second;
//           }
// 
// 
//     QString name = midiCtrlName(num);
//     int min = 0;
//     int max = 127;
// 
//     MidiController::ControllerType t = midiControllerType(num);
//     switch (t) {
//           case MidiController::RPN:
//           case MidiController::NRPN:
//           case MidiController::Controller7:
//           case MidiController::PolyAftertouch:
//           case MidiController::Aftertouch:
//                 max = 127;
//                 break;
//           case MidiController::Controller14:
//           case MidiController::RPN14:
//           case MidiController::NRPN14:
//                 max = 16383;
//                 break;
//           case MidiController::Program:
//                 max = 0xffffff;
//                 break;
//           case MidiController::Pitch:
//                 max = 8191;
//                 min = -8192;
//                 break;
//           case MidiController::Velo:        // cannot happen
//                 break;
//           }
//     MidiController* c = new MidiController(name, num, min, max, 0);
//     defaultMidiController.add(c);
//     return c;
// 
//     for(ciMidiController imc = mcl->begin(); imc != mcl->end(); ++imc)
//     {
//       mc = imc->second;
//       num = mc->num();
//       ctrlH = (num >> 8) & 0x7f;
//       ctrlL = num & 0xff;
//       if(num < CTRL_14_OFFSET)           // 7-bit controller  0 - 0x10000
//       {
//         if(ctrlL == 0xff || ctrlL == a)
//           is_7bit = true;
// 
//         if(ctrlL == 0xff)
//           RPNH_reserved = RPNL_reserved = NRPNH_reserved = NRPNL_reserved = DATAH_reserved = DATAL_reserved = true;
//         else if(ctrlL == CTRL_HRPN)
//           RPNH_reserved = true;
//         else if(ctrlL == CTRL_LRPN)
//           RPNL_reserved = true;
//         else if(ctrlL == CTRL_HNRPN)
//           NRPNH_reserved = true;
//         else if(ctrlL == CTRL_LNRPN)
//           NRPNL_reserved = true;
//         else if(ctrlL == CTRL_HDATA)
//           DATAH_reserved = true;
//         else if(ctrlL == CTRL_LDATA)
//           DATAL_reserved = true;
//       }
//       else if(num < CTRL_RPN_OFFSET)     // 14-bit controller 0x10000 - 0x20000
//       {
//         if(ctrlH == a)
//         {
//           //is_14bitH = true;
//           is_14bit = true;
//           if(!instr->waitForLSB())
//           {
//             MidiRecordEvent single_ev;
//             single_ev.setChannel(chn);
//             single_ev.setType(ME_CONTROLLER);
//             single_ev.setA(CTRL_14_OFFSET + (a << 8) + ctrlL);
//             single_ev.setB((b << 7) + mcs->ctrls[ctrlL]);
//             mdev->recordEvent(single_ev);
//           }
//         }
//         if(ctrlL == 0xff || ctrlL == a)
//         {
//           //is_14bitL = true;
//           is_14bit = true;
//           MidiRecordEvent single_ev;
//           single_ev.setChannel(chn);
//           single_ev.setType(ME_CONTROLLER);
//           single_ev.setA(CTRL_14_OFFSET + (ctrlH << 8) + a);
//           single_ev.setB((mcs->ctrls[ctrlH] << 7) + b);
//           mdev->recordEvent(single_ev);
//         }
// 
//         if(ctrlL == 0xff)
//           RPNH_reserved = RPNL_reserved = NRPNH_reserved = NRPNL_reserved = DATAH_reserved = DATAL_reserved = true;
//         else if(ctrlL == CTRL_HRPN || ctrlH == CTRL_HRPN)
//           RPNH_reserved = true;
//         else if(ctrlL == CTRL_LRPN || ctrlH == CTRL_LRPN)
//           RPNL_reserved = true;
//         else if(ctrlL == CTRL_HNRPN || ctrlH == CTRL_HNRPN)
//           NRPNH_reserved = true;
//         else if(ctrlL == CTRL_LNRPN || ctrlH == CTRL_LNRPN)
//           NRPNL_reserved = true;
//         else if(ctrlL == CTRL_HDATA || ctrlH == CTRL_HDATA)
//           DATAH_reserved = true;
//         else if(ctrlL == CTRL_LDATA || ctrlH == CTRL_LDATA)
//           DATAL_reserved = true;
//       }
//       else if(num < CTRL_NRPN_OFFSET)     // RPN 7-Bit Controller 0x20000 - 0x30000
//       {
//         //if(a == CTRL_HDATA && mcs->ctrls[CTRL_HRPN] < 128 && mcs->ctrls[CTRL_LRPN] < 128)
//         if(a == CTRL_HDATA && !mcs->modeIsNRP && ctrlH == mcs->ctrls[CTRL_HRPN] && (ctrlL == 0xff || ctrlL == mcs->ctrls[CTRL_LRPN]))
//           is_RPN = true;
//       }
//       else if(num < CTRL_INTERNAL_OFFSET) // NRPN 7-Bit Controller 0x30000 - 0x40000
//       {
//         //if(a == CTRL_HDATA && mcs->ctrls[CTRL_HNRPN] < 128 && mcs->ctrls[CTRL_LNRPN] < 128)
//         if(a == CTRL_HDATA && mcs->modeIsNRP && ctrlH == mcs->ctrls[CTRL_HNRPN] && (ctrlL == 0xff || ctrlL == mcs->ctrls[CTRL_LNRPN]))
//           is_NRPN = true;
//       }
//       else if(num < CTRL_RPN14_OFFSET)    // Unaccounted for internal controller  0x40000 - 0x50000
//           continue;
//       else if(num < CTRL_NRPN14_OFFSET)   // RPN14 Controller  0x50000 - 0x60000
//       {
//         //if(a == CTRL_LDATA && mcs->ctrls[CTRL_HRPN] < 128 && mcs->ctrls[CTRL_LRPN] < 128)
//         if(a == CTRL_LDATA && !mcs->modeIsNRP && ctrlH == mcs->ctrls[CTRL_HRPN] && (ctrlL == 0xff || ctrlL == mcs->ctrls[CTRL_LRPN]))
//           is_RPN14 = true;
//       }
//       else if(num < CTRL_NONE_OFFSET)     // NRPN14 Controller 0x60000 - 0x70000
//       {
//         //if(a == CTRL_LDATA && mcs->ctrls[CTRL_HNRPN] < 128 && mcs->ctrls[CTRL_LNRPN] < 128)
//         if(a == CTRL_LDATA && mcs->modeIsNRP && ctrlH == mcs->ctrls[CTRL_HNRPN] && (ctrlL == 0xff || ctrlL == mcs->ctrls[CTRL_LNRPN]))
//           is_NRPN14 = true;
//       }
//     }
//   }

}

//---------------------------------------------------------
//   endCycle
//---------------------------------------------------------

void MidiEncoder::endCycle(unsigned int /*blockSize*/)
{
  // TODO
}

} // namespace MusECore
