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
//   StretchListItem
//---------------------------------------------------------

// struct StretchListItem
// {
//   double _value;
//   
//   // Pre-computed stretch and squish frames.
//   double _stretchedFrame;
//   double _squishedFrame;
// 
//   StretchListItem(double value = 1.0,
//                   double stretchedFrame = 0,
//                   double squishedFrame = 0)
//   {
//     _value = value;
//     _stretchedFrame = stretchedFrame;
//     _squishedFrame = squishedFrame;
//   }
// };

// //---------------------------------------------------------
// //  frameMapItem
// //---------------------------------------------------------
// 
// struct frameMapItem
// {
//   // Pre-computed stretch and squish frames.
//   double _stretchedFrame;
//   double _squishedFrame;
// 
//   frameMapItem(double stretchedFrame = 0,
//                double squishedFrame = 0)
//   {
//     _stretchedFrame = stretchedFrame;
//     _squishedFrame = squishedFrame;
//   }
// };

//---------------------------------------------------------
//   StretchList
//---------------------------------------------------------

// typedef std::map<MuseFrame_t, StretchListItem, std::less<MuseFrame_t> > StretchList_t;
// typedef StretchList_t::iterator iStretchEvent;
// typedef StretchList_t::const_iterator ciStretchEvent;
// typedef StretchList_t::reverse_iterator riStretchEvent;
// typedef StretchList_t::const_reverse_iterator criStretchEvent;
// 
// typedef std::pair<iStretchEvent, iStretchEvent> iStretchEventPair;
// typedef std::pair<ciStretchEvent, ciStretchEvent> ciStretchEventPair;


// typedef std::map<MuseFrame_t, frameMapItem, std::less<MuseFrame_t> > FrameStretchMap_t;
// typedef FrameStretchMap_t::iterator iFrameMapItem;
// typedef FrameStretchMap_t::const_iterator ciFrameMapItem;
// typedef FrameStretchMap_t::reverse_iterator riFrameMapItem;
// typedef FrameStretchMap_t::const_reverse_iterator criFrameMapItem;
// 
// typedef std::pair<iFrameMapItem, iFrameMapItem> iFrameMapItemPair;
// typedef std::pair<ciFrameMapItem, ciFrameMapItem> ciFrameMapItemPair;

// class StretchList {
//       friend struct PendingOperationItem;
//       
//    public:
//       // Can be OR'd together.
//       enum StretchEventType { StretchEvent = 0x01, SamplerateEvent = 0x02, PitchEvent = 0x04 };
//       
//    private:
//       // Intrinsic values.
//       MuseFrame_t _startFrame;
//       MuseFrame_t _endFrame;
//       MuseFrame_t _stretchedEndFrame;
//       MuseFrame_t _squishedEndFrame;
//       double _stretchRatio;
//       double _samplerateRatio;
//       double _pitchRatio;
//       
//       // Lists.
//       StretchList_t _stretchRatioList;
//       StretchList_t _samplerateRatioList;
//       StretchList_t _pitchRatioList;
//       //FrameStretchMap_t _frameStretchMap;
//       
//       // Whether ANY event has a stretch other than 1.0 ie. the map is stretched, a stretcher must be engaged.
//       bool _isStretched;
//       bool _isResampled;
//       bool _isPitchShifted;
// 
//       // Whether the list is already normalized, or else a normalization is required.
//       bool _isNormalized;
//       
//       void add(int type, MuseFrame_t frame, double value, bool do_normalize = true);
//       void add(int type, MuseFrame_t frame, const StretchListItem& e, bool do_normalize = true);
//       void del(int type, MuseFrame_t frame, bool do_normalize = true);
//       void del(int type, const iStretchEvent& e, bool do_normalize = true);
// 
//    public:
//       StretchList();
//       virtual ~StretchList();
//       
//       // Returns whether ANY event has a stretch other than 1.0 ie. the map is stretched, a stretcher must be engaged.
//       bool isStretched()    const { return _isStretched; }
//       bool isResampled()    const { return _isResampled; }
//       bool isPitchShifted() const { return _isPitchShifted; }
//       
//       void normalize();
//       
// //       void normalizeFrames();
// //       void normalizeRatios();
// //       void normalizeListFrames();
// //       void normalizeListRatios();
//       // Whether the list is already normalized, or else a normalization is required.
//       bool isNormalized() const { return _isNormalized; }
//       
//       void clear();
// //       void eraseStretchRange(MuseFrame_t sframe, MuseFrame_t eframe);
// //       void eraseSamplerateRange(MuseFrame_t sframe, MuseFrame_t eframe);
// //       void eraseRange(MuseFrame_t sframe, MuseFrame_t eframe);
//       void eraseRange(int type, MuseFrame_t sframe, MuseFrame_t eframe);
// 
//       void read(Xml&);
//       void write(int, Xml&) const;
//       void dump() const;
// 
//       // ------------------------------------------
//       //  Intrinsic functions:
//       //-------------------------------------------
//       
//       MuseFrame_t startFrame() const { return _startFrame; }
//       MuseFrame_t endFrame() const { return _endFrame; }
//       MuseFrame_t stretchedEndFrame() const { return _stretchedEndFrame; }
//       MuseFrame_t squishedEndFrame() const { return _squishedEndFrame; }
// //       double stretchRatio() const { return _stretchRatio; }
// //       double samplerateRatio() const { return _samplerateRatio; }
// //       double pitchRatio() const { return _pitchRatio; }
// 
//       void setStartFrame(MuseFrame_t frame, bool do_normalize = true);
//       void setEndFrame(MuseFrame_t frame, bool do_normalize = true);
//       void setStretchedEndFrame(MuseFrame_t frame, bool do_normalize = true);
//       void setSquishedEndFrame(MuseFrame_t frame, bool do_normalize = true);
// //       void setStretchRatio(double ratio, bool do_normalize = true);
// //       void setSamplerateRatio(double ratio, bool do_normalize = true);
// //       void setPitchRatio(double ratio, bool do_normalize = true);
//       
//       double ratio(int type) const;
//       void setRatio(int type, double ratio, bool do_normalize = true);
// 
//       
//       // ------------------------------------------
//       //  List functions:
//       //-------------------------------------------
//       
//       iStretchEvent findEvent(int type, MuseFrame_t frame);
//       ciStretchEvent findEvent(int type, const MusECore::MuseFrame_t frame) const;
//       
// //       double stretchAt(MuseFrame_t frame) const;
// //       double samplerateAt(MuseFrame_t frame) const;
//       double ratioAt(int type, MuseFrame_t frame) const;
//       
//       double stretch(MuseFrame_t frame) const;
//       double stretch(double frame) const;
//       double squish(MuseFrame_t frame) const;
//       double squish(double frame) const;
//       MuseFrame_t unStretch(double frame) const;
//       MuseFrame_t unSquish(double frame) const;
//       
//       void setStretch(MuseFrame_t frame, double newStretchRatio);
//       void setSamplerate(MuseFrame_t frame, double newSamplerateRatio);
//       void addStretch(MuseFrame_t frame, double stretch, bool do_normalize = true);
//       void addSamplerate(MuseFrame_t frame, double stretch, bool do_normalize = true);
//       void delStretch(MuseFrame_t frame, bool do_normalize = true);
//       void delSamplerate(MuseFrame_t frame, bool do_normalize = true);
//       
//       void addStretchOperation(MuseFrame_t frame, double stretch, PendingOperationList& ops); 
//       void addSamplerateOperation(MuseFrame_t frame, double samplerate, PendingOperationList& ops); 
//       void delStretchOperation(MuseFrame_t frame, PendingOperationList& ops);
//       void delSamplerateOperation(MuseFrame_t frame, PendingOperationList& ops);
//       };


      

//---------------------------------------------------------
//   StretchEvent
//---------------------------------------------------------

struct StretchListItem
{
  // Can be OR'd together.
  enum StretchEventType { StretchEvent = 0x01, SamplerateEvent = 0x02, PitchEvent = 0x04 };
//   // One of the StretchEventType flags.
  // Combination of StretchEventType flags.
  int _type;
  
  double _stretchRatio;
  double _samplerateRatio;
  double _pitchRatio;
  
  // Pre-computed stretch and squish frames.
  double _finStretchedFrame;
  double _finSquishedFrame;
  double _stretchStretchedFrame;
  double _stretchSquishedFrame;
  double _samplerateStretchedFrame;
  double _samplerateSquishedFrame;

  StretchListItem(double stretchRatio = 1.0,
                  double samplerateRatio = 1.0, 
                  double pitchRatio = 1.0, 
                  int type = 0)
  {
    _stretchRatio = stretchRatio;
    _samplerateRatio = samplerateRatio;
    _pitchRatio = pitchRatio;
    _type = type;
    _finStretchedFrame = _finSquishedFrame = _stretchStretchedFrame = 
      _stretchSquishedFrame = _samplerateStretchedFrame = _samplerateSquishedFrame = 0.0;
  }
};


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
// class FrameStretchMap : public FRAME_STRETCH_MAP 
// {
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
// };




//---------------------------------------------------------
//   StretchList
//---------------------------------------------------------

typedef std::map<MuseFrame_t, StretchListItem, std::less<MuseFrame_t> > StretchList_t;
typedef StretchList_t::iterator iStretchListItem;
typedef StretchList_t::const_iterator ciStretchListItem;
typedef StretchList_t::reverse_iterator riStretchListItem;
typedef StretchList_t::const_reverse_iterator criStretchListItem;

typedef std::pair<iStretchListItem, iStretchListItem> iStretchListItemPair;
typedef std::pair<ciStretchListItem, ciStretchListItem> ciStretchListItemPair;

struct StretchListInfo
{
  bool _isStretched;
  bool _isResampled;
  bool _isPitchShifted;
  StretchListInfo(bool isStretched = false, 
                  bool isResampled = false, 
                  bool isPitchShifted = false) :
    _isStretched(isStretched), 
    _isResampled(isResampled), 
    _isPitchShifted(isPitchShifted)
  {
  }
};

class StretchList : public StretchList_t {
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
      
//       void addStr(MuseFrame_t frame, double stretch, bool do_normalize = true);
//       void addSR(MuseFrame_t frame, double samplerate, bool do_normalize = true);
//       void add(MuseFrame_t frame, const StretchEvent& e, bool do_normalize = true);
      
      void add(StretchListItem::StretchEventType type, MuseFrame_t frame, double value, bool do_normalize = true);
      void add(MuseFrame_t frame, const StretchListItem& e, bool do_normalize = true);
      
//       void del(iStretchEvent, bool do_normalize = true);
//       void delStr(MuseFrame_t frame, bool do_normalize = true);
//       void delSR(MuseFrame_t frame, bool do_normalize = true);
      void del(int types, iStretchListItem, bool do_normalize = true);
      void del(int types, MuseFrame_t frame, bool do_normalize = true);

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
//       void eraseStretchRange(MuseFrame_t sframe, MuseFrame_t eframe);
//       void eraseSamplerateRange(MuseFrame_t sframe, MuseFrame_t eframe);
      void eraseRange(int types, MuseFrame_t sframe, MuseFrame_t eframe);

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
      void setStartFrame(MuseFrame_t frame, bool do_normalize = true);
      void setEndFrame(MuseFrame_t frame, bool do_normalize = true);
      void setStretchedEndFrame(MuseFrame_t frame, bool do_normalize = true);
      void setSquishedEndFrame(MuseFrame_t frame, bool do_normalize = true);
      
      double ratio(StretchListItem::StretchEventType type) const;
      void setRatio(StretchListItem::StretchEventType type, double ratio, bool do_normalize = true);
      
      void modifyOperation(StretchListItem::StretchEventType type, double value, PendingOperationList& ops);

      // ------------------------------------------
      //  List functions:
      //-------------------------------------------
      
      iStretchListItem findEvent(int types, MuseFrame_t frame);
      ciStretchListItem cFindEvent(int types, MuseFrame_t frame) const;
      iStretchListItem previousEvent(int types, iStretchListItem);
      ciStretchListItem cPreviousEvent(int types, ciStretchListItem) const;
      iStretchListItem nextEvent(int types, iStretchListItem);
      ciStretchListItem cNextEvent(int types, ciStretchListItem) const;
      
      double ratioAt(StretchListItem::StretchEventType type, MuseFrame_t frame) const;
      void setRatioAt(StretchListItem::StretchEventType type, MuseFrame_t frame, double ratio, bool do_normalize = true);
      void setRatioAt(StretchListItem::StretchEventType type, iStretchListItem, double ratio, bool do_normalize = true);
      void addRatioAt(StretchListItem::StretchEventType type, MuseFrame_t frame, double ratio, bool do_normalize = true);
      void delRatioAt(int types, MuseFrame_t frame, bool do_normalize = true);
      
//       double stretch(MuseFrame_t frame) const;
      double stretch(MuseFrame_t frame, int type = StretchListItem::StretchEvent | StretchListItem::SamplerateEvent) const;
      double stretch(double frame, int type = StretchListItem::StretchEvent | StretchListItem::SamplerateEvent) const;
      double squish(MuseFrame_t frame, int type = StretchListItem::StretchEvent | StretchListItem::SamplerateEvent) const;
      double squish(double frame, int type = StretchListItem::StretchEvent | StretchListItem::SamplerateEvent) const;
      MuseFrame_t unStretch(double frame, int type = StretchListItem::StretchEvent | StretchListItem::SamplerateEvent) const;
      MuseFrame_t unSquish(double frame, int type = StretchListItem::StretchEvent | StretchListItem::SamplerateEvent) const;
      
//       void addStretchOperation(MuseFrame_t frame, double stretch, PendingOperationList& ops); 
//       void addSamplerateOperation(MuseFrame_t frame, double samplerate, PendingOperationList& ops); 
//       void delStretchOperation(MuseFrame_t frame, PendingOperationList& ops);
//       void delSamplerateOperation(MuseFrame_t frame, PendingOperationList& ops);
      
      void addListOperation(StretchListItem::StretchEventType type, MuseFrame_t frame, double value, PendingOperationList& ops);
      void delListOperation(int types, MuseFrame_t frame, PendingOperationList& ops);
      void modifyListOperation(StretchListItem::StretchEventType type, MuseFrame_t frame, double value, PendingOperationList& ops);
      // Whether deleting the item would cause isStretched, isResampled, or isPitchShifted
      //  to become false.
      StretchListInfo testDelListOperation(int types, MuseFrame_t frame) const;
      };






      
// //---------------------------------------------------------
// //   StretchList
// //---------------------------------------------------------
// 
// //typedef std::map<MuseFrame_t, StretchEvent, std::less<MuseFrame_t> > STRETCHLIST;
// typedef std::multimap<MuseFrame_t, StretchEvent, std::less<MuseFrame_t> > STRETCHLIST;
// typedef STRETCHLIST::iterator iStretchEvent;
// typedef STRETCHLIST::const_iterator ciStretchEvent;
// typedef STRETCHLIST::reverse_iterator riStretchEvent;
// typedef STRETCHLIST::const_reverse_iterator criStretchEvent;
// 
// typedef std::pair<iStretchEvent, iStretchEvent> iStretchEventPair;
// typedef std::pair<ciStretchEvent, ciStretchEvent> ciStretchEventPair;
// 
// class StretchList : public STRETCHLIST {
//       friend struct PendingOperationItem;
//       
//       // Intrinsic values.
//       MuseFrame_t _startFrame;
//       MuseFrame_t _endFrame;
//       MuseFrame_t _stretchedEndFrame;
//       MuseFrame_t _squishedEndFrame;
//       double _stretchRatio;
//       double _samplerateRatio;
//       double _pitchRatio;
//       
//       // Whether ANY event has a stretch other than 1.0 ie. the map is stretched, a stretcher must be engaged.
//       bool _isStretched;
//       bool _isResampled;
//       bool _isPitchShifted;
// 
//       // Whether the list is already normalized, or else a normalization is required.
//       bool _isNormalized;
//       
//       void addStr(MuseFrame_t frame, double stretch, bool do_normalize = true);
//       void addSR(MuseFrame_t frame, double samplerate, bool do_normalize = true);
//       void add(MuseFrame_t frame, const StretchEvent& e, bool do_normalize = true);
//       void del(iStretchEvent, bool do_normalize = true);
//       void delStr(MuseFrame_t frame, bool do_normalize = true);
//       void delSR(MuseFrame_t frame, bool do_normalize = true);
// 
//    public:
//       StretchList();
//       virtual ~StretchList();
//       
//       // Returns whether ANY event has a stretch other than 1.0 ie. the map is stretched, a stretcher must be engaged.
//       bool isStretched()    const { return _isStretched; }
//       bool isResampled()    const { return _isResampled; }
//       bool isPitchShifted() const { return _isPitchShifted; }
//       
//       void normalizeFrames();
//       void normalizeRatios();
//       void normalizeListFrames();
//       void normalizeListRatios();
//       // Whether the list is already normalized, or else a normalization is required.
//       bool isNormalized() const { return _isNormalized; }
//       
//       void clear();
//       void eraseStretchRange(MuseFrame_t sframe, MuseFrame_t eframe);
//       void eraseSamplerateRange(MuseFrame_t sframe, MuseFrame_t eframe);
//       void eraseRange(MuseFrame_t sframe, MuseFrame_t eframe);
// 
//       void read(Xml&);
//       void write(int, Xml&) const;
//       void dump() const;
// 
//       // ------------------------------------------
//       //  Intrinsic functions:
//       //-------------------------------------------
//       
//       MuseFrame_t startFrame() const { return _startFrame; }
//       MuseFrame_t endFrame() const { return _endFrame; }
//       MuseFrame_t stretchedEndFrame() const { return _stretchedEndFrame; }
//       MuseFrame_t squishedEndFrame() const { return _squishedEndFrame; }
//       double stretchRatio() const { return _stretchRatio; }
//       double samplerateRatio() const { return _samplerateRatio; }
//       double pitchRatio() const { return _pitchRatio; }
// 
//       void setStartFrame(MuseFrame_t frame, bool do_normalize = true);
//       void setEndFrame(MuseFrame_t frame, bool do_normalize = true);
//       void setStretchedEndFrame(MuseFrame_t frame, bool do_normalize = true);
//       void setSquishedEndFrame(MuseFrame_t frame, bool do_normalize = true);
//       void setStretchRatio(double ratio, bool do_normalize = true);
//       void setSamplerateRatio(double ratio, bool do_normalize = true);
//       void setPitchRatio(double ratio, bool do_normalize = true);
//       
//       // ------------------------------------------
//       //  List functions:
//       //-------------------------------------------
//       
//       iStretchEvent findEvent(MuseFrame_t frame, int type);
//       const_iterator findEvent(const MusECore::MuseFrame_t frame, int type) const;
//       
//       double stretchAt(MuseFrame_t frame) const;
//       double samplerateAt(MuseFrame_t frame) const;
//       
//       double stretch(MuseFrame_t frame) const;
//       double stretch(double frame) const;
//       double squish(MuseFrame_t frame) const;
//       double squish(double frame) const;
//       MuseFrame_t unStretch(double frame) const;
//       MuseFrame_t unSquish(double frame) const;
//       
//       void setStretch(MuseFrame_t frame, double newStretchRatio);
//       void setSamplerate(MuseFrame_t frame, double newSamplerateRatio);
//       void addStretch(MuseFrame_t frame, double stretch, bool do_normalize = true);
//       void addSamplerate(MuseFrame_t frame, double stretch, bool do_normalize = true);
//       void delStretch(MuseFrame_t frame, bool do_normalize = true);
//       void delSamplerate(MuseFrame_t frame, bool do_normalize = true);
//       
//       void addStretchOperation(MuseFrame_t frame, double stretch, PendingOperationList& ops); 
//       void addSamplerateOperation(MuseFrame_t frame, double samplerate, PendingOperationList& ops); 
//       void delStretchOperation(MuseFrame_t frame, PendingOperationList& ops);
//       void delSamplerateOperation(MuseFrame_t frame, PendingOperationList& ops);
//       };


      
      
      
      
      

      
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

      
      
      
}   // namespace MusECore

#endif
