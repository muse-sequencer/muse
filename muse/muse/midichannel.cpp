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

#include "song.h"
#include "midictrl.h"
#include "al/xml.h"
#include "midiedit/drummap.h"
#include "midichannel.h"
#include "midioutport.h"
#include "miditrack.h"
#include "instruments/minstrument.h"

//---------------------------------------------------------
//   MidiChannel
//---------------------------------------------------------

MidiChannel::MidiChannel(MidiOut* p, int ch)
   : MidiTrackBase()
      {
      _port       = p;
      _channelNo  = ch;
      _drumMap    = 0;
      _useDrumMap = false;
      initMidiController();

      //
      // create minimum set of managed controllers
      // to make midi mixer operational
      //
      MidiInstrument* mi = port()->instrument();
      addMidiController(mi, CTRL_PROGRAM);
      addMidiController(mi, CTRL_VOLUME);
      addMidiController(mi, CTRL_PANPOT);
      addMidiController(mi, CTRL_REVERB_SEND);
      addMidiController(mi, CTRL_CHORUS_SEND);
      addMidiController(mi, CTRL_VARIATION_SEND);

      // TODO: setDefault Values
      }

//---------------------------------------------------------
//   MidiChannel
//---------------------------------------------------------

MidiChannel::~MidiChannel()
      {
      }

//---------------------------------------------------------
//   MidiChannel::write
//---------------------------------------------------------

void MidiChannel::write(Xml& xml) const
      {
      xml.tag("MidiChannel idx=\"%d\"", _channelNo);
      MidiTrackBase::writeProperties(xml);
      xml.intTag("useDrumMap", _useDrumMap);
      xml.etag("MidiChannel");
      }

//---------------------------------------------------------
//   MidiChannel::read
//---------------------------------------------------------

void MidiChannel::read(QDomNode node)
      {
      QString drumMapName;
      while (!node.isNull()) {
            QDomElement e = node.toElement();
            QString tag(e.tagName());
            if (tag == "useDrumMap")
                  _useDrumMap = e.text().toInt();
            else if (MidiTrackBase::readProperties(node))
                  printf("MusE:MidiChannel: unknown tag %s\n", tag.toLatin1().data());
            node = node.nextSibling();
            }
      MidiOut* op = port();
      if (op) {
            MidiInstrument* mi = op->instrument();
            int val = ctrlVal(CTRL_PROGRAM).i;
            _drumMap = mi->getDrumMap(val);
            }
      }

//---------------------------------------------------------
//   playMidiEvent
//---------------------------------------------------------

void MidiChannel::playMidiEvent(MidiEvent* ev)
      {
      if (ev->type() == ME_NOTEON) {
            _meter[0] += ev->dataB()/2;
            if (_meter[0] > 127.0f)
                  _meter[0] = 127.0f;
            }
      ev->setChannel(_channelNo);
      _port->playMidiEvent(ev);
      }

//---------------------------------------------------------
//   setUseDrumMap
//---------------------------------------------------------

void MidiChannel::setUseDrumMap(bool val)
      {
      if (_useDrumMap != val) {
            _useDrumMap = val;
            if (_useDrumMap) {
                  int val = ctrlVal(CTRL_PROGRAM).i;
                  MidiOut* op = port();
                  MidiInstrument* mi = op->instrument();
                  DrumMap* dm = mi->getDrumMap(val);
                  if (dm == 0)
                        dm = &gmDrumMap;
                  _drumMap = dm;
                  }
            else
                  _drumMap = &noDrumMap;
      	for (iRoute i = _inRoutes.begin(); i != _inRoutes.end(); ++i) {
                  MidiTrack* mt = (MidiTrack*)(i->track);
                  mt->changeDrumMap();
                  }
            emit useDrumMapChanged(_useDrumMap);
            }
      }

//---------------------------------------------------------
//   emitControllerChanged
//---------------------------------------------------------

void MidiChannel::emitControllerChanged(int id)
      {
      if (id == CTRL_PROGRAM && _useDrumMap) {
            int val = ctrlVal(id).i;
            MidiOut* op = port();
            MidiInstrument* mi = op->instrument();
            DrumMap* dm = mi->getDrumMap(val);
            if (dm == 0)
                  dm = &gmDrumMap;
            if (dm != _drumMap) {
                  _drumMap = dm;
      		for (iRoute i = _inRoutes.begin(); i != _inRoutes.end(); ++i) {
                  	MidiTrack* mt = (MidiTrack*)(i->track);
	                  mt->changeDrumMap();
      	            }
                  }
            }
      emit controllerChanged(id);
      }

//---------------------------------------------------------
//   isMute
//---------------------------------------------------------

bool MidiChannel::isMute() const
      {
      if (_solo)
            return false;
      if (song->solo())
            return true;
      return _mute;
      }


