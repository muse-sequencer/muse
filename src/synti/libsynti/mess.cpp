//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: mess.cpp,v 1.2 2004/04/15 13:46:18 wschweer Exp $
//  (C) Copyright 2004 Werner Schweer (ws@seh.de)
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

#include "mess.h"
#include "muse/midi_consts.h"
#include "midictrl_consts.h"

static const int FIFO_SIZE = 32;

MessConfig::MessConfig() {
    _segmentSize = 1024;
    _sampleRate = 44100;
    _minMeterVal = 0;
    _useDenormalBias = false;
    _denormalBias = 0.0;
    _leftMouseButtonCanDecrease = false;
    _configPath = 0;
    _cachePath = 0;
    _globalLibPath = 0;
    _globalSharePath = 0;
    _userPath = 0;
    _projectPath = 0;
  }

MessConfig::MessConfig(unsigned int segmentSize,
             int sampleRate,
             int minMeterVal,
             bool useDenormalBias,
             float denormalBias,
             bool leftMouseButtonCanDecrease,
             const char* configPath,
             const char* cachePath,
             const char* globalLibPath,
             const char* globalSharePath,
             const char* userPath,
             const char* projectPath)
{
    _segmentSize = segmentSize;
    _sampleRate = sampleRate;
    _minMeterVal = minMeterVal;
    _useDenormalBias = useDenormalBias;
    _denormalBias = denormalBias;
    _leftMouseButtonCanDecrease = leftMouseButtonCanDecrease;
    _configPath = configPath;
    _cachePath = cachePath;
    _globalLibPath = globalLibPath;
    _globalSharePath = globalSharePath;
    _userPath = userPath;
    _projectPath = projectPath;
}

//---------------------------------------------------------
//   MessP
//---------------------------------------------------------

struct MessP {
      // Event Fifo  synti -> Host:
      MusECore::MidiPlayEvent fifo[FIFO_SIZE];
      volatile int fifoSize;
      int fifoWindex;
      int fifoRindex;
      };

//---------------------------------------------------------
//   Mess
//---------------------------------------------------------

Mess::Mess(int n)
      {
      _channels     = n;
      _sampleRate   = 44100;
      d             = new MessP;
      d->fifoSize   = 0;
      d->fifoWindex = 0;
      d->fifoRindex = 0;
      }

//---------------------------------------------------------
//   Mess
//---------------------------------------------------------

Mess::~Mess()
      {
      delete d;
      }

int Mess::oldMidiStateHeader(const unsigned char** /*data*/) const { return 0; }
int Mess::channels() const       { return _channels;   }
int Mess::sampleRate() const     { return _sampleRate; }
void Mess::setSampleRate(int r)  { _sampleRate = r;    }
void Mess::processMessages() { };
bool Mess::setController(int, int, int) { return false; }
bool Mess::playNote(int, int, int) { return false; }
bool Mess::sysex(int, const unsigned char*) { return false; }
void Mess::getInitData(int* n, const unsigned char**) /*const*/ { *n = 0; }
int Mess::getControllerInfo(int, const char**, int*, int*, int*, int*) const {return 0;}
const char* Mess::getPatchName(int, int, bool) const { return "?"; }
const MidiPatch* Mess::getPatchInfo(int, const MidiPatch*) const { return 0; }
bool Mess::getNoteSampleName(bool /*drum*/, int /*channel*/,
                               int /*patch*/, int /*note*/,
                               const char** /*name*/) const { return false; }
bool Mess::hasNativeGui() const { return false; }
bool Mess::nativeGuiVisible() const { return false; }
void Mess::showNativeGui(bool) {}
void Mess::setNativeGeometry(int, int, int, int) {}
void Mess::guiHeartBeat() {}
void Mess::setNativeGuiWindowTitle(const char*) const { }


//---------------------------------------------------------
//   getNativeGeometry
//    dummy
//---------------------------------------------------------

void Mess::getNativeGeometry(int* x, int* y, int* w, int* h) const
      {
      *x = 0;
      *y = 0;
      *w = 0;
      *h = 0;
      }

//---------------------------------------------------------
//   sendEvent
//    send Event synti -> host
//---------------------------------------------------------

void Mess::sendEvent(MusECore::MidiPlayEvent ev)
      {
      if (d->fifoSize == FIFO_SIZE) {
            printf("event synti->host  fifo overflow\n");
            return;
            }
      d->fifo[d->fifoWindex] = ev;
      d->fifoWindex = (d->fifoWindex + 1) % FIFO_SIZE;
      ++(d->fifoSize);
      }

//---------------------------------------------------------
//   receiveEvent
//    called from host
//---------------------------------------------------------

MusECore::MidiPlayEvent Mess::receiveEvent()
      {
      MusECore::MidiPlayEvent ev = d->fifo[d->fifoRindex];
      d->fifoRindex = (d->fifoRindex + 1) % FIFO_SIZE;
      --(d->fifoSize);
      return ev;
      }

//---------------------------------------------------------
//   eventsPending
//    called from host:
//       while (eventsPending()) {
//             receiveEvent();
//             ...
//---------------------------------------------------------

int Mess::eventsPending() const
      {
      return d->fifoSize;
      }

//---------------------------------------------------------
//   processEvent
//    return true if synti is busy
//---------------------------------------------------------

bool Mess::processEvent(const MusECore::MidiPlayEvent& ev)
      {
      switch(ev.type()) {
            case MusECore::ME_NOTEON:
                  return playNote(ev.channel(), ev.dataA(), ev.dataB());
            case MusECore::ME_NOTEOFF:
                  return playNote(ev.channel(), ev.dataA(), 0);
            case MusECore::ME_SYSEX:
	            return sysex(ev.len(), ev.constData());
            case MusECore::ME_CONTROLLER:
                  return setController(ev.channel(), ev.dataA(), ev.dataB());
            case MusECore::ME_PITCHBEND:       
                  return setController(ev.channel(), MusECore::CTRL_PITCH, ev.dataA());
            case MusECore::ME_AFTERTOUCH:       
                  return setController(ev.channel(), MusECore::CTRL_AFTERTOUCH, ev.dataA());
            // Synths are not allowed to receive ME_PROGRAM, CTRL_HBANK, or CTRL_LBANK alone anymore - only CTRL_PROGRAM.
            //case MusECore::ME_PROGRAM:
            //    return setController(ev.channel(), MusECore::CTRL_PROGRAM, ev.dataA());
            }
      return false;
      }

