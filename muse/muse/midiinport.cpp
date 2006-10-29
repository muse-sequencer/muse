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
#include "midiplugin.h"
#include "midi.h"
#include "midictrl.h"
#include "al/xml.h"
#include "driver/mididev.h"
#include "driver/audiodev.h"
#include "audio.h"

#include "midiinport.h"

//---------------------------------------------------------
//   MidiInPort
//---------------------------------------------------------

MidiInPort::MidiInPort()
   : MidiTrackBase(MIDI_IN)
      {
      _channels   = 1;
      recordRead  = 0;
      recordWrite = 0;
      recordCount = 0;
      }

//---------------------------------------------------------
//   MidiInPort
//---------------------------------------------------------

MidiInPort::~MidiInPort()
      {
      }

//---------------------------------------------------------
//   setName
//---------------------------------------------------------

void MidiInPort::setName(const QString& s)
      {
      Track::setName(s);
      if (!alsaPort(0).isZero())
            midiDriver->setPortName(alsaPort(), s);
      if (!jackPort(0).isZero())
            audioDriver->setPortName(jackPort(), s);
      }

//---------------------------------------------------------
//   MidiInPort::write
//---------------------------------------------------------

void MidiInPort::write(Xml& xml) const
      {
      xml.tag("MidiInPort");
      MidiTrackBase::writeProperties(xml);
      xml.etag("MidiInPort");
      }

//---------------------------------------------------------
//   MidiInPort::read
//---------------------------------------------------------

void MidiInPort::read(QDomNode node)
      {
      while (!node.isNull()) {
            QDomElement e = node.toElement();
            QString tag(e.tagName());
            if (MidiTrackBase::readProperties(node))
                  printf("MusE:MidiInPort: unknown tag %s\n", tag.toLatin1().data());
            node = node.nextSibling();
            }
      }

//---------------------------------------------------------
//   midiReceived
//    called from midiSeq context
//---------------------------------------------------------

#ifndef __APPLE__
void MidiInPort::eventReceived(snd_seq_event_t* ev)
      {
      MidiEvent event;
      event.setB(0);
      event.setTime(audioDriver->framePos());

      switch(ev->type) {
            case SND_SEQ_EVENT_NOTEON:
                  event.setChannel(ev->data.note.channel);
                  event.setType(ME_NOTEON);
                  event.setA(ev->data.note.note);
                  event.setB(ev->data.note.velocity);
                  break;

            case SND_SEQ_EVENT_KEYPRESS:
                  event.setChannel(ev->data.note.channel);
                  event.setType(ME_POLYAFTER);
                  event.setA(ev->data.note.note);
                  event.setB(ev->data.note.velocity);
                  break;

            case SND_SEQ_EVENT_SYSEX:
                  event.setTime(0);      // mark as used
                  event.setType(ME_SYSEX);
                  event.setData((unsigned char*)(ev->data.ext.ptr)+1,
                     ev->data.ext.len-2);
                  break;

            case SND_SEQ_EVENT_NOTEOFF:
                  event.setChannel(ev->data.note.channel);
                  event.setType(ME_NOTEOFF);
                  event.setA(ev->data.note.note);
                  event.setB(ev->data.note.velocity);
                  break;

            case SND_SEQ_EVENT_CHANPRESS:
                  event.setChannel(ev->data.control.channel);
                  event.setType(ME_AFTERTOUCH);
                  event.setA(ev->data.control.value);
                  break;

            case SND_SEQ_EVENT_PGMCHANGE:
                  event.setChannel(ev->data.control.channel);
                  event.setType(ME_PROGRAM);
                  event.setA(ev->data.control.value);
                  break;

            case SND_SEQ_EVENT_PITCHBEND:
                  event.setChannel(ev->data.control.channel);
                  event.setType(ME_CONTROLLER);
                  event.setA(CTRL_PITCH);
                  event.setB(ev->data.control.value);
                  break;

            case SND_SEQ_EVENT_CONTROLLER:
                  event.setChannel(ev->data.control.channel);
                  event.setType(ME_CONTROLLER);
                  event.setA(ev->data.control.param);
                  event.setB(ev->data.control.value);
                  break;
            }

      if (midiInputTrace) {
            printf("MidiInput<%s>: ", name().toLatin1().data());
            event.dump();
            }
      //
      // process midi filter pipeline and add event to
      // _recordEvents
      //

      MPEventList il, ol;
      il.insert(event);
      pipeline()->apply(audio->curTickPos(), audio->nextTickPos(), &il, &ol);

      //
      // update midi meter
      // notify gui of new events
      //
      for (iMPEvent i = ol.begin(); i != ol.end(); ++i) {
            if (i->type() == ME_NOTEON)
                  addMidiMeter(i->dataB());
            song->putEvent(*i);
            if (recordCount == RECORD_FIFO_SIZE) {
                  printf("MusE: eventReceived(): fifo overflow\n");
                  continue;
                  }
            recordFifo[recordWrite] = *i;
            ++recordWrite;
            if (recordWrite == RECORD_FIFO_SIZE)
                  recordWrite = 0;
            ++recordCount;
            }
      }
#endif

//---------------------------------------------------------
//   afterProcess
//    clear all recorded events after a process cycle
//---------------------------------------------------------

void MidiInPort::afterProcess()
      {
      while (tmpRecordCount--) {
            ++recordRead;
            if (recordRead >= RECORD_FIFO_SIZE)
                  recordRead = 0;
            --recordCount;
            }
      }

//---------------------------------------------------------
//   beforeProcess
//---------------------------------------------------------

void MidiInPort::beforeProcess()
      {
      tmpRecordCount = recordCount;
      }

//---------------------------------------------------------
//   getEvents
//    called from jack process context
//---------------------------------------------------------

void MidiInPort::getEvents(unsigned, unsigned, int ch, MPEventList* dst)
      {
      int tmpRecordRead = recordRead;
      for (int i = 0; i < tmpRecordCount; ++i) {
            dst->insert(recordFifo[tmpRecordRead]);
            ++tmpRecordRead;
            if (tmpRecordRead >= RECORD_FIFO_SIZE)
                  tmpRecordRead = 0;
            }
      }

