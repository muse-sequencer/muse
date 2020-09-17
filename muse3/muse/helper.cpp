//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: helper.cpp,v 1.1.1.1 2003/10/27 18:51:27 wschweer Exp $
//  (C) Copyright 2003 Werner Schweer (ws@seh.de)
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

#include <list>

#include "helper.h"
#include "song.h"
#include "app.h"
#include "icons.h"
#include "synth.h"
#include "functions.h"
#include "operations.h"
#include "gconfig.h"

#include "driver/jackmidi.h"
#include "route.h"
#include "mididev.h"
#include "midiport.h"
#include "globaldefs.h"
#include "globals.h"
#include "audio.h"
#include "audiodev.h"
#include "midi_consts.h"
#include "midiseq.h"
#include "midictrl.h"
#include "menutitleitem.h"
#include "dssihost.h"
#include "lv2host.h"
#include "vst_native.h"
#include "appearance.h"
#include "event.h"
#include "popupmenu.h"
#include "mpevent.h"
#include "track.h"
#include "part.h"
#include "drummap.h"
#include "xml.h"

#include <strings.h>

#include <QApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QFileDialog>
#include <QByteArray>
#include <QStyle>
#include <QStyleFactory>
#include <QVector>
#include <QMessageBox>
#include <QActionGroup>
#include <QMenu>
#include <QWidget>

using std::set;

namespace MusEGlobal {
extern bool hIsB;
}

namespace MusECore {


static const char* vall[] = {
      "c","c#","d","d#","e","f","f#","g","g#","a","a#","h"
      };
static const char* valu[] = {
      "C","C#","D","D#","E","F","F#","G","G#","A","A#","H"
      };

//---------------------------------------------------------
//   pitch2string
//---------------------------------------------------------

QString pitch2string(int v)
      {
      if (v < 0 || v > 127)
            return QString("----");
      int octave = (v / 12) - 2;
      QString o = QString::number(octave);
      int i = v % 12;
      QString s(octave < 0 ? valu[i] : vall[i]);
      if (MusEGlobal::hIsB) {
            if (s == "h")
                  s = "b";
            else if (s == "H")
                  s = "B";
            }
      return s + o;
      }


//---------------------------------------------------------
//   dumpMPEvent
//---------------------------------------------------------

void dumpMPEvent(const MEvent* ev)
      {
      fprintf(stderr, "time:%d port:%d chan:%d ", ev->time(), ev->port(), ev->channel()+1);
      if (ev->type() == ME_NOTEON) {   
            QString s = pitch2string(ev->dataA());
            fprintf(stderr, "NoteOn %s(0x%x) %d\n", s.toLatin1().constData(), ev->dataA(), ev->dataB());
           }
      else if (ev->type() == ME_NOTEOFF) {  
            QString s = pitch2string(ev->dataA());
            fprintf(stderr, "NoteOff %s(0x%x) %d\n", s.toLatin1().constData(), ev->dataA(), ev->dataB());
           }
      else if (ev->type() == ME_SYSEX) {
            fprintf(stderr, "SysEx len %d 0x%0x ...\n", ev->len(), ev->constData()[0]);
            }
      else
            fprintf(stderr, "type:0x%02x a=%d b=%d\n", ev->type(), ev->dataA(), ev->dataB());
      }
      
#if 0

// -------------------------------------------------------------------------------------------------------
// enumerateJackMidiDevices()
// This version creates separate devices for Jack midi input and outputs. 
// It does not attempt to pair them together.
// -------------------------------------------------------------------------------------------------------

void enumerateJackMidiDevices()
{
  if(!MusEGlobal::checkAudioDevice())
    return;

  MidiDevice* dev = 0;
  PendingOperationList operations;
  
  // If Jack is running.
  if(MusEGlobal::audioDevice->deviceType() == AudioDevice::JACK_AUDIO)  
  {
    char good_name[ROUTE_PERSISTENT_NAME_SIZE];
    std::list<QString> sl;
//     sl = MusEGlobal::audioDevice->inputPorts(true, 1);  // Ask for second aliases.
    sl = MusEGlobal::audioDevice->inputPorts(true);
    for(std::list<QString>::iterator i = sl.begin(); i != sl.end(); ++i)
    {
      QByteArray ba = (*i).toLatin1();
      const char* port_name = ba.constData();
      void* const port = MusEGlobal::audioDevice->findPort(port_name);
      if(port)
      {
        //dev = MidiJackDevice::createJackMidiDevice(*i, 1); 
        dev = MidiJackDevice::createJackMidiDevice(QString(), 1); // Let it pick the name
        if(dev)
        {
          // Get a good routing name.
          MusEGlobal::audioDevice->portName(port, good_name, ROUTE_PERSISTENT_NAME_SIZE);
          
          const Route dstRoute(Route::JACK_ROUTE, -1, NULL, -1, -1, -1, good_name); // Persistent route.
          // If audio is running, this calls jack_connect() and waits for the audio thread to execute addRoute().
          // If audio is not running, this directly executes addRoute(), bypassing the audio messaging system,
          //  and jack_connect() is not called.
          //MusEGlobal::audio->msgAddRoute(srcRoute, dstRoute);
          //
          // We only want to add the route, not call jack_connect - jack may not have been activated yet.
          // If it has been, we should be calling our graph changed handler soon, it will handle actual connections.
          // If audio is not running yet, this directly executes addRoute(), bypassing the audio messaging system,
          if(!dev->outRoutes()->contains(dstRoute))
            operations.add(MusECore::PendingOperationItem(dev->outRoutes(), dstRoute, MusECore::PendingOperationItem::AddRouteNode));
        }  
      }
    }
    
    //sl = MusEGlobal::audioDevice->outputPorts(true, 1); // Ask for second aliases.
    sl = MusEGlobal::audioDevice->outputPorts(true);
    for(std::list<QString>::iterator i = sl.begin(); i != sl.end(); ++i)
    {
      QByteArray ba = (*i).toLatin1();
      const char* port_name = ba.constData();
      void* const port = MusEGlobal::audioDevice->findPort(port_name);
      if(port)
      {
        dev = MidiJackDevice::createJackMidiDevice(QString(), 2); // Let it pick the name
        if(dev)
        {
          // Get a good routing name.
          MusEGlobal::audioDevice->portName(port, good_name, ROUTE_PERSISTENT_NAME_SIZE);
          const Route srcRoute(Route::JACK_ROUTE, -1, NULL, -1, -1, -1, good_name); // Persistent route.
          if(!dev->inRoutes()->contains(srcRoute))
            operations.add(MusECore::PendingOperationItem(dev->inRoutes(), srcRoute, MusECore::PendingOperationItem::AddRouteNode));
        }  
      }
    }
  }
  if(!operations.empty())
  {
    //operations.add(MusECore::PendingOperationItem((TrackList*)NULL, PendingOperationItem::UpdateSoloStates));
    MusEGlobal::audio->msgExecutePendingOperations(operations); // Don't update here.
    //MusEGlobal::song->update(SC_ROUTE);
  }
}

#else 

// -------------------------------------------------------------------------------------------------------
// enumerateJackMidiDevices()
// This version attempts to pair together Jack midi input and outputs into single MidiDevices,
//  similar to how ALSA presents pairs of inputs and outputs.
// -------------------------------------------------------------------------------------------------------

void enumerateJackMidiDevices()
{
  if(!MusEGlobal::checkAudioDevice())
    return;

  PendingOperationList operations;
  
  // If Jack is running.
  if(MusEGlobal::audioDevice->deviceType() == AudioDevice::JACK_AUDIO)  
  {
    MidiDevice* dev = 0;
    char w_good_name[ROUTE_PERSISTENT_NAME_SIZE];
    char r_good_name[ROUTE_PERSISTENT_NAME_SIZE];
    std::list<QString> wsl;
    std::list<QString> rsl;
    wsl = MusEGlobal::audioDevice->inputPorts(true);
    rsl = MusEGlobal::audioDevice->outputPorts(true);

    for(std::list<QString>::iterator wi = wsl.begin(); wi != wsl.end(); ++wi)
    {
      QByteArray w_ba = (*wi).toLatin1();
      const char* w_port_name = w_ba.constData();

      bool match_found = false;
      void* const w_port = MusEGlobal::audioDevice->findPort(w_port_name);
      if(w_port)
      {
        // Get a good routing name.
        MusEGlobal::audioDevice->portName(w_port, w_good_name, ROUTE_PERSISTENT_NAME_SIZE);
          
        for(std::list<QString>::iterator ri = rsl.begin(); ri != rsl.end(); ++ri)
        {
          QByteArray r_ba = (*ri).toLatin1();
          const char* r_port_name = r_ba.constData();

          void* const r_port = MusEGlobal::audioDevice->findPort(r_port_name);
          if(r_port)
          {
            // Get a good routing name.
            MusEGlobal::audioDevice->portName(r_port, r_good_name, ROUTE_PERSISTENT_NAME_SIZE);

            const size_t w_sz = strlen(w_good_name);
            const size_t r_sz = strlen(r_good_name);
            size_t start_c = 0;
            size_t w_end_c = w_sz;
            size_t r_end_c = r_sz;
            
            while(start_c < w_sz && start_c < r_sz &&
                  w_good_name[start_c] == r_good_name[start_c])
              ++start_c;
            
            while(w_end_c > 0 && r_end_c > 0)
            {
              if(w_good_name[w_end_c - 1] != r_good_name[r_end_c - 1])
                break;
              --w_end_c;
              --r_end_c;
            }
            
            if(w_end_c > start_c && r_end_c > start_c)
            {
              const char* w_str = w_good_name + start_c;
              const char* r_str = r_good_name + start_c;
              const size_t w_len = w_end_c - start_c;
              const size_t r_len = r_end_c - start_c;
              
              // Do we have a matching pair?
              if((w_len == 7 && r_len == 8 &&
                  strncasecmp(w_str, "capture", w_len) == 0 &&
                  strncasecmp(r_str, "playback", r_len) == 0) ||
                  
                 (w_len == 8 && r_len == 7 &&
                  strncasecmp(w_str, "playback", w_len) == 0 &&
                  strncasecmp(r_str, "capture", r_len) == 0) || 
                  
                 (w_len == 5 && r_len == 6 &&
                  strncasecmp(w_str, "input", w_len) == 0 &&
                  strncasecmp(r_str, "output", r_len) == 0) || 
                  
                 (w_len == 6 && r_len == 5 &&
                  strncasecmp(w_str, "output", w_len) == 0 &&
                  strncasecmp(r_str, "input", r_len) == 0) || 
                  
                 (w_len == 2 && r_len == 3 &&
                  strncasecmp(w_str, "in", w_len) == 0 &&
                  strncasecmp(r_str, "out", r_len) == 0) || 
                  
                 (w_len == 3 && r_len == 2 &&
                  strncasecmp(w_str, "out", w_len) == 0 &&
                  strncasecmp(r_str, "in", r_len) == 0) || 
                  
                 (w_len == 1 && r_len == 1 &&
                  strncasecmp(w_str, "p", w_len) == 0 &&
                  strncasecmp(r_str, "c", r_len) == 0) || 
                  
                 (w_len == 1 && r_len == 1 &&
                  strncasecmp(w_str, "c", w_len) == 0 &&
                  strncasecmp(r_str, "p", r_len) == 0))
              {
                dev = MidiJackDevice::createJackMidiDevice(QString(), 3); // Let it pick the name
                if(dev)
                {
                  const Route srcRoute(Route::JACK_ROUTE, -1, NULL, -1, -1, -1, r_good_name); // Persistent route.
                  const Route dstRoute(Route::JACK_ROUTE, -1, NULL, -1, -1, -1, w_good_name); // Persistent route.
                  // We only want to add the route, not call jack_connect - jack may not have been activated yet.
                  // If it has been, we should be calling our graph changed handler soon, it will handle actual connections.
                  // If audio is not running yet, this directly executes addRoute(), bypassing the audio messaging system,
                  if(!dev->inRoutes()->contains(srcRoute))
                    operations.add(MusECore::PendingOperationItem(dev->inRoutes(), srcRoute, MusECore::PendingOperationItem::AddRouteNode));
                  if(!dev->outRoutes()->contains(dstRoute))
                    operations.add(MusECore::PendingOperationItem(dev->outRoutes(), dstRoute, MusECore::PendingOperationItem::AddRouteNode));
                }
                
                rsl.erase(ri);  // Done with this read port. Remove.
                match_found = true;
                break;
              }
            }
          }
        }  
      }

      if(!match_found)
      {
        // No match was found. Create a single writeable device.
        dev = MidiJackDevice::createJackMidiDevice(QString(), 1); // Let it pick the name
        if(dev)
        {
          const Route dstRoute(Route::JACK_ROUTE, -1, NULL, -1, -1, -1, w_good_name); // Persistent route.
          // We only want to add the route, not call jack_connect - jack may not have been activated yet.
          // If it has been, we should be calling our graph changed handler soon, it will handle actual connections.
          // If audio is not running yet, this directly executes addRoute(), bypassing the audio messaging system,
          if(!dev->outRoutes()->contains(dstRoute))
            operations.add(MusECore::PendingOperationItem(dev->outRoutes(), dstRoute, MusECore::PendingOperationItem::AddRouteNode));
        }  
      }

    }

    // Create the remaining readable ports as single readable devices.
    for(std::list<QString>::iterator ri = rsl.begin(); ri != rsl.end(); ++ri)
    {
      dev = MidiJackDevice::createJackMidiDevice(QString(), 2); // Let it pick the name
      if(dev)
      {
        QByteArray r_ba = (*ri).toLatin1();
        const char* r_port_name = r_ba.constData();

        void* const r_port = MusEGlobal::audioDevice->findPort(r_port_name);
        if(r_port)
        {
          // Get a good routing name.
          MusEGlobal::audioDevice->portName(r_port, r_good_name, ROUTE_PERSISTENT_NAME_SIZE);
          const Route srcRoute(Route::JACK_ROUTE, -1, NULL, -1, -1, -1, r_good_name); // Persistent route.
          if(!dev->inRoutes()->contains(srcRoute))
            operations.add(MusECore::PendingOperationItem(dev->inRoutes(), srcRoute, MusECore::PendingOperationItem::AddRouteNode));
        }      
      }  
    }
  }
  
  if(!operations.empty())
  {
    //operations.add(MusECore::PendingOperationItem((TrackList*)NULL, PendingOperationItem::UpdateSoloStates));
    MusEGlobal::audio->msgExecutePendingOperations(operations); // Don't update here.
    //MusEGlobal::song->update(SC_ROUTE);
  }
}
#endif   // enumerateJackMidiDevices

// -------------------------------------------------------------------------------------------------------
// populateMidiPorts()
// Attempts to auto-populate midi ports with found devices.
// -------------------------------------------------------------------------------------------------------

void populateMidiPorts()
{
  if(!MusEGlobal::checkAudioDevice())
    return;

  MusECore::MidiDevice* dev = 0;
  int port_num = 0;
  int jack_midis_found = 0;
  bool def_in_found = false;
//  bool def_out_found = false;

  // If Jack is running, prefer Jack midi devices over ALSA.
  if(MusEGlobal::audioDevice->deviceType() == MusECore::AudioDevice::JACK_AUDIO)  
  {
    for(MusECore::iMidiDevice i = MusEGlobal::midiDevices.begin(); i != MusEGlobal::midiDevices.end(); ++i) 
    {
      dev = *i;
      if(dev)
      {
        ++jack_midis_found;
        MidiPort* mp = &MusEGlobal::midiPorts[port_num];
        MusEGlobal::audio->msgSetMidiDevice(mp, dev);

// robert: removing the default init on several places to allow for the case
// where you rather want the midi track to default to the last created port
// this can only happen if there is _no_ default set

//        // Global function initMidiPorts() already sets defs to port #1, but this will override.
//        if(!def_out_found && dev->rwFlags() & 0x1)
//        {
//          mp->setDefaultOutChannels(1);
//          def_out_found = true;
//        }
//        else
          mp->setDefaultOutChannels(0);

        if(!def_in_found && dev->rwFlags() & 0x2)
        {
          mp->setDefaultInChannels(1);
          def_in_found = true;
        }
        else
          mp->setDefaultInChannels(0);

        if(++port_num == MusECore::MIDI_PORTS)
          return;
      }  
    }
  }
  //else
  // If Jack is not running, use ALSA devices.
  // Try to do the user a favour: If we still have no Jack devices, even if Jack is running, fill with ALSA.
  // It is possible user has Jack running on ALSA back-end but without midi support.
  // IE. They use Jack for audio but use ALSA for midi!
  // If unwanted, remove "|| jack_midis_found == 0".
  if(MusEGlobal::audioDevice->deviceType() == MusECore::AudioDevice::DUMMY_AUDIO || jack_midis_found == 0)  
  {
    for(MusECore::iMidiDevice i = MusEGlobal::midiDevices.begin(); i != MusEGlobal::midiDevices.end(); ++i) 
    {
      if((*i)->deviceType() != MusECore::MidiDevice::ALSA_MIDI)
        continue;
      dev = *i;
      MidiPort* mp = &MusEGlobal::midiPorts[port_num];
      MusEGlobal::audio->msgSetMidiDevice(mp, dev);

      // robert: removing the default init on several places to allow for the case
      // where you rather want the midi track to default to the last created port
      // this can only happen if there is _no_ default set

//      // Global function initMidiPorts() already sets defs to port #1, but this will override.
//      if(!def_out_found && dev->rwFlags() & 0x1)
//      {
//        mp->setDefaultOutChannels(1);
//        def_out_found = true;
//      }
//      else
        mp->setDefaultOutChannels(0);

      if(!def_in_found && dev->rwFlags() & 0x2)
      {
        mp->setDefaultInChannels(1);
        def_in_found = true;
      }
      else
        mp->setDefaultInChannels(0);

      if(++port_num == MusECore::MIDI_PORTS)
        return;
    }
  }
}

Part* partFromSerialNumber(int serial)
{
        TrackList* tl = MusEGlobal::song->tracks();
	for (iTrack it = tl->begin(); it != tl->end(); ++it)
	{
		PartList* pl = (*it)->parts();
		iPart ip;
		for (ip = pl->begin(); ip != pl->end(); ++ip)
			if (ip->second->sn() == serial)
				return ip->second;
	}
	
	printf("ERROR: partFromSerialNumber(%i) wasn't able to find an appropriate part!\n",serial);
	return NULL;
}

bool any_event_selected(const set<const Part*>& parts, bool in_range, RelevantSelectedEvents_t relevant)
{
  return !get_events(parts, in_range ? 3 : 1, relevant).empty();
}

bool drummaps_almost_equal(const DrumMap* one, const DrumMap* two, int len)
{
  for (int i=0; i<len; i++)
    if (!one[i].almost_equals(two[i]))
      return false;

  return true;
}


QSet<Part*> parts_at_tick(unsigned tick)
{
  using MusEGlobal::song;
  
  QSet<Track*> tmp;
  for (iTrack it=song->tracks()->begin(); it!=song->tracks()->end(); it++)
    tmp.insert(*it);
  
  return parts_at_tick(tick, tmp);
}

QSet<Part*> parts_at_tick(unsigned tick, Track* track)
{
  QSet<Track*> tmp;
  tmp.insert(track);
  
  return parts_at_tick(tick, tmp);
}

QSet<Part*> parts_at_tick(unsigned tick, const QSet<Track*>& tracks)
{
  QSet<Part*> result;
  
  for (QSet<Track*>::const_iterator it=tracks.begin(); it!=tracks.end(); it++)
  {
    Track* track=*it;
    
    for (iPart p_it=track->parts()->begin(); p_it!=track->parts()->end(); p_it++)
      if (tick >= p_it->second->tick() && tick <= p_it->second->endTick())
        result.insert(p_it->second);
  }
  
  return result;
}

bool parse_range(const QString& str, int* from, int* to)
{
  int idx = str.indexOf("-");
  if (idx<0) // no "-" in str
  {
    bool ok;
    int i = str.toInt(&ok);
    if (!ok)
    {
      *from=-1; *to=-1;
      return false;
    }
    else
    {
      *from=i; *to=i;
      return true;
    }
  }
  else // there is a "-" in str
  {
    QString str1=str.mid(0,idx);
    QString str2=str.mid(idx+1);
    
    bool ok;
    int i = str1.toInt(&ok);
    if (!ok)
    {
      *from=-1; *to=-1;
      return false;
    }
    else
    {
      *from=i;
      
      i = str2.toInt(&ok);
      if (!ok)
      {
        *from=-1; *to=-1;
        return false;
      }
      else
      {
        *to=i;
        return true;
      }
    }
  }
}

void write_new_style_drummap(int level, Xml& xml, const char* tagname,
                             DrumMap* drummap, bool full)
{
  xml.tag(level++, tagname);
  
  for (int i=0;i<128;i++)
  {
    DrumMap* dm = &drummap[i];
    const DrumMap* idm = &iNewDrumMap[i];
  
    if ( (dm->name != idm->name) || (dm->vol != idm->vol) ||
         (dm->quant != idm->quant) || (dm->len != idm->len) ||
         (dm->lv1 != idm->lv1) || (dm->lv2 != idm->lv2) ||
         (dm->lv3 != idm->lv3) || (dm->lv4 != idm->lv4) ||
         (dm->enote != idm->enote) || (dm->mute != idm->mute) ||
         (dm->port != idm->port) || (dm->channel != idm->channel) ||
         (dm->anote != idm->anote) ||
         (dm->hide != idm->hide) || full)
    {
      xml.tag(level++, "entry pitch=\"%d\"", i);
      
      // when any of these "if"s changes, also update the large "if"
      // above (this scope's parent)
      if (full || dm->name != idm->name)   xml.strTag(level, "name", dm->name);
      if (full || dm->vol != idm->vol)     xml.intTag(level, "vol", dm->vol);
      if (full || dm->quant != idm->quant) xml.intTag(level, "quant", dm->quant);
      if (full || dm->len != idm->len)     xml.intTag(level, "len", dm->len);
      if (full || dm->channel != idm->channel) xml.intTag(level, "channel", dm->channel);
      if (full || dm->port != idm->port)   xml.intTag(level, "port", dm->port);
      if (full || dm->lv1 != idm->lv1)     xml.intTag(level, "lv1", dm->lv1);
      if (full || dm->lv2 != idm->lv2)     xml.intTag(level, "lv2", dm->lv2);
      if (full || dm->lv3 != idm->lv3)     xml.intTag(level, "lv3", dm->lv3);
      if (full || dm->lv4 != idm->lv4)     xml.intTag(level, "lv4", dm->lv4);
      if (full || dm->enote != idm->enote) xml.intTag(level, "enote", dm->enote);
      if (full || dm->anote != idm->anote) xml.intTag(level, "anote", dm->anote);
      if (full || dm->mute != idm->mute)   xml.intTag(level, "mute", dm->mute);
      if (full || dm->hide != idm->hide)   xml.intTag(level, "hide", dm->hide);

      xml.tag(--level, "/entry");
    }
  }

  xml.etag(level, tagname);
}

void read_new_style_drummap(Xml& xml, const char* tagname,
                            DrumMap* drummap, bool compatibility)
{
	for (;;)
	{
		Xml::Token token = xml.parse();
		if (token == Xml::Error || token == Xml::End)
			break;
		const QString& tag = xml.s1();
		switch (token)
		{
			case Xml::TagStart:
				if (tag == "entry")  // then read that entry with a nested loop
        {
          DrumMap* dm=NULL;
          DrumMap temporaryMap;
          for (;;) // nested loop
          {
            Xml::Token token = xml.parse();
            const QString& tag = xml.s1();
            switch (token)
            {
              case Xml::Error:
              case Xml::End:
                goto end_of_nested_for;

              case Xml::Attribut:
                if (tag == "pitch")
                {
                  int pitch = xml.s2().toInt() & 0x7f;
                  if (pitch < 0 || pitch > 127)
                    printf("ERROR: THIS SHOULD NEVER HAPPEN: invalid pitch in read_new_style_drummap()!\n");
                  else
                  {
                    dm = &drummap[pitch];
                  }
                }
                break;

              case Xml::TagStart:
                if (dm==NULL && compatibility == false)
                  printf("ERROR: THIS SHOULD NEVER HAPPEN: no valid 'pitch' attribute in <entry> tag, but sub-tags follow in read_new_style_drummap()!\n");
                else if (dm ==NULL && compatibility == true)
                {
                   dm = &temporaryMap;
                }
                if (tag == "name")
                  dm->name = xml.parse(QString("name"));
                else if (tag == "vol")
                  dm->vol = (unsigned char)xml.parseInt();
                else if (tag == "quant")
                  dm->quant = xml.parseInt();
                else if (tag == "len")
                  dm->len = xml.parseInt();
                else if (tag == "channel")
                  dm->channel = xml.parseInt();
                else if (tag == "port")
                  dm->port = xml.parseInt();
                else if (tag == "lv1")
                  dm->lv1 = xml.parseInt();
                else if (tag == "lv2")
                  dm->lv2 = xml.parseInt();
                else if (tag == "lv3")
                  dm->lv3 = xml.parseInt();
                else if (tag == "lv4")
                  dm->lv4 = xml.parseInt();
                else if (tag == "enote") {
                  dm->enote = xml.parseInt();
                  if (compatibility) {
                      int pitch = temporaryMap.enote;
                      drummap[pitch] = temporaryMap;
                      dm = &drummap[pitch];
                      dm->anote = pitch;
                  }
                }
                else if (tag == "anote")
                  dm->anote = xml.parseInt();
                else if (tag == "mute")
                  dm->mute = xml.parseInt();
                else if (tag == "hide")
                  dm->hide = xml.parseInt();
                else
                  xml.unknown("read_new_style_drummap");
                break;

              case Xml::TagEnd:
                if (tag == "entry")
                  goto end_of_nested_for;

              default:
                break;
            }
          } // end of nested loop
          end_of_nested_for: ;
        } // end of 'if (tag == "entry")'
				else
					xml.unknown("read_new_style_drummap");
				break;

			case Xml::TagEnd:
				if (tag == tagname)
					return;

			default:
				break;
		}
	}
}

int readDrummapsEntryPatchCollection(Xml& xml)
{
  int hbank = (CTRL_PROGRAM_VAL_DONT_CARE >> 16) & 0xff;
  int lbank = (CTRL_PROGRAM_VAL_DONT_CARE >> 8) & 0xff;
  int prog  = CTRL_PROGRAM_VAL_DONT_CARE & 0xff;
  int last_prog, last_hbank, last_lbank; // OBSOLETE. Not used.

  for (;;)
  {
    Xml::Token token = xml.parse();
    const QString& tag = xml.s1();
    switch (token)
    {
      case Xml::Error:
      case Xml::End:
        goto read_end;

      case Xml::TagStart:
        xml.unknown("readDrummapsEntryPatchCollection");
        break;

      case Xml::Attribut:
        // last_prog, last_hbank, last_lbank are OBSOLETE. Not used.
        if (tag == "prog")
          parse_range(xml.s2(), &prog, &last_prog);
        else if (tag == "lbank")
          parse_range(xml.s2(), &lbank, &last_lbank);
        else if (tag == "hbank")
          parse_range(xml.s2(), &hbank, &last_hbank);
        break;

      case Xml::TagEnd:
        if (tag == "patch_collection")
          return ((hbank & 0xff) << 16) | ((lbank & 0xff) << 8) | (prog & 0xff);

      default:
        break;
    }
  }

read_end:
  fprintf(stderr, "ERROR: End or Error in readDrummapsEntryPatchCollection()!\n");
  return CTRL_VAL_UNKNOWN; // an invalid collection
}

void record_controller_change_and_maybe_send(unsigned tick, int ctrl_num, int val, MidiTrack* mt)
{
	MusECore::Event a(MusECore::Controller);
	a.setTick(tick);
	a.setA(ctrl_num);
	a.setB(val);
	MusEGlobal::song->recordEvent(mt, a);
	
	if (MusEGlobal::song->cpos() < mt->getControllerValueLifetime(tick, ctrl_num))
	{
		// this CC has an immediate effect? so send it out to the device.
		MusECore::MidiPlayEvent ev(0, mt->outPort(), mt->outChannel(), MusECore::ME_CONTROLLER, ctrl_num, val);
		MusEGlobal::audio->msgPlayMidiEvent(&ev);
	}
}

} // namespace MusECore

namespace MusEGui {

//---------------------------------------------------------
//   midiPortsPopup
//---------------------------------------------------------

QMenu* midiPortsPopup(QWidget* parent, int checkPort, bool includeDefaultEntry)
      {
      QMenu* p = new QMenu(parent);
      QMenu* subp = 0;
      QAction *act = 0;
      QString name;
      const int openConfigId = MusECore::MIDI_PORTS;
      const int defaultId    = MusECore::MIDI_PORTS + 1;
      
      // Warn if no devices available. Add an item to open midi config. 
      int pi = 0;
      for( ; pi < MusECore::MIDI_PORTS; ++pi)
      {
        MusECore::MidiDevice* md = MusEGlobal::midiPorts[pi].device();
        if(md && (md->rwFlags() & 1))   
          break;
      }
      if(pi == MusECore::MIDI_PORTS)
      {
        act = p->addAction(qApp->translate("@default", QT_TRANSLATE_NOOP("@default", "Warning: No output devices!")));
        act->setCheckable(false);
        act->setData(-1);
        p->addSeparator();
      }
      act = p->addAction(QIcon(*MusEGui::settings_midiport_softsynthsIcon), qApp->translate("@default", QT_TRANSLATE_NOOP("@default", "Open midi config...")));
      act->setCheckable(false);
      act->setData(openConfigId);  
      p->addSeparator();
      
      p->addAction(new MusEGui::MenuTitleItem(qApp->translate("@default", QT_TRANSLATE_NOOP("@default", "Output port/device")), p));

      p->addSeparator();
      
      if(includeDefaultEntry)
      {
        act = p->addAction(qApp->translate("@default", QT_TRANSLATE_NOOP("@default", "default")));
        act->setCheckable(false);
        act->setData(defaultId); 
      }

      QVector<int> alsa_list;
      QVector<int> jack_list;
      QVector<int> synth_list;
      QVector<int> *cur_list;
      QVector<int> unused_list;

      for (int i = 0; i < MusECore::MIDI_PORTS; ++i)
      {
        MusECore::MidiPort* port = &MusEGlobal::midiPorts[i];
        MusECore::MidiDevice* md = port->device();
        if(!md)
        {
          unused_list.push_back(i);
          continue;
        }
        
        // Make deleted audio softsynths not show in select dialog
        if(md->isSynti())
        {
            MusECore::AudioTrack *_track = static_cast<MusECore::AudioTrack *>(static_cast<MusECore::SynthI *>(md));
            MusECore::TrackList* tl = MusEGlobal::song->tracks();
            if(tl->find(_track) == tl->end())
              continue;
        }
        
        // Only writeable ports, or current one.
        if(!(md->rwFlags() & 1) && (i != checkPort))
          continue;
        
        switch(md->deviceType())
        {
          case MusECore::MidiDevice::ALSA_MIDI:
            alsa_list.push_back(i);
          break;
          
          case MusECore::MidiDevice::JACK_MIDI:
            jack_list.push_back(i);
          break;

          case MusECore::MidiDevice::SYNTH_MIDI:
            synth_list.push_back(i);
          break;
        }
      }  
      
      // Order the entire listing by device type.
      for(int dtype = 0; dtype <= MusECore::MidiDevice::SYNTH_MIDI; ++dtype)
      {
        switch(dtype)
        {
          case MusECore::MidiDevice::ALSA_MIDI:
            if(!alsa_list.isEmpty())
              p->addAction(new MusEGui::MenuTitleItem(qApp->translate("@default", QT_TRANSLATE_NOOP("@default", "ALSA")), p));
            cur_list = &alsa_list;
          break;
          
          case MusECore::MidiDevice::JACK_MIDI:
            if(!jack_list.isEmpty())
              p->addAction(new MusEGui::MenuTitleItem(qApp->translate("@default", QT_TRANSLATE_NOOP("@default", "JACK")), p));
            cur_list = &jack_list;
          break;

          case MusECore::MidiDevice::SYNTH_MIDI:
            if(!synth_list.isEmpty())
              p->addAction(new MusEGui::MenuTitleItem(qApp->translate("@default", QT_TRANSLATE_NOOP("@default", "Synth")), p));
            cur_list = &synth_list;
          break;
        }
              
        if(cur_list->isEmpty())
          continue;
        
        int row = 0;
        int sz = cur_list->size();
        
        for (int i = 0; i < sz; ++i) 
        {
          const int port = cur_list->at(i);
          if(port < 0 || port >= MusECore::MIDI_PORTS)
            continue;
          MusECore::MidiPort* mp = &MusEGlobal::midiPorts[port];
          name = QString("%1:%2")
              .arg(port + 1)
              .arg(mp->portname());
              
          act = p->addAction(name);
          act->setData(port);
          act->setCheckable(true);
          act->setChecked(port == checkPort);
          
          ++row;
        }  
      }
      
      int sz = unused_list.size();
      if(sz > 0)
      {
        p->addSeparator();
        for (int i = 0; i < sz; ++i)
        {
          const int port = unused_list.at(i);
          // No submenu yet? Create it now.
          if(!subp)
          {
            subp = new QMenu(p);
            subp->setTitle(qApp->translate("@default", QT_TRANSLATE_NOOP("@default", "Empty ports")));
          }
          act = subp->addAction(QString().setNum(port + 1));
          act->setData(port);
          act->setCheckable(true);
          act->setChecked(port == checkPort);
        }
      }
      
      if(subp)
        p->addMenu(subp);
      return p;
      }

//---------------------------------------------------------
//   midiPortsPopupMenu
//---------------------------------------------------------
void midiPortsPopupMenu(MusECore::Track* t, int x, int y, bool allClassPorts, 
                        const QWidget* widget, bool includeDefaultEntry)
{
  switch(t->type()) {
      case MusECore::Track::MIDI:
      case MusECore::Track::DRUM:
      case MusECore::Track::AUDIO_SOFTSYNTH:
      {
            MusECore::MidiTrack* track = 0;
            MusECore::SynthI* synthi = 0;
            
            int potential_new_port_no=-1;
            int port = -1; 
            
            if(t->isSynthTrack())
            {
              // Cast as SynthI which inherits MidiDevice.
              synthi = static_cast<MusECore::SynthI*>(t);
              if(!synthi)
                return;
              port = synthi->midiPort();
            }
            else
            {
              track = static_cast<MusECore::MidiTrack*>(t);
              port = track->outPort();
            }
            
            // NOTE: If parent is given, causes accelerators to be returned in QAction::text() !
            QMenu* p = MusEGui::midiPortsPopup(0, port, includeDefaultEntry);
            
            // find first free port number
            // do not permit numbers already used in other tracks!
            // except if it's only used in this track.
            int no;
            for (no=0;no<MusECore::MIDI_PORTS;no++)
              if (MusEGlobal::midiPorts[no].device()==NULL)
              {
                MusECore::ciMidiTrack it;
                for (it=MusEGlobal::song->midis()->begin(); it!=MusEGlobal::song->midis()->end(); ++it)
                {
                  MusECore::MidiTrack* mt=*it;
                  if (mt!=t && mt->outPort()==no)
                    break;
                }
                if (it == MusEGlobal::song->midis()->end())
                  break;
                
                // TODO Ports which are used by synths ??
              }

            if (no==MusECore::MIDI_PORTS)
            {
              delete p;
              printf("THIS IS VERY UNLIKELY TO HAPPEN: no free midi ports! you have used all %i!\n",MusECore::MIDI_PORTS);
              break;
            }

            potential_new_port_no=no;
            
            // Do not include unused devices if the track is a synth track.
            if(!synthi)
            {
              typedef std::map<std::string, int > asmap;
              typedef std::map<std::string, int >::iterator imap;

              asmap mapALSA;
              asmap mapJACK;
              asmap mapSYNTH;

              int aix = 0x10000000;
              int jix = 0x20000000;
              int six = 0x30000000;
              for(MusECore::iMidiDevice i = MusEGlobal::midiDevices.begin(); i != MusEGlobal::midiDevices.end(); ++i)
              {
                // don't add devices which are used somewhere
                if((*i)->midiPort() >= 0 && (*i)->midiPort() < MusECore::MIDI_PORTS)
                  continue;
                  
                switch((*i)->deviceType())
                {
                  case MusECore::MidiDevice::ALSA_MIDI:
                    mapALSA.insert(std::pair<std::string, int> ((*i)->name().toStdString(), aix));
                    ++aix;
                  break;
                  
                  case MusECore::MidiDevice::JACK_MIDI:
                    mapJACK.insert(std::pair<std::string, int> ((*i)->name().toStdString(), jix));
                    ++jix;
                  break;
                  
                  case MusECore::MidiDevice::SYNTH_MIDI:
                    mapSYNTH.insert(std::pair<std::string, int> ((*i)->name().toStdString(), six));
                    ++six;
                  break;
                }
              }

              if (!mapALSA.empty() || !mapJACK.empty() || !mapSYNTH.empty())
              {
                QMenu* pup = p->addMenu(qApp->translate("@default", QT_TRANSLATE_NOOP("@default", "Unused Devices")));
                QAction* act;

                if (!mapALSA.empty())
                {
                  pup->addAction(new MusEGui::MenuTitleItem("ALSA:", pup));

                  for(imap i = mapALSA.begin(); i != mapALSA.end(); ++i)
                  {
                    int idx = i->second;
                    QString s(i->first.c_str());
                    MusECore::MidiDevice* md = MusEGlobal::midiDevices.find(s, MusECore::MidiDevice::ALSA_MIDI);
                    if(md)
                    {
                      if(md->deviceType() != MusECore::MidiDevice::ALSA_MIDI)
                        continue;

                      act = pup->addAction(md->name());
                      act->setData(idx);
                    }
                  }
                }

                if (!mapJACK.empty())
                {
                  pup->addAction(new MusEGui::MenuTitleItem("JACK:", pup));

                  for(imap i = mapJACK.begin(); i != mapJACK.end(); ++i)
                  {
                    int idx = i->second;
                    QString s(i->first.c_str());
                    MusECore::MidiDevice* md = MusEGlobal::midiDevices.find(s, MusECore::MidiDevice::JACK_MIDI);
                    if(md)
                    {
                      if(md->deviceType() != MusECore::MidiDevice::JACK_MIDI)
                        continue;

                      act = pup->addAction(md->name());
                      act->setData(idx);
                    }
                  }
                }
                
                if (!mapSYNTH.empty())
                {
                  pup->addAction(new MusEGui::MenuTitleItem("Synth:", pup));

                  for(imap i = mapSYNTH.begin(); i != mapSYNTH.end(); ++i)
                  {
                    int idx = i->second;
                    QString s(i->first.c_str());
                    MusECore::MidiDevice* md = MusEGlobal::midiDevices.find(s, MusECore::MidiDevice::SYNTH_MIDI);
                    if(md)
                    {
                      if(md->deviceType() != MusECore::MidiDevice::SYNTH_MIDI)
                        continue;

                      act = pup->addAction(md->name());
                      act->setData(idx);
                    }
                  }
                }
              }
            }
            
            QAction* act = widget ? p->exec(widget->mapToGlobal(QPoint(x, y)), 0) : p->exec();
            if(!act) 
            {
              delete p;
              break;
            }  
            
            QString acttext=act->text();
            int n = act->data().toInt();
            delete p;
                  
            if(n < 0)              // Invalid item.
              break;
            
            if(n == MusECore::MIDI_PORTS)    // Show port config dialog.
            {
              MusEGlobal::muse->configMidiPorts();
              break;
            }
            else if (n >= 0x10000000)
            {
              // Error, should not happen.
              if(synthi)
                break;
            
              int typ;
              if (n < 0x20000000)
                typ = MusECore::MidiDevice::ALSA_MIDI;
              else if (n < 0x30000000)
                typ = MusECore::MidiDevice::JACK_MIDI;
              else
                typ = MusECore::MidiDevice::SYNTH_MIDI;

              MusECore::MidiDevice* sdev = MusEGlobal::midiDevices.find(acttext, typ);

              MusEGlobal::audio->msgSetMidiDevice(&MusEGlobal::midiPorts[potential_new_port_no], sdev);
              n=potential_new_port_no;
              
              MusEGlobal::song->update();
            }

            MusECore::MidiTrack::ChangedType_t changed = MusECore::MidiTrack::NothingChanged;
            MusEGlobal::audio->msgIdle(true);
            
            // In the case of synths, multiple synths cannot be assigned to the same port.
            if(synthi || (!allClassPorts && !t->selected()))
            {
              if(synthi)
              {
                MusEGlobal::audio->msgSetMidiDevice(&MusEGlobal::midiPorts[n], synthi);
                MusEGlobal::song->update();
                changed |= MusECore::MidiTrack::PortChanged;
              }
              else if(track)
              {
                if(n != track->outPort())
                  changed |= track->setOutPortAndUpdate(n, false);
              }
            }
            else
            {
              if(track)
              {
                MusECore::MidiTrackList* tracks = MusEGlobal::song->midis();
                for(MusECore::iMidiTrack myt = tracks->begin(); myt != tracks->end(); ++myt) 
                {
                  MusECore::MidiTrack* mt = *myt;
                  if(n != mt->outPort() && (allClassPorts || mt->selected()))
                    changed |= mt->setOutPortAndUpdate(n, false);
                }
              }
            }
            
            MusEGlobal::audio->msgIdle(false);
            MusEGlobal::audio->msgUpdateSoloStates();
            MusEGlobal::song->update(SC_ROUTE | ((changed & MusECore::MidiTrack::DrumMapChanged) ? SC_DRUMMAP : 0));
            
            // Prompt and send init sequences.
            MusEGlobal::audio->msgInitMidiDevices(false);
      }
      break;
            
      case MusECore::Track::WAVE:
      case MusECore::Track::AUDIO_OUTPUT:
      case MusECore::Track::AUDIO_INPUT:
      case MusECore::Track::AUDIO_GROUP:
      case MusECore::Track::AUDIO_AUX:    //TODO
            break;
      }
}

//---------------------------------------------------------
//   populateAddSynth
//---------------------------------------------------------

QMenu* populateAddSynth(QWidget* parent)
{
  QMenu* synp = new PopupMenu(parent);
  
  typedef std::multimap<std::string, int > asmap;
  typedef std::multimap<std::string, int >::iterator imap;
  
  
  const int ntypes = MusECore::Synth::SYNTH_TYPE_END;
  asmap smaps[ntypes];
  PopupMenu* mmaps[ntypes];
  for(int itype = 0; itype < ntypes; ++itype)
    mmaps[itype] = 0;
  
  MusECore::Synth* synth;
  MusECore::Synth::Type type;
  
  int ii = 0;
  for(std::vector<MusECore::Synth*>::iterator i = MusEGlobal::synthis.begin(); i != MusEGlobal::synthis.end(); ++i)
  {
    synth = *i;
    type = synth->synthType();

// dssi-vst is dead, really no point in keeping this case around
//#ifdef DSSI_SUPPORT
//    if (type == MusECore::Synth::DSSI_SYNTH && ((MusECore::DssiSynth*)synth)->isDssiVst() ) // Place Wine VSTs in a separate sub menu
//      type = MusECore::Synth::VST_SYNTH;
//#endif

    if(type >= ntypes)
      continue; 
    smaps[type].insert( std::pair<std::string, int> (synth->description().toLower().toStdString(), ii) );
  
    ++ii;
  }
  
  int sz = MusEGlobal::synthis.size();
  for(int itype = 0; itype < ntypes; ++itype)
  {  
    for(imap i = smaps[itype].begin(); i != smaps[itype].end(); ++i) 
    {
      int idx = i->second;
      if(idx > sz)           // Sanity check
        continue;
      synth = MusEGlobal::synthis[idx];
      if(synth)
      {
        // No sub-menu yet? Create it now.
        if(!mmaps[itype])
        {  
          mmaps[itype] = new PopupMenu(parent);
          mmaps[itype]->setToolTipsVisible(true);
          mmaps[itype]->setIcon(*synthIcon);
          mmaps[itype]->setTitle(MusECore::synthType2String((MusECore::Synth::Type)itype));
          synp->addMenu(mmaps[itype]);
        }  
        //QAction* act = mmaps[itype]->addAction(synth->description() + " <" + synth->name() + ">");
        QAction* act = mmaps[itype]->addAction(synth->description());
        act->setData( MENU_ADD_SYNTH_ID_BASE * (itype + 1) + idx );
        if(!synth->uri().isEmpty())
          act->setToolTip(synth->uri());
      }  
    }
  }

  return synp;
}

//---------------------------------------------------------
//   populateAddTrack
//    this is also used in "mixer"
//---------------------------------------------------------

QActionGroup* populateAddTrack(QMenu* addTrack, bool populateAll, bool /*evenIgnoreDrumPreference*/)
      {
      QActionGroup* grp = new QActionGroup(addTrack);
      if (MusEGlobal::config.addHiddenTracks)
        populateAll=true;

      if (populateAll || MusECore::MidiTrack::visible()) {
        QAction* midi = addTrack->addAction(QIcon(*addtrack_addmiditrackIcon),
                                          qApp->translate("@default", QT_TRANSLATE_NOOP("@default", "Add Midi Track")));
        midi->setData(MusECore::Track::MIDI);
        grp->addAction(midi);

        QAction* newdrum = addTrack->addAction(QIcon(*addtrack_newDrumtrackIcon),
                                          qApp->translate("@default", QT_TRANSLATE_NOOP("@default", "Add Drum Track")));
        newdrum->setData(MusECore::Track::DRUM);
        grp->addAction(newdrum);
      }
      if (populateAll || MusECore::WaveTrack::visible()) {
        QAction* wave = addTrack->addAction(QIcon(*addtrack_wavetrackIcon),
                                          qApp->translate("@default", QT_TRANSLATE_NOOP("@default", "Add Wave Track")));
       wave->setData(MusECore::Track::WAVE);
       grp->addAction(wave);
      }

      if (populateAll || MusECore::AudioOutput::visible()) {
        QAction* aoutput = addTrack->addAction(QIcon(*addtrack_audiooutputIcon),
                                               qApp->translate("@default", QT_TRANSLATE_NOOP("@default", "Add Audio Output")));
        aoutput->setData(MusECore::Track::AUDIO_OUTPUT);
        grp->addAction(aoutput);
      }

      if (populateAll || MusECore::AudioGroup::visible()) {
        QAction* agroup = addTrack->addAction(QIcon(*addtrack_audiogroupIcon),
                                              qApp->translate("@default", QT_TRANSLATE_NOOP("@default", "Add Audio Group")));
        agroup->setData(MusECore::Track::AUDIO_GROUP);
        grp->addAction(agroup);
      }

      if (populateAll || MusECore::AudioInput::visible()) {
        QAction* ainput = addTrack->addAction(QIcon(*addtrack_audioinputIcon),
                                              qApp->translate("@default", QT_TRANSLATE_NOOP("@default", "Add Audio Input")));
        ainput->setData(MusECore::Track::AUDIO_INPUT);
        grp->addAction(ainput);
      }

      if (populateAll || MusECore::AudioAux::visible()) {
        QAction* aaux = addTrack->addAction(QIcon(*addtrack_auxsendIcon),
                                            qApp->translate("@default", QT_TRANSLATE_NOOP("@default", "Add Aux Send")));
        aaux->setData(MusECore::Track::AUDIO_AUX);
        grp->addAction(aaux);
      }

      if (populateAll || MusECore::SynthI::visible()) {
        // Create a sub-menu and fill it with found synth types. Make addTrack the owner.
        QMenu* synp = populateAddSynth(addTrack);
        synp->setIcon(*synthIcon);
        synp->setTitle(qApp->translate("@default", QT_TRANSLATE_NOOP("@default", "Add Synth")));

        // Add the sub-menu to the given menu.
        addTrack->addMenu(synp);
      }

      return grp;
      }
  
//---------------------------------------------------------
//   getFilterExtension
//---------------------------------------------------------

QString getFilterExtension(const QString &filter)
{
  // Return the first extension found. Must contain at least one * character.
  
  int pos = filter.indexOf('*');
  if(pos == -1)
    return QString(); 
  
  QString filt;
  int len = filter.length();
  ++pos;
  for( ; pos < len; ++pos)
  {
    QChar c = filter[pos];
    if((c == ')') || (c == ';') || (c == ',') || (c == ' '))
      break; 
    filt += filter[pos];
  }
  return filt;
}

QStringList localizedStringListFromCharArray(const char** array, const char* context)
{
  QStringList temp;
  for (int i=0;array[i];i++)
    temp << qApp->translate(context, array[i]);
  
  return temp;
}

QString browseProjectFolder(QWidget* parent)
{
  QString path;
  if(!MusEGlobal::config.projectBaseFolder.isEmpty())
  {  
    QDir d(MusEGlobal::config.projectBaseFolder);
    path = d.absolutePath();
  }
  
  QString dir = QFileDialog::getExistingDirectory(parent, qApp->translate("@default", QT_TRANSLATE_NOOP("@default", "Select project directory")), path);
  if(dir.isEmpty())
    dir = MusEGlobal::config.projectBaseFolder;
  return dir;
}

QString projectTitleFromFilename(QString filename)
{
  int idx;
  idx = filename.lastIndexOf(".med.bz2", -1, Qt::CaseInsensitive);
  if(idx == -1)
    idx = filename.lastIndexOf(".med.gz", -1, Qt::CaseInsensitive);
  if(idx == -1)
    idx = filename.lastIndexOf(".med", -1, Qt::CaseInsensitive);
   
  if(idx != -1)
    filename.truncate(idx);
  
  QFileInfo fi(filename);

  return fi.fileName();
}

QString projectPathFromFilename(QString filename)
{
  QFileInfo fi(filename);
  return QDir::cleanPath(fi.absolutePath());
}

QString projectExtensionFromFilename(QString filename)
{
  int idx;
  idx = filename.lastIndexOf(".med.bz2", -1, Qt::CaseInsensitive);
  if(idx == -1)
    idx = filename.lastIndexOf(".med.gz", -1, Qt::CaseInsensitive);
  if(idx == -1)
    idx = filename.lastIndexOf(".med", -1, Qt::CaseInsensitive);
  if(idx == -1)
    idx = filename.lastIndexOf(".bz2", -1, Qt::CaseInsensitive);
  if(idx == -1)
    idx = filename.lastIndexOf(".gz", -1, Qt::CaseInsensitive);
   
  return (idx == -1) ? QString() : filename.right(filename.size() - idx);
}

QString getUniqueUntitledName()
{
  QString filename("untitled");
  
  QString fbase(MusEGlobal::config.projectBaseFolder);
  
  QString nfb = fbase;
  if(MusEGlobal::config.projectStoreInFolder) 
    nfb += "/" + filename;
  QFileInfo fi(nfb + "/" + filename + ".med");  // TODO p4.0.40 Check other extensions.
  if(!fi.exists())
    return fi.filePath();

  // Find a new filename
  QString nfn = filename;  
  int idx;
  for (idx=2; idx<10000; idx++) {
      QString num = QString::number(idx);
      nfn = filename + "_" + num;
      nfb = fbase;
      if(MusEGlobal::config.projectStoreInFolder) 
        nfb += "/" + nfn;
      QFileInfo fi(nfb + "/" + nfn + ".med");
      if(!fi.exists())
        return fi.filePath();
  }    

  printf("MusE error: Could not make untitled project name (10000 or more untitled projects in project dir - clean up!\n");
    
  nfb = fbase;
  if(MusEGlobal::config.projectStoreInFolder) 
    nfb += "/" + filename;
  return nfb + "/" + filename + ".med";
}

struct CI {
            int num;
            QString s;
            bool used;
            bool off;
            bool instrument;
            CI(int n, const QString& ss, bool u, bool o, bool i) : num(n), s(ss), used(u), off(o), instrument(i) {}
            };

//---------------------------------------------------
//  populateMidiCtrlMenu
//  Returns estimated width of the completed menu.            
//---------------------------------------------------

int populateMidiCtrlMenu(PopupMenu* menu, MusECore::PartList* part_list, MusECore::Part* cur_part, int curDrumPitch)
      {
      //---------------------------------------------------
      // build list of midi controllers for current
      // MusECore::MidiPort/channel
      //---------------------------------------------------

      MusECore::MidiTrack* track = (MusECore::MidiTrack*)(cur_part->track());
      int channel      = track->outChannel();
      MusECore::MidiPort* port   = &MusEGlobal::midiPorts[track->outPort()];
      bool isNewDrum   = track->type() == MusECore::Track::DRUM;
      bool isMidi      = track->type() == MusECore::Track::MIDI;
      MusECore::MidiInstrument* instr = port->instrument();
      MusECore::MidiCtrlValListList* cll = port->controller();
      const int min = channel << 24;
      const int max = min + 0x1000000;
      const int edit_ins = max + 3;
      const int velo = max + 0x101;
      int est_width = 0;  
      const int patch = port->hwCtrlState(channel, MusECore::CTRL_PROGRAM);
      
      std::list<CI> sList;
      typedef std::list<CI>::iterator isList;
      std::set<int> already_added_nums;

      for (MusECore::iMidiCtrlValList it = cll->lower_bound(min); it != cll->lower_bound(max); ++it) {
            MusECore::MidiCtrlValList* cl = it->second;
            MusECore::MidiController* c   = port->midiController(cl->num(), channel);
            bool isDrumCtrl = (c->isPerNoteController());
            int show = c->showInTracks();
            int cnum = c->num();
            int num = cl->num();
            if (isDrumCtrl) {
                  // Only show controller for current pitch:
                  if (isNewDrum)
                  {
                    if ((curDrumPitch < 0) || ((num & 0xff) != track->drummap()[curDrumPitch].anote))
                          continue;

                  }
                  else if (isMidi)
                  {
                    if ((curDrumPitch < 0) || ((num & 0xff) != curDrumPitch)) // FINDMICH does this work?
                          continue;
                  }
                  else
                    continue;
                  }
            isList i = sList.begin();
            for (; i != sList.end(); ++i) {
                  if (i->num == num)
                        break;
                  }
                  
            if (i == sList.end()) {
                  bool used = false;
                  for (MusECore::iPart ip = part_list->begin(); ip != part_list->end(); ++ip) {
                        const MusECore::EventList& el = ip->second->events();
                        for (MusECore::ciEvent ie = el.begin(); ie != el.end(); ++ie) {
                              const MusECore::Event& e = ie->second;
                              if(e.type() != MusECore::Controller)
                                continue;
                              int ctl_num = e.dataA();
                              // Is it a drum controller event, according to the track port's instrument?
                              MusECore::MidiController *mc = port->drumController(ctl_num);
                              if(mc)
                              {
                                if((ctl_num & 0xff) != curDrumPitch)
                                  continue;
                                if(isNewDrum)
                                  ctl_num = (ctl_num & ~0xff) | track->drummap()[ctl_num & 0x7f].anote;
                              }
                              if(ctl_num == num)
                              {
                                    used = true;
                                    break;
                              }
                                    
                              }
                        if (used)
                              break;
                        }
                  bool off = cl->hwVal() == MusECore::CTRL_VAL_UNKNOWN;  // Does it have a value or is it 'off'?
                  // Filter if not used and off. But if there's something there, we must show it.
                  if(!used && off &&
                     (((isDrumCtrl || isNewDrum) && !(show & MusECore::MidiController::ShowInDrum)) ||
                     (isMidi && !(show & MusECore::MidiController::ShowInMidi))))
                    continue;
                  const bool isinstr = instr->findController(cnum, channel, patch) != nullptr;
                  // Need to distinguish between global default controllers and 
                  //  instrument defined controllers. Instrument takes priority over global
                  //  ie they 'overtake' definition of a global controller such that the
                  //  global def is no longer available.
                  sList.push_back(CI(num, 
                                  isinstr ? MusECore::midiCtrlNumString(cnum, true) + c->name() : MusECore::midiCtrlName(cnum, true), 
                                  used, off, isinstr));
                  already_added_nums.insert(num); 
                  }
            }
      
      QString stext = QWidget::tr("Instrument-defined");
// Width() is obsolete. Qt >= 5.11 use horizontalAdvance().
#if QT_VERSION >= 0x050b00
      int fmw = menu->fontMetrics().horizontalAdvance(stext);
#else
      int fmw = menu->fontMetrics().width(stext);
#endif
      if(fmw > est_width)
        est_width = fmw;
      menu->addAction(new MenuTitleItem(stext, menu));
      
      // Don't allow editing instrument if it's a synth
      if(!port->device() || port->device()->deviceType() != MusECore::MidiDevice::SYNTH_MIDI)
      {
        stext = QWidget::tr("Edit instrument ...");
// Width() is obsolete. Qt >= 5.11 use horizontalAdvance().
#if QT_VERSION >= 0x050b00
        fmw = menu->fontMetrics().horizontalAdvance(stext);
#else
        fmw = menu->fontMetrics().width(stext);
#endif
        if(fmw > est_width)
          est_width = fmw;
        menu->addAction(QIcon(*midi_edit_instrumentIcon), QWidget::tr("Edit instrument ..."))->setData(edit_ins);
        menu->addSeparator();
      }
      
      //
      // populate popup with all controllers available for
      // current instrument
      //

      stext = QWidget::tr("Add");
// Width() is obsolete. Qt >= 5.11 use horizontalAdvance().
#if QT_VERSION >= 0x050b00
      fmw = menu->fontMetrics().horizontalAdvance(stext);
#else
      fmw = menu->fontMetrics().width(stext);
#endif
      if(fmw > est_width)
        est_width = fmw;
      PopupMenu * ctrlSubPop = new PopupMenu(stext, menu, true);  // true = enable stay open
      MusECore::MidiControllerList* mcl = new MusECore::MidiControllerList();
      instr->getControllers(mcl, channel, patch);

      for (MusECore::iMidiController ci = mcl->begin(); ci != mcl->end(); ++ci)
      {
          int show = ci->second->showInTracks();
          if((isNewDrum && !(show & MusECore::MidiController::ShowInDrum)) ||
             (isMidi && !(show & MusECore::MidiController::ShowInMidi)))
            continue;
          int cnum = ci->second->num();
          int num = cnum;
          if(ci->second->isPerNoteController())
          {
            if (isNewDrum && curDrumPitch >= 0)
              num = (cnum & ~0xff) | track->drummap()[curDrumPitch].anote;
            else if (isMidi && curDrumPitch >= 0)
              num = (cnum & ~0xff) | curDrumPitch; //FINDMICH does this work?
            else
              continue;
          }

          // If it's not already in the parent menu...
          if(cll->find(channel, num) == cll->end())
          {
            ctrlSubPop->addAction(MusECore::midiCtrlNumString(cnum, true) + ci->second->name())->setData(num); 
            already_added_nums.insert(num); //cnum);
          }
      }
      delete mcl;

      menu->addMenu(ctrlSubPop);

      menu->addSeparator();
      
      // Add instrument-defined controllers:
      for (isList i = sList.begin(); i != sList.end(); ++i)
      {
        if(!i->instrument)
          continue;

// Width() is obsolete. Qt >= 5.11 use horizontalAdvance().
#if QT_VERSION >= 0x050b00
        fmw = menu->fontMetrics().horizontalAdvance(i->s);
#else
        fmw = menu->fontMetrics().width(i->s);
#endif
        if(fmw > est_width)
          est_width = fmw;

        if (i->used && !i->off)
          menu->addAction(QIcon(*orangedotIcon), i->s)->setData(i->num);
        else if (i->used)
          menu->addAction(QIcon(*greendotIcon), i->s)->setData(i->num);
        else if(!i->off)
          menu->addAction(QIcon(*bluedotIcon), i->s)->setData(i->num);
        else
          menu->addAction(i->s)->setData(i->num);
      }

      stext = QWidget::tr("Others");
// Width() is obsolete. Qt >= 5.11 use horizontalAdvance().
#if QT_VERSION >= 0x050b00
      fmw = menu->fontMetrics().horizontalAdvance(stext);
#else
      fmw = menu->fontMetrics().width(stext);
#endif
      if(fmw > est_width)
        est_width = fmw;
      menu->addAction(new MenuTitleItem(stext, menu));

      // Add a.k.a. Common Controls not found in instrument:
      stext = QWidget::tr("Common Controls");
// Width() is obsolete. Qt >= 5.11 use horizontalAdvance().
#if QT_VERSION >= 0x050b00
      fmw = menu->fontMetrics().horizontalAdvance(stext);
#else
      fmw = menu->fontMetrics().width(stext);
#endif
      if(fmw > est_width)
        est_width = fmw;
      PopupMenu* ccSubPop = new PopupMenu(stext, menu, true);  // true = enable stay open
      for(int num = 0; num < 128; ++num)
        // If it's not already in the parent menu...
        if(already_added_nums.find(num) == already_added_nums.end())
          ccSubPop->addAction(MusECore::midiCtrlName(num, true))->setData(num);

      menu->addMenu(ccSubPop);

      menu->addSeparator();
      
      // Add the special case velocity:
      stext = QWidget::tr("Velocity");
// Width() is obsolete. Qt >= 5.11 use horizontalAdvance().
#if QT_VERSION >= 0x050b00
      fmw = menu->fontMetrics().horizontalAdvance(stext);
#else
      fmw = menu->fontMetrics().width(stext);
#endif
      if(fmw > est_width)
        est_width = fmw;
      menu->addAction(stext)->setData(velo);
      
      // Add global default controllers (all controllers not found in instrument).
      for (isList i = sList.begin(); i != sList.end(); ++i) 
      {
        if(i->instrument)
          continue;

// Width() is obsolete. Qt >= 5.11 use horizontalAdvance().
#if QT_VERSION >= 0x050b00
        fmw = menu->fontMetrics().horizontalAdvance(i->s);
#else
        fmw = menu->fontMetrics().width(i->s);
#endif
        if(fmw > est_width)
          est_width = fmw;
        
        if (i->used && !i->off)
          menu->addAction(QIcon(*orangedotIcon), i->s)->setData(i->num);
        else if (i->used)
          menu->addAction(QIcon(*greendotIcon), i->s)->setData(i->num);
        else if(!i->off)
          menu->addAction(QIcon(*bluedotIcon), i->s)->setData(i->num);
        else
          menu->addAction(i->s)->setData(i->num);
      }
      
      est_width += 60; // Add about 60 for the coloured lights on the left.
      
      return est_width;
      }

//---------------------------------------------------
//  clipQLine
//---------------------------------------------------

QLine clipQLine(int x1, int y1, int x2, int y2, const QRect& rect)
{
  const int rect_x     = rect.x();
  const int rect_y     = rect.y();
  const int rect_right = rect_x + rect.width();
  const int rect_bot   = rect_y + rect.height();
  
  if(x1 < rect_x)
  {
    if(x2 < rect_x)
      return QLine();
    x1 = rect_x;
  }
  else
  if(x1 > rect_right)
  {
    if(x2 > rect_right)
      return QLine();
    x1 = rect_right;
  }
  
  if(x2 < rect_x)
    x2 = rect_x;
  else
  if(x2 > rect_right)
    x2 = rect_right;
  
  if(y1 < rect_y)
  {
    if(y2 < rect_y)
      return QLine();
    y1 = rect_y;
  }
  else
  if(y1 > rect_bot)
  {
    if(y2 > rect_bot)
      return QLine();
    y1 = rect_bot;
  }
  
  if(y2 < rect_y)
    y2 = rect_y;
  if(y2 > rect_bot)
    y2 = rect_bot;

  return QLine(x1, y1, x2, y2);
}

QRect normalizeQRect(const QRect& rect)
{
  int x = rect.x();
  int y = rect.y();
  int w = rect.width();
  int h = rect.height();
  if(w < 0)
  {
    x += w;
    w = -w;
  }
  
  if(h < 0)
  {
    y += h;
    h = -h;
  }
  
  return QRect(x, y, w, h);
}

//---------------------------------------------------------
//   loadQtStyle
//---------------------------------------------------------

//void loadQtStyle(const QString& style)
//{
//    // Style sheets take priority over styles, and actually
//    //  reset the style object name to empty when set.
//    const QString curStyle(qApp->style()->objectName());
//    QStringList styleList = QStyleFactory::keys();

//    if (styleList.indexOf(style) == -1) {

//        if (MusEGlobal::debugMsg)
//            printf("Application Qt style does not exist, setting default.\n");

//        // To find the name of the current style, use objectName().
//        if (curStyle.compare(Appearance::getSetDefaultStyle(), Qt::CaseInsensitive) != 0)
//        {
//            qApp->setStyle(Appearance::getSetDefaultStyle());

//            if (MusEGlobal::debugMsg)
//            {
//                fprintf(stderr, "loadQtStyle: Setting app style to default: %s\n", Appearance::getSetDefaultStyle().toLatin1().constData());
//                fprintf(stderr, "   App style is now: %s\n", qApp->style()->objectName().toLatin1().constData());
//            }

//            // No style object name? It will happen when a stylesheet is active.
//            // Give it a name. NOTE: The object names always seem to be lower case while
//            //  the style factory key names are not.
//            if (qApp->style()->objectName().isEmpty())
//            {
//                qApp->style()->setObjectName(Appearance::getSetDefaultStyle().toLower());
//                if (MusEGlobal::debugMsg)
//                    fprintf(stderr, "   Setting empty style object name. App style is now: %s\n", qApp->style()->objectName().toLatin1().constData());
//            }
//        }
//    }
//    else if (curStyle.compare(style, Qt::CaseInsensitive) != 0)
//    {
//        qApp->setStyle(style);
//        // Do the style again to fix a bug where the arranger is non-responsive.

//        if(MusEGlobal::debugMsg)
//        {
//            fprintf(stderr, "loadTheme setting app style to: %s\n", style.toLatin1().constData());
//            fprintf(stderr, "   app style is now: %s\n", qApp->style()->objectName().toLatin1().constData());
//        }

//        // No style object name? It will happen when a stylesheet is active.
//        // Give it a name. NOTE: The object names always seem to be lower case while
//        //  the style factory key names are not.
//        if(qApp->style()->objectName().isEmpty())
//        {
//            qApp->style()->setObjectName(style.toLower());
//            if(MusEGlobal::debugMsg)
//                fprintf(stderr, "   Setting empty style object name. App style is now: %s\n", qApp->style()->objectName().toLatin1().constData());
//        }
//    }
//}

//---------------------------------------------------------
//   loadTheme
//---------------------------------------------------------

void loadTheme(const QString& theme)
{
    if(MusEGlobal::debugMsg)
        fprintf(stderr, "loadTheme:%s\n", theme.toLatin1().constData());

    QString stylePathUser = MusEGlobal::configPath + "/themes/" + theme + ".qss";
    QString stylePathDef = MusEGlobal::museGlobalShare + "/themes/" + theme + ".qss";

    QFile fdef(stylePathDef);
    if (!fdef.open(QIODevice::ReadOnly)) {
        printf("loading style sheet <%s> failed\n", qPrintable(theme));
        return;
    }
    QByteArray sdef = fdef.readAll();
    fdef.close();

    QByteArray suser;
    if (QFile::exists(stylePathUser)) {
        QFile fuser(stylePathUser);
        if (fuser.open(QIODevice::ReadOnly)) {
            suser = fuser.readAll();
        } else {
            printf("loading style sheet <%s> failed\n", qPrintable(theme));
        }
        fuser.close();
    }

    QString sheet;
    if (suser.isEmpty()) {
        sheet = QString::fromUtf8(sdef.data());
    } else {
        if (MusEGlobal::config.cascadeStylesheets)
            sheet = QString::fromUtf8(sdef.data()) + '\n' + QString::fromUtf8(suser.data());
        else
            sheet = QString::fromUtf8(suser.data());
    }

    qApp->setStyleSheet(sheet);
}

//---------------------------------------------------------
//   updateThemeAndStyle
//    Call when the theme or stylesheet part of the configuration has changed,
//     to actually switch them.
//---------------------------------------------------------

//void updateThemeAndStyle()
//{
//  // Note that setting a stylesheet completely takes over the font until blanked again.
////  qApp->setFont(MusEGlobal::config.fonts[0]); // has no effect
////    loadQtStyle(MusEGlobal::config.style);
//  loadTheme(MusEGlobal::config.theme);
//}


} // namespace MusEGui

