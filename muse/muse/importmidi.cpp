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

#include "globals.h"
#include "muse.h"
#include "song.h"
#include "widgets/filedialog.h"
#include "midi.h"
#include "midifile.h"
#include "transport.h"
#include "midiedit/drummap.h"
#include "al/sig.h"
#include "al/tempo.h"
#include "instruments/minstrument.h"
#include "gconfig.h"
#include "driver/alsamidi.h"
#include "part.h"

//---------------------------------------------------------
//   importMidi
//---------------------------------------------------------

void MusE::importMidi()
      {
      QString empty("");
      importMidi(empty);
      }

void MusE::importMidi(const QString &file)
      {
      QString fn;
      QStringList pattern;

      const char** s = midi_file_pattern;
      while (*s)
            pattern << *s++;

      if (file.isEmpty()) {
            fn = getOpenFileName(lastMidiPath, pattern, this,
               tr("MusE: Import Midi"));
            if (fn.isEmpty())
                  return;
            lastMidiPath = fn;
            }
      else
            fn = file;

      int n = QMessageBox::question(this, appName,
         tr("Add midi file to current project?\n"),
         tr("&Add to Project"),
         tr("&Replace"),
         tr("&Abort"), 0, 2);

      switch (n) {
            case 0:
                  importMidi(fn, true);
                  song->update(-1);
                  break;
            case 1:
                  loadProjectFile(fn, false, false);    // replace
                  break;
            default:
                  return;
            }
      }

//---------------------------------------------------------
//   importMidi
//    return true on error
//---------------------------------------------------------

bool MusE::importMidi(const QString name, bool merge)
      {
      QFile* fp = fileOpen(this, name, QString(".mid"), QIODevice::ReadOnly);
      if (fp == 0)
            return true;
      MidiFile mf;
      bool rv = mf.read(fp);
      fp->close();
      delete fp;

      if (rv) {
            QString s(tr("reading midifile\n  "));
            s += name;
            s += tr("\nfailed: ");
            s += mf.error();
            QMessageBox::critical(this, QString("MusE"), s);
            return rv;
            }
      MidiFileTrackList* etl = mf.trackList();
      int division           = mf.division();

      MidiOutPort* outPort = 0;
      MidiInPort* inPort = 0;

      if (!merge || song->midiOutPorts()->empty()) {
            outPort = new MidiOutPort();
            outPort->setDefaultName();
            song->insertTrack0(outPort, -1);

            //
            // route output to preferred midi device
            //
            if (!config.defaultMidiOutputDevice.isEmpty()) {
                  Route dst(config.defaultMidiOutputDevice, 0, Route::MIDIPORT);
                  outPort->outRoutes()->push_back(dst);
                  }
            //
            // set preferred instrument
            //
            MidiInstrument* instr = 0;    // genericMidiInstrument;
            for (iMidiInstrument mi = midiInstruments.begin(); mi != midiInstruments.end(); ++mi) {
                  if ((*mi)->iname() == config.defaultMidiInstrument) {
                        instr = *mi;
                        break;
                        }
                  }

            if (config.createDefaultMidiInput) {
                  inPort = new MidiInPort();
                  inPort->setDefaultName();
                  song->insertTrack0(inPort, -1);
                  if (config.connectToAllMidiDevices) {
                        std::list<PortName>* ol = midiDriver->inputPorts();
                        for (std::list<PortName>::iterator ip = ol->begin(); ip != ol->end(); ++ip) {
                              Route src(ip->name, 0, Route::MIDIPORT);
                              inPort->inRoutes()->push_back(src);
                              }
                        }
                  else if (!config.defaultMidiInputDevice.isEmpty()) {
                        Route src(config.defaultMidiInputDevice, 0, Route::MIDIPORT);
                        inPort->inRoutes()->push_back(src);
                        }
                  }

            //
            // if midi file is GM/GS/XG this overrides the preferred
            // instrument setting

            if (mf.midiType() != MT_GENERIC) {
                  MidiInstrument* instr2 = 0;
                  for (iMidiInstrument i = midiInstruments.begin(); i != midiInstruments.end(); ++i) {
                        MidiInstrument* mi = *i;
                        switch(mf.midiType()) {
                              case MT_GM:
                                    if (mi->iname() == "GM")
                                          instr2 = mi;
                                    break;
                              case MT_GS:
                                    if (mi->iname() == "GS")
                                          instr2 = mi;
                                    break;
                              case MT_XG:
                                    if (mi->iname() == "XG")
                                          instr2 = mi;
                                    break;
                              case MT_GENERIC: // cannot happen
                                    break;
                              }
                        if (instr2)
                              break;
                        }
                  if (instr2)
                        instr = instr2;
                  }
            if (instr == 0)
                  instr = genericMidiInstrument;
            outPort->setInstrument(instr);
            }
      else
            outPort = song->midiOutPorts()->front();

      //
      // create MidiTrack and copy events to ->events()
      //    - combine note on/off events
      //    - calculate tick value for internal resolution
      //
      for (iMidiFileTrack t = etl->begin(); t != etl->end(); ++t) {
            MPEventList* el  = &((*t)->events);
            if (el->empty())
                  continue;
            //
            // if we split the track, SYSEX and META events go into
            // the first target track

            bool first = true;
            for (int channel = 0; channel < MIDI_CHANNELS; ++channel) {
                  //
                  // check if there are any events for channel in track:
                  //
                  iMPEvent i;
                  for (i = el->begin(); i != el->end(); ++i) {
                        MidiEvent ev = *i;
                        if (ev.type() != ME_SYSEX
                           && ev.type() != ME_META
                           && ev.channel() == channel)
                              break;
                        }
                  if (i == el->end())
                        continue;

                  MidiTrack* track = new MidiTrack();
//TODO3                  if ((*t)->isDrumTrack)
//                        track->setUseDrumMap(true);
                  track->outRoutes()->push_back(Route(outPort->channel(channel), -1, Route::TRACK));
                  if (inPort && config.connectToAllMidiTracks) {
                        for (int ch = 0; ch < MIDI_CHANNELS; ++ch) {
                              Route src(inPort, ch, Route::TRACK);
                              track->inRoutes()->push_back(src);
                              }
                        }

                  EventList* mel = track->events();
                  buildMidiEventList(mel, el, track, division, first);
                  first = false;

                  for (iEvent i = mel->begin(); i != mel->end(); ++i) {
                        Event event = i->second;
                        if (event.type() == Controller) {
                              int ctrl = event.dataA();
                              MidiInstrument* instr = outPort->instrument();
                              MidiChannel* mc = outPort->channel(channel);
                              mc->addMidiController(instr, ctrl);
                              CVal val;
                              val.i = event.dataB();
                              mc->addControllerVal(ctrl, event.tick(), val);
                              }
                        }
#if 0 //TODO3
                  if (channel == 9) {
                        track->setUseDrumMap(true);
                        //
                        // remap drum pitch with drumInmap
                        //
                        EventList* tevents = track->events();
                        for (iEvent i = tevents->begin(); i != tevents->end(); ++i) {
                              Event ev  = i->second;
                              if (ev.isNote()) {
                                    int pitch = drumInmap[ev.pitch()];
                                    ev.setPitch(pitch);
                                    }
                              }
                        }
#endif
                  processTrack(track);
                  if (track->name().isEmpty())
                        track->setDefaultName();
                  song->insertTrack0(track, -1);
                  }
            if (first) {
                  //
                  // track does only contain non-channel messages
                  // (SYSEX or META)
                  //
                  MidiTrack* track = new MidiTrack();
                  addRoute(Route(track, -1, Route::TRACK), Route(outPort->channel(0), -1, Route::TRACK));
                  EventList* mel = track->events();
                  buildMidiEventList(mel, el, track, division, true);
                  processTrack(track);
                  if (track->name().isEmpty())
                        track->setDefaultName();
                  song->insertTrack0(track, -1);
                  }
            }

      if (!merge) {
            TrackList* tl = song->tracks();
            if (!tl->empty()) {
                  Track* track = tl->front();
                  track->setSelected(true);
                  }
            unsigned int l = 1;
            MidiTrackList* mtl = song->midis();
            for (iMidiTrack t = mtl->begin(); t != mtl->end(); ++t) {
                  MidiTrack* track = *t;
                  PartList* parts = track->parts();
                  for (iPart p = parts->begin(); p != parts->end(); ++p) {
                        unsigned last = p->second->tick() + p->second->lenTick();
                        if (last > l)
                              l = last;
                        }
                  }
            song->setLen(l);

            AL::TimeSignature sig = AL::sigmap.timesig(0);
            int z = sig.z;
            int n = sig.n;

            int tempo = AL::tempomap.tempo(0);
            transport->setTimesig(z, n);
//TD            transport->setTempo(tempo);

            bool masterF = !AL::tempomap.empty();
            song->setMasterFlag(masterF);
            transport->setMasterFlag(masterF);

            song->updatePos();
            }
      return false;
      }

//---------------------------------------------------------
//   processTrack
//    divide events into parts
//---------------------------------------------------------

void MusE::processTrack(MidiTrack* track)
      {
      EventList* tevents = track->events();
      if (tevents->empty())
            return;

      //---------------------------------------------------
      //    Parts ermitteln
      //    die Midi-Spuren werden in Parts aufgebrochen;
      //    ein neuer Part wird bei einer Lücke von einem
      //    Takt gebildet; die Länge wird jeweils auf
      //    Takte aufgerundet und aligned
      //---------------------------------------------------

      PartList* pl = track->parts();

      int lastTick = 0;
      for (iEvent i = tevents->begin(); i != tevents->end(); ++i) {
            Event event = i->second;
            int epos = event.tick() + event.lenTick();
            if (epos > lastTick)
                  lastTick = epos;
            }

      int len = song->roundUpBar(lastTick+1);
      int bar2, beat;
      unsigned tick;
      AL::sigmap.tickValues(len, &bar2, &beat, &tick);

      QString partname = track->name();

      int lastOff = 0;
      int st = -1;      // start tick current part
      int x1 = 0;       // start tick current measure
      int x2 = 0;       // end tick current measure

      for (int bar = 0; bar < bar2; ++bar, x1 = x2) {
            x2 = AL::sigmap.bar2tick(bar+1, 0, 0);
            if (lastOff > x2) {
                  // this measure is busy!
                  continue;
                  }
            iEvent i1 = tevents->lower_bound(x1);
            iEvent i2 = tevents->lower_bound(x2);

            if (i1 == i2) {   // empty?
                  if (st != -1) {
                        Part* part = new Part(track);
                        part->setType(AL::TICKS);
                        part->setTick(st);
                        part->setLenTick(x1-st);
// printf("new part %d len: %d\n", st, x1-st);
                        part->setName(partname);
                        pl->add(part);
                        st = -1;
                        }
                  }
            else {
                  if (st == -1)
                        st = x1;    // begin new  part
                  //HACK:
                  //lastOff:
                  for (iEvent i = i1; i != i2; ++i) {
                        Event event = i->second;
                        if (event.type() == Note) {
                              int off = event.tick() + event.lenTick();
                              if (off > lastOff)
                                    lastOff = off;
                              }
                        }
                  }
            }
      if (st != -1) {
            Part* part = new Part(track);
            part->setType(AL::TICKS);
            part->setTick(st);
// printf("new part %d len: %d\n", st, x2-st);
            part->setLenTick(x2-st);
            part->setName(partname);
            pl->add(part);
            }

      //-------------------------------------------------------------
      //    assign events to parts
      //-------------------------------------------------------------

      for (iPart p = pl->begin(); p != pl->end(); ++p) {
            Part* part = p->second;
            int stick = part->tick();
            int etick = part->tick() + part->lenTick();
            iEvent r1 = tevents->lower_bound(stick);
            iEvent r2 = tevents->lower_bound(etick);
            int startTick = part->tick();

            EventList* el = part->events();
            for (iEvent i = r1; i != r2; ++i) {
                  Event ev = i->second;
                  int ntick = ev.tick() - startTick;
                  ev.setTick(ntick);
                  el->add(ev, ntick);
                  }
            tevents->erase(r1, r2);
            }

      if (tevents->size())
            printf("-----------events left: %ld\n", tevents->size());
      for (iEvent i = tevents->begin(); i != tevents->end(); ++i) {
            printf("%d===\n", i->first);
            i->second.dump();
            }
      // all events should be processed:
      assert(tevents->empty());
      }

