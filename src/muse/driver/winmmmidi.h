//=========================================================
//  MusE
//  Linux Music Editor
//
//  Native Windows MIDI (WinMM) input/output, added for the Windows
//  port (see src/README.win32 and the windows_port branch). This
//  codebase previously had no way to reach MIDI hardware on Windows at
//  all: MIDI devices were only ever enumerated through JACK MIDI ports
//  (see helper.cpp's enumerateJackMidiDevices(), gated on the audio
//  backend being JACK_AUDIO) - there was no ALSA-equivalent native
//  backend for Windows. This file is that native backend, modeled
//  after alsamidi.cpp/jackmidi.cpp: same MidiDevice interface, same
//  event-translation semantics as MidiJackDevice::processEvent()
//  (adapted here to plain MIDI wire bytes instead of a Jack buffer),
//  same "own thread polls a self-pipe" integration as ALSA's
//  addAlsaPollFd() (see platform_pipe.h) instead of ALSA's single
//  sequencer fd.
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

#ifndef __WINMMMIDI_H__
#define __WINMMMIDI_H__

#include "config.h"

#ifdef _WIN32

#define WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <mmsystem.h>

#include <atomic>
#include <vector>

#include "mididev.h"
#include "mpevent.h"
#include "lock_free_buffer.h"

namespace MusECore {

// Forward declarations:
class Xml;

//---------------------------------------------------------
//   WinMMRawInEvent
//    A MIDI input message exactly as delivered by midiInProc(), before
//    it is turned into a real MidiRecordEvent. MidiRecordEvent is
//    documented (mpevent.h) as "allocated and deleted in midiseq
//    thread context" - midiInProc() runs on an internal thread created
//    by the Windows multimedia subsystem, not a thread MusE controls
//    or can assume anything about, so this struct is the plain-old-data
//    handoff between that thread and MidiSeq's own thread (which
//    drains it in processInput() and only THERE constructs the real
//    event and calls recordEvent()).
//---------------------------------------------------------

struct WinMMRawInEvent
{
      // Short message: status/data1/data2 as delivered in a MIM_DATA
      // callback's packed dwParam1 (status in the low byte). sysex
      // empty means "this is a short message", non-empty means
      // "ignore status/data1/data2, this is a MIM_LONGDATA buffer".
      unsigned char status = 0;
      unsigned char data1 = 0;
      unsigned char data2 = 0;
      std::vector<unsigned char> sysex;
};

typedef LockFreeMPSCRingBuffer<WinMMRawInEvent> WinMMRawInFifo;

//---------------------------------------------------------
//   MidiWinMMDevice
//---------------------------------------------------------

class MidiWinMMDevice : public MidiDevice {
      // Only one of these is ever non-null for a given rwFlags()
      // configuration: a WinMM device id is either an input OR an
      // output (unlike an ALSA sequencer port, which is bidirectional).
      // A physical device with both directions shows up as two
      // separate ids, and correspondingly two separate MidiWinMMDevice
      // instances here - the simpler of the two pairing strategies
      // already used elsewhere in this codebase (see the #if 0'd
      // "separate devices, no pairing" variant of
      // enumerateJackMidiDevices() in helper.cpp).
      UINT _inDeviceId;
      UINT _outDeviceId;
      bool _hasIn;
      bool _hasOut;

      HMIDIIN _inHandle;
      HMIDIOUT _outHandle;

      // Set just before closeIn() starts tearing the input handle down
      // (midiInStop()/midiInReset()/.../midiInClose()) - checked by
      // midiInProc()'s MIM_LONGDATA handler so it stops re-queueing the
      // sysex buffer once a close is in progress. See the comment in
      // midiInProc() for why this exists.
      std::atomic<bool> _closingIn;

      // Self-pipe (platform_pipe.h) used purely to wake MidiSeq's
      // poll() loop the instant midiInProc() delivers something -
      // selectRfd() returns the read end, already wired into the
      // generic per-device polling MidiSeq::updatePollFd() does for
      // any device with rwFlags() & 0x2 (see midiseq.cpp).
      int _wakeupFds[2];

      WinMMRawInFifo* _rawInEvents;

      // Sysex input accumulation buffer, re-queued after every
      // MIM_LONGDATA callback (see midiInAddBuffer()'s documented
      // usage pattern).
      MIDIHDR _sysexInHdr;
      std::vector<unsigned char> _sysexInBuf;

      // Sysex output buffer. Only one sysex may be "in flight" (sent
      // but not yet MHDR_DONE) at a time per device - see sendSysex().
      MIDIHDR _sysexOutHdr;
      std::vector<unsigned char> _sysexOutBuf;

      // Playback/user event queues, identical role and usage pattern
      // to MidiAlsaDevice's/MidiJackDevice's same-named members:
      // events are transferred here (sorted by time) from the
      // lock-free eventBuffers() every processMidi() cycle, then
      // dispatched in time order.
      SeqMPEventList _outPlaybackEvents;
      SeqMPEventList _outUserEvents;

      static void CALLBACK midiInProc(HMIDIIN handle, UINT msg, DWORD_PTR instance,
                                       DWORD_PTR param1, DWORD_PTR param2);

      bool openIn();
      bool openOut();
      void closeIn();
      void closeOut();

      // Same semantics as MidiJackDevice::processEvent()/queueEvent():
      // processEvent() translates MusE's internal controller/RPN/NRPN/
      // note-off-mode conventions into plain MIDI messages, queueEvent()
      // (here: sendShortMessage()/sendSysex()) is the only genuinely
      // WinMM-specific part, actually handing bytes to the driver.
      bool processEvent(const MidiPlayEvent& ev);
      bool sendShortMessage(int status, int a, int b);
      bool sendSysex(const unsigned char* data, int len);

   protected:
      virtual unsigned int pbForwardShiftFrames() const;

   public:
      MidiWinMMDevice(const QString& name, UINT inDeviceId, bool hasIn, UINT outDeviceId, bool hasOut);
      virtual ~MidiWinMMDevice();

      // 1:Writable 2:Readable 3:Writable+Readable - same convention as
      // createAlsaMidiDevice()/createJackMidiDevice(), but WinMM
      // devices are discovered up front (see initMidiWinMM()) rather
      // than created on demand, so this isn't currently called from
      // outside this file - kept for symmetry/future use.
      static MidiDevice* createWinMMMidiDevice(const QString& name, UINT inDeviceId, bool hasIn,
                                                UINT outDeviceId, bool hasOut);

      virtual QString open();
      virtual void close();

      virtual inline MidiDeviceType deviceType() const { return WINMM_MIDI; }

      virtual int selectRfd();
      inline virtual int selectWfd() { return -1; }
      virtual void processInput();
      virtual void processMidi(unsigned int curFrame = 0);

      virtual void writeRouting(int, Xml&) const {}
      };

} // namespace MusECore

#endif // _WIN32

namespace MusECore {
// Declared unconditionally (mirroring initMidiAlsa()/exitMidiAlsa() in
// alsamidi.h) so mididev.cpp can call these without its own #ifdef
// _WIN32 - the .cpp provides a real implementation under _WIN32 and a
// do-nothing stub otherwise.
extern bool initMidiWinMM();
extern void exitMidiWinMM();
} // namespace MusECore

#endif
