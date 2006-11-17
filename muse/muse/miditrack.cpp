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

#include "miditrack.h"
#include "event.h"
#include "song.h"
#include "midi.h"
#include "midictrl.h"
#include "audio.h"
#include "part.h"
#include "al/tempo.h"
#include "midiedit/drummap.h"

//---------------------------------------------------------
//   MidiTrack
//---------------------------------------------------------

MidiTrack::MidiTrack()
   : MidiTrackBase()
      {
      _transposition  = 0;
      _velocity       = 0;
      _delay          = 0;
      _len            = 100;          // percent
      _compression    = 100;          // percent
      _events         = new EventList;

      initMidiController();
      recordPart      = 0;
      _drumMap        = 0;
      _useDrumMap     = false;

      //
      // create minimal set of managed controllers
      // to make midi mixer operational
      //
      MidiInstrument* mi = genericMidiInstrument;
      addMidiController(mi, CTRL_PROGRAM);
      addMidiController(mi, CTRL_VOLUME);
      addMidiController(mi, CTRL_PANPOT);
      addMidiController(mi, CTRL_REVERB_SEND);
      addMidiController(mi, CTRL_CHORUS_SEND);
      addMidiController(mi, CTRL_VARIATION_SEND);
      }

MidiTrack::~MidiTrack()
      {
      delete _events;
      }

//---------------------------------------------------------
//   newPart
//---------------------------------------------------------

Part* MidiTrack::newPart(Part*p, bool clone)
      {
      Part* part = clone ? new Part(this, p->events()) : new Part(this);
      if (p) {
            part->setName(p->name());
            part->setColorIndex(p->colorIndex());

            *(AL::PosLen*)part = *(AL::PosLen*)p;
            part->setMute(p->mute());
            }
      return part;
      }

//---------------------------------------------------------
//   MidiTrack::write
//---------------------------------------------------------

void MidiTrack::write(Xml& xml) const
      {
      xml.tag("miditrack");
      MidiTrackBase::writeProperties(xml);

      xml.intTag("transposition", _transposition);
      xml.intTag("velocity", _velocity);
      xml.intTag("delay", _delay);
      xml.intTag("len", _len);
      xml.intTag("compression", _compression);
      xml.intTag("useDrumMap", _useDrumMap);

      const PartList* pl = parts();
      for (ciPart p = pl->begin(); p != pl->end(); ++p)
            p->second->write(xml);
      xml.etag("miditrack");
      }

//---------------------------------------------------------
//   MidiTrack::read
//---------------------------------------------------------

void MidiTrack::read(QDomNode node)
      {
      for (; !node.isNull(); node = node.nextSibling()) {
            QDomElement e = node.toElement();
            QString tag(e.tagName());
            QString s(e.text());
            int i = s.toInt();
            if (tag == "transposition")
                  _transposition = i;
            else if (tag == "velocity")
                  _velocity = i;
            else if (tag == "delay")
                  _delay = i;
            else if (tag == "len")
                  _len = i;
            else if (tag == "compression")
                  _compression = i;
            else if (tag == "part") {
                  Part* p = newPart();
                  p->read(node);
                  parts()->add(p);
                  }
            else if (tag == "locked")
                  _locked = i;
            else if (tag == "useDrumMap")
                  _useDrumMap = e.text().toInt();
            else if (MidiTrackBase::readProperties(node))
                  printf("MusE:MidiTrack: unknown tag %s\n", e.tagName().toLatin1().data());
            }
      }

//---------------------------------------------------------
//   playMidiEvent
//---------------------------------------------------------

void MidiTrack::playMidiEvent(MidiEvent* ev)
      {
      foreach (const Route& r, _outRoutes) {
            Track* track = r.dst.track;
            if (track->type() == MIDI_OUT)
                  ((MidiOutPort*)track)->playMidiEvent(ev);
            else if (track->type() == AUDIO_SOFTSYNTH)
                  ((SynthI*)track)->playMidiEvent(ev);
            }
      }

//---------------------------------------------------------
//   startRecording
//    gui context
//---------------------------------------------------------

void MidiTrack::startRecording()
      {
      hbank = 0;
      lbank = 0;
      datah = 0;
      datal = 0;
      rpnh  = 0;
      rpnl  = 0;
      dataType = 0;
      recordedEvents = 0;
      partCreated = false;
      recordPart = 0;
      recordFifo.clear();
      keyDown.clear();

      AL::Pos start = song->punchin() ? song->lPos() : song->cPos();

      for (iPart ip = parts()->begin(); ip != parts()->end(); ++ip) {
            Part* part = ip->second;
            unsigned partStart = part->tick();
            unsigned partEnd   = partStart + part->lenTick();
            if (start.tick() >= partStart && start.tick() < partEnd) {
                  recordPart = part;
                  }
            }
      if (recordPart == 0) {
            //
            // create new part for recording
            //
            recordPart    = new Part(this);
            recordPart->setTrack(this);
            int startTick = song->roundDownBar(start.tick());
            int endTick   = song->roundUpBar(start.tick());
            recordPart->setTick(startTick);
            recordPart->setLenTick(endTick - startTick);
            recordPart->setName(name());
            song->addPart(recordPart);
            partCreated = true;
            }
      }

//---------------------------------------------------------
//   recordBeat
//    gui context
//    update current recording
//---------------------------------------------------------

void MidiTrack::recordBeat()
      {
      int updateFlags = 0;
      unsigned cpos  = song->cpos();
      unsigned ptick = recordPart->tick();

      if (song->punchout()) {
            if (song->rPos() >= song->cPos()) {
                  while (!recordFifo.isEmpty())
                        recordFifo.get();
                  return;
                  }
            }
      while (!recordFifo.isEmpty()) {
            MidiEvent me(recordFifo.get());
            unsigned time = me.time();
            if (song->punchin() && time < song->lpos())
                  continue;
            bool isOff = me.isNoteOff();

        	if (song->punchout() && (time >= song->rpos()) && !isOff)
			continue;

            if (!partCreated && song->recMode() == Song::REC_REPLACE) {
                  // TODO: remove old events
                  }

            time -= ptick;
            if (isOff) {
                  //
                  // process note off
                  //
                  for (std::list<Event>::iterator i = keyDown.begin(); i != keyDown.end(); ++i) {
                        if (i->pitch() == me.dataA()) {
                              unsigned tl = time - i->tick();
                              if (tl != i->lenTick()) {
                                    i->setLenTick(tl);
                                    updateFlags |= SC_EVENT_MODIFIED;
                                    }
                              keyDown.erase(i);
                              break;
                              }
                        }
                  }
            else if (me.type() == ME_NOTEON && me.dataB() != 0) {
                  //
                  // create Note event on "note on"
                  //
                  Event event(Note);
                  event.setTick(time);
                  event.setLenTick(1);
                  event.setPitch(me.dataA());
                  event.setVelo(me.dataB());
                  audio->msgAddEvent(event, recordPart, false);
                  ++recordedEvents;
                  updateFlags |= SC_EVENT_INSERTED;
                  keyDown.push_front(event);
                  }
            else if (me.type() == ME_POLYAFTER) {
                  Event event(PAfter);
                  event.setTick(time);
                  event.setA(me.dataA());
                  event.setB(me.dataB());
                  }
            else if (me.type() == ME_CONTROLLER) {
                  Event event(Controller);
                  event.setTick(time + ptick);
                  switch(me.dataA()) {
                        case CTRL_HBANK:
                              hbank = me.dataB();
                              break;

                        case CTRL_LBANK:
                              lbank = me.dataB();
                              break;

                        case CTRL_HDATA:
                              datah = me.dataB();
                              event.setA(dataType | (rpnh << 8) | rpnl);
                              event.setB(datah);
                              audio->msgAddEvent(event, recordPart, false);
                              ++recordedEvents;
                              updateFlags |= SC_EVENT_INSERTED;
                              break;

                        case CTRL_LDATA:
                              datal = me.dataB();
                              if (dataType == CTRL_NRPN_OFFSET)
                                    dataType = CTRL_NRPN14_OFFSET;
                              else if (dataType == CTRL_RPN_OFFSET)
                                    dataType = CTRL_RPN14_OFFSET;
                              break;

                        case CTRL_HNRPN:
                              rpnh = me.dataB();
                              dataType = CTRL_NRPN_OFFSET;
                              break;

                        case CTRL_LNRPN:
                              rpnl = me.dataB();
                              dataType = CTRL_NRPN_OFFSET;
                              break;

                        case CTRL_HRPN:
                              rpnh     = me.dataB();
                              dataType = CTRL_RPN_OFFSET;
                              break;

                        case CTRL_LRPN:
                              rpnl     = me.dataB();
                              dataType = CTRL_RPN_OFFSET;
                              break;

                        default:
                              event.setA(me.dataA());
                              event.setB(me.dataB());
                              audio->msgAddEvent(event, recordPart, false);
                              ++recordedEvents;
                              updateFlags |= SC_EVENT_INSERTED;
                              break;
                        }
                  }
            else if (me.type() == ME_PROGRAM) {
                  Event event(Controller);
                  event.setTick(time + ptick);
                  event.setA(CTRL_PROGRAM);
                  event.setB((hbank << 16) | (lbank << 8) | me.dataA());
                  audio->msgAddEvent(event, recordPart, false);
                  ++recordedEvents;
                  updateFlags |= SC_EVENT_INSERTED;
                  }
            else if (me.type() == ME_PITCHBEND) {
                  Event event(Controller);
                  event.setTick(time + ptick);
                  event.setA(CTRL_PITCH);
                  event.setB(me.dataA());
                  audio->msgAddEvent(event, recordPart, false);
                  ++recordedEvents;
                  updateFlags |= SC_EVENT_INSERTED;
                  }
            else if (me.type() == ME_SYSEX) {
                  Event event(Sysex);
                  event.setTick(time + ptick);
                  event.setData(me.data(), me.len());
                  audio->msgAddEvent(event, recordPart, false);
                  ++recordedEvents;
                  updateFlags |= SC_EVENT_INSERTED;
                  }
            else if (me.type() == ME_AFTERTOUCH) {
                  Event event(CAfter);
                  event.setTick(time + ptick);
                  event.setA(me.dataA());
                  audio->msgAddEvent(event, recordPart, false);
                  ++recordedEvents;
                  updateFlags |= SC_EVENT_INSERTED;
                  }
            }
      if (partCreated) {
            recordPart->setLenTick(cpos - ptick);
            updateFlags |= SC_PART_MODIFIED;
            }
      //
      // modify len of all hold keys
      //
      for (std::list<Event>::iterator i = keyDown.begin(); i != keyDown.end(); ++i) {
            if (cpos > (i->tick() + ptick))
                  i->setLenTick(cpos - (i->tick() + ptick));
            updateFlags |= SC_EVENT_MODIFIED;
            }
      song->update(updateFlags);
      }

//---------------------------------------------------------
//   stopRecording
//    gui context
//---------------------------------------------------------

void MidiTrack::stopRecording()
      {
      if (recordedEvents == 0 && partCreated) {
            // TD: remove empty part?
            }
      //
      // modify len of all hold keys
      //
      unsigned ptick = recordPart->tick();
      unsigned cpos  = song->cpos();
      for (std::list<Event>::iterator i = keyDown.begin(); i != keyDown.end(); ++i) {
            i->setLenTick(cpos - (i->tick() + ptick));
            }
      //
      // adjust part len && song len
      //
      if (recordPart->lenTick() < (cpos-ptick)) {
            //
            // TODO: check for events outside part boundaries
            //	
            int endTick = song->roundUpBar(cpos);
            recordPart->setLenTick(endTick - ptick);
            }

      unsigned etick = recordPart->endTick();
      if (song->len() < etick)
            song->setLen(etick);
      }

//---------------------------------------------------------
//   clone
//---------------------------------------------------------

void MidiTrack::clone(MidiTrack* t)
      {
      QString name;
      for (int i = 1; ; ++i) {
            name.sprintf("%s-%d", t->name().toLatin1().data(), i);
            TrackList* tl = song->tracks();
            bool found = false;
            for (iTrack it = tl->begin(); it != tl->end(); ++it) {
                  if ((*it)->name() == name) {
                        found = true;
                        break;
                        }
                  }
            if (!found)
                  break;
            }
      setName(name);
      _transposition = t->_transposition;
      _velocity      = t->_velocity;
      _delay         = t->_delay;
      _len           = t->_len;
      _compression   = t->_compression;
      _recordFlag    = t->_recordFlag;
      _mute          = t->_mute;
      _solo          = t->_solo;
      _off           = t->_off;
      _monitor       = t->_monitor;
      _channels      = t->_channels;
      _locked        = t->_locked;
      _inRoutes      = t->_inRoutes;
      _outRoutes     = t->_outRoutes;
      _controller    = t->_controller;
      _autoRead      = t->_autoRead;
      _autoWrite     = t->_autoWrite;
      }

//---------------------------------------------------------
//   isMute
//---------------------------------------------------------

bool MidiTrack::isMute() const
      {
      if (_solo)
            return false;
      if (song->solo())
            return true;
      return _mute;
      }

//---------------------------------------------------------
//   processMidi
//---------------------------------------------------------

void MidiTrack::processMidi(unsigned from, unsigned to, unsigned, unsigned)
      {
      schedEvents.clear();
	//
      // collect events only when transport is rolling
      //
      if (from < to) {
            for (iPart p = parts()->begin(); p != parts()->end(); ++p) {
                  Part* part = p->second;
                  if (part->mute())
                        continue;
                  DrumMap* dm     = drumMap();
                  unsigned offset = _delay + part->tick();

                  if (offset > to)
                        break;

                  EventList* events = part->events();

                  iEvent ie   = events->lower_bound((offset > from) ? 0 : from - offset);
                  iEvent iend = events->lower_bound(to - offset);

                  for (; ie != iend; ++ie) {
                        Event ev = ie->second;
                        if (ev.type() == Meta)        // ignore meta events
                              continue;
                        unsigned tick  = ev.tick() + offset;
                        unsigned frame = AL::tempomap.tick2frame(tick);
                        if (ev.type() == Note) {
                         	if (dm) {
                                    if (dm->entry(dm->outmap(ev.pitch()))->mute)
                                          continue;
                                    }
                              //
                              // maybe we should skip next lines if using a
                              // drummap

                              int pitch = ev.pitch() + _transposition + song->globalPitchShift();
                              if (pitch > 127)
                                    pitch = 127;
                              if (pitch < 0)
                                    pitch = 0;
                              int velo  = ev.velo();
                              velo += _velocity;
                              velo = (velo * _compression) / 100;
                              if (velo > 127)
                                    velo = 127;
                              if (velo < 1)           // no off event
                                    velo = 1;
                              int elen = (ev.lenTick() * _len)/100;
                              if (elen <= 0)     // dont allow zero length
                                    elen = 1;
                              int veloOff = ev.veloOff();

                              unsigned eframe = AL::tempomap.tick2frame(tick+elen);
                              schedEvents.insert(MidiEvent(frame, 0, ME_NOTEON, pitch, velo));
                              schedEvents.insert(MidiEvent(eframe, 0, veloOff ? ME_NOTEOFF : ME_NOTEON, pitch, veloOff));
                              _meter[0] += velo/2;
                              if (_meter[0] > 127.0f)
                                    _meter[0] = 127.0f;
                              }
                        else {
                              schedEvents.insert(MidiEvent(frame, 0, ev));
                              }
                        }
                  }
            //
            // collect controller
            //
            if (autoRead()) {
                  for (iCtrl ic = controller()->begin(); ic != controller()->end(); ++ic) {
                        Ctrl* c = ic->second;
                        iCtrlVal is = c->lowerBound(from);
                        iCtrlVal ie = c->lowerBound(to);
                        for (iCtrlVal ic = is; ic != ie; ++ic) {
                              unsigned frame = AL::tempomap.tick2frame(ic.key());
                              Event ev(Controller);
                              ev.setA(c->id());
                              ev.setB(ic.value().i);
                              schedEvents.insert(MidiEvent(frame, -1, ev));
                              c->setCurVal(ic.value().i);
                              }
                        }
                  }
            }

      //
      // process input routing
      //

      foreach(const Route& r, *inRoutes()) {
            MidiTrackBase* track = (MidiTrackBase*)r.src.track;
            if (track->isMute())
                  continue;
            MidiEventList el;
            track->getEvents(from, to, r.src.channel, &el);

            for (iMidiEvent ie = el.begin(); ie != el.end(); ++ie) {
                  MidiEvent event(*ie);
                  int eventTime = event.time();
                  if (recordFlag() && audio->isRecording()) {
                        unsigned time = AL::tempomap.frame2tick(eventTime);
                        event.setTime(time);  // set tick time
                        recordFifo.put(event);
                        }
             	if (event.type() == ME_NOTEON && (monitor() || recordFlag()))
                  	addMidiMeter(event.dataB());
                  if (monitor()) {
                        if (event.type() == ME_NOTEON) {
                              int pitch = event.dataA() + _transposition + song->globalPitchShift();
                              if (pitch > 127)
                                    pitch = 127;
                              if (pitch < 0)
                                    pitch = 0;
                              event.setA(pitch);
                              if (!event.isNoteOff()) {
                                    int velo = event.dataB() + _velocity;
                                    velo = (velo * _compression) / 100;
                                    if (velo > 127)
                                          velo = 127;
                                    if (velo < 1)
                                          velo = 1;
                                    event.setB(velo);
                                    }
                              }
                        unsigned time = 0; // eventTime + segmentSize*(segmentCount-1);
                        event.setTime(time);
                        schedEvents.insert(event);
                        }
                  }
            }
      }

//---------------------------------------------------------
//   getEvents
//    from/to - midi ticks
//---------------------------------------------------------

void MidiTrack::getEvents(unsigned /*from*/, unsigned /*to*/, int, MidiEventList* dst)
      {
      for (iMidiEvent i = schedEvents.begin(); i != schedEvents.end(); ++i) {
            dst->insert(*i);
            }
      }

//---------------------------------------------------------
//   emitControllerChanged
//---------------------------------------------------------

void MidiTrack::emitControllerChanged(int id)
      {
      if (id == CTRL_PROGRAM && _useDrumMap) {
            int val = ctrlVal(id).i;
            MidiInstrument* mi = instrument();
            DrumMap* dm = mi->getDrumMap(val);
            if (dm == 0)
                  dm = &gmDrumMap;
            if (dm != _drumMap)
                  _drumMap = dm;
            emit drumMapChanged();
            }
      emit controllerChanged(id);
      }

//---------------------------------------------------------
//   setUseDrumMap
//---------------------------------------------------------

void MidiTrack::setUseDrumMap(bool val)
      {
      if (_useDrumMap != val) {
            _useDrumMap = val;
            if (_useDrumMap) {
                  MidiInstrument* mi = instrument();
                  DrumMap* dm;
                  if (mi) {
                        int val = ctrlVal(CTRL_PROGRAM).i;
                        dm = mi->getDrumMap(val);
                        if (dm == 0)
                              dm = &gmDrumMap;
                        }
                  _drumMap = dm;
                  }
            else
                  _drumMap = &noDrumMap;
            emit drumMapChanged();
            emit useDrumMapChanged(_useDrumMap);
            }
      }

//---------------------------------------------------------
//   instrument
//---------------------------------------------------------

MidiInstrument* MidiTrack::instrument()
      {
      if (_outRoutes.isEmpty())
            return genericMidiInstrument;
      return _outRoutes[0].dst.track->instrument();
      }

//---------------------------------------------------------
//   channelNo
//---------------------------------------------------------

int MidiTrack::channelNo() const
      {
      if (_outRoutes.isEmpty())     // TODO: better: remember old channel setting
            return 0;
      return _outRoutes[0].dst.channel;
      }

//---------------------------------------------------------
//   midiOut
//---------------------------------------------------------

MidiOut* MidiTrack::midiOut()
      {
      if (_outRoutes.isEmpty())
            return 0;
      return _outRoutes[0].dst.track->midiOut();
      }

//---------------------------------------------------------
//   setChannel
//---------------------------------------------------------

void MidiTrack::setChannel(int n)
      {
      if (_outRoutes.isEmpty())
            return;
      Route r = _outRoutes[0];
      if (r.dst.channel == n)
            return;
      audio->msgRemoveRoute(r);
      r.dst.channel = n;
      audio->msgAddRoute(r);
	emit channelChanged(n);
      }
