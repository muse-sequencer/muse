//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: importmidi.cpp,v 1.26.2.10 2009/11/05 03:14:35 terminator356 Exp $
//
//  (C) Copyright 1999-2003 Werner Schweer (ws@seh.de)
//  (C) Copyright 2012 Tim E. Real (terminator356 on users dot sourceforge dot net)
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

#include <errno.h>
#include <limits.h>

#include <set>
#include <utility>

#include <QMessageBox>

#include "app.h"
#include "song.h"
#include "globals.h"
#include "filedialog.h"
#include "midi.h"
#include "midifile.h"
#include "midiport.h"
#include "midiseq.h"
#include "transport.h"
#include "arranger.h"
#include "mpevent.h"
#include "event.h"
#include "midictrl.h"
#include "instruments/minstrument.h"
#include "drummap.h"
#include "xml.h"
#include "audio.h"
#include "gconfig.h"

using std::set;
using std::pair;


namespace MusEGui {

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
      if (file.isEmpty()) {
               fn = MusEGui::getOpenFileName(MusEGlobal::lastMidiPath, MusEGlobal::midi_file_pattern, this,
               tr("MusE: Import Midi"), 0);
            if (fn.isEmpty())
                  return;
            MusEGlobal::lastMidiPath = fn;
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
                  MusEGlobal::song->update();
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
      bool popenFlag;
      FILE* fp = MusEGui::fileOpen(this, name, QString(".mid"), "r", popenFlag);
      if (fp == 0)
            return true;
      MusECore::MidiFile mf(fp);
      bool rv = mf.read();
      popenFlag ? pclose(fp) : fclose(fp);
      if (rv) {
            QString s(tr("reading midifile\n  "));
            s += name;
            s += tr("\nfailed: ");
            s += mf.error();
            QMessageBox::critical(this, QString("MusE"), s);
            return rv;
            }
            
      MusECore::MidiFileTrackList* etl = mf.trackList();
      int division     = mf.division();

      // Find the default instrument, we may need it later...
      MusECore::MidiInstrument* def_instr = 0;
      if(!MusEGlobal::config.importMidiDefaultInstr.isEmpty())
      {
        for(MusECore::iMidiInstrument i = MusECore::midiInstruments.begin(); i != MusECore::midiInstruments.end(); ++i) 
        {
          if((*i)->iname() == MusEGlobal::config.importMidiDefaultInstr)   
          {
            def_instr = *i;
            break;
          }  
        }
      }
          
      //
      // Need to set up ports and instruments first
      //
      
      MusECore::MidiFilePortMap* usedPortMap = mf.usedPortMap();
      bool dev_changed = false;
      for(MusECore::iMidiFilePort imp = usedPortMap->begin(); imp != usedPortMap->end(); ++imp) 
      {
        MType midi_type = imp->second._midiType;
        QString instr_name = MusEGlobal::config.importInstrNameMetas ? imp->second._instrName : QString();
        MusECore::MidiInstrument* typed_instr = 0;
        MusECore::MidiInstrument* named_instr = 0;
        // Find a typed instrument and a named instrument, if requested
        for(MusECore::iMidiInstrument i = MusECore::midiInstruments.begin(); i != MusECore::midiInstruments.end(); ++i) 
        {
          MusECore::MidiInstrument* mi = *i;
          if(midi_type != MT_UNKNOWN && midi_type == mi->midiType())
            typed_instr = mi;
          if(!instr_name.isEmpty() && instr_name == mi->iname())
            named_instr = mi;
          if((typed_instr && named_instr) || ((typed_instr && instr_name.isEmpty()) || (named_instr && midi_type == MT_UNKNOWN)))
            break;  // Done searching
        }

        int port = imp->first;
        MusECore::MidiPort* mp = &MusEGlobal::midiPorts[port];
        MusECore::MidiDevice* md = mp->device();
        // Take care of assigning devices to empty ports here rather than in midifile.
        //if(MusEGlobal::config.importDevNameMetas)  // TODO
        {
          if(!md)
          {
            QString dev_name = imp->second._subst4DevName;
            md = MusEGlobal::midiDevices.find(dev_name); // Find any type of midi device - HW, synth etc.
            if(md)
            {
              // TEST: Hopefully shouldn't require any routing saves/restorations as in midi port config set device name...
              MusEGlobal::audio->msgSetMidiDevice(mp, md);
              // TEST: Hopefully can get away with this ouside the loop below...
              // Save settings. Use simple version - do NOT set style or stylesheet, this has nothing to do with that.
              //MusEGlobal::muse->changeConfig(true);     // save configuration file
              //MusEGlobal::audio->msgUpdateSoloStates();
              //MusEGlobal::song->update();
              dev_changed = true;
            }
            else
              printf("importMidi error: assign to empty port: device not found: %s\n", dev_name.toLatin1().constData());
          }
        }

        MusECore::MidiInstrument* instr = 0;
        // Priority is exact named instrument over a typed instrument.
        // This allows a named instrument plus a typed sysex, and the name will take priority. 
        // But it is possible that named mismatches may occur. So this named/typed order is user-selectable.
        if(named_instr && (!typed_instr || MusEGlobal::config.importInstrNameMetas))
        {
          instr = named_instr;
          if(MusEGlobal::debugMsg)
            printf("port:%d named instrument found:%s\n",
                    port, instr->iname().toLatin1().constData());
        }
        else if(typed_instr)
        {
        instr = typed_instr;
        if(MusEGlobal::debugMsg)
          printf("port:%d typed instrument found:%s\n",
                  port, instr->iname().toLatin1().constData());
        }
        else if(def_instr)
        {
          instr = def_instr;
          if(MusEGlobal::debugMsg)
            printf("port:%d no named or typed instrument found. Using default:%s\n",
                    port, instr->iname().toLatin1().constData());
        }
        else
        {
          instr = MusECore::genericMidiInstrument;
          if(MusEGlobal::debugMsg)
            printf("port:%d no named, typed, or default instrument found! Using:%s\n",
                    port, instr->iname().toLatin1().constData());
        }
        
        // If the instrument is one of the three standard GM, GS, or XG, mark the usedPort as "ch 10 is drums".
        // Otherwise it's anybody's guess what channel(s) drums are on.
        // Code is a bit HACKISH just to accomplish passing this bool value to the next stage, where tracks are created. 
        if(instr->midiType() != MT_UNKNOWN)
          imp->second._isStandardDrums = true;
          
        // Set the device's instrument - ONLY for non-synths because they provide their own. 
        if(!md || (md->deviceType() != MusECore::MidiDevice::SYNTH_MIDI))  
        {
          // this overwrites any instrument set for this port:
          if(mp->instrument() != instr)
            mp->changeInstrument(instr);
        }
      }
      
      if(dev_changed)
      {
        // TEST: Hopefully can get away with this here instead of inside the loop above...
        // TEST: Are these really necessary as in midi port config set device name?
        // Save settings. Use simple version - do NOT set style or stylesheet, this has nothing to do with that.
        MusEGlobal::muse->changeConfig(true);     // save configuration file
        MusEGlobal::audio->msgUpdateSoloStates(); // 
        MusEGlobal::song->update();
      }
      
      //
      // create MidiTrack and copy events to ->events()
      //    - combine note on/off events
      //    - calculate tick value for internal resolution
      //
      for (MusECore::iMidiFileTrack t = etl->begin(); t != etl->end(); ++t) {
            MusECore::MPEventList& el  = ((*t)->events);
            if (el.empty())
                  continue;
            //
            // if we split the track, SYSEX and META events go into
            // the first target track

            bool first = true;
            
            // vastly changed by flo: replaced that silly loop
            // with that already_processed-set-check.
            // this makes stuff really fast :)
            
            MusECore::iMPEvent ev;
            set< pair<int,int> > already_processed;
            for (ev = el.begin(); ev != el.end(); ++ev)
            {
              if (ev->type() != MusECore::ME_SYSEX && ev->type() != MusECore::ME_META)
              {
                int channel=ev->channel();
                int port=ev->port();
                
                if (already_processed.find(pair<int,int>(channel, port)) == already_processed.end())
                {
                        already_processed.insert(pair<int,int>(channel, port));
                        
                        MusECore::MidiTrack* track = new MusECore::MidiTrack();
                        if ((*t)->_isDrumTrack)
                        {
                           if (MusEGlobal::config.importMidiNewStyleDrum)
                              track->setType(MusECore::Track::NEW_DRUM);
                           else
                              track->setType(MusECore::Track::DRUM);
                        }
                              
                        track->setOutChannel(channel);
                        track->setOutPort(port);

                        MusECore::MidiPort* mport = &MusEGlobal::midiPorts[port];
                        buildMidiEventList(&track->events, el, track, division, first, false); // Don't do loops.
                        first = false;

                        // Comment Added by T356.
                        // Hmm. buildMidiEventList already takes care of this. 
                        // But it seems to work. How? Must test. 
                        //if (channel == 9 && instr->midiType() != MT_UNKNOWN) {
                        MusECore::ciMidiFilePort imp = usedPortMap->find(port);
                        if(imp != usedPortMap->end() && imp->second._isStandardDrums && channel == 9) { // A bit HACKISH, see above
                           if (MusEGlobal::config.importMidiNewStyleDrum)
                              track->setType(MusECore::Track::NEW_DRUM);
                           else
                           {
                              track->setType(MusECore::Track::DRUM);
                              // remap drum pitch with drumOutmap (was: Inmap. flo93 thought this was wrong)
                              for (MusECore::iEvent i = track->events.begin(); i != track->events.end(); ++i) {
                                    MusECore::Event ev  = i->second;
                                    if (ev.isNote()) {
                                          int pitch = MusEGlobal::drumOutmap[ev.pitch()];
                                          ev.setPitch(pitch);
                                          }
                                    else
                                    if(ev.type() == MusECore::Controller)
                                    {
                                      int ctl = ev.dataA();
                                      MusECore::MidiController *mc = mport->drumController(ctl);
                                      if(mc)
                                        ev.setA((ctl & ~0xff) | MusEGlobal::drumOutmap[ctl & 0x7f]);
                                    }
                              }
                           }
                        }
                              
                        processTrack(track);
                        
                        MusEGlobal::song->insertTrack0(track, -1);
                }
              }
						}
						
            if (first) {
                  //
                  // track does only contain non-channel messages
                  // (SYSEX or META)
                  //
                  MusECore::MidiTrack* track = new MusECore::MidiTrack();
                  track->setOutChannel(0);
                  track->setOutPort(0);
                  buildMidiEventList(&track->events, el, track, division, true, false); // Do SysexMeta. Don't do loops.
                  processTrack(track);
                  MusEGlobal::song->insertTrack0(track, -1);
                  }
            }
            
      if (!merge) {
            MusECore::TrackList* tl = MusEGlobal::song->tracks();
            if (!tl->empty()) {
                  MusECore::Track* track = tl->front();
                  track->setSelected(true);
                  }
            MusEGlobal::song->initLen();

            int z, n;
            AL::sigmap.timesig(0, z, n);

            int tempo = MusEGlobal::tempomap.tempo(0);
            transport->setTimesig(z, n);
            transport->setTempo(tempo);

            bool masterF = !MusEGlobal::tempomap.empty();
            MusEGlobal::song->setMasterFlag(masterF);
            transport->setMasterFlag(masterF);

            MusEGlobal::song->updatePos();

            _arranger->reset();
            }
      else {
            MusEGlobal::song->initLen();
           }

      return false;
      }

//---------------------------------------------------------
//   processTrack
//    divide events into parts
//---------------------------------------------------------

void MusE::processTrack(MusECore::MidiTrack* track)
      {
      MusECore::EventList& tevents = track->events;
      if (tevents.empty())
            return;

      //---------------------------------------------------
      //    Parts ermitteln
      //    die Midi-Spuren werden in Parts aufgebrochen;
      //    ein neuer Part wird bei einer LÃ¯Â¿Â½cke von einem
      //    Takt gebildet; die LÃ¯Â¿Â½nge wird jeweils auf
      //    Takte aufgerundet und aligned
      //---------------------------------------------------

      MusECore::PartList* pl = track->parts();

      int lastTick = 0;
      for (MusECore::ciEvent i = tevents.begin(); i != tevents.end(); ++i) {
            const MusECore::Event& event = i->second;
            int epos = event.tick() + event.lenTick();
            if (epos > lastTick)
                  lastTick = epos;
            }

      QString partname = track->name();
      int len = MusEGlobal::song->roundUpBar(lastTick+1);

      // p3.3.27
      if(MusEGlobal::config.importMidiSplitParts)
      {
        
        int bar2, beat;
        unsigned tick;
        AL::sigmap.tickValues(len, &bar2, &beat, &tick);
        
        int lastOff = 0;
        int st = -1;      // start tick current part
        int x1 = 0;       // start tick current measure
        int x2 = 0;       // end tick current measure
  
        for (int bar = 0; bar < bar2; ++bar, x1 = x2) {
              x2 = AL::sigmap.bar2tick(bar+1, 0, 0);
              if (lastOff > x2) {        
                    continue;
                    }
              MusECore::iEvent i1 = tevents.lower_bound(x1);
              MusECore::iEvent i2 = tevents.lower_bound(x2);
  
              if (i1 == i2) {   // empty?
                    if (st != -1) {
                          MusECore::MidiPart* part = new MusECore::MidiPart(track);
                          part->setTick(st);
                          part->setLenTick((lastOff > x1 ? x2 : x1) - st);
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
                    for (MusECore::ciEvent i = i1; i != i2; ++i) {
                          const MusECore::Event& event = i->second;
                          if (event.type() == MusECore::Note) {
                                int off = event.tick() + event.lenTick();
                                if (off > lastOff)
                                      lastOff = off;
                                }
                          }
                    }
              }
        if (st != -1) {
              MusECore::MidiPart* part = new MusECore::MidiPart(track);
              part->setTick(st);
              part->setLenTick(x2-st);
              part->setName(partname);
              pl->add(part);
              }
      }
      else
      {
        // Just one long part...
        MusECore::MidiPart* part = new MusECore::MidiPart(track);
        part->setTick(0);
        part->setLenTick(len);
        part->setName(partname);
        pl->add(part);
      }

      //-------------------------------------------------------------
      //    assign events to parts
      //-------------------------------------------------------------

      for (MusECore::iPart p = pl->begin(); p != pl->end(); ++p) {
            MusECore::MidiPart* part = (MusECore::MidiPart*)(p->second);
            int stick = part->tick();
            int etick = part->tick() + part->lenTick();
            MusECore::iEvent r1 = tevents.lower_bound(stick);
            MusECore::iEvent r2 = tevents.lower_bound(etick);
            int startTick = part->tick();

            for (MusECore::iEvent i = r1; i != r2; ++i) {
                  MusECore::Event& ev = i->second;
                  int ntick = ev.tick() - startTick;
                  ev.setTick(ntick);
                  part->addEvent(ev);
                  }
            tevents.erase(r1, r2);
            }

      if (tevents.size())
            printf("-----------events left: %zd\n", tevents.size());
      for (MusECore::ciEvent i = tevents.begin(); i != tevents.end(); ++i) {
            printf("%d===\n", i->first);
            i->second.dump();
            }
      // all events should be processed:
      if (!tevents.empty())
        printf("THIS SHOULD NEVER HAPPEN: not all events processed at the end of MusE::processTrack()!\n");
      }

//---------------------------------------------------------
//   importController
//---------------------------------------------------------

void MusE::importController(int channel, MusECore::MidiPort* mport, int n)
      {
      MusECore::MidiInstrument* instr = mport->instrument();
      MusECore::MidiCtrlValListList* vll = mport->controller();
      MusECore::iMidiCtrlValList i = vll->find(channel, n);
      if (i != vll->end())
            return;           // controller does already exist
      MusECore::MidiController* ctrl = 0;
      MusECore::MidiControllerList* mcl = instr->controller();
      for (MusECore::iMidiController i = mcl->begin(); i != mcl->end(); ++i) {
            MusECore::MidiController* mc = i->second;
            int cn = mc->num();
            if (cn == n) {
                  ctrl = mc;
                  break;
                  }
            // wildcard?
            if (mc->isPerNoteController() && ((cn & ~0xff) == (n & ~0xff))) {
                  ctrl = i->second;
                  break;
                  }
            }
      if (ctrl == 0) {
            printf("controller 0x%x not defined for instrument %s, channel %d\n",
               n, instr->iname().toLatin1().constData(), channel);
            }
      MusECore::MidiCtrlValList* newValList = new MusECore::MidiCtrlValList(n);
      vll->add(channel, newValList);
      }


//---------------------------------------------------------
//   importPart
//---------------------------------------------------------
void MusE::importPart()
      {
      unsigned curPos = MusEGlobal::song->cpos();
      MusECore::TrackList* tracks = MusEGlobal::song->tracks();
      MusECore::Track* track = 0;
      // Get first selected track:
      for (MusECore::iTrack i = tracks->begin(); i != tracks->end(); i++) {
            MusECore::Track* t = *i;
            if (t->selected()) {
                  // Changed by T356. Support mixed .mpt files.
                  if (t->isMidiTrack() || t->type() == MusECore::Track::WAVE) {
                        track = t;
                        break;
                        }
                  else {
                        QMessageBox::warning(this, QString("MusE"), tr("Import part is only valid for midi and wave tracks!"));
                        return;
                        }
                  }
            }

      if (track) {
            bool loadAll;
            QString filename = MusEGui::getOpenFileName(QString(""), MusEGlobal::part_file_pattern, this, tr("MusE: load part"), &loadAll);
            if (!filename.isEmpty()){
                  // Make a backup of the current clone list, to retain any 'copy' items,
                  //  so that pasting works properly after.
                  MusECore::CloneList copyCloneList = MusEGlobal::cloneList;
                  // Clear the clone list to prevent any dangerous associations with
                  //  current non-original parts.
                  MusEGlobal::cloneList.clear();
            
                  importPartToTrack(filename, curPos, track);
                  
                  // Restore backup of the clone list, to retain any 'copy' items,
                  //  so that pasting works properly after.
                  MusEGlobal::cloneList.clear();
                  MusEGlobal::cloneList = copyCloneList;
               }   
            }
      else {
            QMessageBox::warning(this, QString("MusE"), tr("No track selected for import"));
            }
      }

//---------------------------------------------------------
//   importPartToTrack
//---------------------------------------------------------
void MusE::importPartToTrack(QString& filename, unsigned tick, MusECore::Track* track)
{
      bool popenFlag = false;
      FILE* fp = MusEGui::fileOpen(this, filename, ".mpt", "r", popenFlag, false, false);

      if(fp) 
      {
        MusECore::Xml xml = MusECore::Xml(fp);
        bool firstPart = true;
        int posOffset = 0;
        int  notDone = 0;
        int  done = 0;
        
        bool end = false;
        MusEGlobal::song->startUndo();
        for (;;) 
        {
          MusECore::Xml::Token token = xml.parse();
          const QString& tag = xml.s1();
          switch (token) 
          {
            case MusECore::Xml::Error:
            case MusECore::Xml::End:
                  end = true;
                  break;
            case MusECore::Xml::TagStart:
                  if (tag == "part") {
                        // Read the part.
                        MusECore::Part* p = 0;
                        p = MusECore::Part::readFromXml(xml, track);
                        // If it could not be created...
                        if(!p)
                        {
                          // Increment the number of parts not done and break.
                          ++notDone;
                          break;
                        } 
                        
                        // Increment the number of parts done.
                        ++done;
                        
                        if(firstPart) 
                        {
                          firstPart=false;
                          posOffset = tick - p->tick();
                        }
                        p->setTick(p->tick() + posOffset);
                        MusEGlobal::audio->msgAddPart(p,false);
                        }
                  else
                        xml.unknown("MusE::importPartToTrack");
                  break;
            case MusECore::Xml::TagEnd:
                  break;
            default:
                  end = true;
                  break;
          }
          if(end)
            break;
        }
        fclose(fp);
        MusEGlobal::song->endUndo(SC_PART_INSERTED);
        
        if(notDone)
        {
          int tot = notDone + done;
          QMessageBox::critical(this, QString("MusE"),
            (tot > 1  ?  tr("%n part(s) out of %1 could not be imported.\nLikely the selected track is the wrong type.","",notDone).arg(tot)
                      :  tr("%n part(s) could not be imported.\nLikely the selected track is the wrong type.","",notDone)));
        }
        
        return;
      }
}

} // namespace MuseGui
