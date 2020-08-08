//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: exportmidi.cpp,v 1.9.2.1 2009/04/01 01:37:10 terminator356 Exp $
//
//  (C) Copyright 1999-2003 Werner Schweer (ws@seh.de)
//  (C) Copyright 2012, 2016 Tim E. Real (terminator356 on users dot sourceforge dot net)
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

#include <stdio.h>
#include <QString>
#include <QMessageBox>

#include "sig.h"  // Tim.
#include "app.h"
#include "midifile.h"
#include "midi.h"
#include "midictrl.h"
#include "globals.h"
#include "filedialog.h"
#include "track.h"
#include "song.h"
#include "mpevent.h"
#include "event.h"
#include "marker/marker.h"
#include "drummap.h"
#include "gconfig.h"

namespace MusECore {

//---------------------------------------------------------
//   addMarkerList
//---------------------------------------------------------

static void addMarkerList(MPEventList* l, int port = 0)
{
  MusECore::MarkerList* ml = MusEGlobal::song->marker();
  for (MusECore::ciMarker m = ml->begin(); m != ml->end(); ++m) {
        QByteArray ba = m->second.name().toLatin1();
        const char* name = ba.constData();
        int len = ba.length();
        MusECore::MidiPlayEvent ev(m->first, port, MusECore::ME_META, (const unsigned char*)name, len);
        ev.setA(MusECore::ME_META_TEXT_6_MARKER);
        l->add(ev);
        }
};

//---------------------------------------------------------
//   addCopyright
//---------------------------------------------------------

static void addCopyright(MPEventList* l, int port = 0)
{
  QByteArray ba = MusEGlobal::config.copyright.toLatin1();
  const char* copyright = ba.constData();
  if (copyright && *copyright) {
        int len = ba.length();
        MusECore::MidiPlayEvent ev(0, port, MusECore::ME_META, (const unsigned char*)copyright, len);
        ev.setA(MusECore::ME_META_TEXT_2_COPYRIGHT);
        l->add(ev);
        }
}

//---------------------------------------------------------
//   addComment
//---------------------------------------------------------

static void addComment(MPEventList* l, const Track* track, int port = 0)
{
    if (!track->comment().isEmpty()) {
          QByteArray ba = track->comment().toLatin1();
          const char* comment = ba.constData();
          int len = ba.length();
          MusECore::MidiPlayEvent ev(0, port, MusECore::ME_META, (const unsigned char*)comment, len);
          ev.setA(MusECore::ME_META_TEXT_1_COMMENT);
          l->add(ev);
          }
}
  
//---------------------------------------------------------
//   addTempomap
//---------------------------------------------------------

static void addTempomap(MPEventList* l, int port = 0)
{
  MusECore::TempoList* tl = &MusEGlobal::tempomap;
  for (MusECore::ciTEvent e = tl->begin(); e != tl->end(); ++e) {
        MusECore::TEvent* event = e->second;
        unsigned char data[3];
        int tempo = event->tempo;
        data[2] = tempo & 0xff;
        data[1] = (tempo >> 8) & 0xff;
        data[0] = (tempo >> 16) & 0xff;
        MusECore::MidiPlayEvent ev(event->tick, port, MusECore::ME_META, data, 3);
        ev.setA(MusECore::ME_META_SET_TEMPO);
        l->add(ev);
        }
}

//---------------------------------------------------------
//   addTimeSignatures
//---------------------------------------------------------

static void addTimeSignatures(MPEventList* l, int port = 0)
{
  const MusECore::SigList* sl = &MusEGlobal::sigmap;
  for (MusECore::ciSigEvent e = sl->begin(); e != sl->end(); ++e) {
        MusECore::SigEvent* event = e->second;
        int sz = (MusEGlobal::config.exp2ByteTimeSigs ? 2 : 4); // export 2 byte timesigs instead of 4 ?
        unsigned char data[sz];
        data[0] = event->sig.z;
        switch(event->sig.n) {
              case 1:  data[1] = 0; break;
              case 2:  data[1] = 1; break;
              case 4:  data[1] = 2; break;
              case 8:  data[1] = 3; break;
              case 16: data[1] = 4; break;
              case 32: data[1] = 5; break;
              case 64: data[1] = 6; break;
              default:
                    fprintf(stderr, "wrong Signature; denominator is %d\n", event->sig.n);
                    break;
              }
        // By T356. In muse the metronome pulse is fixed at 24 (once per quarter-note).
        // The number of 32nd notes per 24 MIDI clock signals (per quarter-note) is 8.
        if(!MusEGlobal::config.exp2ByteTimeSigs)
        {
          data[2] = 24;
          data[3] = 8;
        }  
        
        MusECore::MidiPlayEvent ev(event->tick, port, MusECore::ME_META, data, sz);
          
        ev.setA(MusECore::ME_META_TIME_SIGNATURE);
        l->add(ev);
        }
}
                        
//---------------------------------------------------------
//   addKeySignatures
//---------------------------------------------------------

static void addKeySignatures(MPEventList* l, int port = 0)
{
  MusECore::KeyList* kl = &MusEGlobal::keymap;
  for (MusECore::ciKeyEvent e = kl->cbegin(); e != kl->cend(); ++e) {
        const MusECore::KeyEvent& event = e->second;
        char kc = 0;
        switch(event.key)
        {
          case KEY_SHARP_BEGIN:
          case KEY_SHARP_END:
          case KEY_B_BEGIN:
          case KEY_B_END:
            continue;
          break;
          case KEY_C:   // C or am: uses # for "black keys"
            kc = 0;
          break;
          case KEY_G:
            kc = 1;
          break;
          case KEY_D:
            kc = 2;
          break;
          case KEY_A:
            kc = 3;
          break;
          case KEY_E:
            kc = 4;
          break;
          case KEY_B: // or H in german.
            kc = 5;
          break;
          case KEY_FIS: //replaces F with E#
            kc = 5;
          break;
          case KEY_C_B:  // the same as C: but uses b for "black keys"
            kc = 0;
          break;
          case KEY_F:
            kc = -1;
          break;
          case KEY_BES: // or B in german
            kc = -2;
          break;
          case KEY_ES:
            kc = -3;
          break;
          case KEY_AS:
            kc = -4;
          break;
          case KEY_DES:
            kc = -5;
          break;
          case KEY_GES: //sounds like FIS: but uses b instead of #
            kc = -5;
          break;
          
        };
        unsigned char data[2];
        data[1] = event.minor;
        data[0] = kc;
        MusECore::MidiPlayEvent ev(event.tick, port, MusECore::ME_META, data, 2);
        ev.setA(MusECore::ME_META_KEY_SIGNATURE);
        l->add(ev);
        }
}

//---------------------------------------------------------
//   addController
//---------------------------------------------------------

static void addController(MPEventList* l, int tick, int port, int channel, int a, int b)
      {
      if (a >= CTRL_7_OFFSET && a < (CTRL_7_OFFSET + 0x10000)) {          // 7 Bit Controller
            l->add(MidiPlayEvent(tick, port, channel, ME_CONTROLLER, a, b));
            }
      else if (a >= CTRL_14_OFFSET && a < (CTRL_14_OFFSET + 0x10000)) {     // 14 Bit Controller
            int ctrlH = (a >> 8) & 0x7f;
            int ctrlL = a & 0x7f;
            int dataH = (b >> 7) & 0x7f;
            int dataL = b & 0x7f;
            l->add(MidiPlayEvent(tick, port, channel, ME_CONTROLLER, ctrlH, dataH));
            l->add(MidiPlayEvent(tick+1, port, channel, ME_CONTROLLER, ctrlL, dataL));
            }
      else if (a >= CTRL_RPN_OFFSET && a < (CTRL_RPN_OFFSET + 0x10000)) {     // RPN 7-Bit Controller
            int ctrlH = (a >> 8) & 0x7f;
            int ctrlL = a & 0x7f;
            l->add(MidiPlayEvent(tick, port, channel, ME_CONTROLLER, CTRL_HRPN, ctrlH));
            l->add(MidiPlayEvent(tick+1, port, channel, ME_CONTROLLER, CTRL_LRPN, ctrlL));
            l->add(MidiPlayEvent(tick+2, port, channel, ME_CONTROLLER, CTRL_HDATA, b));
            }
      else if (a >= CTRL_NRPN_OFFSET && a < (CTRL_NRPN_OFFSET + 0x10000)) {     // NRPN 7-Bit Controller
            int ctrlH = (a >> 8) & 0x7f;
            int ctrlL = a & 0x7f;
            l->add(MidiPlayEvent(tick, port, channel, ME_CONTROLLER, CTRL_HNRPN, ctrlH));
            l->add(MidiPlayEvent(tick+1, port, channel, ME_CONTROLLER, CTRL_LNRPN, ctrlL));
            l->add(MidiPlayEvent(tick+2, port, channel, ME_CONTROLLER, CTRL_HDATA, b));
            }
      else if (a == CTRL_PITCH) {
            int r_a = b + 8192;
            int r_b = r_a >> 7;
            l->add(MidiPlayEvent(tick, port, channel, ME_PITCHBEND, r_a & 0x7f, r_b & 0x7f));
            }
      else if (a == CTRL_PROGRAM) {
            int hb = (b >> 16) & 0xff;
            int lb = (b >> 8) & 0xff;
            int pr = b & 0x7f;
            int tickoffset = 0;
            // REMOVE Tim. Song type removal. Hm, TEST is this OK here?
            //switch(MusEGlobal::song->mtype()) {
            //      case MT_GM:       // no HBANK/LBANK
            //            break;
            //      case MT_GS:
            //      case MT_XG:
            //      case MT_UNKNOWN:
                        if (hb != 0xff) {
                              l->add(MidiPlayEvent(tick, port, channel, ME_CONTROLLER, CTRL_HBANK, hb));
                              ++tickoffset;
                              }
                        if (lb != 0xff) {
                              l->add(MidiPlayEvent(tick+tickoffset, port, channel, ME_CONTROLLER, CTRL_LBANK, lb));
                              ++tickoffset;
                              }
            //            break;
            //      }
            l->add(MidiPlayEvent(tick+tickoffset, port, channel, ME_PROGRAM, pr, 0));
            }
      else if(a == CTRL_AFTERTOUCH)
      {
        l->add(MidiPlayEvent(tick, port, channel, ME_AFTERTOUCH, b & 0x7f, 0));
      }
      else if((a | 0xff) == CTRL_POLYAFTER)
      {
        l->add(MidiPlayEvent(tick, port, channel, ME_POLYAFTER, a & 0x7f, b & 0x7f));
      }
      else if (a >= CTRL_INTERNAL_OFFSET && a < CTRL_RPN14_OFFSET)      // Unaccounted for internal controller
            return;
      else if (a >= CTRL_RPN14_OFFSET && a < (CTRL_RPN14_OFFSET + 0x10000)) {     // RPN14 Controller
            int ctrlH = (a >> 8) & 0x7f;
            int ctrlL = a & 0x7f;
            int dataH = (b >> 7) & 0x7f;
            int dataL = b & 0x7f;
            l->add(MidiPlayEvent(tick,   port, channel, ME_CONTROLLER, CTRL_HRPN, ctrlH));
            l->add(MidiPlayEvent(tick+1, port, channel, ME_CONTROLLER, CTRL_LRPN, ctrlL));
            l->add(MidiPlayEvent(tick+2, port, channel, ME_CONTROLLER, CTRL_HDATA, dataH));
            l->add(MidiPlayEvent(tick+3, port, channel, ME_CONTROLLER, CTRL_LDATA, dataL));
            }
      else if (a >= CTRL_NRPN14_OFFSET && a < (CTRL_NRPN14_OFFSET + 0x10000)) {     // NRPN14 Controller
            int ctrlH = (a >> 8) & 0x7f;
            int ctrlL = a & 0x7f;
            int dataH = (b >> 7) & 0x7f;
            int dataL = b & 0x7f;
            l->add(MidiPlayEvent(tick,   port, channel, ME_CONTROLLER, CTRL_HNRPN, ctrlH));
            l->add(MidiPlayEvent(tick+1, port, channel, ME_CONTROLLER, CTRL_LNRPN, ctrlL));
            l->add(MidiPlayEvent(tick+2, port, channel, ME_CONTROLLER, CTRL_HDATA, dataH));
            l->add(MidiPlayEvent(tick+3, port, channel, ME_CONTROLLER, CTRL_LDATA, dataL));
            }
      }

//---------------------------------------------------------
//   addEventList
//     part can be NULL meaning no part used.
//     track can be NULL meaning no concept of drum notes is allowed in init sequences.
//---------------------------------------------------------

static void addEventList(const MusECore::EventList& evlist, MusECore::MPEventList* mpevlist,
                         const MusECore::MidiTrack* track, const MusECore::Part* part, int port, int channel)
{      
  DrumMap dm;
  for (MusECore::ciEvent i = evlist.cbegin(); i != evlist.cend(); ++i) 
  {
    const MusECore::Event& ev = i->second;
    int tick = ev.tick();
    if(part)
      tick += part->tick();
    switch (ev.type()) 
    {
          case MusECore::Note:
          {
                if (ev.velo() == 0) {
                      printf("Warning: midi note has velocity 0, (ignored)\n");
                      continue;
                      }
                int pitch = ev.pitch();
                int fin_pitch = pitch;
                int fin_port = port;
                int fin_chan = channel;

                if(MusEGlobal::config.exportDrumMapOverrides)
                {
                  if (track && track->type() == MusECore::Track::DRUM) {
                        // Map drum-notes to the drum-map values
                        // We must look at what the drum map WOULD say at the note's tick,
                        //  not what it says now at the current cursor.
                        track->getMapItemAt(tick, pitch, dm, WorkingDrumMapEntry::AllOverrides);
                        fin_pitch = dm.anote;
                        // Default to track port if -1 and track channel if -1.
                        // Port is only allowed to change in format 1.
                        if(dm.port != -1 && MusEGlobal::config.smfFormat != 0)
                          fin_port = dm.port;
                        if(dm.channel != -1)
                          fin_chan = dm.channel;
                        }
                }

                // If the port or channel is overridden by a drum map, DO NOT add the event here
                //  because it requires its own track. That is taken care of in exportMidi().
                if(fin_port != port ||
                  // Channel is allowed to be different but can only cause a new track in format 1.
                  (fin_chan != channel && MusEGlobal::config.exportChannelOverridesToNewTrack && MusEGlobal::config.smfFormat != 0))
                  continue;

                int velo  = ev.velo();
                int veloOff  = ev.veloOff();
                int len   = ev.lenTick();

                //---------------------------------------
                //   apply trackinfo values
                //---------------------------------------

                if (track && ((!track->isDrumTrack() && track->transposition)
                    || track->velocity
                    || track->compression != 100
                    || track->len != 100)) {
                      // Transpose only midi not drum tracks.
                      if(!track->isDrumTrack())
                        fin_pitch += track->transposition;
                      if (fin_pitch > 127)
                            fin_pitch = 127;
                      if (fin_pitch < 0)
                            fin_pitch = 0;

                      velo += track->velocity;
                      velo = (velo * track->compression) / 100;
                      if (velo > 127)
                            velo = 127;
                      if (velo < 1)           // no off event
                            velo = 1;
                            // REMOVE Tim. Noteoff. Added. Zero means zero. Should mean no note at all?
                            // Although we might not actually play such an event in the midi.cpp engine, 
                            //  I don't like the idea of discarding the event during export.
                            // Keeping the notes is more important than the velocities, I'd say.
                            //break;
                      
                      len = (len *  track->len) / 100;
                      }
                if (len <= 0)
                      len = 1;

                mpevlist->add(MusECore::MidiPlayEvent(tick, fin_port, fin_chan, MusECore::ME_NOTEON, fin_pitch, velo));
                if(MusEGlobal::config.expOptimNoteOffs)  // Save space by replacing note offs with note on velocity 0
                  mpevlist->add(MusECore::MidiPlayEvent(tick+len, fin_port, fin_chan, MusECore::ME_NOTEON, fin_pitch, 0));
                else
                  mpevlist->add(MusECore::MidiPlayEvent(tick+len, fin_port, fin_chan, MusECore::ME_NOTEOFF, fin_pitch, veloOff));
                }
                break;

          case MusECore::Controller:
          {
                int ctlnum = ev.dataA();
                int fin_ctlnum = ctlnum;
                int fin_port = port;
                int fin_chan = channel;

                if(MusEGlobal::config.exportDrumMapOverrides)
                {
                  // Is it a drum controller event, according to the track port's instrument?
                  if(MusEGlobal::midiPorts[port].drumController(ctlnum))
                  {
                    if (track && track->type() == MusECore::Track::DRUM) {
                      int v_idx = ctlnum & 0x7f;
                      track->getMapItemAt(tick, v_idx, dm, WorkingDrumMapEntry::AllOverrides);
                      fin_ctlnum = (ctlnum & ~0xff) | dm.anote;
                      // Default to track port if -1 and track channel if -1.
                      // Port is only allowed to change in format 1.
                      if(dm.port != -1 && MusEGlobal::config.smfFormat != 0)
                        fin_port = dm.port;
                      if(dm.channel != -1)
                        fin_chan = dm.channel;
                    }
                  }
                }

                // If the port or channel is overridden by a drum map, DO NOT add the event here
                //  because it requires its own track. That is taken care of in exportMidi().
                if(fin_port != port ||
                  // Channel is allowed to be different but can only cause a new track in format 1.
                  (fin_chan != channel && MusEGlobal::config.exportChannelOverridesToNewTrack && MusEGlobal::config.smfFormat != 0))
                  continue;

                addController(mpevlist, tick, fin_port, fin_chan, fin_ctlnum, ev.dataB());
          }
                break;

          case MusECore::Sysex:
                { 
                  mpevlist->add(MusECore::MidiPlayEvent(tick, port, MusECore::ME_SYSEX, ev.eventData()));
                  //MusECore::MidiPlayEvent ev(tick, port, MusECore::ME_SYSEX, ev.eventData());
                  //ev.setChannel(channel);  // Sysex are channelless, but this is required for sorting!
                  //mpevlist->add(ev);
                }
                break;
          case MusECore::Meta:
                {
                MusECore::MidiPlayEvent mpev(tick, port, MusECore::ME_META, ev.eventData());
                mpev.setA(ev.dataA());
                //mpev.setChannel(channel);  // Metas are channelless, but this is required for sorting!
                mpevlist->add(mpev);
                }
                break;
          case MusECore::Wave:
                break;
          }
    }
}

//---------------------------------------------------------
//   addTrackEvents
//---------------------------------------------------------

static void addTrackEvents(MPEventList* l, int port, int channel, const MidiTrack* track)
{
  //---------------------------------------------------
  //    Write all track events.
  //---------------------------------------------------

  const MusECore::PartList* parts = track->cparts();
  for (MusECore::ciPart p = parts->cbegin(); p != parts->cend(); ++p) {
        const MusECore::MidiPart* part    = (MusECore::MidiPart*) (p->second);
        MusECore::addEventList(part->events(), l, track, part, port, channel); 
        }
}

static void writeDeviceOrPortMeta(int port, MPEventList* mpel)
{
  if(port >= 0 && port < MusECore::MIDI_PORTS)
  {
    if(MusEGlobal::config.exportPortsDevices & MusEGlobal::PORT_NUM_META)
    {
      unsigned char port_char = (unsigned char)port;
      MusECore::MidiPlayEvent ev(0, port, MusECore::ME_META, &port_char, 1);
      ev.setA(MusECore::ME_META_PORT_CHANGE);    // Meta port change
      //ev.setChannel(channel);  // Metas are channelless, but this is required for sorting!
      mpel->add(ev);
    }

    if(MusEGlobal::config.exportPortsDevices & MusEGlobal::DEVICE_NAME_META)
    {
      MusECore::MidiDevice* dev = MusEGlobal::midiPorts[port].device();
      const char* str;
      int len;
      QByteArray ba;
      if(dev && !dev->name().isEmpty())
        ba = dev->name().toLatin1();
      else
        ba = QString::number(port).toLatin1();
      str = ba.constData();
      len = ba.length();
      MusECore::MidiPlayEvent ev(0, port, MusECore::ME_META, (const unsigned char*)str, len);
      ev.setA(MusECore::ME_META_TEXT_9_DEVICE_NAME);    // Meta Device Name
      //ev.setChannel(channel);  // Metas are channelless, but this is required for sorting!
      mpel->add(ev);
    }
  }
}

static void writeInitSeqOrInstrNameMeta(int port, int channel, MPEventList* mpel)
{
  if(MidiInstrument* instr = MusEGlobal::midiPorts[port].instrument())
  {
    //--------------------------
    // Port midi init sequence
    //--------------------------
    if(MusEGlobal::config.exportModeInstr & MusEGlobal::MODE_SYSEX)
    {
      EventList* el = instr->midiInit();
      if(!el->empty())
        addEventList(*el, mpel, NULL, NULL, port, channel); // No track or part passed for init sequences
    }

    //--------------------------
    // Instrument Name meta
    //--------------------------
    if(!instr->iname().isEmpty() &&
      (MusEGlobal::config.exportModeInstr & MusEGlobal::INSTRUMENT_NAME_META))
    {
      const char* str;
      int len;
      QByteArray ba = instr->iname().toLatin1();
      str = ba.constData();
      len = ba.length();
      MidiPlayEvent ev(0, port, ME_META, (const unsigned char*)str, len);
      ev.setA(ME_META_TEXT_4_INSTRUMENT_NAME);    // Meta Instrument Name
      //ev.setChannel(channel);  // Metas are channelless, but this is required for sorting!
      mpel->add(ev);
    }
  }
}

static void writeTrackNameMeta(int port, Track* track, MPEventList* mpel)
{
  if (!track->name().isEmpty())
  {
    QByteArray ba = track->name().toLatin1();
    const char* name = ba.constData();
    int len = ba.length();
    MidiPlayEvent ev(0, port, ME_META, (const unsigned char*)name, len);
    ev.setA(ME_META_TEXT_3_TRACK_NAME);    // Meta Sequence/Track Name
    //ev.setChannel(channel);  // Metas are channelless, but this is required for sorting!
    mpel->add(ev);
  }
}

} // namespace MusECore

namespace MusEGui {

//---------------------------------------------------------
//   exportMidi
//---------------------------------------------------------

void MusE::exportMidi()
      {
      if(MusEGlobal::config.smfFormat == 0)  // Want single track? Warn if multiple ports in song...
      {
        MusECore::MidiTrackList* mtl = MusEGlobal::song->midis();       
        int prev_port = -1;
        for(MusECore::ciMidiTrack im = mtl->begin(); im != mtl->end(); ++im) 
        {
          int port = (*im)->outPort();
          if(prev_port == -1)
          {
            prev_port = port;
            continue;
          }
          if(port != prev_port)
          {
            if(QMessageBox::warning(this, 
              tr("MusE: Warning"), 
              tr("The song uses multiple ports but export format 0 (single track) is set.\n"
                 "The first track's port will be used. Playback will likely be wrong\n"
                 " unless the channels used in one port are different from all other ports.\n"
                 "Canceling and setting a different export format would be better.\nContinue?"), 
                 QMessageBox::Ok | QMessageBox::Cancel, QMessageBox::Ok) 
                != QMessageBox::Ok) 
              return;
            break;
          }
        }
      }
      
      MusEGui::MFile file(QString("midis"), QString(".mid"));

      FILE* fp = file.open("w", MusEGlobal::midi_file_save_pattern, this, false, true,
         tr("MusE: Export Midi"));
      if (fp == 0)
            return;
      MusECore::MidiFile mf(fp);

      MusECore::TrackList* tl = MusEGlobal::song->tracks();       // Changed to full track list so user can rearrange tracks.
      MusECore::MidiFileTrackList* mtl = new MusECore::MidiFileTrackList;

      MusECore::DrumMap dm;
      std::set<int> used_ports;
      
      // There will always be at least one track, regardless of format.
      int track_count = 1;
      MusECore::MidiFileTrack* mft = new MusECore::MidiFileTrack;
      mtl->push_back(mft);
      MusECore::MPEventList* l = &(mft->events);
      // Write track marker
      addMarkerList(l);
      // Write copyright
      addCopyright(l);
      // Write comment // TODO Remove?
      //addComment(l, track);
      // Write tempomap
      addTempomap(l);
      // Write time signatures
      addTimeSignatures(l);
      // Write key signatures
      addKeySignatures(l);

        int tr_cnt = 0;
        for(MusECore::ciTrack im = tl->cbegin(); im != tl->cend(); ++im)
        {
          if(!(*im)->isMidiTrack())
            continue;

          if(MusEGlobal::config.smfFormat != 0)
          {  
            mft = new MusECore::MidiFileTrack;
            mtl->push_back(mft);
            l = &(mft->events);
            ++track_count;
          }

          MusECore::MidiTrack* track = (MusECore::MidiTrack*)(*im);
          const int port         = track->outPort();
          const int channel      = track->outChannel();

          if(tr_cnt == 0 || MusEGlobal::config.smfFormat != 0)
          {
            // Write comment
            addComment(l, track, port);
            // Write track name
            writeTrackNameMeta(port, track, l);
            // Write device name or port change meta
            if(MusEGlobal::config.exportPortDeviceSMF0)
              writeDeviceOrPortMeta(port, l);
            //writeInitSeqOrInstrNameMeta(port, channel, l);
          }
          // Write midi port init sequence: GM/GS/XG etc. 
          //  and Instrument Name meta.
          std::set<int>::iterator iup = used_ports.find(port);
          if(iup == used_ports.end())
          {
            if(port >= 0 && port < MusECore::MIDI_PORTS)
            {
              if(tr_cnt == 0 || MusEGlobal::config.smfFormat != 0)
                MusECore::writeInitSeqOrInstrNameMeta(port, channel, l);
              used_ports.insert(port);
            }
          }
          // Write all track events.
          addTrackEvents(l, port, channel, track);

          ++tr_cnt;
        }

      // For drum tracks with drum map port overrides, we may need to add extra tracks.
      // But we can can only do that if multi-track format is chosen.
      if(MusEGlobal::config.exportDrumMapOverrides && MusEGlobal::config.smfFormat != 0)
      {
        MusECore::MidiFileTrackList aux_mtl;
        for (MusECore::ciTrack it = tl->begin(); it != tl->end(); ++it)
        {
          if(!(*it)->isDrumTrack())
            continue;

          MusECore::MidiTrack* track = static_cast<MusECore::MidiTrack*>(*it);

          int port         = track->outPort();
          int channel      = track->outChannel();

          MusECore::PartList* parts = track->parts();
          for (MusECore::iPart ip = parts->begin(); ip != parts->end(); ++ip)
          {
            MusECore::MidiPart* part = (MusECore::MidiPart*) (ip->second);
            const MusECore::EventList& evlist = part->events();
            for (MusECore::ciEvent iev = evlist.begin(); iev != evlist.end(); ++iev)
            {
              const MusECore::Event& ev = iev->second;
              int tick = ev.tick() + part->tick();
              switch (ev.type())
              {
                    case MusECore::Note:
                    {
                          if (ev.velo() == 0) {
                                fprintf(stderr, "Warning: midi note has velocity 0, (ignored)\n");
                                continue;
                                }
                          int pitch;
                          int fin_port = port;
                          int fin_chan = channel;

                          if (track->type() == MusECore::Track::DRUM) {
                                // Map drum-notes to the drum-map values
                                // We must look at what the drum map WOULD say at the note's tick,
                                //  not what it says now at the current cursor.
                                int instr = ev.pitch();
                                track->getMapItemAt(tick, instr, dm, MusECore::WorkingDrumMapEntry::AllOverrides);
                                pitch = dm.anote;
                                // Default to track port if -1 and track channel if -1.
                                if(dm.port != -1)
                                  fin_port = dm.port;
                                if(dm.channel != -1)
                                  fin_chan = dm.channel;
                                }

                          // We are only looking for port or channel overrides. They require a separate track.
                          if(fin_port == port &&
                             (fin_chan == channel || !MusEGlobal::config.exportChannelOverridesToNewTrack))
                            continue;

                          // Is there already a MidiFileTrack for the port?
                          MusECore::MidiFileTrack* aux_mft = 0;
                          for(MusECore::iMidiFileTrack imft = aux_mtl.begin(); imft != aux_mtl.end(); ++imft)
                          {
                            MusECore::MidiFileTrack* t = *imft;
                            // Take the first event's port (in this case they are all the same).
                            const MusECore::MidiPlayEvent& mpe = *t->events.begin();
                            if(mpe.port() == fin_port)
                            {
                              aux_mft = t;
                              break;
                            }
                          }
                          // If not, create a new MidiFileTrack.
                          if(!aux_mft)
                          {
                            aux_mft = new MusECore::MidiFileTrack;
                            aux_mft->_isDrumTrack = true;

                            //-----------------------------------
                            //   track name
                            //-----------------------------------
                            // TODO: Maybe append some text here?
                            MusECore::writeTrackNameMeta(fin_port, track, &aux_mft->events);

                            //-----------------------------------------
                            //    Write device name or port change meta
                            //-----------------------------------------
                            MusECore::writeDeviceOrPortMeta(fin_port, &aux_mft->events);

                            //---------------------------------------------------
                            //    Write midi port init sequence: GM/GS/XG etc.
                            //     and Instrument Name meta.
                            //---------------------------------------------------
                            std::set<int>::iterator iup = used_ports.find(fin_port);
                            if(iup == used_ports.end())
                            {
                              MusECore::writeInitSeqOrInstrNameMeta(fin_port, fin_chan, &aux_mft->events);
                              used_ports.insert(fin_port);
                            }

                            aux_mtl.push_back(aux_mft);
                            mtl->push_back(aux_mft);

                            // Increment the track count.
                            ++track_count;
                          }

                          int velo  = ev.velo();
                          int veloOff  = ev.veloOff();
                          int len   = ev.lenTick();

                          //---------------------------------------
                          //   apply trackinfo values
                          //---------------------------------------

                          if (track && (track->velocity
                              || track->compression != 100
                              || track->len != 100)) {
                                if (pitch > 127)
                                      pitch = 127;
                                if (pitch < 0)
                                      pitch = 0;

                                velo += track->velocity;
                                velo = (velo * track->compression) / 100;
                                if (velo > 127)
                                      velo = 127;
                                if (velo < 1)           // no off event
                                      velo = 1;
                                      // REMOVE Tim. Noteoff. Added. Zero means zero. Should mean no note at all?
                                      // Although we might not actually play such an event in the midi.cpp engine,
                                      //  I don't like the idea of discarding the event during export.
                                      // Keeping the notes is more important than the velocities, I'd say.
                                      //break;

                                len = (len *  track->len) / 100;
                                }
                          if (len <= 0)
                                len = 1;

                          aux_mft->events.add(MusECore::MidiPlayEvent(tick, fin_port, fin_chan, MusECore::ME_NOTEON, pitch, velo));
                          if(MusEGlobal::config.expOptimNoteOffs)  // Save space by replacing note offs with note on velocity 0
                            aux_mft->events.add(MusECore::MidiPlayEvent(tick+len, fin_port, fin_chan, MusECore::ME_NOTEON, pitch, 0));
                          else
                            aux_mft->events.add(MusECore::MidiPlayEvent(tick+len, fin_port, fin_chan, MusECore::ME_NOTEOFF, pitch, veloOff));
                          }
                          break;

                    case MusECore::Controller:
                    {
                          int fin_port = port;
                          int fin_chan = channel;
                          int ctlnum = ev.dataA();
                          int fin_ctlnum = ctlnum;

                          // Is it a drum controller event, according to the track port's instrument?
                          if(MusEGlobal::midiPorts[port].drumController(ctlnum))
                          {
                            if (track->type() == MusECore::Track::DRUM) {
                              int v_idx = ctlnum & 0x7f;
                              track->getMapItemAt(tick, v_idx, dm, MusECore::WorkingDrumMapEntry::AllOverrides);
                              fin_ctlnum = (ctlnum & ~0xff) | dm.anote;
                              // Default to track port if -1 and track channel if -1.
                              if(dm.port != -1)
                                fin_port = dm.port;
                              if(dm.channel != -1)
                                fin_chan = dm.channel;
                            }
                          }

                          // We are only looking for port or channel overrides. They require a separate track.
                          if(fin_port == port &&
                             (fin_chan == channel || !MusEGlobal::config.exportChannelOverridesToNewTrack))
                            continue;

                          // Is there already a MidiFileTrack for the port?
                          MusECore::MidiFileTrack* aux_mft = 0;
                          for(MusECore::iMidiFileTrack imft = aux_mtl.begin(); imft != aux_mtl.end(); ++imft)
                          {
                            MusECore::MidiFileTrack* t = *imft;
                            // Take the first event's port (in this case they are all the same).
                            const MusECore::MidiPlayEvent& mpe = *t->events.begin();
                            if(mpe.port() == fin_port)
                            {
                              aux_mft = t;
                              break;
                            }
                          }
                          // If not, create a new MidiFileTrack.
                          if(!aux_mft)
                          {
                            aux_mft = new MusECore::MidiFileTrack;
                            aux_mft->_isDrumTrack = true;

                            //-----------------------------------
                            //   track name
                            //-----------------------------------
                            // TODO: Maybe append some text here?
                            MusECore::writeTrackNameMeta(fin_port, track, &aux_mft->events);

                            //-----------------------------------------
                            //    Write device name or port change meta
                            //-----------------------------------------
                            MusECore::writeDeviceOrPortMeta(fin_port, &aux_mft->events);

                            //---------------------------------------------------
                            //    Write midi port init sequence: GM/GS/XG etc.
                            //     and Instrument Name meta.
                            //---------------------------------------------------
                            std::set<int>::iterator iup = used_ports.find(fin_port);
                            if(iup == used_ports.end())
                            {
                              MusECore::writeInitSeqOrInstrNameMeta(fin_port, fin_chan, &aux_mft->events);
                              used_ports.insert(fin_port);
                            }

                            aux_mtl.push_back(aux_mft);
                            mtl->push_back(aux_mft);

                            // Increment the track count.
                            ++track_count;
                          }

                          MusECore::addController(&aux_mft->events, tick, fin_port, fin_chan, fin_ctlnum, ev.dataB());
                    }
                          break;

                    case MusECore::Sysex:
                    case MusECore::Meta:
                    case MusECore::Wave:
                          break;
                    }
              }
          }
        }
      }





      mf.setDivision(MusEGlobal::config.midiDivision);
      // Takes ownership of mtl and its contents.
      mf.setTrackList(mtl, track_count);
      mf.write();

      }

} // namespace MusEGui
