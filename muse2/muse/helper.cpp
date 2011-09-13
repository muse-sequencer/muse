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

#include "helper.h"
#include "part.h"
#include "track.h"
#include "song.h"
#include "app.h"
#include "icons.h"
#include "synth.h"
#include "functions.h"

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

namespace MusEUtil {

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
	TrackList* tl = song->tracks();
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
  
  MessSynth* synMESS   = 0;
  QMenu* synpMESS = 0;
  asmap mapMESS;

  #ifdef DSSI_SUPPORT
  DssiSynth* synDSSI   = 0;
  QMenu* synpDSSI = 0;
  asmap mapDSSI;
  #endif                  
  
  #ifdef VST_SUPPORT
  VstSynth*  synVST    = 0;
  QMenu* synpVST  = 0;
  asmap mapVST;
  #endif                  
  
  // Not necessary, but what the heck.
  QMenu* synpOther = 0;
  asmap mapOther;
  
  //const int synth_base_id = 0x1000;
  int ii = 0;
  for(std::vector<Synth*>::iterator i = synthis.begin(); i != synthis.end(); ++i) 
  {
    synMESS = dynamic_cast<MessSynth*>(*i);
    if(synMESS)
    {
      mapMESS.insert( std::pair<std::string, int> (std::string(synMESS->description().toLower().toLatin1().constData()), ii) );
    }
    else
    {
      
      #ifdef DSSI_SUPPORT
      synDSSI = dynamic_cast<DssiSynth*>(*i);
      if(synDSSI)
      {
        mapDSSI.insert( std::pair<std::string, int> (std::string(synDSSI->description().toLower().toLatin1().constData()), ii) );
      }
      else
      #endif                      
      
      {
        #ifdef VST_SUPPORT
        synVST = dynamic_cast<VstSynth*>(*i);
        if(synVST)
        {
          mapVST.insert( std::pair<std::string, int> (std::string(synVST->description().toLower().toLatin1().constData()), ii) );
        }
        else
        #endif                      
        
        {
          mapOther.insert( std::pair<std::string, int> (std::string((*i)->description().toLower().toLatin1().constData()), ii) );
        }
      }
    }
  
    ++ii;
  }
  
  int sz = synthis.size();
  for(imap i = mapMESS.begin(); i != mapMESS.end(); ++i) 
  {
    int idx = i->second;
    if(idx > sz)           // Sanity check
      continue;
    Synth* s = synthis[idx];
    if(s)
    {
      // No MESS sub-menu yet? Create it now.
      if(!synpMESS)
        synpMESS = new QMenu(parent);
      QAction* sM = synpMESS->addAction(QT_TRANSLATE_NOOP("@default", s->description()) + " <" + QT_TRANSLATE_NOOP("@default", s->name()) + ">");
      sM->setData(MENU_ADD_SYNTH_ID_BASE + idx);
    }  
  }
  
  #ifdef DSSI_SUPPORT
  for(imap i = mapDSSI.begin(); i != mapDSSI.end(); ++i) 
  {
    int idx = i->second;
    if(idx > sz)           
      continue;
    Synth* s = synthis[idx];
    if(s)
    {
      // No DSSI sub-menu yet? Create it now.
      if(!synpDSSI)
        synpDSSI = new QMenu(parent);
      //synpDSSI->insertItem(QT_TRANSLATE_NOOP("@default", s->description()) + " <" + QT_TRANSLATE_NOOP("@default", s->name()) + ">", MENU_ADD_SYNTH_ID_BASE + idx);
      QAction* sD = synpDSSI->addAction(QT_TRANSLATE_NOOP("@default", s->description()) + " <" + QT_TRANSLATE_NOOP("@default", s->name()) + ">");
      sD->setData(MENU_ADD_SYNTH_ID_BASE + idx);
    }  
  }
  #endif
  
  #ifdef VST_SUPPORT
  for(imap i = mapVST.begin(); i != mapVST.end(); ++i) 
  {
    int idx = i->second;
    if(idx > sz)           
      continue;
    Synth* s = synthis[idx];
    if(s)
    {
      // No VST sub-menu yet? Create it now.
      if(!synpVST)
        synpVST = new QMenu(parent);
      QAction* sV = synpVST->addAction(QT_TRANSLATE_NOOP("@default", s->description()) + " <" + QT_TRANSLATE_NOOP("@default", s->name()) + ">");
      sV->setData(MENU_ADD_SYNTH_ID_BASE + idx);
    }  
  }
  #endif
  
  for(imap i = mapOther.begin(); i != mapOther.end(); ++i) 
  {
    int idx = i->second;
    if(idx > sz)          
      continue;
    Synth* s = synthis[idx];
    // No Other sub-menu yet? Create it now.
    if(!synpOther)
      synpOther = new QMenu(parent);
    //synpOther->insertItem(QT_TRANSLATE_NOOP("@default", s->description()) + " <" + QT_TRANSLATE_NOOP("@default", s->name()) + ">", MENU_ADD_SYNTH_ID_BASE + idx);
    QAction* sO = synpOther->addAction(QT_TRANSLATE_NOOP("@default", s->description()) + " <" + QT_TRANSLATE_NOOP("@default", s->name()) + ">");
    sO->setData(MENU_ADD_SYNTH_ID_BASE + idx);
  }
  
  if(synpMESS)
  {
    synpMESS->setIcon(*synthIcon);
    synpMESS->setTitle(QT_TRANSLATE_NOOP("@default", "MESS"));
    synp->addMenu(synpMESS);
  }
  
  #ifdef DSSI_SUPPORT
  if(synpDSSI)
  {
    synpDSSI->setIcon(*synthIcon);
    synpDSSI->setTitle(QT_TRANSLATE_NOOP("@default", "DSSI"));
    synp->addMenu(synpDSSI);
  }  
  #endif
  
  #ifdef VST_SUPPORT
  if(synpVST)
  {
    synpVST->setIcon(*synthIcon);
    synpVST->setTitle(QT_TRANSLATE_NOOP("@default", "FST"));
    synp->addMenu(synpVST);
  }  
  #endif
  
  if(synpOther)
  {
    synpOther->setIcon(*synthIcon);
    synpOther->setTitle(QObject::tr("Other"));
    synp->addMenu(synpOther);
  }
  
  return synp;
}


//---------------------------------------------------------
//   populateAddTrack
//    this is also used in "mixer"
//---------------------------------------------------------

QActionGroup* populateAddTrack(QMenu* addTrack)
      {
      QActionGroup* grp = new QActionGroup(addTrack);

      QAction* midi = addTrack->addAction(QIcon(*addtrack_addmiditrackIcon),
                                          QT_TRANSLATE_NOOP("@default", "Add Midi Track"));
      midi->setData(Track::MIDI);
      grp->addAction(midi);
      QAction* drum = addTrack->addAction(QIcon(*addtrack_drumtrackIcon),
                                          QT_TRANSLATE_NOOP("@default", "Add Drum Track"));
      drum->setData(Track::DRUM);
      grp->addAction(drum);
      QAction* wave = addTrack->addAction(QIcon(*addtrack_wavetrackIcon),
                                          QT_TRANSLATE_NOOP("@default", "Add Wave Track"));
      wave->setData(Track::WAVE);
      grp->addAction(wave);
      QAction* aoutput = addTrack->addAction(QIcon(*addtrack_audiooutputIcon),
                                             QT_TRANSLATE_NOOP("@default", "Add Audio Output"));
      aoutput->setData(Track::AUDIO_OUTPUT);
      grp->addAction(aoutput);
      QAction* agroup = addTrack->addAction(QIcon(*addtrack_audiogroupIcon),
                                            QT_TRANSLATE_NOOP("@default", "Add Audio Group"));
      agroup->setData(Track::AUDIO_GROUP);
      grp->addAction(agroup);
      QAction* ainput = addTrack->addAction(QIcon(*addtrack_audioinputIcon),
                                            QT_TRANSLATE_NOOP("@default", "Add Audio Input"));
      ainput->setData(Track::AUDIO_INPUT);
      grp->addAction(ainput);
      QAction* aaux = addTrack->addAction(QIcon(*addtrack_auxsendIcon),
                                          QT_TRANSLATE_NOOP("@default", "Add Aux Send"));
      aaux->setData(Track::AUDIO_AUX);
      grp->addAction(aaux);

      // Create a sub-menu and fill it with found synth types. Make addTrack the owner.
      QMenu* synp = populateAddSynth(addTrack);
      synp->setIcon(*synthIcon);
      synp->setTitle(QT_TRANSLATE_NOOP("@default", "Add Synth"));

      // Add the sub-menu to the given menu.
      addTrack->addMenu(synp);
      
      //QObject::connect(addTrack, SIGNAL(triggered(QAction *)), song, SLOT(addNewTrack(QAction *)));

      return grp;
      }

bool any_event_selected(const set<Part*>& parts, bool in_range)
{
  return !get_events(parts, in_range ? 3 : 1).empty();
}

} // namespace MusEUtil
