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
#include "song.h"
#include "al/tempo.h"
#include "al/xml.h"
#include "icons.h"
#include "audio.h"
#include "gconfig.h"
#include "midictrl.h"
#include "part.h"
#include "gui.h"
#include "driver/audiodev.h"

// synchronize with TrackType!:

const char* Track::_cname[] = {
      "AudioOut", "Group", "Wave", "AudioIn",
      "Synth", "Midi", "MidiOut", "MidiIn", "M-Synth"
      };

const char* Track::_clname[] = {
      "Audio Output", "Audio Group", "Wave Track", "Audio Input",
      "Synti", "Midi Track", "Midi Output", "Midi Input",
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
      _autoRead      = autoReadDefault();
      _autoWrite     = autoWriteDefault();

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
//            if (!_alsaPort[i].isZero())
//                  midiDriver->unregisterPort(_alsaPort[i]);
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
            if (!found) {
                  setName(s);
                  break;
                  }
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
      xml.tag("name", _name);
      if (!_comment.isEmpty())
            xml.tag("comment", _comment);
      if (_recordFlag)
            xml.tag("record", _recordFlag);
      if (mute() != muteDefault())
            xml.tag("mute", mute());
      if (solo())
            xml.tag("solo", solo());
      if (off())
            xml.tag("off", off());
      if (_channels)
            xml.tag("channels", _channels);
      if (_locked)
            xml.tag("locked", _locked);
      if (_monitor)
            xml.tag("monitor", _monitor);
      if (_autoRead != autoReadDefault())
            xml.tag("autoRead", _autoRead);
      if (_autoWrite != autoWriteDefault())
            xml.tag("autoWrite", _autoWrite);
      if (_selected)
            xml.tag("selected", _selected);
      for (ciCtrl icl = controller()->begin(); icl != controller()->end(); ++icl)
            icl->second->write(xml);
      if (arrangerTrack.tw)
            xml.tag("height", arrangerTrack.tw->height());
      for (ciArrangerTrack i = subtracks.begin(); i != subtracks.end(); ++i) {
            xml.stag("subtrack");
            xml.tag("height", (*i)->tw->height());
            xml.tag("ctrl", (*i)->ctrl);
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
//   changeCtrlName
//---------------------------------------------------------

void Track::changeCtrlName(Ctrl* c, const QString& s)
      {
      c->setName(s);
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
//            muse->selectionChanged();
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
      if (type() == AUDIO_INPUT || type() == MIDI_IN) {
            foreach(Route r, _inRoutes) {
                  xml.stag("Route");
                  r.src.write(xml, "src");
                  r.dst.write(xml, "dst");
                  xml.etag("Route");
                  }
            }
      foreach(Route r, _outRoutes) {
            xml.stag("Route");
            r.src.write(xml, "src");
            r.dst.write(xml, "dst");
            xml.etag("Route");
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
            if (!jackPort(0).isZero())
                  printf("Track::activate1() midi: jack port already active!\n");
            if (type() == MIDI_OUT) {
                  _jackPort[0] = audioDriver->registerOutPort(_name, true);
                  }
            else if (type() == MIDI_IN) {
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
      foreach(Route r, _outRoutes) {
            if (r.dst.type == RouteNode::JACKMIDIPORT) {
                  audioDriver->connect(_jackPort[0], r.dst.port);
                  r.disconnected = false;
                  }
            else if (r.dst.type == RouteNode::AUDIOPORT) {
                  audioDriver->connect(_jackPort[r.src.channel], r.dst.port);
                  r.disconnected = false;
                  }
            }
      foreach(Route r, _inRoutes) {
            if (r.src.type == RouteNode::JACKMIDIPORT) {
                  audioDriver->connect(r.src.port, _jackPort[0]);
                  r.disconnected = false;
                  }
            else if (r.src.type == RouteNode::AUDIOPORT) {
                  audioDriver->connect(r.src.port, _jackPort[r.dst.channel]);
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
      foreach(Route r, _outRoutes) {
            if (r.dst.type == RouteNode::JACKMIDIPORT) {
                  r.disconnected = true;
                  audioDriver->disconnect(_jackPort[0], r.dst.port);
                  }
            else if (r.dst.type == RouteNode::AUDIOPORT) {
                  audioDriver->disconnect(_jackPort[r.src.channel], r.dst.port);
                  r.disconnected = true;
                  }
            }
      foreach(Route r, _inRoutes) {
            if (r.src.type == RouteNode::JACKMIDIPORT) {
                  r.disconnected = true;
                  audioDriver->disconnect(r.src.port, _jackPort[0]);
                  }
            else if (r.src.type == RouteNode::AUDIOPORT) {
                  r.disconnected = true;
                  audioDriver->disconnect(r.src.port, _jackPort[r.dst.channel]);
                  }
            }
      for (int i = 0; i < channels(); ++i) {
            if (!_jackPort[i].isZero()) {
                  audioDriver->unregisterPort(_jackPort[i]);
                  _jackPort[i].setZero();
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

//---------------------------------------------------------
//   splitPart
//    split part "part" at "tick" position
//    create two new parts p1 and p2
//---------------------------------------------------------

void Track::splitPart(Part* part, int tickpos, Part*& p1, Part*& p2)
      {
      int l1 = 0;       // len of first new part (ticks or samples)
      int l2 = 0;       // len of second new part

      int samplepos = AL::tempomap.tick2frame(tickpos);

      switch (type()) {
            case WAVE:
                  l1 = samplepos - part->frame();
                  l2 = part->lenFrame() - l1;
                  break;
            case MIDI:
                  l1 = tickpos - part->tick();
                  l2 = part->lenTick() - l1;
                  break;
            default:
                  return;
            }

      if (l1 <= 0 || l2 <= 0)
            return;

      p1 = newPart(part);     // new left part
      p2 = newPart(part);     // new right part

      switch (type()) {
            case WAVE:
                  p1->setLenFrame(l1);
                  p2->setFrame(samplepos);
                  p2->setLenFrame(l2);
                  break;
            case MIDI:
                  p1->setLenTick(l1);
                  p2->setTick(tickpos);
                  p2->setLenTick(l2);
                  break;
            default:
                  break;
            }

      EventList* se  = part->events();
      EventList* de1 = p1->events();
      EventList* de2 = p2->events();

      if (type() == WAVE) {
            int ps   = part->frame();
            int d1p1 = p1->frame();
            int d2p1 = p1->endFrame();
            int d1p2 = p2->frame();
            int d2p2 = p2->endFrame();
            for (iEvent ie = se->begin(); ie != se->end(); ++ie) {
                  Event event = ie->second;
                  int s1 = event.frame() + ps;
                  int s2 = event.endFrame() + ps;

                  if ((s2 > d1p1) && (s1 < d2p1)) {
                        Event si = event.mid(d1p1 - ps, d2p1 - ps);
                        de1->add(si);
                        }
                  if ((s2 > d1p2) && (s1 < d2p2)) {
                        Event si = event.mid(d1p2 - ps, d2p2 - ps);
                        si.setFrame(si.frame() - l1);       //??
                        si.setFrame(0);                     //??
                        de2->add(si);
                        }
                  }
            }
      else {
            for (iEvent ie = se->begin(); ie != se->end(); ++ie) {
                  Event event = ie->second.clone();
                  int t = event.tick();
                  if (t >= l1) {
                        event.move(-l1);
                        de2->add(event);
                        }
                  else
                        de1->add(event);
                  }
            }
      }

//---------------------------------------------------------
//   addInRoute
//---------------------------------------------------------

void Track::addInRoute(const Route& r)
      {
      if (_inRoutes.indexOf(r) != -1) {
            printf("Track::addInRoute: route already there\n");
            return;
            }
      _inRoutes.push_back(r);
      }

//---------------------------------------------------------
//   addOutRoute
//---------------------------------------------------------

void Track::addOutRoute(const Route& r)
      {
      if (_outRoutes.indexOf(r) != -1) {
            printf("Track::addOutRoute: route already there\n");
            return;
            }
      _outRoutes.push_back(r);
      }

//---------------------------------------------------------
//   inRouteExists
//---------------------------------------------------------

bool Track::inRouteExists(const Route& r) const
      {
      return _inRoutes.indexOf(r) != -1;
      }

//---------------------------------------------------------
//   outRouteExists
//---------------------------------------------------------

bool Track::outRouteExists(const Route& r) const
      {
      return _outRoutes.indexOf(r) != -1;
      }

