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

#ifndef __MIDICHANNEL_H__
#define __MIDICHANNEL_H__

#include "track.h"

//---------------------------------------------------------
//   MidiChannel
//---------------------------------------------------------

class MidiOutPort;

class MidiChannel : public MidiTrackBase {
      Q_OBJECT

      DrumMap* _drumMap;
      bool _useDrumMap;
      MidiOutPort* _port;
      int _channelNo;

      void clearDevice();

   signals:
      void useDrumMapChanged(bool);

   public:
      MidiChannel(MidiOutPort*, int);
      ~MidiChannel();

      MidiOutPort* port() const           { return _port; }
      int channelNo() const               { return _channelNo; }

      virtual void write(Xml&) const;
      virtual void read(QDomNode);
      virtual Track* newTrack() const     { return 0; }
      virtual bool isMute() const;
      virtual Part* newPart(Part*, bool)  { return 0; }

      bool guiVisible() const;
      bool hasGui() const;

      // void putEvent(const MidiEvent&);
      void playMidiEvent(MidiEvent* ev);

      bool useDrumMap() const            { return _useDrumMap;    }
      void setUseDrumMap(bool val);
      DrumMap* drumMap() const           { return _drumMap; }

      virtual void emitControllerChanged(int id);
      };

#endif

