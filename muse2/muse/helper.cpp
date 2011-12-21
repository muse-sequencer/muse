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
#include "part.h"
#include "track.h"
#include "song.h"
#include "app.h"
#include "icons.h"
#include "synth.h"
#include "functions.h"
#include "gconfig.h"

#include "driver/jackmidi.h"
#include "route.h"
#include "mididev.h"
#include "globaldefs.h"
#include "audio.h"
#include "audiodev.h"
#include "midiseq.h"

#include <QApplication>
#include <QDir>
#include <QFileInfo>
#include <QFileDialog>
#include <QString>
//#include <QTemporaryFile>

#ifdef DSSI_SUPPORT
#include "dssihost.h"
#endif

#ifdef VST_SUPPORT
#include "vst.h"
#endif

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
      QString o;
      o.sprintf("%d", octave);
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

bool any_event_selected(const set<Part*>& parts, bool in_range)
{
  return !get_events(parts, in_range ? 3 : 1).empty();
}

bool drummaps_almost_equal(DrumMap* one, DrumMap* two, int len)
{
  for (int i=0; i<len; i++)
  {
    DrumMap tmp = one[i];
    tmp.mute=two[i].mute;
    if (tmp!=two[i])
      return false;
  }
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


} // namespace MusECore

namespace MusEGui {

//---------------------------------------------------------
//   populateAddSynth
//---------------------------------------------------------

QMenu* populateAddSynth(QWidget* parent)
{
  QMenu* synp = new QMenu(parent);
  
  //typedef std::multimap<std::string, int, addSynth_cmp_str > asmap;
  typedef std::multimap<std::string, int > asmap;
  
  //typedef std::multimap<std::string, int, addSynth_cmp_str >::iterator imap;
  typedef std::multimap<std::string, int >::iterator imap;
  
  
  int ntypes = MusECore::Synth::SYNTH_TYPE_END;
  asmap smaps[ntypes];
  QMenu* mmaps[ntypes];
  for(int itype = 0; itype < ntypes; ++itype)
    mmaps[itype] = 0;
  
  MusECore::Synth* synth;
  MusECore::Synth::Type type;
  
  int ii = 0;
  for(std::vector<MusECore::Synth*>::iterator i = MusEGlobal::synthis.begin(); i != MusEGlobal::synthis.end(); ++i) 
  {
    synth = *i;
    type = synth->synthType();
    if(type >= ntypes)
      continue; 
    smaps[type].insert( std::pair<std::string, int> (std::string(synth->description().toLower().toLatin1().constData()), ii) );
  
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
          mmaps[itype] = new QMenu(parent);
          mmaps[itype]->setIcon(*synthIcon);
          mmaps[itype]->setTitle(MusECore::synthType2String((MusECore::Synth::Type)itype));
          synp->addMenu(mmaps[itype]);
        }  
        QAction* act = mmaps[itype]->addAction(synth->description() + " <" + synth->name() + ">");
        act->setData( MENU_ADD_SYNTH_ID_BASE * (itype + 1) + idx );
      }  
    }
  }

  return synp;
}

//---------------------------------------------------------
//   populateAddTrack
//    this is also used in "mixer"
//---------------------------------------------------------

QActionGroup* populateAddTrack(QMenu* addTrack, bool populateAll)
      {
      QActionGroup* grp = new QActionGroup(addTrack);
      if (MusEGlobal::config.addHiddenTracks)
        populateAll=true;

      if (populateAll || MusECore::MidiTrack::visible()) {
        QAction* midi = addTrack->addAction(QIcon(*addtrack_addmiditrackIcon),
                                          qApp->translate("@default", QT_TRANSLATE_NOOP("@default", "Add Midi Track")));
        midi->setData(MusECore::Track::MIDI);
        grp->addAction(midi);
      }
      if (populateAll || MusECore::MidiTrack::visible()) {
        QAction* drum = addTrack->addAction(QIcon(*addtrack_drumtrackIcon),
                                          qApp->translate("@default", QT_TRANSLATE_NOOP("@default", "Add Drum Track")));
        drum->setData(MusECore::Track::DRUM);
        grp->addAction(drum);
      }
      if (populateAll || MusECore::MidiTrack::visible()) {
        QAction* newdrum = addTrack->addAction(QIcon(*addtrack_drumtrackIcon),
                                          qApp->translate("@default", QT_TRANSLATE_NOOP("@default", "Add New Style Drum Track")));
        newdrum->setData(MusECore::Track::NEW_DRUM);
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

      //QObject::connect(addTrack, SIGNAL(triggered(QAction *)), MusEGlobal::song, SLOT(addNewTrack(QAction *)));

      return grp;
      }
  
//---------------------------------------------------------
//   getFilterExtension
//---------------------------------------------------------

QString getFilterExtension(const QString &filter)
{
  //
  // Return the first extension found. Must contain at least one * character.
  //
  
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
  
  QString dir = QFileDialog::getExistingDirectory(parent, qApp->tr("Select project directory"), path);
  if(dir.isEmpty())
    dir = MusEGlobal::config.projectBaseFolder;
  //  projDirLineEdit->setText(dir);
  //return QFileDialog::getExistingDirectory(this, qApp.tr("Select project directory"), path);
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

  //return fi.baseName();
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
  //QTemporaryFile tf(MusEGlobal::config.projectBaseFolder +"/" + s + "XXXXXX.med");
  //if(tf.open())
  //  s = MusEGui::projectTitleFromFilename(tf.fileName());
  
  QString fbase(MusEGlobal::config.projectBaseFolder);
  
  QString nfb = fbase;
  if(MusEGlobal::config.projectStoreInFolder) 
    nfb += "/" + filename;
  QFileInfo fi(nfb + "/" + filename + ".med");  // TODO p4.0.40 Check other extensions.
  if(!fi.exists())
    //return filename;
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
        //break;
        return fi.filePath();
  }    

  //if(idx >= 10000)
    printf("MusE error: Could not make untitled project name (10000 or more untitled projects in project dir - clean up!\n");
  
  //return nfn;
    
  nfb = fbase;
  if(MusEGlobal::config.projectStoreInFolder) 
    nfb += "/" + filename;
  return nfb + "/" + filename + ".med";
}


#if 1

// -------------------------------------------------------------------------------------------------------
// populateMidiPorts()
// This version creats separate devices for input and output ports. 
// It does not attempt to pair them together.
// -------------------------------------------------------------------------------------------------------
void populateMidiPorts()
{
  if(!MusEGlobal::checkAudioDevice())
    return;

  MusECore::MidiDevice* dev = 0;
  
  int port_num = 0;
  
  // If Jack is running, prefer Jack midi devices over ALSA.
  if(MusEGlobal::audioDevice->deviceType() == MusECore::AudioDevice::JACK_AUDIO)  
  {
    std::list<QString> sl;
    sl = MusEGlobal::audioDevice->inputPorts(true, 1);  // Ask for second aliases.
    for(std::list<QString>::iterator i = sl.begin(); i != sl.end(); ++i)
    {
      dev = MusECore::MidiJackDevice::createJackMidiDevice(*i, 1); 
      if(dev)
      {
        //printf("populateMidiPorts Created jack writeable device: %s\n", dev->name().toLatin1().constData()); 
        //dev->setOpenFlags(1);
        MusEGlobal::midiSeq->msgSetMidiDevice(&MusEGlobal::midiPorts[port_num], dev);
        MusECore::Route srcRoute(dev, -1);
        MusECore::Route dstRoute(*i, true, -1, MusECore::Route::JACK_ROUTE);
        MusEGlobal::audio->msgAddRoute(srcRoute, dstRoute);
        if(++port_num == MIDI_PORTS)
          return;
      }  
    }
    
    sl = MusEGlobal::audioDevice->outputPorts(true, 1); // Ask for second aliases.
    for(std::list<QString>::iterator i = sl.begin(); i != sl.end(); ++i)
    {
      dev = MusECore::MidiJackDevice::createJackMidiDevice(*i, 2); 
      if(dev)
      {
        //printf("populateMidiPorts Created jack readable device: %s\n", dev->name().toLatin1().constData()); 
        //dev->setOpenFlags(2);
        MusEGlobal::midiSeq->msgSetMidiDevice(&MusEGlobal::midiPorts[port_num], dev);
        MusECore::Route srcRoute(*i, false, -1, MusECore::Route::JACK_ROUTE);
        MusECore::Route dstRoute(dev, -1);
        MusEGlobal::audio->msgAddRoute(srcRoute, dstRoute);
        if(++port_num == MIDI_PORTS)
          return;
      }  
    }
  }
  else
  // If Jack is not running, use ALSA devices.
  if(MusEGlobal::audioDevice->deviceType() == MusECore::AudioDevice::DUMMY_AUDIO)  
  {
    for(MusECore::iMidiDevice i = MusEGlobal::midiDevices.begin(); i != MusEGlobal::midiDevices.end(); ++i) 
    {
      if((*i)->deviceType() != MusECore::MidiDevice::ALSA_MIDI)
        continue;
      dev = *i;
      // Select only sensible devices first - not thru etc.
      //if( ... )
      //  continue;
      
      //dev->setOpenFlags(1);
      MusEGlobal::midiSeq->msgSetMidiDevice(&MusEGlobal::midiPorts[port_num], dev);
        
      if(++port_num == MIDI_PORTS)
        return;
    }

    //for(MusECore::iMidiDevice i = MusEGlobal::midiDevices.begin(); i != MusEGlobal::midiDevices.end(); ++i) 
    //{
    //  if((*i)->deviceType() != MusECore::MidiDevice::ALSA_MIDI)
    //    continue;
    //  // Select the ones ignored in the first pass.
    //  if(!  ... )
    //    continue;
    //    
    //  dev->setOpenFlags(1);
    //  MusEGlobal::midiSeq->msgSetMidiDevice(port_num, dev);
    //    
    //  if(++port_num == MIDI_PORTS)
    //    return;
    //}
  }
    
  //MusEGlobal::muse->changeConfig(true);     // save configuration file
  //MusEGlobal::song->update();
  
}

#else // this code is disabled

DISABLED AND MAYBE OUT-OF-DATE CODE!
the code below is disabled for a longer period of time,
there were certain changes and merges. dunno if that code
works at all. before activating it again, intensively
verify whether its still okay!

// -------------------------------------------------------------------------------------------------------
// populateMidiPorts()
// This version worked somewhat well with system devices. 
// But no, it is virtually impossible to tell from the names whether ports should be paired.
// There is too much room for error - what markers to look for ("capture_"/"playback_") etc.
// It works kind of OK with 'seq' Jack Midi ALSA devices, but not for 'raw' which have a different
//  naming structure ("in-hw-0-0-0"/"out-hw-0-0-0").
// It also fails to combine if the ports were named by a client app, for example another instance of MusE.
// -------------------------------------------------------------------------------------------------------

void populateMidiPorts()
{
  if(!MusEGlobal::checkAudioDevice())
    return;

  MusECore::MidiDevice* dev = 0;
  
  int port_num = 0;
  
  // If Jack is running, prefer Jack midi devices over ALSA.
  if(MusEGlobal::audioDevice->deviceType() == MusECore::AudioDevice::JACK_AUDIO)  
  {
    std::list<QString> wsl;
    std::list<QString> rsl;
    //wsl = MusEGlobal::audioDevice->inputPorts(true, 1);  // Ask for second aliases.
    wsl = MusEGlobal::audioDevice->inputPorts(true, 0);  // Ask for first aliases.
    //rsl = MusEGlobal::audioDevice->outputPorts(true, 1); // Ask for second aliases.
    rsl = MusEGlobal::audioDevice->outputPorts(true, 0); // Ask for first aliases.

    for(std::list<QString>::iterator wi = wsl.begin(); wi != wsl.end(); ++wi)
    {
      QString ws = *wi;
      int y = ws.lastIndexOf("_");
      if(y >= 1)
      {  
        int x = ws.lastIndexOf("_", y-1);
        if(x >= 0)
          ws.remove(x, y - x);
      }
      
      
      bool match_found = false;
      for(std::list<QString>::iterator ri = rsl.begin(); ri != rsl.end(); ++ri)
      {
        QString rs = *ri;
        int y = rs.lastIndexOf("_");
        if(y >= 1)
        {  
          int x = rs.lastIndexOf("_", y-1);
          if(x >= 0)
            rs.remove(x, y - x);
        }
        
        // Do we have a matching pair?
        if(rs == ws)
        {
          // Would like to remove the client name, but no, we need it as a distinguishing identifier.
          //int z = ws.indexOf(":");
          //if(z >= 0)
          //  ws.remove(0, z + 1);
          
          dev = MusECore::MidiJackDevice::createJackMidiDevice(ws, 3); 
          if(dev)
          {
            //printf("populateMidiPorts Created jack writeable/readable device: %s\n", dev->name().toLatin1().constData()); 
            //dev->setOpenFlags(1);
            MusEGlobal::midiSeq->msgSetMidiDevice(&MusEGlobal::midiPorts[port_num], dev);
            MusECore::Route devRoute(dev, -1);
            MusECore::Route wdstRoute(*wi, true, -1, MusECore::Route::JACK_ROUTE);
            MusECore::Route rsrcRoute(*ri, false, -1, MusECore::Route::JACK_ROUTE);
            MusEGlobal::audio->msgAddRoute(devRoute, wdstRoute);
            MusEGlobal::audio->msgAddRoute(rsrcRoute, devRoute);
            if(++port_num == MIDI_PORTS)
              return;
          }  
          
          rsl.erase(ri);  // Done with this read port. Remove.
          match_found = true;
          break;
        }
      }  
      
      if(!match_found)
      {
        // No match was found. Create a single writeable device.
        QString s = *wi;
        // Would like to remove the client name, but no, we need it as a distinguishing identifier.
        //int z = s.indexOf(":");
        //if(z >= 0)
        //  s.remove(0, z + 1);
        dev = MusECore::MidiJackDevice::createJackMidiDevice(s, 1); 
        if(dev)
        {
          //printf("populateMidiPorts Created jack writeable device: %s\n", dev->name().toLatin1().constData()); 
          //dev->setOpenFlags(1);
          MusEGlobal::midiSeq->msgSetMidiDevice(&MusEGlobal::midiPorts[port_num], dev);
          MusECore::Route srcRoute(dev, -1);
          MusECore::Route dstRoute(*wi, true, -1, MusECore::Route::JACK_ROUTE);
          MusEGlobal::audio->msgAddRoute(srcRoute, dstRoute);
          if(++port_num == MIDI_PORTS)
            return;
        }  
      }
    }

    // Create the remaining readable ports as single readable devices.
    for(std::list<QString>::iterator ri = rsl.begin(); ri != rsl.end(); ++ri)
    {
      QString s = *ri;
      // Would like to remove the client name, but no, we need it as a distinguishing identifier.
      //int z = s.indexOf(":");
      //if(z >= 0)
      //  s.remove(0, z + 1);
      dev = MusECore::MidiJackDevice::createJackMidiDevice(s, 2); 
      if(dev)
      {
        //printf("populateMidiPorts Created jack readable device: %s\n", dev->name().toLatin1().constData()); 
        //dev->setOpenFlags(2);
        MusEGlobal::midiSeq->msgSetMidiDevice(&MusEGlobal::midiPorts[port_num], dev);
        MusECore::Route srcRoute(*ri, false, -1, MusECore::Route::JACK_ROUTE);
        MusECore::Route dstRoute(dev, -1);
        MusEGlobal::audio->msgAddRoute(srcRoute, dstRoute);
        if(++port_num == MIDI_PORTS)
          return;
      }  
    }
  }
  else
  // If Jack is not running, use ALSA devices.
  if(MusEGlobal::audioDevice->deviceType() == MusECore::AudioDevice::DUMMY_AUDIO)  
  {
    for(MusECore::iMidiDevice i = MusEGlobal::midiDevices.begin(); i != MusEGlobal::midiDevices.end(); ++i) 
    {
      if((*i)->deviceType() != MusECore::MidiDevice::ALSA_MIDI)
        continue;
      dev = *i;
      // Select only sensible devices first - not thru etc.
      //if( ... )
      //  continue;
      
      //dev->setOpenFlags(1);
      MusEGlobal::midiSeq->msgSetMidiDevice(&MusEGlobal::midiPorts[port_num], dev);
        
      if(++port_num == MIDI_PORTS)
        return;
    }

    //for(MusECore::iMidiDevice i = MusEGlobal::midiDevices.begin(); i != MusEGlobal::midiDevices.end(); ++i) 
    //{
    //  if((*i)->deviceType() != MusECore::MidiDevice::ALSA_MIDI)
    //    continue;
    //  // Select the ones ignored in the first pass.
    //  if(!  ... )
    //    continue;
    //    
    //  dev->setOpenFlags(1);
    //  MusEGlobal::midiSeq->msgSetMidiDevice(port_num, dev);
    //    
    //  if(++port_num == MIDI_PORTS)
    //    return;
    //}
  }
    
  //MusEGlobal::muse->changeConfig(true);     // save configuration file
  //MusEGlobal::song->update();
  
}
#endif   // populateMidiPorts


} // namespace MusEGui

