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

#include "muse_math.h"
#include "gconfig.h"
#include "fastlog.h"
#include "globals.h"
#include "ctrl.h"
#include "midictrl.h"
#include "hex_float.h"

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
  _flags = VAL_NOFLAGS;
}

CtrlVal::CtrlVal(double v, bool selected, bool discrete, bool groupEnd)
{
  val = v;
  _flags = VAL_NOFLAGS;
  if(selected)
    _flags |= VAL_SELECTED;
  if(!groupEnd)
    _flags |= VAL_NON_GROUP_END;
  if(discrete)
    _flags |= VAL_DISCRETE;
}

CtrlVal::CtrlVal(double v, CtrlValueFlags f)
{
  val = v;
  _flags = f;
}

CtrlVal::CtrlValueFlags CtrlVal::flags() const { return _flags; }
void CtrlVal::setFlags(CtrlVal::CtrlValueFlags f) { _flags = f; }
bool CtrlVal::selected() const { return _flags & VAL_SELECTED; }
void CtrlVal::setSelected(bool v) { if(v) _flags |= VAL_SELECTED; else _flags &= ~VAL_SELECTED; }
bool CtrlVal::groupEnd() const { return !(_flags & VAL_NON_GROUP_END); }
void CtrlVal::setGroupEnd(bool v) { if(!v) _flags |= VAL_NON_GROUP_END; else _flags &= ~VAL_NON_GROUP_END;}
double CtrlVal::value() const { return val; }
void CtrlVal::setValue(double v) { val = v; }
bool CtrlVal::discrete() const { return _flags & VAL_DISCRETE; }
void CtrlVal::setDiscrete(bool v) { if(v) _flags |= VAL_DISCRETE; else _flags &= ~VAL_DISCRETE; }

CtrlRecVal::CtrlRecVal(unsigned int f, int n, double v)
  : frame(f), val(v), id(n), _flags(ARVT_NOFLAGS) {}
CtrlRecVal::CtrlRecVal(unsigned int f, int n, double v, CtrlRecValueFlags flags)
  : frame(f), val(v), id(n), _flags(flags) {}

bool CtrlRecList::addInitial(const CtrlRecVal& val)
{
  for(iCtrlRec ic = begin(); ic != end(); ++ic)
  {
    CtrlRecVal& rv = *ic;
    // When adding initial values to the list, all existing items' frames should be the same.
    if(rv.frame != val.frame)
    {
      fprintf(stderr, " Error: CtrlRecList::addInitial: Frames are not the same: %d -> %d\n", val.frame, rv.frame);
      return false;
    }
    // We found an item at the given frame. Does it not have the given id?
    if(rv.id != val.id)
      continue;
    // An existing item was found at the given frame with the given id. Replace the item.
    rv = val;
    return true;
  }
  // No existing item was found at the given frame with the given id. Add a new item.
  push_back(val);
  return true;
}

int MidiAudioCtrlStruct::id() const        { return _id; }
void MidiAudioCtrlStruct::setId(int id) {
    _id = id; }
MidiAudioCtrlStruct::IdType MidiAudioCtrlStruct::idType() const { return _idType; }
void MidiAudioCtrlStruct::setIdType(MidiAudioCtrlStruct::IdType idType) { _idType = idType; }
Track* MidiAudioCtrlStruct::track() const { return _track; }
void MidiAudioCtrlStruct::setTrack(Track* track) { _track = track; }

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
  double amin, amax, fmin, fmax;
  audio_ctrl_list->range(&amin, &amax);
  // Support 'reversed' controls.
  bool areversed;
  if(amin > amax)
  {
    fmin = amax;
    fmax = amin;
    areversed = true;
  }
  else
  {
    fmin = amin;
    fmax = amax;
    areversed = false;
  }
  double frng = fmax - fmin;             // The audio control range.

  MidiController::ControllerType t = midiControllerType(midi_ctlnum);
  CtrlValueType aud_t = audio_ctrl_list->valueType();
  const int ac_id = audio_ctrl_list->id();

  #ifdef _CTRL_DEBUG_
  fprintf(stderr, "midi2AudioCtrlValue: midi_ctlnum:%d val:%d fmin:%f fmax:%f\n", midi_ctlnum, midi_val, fmin, fmax);
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

  double fictlrng;   // Float version of the integer midi range.
  double normval;  // Float version of the normalized midi value.

  // ----------  TODO: Do stuff with the mapper, if supplied.

  if(aud_t == VAL_LOG)
  {
    double ret, fmindb, fmaxdb, frngdb;
    // If the audio controller minimum is 0.0 (-inf dB).
    if(fmin <= 0.0)
    {
      // If the midi value is zero.
      if(bval == 0)
      {
        // Return 0.0 (-inf dB).
        #ifdef _CTRL_DEBUG_
        fprintf(stderr, "midi2AudioCtrlValue: is VAL_LOG, audio controller min is zero, midi value is 0, returning 0.\n");
        #endif
        return 0.0;
      }

      // Adjust the midi scale bottom end by +1.
      fictlrng = double(ctlmx - (ctlmn + 1));
    }
    else
    {
      fictlrng = double(ctlmx - ctlmn);
    }

    normval = double(bval) / fictlrng;

    // Special support for built-in audio volume controllers.
    if(ac_id == AC_VOLUME)
    {
      // If the audio controller minimum is 0.0 (-inf dB).
      if(fmin <= 0.0)
        // For the volume control, use the app's minimum slider dB setting directly,
        //  since that's what it's meant for.
        fmindb = MusEGlobal::config.minSlider;
      else
        fmindb = 20.0*log10(fmin);
      fmaxdb = 20.0*log10(fmax);
    }
    else
    {
      // For all other controls, find an appropriate minimum value.
      const double fminhint = museRangeMinValHint(fmin, fmax, true, false, false, MusEGlobal::config.minSlider);
      // FIXME: Although this should be correct, some sliders show "---" at top end, some don't.
      // Possibly because of use of fast_log10 in value(), and in sliders and automation IIRC.
      fmindb = 20.0*log10(fminhint);
      fmaxdb = 20.0*log10(fmax);
    }

    frngdb = fmaxdb - fmindb;
    if(areversed)
      ret = exp10((fmaxdb - normval * frngdb) / 20.0);
    else
      ret = exp10((normval * frngdb + fmindb) / 20.0);

    if(ret < fmin)
      ret = fmin;
    if(ret > fmax)
      ret = fmax;

    #ifdef _CTRL_DEBUG_
    fprintf(stderr, "midi2AudioCtrlValue: is VAL_LOG normval:%f frng:%f frngdb:%f returning:%f\n", normval, frng, frngdb, ret);
    #endif
    return ret;
  }

  fictlrng = double(ctlmx - ctlmn);
  normval = double(bval) / fictlrng;

  if(aud_t == VAL_LINEAR)
  {
    double ret;
    if(areversed)
      ret = fmax - normval * frng;
    else
      ret = normval * frng + fmin;
    if(ret < fmin)
      ret = fmin;
    if(ret > fmax)
      ret = fmax;
    #ifdef _CTRL_DEBUG_
    fprintf(stderr, "midi2AudioCtrlValue: is VAL_LINEAR normval:%f frng:%f returning:%f\n", normval, frng, ret);
    #endif
    return ret;
  }

  // TODO VAL_ENUM should be handled separately. Should not assume it is consecutive integers.
  if(aud_t == VAL_INT || aud_t == VAL_ENUM)
  {
    double ret;
    if(areversed)
      ret = int(fmax - normval * frng);
    else
      ret = int(normval * frng + fmin);
    if(ret < fmin)
      ret = fmin;
    if(ret > fmax)
      ret = fmax;
    #ifdef _CTRL_DEBUG_
    fprintf(stderr, "midi2AudioCtrlValue: is VAL_INT returning:%f\n", ret);
    #endif
    return ret;
  }

  if(aud_t == VAL_BOOL)
  {
    #ifdef _CTRL_DEBUG_
    fprintf(stderr, "midi2AudioCtrlValue: is VAL_BOOL\n");
    #endif
    // From official midi 1.0 specs:
    // "If a receiver is expecting switch information it should recognize 0-63 (00H-3FH) as "OFF"
    //   and 64-127 (40H-7FH) as "ON". This is because a receiver has no way of knowing whether the
    //   message information is from a switch or a continuous controller."
    //if(midi_val > ((ctlmx - ctlmn)/2 + ctlmn))
    if(areversed)
    {
      if((fmax - normval * frng) > (fmax - frng/2.0))
        return fmin;
      else
        return fmax;
    }
    else
    {
      if((normval * frng + fmin) > (frng/2.0 + fmin))
        return fmax;
      else
        return fmin;
    }
  }

  // TODO Where to grab the enum list from?
  //if(aud_t == VAL_ENUM)
  //{
  //}

  fprintf(stderr, "midi2AudioCtrlValue: unknown audio controller type:%d\n", aud_t);
  return 0.0;
}

//---------------------------------------------------------
// Midi to audio controller stuff
//---------------------------------------------------------

MidiAudioCtrlStruct::MidiAudioCtrlStruct()  
 : _idType(AudioControl), _id (0), _track(nullptr)
{ 
};

MidiAudioCtrlStruct::MidiAudioCtrlStruct(
  MidiAudioCtrlStruct::IdType idType, int id, Track* track)
 : _idType(idType), _id(id), _track(track)
{ 
};

// Static.
MidiAudioCtrlMap_idx_t MidiAudioCtrlMap::index_hash(int midi_port, int midi_chan, int midi_ctrl_num)
{ 
  return ((MidiAudioCtrlMap_idx_t(midi_port) & 0xff) << 24) | 
          ((MidiAudioCtrlMap_idx_t(midi_chan) & 0xf) << 20) | 
          (MidiAudioCtrlMap_idx_t(midi_ctrl_num) & 0xfffff);  
}

// Static.
void MidiAudioCtrlMap::hash_values(MidiAudioCtrlMap_idx_t hash, int* midi_port, int* midi_chan, int* midi_ctrl_num)
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
    if(imacp->second.idType() == macs.idType() && imacp->second.id() == macs.id())
       return imacp;
  return insert(std::pair<MidiAudioCtrlMap_idx_t, MidiAudioCtrlStruct >(h, macs));
}

void MidiAudioCtrlMap::erase_ctrl_struct(int midi_port, int midi_chan, int midi_ctrl_num, MidiAudioCtrlStruct::IdType type, int id)
{
  MidiAudioCtrlMap_idx_t h = index_hash(midi_port, midi_chan, midi_ctrl_num);
  std::pair<iMidiAudioCtrlMap, iMidiAudioCtrlMap> range = equal_range(h);
  MidiAudioCtrlMap macm;
  macm.insert(range.first, range.second);
  for(iMidiAudioCtrlMap imacm = macm.begin(); imacm != macm.end(); ++imacm)
    if(imacm->second.idType() == type && imacm->second.id() == id)
       erase(imacm);
}

void MidiAudioCtrlMap::find_audio_ctrl_structs(
  MidiAudioCtrlStruct::IdType type, int id,
  const Track* track, bool anyTracks, bool includeNullTracks, AudioMidiCtrlStructMap* amcs) //const
{
  for(iMidiAudioCtrlMap imacm = begin(); imacm != end(); ++imacm)
  {
    const Track* t = imacm->second.track();
    if(imacm->second.idType() == type && imacm->second.id() == id &&
      (t == track ||
      (t == nullptr && includeNullTracks) ||
      (anyTracks && (t != nullptr || includeNullTracks)) ))
      amcs->push_back(imacm);
  }
}

void MidiAudioCtrlMap::write(int level, Xml& xml, const Track* track) const
{
  for(ciMidiAudioCtrlMap imacm = begin(); imacm != end();  ++imacm)
  {
      // Write only the assignments for the given track pointer (which can be NULL).
      if(imacm->second.track() != track)
        continue;
      int port, chan, mctrl;
      hash_values(imacm->first, &port, &chan, &mctrl);
      const int id = imacm->second.id();
      const MidiAudioCtrlStruct::IdType type = imacm->second.idType();
      QString s= QString("midiAssign port=\"%1\" ch=\"%2\" mctrl=\"%3\" type=\"%4\" id=\"%5\"")
                          .arg(port)
                          .arg(chan)
                          .arg(mctrl)
                          .arg(type)
                          .arg(id);
      xml.tag(level++, s.toLatin1().constData());

      // TODO
      //const MidiAudioCtrlStruct& macs = imacs->second;
      //xml.intTag(level, "macs ???", macs.);

      xml.etag(level--, "midiAssign");
  }
}

//---------------------------------------------------------
//   read
//---------------------------------------------------------

void MidiAudioCtrlMap::read(Xml& xml, Track* track)
      {
      int port = -1, chan = -1, midi_ctrl = -1;
      MidiAudioCtrlStruct macs(MidiAudioCtrlStruct::AudioControl, -1, track);

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
                                printf("MidiAudioCtrlMap::read failed reading port string: %s\n", xml.s2().toLatin1().constData());
                              }
                        }
                        else if (tag == "ch")
                        {
                              chan = loc.toInt(xml.s2(), &ok);
                              if(!ok)
                              {
                                ++errcount;
                                printf("MidiAudioCtrlMap::read failed reading ch string: %s\n", xml.s2().toLatin1().constData());
                              }
                        }
                        else if (tag == "mctrl")
                        {
                              midi_ctrl = loc.toInt(xml.s2(), &ok);
                              if(!ok)
                              {
                                ++errcount;
                                printf("MidiAudioCtrlMap::read failed reading mctrl string: %s\n", xml.s2().toLatin1().constData());
                              }
                        }
                        else if (tag == "type")
                        {
                              const int type = loc.toInt(xml.s2(), &ok);
                              if(ok)
                                macs.setIdType(MidiAudioCtrlStruct::IdType(type));
                              else
                              {
                                ++errcount;
                                printf("MidiAudioCtrlPortMap::read failed reading type string: %s\n", xml.s2().toLatin1().constData());
                              }
                        }
                        // Tag actrl is obsolete, changed to id now.
                        else if (tag == "actrl" || tag == "id")
                        {
                              macs.setId(loc.toInt(xml.s2(), &ok));
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
                        // Tag midiMapper is obsolete, changed to midiAssign now.
                        if (xml.s1() == "midiMapper" || xml.s1() == "midiAssign")
                        {
                              if(errcount == 0 && port != -1 && chan != -1 && midi_ctrl != -1 && macs.id() != -1)
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
      _min     = 0.0;
      _max     = 1.0;
      _valueType = VAL_LINEAR;
      _dontShow = dontShow;
      _visible = false;
      _valueUnit = -1;
      _displayHint = DisplayDefault;
      initColor(0);
      }

CtrlList::CtrlList(int id, bool dontShow)
      {
      _id      = id;
      _default = 0.0;
      _curVal  = 0.0;
      _mode    = INTERPOLATE;
      _min     = 0.0;
      _max     = 1.0;
      _valueType = VAL_LINEAR;
      _valueUnit = -1;
      _displayHint = DisplayDefault;

      _dontShow = dontShow;
      _visible = false;
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
      _valueUnit = -1;
      _displayHint = DisplayDefault;
      initColor(id);
}

CtrlList::CtrlList(const CtrlList& l, int flags)
{
  _id          = l._id;
  _valueType   = l._valueType;
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
  _valueUnit     = cl._valueUnit;
  _displayHint   = cl._displayHint;
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
    _valueUnit     = l._valueUnit;
    _displayHint   = l._displayHint;
  }
  
  if(flags & ASSIGN_VALUES)
    CtrlList_t::operator=(l); // Let map copy the items.
}

//---------------------------------------------------------
//   getInterpolation
//   Fills CtrlInterpolate struct for given frame.
//   cur_val_only means read the current 'manual' value, not from the list even if it is not empty.
//   CtrlInterpolate member eFrameValid can be false meaning no next value (wide-open, endless).
//---------------------------------------------------------

void CtrlList::getInterpolation(unsigned int frame, bool cur_val_only, CtrlInterpolate* interp) const
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
  }
  else if(i == begin())
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
    const bool cvDiscrete = i->second.discrete();

    // For now we do not allow interpolation of integer or enum controllers.
    // TODO: It would require custom line drawing and corresponding hit detection.
    if(_mode == DISCRETE || cvDiscrete)
      interp->doInterp = false;
    else
      interp->doInterp = (interp->eVal != interp->sVal && interp->eFrame > interp->sFrame);
  }
}

//---------------------------------------------------------
//   interpolate
//   Returns interpolated value at given frame, from a CtrlInterpolate struct.
//   For speed, no checking is done for frame = frame2, or even CtrlInterpolate::doInterp.
//   Those are to be taken care of before calling this routine. See getInterpolation().
//---------------------------------------------------------

double CtrlList::interpolate(unsigned int frame, const CtrlInterpolate& interp) const
{
  const unsigned int frame1 = interp.sFrame;
  const unsigned int frame2 = interp.eFrame;
  double val1 = interp.sVal;
  double val2 = interp.eVal;

  const bool islog = _valueType == MusECore::CtrlValueType::VAL_LOG;
  const double clmax = museMax(_min, _max);
  const double clmin = museMin(_min, _max);
  const double clmin_lim = museRangeMinValHint(
    clmin, clmax,
    islog,
    _valueType == MusECore::CtrlValueType::VAL_INT,
    _displayHint == MusECore::CtrlList::DisplayLogDB,
    MusEGlobal::config.minSlider,
    0.05);

  //--------------------------------------------------
  // Handle special cases we can get out of the way.
  //--------------------------------------------------
  if(!interp.eFrameValid || frame >= frame2)
  {
    if(islog && val2 <= clmin_lim)
    {
      // If the minimum allows to go to zero, and if the value is equal
      //  to or less than our minimum setting, make it jump to 0.0 (-inf).
      if(clmin <= 0.0)
        return 0.0;
      // Otherwise just limit the value to the minimum.
      else
        return clmin_lim;
    }
    return val2;
  }
  if(frame <= frame1 || val1 == val2)
  {
    if(islog && val1 <= clmin_lim)
    {
      if(clmin <= 0.0)
        return 0.0;
      else
        return clmin_lim;
    }
    return val1;
  }

  double rv;
  switch(_valueType)
  {
    case VAL_LOG:
      if(val1 <= clmin_lim)
        val1 = clmin_lim;
      val1 = 20.0*fast_log10(val1);
      if(val2 <= clmin_lim)
        val2 = clmin_lim;
      val2 = 20.0*fast_log10(val2);
      val1 += (double(frame - frame1) * (val2 - val1)) / double(frame2 - frame1);
      val1 = exp10(val1/20.0);
      rv = val1;
    break;

    case VAL_LINEAR:
      val1 += (double(frame - frame1) * (val2 - val1)) / double(frame2 - frame1);
      rv = val1;
    break;

    case VAL_INT:
    {
      val1 += (double(frame - frame1) * (val2 - val1)) / double(frame2 - frame1);
      // Here we want to round halfway cases rather than floor or ceil
      //  so that the result is split evenly on the graph.
      val1 = round(val1);
      // Now we must make sure the result isn't out of bounds.
      // If the min or max happen to be in between values, we need to truncate them.
      const double mint = trunc(clmin);
      const double maxt = trunc(clmax);
      if(val1 < mint)
        val1 = mint;
      if(val1 > maxt)
        val1 = maxt;
      rv = val1;
    }
    break;

    case VAL_BOOL:
    case VAL_ENUM:
      rv = val1;
    break;
  }

  return rv;
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
  // if we are past all items just return the last value
  if (i == end())
  {
    --i;
    if(nextFrameValid)
      *nextFrameValid = false;
    if(nextFrame)
      *nextFrame = 0;
    return i->second.value();
  }
  else if(i == begin())
  {
    nframe = i->first;
    rv = i->second.value();
  }
  else
  {
    const unsigned int frame2 = i->first;
    double val2 = i->second.value();
    --i;
    const unsigned int frame1 = i->first;
    double val1   = i->second.value();
    const bool cvDiscrete = i->second.discrete();

    // For now we do not allow interpolation of integer or enum controllers.
    // TODO: It would require custom line drawing and corresponding hit detection.
    if(_mode == DISCRETE || cvDiscrete)
    {
      nframe = frame2;
      rv = val1;
    }
    else
    {
      if(val2 != val1)
        nframe = 0; // Zero signifies the next frame should be determined by caller.
      else
        nframe = frame2;

      switch(_valueType)
      {
        case VAL_LOG:
        {
          const double clmax = museMax(_min, _max);
          const double clmin = museMin(_min, _max);
          const double clmin_lim = museRangeMinValHint(
            clmin, clmax,
            true,
            false,
            _displayHint == MusECore::CtrlList::DisplayLogDB,
            MusEGlobal::config.minSlider,
            0.05);

          //--------------------------------------------------
          // Handle special cases we can get out of the way.
          //--------------------------------------------------
          if(frame >= frame2)
          {
            if(val2 <= clmin_lim)
            {
              // If the minimum allows to go to zero, and if the value is equal
              //  to or less than our minimum setting, make it jump to 0.0 (-inf).
              if(clmin <= 0.0)
                val2 = 0.0;
              // Otherwise just limit the value to the minimum.
              else
                val2 = clmin_lim;
            }
            rv = val2;
          }
          else if(frame <= frame1 || val1 == val2)
          {
            if(val1 <= clmin_lim)
            {
              if(clmin <= 0.0)
                val1 = 0.0;
              else
                val1 = clmin_lim;
            }
            rv = val1;
          }
          else
          {
            if(val1 <= clmin_lim)
              val1 = clmin_lim;
            if(val2 <= clmin_lim)
              val2 = clmin_lim;
            val1 = 20.0*fast_log10(val1);
            val2 = 20.0*fast_log10(val2);
            val1 += (double(frame - frame1) * (val2 - val1)) / double(frame2 - frame1);
            val1 = exp10(val1 / 20.0);
            rv = val1;
          }
        }
        break;

        case VAL_LINEAR:
          val1 += (double(frame - frame1) * (val2 - val1)) / double(frame2 - frame1);
          rv = val1;
        break;

        case VAL_INT:
        {
          val1 += (double(frame - frame1) * (val2 - val1)) / double(frame2 - frame1);
          // Here we want to round halfway cases rather than floor or ceil
          //  so that the result is split evenly on the graph.
          val1 = round(val1);
          // Now we must make sure the result isn't out of bounds.
          // If the min or max happen to be in between values, we need to truncate them.
          const double mint = trunc(museMin(_min, _max));
          const double maxt = trunc(museMax(_min, _max));
          if(val1 < mint)
            val1 = mint;
          if(val1 > maxt)
            val1 = maxt;
          rv = val1;
        }
        break;

        case VAL_BOOL:
        case VAL_ENUM:
          nframe = frame2;
          rv = val1;
        break;
      }
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
int CtrlList::valueUnit() const { return _valueUnit; }
void CtrlList::setValueUnit(int idx) { _valueUnit = idx; }
CtrlList::DisplayHints CtrlList::displayHint() const { return _displayHint; }
void CtrlList::setDisplayHint(const CtrlList::DisplayHints &h) { _displayHint = h; }

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
  
  _curVal = val;
}

//------------------------------------------
// The following methods are for debugging
//------------------------------------------

#ifdef _CTRLLIST_DEBUG_METHODS_

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
  return *this;
}

void CtrlList::swap(CtrlList& cl) noexcept
{
#ifdef _CTRL_DEBUG_
  printf("CtrlList::swap id:%d\n", cl.id());  
#endif
  CtrlList_t::swap(cl);
}

std::pair<iCtrl, bool> CtrlList::insert(const CtrlListInsertPair_t& p)
{
#ifdef _CTRL_DEBUG_
  printf("CtrlList::insert1 frame:%u val:%f\n", p.first, p.second.val);
#endif
  std::pair<iCtrl, bool> res = CtrlList_t::insert(p);
  return res;
}

template< class P > std::pair<iCtrl, bool> CtrlList::insert(P&& value)
{
#ifdef _CTRL_DEBUG_
  printf("CtrlList::insert2 frame:%u val:%f\n", value.first, value.second.val);
#endif
  std::pair<iCtrl, bool> res = CtrlList_t::insert(value);
  return res;
}

std::pair<iCtrl, bool> CtrlList::insert(CtrlListInsertPair_t&& p)
{
#ifdef _CTRL_DEBUG_
  printf("CtrlList::insert3 frame:%u val:%f\n", p.first, p.second.val);
#endif
  std::pair<iCtrl, bool> res = CtrlList_t::insert(p);
  return res;
}

iCtrl CtrlList::insert(ciCtrl ic, const CtrlListInsertPair_t& p)
{
#ifdef _CTRL_DEBUG_
  printf("CtrlList::insert4 frame:%u val:%f\n", p.first, p.second.val);
#endif
  iCtrl res = CtrlList_t::insert(ic, p);
  return res;
}

template< class P > iCtrl CtrlList::insert(ciCtrl ic, P&& value)
{
#ifdef _CTRL_DEBUG_
  printf("CtrlList::insert5 frame:%u val:%f\n", value.first, value.second.val);
#endif
  iCtrl res = CtrlList_t::insert(ic, value);
  return res;
}

iCtrl CtrlList::insert(ciCtrl ic, CtrlListInsertPair_t&& value)
{
#ifdef _CTRL_DEBUG_
  printf("CtrlList::insert6 frame:%u val:%f\n", value.first, value.second.val);
#endif
  iCtrl res = CtrlList_t::insert(ic, value);
  return res;
}

void CtrlList::insert(iCtrl first, iCtrl last)
{
#ifdef _CTRL_DEBUG_
  printf("CtrlList::insert7 first frame:%u last frame:%d\n", first->first, last->first);
#endif
  CtrlList_t::insert(first, last);
}

// void CtrlList::insert(std::initializer_list<CtrlListInsertPair_t> ilist)
// {
// #ifdef _CTRL_DEBUG_
//   printf("CtrlList::insert8\n");
// #endif
//   // TODO: Hm, error, no matching member function.
//   CtrlList_t::insert(ilist);
// }

// CtrlList::insert_return_type CtrlList::insert(CtrlList::node_type&& nh)
// {
// #ifdef _CTRL_DEBUG_
//   printf("CtrlList::insert9\n");
// #endif
//   // TODO: Hm, error, no matching member function.
//   CtrlList::insert_return_type ret = CtrlList_t::insert(nh);
//   return ret;
// }

// iCtrl CtrlList::insert(ciCtrl ic, CtrlList::node_type&& nh)
// {
// #ifdef _CTRL_DEBUG_
//   printf("CtrlList::insert10\n");
// #endif
//   // TODO: Hm, error, no matching member function.
//   iCtrl ret = CtrlList_t::insert(ic, nh);
//   return ret;
// }

template <class M> std::pair<iCtrl, bool> CtrlList::insert_or_assign(const unsigned int& k, M&& obj)
{
#ifdef _CTRL_DEBUG_
  printf("CtrlList::insert_or_assign1 frame:%u\n", k);
#endif
  std::pair<iCtrl, bool> res = CtrlList_t::insert_or_assign(k, obj);
  return res;
}

template <class M> std::pair<iCtrl, bool> CtrlList::insert_or_assign(unsigned int&& k, M&& obj)
{
#ifdef _CTRL_DEBUG_
  printf("CtrlList::insert_or_assign2 frame:%u\n", k);
#endif
  std::pair<iCtrl, bool> res = CtrlList_t::insert_or_assign(k, obj);
  return res;
}

template <class M> iCtrl CtrlList::insert_or_assign(ciCtrl hint, const unsigned int& k, M&& obj)
{
#ifdef _CTRL_DEBUG_
  printf("CtrlList::insert_or_assign3 frame:%u\n", k);
#endif
  iCtrl res = CtrlList_t::insert_or_assign(hint, k, obj);
  return res;
}

template <class M> iCtrl CtrlList::insert_or_assign(ciCtrl hint, unsigned int&& k, M&& obj)
{
#ifdef _CTRL_DEBUG_
  printf("CtrlList::insert_or_assign4 frame:%u\n", k);
#endif
  iCtrl res = CtrlList_t::insert_or_assign(hint, k, obj);
  return res;
}

iCtrl CtrlList::erase(iCtrl ictl)
{
#ifdef _CTRL_DEBUG_
  printf("CtrlList::erase1A iCtrl frame:%u val:%f\n", ictl->first, ictl->second.val);
#endif
  iCtrl res = CtrlList_t::erase(ictl);
  return res;
}

iCtrl CtrlList::erase(ciCtrl ictl)
{
#ifdef _CTRL_DEBUG_
  printf("CtrlList::erase1B iCtrl frame:%u val:%f\n", ictl->first, ictl->second.val);
#endif
  iCtrl res = CtrlList_t::erase(ictl);
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
  return res;
}

CtrlList_t::size_type CtrlList::erase(unsigned int frame)
{
#ifdef _CTRL_DEBUG_
  printf("CtrlList::erase3 frame:%u\n", frame);
#endif
  size_type res = CtrlList_t::erase(frame);
  return res;
}

void CtrlList::clear() noexcept
{
#ifdef _CTRL_DEBUG_
  printf("CtrlList::clear\n");  
#endif
  CtrlList_t::clear();
}

#endif // _CTRLLIST_DEBUG_METHODS_


//---------------------------------------------------------
//   add
//   Add, or replace, an event at time frame having value val. 
//---------------------------------------------------------

std::pair<CtrlList::iterator, bool> CtrlList::add(unsigned int frame, double value, bool selected, bool discrete, bool groupEnd)
      {
      return insert_or_assign(frame, CtrlVal(value, selected, discrete, groupEnd));
      }

std::pair<CtrlList::iterator, bool> CtrlList::add(unsigned int frame, const CtrlVal& cv)
      {
      return insert_or_assign(frame, cv);
      }

std::pair<CtrlList::iterator, bool> CtrlList::add(unsigned int frame, double value, CtrlVal::CtrlValueFlags flags)
      {
      return insert_or_assign(frame, CtrlVal(value, flags));
      }

//---------------------------------------------------------
//   modify
//---------------------------------------------------------

std::pair<CtrlList::iterator, bool> CtrlList::modify(unsigned int frame, double value, CtrlVal::CtrlValueFlags flags,
  CtrlVal::CtrlModifyValueFlags validModifyFlags, CtrlVal::CtrlModifyValueFlags validAddFlags)
{
  iterator ic = find(frame);
  if(ic == end())
  {
    const CtrlVal::CtrlModifyValueFlags f = validAddFlags & CtrlVal::VAL_MODIFY_SAME_AS ? validModifyFlags : validAddFlags;
    return insert(CtrlListInsertPair_t(frame, CtrlVal(f & CtrlVal::VAL_MODIFY_VALUE ? value : 0.0, (flags & f) & CtrlVal::VAL_FLAGS_MASK)));
  }
  else
  {
    const CtrlVal::CtrlModifyValueFlags f = validModifyFlags & CtrlVal::VAL_MODIFY_SAME_AS ? validAddFlags : validModifyFlags;
    modify(ic, value, flags, f);
  }
  return std::pair<iterator, bool>(ic, false);
}

void CtrlList::modify(iterator ic, double value, CtrlVal::CtrlValueFlags flags, CtrlVal::CtrlModifyValueFlags validModifyFlags)
{
  if(validModifyFlags & CtrlVal::VAL_MODIFY_VALUE)
    ic->second.setValue(value);
  ic->second.setFlags(((ic->second.flags() & ~validModifyFlags) | (flags & validModifyFlags)) & CtrlVal::VAL_FLAGS_MASK);
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
  _curVal = v;
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
  CtrlVal::CtrlValueFlags flags = CtrlVal::VAL_NOFLAGS;

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

    // Accept either decimal or hex value strings.
    val = MusELib::museStringToDouble(vs, &ok);
    if(!ok)
    {
      fprintf(stderr,"CtrlList::readValues failed reading value string: %s\n", vs.toLatin1().constData());
      break;
    }

    // Check for optional info before the comma.
    sel = false;
    groupEnd = false;
    bool optErr = false;
    flags = CtrlVal::VAL_NOFLAGS;
    QString flagsStr;
    while(i < len && tag[i] != ',')
    {
      while(i < len && (tag[i] == ' ' || tag[i] == '\n'))
        ++i;
      if(i == len)
      {
        optErr = true;
        break;
      }

      QString os;
      while(i < len && tag[i] != ' ' && tag[i] != ',')
      {
        os.append(tag[i]);
        flagsStr.append(tag[i]);
        ++i;
      }

      // The 'selected' optional character. Obsolete.
      if(os == QString('s'))
        sel = true;
      // The 'group end' optional character. Obsolete.
      else if(os == QString('g'))
        groupEnd = true;
      else
      {
        flags = CtrlVal::CtrlValueFlags(os.toInt(&ok));
        if(!ok)
        {
          fprintf(stderr, "CtrlList::readValues failed: unrecognized optional item string: %s\n", os.toLatin1().constData());
          optErr = true;
          break;
        }
      }
    }
    if(optErr)
      break;

    // For now, this conversion only has a TEMPORARY effect during song loading.
    // See comments in Song::read at the "samplerate" tag.
    frame = MusEGlobal::convertFrame4ProjectSampleRate(frame, samplerate);

    // If these obsolete items are found, force the newer corresponding flags
    if(sel)
      flags |= CtrlVal::VAL_SELECTED;
    // Note the inverted logic.
    if(!groupEnd)
      flags |= CtrlVal::VAL_NON_GROUP_END;
    // Add will replace if found.
    // Although we should limit the value to min/max right now, we can't do that yet
    //  because min/max might not have been set yet.
    add(frame, val, flags);

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
                              // Accept either decimal or hex value strings.
                              _curVal = MusELib::museStringToDouble(xml.s2(), &ok);
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
                              // Accept either decimal or hex value strings.
                              min = MusELib::museStringToDouble(xml.s2(), &minOk);
                              if(!minOk)
                                fprintf(stderr, "CtrlList::read failed reading min string: %s\n",
                                        xml.s2().toLatin1().constData());
                        }
                        else if (tag == "max")
                        {
                              // Accept either decimal or hex value strings.
                              max = MusELib::museStringToDouble(xml.s2(), &maxOk);
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
        // Use hex value string when appropriate.
        QString s= QString("controller id=\"%1\" cur=\"%2\"").arg(id()).arg(MusELib::museStringFromDouble(curVal()));
        s += QString(" color=\"%1\" visible=\"%2\"").arg(color().name()).arg(isVisible());
        xml.tag(level++, s.toLatin1().constData());
        int i = 0;
        for (ciCtrl ic = cbegin(); ic != cend(); ++ic) {
              // Write the item's frame and value.
              // Use hex value string when appropriate.
              QString ss = QString("%1 %2").arg(ic->first).arg(MusELib::museStringFromDouble(ic->second.value()));

              CtrlVal::CtrlValueFlags flags = ic->second.flags();
              // This write() is for the song files. Strip out some flags to avoid clutter since
              //  they do not need to be stored or restored.
              flags &= ~CtrlVal::VAL_NON_GROUP_END;
              if(flags != CtrlVal::VAL_NOFLAGS)
                ss += QString(" %1").arg(flags);
              ss += ", ";
              xml.nput(level, ss.toLatin1().constData());

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
  _discrete = true;
}

AudioAutomationItem::AudioAutomationItem(unsigned int frame, double value, bool groupEnd, /*bool defaultInterpolate,*/ bool discrete)
{
  _value = value;
  _wrkFrame = frame;
  _wrkVal = _value;
  _groupEnd = groupEnd;
  _discrete = discrete;
}

AudioAutomationItem::AudioAutomationItem(unsigned int frame, const CtrlVal& cv)
{
  _value = cv.value();
  _wrkFrame = frame;
  _wrkVal = _value;
  _groupEnd = cv.groupEnd();
  _discrete = cv.discrete();
}

bool AudioAutomationItemMap::addSelected(
  int id, unsigned int frame, const AudioAutomationItem& item)
{
  AudioAutomationItemMapInsertResult res =
    insert(AudioAutomationItemMapInsertPair(id, AudioAutomationItemMapStruct()));
  AudioAutomationItemMapStruct& aal = res.first->second;
  AudioAutomationItemListInsertResult res2 = aal._selectedList.insert_or_assign(frame, item);
  return res2.second;
}

bool AudioAutomationItemMap::delSelected(int id, unsigned int frame)
{
  iAudioAutomationItemMap iaam = find(id);
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

bool AudioAutomationItemMap::clearSelected(int id)
{
  iAudioAutomationItemMap iaam = find(id);
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

bool AudioAutomationItemMap::itemsAreSelected(int id) const
{
  ciAudioAutomationItemMap iaam = find(id);
  if(iaam == cend())
    return false;
  return !iaam->second._selectedList.empty();
}

bool AudioAutomationItemTrackMap::addSelected(
  const Track* track, int id, unsigned int frame, const AudioAutomationItem& item)
{
  AudioAutomationItemTrackMapInsertResult res =
    insert(AudioAutomationItemTrackMapInsertPair(track, AudioAutomationItemMap()));
  AudioAutomationItemMap& aam = res.first->second;
  return aam.addSelected(id, frame, item);
}

bool AudioAutomationItemTrackMap::delSelected(const Track* track, int id, unsigned int frame)
{
  iAudioAutomationItemTrackMap iaatm = find(track);
  if(iaatm == end())
    return false;
  if(!iaatm->second.delSelected(id, frame))
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

bool AudioAutomationItemTrackMap::clearSelected(const Track* track)
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

bool AudioAutomationItemTrackMap::clearSelected(const Track* track, int id)
{
  iAudioAutomationItemTrackMap iaatm = find(track);
  if(iaatm == end())
    return false;
  if(!iaatm->second.clearSelected(id))
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

bool AudioAutomationItemTrackMap::itemsAreSelected(const Track* track) const
{
  ciAudioAutomationItemTrackMap iaatm = find(track);
  if(iaatm == cend())
    return false;
  return iaatm->second.itemsAreSelected();
}

bool AudioAutomationItemTrackMap::itemsAreSelected(const Track* track, int id) const
{
  ciAudioAutomationItemTrackMap iaatm = find(track);
  if(iaatm == cend())
    return false;
  return iaatm->second.itemsAreSelected(id);
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

CtrlGUIMessage::CtrlGUIMessage()
  : _type(PAINT_UPDATE), _id(-1), _frame(0), _value(0.0)
{
}

CtrlGUIMessage::CtrlGUIMessage(const Track* track, int id, unsigned int frame, double value, Type type)
  : _type(type), _track(track), _id(id), _frame(frame), _value(value)
{
}

} // namespace MusECore
