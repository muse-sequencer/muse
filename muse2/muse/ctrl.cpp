//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: ctrl.cpp,v 1.1.2.4 2009/06/10 00:34:59 terminator356 Exp $
//
//    controller handling for mixer automation
//
//  (C) Copyright 2003 Werner Schweer (ws@seh.de)
//  (C) Copyright 2011-2012 Tim E. Real (terminator356 on users dot sourceforge dot net)
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

// Turn on debugging messages
//#define _CTRL_DEBUG_

#include <QLocale>
#include <QColor>
#include <map>

#include <math.h>

#include "gconfig.h"
#include "fastlog.h"
#include "globals.h"
#include "ctrl.h"
#include "midictrl.h"
#include "xml.h"

namespace MusECore {

void CtrlList::initColor(int i)
{
  QColor collist[] = { Qt::red, Qt::yellow, Qt::blue , Qt::black, Qt::white, Qt::green };

  if (i < 6)
    _displayColor = collist[i%6];
  else
    _displayColor = Qt::green;
  _visible = false;
}

//---------------------------------------------------------
//   midi2AudioCtrlValue
//   Apply mapper if it is non-null
//---------------------------------------------------------

double midi2AudioCtrlValue(const CtrlList* audio_ctrl_list, const MidiAudioCtrlStruct* /*mapper*/, int midi_ctlnum, int midi_val)
{
  double fmin, fmax;
  audio_ctrl_list->range(&fmin, &fmax);
  double frng = fmax - fmin;             // The audio control range.
  
  MidiController::ControllerType t = midiControllerType(midi_ctlnum);
  CtrlValueType aud_t = audio_ctrl_list->valueType();
  
  #ifdef _CTRL_DEBUG_
  printf("midi2AudioCtrlValue: midi_ctlnum:%d val:%d fmin:%f fmax:%f\n", midi_ctlnum, midi_val, fmin, fmax);  
  #endif  
  
  int ctlmn = 0;
  int ctlmx = 127;
  
  int bval = midi_val;
  switch(t) 
  {
    case MidiController::RPN:
    case MidiController::NRPN:
    case MidiController::Controller7:
      ctlmn = 0;
      ctlmx = 127;
    break;
    case MidiController::Controller14:
    case MidiController::RPN14:
    case MidiController::NRPN14:
      ctlmn = 0;
      ctlmx = 16383;
    break;
    case MidiController::Program:
      ctlmn = 0;
      ctlmx = 0xffffff;
    break;
    case MidiController::Pitch:
      ctlmn = -8192;
      ctlmx = 8191;
      bval += 8192;
    break;
    case MidiController::Velo:        // cannot happen
    default:
      break;
  }

  double fictlrng = double(ctlmx - ctlmn);   // Float version of the integer midi range.
  double normval = double(bval) / fictlrng;  // Float version of the normalized midi value.

  // ----------  TODO: Do stuff with the mapper, if supplied.
  
  if(aud_t == VAL_LOG)
  {
    // FIXME: Although this should be correct, some sliders show "---" at top end, some don't. 
    // Possibly because of use of fast_log10 in value(), and in sliders and automation IIRC.
    fmin = 20.0*log10(fmin);
    fmax = 20.0*log10(fmax);
    frng = fmax - fmin;
    double ret = exp10((normval * frng + fmin) / 20.0);
    #ifdef _CTRL_DEBUG_
    printf("midi2AudioCtrlValue: is VAL_LOG normval:%f frng:%f returning:%f\n", normval, frng, ret);          
    #endif
    return ret;
  }

  if(aud_t == VAL_LINEAR)
  {
    double ret = normval * frng + fmin;
    #ifdef _CTRL_DEBUG_
    printf("midi2AudioCtrlValue: is VAL_LINEAR normval:%f frng:%f returning:%f\n", normval, frng, ret);       
    #endif
    return ret;
  }

  if(aud_t == VAL_INT)
  {
    double ret = int(normval * frng + fmin);
    #ifdef _CTRL_DEBUG_
    printf("midi2AudioCtrlValue: is VAL_INT returning:%f\n", ret);   
    #endif
    return ret;  
  }
  
  if(aud_t == VAL_BOOL) 
  {
    #ifdef _CTRL_DEBUG_
    printf("midi2AudioCtrlValue: is VAL_BOOL\n");  
    #endif
    //if(midi_val > ((ctlmx - ctlmn)/2 + ctlmn))
    if((normval * frng + fmin) > (frng/2.0 + fmin))
      return fmax;
    else
      return fmin;
  }
  
  printf("midi2AudioCtrlValue: unknown audio controller type:%d\n", aud_t);
  return 0.0;
}      

//---------------------------------------------------------
// Midi to audio controller stuff
//---------------------------------------------------------

MidiAudioCtrlStruct::MidiAudioCtrlStruct()  
{ 
  _audio_ctrl_id = 0;
};

MidiAudioCtrlStruct::MidiAudioCtrlStruct(int audio_ctrl_id) : _audio_ctrl_id(audio_ctrl_id) 
{ 
};

MidiAudioCtrlMap_idx_t MidiAudioCtrlMap::index_hash(int midi_port, int midi_chan, int midi_ctrl_num) const
{ 
  return ((MidiAudioCtrlMap_idx_t(midi_port) & 0xff) << 24) | 
          ((MidiAudioCtrlMap_idx_t(midi_chan) & 0xf) << 20) | 
          (MidiAudioCtrlMap_idx_t(midi_ctrl_num) & 0xfffff);  
}

void MidiAudioCtrlMap::hash_values(MidiAudioCtrlMap_idx_t hash, int* midi_port, int* midi_chan, int* midi_ctrl_num) const
{
  if(midi_ctrl_num)
    *midi_ctrl_num = hash & 0xfffff;
  if(midi_chan)
    *midi_chan     = (hash >> 20) & 0xf;
  if(midi_port)
    *midi_port     = (hash >> 24) & 0xff;
}

iMidiAudioCtrlMap MidiAudioCtrlMap::add_ctrl_struct(int midi_port, int midi_chan, int midi_ctrl_num,  
                                                    const MidiAudioCtrlStruct& macs)
{
  MidiAudioCtrlMap_idx_t h = index_hash(midi_port, midi_chan, midi_ctrl_num);
  std::pair<iMidiAudioCtrlMap, iMidiAudioCtrlMap> range = equal_range(h);
  for(iMidiAudioCtrlMap imacp = range.first; imacp != range.second; ++imacp)
    if(imacp->second.audioCtrlId() == macs.audioCtrlId())
       return imacp;
  return insert(std::pair<MidiAudioCtrlMap_idx_t, MidiAudioCtrlStruct >(h, macs));
}

void MidiAudioCtrlMap::erase_ctrl_struct(int midi_port, int midi_chan, int midi_ctrl_num, int audio_ctrl_id)
{
  MidiAudioCtrlMap_idx_t h = index_hash(midi_port, midi_chan, midi_ctrl_num);
  std::pair<iMidiAudioCtrlMap, iMidiAudioCtrlMap> range = equal_range(h);
  MidiAudioCtrlMap macm;
  macm.insert(range.first, range.second);
  for(iMidiAudioCtrlMap imacm = macm.begin(); imacm != macm.end(); ++imacm)
    if(imacm->second.audioCtrlId() == audio_ctrl_id)
       erase(imacm);
}

void MidiAudioCtrlMap::find_audio_ctrl_structs(int audio_ctrl_id, AudioMidiCtrlStructMap* amcs) //const
{
  for(iMidiAudioCtrlMap imacm = begin(); imacm != end(); ++imacm)
    if(imacm->second.audioCtrlId() == audio_ctrl_id)
      amcs->push_back(imacm);
}

void MidiAudioCtrlMap::write(int level, Xml& xml) const
{
  for(ciMidiAudioCtrlMap imacm = begin(); imacm != end();  ++imacm)
  {
      int port, chan, mctrl;
      hash_values(imacm->first, &port, &chan, &mctrl);
      int actrl = imacm->second.audioCtrlId();
      QString s= QString("midiMapper port=\"%1\" ch=\"%2\" mctrl=\"%3\" actrl=\"%4\"")
                          .arg(port)
                          .arg(chan)
                          .arg(mctrl)
                          .arg(actrl);
      xml.tag(level++, s.toAscii().constData());
      
      // TODO
      //const MidiAudioCtrlStruct& macs = imacs->second;
      //xml.intTag(level, "macs ???", macs.);
      
      xml.etag(level--, "midiMapper");
  }
}

//---------------------------------------------------------
//   read
//---------------------------------------------------------

void MidiAudioCtrlMap::read(Xml& xml)
      {
      int port = -1, chan = -1, midi_ctrl = -1;
      MidiAudioCtrlStruct macs(-1);
      
      QLocale loc = QLocale::c();
      bool ok;
      int errcount = 0;
      for (;;) {
            Xml::Token token = xml.parse();
            const QString& tag = xml.s1();
            switch (token) {
                  case Xml::Error:
                  case Xml::End:
                        return;
                  case Xml::Attribut:
                        if (tag == "port")
                        {
                              port = loc.toInt(xml.s2(), &ok);
                              if(!ok)
                              { 
                                ++errcount;
                                printf("MidiAudioCtrlPortMap::read failed reading port string: %s\n", xml.s2().toLatin1().constData());
                              }
                        }
                        else if (tag == "ch")
                        {
                              chan = loc.toInt(xml.s2(), &ok);
                              if(!ok)
                              {
                                ++errcount;
                                printf("MidiAudioCtrlPortMap::read failed reading ch string: %s\n", xml.s2().toLatin1().constData());
                              }
                        }        
                        else if (tag == "mctrl")
                        {
                              midi_ctrl = loc.toInt(xml.s2(), &ok);
                              if(!ok)
                              {
                                ++errcount;
                                printf("MidiAudioCtrlPortMap::read failed reading mctrl string: %s\n", xml.s2().toLatin1().constData());
                              } 
                        }
                        else if (tag == "actrl")
                        {
                              macs.setAudioCtrlId(loc.toInt(xml.s2(), &ok));
                              if(!ok)
                              {
                                ++errcount;
                                printf("MidiAudioCtrlPortMap::read failed reading actrl string: %s\n", xml.s2().toLatin1().constData());
                              }
                        }
                        else
                              printf("unknown tag %s\n", tag.toLatin1().constData());
                        break;
                  case Xml::TagStart:
                        // TODO
                        //if (tag == "???") {  
                        //      }
                        //else
                              xml.unknown("midiMapper");
                        break;
                  case Xml::TagEnd:
                        if (xml.s1() == "midiMapper")
                        {
                              if(errcount == 0 && port != -1 && chan != -1 && midi_ctrl != -1 && macs.audioCtrlId() != -1)
                                  add_ctrl_struct(port, chan, midi_ctrl, macs);
                              return;
                        }
                  default:
                        break;
                  }
            }
      }
//---------------------------------------------------------
//   CtrlList
//---------------------------------------------------------

CtrlList::CtrlList()
      {
      _id      = 0;
      _default = 0.0;
      _curVal  = 0.0;
      _mode    = INTERPOLATE;
      _dontShow = false;
      _visible = false;
      _guiUpdatePending = false;
      initColor(0);
      }

CtrlList::CtrlList(int id)
      {
      _id      = id;
      _default = 0.0;
      _curVal  = 0.0;
      _mode    = INTERPOLATE;
      _dontShow = false;
      _visible = false;
      _guiUpdatePending = false;
      initColor(id);
      }

CtrlList::CtrlList(int id, QString name, double min, double max, CtrlValueType v, bool dontShow)
{
      _id      = id;
      _default = 0.0;
      _curVal  = 0.0;
      _mode    = INTERPOLATE;
      _name    = name;
      _min     = min;
      _max     = max;
      _valueType = v;
      _dontShow = dontShow;
      _visible = false;
      _guiUpdatePending = false;
      initColor(id);
}

//---------------------------------------------------------
//   assign
//---------------------------------------------------------

void CtrlList::assign(const CtrlList& l, int flags)
{
  if(flags & ASSIGN_PROPERTIES)
  {
    _id            = l._id;
    _default       = l._default;
    _curVal        = l._curVal;
    _mode          = l._mode;
    _name          = l._name;
    _min           = l._min;
    _max           = l._max;
    _valueType     = l._valueType;
    _dontShow      = l._dontShow;
    _displayColor  = l._displayColor;
    _visible       = l._visible;
  }
  
  if(flags & ASSIGN_VALUES)
  {
    *this = l; // Let the vector assign values.
    _guiUpdatePending = true;
  }
}

//---------------------------------------------------------
//   value
//   Returns value at frame.
//   cur_val_only means read the current 'manual' value, not from the list even if it is not empty.
//   If passed a nextFrame, sets nextFrame to the next event frame, or -1 if no next frame (wide-open), or, 
//    since CtrlList is a map, ZERO if should be replaced with some other frame by the caller (interpolation). 
//---------------------------------------------------------

double CtrlList::value(int frame, bool cur_val_only, int* nextFrame) const
{
      if(cur_val_only || empty()) 
      {
        if(nextFrame)
          *nextFrame = -1;
        return _curVal;
      }

      double rv;
      int nframe;

      ciCtrl i = upper_bound(frame); // get the index after current frame
      if (i == end()) { // if we are past all items just return the last value
            --i;
            if(nextFrame)
              *nextFrame = -1;
            return i->second.val;
            }
      else if(_mode == DISCRETE)
      {
        if(i == begin())
        {
            nframe = i->second.frame;
            rv = i->second.val;
        }  
        else
        {  
          nframe = i->second.frame;
          --i;
          rv = i->second.val;
        }  
      }
      else {                  // INTERPOLATE
        if (i == begin()) {
            nframe = i->second.frame;
            rv = i->second.val;
        }
        else {
            int frame2 = i->second.frame;
            double val2 = i->second.val;
            --i;
            int frame1 = i->second.frame;
            double val1   = i->second.val;

            
            if(val2 != val1)
              nframe = 0; // Zero signifies the next frame should be determined by caller.
            else
              nframe = frame2;
            
            if (_valueType == VAL_LOG) {
              val1 = 20.0*fast_log10(val1);
              if (val1 < MusEGlobal::config.minSlider)
                val1=MusEGlobal::config.minSlider;
              val2 = 20.0*fast_log10(val2);
              if (val2 < MusEGlobal::config.minSlider)
                val2=MusEGlobal::config.minSlider;
            }

            val2  -= val1;
            val1 += (double(frame - frame1) * val2)/double(frame2 - frame1);
    
            if (_valueType == VAL_LOG) {
              val1 = exp10(val1/20.0);
            }

            rv = val1;
          }
      }

      if(nextFrame)
          *nextFrame = nframe;
      
      return rv;
}

//---------------------------------------------------------
//   curVal
//   returns the static 'manual' value
//---------------------------------------------------------
double CtrlList::curVal() const
{ 
  return _curVal;
}

//---------------------------------------------------------
//   setCurVal
//   Sets the static 'manual' value
//---------------------------------------------------------
void CtrlList::setCurVal(double val)
{
#ifdef _CTRL_DEBUG_
  printf("CtrlList::setCurVal val:%f\n", val);  
#endif
  
  bool upd = (val != _curVal);
  _curVal = val;
  // If empty, any controller graphs etc. will be displaying this value.
  // Otherwise they'll be displaying the list, so update is not required.
  if(empty() && upd)     
    _guiUpdatePending = true;
}

//---------------------------------------------------------
//
//     Catch all insert, erase, clear etc.
//
//---------------------------------------------------------

CtrlList& CtrlList::operator=(const CtrlList& cl)
{
#ifdef _CTRL_DEBUG_
  printf("CtrlList::operator= id:%d\n", cl.id());  
#endif
  std::map<int, CtrlVal, std::less<int> >::operator=(cl);
  _guiUpdatePending = true;
  return *this;
}

void CtrlList::swap(CtrlList& cl)
{
#ifdef _CTRL_DEBUG_
  printf("CtrlList::swap id:%d\n", cl.id());  
#endif
  std::map<int, CtrlVal, std::less<int> >::swap(cl);
  cl.setGuiUpdatePending(true);
  _guiUpdatePending = true;
}

std::pair<iCtrl, bool> CtrlList::insert(const std::pair<int, CtrlVal>& p)
{
#ifdef _CTRL_DEBUG_
  printf("CtrlList::insert frame:%d val:%f\n", p.first, p.second.val);  
#endif
  std::pair<iCtrl, bool> res = std::map<int, CtrlVal, std::less<int> >::insert(p);
  _guiUpdatePending = true;
  return res;
}

iCtrl CtrlList::insert(iCtrl ic, const std::pair<int, CtrlVal>& p)
{
#ifdef _CTRL_DEBUG_
  printf("CtrlList::insert2 frame:%d val:%f\n", p.first, p.second.val); 
#endif
  iCtrl res = std::map<int, CtrlVal, std::less<int> >::insert(ic, p);
  _guiUpdatePending = true;
  return res;
}

void CtrlList::erase(iCtrl ictl)
{
#ifdef _CTRL_DEBUG_
  printf("CtrlList::erase iCtrl frame:%d val:%f\n", ictl->second.frame, ictl->second.val);  
#endif
  std::map<int, CtrlVal, std::less<int> >::erase(ictl);
  _guiUpdatePending = true;
}

std::map<int, CtrlVal, std::less<int> >::size_type CtrlList::erase(int frame)
{
#ifdef _CTRL_DEBUG_
  printf("CtrlList::erase frame:%d\n", frame);  
#endif
  std::map<int, CtrlVal, std::less<int> >::size_type res = std::map<int, CtrlVal, std::less<int> >::erase(frame);
  _guiUpdatePending = true;
  return res;
}

void CtrlList::erase(iCtrl first, iCtrl last)
{
#ifdef _CTRL_DEBUG_
  printf("CtrlList::erase range first frame:%d val:%f second frame:%d val:%f\n", 
         first->second.frame, first->second.val,
         last->second.frame, last->second.val);  
#endif
  std::map<int, CtrlVal, std::less<int> >::erase(first, last);
  _guiUpdatePending = true;
}

void CtrlList::clear()
{
#ifdef _CTRL_DEBUG_
  printf("CtrlList::clear\n");  
#endif
  std::map<int, CtrlVal, std::less<int> >::clear();
  _guiUpdatePending = true;
}

//---------------------------------------------------------
//   add
//   Add, or replace, an event at time frame having value val. 
//---------------------------------------------------------

void CtrlList::add(int frame, double val)
      {
      iCtrl e = find(frame);
      if (e != end())
      {
            bool upd = (val != e->second.val);
            e->second.val = val;
#ifdef _CTRL_DEBUG_
            printf("CtrlList::add frame:%d val:%f\n", frame, val);  
#endif
            if(upd)
              _guiUpdatePending = true;
      }
      else
            insert(std::pair<const int, CtrlVal> (frame, CtrlVal(frame, val)));
      }

//---------------------------------------------------------
//   del
//---------------------------------------------------------

void CtrlList::del(int frame)
      {
      iCtrl e = find(frame);
      if (e == end())
            return;
      
      erase(e);
      }

//---------------------------------------------------------
//   updateCurValues
//   Set the current static 'manual' value (non-automation value) 
//    from the automation value at the given time.
//---------------------------------------------------------

void CtrlList::updateCurValue(int frame)
{
  double v = value(frame);
  bool upd = (v != _curVal);
  _curVal = v;
  // If empty, any controller graphs etc. will be displaying this value.
  // Otherwise they'll be displaying the list, so update is not required.
  if(empty() && upd)     
    _guiUpdatePending = true;
}
      
//---------------------------------------------------------
//   read
//---------------------------------------------------------

void CtrlList::read(Xml& xml)
      {
      QLocale loc = QLocale::c();
      bool ok;
      for (;;) {
            Xml::Token token = xml.parse();
            const QString& tag = xml.s1();
            switch (token) {
                  case Xml::Error:
                  case Xml::End:
                        return;
                  case Xml::Attribut:
                        if (tag == "id")
                        {
                              _id = loc.toInt(xml.s2(), &ok);
                              if(!ok)
                                printf("CtrlList::read failed reading _id string: %s\n", xml.s2().toLatin1().constData());
                        }
                        else if (tag == "cur")
                        {
                              _curVal = loc.toDouble(xml.s2(), &ok);
                              if(!ok)
                                printf("CtrlList::read failed reading _curVal string: %s\n", xml.s2().toLatin1().constData());
                        }        
                        else if (tag == "visible")
                        {
                              _visible = loc.toInt(xml.s2(), &ok);
                              if(!ok)
                                printf("CtrlList::read failed reading _visible string: %s\n", xml.s2().toLatin1().constData());
                        }
                        else if (tag == "color")
                        {
#if QT_VERSION >= 0x040700
                              ok = _displayColor.isValidColor(xml.s2());
                              if (!ok) {
                                printf("CtrlList::read failed reading color string: %s\n", xml.s2().toLatin1().constData());
                                break;
                              }
#endif
                              _displayColor.setNamedColor(xml.s2());
                        }
                        else
                              printf("unknown tag %s\n", tag.toLatin1().constData());
                        break;
                  case Xml::Text:
                        {
                          int len = tag.length();
                          int frame;
                          double val;
  
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
                                
                                frame = loc.toInt(fs, &ok);
                                if(!ok)
                                {
                                  printf("CtrlList::read failed reading frame string: %s\n", fs.toLatin1().constData());
                                  break;
                                }
                                  
                                while(i < len && (tag[i] == ' ' || tag[i] == '\n'))
                                  ++i;
                                if(i == len)
                                      break;
                                
                                QString vs;
                                while(i < len && tag[i] != ' ' && tag[i] != ',')
                                {
                                  vs.append(tag[i]); 
                                  ++i;
                                }
                                
                                val = loc.toDouble(vs, &ok);
                                if(!ok)
                                {
                                  printf("CtrlList::read failed reading value string: %s\n", vs.toLatin1().constData());
                                  break;
                                }
                                  
                                add(frame, val);
                                
                                if(i == len)
                                      break;
                          }
                        }
                        break;
                  case Xml::TagEnd:
                        if (xml.s1() == "controller")
                              return;
                  default:
                        break;
                  }
            }
      }

//---------------------------------------------------------
//   add
//---------------------------------------------------------

void CtrlListList::add(CtrlList* vl)
      {
      insert(std::pair<const int, CtrlList*>(vl->id(), vl));
      }

//---------------------------------------------------------
//   value
//   Returns value at frame for controller with id ctrlId.
//   cur_val_only means read the current 'manual' value, not from the list even if it is not empty.
//   If passed a nextFrame, sets nextFrame to the next event frame, or -1 if no next frame (wide-open), or, 
//    since CtrlList is a map, ZERO if should be replaced with some other frame by the caller (interpolation). 
//---------------------------------------------------------

double CtrlListList::value(int ctrlId, int frame, bool cur_val_only, int* nextFrame) const     
      {
      ciCtrlList cl = find(ctrlId);
      if (cl == end())
      {
        if(nextFrame)
          *nextFrame = -1;
        return 0.0;
      }
      
      return cl->second->value(frame, cur_val_only, nextFrame);  
      }

//---------------------------------------------------------
//   updateCurValues
//   Set the current 'manual' values (non-automation values) 
//    from the automation values at the given time.
//   This is typically called right after a track's automation type changes
//    to OFF, so that the manual value becomes the last automation value.
//   There are some interesting advantages to having completely independent 
//    'manual' and automation values, but the jumping around when switching to OFF
//    becomes disconcerting.
//---------------------------------------------------------

void CtrlListList::updateCurValues(int frame)
{
  for(ciCtrlList cl = begin(); cl != end(); ++cl)
    cl->second->updateCurValue(frame);
}

//---------------------------------------------------------
//   value
//---------------------------------------------------------

void CtrlListList::write(int level, Xml& xml) const
{
  for (ciCtrlList icl = begin(); icl != end(); ++icl) {
        const CtrlList* cl = icl->second;

        QString s= QString("controller id=\"%1\" cur=\"%2\"").arg(cl->id()).arg(cl->curVal()).toAscii().constData();
        s += QString(" color=\"%1\" visible=\"%2\"").arg(cl->color().name()).arg(cl->isVisible());
        xml.tag(level++, s.toAscii().constData());
        int i = 0;
        for (ciCtrl ic = cl->begin(); ic != cl->end(); ++ic) {
              QString s("%1 %2, ");
              xml.nput(level, s.arg(ic->second.frame).arg(ic->second.val).toAscii().constData());
              ++i;
              if (i >= 4) {
                    xml.put(level, "");
                    i = 0;
                    }
              }
        if (i)
              xml.put(level, "");
        xml.etag(level--, "controller");
        }
  
  _midi_controls.write(level, xml);
}


} // namespace MusECore
