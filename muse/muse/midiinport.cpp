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
#include "audiodev.h"
#include "audio.h"
#include "gconfig.h"
#include "midiinport.h"
#include "jackaudio.h"

//---------------------------------------------------------
//   MidiInPort
//---------------------------------------------------------

MidiInPort::MidiInPort()
   : MidiTrackBase()
      {
      _channels   = 1;
      for (int i = 0; i < MIDI_CHANNELS; ++i)
            activity[i] = 0;
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
      if (!jackPort(0).isZero())
            audioDriver->setPortName(jackPort(), s);
      }

//---------------------------------------------------------
//   MidiInPort::write
//---------------------------------------------------------

void MidiInPort::write(Xml& xml) const
      {
      xml.stag("MidiInPort");
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

void MidiInPort::eventReceived(jack_midi_event_t* ev)
      {
      MidiEvent event;
      event.setB(0);

      //
      // move all events 2*segmentSize into the future to get
      // jitterfree playback
      //
      //  cycle   n-1         n          n+1
      //          -+----------+----------+----------+-
      //               ^          ^          ^
      //               catch      process    play
      //
      const SeqTime* st = audio->seqTime();

//      unsigned curFrame = st->startFrame() + segmentSize;
      unsigned curFrame = st->lastFrameTime;
      event.setTime(curFrame + ev->time);

      event.setChannel(*(ev->buffer) & 0xf);
      int type = *(ev->buffer) & 0xf0;
      int a    = *(ev->buffer + 1) & 0x7f;
      int b    = *(ev->buffer + 2) & 0x7f;
      event.setType(type);
      switch(type) {
            case ME_NOTEON:
            case ME_NOTEOFF:
            case ME_CONTROLLER:
                  event.setA(*(ev->buffer + 1));
                  event.setB(*(ev->buffer + 2));
                  break;
            case ME_PROGRAM:
            case ME_AFTERTOUCH:
                  event.setA(*(ev->buffer + 1));
                  break;

            case ME_PITCHBEND:
                  event.setA(((b << 7) + a) - 8192);
                  break;

            case ME_SYSEX:
                  {
                  int type = *(ev->buffer) & 0xff;
                  switch(type) {
                        case ME_SYSEX:
                              event.setTime(0);      // mark as used
                              event.setType(ME_SYSEX);
                              event.setData((unsigned char*)(ev->buffer + 1),
                                 ev->size - 2);
                              break;
                        case ME_CLOCK:
                        case ME_SENSE:
                              break;
                        default:
                              printf("unknown event 0x%02x\n", type);
                              return;
                        }
                  }
                  return;
            }

      if (midiInputTrace) {
            printf("MidiInput<%s>: ", name().toLatin1().data());
            event.dump();
            }
      //
      // process midi filter pipeline and add event to
      // _recordEvents
      //

      MidiEventList il, ol;
      il.insert(event);
      pipeline()->apply(st->curTickPos, st->nextTickPos, &il, &ol);

      //
      // update midi activity
      // notify gui of new events
      //

      for (iMidiEvent i = ol.begin(); i != ol.end(); ++i) {
            triggerActivity(i->channel());
            song->putEvent(*i);
            if (recordFifo.put(*i))
                  printf("MusE: eventReceived(): fifo overflow\n");
            }
      }

//---------------------------------------------------------
//   afterProcess
//    clear all recorded events after a process cycle
//---------------------------------------------------------

void MidiInPort::afterProcess()
      {
      while (tmpRecordCount--)
            recordFifo.remove();
      }

//---------------------------------------------------------
//   beforeProcess
//    "freeze" fifo for this process cycle
//---------------------------------------------------------

void MidiInPort::beforeProcess()
      {
      if (!jackPort(0).isZero())
            audioDriver->collectMidiEvents(this, jackPort(0));
      tmpRecordCount = recordFifo.getSize();
      }

//---------------------------------------------------------
//   getEvents
//    called from jack process context
//    This method can be called multiple times in a process
//    cycle so we have to empty the fifo at
//    "afterProcess()".
//---------------------------------------------------------

void MidiInPort::getEvents(unsigned, unsigned, int ch, MidiEventList* dst)
      {
      for (int i = 0; i < tmpRecordCount; ++i) {
            const MidiEvent& ev = recordFifo.peek(i);
            if (ch == -1 || (ev.channel() == ch))
                  dst->insert(ev);
            }
      }

//---------------------------------------------------------
//   checkActivity
//---------------------------------------------------------

bool MidiInPort::checkActivity(int channel)
      {
      if (activity[channel])
            --activity[channel];
      return activity[channel] != 0;
      }

//---------------------------------------------------------
//   triggerActivity
//---------------------------------------------------------

void MidiInPort::triggerActivity(int channel)
      {
      activity[channel] = config.guiRefresh / 5 + 1;   // hold for >= 1/5 sec
      }

