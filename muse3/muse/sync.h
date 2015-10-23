//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: sync.h,v 1.1.1.1.2.2 2009/04/01 01:37:11 terminator356 Exp $
//
//  (C) Copyright 2003 Werner Schweer (ws@seh.de)
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

#ifndef __SYNC_H__
#define __SYNC_H__

#include "mtc.h"
#include "value.h"
#include "globaldefs.h"

namespace MusECore {

class Xml;

class MidiSyncInfo
{
  public:
    enum SyncRecFilterPresetType { NONE=0, TINY, SMALL, MEDIUM, LARGE, LARGE_WITH_PRE_DETECT, TYPE_END };
    
  private:
    int _port;
    
    int _idOut;
    int _idIn;
    
    bool _sendMC;
    bool _sendMRT;
    bool _sendMMC;
    bool _sendMTC;
    bool _recMC;
    bool _recMRT;
    bool _recMMC;
    bool _recMTC;
    
    int _recMTCtype;
    
    bool _recRewOnStart;
    
    double   _lastClkTime;
    double   _lastTickTime;
    double   _lastMRTTime;
    double   _lastMMCTime;
    double   _lastMTCTime;
    double   _lastActTime[MIDI_CHANNELS];
    bool     _clockTrig;
    bool     _tickTrig;
    bool     _MRTTrig;
    bool     _MMCTrig;
    bool     _MTCTrig;
    bool     _actTrig[MIDI_CHANNELS];
    bool     _clockDetect;
    bool     _tickDetect;
    bool     _MRTDetect;
    bool     _MMCDetect;
    bool     _MTCDetect;
    bool     _actDetect[MIDI_CHANNELS];
    int      _actDetectBits;
    
  public:
    MidiSyncInfo();
    MidiSyncInfo& operator= (const MidiSyncInfo &sp);
    MidiSyncInfo& copyParams(const MidiSyncInfo &sp);
    
    int port() const              { return _port; }
    void setPort(const int p)     { _port = p; }
    
    int idOut() const             { return _idOut; }
    int idIn() const              { return _idIn; }
    void setIdOut(const int v)    { _idOut = v; }
    void setIdIn(const int v)     { _idIn = v; }
    
    bool MCOut() const            { return _sendMC; }
    bool MRTOut() const           { return _sendMRT; }
    bool MMCOut() const           { return _sendMMC; }
    bool MTCOut() const           { return _sendMTC; }
    
    bool MCIn() const             { return _recMC; }
    bool MRTIn() const            { return _recMRT; }
    bool MMCIn() const            { return _recMMC; }
    bool MTCIn() const            { return _recMTC; }
    
    void setMCOut(const bool v)   { _sendMC = v; }
    void setMRTOut(const bool v)  { _sendMRT = v; }
    void setMMCOut(const bool v)  { _sendMMC = v; }
    void setMTCOut(const bool v)  { _sendMTC = v; }
    
    void setMCIn(const bool v);   
    void setMRTIn(const bool v);   
    void setMMCIn(const bool v);   
    void setMTCIn(const bool v);   
    
    void setTime(); 
    
    bool recRewOnStart() const            { return _recRewOnStart; }
    void setRecRewOnStart(const bool v)   { _recRewOnStart = v; }
    
    bool MCSyncDetect() const             { return _clockDetect; }          
    void trigMCSyncDetect();
    
    bool tickDetect() const       { return _tickDetect; }           
    void trigTickDetect();
    
    bool MTCDetect() const       { return _MTCDetect; }           
    void trigMTCDetect();
    int recMTCtype() const       { return _recMTCtype; }
    void setRecMTCtype(int t)    { _recMTCtype = t; }
    
    bool MRTDetect() const       { return _MRTDetect; }           
    void trigMRTDetect();
    
    bool MMCDetect() const       { return _MMCDetect; }           
    void trigMMCDetect();
    
    int  actDetectBits() const    { return _actDetectBits; }
    bool actDetect(const int ch) const;
    void trigActDetect(const int ch);
    
    bool isDefault() const;
    void read(Xml& xml);
    void write(int level, Xml& xml);
};

} // namespace MusECore

namespace MusEGlobal {

extern bool debugSync;

extern int mtcType;
extern MusECore::MTC mtcOffset;
extern MusECore::BValue extSyncFlag;
extern int volatile curMidiSyncInPort;
extern MusECore::BValue useJackTransport;
extern bool volatile jackTransportMaster;
extern unsigned int syncSendFirstClockDelay; // In milliseconds.
extern unsigned int volatile lastExtMidiSyncTick;
extern MusECore::MidiSyncInfo::SyncRecFilterPresetType syncRecFilterPreset;
extern double syncRecTempoValQuant;

} // namespace MusEGlobal

#endif

