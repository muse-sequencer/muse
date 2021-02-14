//=========================================================
//  MusE
//  Linux Music Editor
//
//  time_stretch.h
//  Copyright (C) 2010-2020 Tim E. Real (terminator356 on users dot sourceforge dot net)
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

#include <map>

#include "muse_time.h"
#include "xml.h"

#ifndef MUSE_TIME_STRETCH_MAX_FRAME
#define MUSE_TIME_STRETCH_MAX_FRAME (0x7ffffffffffffffeL)
//#define MUSE_TIME_STRETCH_MAX_FRAME ((1 << (sizeof(MuseFrame_t) - 1)) - 1)
#endif

namespace MusECore {

//---------------------------------------------------------
//   StretchEvent
//---------------------------------------------------------

struct StretchListItem
{
  // Can be OR'd together.
  enum StretchEventType { StretchEvent = 0x01, SamplerateEvent = 0x02, PitchEvent = 0x04 };
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
      
      void add(StretchListItem::StretchEventType type, MuseFrame_t frame, double value, bool do_normalize = true);
      void add(MuseFrame_t frame, const StretchListItem& e, bool do_normalize = true);
      
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
      
      double stretch(MuseFrame_t frame, int type = StretchListItem::StretchEvent | StretchListItem::SamplerateEvent) const;
      double stretch(double frame, int type = StretchListItem::StretchEvent | StretchListItem::SamplerateEvent) const;
      double squish(MuseFrame_t frame, int type = StretchListItem::StretchEvent | StretchListItem::SamplerateEvent) const;
      double squish(double frame, int type = StretchListItem::StretchEvent | StretchListItem::SamplerateEvent) const;
      MuseFrame_t unStretch(double frame, int type = StretchListItem::StretchEvent | StretchListItem::SamplerateEvent) const;
      MuseFrame_t unSquish(double frame, int type = StretchListItem::StretchEvent | StretchListItem::SamplerateEvent) const;
      
      // Whether deleting the item would cause isStretched, isResampled, or isPitchShifted
      //  to become false.
      StretchListInfo testDelListOperation(int types, MuseFrame_t frame) const;
      };

      
}   // namespace MusECore

#endif
