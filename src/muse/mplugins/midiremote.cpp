//=========================================================
//  MusE
//  Linux Music Editor
//    midiremote.cpp
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

#include <QString>
#include <QByteArray>

#include "midiremote.h"
#include "globaldefs.h"

namespace MusECore {

MidiRemoteStruct::MidiRemoteStruct(int noteport, int notechannel, int note, MidiRemoteValType notevaltype, bool noteenable,
      int ccport, int ccchannel, int ccnum, MidiRemoteValType ccvaltype, bool ccenable) :
  _noteenable(noteenable), _noteport(noteport), _notechannel(notechannel), _note(note),
  _ccenable(ccenable), _ccport(ccport), _ccchannel(ccchannel), _ccnum(ccnum),
  _noteValType(notevaltype), _ccValType(ccvaltype)
{
}

void MidiRemoteStruct::read(const char *name, Xml& xml)
{
  int v    = 0;
  bool     ok;
  int base = 10;

  for (;;) {
        Xml::Token token = xml.parse();
        const QString& tag = xml.s1();
        switch (token) {
              case Xml::Error:
              case Xml::End:
                    return;
              case Xml::Attribut:
                    {
                      if(tag == "noteport")
                      {
                        v = xml.s2().toInt(&ok, base);
                        if(ok)
                        {
                          if(v < -1 || v >= MusECore::MIDI_PORTS)
                            v = -1;
                          _noteport = v;
                        }
                      }
                      else if(tag == "notechan")
                      {
                        v = xml.s2().toInt(&ok, base);
                        if(ok)
                        {
                          if(v < -1 || v >= MusECore::MUSE_MIDI_CHANNELS)
                            v = -1;
                          _notechannel = v;
                        }
                      }
                      else if(tag == "note")
                      {
                        v = xml.s2().toInt(&ok, base);
                        if(ok)
                        {
                          if(v < 0 || v > 127)
                            v = 0;
                          _note = v;
                        }
                      }
                      else if(tag == "notevaltype")
                      {
                        v = xml.s2().toInt(&ok, base);
                        if(ok)
                        {
                          if(v < 0 || v > MidiRemoteValTypeEnd)
                            v = MidiRemoteValTrigger;
                          _noteValType = MidiRemoteValType(v);
                        }
                      }
                      else if(tag == "noteen")
                      {
                        v = xml.s2().toInt(&ok, base);
                        if(ok)
                          _noteenable = v;
                      }
                      else if(tag == "ccport")
                      {
                        v = xml.s2().toInt(&ok, base);
                        if(ok)
                        {
                          if(v < -1 || v >= MusECore::MIDI_PORTS)
                            v = -1;
                          _ccport = v;
                        }
                      }
                      else if(tag == "ccchan")
                      {
                        v = xml.s2().toInt(&ok, base);
                        if(ok)
                        {
                          if(v < -1 || v >= MusECore::MUSE_MIDI_CHANNELS)
                            v = -1;
                          _ccchannel = v;
                        }
                      }
                      else if(tag == "ccnum")
                      {
                        v = xml.s2().toInt(&ok, base);
                        if(ok)
                        {
                          if(v < 0 || v > 127)
                            v = 0;
                          _ccnum = v;
                        }
                      }
                      else if(tag == "ccvaltype")
                      {
                        v = xml.s2().toInt(&ok, base);
                        if(ok)
                        {
                          if(v < 0 || v > MidiRemoteValTypeEnd)
                            v = MidiRemoteValTrigger;
                          _ccValType = MidiRemoteValType(v);
                        }
                      }
                      else if(tag == "ccen")
                      {
                        v = xml.s2().toInt(&ok, base);
                        if(ok)
                          _ccenable = v;
                      }
                      else
                        fprintf(stderr,"MidiRemoteStruct::read unknown tag %s\n", tag.toLocal8Bit().constData());
                    }
                    break;
              case Xml::TagStart:
                    xml.unknown("MidiRemoteStruct");
                    break;
              case Xml::TagEnd:
                    if (tag == name) {
                          return;
                          }
              default:
                    break;
              }
        }
}

void MidiRemoteStruct::write(const char *name, int level, Xml& xml) const
{
  xml.nput(level,
    "<%s noteport=\"%d\" notechan=\"%d\" note=\"%d\" notevaltype=\"%d\" noteen=\"%d\""
    " ccport=\"%d\" ccchan=\"%d\" ccnum=\"%d\" ccvaltype=\"%d\" ccen=\"%d\"",
    Xml::xmlString(name).toUtf8().constData(), _noteport, _notechannel, _note, _noteValType, _noteenable,
    _ccport, _ccchannel, _ccnum, _ccValType, _ccenable);
  xml.put(" />");
}

bool MidiRemoteStruct::matchesNote(int port, int chan, int note) const
{
  return _noteenable && (_noteport == -1 || port == _noteport) && (_notechannel == -1 || chan == _notechannel) && note == _note;
}

bool MidiRemoteStruct::matchesCC(int port, int chan, int ccnum) const
{
  return _ccenable && (_ccport == -1 || port == _ccport) && (_ccchannel == -1 || chan == _ccchannel) && ccnum == _ccnum;
}


MidiRemote::MidiRemote() :
  _stepRecPort(-1),
  _stepRecChan(-1)
{
  initialize();
}

MidiRemote::MidiRemote(
  int stepRecPort,
  int stepRecChan,
  const MidiRemoteStruct& stepRecRest,
  const MidiRemoteStruct& stop,
  const MidiRemoteStruct& rec,
  const MidiRemoteStruct& gotoLeftMark,
  const MidiRemoteStruct& play,
  const MidiRemoteStruct& forward,
  const MidiRemoteStruct& backward) :
  _stepRecPort(stepRecPort),
  _stepRecChan(stepRecChan),
  _stepRecRest(stepRecRest),
  _stop(stop),
  _rec(rec),
  _gotoLeftMark(gotoLeftMark),
  _play(play),
  _forward(forward),
  _backward(backward)
{
}

void MidiRemote::read(Xml& xml)
{
  for (;;) {
        Xml::Token token = xml.parse();
        const QString& tag = xml.s1();
        switch (token) {
              case Xml::Error:
              case Xml::End:
                    return;
              case Xml::Attribut:
                      fprintf(stderr,"MidiRemote::read unknown tag %s\n", tag.toLocal8Bit().constData());
                    break;
              case Xml::TagStart:
                    {
                      const QByteArray ba = tag.toUtf8();
                      const char *name = ba.constData();
                      if (tag == "stepRecPort")
                        _stepRecPort = xml.parseInt();
                      else if (tag == "stepRecChan")
                        _stepRecChan = xml.parseInt();
                      else if(tag == "stepRecRest")
                        _stepRecRest.read(name, xml);
                      else if(tag == "stop")
                        _stop.read(name, xml);
                      else if(tag == "rec")
                        _rec.read(name, xml);
                      else if(tag == "gotoLeftMark")
                        _gotoLeftMark.read(name, xml);
                      else if(tag == "play")
                        _play.read(name, xml);
                      else if(tag == "forward")
                        _forward.read(name, xml);
                      else if(tag == "backward")
                        _backward.read(name, xml);
                      else
                        xml.unknown("MidiRemote");
                    }
                    break;
              case Xml::TagEnd:
                    if (tag == "midiRemote") {
                          return;
                          }
              default:
                    break;
              }
        }
}

void MidiRemote::write(int level, Xml& xml) const
{
  xml.tag(level++, "midiRemote");

  xml.intTag(level, "stepRecPort", _stepRecPort);
  xml.intTag(level, "stepRecChan", _stepRecChan);
  _stepRecRest.write("stepRecRest", level, xml);
  _stop.write("stop", level, xml);
  _rec.write("rec", level, xml);
  _gotoLeftMark.write("gotoLeftMark", level, xml);
  _play.write("play", level, xml);
  _forward.write("forward", level, xml);
  _backward.write("backward", level, xml);

  xml.etag(--level, "midiRemote");
}

bool MidiRemote::matchesStepRec(int port, int chan) const
{
  return (_stepRecPort == -1 || port == _stepRecPort) && (_stepRecChan == -1 || chan == _stepRecChan);
}

bool MidiRemote::matches(int port, int chan, int dataA, bool matchNote, bool matchCC, bool matchStepRec) const
{
  return
    (matchNote &&
      (_stepRecRest.matchesNote(port, chan, dataA) ||
       _stop.matchesNote(port, chan, dataA) ||
       _rec.matchesNote(port, chan, dataA) ||
       _gotoLeftMark.matchesNote(port, chan, dataA) ||
       _play.matchesNote(port, chan, dataA) ||
       _forward.matchesNote(port, chan, dataA) ||
       _backward.matchesNote(port, chan, dataA))
    ) ||
    (matchCC &&
      (_stepRecRest.matchesCC(port, chan, dataA) ||
       _stop.matchesCC(port, chan, dataA) ||
       _rec.matchesCC(port, chan, dataA) ||
       _gotoLeftMark.matchesCC(port, chan, dataA) ||
       _play.matchesCC(port, chan, dataA) ||
       _forward.matchesCC(port, chan, dataA) ||
       _backward.matchesCC(port, chan, dataA))
    ) ||
    (matchStepRec &&
      ((_stepRecPort == -1 || _stepRecPort == port) &&
       (_stepRecChan == -1 || _stepRecChan == chan))
    );
}

void MidiRemote::initialize()
{
  *this =
  {
    // stepRec Only the note port and channel are used here.
    -1, -1,
    // stepRecRest
    { -1, -1, 36, MusECore::MidiRemoteStruct::MidiRemoteValTrigger, false, -1, -1, 116, MusECore::MidiRemoteStruct::MidiRemoteValTrigger, false },
    // stop
    { -1, -1, 28, MusECore::MidiRemoteStruct::MidiRemoteValTrigger, false, -1, -1, 114, MusECore::MidiRemoteStruct::MidiRemoteValTrigger, false },
    // rec
    { -1, -1, 31, MusECore::MidiRemoteStruct::MidiRemoteValTrigger, false, -1, -1, 117, MusECore::MidiRemoteStruct::MidiRemoteValTrigger, false },
    // gotoLeftMark
    { -1, -1, 33, MusECore::MidiRemoteStruct::MidiRemoteValTrigger, false, -1, -1, 111, MusECore::MidiRemoteStruct::MidiRemoteValTrigger, false },
    // play
    { -1, -1, 29, MusECore::MidiRemoteStruct::MidiRemoteValTrigger, false, -1, -1, 115, MusECore::MidiRemoteStruct::MidiRemoteValTrigger, false },
    // forward
    { -1, -1, 26, MusECore::MidiRemoteStruct::MidiRemoteValTrigger, false, -1, -1, 113, MusECore::MidiRemoteStruct::MidiRemoteValTrigger, false },
    // backward
    { -1, -1, 24, MusECore::MidiRemoteStruct::MidiRemoteValTrigger, false, -1, -1, 112, MusECore::MidiRemoteStruct::MidiRemoteValTrigger, false }
  };
};

} // namespace MusECore
