//=============================================================================
//  MusE
//  Linux Music Editor
//  $Id:$
//
//  Copyright (C) 2002-2006 by Werner Schweer and others
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License version 2.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//=============================================================================

#include "track.h"
#include "midiplugin.h"
#include "song.h"
#include "al/xml.h"
#include "icons.h"
#include "muse.h"
#include "audio.h"
#include "midi.h"
#include "gconfig.h"
#include "midictrl.h"
#include "instruments/minstrument.h"
#include "part.h"
#include "gui.h"
#include "midioutport.h"
#include "midichannel.h"
#include "driver/audiodev.h"
#include "driver/mididev.h"

// synchronize with TrackType!:

const char* Track::_cname[] = {
      "AudioOut", "Group", "Wave", "AudioIn",
      "Synth", "Midi", "MidiOut", "MidiIn", "M-Ch", "M-Synth"
      };

const char* Track::_clname[] = {
      "Audio Output", "Audio Group", "Wave Track", "Audio Input",
      "Synti", "Midi Track", "Midi Output", "Midi Input", "Midi Channel",
      "Midi Synth"
      };

//---------------------------------------------------------
//   ArrangerTrack
//---------------------------------------------------------

ArrangerTrack::ArrangerTrack() 
	{
	tw   = 0;
      ctrl = -1;        // first ctrl in list
      controller = 0;
      h    = defaultTrackHeight;
      }

//---------------------------------------------------------
//   ccolor
//    return track specific track background color
//---------------------------------------------------------

QColor Track::ccolor() const
      {
      return config.trackBg[type()];
      }

//---------------------------------------------------------
//   pixmap
//---------------------------------------------------------

QPixmap* Track::pixmap(TrackType t)
      {
      switch(t) {
            case AUDIO_OUTPUT: return addtrack_audiooutputIcon;
            case AUDIO_GROUP:  return addtrack_audiogroupIcon;
            case WAVE:         return addtrack_wavetrackIcon;
            case AUDIO_INPUT:  return addtrack_audioinputIcon;
            case AUDIO_SOFTSYNTH: return addtrack_audioinputIcon; // DEBUG
            default:
            case MIDI:         return addtrack_addmiditrackIcon;
            case MIDI_OUT:     return addtrack_addmiditrackIcon;
            case MIDI_IN:      return addtrack_addmiditrackIcon;
            case MIDI_CHANNEL: return addtrack_addmiditrackIcon;
            }
      }

//---------------------------------------------------------
//   Track
//---------------------------------------------------------

Track::Track()
      {
      _tt            = AL::TICKS;
      _recordFlag    = false;
      _monitor       = false;
      _mute          = false;
      _solo          = false;
      _off           = false;
      _channels      = 0;           // 1 - mono, 2 - stereo
      _selected      = false;
      _locked        = false;
      _autoRead      = false;
      _autoWrite     = false;

      for (int i = 0; i < MAX_CHANNELS; ++i) {
            _meter[i]     = 0.0f;
            _peak[i]      = 0.0f;
            _peakTimer[i] = 0;
            }
      _sendSync       = false;
      _deviceId       = 127;
      _parts = new PartList;
      }

//---------------------------------------------------------
//   ~Track
//---------------------------------------------------------

Track::~Track()
	{
      delete _parts;

      for (int i = 0; i < MAX_CHANNELS; ++i) {
            if (!_alsaPort[i].isZero())
                  midiDriver->unregisterPort(_alsaPort[i]);
            if (!_jackPort[i].isZero())
                  audioDriver->unregisterPort(_jackPort[i]);
            }
      
      }

//---------------------------------------------------------
//   setDefaultName
//    generate unique name for track
//---------------------------------------------------------

void Track::setDefaultName()
      {
      QString base;
      switch(type()) {
            case MIDI:
            case WAVE:
                  base = QString("Track");
                  break;
            case MIDI_CHANNEL:
                  {
                  MidiOut* mop = ((MidiChannel*)this)->port();
                  int no = ((MidiChannel*)this)->channelNo();
                  base = QString("%1:%2").arg(mop->track->name()).arg(no + 1);
                  setName(base);
                  return;
                  }
                  break;
            case AUDIO_GROUP:
                  base = QString("Group");
                  break;
            case AUDIO_SOFTSYNTH:
                  // base = QString("Synth");
            	return;
            case AUDIO_OUTPUT:
            case AUDIO_INPUT:
            case MIDI_OUT:
            case MIDI_IN:
            case MIDI_SYNTI:
            case TRACK_TYPES:
                  base = cname();
                  break;
            };
      //
      // create unique name
      //
      for (int i = 1;; ++i) {
            QString s;
            if (i == 1)
                  s = base;
            else
                  s = QString("%1 %2").arg(base).arg(i);
            bool found = false;
            TrackList* tl = song->tracks();
            for (iTrack it = tl->begin(); it != tl->end(); ++it) {
                  Track* track = *it;
                  if (track->name() == s) {
                        found = true;
                        break;
                        }
                  }
            MidiChannelList* mc = song->midiChannel();
            for (iMidiChannel ic = mc->begin(); ic != mc->end(); ++ic) {
                  MidiChannel* t = *ic;
                  if (t->name() == s) {
                        found = true;
                        break;
                        }
                  }
            if (!found) {
                  setName(s);
                  break;
                  }
            }
      if (type() == MIDI_OUT) {
            MidiOutPort* mop = (MidiOutPort*) this;
            for (int i = 0; i < MIDI_CHANNELS; ++i)
                  mop->channel(i)->setDefaultName();
            }
      }

//---------------------------------------------------------
//   dump
//---------------------------------------------------------

void Track::dump() const
      {
      printf("Track <%s>: typ %d, parts %zd sel %d\n",
         _name.toLatin1().data(), type(), _parts->size(), _selected);
      }

//---------------------------------------------------------
//   addPart
//---------------------------------------------------------

void Track::addPart(Part* p)
      {
      p->setTrack(this);
      _parts->add(p);
      }

//---------------------------------------------------------
//   findPart
//---------------------------------------------------------

Part* Track::findPart(unsigned tick)
      {
      for (iPart i = _parts->begin(); i != _parts->end(); ++i) {
            Part* part = i->second;
            if (tick >= part->tick() && tick < (part->tick()+part->lenTick()))
                  return part;
            }
      return 0;
      }

//---------------------------------------------------------
//   Track::writeProperties
//---------------------------------------------------------

void Track::writeProperties(Xml& xml) const
      {
      xml.strTag("name", _name);
      if (!_comment.isEmpty())
            xml.strTag("comment", _comment);
      if (_recordFlag)
            xml.intTag("record", _recordFlag);
      if (mute() != muteDefault())
            xml.intTag("mute", mute());
      if (solo())
            xml.intTag("solo", solo());
      if (off())
            xml.intTag("off", off());
      if (_channels)
            xml.intTag("channels", _channels);
      if (_locked)
            xml.intTag("locked", _locked);
      if (_monitor)
            xml.intTag("monitor", _monitor);
      if (!_autoRead)
            xml.intTag("autoRead", _autoRead);
      if (_autoWrite)
            xml.intTag("autoWrite", _autoWrite);
      if (_selected)
            xml.intTag("selected", _selected);
      for (ciCtrl icl = controller()->begin(); icl != controller()->end(); ++icl)
            icl->second->write(xml);
      if (arrangerTrack.tw)
            xml.intTag("height", arrangerTrack.tw->height());
      for (ciArrangerTrack i = subtracks.begin(); i != subtracks.end(); ++i) {
            xml.tag("subtrack");
            xml.intTag("height", (*i)->tw->height());
            xml.intTag("ctrl", (*i)->ctrl);
            xml.etag("subtrack");
            }
      }

//---------------------------------------------------------
//   Track::readProperties
//---------------------------------------------------------

bool Track::readProperties(QDomNode node)
      {
      QDomElement e = node.toElement();
      QString tag(e.tagName());
      QString s(e.text());
      int i = s.toInt();

      if (tag == "name")
            setName(s);
      else if (tag == "comment")
            _comment = s;
      else if (tag == "record") {
            bool recordFlag = i;
            setRecordFlag(recordFlag);
            }
      else if (tag == "mute")
            _mute = i;
      else if (tag == "solo")
            _solo = i;
      else if (tag == "off")
            _off = i;
      else if (tag == "channels")
            _channels = i;
      else if (tag == "locked")
            _locked = i;
      else if (tag == "monitor")
            _monitor = i;
      else if (tag == "selected")
            _selected = i;
      else if (tag == "autoRead")
            _autoRead = i;
      else if (tag == "autoWrite")
            _autoWrite = i;
      else if (tag == "controller") {
            Ctrl* l = new Ctrl();
            l->read(node, false);

            iCtrl icl = controller()->find(l->id());
            if (icl == controller()->end())
                  controller()->add(l);
            else {  //???
                  Ctrl* d = icl->second;
                  for (iCtrlVal i = l->begin(); i != l->end(); ++i)
                        d->insert(i.key(), i.value());
                  d->setCurVal(l->curVal());
                  d->setDefault(l->getDefault());
                  delete l;
                  }
            }
      else if (tag == "height")
            arrangerTrack.h = i < minTrackHeight ? minTrackHeight : i;
      else if (tag == "subtrack") {
            ArrangerTrack* st = new ArrangerTrack;
            for (QDomNode n = node.firstChild(); !n.isNull(); n = n.nextSibling()) {
                  QDomElement e = n.toElement();
                  QString tag   = e.tagName();
                  QString s     = e.text();
                  int i         = s.toInt();
                  if (tag == "height")
                        st->h = i;
                  else if (tag == "ctrl") {
                        st->ctrl = i;
                        }
                  else
                        printf("Track::subtrack: unknown tag <%s>\n", tag.toLatin1().data());
                  }
            subtracks.push_back(st);
            }
      else
            return true;
      return false;
      }

//---------------------------------------------------------
//   addController
//---------------------------------------------------------

void Track::addController(Ctrl* list)
      {
      iCtrl i = controller()->find(list->id());
      if (i != controller()->end()) {
            // printf("%s(%s)::addController(%s): already there 0x%x\n",
            // cname().toLatin1().data(), name().toLatin1().data(), list->name().toLatin1().data(), list->id());
            // abort();
            return;
            }
      controller()->add(list);
      emit clChanged();
      }

//---------------------------------------------------------
//   addMidiController
//---------------------------------------------------------

void Track::addMidiController(MidiInstrument* mi, int ctrl)
      {
      iCtrl cl = _controller.find(ctrl);
      if (cl != _controller.end())
            return;

      MidiController* mc = mi->midiController(ctrl);
      Ctrl* pvl;
      if (mc) {
            pvl = new Ctrl(mc);
            }
      else {
            printf("unknown midi controller %x\n", ctrl);
            pvl = new Ctrl(ctrl, QString("unknown"));
            pvl->setCurVal(CTRL_VAL_UNKNOWN);
            pvl->setType(Ctrl::DISCRETE | Ctrl::INT);
            }
      addController(pvl);
      }

//---------------------------------------------------------
//   removeController
//---------------------------------------------------------

void Track::removeController(int id)
      {
      iCtrl i = controller()->find(id);
      if (i == controller()->end()) {
            printf("Track::removeController id 0x%x not found, listsize %zd\n",
               id, controller()->size());
            return;
            }
      controller()->erase(i);
      emit clChanged();
      }

//---------------------------------------------------------
//   addControllerVal
//    return true if new controller value added
//---------------------------------------------------------

bool Track::addControllerVal(int id, unsigned time, CVal val)
      {
      iCtrl i = controller()->find(id);
      if (i == controller()->end()) {
            if ((id & 0xf0000) == CTRL_NRPN_OFFSET) {
                  int msb = id & 0xff00;
                  // int lsb = id & 0xff;
                  int nid = CTRL_NRPN_OFFSET + msb + 0xff;
                  i = controller()->find(nid);
                  if (i != controller()->end()) {
                        Ctrl* c = new Ctrl(*(i->second));
                        c->setId(id);
                        addController(c);
// printf("add pitch ctrl %x\n", id);
                        return c->add(time, val);
                        }
                  }
            printf("Track::addControllerVal(): id 0x%x not found, listsize %zd\n",
               id, controller()->size());
            return false;
            }
      return i->second->add(time, val);
      }

//---------------------------------------------------------
//   removeControllerVal
//---------------------------------------------------------

void Track::removeControllerVal(int id, unsigned time)
      {
      iCtrl i = controller()->find(id);
      if (i == controller()->end()) {
            printf("Track::removeControllerVal(): id 0x%x not found, listsize %zd\n",
               id, controller()->size());
            return;
            }
      i->second->del(time);
      }

//---------------------------------------------------------
//   getController
//---------------------------------------------------------

Ctrl* Track::getController(int id) const
      {
      ciCtrl i = controller()->find(id);
      if (i == controller()->end()) {
//            printf("%s(%s)::getController(%d) size %d: not found\n",
//               cname().toLatin1().data(), name().toLatin1().data(), id, controller()->size());
//            const CtrlList* cl = controller();
//            for (ciCtrl i = cl->begin(); i != cl->end(); ++i)
//                  printf("   Ctrl %d\n", i->first);
            return 0;
            }
      return i->second;
      }

//---------------------------------------------------------
//   controllerNames
//---------------------------------------------------------

ControllerNameList* Track::controllerNames() const
      {
      ControllerNameList* l = new ControllerNameList;
      for (ciCtrl i = controller()->begin(); i != controller()->end(); ++i)
            l->push_back(ControllerName(i->second->name(), i->second->id()));
      return l;
      }

//---------------------------------------------------------
//   setRecordFlag
//---------------------------------------------------------

void Track::setRecordFlag(bool f)
      {
      if (_recordFlag != f) {
            _recordFlag = f;
            emit recordChanged(_recordFlag);
            }
      }

//---------------------------------------------------------
//   setMonitor
//---------------------------------------------------------

void Track::setMonitor(bool f)
      {
      if (_monitor != f) {
            _monitor = f;
            emit monitorChanged(_monitor);
            }
      }

//---------------------------------------------------------
//   setSelected
//---------------------------------------------------------

void Track::setSelected(bool f)
      {
      if (f != _selected) {
            _selected = f;
            emit selectionChanged(_selected);
            muse->selectionChanged();
            }
      }

//---------------------------------------------------------
//   setController
//    called from GUI
//---------------------------------------------------------

#if 0
void Track::setController(Pos pos, int id, int v)
      {
      CVal val;
      val.i = v;
      setController(pos, id, val);
      }

void Track::setController(Pos pos, int id, double v)
      {
      CVal val;
      val.f = v;
      setController(pos, id, val);
      }

void Track::setController(Pos pos, int id, CVal val)
      {
       Ctrl* c = getController(id);
       if (c == 0) {
            printf("no controller 0x%x %s\n", id, name().toLatin1().data());
            return;
            }
      if (isMidiTrack()) {
            int port    = ((MidiTrack*)this)->outPort();
            int channel = ((MidiTrack*)this)->outChannel();
            MidiEvent ev(0, port, channel, ME_CONTROLLER, id, val.i);
            audio->msgPlayMidiEvent(&ev);
            }
      else {
            // non midi controller are current once set
            c->setCurVal(val);
            }
      c->setSchedVal(val);
      if (autoWrite()) {
            unsigned time = _tt == AL::FRAMES ? pos.frame() : pos.tick();
            if (audio->isPlaying())
                  _recEvents.push_back(CtrlRecVal(time, id, val));
            else
                  song->addControllerVal(this, c, id, time, val);
            }
      emit controllerChanged(id);
      }
#endif

//---------------------------------------------------------
//   startAutoRecord
//    slider/knob touched
//---------------------------------------------------------

void Track::startAutoRecord(int n)
      {
      Ctrl* ctrl = getController(n);
      if (ctrl) {
            ctrl->setTouched(true);
            if (audio->isPlaying() && autoWrite())
                  _recEvents.push_back(CtrlRecVal(song->cPos().frame(), n, 1));
            }
      else
            printf("no ctrl 0x%x\n", n);
      }

//---------------------------------------------------------
//   stopAutoRecord
//    slider/knob released
//---------------------------------------------------------

void Track::stopAutoRecord(int n)
      {
      Ctrl* ctrl = getController(n);
      if (ctrl) {
            ctrl->setTouched(false);
            if (audio->isPlaying() && autoWrite())
                  _recEvents.push_back(CtrlRecVal(song->cPos().frame(), n, 2));
            }
      else
            printf("no ctrl 0x%x\n", n);
      }

//---------------------------------------------------------
//   setName
//---------------------------------------------------------

void Track::setName(const QString& s)
      {
      _name = s;
      emit nameChanged(_name);
      }

//---------------------------------------------------------
//   setAutoRead
//---------------------------------------------------------

void Track::setAutoRead(bool val)
      {
      if (_autoRead != val) {
            _autoRead = val;
            emit autoReadChanged(_autoRead);
            }
      }

//---------------------------------------------------------
//   setAutoWrite
//---------------------------------------------------------

void Track::setAutoWrite(bool val)
      {
      if (_autoWrite != val) {
            _autoWrite = val;
            emit autoWriteChanged(_autoWrite);
            }
      }

//---------------------------------------------------------
//   cpos
//---------------------------------------------------------

unsigned Track::cpos() const
      {
      return timeType() == AL::TICKS ? song->cPos().tick() : song->cPos().frame();
      }

//---------------------------------------------------------
//   updateController
//---------------------------------------------------------

void Track::updateController()
      {
      CtrlList* cl = controller();
      for (iCtrl i = cl->begin(); i != cl->end(); ++i) {
            Ctrl* c = i->second;
            if (c->changed()) {
                  c->setChanged(false);
                  emit controllerChanged(c->id());
                  }
            }
      }

//---------------------------------------------------------
//   writeRouting
//---------------------------------------------------------

void Track::writeRouting(Xml& xml) const
      {
      foreach(Route src, _inRoutes) {
            Route dst((Track*)this);
            if (type() == AUDIO_INPUT || type() == MIDI_IN) {
                  xml.tag("Route");
                  dst.channel = src.channel;
                  src.write(xml, "src");
                  dst.write(xml, "dst");
                  xml.etag("Route");
                  }
            else if (src.type == Route::AUXPLUGIN) {
                  xml.tag("Route");
                  src.write(xml, "src");
                  dst.write(xml, "dst");
                  xml.etag("Route");
                  }
            }
      foreach(Route r, _outRoutes) {
            Route src((Track*)this);
            Route dst(r);
            if (type() == MIDI_IN) {
                  src.channel = dst.channel;
                  dst.channel = 0;
                  }
            else if (type() == AUDIO_OUTPUT)
                  src.channel = r.channel;
            xml.tag("Route");
            src.write(xml, "src");
            dst.write(xml, "dst");
            xml.etag("Route");
            }
      }

//---------------------------------------------------------
//   MidiTrackBase
//---------------------------------------------------------

MidiTrackBase::MidiTrackBase()
   : Track()
      {
      _pipeline = new MidiPipeline();
      }

//---------------------------------------------------------
//   MidiTrackBase
//---------------------------------------------------------

MidiTrackBase::~MidiTrackBase()
      {
      foreach(MidiPluginI* plugin, *_pipeline)
            delete plugin;
      delete _pipeline;
      }

//---------------------------------------------------------
//   MidiTrackBase::writeProperties
//---------------------------------------------------------

void MidiTrackBase::writeProperties(Xml& xml) const
      {
      Track::writeProperties(xml);
      for (ciMidiPluginI ip = _pipeline->begin(); ip != _pipeline->end(); ++ip) {
            if (*ip)
                  (*ip)->writeConfiguration(xml);
            }
      }

//---------------------------------------------------------
//   MidiTrackBase::readProperties
//---------------------------------------------------------

bool MidiTrackBase::readProperties(QDomNode node)
      {
      QDomElement e = node.toElement();
      QString tag(e.tagName());
      if (tag == "midiPlugin") {
            MidiPluginI* pi = new MidiPluginI(this);
            if (pi->readConfiguration(node))
                  delete pi;
            else
                  addPlugin(pi, -1);
            }
      else
            return Track::readProperties(node);
      return false;
      }

//---------------------------------------------------------
//   plugin
//---------------------------------------------------------

MidiPluginI* MidiTrackBase::plugin(int idx) const
      {
      return _pipeline->value(idx);
      }

//---------------------------------------------------------
//   addPlugin
//    idx    = -1     append
//    plugin = 0   remove slot
//---------------------------------------------------------

void MidiTrackBase::addPlugin(MidiPluginI* plugin, int idx)
      {
      if (plugin == 0) {
#if 0
            MidiPluginI* oldPlugin = (*_pipeline)[idx];
            if (oldPlugin) {
                  int controller = oldPlugin->plugin()->parameter();
                  for (int i = 0; i < controller; ++i) {
                        int id = (idx + 1) * 0x1000 + i;
                        removeController(id);
                        }
                  }
#endif
            }
      if (idx == -1)
            idx = _pipeline->size();

      if (plugin) {
            _pipeline->insert(idx, plugin);
#if 0
            int ncontroller = plugin->plugin()->parameter();
            for (int i = 0; i < ncontroller; ++i) {
                  int id = (idx + 1) * 0x1000 + i;
                  QString name(plugin->getParameterName(i));
                  double min, max;
                  plugin->range(i, &min, &max);
                  Ctrl* cl = getController(id);
                  if (cl == 0) {
                        cl = new Ctrl(id, name);
                        cl->setRange(min, max);
                        double defaultValue = plugin->defaultValue(i);
                        cl->setDefault(defaultValue);
                        cl->setCurVal(defaultValue);
                        addController(cl);
                        }
                  plugin->setParam(i, cl->schedVal().f);
                  plugin->setControllerList(cl);
                  }
#endif
            }
      else {
            _pipeline->removeAt(idx);
            }
      }


//---------------------------------------------------------
//   hwCtrlState
//---------------------------------------------------------

int Track::hwCtrlState(int ctrl) const
      {
      ciCtrl cl = _controller.find(ctrl);
      if (cl == _controller.end()) {
            if (debugMsg)
                  printf("hwCtrlState: ctrl 0x%x not found\n", ctrl);
            return CTRL_VAL_UNKNOWN;
            }
      Ctrl* vl = cl->second;
      return vl->curVal().i;
      }

//---------------------------------------------------------
//   setHwCtrlState
//---------------------------------------------------------

void Track::setHwCtrlState(int ctrl, int val)
      {
      iCtrl cl = _controller.find(ctrl);
      if (cl == _controller.end()) {
            // try to add new controller
            if (debugMsg)
                  printf("setHwCtrlState(0x%x,0x%x): not found\n", ctrl, val);
            return;
            }
      Ctrl* vl = cl->second;
// printf("setHwCtrlState ctrl %x  val %x\n", ctrl, val);
      vl->setChanged(true);
      return vl->setCurVal(val);
      }

//---------------------------------------------------------
//   getCtrl
//---------------------------------------------------------

int Track::getCtrl(int tick, int ctrl) const
      {
      ciCtrl cl = _controller.find(ctrl);
      if (cl == _controller.end()) {
            if (debugMsg)
                  printf("getCtrl: controller %d(0x%x) not found %zd\n",
                     ctrl, ctrl, _controller.size());
            return CTRL_VAL_UNKNOWN;
            }
      return cl->second->value(tick).i;
      }

//---------------------------------------------------------
//   setSolo
//---------------------------------------------------------

bool Track::setSolo(bool val)
      {
      if (_solo != val) {
            _solo = val;
            emit soloChanged(_solo);
            return true;
            }
      return false;
      }

//---------------------------------------------------------
//   setMute
//    return true if mute changed
//---------------------------------------------------------

bool Track::setMute(bool val)
      {
      if (_mute != val) {
            _mute = val;
            emit muteChanged(_mute);
            return true;
            }
      return false;
      }

//---------------------------------------------------------
//   setOff
//    return true if state changed
//---------------------------------------------------------

bool Track::setOff(bool val)
      {
      if (_off != val) {
            _off = val;
            emit offChanged(_off);
            return true;
            }
      return false;
      }

//---------------------------------------------------------
//   setChannels
//---------------------------------------------------------

void Track::setChannels(int n)
      {
      _channels = n;
      for (int i = 0; i < _channels; ++i) {
            _meter[i] = 0.0f;
            _peak[i]  = 0.0f;
            }
      }

//---------------------------------------------------------
//   resetMeter
//---------------------------------------------------------

void Track::resetMeter()
      {
      for (int i = 0; i < _channels; ++i)
            _meter[i] = 0.0f;
      }

//---------------------------------------------------------
//   resetPeaks
//---------------------------------------------------------

void Track::resetPeaks()
      {
      for (int i = 0; i < _channels; ++i)
            _peak[i] = 0;
      }

//---------------------------------------------------------
//   resetAllMeter
//---------------------------------------------------------

void Track::resetAllMeter()
      {
      TrackList* tl = song->tracks();
      for (iTrack i = tl->begin(); i != tl->end(); ++i)
            (*i)->resetMeter();
      }

//---------------------------------------------------------
//   activate1
//    register JACK and ALSA ports
//---------------------------------------------------------

void Track::activate1()
      {
      if (isMidiTrack()) {
            if (!alsaPort(0).isZero())
                  printf("Track::activate1() midi: alsa port already active!\n");
            if (!jackPort(0).isZero())
                  printf("Track::activate1() midi: jack port already active!\n");
            if (type() == MIDI_OUT) {
                  _alsaPort[0] = midiDriver->registerInPort(_name, true);
                  _jackPort[0] = audioDriver->registerOutPort(_name, true);
                  }
            else if (type() == MIDI_IN) {
                  _alsaPort[0] = midiDriver->registerOutPort(_name, true);
                  _jackPort[0] = audioDriver->registerInPort(_name, true);
                  }
            return;
            }

      for (int i = 0; i < channels(); ++i) {
            if (!jackPort(i).isZero())
                  printf("Track<%s>::activate1(): channel %d already active!\n",
                    name().toLatin1().data(), i);
            else {
                  QString s(QString("%1-%2").arg(_name).arg(i));
                  if (type() == AUDIO_OUTPUT)
                        _jackPort[i] = audioDriver->registerOutPort(s, false);
                  else if (type() == AUDIO_INPUT)
                        _jackPort[i] = audioDriver->registerInPort(s, false);
                  }
            }
      }

//---------------------------------------------------------
//   activate2
//    connect all JACK/ALSA in/out routes
//    connect to JACK only works if JACK is running
//---------------------------------------------------------

void Track::activate2()
      {
      if (audioState != AUDIO_RUNNING) {
            printf("Track::activate2(): no audio running !\n");
            abort();
            }
      for (iRoute ri = _outRoutes.begin(); ri != _outRoutes.end(); ++ri) {
            Route& r = *ri;
            if (r.type == Route::JACKMIDIPORT) {
                  audioDriver->connect(_jackPort[0], r.port);
                  r.disconnected = false;
                  }
            else if (r.type == Route::AUDIOPORT) {
                  audioDriver->connect(_jackPort[r.channel], r.port);
                  r.disconnected = false;
                  }
            else if (r.type == Route::MIDIPORT) {
                  midiDriver->connect(_alsaPort[0], r.port);
                  r.disconnected = false;
                  }
            }
      for (iRoute ri = _inRoutes.begin(); ri != _inRoutes.end(); ++ri) {
            Route& r = *ri;
            if (r.type == Route::JACKMIDIPORT) {
                  audioDriver->connect(r.port, _jackPort[0]);
                  r.disconnected = false;
                  }
            else if (r.type == Route::AUDIOPORT) {
                  audioDriver->connect(r.port, _jackPort[r.channel]);
                  r.disconnected = false;
                  }
            else if (r.type == Route::MIDIPORT) {
                  midiDriver->connect(r.port, _alsaPort[0]);
                  r.disconnected = false;
                  }
            }
      }

//---------------------------------------------------------
//   deactivate
//    disconnect and unregister JACK and ALSA ports
//---------------------------------------------------------

void Track::deactivate()
      {
// printf("deactivate<%s>\n", name().toLatin1().data());
      for (iRoute ri = _outRoutes.begin(); ri != _outRoutes.end(); ++ri) {
            Route& r = *ri;
            if (r.type == Route::JACKMIDIPORT) {
                  r.disconnected = true;
                  audioDriver->disconnect(_jackPort[0], r.port);
                  }
            else if (r.type == Route::AUDIOPORT) {
                  audioDriver->disconnect(_jackPort[r.channel], r.port);
                  r.disconnected = true;
                  }
            else if (r.type == Route::MIDIPORT) {
                  r.disconnected = true;
                  midiDriver->disconnect(_alsaPort[0], r.port);
                  }
            }
      for (iRoute ri = _inRoutes.begin(); ri != _inRoutes.end(); ++ri) {
            Route& r = *ri;
            if (r.type == Route::JACKMIDIPORT) {
                  r.disconnected = true;
                  audioDriver->disconnect(r.port, _jackPort[0]);
                  }
            else if (r.type == Route::AUDIOPORT) {
                  r.disconnected = true;
                  audioDriver->disconnect(r.port, _jackPort[r.channel]);
                  }
            else if (r.type == Route::MIDIPORT) {
                  r.disconnected = true;
                  midiDriver->disconnect(r.port, _alsaPort[0]);
                  }
            }
      for (int i = 0; i < channels(); ++i) {
            if (!_jackPort[i].isZero()) {
                  audioDriver->unregisterPort(_jackPort[i]);
                  _jackPort[i].setZero();
                  }
            if (!_alsaPort[i].isZero()) {
                  midiDriver->unregisterPort(_alsaPort[i]);
                  _alsaPort[i].setZero();
                  }
            }
      }

//---------------------------------------------------------
//   setSendSync
//---------------------------------------------------------

void Track::setSendSync(bool val)
      {
      _sendSync = val;
      emit sendSyncChanged(val);
      }


