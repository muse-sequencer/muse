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
#include "muse_math.h"

#include "globaldefs.h"
#include "midictrl.h"
#include "globals.h"
#include "audio.h"
#include "midi_consts.h"
#include "midiport.h"
#include "minstrument.h"
#include "track.h"
#include "mpevent.h"

namespace MusECore {

enum {
      COL_NAME = 0, COL_TYPE,
      COL_HNUM, COL_LNUM, COL_MIN, COL_MAX
      };

MidiControllerList defaultMidiController;
//
// some global controller which are always available:
//
// Zero note on vel is not allowed now.
MidiController veloCtrl("Velocity",               CTRL_VELOCITY,       1,      127,      0,      0);
MidiController pitchCtrl("PitchBend",             CTRL_PITCH,      -8192,    +8191,      0,      0);
MidiController programCtrl("Program",             CTRL_PROGRAM,        0, 0xffffff,      0,      0);
MidiController mastervolCtrl("MasterVolume",      CTRL_MASTER_VOLUME,  0,   0x3fff, 0x3000, 0x3000);
MidiController volumeCtrl("MainVolume",           CTRL_VOLUME,         0,      127,    100,    100);
MidiController panCtrl("Pan",                     CTRL_PANPOT,       -64,       63,      0,      0);
MidiController reverbSendCtrl("ReverbSend",       CTRL_REVERB_SEND,    0,      127,      0,      0);
MidiController chorusSendCtrl("ChorusSend",       CTRL_CHORUS_SEND,    0,      127,      0,      0);
MidiController variationSendCtrl("VariationSend", CTRL_VARIATION_SEND, 0,      127,      0,      0);

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
//   MidiCtrlValList
//---------------------------------------------------------

MidiCtrlValList::MidiCtrlValList(int c)
      {
      ctrlNum = c;
      _hwVal = _lastValidHWVal = _lastValidByte2 = _lastValidByte1 = _lastValidByte0 = CTRL_VAL_UNKNOWN;
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
// resetAllHwVals
//---------------------------------------------------------

bool MidiCtrlValListList::resetAllHwVals(bool doLastHwValue)
{
  bool changed = false;
  for(iMidiCtrlValList imcvl = begin(); imcvl != end(); ++imcvl)
  {
    if(imcvl->second)
    {
      if(imcvl->second->resetHwVal(doLastHwValue))
        changed = true;
    }
  }
  return changed;
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
  for(int ch = 0; ch < MusECore::MUSE_MIDI_CHANNELS; ++ch)
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


bool MidiCtrlValList::resetHwVal(bool doLastHwValue)
{
  bool changed = false;
  if(!hwValIsUnknown())
  {
    _hwVal = CTRL_VAL_UNKNOWN;
    changed = true;
  }
  
  if(doLastHwValue)
  {
    if(!lastHwValIsUnknown())
      changed = true;
    _lastValidHWVal = _lastValidByte2 = _lastValidByte1 = _lastValidByte0 = CTRL_VAL_UNKNOWN;
  }
    
  return changed;
}

//---------------------------------------------------------
//   setHwVal
//   Returns false if value is already equal, true if value is changed.
//---------------------------------------------------------

bool MidiCtrlValList::setHwVal(const double v)
{
  const double r_v = muse_round2micro(v);
  if(_hwVal == r_v)
    return false;

  _hwVal = r_v;

  const int i_val = MidiController::dValToInt(_hwVal);
  if(!MidiController::iValIsUnknown(i_val))
  {
    _lastValidHWVal = _hwVal;
    const int hb = (i_val >> 16) & 0xff;
    const int lb = (i_val >> 8) & 0xff;
    const int pr = i_val & 0xff;
    if(hb >= 0 && hb <= 127)
      _lastValidByte2 = hb;
    if(lb >= 0 && lb <= 127)
      _lastValidByte1 = lb;
    if(pr >= 0 && pr <= 127)
      _lastValidByte0 = pr;
  }

  return true;
}

//---------------------------------------------------------
//   setHwVals
//   Sets current and last HW values.
//   Handy for forcing labels to show 'off' and knobs to show specific values 
//    without having to send two messages.
//   Returns false if both values are already set, true if either value is changed.
//---------------------------------------------------------

bool MidiCtrlValList::setHwVals(const double v, const double lastv)
{
  const double r_v = muse_round2micro(v);
  const double r_lastv = muse_round2micro(lastv);

  if(_hwVal == r_v && _lastValidHWVal == r_lastv)
    return false;

  _hwVal = r_v;
  // Don't want to break our own rules - _lastValidHWVal can't be unknown while _hwVal is valid...
  // But _hwVal can be unknown while _lastValidHWVal is valid...
  if(MidiController::dValIsUnknown(r_lastv))
    _lastValidHWVal = _hwVal;
  else
    _lastValidHWVal = r_lastv;

  const int i_lasthwval = MidiController::dValToInt(_lastValidHWVal);
  if(!MidiController::iValIsUnknown(i_lasthwval))
  {
    const int hb = (i_lasthwval >> 16) & 0xff;
    const int lb = (i_lasthwval >> 8) & 0xff;
    const int pr = i_lasthwval & 0xff;
    if(hb >= 0 && hb <= 127)
      _lastValidByte2 = hb;
    if(lb >= 0 && lb <= 127)
      _lastValidByte1 = lb;
    if(pr >= 0 && pr <= 127)
      _lastValidByte0 = pr;
  }

  return true;
}

//---------------------------------------------------------
//   partAtTick
//---------------------------------------------------------

Part* MidiCtrlValList::partAtTick(unsigned int tick) const
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

iMidiCtrlVal MidiCtrlValList::iValue(unsigned int tick) 
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

int MidiCtrlValList::value(unsigned int tick) const
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

int MidiCtrlValList::value(unsigned int tick, Part* part) const
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

int MidiCtrlValList::visibleValue(unsigned int tick, bool inclMutedParts, bool inclMutedTracks, bool inclOffTracks) const
{
  // Determine value at tick, using values stored by ANY part,
  //  ignoring values that are OUTSIDE of their parts, or muted or off parts or tracks...

  // Get the first value found with with a tick equal or greater than specified tick.
  ciMidiCtrlVal i = lower_bound(tick);
  // Since values from different parts can have the same tick, scan for part in all values at that tick.
  for(ciMidiCtrlVal j = i; j != end() && j->first == tick; ++j)
  {
    const Part* part = j->second.part;
    // Ignore values that are outside of the part.
    if(tick < part->tick() || tick >= (part->tick() + part->lenTick()))
      continue;
    // Ignore if part or track is muted or off.
    if(!inclMutedParts && part->mute())
      continue;
    const Track* track = part->track();
    if(track && ((!inclMutedTracks && track->isMute()) || (!inclOffTracks && track->off())))
      continue;
    return j->second.val;
  }
  // Scan for part in all previous values, regardless of tick.
  while(i != begin())
  {
    --i;
    const Part* part = i->second.part;
    // Ignore values that are outside of the part.
    if(tick < part->tick() || tick >= (part->tick() + part->lenTick()))
      continue;
    // Ignore if part or track is muted or off.
    if(!inclMutedParts && part->mute())
      continue;
    const Track* track = part->track();
    if(track && ((!inclMutedTracks && track->isMute()) || (!inclOffTracks && track->off())))
      continue;
    return i->second.val;
  }
  // No previous values were found belonging to the specified part.
  return CTRL_VAL_UNKNOWN;
}

int MidiCtrlValList::visibleValue(unsigned int tick, Part* part, bool inclMutedParts, bool inclMutedTracks, bool inclOffTracks) const
{
  // Determine value at tick, using values stored by the SPECIFIC part,
  //  ignoring values that are OUTSIDE of the part, or muted or off part or track...

  if((!inclMutedParts && part->mute()) || (part->track() && ((!inclMutedTracks && part->track()->isMute()) || (!inclOffTracks && part->track()->off()))))
    return CTRL_VAL_UNKNOWN;

  // Get the first value found with with a tick equal or greater than specified tick.
  ciMidiCtrlVal i = lower_bound(tick);
  // Ignore if part or track is muted or off.
  // Since values from different parts can have the same tick, scan for part in all values at that tick.
  for(ciMidiCtrlVal j = i; j != end() && j->first == tick; ++j)
  {
    if(j->second.part == part)
    {
      // Ignore values that are outside of the part.
      if(tick < part->tick() || tick >= (part->tick() + part->lenTick()))
        continue;
      return j->second.val;
    }
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
//    return true if new controller value added
//   Accepts duplicate controller items at the same position, to accurately reflect
//    what is really in the event lists.
//---------------------------------------------------------

bool MidiCtrlValList::addMCtlVal(unsigned int tick, int val, Part* part)
      {
      insert(MidiCtrlValListInsertPair_t(tick, MidiCtrlVal(part, val)));
      return true;
      }

//---------------------------------------------------------
//   del
//---------------------------------------------------------

void MidiCtrlValList::delMCtlVal(unsigned int tick, Part* part, int val)
{
      iMidiCtrlVal e = findMCtlVal(tick, part, val);
      if (e == end()) {
	if(MusEGlobal::debugMsg)
              printf("MidiCtrlValList::delMCtlVal(%u): not found (size %zd)\n", tick, size());
            return;
            }
      erase(e);
}

//---------------------------------------------------------
//   find
//---------------------------------------------------------

iMidiCtrlVal MidiCtrlValList::findMCtlVal(unsigned int tick, Part* part, int val)
{
  MidiCtrlValRange range = equal_range(tick);
  for(iMidiCtrlVal i = range.first; i != range.second; ++i) 
  {
    if(i->second.part == part && (val == -1 || i->second.val == val))
      return i;
  }
  return end();
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
  if(type != ME_PITCHBEND && type != ME_AFTERTOUCH && type != ME_POLYAFTER && type != ME_PROGRAM && type != ME_CONTROLLER)
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
