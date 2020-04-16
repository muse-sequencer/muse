//=========================================================
//  MusE
//  Linux Music Editor
//    tracks_duplicate.cpp
//  (C) Copyright 2011 Tim E. Real (terminator356 on sourceforge.net)
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

#include "name_factory.h"

#include "song.h"

namespace MusECore {
  
NameFactoryBase::NameFactoryBase()
  : QStringList()
{

}

TrackNameFactory::TrackNameFactory()
  : NameFactoryBase()
{
}

TrackNameFactory::TrackNameFactory(Track::TrackType type, QString base, int copies)
  : NameFactoryBase()
{
  genUniqueNames(type, base, copies);
}

//---------------------------------------------------------
//   genUniqueNames
//---------------------------------------------------------

bool TrackNameFactory::genUniqueNames(Track::TrackType type, QString base, int copies)
{
  clear();

  int numberIndex=0;
      
  int num_base = 1;
  if(base.isEmpty())
  {  
    switch(type)
    {
      case Track::MIDI:
      case Track::NEW_DRUM:
      case Track::WAVE:
            base = QString("Track");
            break;
      case Track::AUDIO_OUTPUT:
            base = QString("Out");
            break;
      case Track::AUDIO_GROUP:
            base = QString("Group");
            break;
      case Track::AUDIO_AUX:
            base = QString("Aux");
            break;
      case Track::AUDIO_INPUT:
            base = QString("Input");
            break;
      case Track::AUDIO_SOFTSYNTH:
            base = QString("Synth");
            break;
    };
    base += " ";
  }        
  else 
  {
    // assign new names to copied tracks.
    numberIndex = base.lastIndexOf("#");
    // according to Qt doc for lastIndexOf it should return -1 when not found
    // apparently it returns str_size+1 ?! Let's catch both
    if (numberIndex == -1 || numberIndex > base.size())
    {
      num_base = 2;
      base += " #";                                       
    }
    else
    {
      bool ok;
      num_base = base.right(base.size() - numberIndex - 1).toInt(&ok);
      if(ok)
      {
        // The characters to the right of the '#' converted successfully to a number.
        // Increment the number, and truncate the string to remove the existing number.
        ++num_base;
        base.truncate(numberIndex + 1);
      }
      else
      {
        // The characters to the right of the '#' did not convert to a number.
        // So leave it alone and append a new '#' and number.
        num_base = 2;
        base += " #";                                       
      }
    }
  }
    
  for(int cp = 0; cp < copies; ++cp)
  {
    for (int i = num_base; true; ++i)
    {
      const QString s = base + QString::number(i);
      const Track* track = MusEGlobal::song->findTrack(s);
      if(track == 0)
      {
        if(_usedNames.indexOf(s) == -1)
        {
          _usedNames.append(s);
          append(s);
          break;
        }
      }
    }
  }

  return true;
}

} // namespace MusECore
