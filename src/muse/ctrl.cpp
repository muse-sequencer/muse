//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: ctrl.cpp,v 1.1.2.4 2009/06/10 00:34:59 terminator356 Exp $
//
//    controller handling for mixer automation
//
//  (C) Copyright 2003 Werner Schweer (ws@seh.de)
//  (C) Copyright 2011-2013 Tim E. Real (terminator356 on users dot sourceforge dot net)
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
#include <map>

#include "muse_math.h"
#include "gconfig.h"
#include "fastlog.h"
#include "globals.h"
#include "ctrl.h"
#include "midictrl.h"

// Forwards from header:
#include "xml.h"
#include "track.h"

namespace MusECore {

CtrlInterpolate::CtrlInterpolate(
  unsigned int sframe, unsigned int eframe, bool eframevalid, double sval, double eval,
  bool end_stop, bool do_interpolate)
{
  sFrame = sframe;
  sVal   = sval;
  eFrame = eframe;
  eFrameValid = eframevalid;
  eVal   = eval;
  eStop = end_stop;
  doInterp = do_interpolate;
}

CtrlVal::CtrlVal()
{
  val   = 0;
  _flags = VAL_GROUP_END;
}

CtrlVal::CtrlVal(double v, bool selected, bool groupEnd)
{
  val   = v;
  _flags = VAL_NOFLAGS;
  if(selected)
    _flags |= VAL_SELECTED;
  if(groupEnd)
    _flags |= VAL_GROUP_END;
}

CtrlValueFlags CtrlVal::flags() const { return _flags; }
void CtrlVal::setFlags(CtrlValueFlags f) { _flags = f; }
bool CtrlVal::selected() const { return _flags & VAL_SELECTED; }
void CtrlVal::setSelected(bool v) { if(v) _flags |= VAL_SELECTED; else _flags &= ~VAL_SELECTED; }
bool CtrlVal::groupEnd() const { return _flags & VAL_GROUP_END; }
void CtrlVal::setGroupEnd(bool v) { if(v) _flags |= VAL_GROUP_END; else _flags &= ~VAL_GROUP_END;}
double CtrlVal::value() const { return val; }
void CtrlVal::setValue(double v) { val = v; }

CtrlRecVal::CtrlRecVal(unsigned int f, int n, double v)
  : frame(f), val(v), id(n), type(ARVT_VAL) {}
CtrlRecVal::CtrlRecVal(unsigned int f, int n, double v, CtrlRecValueType t)
  : frame(f), val(v), id(n), type(t) {}


int MidiAudioCtrlStruct::audioCtrlId() const        { return _audio_ctrl_id; }
void MidiAudioCtrlStruct::setAudioCtrlId(int actrl) { _audio_ctrl_id = actrl; }

void CtrlList::initColor(int i)
{
  const QColor collist[] = { Qt::red, Qt::yellow, Qt::blue , Qt::black, Qt::white, Qt::green };

  if (i < 6)
    _displayColor = collist[i%6];
  else
  {
    // Let sliders all have different but unique colors
    // Some prime number magic
    const uint j = i+1;
    const uint c1 = j * 211  % 256;
    const uint c2 = j * j * 137  % 256;
    const uint c3 = j * j * j * 43  % 256;
    _displayColor = QColor(c1, c2, c3);
  }
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
      xml.tag(level++, s.toLatin1().constData());
      
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

CtrlList::CtrlList(bool dontShow)
      {
      // The controller can be -1 meaning no particular id, for copy/paste etc.
      _id      = -1;
      _default = 0.0;
      _curVal  = 0.0;
      _mode    = INTERPOLATE;
      _min     = 0;
      _max     = 0;
      _valueType = VAL_LINEAR;

      _dontShow = dontShow;
      _visible = false;
      _guiUpdatePending = false;
      initColor(0);
      }

CtrlList::CtrlList(int id, bool dontShow)
      {
      _id      = id;
      _default = 0.0;
      _curVal  = 0.0;
      _mode    = INTERPOLATE;
      _min     = 0;
      _max     = 0;
      _valueType = VAL_LINEAR;

      _dontShow = dontShow;
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

CtrlList::CtrlList(const CtrlList& l, int flags)
{
  _id        = l._id;
  _valueType = l._valueType;
  assign(l, flags | ASSIGN_PROPERTIES);
}

CtrlList::CtrlList(const CtrlList& cl)
 // Let map copy the items.
 : CtrlList_t(cl)
{
  _id            = cl._id;
  _default       = cl._default;
  _curVal        = cl._curVal;
  _mode          = cl._mode;
  _name          = cl._name;
  _min           = cl._min;
  _max           = cl._max;
  _valueType     = cl._valueType;
  _dontShow      = cl._dontShow;
  _displayColor  = cl._displayColor;
  _visible       = cl._visible;
  _guiUpdatePending = true;
}

//---------------------------------------------------------
//   assign
//---------------------------------------------------------

void CtrlList::assign(const CtrlList& l, int flags)
{
  if(flags & ASSIGN_PROPERTIES)
  {
    _default       = l._default;
    _curVal        = l._curVal;
    _mode          = l._mode;
    _name          = l._name;
    _min           = l._min;
    _max           = l._max;
    _dontShow      = l._dontShow;
    _displayColor  = l._displayColor;
    _visible       = l._visible;
  }
  
  if(flags & ASSIGN_VALUES)
  {
    CtrlList_t::operator=(l); // Let map copy the items.
    _guiUpdatePending = true;
  }
}

//---------------------------------------------------------
//   getInterpolation
//   Fills CtrlInterpolate struct for given frame.
//   cur_val_only means read the current 'manual' value, not from the list even if it is not empty.
//   CtrlInterpolate member eFrameValid can be false meaning no next value (wide-open, endless).
//---------------------------------------------------------

void CtrlList::getInterpolation(unsigned int frame, bool cur_val_only, CtrlInterpolate* interp)
{
  interp->eStop = false; // During processing, control FIFO ring buffers will set this true.

  if(cur_val_only || empty())
  {
    interp->sFrame = 0;
    interp->eFrame = 0;
    interp->eFrameValid = false;
    interp->sVal = _curVal;
    interp->eVal = _curVal;
    interp->doInterp = false;
    return;
  }
  ciCtrl i = upper_bound(frame); // get the index after current frame
  if (i == end())   // if we are past all items just return the last value
  { 
        --i;
        interp->sFrame = i->first;
        interp->eFrame = 0;
        interp->eFrameValid = false;
        interp->sVal = i->second.value();
        interp->eVal = i->second.value();
        interp->doInterp = false;
        return;
  }
  else if(_mode == DISCRETE)
  {
    if(i == begin())
    {
      interp->sFrame = 0;
      interp->eFrame = i->first;
      interp->eFrameValid = true;
      interp->sVal = i->second.value();
      interp->eVal = i->second.value();
      interp->doInterp = false;
    }
    else
    {
      interp->eFrame = i->first;
      interp->eFrameValid = true;
      interp->eVal = i->second.value();
      --i;
      interp->sFrame = i->first;
      interp->sVal = i->second.value();
      interp->doInterp = false;
    }
  }
  else   // INTERPOLATE
  {                  
    if(i == begin())
    {
      interp->sFrame = 0;
      interp->eFrame = i->first;
      interp->eFrameValid = true;
      interp->sVal = i->second.value();
      interp->eVal = i->second.value();
      interp->doInterp = false;
    }
    else
    {
      interp->eFrame = i->first;
      interp->eFrameValid = true;
      interp->eVal = i->second.value();
      --i;
      interp->sFrame = i->first;
      interp->sVal = i->second.value();
      interp->doInterp = (interp->eVal != interp->sVal && interp->eFrame > interp->sFrame);
    }
  }
}

//---------------------------------------------------------
//   interpolate
//   Returns interpolated value at given frame, from a CtrlInterpolate struct.
//   For speed, no checking is done for frame = frame2, val1 = val2 or even CtrlInterpolate::doInterp.
//   Those are to be taken care of before calling this routine. See getInterpolation().
//---------------------------------------------------------

double CtrlList::interpolate(unsigned int frame, const CtrlInterpolate& interp)
{
  const unsigned int frame1 = interp.sFrame;
  const unsigned int frame2 = interp.eFrame;
  double val1 = interp.sVal;
  double val2 = interp.eVal;
  if(!interp.eFrameValid || frame >= frame2)
  {
    if(_valueType == VAL_LOG)
    {
      const double min = exp10(MusEGlobal::config.minSlider / 20.0);  // TODO Try fastexp10
      if(val2 < min)
        val2 = min;
    }
    return val2;
  }
  if(frame <= frame1)
  {
    if(_valueType == VAL_LOG)
    {
      const double min = exp10(MusEGlobal::config.minSlider / 20.0);  // TODO Try fastexp10
      if(val1 < min)
        val1 = min;
    }
    return val1;
  }

  if(_valueType == VAL_LOG)
  {
    val1 = 20.0*fast_log10(val1);
    if (val1 < MusEGlobal::config.minSlider)
      val1=MusEGlobal::config.minSlider;
    val2 = 20.0*fast_log10(val2);
    if (val2 < MusEGlobal::config.minSlider)
      val2=MusEGlobal::config.minSlider;
  }
  val2 -= val1;
  val1 += (double(frame - frame1) * val2) / double(frame2 - frame1);
  if (_valueType == VAL_LOG)
    val1 = exp10(val1/20.0);
  return val1;
}

//---------------------------------------------------------
//   value
//   Returns value at frame.
//   cur_val_only means read the current 'manual' value, not from the list even if it is not empty.
//   If passed a nextFrame, sets nextFrame to the next event frame, or 0 and eFrameValid false if no next frame (wide-open), or, 
//    since CtrlList is a map, ZERO if should be replaced with some other frame by the caller (interpolation). 
//---------------------------------------------------------

double CtrlList::value(unsigned int frame, bool cur_val_only, unsigned int* nextFrame, bool* nextFrameValid) const
{
      if(cur_val_only || empty()) 
      {
        if(nextFrameValid)
          *nextFrameValid = false;
        if(nextFrame)
          *nextFrame = 0;
        return _curVal;
      }

      double rv;
      unsigned int nframe;

      ciCtrl i = upper_bound(frame); // get the index after current frame
      if (i == end()) { // if we are past all items just return the last value
            --i;
            if(nextFrameValid)
              *nextFrameValid = false;
            if(nextFrame)
              *nextFrame = 0;
            return i->second.value();
            }
      else if(_mode == DISCRETE)
      {
        if(i == begin())
        {
            nframe = i->first;
            rv = i->second.value();
        }  
        else
        {  
          nframe = i->first;
          --i;
          rv = i->second.value();
        }  
      }
      else {                  // INTERPOLATE
        if (i == begin()) {
            nframe = i->first;
            rv = i->second.value();
        }
        else {
            const unsigned int frame2 = i->first;
            double val2 = i->second.value();
            --i;
            const unsigned int frame1 = i->first;
            double val1   = i->second.value();

            
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
      if(nextFrameValid)
          *nextFrameValid = true;
      
      return rv;
}

CtrlList::Mode CtrlList::mode() const          { return _mode; }
void CtrlList::setMode(Mode m)       { _mode = m; }
double CtrlList::getDefault() const   { return _default; }
void CtrlList::setDefault(double val) { _default = val; }
int CtrlList::id() const             { return _id; }
void CtrlList::setId(int v)          { _id = v; }
QString CtrlList::name() const       { return _name; }
void CtrlList::setName(const QString& s) { _name = s; }
double CtrlList::minVal() const { return _min; }
double CtrlList::maxVal() const { return _max; }
void CtrlList::setRange(double min, double max) {
      _min = min;
      _max = max;
      }
void CtrlList::range(double* min, double* max) const {
      *min = _min;
      *max = _max;
      }
CtrlValueType CtrlList::valueType() const { return _valueType; }
void CtrlList::setValueType(CtrlValueType t) { _valueType = t; }
void CtrlList::setColor( QColor c ) { _displayColor = c;}
QColor CtrlList::color() const { return _displayColor; }
void CtrlList::setVisible(bool v) { _visible = v; }
bool CtrlList::isVisible() const { return _visible; }
bool CtrlList::dontShow() const { return _dontShow; }
bool CtrlList::guiUpdatePending() const { return _guiUpdatePending; }
void CtrlList::setGuiUpdatePending(bool v) { _guiUpdatePending = v; }

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
  _id            = cl._id;
  _default       = cl._default;
  _curVal        = cl._curVal;
  _mode          = cl._mode;
  _name          = cl._name;
  _min           = cl._min;
  _max           = cl._max;
  _valueType     = cl._valueType;
  _dontShow      = cl._dontShow;
  _displayColor  = cl._displayColor;
  _visible       = cl._visible;
  
  // Let map copy the items.
  CtrlList_t::operator=(cl);
  _guiUpdatePending = true;
  return *this;
}

void CtrlList::swap(CtrlList& cl) noexcept
{
#ifdef _CTRL_DEBUG_
  printf("CtrlList::swap id:%d\n", cl.id());  
#endif
  CtrlList_t::swap(cl);
  cl.setGuiUpdatePending(true);
  _guiUpdatePending = true;
}

std::pair<iCtrl, bool> CtrlList::insert(const CtrlListInsertPair_t& p)
{
#ifdef _CTRL_DEBUG_
  printf("CtrlList::insert1 frame:%u val:%f\n", p.first, p.second.val);
#endif
  std::pair<iCtrl, bool> res = CtrlList_t::insert(p);
  _guiUpdatePending = true;
  return res;
}

template< class P > std::pair<iCtrl, bool> CtrlList::insert(P&& value)
{
#ifdef _CTRL_DEBUG_
  printf("CtrlList::insert2 frame:%u val:%f\n", value.first, value.second.val);
#endif
  std::pair<iCtrl, bool> res = CtrlList_t::insert(value);
  _guiUpdatePending = true;
  return res;
}

std::pair<iCtrl, bool> CtrlList::insert(CtrlListInsertPair_t&& p)
{
#ifdef _CTRL_DEBUG_
  printf("CtrlList::insert3 frame:%u val:%f\n", p.first, p.second.val);
#endif
  std::pair<iCtrl, bool> res = CtrlList_t::insert(p);
  _guiUpdatePending = true;
  return res;
}

iCtrl CtrlList::insert(ciCtrl ic, const CtrlListInsertPair_t& p)
{
#ifdef _CTRL_DEBUG_
  printf("CtrlList::insert4 frame:%u val:%f\n", p.first, p.second.val);
#endif
  iCtrl res = CtrlList_t::insert(ic, p);
  _guiUpdatePending = true;
  return res;
}

template< class P > iCtrl CtrlList::insert(ciCtrl ic, P&& value)
{
#ifdef _CTRL_DEBUG_
  printf("CtrlList::insert5 frame:%u val:%f\n", value.first, value.second.val);
#endif
  iCtrl res = CtrlList_t::insert(ic, value);
  _guiUpdatePending = true;
  return res;
}

iCtrl CtrlList::insert(ciCtrl ic, CtrlListInsertPair_t&& value)
{
#ifdef _CTRL_DEBUG_
  printf("CtrlList::insert6 frame:%u val:%f\n", value.first, value.second.val);
#endif
  iCtrl res = CtrlList_t::insert(ic, value);
  _guiUpdatePending = true;
  return res;
}

void CtrlList::insert(iCtrl first, iCtrl last)
{
#ifdef _CTRL_DEBUG_
  printf("CtrlList::insert7 first frame:%u last frame:%d\n", first->first, last->first);
#endif
  CtrlList_t::insert(first, last);
  _guiUpdatePending = true;
}

// void CtrlList::insert(std::initializer_list<CtrlListInsertPair_t> ilist)
// {
// #ifdef _CTRL_DEBUG_
//   printf("CtrlList::insert8\n");
// #endif
//   // TODO: Hm, error, no matching member function.
//   CtrlList_t::insert(ilist);
//   _guiUpdatePending = true;
// }

// CtrlList::insert_return_type CtrlList::insert(CtrlList::node_type&& nh)
// {
// #ifdef _CTRL_DEBUG_
//   printf("CtrlList::insert9\n");
// #endif
//   // TODO: Hm, error, no matching member function.
//   CtrlList::insert_return_type ret = CtrlList_t::insert(nh);
//   _guiUpdatePending = true;
//   return ret;
// }

// iCtrl CtrlList::insert(ciCtrl ic, CtrlList::node_type&& nh)
// {
// #ifdef _CTRL_DEBUG_
//   printf("CtrlList::insert10\n");
// #endif
//   // TODO: Hm, error, no matching member function.
//   iCtrl ret = CtrlList_t::insert(ic, nh);
//   _guiUpdatePending = true;
//   return ret;
// }

template <class M> std::pair<iCtrl, bool> CtrlList::insert_or_assign(const unsigned int& k, M&& obj)
{
#ifdef _CTRL_DEBUG_
  printf("CtrlList::insert_or_assign1 frame:%u\n", k);
#endif
  std::pair<iCtrl, bool> res = CtrlList_t::insert_or_assign(k, obj);
  _guiUpdatePending = true;
  return res;
}

template <class M> std::pair<iCtrl, bool> CtrlList::insert_or_assign(unsigned int&& k, M&& obj)
{
#ifdef _CTRL_DEBUG_
  printf("CtrlList::insert_or_assign2 frame:%u\n", k);
#endif
  std::pair<iCtrl, bool> res = CtrlList_t::insert_or_assign(k, obj);
  _guiUpdatePending = true;
  return res;
}

template <class M> iCtrl CtrlList::insert_or_assign(ciCtrl hint, const unsigned int& k, M&& obj)
{
#ifdef _CTRL_DEBUG_
  printf("CtrlList::insert_or_assign3 frame:%u\n", k);
#endif
  iCtrl res = CtrlList_t::insert_or_assign(hint, k, obj);
  _guiUpdatePending = true;
  return res;
}

template <class M> iCtrl CtrlList::insert_or_assign(ciCtrl hint, unsigned int&& k, M&& obj)
{
#ifdef _CTRL_DEBUG_
  printf("CtrlList::insert_or_assign4 frame:%u\n", k);
#endif
  iCtrl res = CtrlList_t::insert_or_assign(hint, k, obj);
  _guiUpdatePending = true;
  return res;
}

iCtrl CtrlList::erase(iCtrl ictl)
{
#ifdef _CTRL_DEBUG_
  printf("CtrlList::erase1A iCtrl frame:%u val:%f\n", ictl->first, ictl->second.val);
#endif
  iCtrl res = CtrlList_t::erase(ictl);
  _guiUpdatePending = true;
  return res;
}

iCtrl CtrlList::erase(ciCtrl ictl)
{
#ifdef _CTRL_DEBUG_
  printf("CtrlList::erase1B iCtrl frame:%u val:%f\n", ictl->first, ictl->second.val);
#endif
  iCtrl res = CtrlList_t::erase(ictl);
  _guiUpdatePending = true;
  return res;
}

CtrlList::iterator CtrlList::erase(ciCtrl first, ciCtrl last)
{
#ifdef _CTRL_DEBUG_
  printf("CtrlList::erase2 range first frame:%u val:%f second frame:%u val:%f\n",
         first->first, first->second.val,
         last->first, last->second.val);
#endif
  iCtrl res = CtrlList_t::erase(first, last);
  _guiUpdatePending = true;
  return res;
}

CtrlList_t::size_type CtrlList::erase(unsigned int frame)
{
#ifdef _CTRL_DEBUG_
  printf("CtrlList::erase3 frame:%u\n", frame);
#endif
  size_type res = CtrlList_t::erase(frame);
  _guiUpdatePending = true;
  return res;
}

void CtrlList::clear() noexcept
{
#ifdef _CTRL_DEBUG_
  printf("CtrlList::clear\n");  
#endif
  CtrlList_t::clear();
  _guiUpdatePending = true;
}

//---------------------------------------------------------
//   add
//   Add, or replace, an event at time frame having value val. 
//---------------------------------------------------------

std::pair<CtrlList::iterator, bool> CtrlList::add(unsigned int frame, double value, bool selected, bool groupEnd)
      {
      return insert_or_assign(frame, CtrlVal(value, selected, groupEnd));
      }

//---------------------------------------------------------
//   del
//---------------------------------------------------------

void CtrlList::del(unsigned int frame)
      {
      iCtrl e = find(frame);
      if (e == end())
            return;
      erase(e);
      }

bool CtrlList::updateGroups()
{
  // Do not include hidden controller lists.
  //if(!isVisible())
  //  return false;

  bool res = false;
  for(iCtrl ic = begin(); ic != end(); ++ic)
  {
    CtrlVal& cv = ic->second;
    // Include only selected controller values.
    if(!cv.selected())
      continue;

    // Check whether this item is the end of a group ie. the next item is unselected.
    ciCtrl ic_next = ic;
    ++ic_next;
    const bool isGroupEnd = (ic_next == cend()) || (!ic_next->second.selected());
    if(cv.groupEnd() != isGroupEnd)
    {
      res = true;
      cv.setGroupEnd(isGroupEnd);
    }
  }
  return res;
}

bool CtrlList::updateGroups(CtrlList::iterator ic)
{
  // Do not include hidden controller lists.
  //if(!isVisible())
  //  return false;

  bool res = false;
  CtrlVal& cv = ic->second;
  if(cv.selected())
  {
    ciCtrl ic_next = ic;
    ++ic_next;
    const bool isGroupEnd = (ic_next == cend()) || (!ic_next->second.selected());
    if(cv.groupEnd() != isGroupEnd)
    {
      res = true;
      cv.setGroupEnd(isGroupEnd);
    }
  }
  if(ic != begin())
  {
    iCtrl ic_prev = ic;
    --ic_prev;
    CtrlVal& cv_prev = ic_prev->second;
    if(cv_prev.selected())
    {
      const bool prevIsGroupEnd = !cv.selected();
      if(cv_prev.groupEnd() != prevIsGroupEnd)
      {
        res = true;
        cv_prev.setGroupEnd(prevIsGroupEnd);
      }
    }
  }
  return res;
}

bool CtrlList::updateGroups(unsigned int frame)
{
  // Do not include hidden controller lists.
  //if(!isVisible())
  //  return false;

  iCtrl ic = find(frame);
  if(ic == end())
    return false;
  return updateGroups(ic);
}

//---------------------------------------------------------
//   updateCurValues
//   Set the current static 'manual' value (non-automation value) 
//    from the automation value at the given time.
//---------------------------------------------------------

void CtrlList::updateCurValue(unsigned int frame)
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
//   readValues
//---------------------------------------------------------

void CtrlList::readValues(const QString& tag, const int samplerate)
{
  QLocale loc = QLocale::c();
  bool ok;
  int len = tag.length();
  unsigned int frame;
  double val;
  bool sel = false;
  bool groupEnd = false;

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

    frame = loc.toUInt(fs, &ok);
    if(!ok)
    {
      fprintf(stderr, "CtrlList::readValues failed reading frame string: %s\n", fs.toLatin1().constData());
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
      fprintf(stderr,"CtrlList::readValues failed reading value string: %s\n", vs.toLatin1().constData());
      break;
    }

    // Check for optional info before the comma.
    sel = false;
    groupEnd = false;
    bool optErr = false;
    while(i < len && tag[i] != ',')
    {
      while(i < len && (tag[i] == ' ' || tag[i] == '\n'))
        ++i;
      if(i == len)
      {
        optErr = true;
        break;
      }

      QString vs;
      while(i < len && tag[i] != ' ' && tag[i] != ',')
      {
        vs.append(tag[i]);
        ++i;
      }

      // The 'selected' optional character.
      if(vs == QString('s'))
        sel = true;
      // The 'group end' optional character.
      else if(vs == QString('g'))
        groupEnd = true;
      else
      {
        fprintf(stderr, "CtrlList::readValues failed: unrecognized optional item string: %s\n", vs.toLatin1().constData());
        optErr = true;
        break;
      }
    }
    if(optErr)
      break;

    // For now, this conversion only has a TEMPORARY effect during song loading.
    // See comments in Song::read at the "samplerate" tag.
    frame = MusEGlobal::convertFrame4ProjectSampleRate(frame, samplerate);

    add(frame, val, sel, groupEnd);

    if(i == len)
      break;
  }
}

//---------------------------------------------------------
//   read
//---------------------------------------------------------

bool CtrlList::read(Xml& xml)
      {
      QLocale loc = QLocale::c();
      bool ok;
      int id = -1;
      bool idOk;
      double min;
      double max;
      bool minOk = false;
      bool maxOk = false;
      int valType = VAL_LINEAR;
      int samplerate = MusEGlobal::sampleRate;

      for (;;) {
            Xml::Token token = xml.parse();
            const QString& tag = xml.s1();
            switch (token) {
                  case Xml::Error:
                  case Xml::End:
                        return false;
                  case Xml::Attribut:
                        if (tag == "id")
                        {
                              id = loc.toInt(xml.s2(), &idOk);
                              if(!idOk)
                                fprintf(stderr, "CtrlList::read failed reading _id string: %s\n", xml.s2().toLatin1().constData());
                        }
                        else if (tag == "cur")
                        {
                              _curVal = loc.toDouble(xml.s2(), &ok);
                              if(!ok)
                                fprintf(stderr, "CtrlList::read failed reading _curVal string: %s\n", xml.s2().toLatin1().constData());
                        }
                        else if (tag == "visible")
                        {
                              _visible = loc.toInt(xml.s2(), &ok);
                              if(!ok)
                                fprintf(stderr, "CtrlList::read failed reading _visible string: %s\n", xml.s2().toLatin1().constData());
                        }
                        else if (tag == "color")
                        {
                              ok = _displayColor.isValidColor(xml.s2());
                              if (ok)
                                _displayColor.setNamedColor(xml.s2());
                              else
                              {
                                fprintf(stderr, "CtrlList::read failed reading color string: %s\n", xml.s2().toLatin1().constData());
                              }
                        }

                        // The valueType, min, max, and samplerate tags are typically only used for copy/paste operations
                        //  and are not saved in a song file since they are set by the owner track.
                        else if (tag == "valueType")
                        {
                              valType = loc.toInt(xml.s2(), &ok);
                              if(ok)
                                setValueType(MusECore::CtrlValueType(valType));
                              else
                                fprintf(stderr, "CtrlList::read failed reading valueType string: %s\n",
                                        xml.s2().toLatin1().constData());
                        }
                        else if (tag == "min")
                        {
                              min = loc.toDouble(xml.s2(), &minOk);
                              if(!minOk)
                                fprintf(stderr, "CtrlList::read failed reading min string: %s\n",
                                        xml.s2().toLatin1().constData());
                        }
                        else if (tag == "max")
                        {
                              max = loc.toDouble(xml.s2(), &maxOk);
                              if(!maxOk)
                                fprintf(stderr, "CtrlList::read failed reading max string: %s\n",
                                        xml.s2().toLatin1().constData());
                        }
                        else if (tag == "samplerate")
                        {
                              samplerate = loc.toInt(xml.s2(), &ok);
                              if(!ok)
                                fprintf(stderr, "CtrlList::read failed reading samplerate string: %s\n",
                                        xml.s2().toLatin1().constData());
                        }
                        else
                              fprintf(stderr,"CtrlList::read unknown tag %s\n", tag.toLatin1().constData());
                        break;
                  case Xml::Text:
                        {
                          readValues(tag, samplerate);
                        }
                        break;
                  case Xml::TagEnd:
                        if (xml.s1() == "controller")
                        {
                              setId(id);
                              if(minOk && maxOk)
                                setRange(min, max);

                              return true;
                        }
                  default:
                        break;
                  }
            }
            return false;
      }

void CtrlList::write(int level, Xml& xml, bool /*isCopy*/) const
{
        QString s= QString("controller id=\"%1\" cur=\"%2\"").arg(id()).arg(curVal());
        s += QString(" color=\"%1\" visible=\"%2\"").arg(color().name()).arg(isVisible());
        xml.tag(level++, s.toLatin1().constData());
        int i = 0;
        for (ciCtrl ic = cbegin(); ic != cend(); ++ic) {
              QString s(ic->second.selected() ? "%1 %2 s, " : "%1 %2, ");
              xml.nput(level, s.arg(ic->first).arg(ic->second.value()).toLatin1().constData());
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

//---------------------------------------------------------
//   add
//---------------------------------------------------------

bool CtrlListList::add(CtrlList* vl)
      {
      // The controller id can be -1 meaning no particular id, for copy/paste etc.
      // If so, we can't add it here.
      if(vl->id() < 0)
        return false;

      return insert(std::pair<const int, CtrlList*>(vl->id(), vl)).second;
      }

bool CtrlListList::del(int id)
{
  iCtrlList i = find(id);
  if(i != end())
  {
    delete i->second;
    erase(i);
    return true;
  }
  return false;
}

bool CtrlListList::del(iCtrlList i)
{
  if(i != end())
  {
    delete i->second;
    erase(i);
    return true;
  }
  return false;
}

bool CtrlListList::updateGroups()
{
  bool res = false;
  for(iCtrlList i = begin(); i != end(); ++i)
    if(i->second->updateGroups())
      res = true;
  return res;
}

void CtrlListList::clearDelete() {
      for(iCtrlList i = begin(); i != end(); ++i)
        delete i->second;
      clear();
      }
iCtrlList CtrlListList::find(int id) {
      return std::map<int, CtrlList*, std::less<int> >::find(id);
      }
ciCtrlList CtrlListList::find(int id) const {
      return std::map<int, CtrlList*, std::less<int> >::find(id);
      }

MidiAudioCtrlMap* CtrlListList::midiControls() { return &_midi_controls; }

void CtrlListList::clearAllAutomation() {
      for(iCtrlList i = begin(); i != end(); ++i)
        i->second->clear();
      }

//---------------------------------------------------------
//   value
//   Returns value at frame for controller with id ctrlId.
//   cur_val_only means read the current 'manual' value, not from the list even if it is not empty.
//   If passed a nextFrame, sets nextFrame to the next event frame, or 0 and eFrameValid false if no next frame (wide-open), or, 
//    since CtrlList is a map, ZERO if should be replaced with some other frame by the caller (interpolation). 
//---------------------------------------------------------

double CtrlListList::value(int ctrlId, unsigned int frame, bool cur_val_only,
                           unsigned int* nextFrame, bool* nextFrameValid) const
      {
      ciCtrlList cl = find(ctrlId);
      if (cl == end())
      {
        if(nextFrameValid)
          *nextFrameValid = false;
        if(nextFrame)
          *nextFrame = 0;
        return 0.0;
      }
      
      return cl->second->value(frame, cur_val_only, nextFrame, nextFrameValid);
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

void CtrlListList::updateCurValues(unsigned int frame)
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
        cl->write(level, xml);
        }
  
  _midi_controls.write(level, xml);
}

void CtrlListList::initColors()
{
  for(iCtrlList icl = begin(); icl != end(); ++icl)
    icl->second->initColor(icl->second->id());
}

//---------------------------------------------------------
//   AutomationItem
//---------------------------------------------------------

AudioAutomationItem::AudioAutomationItem()
{
  _value = 0;
  _wrkFrame = 0;
  _wrkVal = 0;
  _groupEnd = false;
}

AudioAutomationItem::AudioAutomationItem(unsigned int frame, double value, bool groupEnd)
{
  _value = value;
  _wrkFrame = frame;
  _wrkVal = _value;
  _groupEnd = groupEnd;
}

bool AudioAutomationItemMap::addSelected(
  CtrlList* ctrlList, unsigned int frame, const AudioAutomationItem& item)
{
  AudioAutomationItemMapInsertResult res =
    insert(AudioAutomationItemMapInsertPair(ctrlList, AudioAutomationItemMapStruct()));
  AudioAutomationItemMapStruct& aal = res.first->second;
  AudioAutomationItemListInsertResult res2 = aal._selectedList.insert_or_assign(frame, item);
  return res2.second;
}

bool AudioAutomationItemMap::delSelected(CtrlList* ctrlList, unsigned int frame)
{
  iAudioAutomationItemMap iaam = find(ctrlList);
  if(iaam == end())
    return false;
  if(iaam->second._selectedList.erase(frame) > 0)
  {
    if(iaam->second._selectedList.empty())
      erase(iaam);
    return true;
  }
  return false;
}

bool AudioAutomationItemMap::clearSelected()
{
  bool ret = false;
  for(iAudioAutomationItemMap iaam = begin(); iaam != end(); ++iaam)
  {
    iaam->second._selectedList.clear();
    ret = true;
  }
  return ret;
}

bool AudioAutomationItemMap::clearSelected(CtrlList* ctrlList)
{
  iAudioAutomationItemMap iaam = find(ctrlList);
  if(iaam == end())
    return false;
  iaam->second._selectedList.clear();
  return true;
}

bool AudioAutomationItemMap::itemsAreSelected() const
{
  for(ciAudioAutomationItemMap iaam = cbegin(); iaam != cend(); ++iaam)
  {
    if(!iaam->second._selectedList.empty())
      return true;
  }
  return false;
}

bool AudioAutomationItemMap::itemsAreSelected(CtrlList* ctrlList) const
{
  ciAudioAutomationItemMap iaam = find(ctrlList);
  if(iaam == cend())
    return false;
  return !iaam->second._selectedList.empty();
}

bool AudioAutomationItemTrackMap::addSelected(
  Track* track, CtrlList* ctrlList, unsigned int frame, const AudioAutomationItem& item)
{
  AudioAutomationItemTrackMapInsertResult res =
    insert(AudioAutomationItemTrackMapInsertPair(track, AudioAutomationItemMap()));
  AudioAutomationItemMap& aam = res.first->second;
  return aam.addSelected(ctrlList, frame, item);
}

bool AudioAutomationItemTrackMap::delSelected(Track* track, CtrlList* ctrlList, unsigned int frame)
{
  iAudioAutomationItemTrackMap iaatm = find(track);
  if(iaatm == end())
    return false;
  if(!iaatm->second.delSelected(ctrlList, frame))
    return false;
  if(iaatm->second.empty())
    erase(iaatm);
  return true;
}

bool AudioAutomationItemTrackMap::clearSelected()
{
  bool ret = false;
  for(iAudioAutomationItemTrackMap iaatm = begin(); iaatm != end(); )
  {
    if(iaatm->second.clearSelected())
    {
      if(iaatm->second.empty())
        iaatm = erase(iaatm);
      else
        ++iaatm;
      ret = true;
    }
  }
  return ret;
}

bool AudioAutomationItemTrackMap::clearSelected(Track* track)
{
  iAudioAutomationItemTrackMap iaatm = find(track);
  if(iaatm == end())
    return false;
  if(!iaatm->second.clearSelected())
    return false;
  if(iaatm->second.empty())
    erase(iaatm);
  return true;
}

bool AudioAutomationItemTrackMap::clearSelected(Track* track, CtrlList* ctrlList)
{
  iAudioAutomationItemTrackMap iaatm = find(track);
  if(iaatm == end())
    return false;
  if(!iaatm->second.clearSelected(ctrlList))
    return false;
  if(iaatm->second.empty())
    erase(iaatm);
  return true;
}

bool AudioAutomationItemTrackMap::itemsAreSelected() const
{
  for(ciAudioAutomationItemTrackMap iaatm = cbegin(); iaatm != cend(); )
  {
    if(iaatm->second.itemsAreSelected())
      return true;
  }
  return false;
}

bool AudioAutomationItemTrackMap::itemsAreSelected(Track* track) const
{
  ciAudioAutomationItemTrackMap iaatm = find(track);
  if(iaatm == cend())
    return false;
  return iaatm->second.itemsAreSelected();
}

bool AudioAutomationItemTrackMap::itemsAreSelected(Track* track, CtrlList* ctrlList) const
{
  ciAudioAutomationItemTrackMap iaatm = find(track);
  if(iaatm == cend())
    return false;
  return iaatm->second.itemsAreSelected(ctrlList);
}

PasteCtrlListStruct::PasteCtrlListStruct()
{
  _minFrame = 0;
}

PasteCtrlListList::PasteCtrlListList()
  : _minFrame(0)
{
}

bool PasteCtrlListList::add(int ctrlId, const PasteCtrlListStruct& pcls)
{
  const bool isFirst = empty();
  PasteCtrlListListInsertResult res = insert(PasteCtrlListListInsertPair(ctrlId, pcls));
  if(!res.second)
    return false;
  if(!pcls._ctrlList.empty() && (isFirst || pcls._minFrame < _minFrame))
    _minFrame = pcls._minFrame;
  return true;
}

PasteCtrlTrackMap::PasteCtrlTrackMap()
  : _minFrame(0)
{
}
bool PasteCtrlTrackMap::add(const QUuid& trackUuid, const PasteCtrlListList& pcll)
{
  const bool isFirst = empty();
  PasteCtrlTrackMapInsertResult res = insert(PasteCtrlTrackMapInsertPair(trackUuid, pcll));
  if(!res.second)
    return false;
  if(!pcll.empty() && (isFirst || pcll._minFrame < _minFrame))
    _minFrame = pcll._minFrame;
  return true;
}

} // namespace MusECore
