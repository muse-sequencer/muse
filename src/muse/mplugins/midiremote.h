//=========================================================
//  MusE
//  Linux Music Editor
//    midiremote.h
//  (C) Copyright 2023 Tim E. Real (terminator356 on users dot sourceforge dot net)
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

#ifndef __MIDIREMOTE_H__
#define __MIDIREMOTE_H__

#include "xml.h"

namespace MusECore {

class MidiRemoteStruct
{
  public:
    enum MidiRemoteValType {
      MidiRemoteValTrigger = 0,
      MidiRemoteValToggle,
      MidiRemoteValMomentary,
      MidiRemoteValTypeEnd = MidiRemoteValMomentary
    };

    bool _noteenable;
    // Can be -1 meaning 'any'.
    int _noteport;
    // Can be -1 meaning 'any'.
    int _notechannel;
    // Can be 0 - 127.
    int _note;
    bool _ccenable;
    // Can be -1 meaning 'any'.
    int _ccport;
    // Can be -1 meaning 'any'.
    int _ccchannel;
    // Can be 0 - 127.
    int _ccnum;
    MidiRemoteValType _noteValType;
    MidiRemoteValType _ccValType;

    MidiRemoteStruct(
      int noteport = -1, int notechannel = -1, int note = 0, MidiRemoteValType notevaltype = MidiRemoteValTrigger, bool noteenable = false,
      int ccport = -1, int ccchannel = -1, int ccnum = 0, MidiRemoteValType ccvaltype = MidiRemoteValTrigger, bool ccenable = false);

    void read(const char *name, Xml&);
    void write(const char *name, int level, Xml&) const;
    bool matchesNote(int port, int chan, int note) const;
    bool matchesCC(int port, int chan, int ccnum) const;
};

class MidiRemote
{
  public:
    MidiRemote();
    MidiRemote(
      int stepRecPort,
      int stepRecChan,
      const MidiRemoteStruct& stepRecRest,
      const MidiRemoteStruct& stop,
      const MidiRemoteStruct& rec,
      const MidiRemoteStruct& gotoLeftMark,
      const MidiRemoteStruct& play,
      const MidiRemoteStruct& forward,
      const MidiRemoteStruct& backward);

    void read(Xml&);
    void write(int level, Xml&) const;
    bool matchesStepRec(int port, int chan) const;
    bool matches(int port, int chan, int dataA, bool matchNote = true, bool matchCC = true, bool matchStepRec = true) const;
    void initialize();

    int _stepRecPort;
    int _stepRecChan;
    MidiRemoteStruct _stepRecRest;
    MidiRemoteStruct _stop;
    MidiRemoteStruct _rec;
    MidiRemoteStruct _gotoLeftMark;
    MidiRemoteStruct _play;
    MidiRemoteStruct _forward;
    MidiRemoteStruct _backward;
};

} // namespace MusECore

#endif

