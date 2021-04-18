//=========================================================
//  MusE
//  Linux Music Editor
//
//  (C) Copyright 1999/2000 Werner Schweer (ws@seh.de)
//  drum_ordering.cpp
//  Moved out of drummap.h by Tim.
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

#include "drum_ordering.h"
#include "song.h"

namespace MusEGlobal {

global_drum_ordering_t global_drum_ordering;

void global_drum_ordering_t::cleanup()
{
  using MusEGlobal::song;
  using MusECore::MidiTrack;
  using MusECore::ciTrack;
  
  QSet<MidiTrack*> tracks;
  for (ciTrack it = song->tracks()->begin(); it != song->tracks()->end(); it++)
    tracks.insert( dynamic_cast<MidiTrack*>(*it) );
  
  for (iterator it = begin(); it != end();)
  {
    if (!tracks.contains(it->first))
      it=erase(it);
    else
      it++;
  }
}

void global_drum_ordering_t::write(int level, MusECore::Xml& xml)
{
  cleanup();
  
  xml.tag(level++, "drum_ordering");

  for (iterator it = begin(); it != end(); it++)
    write_single(level, xml, *it);

  xml.etag(level, "drum_ordering");
}

void global_drum_ordering_t::write_single(int level, MusECore::Xml& xml, const entry_t& entry)
{
  const int trk_idx = MusEGlobal::song->tracks()->index(entry.first);
  if(trk_idx >= 0)
  {
    const QString s= QString("<item track=\"%1\" instr=\"%2\" />").arg(trk_idx).arg(entry.second);
    xml.put(level, "%s", s.toLatin1().constData());
  }
}

void global_drum_ordering_t::read(MusECore::Xml& xml)
{
  using MusECore::Xml;
  
  clear();
  
  for (;;)
  {
    Xml::Token token = xml.parse();
    if (token == Xml::Error || token == Xml::End)
      break;
      
    const QString& tag = xml.s1();
    switch (token)
    {
      case Xml::TagStart:
        // OBSOLETE. Keep for backwards compatibility.
        if (tag == "entry")
          append(read_single(xml));
        else if (tag == "item")
          append(read_item(xml));
        else
          xml.unknown("global_drum_ordering_t");
        break;
        
      case Xml::TagEnd:
        if (tag == "drum_ordering")
          return;
        
      default:
        break;
    }
  }
}
  
global_drum_ordering_t::entry_t global_drum_ordering_t::read_single(MusECore::Xml& xml)
{
  using MusECore::Xml;
  using MusEGlobal::song;
  using MusECore::ciTrack;
  
  entry_t entry;
  entry.first=nullptr;
  entry.second=-1;
  
	for (;;)
	{
		Xml::Token token = xml.parse();
		if (token == Xml::Error || token == Xml::End)
			break;
			
		const QString& tag = xml.s1();
		switch (token)
		{
			case Xml::TagStart:
				if (tag == "track")
        {
					QString track_name=xml.parse1();
          
          ciTrack it;
          for (it = song->tracks()->begin(); it != song->tracks()->end(); it++)
            if (track_name == (*it)->name())
              break;
          
          if (it != song->tracks()->end())
            entry.first=dynamic_cast<MusECore::MidiTrack*>(*it);
        }
				else if (tag == "instrument")
					entry.second=xml.parseInt();
				else
					xml.unknown("global_drum_ordering_t (single entry)");
				break;
				
			case Xml::TagEnd:
				if (tag == "entry")
					goto end_of_read_single;
				
			default:
				break;
		}
	}

  end_of_read_single:
  
  if (entry.first == NULL)
    fprintf(stderr, "ERROR: global_drum_ordering_t::read_single() couldn't find the specified track!\n");
  
  if (entry.second < 0 || entry.second > 127)
    fprintf(stderr, "ERROR: global_drum_ordering_t::read_single(): instrument number is out of bounds (%i)!\n", entry.second);
  
  return entry;
}

global_drum_ordering_t::entry_t global_drum_ordering_t::read_item(MusECore::Xml& xml)
{
  using MusECore::Xml;
  using MusEGlobal::song;
  using MusECore::ciTrack;

  entry_t entry;
  entry.first=nullptr;
  entry.second=-1;

  int trk_idx = -1;
  int instr = -1;

  for (;;)
  {
    Xml::Token token = xml.parse();
    if (token == Xml::Error || token == Xml::End)
      break;
      
    const QString& tag = xml.s1();
    switch (token)
    {
      case Xml::TagStart:
          xml.unknown("global_drum_ordering_t (single item)");
        break;
        
      case Xml::Attribut:
            if (tag == "track")
              trk_idx = xml.s2().toInt();
            else if (tag == "instr")
              instr = xml.s2().toInt();
            else
              fprintf(stderr, "unknown tag %s\n", tag.toLatin1().constData());
        break;

      case Xml::TagEnd:
        if (tag == "item")
          goto end_of_read_item;
        break;
        
      default:
        break;
    }
  }

  end_of_read_item:
  
  if(trk_idx < 0)
  {
    fprintf(stderr, "ERROR: global_drum_ordering_t::read_single() invalid track index (%i)!\n", trk_idx);
    return entry;
  }
  if(instr < 0 || instr > 127)
  {
    fprintf(stderr, "ERROR: global_drum_ordering_t::read_single(): instrument number is out of bounds (%i)!\n", instr);
    return entry;
  }

  MusECore::Track* trk = MusEGlobal::song->tracks()->index(trk_idx);
  if(!trk)
  {
    fprintf(stderr, "ERROR: global_drum_ordering_t::read_single() couldn't find the specified track at idx %i !\n", trk_idx);
    return entry;
  }
  if(!trk->isMidiTrack())
  {
    fprintf(stderr, "ERROR: global_drum_ordering_t::read_single() track is not a midi track at idx %i !\n", trk_idx);
    return entry;
  }
  
  entry.first = static_cast<MusECore::MidiTrack*>(trk);
  entry.second = instr;
  
  return entry;
}

} // namespace MusEGlobal
