//=========================================================
//  MusE
//  Linux Music Editor
//
//  time_stretch.h
//  Copyright (C) 2016 Tim E. Real (terminator356 on users dot sourceforge dot net)
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

#ifndef __TIME_STRETCH_H__
#define __TIME_STRETCH_H__

// #include <sys/time.h>
//#include <stdint.h>
#include <map>
//#include <vector>

#include "muse_time.h"

// #ifndef MAX_TICK
// #define MAX_TICK (0x7fffffff/100)
// #endif

// #ifndef MAX_FRAME
// //#define MAX_FRAME (0x7ffffffffffffffeL)
// #define MAX_FRAME ((1 << (sizeof(MuseFrame_t) - 1)) - 1)
// #endif

// Tempo ring buffer size
// #define TEMPO_FIFO_SIZE    1024

#define USE_ALTERNATE_STRETCH_LIST

namespace MusECore {

class Xml;
class PendingOperationList;
struct PendingOperationItem;



#ifndef USE_ALTERNATE_STRETCH_LIST

//---------------------------------------------------------
//   StretchEvent
//---------------------------------------------------------

struct StretchEvent {
      double _stretch;
      MuseFrame_t _frame; 
      double _newFrame;  

      MuseFrame_t read(Xml&);
      void write(int, Xml&, MuseFrame_t) const;

      StretchEvent() { }
      StretchEvent(double stretch, MuseFrame_t frame) {
            _stretch = stretch;
            _frame  = frame;
            _newFrame = 0.0;
            }
      };

//---------------------------------------------------------
//   StretchList
//---------------------------------------------------------

typedef std::map<MuseFrame_t, StretchEvent*, std::less<MuseFrame_t> > STRETCHLIST;
typedef STRETCHLIST::iterator iStretchEvent;
typedef STRETCHLIST::const_iterator ciStretchEvent;
typedef STRETCHLIST::reverse_iterator riStretchEvent;
typedef STRETCHLIST::const_reverse_iterator criStretchEvent;

class StretchList : public STRETCHLIST {
    
   friend struct PendingOperationItem;
    
//       int _tempoSN;           // serial no to track tempo changes
//       bool useList;
//       int _tempo;             // tempo if not using tempo list
//       int _globalTempo;       // %percent 50-200%
      // Whether ANY event has a stretch other than 1.0 ie. the map is stretched, a stretcher must be engaged.
      bool _isStretched;

      void add(MuseFrame_t frame, double stretch, bool do_normalize = true);
      void add(MuseFrame_t frame, StretchEvent* e, bool do_normalize = true);
      void del(iStretchEvent, bool do_normalize = true);
      void del(MuseFrame_t frame, bool do_normalize = true);

   public:
      StretchList();
      ~StretchList();
      void normalize();
      void clear();
      void eraseRange(MuseFrame_t sframe, MuseFrame_t eframe);

      void read(Xml&);
      void write(int, Xml&) const;
      void dump() const;

      //int stretch(unsigned tick) const;
      double stretchAt(MuseFrame_t frame) const;
//       unsigned tick2frame(unsigned tick, unsigned frame, int* sn) const;
//       unsigned tick2frame(unsigned tick, int* sn = 0) const;
//       unsigned frame2tick(unsigned frame, int* sn = 0) const;
//       unsigned frame2tick(unsigned frame, unsigned tick, int* sn) const;
//       unsigned deltaTick2frame(unsigned tick1, unsigned tick2, int* sn = 0) const;
//       unsigned deltaFrame2tick(unsigned frame1, unsigned frame2, int* sn = 0) const;
      
      double stretch(MuseFrame_t frame) const;
      double stretch(double frame) const;
      //double unStretch(MuseFrame_t frame) const;
      MuseFrame_t unStretch(double frame) const;
      // Returns whether ANY event has a stretch other than 1.0 ie. the map is stretched, a stretcher must be engaged.
      bool isStretched() const { return _isStretched; }
      
//       int tempoSN() const { return _tempoSN; }
      void setStretch(MuseFrame_t frame, double newStretch);
      void addStretch(MuseFrame_t frame, double stretch, bool do_normalize = true);
      void delStretch(MuseFrame_t frame, bool do_normalize = true);
//       bool masterFlag() const { return useList; }
//       bool setMasterFlag(unsigned tick, bool val);
//       int globalTempo() const           { return _globalTempo; }
//       void setGlobalTempo(int val);
      
      void addOperation(MuseFrame_t frame, double stretch, PendingOperationList& ops); 
      void delOperation(MuseFrame_t frame, PendingOperationList& ops);
      };

#else // USE_ALTERNATE_STRETCH_LIST

//---------------------------------------------------------
//   StretchEvent
//---------------------------------------------------------

struct StretchEvent
{
  // Can be OR'd together.
  enum StretchEventType { StretchEventType = 0x01, SamplerateEventType = 0x02, PitchEventType = 0x04 };
//   // Combination of StretchEventType flags.
  // One of the StretchEventType flags.
  int _type;
  
  double _stretchRatio;
  double _samplerateRatio;
  double _pitchRatio;
  
  // Pre-computed stretch and squish frames.
  double _stretchedFrame;
  double _squishedFrame;

  StretchEvent(double stretchRatio = 1.0,
               double samplerateRatio = 1.0, 
               double pitchRatio = 1.0, 
               int type = 0)
  {
    _stretchRatio = stretchRatio;
    _samplerateRatio = samplerateRatio;
    _pitchRatio = pitchRatio;
    _type = type;
    _stretchedFrame = _squishedFrame = 0.0;
  }
};

//---------------------------------------------------------
//   StretchList
//---------------------------------------------------------

//typedef std::map<MuseFrame_t, StretchEvent, std::less<MuseFrame_t> > STRETCHLIST;
typedef std::multimap<MuseFrame_t, StretchEvent, std::less<MuseFrame_t> > STRETCHLIST;
typedef STRETCHLIST::iterator iStretchEvent;
typedef STRETCHLIST::const_iterator ciStretchEvent;
typedef STRETCHLIST::reverse_iterator riStretchEvent;
typedef STRETCHLIST::const_reverse_iterator criStretchEvent;

typedef std::pair<iStretchEvent, iStretchEvent> iStretchEventPair;
typedef std::pair<ciStretchEvent, ciStretchEvent> ciStretchEventPair;

class StretchList : public STRETCHLIST {
      friend struct PendingOperationItem;
      
      // Intrinsic values.
      MuseFrame_t _startFrame;
      MuseFrame_t _endFrame;
      MuseFrame_t _stretchedEndFrame;
      MuseFrame_t _squishedEndFrame;
      double _stretchRatio;
      double _samplerateRatio;
      double _pitchRatio;
      
      // Whether ANY event has a stretch other than 1.0 ie. the map is stretched, a stretcher must be engaged.
      bool _isStretched;
      bool _isResampled;
      bool _isPitchShifted;

      // Whether the list is already normalized, or else a normalization is required.
      bool _isNormalized;
      
      void addStr(MuseFrame_t frame, double stretch, bool do_normalize = true);
      void addSR(MuseFrame_t frame, double samplerate, bool do_normalize = true);
      void add(MuseFrame_t frame, const StretchEvent& e, bool do_normalize = true);
      void del(iStretchEvent, bool do_normalize = true);
      void delStr(MuseFrame_t frame, bool do_normalize = true);
      void delSR(MuseFrame_t frame, bool do_normalize = true);

   public:
      StretchList();
      virtual ~StretchList();
      
      // Returns whether ANY event has a stretch other than 1.0 ie. the map is stretched, a stretcher must be engaged.
      bool isStretched()    const { return _isStretched; }
      bool isResampled()    const { return _isResampled; }
      bool isPitchShifted() const { return _isPitchShifted; }
      
      void normalizeFrames();
      void normalizeRatios();
      void normalizeListFrames();
      void normalizeListRatios();
      // Whether the list is already normalized, or else a normalization is required.
      bool isNormalized() const { return _isNormalized; }
      
      void clear();
      void eraseStretchRange(MuseFrame_t sframe, MuseFrame_t eframe);
      void eraseSamplerateRange(MuseFrame_t sframe, MuseFrame_t eframe);
      void eraseRange(MuseFrame_t sframe, MuseFrame_t eframe);

      void read(Xml&);
      void write(int, Xml&) const;
      void dump() const;

      // ------------------------------------------
      //  Intrinsic functions:
      //-------------------------------------------
      
      MuseFrame_t startFrame() const { return _startFrame; }
      MuseFrame_t endFrame() const { return _endFrame; }
      MuseFrame_t stretchedEndFrame() const { return _stretchedEndFrame; }
      MuseFrame_t squishedEndFrame() const { return _squishedEndFrame; }
      double stretchRatio() const { return _stretchRatio; }
      double samplerateRatio() const { return _samplerateRatio; }
      double pitchRatio() const { return _pitchRatio; }

      void setStartFrame(MuseFrame_t frame, bool do_normalize = true);
      void setEndFrame(MuseFrame_t frame, bool do_normalize = true);
      void setStretchedEndFrame(MuseFrame_t frame, bool do_normalize = true);
      void setSquishedEndFrame(MuseFrame_t frame, bool do_normalize = true);
      void setStretchRatio(double ratio, bool do_normalize = true);
      void setSamplerateRatio(double ratio, bool do_normalize = true);
      void setPitchRatio(double ratio, bool do_normalize = true);
      
      // ------------------------------------------
      //  List functions:
      //-------------------------------------------
      
      iStretchEvent findEvent(MuseFrame_t frame, int type);
      const_iterator findEvent(const MusECore::MuseFrame_t frame, int type) const;
      
      double stretchAt(MuseFrame_t frame) const;
      double samplerateAt(MuseFrame_t frame) const;
      
      double stretch(MuseFrame_t frame) const;
      double stretch(double frame) const;
      double squish(MuseFrame_t frame) const;
      double squish(double frame) const;
      MuseFrame_t unStretch(double frame) const;
      MuseFrame_t unSquish(double frame) const;
      
      void setStretch(MuseFrame_t frame, double newStretchRatio);
      void setSamplerate(MuseFrame_t frame, double newSamplerateRatio);
      void addStretch(MuseFrame_t frame, double stretch, bool do_normalize = true);
      void addSamplerate(MuseFrame_t frame, double stretch, bool do_normalize = true);
      void delStretch(MuseFrame_t frame, bool do_normalize = true);
      void delSamplerate(MuseFrame_t frame, bool do_normalize = true);
      
      void addStretchOperation(MuseFrame_t frame, double stretch, PendingOperationList& ops); 
      void addSamplerateOperation(MuseFrame_t frame, double samplerate, PendingOperationList& ops); 
      void delStretchOperation(MuseFrame_t frame, PendingOperationList& ops);
      void delSamplerateOperation(MuseFrame_t frame, PendingOperationList& ops);
      };



      
// //---------------------------------------------------------
// //   StretchList
// //---------------------------------------------------------
// 
// //typedef std::map<MuseFrame_t, StretchEvent, std::less<MuseFrame_t> > STRETCHLIST;
// typedef std::map<MuseFrame_t, double, std::less<MuseFrame_t> > STRETCHLIST;
// typedef STRETCHLIST::iterator iStretchEvent;
// typedef STRETCHLIST::const_iterator ciStretchEvent;
// typedef STRETCHLIST::reverse_iterator riStretchEvent;
// typedef STRETCHLIST::const_reverse_iterator criStretchEvent;
// 
// class StretchList : public STRETCHLIST {
//     
//    friend struct PendingOperationItem;
//     
// //       int _tempoSN;           // serial no to track tempo changes
// //       bool useList;
// //       int _tempo;             // tempo if not using tempo list
// //       int _globalTempo;       // %percent 50-200%
//       // Whether ANY event has a stretch other than 1.0 ie. the map is stretched, a stretcher must be engaged.
//       bool _isStretched;
// 
//       void add(MuseFrame_t frame, double stretch);
//       //void add(MuseFrame_t frame, const StretchEvent& e, bool do_normalize = true);
//       void del(iStretchEvent);
//       void del(MuseFrame_t frame);
// 
//    public:
//       StretchList();
//       ~StretchList();
//       //void normalize();
//       void clear();
//       void eraseRange(MuseFrame_t sframe, MuseFrame_t eframe);
// 
//       void read(Xml&);
//       void write(int, Xml&) const;
//       void dump() const;
// 
//       //int stretch(unsigned tick) const;
//       double stretchAt(MuseFrame_t frame) const;
// //       unsigned tick2frame(unsigned tick, unsigned frame, int* sn) const;
// //       unsigned tick2frame(unsigned tick, int* sn = 0) const;
// //       unsigned frame2tick(unsigned frame, int* sn = 0) const;
// //       unsigned frame2tick(unsigned frame, unsigned tick, int* sn) const;
// //       unsigned deltaTick2frame(unsigned tick1, unsigned tick2, int* sn = 0) const;
// //       unsigned deltaFrame2tick(unsigned frame1, unsigned frame2, int* sn = 0) const;
//       
// //       double stretch(MuseFrame_t frame) const;
// //       double stretch(double frame) const;
// //       //double unStretch(MuseFrame_t frame) const;
// //       MuseFrame_t unStretch(double frame) const;
//       // Returns whether ANY event has a stretch other than 1.0 ie. the map is stretched, a stretcher must be engaged.
//       bool isStretched() const { return _isStretched; }
//       
// //       int tempoSN() const { return _tempoSN; }
//       void setStretch(MuseFrame_t frame, double newStretch);
//       void addStretch(MuseFrame_t frame, double stretch);
//       void delStretch(MuseFrame_t frame);
// //       bool masterFlag() const { return useList; }
// //       bool setMasterFlag(unsigned tick, bool val);
// //       int globalTempo() const           { return _globalTempo; }
// //       void setGlobalTempo(int val);
//       
//       void addOperation(MuseFrame_t frame, double stretch, PendingOperationList& ops); 
//       void delOperation(MuseFrame_t frame, PendingOperationList& ops);
//       };

#endif // USE_ALTERNATE_STRETCH_LIST      

      
      
// //---------------------------------------------------------
// //   FrameStretchMap
// //---------------------------------------------------------
// 
// typedef std::map<MuseFrame_t, double, std::less<MuseFrame_t> > FRAME_STRETCH_MAP;
// typedef FRAME_STRETCH_MAP::iterator iFrameStretch;
// typedef FRAME_STRETCH_MAP::const_iterator ciFrameStretch;
// typedef FRAME_STRETCH_MAP::reverse_iterator riFrameStretch;
// typedef FRAME_STRETCH_MAP::const_reverse_iterator criFrameStretch;
// 
// class FrameStretchMap : public FRAME_STRETCH_MAP {
//     
//    friend struct PendingOperationItem;
//     
// //       int _tempoSN;           // serial no to track tempo changes
// //       bool useList;
// //       int _tempo;             // tempo if not using tempo list
// //       int _globalTempo;       // %percent 50-200%
//       // Whether ANY event has a stretch other than 1.0 ie. the map is stretched, a stretcher must be engaged.
//       bool _isStretched;
// 
//       void add(MuseFrame_t frame, double stretch, bool do_normalize = true);
//       //void add(MuseFrame_t frame, StretchEvent* e, bool do_normalize = true);
//       void add(MuseFrame_t frame, const StretchEvent& e, bool do_normalize = true);
//       void del(iStretchEvent, bool do_normalize = true);
//       void del(MuseFrame_t frame, bool do_normalize = true);
// 
//    public:
//       FrameStretchMap();
//       ~FrameStretchMap();
//       void normalize();
//       void clear();
//       void eraseRange(MuseFrame_t sframe, MuseFrame_t eframe);
// 
//       void read(Xml&);
//       void write(int, Xml&) const;
//       void dump() const;
// 
//       //int stretch(unsigned tick) const;
//       double frameAt(MuseFrame_t frame) const;
// //       unsigned tick2frame(unsigned tick, unsigned frame, int* sn) const;
// //       unsigned tick2frame(unsigned tick, int* sn = 0) const;
// //       unsigned frame2tick(unsigned frame, int* sn = 0) const;
// //       unsigned frame2tick(unsigned frame, unsigned tick, int* sn) const;
// //       unsigned deltaTick2frame(unsigned tick1, unsigned tick2, int* sn = 0) const;
// //       unsigned deltaFrame2tick(unsigned frame1, unsigned frame2, int* sn = 0) const;
//       
//       double stretch(MuseFrame_t frame) const;
//       double stretch(double frame) const;
//       //double unStretch(MuseFrame_t frame) const;
//       MuseFrame_t unStretch(double frame) const;
//       // Returns whether ANY event has a stretch other than 1.0 ie. the map is stretched, a stretcher must be engaged.
//       bool isStretched() const { return _isStretched; }
//       
// //       int tempoSN() const { return _tempoSN; }
//       void setFrame(MuseFrame_t frame, double newFrame);
//       void addFrame(MuseFrame_t frame, double newFrame, bool do_normalize = true);
//       void delFrame(MuseFrame_t frame, bool do_normalize = true);
// //       bool masterFlag() const { return useList; }
// //       bool setMasterFlag(unsigned tick, bool val);
// //       int globalTempo() const           { return _globalTempo; }
// //       void setGlobalTempo(int val);
//       
// //       void addOperation(MuseFrame_t frame, double stretch, PendingOperationList& ops); 
// //       void delOperation(MuseFrame_t frame, PendingOperationList& ops);
//       };
      
      
}   // namespace MusECore

#endif
