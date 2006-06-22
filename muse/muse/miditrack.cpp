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
   : MidiTrackBase(MIDI)
      {
      init();
      _events = new EventList;
      recordPart = 0;
      }

MidiTrack::~MidiTrack()
      {
      delete _events;
      }

//---------------------------------------------------------
//   init
//---------------------------------------------------------

void MidiTrack::init()
      {
      transposition  = 0;
      velocity       = 0;
      delay          = 0;
      len            = 100;          // percent
      compression    = 100;          // percent
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

      xml.intTag("transposition", transposition);
      xml.intTag("velocity", velocity);
      xml.intTag("delay", delay);
      xml.intTag("len", len);
      xml.intTag("compression", compression);

      const PartList* pl = cparts();
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
                  transposition = i;
            else if (tag == "velocity")
                  velocity = i;
            else if (tag == "delay")
                  delay = i;
            else if (tag == "len")
                  len = i;
            else if (tag == "compression")
                  compression = i;
            else if (tag == "part") {
                  Part* p = newPart();
                  p->read(node);
                  parts()->add(p);
                  }
            else if (tag == "locked")
                  _locked = i;
            else if (MidiTrackBase::readProperties(node))
                  printf("MusE:MidiTrack: unknown tag %s\n", e.tagName().toLatin1().data());
            }
      }

//---------------------------------------------------------
//   channel
//---------------------------------------------------------

MidiChannel* MidiTrack::channel() const
      {
      if (_outRoutes.empty())
            return 0;
      return (MidiChannel*)(_outRoutes.front().track);
      }

//---------------------------------------------------------
//   playMidiEvent
//---------------------------------------------------------

void MidiTrack::playMidiEvent(MidiEvent* ev)
      {
      const RouteList* rl = &_outRoutes;
      for (ciRoute r = rl->begin(); r != rl->end(); ++r) {
            ((MidiChannel*)r->track)->playMidiEvent(ev);
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
            audio->msgAddPart(recordPart, false);
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
            bool isOff = (me.type() == ME_NOTEON && me.dataB() == 0)
		             || (me.type() == ME_NOTEOFF);
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
                  event.setTick(time);
                  switch(me.dataA()) {
                        case CTRL_HBANK:
                              hbank = me.dataB();
                              break;

                        case CTRL_LBANK:
                              lbank = me.dataB();
                              break;

                        case CTRL_HDATA:
                              datah = me.dataB();
                              event.setType(Controller);
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
                              event.setType(Controller);
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
                  event.setTick(time);
                  event.setA(CTRL_PROGRAM);
                  event.setB((hbank << 16) | (lbank << 8) | me.dataA());
                  audio->msgAddEvent(event, recordPart, false);
                  ++recordedEvents;
                  updateFlags |= SC_EVENT_INSERTED;
                  }
            else if (me.type() == ME_PITCHBEND) {
                  Event event(Controller);
                  event.setTick(time);
                  event.setA(CTRL_PITCH);
                  event.setB(me.dataA());
                  audio->msgAddEvent(event, recordPart, false);
                  ++recordedEvents;
                  updateFlags |= SC_EVENT_INSERTED;
                  }
            else if (me.type() == ME_SYSEX) {
                  Event event(Sysex);
                  event.setTick(time);
                  event.setData(me.data(), me.len());
                  audio->msgAddEvent(event, recordPart, false);
                  ++recordedEvents;
                  updateFlags |= SC_EVENT_INSERTED;
                  }
            else if (me.type() == ME_AFTERTOUCH) {
                  Event event(CAfter);
                  event.setTick(time);
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
      transposition = t->transposition;
      velocity      = t->velocity;
      delay         = t->delay;
      len           = t->len;
      compression   = t->compression;
      _recordFlag   = t->_recordFlag;
      _mute         = t->_mute;
      _solo         = t->_solo;
      _off          = t->_off;
      _monitor      = t->_monitor;
      _channels     = t->_channels;
      _locked       = t->_locked;
      _inRoutes     = t->_inRoutes;
      _outRoutes    = t->_outRoutes;
      _controller   = t->_controller;
      _autoRead     = t->_autoRead;
      _autoWrite    = t->_autoWrite;
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
//   drumMap
//	return drum map for this track
//	return zero if no drum map is used
//---------------------------------------------------------

DrumMap* MidiTrack::drumMap() const
	{
      MidiChannel* mc = channel();
      if (mc && mc->useDrumMap())
            return mc->drumMap();
	return 0;
      }

//---------------------------------------------------------
//   changeDrumMap
//---------------------------------------------------------

void MidiTrack::changeDrumMap() const
	{
      emit drumMapChanged();
      }

//---------------------------------------------------------
//   getEvents
//    from/to - midi ticks
//---------------------------------------------------------

void MidiTrack::getEvents(unsigned from, unsigned to, int, MPEventList* dst)
      {
      if (from > to) {
            printf("getEvents(): FATAL: cur > next %d > %d\n", from, to);
            return;
            }
	//
      // collect events only when transport is rolling
      //
      if (from < to) {
            for (iPart p = parts()->begin(); p != parts()->end(); ++p) {
                  Part* part = p->second;
                  if (part->mute())
                        continue;
                  DrumMap* dm = ((MidiTrack*)part->track())->drumMap();
                  unsigned offset = delay + part->tick();

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
                        unsigned frame = AL::tempomap.tick2frame(tick) + audio->getFrameOffset();
                        if (ev.type() == Note) {
                         	if (dm) {
                                    if (dm->entry(dm->outmap(ev.pitch()))->mute)
                                          continue;
                                    }
                              //
                              // maybe we should skip next lines if using a
                              // drummap

                              int pitch = ev.pitch() + transposition + song->globalPitchShift();
                              if (pitch > 127)
                                    pitch = 127;
                              if (pitch < 0)
                                    pitch = 0;
                              int velo  = ev.velo();
                              velo += velocity;
                              velo = (velo * compression) / 100;
                              if (velo > 127)
                                    velo = 127;
                              if (velo < 1)           // no off event
                                    velo = 1;
                              int elen = (ev.lenTick() * len)/100;
                              if (elen <= 0)     // don´t allow zero length
                                    elen = 1;
                              int veloOff = ev.veloOff();

                              unsigned eframe = AL::tempomap.tick2frame(tick+elen) + audio->getFrameOffset();
                              dst->add(MidiEvent(frame, 0, 0x90, pitch, velo));
                              dst->add(MidiEvent(eframe, 0, veloOff ? 0x80 : 0x90, pitch, veloOff));
                              _meter[0] += velo/2;
                              if (_meter[0] > 127.0f)
                                    _meter[0] = 127.0f;
                              }
                        else {
                              dst->add(MidiEvent(frame, 0, ev));
                              }
                        }
                  }
            }

      //
      // process input routing
      //

      RouteList* rl = inRoutes();
      for (iRoute i = rl->begin(); i != rl->end(); ++i) {
            MidiTrackBase* track = (MidiTrackBase*)i->track;
            if (track->isMute())
                  continue;
            MPEventList el;
            track->getEvents(from, to, i->channel, &el);

            for (iMPEvent ie = el.begin(); ie != el.end(); ++ie) {
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
                              int pitch = event.dataA() + transposition + song->globalPitchShift();
                              if (pitch > 127)
                                    pitch = 127;
                              if (pitch < 0)
                                    pitch = 0;
                              event.setA(pitch);
                              if (!event.isNoteOff()) {
                                    int velo = event.dataB() + velocity;
                                    velo = (velo * compression) / 100;
                                    if (velo > 127)
                                          velo = 127;
                                    if (velo < 1)
                                          velo = 1;
                                    event.setB(velo);
                                    }
                              }
                        unsigned time = eventTime + segmentSize*(segmentCount-1);
                        event.setTime(time);
                        dst->add(event);
                        }
                  }
            }
      }


