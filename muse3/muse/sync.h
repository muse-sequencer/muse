//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: sync.h,v 1.1.1.1.2.2 2009/04/01 01:37:11 terminator356 Exp $
//
//  (C) Copyright 2003 Werner Schweer (ws@seh.de)
//  (C) Copyright 2016 Tim E. Real (terminator356 on sourceforge.net)
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

#include <stdint.h>

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

    uint64_t _lastClkTime;
    uint64_t _lastTickTime;
    uint64_t _lastMRTTime;
    uint64_t _lastMMCTime;
    uint64_t _lastMTCTime;
    uint64_t _lastActTime[MusECore::MUSE_MIDI_CHANNELS];
    
    bool     _clockTrig;
    bool     _tickTrig;
    bool     _MRTTrig;
    bool     _MMCTrig;
    bool     _MTCTrig;
    bool     _actTrig[MusECore::MUSE_MIDI_CHANNELS];
    bool     _clockDetect;
    bool     _tickDetect;
    bool     _MRTDetect;
    bool     _MMCDetect;
    bool     _MTCDetect;
    bool     _actDetect[MusECore::MUSE_MIDI_CHANNELS];
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


//---------------------------------------------------------
//   ExtMidiClock
//   Holds the frame of each external clock, 
//    and play state at that time.
//---------------------------------------------------------

class ExtMidiClock
{
  public:
    enum ExternState { ExternStopped = 0, ExternStarting, ExternContinuing, ExternStarted, ExternContinued };
    
  private:
    // The frame at which this clock arrived.
    unsigned int _frame;
    // The play state of the external device when this clock arrived.
    ExternState _externState;
    // Whether this clock is the first clock after a start or continue.
    bool _isFirstClock;
    // Whether this is a valid structure.
    bool _isValid;
    
  public:
    ExtMidiClock() : _frame(0), _externState(ExternStopped), _isFirstClock(false), _isValid(false) { };
    ExtMidiClock(unsigned int frame, ExternState extState, bool firstClock) : 
                 _frame(frame), _externState(extState), _isFirstClock(firstClock), _isValid(true) { };
    
    // The frame at which this clock arrived.
    unsigned int frame() const { return _frame; }
    // The play state of the external device when this clock arrived.
    ExternState externState() const { return _externState; }
    // Whether this clock is the first clock after a start or continue.
    bool isFirstClock() const { return _isFirstClock; }
    // Whether this is a valid structure.
    bool isValid() const { return _isValid; }
    bool isPlaying() const
    {
      switch(_externState)
      {
        case ExternStopped:
        case ExternStarting:
        case ExternContinuing:
          return false;
        break;
        
        case ExternStarted:
        case ExternContinued:
          return true;
        break;
      };
      return false;
    }
    bool isRunning() const
    {
      switch(_externState)
      {
        case ExternStopped:
          return false;
        break;
        
        case ExternStarting:
        case ExternContinuing:
        case ExternStarted:
        case ExternContinued:
          return true;
        break;
      };
      return false;
    }
};

//---------------------------------------------------------
//   MidiSyncContainer
//---------------------------------------------------------

class MidiSyncContainer {
  private:
// REMOVE Tim. clock. Removed.  
      unsigned int _midiClock; // Accumulator for clock output.

/* Testing */
      ExtMidiClock::ExternState playStateExt;   // used for keeping play state in sync functions
      int recTick;            // ext sync tick position
      double mclock1, mclock2;
      double songtick1, songtick2;
      int recTick1, recTick2;
      int lastTempo;
      double timediff[16][48];
      int storedtimediffs;
      int    _avgClkDiffCounter[16];
      double _lastRealTempo;
      bool _averagerFull[16];
      int _clockAveragerPoles;
      int* _clockAveragerStages;
      bool _preDetect;
      double _tempoQuantizeAmount;
      MidiSyncInfo::SyncRecFilterPresetType _syncRecFilterPreset;

      void setSyncRecFilterPresetArrays();
      void alignAllTicks(int frameOverride = 0);
/* Testing */

      void mtcSyncMsg(const MTC&, int, bool);

   public:
      MidiSyncContainer();
      virtual ~MidiSyncContainer();

// REMOVE Tim. clock. Removed.  
      unsigned int midiClock() const { return _midiClock; }
      void setMidiClock(unsigned int val) { _midiClock = val; }
      ExtMidiClock::ExternState externalPlayState() const { return playStateExt; }
      void setExternalPlayState(ExtMidiClock::ExternState v) { playStateExt = v; }
      bool isPlaying() const
      {
        switch(playStateExt)
        {
          case ExtMidiClock::ExternStopped:
          case ExtMidiClock::ExternStarting:
          case ExtMidiClock::ExternContinuing:
            return false;
          break;
          
          case ExtMidiClock::ExternStarted:
          case ExtMidiClock::ExternContinued:
            return true;
          break;
        };
        return false;
      }
      bool isRunning() const
      {
        switch(playStateExt)
        {
          case ExtMidiClock::ExternStopped:
            return false;
          break;
          
          case ExtMidiClock::ExternStarting:
          case ExtMidiClock::ExternContinuing:
          case ExtMidiClock::ExternStarted:
          case ExtMidiClock::ExternContinued:
            return true;
          break;
        };
        return false;
      }
      void realtimeSystemInput(int port, int type);
      // Starts transport if necessary. Adds clock to tempo list.
      // Returns a clock structure including frame, state, and whether the clock was a
      //  'first clock' after a start or continue message.
      ExtMidiClock midiClockInput(int port, unsigned int frame); 
      void mtcInputQuarter(int, unsigned char);
      void setSongPosition(int, int);
      void mmcInput(int, const unsigned char*, int);
      void mtcInputFull(int, const unsigned char*, int);
      void nonRealtimeSystemSysex(int, const unsigned char*, int);

      MidiSyncInfo::SyncRecFilterPresetType syncRecFilterPreset() const { return _syncRecFilterPreset; }
      void setSyncRecFilterPreset(MidiSyncInfo::SyncRecFilterPresetType type);
      double recTempoValQuant() const { return _tempoQuantizeAmount; }
      void setRecTempoValQuant(double q) { _tempoQuantizeAmount = q; }
};

} // namespace MusECore

namespace MusEGlobal {

extern bool debugSync;

extern int mtcType;
extern MusECore::MTC mtcOffset;
extern bool extSyncFlag;
extern bool useJackTransport;
extern bool volatile jackTransportMaster;
extern bool transportMasterState;
extern unsigned int syncSendFirstClockDelay; // In milliseconds.
extern unsigned int volatile lastExtMidiSyncTick;
extern unsigned int volatile curExtMidiSyncTick;
extern MusECore::MidiSyncInfo::SyncRecFilterPresetType syncRecFilterPreset;
extern double syncRecTempoValQuant;

extern MusECore::MidiSyncContainer midiSyncContainer;

} // namespace MusEGlobal

#endif

