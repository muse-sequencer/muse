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
#include "gconfig.h"

#include <QApplication>

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

} // namespace MusEGui

