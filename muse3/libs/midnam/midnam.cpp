//=========================================================
//  MusE
//  Linux Music Editor
//
//  midnam.cpp
//  (C) Copyright 2019, 2020 Tim E. Real (terminator356 on users dot sourceforge dot net)
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

#include "midnam.h"
#include "midi_consts.h"
#include <QList>

// Channel range is from 1 to 16.
#define MIDNAM_MAX_CHANNEL 16

namespace MusECore {

//----------------------
// TODO MTCQuarterFrame
//----------------------
// bool readMTCQuarterFrame(
//   MusECore::Xml& xml, MidiPlayEvent& ev, int time_ms,
//   int defaultPort = 0
//   )
// {
// }

bool readSystemReset(
  MusECore::Xml& xml, MidiPlayEvent& ev, int time_ms,
  int defaultPort = 0
  )
{
  for (;;) {
        MusECore::Xml::Token token(xml.parse());
        const QString& tag(xml.s1());
        switch (token) {
              case MusECore::Xml::Error:
              case MusECore::Xml::End:
                    return false;
              case MusECore::Xml::TagStart:
                      xml.unknown("readSystemReset");
                    break;
              case MusECore::Xml::Attribut:
                    break;
              case MusECore::Xml::TagEnd:
                    if (tag == "SystemReset")
                    {
                      ev = MidiPlayEvent(time_ms, defaultPort, 0, ME_RESET, 0, 0);
                      return true;
                    }
              default:
                    break;
              }
        }
  return false;
}

bool readActiveSensing(
  MusECore::Xml& xml, MidiPlayEvent& ev, int time_ms,
  int defaultPort = 0
  )
{
  for (;;) {
        MusECore::Xml::Token token(xml.parse());
        const QString& tag(xml.s1());
        switch (token) {
              case MusECore::Xml::Error:
              case MusECore::Xml::End:
                    return false;
              case MusECore::Xml::TagStart:
                      xml.unknown("readActiveSensing");
                    break;
              case MusECore::Xml::Attribut:
                    break;
              case MusECore::Xml::TagEnd:
                    if (tag == "ActiveSensing")
                    {
                      ev = MidiPlayEvent(time_ms, defaultPort, 0, ME_SENSE, 0, 0);
                      return true;
                    }
              default:
                    break;
              }
        }
  return false;
}

bool readStop(
  MusECore::Xml& xml, MidiPlayEvent& ev, int time_ms,
  int defaultPort = 0
  )
{
  for (;;) {
        MusECore::Xml::Token token(xml.parse());
        const QString& tag(xml.s1());
        switch (token) {
              case MusECore::Xml::Error:
              case MusECore::Xml::End:
                    return false;
              case MusECore::Xml::TagStart:
                      xml.unknown("readStop");
                    break;
              case MusECore::Xml::Attribut:
                    break;
              case MusECore::Xml::TagEnd:
                    if (tag == "Stop")
                    {
                      ev = MidiPlayEvent(time_ms, defaultPort, 0, ME_STOP, 0, 0);
                      return true;
                    }
              default:
                    break;
              }
        }
  return false;
}

bool readContinue(
  MusECore::Xml& xml, MidiPlayEvent& ev, int time_ms,
  int defaultPort = 0
  )
{
  for (;;) {
        MusECore::Xml::Token token(xml.parse());
        const QString& tag(xml.s1());
        switch (token) {
              case MusECore::Xml::Error:
              case MusECore::Xml::End:
                    return false;
              case MusECore::Xml::TagStart:
                      xml.unknown("readContinue");
                    break;
              case MusECore::Xml::Attribut:
                    break;
              case MusECore::Xml::TagEnd:
                    if (tag == "Continue")
                    {
                      ev = MidiPlayEvent(time_ms, defaultPort, 0, ME_CONTINUE, 0, 0);
                      return true;
                    }
              default:
                    break;
              }
        }
  return false;
}

bool readStart(
  MusECore::Xml& xml, MidiPlayEvent& ev, int time_ms,
  int defaultPort = 0
  )
{
  for (;;) {
        MusECore::Xml::Token token(xml.parse());
        const QString& tag(xml.s1());
        switch (token) {
              case MusECore::Xml::Error:
              case MusECore::Xml::End:
                    return false;
              case MusECore::Xml::TagStart:
                      xml.unknown("readStart");
                    break;
              case MusECore::Xml::Attribut:
                    break;
              case MusECore::Xml::TagEnd:
                    if (tag == "Start")
                    {
                      ev = MidiPlayEvent(time_ms, defaultPort, 0, ME_START, 0, 0);
                      return true;
                    }
              default:
                    break;
              }
        }
  return false;
}

bool readTimingClock(
  MusECore::Xml& xml, MidiPlayEvent& ev, int time_ms,
  int defaultPort = 0
  )
{
  for (;;) {
        MusECore::Xml::Token token(xml.parse());
        const QString& tag(xml.s1());
        switch (token) {
              case MusECore::Xml::Error:
              case MusECore::Xml::End:
                    return false;
              case MusECore::Xml::TagStart:
                      xml.unknown("readTimingClock");
                    break;
              case MusECore::Xml::Attribut:
                    break;
              case MusECore::Xml::TagEnd:
                    if (tag == "TimingClock")
                    {
                      ev = MidiPlayEvent(time_ms, defaultPort, 0, ME_CLOCK, 0, 0);
                      return true;
                    }
              default:
                    break;
              }
        }
  return false;
}

bool readTuneRequest(
  MusECore::Xml& xml, MidiPlayEvent& ev, int time_ms,
  int defaultPort = 0
  )
{
  for (;;) {
        MusECore::Xml::Token token(xml.parse());
        const QString& tag(xml.s1());
        switch (token) {
              case MusECore::Xml::Error:
              case MusECore::Xml::End:
                    return false;
              case MusECore::Xml::TagStart:
                      xml.unknown("readTuneRequest");
                    break;
              case MusECore::Xml::Attribut:
                    break;
              case MusECore::Xml::TagEnd:
                    if (tag == "TuneRequest")
                    {
                      ev = MidiPlayEvent(time_ms, defaultPort, 0, ME_TUNE_REQ, 0, 0);
                      return true;
                    }
              default:
                    break;
              }
        }
  return false;
}

bool readSongSelect(
  MusECore::Xml& xml, MidiPlayEvent& ev, int time_ms,
  int defaultPort = 0
  )
{
  int number = -1;

  for (;;) {
        MusECore::Xml::Token token(xml.parse());
        const QString& tag(xml.s1());
        switch (token) {
              case MusECore::Xml::Error:
              case MusECore::Xml::End:
                    return false;
              case MusECore::Xml::TagStart:
                      xml.unknown("readSongSelect");
                    break;
              case MusECore::Xml::Attribut:
                    if (tag == "Number") {
                          number = xml.s2().toInt();
                          }
                    break;
              case MusECore::Xml::TagEnd:
                    if (tag == "SongSelect")
                    {
                      if(number < 0)
                        return false;
                      // <!-- 0-127 -->
                      ev = MidiPlayEvent(time_ms, defaultPort, 0, ME_SONGSEL, number, 0);
                      return true;
                    }
              default:
                    break;
              }
        }
  return false;
}

bool readSongPositionPointer(
  MusECore::Xml& xml, MidiPlayEvent& ev, int time_ms,
  int defaultPort = 0
  )
{
  int position = -1;

  for (;;) {
        MusECore::Xml::Token token(xml.parse());
        const QString& tag(xml.s1());
        switch (token) {
              case MusECore::Xml::Error:
              case MusECore::Xml::End:
                    return false;
              case MusECore::Xml::TagStart:
                      xml.unknown("readSongPositionPointer");
                    break;
              case MusECore::Xml::Attribut:
                    if (tag == "Position") {
                          position = xml.s2().toInt();
                          }
                    break;
              case MusECore::Xml::TagEnd:
                    if (tag == "SongPositionPointer")
                    {
                      if(position < 0)
                        return false;
                      // <!-- 0-16383 -->
                      ev = MidiPlayEvent(time_ms, defaultPort, 0, ME_SONGPOS, position, 0);
                      return true;
                    }
              default:
                    break;
              }
        }
  return false;
}

bool readSysEx(
  MusECore::Xml& xml, MidiPlayEvent& ev, int time_ms,
  int defaultPort = 0,
  int defaultChannel = 0)
{
  int chan = -1;
  QByteArray bytes;

  for (;;) {
        MusECore::Xml::Token token(xml.parse());
        const QString& tag(xml.s1());
        switch (token) {
              case MusECore::Xml::Error:
              case MusECore::Xml::End:
                    return false;
              case MusECore::Xml::TagStart:
                      xml.unknown("readSysEx");
                    break;
              case MusECore::Xml::Attribut:
                    break;
              case Xml::Text:
                    {
                      // <!ELEMENT SysEx (#PCDATA | SysExDeviceID | SysExChannel)*>		
                      // <!-- contains string of hex bytes without radix information, e.g.:
                      // <SysEx>F0 01 02 <SysExDeviceID/> 03 04 05 <SysExChannel Multiplier="1" Offset="32"/> F7</SysEx>	
                      // -->
                      //
                      // <!ELEMENT SysExDeviceID EMPTY>		<!-- Byte as a function of user-visible Device ID
                      //                    e.g. 17 as the default on many Roland devices.
                      //                    Most sysex messages will use this element. -->
                      //  <!ATTLIST SysExDeviceID
                      //    Multiplier	NMTOKEN	"1"
                      //    Offset		NMTOKEN	"0">	<!-- multiplier and offset are decimal -->	
                      //    
                      // <!ELEMENT SysExChannel	EMPTY>		<!-- Byte as a function of MIDI Channel 1-16, for
                      //                    specialized situations where a sys-ex message
                      //                    is being directed to a specific MIDI channel.
                      //                    Generally only older devices require use of this. -->
                      //  <!ATTLIST SysExChannel
                      //    Multiplier	NMTOKEN	"1"
                      //    Offset		NMTOKEN	"0">	<!-- multiplier and offset are decimal -->
                      //
                      //
                      // I found only TWO examples on the net that use SysExDeviceID,
                      //  others don't use SysExDeviceID in the SysEx string:
                      //
                      // Example from https://github.com/Ardour/ardour/blob/master/libs/pbd/test/ProtoolsPatchFile.midnam
                      // <SysEx>F0 41 <SysExDeviceID Offset="00" /> 42 12 40 00 7F 00 41 F7</SysEx>
                      //
                      // Example from https://github.com/Ardour/ardour/blob/master/patchfiles/Korg_iSeries.midnam
                      // <SysEx>F0 42<SysExDeviceID Offset="30"/> 39 4E 06 F7</SysEx>
                      //
                      // Notice in that example there is NO SPACE between 42 and <SysExDeviceID...
                      //  making it difficult to split !!!
                      //
                      //
                      // FIXME: What are the multiplier and offset for?
                      //        I could find no information on this.
                      // FIXME: How to extract them from the hex string?
                      //        Will our xml class automatically stop
                      //         after the 42 and give us the ending tag,
                      //         and how would we continue after the end tag?
    
                      QByteArray ba = tag.toLatin1();
                      // In case there are embedded tags which have no space
                      //  separating them from values, insert spaces so that
                      //  they are treated separately by the split.
                      ba.replace('<', " <");
                      ba.replace('>', "> ");
                      // Simplify to get rid of extra space before splitting.
                      ba = ba.simplified();
                      const QList<QByteArray> vals = ba.split(' ');
                      bool ok;
                      bool in_tag = false;
                      for(QList<QByteArray>::const_iterator il = vals.cbegin(); il != vals.cend(); ++il)
                      {
                        const QByteArray& l_ba = *il;
                        if(in_tag)
                        {
                          if(l_ba.endsWith('>'))
                            in_tag = false;
                          continue;
                        }
                        else if(l_ba.startsWith('<'))
                        {
                          in_tag = true;
                          continue;
                        }
                        const unsigned int c = l_ba.toUInt(&ok, 16);
                        // Anything higher than a byte is an error.
                        if(!ok || c >= 0x100)
                          continue;
                        bytes.append(c);
                      }
                    }
                    break;
              case MusECore::Xml::TagEnd:
                    if (tag == "SysEx")
                    {
                        if(chan == 0 || chan > MIDNAM_MAX_CHANNEL || bytes.isEmpty())
                          return false;
                        if(chan < 0)
                          chan = defaultChannel;
                        else
                          --chan;
                        
                        // Now create the data.
                        ev.setTime(time_ms);
                        ev.setPort(defaultPort);
                        ev.setChannel(chan);
                        ev.setType(ME_SYSEX);
                        ev.setData((const unsigned char*)bytes.constData(), bytes.size());
                        return true;
                    }
              default:
                    break;
              }
        }
  return false;
}

bool readNRPNChange(
  MusECore::Xml& xml, MidiPlayEvent& ev, int time_ms,
  int defaultPort = 0,
  bool channelRequired = false,
  int defaultChannel = 0)
{
  int chan = -1;
  int nrpn = -1;
  int value = -1;

  for (;;) {
        MusECore::Xml::Token token(xml.parse());
        const QString& tag(xml.s1());
        switch (token) {
              case MusECore::Xml::Error:
              case MusECore::Xml::End:
                    return false;
              case MusECore::Xml::TagStart:
                      xml.unknown("readNRPNChange");
                    break;
              case MusECore::Xml::Attribut:
                    if (tag == "Channel") {
                          chan = xml.s2().toInt();
                          }
                    else if (tag == "NRPN") {
                          nrpn = xml.s2().toInt();
                          }
                    else if (tag == "Value") {
                          value = xml.s2().toInt();
                          }
                    break;
              case MusECore::Xml::TagEnd:
                    if (tag == "NRPNChange")
                    {
                        if((channelRequired && chan < 0) || chan == 0 || chan > MIDNAM_MAX_CHANNEL ||
                           nrpn < 0 || value < 0)
                          return false;
                        if(chan < 0)
                          chan = defaultChannel;
                        else
                          --chan;
                        // 14-bit control changes:
                        // Value is 0..16383
                        // ControlChange14's Control is MSB 0..31
                        // RPNN and NRPN are 0..16383
                        nrpn = ((nrpn & 0x3f80) << 1) + (nrpn & 0x7f) + CTRL_NRPN_OFFSET;
                        ev = MidiPlayEvent(time_ms, defaultPort, chan, ME_CONTROLLER, nrpn, value);
                        return true;
                    }
              default:
                    break;
              }
        }
  return false;
}

bool readRPNChange(
  MusECore::Xml& xml, MidiPlayEvent& ev, int time_ms,
  int defaultPort = 0,
  bool channelRequired = false,
  int defaultChannel = 0)
{
  int chan = -1;
  int rpn = -1;
  int value = -1;

  for (;;) {
        MusECore::Xml::Token token(xml.parse());
        const QString& tag(xml.s1());
        switch (token) {
              case MusECore::Xml::Error:
              case MusECore::Xml::End:
                    return false;
              case MusECore::Xml::TagStart:
                      xml.unknown("readRPNChange");
                    break;
              case MusECore::Xml::Attribut:
                    if (tag == "Channel") {
                          chan = xml.s2().toInt();
                          }
                    else if (tag == "RPN") {
                          rpn = xml.s2().toInt();
                          }
                    else if (tag == "Value") {
                          value = xml.s2().toInt();
                          }
                    break;
              case MusECore::Xml::TagEnd:
                    if (tag == "RPNChange")
                    {
                        if((channelRequired && chan < 0) || chan == 0 || chan > MIDNAM_MAX_CHANNEL ||
                           rpn < 0 || value < 0)
                          return false;
                        if(chan < 0)
                          chan = defaultChannel;
                        else
                          --chan;
                        // 14-bit control changes:
                        // Value is 0..16383
                        // ControlChange14's Control is MSB 0..31
                        // RPNN and NRPN are 0..16383
                        rpn = ((rpn & 0x3f80) << 1) + (rpn & 0x7f) + CTRL_RPN_OFFSET;
                        ev = MidiPlayEvent(time_ms, defaultPort, chan, ME_CONTROLLER, rpn, value);
                        return true;
                    }
              default:
                    break;
              }
        }
  return false;
}

bool readControlChange14(
  MusECore::Xml& xml, MidiPlayEvent& ev, int time_ms,
  int defaultPort = 0,
  bool channelRequired = false,
  int defaultChannel = 0)
{
  int chan = -1;
  int control = -1;
  int value = -1;

  for (;;) {
        MusECore::Xml::Token token(xml.parse());
        const QString& tag(xml.s1());
        switch (token) {
              case MusECore::Xml::Error:
              case MusECore::Xml::End:
                    return false;
              case MusECore::Xml::TagStart:
                      xml.unknown("readControlChange14");
                    break;
              case MusECore::Xml::Attribut:
                    if (tag == "Channel") {
                          chan = xml.s2().toInt();
                          }
                    else if (tag == "Control") {
                          control = xml.s2().toInt();
                          }
                    else if (tag == "Value") {
                          value = xml.s2().toInt();
                          }
                    break;
              case MusECore::Xml::TagEnd:
                    if (tag == "ControlChange14")
                    {
                        if((channelRequired && chan < 0) || chan == 0 || chan > MIDNAM_MAX_CHANNEL ||
                           control < 0 || value < 0)
                          return false;
                        if(chan < 0)
                          chan = defaultChannel;
                        else
                          --chan;
                        // 14-bit control changes:
                        // Value is 0..16383
                        // ControlChange14's Control is MSB 0..31
                        // NOTE: This does not support our own separate high and low control numbers,
                        //        only standard MSB and implied LSB.
                        control = ((control + 0x20) << 8) + control + CTRL_14_OFFSET;
                        ev = MidiPlayEvent(time_ms, defaultPort, chan, ME_CONTROLLER, control, value);
                        return true;
                    }
              default:
                    break;
              }
        }
  return false;
}

bool readPolyMode(
  MusECore::Xml& xml, MidiPlayEvent& ev, int time_ms,
  int defaultPort = 0,
  bool channelRequired = false,
  int defaultChannel = 0)
{
  int chan = -1;

  for (;;) {
        MusECore::Xml::Token token(xml.parse());
        const QString& tag(xml.s1());
        switch (token) {
              case MusECore::Xml::Error:
              case MusECore::Xml::End:
                    return false;
              case MusECore::Xml::TagStart:
                      xml.unknown("readPolyMode");
                    break;
              case MusECore::Xml::Attribut:
                    if (tag == "Channel") {
                          chan = xml.s2().toInt();
                          }
                    break;
              case MusECore::Xml::TagEnd:
                    if (tag == "PolyMode")
                    {
                        if((channelRequired && chan < 0) || chan == 0 || chan > MIDNAM_MAX_CHANNEL)
                          return false;
                        if(chan < 0)
                          chan = defaultChannel;
                        else
                          --chan;
                        ev = MidiPlayEvent(time_ms, defaultPort, chan, ME_CONTROLLER, CTRL_POLY_MODE_ON, 0);
                        return true;
                    }
              default:
                    break;
              }
        }
  return false;
}

bool readMonoMode(
  MusECore::Xml& xml, MidiPlayEvent& ev, int time_ms,
  int defaultPort = 0,
  bool channelRequired = false,
  int defaultChannel = 0)
{
  int chan = -1;
  int value = -1;

  for (;;) {
        MusECore::Xml::Token token(xml.parse());
        const QString& tag(xml.s1());
        switch (token) {
              case MusECore::Xml::Error:
              case MusECore::Xml::End:
                    return false;
              case MusECore::Xml::TagStart:
                      xml.unknown("readMonoMode");
                    break;
              case MusECore::Xml::Attribut:
                    if (tag == "Channel") {
                          chan = xml.s2().toInt();
                          }
                    else if (tag == "Value") {
                          value = xml.s2().toInt();
                          }
                    break;
              case MusECore::Xml::TagEnd:
                    if (tag == "MonoMode")
                    {
                        if((channelRequired && chan < 0) || chan == 0 || chan > MIDNAM_MAX_CHANNEL ||
                           value < 0)
                          return false;
                        if(chan < 0)
                          chan = defaultChannel;
                        else
                          --chan;
                        ev = MidiPlayEvent(time_ms, defaultPort, chan, ME_CONTROLLER, CTRL_MONO_MODE_ON, value);
                        return true;
                    }
              default:
                    break;
              }
        }
  return false;
}

bool readOmniOn(
  MusECore::Xml& xml, MidiPlayEvent& ev, int time_ms,
  int defaultPort = 0,
  bool channelRequired = false,
  int defaultChannel = 0)
{
  int chan = -1;

  for (;;) {
        MusECore::Xml::Token token(xml.parse());
        const QString& tag(xml.s1());
        switch (token) {
              case MusECore::Xml::Error:
              case MusECore::Xml::End:
                    return false;
              case MusECore::Xml::TagStart:
                      xml.unknown("readOmniOn");
                    break;
              case MusECore::Xml::Attribut:
                    if (tag == "Channel") {
                          chan = xml.s2().toInt();
                          }
                    break;
              case MusECore::Xml::TagEnd:
                    if (tag == "OmniOn")
                    {
                        if((channelRequired && chan < 0) || chan == 0 || chan > MIDNAM_MAX_CHANNEL)
                          return false;
                        if(chan < 0)
                          chan = defaultChannel;
                        else
                          --chan;
                        ev = MidiPlayEvent(time_ms, defaultPort, chan, ME_CONTROLLER, CTRL_OMNI_MODE_ON, 0);
                        return true;
                    }
              default:
                    break;
              }
        }
  return false;
}

bool readOmniOff(
  MusECore::Xml& xml, MidiPlayEvent& ev, int time_ms,
  int defaultPort = 0,
  bool channelRequired = false,
  int defaultChannel = 0)
{
  int chan = -1;

  for (;;) {
        MusECore::Xml::Token token(xml.parse());
        const QString& tag(xml.s1());
        switch (token) {
              case MusECore::Xml::Error:
              case MusECore::Xml::End:
                    return false;
              case MusECore::Xml::TagStart:
                      xml.unknown("readOmniOff");
                    break;
              case MusECore::Xml::Attribut:
                    if (tag == "Channel") {
                          chan = xml.s2().toInt();
                          }
                    break;
              case MusECore::Xml::TagEnd:
                    if (tag == "OmniOff")
                    {
                        if((channelRequired && chan < 0) || chan == 0 || chan > MIDNAM_MAX_CHANNEL)
                          return false;
                        if(chan < 0)
                          chan = defaultChannel;
                        else
                          --chan;
                        ev = MidiPlayEvent(time_ms, defaultPort, chan, ME_CONTROLLER, CTRL_OMNI_MODE_OFF, 0);
                        return true;
                    }
              default:
                    break;
              }
        }
  return false;
}

bool readLocalControl(
  MusECore::Xml& xml, MidiPlayEvent& ev, int time_ms,
  int defaultPort = 0,
  bool channelRequired = false,
  int defaultChannel = 0)
{
  int chan = -1;
  int value = -1;
  QString val_txt;

  for (;;) {
        MusECore::Xml::Token token(xml.parse());
        const QString& tag(xml.s1());
        switch (token) {
              case MusECore::Xml::Error:
              case MusECore::Xml::End:
                    return false;
              case MusECore::Xml::TagStart:
                      xml.unknown("readLocalControl");
                    break;
              case MusECore::Xml::Attribut:
                    if (tag == "Channel") {
                          chan = xml.s2().toInt();
                          }
                    else if (tag == "Value") {
                          val_txt = xml.s2();
                          if(val_txt.compare("on", Qt::CaseInsensitive) == 0)
                            value = 127;
                          else if(val_txt.compare("off", Qt::CaseInsensitive) == 0)
                            value = 0;
                          }
                    break;
              case MusECore::Xml::TagEnd:
                    if (tag == "LocalControl")
                    {
                        if((channelRequired && chan < 0) || chan == 0 || chan > MIDNAM_MAX_CHANNEL ||
                           value < 0)
                          return false;
                        if(chan < 0)
                          chan = defaultChannel;
                        else
                          --chan;
                        ev = MidiPlayEvent(time_ms, defaultPort, chan, ME_CONTROLLER, CTRL_LOCAL_OFF, value);
                        return true;
                    }
              default:
                    break;
              }
        }
  return false;
}

bool readResetAllControllers(
  MusECore::Xml& xml, MidiPlayEvent& ev, int time_ms,
  int defaultPort = 0,
  bool channelRequired = false,
  int defaultChannel = 0)
{
  int chan = -1;

  for (;;) {
        MusECore::Xml::Token token(xml.parse());
        const QString& tag(xml.s1());
        switch (token) {
              case MusECore::Xml::Error:
              case MusECore::Xml::End:
                    return false;
              case MusECore::Xml::TagStart:
                      xml.unknown("readResetAllControllers");
                    break;
              case MusECore::Xml::Attribut:
                    if (tag == "Channel") {
                          chan = xml.s2().toInt();
                          }
                    break;
              case MusECore::Xml::TagEnd:
                    if (tag == "ResetAllControllers")
                    {
                        if((channelRequired && chan < 0) || chan == 0 || chan > MIDNAM_MAX_CHANNEL)
                          return false;
                        if(chan < 0)
                          chan = defaultChannel;
                        else
                          --chan;
                        ev = MidiPlayEvent(time_ms, defaultPort, chan, ME_CONTROLLER, CTRL_RESET_ALL_CTRL, 0);
                        return true;
                    }
              default:
                    break;
              }
        }
  return false;
}

bool readAllNotesOff(
  MusECore::Xml& xml, MidiPlayEvent& ev, int time_ms,
  int defaultPort = 0,
  bool channelRequired = false,
  int defaultChannel = 0)
{
  int chan = -1;

  for (;;) {
        MusECore::Xml::Token token(xml.parse());
        const QString& tag(xml.s1());
        switch (token) {
              case MusECore::Xml::Error:
              case MusECore::Xml::End:
                    return false;
              case MusECore::Xml::TagStart:
                      xml.unknown("readAllNotesOff");
                    break;
              case MusECore::Xml::Attribut:
                    if (tag == "Channel") {
                          chan = xml.s2().toInt();
                          }
                    break;
              case MusECore::Xml::TagEnd:
                    if (tag == "AllNotesOff")
                    {
                        if((channelRequired && chan < 0) || chan == 0 || chan > MIDNAM_MAX_CHANNEL)
                          return false;
                        if(chan < 0)
                          chan = defaultChannel;
                        else
                          --chan;
                        ev = MidiPlayEvent(time_ms, defaultPort, chan, ME_CONTROLLER, CTRL_ALL_NOTES_OFF, 0);
                        return true;
                    }
              default:
                    break;
              }
        }
  return false;
}

bool readAllSoundOff(
  MusECore::Xml& xml, MidiPlayEvent& ev, int time_ms,
  int defaultPort = 0,
  bool channelRequired = false,
  int defaultChannel = 0)
{
  int chan = -1;

  for (;;) {
        MusECore::Xml::Token token(xml.parse());
        const QString& tag(xml.s1());
        switch (token) {
              case MusECore::Xml::Error:
              case MusECore::Xml::End:
                    return false;
              case MusECore::Xml::TagStart:
                      xml.unknown("readAllSoundOff");
                    break;
              case MusECore::Xml::Attribut:
                    if (tag == "Channel") {
                          chan = xml.s2().toInt();
                          }
                    break;
              case MusECore::Xml::TagEnd:
                    if (tag == "AllSoundOff")
                    {
                        if((channelRequired && chan < 0) || chan == 0 || chan > MIDNAM_MAX_CHANNEL)
                          return false;
                        if(chan < 0)
                          chan = defaultChannel;
                        else
                          --chan;
                        ev = MidiPlayEvent(time_ms, defaultPort, chan, ME_CONTROLLER, CTRL_ALL_SOUNDS_OFF, 0);
                        return true;
                    }
              default:
                    break;
              }
        }
  return false;
}

bool readPitchBendChange(
  MusECore::Xml& xml, MidiPlayEvent& ev, int time_ms,
  int defaultPort = 0,
  bool channelRequired = false,
  int defaultChannel = 0)
{
  int chan = -1;
  int value = -1;

  for (;;) {
        MusECore::Xml::Token token(xml.parse());
        const QString& tag(xml.s1());
        switch (token) {
              case MusECore::Xml::Error:
              case MusECore::Xml::End:
                    return false;
              case MusECore::Xml::TagStart:
                      xml.unknown("readPitchBendChange");
                    break;
              case MusECore::Xml::Attribut:
                    if (tag == "Channel") {
                          chan = xml.s2().toInt();
                          }
                    else if (tag == "Value") {
                          value = xml.s2().toInt();
                          }
                    break;
              case MusECore::Xml::TagEnd:
                    if (tag == "PitchBendChange")
                    {
                        if((channelRequired && chan < 0) || chan == 0 || chan > MIDNAM_MAX_CHANNEL ||
                           value < 0)
                          return false;
                        if(chan < 0)
                          chan = defaultChannel;
                        else
                          --chan;
                        // PitchBend's Value is 0..16383
                        // Our MPEvent range is -8192..8191 in data A.
                        value -= 8192;
                        ev = MidiPlayEvent(time_ms, defaultPort, chan, ME_PITCHBEND, value, 0);
                        return true;
                    }
              default:
                    break;
              }
        }
  return false;
}

bool readChannelKeyPressure(
  MusECore::Xml& xml, MidiPlayEvent& ev, int time_ms,
  int defaultPort = 0,
  bool channelRequired = false,
  int defaultChannel = 0)
{
  int chan = -1;
  int pressure = -1;

  for (;;) {
        MusECore::Xml::Token token(xml.parse());
        const QString& tag(xml.s1());
        switch (token) {
              case MusECore::Xml::Error:
              case MusECore::Xml::End:
                    return false;
              case MusECore::Xml::TagStart:
                      xml.unknown("readChannelKeyPressure");
                    break;
              case MusECore::Xml::Attribut:
                    if (tag == "Channel") {
                          chan = xml.s2().toInt();
                          }
                    else if (tag == "Pressure") {
                          pressure = xml.s2().toInt();
                          }
                    break;
              case MusECore::Xml::TagEnd:
                    if (tag == "ChannelKeyPressure")
                    {
                        if((channelRequired && chan < 0) || chan == 0 || chan > MIDNAM_MAX_CHANNEL ||
                           pressure < 0)
                          return false;
                        if(chan < 0)
                          chan = defaultChannel;
                        else
                          --chan;
                        ev = MidiPlayEvent(time_ms, defaultPort, chan, ME_AFTERTOUCH, pressure, 0);
                        return true;
                    }
              default:
                    break;
              }
        }
  return false;
}

bool readProgramChange(
  MusECore::Xml& xml, MidiPlayEvent& ev, int time_ms,
  int defaultPort = 0,
  bool channelRequired = false,
  int defaultChannel = 0)
{
  int chan = -1;
  int number = -1;

  for (;;) {
        MusECore::Xml::Token token(xml.parse());
        const QString& tag(xml.s1());
        switch (token) {
              case MusECore::Xml::Error:
              case MusECore::Xml::End:
                    return false;
              case MusECore::Xml::TagStart:
                      xml.unknown("readProgramChange");
                    break;
              case MusECore::Xml::Attribut:
                    if (tag == "Channel") {
                          chan = xml.s2().toInt();
                          }
                    else if (tag == "Number") {
                          number = xml.s2().toInt();
                          }
                    break;
              case MusECore::Xml::TagEnd:
                    if (tag == "ProgramChange")
                    {
                        if((channelRequired && chan < 0) || chan == 0 || chan > MIDNAM_MAX_CHANNEL ||
                           number < 0)
                          return false;
                        if(chan < 0)
                          chan = defaultChannel;
                        else
                          --chan;
                        ev = MidiPlayEvent(time_ms, defaultPort, chan, ME_PROGRAM, number, 0);
                        return true;
                    }
              default:
                    break;
              }
        }
  return false;
}

bool readControlChange(
  MusECore::Xml& xml, MidiPlayEvent& ev, int time_ms,
  int defaultPort = 0,
  bool channelRequired = false,
  int defaultChannel = 0)
{
  int chan = -1;
  int control = -1;
  int value = -1;

  for (;;) {
        MusECore::Xml::Token token(xml.parse());
        const QString& tag(xml.s1());
        switch (token) {
              case MusECore::Xml::Error:
              case MusECore::Xml::End:
                    return false;
              case MusECore::Xml::TagStart:
                      xml.unknown("readControlChange");
                    break;
              case MusECore::Xml::Attribut:
                    if (tag == "Channel") {
                          chan = xml.s2().toInt();
                          }
                    else if (tag == "Control") {
                          control = xml.s2().toInt();
                          }
                    else if (tag == "Value") {
                          value = xml.s2().toInt();
                          }
                    break;
              case MusECore::Xml::TagEnd:
                    if (tag == "ControlChange")
                    {
                        if((channelRequired && chan < 0) || chan == 0 || chan > MIDNAM_MAX_CHANNEL ||
                           control < 0 || value < 0)
                          return false;
                        if(chan < 0)
                          chan = defaultChannel;
                        else
                          --chan;
                        ev = MidiPlayEvent(time_ms, defaultPort, chan, ME_CONTROLLER, control, value);
                        return true;
                    }
              default:
                    break;
              }
        }
  return false;
}

bool readPolyKeyPressure(
  MusECore::Xml& xml, MidiPlayEvent& ev, int time_ms,
  int defaultPort = 0,
  bool channelRequired = false,
  int defaultChannel = 0)
{
  int chan = -1;
  int note = -1;
  int pressure = -1;

  for (;;) {
        MusECore::Xml::Token token(xml.parse());
        const QString& tag(xml.s1());
        switch (token) {
              case MusECore::Xml::Error:
              case MusECore::Xml::End:
                    return false;
              case MusECore::Xml::TagStart:
                      xml.unknown("readPolyKeyPressure");
                    break;
              case MusECore::Xml::Attribut:
                    if (tag == "Channel") {
                          chan = xml.s2().toInt();
                          }
                    else if (tag == "Note") {
                          note = xml.s2().toInt();
                          }
                    else if (tag == "Pressure") {
                          pressure = xml.s2().toInt();
                          }
                    break;
              case MusECore::Xml::TagEnd:
                    if (tag == "PolyKeyPressure")
                    {
                        if((channelRequired && chan < 0) || chan == 0 || chan > MIDNAM_MAX_CHANNEL ||
                           note < 0 || pressure < 0)
                          return false;
                        if(chan < 0)
                          chan = defaultChannel;
                        else
                          --chan;
                        ev = MidiPlayEvent(time_ms, defaultPort, chan, ME_POLYAFTER, note, pressure);
                        return true;
                    }
              default:
                    break;
              }
        }
  return false;
}

bool readNoteOff(MusECore::Xml& xml, MidiPlayEvent& ev, int time_ms,
  int defaultPort = 0,
  bool channelRequired = false,
  int defaultChannel = 0)
{
  int chan = -1;
  int note = -1;
  int vel = -1;

  for (;;) {
        MusECore::Xml::Token token(xml.parse());
        const QString& tag(xml.s1());
        switch (token) {
              case MusECore::Xml::Error:
              case MusECore::Xml::End:
                    return false;
              case MusECore::Xml::TagStart:
                      xml.unknown("readNoteOff");
                    break;
              case MusECore::Xml::Attribut:
                    if (tag == "Channel") {
                          chan = xml.s2().toInt();
                          }
                    else if (tag == "Note") {
                          note = xml.s2().toInt();
                          }
                    else if (tag == "Velocity") {
                          vel = xml.s2().toInt();
                          }
                    break;
              case MusECore::Xml::TagEnd:
                    if (tag == "NoteOff")
                    {
                        if((channelRequired && chan < 0) || chan == 0 || chan > MIDNAM_MAX_CHANNEL ||
                           note < 0 || vel < 0)
                          return false;
                        if(chan < 0)
                          chan = defaultChannel;
                        else
                          --chan;
                        ev = MidiPlayEvent(time_ms, defaultPort, chan, ME_NOTEOFF, note, vel);
                        return true;
                    }
              default:
                    break;
              }
        }
  return false;
}

bool readNoteOn(MusECore::Xml& xml, MidiPlayEvent& ev, int time_ms,
  int defaultPort = 0,
  bool channelRequired = false,
  int defaultChannel = 0)
{
  int chan = -1;
  int note = -1;
  int vel = -1;

  for (;;) {
        MusECore::Xml::Token token(xml.parse());
        const QString& tag(xml.s1());
        switch (token) {
              case MusECore::Xml::Error:
              case MusECore::Xml::End:
                    return false;
              case MusECore::Xml::TagStart:
                      xml.unknown("readNoteOn");
                    break;
              case MusECore::Xml::Attribut:
                    if (tag == "Channel") {
                          chan = xml.s2().toInt();
                          }
                    else if (tag == "Note") {
                          note = xml.s2().toInt();
                          }
                    else if (tag == "Velocity") {
                          vel = xml.s2().toInt();
                          }
                    break;
              case MusECore::Xml::TagEnd:
                    if (tag == "NoteOn")
                    {
                        if((channelRequired && chan < 0) || chan == 0 || chan > MIDNAM_MAX_CHANNEL ||
                           note < 0 || vel < 0)
                          return false;
                        if(chan < 0)
                          chan = defaultChannel;
                        else
                          --chan;
                        ev = MidiPlayEvent(time_ms, defaultPort, chan, ME_NOTEON, note, vel);
                        return true;
                    }
              default:
                    break;
              }
        }
  return false;
}


//========================================================================


bool readMIDIDelay(MusECore::Xml& xml, int& delay)
{
  int ms = -1;

  for (;;) {
        MusECore::Xml::Token token(xml.parse());
        const QString& tag(xml.s1());
        switch (token) {
              case MusECore::Xml::Error:
              case MusECore::Xml::End:
                    return false;
              case MusECore::Xml::TagStart:
                      xml.unknown("readMIDIDelay");
                    break;
              case MusECore::Xml::Attribut:
                    if (tag == "Milliseconds") {
                          ms = xml.s2().toInt();
                          }
                    break;
              case MusECore::Xml::TagEnd:
                    if (tag == "MIDIDelay")
                    {
                      if(ms < 0)
                        return false;
                      delay = ms;
                      return true;
                    }
              default:
                    break;
              }
        }
  return false;
}

// Writes element name, and channel if not zero.
void writeMIDICommandElementPrefix(int level, MusECore::Xml& xml, const QString& element, int chan = 0)
{
  xml.nput(level, "<%s ", Xml::xmlString(element).toLocal8Bit().constData());
  if(chan != 0)
    // MidName Channel range is 1..16
    xml.nput(level, "Channel=\"%d\" ", chan + 1);
}

void writeMIDICommand(int level, MusECore::Xml& xml, const MidiPlayEvent& ev, int cur_time_ms)
{
  const int chan = ev.channel();
  const int type = ev.type();
  const int dataA = ev.dataA();
  const int dataB = ev.dataB();
  
  const int ev_time_ms = ev.time();
  if(ev_time_ms > cur_time_ms)
    xml.put(level, "<MIDIDelay Milliseconds=\"%d\" />", ev_time_ms - cur_time_ms);

  switch(type) {
        case ME_NOTEON:
              writeMIDICommandElementPrefix(level, xml, "NoteOn", chan);
              xml.put(level, "Note=\"%d\" Velocity=\"%d\" />", dataA & 0x7f, dataB & 0x7f);
              break;
        case ME_NOTEOFF:
              writeMIDICommandElementPrefix(level, xml, "NoteOff", chan);
              xml.put(level, "Note=\"%d\" Velocity=\"%d\" />", dataA & 0x7f, dataB & 0x7f);
              break;
        case ME_POLYAFTER:
              writeMIDICommandElementPrefix(level, xml, "PolyKeyPressure", chan);
              xml.put(level, "Note=\"%d\" Pressure=\"%d\" />", dataA & 0x7f, dataB & 0x7f);
              break;
        case ME_CONTROLLER:
              switch(dataA)
              {
                case CTRL_ALL_SOUNDS_OFF:
                  writeMIDICommandElementPrefix(level, xml, "AllSoundOff", chan);
                  xml.put(level, " />");
                break;
                case CTRL_RESET_ALL_CTRL:
                  writeMIDICommandElementPrefix(level, xml, "ResetAllControllers", chan);
                  xml.put(level, " />");
                break;
                case CTRL_LOCAL_OFF:
                  writeMIDICommandElementPrefix(level, xml, "LocalControl", chan);
                  xml.put(level, "Value=\"%d\" />", dataB & 0x7f);
                break;
                case CTRL_ALL_NOTES_OFF:
                  writeMIDICommandElementPrefix(level, xml, "AllNotesOff", chan);
                  xml.put(level, " />");
                break;
                case CTRL_OMNI_MODE_OFF:
                  writeMIDICommandElementPrefix(level, xml, "OmniOff", chan);
                  xml.put(level, " />");
                break;
                case CTRL_OMNI_MODE_ON:
                  writeMIDICommandElementPrefix(level, xml, "OmniOn", chan);
                  xml.put(level, " />");
                break;
                case CTRL_MONO_MODE_ON:
                  writeMIDICommandElementPrefix(level, xml, "MonoMode", chan);
                  xml.put(level, "Value=\"%d\" />", dataB & 0x7f);
                break;
                case CTRL_POLY_MODE_ON:
                  writeMIDICommandElementPrefix(level, xml, "PolyMode", chan);
                  xml.put(level, " />");
                break;

                default:
                  // 14-bit control changes:
                  // Value is 0..16383
                  // ControlChange14's Control is MSB 0..31
                  // RPNN and NRPN are 0..16383

                  if (dataA < CTRL_14_OFFSET) {
                        // 7 Bit Controller
                        writeMIDICommandElementPrefix(level, xml, "ControlChange", chan);
                        xml.put(level, "Control=\"%d\" Value=\"%d\" />", dataA & 0x7f, dataB & 0x7f);
                  }
                  else if (dataA < CTRL_RPN_OFFSET) {
                        // 14 bit high resolution controller
                        const int dt = (dataA >> 8) & 0x7f;
                        // NOTE: This does not support our own separate high and low control numbers,
                        //        only standard MSB and implied LSB. Use our MSB but discard the LSB.
                        if(dt < 0x20)
                        {
                          writeMIDICommandElementPrefix(level, xml, "ControlChange14", chan);
                          xml.put(level, "Control=\"%d\" Value=\"%d\" />",
                                  dt, dataB & 0x7f);
                        }
                  }
                  else if (dataA < CTRL_NRPN_OFFSET) {
                        // RPN 7-Bit Controller
                        writeMIDICommandElementPrefix(level, xml, "RPNChange", chan);
                        xml.put(level, "RPN=\"%d\" Value=\"%d\" />",
                                (((dataA & 0x7f00) >> 1) | (dataA & 0x7f)) , dataB & 0x7f);
                  }
                  else if (dataA < CTRL_INTERNAL_OFFSET) {
                        // NRPN 7-Bit Controller
                        writeMIDICommandElementPrefix(level, xml, "NRPNChange", chan);
                        xml.put(level, "NRPN=\"%d\" Value=\"%d\" />",
                                (((dataA & 0x7f00) >> 1) | (dataA & 0x7f)) , dataB & 0x7f);
                  }
                  else if (dataA == CTRL_PITCH) {
                        writeMIDICommandElementPrefix(level, xml, "PitchBendChange", chan);
                        // Our MPEvent range is -8192..8191 in data B.
                        // PitchBend's Value is 0..16383
                        xml.put(level, "Value=\"%d\" />", dataB + 8192);
                  }
                  else if (dataA == CTRL_PROGRAM) {
                        const int hb = (dataB >> 16) & 0xff;
                        const int lb = (dataB >> 8) & 0xff;
                        const int pr = dataB & 0xff;
                        if (hb != 0xff) {
                          writeMIDICommandElementPrefix(level, xml, "ControlChange", chan);
                          xml.put(level, "Control=\"%d\" Value=\"%d\" />", CTRL_HBANK, hb);
                        }
                        if (lb != 0xff) {
                          writeMIDICommandElementPrefix(level, xml, "ControlChange", chan);
                          xml.put(level, "Control=\"%d\" Value=\"%d\" />", CTRL_LBANK, lb);
                        }
                        if (pr != 0xff) {
                          writeMIDICommandElementPrefix(level, xml, "ProgramChange", chan);
                          xml.put(level, "Number=\"%d\" />", pr);
                        }
                  }
                  else if ((dataA | 0xff) == CTRL_POLYAFTER) {
                        writeMIDICommandElementPrefix(level, xml, "PolyKeyPressure", chan);
                        xml.put(level, "Note=\"%d\" Pressure=\"%d\" />", dataA & 0x7f, dataB & 0x7f);
                  }
                  else if (dataA == CTRL_AFTERTOUCH) {
                        writeMIDICommandElementPrefix(level, xml, "ChannelKeyPressure", chan);
                        xml.put(level, "Pressure=\"%d\" />", dataB & 0x7f);
                  }
// UNSUPPORTED:
//                   else if (dataA < CTRL_NRPN14_OFFSET) {
//                         // RPN14 Controller
//                         writeMIDICommandElementPrefix(level, xml, "ControlChange", chan);
//                         xml.put(level, "Control=\"%d\" Value=\"%d\" />", dataA & 0x7f, dataB & 0x7f);
//                   }
//                   else if (dataA < CTRL_NONE_OFFSET) {
//                         // NRPN14 Controller
//                         writeMIDICommandElementPrefix(level, xml, "ControlChange", chan);
//                         xml.put(level, "Control=\"%d\" Value=\"%d\" />", dataA & 0x7f, dataB & 0x7f);
//                   }

                  else {
                        writeMIDICommandElementPrefix(level, xml, "ControlChange", chan);
                        xml.put(level, "Control=\"%d\" Value=\"%d\" />", dataA & 0x7f, dataB & 0x7f);
                  }
              }
              break;
        case ME_PROGRAM:
              writeMIDICommandElementPrefix(level, xml, "ProgramChange", chan);
              xml.put(level, "Number=\"%d\" />", dataA & 0x7f);
              break;
        case ME_AFTERTOUCH:
              writeMIDICommandElementPrefix(level, xml, "ChannelKeyPressure", chan);
              xml.put(level, "Pressure=\"%d\" />", dataA & 0x7f);
              break;
        case ME_PITCHBEND:
              writeMIDICommandElementPrefix(level, xml, "PitchBendChange", chan);
              // Our MPEvent range is -8192..8191 in data A.
              // PitchBend's Value is 0..16383
              xml.put(level, "Value=\"%d\" />", dataA + 8192);
              break;

        case ME_SYSEX:
              {
                // TODO: Insert SysExDeviceID and (rare, old) SysExChannel tags.
                //       We ignore them and do not generate them for now.
                int len = ev.len();
                if(len > 0)
                {
                  const unsigned char* data = ev.constData();
                  // Do we want to start it on a new line? Nah, not here...
                  //xml.put(level, "<SysEx> ");
                  //xml.nput(level, "");
                  xml.nput(level, "<SysEx> ");
                  for (int i = 0; i < len; ++i) {
                        if (i && ((i % 16) == 0)) {
                              xml.nput("\n");
                              xml.nput(level, "");
                              }
                        xml.nput("%02x ", data[i] & 0xff);
                        }
                  xml.nput("\n");
                  xml.etag(level, "SysEx");
                }
              }
              break;

        case ME_MTC_QUARTER:
              // TODO: MTCQuarterFrame
              break;
              
        case ME_SONGPOS:
              xml.put(level, "<SongPositionPointer Position=\"%d\" />", dataA & 0x3fff);
              break;
        case ME_SONGSEL:
              xml.put(level, "<SongSelect Number=\"%d\" />", dataA & 0x7f);
              break;
        case ME_TUNE_REQ:
              xml.put(level, "<TuneRequest />");
              break;
        case ME_CLOCK:
              xml.put(level, "<TimingClock />");
              break;
        case ME_START:
              xml.put(level, "<Start />");
              break;
        case ME_CONTINUE:
              xml.put(level, "<Continue />");
              break;
        case ME_STOP:
              xml.put(level, "<Stop />");
              break;
        case ME_SENSE:
              xml.put(level, "<ActiveSensing />");
              break;
        case ME_RESET:
              xml.put(level, "<SystemReset />");
              break;
        default:
              fprintf(stderr, "writeMIDICommand event type %x not supported\n", ev.type());
              break;
        }
}


void MidiNamMIDICommands::write(int level, MusECore::Xml& xml) const
{
  if(empty())
    return;
  int time_ms = 0;
  xml.tag(level++, _isPatchMIDICommands ? "PatchMIDICommands" : "MIDICommands");
  for(ciMPEvent i = cbegin(); i != cend(); ++i)
  {
    writeMIDICommand(level, xml, *i, time_ms);
    time_ms += i->time();
  }
  xml.etag(--level, _isPatchMIDICommands ? "PatchMIDICommands" : "MIDICommands");
}


bool MidiNamMIDICommands::read(
  MusECore::Xml& xml,
  bool includeSysEx,
  int defaultPort,
  bool channelRequired,
  int defaultChannel)
{
  int time_ms = 0;

  for (;;) {
        MusECore::Xml::Token token(xml.parse());
        const QString& tag(xml.s1());
        switch (token) {
              case MusECore::Xml::Error:
              case MusECore::Xml::End:
                    return false;
              case MusECore::Xml::TagStart:
                    {
                        MidiPlayEvent ev;
                        if (tag == "MIDIDelay") {
                          int d;
                          if(readMIDIDelay(xml, d))
                            time_ms += d;
                        }
                        else if (tag == "NoteOn") {
                          if(readNoteOn(xml, ev, time_ms, defaultPort, channelRequired, defaultChannel))
                            add(ev);
                        }
                        else if (tag == "NoteOff") {
                          if(readNoteOff(xml, ev, time_ms, defaultPort, channelRequired, defaultChannel))
                            add(ev);
                        }
                        else if (tag == "PolyKeyPressure") {
                          if(readPolyKeyPressure(xml, ev, time_ms, defaultPort, channelRequired, defaultChannel))
                            add(ev);
                        }
                        else if (tag == "ControlChange") {
                          if(readControlChange(xml, ev, time_ms, defaultPort, channelRequired, defaultChannel))
                          {
                            // If it is a bank high or low controller,
                            //  remember the values.
                            if(ev.type() == ME_CONTROLLER)
                            {
                              const int ctl = ev.dataA();
                              const int val = ev.dataB();
                              if(ctl == CTRL_HBANK)
                              {
                                _bankH = val & 0x7f;
                                _hasBankH = true;
                              }
                              if(ctl == CTRL_LBANK)
                              {
                                _bankL = val & 0x7f;
                                _hasBankL = true;
                              }
                            }
                            add(ev);
                          }
                        }
                        else if (tag == "ProgramChange") {
                          if(readProgramChange(xml, ev, time_ms, defaultPort, channelRequired, defaultChannel))
                            add(ev);
                        }
                        else if (tag == "ChannelKeyPressure") {
                          if(readChannelKeyPressure(xml, ev, time_ms, defaultPort, channelRequired, defaultChannel))
                            add(ev);
                        }
                        else if (tag == "PitchBendChange") {
                          if(readPitchBendChange(xml, ev, time_ms, defaultPort, channelRequired, defaultChannel))
                            add(ev);
                        }

                        else if (tag == "AllSoundOff") {
                          if(readAllSoundOff(xml, ev, time_ms, defaultPort, channelRequired, defaultChannel))
                            add(ev);
                        }
                        else if (tag == "ResetAllControllers") {
                          if(readResetAllControllers(xml, ev, time_ms, defaultPort, channelRequired, defaultChannel))
                            add(ev);
                        }
                        else if (tag == "LocalControl") {
                          if(readLocalControl(xml, ev, time_ms, defaultPort, channelRequired, defaultChannel))
                            add(ev);
                        }
                        else if (tag == "AllNotesOff") {
                          if(readAllNotesOff(xml, ev, time_ms, defaultPort, channelRequired, defaultChannel))
                            add(ev);
                        }
                        else if (tag == "OmniOff") {
                          if(readOmniOff(xml, ev, time_ms, defaultPort, channelRequired, defaultChannel))
                            add(ev);
                        }
                        else if (tag == "OmniOn") {
                          if(readOmniOn(xml, ev, time_ms, defaultPort, channelRequired, defaultChannel))
                            add(ev);
                        }
                        else if (tag == "MonoMode") {
                          if(readMonoMode(xml, ev, time_ms, defaultPort, channelRequired, defaultChannel))
                            add(ev);
                        }
                        else if (tag == "PolyMode") {
                          if(readPolyMode(xml, ev, time_ms, defaultPort, channelRequired, defaultChannel))
                            add(ev);
                        }

                        else if (tag == "ControlChange14") {
                          if(readControlChange14(xml, ev, time_ms, defaultPort, channelRequired, defaultChannel))
                            add(ev);
                        }
                        else if (tag == "RPNChange") {
                          if(readRPNChange(xml, ev, time_ms, defaultPort, channelRequired, defaultChannel))
                            add(ev);
                        }
                        else if (tag == "NRPNChange") {
                          if(readNRPNChange(xml, ev, time_ms, defaultPort, channelRequired, defaultChannel))
                            add(ev);
                        }

                        else if (tag == "SysEx" && includeSysEx) {
                          if(readSysEx(xml, ev, time_ms, defaultPort, defaultChannel))
                            add(ev);
                        }

                        // TODO
                        //else if (tag == "MTCQuarterFrame") {
                        //  if(readMTCQuarterFrame(xml, ev, time_ms, defaultPort))
                        //    add(ev);
                        //}
                        else if (tag == "SongPositionPointer") {
                          if(readSongPositionPointer(xml, ev, time_ms, defaultPort))
                            add(ev);
                        }
                        else if (tag == "SongSelect") {
                          if(readSongSelect(xml, ev, time_ms, defaultPort))
                            add(ev);
                        }
                        else if (tag == "TuneRequest") {
                          if(readTuneRequest(xml, ev, time_ms, defaultPort))
                            add(ev);
                        }
                        else if (tag == "TimingClock") {
                          if(readTimingClock(xml, ev, time_ms, defaultPort))
                            add(ev);
                        }
                        else if (tag == "Start") {
                          if(readStart(xml, ev, time_ms, defaultPort))
                            add(ev);
                        }
                        else if (tag == "Continue") {
                          if(readContinue(xml, ev, time_ms, defaultPort))
                            add(ev);
                        }
                        else if (tag == "Stop") {
                          if(readStop(xml, ev, time_ms, defaultPort))
                            add(ev);
                        }
                        else if (tag == "ActiveSensing") {
                          if(readActiveSensing(xml, ev, time_ms, defaultPort))
                            add(ev);
                        }
                        else if (tag == "SystemReset") {
                          if(readSystemReset(xml, ev, time_ms, defaultPort))
                            add(ev);
                        }
                        else
                          xml.unknown("readMIDICommands");
                    }
                    break;
              case MusECore::Xml::Attribut:
                    break;
              case MusECore::Xml::TagEnd:
                    if (tag == "MIDICommands") {
                        _isPatchMIDICommands = false;
                        return true;
                    }
                    else if (tag == "PatchMIDICommands") {
                        _isPatchMIDICommands = true;
                        return true;
                    }
              default:
                    break;
              }
        }
  return false;
}



//========================================================================



void MidiNamAvailableChannel::write(int level, MusECore::Xml& xml) const
{
  // MidNam Channel range is 1..16
  xml.put(level, "<AvailableChannel Channel=\"%d\" Available=\"%s\" />",
          _channel + 1, _available ? "true" : "false");
}

bool MidiNamAvailableChannel::read(MusECore::Xml& xml)
{
  int chan = -1;
  // Default is true.
  bool available = true;

  for (;;) {
        MusECore::Xml::Token token(xml.parse());
        const QString& tag(xml.s1());
        switch (token) {
              case MusECore::Xml::Error:
              case MusECore::Xml::End:
                    return false;
              case MusECore::Xml::TagStart:
                      xml.unknown("MidiNamAvailableChannel");
                    break;
              case MusECore::Xml::Attribut:
                    if (tag == "Channel") {
                          chan = xml.s2().toInt();
                          }
                    else if (tag == "Available") {
                          const QString& avail = xml.s2();
                          if(avail.compare("true", Qt::CaseInsensitive) == 0)
                            available = true;
                          else if(avail.compare("false", Qt::CaseInsensitive) == 0)
                            available = false;
                          }
                    break;
              case MusECore::Xml::TagEnd:
                    if (tag == "AvailableChannel")
                    {
                      // Channel range is 1 .. 16
                      if(chan <= 0 || chan > MIDNAM_MAX_CHANNEL)
                        return false;
                      --chan;
                      _channel = chan;
                      _available = available;
                      return true;
                    }
              default:
                    break;
              }
        }
  return false;
}


//----------------------------------------------------------------


bool MidiNamAvailableForChannels::add(const MidiNamAvailableChannel& a)
{
  return insert(MidiNamAvailableForChannelsPair(a.channel(), a)).second;
}

void MidiNamAvailableForChannels::write(int level, MusECore::Xml& xml) const
{
  if(empty())
    return;
  xml.tag(level++, "AvailableForChannels");
  for(const_iterator i = cbegin(); i != cend(); ++i)
    i->second.write(level, xml);
  xml.etag(--level, "AvailableForChannels");
}

void MidiNamAvailableForChannels::read(MusECore::Xml& xml)
      {
      for (;;) {
            MusECore::Xml::Token token(xml.parse());
            const QString& tag(xml.s1());
            switch (token) {
                  case MusECore::Xml::Error:
                  case MusECore::Xml::End:
                        return;
                  case MusECore::Xml::TagStart:
                        if (tag == "AvailableChannel")
                        {
                          MidiNamAvailableChannel n;
                          if(n.read(xml))
                            add(n);
                        }
                        else
                          xml.unknown("MidiNamAvailableForChannels");
                        break;
                  case MusECore::Xml::Attribut:
                        break;
                  case MusECore::Xml::TagEnd:
                        if (tag == "AvailableForChannels")
                            return;
                  default:
                        break;
                  }
            }
      
      }


//----------------------------------------------------------------


void MidiNamChannelNameSetAssign::write(int level, MusECore::Xml& xml) const
{
  // MidNam Channel range is 1..16
  xml.put(level, "<ChannelNameSetAssign Channel=\"%d\" NameSet=\"%s\" />",
          _channel + 1, Xml::xmlString(_name).toLocal8Bit().constData());
}

bool MidiNamChannelNameSetAssign::read(MusECore::Xml& xml)
{
  int chan = -1;
  QString nameset;

  for (;;) {
        MusECore::Xml::Token token(xml.parse());
        const QString& tag(xml.s1());
        switch (token) {
              case MusECore::Xml::Error:
              case MusECore::Xml::End:
                    return false;
              case MusECore::Xml::TagStart:
                      xml.unknown("MidiNamChannelNameSetAssign");
                    break;
              case MusECore::Xml::Attribut:
                    if (tag == "Channel") {
                          chan = xml.s2().toInt();
                          }
                    else if (tag == "NameSet") {
                          nameset = xml.s2();
                          }
                    break;
              case MusECore::Xml::TagEnd:
                    if (tag == "ChannelNameSetAssign")
                    {
                      // Channel range is 1 .. 16
                      if(chan <= 0 || chan > MIDNAM_MAX_CHANNEL || nameset.isEmpty())
                        return false;
                      --chan;
                      _channel = chan;
                      _name = nameset;
                      return true;
                    }
              default:
                    break;
              }
        }
  return false;
}

bool MidiNamChannelNameSetAssign::gatherReferences(MidNamReferencesList* refs) const
{
  return refs->channelNameSetAssignObjs.add(const_cast<MidiNamChannelNameSetAssign*>(this));
}

const MidiNamPatch* MidiNamChannelNameSetAssign::findPatch(int channel, int patch) const
{
  // Go directly to the reference object.
  if(!_p_ref)
    return nullptr;
  return _p_ref->findPatch(channel, patch);
}

const MidiNamPatchBankList* MidiNamChannelNameSetAssign::getPatchBanks(int channel) const
{
  // Go directly to the reference object.
  if(!_p_ref)
    return nullptr;
  return _p_ref->getPatchBanks(channel);
}

bool MidiNamChannelNameSetAssign::getNoteSampleName(
  bool drum, int channel, int patch, int note, QString* name) const
{
  if(!name)
    return false;
  // Go directly to the reference object.
  if(_p_ref)
    return _p_ref->getNoteSampleName(drum, channel, patch, note, name);
  return false;
}


//----------------------------------------------------------------

bool MidiNamChannelNameSetAssignments::add(const MidiNamChannelNameSetAssign& a)
{
  return insert(MidiNamChannelNameSetAssignmentsPair(a.channel(), a)).second;
}

void MidiNamChannelNameSetAssignments::write(int level, MusECore::Xml& xml) const
{
  if(empty())
    return;
  xml.tag(level++, "ChannelNameSetAssignments");
  for(const_iterator i = cbegin(); i != cend(); ++i)
    i->second.write(level, xml);
  xml.etag(--level, "ChannelNameSetAssignments");
}

void MidiNamChannelNameSetAssignments::read(MusECore::Xml& xml)
      {
      for (;;) {
            MusECore::Xml::Token token(xml.parse());
            const QString& tag(xml.s1());
            switch (token) {
                  case MusECore::Xml::Error:
                  case MusECore::Xml::End:
                        return;
                  case MusECore::Xml::TagStart:
                        if (tag == "ChannelNameSetAssign")
                        {
                          MidiNamChannelNameSetAssign n;
                          if(n.read(xml))
                            add(n);
                        }
                        else
                          xml.unknown("MidiNamChannelNameSetAssignments");
                        break;
                  case MusECore::Xml::Attribut:
                        break;
                  case MusECore::Xml::TagEnd:
                        if (tag == "ChannelNameSetAssignments")
                        {
                            _hasChannelNameSetAssignments = true;
                            return;
                        }
                  default:
                        break;
                  }
            }
      
      }

bool MidiNamChannelNameSetAssignments::gatherReferences(MidNamReferencesList* refs) const
{
  for(const_iterator i = cbegin(); i != cend(); ++i)
    i->second.gatherReferences(refs);
  return true;
}

const MidiNamPatch* MidiNamChannelNameSetAssignments::findPatch(int channel, int patch) const
{
  if(!_hasChannelNameSetAssignments)
    return nullptr;

  const_iterator ia = find(channel);
  if(ia == cend())
    return nullptr;
    
  const MidiNamChannelNameSetAssign& cns = ia->second;
  return cns.findPatch(channel, patch);
}

const MidiNamPatchBankList* MidiNamChannelNameSetAssignments::getPatchBanks(int channel) const
{
  if(!_hasChannelNameSetAssignments)
    return nullptr;

  const_iterator ia = find(channel);
  if(ia == cend())
    return nullptr;
    
  const MidiNamChannelNameSetAssign& cns = ia->second;
  return cns.getPatchBanks(channel);
}

bool MidiNamChannelNameSetAssignments::getNoteSampleName(
  bool drum, int channel, int patch, int note, QString* name) const
{
  if(!name)
    return false;

  if(_hasChannelNameSetAssignments)
  {
    const_iterator ia = find(channel);
    if(ia != cend())
    {
      const MidiNamChannelNameSetAssign& cns = ia->second;
      return cns.getNoteSampleName(drum, channel, patch, note, name);
    }
  }

  return false;
}


//----------------------------------------------------------------


void MidiNamNote::write(int level, MusECore::Xml& xml) const
{
  xml.put(level, "<Note Number=\"%d\" Name=\"%s\" />", _number, Xml::xmlString(_name).toLocal8Bit().constData());
}

bool MidiNamNote::read(MusECore::Xml& xml)
{
  int number = -1;
  QString name;

  for (;;) {
        MusECore::Xml::Token token(xml.parse());
        const QString& tag(xml.s1());
        switch (token) {
              case MusECore::Xml::Error:
              case MusECore::Xml::End:
                    return false;
              case MusECore::Xml::TagStart:
                      xml.unknown("MidiNamNote");
                    break;
              case MusECore::Xml::Attribut:
                    if (tag == "Number") {
                          number = xml.s2().toInt();
                          }
                    else if (tag == "Name") {
                          name = xml.s2();
                          }
                    break;
              case MusECore::Xml::TagEnd:
                    if (tag == "Note")
                    {
                      if(number < 0 || name.isEmpty())
                        return false;
                      _number = number;
                      _name = name;
                      return true;
                    }
              default:
                    break;
              }
        }
  return false;
}

//----------------------------------------------------------------


bool MidiNamNotes::add(const MidiNamNote& a)
{
  return insert(MidiNamNotesPair(a.number(), a)).second;
}

bool MidiNamNotes::addNoteGroup(const MidiNamNoteGroup& a)
{
  return _noteGroups.add(a);
}

void MidiNamNotes::write(int level, MusECore::Xml& xml) const
{
  _noteGroups.write(level, xml, this);

  for(const_iterator i = cbegin(); i != cend(); ++i)
  {
    // If the note is part of a group, the group will take care of writing it instead.
    const int note = i->second.number();
    ciMidiNamNoteGroups ig = _noteGroups.cbegin();
    for( ; ig != _noteGroups.cend(); ++ig)
    {
      if(ig->second.find(note) != ig->second.cend())
        break;
    }
    if(ig != _noteGroups.cend())
      continue;
    i->second.write(level, xml);
  }
}

bool MidiNamNotes::getNoteSampleName(
  bool /*drum*/, int /*channel*/, int /*patch*/, int note, QString* name) const
{
  if(!name)
    return false;

  const_iterator i = find(note);
  if(i == cend())
  {
    // It has a note name list but no note was found.
    *name = QString();
    return true;
  }

  *name = i->second.name();

  return true;
}


//----------------------------------------------------------------


void MidiNamNoteGroup::write(int level, MusECore::Xml& xml, const MidiNamNotes* notes) const
{
  xml.tag(level++, "NoteGroup Name=\"%s\"", Xml::xmlString(_name).toLocal8Bit().constData());
  for(const_iterator i = cbegin(); i != cend(); ++i)
  {
    // Write only the notes that are part of the group.
    ciMidiNamNotes in = notes->find(*i);
    if(in == notes->cend())
      continue;
    in->second.write(level, xml);
  }
  xml.etag(--level, "NoteGroup");
}

void MidiNamNoteGroup::read(MusECore::Xml& xml, MidiNamNotes* notes)
      {
      for (;;) {
            MusECore::Xml::Token token(xml.parse());
            const QString& tag(xml.s1());
            switch (token) {
                  case MusECore::Xml::Error:
                  case MusECore::Xml::End:
                        return;
                  case MusECore::Xml::TagStart:
                        if (tag == "Note")
                        {
                          MidiNamNote n;
                          if(n.read(xml))
                          {
                            if(notes->add(n))
                              insert(n.number());
                          }
                        }
                        else
                          xml.unknown("MidiNamNoteGroup");
                        break;
                  case MusECore::Xml::Attribut:
                        if (tag == "Name") {
                          _name = xml.s2();
                          }
                        break;
                  case MusECore::Xml::TagEnd:
                        if (tag == "NoteGroup")
                            return;
                  default:
                        break;
                  }
            }
      
      }


//----------------------------------------------------------------


bool MidiNamNoteGroups::add(const MidiNamNoteGroup& a)
{
  insert(MidiNamNoteGroupsPair(a.name(), a));
  return true;
}

void MidiNamNoteGroups::write(int level, MusECore::Xml& xml, const MidiNamNotes* notes) const
{
  for(const_iterator i = cbegin(); i != cend(); ++i)
    i->second.write(level, xml, notes);
}


//----------------------------------------------------------------


void MidNamNoteNameList::write(int level, MusECore::Xml& xml) const
{
  if(isReference()) {
    xml.put(level, "<UsesNoteNameList Name=\"%s\" />", Xml::xmlString(_name).toLocal8Bit().constData());
  }
  else {
    xml.tag(level++, "NoteNameList Name=\"%s\"", Xml::xmlString(_name).toLocal8Bit().constData());
    _noteList.write(level, xml);
    xml.etag(--level, "NoteNameList");
  }
}

bool MidNamNoteNameList::read(MusECore::Xml& xml)
{
  for (;;) {
        MusECore::Xml::Token token(xml.parse());
        const QString& tag(xml.s1());
        switch (token) {
              case MusECore::Xml::Error:
              case MusECore::Xml::End:
                    return false;
              case MusECore::Xml::TagStart:
                    if (tag == "NoteGroup")
                    {
                      MidiNamNoteGroup n;
                      n.read(xml, &_noteList);
                      _noteList.addNoteGroup(n);
                    }
                    else if (tag == "Note")
                    {
                      MidiNamNote n;
                      if(n.read(xml))
                        _noteList.add(n);
                    }
                    else
                      xml.unknown("MidNamNoteNameList");
                    break;
              case MusECore::Xml::Attribut:
                    if (tag == "Name") {
                          _name = xml.s2();
                          }
                    break;
              case MusECore::Xml::TagEnd:
                    if (tag == "NoteNameList")
                    {
                      _isReference = false;
                      // This is an actual note name list,
                      //  regardless if it is empty.
                      _hasNoteNameList = true;
                      return true;
                    }
                    else if (tag == "UsesNoteNameList")
                    {
                      _isReference = true;
                      return true;
                    }
              default:
                    break;
              }
        }
  return false;
}

bool MidNamNoteNameList::gatherReferences(MidNamReferencesList* refs) const
{
  return refs->noteNameListObjs.add(const_cast<MidNamNoteNameList*>(this));
}

bool MidNamNoteNameList::getNoteSampleName(
  bool drum, int channel, int patch, int note, QString* name) const
{
  if(!name)
    return false;

  if(!_hasNoteNameList)
    return false;

  return _noteList.getNoteSampleName(drum, channel, patch, note, name);
}


//----------------------------------------------------------------


void MidiNamVal::write(int level, MusECore::Xml& xml) const
{
  xml.put(level, "<Value Number=\"%d\" Name=\"%s\" />", _number, Xml::xmlString(_name).toLocal8Bit().constData());
}

bool MidiNamVal::read(MusECore::Xml& xml)
{
  int number = -1;
  QString name;
  for (;;) {
        MusECore::Xml::Token token(xml.parse());
        const QString& tag(xml.s1());
        switch (token) {
              case MusECore::Xml::Error:
              case MusECore::Xml::End:
                    return false;
              case MusECore::Xml::TagStart:
                      xml.unknown("MidiNamVal");
                    break;
              case MusECore::Xml::Attribut:
                    if (tag == "Number") {
                          number = xml.s2().toInt();
                          }
                    else if (tag == "Name") {
                          name = xml.s2();
                          }
                    break;
              case MusECore::Xml::TagEnd:
                    if (tag == "Value")
                    {
                      if(number < 0 || name.isEmpty())
                        return false;
                      _number = number;
                      _name = name;
                      return true;
                    }
              default:
                    break;
              }
        }
  return false;
}

//----------------------------------------------------------------


void MidiNamValNames::write(int level, MusECore::Xml& xml) const
{
  if(isReference()) {
    xml.put(level, "<UsesValueNameList Name=\"%s\" />", Xml::xmlString(_name).toLocal8Bit().constData());
  }
  else {
    xml.tag(level++, "ValueNameList Name=\"%s\"", Xml::xmlString(_name).toLocal8Bit().constData());
    for(const_iterator i = cbegin(); i != cend(); ++i)
      (*i).write(level, xml);
    xml.etag(--level, "ValueNameList");
  }
}

void MidiNamValNames::read(MusECore::Xml& xml)
      {
      for (;;) {
            MusECore::Xml::Token token(xml.parse());
            const QString& tag(xml.s1());
            switch (token) {
                  case MusECore::Xml::Error:
                  case MusECore::Xml::End:
                        return;
                  case MusECore::Xml::TagStart:
                        if (tag == "Value")
                        {
                          MidiNamVal n;
                          if(n.read(xml))
                            insert(n);
                        }
                        else
                          xml.unknown("MidiNamValNames");
                        break;
                  case MusECore::Xml::Attribut:
                        if (tag == "Name") {
                              _name = xml.s2();
                              }
                        break;
                  case MusECore::Xml::TagEnd:
                        if (tag == "ValueNameList") {
                            _isReference = false;
                            return;
                        }
                        else if (tag == "UsesValueNameList") {
                            _isReference = true;
                            return;
                        }
                  default:
                        break;
                  }
            }
      
      }

bool MidiNamValNames::gatherReferences(MidNamReferencesList* refs) const
{
  return refs->valNamesObjs.add(const_cast<MidiNamValNames*>(this));
}


//----------------------------------------------------------------


void MidiNamValues::write(int level, MusECore::Xml& xml) const
{
  xml.nput(level, "<Values Min=\"%d\" Max=\"%d\"", _min, _max);

  if(_default != 0)
    xml.nput(level, " Default=\"%d\"", _default);
  if(_units != 0)
    xml.nput(level, " Units=\"%d\"", _units);
  if(_mapping != 0)
    xml.nput(level, " Mapping=\"%d\"", _mapping);
  
  if(_valueNames.empty())
  {
    xml.put(level, " />");
  }
  else
  {
    xml.put(level++, " >");

    _valueNames.write(level, xml);
    
    xml.etag(--level, "Values");
  }
}

bool MidiNamValues::read(MusECore::Xml& xml)
      {
      bool have_min = false;
      bool have_max = false;
      int min = 0;
      int max = 0;

      for (;;) {
            MusECore::Xml::Token token(xml.parse());
            const QString& tag(xml.s1());
            switch (token) {
                  case MusECore::Xml::Error:
                  case MusECore::Xml::End:
                        return false;
                  case MusECore::Xml::TagStart:
                        if (tag == "ValueNameList")
                          _valueNames.read(xml);
                        else if (tag == "UsesValueNameList")
                          _valueNames.read(xml);
                        else
                          xml.unknown("MidiNamValues");
                        break;
                  case MusECore::Xml::Attribut:
                        if (tag == "Min") {
                              have_min = true;
                              min = xml.s2().toInt();
                              }
                        else if (tag == "Max") {
                              have_max = true;
                              max = xml.s2().toInt();
                              }
                        else if (tag == "Default") {
                              _default = xml.s2().toInt();
                              }
                        else if (tag == "Units") {
                              _units = xml.s2().toInt();
                              }
                        else if (tag == "Mapping") {
                              _mapping = xml.s2().toInt();
                              }
                        break;
                  case MusECore::Xml::TagEnd:
                        if (tag == "Values")
                        {
                            if(!have_min || !have_max)
                              return false;
                            _min = min;
                            _max = max;
                            return true;
                        }
                  default:
                        break;
                  }
            }

        return false;
      }

      
//----------------------------------------------------------------

      
void MidiNamCtrl::write(int level, MusECore::Xml& xml) const
{
  const char* type_str = "7bit";
  switch(_type)
  {
    case SevenBit:
      type_str = "7bit";
    break;
    case FourteenBit:
      type_str = "14bit";
    break;
    case RPN:
      type_str = "RPN";
    break;
    case NRPN:
      type_str = "NRPN";
    break;
  }
  xml.nput(level, "<Control Type=\"%s\" Number=\"%d\" Name=\"%s\"",
          type_str,
          _number,
          Xml::xmlString(_name).toLocal8Bit().constData());

  if(_values.empty())
  {
    xml.put(level, " />");
  }
  else
  {
    xml.put(level++, " >");

    _values.write(level, xml);
    
    xml.etag(--level, "Control");
  }
}

bool MidiNamCtrl::read(MusECore::Xml& xml)
      {
      Type type = SevenBit;
      int number = -1;
      QString name;

      for (;;) {
            MusECore::Xml::Token token(xml.parse());
            const QString& tag(xml.s1());
            switch (token) {
                  case MusECore::Xml::Error:
                  case MusECore::Xml::End:
                        return false;
                  case MusECore::Xml::TagStart:
                        if (tag == "Values")
                        {
                          MidiNamValues n;
                          if(n.read(xml))
                            _values = n;
                        }
                        else
                          xml.unknown("MidiNamCtrl");
                        break;
                  case MusECore::Xml::Attribut:
                        if (tag == "Type") {
                              const QString& type_txt = xml.s2();
                              if(type_txt.compare("7bit", Qt::CaseInsensitive) == 0)
                                type = SevenBit;
                              else if(type_txt.compare("14bit", Qt::CaseInsensitive) == 0)
                                type = FourteenBit;
                              else if(type_txt.compare("RPN", Qt::CaseInsensitive) == 0)
                                type = RPN;
                              else if(type_txt.compare("NRPN", Qt::CaseInsensitive) == 0)
                                type = NRPN;
                              }
                        else if (tag == "Number") {
                              number = xml.s2().toInt();
                              }
                        else if (tag == "Name") {
                              name = xml.s2();
                              }
                        break;
                  case MusECore::Xml::TagEnd:
                        if (tag == "Control")
                        {
                            if(number < 0 || name.isEmpty())
                              return false;
                            _number = number;
                            _name = name;
                            _type = type;
                            return true;
                        }
                  default:
                        break;
                  }
            }
      
        return false;
      }


//----------------------------------------------------------------

      
void MidiNamCtrls::write(int level, MusECore::Xml& xml) const
{
  if(isReference()) {
    xml.put(level, "<UsesControlNameList Name=\"%s\" />", Xml::xmlString(_name).toLocal8Bit().constData());
  }
  else {
    xml.tag(level++, "ControlNameList Name=\"%s\"", Xml::xmlString(_name).toLocal8Bit().constData());
    for(const_iterator i = cbegin(); i != cend(); ++i)
      (*i).write(level, xml);
    xml.etag(--level, "ControlNameList");
  }
}

void MidiNamCtrls::read(MusECore::Xml& xml)
      {
      for (;;) {
            MusECore::Xml::Token token(xml.parse());
            const QString& tag(xml.s1());
            switch (token) {
                  case MusECore::Xml::Error:
                  case MusECore::Xml::End:
                        return;
                  case MusECore::Xml::TagStart:
                        if (tag == "Control")
                        {
                          MidiNamCtrl ctrl;
                          if(ctrl.read(xml))
                            insert(ctrl);
                        }
                        else
                          xml.unknown("MidiNamCtrls");
                        break;
                  case MusECore::Xml::Attribut:
                        if (tag == "Name") {
                              _name = xml.s2();
                              }
                        break;
                  case MusECore::Xml::TagEnd:
                        if (tag == "ControlNameList") {
                            _isReference = false;
                            return;
                        }
                        else if (tag == "UsesControlNameList") {
                            _isReference = true;
                            return;
                        }
                  default:
                        break;
                  }
            }
      
      }

bool MidiNamCtrls::gatherReferences(MidNamReferencesList* refs) const
{
  return refs->ctrlsObjs.add(const_cast<MidiNamCtrls*>(this));
}


//-----------------------------------------------------------


void MidiNamPatch::write(int level, MusECore::Xml& xml) const
{
  xml.nput(level, "<Patch Number=\"%s\" Name=\"%s\" ProgramChange=\"%d\"",
          Xml::xmlString(_number).toLocal8Bit().constData(),
          Xml::xmlString(_name).toLocal8Bit().constData(),
             _patchNumber);
  if(_patchMIDICommands.empty() &&
    _channelNameSetAssignments.empty() &&
    !_noteNameList.isReference() && _noteNameList.empty() &&
    !_controlNameList.isReference() && _controlNameList.empty())
  {
    xml.put(level, " />");
  }
  else
  {
    xml.put(level++, " >");

    _patchMIDICommands.write(level, xml);
    _channelNameSetAssignments.write(level, xml);
    _noteNameList.write(level, xml);
    _controlNameList.write(level, xml);

    xml.etag(--level, "Patch");
  }
}

bool MidiNamPatch::read(MusECore::Xml& xml)
{
  QString number;
  QString name;
  // Start with what we've got, ie. say from the ctor.
  int patch_num = _patchNumber;

  for (;;) {
        MusECore::Xml::Token token(xml.parse());
        const QString& tag(xml.s1());
        switch (token) {
              case MusECore::Xml::Error:
              case MusECore::Xml::End:
                    return false;
              case MusECore::Xml::TagStart:
                    if (tag == "PatchMIDICommands")
                    {
                      if(_patchMIDICommands.read(xml, false))
                      {
                        if(_patchMIDICommands.hasBankH())
                          patch_num |= 
                            (patch_num & 0x00ffff) | (_patchMIDICommands.bankH() << 16);
                        if(_patchMIDICommands.hasBankL())
                          patch_num |=
                            (patch_num & 0xff00ff) | (_patchMIDICommands.bankL() << 8);
                      }
                    }
                    else if (tag == "ChannelNameSetAssignments")
                      _channelNameSetAssignments.read(xml);

                    else if (tag == "UsesNoteNameList")
                      _noteNameList.read(xml);
                    else if (tag == "NoteNameList")
                      _noteNameList.read(xml);

                    else if (tag == "UsesControlNameList")
                      _controlNameList.read(xml);
                    else if (tag == "ControlNameList")
                      _controlNameList.read(xml);

                    else
                      xml.unknown("MidiNamPatch");
                    break;
              case MusECore::Xml::Attribut:
                    if (tag == "Number") {
                          number = xml.s2();
                          }
                    else if (tag == "Name") {
                          name = xml.s2();
                          }
                    else if (tag == "ProgramChange") {
                          // Set high and low bank to 'unknown' (default) at first.
                          patch_num = 0xffff00 | (xml.s2().toInt() & 0x7f);
                          }
                    break;
              case MusECore::Xml::TagEnd:
                    if (tag == "Patch")
                    {
                      if(number.isEmpty() || 
                         name.isEmpty()
                         // # IMPLIED
                         /*|| patch_num < 0*/)
                        return false;
                      _number = number;
                      _name = name;
                      _patchNumber = patch_num;
                      return true;
                    }
              default:
                    break;
              }
        }
  return false;
}

bool MidiNamPatch::gatherReferences(MidNamReferencesList* refs) const
{
  _channelNameSetAssignments.gatherReferences(refs);
  _noteNameList.gatherReferences(refs);
  _controlNameList.gatherReferences(refs);
  return true;
}

bool MidiNamPatch::getNoteSampleName(
  bool drum, int channel, int patch, int note, QString* name) const
{
  if(!name)
    return false;

  if(_channelNameSetAssignments.hasChannelNameSetAssignments())
  {
    return _channelNameSetAssignments.getNoteSampleName(drum, channel, patch, note, name);
  }
  else
  {
    const MidNamNoteNameList* n = noteNameList().objectOrRef();
    return n->getNoteSampleName(drum, channel, patch, note, name);
  }
}


//----------------------------------------------------------------


bool MidiNamPatchNameList::add(const MidiNamPatch& a)
{
  return insert(MidiNamPatchNameListPair(a.patchNumber(), a)).second;
}

void MidiNamPatchNameList::write(int level, MusECore::Xml& xml) const
{
  if(isReference()) {
    xml.put(level, "<UsesPatchNameList Name=\"%s\" />", Xml::xmlString(_name).toLocal8Bit().constData());
  }
  else {
    xml.tag(level++, "PatchNameList Name=\"%s\"", Xml::xmlString(_name).toLocal8Bit().constData());
    for(const_iterator i = cbegin(); i != cend(); ++i)
      i->second.write(level, xml);
    xml.etag(--level, "PatchNameList");
  }
}

void MidiNamPatchNameList::read(MusECore::Xml& xml)
      {
      for (;;) {
            MusECore::Xml::Token token(xml.parse());
            const QString& tag(xml.s1());
            switch (token) {
                  case MusECore::Xml::Error:
                  case MusECore::Xml::End:
                        return;
                  case MusECore::Xml::TagStart:
                        if (tag == "Patch")
                        {
                          MidiNamPatch n;
                          if(n.read(xml))
                            add(n);
                        }
                        else
                          xml.unknown("MidiNamPatchNameList");
                        break;
                  case MusECore::Xml::Attribut:
                        if (tag == "Name") {
                          _name = xml.s2();
                          }
                        break;
                  case MusECore::Xml::TagEnd:
                        if (tag == "PatchNameList") {
                            _isReference = false;
                            return;
                        }
                        else if (tag == "UsesPatchNameList") {
                            _isReference = true;
                            return;
                        }
                  default:
                        break;
                  }
            }
      
      }

bool MidiNamPatchNameList::gatherReferences(MidNamReferencesList* refs) const
{
  for(const_iterator i = cbegin(); i != cend(); ++i)
    i->second.gatherReferences(refs);
  return refs->patchNameListObjs.add(const_cast<MidiNamPatchNameList*>(this));
}

const MidiNamPatch* MidiNamPatchNameList::findPatch(int patch, int /*bankHL*/) const
{
//   const int bankH = (bankHL >> 8) & 0xff;
//   const int bankL = bankHL & 0xff;
// 
//   const int patch_bankH = (patch >> 16) & 0xff;
//   const int patch_bankL = (patch >> 8) & 0xff;
//   const int patch_prog = patch & 0xff;
// 
//   for(const_iterator i = cbegin(); i != cend(); ++i)
//   {
//     const MidiNamPatch& p = i->second;
//     const int p_num = p.patchNumber();
//     // Replace the patch's bank high or low if bank high or low are given.
//     const int p_bankH = bankH != 0xff ? bankH : ((p_num >> 16) & 0xff);
//     const int p_bankL = bankL != 0xff ? bankL : ((p_num >> 8) & 0xff);
//     const int p_prog = p_num & 0xff;
// 
//     // Are we looking for a grand default patch and the resulting patch number is CTRL_PROGRAM_VAL_DONT_CARE?
//     // Or else does the resulting patch number match what we're looking for?
//     if((patch == CTRL_VAL_UNKNOWN && p_bankH == 0xff && p_bankL == 0xff && p_prog == 0xff) ||
//        (p_bankH == patch_bankH && p_bankL == patch_bankL && p_prog == patch_prog))
//       return &i->second;
//   }
// 
//   return nullptr;


  const MidiNamPatchNameList* n = objectOrRef();
  
  const_iterator i;

  // Are we looking for a grand default patch?
  if(patch == CTRL_VAL_UNKNOWN)
  {
    i = n->find(CTRL_PROGRAM_VAL_DONT_CARE);
  }
  else
  {
    // Try looking for the verbose patch number.
    i = n->find(patch);
    if(i == n->cend())
    {
      // Verbose patch number was not found. Try looking for defaults...
      const int bankH = (patch >> 16) & 0xff;
      const int bankL = (patch >> 8) & 0xff;
      const int prog  = patch & 0xff;

      // See if there are defaults for each individual member...
      if(bankH != 0xff)
        i = n->find(patch | 0xff0000);
      if(i == n->cend() && bankL != 0xff)
        i = n->find(patch | 0xff00);
      if(i == n->cend() && prog != 0xff)
        i = n->find(patch | 0xff);
      // See if there are defaults for combinations of individual members:
      if(i == n->cend() && bankH != 0xff && bankL != 0xff)
        i = n->find(patch | 0xffff00);
      if(i == n->cend() && bankH != 0xff && prog != 0xff)
        i = n->find(patch | 0xff00ff);
      if(i == n->cend() && bankL != 0xff && prog != 0xff)
        i = n->find(patch | 0xffff);
      // And finally, see if there's a grand default patch.
      if(i == n->cend())
        i = n->find(CTRL_PROGRAM_VAL_DONT_CARE);
    }
  }

  if(i == n->cend())
    return nullptr;

  return &i->second;
}

bool MidiNamPatchNameList::getNoteSampleName(
  bool drum, int channel, int patch, int note, QString* name, int bankHL) const
{
  if(!name)
    return false;

  // REMOVE Tim. midnam.
  // FIXME: This may not be right.
  //        It is possible there may be only a bankH in the patch bank and
  //         only a bankL in each patch. Need to let the patch/patchlist
  //         sort of, kind of, search first. Test later with real midnam files.

//   const_iterator i;
// 
//   // Are we looking for a grand default patch?
//   if(patch == CTRL_VAL_UNKNOWN)
//   {
//     i = find(CTRL_PROGRAM_VAL_DONT_CARE);
//   }
//   else
//   {
//     // Try looking for the verbose patch number.
//     i = find(patch);
//     if(i == cend())
//     {
//       // Verbose patch number was not found. Try looking for defaults...
//       const int bankH = (patch >> 16) & 0xff;
//       const int bankL = (patch >> 8) & 0xff;
//       const int prog  = patch & 0xff;
// 
//       // See if there are defaults for each individual member...
//       if(bankH != 0xff)
//         i = find(patch | 0xff0000);
//       else if(bankL != 0xff)
//         i = find(patch | 0xff00);
//       else if(prog != 0xff)
//         i = find(patch | 0xff);
//       // See if there are defaults for combinations of individual members:
//       else if(bankH != 0xff && bankL != 0xff)
//         i = find(patch | 0xffff00);
//       else if(bankH != 0xff && prog != 0xff)
//         i = find(patch | 0xff00ff);
//       else if(bankL != 0xff && prog != 0xff)
//         i = find(patch | 0xffff);
//       // And finally, see if there's a grand default patch.
//       else
//         i = find(CTRL_PROGRAM_VAL_DONT_CARE);
//     }
//   }
// 
//   if(i == cend())
//     return false;
// 
//   const MidiNamPatch& mp = i->second;
//   return mp.getNoteSampleName(drum, channel, patch, note, name);
  
  const MidiNamPatch* mp = findPatch(patch, bankHL);
  if(!mp)
    return false;
  return mp->getNoteSampleName(drum, channel, patch, note, name);
}


//----------------------------------------------------------------


void MidiNamPatchBank::write(int level, MusECore::Xml& xml) const
{
  xml.nput(level, "<PatchBank Name=\"%s\" ROM=\"%s\"",
          Xml::xmlString(_name).toLocal8Bit().constData(),
          _ROM ? "true" : "false");
  if(_MIDICommands.empty() &&
    !_patchNameList.isReference() && _patchNameList.empty())
  {
    xml.put(level, " />");
  }
  else
  {
    xml.put(level++, " >");

    _MIDICommands.write(level, xml);
    _patchNameList.write(level, xml);

    xml.etag(--level, "PatchBank");
  }
}

bool MidiNamPatchBank::read(MusECore::Xml& xml)
{
  QString name;
  bool ROM = false;
  // Set bank to 'unknown' (default) at first.
  int bankHL = 0xffff;

  for (;;) {
        MusECore::Xml::Token token(xml.parse());
        const QString& tag(xml.s1());
        switch (token) {
              case MusECore::Xml::Error:
              case MusECore::Xml::End:
                    return false;
              case MusECore::Xml::TagStart:
                    if (tag == "MIDICommands") {
                      if(_MIDICommands.read(xml, true))
                        bankHL = (_MIDICommands.bankH() << 8)  | _MIDICommands.bankL();
                    }

                    else if (tag == "UsesPatchNameList")
                      _patchNameList.read(xml);
                    else if (tag == "PatchNameList")
                      _patchNameList.read(xml);

                    else
                      xml.unknown("MidiNamPatchBank");
                    break;
              case MusECore::Xml::Attribut:
                    if (tag == "Name") {
                          name = xml.s2();
                          }
                    else if (tag == "ROM") {
                          ROM = xml.s2().toInt();
                          const QString& rom_s = xml.s2();
                          if(rom_s.compare("true", Qt::CaseInsensitive) == 0)
                            ROM = true;
                          else if(rom_s.compare("false", Qt::CaseInsensitive) == 0)
                            ROM = false;
                          }
                    break;
              case MusECore::Xml::TagEnd:
                    if (tag == "PatchBank")
                    {
                      // #IMPLIED
                      //if(name.isEmpty())
                      //  return false;
                      _name = name;
                      _ROM = ROM;
                      _bankHL = bankHL;
                      return true;
                    }
              default:
                    break;
              }
        }
  return false;
}

bool MidiNamPatchBank::gatherReferences(MidNamReferencesList* refs) const
{
  return _patchNameList.gatherReferences(refs);
}

const MidiNamPatch* MidiNamPatchBank::findPatch(int patch) const
{
//   const int bankH = (bankHL >> 8) & 0xff;
//   const int bankL = bankHL & 0xff;
// 
//   const int patch_bankH = (patch >> 16) & 0xff;
//   const int patch_bankL = (patch >> 8) & 0xff;
//   const int patch_prog = patch & 0xff;
// 
//   for(const_iterator i = cbegin(); i != cend(); ++i)
//   {
//     const MidiNamPatch& p = i->second;
//     const int p_num = p.patchNumber();
//     // Replace the patch's bank high or low if bank high or low are given.
//     const int p_bankH = bankH != 0xff ? bankH : ((p_num >> 16) & 0xff);
//     const int p_bankL = bankL != 0xff ? bankL : ((p_num >> 8) & 0xff);
//     const int p_prog = p_num & 0xff;
// 
//     // Are we looking for a grand default patch and the resulting patch number is CTRL_PROGRAM_VAL_DONT_CARE?
//     // Or else does the resulting patch number match what we're looking for?
//     if((patch == CTRL_VAL_UNKNOWN && p_bankH == 0xff && p_bankL == 0xff && p_prog == 0xff) ||
//        (p_bankH == patch_bankH && p_bankL == patch_bankL && p_prog == patch_prog))
//       return &i->second;
//   }
// 
//   return nullptr;




// //   const_iterator i;
// 
//   // Are we looking for a grand default patch?
//   if(patch == CTRL_VAL_UNKNOWN)
//   {
//     // This bank not the grand default bank?
//     if(_bankHL != 0xffff)
//       return nullptr;
//     // Pass on to the patch list.
//     return _patchNameList.objectOrRef()->findPatch(patch);
//   }
//   else
//   {
//     // Try looking for the verbose patch number.
//     i = find(patch);
//     if(i == cend())
//     {
//       // Verbose patch number was not found. Try looking for defaults...
//       const int bankH = (patch >> 16) & 0xff;
//       const int bankL = (patch >> 8) & 0xff;
//       const int prog  = patch & 0xff;
// 
//       // See if there are defaults for each individual member...
//       if(bankH != 0xff)
//         i = find(patch | 0xff0000);
//       else if(bankL != 0xff)
//         i = find(patch | 0xff00);
//       else if(prog != 0xff)
//         i = find(patch | 0xff);
//       // See if there are defaults for combinations of individual members:
//       else if(bankH != 0xff && bankL != 0xff)
//         i = find(patch | 0xffff00);
//       else if(bankH != 0xff && prog != 0xff)
//         i = find(patch | 0xff00ff);
//       else if(bankL != 0xff && prog != 0xff)
//         i = find(patch | 0xffff);
//       // And finally, see if there's a grand default patch.
//       else
//         i = find(CTRL_PROGRAM_VAL_DONT_CARE);
//     }
//   }
// 
//   if(i == cend())
//     return nullptr;
// 
//   return &i->second;
//   
  
  
  
  
//   const bool has_bankH = _MIDICommands.hasBankH();
//   const bool has_bankL = _MIDICommands.hasBankL();
// 
//   int fin_patch = patch;
// 
//   // Does the bank have its own bank number midi commands?
//   if(has_bankH || has_bankL)
//   {
//     int patch_bank_hl;
//     // Are we looking for a grand default patch?
//     if(patch == CTRL_VAL_UNKNOWN)
//     {
//       patch_bank_hl = 0xffff;
//       fin_patch = CTRL_PROGRAM_VAL_DONT_CARE;
//     }
//     else
//     {
//       patch_bank_hl = (patch >> 8) & 0xffff;
//     }
//     
//     
//     // If it is not the bank number we're looking for, return.
//     if(patch_bank_hl != _bankHL)
//       return false;
//     // Set the bank high and low to 'unknown' (default) so that the _patchNameList can find it.
//     // Patches can have their own midi controller commands, but if there are bank controller commands,
//     //  in this case that would interfere with the bank number. So likely if patches have their
//     //  own controller commands, they should not have bank controller commands, and therefore
//     //  should all match on bank == 0xffff.
//     fin_patch |= 0xffff00;
//   }
//   
//   return _patchNameList.getNoteSampleName(drum, channel, fin_patch, note, name);
  
  return _patchNameList.findPatch(patch);
}

bool MidiNamPatchBank::getNoteSampleName(
  bool drum, int channel, int patch, int note, QString* name) const
{
  if(!name)
    return false;

  // REMOVE Tim. midnam.
  // FIXME: This may not be right.
  //        It is possible there may be only a bankH in the patch bank and
  //         only a bankL in each patch. Need to let the patch/patchlist
  //         sort of, kind of, search first. Test later with real midnam files.

//   const bool has_bankH = _MIDICommands.hasBankH();
//   const bool has_bankL = _MIDICommands.hasBankL();
// 
//   int fin_patch = patch;
// 
//   // Does the bank have its own bank number midi commands?
//   if(has_bankH || has_bankL)
//   {
//     int patch_bank_hl;
//     // Are we looking for a grand default patch?
//     if(patch == CTRL_VAL_UNKNOWN)
//     {
//       patch_bank_hl = 0xffff;
//       fin_patch = CTRL_PROGRAM_VAL_DONT_CARE;
//     }
//     else
//     {
//       patch_bank_hl = (patch >> 8) & 0xffff;
//     }
//     
//     
//     // If it is not the bank number we're looking for, return.
//     if(patch_bank_hl != _bankHL)
//       return false;
//     // Set the bank high and low to 'unknown' (default) so that the _patchNameList can find it.
//     // Patches can have their own midi controller commands, but if there are bank controller commands,
//     //  in this case that would interfere with the bank number. So likely if patches have their
//     //  own controller commands, they should not have bank controller commands, and therefore
//     //  should all match on bank == 0xffff.
//     fin_patch |= 0xffff00;
//   }
//   
//   return _patchNameList.getNoteSampleName(drum, channel, fin_patch, note, name);

  return _patchNameList.getNoteSampleName(drum, channel, patch, note, name, _bankHL);
}


//----------------------------------------------------------------


bool MidiNamPatchBankList::add(const MidiNamPatchBank& a)
{
  return insert(MidiNamPatchBankListPair(a.bankHL(), a)).second;
}

void MidiNamPatchBankList::write(int level, MusECore::Xml& xml) const
{
  for(const_iterator i = cbegin(); i != cend(); ++i)
    i->second.write(level, xml);
}

bool MidiNamPatchBankList::gatherReferences(MidNamReferencesList* refs) const
{
  for(const_iterator i = cbegin(); i != cend(); ++i)
    i->second.gatherReferences(refs);
  return true;
}

const MidiNamPatch* MidiNamPatchBankList::findPatch(int patch) const
{
//   const int patch_bankH = (patch >> 16) & 0xff;
//   const int patch_bankL = (patch >> 8) & 0xff;
//   const int patch_prog = patch & 0xff;
// 
//   for(const_iterator i = cbegin(); i != cend(); ++i)
//   {
//     const MidiNamPatchBank& p = i->second;
//     const int bankHL = p.bankHL();
//     const int bankH = (bankHL >> 8) & 0xff;
//     const int bankL = bankHL & 0xff;
// 
//     const int p_num = p.patchNumber();
//     // Replace the patch's bank high or low if bank high or low are given.
//     const int p_bankH = bankH != 0xff ? bankH : ((p_num >> 16) & 0xff);
//     const int p_bankL = bankL != 0xff ? bankL : ((p_num >> 8) & 0xff);
//     const int p_prog = p_num & 0xff;
// 
//     // Are we looking for a grand default patch and the resulting patch number is CTRL_PROGRAM_VAL_DONT_CARE?
//     // Or else does the resulting patch number match what we're looking for?
//     if((patch == CTRL_VAL_UNKNOWN && p_bankH == 0xff && p_bankL == 0xff && p_prog == 0xff) ||
//        (p_bankH == patch_bankH && p_bankL == patch_bankL && p_prog == patch_prog))
//       return &i->second;
//   }
// 
//   return nullptr;
  
  
  
  const_iterator i;

  // Are we looking for a grand default patch?
  if(patch == CTRL_VAL_UNKNOWN)
  {
    // See if this bank has a grand default bank.
    i = find(0xffff);
//     if(_bankHL != 0xffff)
//       return nullptr;
//     // Pass on to the patch list.
//     return _patchNameList.objectOrRef()->findPatch(patch);
  }
  else
  {
    // Try looking for the verbose bank number.
    const int patch_bankHL = (patch >> 8) & 0xffff;
    i = find(patch_bankHL);
    if(i == cend())
    {
      // Verbose bank number was not found. Try looking for defaults...
      const int bankH = (patch_bankHL >> 8) & 0xff;
      const int bankL = patch_bankHL & 0xff;

      // See if there are defaults for each individual member...
      if(bankH != 0xff)
        i = find(patch_bankHL | 0xff00);
      else if(bankL != 0xff)
        i = find(patch_bankHL | 0xff);
      // And finally, see if there's a grand default bank.
      else
        i = find(0xffff);
    }
  }

  if(i == cend())
    return nullptr;

  return i->second.findPatch(patch);
}

bool MidiNamPatchBankList::getNoteSampleName(
  bool drum, int channel, int patch, int note, QString* name) const
{
  if(!name)
    return false;

  const MidiNamPatch* p = findPatch(patch);
  if(!p)
    return false;

  return p->getNoteSampleName(drum, channel, patch, note, name);
}


//----------------------------------------------------------------


void MidNamChannelNameSet::write(int level, MusECore::Xml& xml) const
{
  xml.nput(level, "<ChannelNameSet Name=\"%s\"",
          Xml::xmlString(_name).toLocal8Bit().constData());
  if(_availableForChannels.empty() &&
    _patchBankList.empty() &&
    !_noteNameList.isReference() && _noteNameList.empty() &&
    !_controlNameList.isReference() && _controlNameList.empty())
  {
    xml.put(level, " />");
  }
  else
  {
    xml.put(level++, " >");

    _availableForChannels.write(level, xml);

    _noteNameList.write(level, xml);
    _controlNameList.write(level, xml);

    _patchBankList.write(level, xml);

    xml.etag(--level, "ChannelNameSet");
  }
}

bool MidNamChannelNameSet::read(MusECore::Xml& xml)
{
  QString name;

  for (;;) {
        MusECore::Xml::Token token(xml.parse());
        const QString& tag(xml.s1());
        switch (token) {
              case MusECore::Xml::Error:
              case MusECore::Xml::End:
                    return false;
              case MusECore::Xml::TagStart:
                    if (tag == "AvailableForChannels")
                      _availableForChannels.read(xml);
                    else if (tag == "PatchBank")
                    {
                      MidiNamPatchBank n;
                      if(n.read(xml))
                        _patchBankList.add(n);
                    }

                    else if (tag == "UsesNoteNameList")
                      _noteNameList.read(xml);
                    else if (tag == "NoteNameList")
                      _noteNameList.read(xml);

                    else if (tag == "UsesControlNameList")
                      _controlNameList.read(xml);
                    else if (tag == "ControlNameList")
                      _controlNameList.read(xml);

                    else
                      xml.unknown("MidNamChannelNameSet");
                    break;
              case MusECore::Xml::Attribut:
                    if (tag == "Name") {
                          name = xml.s2();
                          }
                    break;
              case MusECore::Xml::TagEnd:
                    if (tag == "ChannelNameSet")
                    {
                      if(name.isEmpty())
                        return false;
                      _name = name;
                      return true;
                    }
              default:
                    break;
              }
        }
  return false;
}

bool MidNamChannelNameSet::gatherReferences(MidNamReferencesList* refs) const
{
  _noteNameList.gatherReferences(refs);
  _controlNameList.gatherReferences(refs);
  _patchBankList.gatherReferences(refs);
  return refs->channelNameSetObjs.add(const_cast<MidNamChannelNameSet*>(this));
}

const MidiNamPatch* MidNamChannelNameSet::findPatch(int channel, int patch) const
{
  if(_availableForChannels.find(channel) == _availableForChannels.end())
    return nullptr;

  return _patchBankList.findPatch(patch);
}

const MidiNamPatchBankList* MidNamChannelNameSet::getPatchBanks(int channel) const
{
  if(_availableForChannels.find(channel) == _availableForChannels.end())
    return nullptr;

  return &_patchBankList;
}

bool MidNamChannelNameSet::getNoteSampleName(
  bool drum, int channel, int patch, int note, QString* name) const
{
  if(!name)
    return false;
  if(_availableForChannels.find(channel) == _availableForChannels.end())
    return false;

  // FIXME Whether to return or not... Need an enum return value.
  if(_patchBankList.getNoteSampleName(drum, channel, patch, note, name))
    return true;
  
  const MidNamNoteNameList* n = _noteNameList.objectOrRef();
  return n->getNoteSampleName(drum, channel, patch, note, name);
}


//----------------------------------------------------------------


void MidiNamChannelNameSetList::write(int level, MusECore::Xml& xml) const
{
  for(const_iterator i = cbegin(); i != cend(); ++i)
    (*i).write(level, xml);
}

bool MidiNamChannelNameSetList::gatherReferences(MidNamReferencesList* refs) const
{
  for(const_iterator i = cbegin(); i != cend(); ++i)
    i->gatherReferences(refs);
  return true;
}

const MidiNamPatch* MidiNamChannelNameSetList::findPatch(int channel, int patch) const
{
  for(const_iterator i = cbegin(); i != cend(); ++i)
  {
    const MidiNamPatch* p = i->findPatch(channel, patch);
    if(p)
      return p;
  }
  return nullptr;
}

const MidiNamPatchBankList* MidiNamChannelNameSetList::getPatchBanks(int channel) const
{
  for(const_iterator i = cbegin(); i != cend(); ++i)
  {
    const MidiNamPatchBankList* p = i->getPatchBanks(channel);
    if(p)
      return p;
  }
  return nullptr;
}

bool MidiNamChannelNameSetList::getNoteSampleName(
  bool drum, int channel, int patch, int note, QString* name) const
{
  if(!name)
    return false;

  for(const_iterator i = cbegin(); i != cend(); ++i)
  {
    // FIXME Whether to return or not... Need an enum return value.
    if(i->getNoteSampleName(drum, channel, patch, note, name))
      return true;
  }
  
  return false;
}


//----------------------------------------------------------------


void MidNamDeviceModeEnable::write(int level, MusECore::Xml& xml) const
{
  if(_MIDICommands.empty())
    return;
  xml.tag(level++, "DeviceModeEnable");
  _MIDICommands.write(level, xml);
  xml.etag(--level, "DeviceModeEnable");
}

bool MidNamDeviceModeEnable::read(MusECore::Xml& xml)
{
  for (;;) {
        MusECore::Xml::Token token(xml.parse());
        const QString& tag(xml.s1());
        switch (token) {
              case MusECore::Xml::Error:
              case MusECore::Xml::End:
                    return false;
              case MusECore::Xml::TagStart:
                    if (tag == "MIDICommands")
                      _MIDICommands.read(xml, true);
                    else
                      xml.unknown("MidNamDeviceModeEnable");
                    break;
              case MusECore::Xml::Attribut:
                    break;
              case MusECore::Xml::TagEnd:
                    if (tag == "DeviceModeEnable")
                      return true;
              default:
                    break;
              }
        }
  return false;
}


//----------------------------------------------------------------


void MidNamDeviceModeDisable::write(int level, MusECore::Xml& xml) const
{
  if(_MIDICommands.empty())
    return;
  xml.tag(level++, "DeviceModeDisable");
  _MIDICommands.write(level, xml);
  xml.etag(--level, "DeviceModeDisable");
}

bool MidNamDeviceModeDisable::read(MusECore::Xml& xml)
{
  for (;;) {
        MusECore::Xml::Token token(xml.parse());
        const QString& tag(xml.s1());
        switch (token) {
              case MusECore::Xml::Error:
              case MusECore::Xml::End:
                    return false;
              case MusECore::Xml::TagStart:
                    if (tag == "MIDICommands")
                      _MIDICommands.read(xml, true);
                    else
                      xml.unknown("MidNamDeviceModeDisable");
                    break;
              case MusECore::Xml::Attribut:
                    break;
              case MusECore::Xml::TagEnd:
                    if (tag == "DeviceModeDisable")
                      return true;
              default:
                    break;
              }
        }
  return false;
}


//----------------------------------------------------------------


void MidNamNameList::write(int level, MusECore::Xml& xml) const
{
  _patchNameList.write(level, xml);
  _noteNameList.write(level, xml);
  _controlNameList.write(level, xml);
  _valueNameList.write(level, xml);
}

bool MidNamNameList::read(MusECore::Xml& xml)
{
  const QString& tag(xml.s1());
  if (tag == "PatchNameList")
    _patchNameList.read(xml);
  else if (tag == "NoteNameList")
    _noteNameList.read(xml);
  else if (tag == "ControlNameList")
    _controlNameList.read(xml);
  else if (tag == "ValueNameList")
    _valueNameList.read(xml);
  else
    return false;

  return true;
}

bool MidNamNameList::gatherReferences(MidNamReferencesList* refs) const
{
  _patchNameList.gatherReferences(refs);
  _noteNameList.gatherReferences(refs);
  _controlNameList.gatherReferences(refs);
  _valueNameList.gatherReferences(refs);
  return true;
}


//----------------------------------------------------------------


void MidNamDeviceMode::write(int level, MusECore::Xml& xml) const
{
  if(isReference()) {
    xml.put(level, "<SupportsStandardDeviceMode Name=\"%s\" />", Xml::xmlString(_name).toLocal8Bit().constData());
  }
  else {
    xml.nput(level, _isCustomDeviceMode ? "<CustomDeviceMode Name=\"%s\"" : "<StandardDeviceMode Name=\"%s\"",
            Xml::xmlString(_name).toLocal8Bit().constData());
    if(_deviceModeEnable.MIDICommands().empty() &&
      _deviceModeDisable.MIDICommands().empty() &&
      _channelNameSetAssignments.empty() &&
      (_isCustomDeviceMode || _channelNameSetList.empty()) &&
      _nameList.empty())
    {
      xml.put(level, " />");
    }
    else
    {
      xml.put(level++, " >");

      _deviceModeEnable.write(level, xml);
      _deviceModeDisable.write(level, xml);
      _channelNameSetAssignments.write(level, xml);
      if(!_isCustomDeviceMode)
        _channelNameSetList.write(level, xml);
      _nameList.write(level, xml);
      
      xml.etag(--level, _isCustomDeviceMode ? "CustomDeviceMode" : "StandardDeviceMode");
    }
  }
}

bool MidNamDeviceMode::read(MusECore::Xml& xml)
{
  QString name;

  for (;;) {
        MusECore::Xml::Token token(xml.parse());
        const QString& tag(xml.s1());
        switch (token) {
              case MusECore::Xml::Error:
              case MusECore::Xml::End:
                    return false;
              case MusECore::Xml::TagStart:
                    if (tag == "DeviceModeEnable")
                      _deviceModeEnable.read(xml);
                    else if (tag == "DeviceModeDisable")
                      _deviceModeDisable.read(xml);
                    else if (tag == "ChannelNameSetAssignments")
                      _channelNameSetAssignments.read(xml);

                    else if (tag == "ChannelNameSet")
                    {
                      MidNamChannelNameSet n;
                      if(n.read(xml))
                        _channelNameSetList.insert(n);
                    }

                    // Special, different from other reads:
                    // For convenience, hand the tag off to
                    //  the NameList to check for several names.
                    // Returns true if recognized and processed.
                    else if (!_nameList.read(xml))
                      xml.unknown("MidNamDeviceMode");

                    break;
              case MusECore::Xml::Attribut:
                    if (tag == "Name") {
                          name = xml.s2();
                          }
                    break;
              case MusECore::Xml::TagEnd:
                    if (tag == "StandardDeviceMode")
                    {
                      if(name.isEmpty())
                        return false;
                      _name = name;
                      _isCustomDeviceMode = false;
                      // Point it to this, it's not a reference.
                      _p_ref = this;
                      return true;
                    }
                    else if (tag == "CustomDeviceMode")
                    {
                      if(name.isEmpty())
                        return false;
                      _name = name;
                      _isCustomDeviceMode = true;
                      _isReference = false;
                      return true;
                    }
                    else if (tag == "SupportsStandardDeviceMode")
                    {
                      if(name.isEmpty())
                        return false;
                      _name = name;
                      _isCustomDeviceMode = false;
                      _isReference = true;
                      return true;
                    }
              default:
                    break;
              }
        }
  return false;
}

bool MidNamDeviceMode::gatherReferences(MidNamReferencesList* refs) const
{
  _channelNameSetAssignments.gatherReferences(refs);
  _nameList.gatherReferences(refs);
  _channelNameSetList.gatherReferences(refs);
  return refs->deviceModeObjs.add(const_cast<MidNamDeviceMode*>(this));
}

const MidiNamPatch* MidNamDeviceMode::findPatch(int channel, int patch) const
{
  ciMidiNamChannelNameSetAssignments ia = _channelNameSetAssignments.find(channel);
  if(ia == _channelNameSetAssignments.cend())
    return nullptr;

  const MidiNamChannelNameSetAssign& cns = ia->second;
  // FIXME Whether to return or not... Need an enum return value.
  const MidiNamPatch* p = cns.findPatch(channel, patch);
  if(p)
    return p;
  
  if(!_isCustomDeviceMode)
    return _channelNameSetList.findPatch(channel, patch);
    
  return nullptr;
}

const MidiNamPatchBankList* MidNamDeviceMode::getPatchBanks(int channel) const
{
  ciMidiNamChannelNameSetAssignments ia = _channelNameSetAssignments.find(channel);
  if(ia == _channelNameSetAssignments.cend())
    return nullptr;

  const MidiNamChannelNameSetAssign& cns = ia->second;
  // FIXME Whether to return or not... Need an enum return value.
  const MidiNamPatchBankList* p = cns.getPatchBanks(channel);
  if(p)
    return p;
  
  if(!_isCustomDeviceMode)
    return _channelNameSetList.getPatchBanks(channel);
    
  return nullptr;
}

bool MidNamDeviceMode::getNoteSampleName(
  bool drum, int channel, int patch, int note, QString* name) const
{
  if(!name)
    return false;

  ciMidiNamChannelNameSetAssignments ia = _channelNameSetAssignments.find(channel);
  if(ia == _channelNameSetAssignments.cend())
    return false;

  const MidiNamChannelNameSetAssign& cns = ia->second;
  // FIXME Whether to return or not... Need an enum return value.
  if(cns.getNoteSampleName(drum, channel, patch, note, name))
    return true;
  
  if(!_isCustomDeviceMode)
    return _channelNameSetList.getNoteSampleName(drum, channel, patch, note, name);
    
  return false;
}


//----------------------------------------------------------------


void MidNamDeviceModeList::write(int level, MusECore::Xml& xml) const
{
  for(const_iterator i = cbegin(); i != cend(); ++i)
    (*i).write(level, xml);
}

bool MidNamDeviceModeList::gatherReferences(MidNamReferencesList* refs) const
{
  for(const_iterator i = cbegin(); i != cend(); ++i)
    i->gatherReferences(refs);
  return true;
}


//----------------------------------------------------------------


void MidNamDevice::write(int level, MusECore::Xml& xml) const
{
  xml.put(level, "<Device Name=\"%s\" UniqueID=\"%d\" />",
          Xml::xmlString(_name).toLocal8Bit().constData(), _uniqueID);
}

bool MidNamDevice::read(MusECore::Xml& xml)
{
  QString name;
  int uniqueID = -1;

  for (;;) {
        MusECore::Xml::Token token(xml.parse());
        const QString& tag(xml.s1());
        switch (token) {
              case MusECore::Xml::Error:
              case MusECore::Xml::End:
                    return false;
              case MusECore::Xml::TagStart:
                      xml.unknown("MidNamDevice");
                    break;
              case MusECore::Xml::Attribut:
                    if (tag == "Name") {
                          name = xml.s2();
                          }
                    else if (tag == "UniqueID") {
                          uniqueID = xml.s2().toInt();
                          }
                    break;
              case MusECore::Xml::TagEnd:
                    if (tag == "Device")
                    {
                      if(name.isEmpty()
                         || uniqueID < 0)
                        return false;
                      _name = name;
                      _uniqueID = uniqueID;
                      return true;
                    }
              default:
                    break;
              }
        }
  return false;
}


//----------------------------------------------------------------


void MidNamModel::write(int level, MusECore::Xml& xml) const
{
  xml.strTag(level, "Model", _model);
}

bool MidNamModel::read(MusECore::Xml& xml)
{
  for (;;) {
        MusECore::Xml::Token token(xml.parse());
        const QString& tag(xml.s1());
        switch (token) {
              case MusECore::Xml::Error:
              case MusECore::Xml::End:
                    return false;
              case MusECore::Xml::TagStart:
                      xml.unknown("MidNamModel");
                    break;
              case MusECore::Xml::Attribut:
                    break;
              case MusECore::Xml::Text:
                      _model = tag;
                    break;
              case MusECore::Xml::TagEnd:
                    if (tag == "Model")
                      return true;
              default:
                    break;
              }
        }
  return false;
}


//----------------------------------------------------------------


void MidiNamModelList::write(int level, MusECore::Xml& xml) const
{
  for(const_iterator i = cbegin(); i != cend(); ++i)
    (*i).write(level, xml);
}


//----------------------------------------------------------------


void MidNamManufacturer::write(int level, MusECore::Xml& xml) const
{
  xml.strTag(level, "Manufacturer", _manufacturer);
}

bool MidNamManufacturer::read(MusECore::Xml& xml)
{
  for (;;) {
        MusECore::Xml::Token token(xml.parse());
        const QString& tag(xml.s1());
        switch (token) {
              case MusECore::Xml::Error:
              case MusECore::Xml::End:
                    return false;
              case MusECore::Xml::TagStart:
                      xml.unknown("MidNamManufacturer");
                    break;
              case MusECore::Xml::Attribut:
                    break;
              case MusECore::Xml::Text:
                      _manufacturer = tag;
                    break;
              case MusECore::Xml::TagEnd:
                    if (tag == "Manufacturer")
                      return true;
              default:
                    break;
              }
        }
  return false;
}


//----------------------------------------------------------------


void MidNamAuthor::write(int level, MusECore::Xml& xml) const
{
  xml.strTag(level, "Author", _author);
}

bool MidNamAuthor::read(MusECore::Xml& xml)
{
  for (;;) {
        MusECore::Xml::Token token(xml.parse());
        const QString& tag(xml.s1());
        switch (token) {
              case MusECore::Xml::Error:
              case MusECore::Xml::End:
                    return false;
              case MusECore::Xml::TagStart:
                      xml.unknown("MidNamAuthor");
                    break;
              case MusECore::Xml::Attribut:
                    break;
              case MusECore::Xml::Text:
                      _author = tag;
                    break;
              case MusECore::Xml::TagEnd:
                    if (tag == "Author")
                      return true;
              default:
                    break;
              }
        }
  return false;
}


//----------------------------------------------------------------


void MidNamExtendingDeviceNames::write(int level, MusECore::Xml& xml) const
{
  xml.tag(level++, "ExtendingDeviceNames");

  _manufacturer.write(level, xml);
  _modelList.write(level, xml);
  _device.write(level, xml);
  _nameList.write(level, xml);

  xml.etag(--level, "ExtendingDeviceNames");
}

bool MidNamExtendingDeviceNames::read(MusECore::Xml& xml)
{
  for (;;) {
        MusECore::Xml::Token token(xml.parse());
        const QString& tag(xml.s1());
        switch (token) {
              case MusECore::Xml::Error:
              case MusECore::Xml::End:
                    return false;
              case MusECore::Xml::TagStart:
                    if (tag == "Manufacturer")
                      _manufacturer.read(xml);

                    else if (tag == "Model")
                    {
                      MidNamModel n;
                      if(n.read(xml))
                        _modelList.insert(n);
                    }

                    else if (tag == "Device")
                      _device.read(xml);

                    // Special, different from other reads:
                    // For convenience, hand the tag off to
                    //  the NameList to check for several names.
                    // Returns true if recognized and processed.
                    else if (!_nameList.read(xml))
                      xml.unknown("MidNamExtendingDeviceNames");

                    break;
              case MusECore::Xml::Attribut:
                    break;
              case MusECore::Xml::TagEnd:
                    if (tag == "ExtendingDeviceNames")
                      return true;
              default:
                    break;
              }
        }
  return false;
}

bool MidNamExtendingDeviceNames::gatherReferences(MidNamReferencesList* refs) const
{
  return _nameList.gatherReferences(refs);
}


//----------------------------------------------------------------


void MidNamExtendingDeviceNamesList::write(int level, MusECore::Xml& xml) const
{
  for(const_iterator i = cbegin(); i != cend(); ++i)
    (*i).write(level, xml);
}

bool MidNamExtendingDeviceNamesList::gatherReferences(MidNamReferencesList* refs) const
{
  for(const_iterator i = cbegin(); i != cend(); ++i)
    i->gatherReferences(refs);
  return true;
}


//----------------------------------------------------------------


void MidNamMasterDeviceNames::write(int level, MusECore::Xml& xml) const
{
  xml.tag(level++, "MasterDeviceNames");

  _manufacturer.write(level, xml);
  _modelList.write(level, xml);
  _device.write(level, xml);
  _deviceModeList.write(level, xml);
  _channelNameSetList.write(level, xml);
  _nameList.write(level, xml);

  xml.etag(--level, "MasterDeviceNames");
}

bool MidNamMasterDeviceNames::read(MusECore::Xml& xml)
{
  for (;;) {
        MusECore::Xml::Token token(xml.parse());
        const QString& tag(xml.s1());
        switch (token) {
              case MusECore::Xml::Error:
              case MusECore::Xml::End:
                    return false;
              case MusECore::Xml::TagStart:
                    if (tag == "Manufacturer")
                      _manufacturer.read(xml);

                    else if (tag == "Model")
                    {
                      MidNamModel n;
                      if(n.read(xml))
                        _modelList.insert(n);
                    }

                    else if (tag == "Device")
                      _device.read(xml);

                    else if (tag == "CustomDeviceMode")
                    {
                      MidNamDeviceMode n;
                      if(n.read(xml))
                        _deviceModeList.insert(n);
                    }

                    else if (tag == "SupportsStandardDeviceMode")
                    {
                      MidNamDeviceMode n;
                      if(n.read(xml))
                        _deviceModeList.insert(n);
                    }

                    else if (tag == "ChannelNameSet")
                    {
                      MidNamChannelNameSet n;
                      if(n.read(xml))
                        _channelNameSetList.insert(n);
                    }

                    // Special, different from other reads:
                    // For convenience, hand the tag off to
                    //  the NameList to check for several names.
                    // Returns true if recognized and processed.
                    else if (!_nameList.read(xml))
                      xml.unknown("MidNamMasterDeviceNames");

                    break;
              case MusECore::Xml::Attribut:
                    break;
              case MusECore::Xml::TagEnd:
                    if (tag == "MasterDeviceNames")
                      return true;
              default:
                    break;
              }
        }
  return false;
}

bool MidNamMasterDeviceNames::gatherReferences(MidNamReferencesList* refs) const
{
  _deviceModeList.gatherReferences(refs);
  _channelNameSetList.gatherReferences(refs);
  _nameList.gatherReferences(refs);
  _channelNameSetList.gatherReferences(refs);
  return true;
}

const MidiNamPatch* MidNamMasterDeviceNames::findPatch(int channel, int patch) const
{
  if(!deviceModeList()->empty())
  {
    // We currently can only deal with one list.
    const MidNamDeviceMode& dm = *deviceModeList()->begin();
    return dm.findPatch(channel, patch);
  }
  
  return _channelNameSetList.findPatch(channel, patch);
}

const MidiNamPatchBankList* MidNamMasterDeviceNames::getPatchBanks(int channel) const
{
  if(!deviceModeList()->empty())
  {
    // We currently can only deal with one list.
    const MidNamDeviceMode& dm = *deviceModeList()->begin();
    return dm.getPatchBanks(channel);
  }
  
  return _channelNameSetList.getPatchBanks(channel);
}

bool MidNamMasterDeviceNames::getNoteSampleName(
  bool drum, int channel, int patch, int note, QString* name) const
{
  if(!name)
    return false;

  if(!deviceModeList()->empty())
  {
    // We currently can only deal with one list.
    const MidNamDeviceMode& dm = *deviceModeList()->begin();
    return dm.getNoteSampleName(drum, channel, patch, note, name);
  }
  
  return _channelNameSetList.getNoteSampleName(drum, channel, patch, note, name);
}


//----------------------------------------------------------------


void MidNamMasterDeviceNamesList::write(int level, MusECore::Xml& xml) const
{
  for(const_iterator i = cbegin(); i != cend(); ++i)
    (*i).write(level, xml);
}


bool MidNamMasterDeviceNamesList::gatherReferences(MidNamReferencesList* refs) const
{
  for(const_iterator i = cbegin(); i != cend(); ++i)
    i->gatherReferences(refs);
  return true;
}

//----------------------------------------------------------------


bool MidNamReferencesList::resolveReferences() const
{
  for(MidNamNoteNameListRefs_t::const_iterator ir = noteNameListObjs.cbegin();
      ir != noteNameListObjs.cend(); ++ir)
  {
    MidNamNoteNameList* lobj = *ir;
    // Looking for objects not references...
    // Make sure it has a name to reference to!
    if(lobj->isReference() || lobj->name().isEmpty())
      continue;
    for(MidNamNoteNameListRefs_t::const_iterator iref = noteNameListObjs.cbegin();
        iref != noteNameListObjs.cend(); ++iref)
    {
      MidNamNoteNameList* lref = *iref;
      // Looking for references not objects...
      if(!lref->isReference())
        continue;
      // Do the names match? Set the reference's object.
      if(lobj->name() == lref->name())
        lref->setObjectOrRef(lobj);
    }
  }
  
  for(MidiNamValNamesRefs_t::const_iterator ir = valNamesObjs.cbegin();
      ir != valNamesObjs.cend(); ++ir)
  {
    MidiNamValNames* lobj = *ir;
    // Looking for objects not references...
    // Make sure it has a name to reference to!
    if(lobj->isReference() || lobj->name().isEmpty())
      continue;
    for(MidiNamValNamesRefs_t::const_iterator iref = valNamesObjs.cbegin();
        iref != valNamesObjs.cend(); ++iref)
    {
      MidiNamValNames* lref = *iref;
      // Looking for references not objects...
      if(!lref->isReference())
        continue;
      // Do the names match? Set the reference's object.
      if(lobj->name() == lref->name())
        lref->setObjectOrRef(lobj);
    }
  }
  
  for(MidiNamCtrlsRefs_t::const_iterator ir = ctrlsObjs.cbegin();
      ir != ctrlsObjs.cend(); ++ir)
  {
    MidiNamCtrls* lobj = *ir;
    // Looking for objects not references...
    // Make sure it has a name to reference to!
    if(lobj->isReference() || lobj->name().isEmpty())
      continue;
    for(MidiNamCtrlsRefs_t::const_iterator iref = ctrlsObjs.cbegin();
        iref != ctrlsObjs.cend(); ++iref)
    {
      MidiNamCtrls* lref = *iref;
      // Looking for references not objects...
      if(!lref->isReference())
        continue;
      // Do the names match? Set the reference's object.
      if(lobj->name() == lref->name())
        lref->setObjectOrRef(lobj);
    }
  }
  
  for(MidiNamPatchNameListRefs_t::const_iterator ir = patchNameListObjs.cbegin();
      ir != patchNameListObjs.cend(); ++ir)
  {
    MidiNamPatchNameList* lobj = *ir;
    // Looking for objects not references...
    // Make sure it has a name to reference to!
    if(lobj->isReference() || lobj->name().isEmpty())
      continue;
    for(MidiNamPatchNameListRefs_t::const_iterator iref = patchNameListObjs.cbegin();
        iref != patchNameListObjs.cend(); ++iref)
    {
      MidiNamPatchNameList* lref = *iref;
      // Looking for references not objects...
      if(!lref->isReference())
        continue;
      // Do the names match? Set the reference's object.
      if(lobj->name() == lref->name())
        lref->setObjectOrRef(lobj);
    }
  }
  
  for(MidNamDeviceModeRefs_t::const_iterator ir = deviceModeObjs.cbegin();
      ir != deviceModeObjs.cend(); ++ir)
  {
    MidNamDeviceMode* lobj = *ir;
    // Looking for objects not references...
    // Make sure it has a name to reference to!
    if(lobj->isReference() || lobj->name().isEmpty())
      continue;
    for(MidNamDeviceModeRefs_t::const_iterator iref = deviceModeObjs.cbegin();
        iref != deviceModeObjs.cend(); ++iref)
    {
      MidNamDeviceMode* lref = *iref;
      // Looking for references not objects...
      if(!lref->isReference())
        continue;
      // Do the names match? Set the reference's object.
      if(lobj->name() == lref->name())
        lref->setObjectOrRef(lobj);
    }
  }
  
  for(MidNamChannelNameSetRefs_t::const_iterator ir = channelNameSetObjs.cbegin();
      ir != channelNameSetObjs.cend(); ++ir)
  {
    MidNamChannelNameSet* lobj = *ir;
    // Unlike the other referencing classes, MidNamChannelNameSet is not referenceable.
    // Here we are looking for a separate corresponding MidNamChannelNameSet.
    // Make sure it has a name to reference to!
    if(/*lobj->isReference() ||*/ lobj->name().isEmpty())
      continue;
    for(MidiNamChannelNameSetAssignRefs_t::const_iterator iref = channelNameSetAssignObjs.cbegin();
        iref != channelNameSetAssignObjs.cend(); ++iref)
    {
      MidiNamChannelNameSetAssign* lref = *iref;
      // Here we are just looking for the name of a separate corresponding MidNamChannelNameSet.
      //if(!lref->isReference())
      //  continue;
      // Do the names match? Set the reference's object.
      if(lobj->name() == lref->name())
        lref->setObjectOrRef(lobj);
    }
  }
  
  return true;
}


//----------------------------------------------------------------


void MidNamMIDINameDocument::write(int level, MusECore::Xml& xml) const
{
  xml.tag(level++, "MIDINameDocument");

  _author.write(level, xml);
  _masterDeviceNamesList.write(level, xml);
  _extendingDeviceNamesList.write(level, xml);
  _standardDeviceModeList.write(level, xml);

  xml.etag(--level, "MIDINameDocument");
}

bool MidNamMIDINameDocument::read(MusECore::Xml& xml)
{
  for (;;) {
        MusECore::Xml::Token token(xml.parse());
        const QString& tag(xml.s1());
        switch (token) {
              case MusECore::Xml::Error:
              case MusECore::Xml::End:
                    return false;
              case MusECore::Xml::TagStart:
                    if (tag == "Author")
                      _author.read(xml);

                    else if (tag == "MasterDeviceNames")
                    {
                      MidNamMasterDeviceNames n;
                      if(n.read(xml))
                        _masterDeviceNamesList.push_back(n);
                    }

                    else if (tag == "ExtendingDeviceNames")
                    {
                      MidNamExtendingDeviceNames n;
                      if(n.read(xml))
                        _extendingDeviceNamesList.push_back(n);
                    }

                    else if (tag == "StandardDeviceMode")
                    {
                      MidNamDeviceMode n;
                      if(n.read(xml))
                        _standardDeviceModeList.insert(n);
                    }

                    else
                      xml.unknown("MidNamMIDINameDocument");

                    break;
              case MusECore::Xml::Attribut:
                    break;
              case MusECore::Xml::TagEnd:
                    if (tag == "MIDINameDocument")
                      return true;
                    break;
              case MusECore::Xml::Proc:
              default:
                    break;
              }
        }
  return false;
}

bool MidNamMIDINameDocument::gatherReferences(MidNamReferencesList* refs) const
{
  _masterDeviceNamesList.gatherReferences(refs);
  _extendingDeviceNamesList.gatherReferences(refs);
  _standardDeviceModeList.gatherReferences(refs);
  return true;
}

bool MidNamMIDINameDocument::resolveReferences()
{
  MidNamReferencesList refs;
  gatherReferences(&refs);
  return refs.resolveReferences();
}

const MidiNamPatch* MidNamMIDINameDocument::findPatch(int channel, int patch) const
{
  // Which of the three exclusively possible device lists is dominant (has stuff in it)?
  if(!_masterDeviceNamesList.empty())
  {
    // We currently can only deal with one list.
    const MidNamMasterDeviceNames& mdn = _masterDeviceNamesList.front();
    if(!mdn.deviceModeList()->empty())
    {
      // We currently can only deal with one list.
      const MidNamDeviceMode& dm = *mdn.deviceModeList()->begin();
      return dm.findPatch(channel, patch);
    }
  }
  else if(!_extendingDeviceNamesList.empty())
  {
    
  }
  else if(!_standardDeviceModeList.empty())
  {
    
  }

  return nullptr;
}

const MidiNamPatchBankList* MidNamMIDINameDocument::getPatchBanks(int channel) const
{
  // Which of the three exclusively possible device lists is dominant (has stuff in it)?
  if(!_masterDeviceNamesList.empty())
  {
    // We currently can only deal with one list.
    const MidNamMasterDeviceNames& mdn = _masterDeviceNamesList.front();
    if(!mdn.deviceModeList()->empty())
    {
      // We currently can only deal with one list.
      const MidNamDeviceMode& dm = *mdn.deviceModeList()->begin();
      return dm.getPatchBanks(channel);
    }
  }
  else if(!_extendingDeviceNamesList.empty())
  {
    
  }
  else if(!_standardDeviceModeList.empty())
  {
    
  }

  return nullptr;
}

bool MidNamMIDINameDocument::getNoteSampleName(
  bool drum, int channel, int patch, int note, QString* name) const
{
  if(!name)
    return false;

  // Which of the three exclusively possible device lists is dominant (has stuff in it)?
  if(!_masterDeviceNamesList.empty())
  {
    // We currently can only deal with one list.
    const MidNamMasterDeviceNames& mdn = _masterDeviceNamesList.front();
    if(!mdn.deviceModeList()->empty())
    {
      // We currently can only deal with one list.
      const MidNamDeviceMode& dm = *mdn.deviceModeList()->begin();
      return dm.getNoteSampleName(drum, channel, patch, note, name);
    }
  }
  else if(!_extendingDeviceNamesList.empty())
  {
    
  }
  else if(!_standardDeviceModeList.empty())
  {
    
  }

  return false;
}


//----------------------------------------------------------------


void MidNamMIDIName::write(int level, MusECore::Xml& xml) const
{
  _MIDINameDocument.write(level, xml);
}


bool MidNamMIDIName::read(MusECore::Xml& xml)
{
  // Be sure to clear the whole thing before reading.
  clear();

  for (;;) {
        MusECore::Xml::Token token(xml.parse());
        const QString& tag(xml.s1());
        switch (token) {
              case MusECore::Xml::Error:
                    return false;
              case MusECore::Xml::End:
                    // We've reached the end of the file, without error.
                    // Everything's fine.
                    // Be sure to resolve references!
                    resolveReferences();
                    return true;
              case MusECore::Xml::TagStart:
                    if (tag == "MIDINameDocument")
                    {
                      if(!_MIDINameDocument.read(xml))
                        return false;
                      // We have successfully loaded a valid document.
                      // It's no longer empty.
                      _isEmpty = false;
                    }

                    else
                      xml.unknown("MidNamMIDIName");

                    break;
              // No attributes or tag ends to look for here.
              case MusECore::Xml::Attribut:
              case MusECore::Xml::TagEnd:
              default:
                    break;
              }
        }
  return false;
}

const MidiNamPatch* MidNamMIDIName::findPatch(int channel, int patch) const
{
  return _MIDINameDocument.findPatch(channel, patch);
}

const MidiNamPatchBankList* MidNamMIDIName::getPatchBanks(int channel) const
{
  return _MIDINameDocument.getPatchBanks(channel);
}

bool MidNamMIDIName::getNoteSampleName(
  bool drum, int channel, int patch, int note, QString* name) const
{
  if(!name)
    return false;

  return _MIDINameDocument.getNoteSampleName(drum, channel, patch, note, name);
}


//----------------------------------------------------------------

void MidNamMIDINameDocumentList::write(int level, MusECore::Xml& xml) const
{
  for(const_iterator i = cbegin(); i != cend(); ++i)
    (*i).write(level, xml);
}


bool MidNamMIDINameDocumentList::read(MusECore::Xml& xml)
{
  for (;;) {
        MusECore::Xml::Token token(xml.parse());
        const QString& tag(xml.s1());
        switch (token) {
              case MusECore::Xml::Error:
                    return false;
              case MusECore::Xml::End:
                    // We've reached the end of the file, without error.
                    // Everything's fine.
                    return true;
              case MusECore::Xml::TagStart:
                    if (tag == "MIDINameDocument")
                    {
                      MidNamMIDINameDocument n;
                      //MidNamMIDIName n;
                      if(n.read(xml))
                        push_back(n);
                    }

                    else
                      xml.unknown("MidNamMIDINameDocumentList");

                    break;
              // No attributes or tag ends to look for here.
              case MusECore::Xml::Attribut:
              case MusECore::Xml::TagEnd:
              default:
                    break;
              }
        }
  return false;
}


//----------------------------------------------------------------


} // namespace MusECore


