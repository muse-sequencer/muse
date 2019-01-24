//=========================================================
//  MusE
//  Linux Music Editor
//
//  time_stretch.cpp
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

//#include <QLocale>

#include <stdio.h>
//#include <errno.h>
#include <cmath>

#include "time_stretch.h"
#include "xml.h"
// #include "operations.h"


// #ifndef USE_ALTERNATE_STRETCH_LIST
//   #ifndef MUSE_TIME_STRETCH_MAX_FRAME
//   #define MUSE_TIME_STRETCH_MAX_FRAME (0x7ffffffffffffffeL)
//   //#define MUSE_TIME_STRETCH_MAX_FRAME ((1 << (sizeof(MuseFrame_t) - 1)) - 1)
//   #endif
// #endif // USE_ALTERNATE_STRETCH_LIST

#define ERROR_TIMESTRETCH(dev, format, args...)  fprintf(dev, format, ##args)
#define INFO_TIMESTRETCH(dev, format, args...) // fprintf(dev, format, ##args)
// REMOVE Tim. samplerate. Enabled.
// For debugging output: Uncomment the fprintf section.
#define DEBUG_TIMESTRETCH(dev, format, args...) // fprintf(dev, format, ##args)

namespace MusECore {

  
#ifndef USE_ALTERNATE_STRETCH_LIST
  
//---------------------------------------------------------
//   StretchList
//---------------------------------------------------------

StretchList::StretchList()
      {
      _isStretched = false;
//       _tempo   = 500000;
//       insert(std::pair<const unsigned, TEvent*> (MAX_TICK+1, new TEvent(_tempo, 0)));
      insert(std::pair<const MuseFrame_t, StretchEvent*> (MUSE_TIME_STRETCH_MAX_FRAME+1, new StretchEvent(1.0, 0)));
//       _tempoSN     = 1;
//       _globalTempo = 100;
//       useList      = true;
      }

StretchList::~StretchList()
      {
      for (iStretchEvent i = begin(); i != end(); ++i)
            delete i->second;
      }

//---------------------------------------------------------
//   add
//---------------------------------------------------------

void StretchList::add(MuseFrame_t frame, double stretch, bool do_normalize)
      {
      if (frame > MUSE_TIME_STRETCH_MAX_FRAME)
            frame = MUSE_TIME_STRETCH_MAX_FRAME;
      iStretchEvent e = upper_bound(frame);

      if (frame == e->second->_frame)
            e->second->_stretch = stretch;
      else {
            StretchEvent* ne = e->second;
            StretchEvent* ev = new StretchEvent(ne->_stretch, ne->_frame);
            ne->_stretch = stretch;
            ne->_frame = frame;
            insert(std::pair<const MuseFrame_t, StretchEvent*> (frame, ev));
            }
      if(do_normalize)      
        normalize();
      }


void StretchList::add(MuseFrame_t frame, StretchEvent* e, bool do_normalize)
{
  double stretch = e->_stretch;
  std::pair<iStretchEvent, bool> res = insert(std::pair<const MuseFrame_t, StretchEvent*> (frame, e));
  if(!res.second)
  {
    fprintf(stderr, "StretchList::add insert failed: stretchlist:%p stretch:%p %f frame:%ld\n", 
                      this, e, stretch, e->_frame);
  }
  else
  {
    iStretchEvent ine = res.first;
    ++ine; // There is always a 'next' stretch event - there is always one at index MUSE_TIME_STRETCH_MAX_FRAME + 1.
    StretchEvent* ne = ine->second;
    
    // Swap the values. (This is how the stretch list works.)
    e->_stretch = ne->_stretch;
    e->_frame = ne->_frame;
    ne->_stretch = stretch;
    ne->_frame = frame;
    
    if(do_normalize)      
      normalize();
  }
}

// //---------------------------------------------------------
// //   addOperation
// //---------------------------------------------------------
// 
// void StretchList::addOperation(MuseFrame_t frame, double stretch, PendingOperationList& ops)
// {
//   if (frame > MUSE_TIME_STRETCH_MAX_FRAME)
//     frame = MUSE_TIME_STRETCH_MAX_FRAME;
//   iStretchEvent e = upper_bound(frame);
// 
//   if(frame == e->second->_frame)
//     ops.add(PendingOperationItem(this, e, stretch, PendingOperationItem::ModifyStretch));
//   else 
//   {
//     PendingOperationItem poi(this, 0, frame, PendingOperationItem::AddStretch);
//     iPendingOperation ipo = ops.findAllocationOp(poi);
//     if(ipo != ops.end())
//     {
//       PendingOperationItem& poi = *ipo;
//       // Simply replace the value.
//       poi._stretch_event->_stretch = stretch;
//     }
//     else
//     {
//       poi._stretch_event = new StretchEvent(stretch, frame); // These are the desired frame and stretch but...
//       ops.add(poi);                               //  add will do the proper swapping with next event.
//     }
//   }
// }

//---------------------------------------------------------
//   normalize
//---------------------------------------------------------

void StretchList::normalize()
      {
      double dtime;
      MuseFrame_t dframe;
      //MuseFrame_t newFrame = 0;
      double newFrame = 0;
      _isStretched = false;
      for (iStretchEvent e = begin(); e != end(); ++e) {
            e->second->_finStretchedFrame = newFrame;
            // If ANY event has a stretch other than 1.0, the map is stretched, a stretcher must be engaged.
            if(e->second->_stretch != 1.0)
              _isStretched = true;
            dframe = e->first - e->second->_frame;
            //double dtime = double(dframe) / (MusEGlobal::config.division * _globalTempo * 10000.0/e->second->tempo);
            dtime = double(dframe) / e->second->_stretch;
            //newFrame += lrint(dtime * MusEGlobal::sampleRate);
            //newFrame += lrint(dtime);
            newFrame += dtime;
            }
      }

//---------------------------------------------------------
//   dump
//---------------------------------------------------------

void StretchList::dump() const
      {
      fprintf(stderr, "\nStretchList:\n");
      for (ciStretchEvent i = begin(); i != end(); ++i) {
            fprintf(stderr, "%6ld %06ld Stretch %f newFrame %f\n",
               i->first, i->second->_frame, i->second->_stretch,
               i->second->_finStretchedFrame);
            }
      }

//---------------------------------------------------------
//   clear
//---------------------------------------------------------

void StretchList::clear()
      {
      for (iStretchEvent i = begin(); i != end(); ++i)
            delete i->second;
      STRETCHLIST::clear();
      insert(std::pair<const MuseFrame_t, StretchEvent*> (MUSE_TIME_STRETCH_MAX_FRAME+1, new StretchEvent(1.0, 0)));
      //++_tempoSN;
      }

//---------------------------------------------------------
//   eraseRange
//---------------------------------------------------------

void StretchList::eraseRange(MuseFrame_t sframe, MuseFrame_t eframe)
{
    if(sframe >= eframe || sframe > MUSE_TIME_STRETCH_MAX_FRAME)
      return;
    if(eframe > MUSE_TIME_STRETCH_MAX_FRAME)
      eframe = MUSE_TIME_STRETCH_MAX_FRAME;
    
    //iStretchEvent se = MusEGlobal::tempomap.upper_bound(stick); // TODO FIXME Hm, suspicious - fix this in tempo.cpp as well...
    iStretchEvent se = upper_bound(sframe);
    if(se == end() || (se->first == MUSE_TIME_STRETCH_MAX_FRAME+1))
      return;

    //iStretchEvent ee = MusEGlobal::tempomap.upper_bound(etick); // FIXME
    iStretchEvent ee = upper_bound(eframe);

    ee->second->_stretch = se->second->_stretch;
    ee->second->_frame = se->second->_frame;

    for(iStretchEvent ite = se; ite != ee; ++ite)
      delete ite->second;
    erase(se, ee); // Erase range does NOT include the last element.
    normalize();
    //++_tempoSN;
}
      
//---------------------------------------------------------
//   tempo
//---------------------------------------------------------

// int StretchList::tempo(MuseFrame_t frame) const
//       {
//       if (useList) {
//             ciTEvent i = upper_bound(tick);
//             if (i == end()) {
//                   printf("no TEMPO at tick %d,0x%x\n", tick, tick);
//                   return 1000;
//                   }
//             return i->second->tempo;
//             }
//       else
//             return _tempo;
//       }

//---------------------------------------------------------
//   stretchAt
//---------------------------------------------------------

double StretchList::stretchAt(MuseFrame_t frame) const
      {
            ciStretchEvent i = upper_bound(frame);
            if (i == end()) {
                  fprintf(stderr, "stretchAt: no STRETCH at frame %ld,0x%lx\n", frame, frame);
                  return 1.0;
                  }
            return i->second->_stretch;
      }

//---------------------------------------------------------
//   del
//---------------------------------------------------------

void StretchList::del(MuseFrame_t frame, bool do_normalize)
      {
      iStretchEvent e = find(frame);
      if (e == end()) {
            fprintf(stderr, "StretchList::del(%ld): not found\n", frame);
            return;
            }
      del(e, do_normalize);
      //++_tempoSN;
      }

void StretchList::del(iStretchEvent e, bool do_normalize)
      {
      iStretchEvent ne = e;
      ++ne;
      if (ne == end()) {
            printf("StretchList::del() HALLO\n");
            return;
            }
      ne->second->_stretch = e->second->_stretch;
      ne->second->_frame  = e->second->_frame;
      erase(e);
      if(do_normalize)
        normalize();
      //++_tempoSN;
      }

// //---------------------------------------------------------
// //   delOperation
// //---------------------------------------------------------
// 
// void StretchList::delOperation(MuseFrame_t frame, PendingOperationList& ops)
// {
//   iStretchEvent e = find(frame);
//   if (e == end()) {
//         printf("StretchList::delOperation frame:%ld not found\n", frame);
//         return;
//         }
//   PendingOperationItem poi(this, e, PendingOperationItem::DeleteStretch);
//   // NOTE: Deletion is done in post-RT stage 3.
//   ops.add(poi);
// }

//---------------------------------------------------------
//   setStretch
//---------------------------------------------------------

void StretchList::setStretch(MuseFrame_t frame, double newStretch)
      {
      //if (useList)
            add(frame, newStretch);
      //else
      //      _tempo = newTempo;
      //++_tempoSN;
      }

// //---------------------------------------------------------
// //   setGlobalTempo
// //---------------------------------------------------------
// 
// void TempoList::setGlobalTempo(int val)
//       {
//       _globalTempo = val;
//       ++_tempoSN;
//       normalize();
//       }

//---------------------------------------------------------
//   addTempo
//---------------------------------------------------------

void StretchList::addStretch(MuseFrame_t frame, double stretch, bool do_normalize)
      {
      add(frame, stretch, do_normalize);
      //++_tempoSN;
      }

//---------------------------------------------------------
//   delStretch
//---------------------------------------------------------

void StretchList::delStretch(MuseFrame_t frame, bool do_normalize)
      {
      del(frame, do_normalize);
      //++_tempoSN;
      }

// //---------------------------------------------------------
// //   setMasterFlag
// //---------------------------------------------------------
// 
// bool StretchList::setMasterFlag(unsigned /*tick*/, bool val)
//       {
//       if (useList != val) {
//             useList = val;
//             ++_tempoSN;
//             return true;
//             }
//       return false;
//       }

// //---------------------------------------------------------
// //   tick2frame
// //---------------------------------------------------------
// 
// unsigned StretchList::tick2frame(unsigned tick, unsigned frame, int* sn) const
//       {
//       return (*sn == _tempoSN) ? frame : tick2frame(tick, sn);
//       }

//---------------------------------------------------------
//   stretch
//---------------------------------------------------------

//unsigned StretchList::tick2frame(unsigned tick, int* sn) const
double StretchList::stretch(MuseFrame_t frame) const
      {
      double f;
      //if (useList)
      {
            ciStretchEvent i = upper_bound(frame);
            if (i == end()) {
                  fprintf(stderr, "StretchList::stretch(%ld,0x%lx): not found\n", frame, frame);
                  return 1.0;
                  }
            MuseFrame_t dframe = frame - i->second->_frame;
            //double dtime   = double(dtick) / (MusEGlobal::config.division * _globalTempo * 10000.0/ i->second->tempo);
            double dtime   = double(dframe) / i->second->_stretch;
            //unsigned dframe   = lrint(dtime * MusEGlobal::sampleRate);
            //MuseFrame_t dNewframe   = lrint(dtime);
            //f = i->second->_finStretchedFrame + dNewframe;
            f = i->second->_finStretchedFrame + dtime;
      }
      //else
      //{
      //      double t = (double(tick) * double(_tempo)) / (double(MusEGlobal::config.division) * _globalTempo * 10000.0);
      //      f = lrint(t * MusEGlobal::sampleRate);
      //}
      //if (sn)
      //      *sn = _tempoSN;
      return f;
      }

double StretchList::stretch(double frame) const
      {
      const MuseFrame_t muse_frame = frame;
      double f;
      
      //if (useList)
      {
            ciStretchEvent i = upper_bound(muse_frame);
            if (i == end()) {
                  fprintf(stderr, "StretchList::stretch(%ld,0x%lx): not found\n", muse_frame, muse_frame);
                  return 1.0;
                  }
            double dframe = frame - (double)i->second->_frame;
            double dtime   = dframe / i->second->_stretch;
            f = i->second->_finStretchedFrame + dtime;
      }
      //else
      //{
      //      double t = (double(tick) * double(_tempo)) / (double(MusEGlobal::config.division) * _globalTempo * 10000.0);
      //      f = lrint(t * MusEGlobal::sampleRate);
      //}
      //if (sn)
      //      *sn = _tempoSN;
      return f;
      }

// //---------------------------------------------------------
// //   frame2tick
// //    return cached value t if list did not change
// //---------------------------------------------------------
// 
// unsigned StretchList::frame2tick(unsigned frame, unsigned t, int* sn) const
//       {
//       return (*sn == _tempoSN) ? t : frame2tick(frame, sn);
//       }

//---------------------------------------------------------
//   unStretch
//---------------------------------------------------------

//unsigned StretchList::frame2tick(unsigned frame, int* sn) const
//double StretchList::unStretch(MuseFrame_t frame) const
MuseFrame_t StretchList::unStretch(double frame) const
      {
      //unsigned tick;
      MuseFrame_t uframe;
      //if (useList)
      {
            ciStretchEvent e;
            for (e = begin(); e != end();) {
                  ciStretchEvent ee = e;
                  ++ee;
                  if (ee == end())
                        break;
                  if (frame < ee->second->_finStretchedFrame)
                        break;
                  e = ee;
                  }
            //unsigned te  = e->second->tempo;
            double te  = e->second->_stretch;
            //int dframe   = frame - e->second->frame;
            //MuseFrame_t dframe   = frame - e->second->_finStretchedFrame;
            double dframe  = frame - e->second->_finStretchedFrame;
            //double dtime = double(dframe) / double(MusEGlobal::sampleRate);
            //double dtime = double(dframe);
            //tick         = e->second->tick + lrint(dtime * _globalTempo * MusEGlobal::config.division * 10000.0 / te);
            //uframe         = e->second->_frame + lrint(dtime * te);
            uframe         = e->second->_frame + lrint(dframe * te);
      }
      //else
      //      tick = lrint((double(frame)/double(MusEGlobal::sampleRate)) * _globalTempo * MusEGlobal::config.division * 10000.0 / double(_tempo));
      //if (sn)
      //      *sn = _tempoSN;
      //return tick;
      return uframe;
      }

// //---------------------------------------------------------
// //   deltaTick2frame
// //---------------------------------------------------------
// 
// unsigned TempoList::deltaTick2frame(unsigned tick1, unsigned tick2, int* sn) const
//       {
//       int f1, f2;
//       if (useList) {
//             ciTEvent i = upper_bound(tick1);
//             if (i == end()) {
//                   printf("TempoList::deltaTick2frame: tick1:%d not found\n", tick1);
//                   // abort();
//                   return 0;
//                   }
//             unsigned dtick = tick1 - i->second->tick;
//             double dtime   = double(dtick) / (MusEGlobal::config.division * _globalTempo * 10000.0/ i->second->tempo);
//             unsigned dframe   = lrint(dtime * MusEGlobal::sampleRate);
//             f1 = i->second->frame + dframe;
//             
//             i = upper_bound(tick2);
//             if (i == end()) {
//                   return 0;
//                   }
//             dtick = tick2 - i->second->tick;
//             dtime   = double(dtick) / (MusEGlobal::config.division * _globalTempo * 10000.0/ i->second->tempo);
//             dframe   = lrint(dtime * MusEGlobal::sampleRate);
//             f2 = i->second->frame + dframe;
//             }
//       else {
//             double t = (double(tick1) * double(_tempo)) / (double(MusEGlobal::config.division) * _globalTempo * 10000.0);
//             f1 = lrint(t * MusEGlobal::sampleRate);
//             
//             t = (double(tick2) * double(_tempo)) / (double(MusEGlobal::config.division) * _globalTempo * 10000.0);
//             f2 = lrint(t * MusEGlobal::sampleRate);
//             }
//       if (sn)
//             *sn = _tempoSN;
//       // FIXME: Caution: This should be rounded off properly somehow, but how to do that? 
//       //                 But it seems to work so far.
//       return f2 - f1;
//       }
// 
// 
// //---------------------------------------------------------
// //   deltaFrame2tick
// //---------------------------------------------------------
// 
// unsigned TempoList::deltaFrame2tick(unsigned frame1, unsigned frame2, int* sn) const
//       {
//       unsigned tick1, tick2;
//       if (useList) {
//             ciTEvent e;
//             for (e = begin(); e != end();) {
//                   ciTEvent ee = e;
//                   ++ee;
//                   if (ee == end())
//                         break;
//                   if (frame1 < ee->second->frame)
//                         break;
//                   e = ee;
//                   }
//             unsigned te  = e->second->tempo;
//             int dframe   = frame1 - e->second->frame;
//             double dtime = double(dframe) / double(MusEGlobal::sampleRate);
//             tick1         = e->second->tick + lrint(dtime * _globalTempo * MusEGlobal::config.division * 10000.0 / te);
//             
//             for (e = begin(); e != end();) {
//                   ciTEvent ee = e;
//                   ++ee;
//                   if (ee == end())
//                         break;
//                   if (frame2 < ee->second->frame)
//                         break;
//                   e = ee;
//                   }
//             te  = e->second->tempo;
//             dframe   = frame2 - e->second->frame;
//             dtime = double(dframe) / double(MusEGlobal::sampleRate);
//             tick2         = e->second->tick + lrint(dtime * _globalTempo * MusEGlobal::config.division * 10000.0 / te);
//             }
//       else
//       {
//             tick1 = lrint((double(frame1)/double(MusEGlobal::sampleRate)) * _globalTempo * MusEGlobal::config.division * 10000.0 / double(_tempo));
//             tick2 = lrint((double(frame2)/double(MusEGlobal::sampleRate)) * _globalTempo * MusEGlobal::config.division * 10000.0 / double(_tempo));
//       }
//       if (sn)
//             *sn = _tempoSN;
//       // FIXME: Caution: This should be rounded off properly somehow, but how to do that? 
//       //                 But it seems to work so far.
//       return tick2 - tick1;
//       }
    
//---------------------------------------------------------
//   write
//---------------------------------------------------------

void StretchList::write(int level, Xml& xml) const
      {
      //xml.put(level++, "<stretchlist fix=\"%d\">", _tempo);
      xml.put(level++, "<stretchlist>");
      //if (_globalTempo != 100)
      //      xml.intTag(level, "globalTempo", _globalTempo);
      for (ciStretchEvent i = begin(); i != end(); ++i)
            i->second->write(level, xml, i->first);
      xml.tag(level, "/stretchlist");
      }

//---------------------------------------------------------
//   read
//---------------------------------------------------------

void StretchList::read(Xml& xml)
      {
      for (;;) {
            Xml::Token token = xml.parse();
            const QString& tag = xml.s1();
            switch (token) {
                  case Xml::Error:
                  case Xml::End:
                  case Xml::Attribut: //
                        return;
                  case Xml::TagStart:
                        if (tag == "stretch") {
                              StretchEvent* t = new StretchEvent();
                              //unsigned tick = t->read(xml);
                              MuseFrame_t frame = t->read(xml);
                              iStretchEvent pos = find(frame);
                              if (pos != end())
                                    erase(pos);
                              insert(std::pair<const MuseFrame_t, StretchEvent*> (frame, t));
                              }
                        //else if (tag == "globalTempo")
                        //      _globalTempo = xml.parseInt();
                        else
                              xml.unknown("StretchList");
                        break;
                  //case Xml::Attribut:
                  //      if (tag == "fix")
                  //            _tempo = xml.s2().toInt();
                  //      break;
                  case Xml::TagEnd:
                        if (tag == "stretchlist") {
                              normalize();
                              //++_tempoSN;
                              return;
                              }
                  default:
                        break;
                  }
            }
      }

//---------------------------------------------------------
//   StretchEvent::write
//---------------------------------------------------------

void StretchEvent::write(int level, Xml& xml, MuseFrame_t at) const
      {
      //xml.tag(level++, "tempo at=\"%d\"", at);
      xml.tag(level++, "stretch at=\"%ld\"", at);
      //xml.intTag(level, "tick", tick);
      xml.longIntTag(level, "frame", _frame);
      //xml.intTag(level, "val", tempo);
      xml.doubleTag(level, "val", _stretch);
      //xml.tag(level, "/tempo");
      xml.tag(level, "/stretch");
      }

//---------------------------------------------------------
//   StretchEvent::read
//---------------------------------------------------------

MuseFrame_t StretchEvent::read(Xml& xml)
      {
      MuseFrame_t at = 0;
      for (;;) {
            Xml::Token token = xml.parse();
            const QString& tag = xml.s1();
            switch (token) {
                  case Xml::Error:
                  case Xml::End:
                        return 0;
                  case Xml::TagStart:
                        if (tag == "frame")
                              _frame = xml.parseLongInt();
                        else if (tag == "val")
                              _stretch = xml.parseDouble();
                        else
                              xml.unknown("StretchEvent");
                        break;
                  case Xml::Attribut:
                        if (tag == "at")
                              at = xml.s2().toLong();
                        break;
                  case Xml::TagEnd:
                        if (tag == "stretch") {
                              return at;
                              }
                  default:
                        break;
                  }
            }
      return 0;
      }




#else  // USE_ALTERNATE_STRETCH_LIST



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

//   e->second._type &= ~types;
//   if(e->second._type == 0)
//     erase(e);
//
//   // Mark as invalidated, normalization is required.
//   _isNormalized = false;
//
//   if(do_normalize)
//     normalizeListFrames();
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
  //double newFrame = 0;
  
  MuseFrame_t thisFrame, prevFrame;
  double prevNewFrame;
  double prevNewUnFrame;
  double prevNewStretchFrame;
  double prevNewUnStretchFrame;
  double prevNewSamplerateFrame;
  double prevNewUnSamplerateFrame;
  
//   double prevStretch = 1.0;
//   double prevSamplerate = 1.0;
//   double prevPitch = 1.0;
  double prevStretch;
  double prevSamplerate;
  double prevPitch;
  
////If ANY intrinsic or list event has a stretch or samplerate other than 1.0,
//// the map is stretched, a stretcher or samplerate converter must be engaged.

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
//       prevFrame = prevNewFrame = se._finStretchedFrame = thisFrame;
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
      
      //dtime = double(dframe) / (prevStretch + prevSamplerate - 1.0);
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

//     if(se._type & StretchListItem::StretchEvent)
//       prevStretch = se._stretchRatio;
//     else
//       se._stretchRatio = prevStretch;
//     
//     if(se._type & StretchListItem::SamplerateEvent)
//       prevSamplerate = se._samplerateRatio;
//     else
//       se._samplerateRatio = prevSamplerate;
//     
//     if(se._type & StretchListItem::PitchEvent)
//       prevPitch = se._pitchRatio;
//     else
//       se._pitchRatio = prevPitch;
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
//       for (;;) {
//             Xml::Token token = xml.parse();
//             const QString& tag = xml.s1();
//             switch (token) {
//                   case Xml::Error:
//                   case Xml::End:
//                   case Xml::Attribut: //
//                         return;
//                   case Xml::TagStart:
//                         if (tag == "stretch") {
// //                               StretchEvent* t = new StretchEvent();
// //                               //unsigned tick = t->read(xml);
// //                               MuseFrame_t frame = t->read(xml);
// //                               iStretchEvent pos = find(frame);
// //                               if (pos != end())
// //                                     erase(pos);
// //                               insert(std::pair<const MuseFrame_t, StretchEvent*> (frame, t));
//                               
//                               StretchEvent e;
//                               MuseFrame_t frame = e.read(xml);
//                               std::pair<iStretchEvent, bool> res = insert(std::pair<const MuseFrame_t, StretchEvent> (frame, e));
//                               // Item already exists? Assign.
//                               if(!res.second)
//                                 res.first->second = e;
//                               
//                               }
//                         //else if (tag == "globalTempo")
//                         //      _globalTempo = xml.parseInt();
//                         else
//                               xml.unknown("StretchList");
//                         break;
//                   //case Xml::Attribut:
//                   //      if (tag == "fix")
//                   //            _tempo = xml.s2().toInt();
//                   //      break;
//                   case Xml::TagEnd:
//                         if (tag == "stretchlist") {
//                               normalize();
//                               //++_tempoSN;
//                               return;
//                               }
//                   default:
//                         break;
//                   }
//             }
        
        
      //QLocale loc = QLocale::c();
      bool ok;
      for (;;) {
            Xml::Token token = xml.parse();
            const QString& tag = xml.s1();
            switch (token) {
                  case Xml::Error:
                  case Xml::End:
                        return;
                  case Xml::Attribut:
//                         if (tag == "id")
//                         {
//                               _id = loc.toInt(xml.s2(), &ok);
//                               if(!ok)
//                                 printf("CtrlList::read failed reading _id string: %s\n", xml.s2().toLatin1().constData());
//                         }
//                         else if (tag == "cur")
//                         {
//                               _curVal = loc.toDouble(xml.s2(), &ok);
//                               if(!ok)
//                                 printf("CtrlList::read failed reading _curVal string: %s\n", xml.s2().toLatin1().constData());
//                         }        
//                         else if (tag == "visible")
//                         {
//                               _visible = loc.toInt(xml.s2(), &ok);
//                               if(!ok)
//                                 printf("CtrlList::read failed reading _visible string: %s\n", xml.s2().toLatin1().constData());
//                         }
//                         else if (tag == "color")
//                         {
// #if QT_VERSION >= 0x040700
//                               ok = _displayColor.isValidColor(xml.s2());
//                               if (!ok) {
//                                 printf("CtrlList::read failed reading color string: %s\n", xml.s2().toLatin1().constData());
//                                 break;
//                               }
// #endif
//                               _displayColor.setNamedColor(xml.s2());
//                         }
//                         else
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
                                
                                //int frame = loc.toLong(fs, &ok);
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
                                //double stretchVal = loc.toDouble(stretchStr, &ok);
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
                                //double SRVal = loc.toDouble(SRStr, &ok);
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
                                //double pitchVal = loc.toDouble(pitchStr, &ok);
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
                                //int typeVal = loc.toInt(typeStr, &ok);
                                int typeVal = typeStr.toInt(&ok);
                                if(!ok)
                                {
                                  ERROR_TIMESTRETCH(stderr, "StretchList::read failed reading type string: %s\n", typeStr.toLatin1().constData());
                                  break;
                                }

                                
// REMOVE Tim. samplerate. Changed.
//                                 add(frame, stretchVal, false); // Defer normalize until tag end.
                                add(frame, StretchListItem(stretchVal, SRVal, pitchVal, typeVal), false); // Defer normalize until tag end.
                                // For now, the conversion only has a TEMPORARY effect during song loading.
                                // See comments in Song::read at the "samplerate" tag.
                                //add(MusEGlobal::convertFrame4ProjectSampleRate(frame), val);
                                
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
//       //xml.put(level++, "<stretchlist fix=\"%d\">", _tempo);
//       xml.put(level++, "<stretchlist>");
//       //if (_globalTempo != 100)
//       //      xml.intTag(level, "globalTempo", _globalTempo);
//       for (ciStretchEvent i = begin(); i != end(); ++i)
//             //i->second->write(level, xml, i->first);
//             i->second.write(level, xml, i->first);
//       xml.tag(level, "/stretchlist");
      
      
  if(empty())
    return;
  
  const StretchList* sl = this;
  
  //for (ciStretchEvent ise = begin(); ise != end(); ++ise)
  //{
        //const CtrlList* cl = icl->second;

        //QString s= QString("stretchlist");
        //s += QString(" color=\"%1\" visible=\"%2\"").arg(cl->color().name()).arg(cl->isVisible());
        //xml.tag(level++, s.toLatin1().constData());
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
  //}
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

// void StretchList::modifyOperation(StretchListItem::StretchEventType type, double value, PendingOperationList& ops)
// {
//   ops.add(PendingOperationItem(type, this, value, PendingOperationItem::ModifyStretchListRatio));
// }


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

// double StretchList::stretch(MuseFrame_t frame) const
// {
//   const StretchList* sl = this;
//   double f;
//   MuseFrame_t prevFrame;
//   double prevNewFrame;
//   double prevStretch;
//   double prevSamplerate;
//   //if (useList)
//   {
//     ciStretchListItem i = sl->upper_bound(frame);
//     if(i == sl->begin())
//       return frame;
//     
//     --i;
//     prevFrame = i->first;
//     prevNewFrame = i->second._finStretchedFrame;
//     prevStretch = i->second._stretchRatio;
//     prevSamplerate = i->second._samplerateRatio;
//     
//     const MuseFrame_t dframe = frame - prevFrame;
//     const double factor = (_samplerateRatio * prevSamplerate) / (_stretchRatio * prevStretch);
//     const double dtime   = double(dframe) * factor;
//     
//     f = prevNewFrame + dtime;
//   }
//   return f;
// }
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
//     if(((type & StretchListItem::StretchEvent) &&    // Full conversion requested.
//         (type & StretchListItem::SamplerateEvent) && //
//         frame < e->second._finStretchedFrame) ||
//        ((type & StretchListItem::StretchEvent) &&    // Stretch only.
//         frame < e->second._stretchStretchedFrame) ||
//        ((type & StretchListItem::SamplerateEvent) && // Samplerate only. 
//         frame < e->second._samplerateStretchedFrame))
//       break;
    
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
//     if(((type & StretchListItem::StretchEvent) &&    // Full conversion requested.
//         (type & StretchListItem::SamplerateEvent) && //
//         frame < e->second._finSquishedFrame) ||
//        ((type & StretchListItem::StretchEvent) &&    // Stretch only.
//         frame < e->second._stretchSquishedFrame) ||
//        ((type & StretchListItem::SamplerateEvent) && // Samplerate only. 
//         frame < e->second._samplerateSquishedFrame))
//       break;
    
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

  return prevFrame + lrint((frame - prevNewUnFrame) * factor);
}

// void StretchList::addListOperation(StretchListItem::StretchEventType type, MuseFrame_t frame, double value, PendingOperationList& ops)
// {
//   iStretchListItem ie = find(frame);
//   if(ie != end())
//     ops.add(PendingOperationItem(type, this, ie, frame, value, PendingOperationItem::ModifyStretchListRatioAt));
//   else
//     ops.add(PendingOperationItem(type, this, frame, value, PendingOperationItem::AddStretchListRatioAt));
// }
// 
// void StretchList::delListOperation(int types, MuseFrame_t frame, PendingOperationList& ops)
// {
//   // Do not delete the item at zeroth frame.
//   if(frame == 0)
//     return;
//   
//   iStretchListItem e = find(frame);
//   if (e == end()) {
//         ERROR_TIMESTRETCH(stderr, "StretchList::delOperation frame:%ld not found\n", frame);
//         return;
//         }
//   PendingOperationItem poi(types, this, e, PendingOperationItem::DeleteStretchListRatioAt);
//   // NOTE: Deletion is done in post-RT stage 3.
//   ops.add(poi);
// }
// 
// void StretchList::modifyListOperation(StretchListItem::StretchEventType type, MuseFrame_t frame, double value, PendingOperationList& ops)
// {
//   iStretchListItem ie = find(frame);
//   if(ie == end()) {
//         ERROR_TIMESTRETCH(stderr, "StretchList::modifyListOperation frame:%ld not found\n", frame);
//         return;
//         }
//   ops.add(PendingOperationItem(type, this, ie, frame, value, PendingOperationItem::ModifyStretchListRatioAt));
// }

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















// //---------------------------------------------------------
// //   StretchList
// //---------------------------------------------------------
// 
// StretchList::StretchList()
// {
//   _isStretched = false;
//   _isResampled = false;
//   _isPitchShifted = false;
//   _isNormalized = false;
//   _startFrame = 0;
//   _endFrame = 0;
//   _stretchedEndFrame = 0;
//   _squishedEndFrame = 0;
//   _stretchRatio = 1.0;
//   _samplerateRatio = 1.0;
//   _pitchRatio = 1.0;
// }
// 
// StretchList::~StretchList()
// {
// }
// 
// void StretchList::add(int type, MuseFrame_t frame, double value, bool do_normalize = true)
// {
//   StretchList_t* list;
//   switch(type)
//   {
//     case StretchEvent:
//       list = &_stretchRatioList;
//     break;
//     
//     case SamplerateEvent:
//       list = &_samplerateRatioList;
//     break;
//     
//     case PitchEvent:
//       list = &_pitchRatioList;
//     break;
//   }
//   
//   std::pair<iStretchEvent, bool> res = list->insert(
//     std::pair<const MuseFrame_t, StretchListItem> (frame, StretchListItem(value)));
//   
//   // Item already exists? Assign.
//   if(!res.second)
//     // Set the stretch. But leave the samplerate alone.
//     res.first->second._value = value;
//   
//   // Mark as invalidated, normalization is required.
//   _isNormalized = false;
//   
//   if(do_normalize)      
//     normalize();
// }
// 
// void StretchList::add(int type, MuseFrame_t frame, const StretchListItem& e, bool do_normalize = true)
// {
//   StretchList_t* list;
//   switch(type)
//   {
//     case StretchEvent:
//       list = &_stretchRatioList;
//     break;
//     
//     case SamplerateEvent:
//       list = &_samplerateRatioList;
//     break;
//     
//     case PitchEvent:
//       list = &_pitchRatioList;
//     break;
//   }
//   
//   std::pair<iStretchEvent, bool> res = list->insert(
//     std::pair<const MuseFrame_t, StretchListItem> (frame, e));
//   
//   // Item already exists? Assign.
//   if(!res.second)
//     res.first->second = e; // Assign.
//   
//   // Mark as invalidated, normalization is required.
//   _isNormalized = false;
//   
//   if(do_normalize)      
//     normalize();
// }
// 
// void StretchList::del(int type, MuseFrame_t frame, bool do_normalize = true)
// {
//   StretchList_t* list;
//   switch(type)
//   {
//     case StretchEvent:
//       list = &_stretchRatioList;
//     break;
//     
//     case SamplerateEvent:
//       list = &_samplerateRatioList;
//     break;
//     
//     case PitchEvent:
//       list = &_pitchRatioList;
//     break;
//   }
// 
//   iStretchEvent e = list->find(frame);
//   if(e == list->end()) 
//   {
//     fprintf(stderr, "StretchList::delStr(%ld): not found\n", frame);
//     return;
//   }
// 
//   list->erase(e);
//   
//   // Mark as invalidated, normalization is required.
//   _isNormalized = false;
//   
//   if(do_normalize)
//     normalize();
// }
// 
// void StretchList::del(int type, const iStretchEvent& e, bool do_normalize = true)
// {
//   StretchList_t* list;
//   switch(type)
//   {
//     case StretchEvent:
//       list = &_stretchRatioList;
//     break;
//     
//     case SamplerateEvent:
//       list = &_samplerateRatioList;
//     break;
//     
//     case PitchEvent:
//       list = &_pitchRatioList;
//     break;
//   }
// 
//   list->erase(e);
//   
//   // Mark as invalidated, normalization is required.
//   _isNormalized = false;
//   
//   if(do_normalize)
//     normalize();
// }
// 
// 
// void StretchList::normalize()
// {
//   double dtime;
//   double factor;
//   double duntime;
//   MuseFrame_t dframe;
//   //double newFrame = 0;
//   
//   MuseFrame_t prevFrame;
//   double prevNewFrame;
//   double prevNewUnFrame;
//   double prevStretch = 1.0;
//   double prevSamplerate = 1.0;
//   double prevPitch = 1.0;
//   _isStretched = false;
//   _isResampled = false;
//   _isPitchShifted = false;
//   
//   
//   for(iStretchEvent ise = begin(); ise != end(); ++ise)
//   {
//     StretchEvent& se = ise->second;
//     
//     // If ANY event has a stretch or samplerate other than 1.0, the map is stretched, 
//     //  a stretcher or samplerate converter must be engaged.
//     if(((se._type & StretchEvent::StretchEventType) && se._stretchRatio != 1.0) || _stretchRatio != 1.0) 
//       _isStretched = true;
//     if(((se._type & StretchEvent::SamplerateEventType) && se._samplerateRatio != 1.0) || _samplerateRatio != 1.0)
//       _isResampled = true;
//     if(((se._type & StretchEvent::PitchEventType) && se._pitchRatio != 1.0) || _pitchRatio != 1.0)
//       _isPitchShifted = true;
//     
//     if(ise == begin())
//     {
// //       prevFrame = prevNewFrame = se._finStretchedFrame = ise->first;
//       prevFrame = prevNewUnFrame = prevNewFrame = se._finSquishedFrame = se._finStretchedFrame = ise->first;
//     }
//     else
//     {
//       dframe = ise->first - prevFrame;
//       
//       //dtime = double(dframe) / (prevStretch + prevSamplerate - 1.0);
//       factor = (_samplerateRatio * prevSamplerate) / (_stretchRatio * prevStretch);
//       dtime = double(dframe) * factor;
//       se._finStretchedFrame = prevNewFrame + dtime;
//       prevNewFrame = se._finStretchedFrame;
//       
//       duntime = double(dframe) / factor;
//       se._finSquishedFrame = prevNewUnFrame + duntime;
//       prevNewUnFrame = se._finSquishedFrame;
//       
//       prevFrame = ise->first;
//     }
// 
//     if(se._type & StretchEvent::StretchEventType)
//       prevStretch = se._stretchRatio;
//     else
//       se._stretchRatio = prevStretch;
//     
//     if(se._type & StretchEvent::SamplerateEventType)
//       prevSamplerate = se._samplerateRatio;
//     else
//       se._samplerateRatio = prevSamplerate;
//     
//     if(se._type & StretchEvent::PitchEventType)
//       prevPitch = se._pitchRatio;
//     else
//       se._pitchRatio = prevPitch;
//   }
//   
//   // TODO 
//   normalizeFrames();
//   
//   
//   
//   
//   //iStretchEvent curEvent = StretchList_t::end();
//   iStretchEvent curEvent;
//   MuseFrame_t stretch_frame = 0;
//   iStretchEvent istretch = _stretchRatioList.begin();
//   if(istretch != _stretchRatioList.end())
//   {
//     stretch_frame = istretch->first;
//     curEvent = istretch;
//   }
// 
//   MuseFrame_t samplerate_frame = 0;
//   iStretchEvent isamplerate = _samplerateRatioList.begin();  
//   if(isamplerate != _samplerateRatioList.end())
//   {
//     samplerate_frame = isamplerate->first;
//     if(samplerate_frame < stretch_frame || istretch == _stretchRatioList.end())
//       curEvent = isamplerate;
//   }
//   
//   //stretch_frame < samplerate_frame ? istretch : isamplerate;
//   
//   prevFrame = prevNewUnFrame = prevNewFrame = se._finSquishedFrame = se._finStretchedFrame = ise->first;
//   
//   
//   
//   // Mark as validated, normalization is done.
//   _isNormalized = true;
//   
// #ifdef DEBUG_TIMESTRETCH
//   dump();
// #endif
// }
//       
// //---------------------------------------------------------
// //   clear
// //---------------------------------------------------------
// 
// void StretchList::clear()
// {
//   _stretchRatioList.clear();
//   _samplerateRatioList.clear();
//   _pitchRatioList.clear();
//   //_frameStretchMap.clear();
//   
//   // Mark as invalidated, normalization is required.
//   _isNormalized = false;
// }
// 
// //---------------------------------------------------------
// //   eraseRange
// //---------------------------------------------------------
// 
// void StretchList::eraseRange(int type, MuseFrame_t sframe, MuseFrame_t eframe)
// {
//   if(sframe >= eframe)
//     return;
//   
//   if(type & StretchEvent)
//   {
//     iStretchEvent se = _stretchRatioList.lower_bound(sframe);
//     if(se != _stretchRatioList.end())
//     {
//       iStretchEvent ee = _stretchRatioList.upper_bound(eframe);
//       _stretchRatioList.erase(se, ee); // Erase range does NOT include the last element.
//     }
//   }
//   
//   if(type & SamplerateEvent)
//   {
//     iStretchEvent se = _samplerateRatioList.lower_bound(sframe);
//     if(se != _samplerateRatioList.end())
//     {
//       iStretchEvent ee = _samplerateRatioList.upper_bound(eframe);
//       _samplerateRatioList.erase(se, ee); // Erase range does NOT include the last element.
//     }
//   }
//   
//   if(type & PitchEvent)
//   {
//     iStretchEvent se = _pitchRatioList.lower_bound(sframe);
//     if(se != _pitchRatioList.end())
//     {
//       iStretchEvent ee = _pitchRatioList.upper_bound(eframe);
//       _pitchRatioList.erase(se, ee); // Erase range does NOT include the last element.
//     }
//   }
// 
// //   // Check if _frameStretchMap needs updating.
// //   const iFrameMapItem se = _frameStretchMap.lower_bound(sframe);
// //   if(se != _frameStretchMap.end())
// //   {
// //     const iFrameMapItem ee = _frameStretchMap.upper_bound(eframe);
// //     MuseFrame_t frame;
// //     iFrameMapItem ise_save;
// //     for(iFrameMapItem ise = se; ise != ee; )
// //     {
// //       frame = ise->first;
// //       // No stretch ratio or samplerate ratio at this frame? Erase the stretch map item.
// //       if(_stretchRatioList.find(frame) == _stretchRatioList.end() && 
// //          _samplerateRatioList.find(frame) == _samplerateRatioList.end())
// //       {
// //         ise_save = ise;
// //         _frameStretchMap.erase(ise);
// //         ise = ise_save;
// //       }
// //       else
// //         ++ise;
// //     }
// //   }
//   
//   // Mark as invalidated, normalization is required.
//   _isNormalized = false;
//   
//   normalize();
// }
//       
// //---------------------------------------------------------
// //   read
// //---------------------------------------------------------
// 
// void StretchList::read(Xml& xml)
// {
//       //QLocale loc = QLocale::c();
//       bool ok;
//       for (;;) {
//             Xml::Token token = xml.parse();
//             const QString& tag = xml.s1();
//             switch (token) {
//                   case Xml::Error:
//                   case Xml::End:
//                         return;
//                   case Xml::Attribut:
//                           fprintf(stderr, "stretchlist unknown tag %s\n", tag.toLatin1().constData());
//                         break;
//                   case Xml::Text:
//                         {
//                           int len = tag.length();
//                           int i = 0;
//                           for(;;) 
//                           {
//                                 while(i < len && (tag[i] == ',' || tag[i] == ' ' || tag[i] == '\n'))
//                                   ++i;
//                                 if(i == len)
//                                       break;
//                                 
//                                 QString fs;
//                                 while(i < len && tag[i] != ' ')
//                                 {
//                                   fs.append(tag[i]); 
//                                   ++i;
//                                 }
//                                 if(i == len)
//                                       break;
//                                 
//                                 MuseFrame_t frame = fs.toLong(&ok);
//                                 if(!ok)
//                                 {
//                                   fprintf(stderr, "CtrlList::read failed reading frame string: %s\n", fs.toLatin1().constData());
//                                   break;
//                                 }
//                                   
//                                 while(i < len && (tag[i] == ' ' || tag[i] == '\n'))
//                                   ++i;
//                                 if(i == len)
//                                       break;
//                                 QString stretchStr;
//                                 while(i < len && tag[i] != ' ' && tag[i] != ',')
//                                 {
//                                   stretchStr.append(tag[i]); 
//                                   ++i;
//                                 }
//                                 //double stretchVal = loc.toDouble(stretchStr, &ok);
//                                 double stretchVal = stretchStr.toDouble(&ok);
//                                 if(!ok)
//                                 {
//                                   fprintf(stderr, "CtrlList::read failed reading stretch ratio string: %s\n", stretchStr.toLatin1().constData());
//                                   break;
//                                 }
// 
//                                 while(i < len && (tag[i] == ' ' || tag[i] == '\n'))
//                                   ++i;
//                                 if(i == len)
//                                       break;
//                                 QString SRStr;
//                                 while(i < len && tag[i] != ' ' && tag[i] != ',')
//                                 {
//                                   SRStr.append(tag[i]); 
//                                   ++i;
//                                 }
//                                 //double SRVal = loc.toDouble(SRStr, &ok);
//                                 double SRVal = SRStr.toDouble(&ok);
//                                 if(!ok)
//                                 {
//                                   fprintf(stderr, "CtrlList::read failed reading samplerate ratio string: %s\n", SRStr.toLatin1().constData());
//                                   break;
//                                 }
//                                 
//                                 
//                                 while(i < len && (tag[i] == ' ' || tag[i] == '\n'))
//                                   ++i;
//                                 if(i == len)
//                                       break;
//                                 QString typeStr;
//                                 while(i < len && tag[i] != ' ' && tag[i] != ',')
//                                 {
//                                   typeStr.append(tag[i]); 
//                                   ++i;
//                                 }
//                                 //int typeVal = loc.toInt(typeStr, &ok);
//                                 int typeVal = typeStr.toInt(&ok);
//                                 if(!ok)
//                                 {
//                                   fprintf(stderr, "CtrlList::read failed reading value string: %s\n", typeStr.toLatin1().constData());
//                                   break;
//                                 }
// 
//                                 
// // REMOVE Tim. samplerate. Changed.
// //                                 add(frame, stretchVal, false); // Defer normalize until tag end.
//                                 add(frame, StretchEvent(stretchVal, SRVal, typeVal), false); // Defer normalize until tag end.
//                                 // For now, the conversion only has a TEMPORARY effect during song loading.
//                                 // See comments in Song::read at the "samplerate" tag.
//                                 //add(MusEGlobal::convertFrame4ProjectSampleRate(frame), val);
//                                 
//                                 if(i == len)
//                                       break;
//                           }
//                         }
//                         break;
//                   case Xml::TagEnd:
//                         if (tag == "stretchlist")
//                         {
//                               normalize();
//                               return;
//                         }
//                   default:
//                         break;
//                   }
//             }
//         
// }
// 
// //---------------------------------------------------------
// //   write
// //---------------------------------------------------------
// 
// void StretchList::write(int level, Xml& xml) const
// {
// //       //xml.put(level++, "<stretchlist fix=\"%d\">", _tempo);
// //       xml.put(level++, "<stretchlist>");
// //       //if (_globalTempo != 100)
// //       //      xml.intTag(level, "globalTempo", _globalTempo);
// //       for (ciStretchEvent i = begin(); i != end(); ++i)
// //             //i->second->write(level, xml, i->first);
// //             i->second.write(level, xml, i->first);
// //       xml.tag(level, "/stretchlist");
//       
//       
//   if(empty())
//     return;
//   
//   //for (ciStretchEvent ise = begin(); ise != end(); ++ise)
//   //{
//         //const CtrlList* cl = icl->second;
// 
//         //QString s= QString("stretchlist");
//         //s += QString(" color=\"%1\" visible=\"%2\"").arg(cl->color().name()).arg(cl->isVisible());
//         //xml.tag(level++, s.toLatin1().constData());
//         xml.tag(level++, "stretchlist");
//         int i = 0;
//         QString seStr("%1 %2 %3 %4, ");
//         for (ciStretchEvent ise = begin(); ise != end(); ++ise) {
//               xml.nput(level, 
//                        seStr.arg(ise->first)
//                             .arg(ise->second._stretchRatio)
//                             .arg(ise->second._samplerateRatio)
//                             .arg(ise->second._type)
//                             .toLatin1().constData());
//               ++i;
//               if (i >= 3) {
//                     xml.put(level, "");
//                     i = 0;
//                     }
//               }
//         if (i)
//               xml.put(level, "");
//         xml.etag(level--, "stretchlist");
//   //}
//   
//       
// }
// 
// //---------------------------------------------------------
// //   dump
// //---------------------------------------------------------
// 
// void StretchList::dump() const
// {
// //   fprintf(stderr, "\nStretchList: isNormalized:%d\n", _isNormalized);
// //   for(ciStretchEvent i = _stretchRatioList.begin(); i != _stretchRatioList.end(); ++i) 
// //     fprintf(stderr, "frame:%6ld StretchRatio:%f\n", i->first, i->second._value);
// //   
// //   for(ciStretchEvent i = _samplerateRatioList.begin(); i != _samplerateRatioList.end(); ++i) 
// //     fprintf(stderr, "frame:%6ld SamplerateRatio:%f\n", i->first, i->second._value);
// //   
// //   for(ciStretchEvent i = _pitchRatioList.begin(); i != _pitchRatioList.end(); ++i) 
// //     fprintf(stderr, "frame:%6ld PitchRatio:%f\n", i->first, i->second._value);
// //   
// //   for(ciFrameMapItem i = _frameStretchMap.begin(); i != _frameStretchMap.end(); ++i) 
// //   {
// //     fprintf(stderr, "frame:%6ld newFrame:%f newUnFrame:%f\n",
// //       i->first, i->second._finStretchedFrame, i->second._finSquishedFrame);
// //   }
//   
//   
//   fprintf(stderr, "\nStretchList: isNormalized:%d\n", _isNormalized);
//   
//   for(ciStretchEvent i = _stretchRatioList.begin(); i != _stretchRatioList.end(); ++i) 
//     fprintf(stderr, "frame:%6ld StretchRatio:%f stretchedFrame:%f squishedFrame:%f\n", 
//             i->first, i->second._value, i->second._finStretchedFrame, i->second._finSquishedFrame);
//   
//   for(ciStretchEvent i = _samplerateRatioList.begin(); i != _samplerateRatioList.end(); ++i) 
//     fprintf(stderr, "frame:%6ld SamplerateRatio:%f stretchedFrame:%f squishedFrame:%f\n", 
//             i->first, i->second._value, i->second._finStretchedFrame, i->second._finSquishedFrame);
//   
//   for(ciStretchEvent i = _pitchRatioList.begin(); i != _pitchRatioList.end(); ++i) 
//     fprintf(stderr, "frame:%6ld PitchRatio:%f stretchedFrame:%f squishedFrame:%f\n", 
//             i->first, i->second._value, i->second._finStretchedFrame, i->second._finSquishedFrame);
// }
// 
// //---------------------------------------------------------
// //   dump
// //---------------------------------------------------------
// 
// double StretchList::ratio(int type) const 
// { 
//   switch(type)
//   {
//     case StretchEvent:
//       return _stretchRatio; 
//     break;
//     
//     case SamplerateEvent:
//       return _samplerateRatio; 
//     break;
//     
//     case PitchEvent:
//       return _pitchRatio; 
//     break;
//   }
//   return 1.0;
// }
//       
// void StretchList::setRatio(int type, double ratio, bool do_normalize)
// {
//   switch(type)
//   {
//     case StretchEvent:
//       _stretchRatio = ratio;
//     break;
//     
//     case SamplerateEvent:
//       _samplerateRatio = ratio;
//     break;
//     
//     case PitchEvent:
//       _pitchRatio = ratio;
//     break;
//   }
// 
//   // Mark as invalidated, normalization is required.
//   _isNormalized = false;
//   
//   if(do_normalize)
//     normalize();
// }
//       
// void StretchList::setStartFrame(MuseFrame_t frame, bool do_normalize)
// { 
//   _startFrame = frame;
// 
//   // Mark as invalidated, normalization is required.
//   _isNormalized = false;
//   
//   if(do_normalize)
//     normalize();
// }
// 
// void StretchList::setEndFrame(MuseFrame_t frame, bool do_normalize)
// { 
//   _endFrame = frame;
// 
//   // Mark as invalidated, normalization is required.
//   _isNormalized = false;
//   
//   if(do_normalize)
//     normalize();
// }
// 
// void StretchList::setStretchedEndFrame(MuseFrame_t frame, bool do_normalize)
// { 
//   _stretchedEndFrame = frame;
// 
//   // Mark as invalidated, normalization is required.
//   _isNormalized = false;
//   
//   if(do_normalize)
//     normalize();
// }
// 
// void StretchList::setSquishedEndFrame(MuseFrame_t frame, bool do_normalize)
// { 
//   _squishedEndFrame = frame;
// 
//   // Mark as invalidated, normalization is required.
//   _isNormalized = false;
//   
//   if(do_normalize)
//     normalize();
// }
// 
// iStretchEvent StretchList::findEvent(int type, MuseFrame_t frame)
// {
//   StretchList_t* list;
//   switch(type)
//   {
//     case StretchEvent:
//       list = &_stretchRatioList;
//     break;
//     
//     case SamplerateEvent:
//       list = &_samplerateRatioList;
//     break;
//     
//     case PitchEvent:
//       list = &_pitchRatioList;
//     break;
//   }
//   return list->find(frame);
// }
// 
// ciStretchEvent StretchList::findEvent(int type, MuseFrame_t frame) const
// {
//   const StretchList_t* list;
//   switch(type)
//   {
//     case StretchEvent:
//       list = &_stretchRatioList;
//     break;
//     
//     case SamplerateEvent:
//       list = &_samplerateRatioList;
//     break;
//     
//     case PitchEvent:
//       list = &_pitchRatioList;
//     break;
//   }
//   return list->find(frame);
// }
// 
// double StretchList::ratioAt(int type, MuseFrame_t frame) const
// {
//   const StretchList_t* list;
//   switch(type)
//   {
//     case StretchEvent:
//       list = &_stretchRatioList;
//     break;
//     
//     case SamplerateEvent:
//       list = &_samplerateRatioList;
//     break;
//     
//     case PitchEvent:
//       list = &_pitchRatioList;
//     break;
//   }
//   ciStretchEvent i = list->upper_bound(frame);
//   if(i == list->begin())
//     return 1.0;
//   --i;
//   return i->second._value;
//   
// }
//       
//       
//       
//       
//       
// 
// 
// //---------------------------------------------------------
// //   StretchList
// //---------------------------------------------------------
// 
// StretchList::StretchList()
// {
//   _isStretched = false;
//   _isResampled = false;
//   _isPitchShifted = false;
//   _isNormalized = false;
//   _startFrame = 0;
//   _endFrame = 0;
//   _stretchedEndFrame = 0;
//   _squishedEndFrame = 0;
//   _stretchRatio = 1.0;
//   _samplerateRatio = 1.0;
//   _pitchRatio = 1.0;
// }
// 
// StretchList::~StretchList()
// {
// }
// 
// //---------------------------------------------------------
// //   add
// //---------------------------------------------------------
// 
// // void StretchList::add(MuseFrame_t frame, double stretch, bool do_normalize)
// //       {
// //       std::pair<iStretchEvent, bool> res = insert(std::pair<const MuseFrame_t, StretchEvent> (frame, StretchEvent(stretch)));
// //       // Item already exists? Assign.
// //       if(!res.second)
// //         res.first->second._stretch = stretch;
// //       
// //       if(do_normalize)      
// //         normalize();
// //       }
// void StretchList::addStr(MuseFrame_t frame, double stretch, bool do_normalize)
// {
//   // The '1.0' samplerate will be filled in if neccessary by normalize() below.
//   std::pair<iStretchEvent, bool> res = 
//     insert(std::pair<const MuseFrame_t, StretchEvent> 
//       (frame, StretchEvent(stretch, 1.0, StretchEvent::StretchEventType)));
//     
//   // Item already exists? Assign.
//   if(!res.second)
//   {
//     // Set the stretch. But leave the samplerate alone.
//     res.first->second._stretchRatio = stretch;
//     // Combine the type.
//     res.first->second._type |= StretchEvent::StretchEventType;
//   }
//   
//   // Mark as invalidated, normalization is required.
//   _isNormalized = false;
//   
//   if(do_normalize)      
//     normalizeListFrames();
// }
// 
// void StretchList::addSR(MuseFrame_t frame, double samplerate, bool do_normalize)
// {
//   // The '1.0' stretch will be filled in if neccessary by normalize() below.
//   std::pair<iStretchEvent, bool> res = 
//     insert(std::pair<const MuseFrame_t, StretchEvent> 
//       (frame, StretchEvent(1.0, samplerate, StretchEvent::SamplerateEventType)));
//     
//   // Item already exists? Assign.
//   if(!res.second)
//   {
//     // Set the samplerate. But leave the stretch alone.
//     res.first->second._samplerateRatio = samplerate;
//     // Combine the type.
//     res.first->second._type |= StretchEvent::SamplerateEventType;
//   }
//   
//   // Mark as invalidated, normalization is required.
//   _isNormalized = false;
//   
//   if(do_normalize)      
//     normalizeListFrames();
// }
// 
// void StretchList::add(MuseFrame_t frame, const StretchEvent& e, bool do_normalize)
// {
//   std::pair<iStretchEvent, bool> res = insert(std::pair<const MuseFrame_t, StretchEvent> (frame, e));
//   
//   // Item already exists? Assign.
//   if(!res.second)
//   {
//     //fprintf(stderr, "StretchList::add insert failed: stretchlist:%p stretch:%f samplerate:%f frame:%ld\n", 
//     //                  this, e._stretchRatio, e._samplerateRatio, frame);
// //     // Set the stretch. But leave the samplerate alone.
// //     res.first->second._stretchRatio = e._stretchRatio;
// //     // Set the samplerate. But leave the stretch alone.
// //     res.first->second._samplerateRatio = e._samplerateRatio;
// //     // Combine the type.
// //     res.first->second._type = e._type;
//     res.first->second = e; // Assign.
//   }
// //   else
// //   {
// //     iStretchEvent ine = res.first;
// //     ++ine; // There is always a 'next' stretch event - there is always one at index MUSE_TIME_STRETCH_MAX_FRAME + 1.
// //     StretchEvent* ne = ine->second;
// //     
// //     // Swap the values. (This is how the stretch list works.)
// //     e->_stretch = ne->_stretch;
// //     e->_frame = ne->_frame;
// //     ne->_stretch = stretch;
// //     ne->_frame = frame;
//     
//   // Mark as invalidated, normalization is required.
//   _isNormalized = false;
//   
//     if(do_normalize)      
//       normalizeListFrames();
// //   }
// }
// 
// //---------------------------------------------------------
// //   addStretchOperation
// //---------------------------------------------------------
// 
// void StretchList::addStretchOperation(MuseFrame_t frame, double stretch, PendingOperationList& ops)
// {
//   iStretchEvent ie = find(frame);
//   if(ie != end())
//     ops.add(PendingOperationItem(this, ie, frame, stretch, PendingOperationItem::ModifyStretchRatioAt));
//   else
//     ops.add(PendingOperationItem(this, frame, stretch, PendingOperationItem::AddStretchRatioAt));
// }
// 
// void StretchList::addSamplerateOperation(MuseFrame_t frame, double samplerate, PendingOperationList& ops)
// {
//   iStretchEvent ie = find(frame);
//   if(ie != end())
//     ops.add(PendingOperationItem(this, ie, frame, samplerate, PendingOperationItem::ModifySamplerateRatioAt));
//   else
//     ops.add(PendingOperationItem(this, frame, samplerate, PendingOperationItem::AddSamplerateRatioAt));
// }
// 
// //---------------------------------------------------------
// //   delStretchOperation
// //---------------------------------------------------------
// 
// void StretchList::delStretchOperation(MuseFrame_t frame, PendingOperationList& ops)
// {
//   iStretchEvent e = find(frame);
//   if (e == end()) {
//         printf("StretchList::delStretchOperation frame:%ld not found\n", frame);
//         return;
//         }
//   PendingOperationItem poi(this, e, PendingOperationItem::DeleteStretchRatioAt);
//   // NOTE: Deletion is done in post-RT stage 3.
//   ops.add(poi);
// }
// 
// //---------------------------------------------------------
// //   delSamplerateOperation
// //---------------------------------------------------------
// 
// void StretchList::delSamplerateOperation(MuseFrame_t frame, PendingOperationList& ops)
// {
//   iStretchEvent e = find(frame);
//   if (e == end()) {
//         printf("StretchList::delSamplerateOperation frame:%ld not found\n", frame);
//         return;
//         }
//   PendingOperationItem poi(this, e, PendingOperationItem::DeleteSamplerateRatioAt);
//   // NOTE: Deletion is done in post-RT stage 3.
//   ops.add(poi);
// }
// 
// //---------------------------------------------------------
// //   normalizeFrames
// //---------------------------------------------------------
// 
// void StretchList::normalizeFrames()
// {
//   _stretchedEndFrame = stretch(_endFrame);
//   _squishedEndFrame = squish(_endFrame);
// }
// 
// void StretchList::normalizeRatios()
// {
//   
// }
// 
// void StretchList::normalizeListFrames()
// {
//   double dtime;
//   double factor;
//   double duntime;
//   MuseFrame_t dframe;
//   //double newFrame = 0;
//   
//   MuseFrame_t prevFrame;
//   double prevNewFrame;
//   double prevNewUnFrame;
//   double prevStretch = 1.0;
//   double prevSamplerate = 1.0;
//   double prevPitch = 1.0;
//   _isStretched = false;
//   _isResampled = false;
//   _isPitchShifted = false;
//   for(iStretchEvent ise = begin(); ise != end(); ++ise)
//   {
//     StretchEvent& se = ise->second;
//     
//     // If ANY event has a stretch or samplerate other than 1.0, the map is stretched, 
//     //  a stretcher or samplerate converter must be engaged.
//     if(((se._type & StretchEvent::StretchEventType) && se._stretchRatio != 1.0) || _stretchRatio != 1.0) 
//       _isStretched = true;
//     if(((se._type & StretchEvent::SamplerateEventType) && se._samplerateRatio != 1.0) || _samplerateRatio != 1.0)
//       _isResampled = true;
//     if(((se._type & StretchEvent::PitchEventType) && se._pitchRatio != 1.0) || _pitchRatio != 1.0)
//       _isPitchShifted = true;
//     
//     if(ise == begin())
//     {
// //       prevFrame = prevNewFrame = se._finStretchedFrame = ise->first;
//       prevFrame = prevNewUnFrame = prevNewFrame = se._finSquishedFrame = se._finStretchedFrame = ise->first;
//     }
//     else
//     {
//       dframe = ise->first - prevFrame;
//       
//       //dtime = double(dframe) / (prevStretch + prevSamplerate - 1.0);
//       factor = (_samplerateRatio * prevSamplerate) / (_stretchRatio * prevStretch);
//       dtime = double(dframe) * factor;
//       se._finStretchedFrame = prevNewFrame + dtime;
//       prevNewFrame = se._finStretchedFrame;
//       
//       duntime = double(dframe) / factor;
//       se._finSquishedFrame = prevNewUnFrame + duntime;
//       prevNewUnFrame = se._finSquishedFrame;
//       
//       prevFrame = ise->first;
//     }
// 
//     if(se._type & StretchEvent::StretchEventType)
//       prevStretch = se._stretchRatio;
//     else
//       se._stretchRatio = prevStretch;
//     
//     if(se._type & StretchEvent::SamplerateEventType)
//       prevSamplerate = se._samplerateRatio;
//     else
//       se._samplerateRatio = prevSamplerate;
//     
//     if(se._type & StretchEvent::PitchEventType)
//       prevPitch = se._pitchRatio;
//     else
//       se._pitchRatio = prevPitch;
//   }
//   
//   // TODO 
//   normalizeFrames();
//   
//   // Mark as validated, normalization is done.
//   _isNormalized = true;
//   
// #ifdef DEBUG_TIMESTRETCH
//   dump();
// #endif
// }
// 
// void StretchList::normalizeListRatios()
// {
//   
// }
// 
// 
// 
// //---------------------------------------------------------
// //   dump
// //---------------------------------------------------------
// 
// void StretchList::dump() const
// {
//   fprintf(stderr, "\nStretchList:\n");
//   for(ciStretchEvent i = begin(); i != end(); ++i) 
//   {
//     fprintf(stderr, "frame:%6ld StretchRatio:%f SamplerateRatio:%f PitchRatio:%f "
//                     "newFrame:%f newUnFrame:%f isNormalized:%d\n",
//       i->first, i->second._stretchRatio, i->second._samplerateRatio, i->second._pitchRatio, 
//       i->second._finStretchedFrame, i->second._finSquishedFrame, _isNormalized);
//   }
// }
// 
// //---------------------------------------------------------
// //   clear
// //---------------------------------------------------------
// 
// void StretchList::clear()
// {
//   STRETCHLIST::clear();
//   // Mark as invalidated, normalization is required.
//   _isNormalized = false;
// }
// 
// //---------------------------------------------------------
// //   eraseStretchRange
// //---------------------------------------------------------
// 
// void StretchList::eraseStretchRange(MuseFrame_t sframe, MuseFrame_t eframe)
// {
//   if(sframe >= eframe)
//     return;
//   iStretchEvent se = lower_bound(sframe);
//   if(se == end())
//     return;
//   iStretchEvent ee = upper_bound(eframe);
//   
//   for(iStretchEvent ise = se; ise != ee; )
//   {
//     ise->second._type &= ~StretchEvent::StretchEventType;
//     if(ise->second._type == 0)
//     {
//       iStretchEvent ise_save = ise;
//       erase(ise);
//       ise = ise_save;
//     }
//     else
//       ++ise;
//   }
//   
//   // Mark as invalidated, normalization is required.
//   _isNormalized = false;
//   
//   normalizeListFrames();
// }
//       
// //---------------------------------------------------------
// //   eraseSamplerateRange
// //---------------------------------------------------------
// 
// void StretchList::eraseSamplerateRange(MuseFrame_t sframe, MuseFrame_t eframe)
// {
//   if(sframe >= eframe)
//     return;
//   iStretchEvent se = lower_bound(sframe);
//   if(se == end())
//     return;
//   iStretchEvent ee = upper_bound(eframe);
//   
//   for(iStretchEvent ise = se; ise != ee; )
//   {
//     ise->second._type &= ~StretchEvent::SamplerateEventType;
//     if(ise->second._type == 0)
//     {
//       iStretchEvent ise_save = ise;
//       erase(ise);
//       ise = ise_save;
//     }
//     else
//       ++ise;
//   }
//   
//   // Mark as invalidated, normalization is required.
//   _isNormalized = false;
//   
//   normalizeListFrames();
// }
//       
// //---------------------------------------------------------
// //   eraseRange
// //---------------------------------------------------------
// 
// void StretchList::eraseRange(MuseFrame_t sframe, MuseFrame_t eframe)
// {
//   if(sframe >= eframe)
//     return;
//   iStretchEvent se = lower_bound(sframe);
//   if(se == end())
//     return;
//   iStretchEvent ee = upper_bound(eframe);
//   erase(se, ee); // Erase range does NOT include the last element.
//     
//   // Mark as invalidated, normalization is required.
//   _isNormalized = false;
//   
//   normalizeListFrames();
// }
// 
// 
// void StretchList::setStartFrame(MuseFrame_t frame, bool do_normalize)
// { 
//   _startFrame = frame;
//   if(do_normalize)
//     normalizeListFrames();
// }
// 
// void StretchList::setEndFrame(MuseFrame_t frame, bool do_normalize)
// { 
//   _endFrame = frame;
//   if(do_normalize)
//     normalizeListFrames();
// }
// 
// void StretchList::setStretchedEndFrame(MuseFrame_t frame, bool do_normalize)
// { 
//   _stretchedEndFrame = frame;
//   if(do_normalize)
//     normalizeListFrames();
// }
// 
// void StretchList::setSquishedEndFrame(MuseFrame_t frame, bool do_normalize)
// { 
//   _squishedEndFrame = frame;
//   if(do_normalize)
//     normalizeListFrames();
// }
// 
// void StretchList::setStretchRatio(double ratio, bool do_normalize)
// { 
//   _stretchRatio = ratio;
//   if(do_normalize)
//     normalizeListFrames();
// }
// 
// void StretchList::setSamplerateRatio(double ratio, bool do_normalize)
// { 
//   _samplerateRatio = ratio;
//   if(do_normalize)
//     normalizeListFrames();
// }
// 
// void StretchList::setPitchRatio(double ratio, bool do_normalize)
// { 
//   _pitchRatio = ratio;
//   if(do_normalize)
//     normalizeListFrames();
// }
// 
// 
// iStretchEvent StretchList::findEvent(MuseFrame_t frame, int type)
// {
//   iStretchEventPair res = equal_range(frame);
//   for(iStretchEvent ise = res.first; ise != res.second; ++ise)
//   {
//     if(ise->second._type == type)
//       return ise;
//   }
//   return end();
// }
// 
// ciStretchEvent StretchList::findEvent(MuseFrame_t frame, int type) const
// {
//   ciStretchEventPair res = equal_range(frame);  // FIXME Calls non-const version ??
//   for(ciStretchEvent ise = res.first; ise != res.second; ++ise)
//   {
//     if(ise->second._type == type)
//       return ise;
//   }
//   return end();
// }
// 
// 
// //---------------------------------------------------------
// //   stretchAt
// //---------------------------------------------------------
// 
// double StretchList::stretchAt(MuseFrame_t frame) const
// {
//   ciStretchEvent i = upper_bound(frame);
//   if(i == begin())
//     return 1.0;
//   --i;
//   return i->second._stretchRatio;
// }
// 
// //---------------------------------------------------------
// //   samplerateAt
// //---------------------------------------------------------
// 
// double StretchList::samplerateAt(MuseFrame_t frame) const
// {
//   ciStretchEvent i = upper_bound(frame);
//   if(i == begin())
//     return 1.0;
//   --i;
//   return i->second._samplerateRatio;
// }
// 
// // //---------------------------------------------------------
// // //   eventAt
// // //---------------------------------------------------------
// // 
// // StretchEvent StretchList::eventAt(MuseFrame_t frame) const
// // {
// //   ciStretchEvent i = upper_bound(frame);
// //   if(i == begin())
// //     return 1.0;
// //   --i;
// //   return i->second._samplerateRatio;
// // }
// 
// //---------------------------------------------------------
// //   delStr
// //---------------------------------------------------------
// 
// void StretchList::delStr(MuseFrame_t frame, bool do_normalize)
// {
//   iStretchEvent e = find(frame);
//   if(e == end()) 
//   {
//     fprintf(stderr, "StretchList::delStr(%ld): not found\n", frame);
//     return;
//   }
//   e->second._type &= ~StretchEvent::StretchEventType;
//   if(e->second._type == 0)
//     erase(e);
//   
//   // Mark as invalidated, normalization is required.
//   _isNormalized = false;
//   
//   if(do_normalize)
//     normalizeListFrames();
// }
// 
// //---------------------------------------------------------
// //   delSR
// //---------------------------------------------------------
// 
// void StretchList::delSR(MuseFrame_t frame, bool do_normalize)
// {
//   iStretchEvent e = find(frame);
//   if(e == end()) 
//   {
//     fprintf(stderr, "StretchList::delSR(%ld): not found\n", frame);
//     return;
//   }
//   e->second._type &= ~StretchEvent::SamplerateEventType;
//   if(e->second._type == 0)
//     erase(e);
//   
//   // Mark as invalidated, normalization is required.
//   _isNormalized = false;
//   
//   if(do_normalize)
//     normalizeListFrames();
// }
// 
// //---------------------------------------------------------
// //   del
// //---------------------------------------------------------
// 
// void StretchList::del(iStretchEvent e, bool do_normalize)
// {
//   erase(e);
//   
//   // Mark as invalidated, normalization is required.
//   _isNormalized = false;
//   
//   if(do_normalize)
//     normalizeListFrames();
// }
//       
// //---------------------------------------------------------
// //   setStretch
// //---------------------------------------------------------
// 
// void StretchList::setStretch(MuseFrame_t frame, double newStretch)
// {
//   addStr(frame, newStretch);
// }
// 
// //---------------------------------------------------------
// //   addStretch
// //---------------------------------------------------------
// 
// void StretchList::addStretch(MuseFrame_t frame, double stretch, bool do_normalize)
// {
//   addStr(frame, stretch, do_normalize);
// }
// 
// //---------------------------------------------------------
// //   delStretch
// //---------------------------------------------------------
// 
// void StretchList::delStretch(MuseFrame_t frame, bool do_normalize)
// {
//   delStr(frame, do_normalize);
// }
// 
// //---------------------------------------------------------
// //   setSamplerate
// //---------------------------------------------------------
// 
// void StretchList::setSamplerate(MuseFrame_t frame, double newSamplerate)
// {
//   addSR(frame, newSamplerate);
// }
// 
// //---------------------------------------------------------
// //   addSamplerate
// //---------------------------------------------------------
// 
// void StretchList::addSamplerate(MuseFrame_t frame, double samplerate, bool do_normalize)
// {
//   addSR(frame, samplerate, do_normalize);
// }
// 
// //---------------------------------------------------------
// //   delSamplerate
// //---------------------------------------------------------
// 
// void StretchList::delSamplerate(MuseFrame_t frame, bool do_normalize)
// {
//   delSR(frame, do_normalize);
// }
//       
// //---------------------------------------------------------
// //   stretch
// //---------------------------------------------------------
// 
// double StretchList::stretch(MuseFrame_t frame) const
// {
//   double f;
//   MuseFrame_t prevFrame;
//   double prevNewFrame;
//   double prevStretch;
//   double prevSamplerate;
//   //if (useList)
//   {
//     ciStretchEvent i = upper_bound(frame);
//     if(i == begin())
//       return frame;
//     
//     --i;
//     prevFrame = i->first;
//     prevNewFrame = i->second._finStretchedFrame;
//     prevStretch = i->second._stretchRatio;
//     prevSamplerate = i->second._samplerateRatio;
//     
//     const MuseFrame_t dframe = frame - prevFrame;
//     const double factor = (_samplerateRatio * prevSamplerate) / (_stretchRatio * prevStretch);
//     const double dtime   = double(dframe) * factor;
//     
//     f = prevNewFrame + dtime;
//   }
//   return f;
// }
// 
// double StretchList::stretch(double frame) const
// {
//   double f;
//   MuseFrame_t prevFrame;
//   double prevNewFrame;
//   double prevStretch;
//   double prevSamplerate;
//   //if (useList)
//   {
//     ciStretchEvent i = upper_bound(frame);
//     if(i == begin())
//       return frame;
//     
//     --i;
//     prevFrame = i->first;
//     prevNewFrame = i->second._finStretchedFrame;
//     prevStretch = i->second._stretchRatio;
//     prevSamplerate = i->second._samplerateRatio;
//     
//     const double dframe = frame - (double)prevFrame;
//     const double factor = (_samplerateRatio * prevSamplerate) / (_stretchRatio * prevStretch);
//     const double dtime   = dframe * factor;
//     f = prevNewFrame + dtime;
//   }
//   return f;
// }
// 
// double StretchList::squish(MuseFrame_t frame) const
// {
//   double f;
//   MuseFrame_t prevFrame;
//   double prevNewUnFrame;
//   double prevStretch;
//   double prevSamplerate;
//   //if (useList)
//   {
//     ciStretchEvent i = upper_bound(frame);
//     if(i == begin())
//       return frame;
//     
//     --i;
//     prevFrame = i->first;
//     prevNewUnFrame = i->second._finSquishedFrame;
//     prevStretch = i->second._stretchRatio;
//     prevSamplerate = i->second._samplerateRatio;
//     
//     const MuseFrame_t dframe = frame - prevFrame;
//     const double factor = (_stretchRatio * prevStretch) / (_samplerateRatio * prevSamplerate);
//     const double dtime   = (double)dframe * factor;
//     f = prevNewUnFrame + dtime;
//   }
//   return f;
// }
//       
// double StretchList::squish(double frame) const
// {
//   double f;
//   MuseFrame_t prevFrame;
//   double prevNewUnFrame;
//   double prevStretch;
//   double prevSamplerate;
//   //if (useList)
//   {
//     ciStretchEvent i = upper_bound(frame);
//     if(i == begin())
//       return frame;
//     
//     --i;
//     prevFrame = i->first;
//     prevNewUnFrame = i->second._finSquishedFrame;
//     prevStretch = i->second._stretchRatio;
//     prevSamplerate = i->second._samplerateRatio;
//     
//     const double dframe = frame - (double)prevFrame;
//     const double factor = (_stretchRatio * prevStretch) / (_samplerateRatio * prevSamplerate);
//     const double dtime   = dframe * factor;
//     f = prevNewUnFrame + dtime;
//   }
//   return f;
// }
//       
// //---------------------------------------------------------
// //   unStretch
// //---------------------------------------------------------
// 
// MuseFrame_t StretchList::unStretch(double frame) const
// {
//   if(empty())
//     return frame;
//     
//   MuseFrame_t prevFrame;
//   double prevNewFrame;
//   double prevStretch;
//   double prevSamplerate;
//   MuseFrame_t uframe;
//   //if (useList)
//   {
//     ciStretchEvent e;
//     for(e = begin(); e != end(); ++e) 
//     {
//       if(frame < e->second._finSquishedFrame)
//         break;
//     }
//           
//     if(e == begin())
//       return frame;
//           
//     --e;
//     prevFrame = e->first;
//     prevNewFrame = e->second._finStretchedFrame;
//     prevStretch = e->second._stretchRatio;
//     prevSamplerate = e->second._samplerateRatio;
//     
//     const double factor = (_stretchRatio * prevStretch) / (_samplerateRatio * prevSamplerate);
//     uframe = prevFrame + lrint((frame - prevNewFrame) * factor);
//   }
//   return uframe;
// }
// 
// //---------------------------------------------------------
// //   unStretch
// //---------------------------------------------------------
// 
// MuseFrame_t StretchList::unSquish(double frame) const
// {
//   if(empty())
//     return frame;
//     
//   MuseFrame_t prevFrame;
//   double prevNewUnFrame;
//   double prevStretch;
//   double prevSamplerate;
//   MuseFrame_t uframe;
//   //if (useList)
//   {
//     ciStretchEvent e;
//     for(e = begin(); e != end(); ++e) 
//     {
//       if(frame < e->second._finSquishedFrame)
//         break;
//     }
//           
//     if(e == begin())
//       return frame;
//           
//     --e;
//     prevFrame = e->first;
//     prevNewUnFrame = e->second._finSquishedFrame;
//     prevStretch = e->second._stretchRatio;
//     prevSamplerate = e->second._samplerateRatio;
//     
//     const double factor = (_samplerateRatio * prevSamplerate) / (_stretchRatio * prevStretch);
//     uframe = prevFrame + lrint((frame - prevNewUnFrame) * factor);
//   }
//   return uframe;
// }
// 
// //---------------------------------------------------------
// //   write
// //---------------------------------------------------------
// 
// void StretchList::write(int level, Xml& xml) const
// {
// //       //xml.put(level++, "<stretchlist fix=\"%d\">", _tempo);
// //       xml.put(level++, "<stretchlist>");
// //       //if (_globalTempo != 100)
// //       //      xml.intTag(level, "globalTempo", _globalTempo);
// //       for (ciStretchEvent i = begin(); i != end(); ++i)
// //             //i->second->write(level, xml, i->first);
// //             i->second.write(level, xml, i->first);
// //       xml.tag(level, "/stretchlist");
//       
//       
//   if(empty())
//     return;
//   
//   //for (ciStretchEvent ise = begin(); ise != end(); ++ise)
//   //{
//         //const CtrlList* cl = icl->second;
// 
//         //QString s= QString("stretchlist");
//         //s += QString(" color=\"%1\" visible=\"%2\"").arg(cl->color().name()).arg(cl->isVisible());
//         //xml.tag(level++, s.toLatin1().constData());
//         xml.tag(level++, "stretchlist");
//         int i = 0;
//         QString seStr("%1 %2 %3 %4, ");
//         for (ciStretchEvent ise = begin(); ise != end(); ++ise) {
//               xml.nput(level, 
//                        seStr.arg(ise->first)
//                             .arg(ise->second._stretchRatio)
//                             .arg(ise->second._samplerateRatio)
//                             .arg(ise->second._type)
//                             .toLatin1().constData());
//               ++i;
//               if (i >= 3) {
//                     xml.put(level, "");
//                     i = 0;
//                     }
//               }
//         if (i)
//               xml.put(level, "");
//         xml.etag(level--, "stretchlist");
//   //}
//   
//       
// }
// 
// //---------------------------------------------------------
// //   read
// //---------------------------------------------------------
// 
// void StretchList::read(Xml& xml)
//       {
// //       for (;;) {
// //             Xml::Token token = xml.parse();
// //             const QString& tag = xml.s1();
// //             switch (token) {
// //                   case Xml::Error:
// //                   case Xml::End:
// //                   case Xml::Attribut: //
// //                         return;
// //                   case Xml::TagStart:
// //                         if (tag == "stretch") {
// // //                               StretchEvent* t = new StretchEvent();
// // //                               //unsigned tick = t->read(xml);
// // //                               MuseFrame_t frame = t->read(xml);
// // //                               iStretchEvent pos = find(frame);
// // //                               if (pos != end())
// // //                                     erase(pos);
// // //                               insert(std::pair<const MuseFrame_t, StretchEvent*> (frame, t));
// //                               
// //                               StretchEvent e;
// //                               MuseFrame_t frame = e.read(xml);
// //                               std::pair<iStretchEvent, bool> res = insert(std::pair<const MuseFrame_t, StretchEvent> (frame, e));
// //                               // Item already exists? Assign.
// //                               if(!res.second)
// //                                 res.first->second = e;
// //                               
// //                               }
// //                         //else if (tag == "globalTempo")
// //                         //      _globalTempo = xml.parseInt();
// //                         else
// //                               xml.unknown("StretchList");
// //                         break;
// //                   //case Xml::Attribut:
// //                   //      if (tag == "fix")
// //                   //            _tempo = xml.s2().toInt();
// //                   //      break;
// //                   case Xml::TagEnd:
// //                         if (tag == "stretchlist") {
// //                               normalize();
// //                               //++_tempoSN;
// //                               return;
// //                               }
// //                   default:
// //                         break;
// //                   }
// //             }
//         
//         
//       //QLocale loc = QLocale::c();
//       bool ok;
//       for (;;) {
//             Xml::Token token = xml.parse();
//             const QString& tag = xml.s1();
//             switch (token) {
//                   case Xml::Error:
//                   case Xml::End:
//                         return;
//                   case Xml::Attribut:
// //                         if (tag == "id")
// //                         {
// //                               _id = loc.toInt(xml.s2(), &ok);
// //                               if(!ok)
// //                                 printf("CtrlList::read failed reading _id string: %s\n", xml.s2().toLatin1().constData());
// //                         }
// //                         else if (tag == "cur")
// //                         {
// //                               _curVal = loc.toDouble(xml.s2(), &ok);
// //                               if(!ok)
// //                                 printf("CtrlList::read failed reading _curVal string: %s\n", xml.s2().toLatin1().constData());
// //                         }        
// //                         else if (tag == "visible")
// //                         {
// //                               _visible = loc.toInt(xml.s2(), &ok);
// //                               if(!ok)
// //                                 printf("CtrlList::read failed reading _visible string: %s\n", xml.s2().toLatin1().constData());
// //                         }
// //                         else if (tag == "color")
// //                         {
// // #if QT_VERSION >= 0x040700
// //                               ok = _displayColor.isValidColor(xml.s2());
// //                               if (!ok) {
// //                                 printf("CtrlList::read failed reading color string: %s\n", xml.s2().toLatin1().constData());
// //                                 break;
// //                               }
// // #endif
// //                               _displayColor.setNamedColor(xml.s2());
// //                         }
// //                         else
//                               fprintf(stderr, "stretchlist unknown tag %s\n", tag.toLatin1().constData());
//                         break;
//                   case Xml::Text:
//                         {
//                           int len = tag.length();
//                           int i = 0;
//                           for(;;) 
//                           {
//                                 while(i < len && (tag[i] == ',' || tag[i] == ' ' || tag[i] == '\n'))
//                                   ++i;
//                                 if(i == len)
//                                       break;
//                                 
//                                 QString fs;
//                                 while(i < len && tag[i] != ' ')
//                                 {
//                                   fs.append(tag[i]); 
//                                   ++i;
//                                 }
//                                 if(i == len)
//                                       break;
//                                 
//                                 //int frame = loc.toLong(fs, &ok);
//                                 MuseFrame_t frame = fs.toLong(&ok);
//                                 if(!ok)
//                                 {
//                                   fprintf(stderr, "CtrlList::read failed reading frame string: %s\n", fs.toLatin1().constData());
//                                   break;
//                                 }
//                                   
//                                 while(i < len && (tag[i] == ' ' || tag[i] == '\n'))
//                                   ++i;
//                                 if(i == len)
//                                       break;
//                                 QString stretchStr;
//                                 while(i < len && tag[i] != ' ' && tag[i] != ',')
//                                 {
//                                   stretchStr.append(tag[i]); 
//                                   ++i;
//                                 }
//                                 //double stretchVal = loc.toDouble(stretchStr, &ok);
//                                 double stretchVal = stretchStr.toDouble(&ok);
//                                 if(!ok)
//                                 {
//                                   fprintf(stderr, "CtrlList::read failed reading stretch ratio string: %s\n", stretchStr.toLatin1().constData());
//                                   break;
//                                 }
// 
//                                 while(i < len && (tag[i] == ' ' || tag[i] == '\n'))
//                                   ++i;
//                                 if(i == len)
//                                       break;
//                                 QString SRStr;
//                                 while(i < len && tag[i] != ' ' && tag[i] != ',')
//                                 {
//                                   SRStr.append(tag[i]); 
//                                   ++i;
//                                 }
//                                 //double SRVal = loc.toDouble(SRStr, &ok);
//                                 double SRVal = SRStr.toDouble(&ok);
//                                 if(!ok)
//                                 {
//                                   fprintf(stderr, "CtrlList::read failed reading samplerate ratio string: %s\n", SRStr.toLatin1().constData());
//                                   break;
//                                 }
//                                 
//                                 
//                                 while(i < len && (tag[i] == ' ' || tag[i] == '\n'))
//                                   ++i;
//                                 if(i == len)
//                                       break;
//                                 QString typeStr;
//                                 while(i < len && tag[i] != ' ' && tag[i] != ',')
//                                 {
//                                   typeStr.append(tag[i]); 
//                                   ++i;
//                                 }
//                                 //int typeVal = loc.toInt(typeStr, &ok);
//                                 int typeVal = typeStr.toInt(&ok);
//                                 if(!ok)
//                                 {
//                                   fprintf(stderr, "CtrlList::read failed reading value string: %s\n", typeStr.toLatin1().constData());
//                                   break;
//                                 }
// 
//                                 
// // REMOVE Tim. samplerate. Changed.
// //                                 add(frame, stretchVal, false); // Defer normalize until tag end.
//                                 add(frame, StretchEvent(stretchVal, SRVal, typeVal), false); // Defer normalize until tag end.
//                                 // For now, the conversion only has a TEMPORARY effect during song loading.
//                                 // See comments in Song::read at the "samplerate" tag.
//                                 //add(MusEGlobal::convertFrame4ProjectSampleRate(frame), val);
//                                 
//                                 if(i == len)
//                                       break;
//                           }
//                         }
//                         break;
//                   case Xml::TagEnd:
//                         if (tag == "stretchlist")
//                         {
//                               normalizeListFrames();
//                               return;
//                         }
//                   default:
//                         break;
//                   }
//             }
//         
//       }








//=================================================================================
//=================================================================================
//=================================================================================




// //---------------------------------------------------------
// //   StretchList
// //---------------------------------------------------------
// 
// StretchList::StretchList()
//       {
//       _isStretched = false;
// //       _tempo   = 500000;
// //       insert(std::pair<const unsigned, TEvent*> (MAX_TICK+1, new TEvent(_tempo, 0)));
//       
//       //insert(std::pair<const MuseFrame_t, StretchEvent*> (MUSE_TIME_STRETCH_MAX_FRAME+1, new StretchEvent(1.0, 0)));
//       //insert(std::pair<const MuseFrame_t, StretchEvent*> (0, new StretchEvent(1.0, 0)));
//       
// //       _tempoSN     = 1;
// //       _globalTempo = 100;
// //       useList      = true;
//       }
// 
// StretchList::~StretchList()
//       {
//       //for (iStretchEvent i = begin(); i != end(); ++i)
//       //      delete i->second;
//       }
// 
// //---------------------------------------------------------
// //   add
// //---------------------------------------------------------
// 
// void StretchList::add(MuseFrame_t frame, double stretch, bool do_normalize)
//       {
// //       if (frame > MUSE_TIME_STRETCH_MAX_FRAME)
// //             frame = MUSE_TIME_STRETCH_MAX_FRAME;
// //       iStretchEvent e = upper_bound(frame);
// // 
// //       if (frame == e->second->_frame)
// //             e->second->_stretch = stretch;
// //       else {
// //             StretchEvent* ne = e->second;
// //             StretchEvent* ev = new StretchEvent(ne->_stretch, ne->_frame);
// //             ne->_stretch = stretch;
// //             ne->_frame = frame;
// //             insert(std::pair<const MuseFrame_t, StretchEvent*> (frame, ev));
// //             }
//             
// //       iStretchEvent e = find(frame);
// //       if(e != end())
// //         e->second._stretch = stretch;
// //       else
// //         insert(std::pair<const MuseFrame_t, StretchEvent> (frame, StretchEvent(stretch)));
// 
//       std::pair<iStretchEvent, bool> res = insert(std::pair<const MuseFrame_t, StretchEvent> (frame, StretchEvent(stretch)));
//       // Item already exists? Assign.
//       if(!res.second)
//         res.first->second._stretch = stretch;
//       
//       if(do_normalize)      
//         normalize();
//       }
// 
// 
// void StretchList::add(MuseFrame_t frame, const StretchEvent& e, bool do_normalize)
// {
//   //double stretch = e->_stretch;
//   double stretch = e._stretchRatio;
//   //std::pair<iStretchEvent, bool> res = insert(std::pair<const MuseFrame_t, StretchEvent*> (frame, e));
//   std::pair<iStretchEvent, bool> res = insert(std::pair<const MuseFrame_t, StretchEvent> (frame, e));
//   if(!res.second)
//   {
//     fprintf(stderr, "StretchList::add insert failed: stretchlist:%p stretch:%f frame:%ld\n", 
//                       this, stretch, frame);
//   }
//   else
//   {
// //     iStretchEvent ine = res.first;
// //     ++ine; // There is always a 'next' stretch event - there is always one at index MUSE_TIME_STRETCH_MAX_FRAME + 1.
// //     StretchEvent* ne = ine->second;
// //     
// //     // Swap the values. (This is how the stretch list works.)
// //     e->_stretch = ne->_stretch;
// //     e->_frame = ne->_frame;
// //     ne->_stretch = stretch;
// //     ne->_frame = frame;
//     
//     if(do_normalize)      
//       normalize();
//   }
// }
// 
// //---------------------------------------------------------
// //   addOperation
// //---------------------------------------------------------
// 
// void StretchList::addOperation(MuseFrame_t frame, double stretch, PendingOperationList& ops)
// {
// //   if (frame > MUSE_TIME_STRETCH_MAX_FRAME)
// //     frame = MUSE_TIME_STRETCH_MAX_FRAME;
// //   iStretchEvent e = upper_bound(frame);
// // 
// //   if(frame == e->second->_frame)
// //     ops.add(PendingOperationItem(this, e, stretch, PendingOperationItem::ModifyStretch));
// //   else 
// //   {
// //     PendingOperationItem poi(this, 0, frame, PendingOperationItem::AddStretch);
// //     iPendingOperation ipo = ops.findAllocationOp(poi);
// //     if(ipo != ops.end())
// //     {
// //       PendingOperationItem& poi = *ipo;
// //       // Simply replace the value.
// //       poi._stretch_event->_stretch = stretch;
// //     }
// //     else
// //     {
// //       poi._stretch_event = new StretchEvent(stretch, frame); // These are the desired frame and stretch but...
// //       ops.add(poi);                               //  add will do the proper swapping with next event.
// //     }
// //   }
//   
//   
//   iStretchEvent ie = find(frame);
//   if(ie != end())
//     ops.add(PendingOperationItem(this, ie, stretch, PendingOperationItem::ModifyStretch));
//   else
//     ops.add(PendingOperationItem(this, frame, stretch, PendingOperationItem::AddStretch));
// }
// 
// //---------------------------------------------------------
// //   normalize
// //---------------------------------------------------------
// 
// void StretchList::normalize()
// {
// //       double dtime;
// //       MuseFrame_t dframe;
// //       double newFrame = 0;
// //       _isStretched = false;
// //       for (iStretchEvent e = begin(); e != end(); ++e) {
// //             e->second->_finStretchedFrame = newFrame;
// //             // If ANY event has a stretch other than 1.0, the map is stretched, a stretcher must be engaged.
// //             if(e->second->_stretch != 1.0)
// //               _isStretched = true;
// //             dframe = e->first - e->second->_frame;
// //             dtime = double(dframe) / e->second->_stretch;
// //             newFrame += dtime;
// //             }
//         
//   double dtime;
//   MuseFrame_t dframe;
//   //double newFrame = 0;
//   
//   MuseFrame_t prevFrame;
//   double prevNewFrame;
//   double prevStretch = 1.0;
//   _isStretched = false;
//   for (iStretchEvent e = begin(); e != end(); ++e)
//   {
//     // If ANY event has a stretch other than 1.0, the map is stretched, a stretcher must be engaged.
//     if(e->second._stretch != 1.0)
//       _isStretched = true;
//     
//     if(e == begin())
//     {
//       prevFrame = prevNewFrame = e->second._finStretchedFrame = e->first;
//     }
//     else
//     {
//       dframe = e->first - prevFrame;
//       dtime = double(dframe) / prevStretch;
//       e->second._finStretchedFrame = prevNewFrame + dtime;
//       prevNewFrame = e->second._finStretchedFrame;
//       prevFrame = e->first;
//     }
// 
//     prevStretch = e->second._stretch;
//   }
//         
// }
// 
// //---------------------------------------------------------
// //   dump
// //---------------------------------------------------------
// 
// void StretchList::dump() const
//       {
//       fprintf(stderr, "\nStretchList:\n");
//       for (ciStretchEvent i = begin(); i != end(); ++i) {
//             fprintf(stderr, "%6ld Stretch %f newFrame %f\n",
//                i->first, i->second._stretch, i->second._finStretchedFrame);
//             }
//       }
// 
// //---------------------------------------------------------
// //   clear
// //---------------------------------------------------------
// 
// void StretchList::clear()
//       {
//       //for (iStretchEvent i = begin(); i != end(); ++i)
//       //      delete i->second;
//       STRETCHLIST::clear();
//       //insert(std::pair<const MuseFrame_t, StretchEvent*> (MUSE_TIME_STRETCH_MAX_FRAME+1, new StretchEvent(1.0, 0)));
//       
//       //++_tempoSN;
//       }
// 
// //---------------------------------------------------------
// //   eraseRange
// //---------------------------------------------------------
// 
// void StretchList::eraseRange(MuseFrame_t sframe, MuseFrame_t eframe)
// {
// //     if(sframe >= eframe || sframe > MUSE_TIME_STRETCH_MAX_FRAME)
// //       return;
// //     if(eframe > MUSE_TIME_STRETCH_MAX_FRAME)
// //       eframe = MUSE_TIME_STRETCH_MAX_FRAME;
// //     
// //     //iStretchEvent se = MusEGlobal::tempomap.upper_bound(stick); // TODO FIXME Hm, suspicious - fix this in tempo.cpp as well...
// //     iStretchEvent se = upper_bound(sframe);
// //     if(se == end() || (se->first == MUSE_TIME_STRETCH_MAX_FRAME+1))
// //       return;
// // 
// //     //iStretchEvent ee = MusEGlobal::tempomap.upper_bound(etick); // FIXME
// //     iStretchEvent ee = upper_bound(eframe);
// // 
// //     ee->second->_stretch = se->second->_stretch;
// //     ee->second->_frame = se->second->_frame;
// // 
// //     for(iStretchEvent ite = se; ite != ee; ++ite)
// //       delete ite->second;
// //     erase(se, ee); // Erase range does NOT include the last element.
// //     normalize();
// //     //++_tempoSN;
//     
//     
//     if(sframe >= eframe)
//       return;
//     iStretchEvent se = lower_bound(sframe);
//     if(se == end())
//       return;
//     iStretchEvent ee = upper_bound(eframe);
// 
//     erase(se, ee); // Erase range does NOT include the last element.
//     
//     normalize();
//     //++_tempoSN;
// }
//       
// //---------------------------------------------------------
// //   stretchAt
// //---------------------------------------------------------
// 
// double StretchList::stretchAt(MuseFrame_t frame) const
//       {
// //             ciStretchEvent i = upper_bound(frame);
// //             if (i == end()) {
// //                   fprintf(stderr, "stretchAt: no STRETCH at frame %ld,0x%lx\n", frame, frame);
// //                   return 1.0;
// //                   }
// //             return i->second._stretch;
//         
//             ciStretchEvent i = upper_bound(frame);
//             if(i == begin())
//               return 1.0;
//             --i;
//             return i->second._stretch;
//       }
// 
// //---------------------------------------------------------
// //   del
// //---------------------------------------------------------
// 
// void StretchList::del(MuseFrame_t frame, bool do_normalize)
//       {
//       iStretchEvent e = find(frame);
//       if (e == end()) {
//             fprintf(stderr, "StretchList::del(%ld): not found\n", frame);
//             return;
//             }
//       del(e, do_normalize);
//       //++_tempoSN;
//       }
// 
// void StretchList::del(iStretchEvent e, bool do_normalize)
//       {
// //       iStretchEvent ne = e;
// //       ++ne;
// //       if (ne == end()) {
// //             printf("StretchList::del() HALLO\n");
// //             return;
// //             }
// //       ne->second->_stretch = e->second->_stretch;
// //       ne->second->_frame  = e->second->_frame;
//       
//       erase(e);
//       if(do_normalize)
//         normalize();
//       //++_tempoSN;
//       }
// 
// //---------------------------------------------------------
// //   delOperation
// //---------------------------------------------------------
// 
// void StretchList::delOperation(MuseFrame_t frame, PendingOperationList& ops)
// {
//   iStretchEvent e = find(frame);
//   if (e == end()) {
//         printf("StretchList::delOperation frame:%ld not found\n", frame);
//         return;
//         }
//   PendingOperationItem poi(this, e, PendingOperationItem::DeleteStretch);
//   // NOTE: Deletion is done in post-RT stage 3.
//   ops.add(poi);
// }
// 
// //---------------------------------------------------------
// //   setStretch
// //---------------------------------------------------------
// 
// void StretchList::setStretch(MuseFrame_t frame, double newStretch)
//       {
//       //if (useList)
//             add(frame, newStretch);
//       //else
//       //      _tempo = newTempo;
//       //++_tempoSN;
//       }
// 
// //---------------------------------------------------------
// //   addTempo
// //---------------------------------------------------------
// 
// void StretchList::addStretch(MuseFrame_t frame, double stretch, bool do_normalize)
//       {
//       add(frame, stretch, do_normalize);
//       //++_tempoSN;
//       }
// 
// //---------------------------------------------------------
// //   delStretch
// //---------------------------------------------------------
// 
// void StretchList::delStretch(MuseFrame_t frame, bool do_normalize)
//       {
//       del(frame, do_normalize);
//       //++_tempoSN;
//       }
// 
// //---------------------------------------------------------
// //   stretch
// //---------------------------------------------------------
// 
// //unsigned StretchList::tick2frame(unsigned tick, int* sn) const
// double StretchList::stretch(MuseFrame_t frame) const
//       {
// //       double f;
// //       //if (useList)
// //       {
// //             ciStretchEvent i = upper_bound(frame);
// //             if (i == end()) {
// //                   fprintf(stderr, "StretchList::stretch(%ld,0x%lx): not found\n", frame, frame);
// //                   return 1.0;
// //                   }
// //             MuseFrame_t dframe = frame - i->second->_frame;
// //             //double dtime   = double(dtick) / (MusEGlobal::config.division * _globalTempo * 10000.0/ i->second->tempo);
// //             double dtime   = double(dframe) / i->second->_stretch;
// //             //unsigned dframe   = lrint(dtime * MusEGlobal::sampleRate);
// //             //MuseFrame_t dNewframe   = lrint(dtime);
// //             //f = i->second->_finStretchedFrame + dNewframe;
// //             f = i->second->_finStretchedFrame + dtime;
// //       }
// //       //else
// //       //{
// //       //      double t = (double(tick) * double(_tempo)) / (double(MusEGlobal::config.division) * _globalTempo * 10000.0);
// //       //      f = lrint(t * MusEGlobal::sampleRate);
// //       //}
// //       //if (sn)
// //       //      *sn = _tempoSN;
// //       return f;
//         
//         
//       double f;
//       MuseFrame_t prevFrame;
//       double prevNewFrame;
//       double prevStretch;
//       //if (useList)
//       {
//             ciStretchEvent i = upper_bound(frame);
// //             if (i == end()) {
// //                   fprintf(stderr, "StretchList::stretch(%ld,0x%lx): not found\n", frame, frame);
// //                   return 1.0;
// //                   }
//             
//             if(i == begin())
//               return frame;
//             
//             --i;
//             prevFrame = i->first;
//             prevNewFrame = i->second._finStretchedFrame;
//             prevStretch = i->second._stretch;
//             
//             //MuseFrame_t dframe = frame - i->second->_frame;
//             MuseFrame_t dframe = frame - prevFrame;
//             
//             //double dtime   = double(dtick) / (MusEGlobal::config.division * _globalTempo * 10000.0/ i->second->tempo);
//             
//             //double dtime   = double(dframe) / i->second->_stretch;
//             double dtime   = double(dframe) / prevStretch;
//             
//             //unsigned dframe   = lrint(dtime * MusEGlobal::sampleRate);
//             //MuseFrame_t dNewframe   = lrint(dtime);
//             //f = i->second->_finStretchedFrame + dNewframe;
//             
//             //f = i->second->_finStretchedFrame + dtime;
//             f = prevNewFrame + dtime;
//       }
//       //else
//       //{
//       //      double t = (double(tick) * double(_tempo)) / (double(MusEGlobal::config.division) * _globalTempo * 10000.0);
//       //      f = lrint(t * MusEGlobal::sampleRate);
//       //}
//       //if (sn)
//       //      *sn = _tempoSN;
//       return f;
//         
//       }
// 
// double StretchList::stretch(double frame) const
//       {
// //       const MuseFrame_t muse_frame = frame;
// //       double f;
// //       
// //       //if (useList)
// //       {
// //             ciStretchEvent i = upper_bound(muse_frame);
// //             if (i == end()) {
// //                   fprintf(stderr, "StretchList::stretch(%ld,0x%lx): not found\n", muse_frame, muse_frame);
// //                   return 1.0;
// //                   }
// //             double dframe = frame - (double)i->second->_frame;
// //             double dtime   = dframe / i->second->_stretch;
// //             f = i->second->_finStretchedFrame + dtime;
// //       }
// //       //else
// //       //{
// //       //      double t = (double(tick) * double(_tempo)) / (double(MusEGlobal::config.division) * _globalTempo * 10000.0);
// //       //      f = lrint(t * MusEGlobal::sampleRate);
// //       //}
// //       //if (sn)
// //       //      *sn = _tempoSN;
// //       return f;
//       
//       
//       
//       double f;
//       MuseFrame_t prevFrame;
//       double prevNewFrame;
//       double prevStretch;
//       //if (useList)
//       {
//             ciStretchEvent i = upper_bound(frame);
// //             if (i == end()) {
// //                   fprintf(stderr, "StretchList::stretch(%ld,0x%lx): not found\n", frame, frame);
// //                   return 1.0;
// //                   }
//             
//             if(i == begin())
//               return frame;
//             
//             --i;
//             prevFrame = i->first;
//             prevNewFrame = i->second._finStretchedFrame;
//             prevStretch = i->second._stretch;
//             
//             //MuseFrame_t dframe = frame - i->second->_frame;
//             
//             //MuseFrame_t dframe = frame - prevFrame;
//             double dframe = frame - (double)prevFrame;
//             
//             //double dtime   = double(dtick) / (MusEGlobal::config.division * _globalTempo * 10000.0/ i->second->tempo);
//             //double dtime   = double(dframe) / i->second->_stretch;
//             
//             //double dtime   = double(dframe) / prevStretch;
//             double dtime   = dframe / prevStretch;
//             
//             //unsigned dframe   = lrint(dtime * MusEGlobal::sampleRate);
//             //MuseFrame_t dNewframe   = lrint(dtime);
//             //f = i->second->_finStretchedFrame + dNewframe;
//             
//             //f = i->second->_finStretchedFrame + dtime;
//             f = prevNewFrame + dtime;
//       }
//       //else
//       //{
//       //      double t = (double(tick) * double(_tempo)) / (double(MusEGlobal::config.division) * _globalTempo * 10000.0);
//       //      f = lrint(t * MusEGlobal::sampleRate);
//       //}
//       //if (sn)
//       //      *sn = _tempoSN;
//       return f;
//       
//       }
// 
// //---------------------------------------------------------
// //   unStretch
// //---------------------------------------------------------
// 
// //unsigned StretchList::frame2tick(unsigned frame, int* sn) const
// //double StretchList::unStretch(MuseFrame_t frame) const
// MuseFrame_t StretchList::unStretch(double frame) const
//       {
//       //unsigned tick;
// //       MuseFrame_t uframe;
// //       //if (useList)
// //       {
// //             ciStretchEvent e;
// //             for (e = begin(); e != end();) {
// //                   ciStretchEvent ee = e;
// //                   ++ee;
// //                   if (ee == end())
// //                         break;
// //                   if (frame < ee->second->_finStretchedFrame)
// //                         break;
// //                   e = ee;
// //                   }
// //             //unsigned te  = e->second->tempo;
// //             double te  = e->second->_stretch;
// //             //int dframe   = frame - e->second->frame;
// //             //MuseFrame_t dframe   = frame - e->second->_finStretchedFrame;
// //             double dframe  = frame - e->second->_finStretchedFrame;
// //             //double dtime = double(dframe) / double(MusEGlobal::sampleRate);
// //             //double dtime = double(dframe);
// //             //tick         = e->second->tick + lrint(dtime * _globalTempo * MusEGlobal::config.division * 10000.0 / te);
// //             //uframe         = e->second->_frame + lrint(dtime * te);
// //             uframe         = e->second->_frame + lrint(dframe * te);
// //       }
// //       //else
// //       //      tick = lrint((double(frame)/double(MusEGlobal::sampleRate)) * _globalTempo * MusEGlobal::config.division * 10000.0 / double(_tempo));
// //       //if (sn)
// //       //      *sn = _tempoSN;
// //       //return tick;
// //       return uframe;
// 
//       if(empty())
//         return frame;
//         
//       MuseFrame_t prevFrame;
//       double prevNewFrame;
//       double prevStretch;
//       MuseFrame_t uframe;
//       //if (useList)
//       {
//             ciStretchEvent e;
// //             for (e = begin(); e != end();) 
//             for(e = begin(); e != end(); ++e) 
//             {
//               
//               
// //                   ciStretchEvent ee = e;
// //                   ++ee;
// //                   if (ee == end())
// //                         break;
// //                   if (frame < ee->second->_finStretchedFrame)
// //                         break;
// //                   e = ee;
//                   
//                   if (frame < e->second._finStretchedFrame)
//                         break;
//             }
//                   
//             if(e == begin())
//               return frame;
//                   
//             --e;
//             prevFrame = e->first;
//             prevNewFrame = e->second._finStretchedFrame;
//             prevStretch = e->second._stretch;
//             
//             //unsigned te  = e->second->tempo;
//             
//             //double te  = e->second->_stretch;
//             //double te  = prevStretch;
//             
//             //int dframe   = frame - e->second->frame;
//             //MuseFrame_t dframe   = frame - e->second->_finStretchedFrame;
//             
//             //double dframe  = frame - e->second->_finStretchedFrame;
//             double dframe  = frame - prevNewFrame;
//             
//             //double dtime = double(dframe) / double(MusEGlobal::sampleRate);
//             //double dtime = double(dframe);
//             //tick         = e->second->tick + lrint(dtime * _globalTempo * MusEGlobal::config.division * 10000.0 / te);
//             //uframe         = e->second->_frame + lrint(dtime * te);
//             
//             //uframe         = e->second->_frame + lrint(dframe * te);
//             uframe         = prevFrame + lrint(dframe * prevStretch);
//       }
//       //else
//       //      tick = lrint((double(frame)/double(MusEGlobal::sampleRate)) * _globalTempo * MusEGlobal::config.division * 10000.0 / double(_tempo));
//       //if (sn)
//       //      *sn = _tempoSN;
//       //return tick;
//       return uframe;
//         
//       }
// 
// //---------------------------------------------------------
// //   write
// //---------------------------------------------------------
// 
// void StretchList::write(int level, Xml& xml) const
// {
// //       //xml.put(level++, "<stretchlist fix=\"%d\">", _tempo);
// //       xml.put(level++, "<stretchlist>");
// //       //if (_globalTempo != 100)
// //       //      xml.intTag(level, "globalTempo", _globalTempo);
// //       for (ciStretchEvent i = begin(); i != end(); ++i)
// //             //i->second->write(level, xml, i->first);
// //             i->second.write(level, xml, i->first);
// //       xml.tag(level, "/stretchlist");
//       
//       
//   if(empty())
//     return;
//   
//   //for (ciStretchEvent ise = begin(); ise != end(); ++ise)
//   //{
//         //const CtrlList* cl = icl->second;
// 
//         QString s= QString("stretchlist");
//         //s += QString(" color=\"%1\" visible=\"%2\"").arg(cl->color().name()).arg(cl->isVisible());
//         xml.tag(level++, s.toLatin1().constData());
//         int i = 0;
//         for (ciStretchEvent ise = begin(); ise != end(); ++ise) {
//               QString s("%1 %2, ");
//               xml.nput(level, s.arg(ise->first).arg(ise->second._stretch).toLatin1().constData());
//               ++i;
//               if (i >= 4) {
//                     xml.put(level, "");
//                     i = 0;
//                     }
//               }
//         if (i)
//               xml.put(level, "");
//         xml.etag(level--, "stretchlist");
//   //}
//   
//       
// }
// 
// //---------------------------------------------------------
// //   read
// //---------------------------------------------------------
// 
// void StretchList::read(Xml& xml)
//       {
// //       for (;;) {
// //             Xml::Token token = xml.parse();
// //             const QString& tag = xml.s1();
// //             switch (token) {
// //                   case Xml::Error:
// //                   case Xml::End:
// //                   case Xml::Attribut: //
// //                         return;
// //                   case Xml::TagStart:
// //                         if (tag == "stretch") {
// // //                               StretchEvent* t = new StretchEvent();
// // //                               //unsigned tick = t->read(xml);
// // //                               MuseFrame_t frame = t->read(xml);
// // //                               iStretchEvent pos = find(frame);
// // //                               if (pos != end())
// // //                                     erase(pos);
// // //                               insert(std::pair<const MuseFrame_t, StretchEvent*> (frame, t));
// //                               
// //                               StretchEvent e;
// //                               MuseFrame_t frame = e.read(xml);
// //                               std::pair<iStretchEvent, bool> res = insert(std::pair<const MuseFrame_t, StretchEvent> (frame, e));
// //                               // Item already exists? Assign.
// //                               if(!res.second)
// //                                 res.first->second = e;
// //                               
// //                               }
// //                         //else if (tag == "globalTempo")
// //                         //      _globalTempo = xml.parseInt();
// //                         else
// //                               xml.unknown("StretchList");
// //                         break;
// //                   //case Xml::Attribut:
// //                   //      if (tag == "fix")
// //                   //            _tempo = xml.s2().toInt();
// //                   //      break;
// //                   case Xml::TagEnd:
// //                         if (tag == "stretchlist") {
// //                               normalize();
// //                               //++_tempoSN;
// //                               return;
// //                               }
// //                   default:
// //                         break;
// //                   }
// //             }
//         
//         
//       //QLocale loc = QLocale::c();
//       bool ok;
//       for (;;) {
//             Xml::Token token = xml.parse();
//             const QString& tag = xml.s1();
//             switch (token) {
//                   case Xml::Error:
//                   case Xml::End:
//                         return;
//                   case Xml::Attribut:
// //                         if (tag == "id")
// //                         {
// //                               _id = loc.toInt(xml.s2(), &ok);
// //                               if(!ok)
// //                                 printf("CtrlList::read failed reading _id string: %s\n", xml.s2().toLatin1().constData());
// //                         }
// //                         else if (tag == "cur")
// //                         {
// //                               _curVal = loc.toDouble(xml.s2(), &ok);
// //                               if(!ok)
// //                                 printf("CtrlList::read failed reading _curVal string: %s\n", xml.s2().toLatin1().constData());
// //                         }        
// //                         else if (tag == "visible")
// //                         {
// //                               _visible = loc.toInt(xml.s2(), &ok);
// //                               if(!ok)
// //                                 printf("CtrlList::read failed reading _visible string: %s\n", xml.s2().toLatin1().constData());
// //                         }
// //                         else if (tag == "color")
// //                         {
// // #if QT_VERSION >= 0x040700
// //                               ok = _displayColor.isValidColor(xml.s2());
// //                               if (!ok) {
// //                                 printf("CtrlList::read failed reading color string: %s\n", xml.s2().toLatin1().constData());
// //                                 break;
// //                               }
// // #endif
// //                               _displayColor.setNamedColor(xml.s2());
// //                         }
// //                         else
//                               fprintf(stderr, "stretchlist unknown tag %s\n", tag.toLatin1().constData());
//                         break;
//                   case Xml::Text:
//                         {
//                           int len = tag.length();
//                           //int frame;
//                           MuseFrame_t frame;
//                           double val;
//   
//                           int i = 0;
//                           for(;;) 
//                           {
//                                 while(i < len && (tag[i] == ',' || tag[i] == ' ' || tag[i] == '\n'))
//                                   ++i;
//                                 if(i == len)
//                                       break;
//                                 
//                                 QString fs;
//                                 while(i < len && tag[i] != ' ')
//                                 {
//                                   fs.append(tag[i]); 
//                                   ++i;
//                                 }
//                                 if(i == len)
//                                       break;
//                                 
//                                 //frame = loc.toInt(fs, &ok);
//                                 frame = fs.toLong(&ok);
//                                 if(!ok)
//                                 {
//                                   fprintf(stderr, "CtrlList::read failed reading frame string: %s\n", fs.toLatin1().constData());
//                                   break;
//                                 }
//                                   
//                                 while(i < len && (tag[i] == ' ' || tag[i] == '\n'))
//                                   ++i;
//                                 if(i == len)
//                                       break;
//                                 
//                                 QString vs;
//                                 while(i < len && tag[i] != ' ' && tag[i] != ',')
//                                 {
//                                   vs.append(tag[i]); 
//                                   ++i;
//                                 }
//                                 
//                                 //val = loc.toDouble(vs, &ok);
//                                 val = vs.toDouble(&ok);
//                                 if(!ok)
//                                 {
//                                   fprintf(stderr, "CtrlList::read failed reading value string: %s\n", vs.toLatin1().constData());
//                                   break;
//                                 }
//                                   
// // REMOVE Tim. samplerate. Changed.
//                                 add(frame, val, false); // Defer normalize until tag end.
//                                 // For now, the conversion only has a TEMPORARY effect during song loading.
//                                 // See comments in Song::read at the "samplerate" tag.
//                                 //add(MusEGlobal::convertFrame4ProjectSampleRate(frame), val);
//                                 
//                                 if(i == len)
//                                       break;
//                           }
//                         }
//                         break;
//                   case Xml::TagEnd:
//                         if (tag == "stretchlist")
//                         {
//                               normalize();
//                               return;
//                         }
//                   default:
//                         break;
//                   }
//             }
//         
//       }

// //---------------------------------------------------------
// //   StretchEvent::write
// //---------------------------------------------------------
// 
// void StretchEvent::write(int level, Xml& xml, MuseFrame_t at) const
//       {
//       //xml.tag(level++, "tempo at=\"%d\"", at);
//       xml.tag(level++, "stretch at=\"%ld\" val=\"%f\" >", at, _stretch);
//       //xml.intTag(level, "tick", tick);
// //       xml.longIntTag(level, "frame", _frame);
//       //xml.intTag(level, "val", tempo);
// //       xml.doubleTag(level, "val", _stretch);
//       //xml.tag(level, "/tempo");
// //       xml.tag(level, "/stretch");
//       
//       
//       
//       }
// 
// //---------------------------------------------------------
// //   StretchEvent::read
// //---------------------------------------------------------
// 
// MuseFrame_t StretchEvent::read(Xml& xml)
//       {
//       MuseFrame_t at = 0;
//       for (;;) {
//             Xml::Token token = xml.parse();
//             const QString& tag = xml.s1();
//             switch (token) {
//                   case Xml::Error:
//                   case Xml::End:
//                         return 0;
// //                   case Xml::TagStart:
// //                         if (tag == "frame")
// //                               _frame = xml.parseLongInt();
// //                         else 
// //                         if (tag == "val")
// //                               _stretch = xml.parseDouble();
// //                         else
// //                               xml.unknown("StretchEvent");
// //                         break;
//                   case Xml::TagStart:
//                         xml.unknown("stretch");
//                         break;
//                   case Xml::Attribut:
//                         if (tag == "at")
//                               at = xml.s2().toLong();
//                         if (tag == "val")
//                               _stretch = xml.s2().toDouble();
//                         break;
//                   case Xml::TagEnd:
//                         if (tag == "stretch") {
//                               return at;
//                               }
//                   default:
//                         break;
//                   }
//             }
//       return 0;
//       }





//======================================================

//---------------------------------------------------------
//   StretchList
//---------------------------------------------------------

// StretchList::StretchList()
//       {
//       _isStretched = false;
// //       _tempo   = 500000;
// //       insert(std::pair<const unsigned, TEvent*> (MAX_TICK+1, new TEvent(_tempo, 0)));
//       
//       //insert(std::pair<const MuseFrame_t, StretchEvent*> (MUSE_TIME_STRETCH_MAX_FRAME+1, new StretchEvent(1.0, 0)));
//       //insert(std::pair<const MuseFrame_t, StretchEvent*> (0, new StretchEvent(1.0, 0)));
//       
// //       _tempoSN     = 1;
// //       _globalTempo = 100;
// //       useList      = true;
//       }
// 
// StretchList::~StretchList()
//       {
//       //for (iStretchEvent i = begin(); i != end(); ++i)
//       //      delete i->second;
//       }
// 
// //---------------------------------------------------------
// //   add
// //---------------------------------------------------------
// 
// // void StretchList::add(MuseFrame_t frame, double stretch, bool do_normalize)
// //       {
// //       std::pair<iStretchEvent, bool> res = insert(std::pair<const MuseFrame_t, StretchEvent> (frame, StretchEvent(stretch)));
// //       // Item already exists? Assign.
// //       if(!res.second)
// //         res.first->second._stretch = stretch;
// //       
// //       if(do_normalize)      
// //         normalize();
// //       }
// void StretchList::add(MuseFrame_t frame, double stretch)
//       {
//       std::pair<iStretchEvent, bool> res = insert(std::pair<const MuseFrame_t, double> (frame, stretch));
//       // Item already exists? Assign.
//       if(!res.second)
//         res.first->second = stretch;
//       }
// 
// // void StretchList::add(MuseFrame_t frame, const StretchEvent& e, bool do_normalize)
// // {
// //   double stretch = e._stretch;
// //   std::pair<iStretchEvent, bool> res = insert(std::pair<const MuseFrame_t, StretchEvent> (frame, e));
// //   if(!res.second)
// //   {
// //     fprintf(stderr, "StretchList::add insert failed: stretchlist:%p stretch:%f frame:%ld\n", 
// //                       this, stretch, frame);
// //   }
// //   else
// //   {
// //     if(do_normalize)      
// //       normalize();
// //   }
// // }
// 
// //---------------------------------------------------------
// //   addOperation
// //---------------------------------------------------------
// 
// void StretchList::addOperation(MuseFrame_t frame, double stretch, PendingOperationList& ops)
// {
// //   if (frame > MUSE_TIME_STRETCH_MAX_FRAME)
// //     frame = MUSE_TIME_STRETCH_MAX_FRAME;
// //   iStretchEvent e = upper_bound(frame);
// // 
// //   if(frame == e->second->_frame)
// //     ops.add(PendingOperationItem(this, e, stretch, PendingOperationItem::ModifyStretch));
// //   else 
// //   {
// //     PendingOperationItem poi(this, 0, frame, PendingOperationItem::AddStretch);
// //     iPendingOperation ipo = ops.findAllocationOp(poi);
// //     if(ipo != ops.end())
// //     {
// //       PendingOperationItem& poi = *ipo;
// //       // Simply replace the value.
// //       poi._stretch_event->_stretch = stretch;
// //     }
// //     else
// //     {
// //       poi._stretch_event = new StretchEvent(stretch, frame); // These are the desired frame and stretch but...
// //       ops.add(poi);                               //  add will do the proper swapping with next event.
// //     }
// //   }
//   
//   
//   iStretchEvent ie = find(frame);
//   if(ie != end())
//     ops.add(PendingOperationItem(this, ie, stretch, PendingOperationItem::ModifyStretch));
//   else
//     ops.add(PendingOperationItem(this, frame, stretch, PendingOperationItem::AddStretch));
// }
// 
// //---------------------------------------------------------
// //   normalize
// //---------------------------------------------------------
// 
// // void StretchList::normalize()
// // {
// // //       double dtime;
// // //       MuseFrame_t dframe;
// // //       double newFrame = 0;
// // //       _isStretched = false;
// // //       for (iStretchEvent e = begin(); e != end(); ++e) {
// // //             e->second->_finStretchedFrame = newFrame;
// // //             // If ANY event has a stretch other than 1.0, the map is stretched, a stretcher must be engaged.
// // //             if(e->second->_stretch != 1.0)
// // //               _isStretched = true;
// // //             dframe = e->first - e->second->_frame;
// // //             dtime = double(dframe) / e->second->_stretch;
// // //             newFrame += dtime;
// // //             }
// //         
// //   double dtime;
// //   MuseFrame_t dframe;
// //   //double newFrame = 0;
// //   
// //   MuseFrame_t prevFrame;
// //   double prevNewFrame;
// //   double prevStretch = 1.0;
// //   _isStretched = false;
// //   for (iStretchEvent e = begin(); e != end(); ++e)
// //   {
// //     // If ANY event has a stretch other than 1.0, the map is stretched, a stretcher must be engaged.
// //     if(e->second._stretch != 1.0)
// //       _isStretched = true;
// //     
// //     if(e == begin())
// //     {
// //       prevFrame = prevNewFrame = e->second._finStretchedFrame = e->first;
// //     }
// //     else
// //     {
// //       dframe = e->first - prevFrame;
// //       dtime = double(dframe) / prevStretch;
// //       e->second._finStretchedFrame = prevNewFrame + dtime;
// //       prevNewFrame = e->second._finStretchedFrame;
// //       prevFrame = e->first;
// //     }
// // 
// //     prevStretch = e->second._stretch;
// //   }
// //         
// // }
// 
// //---------------------------------------------------------
// //   dump
// //---------------------------------------------------------
// 
// // void StretchList::dump() const
// //       {
// //       fprintf(stderr, "\nStretchList:\n");
// //       for (ciStretchEvent i = begin(); i != end(); ++i) {
// //             fprintf(stderr, "%6ld Stretch %f newFrame %f\n",
// //                i->first, i->second._stretch, i->second._finStretchedFrame);
// //             }
// //       }
// void StretchList::dump() const
//       {
//       fprintf(stderr, "\nStretchList:\n");
//       for (ciStretchEvent i = begin(); i != end(); ++i) {
//             fprintf(stderr, "%6ld Stretch %f\n",
//                i->first, i->second);
//             }
//       }
// 
// //---------------------------------------------------------
// //   clear
// //---------------------------------------------------------
// 
// void StretchList::clear()
//       {
//       //for (iStretchEvent i = begin(); i != end(); ++i)
//       //      delete i->second;
//       STRETCHLIST::clear();
//       //insert(std::pair<const MuseFrame_t, StretchEvent*> (MUSE_TIME_STRETCH_MAX_FRAME+1, new StretchEvent(1.0, 0)));
//       
//       //++_tempoSN;
//       }
// 
// //---------------------------------------------------------
// //   eraseRange
// //---------------------------------------------------------
// 
// // void StretchList::eraseRange(MuseFrame_t sframe, MuseFrame_t eframe)
// // {
// //     if(sframe >= eframe)
// //       return;
// //     iStretchEvent se = lower_bound(sframe);
// //     if(se == end())
// //       return;
// //     iStretchEvent ee = upper_bound(eframe);
// // 
// //     erase(se, ee); // Erase range does NOT include the last element.
// //     
// //     normalize();
// // }
// void StretchList::eraseRange(MuseFrame_t sframe, MuseFrame_t eframe)
// {
//     if(sframe >= eframe)
//       return;
//     iStretchEvent se = lower_bound(sframe);
//     if(se == end())
//       return;
//     iStretchEvent ee = upper_bound(eframe);
// 
//     erase(se, ee); // Erase range does NOT include the last element.
// }
//       
// //---------------------------------------------------------
// //   stretchAt
// //---------------------------------------------------------
// 
// // double StretchList::stretchAt(MuseFrame_t frame) const
// //       {
// // //             ciStretchEvent i = upper_bound(frame);
// // //             if (i == end()) {
// // //                   fprintf(stderr, "stretchAt: no STRETCH at frame %ld,0x%lx\n", frame, frame);
// // //                   return 1.0;
// // //                   }
// // //             return i->second._stretch;
// //         
// //             ciStretchEvent i = upper_bound(frame);
// //             if(i == begin())
// //               return 1.0;
// //             --i;
// //             return i->second._stretch;
// //       }
// double StretchList::stretchAt(MuseFrame_t frame) const
//       {
//             ciStretchEvent i = upper_bound(frame);
//             if(i == begin())
//               return 1.0;
//             --i;
//             return i->second;
//       }
// 
// //---------------------------------------------------------
// //   del
// //---------------------------------------------------------
// 
// // void StretchList::del(MuseFrame_t frame, bool do_normalize)
// //       {
// //       iStretchEvent e = find(frame);
// //       if (e == end()) {
// //             fprintf(stderr, "StretchList::del(%ld): not found\n", frame);
// //             return;
// //             }
// //       del(e, do_normalize);
// //       //++_tempoSN;
// //       }
// void StretchList::del(MuseFrame_t frame)
//       {
//       iStretchEvent e = find(frame);
//       if (e == end()) {
//             fprintf(stderr, "StretchList::del(%ld): not found\n", frame);
//             return;
//             }
//       del(e);
//       }
// 
// // void StretchList::del(iStretchEvent e, bool do_normalize)
// //       {
// // //       iStretchEvent ne = e;
// // //       ++ne;
// // //       if (ne == end()) {
// // //             printf("StretchList::del() HALLO\n");
// // //             return;
// // //             }
// // //       ne->second->_stretch = e->second->_stretch;
// // //       ne->second->_frame  = e->second->_frame;
// //       
// //       erase(e);
// //       if(do_normalize)
// //         normalize();
// //       //++_tempoSN;
// //       }
// void StretchList::del(iStretchEvent e)
//       {
//       erase(e);
//       }
// 
// //---------------------------------------------------------
// //   delOperation
// //---------------------------------------------------------
// 
// void StretchList::delOperation(MuseFrame_t frame, PendingOperationList& ops)
// {
//   iStretchEvent e = find(frame);
//   if (e == end()) {
//         printf("StretchList::delOperation frame:%ld not found\n", frame);
//         return;
//         }
//   PendingOperationItem poi(this, e, PendingOperationItem::DeleteStretch);
//   // NOTE: Deletion is done in post-RT stage 3.
//   ops.add(poi);
// }
// 
// //---------------------------------------------------------
// //   setStretch
// //---------------------------------------------------------
// 
// void StretchList::setStretch(MuseFrame_t frame, double newStretch)
//       {
//       //if (useList)
//             add(frame, newStretch);
//       //else
//       //      _tempo = newTempo;
//       //++_tempoSN;
//       }
// 
// //---------------------------------------------------------
// //   addTempo
// //---------------------------------------------------------
// 
// // void StretchList::addStretch(MuseFrame_t frame, double stretch, bool do_normalize)
// //       {
// //       add(frame, stretch, do_normalize);
// //       //++_tempoSN;
// //       }
// void StretchList::addStretch(MuseFrame_t frame, double stretch)
//       {
//       add(frame, stretch);
//       }
// 
// //---------------------------------------------------------
// //   delStretch
// //---------------------------------------------------------
// 
// // void StretchList::delStretch(MuseFrame_t frame, bool do_normalize)
// //       {
// //       del(frame, do_normalize);
// //       //++_tempoSN;
// //       }
// void StretchList::delStretch(MuseFrame_t frame)
//       {
//       del(frame);
//       }
// 
// //---------------------------------------------------------
// //   stretch
// //---------------------------------------------------------
// 
// // //unsigned StretchList::tick2frame(unsigned tick, int* sn) const
// // double StretchList::stretch(MuseFrame_t frame) const
// //       {
// // //       double f;
// // //       //if (useList)
// // //       {
// // //             ciStretchEvent i = upper_bound(frame);
// // //             if (i == end()) {
// // //                   fprintf(stderr, "StretchList::stretch(%ld,0x%lx): not found\n", frame, frame);
// // //                   return 1.0;
// // //                   }
// // //             MuseFrame_t dframe = frame - i->second->_frame;
// // //             //double dtime   = double(dtick) / (MusEGlobal::config.division * _globalTempo * 10000.0/ i->second->tempo);
// // //             double dtime   = double(dframe) / i->second->_stretch;
// // //             //unsigned dframe   = lrint(dtime * MusEGlobal::sampleRate);
// // //             //MuseFrame_t dNewframe   = lrint(dtime);
// // //             //f = i->second->_finStretchedFrame + dNewframe;
// // //             f = i->second->_finStretchedFrame + dtime;
// // //       }
// // //       //else
// // //       //{
// // //       //      double t = (double(tick) * double(_tempo)) / (double(MusEGlobal::config.division) * _globalTempo * 10000.0);
// // //       //      f = lrint(t * MusEGlobal::sampleRate);
// // //       //}
// // //       //if (sn)
// // //       //      *sn = _tempoSN;
// // //       return f;
// //         
// //         
// //       double f;
// //       MuseFrame_t prevFrame;
// //       double prevNewFrame;
// //       double prevStretch;
// //       //if (useList)
// //       {
// //             ciStretchEvent i = upper_bound(frame);
// // //             if (i == end()) {
// // //                   fprintf(stderr, "StretchList::stretch(%ld,0x%lx): not found\n", frame, frame);
// // //                   return 1.0;
// // //                   }
// //             
// //             if(i == begin())
// //               return frame;
// //             
// //             --i;
// //             prevFrame = i->first;
// //             prevNewFrame = i->second._finStretchedFrame;
// //             prevStretch = i->second._stretch;
// //             
// //             //MuseFrame_t dframe = frame - i->second->_frame;
// //             MuseFrame_t dframe = frame - prevFrame;
// //             
// //             //double dtime   = double(dtick) / (MusEGlobal::config.division * _globalTempo * 10000.0/ i->second->tempo);
// //             
// //             //double dtime   = double(dframe) / i->second->_stretch;
// //             double dtime   = double(dframe) / prevStretch;
// //             
// //             //unsigned dframe   = lrint(dtime * MusEGlobal::sampleRate);
// //             //MuseFrame_t dNewframe   = lrint(dtime);
// //             //f = i->second->_finStretchedFrame + dNewframe;
// //             
// //             //f = i->second->_finStretchedFrame + dtime;
// //             f = prevNewFrame + dtime;
// //       }
// //       //else
// //       //{
// //       //      double t = (double(tick) * double(_tempo)) / (double(MusEGlobal::config.division) * _globalTempo * 10000.0);
// //       //      f = lrint(t * MusEGlobal::sampleRate);
// //       //}
// //       //if (sn)
// //       //      *sn = _tempoSN;
// //       return f;
// //         
// //       }
// // 
// // double StretchList::stretch(double frame) const
// //       {
// // //       const MuseFrame_t muse_frame = frame;
// // //       double f;
// // //       
// // //       //if (useList)
// // //       {
// // //             ciStretchEvent i = upper_bound(muse_frame);
// // //             if (i == end()) {
// // //                   fprintf(stderr, "StretchList::stretch(%ld,0x%lx): not found\n", muse_frame, muse_frame);
// // //                   return 1.0;
// // //                   }
// // //             double dframe = frame - (double)i->second->_frame;
// // //             double dtime   = dframe / i->second->_stretch;
// // //             f = i->second->_finStretchedFrame + dtime;
// // //       }
// // //       //else
// // //       //{
// // //       //      double t = (double(tick) * double(_tempo)) / (double(MusEGlobal::config.division) * _globalTempo * 10000.0);
// // //       //      f = lrint(t * MusEGlobal::sampleRate);
// // //       //}
// // //       //if (sn)
// // //       //      *sn = _tempoSN;
// // //       return f;
// //       
// //       
// //       
// //       double f;
// //       MuseFrame_t prevFrame;
// //       double prevNewFrame;
// //       double prevStretch;
// //       //if (useList)
// //       {
// //             ciStretchEvent i = upper_bound(frame);
// // //             if (i == end()) {
// // //                   fprintf(stderr, "StretchList::stretch(%ld,0x%lx): not found\n", frame, frame);
// // //                   return 1.0;
// // //                   }
// //             
// //             if(i == begin())
// //               return frame;
// //             
// //             --i;
// //             prevFrame = i->first;
// //             prevNewFrame = i->second._finStretchedFrame;
// //             prevStretch = i->second._stretch;
// //             
// //             //MuseFrame_t dframe = frame - i->second->_frame;
// //             
// //             //MuseFrame_t dframe = frame - prevFrame;
// //             double dframe = frame - (double)prevFrame;
// //             
// //             //double dtime   = double(dtick) / (MusEGlobal::config.division * _globalTempo * 10000.0/ i->second->tempo);
// //             //double dtime   = double(dframe) / i->second->_stretch;
// //             
// //             //double dtime   = double(dframe) / prevStretch;
// //             double dtime   = dframe / prevStretch;
// //             
// //             //unsigned dframe   = lrint(dtime * MusEGlobal::sampleRate);
// //             //MuseFrame_t dNewframe   = lrint(dtime);
// //             //f = i->second->_finStretchedFrame + dNewframe;
// //             
// //             //f = i->second->_finStretchedFrame + dtime;
// //             f = prevNewFrame + dtime;
// //       }
// //       //else
// //       //{
// //       //      double t = (double(tick) * double(_tempo)) / (double(MusEGlobal::config.division) * _globalTempo * 10000.0);
// //       //      f = lrint(t * MusEGlobal::sampleRate);
// //       //}
// //       //if (sn)
// //       //      *sn = _tempoSN;
// //       return f;
// //       
// //       }
// // 
// // //---------------------------------------------------------
// // //   unStretch
// // //---------------------------------------------------------
// // 
// // //unsigned StretchList::frame2tick(unsigned frame, int* sn) const
// // //double StretchList::unStretch(MuseFrame_t frame) const
// // MuseFrame_t StretchList::unStretch(double frame) const
// //       {
// //       //unsigned tick;
// // //       MuseFrame_t uframe;
// // //       //if (useList)
// // //       {
// // //             ciStretchEvent e;
// // //             for (e = begin(); e != end();) {
// // //                   ciStretchEvent ee = e;
// // //                   ++ee;
// // //                   if (ee == end())
// // //                         break;
// // //                   if (frame < ee->second->_finStretchedFrame)
// // //                         break;
// // //                   e = ee;
// // //                   }
// // //             //unsigned te  = e->second->tempo;
// // //             double te  = e->second->_stretch;
// // //             //int dframe   = frame - e->second->frame;
// // //             //MuseFrame_t dframe   = frame - e->second->_finStretchedFrame;
// // //             double dframe  = frame - e->second->_finStretchedFrame;
// // //             //double dtime = double(dframe) / double(MusEGlobal::sampleRate);
// // //             //double dtime = double(dframe);
// // //             //tick         = e->second->tick + lrint(dtime * _globalTempo * MusEGlobal::config.division * 10000.0 / te);
// // //             //uframe         = e->second->_frame + lrint(dtime * te);
// // //             uframe         = e->second->_frame + lrint(dframe * te);
// // //       }
// // //       //else
// // //       //      tick = lrint((double(frame)/double(MusEGlobal::sampleRate)) * _globalTempo * MusEGlobal::config.division * 10000.0 / double(_tempo));
// // //       //if (sn)
// // //       //      *sn = _tempoSN;
// // //       //return tick;
// // //       return uframe;
// // 
// //       if(empty())
// //         return frame;
// //         
// //       MuseFrame_t prevFrame;
// //       double prevNewFrame;
// //       double prevStretch;
// //       MuseFrame_t uframe;
// //       //if (useList)
// //       {
// //             ciStretchEvent e;
// // //             for (e = begin(); e != end();) 
// //             for(e = begin(); e != end(); ++e) 
// //             {
// //               
// //               
// // //                   ciStretchEvent ee = e;
// // //                   ++ee;
// // //                   if (ee == end())
// // //                         break;
// // //                   if (frame < ee->second->_finStretchedFrame)
// // //                         break;
// // //                   e = ee;
// //                   
// //                   if (frame < e->second._finStretchedFrame)
// //                         break;
// //             }
// //                   
// //             if(e == begin())
// //               return frame;
// //                   
// //             --e;
// //             prevFrame = e->first;
// //             prevNewFrame = e->second._finStretchedFrame;
// //             prevStretch = e->second._stretch;
// //             
// //             //unsigned te  = e->second->tempo;
// //             
// //             //double te  = e->second->_stretch;
// //             //double te  = prevStretch;
// //             
// //             //int dframe   = frame - e->second->frame;
// //             //MuseFrame_t dframe   = frame - e->second->_finStretchedFrame;
// //             
// //             //double dframe  = frame - e->second->_finStretchedFrame;
// //             double dframe  = frame - prevNewFrame;
// //             
// //             //double dtime = double(dframe) / double(MusEGlobal::sampleRate);
// //             //double dtime = double(dframe);
// //             //tick         = e->second->tick + lrint(dtime * _globalTempo * MusEGlobal::config.division * 10000.0 / te);
// //             //uframe         = e->second->_frame + lrint(dtime * te);
// //             
// //             //uframe         = e->second->_frame + lrint(dframe * te);
// //             uframe         = prevFrame + lrint(dframe * prevStretch);
// //       }
// //       //else
// //       //      tick = lrint((double(frame)/double(MusEGlobal::sampleRate)) * _globalTempo * MusEGlobal::config.division * 10000.0 / double(_tempo));
// //       //if (sn)
// //       //      *sn = _tempoSN;
// //       //return tick;
// //       return uframe;
// //         
// //       }
// 
// //---------------------------------------------------------
// //   write
// //---------------------------------------------------------
// 
// // void StretchList::write(int level, Xml& xml) const
// // {
// // //       //xml.put(level++, "<stretchlist fix=\"%d\">", _tempo);
// // //       xml.put(level++, "<stretchlist>");
// // //       //if (_globalTempo != 100)
// // //       //      xml.intTag(level, "globalTempo", _globalTempo);
// // //       for (ciStretchEvent i = begin(); i != end(); ++i)
// // //             //i->second->write(level, xml, i->first);
// // //             i->second.write(level, xml, i->first);
// // //       xml.tag(level, "/stretchlist");
// //       
// //       
// //   if(empty())
// //     return;
// //   
// //   //for (ciStretchEvent ise = begin(); ise != end(); ++ise)
// //   //{
// //         //const CtrlList* cl = icl->second;
// // 
// //         QString s= QString("stretchlist");
// //         //s += QString(" color=\"%1\" visible=\"%2\"").arg(cl->color().name()).arg(cl->isVisible());
// //         xml.tag(level++, s.toLatin1().constData());
// //         int i = 0;
// //         for (ciStretchEvent ise = begin(); ise != end(); ++ise) {
// //               QString s("%1 %2, ");
// //               xml.nput(level, s.arg(ise->first).arg(ise->second._stretch).toLatin1().constData());
// //               ++i;
// //               if (i >= 4) {
// //                     xml.put(level, "");
// //                     i = 0;
// //                     }
// //               }
// //         if (i)
// //               xml.put(level, "");
// //         xml.etag(level--, "stretchlist");
// //   //}
// //   
// //       
// // }
// void StretchList::write(int level, Xml& xml) const
// {
// //       //xml.put(level++, "<stretchlist fix=\"%d\">", _tempo);
// //       xml.put(level++, "<stretchlist>");
// //       //if (_globalTempo != 100)
// //       //      xml.intTag(level, "globalTempo", _globalTempo);
// //       for (ciStretchEvent i = begin(); i != end(); ++i)
// //             //i->second->write(level, xml, i->first);
// //             i->second.write(level, xml, i->first);
// //       xml.tag(level, "/stretchlist");
//       
//       
//   if(empty())
//     return;
//   
//   //for (ciStretchEvent ise = begin(); ise != end(); ++ise)
//   //{
//         //const CtrlList* cl = icl->second;
// 
//         QString s= QString("stretchlist");
//         //s += QString(" color=\"%1\" visible=\"%2\"").arg(cl->color().name()).arg(cl->isVisible());
//         xml.tag(level++, s.toLatin1().constData());
//         int i = 0;
//         for (ciStretchEvent ise = begin(); ise != end(); ++ise) {
//               QString s("%1 %2, ");
//               xml.nput(level, s.arg(ise->first).arg(ise->second).toLatin1().constData());
//               ++i;
//               if (i >= 4) {
//                     xml.put(level, "");
//                     i = 0;
//                     }
//               }
//         if (i)
//               xml.put(level, "");
//         xml.etag(level--, "stretchlist");
//   //}
//   
//       
// }
// 
// //---------------------------------------------------------
// //   read
// //---------------------------------------------------------
// 
// // void StretchList::read(Xml& xml)
// //       {
// // //       for (;;) {
// // //             Xml::Token token = xml.parse();
// // //             const QString& tag = xml.s1();
// // //             switch (token) {
// // //                   case Xml::Error:
// // //                   case Xml::End:
// // //                   case Xml::Attribut: //
// // //                         return;
// // //                   case Xml::TagStart:
// // //                         if (tag == "stretch") {
// // // //                               StretchEvent* t = new StretchEvent();
// // // //                               //unsigned tick = t->read(xml);
// // // //                               MuseFrame_t frame = t->read(xml);
// // // //                               iStretchEvent pos = find(frame);
// // // //                               if (pos != end())
// // // //                                     erase(pos);
// // // //                               insert(std::pair<const MuseFrame_t, StretchEvent*> (frame, t));
// // //                               
// // //                               StretchEvent e;
// // //                               MuseFrame_t frame = e.read(xml);
// // //                               std::pair<iStretchEvent, bool> res = insert(std::pair<const MuseFrame_t, StretchEvent> (frame, e));
// // //                               // Item already exists? Assign.
// // //                               if(!res.second)
// // //                                 res.first->second = e;
// // //                               
// // //                               }
// // //                         //else if (tag == "globalTempo")
// // //                         //      _globalTempo = xml.parseInt();
// // //                         else
// // //                               xml.unknown("StretchList");
// // //                         break;
// // //                   //case Xml::Attribut:
// // //                   //      if (tag == "fix")
// // //                   //            _tempo = xml.s2().toInt();
// // //                   //      break;
// // //                   case Xml::TagEnd:
// // //                         if (tag == "stretchlist") {
// // //                               normalize();
// // //                               //++_tempoSN;
// // //                               return;
// // //                               }
// // //                   default:
// // //                         break;
// // //                   }
// // //             }
// //         
// //         
// //       //QLocale loc = QLocale::c();
// //       bool ok;
// //       for (;;) {
// //             Xml::Token token = xml.parse();
// //             const QString& tag = xml.s1();
// //             switch (token) {
// //                   case Xml::Error:
// //                   case Xml::End:
// //                         return;
// //                   case Xml::Attribut:
// // //                         if (tag == "id")
// // //                         {
// // //                               _id = loc.toInt(xml.s2(), &ok);
// // //                               if(!ok)
// // //                                 printf("CtrlList::read failed reading _id string: %s\n", xml.s2().toLatin1().constData());
// // //                         }
// // //                         else if (tag == "cur")
// // //                         {
// // //                               _curVal = loc.toDouble(xml.s2(), &ok);
// // //                               if(!ok)
// // //                                 printf("CtrlList::read failed reading _curVal string: %s\n", xml.s2().toLatin1().constData());
// // //                         }        
// // //                         else if (tag == "visible")
// // //                         {
// // //                               _visible = loc.toInt(xml.s2(), &ok);
// // //                               if(!ok)
// // //                                 printf("CtrlList::read failed reading _visible string: %s\n", xml.s2().toLatin1().constData());
// // //                         }
// // //                         else if (tag == "color")
// // //                         {
// // // #if QT_VERSION >= 0x040700
// // //                               ok = _displayColor.isValidColor(xml.s2());
// // //                               if (!ok) {
// // //                                 printf("CtrlList::read failed reading color string: %s\n", xml.s2().toLatin1().constData());
// // //                                 break;
// // //                               }
// // // #endif
// // //                               _displayColor.setNamedColor(xml.s2());
// // //                         }
// // //                         else
// //                               fprintf(stderr, "stretchlist unknown tag %s\n", tag.toLatin1().constData());
// //                         break;
// //                   case Xml::Text:
// //                         {
// //                           int len = tag.length();
// //                           //int frame;
// //                           MuseFrame_t frame;
// //                           double val;
// //   
// //                           int i = 0;
// //                           for(;;) 
// //                           {
// //                                 while(i < len && (tag[i] == ',' || tag[i] == ' ' || tag[i] == '\n'))
// //                                   ++i;
// //                                 if(i == len)
// //                                       break;
// //                                 
// //                                 QString fs;
// //                                 while(i < len && tag[i] != ' ')
// //                                 {
// //                                   fs.append(tag[i]); 
// //                                   ++i;
// //                                 }
// //                                 if(i == len)
// //                                       break;
// //                                 
// //                                 //frame = loc.toInt(fs, &ok);
// //                                 frame = fs.toLong(&ok);
// //                                 if(!ok)
// //                                 {
// //                                   fprintf(stderr, "CtrlList::read failed reading frame string: %s\n", fs.toLatin1().constData());
// //                                   break;
// //                                 }
// //                                   
// //                                 while(i < len && (tag[i] == ' ' || tag[i] == '\n'))
// //                                   ++i;
// //                                 if(i == len)
// //                                       break;
// //                                 
// //                                 QString vs;
// //                                 while(i < len && tag[i] != ' ' && tag[i] != ',')
// //                                 {
// //                                   vs.append(tag[i]); 
// //                                   ++i;
// //                                 }
// //                                 
// //                                 //val = loc.toDouble(vs, &ok);
// //                                 val = vs.toDouble(&ok);
// //                                 if(!ok)
// //                                 {
// //                                   fprintf(stderr, "CtrlList::read failed reading value string: %s\n", vs.toLatin1().constData());
// //                                   break;
// //                                 }
// //                                   
// // // REMOVE Tim. samplerate. Changed.
// //                                 add(frame, val, false); // Defer normalize until tag end.
// //                                 // For now, the conversion only has a TEMPORARY effect during song loading.
// //                                 // See comments in Song::read at the "samplerate" tag.
// //                                 //add(MusEGlobal::convertFrame4ProjectSampleRate(frame), val);
// //                                 
// //                                 if(i == len)
// //                                       break;
// //                           }
// //                         }
// //                         break;
// //                   case Xml::TagEnd:
// //                         if (tag == "stretchlist")
// //                         {
// //                               normalize();
// //                               return;
// //                         }
// //                   default:
// //                         break;
// //                   }
// //             }
// //         
// //       }
// void StretchList::read(Xml& xml)
//       {
// //       for (;;) {
// //             Xml::Token token = xml.parse();
// //             const QString& tag = xml.s1();
// //             switch (token) {
// //                   case Xml::Error:
// //                   case Xml::End:
// //                   case Xml::Attribut: //
// //                         return;
// //                   case Xml::TagStart:
// //                         if (tag == "stretch") {
// // //                               StretchEvent* t = new StretchEvent();
// // //                               //unsigned tick = t->read(xml);
// // //                               MuseFrame_t frame = t->read(xml);
// // //                               iStretchEvent pos = find(frame);
// // //                               if (pos != end())
// // //                                     erase(pos);
// // //                               insert(std::pair<const MuseFrame_t, StretchEvent*> (frame, t));
// //                               
// //                               StretchEvent e;
// //                               MuseFrame_t frame = e.read(xml);
// //                               std::pair<iStretchEvent, bool> res = insert(std::pair<const MuseFrame_t, StretchEvent> (frame, e));
// //                               // Item already exists? Assign.
// //                               if(!res.second)
// //                                 res.first->second = e;
// //                               
// //                               }
// //                         //else if (tag == "globalTempo")
// //                         //      _globalTempo = xml.parseInt();
// //                         else
// //                               xml.unknown("StretchList");
// //                         break;
// //                   //case Xml::Attribut:
// //                   //      if (tag == "fix")
// //                   //            _tempo = xml.s2().toInt();
// //                   //      break;
// //                   case Xml::TagEnd:
// //                         if (tag == "stretchlist") {
// //                               normalize();
// //                               //++_tempoSN;
// //                               return;
// //                               }
// //                   default:
// //                         break;
// //                   }
// //             }
//         
//         
//       //QLocale loc = QLocale::c();
//       bool ok;
//       for (;;) {
//             Xml::Token token = xml.parse();
//             const QString& tag = xml.s1();
//             switch (token) {
//                   case Xml::Error:
//                   case Xml::End:
//                         return;
//                   case Xml::Attribut:
// //                         if (tag == "id")
// //                         {
// //                               _id = loc.toInt(xml.s2(), &ok);
// //                               if(!ok)
// //                                 printf("CtrlList::read failed reading _id string: %s\n", xml.s2().toLatin1().constData());
// //                         }
// //                         else if (tag == "cur")
// //                         {
// //                               _curVal = loc.toDouble(xml.s2(), &ok);
// //                               if(!ok)
// //                                 printf("CtrlList::read failed reading _curVal string: %s\n", xml.s2().toLatin1().constData());
// //                         }        
// //                         else if (tag == "visible")
// //                         {
// //                               _visible = loc.toInt(xml.s2(), &ok);
// //                               if(!ok)
// //                                 printf("CtrlList::read failed reading _visible string: %s\n", xml.s2().toLatin1().constData());
// //                         }
// //                         else if (tag == "color")
// //                         {
// // #if QT_VERSION >= 0x040700
// //                               ok = _displayColor.isValidColor(xml.s2());
// //                               if (!ok) {
// //                                 printf("CtrlList::read failed reading color string: %s\n", xml.s2().toLatin1().constData());
// //                                 break;
// //                               }
// // #endif
// //                               _displayColor.setNamedColor(xml.s2());
// //                         }
// //                         else
//                               fprintf(stderr, "stretchlist unknown tag %s\n", tag.toLatin1().constData());
//                         break;
//                   case Xml::Text:
//                         {
//                           int len = tag.length();
//                           //int frame;
//                           MuseFrame_t frame;
//                           double val;
//   
//                           int i = 0;
//                           for(;;) 
//                           {
//                                 while(i < len && (tag[i] == ',' || tag[i] == ' ' || tag[i] == '\n'))
//                                   ++i;
//                                 if(i == len)
//                                       break;
//                                 
//                                 QString fs;
//                                 while(i < len && tag[i] != ' ')
//                                 {
//                                   fs.append(tag[i]); 
//                                   ++i;
//                                 }
//                                 if(i == len)
//                                       break;
//                                 
//                                 //frame = loc.toInt(fs, &ok);
//                                 frame = fs.toLong(&ok);
//                                 if(!ok)
//                                 {
//                                   fprintf(stderr, "CtrlList::read failed reading frame string: %s\n", fs.toLatin1().constData());
//                                   break;
//                                 }
//                                   
//                                 while(i < len && (tag[i] == ' ' || tag[i] == '\n'))
//                                   ++i;
//                                 if(i == len)
//                                       break;
//                                 
//                                 QString vs;
//                                 while(i < len && tag[i] != ' ' && tag[i] != ',')
//                                 {
//                                   vs.append(tag[i]); 
//                                   ++i;
//                                 }
//                                 
//                                 //val = loc.toDouble(vs, &ok);
//                                 val = vs.toDouble(&ok);
//                                 if(!ok)
//                                 {
//                                   fprintf(stderr, "CtrlList::read failed reading value string: %s\n", vs.toLatin1().constData());
//                                   break;
//                                 }
//                                   
// // REMOVE Tim. samplerate. Changed.
//                                 add(frame, val, false); // Defer normalize until tag end.
//                                 // For now, the conversion only has a TEMPORARY effect during song loading.
//                                 // See comments in Song::read at the "samplerate" tag.
//                                 //add(MusEGlobal::convertFrame4ProjectSampleRate(frame), val);
//                                 
//                                 if(i == len)
//                                       break;
//                           }
//                         }
//                         break;
//                   case Xml::TagEnd:
//                         if (tag == "stretchlist")
//                         {
//                           return;
//                         }
//                   default:
//                         break;
//                   }
//             }
//         
//       }
// 
// // //---------------------------------------------------------
// // //   StretchEvent::write
// // //---------------------------------------------------------
// // 
// // void StretchEvent::write(int level, Xml& xml, MuseFrame_t at) const
// //       {
// //       //xml.tag(level++, "tempo at=\"%d\"", at);
// //       xml.tag(level++, "stretch at=\"%ld\" val=\"%f\" >", at, _stretch);
// //       //xml.intTag(level, "tick", tick);
// // //       xml.longIntTag(level, "frame", _frame);
// //       //xml.intTag(level, "val", tempo);
// // //       xml.doubleTag(level, "val", _stretch);
// //       //xml.tag(level, "/tempo");
// // //       xml.tag(level, "/stretch");
// //       
// //       
// //       
// //       }
// // 
// // //---------------------------------------------------------
// // //   StretchEvent::read
// // //---------------------------------------------------------
// // 
// // MuseFrame_t StretchEvent::read(Xml& xml)
// //       {
// //       MuseFrame_t at = 0;
// //       for (;;) {
// //             Xml::Token token = xml.parse();
// //             const QString& tag = xml.s1();
// //             switch (token) {
// //                   case Xml::Error:
// //                   case Xml::End:
// //                         return 0;
// // //                   case Xml::TagStart:
// // //                         if (tag == "frame")
// // //                               _frame = xml.parseLongInt();
// // //                         else 
// // //                         if (tag == "val")
// // //                               _stretch = xml.parseDouble();
// // //                         else
// // //                               xml.unknown("StretchEvent");
// // //                         break;
// //                   case Xml::TagStart:
// //                         xml.unknown("stretch");
// //                         break;
// //                   case Xml::Attribut:
// //                         if (tag == "at")
// //                               at = xml.s2().toLong();
// //                         if (tag == "val")
// //                               _stretch = xml.s2().toDouble();
// //                         break;
// //                   case Xml::TagEnd:
// //                         if (tag == "stretch") {
// //                               return at;
// //                               }
// //                   default:
// //                         break;
// //                   }
// //             }
// //       return 0;
// //       }

#endif // USE_ALTERNATE_STRETCH_LIST






//---------------------------------------------------------
//   FrameStretchMap
//---------------------------------------------------------

// FrameStretchMap::FrameStretchMap()
//       {
//       _isStretched = false;
//       }
// 
// FrameStretchMap::~FrameStretchMap()
//       {
//       }
// 
// //---------------------------------------------------------
// //   add
// //---------------------------------------------------------
// 
// void FrameStretchMap::add(MuseFrame_t frame, double new_frame)
//       {
//       std::pair<iFrameStretch, bool> res = insert(std::pair<const MuseFrame_t, const double> (frame, new_frame));
//       // Item already exists? Assign.
//       if(!res.second)
//         res.first->second = new_frame;
//       }
// 
// //---------------------------------------------------------
// //   addOperation
// //---------------------------------------------------------
// 
// // void FrameStretchMap::addOperation(MuseFrame_t frame, double stretch, PendingOperationList& ops)
// // {
// //   iStretchEvent ie = find(frame);
// //   if(ie != end())
// //     ops.add(PendingOperationItem(this, ie, stretch, PendingOperationItem::ModifyStretch));
// //   else
// //     ops.add(PendingOperationItem(this, frame, stretch, PendingOperationItem::AddStretch));
// // }
// 
// //---------------------------------------------------------
// //   delOperation
// //---------------------------------------------------------
// 
// // void FrameStretchMap::delOperation(MuseFrame_t frame, PendingOperationList& ops)
// // {
// //   iStretchEvent e = find(frame);
// //   if (e == end()) {
// //         printf("FrameStretchMap::delOperation frame:%ld not found\n", frame);
// //         return;
// //         }
// //   PendingOperationItem poi(this, e, PendingOperationItem::DeleteStretch);
// //   // NOTE: Deletion is done in post-RT stage 3.
// //   ops.add(poi);
// // }
// 
// //---------------------------------------------------------
// //   normalize
// //---------------------------------------------------------
// 
// void FrameStretchMap::normalize()
// {
// //       double dtime;
// //       MuseFrame_t dframe;
// //       double newFrame = 0;
// //       _isStretched = false;
// //       for (iStretchEvent e = begin(); e != end(); ++e) {
// //             e->second->_finStretchedFrame = newFrame;
// //             // If ANY event has a stretch other than 1.0, the map is stretched, a stretcher must be engaged.
// //             if(e->second->_stretch != 1.0)
// //               _isStretched = true;
// //             dframe = e->first - e->second->_frame;
// //             dtime = double(dframe) / e->second->_stretch;
// //             newFrame += dtime;
// //             }
//         
//   double dtime;
//   MuseFrame_t dframe;
//   //double newFrame = 0;
//   
//   MuseFrame_t prevFrame;
//   double prevNewFrame;
//   double prevStretch = 1.0;
//   _isStretched = false;
//   for (iStretchEvent e = begin(); e != end(); ++e)
//   {
//     // If ANY event has a stretch other than 1.0, the map is stretched, a stretcher must be engaged.
//     if(e->second._stretch != 1.0)
//       _isStretched = true;
//     
//     if(e == begin())
//     {
//       prevFrame = prevNewFrame = e->second._finStretchedFrame = e->first;
//     }
//     else
//     {
//       dframe = e->first - prevFrame;
//       dtime = double(dframe) / prevStretch;
//       e->second._finStretchedFrame = prevNewFrame + dtime;
//       prevNewFrame = e->second._finStretchedFrame;
//       prevFrame = e->first;
//     }
// 
//     prevStretch = e->second._stretch;
//   }
//         
// }
// 
// //---------------------------------------------------------
// //   dump
// //---------------------------------------------------------
// 
// void FrameStretchMap::dump() const
//       {
//       fprintf(stderr, "\nFrameStretchMap:\n");
//       for (ciFrameStretch i = begin(); i != end(); ++i) {
//             fprintf(stderr, "Frame:%6ld new frame:%f\n",
//                i->first, i->second);
//             }
//       }
// 
// //---------------------------------------------------------
// //   clear
// //---------------------------------------------------------
// 
// void FrameStretchMap::clear()
//       {
//       //for (iStretchEvent i = begin(); i != end(); ++i)
//       //      delete i->second;
//       FRAME_STRETCH_MAP::clear();
//       //insert(std::pair<const MuseFrame_t, StretchEvent*> (MUSE_TIME_STRETCH_MAX_FRAME+1, new StretchEvent(1.0, 0)));
//       
//       //++_tempoSN;
//       }
// 
// //---------------------------------------------------------
// //   eraseRange
// //---------------------------------------------------------
// 
// void FrameStretchMap::eraseRange(MuseFrame_t sframe, MuseFrame_t eframe)
// {
//     if(sframe >= eframe)
//       return;
//     iFrameStretch se = lower_bound(sframe);
//     if(se == end())
//       return;
//     iFrameStretch ee = upper_bound(eframe);
// 
//     erase(se, ee); // Erase range does NOT include the last element.
//     
//     normalize();
// }
//       
// //---------------------------------------------------------
// //   frameAt
// //---------------------------------------------------------
// 
// double FrameStretchMap::frameAt(MuseFrame_t frame) const
//       {
//             ciFrameStretch i = upper_bound(frame);
//             if(i == begin())
//               return 1.0;
//             --i;
//             return i->second;
//       }
// 
// //---------------------------------------------------------
// //   del
// //---------------------------------------------------------
// 
// void FrameStretchMap::del(MuseFrame_t frame, bool do_normalize)
//       {
//       iFrameStretch e = find(frame);
//       if (e == end()) {
//             fprintf(stderr, "FrameStretchMap::del(%ld): not found\n", frame);
//             return;
//             }
//       del(e, do_normalize);
//       }
// 
// void FrameStretchMap::del(iFrameStretch e, bool do_normalize)
//       {
//       erase(e);
//       if(do_normalize)
//         normalize();
//       }
// 
// //---------------------------------------------------------
// //   setFrame
// //---------------------------------------------------------
// 
// void FrameStretchMap::setFrame(MuseFrame_t frame, double newFrame)
//       {
//       //if (useList)
//             add(frame, newFrame);
//       //else
//       //      _tempo = newTempo;
//       }
// 
// //---------------------------------------------------------
// //   addFrame
// //---------------------------------------------------------
// 
// void FrameStretchMap::addFrame(MuseFrame_t frame, double newFrame, bool do_normalize)
//       {
//       add(frame, newFrame, do_normalize);
//       //++_tempoSN;
//       }
// 
// //---------------------------------------------------------
// //   delStretch
// //---------------------------------------------------------
// 
// void FrameStretchMap::delFrame(MuseFrame_t frame, bool do_normalize)
//       {
//       del(frame, do_normalize);
//       //++_tempoSN;
//       }
// 
// //---------------------------------------------------------
// //   stretch
// //---------------------------------------------------------
// 
// //unsigned FrameStretchMap::tick2frame(unsigned tick, int* sn) const
// double FrameStretchMap::stretch(MuseFrame_t frame) const
//       {
// //       double f;
// //       //if (useList)
// //       {
// //             ciStretchEvent i = upper_bound(frame);
// //             if (i == end()) {
// //                   fprintf(stderr, "FrameStretchMap::stretch(%ld,0x%lx): not found\n", frame, frame);
// //                   return 1.0;
// //                   }
// //             MuseFrame_t dframe = frame - i->second->_frame;
// //             //double dtime   = double(dtick) / (MusEGlobal::config.division * _globalTempo * 10000.0/ i->second->tempo);
// //             double dtime   = double(dframe) / i->second->_stretch;
// //             //unsigned dframe   = lrint(dtime * MusEGlobal::sampleRate);
// //             //MuseFrame_t dNewframe   = lrint(dtime);
// //             //f = i->second->_finStretchedFrame + dNewframe;
// //             f = i->second->_finStretchedFrame + dtime;
// //       }
// //       //else
// //       //{
// //       //      double t = (double(tick) * double(_tempo)) / (double(MusEGlobal::config.division) * _globalTempo * 10000.0);
// //       //      f = lrint(t * MusEGlobal::sampleRate);
// //       //}
// //       //if (sn)
// //       //      *sn = _tempoSN;
// //       return f;
//         
//         
//       double f;
//       MuseFrame_t prevFrame;
//       double prevNewFrame;
//       double prevStretch;
//       //if (useList)
//       {
//             ciFrameStretch i = upper_bound(frame);
// //             if (i == end()) {
// //                   fprintf(stderr, "FrameStretchMap::stretch(%ld,0x%lx): not found\n", frame, frame);
// //                   return 1.0;
// //                   }
//             
//             if(i == begin())
//               return frame;
//             
//             --i;
//             prevFrame = i->first;
//             prevNewFrame = i->second._finStretchedFrame;
//             prevStretch = i->second._stretch;
//             
//             //MuseFrame_t dframe = frame - i->second->_frame;
//             MuseFrame_t dframe = frame - prevFrame;
//             
//             //double dtime   = double(dtick) / (MusEGlobal::config.division * _globalTempo * 10000.0/ i->second->tempo);
//             
//             //double dtime   = double(dframe) / i->second->_stretch;
//             double dtime   = double(dframe) / prevStretch;
//             
//             //unsigned dframe   = lrint(dtime * MusEGlobal::sampleRate);
//             //MuseFrame_t dNewframe   = lrint(dtime);
//             //f = i->second->_finStretchedFrame + dNewframe;
//             
//             //f = i->second->_finStretchedFrame + dtime;
//             f = prevNewFrame + dtime;
//       }
//       //else
//       //{
//       //      double t = (double(tick) * double(_tempo)) / (double(MusEGlobal::config.division) * _globalTempo * 10000.0);
//       //      f = lrint(t * MusEGlobal::sampleRate);
//       //}
//       //if (sn)
//       //      *sn = _tempoSN;
//       return f;
//         
//       }
// 
// double FrameStretchMap::stretch(double frame) const
//       {
// //       const MuseFrame_t muse_frame = frame;
// //       double f;
// //       
// //       //if (useList)
// //       {
// //             ciStretchEvent i = upper_bound(muse_frame);
// //             if (i == end()) {
// //                   fprintf(stderr, "FrameStretchMap::stretch(%ld,0x%lx): not found\n", muse_frame, muse_frame);
// //                   return 1.0;
// //                   }
// //             double dframe = frame - (double)i->second->_frame;
// //             double dtime   = dframe / i->second->_stretch;
// //             f = i->second->_finStretchedFrame + dtime;
// //       }
// //       //else
// //       //{
// //       //      double t = (double(tick) * double(_tempo)) / (double(MusEGlobal::config.division) * _globalTempo * 10000.0);
// //       //      f = lrint(t * MusEGlobal::sampleRate);
// //       //}
// //       //if (sn)
// //       //      *sn = _tempoSN;
// //       return f;
//       
//       
//       
//       double f;
//       MuseFrame_t prevFrame;
//       double prevNewFrame;
//       double prevStretch;
//       //if (useList)
//       {
//             ciStretchEvent i = upper_bound(frame);
// //             if (i == end()) {
// //                   fprintf(stderr, "FrameStretchMap::stretch(%ld,0x%lx): not found\n", frame, frame);
// //                   return 1.0;
// //                   }
//             
//             if(i == begin())
//               return frame;
//             
//             --i;
//             prevFrame = i->first;
//             prevNewFrame = i->second._finStretchedFrame;
//             prevStretch = i->second._stretch;
//             
//             //MuseFrame_t dframe = frame - i->second->_frame;
//             
//             //MuseFrame_t dframe = frame - prevFrame;
//             double dframe = frame - (double)prevFrame;
//             
//             //double dtime   = double(dtick) / (MusEGlobal::config.division * _globalTempo * 10000.0/ i->second->tempo);
//             //double dtime   = double(dframe) / i->second->_stretch;
//             
//             //double dtime   = double(dframe) / prevStretch;
//             double dtime   = dframe / prevStretch;
//             
//             //unsigned dframe   = lrint(dtime * MusEGlobal::sampleRate);
//             //MuseFrame_t dNewframe   = lrint(dtime);
//             //f = i->second->_finStretchedFrame + dNewframe;
//             
//             //f = i->second->_finStretchedFrame + dtime;
//             f = prevNewFrame + dtime;
//       }
//       //else
//       //{
//       //      double t = (double(tick) * double(_tempo)) / (double(MusEGlobal::config.division) * _globalTempo * 10000.0);
//       //      f = lrint(t * MusEGlobal::sampleRate);
//       //}
//       //if (sn)
//       //      *sn = _tempoSN;
//       return f;
//       
//       }
// 
// //---------------------------------------------------------
// //   unStretch
// //---------------------------------------------------------
// 
// //unsigned FrameStretchMap::frame2tick(unsigned frame, int* sn) const
// //double FrameStretchMap::unStretch(MuseFrame_t frame) const
// MuseFrame_t FrameStretchMap::unStretch(double frame) const
//       {
//       //unsigned tick;
// //       MuseFrame_t uframe;
// //       //if (useList)
// //       {
// //             ciStretchEvent e;
// //             for (e = begin(); e != end();) {
// //                   ciStretchEvent ee = e;
// //                   ++ee;
// //                   if (ee == end())
// //                         break;
// //                   if (frame < ee->second->_finStretchedFrame)
// //                         break;
// //                   e = ee;
// //                   }
// //             //unsigned te  = e->second->tempo;
// //             double te  = e->second->_stretch;
// //             //int dframe   = frame - e->second->frame;
// //             //MuseFrame_t dframe   = frame - e->second->_finStretchedFrame;
// //             double dframe  = frame - e->second->_finStretchedFrame;
// //             //double dtime = double(dframe) / double(MusEGlobal::sampleRate);
// //             //double dtime = double(dframe);
// //             //tick         = e->second->tick + lrint(dtime * _globalTempo * MusEGlobal::config.division * 10000.0 / te);
// //             //uframe         = e->second->_frame + lrint(dtime * te);
// //             uframe         = e->second->_frame + lrint(dframe * te);
// //       }
// //       //else
// //       //      tick = lrint((double(frame)/double(MusEGlobal::sampleRate)) * _globalTempo * MusEGlobal::config.division * 10000.0 / double(_tempo));
// //       //if (sn)
// //       //      *sn = _tempoSN;
// //       //return tick;
// //       return uframe;
// 
//       if(empty())
//         return frame;
//         
//       MuseFrame_t prevFrame;
//       double prevNewFrame;
//       double prevStretch;
//       MuseFrame_t uframe;
//       //if (useList)
//       {
//             ciStretchEvent e;
// //             for (e = begin(); e != end();) 
//             for(e = begin(); e != end(); ++e) 
//             {
//               
//               
// //                   ciStretchEvent ee = e;
// //                   ++ee;
// //                   if (ee == end())
// //                         break;
// //                   if (frame < ee->second->_finStretchedFrame)
// //                         break;
// //                   e = ee;
//                   
//                   if (frame < e->second._finStretchedFrame)
//                         break;
//             }
//                   
//             if(e == begin())
//               return frame;
//                   
//             --e;
//             prevFrame = e->first;
//             prevNewFrame = e->second._finStretchedFrame;
//             prevStretch = e->second._stretch;
//             
//             //unsigned te  = e->second->tempo;
//             
//             //double te  = e->second->_stretch;
//             //double te  = prevStretch;
//             
//             //int dframe   = frame - e->second->frame;
//             //MuseFrame_t dframe   = frame - e->second->_finStretchedFrame;
//             
//             //double dframe  = frame - e->second->_finStretchedFrame;
//             double dframe  = frame - prevNewFrame;
//             
//             //double dtime = double(dframe) / double(MusEGlobal::sampleRate);
//             //double dtime = double(dframe);
//             //tick         = e->second->tick + lrint(dtime * _globalTempo * MusEGlobal::config.division * 10000.0 / te);
//             //uframe         = e->second->_frame + lrint(dtime * te);
//             
//             //uframe         = e->second->_frame + lrint(dframe * te);
//             uframe         = prevFrame + lrint(dframe * prevStretch);
//       }
//       //else
//       //      tick = lrint((double(frame)/double(MusEGlobal::sampleRate)) * _globalTempo * MusEGlobal::config.division * 10000.0 / double(_tempo));
//       //if (sn)
//       //      *sn = _tempoSN;
//       //return tick;
//       return uframe;
//         
//       }
// 
// //---------------------------------------------------------
// //   write
// //---------------------------------------------------------
// 
// void FrameStretchMap::write(int level, Xml& xml) const
// {
// //       //xml.put(level++, "<stretchlist fix=\"%d\">", _tempo);
// //       xml.put(level++, "<stretchlist>");
// //       //if (_globalTempo != 100)
// //       //      xml.intTag(level, "globalTempo", _globalTempo);
// //       for (ciStretchEvent i = begin(); i != end(); ++i)
// //             //i->second->write(level, xml, i->first);
// //             i->second.write(level, xml, i->first);
// //       xml.tag(level, "/stretchlist");
//       
//       
//   if(empty())
//     return;
//   
//   //for (ciStretchEvent ise = begin(); ise != end(); ++ise)
//   //{
//         //const CtrlList* cl = icl->second;
// 
//         QString s= QString("stretchlist");
//         //s += QString(" color=\"%1\" visible=\"%2\"").arg(cl->color().name()).arg(cl->isVisible());
//         xml.tag(level++, s.toLatin1().constData());
//         int i = 0;
//         for (ciStretchEvent ise = begin(); ise != end(); ++ise) {
//               QString s("%1 %2, ");
//               xml.nput(level, s.arg(ise->first).arg(ise->second._stretch).toLatin1().constData());
//               ++i;
//               if (i >= 4) {
//                     xml.put(level, "");
//                     i = 0;
//                     }
//               }
//         if (i)
//               xml.put(level, "");
//         xml.etag(level--, "stretchlist");
//   //}
//   
//       
// }
// 
// //---------------------------------------------------------
// //   read
// //---------------------------------------------------------
// 
// void FrameStretchMap::read(Xml& xml)
//       {
// //       for (;;) {
// //             Xml::Token token = xml.parse();
// //             const QString& tag = xml.s1();
// //             switch (token) {
// //                   case Xml::Error:
// //                   case Xml::End:
// //                   case Xml::Attribut: //
// //                         return;
// //                   case Xml::TagStart:
// //                         if (tag == "stretch") {
// // //                               StretchEvent* t = new StretchEvent();
// // //                               //unsigned tick = t->read(xml);
// // //                               MuseFrame_t frame = t->read(xml);
// // //                               iStretchEvent pos = find(frame);
// // //                               if (pos != end())
// // //                                     erase(pos);
// // //                               insert(std::pair<const MuseFrame_t, StretchEvent*> (frame, t));
// //                               
// //                               StretchEvent e;
// //                               MuseFrame_t frame = e.read(xml);
// //                               std::pair<iStretchEvent, bool> res = insert(std::pair<const MuseFrame_t, StretchEvent> (frame, e));
// //                               // Item already exists? Assign.
// //                               if(!res.second)
// //                                 res.first->second = e;
// //                               
// //                               }
// //                         //else if (tag == "globalTempo")
// //                         //      _globalTempo = xml.parseInt();
// //                         else
// //                               xml.unknown("StretchList");
// //                         break;
// //                   //case Xml::Attribut:
// //                   //      if (tag == "fix")
// //                   //            _tempo = xml.s2().toInt();
// //                   //      break;
// //                   case Xml::TagEnd:
// //                         if (tag == "stretchlist") {
// //                               normalize();
// //                               //++_tempoSN;
// //                               return;
// //                               }
// //                   default:
// //                         break;
// //                   }
// //             }
//         
//         
//       //QLocale loc = QLocale::c();
//       bool ok;
//       for (;;) {
//             Xml::Token token = xml.parse();
//             const QString& tag = xml.s1();
//             switch (token) {
//                   case Xml::Error:
//                   case Xml::End:
//                         return;
//                   case Xml::Attribut:
// //                         if (tag == "id")
// //                         {
// //                               _id = loc.toInt(xml.s2(), &ok);
// //                               if(!ok)
// //                                 printf("CtrlList::read failed reading _id string: %s\n", xml.s2().toLatin1().constData());
// //                         }
// //                         else if (tag == "cur")
// //                         {
// //                               _curVal = loc.toDouble(xml.s2(), &ok);
// //                               if(!ok)
// //                                 printf("CtrlList::read failed reading _curVal string: %s\n", xml.s2().toLatin1().constData());
// //                         }        
// //                         else if (tag == "visible")
// //                         {
// //                               _visible = loc.toInt(xml.s2(), &ok);
// //                               if(!ok)
// //                                 printf("CtrlList::read failed reading _visible string: %s\n", xml.s2().toLatin1().constData());
// //                         }
// //                         else if (tag == "color")
// //                         {
// // #if QT_VERSION >= 0x040700
// //                               ok = _displayColor.isValidColor(xml.s2());
// //                               if (!ok) {
// //                                 printf("CtrlList::read failed reading color string: %s\n", xml.s2().toLatin1().constData());
// //                                 break;
// //                               }
// // #endif
// //                               _displayColor.setNamedColor(xml.s2());
// //                         }
// //                         else
//                               fprintf(stderr, "stretchlist unknown tag %s\n", tag.toLatin1().constData());
//                         break;
//                   case Xml::Text:
//                         {
//                           int len = tag.length();
//                           //int frame;
//                           MuseFrame_t frame;
//                           double val;
//   
//                           int i = 0;
//                           for(;;) 
//                           {
//                                 while(i < len && (tag[i] == ',' || tag[i] == ' ' || tag[i] == '\n'))
//                                   ++i;
//                                 if(i == len)
//                                       break;
//                                 
//                                 QString fs;
//                                 while(i < len && tag[i] != ' ')
//                                 {
//                                   fs.append(tag[i]); 
//                                   ++i;
//                                 }
//                                 if(i == len)
//                                       break;
//                                 
//                                 //frame = loc.toInt(fs, &ok);
//                                 frame = fs.toLong(&ok);
//                                 if(!ok)
//                                 {
//                                   fprintf(stderr, "CtrlList::read failed reading frame string: %s\n", fs.toLatin1().constData());
//                                   break;
//                                 }
//                                   
//                                 while(i < len && (tag[i] == ' ' || tag[i] == '\n'))
//                                   ++i;
//                                 if(i == len)
//                                       break;
//                                 
//                                 QString vs;
//                                 while(i < len && tag[i] != ' ' && tag[i] != ',')
//                                 {
//                                   vs.append(tag[i]); 
//                                   ++i;
//                                 }
//                                 
//                                 //val = loc.toDouble(vs, &ok);
//                                 val = vs.toDouble(&ok);
//                                 if(!ok)
//                                 {
//                                   fprintf(stderr, "CtrlList::read failed reading value string: %s\n", vs.toLatin1().constData());
//                                   break;
//                                 }
//                                   
// // REMOVE Tim. samplerate. Changed.
//                                 add(frame, val, false); // Defer normalize until tag end.
//                                 // For now, the conversion only has a TEMPORARY effect during song loading.
//                                 // See comments in Song::read at the "samplerate" tag.
//                                 //add(MusEGlobal::convertFrame4ProjectSampleRate(frame), val);
//                                 
//                                 if(i == len)
//                                       break;
//                           }
//                         }
//                         break;
//                   case Xml::TagEnd:
//                         if (tag == "stretchlist")
//                         {
//                               normalize();
//                               return;
//                         }
//                   default:
//                         break;
//                   }
//             }
//         
//       }
// 
// // //---------------------------------------------------------
// // //   StretchEvent::write
// // //---------------------------------------------------------
// // 
// // void StretchEvent::write(int level, Xml& xml, MuseFrame_t at) const
// //       {
// //       //xml.tag(level++, "tempo at=\"%d\"", at);
// //       xml.tag(level++, "stretch at=\"%ld\" val=\"%f\" >", at, _stretch);
// //       //xml.intTag(level, "tick", tick);
// // //       xml.longIntTag(level, "frame", _frame);
// //       //xml.intTag(level, "val", tempo);
// // //       xml.doubleTag(level, "val", _stretch);
// //       //xml.tag(level, "/tempo");
// // //       xml.tag(level, "/stretch");
// //       
// //       
// //       
// //       }
// // 
// // //---------------------------------------------------------
// // //   StretchEvent::read
// // //---------------------------------------------------------
// // 
// // MuseFrame_t StretchEvent::read(Xml& xml)
// //       {
// //       MuseFrame_t at = 0;
// //       for (;;) {
// //             Xml::Token token = xml.parse();
// //             const QString& tag = xml.s1();
// //             switch (token) {
// //                   case Xml::Error:
// //                   case Xml::End:
// //                         return 0;
// // //                   case Xml::TagStart:
// // //                         if (tag == "frame")
// // //                               _frame = xml.parseLongInt();
// // //                         else 
// // //                         if (tag == "val")
// // //                               _stretch = xml.parseDouble();
// // //                         else
// // //                               xml.unknown("StretchEvent");
// // //                         break;
// //                   case Xml::TagStart:
// //                         xml.unknown("stretch");
// //                         break;
// //                   case Xml::Attribut:
// //                         if (tag == "at")
// //                               at = xml.s2().toLong();
// //                         if (tag == "val")
// //                               _stretch = xml.s2().toDouble();
// //                         break;
// //                   case Xml::TagEnd:
// //                         if (tag == "stretch") {
// //                               return at;
// //                               }
// //                   default:
// //                         break;
// //                   }
// //             }
// //       return 0;
// //       }



} // namespace MusECore
