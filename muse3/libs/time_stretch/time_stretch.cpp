//=========================================================
//  MusE
//  Linux Music Editor
//
//  time_stretch.cpp
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

#include <stdio.h>
#include "muse_math.h"

#include "time_stretch.h"
#include "xml.h"

// For debugging output: Uncomment the fprintf section.
#define ERROR_TIMESTRETCH(dev, format, args...) fprintf(dev, format, ##args)
#define INFO_TIMESTRETCH(dev, format, args...) // fprintf(dev, format, ##args)
#define DEBUG_TIMESTRETCH(dev, format, args...) // fprintf(dev, format, ##args)

namespace MusECore {

//---------------------------------------------------------
//   StretchList
//---------------------------------------------------------

StretchList::StretchList()
{
  _isStretched = false;
  _isResampled = false;
  _isPitchShifted = false;
  _startFrame = 0;
  _endFrame = 0;
  _stretchedEndFrame = 0;
  _squishedEndFrame = 0;
  _stretchRatio = 1.0;
  _samplerateRatio = 1.0;
  _pitchRatio = 1.0;

  // Ensure that there is always an item at frame zero.
  insert(std::pair<const MuseFrame_t, StretchListItem> 
    (0, StretchListItem(1.0, 1.0, 1.0, 
                        StretchListItem::StretchEvent | 
                        StretchListItem::SamplerateEvent | 
                        StretchListItem::PitchEvent)));
  
  // Technically it is normalized now, since StretchListItem
  //  constructor fills in zeros for the frame values.
  _isNormalized = true;
}

StretchList::~StretchList()
{
}

void StretchList::add(StretchListItem::StretchEventType type, MuseFrame_t frame, double value, bool do_normalize)
{
  // Some '1.0' values will be filled in if neccessary by normalize() below.
  double str = 1.0;
  double srr = 1.0;
  double psr = 1.0;
  switch(type)
  {
    case StretchListItem::StretchEvent:
      str = value;
    break;
    
    case StretchListItem::SamplerateEvent:
      srr = value;
    break;
    
    case StretchListItem::PitchEvent:
      psr = value;
    break;
  }
  
  std::pair<iStretchListItem, bool> res = 
    insert(std::pair<const MuseFrame_t, StretchListItem> 
      (frame, StretchListItem(str, srr, psr, type)));
    
  // Item already exists? Assign.
  if(!res.second)
  {
    // Set the type's value. But leave the others alone.
    switch(type)
    {
      case StretchListItem::StretchEvent:
        res.first->second._stretchRatio = value;
      break;
      
      case StretchListItem::SamplerateEvent:
        res.first->second._samplerateRatio = value;
      break;
      
      case StretchListItem::PitchEvent:
        res.first->second._pitchRatio = value;
      break;
    }
    
    // Combine the type.
    res.first->second._type |= type;
  }
  
  // Mark as invalidated, normalization is required.
  _isNormalized = false;
  
  if(do_normalize)      
    normalizeListFrames();
}

void StretchList::add(MuseFrame_t frame, const StretchListItem& e, bool do_normalize)
{
  std::pair<iStretchListItem, bool> res = insert(std::pair<const MuseFrame_t, StretchListItem> (frame, e));
  
  // Item already exists? Assign.
  if(!res.second)
  {
    //res.first->second = e;
    res.first->second._stretchRatio = e._stretchRatio;
    res.first->second._samplerateRatio = e._samplerateRatio;
    res.first->second._pitchRatio = e._pitchRatio;
  }
    
  // Mark as invalidated, normalization is required.
  _isNormalized = false;
  
  if(do_normalize)      
    normalizeListFrames();
}

void StretchList::del(int types, MuseFrame_t frame, bool do_normalize)
{
  // Do not delete the item at zeroth frame.
  if(frame == 0)
    return;
  
  iStretchListItem e = find(frame);
  if(e == end()) 
  {
    ERROR_TIMESTRETCH(stderr, "StretchList::del(%ld): not found\n", frame);
    return;
  }

  del(types, e, do_normalize);
}

void StretchList::del(int types, iStretchListItem item, bool do_normalize)
{
  // Do not delete the item at zeroth frame.
  if(item->first == 0)
    return;
  
  // We must restore any previous event's ratio to 1.0.
  // This is crucial so that when the last user marker is finally removed, the special zeroth marker (non-user)
  //  will be set to 1.0 ratio and the converters are not required anymore and can be deleted.
  if(types & StretchListItem::StretchEvent)
  {
    iStretchListItem prevStretchTyped = previousEvent(StretchListItem::StretchEvent, item);
    if(prevStretchTyped != end())
      prevStretchTyped->second._stretchRatio = 1.0;
  }
  if(types & StretchListItem::SamplerateEvent)
  {
    iStretchListItem prevSamplerateTyped = previousEvent(StretchListItem::SamplerateEvent, item);
    if(prevSamplerateTyped != end())
      prevSamplerateTyped->second._samplerateRatio = 1.0;
  }
  if(types & StretchListItem::PitchEvent)
  {
    iStretchListItem prevPitchTyped = previousEvent(StretchListItem::PitchEvent, item);
    if(prevPitchTyped != end())
      prevPitchTyped->second._stretchRatio = 1.0;
  }

  item->second._type &= ~types;

  if(item->second._type == 0)
    erase(item);
  
  // Mark as invalidated, normalization is required.
  _isNormalized = false;
  
  if(do_normalize)
    normalizeListFrames();
}


void StretchList::normalizeFrames()
{
  _stretchedEndFrame = stretch(_endFrame);
  _squishedEndFrame = squish(_endFrame);
}

void StretchList::normalizeRatios()
{
  
}

void StretchList::normalizeListFrames()
{
  double dtime;
  double factor;
  double duntime;
  MuseFrame_t dframe;
  
  MuseFrame_t thisFrame, prevFrame;
  double prevNewFrame;
  double prevNewUnFrame;
  double prevNewStretchFrame;
  double prevNewUnStretchFrame;
  double prevNewSamplerateFrame;
  double prevNewUnSamplerateFrame;
  
  double prevStretch;
  double prevSamplerate;
  double prevPitch;
  
  // If any intrinsic value has a stretch or samplerate other than 1.0,
  //  the map is stretched, a stretcher or samplerate converter must be engaged.
  _isStretched = (_stretchRatio != 1.0);
  _isResampled = (_samplerateRatio != 1.0);
  _isPitchShifted = (_pitchRatio != 1.0);
  for(iStretchListItem ise = begin(); ise != end(); ++ise)
  {
    thisFrame = ise->first;
    StretchListItem& se = ise->second;

    // The policy is such that if there are user items (non zeroth item) of a given type,
    //  the list is said to be in that state (stretched, resampled, shifted etc), even if all
    //  the items' ratios are 1.0.
    // Ignore the special zeroth frame.
    // If the zeroth frame is the only item, its ratios must (should) all be at 1.0 right now
    //  so they will be ignored.
    if(thisFrame != 0)
    {
      if(((se._type & StretchListItem::StretchEvent))) //&& se._stretchRatio != 1.0))
        _isStretched = true;
      if(((se._type & StretchListItem::SamplerateEvent))) //&& se._samplerateRatio != 1.0))
        _isResampled = true;
      if(((se._type & StretchListItem::PitchEvent))) //&& se._pitchRatio != 1.0))
        _isPitchShifted = true;
    }
    
    if(ise == begin())
    {
      prevFrame = prevNewUnFrame = prevNewFrame = 
        prevNewStretchFrame = prevNewUnStretchFrame = 
        prevNewSamplerateFrame = prevNewUnSamplerateFrame = 
        se._finSquishedFrame = se._finStretchedFrame = 
        se._stretchStretchedFrame = se._stretchSquishedFrame = 
        se._samplerateStretchedFrame = se._samplerateSquishedFrame = thisFrame;
        
      prevStretch = se._stretchRatio;
      prevSamplerate = se._samplerateRatio;
      prevPitch = se._pitchRatio;
    }
    else
    {
      dframe = thisFrame - prevFrame;
      
      factor = (_samplerateRatio * prevSamplerate) / (_stretchRatio * prevStretch);
      dtime = double(dframe) * factor;
      se._finStretchedFrame = prevNewFrame + dtime;
      prevNewFrame = se._finStretchedFrame;
      
      duntime = double(dframe) / factor;
      se._finSquishedFrame = prevNewUnFrame + duntime;
      prevNewUnFrame = se._finSquishedFrame;


      factor = 1.0 / (_stretchRatio * prevStretch);
      dtime = double(dframe) * factor;
      se._stretchStretchedFrame = prevNewStretchFrame + dtime;
      prevNewStretchFrame = se._stretchStretchedFrame;
      
      duntime = double(dframe) / factor;
      se._stretchSquishedFrame = prevNewUnStretchFrame + duntime;
      prevNewUnStretchFrame = se._stretchSquishedFrame;


      
      factor = (_samplerateRatio * prevSamplerate);
      dtime = double(dframe) * factor;
      se._samplerateStretchedFrame = prevNewSamplerateFrame + dtime;
      prevNewSamplerateFrame = se._samplerateStretchedFrame;
      
      duntime = double(dframe) / factor;
      se._samplerateSquishedFrame = prevNewUnSamplerateFrame + duntime;
      prevNewUnSamplerateFrame = se._samplerateSquishedFrame;

      
      prevFrame = thisFrame;
      
      if(se._type & StretchListItem::StretchEvent)
        prevStretch = se._stretchRatio;
      else
        se._stretchRatio = prevStretch;
      
      if(se._type & StretchListItem::SamplerateEvent)
        prevSamplerate = se._samplerateRatio;
      else
        se._samplerateRatio = prevSamplerate;
      
      if(se._type & StretchListItem::PitchEvent)
        prevPitch = se._pitchRatio;
      else
        se._pitchRatio = prevPitch;
    }
  }
  
  // TODO 
  normalizeFrames();
  
  // Mark as validated, normalization is done.
  _isNormalized = true;
  
#ifdef DEBUG_TIMESTRETCH
  dump();
#endif
}

void StretchList::normalizeListRatios()
{
  
}

//---------------------------------------------------------
//   clear
//---------------------------------------------------------

void StretchList::clear()
{
  StretchList_t::clear();
  
  // Ensure that there is always an item at frame zero.
  insert(std::pair<const MuseFrame_t, StretchListItem> 
    (0, StretchListItem(1.0, 1.0, 1.0, 
                        StretchListItem::StretchEvent | 
                        StretchListItem::SamplerateEvent | 
                        StretchListItem::PitchEvent)));
  
  // Technically it is normalized now, since StretchListItem
  //  constructor fills in zeros for the frame values.
  _isNormalized = true;
}

//---------------------------------------------------------
//   eraseRange
//---------------------------------------------------------

void StretchList::eraseRange(int types, MuseFrame_t sframe, MuseFrame_t eframe)
{
  if(sframe >= eframe)
    return;
  iStretchListItem se = lower_bound(sframe);
  if(se == end())
    return;
  iStretchListItem ee = upper_bound(eframe);
  
  for(iStretchListItem ise = se; ise != ee; )
  {
    // Do not delete the item at zeroth frame.
    if(ise->first == 0)
    {
      ++ise;
      continue;
    }
    
    ise->second._type &= ~types;
    if(ise->second._type == 0)
    {
      iStretchListItem ise_save = ise;
      erase(ise);
      ise = ise_save;
    }
    else
      ++ise;
  }
  
  // Mark as invalidated, normalization is required.
  _isNormalized = false;
  
  normalizeListFrames();
}
      
//---------------------------------------------------------
//   read
//---------------------------------------------------------

void StretchList::read(Xml& xml)
      {
      bool ok;
      for (;;) {
            Xml::Token token = xml.parse();
            const QString& tag = xml.s1();
            switch (token) {
                  case Xml::Error:
                  case Xml::End:
                        return;
                  case Xml::Attribut:
                          ERROR_TIMESTRETCH(stderr, "stretchlist unknown tag %s\n", tag.toLatin1().constData());
                        break;
                  case Xml::Text:
                        {
                          int len = tag.length();
                          int i = 0;
                          for(;;) 
                          {
                                while(i < len && (tag[i] == ',' || tag[i] == ' ' || tag[i] == '\n'))
                                  ++i;
                                if(i == len)
                                      break;
                                
                                QString fs;
                                while(i < len && tag[i] != ' ')
                                {
                                  fs.append(tag[i]); 
                                  ++i;
                                }
                                if(i == len)
                                      break;
                                
                                MuseFrame_t frame = fs.toLong(&ok);
                                if(!ok)
                                {
                                  ERROR_TIMESTRETCH(stderr, "StretchList::read failed reading frame string: %s\n", fs.toLatin1().constData());
                                  break;
                                }

                                
                                while(i < len && (tag[i] == ' ' || tag[i] == '\n'))
                                  ++i;
                                if(i == len)
                                      break;
                                QString stretchStr;
                                while(i < len && tag[i] != ' ' && tag[i] != ',')
                                {
                                  stretchStr.append(tag[i]); 
                                  ++i;
                                }
                                double stretchVal = stretchStr.toDouble(&ok);
                                if(!ok)
                                {
                                  ERROR_TIMESTRETCH(stderr, "StretchList::read failed reading stretch ratio string: %s\n", stretchStr.toLatin1().constData());
                                  break;
                                }

                                
                                while(i < len && (tag[i] == ' ' || tag[i] == '\n'))
                                  ++i;
                                if(i == len)
                                      break;
                                QString SRStr;
                                while(i < len && tag[i] != ' ' && tag[i] != ',')
                                {
                                  SRStr.append(tag[i]); 
                                  ++i;
                                }
                                double SRVal = SRStr.toDouble(&ok);
                                if(!ok)
                                {
                                  ERROR_TIMESTRETCH(stderr, "StretchList::read failed reading samplerate ratio string: %s\n", SRStr.toLatin1().constData());
                                  break;
                                }
                                
                                while(i < len && (tag[i] == ' ' || tag[i] == '\n'))
                                  ++i;
                                if(i == len)
                                      break;
                                QString pitchStr;
                                while(i < len && tag[i] != ' ' && tag[i] != ',')
                                {
                                  pitchStr.append(tag[i]); 
                                  ++i;
                                }
                                double pitchVal = pitchStr.toDouble(&ok);
                                if(!ok)
                                {
                                  ERROR_TIMESTRETCH(stderr, "StretchList::read failed reading pitch ratio string: %s\n", pitchStr.toLatin1().constData());
                                  break;
                                }

                                
                                while(i < len && (tag[i] == ' ' || tag[i] == '\n'))
                                  ++i;
                                if(i == len)
                                      break;
                                QString typeStr;
                                while(i < len && tag[i] != ' ' && tag[i] != ',')
                                {
                                  typeStr.append(tag[i]); 
                                  ++i;
                                }
                                int typeVal = typeStr.toInt(&ok);
                                if(!ok)
                                {
                                  ERROR_TIMESTRETCH(stderr, "StretchList::read failed reading type string: %s\n", typeStr.toLatin1().constData());
                                  break;
                                }

                                // Defer normalize until tag end.
                                add(frame, StretchListItem(stretchVal, SRVal, pitchVal, typeVal), false);
                                
                                if(i == len)
                                      break;
                          }
                        }
                        break;
                  case Xml::TagEnd:
                        if (tag == "stretchlist")
                        {
                              normalizeListFrames();
                              return;
                        }
                  default:
                        break;
                  }
            }
        
      }

//---------------------------------------------------------
//   write
//---------------------------------------------------------

void StretchList::write(int level, Xml& xml) const
{
  if(empty())
    return;
  
  const StretchList* sl = this;
  
  xml.tag(level++, "stretchlist");
  int i = 0;
  QString seStr("%1 %2 %3 %4 %5, ");
  for (ciStretchListItem ise = sl->begin(); ise != sl->end(); ++ise) {
        xml.nput(level, 
                  seStr.arg(ise->first)
                      .arg(ise->second._stretchRatio)
                      .arg(ise->second._samplerateRatio)
                      .arg(ise->second._pitchRatio)
                      .arg(ise->second._type)
                      .toLatin1().constData());
        ++i;
        if (i >= 3) {
              xml.put(level, "");
              i = 0;
              }
        }
  if (i)
        xml.put(level, "");
  xml.etag(level--, "stretchlist");
}

//---------------------------------------------------------
//   dump
//---------------------------------------------------------

void StretchList::dump() const
{
  const StretchList* sl = this;
  INFO_TIMESTRETCH(stderr, "\nStretchList: isNormalized:%d\n", _isNormalized);
  for(ciStretchListItem i = sl->begin(); i != sl->end(); ++i) 
  {
    INFO_TIMESTRETCH(stderr, "frame:%6ld StretchRatio:%f SamplerateRatio:%f PitchRatio:%f "
                    "stretchedFrame:%f squishedFrame:%f\n",
      i->first, i->second._stretchRatio, i->second._samplerateRatio, i->second._pitchRatio, 
      i->second._finStretchedFrame, i->second._finSquishedFrame);
  }
}

// ------------------------------------------
//  Intrinsic functions:
//-------------------------------------------

void StretchList::setStartFrame(MuseFrame_t frame, bool do_normalize)
{ 
  _startFrame = frame;

  // Mark as invalidated, normalization is required.
  _isNormalized = false;
  
  if(do_normalize)
    normalizeListFrames();
}

void StretchList::setEndFrame(MuseFrame_t frame, bool do_normalize)
{ 
  _endFrame = frame;

  // Mark as invalidated, normalization is required.
  _isNormalized = false;
  
  if(do_normalize)
    normalizeListFrames();
}

void StretchList::setStretchedEndFrame(MuseFrame_t frame, bool do_normalize)
{ 
  _stretchedEndFrame = frame;

  // Mark as invalidated, normalization is required.
  _isNormalized = false;
  
  if(do_normalize)
    normalizeListFrames();
}

void StretchList::setSquishedEndFrame(MuseFrame_t frame, bool do_normalize)
{ 
  _squishedEndFrame = frame;

  // Mark as invalidated, normalization is required.
  _isNormalized = false;
  
  if(do_normalize)
    normalizeListFrames();
}

double StretchList::ratio(StretchListItem::StretchEventType type) const
{
  switch(type)
  {
    case StretchListItem::StretchEvent:
      return _stretchRatio;
    break;
    
    case StretchListItem::SamplerateEvent:
      return _samplerateRatio;
    break;
    
    case StretchListItem::PitchEvent:
      return _pitchRatio;
    break;
  }
  return 1.0;
}

void StretchList::setRatio(StretchListItem::StretchEventType type, double ratio, bool do_normalize)
{ 
  switch(type)
  {
    case StretchListItem::StretchEvent:
      _stretchRatio = ratio;
    break;
    
    case StretchListItem::SamplerateEvent:
      _samplerateRatio = ratio;
    break;
    
    case StretchListItem::PitchEvent:
      _pitchRatio = ratio;
    break;
  }
  
  // Mark as invalidated, normalization is required.
  _isNormalized = false;
  
  if(do_normalize)
    normalizeListFrames();
}

// ------------------------------------------
//  List functions:
//-------------------------------------------

iStretchListItem StretchList::findEvent(int types, MuseFrame_t frame)
{
  iStretchListItemPair res = equal_range(frame);
  for(iStretchListItem ise = res.first; ise != res.second; ++ise)
  {
    if(ise->second._type & types)
      return ise;
  }
  return end();
}

ciStretchListItem StretchList::cFindEvent(int types, MuseFrame_t frame) const
{
  const StretchList* sl = this;
  ciStretchListItemPair res = sl->equal_range(frame);  // FIXME Calls non-const version unless cast ??
  for(ciStretchListItem ise = res.first; ise != res.second; ++ise)
  {
    if(ise->second._type & types)
      return ise;
  }
  return sl->end();
}

iStretchListItem StretchList::previousEvent(int types, iStretchListItem item)
{
  iStretchListItem i = item;
  while(i != begin())
  {
    --i;
    if(i->second._type & types)
      return i;
  }
  return end();
}

ciStretchListItem StretchList::cPreviousEvent(int types, ciStretchListItem item) const
{
  const StretchList* sl = this;
  ciStretchListItem i = item;
  while(i != sl->begin())
  {
    --i;
    if(i->second._type & types)
      return i;
  }
  return sl->end();
}

iStretchListItem StretchList::nextEvent(int types, iStretchListItem item)
{
  iStretchListItem i = item;
  while(i != end())
  {
    ++i;
    if(i->second._type & types)
      return i;
  }
  return end();
}

ciStretchListItem StretchList::cNextEvent(int types, ciStretchListItem item) const
{
  const StretchList* sl = this;
  ciStretchListItem i = item;
  while(i != sl->end())
  {
    ++i;
    if(i->second._type & types)
      return i;
  }
  return sl->end();
}



//---------------------------------------------------------
//   ratioAt
//---------------------------------------------------------

double StretchList::ratioAt(StretchListItem::StretchEventType type, MuseFrame_t frame) const
{
  // If the zeroth frame is the only item, its ratios must (should) all be at 1.0 right now
  //  so they will be ignored.
  const StretchList* sl = this;
  if(sl->size() == 1)
    return 1.0;

  ciStretchListItem i = sl->upper_bound(frame);
  if(i == sl->begin())
    return 1.0;
  --i;

  switch(type)
  {
    case StretchListItem::StretchEvent:
      return i->second._stretchRatio;
    break;
    
    case StretchListItem::SamplerateEvent:
      return i->second._samplerateRatio;
    break;
    
    case StretchListItem::PitchEvent:
      return i->second._pitchRatio;
    break;
  }
  
  return 1.0;
}

//---------------------------------------------------------
//   setRatioAt
//---------------------------------------------------------

void StretchList::setRatioAt(StretchListItem::StretchEventType type, MuseFrame_t frame, double ratio, bool do_normalize)
{
  add(type, frame, ratio, do_normalize);
}

void StretchList::setRatioAt(StretchListItem::StretchEventType type, iStretchListItem item, double ratio, bool do_normalize)
{
  item->second._type |= type;
  switch(type)
  {
    case StretchListItem::StretchEvent:
      item->second._stretchRatio = ratio;
    break;
    
    case StretchListItem::SamplerateEvent:
      item->second._samplerateRatio = ratio;
    break;
    
    case StretchListItem::PitchEvent:
      item->second._pitchRatio = ratio;
    break;
  }
  
  // Mark as invalidated, normalization is required.
  _isNormalized = false;
  
  if(do_normalize)
    normalizeListFrames();
}

void StretchList::addRatioAt(StretchListItem::StretchEventType type, MuseFrame_t frame, double ratio, bool do_normalize)
{
  add(type, frame, ratio, do_normalize);
}

void StretchList::delRatioAt(int types, MuseFrame_t frame, bool do_normalize)
{
  del(types, frame, do_normalize);
}

//---------------------------------------------------------
//   stretch
//---------------------------------------------------------

double StretchList::stretch(MuseFrame_t frame, int type) const
{
  const StretchList* sl = this;
  MuseFrame_t prevFrame;
  double prevNewFrame;
  double prevStretch;
  double prevSamplerate;
  double dtime;
  
  ciStretchListItem i = sl->upper_bound(frame);
  if(i == sl->begin())
    return frame;
  
  --i;
  prevFrame = i->first;
  prevStretch = i->second._stretchRatio;
  prevSamplerate = i->second._samplerateRatio;
  const MuseFrame_t dframe = frame - prevFrame;
  
  // Full conversion requested.
  if((type & StretchListItem::StretchEvent) && (type & StretchListItem::SamplerateEvent))
  {
    prevNewFrame = i->second._finStretchedFrame;
    dtime = double(dframe) * (_samplerateRatio * prevSamplerate) / (_stretchRatio * prevStretch);
  }
  // Stretch only.
  else if(type & StretchListItem::StretchEvent)
  {
    prevNewFrame = i->second._stretchStretchedFrame;
    dtime   = double(dframe) / (_stretchRatio * prevStretch);
  }
  // Samplerate only.
  else if(type & StretchListItem::SamplerateEvent)
  {
    prevNewFrame = i->second._samplerateStretchedFrame;
    dtime   = double(dframe) * _samplerateRatio * prevSamplerate;
  }
  
  return prevNewFrame + dtime;
}

double StretchList::stretch(double frame, int type) const
{
  const StretchList* sl = this;
  MuseFrame_t prevFrame;
  double prevNewFrame;
  double prevStretch;
  double prevSamplerate;
  double dtime;
  
  ciStretchListItem i = sl->upper_bound(frame);
  if(i == sl->begin())
    return frame;
  
  --i;
  prevFrame = i->first;
  prevStretch = i->second._stretchRatio;
  prevSamplerate = i->second._samplerateRatio;
  const double dframe = frame - (double)prevFrame;

  // Full conversion requested.
  if((type & StretchListItem::StretchEvent) && (type & StretchListItem::SamplerateEvent))
  {
    prevNewFrame = i->second._finStretchedFrame;
    dtime = dframe * (_samplerateRatio * prevSamplerate) / (_stretchRatio * prevStretch);
  }
  // Stretch only.
  else if(type & StretchListItem::StretchEvent)
  {
    prevNewFrame = i->second._stretchStretchedFrame;
    dtime   = dframe / (_stretchRatio * prevStretch);
  }
  // Samplerate only.
  else if(type & StretchListItem::SamplerateEvent)
  {
    prevNewFrame = i->second._samplerateStretchedFrame;
    dtime   = dframe * _samplerateRatio * prevSamplerate;
  }

  return prevNewFrame + dtime;
}

double StretchList::squish(MuseFrame_t frame, int type) const
{
  const StretchList* sl = this;
  MuseFrame_t prevFrame;
  double prevNewUnFrame;
  double prevStretch;
  double prevSamplerate;
  double dtime;
  
  ciStretchListItem i = sl->upper_bound(frame);
  if(i == sl->begin())
    return frame;
  
  --i;
  prevFrame = i->first;
  prevStretch = i->second._stretchRatio;
  prevSamplerate = i->second._samplerateRatio;
  const MuseFrame_t dframe = frame - prevFrame;
    
  // Full conversion requested.
  if((type & StretchListItem::StretchEvent) && (type & StretchListItem::SamplerateEvent))
  {
    prevNewUnFrame = i->second._finSquishedFrame;
    dtime = double(dframe) * (_stretchRatio * prevStretch) / (_samplerateRatio * prevSamplerate);
  }
  // Stretch only.
  else if(type & StretchListItem::StretchEvent)
  {
    prevNewUnFrame = i->second._stretchSquishedFrame;
    dtime   = double(dframe) * (_stretchRatio * prevStretch);
  }
  // Samplerate only.
  else if(type & StretchListItem::SamplerateEvent)
  {
    prevNewUnFrame = i->second._samplerateSquishedFrame;
    dtime   = double(dframe) / (_samplerateRatio * prevSamplerate);
  }
    
  return prevNewUnFrame + dtime;
}
      
double StretchList::squish(double frame, int type) const
{
  const StretchList* sl = this;
  MuseFrame_t prevFrame;
  double prevNewUnFrame;
  double prevStretch;
  double prevSamplerate;
  double dtime;
  
  ciStretchListItem i = sl->upper_bound(frame);
  if(i == sl->begin())
    return frame;
  
  --i;
  prevFrame = i->first;
  prevStretch = i->second._stretchRatio;
  prevSamplerate = i->second._samplerateRatio;
  const double dframe = frame - (double)prevFrame;
    
  // Full conversion requested.
  if((type & StretchListItem::StretchEvent) && (type & StretchListItem::SamplerateEvent))
  {
    prevNewUnFrame = i->second._finSquishedFrame;
    dtime = dframe * (_stretchRatio * prevStretch) / (_samplerateRatio * prevSamplerate);
  }
  // Stretch only.
  else if(type & StretchListItem::StretchEvent)
  {
    prevNewUnFrame = i->second._stretchSquishedFrame;
    dtime   = dframe * (_stretchRatio * prevStretch);
  }
  // Samplerate only.
  else if(type & StretchListItem::SamplerateEvent)
  {
    prevNewUnFrame = i->second._samplerateSquishedFrame;
    dtime   = dframe / (_samplerateRatio * prevSamplerate);
  }
    
  return prevNewUnFrame + dtime;
}
      
//---------------------------------------------------------
//   unStretch
//---------------------------------------------------------

MuseFrame_t StretchList::unStretch(double frame, int type) const
{
  const StretchList* sl = this;
  if(sl->empty())
    return frame;
    
  MuseFrame_t prevFrame;
  double prevNewFrame;
  double prevStretch;
  double prevSamplerate;
  double factor;
  
  ciStretchListItem e;
  for(e = sl->begin(); e != sl->end(); ++e) 
  {
    if((type & StretchListItem::StretchEvent) &&    // Full conversion requested.
        (type & StretchListItem::SamplerateEvent))
    { 
      if(frame < e->second._finStretchedFrame)
        break;
      else
        continue;
    } 
    else if(type & StretchListItem::StretchEvent)   // Only stretch conversion requested.
    {
      if(frame < e->second._stretchStretchedFrame)
        break;
      else
        continue;
    }
    else if(type & StretchListItem::SamplerateEvent) // Only samplerate conversion requested.
    {
      if(frame < e->second._samplerateStretchedFrame)
        break;
      else
        continue;
    }
  }
        
  if(e == sl->begin())
    return frame;
        
  --e;
  prevFrame = e->first;
  prevStretch = e->second._stretchRatio;
  prevSamplerate = e->second._samplerateRatio;
  
  
  // Full conversion requested.
  if((type & StretchListItem::StretchEvent) && (type & StretchListItem::SamplerateEvent))
  {
    prevNewFrame = e->second._finStretchedFrame;
    factor = (_stretchRatio * prevStretch) / (_samplerateRatio * prevSamplerate);
  }
  // Stretch only.
  else if(type & StretchListItem::StretchEvent)
  {
    prevNewFrame = e->second._stretchStretchedFrame;
    factor = (_stretchRatio * prevStretch);
  }
  // Samplerate only.
  else if(type & StretchListItem::SamplerateEvent)
  {
    prevNewFrame = e->second._samplerateStretchedFrame;
    factor = 1.0 / (_samplerateRatio * prevSamplerate);
  }
    
  return prevFrame + lrint((frame - prevNewFrame) * factor);
}

//---------------------------------------------------------
//   unStretch
//---------------------------------------------------------

MuseFrame_t StretchList::unSquish(double frame, int type) const
{
  const StretchList* sl = this;
  if(sl->empty())
    return frame;
    
  MuseFrame_t prevFrame;
  double prevNewUnFrame;
  double prevStretch;
  double prevSamplerate;
  double factor;
  
  ciStretchListItem e;
  for(e = sl->begin(); e != sl->end(); ++e) 
  {
    if((type & StretchListItem::StretchEvent) &&    // Full conversion requested.
        (type & StretchListItem::SamplerateEvent))
    { 
      if(frame < e->second._finSquishedFrame)
        break;
      else
        continue;
    } 
    else if(type & StretchListItem::StretchEvent)   // Only stretch conversion requested.
    {
      if(frame < e->second._stretchSquishedFrame)
        break;
      else
        continue;
    }
    else if(type & StretchListItem::SamplerateEvent) // Only samplerate conversion requested.
    {
      if(frame < e->second._samplerateSquishedFrame)
        break;
      else
        continue;
    }
  }
        
  if(e == sl->begin())
    return frame;
        
  --e;
  prevFrame = e->first;
  prevStretch = e->second._stretchRatio;
  prevSamplerate = e->second._samplerateRatio;
  
  // Full conversion requested.
  if((type & StretchListItem::StretchEvent) && (type & StretchListItem::SamplerateEvent))
  {
    prevNewUnFrame = e->second._finSquishedFrame;
    factor = (_samplerateRatio * prevSamplerate) / (_stretchRatio * prevStretch);
  }
  // Stretch only.
  else if(type & StretchListItem::StretchEvent)
  {
    prevNewUnFrame = e->second._stretchSquishedFrame;
    factor = 1.0 / (_stretchRatio * prevStretch);
  }
  // Samplerate only.
  else if(type & StretchListItem::SamplerateEvent)
  {
    prevNewUnFrame = e->second._samplerateSquishedFrame;
    factor = (_samplerateRatio * prevSamplerate);
  }

  // FIXME: Hm, lrint? Try returning double.
  return prevFrame + lrint((frame - prevNewUnFrame) * factor);
}

StretchListInfo StretchList::testDelListOperation(int types, MuseFrame_t frame) const
{
  // The policy is such that if (after deletion) there are still user items (non zeroth item) of a given type,
  //  the list is said to still be in that state (stretched, resampled, shifted etc),
  //  even if all the items' ratios are 1.0.
  StretchListInfo info;
  MuseFrame_t fr;
  // If any intrinsic value has a stretch or samplerate other than 1.0,
  //  the map is stretched, a stretcher or samplerate converter must be engaged.
  info._isStretched = (_stretchRatio != 1.0);
  info._isResampled = (_samplerateRatio != 1.0);
  info._isPitchShifted = (_pitchRatio != 1.0);
  for(ciStretchListItem ise = begin(); ise != end(); ++ise)
  {
    fr = ise->first;
    // Ignore the special zeroth frame.
    // If the zeroth frame is the only item, its ratios must (should) all be at 1.0 right now
    //  so they will be ignored.
    if(fr == 0)
      continue;

    const StretchListItem& se = ise->second;
    if(((se._type & StretchListItem::StretchEvent) && 
       //(types & StretchListItem::StretchEvent) &&
       (!(types & StretchListItem::StretchEvent) ||
       fr != frame))) //&&
       //se._stretchRatio != 1.0))
      info._isStretched = true;
    
    if(((se._type & StretchListItem::SamplerateEvent) && 
       //(types & StretchListItem::SamplerateEvent) &&
       (!(types & StretchListItem::SamplerateEvent) ||
       fr != frame))) //&&
       //se._samplerateRatio != 1.0))
      info._isResampled = true;

    if(((se._type & StretchListItem::PitchEvent) && 
       //(types & StretchListItem::PitchEvent) &&
       (!(types & StretchListItem::PitchEvent) ||
       fr != frame))) //&&
       //se._pitchRatio != 1.0))
      info._isPitchShifted = true;
  }
  return info;
}

} // namespace MusECore
