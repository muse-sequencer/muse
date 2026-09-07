//=========================================================
//  MusE
//  Linux Music Editor
//
//  Native Windows MIDI (WinMM) driver. See winmmmidi.h for the design
//  rationale and how this maps onto alsamidi.cpp/jackmidi.cpp's
//  patterns.
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

#include "winmmmidi.h"

#ifdef _WIN32

#include <stdio.h>
#include <string.h>

#include <QByteArray>

#include "audio.h"
#include "globals.h"
#include "gconfig.h"
#include "midi_consts.h"
#include "midictrl.h"
#include "midiport.h"
#include "midiseq.h"
#include "minstrument.h"
#include "platform_pipe.h"

namespace MusECore {

//---------------------------------------------------------
//   MidiWinMMDevice
//---------------------------------------------------------

MidiWinMMDevice::MidiWinMMDevice(const QString& n, UINT inDeviceId, bool hasIn, UINT outDeviceId, bool hasOut)
   : MidiDevice(n)
      {
      _inDeviceId = inDeviceId;
      _outDeviceId = outDeviceId;
      _hasIn = hasIn;
      _hasOut = hasOut;
      _inHandle = nullptr;
      _outHandle = nullptr;
      _closingIn = false;
      _wakeupFds[0] = _wakeupFds[1] = -1;
      _rawInEvents = new WinMMRawInFifo(256);
      memset(&_sysexInHdr, 0, sizeof(_sysexInHdr));
      _sysexInBuf.resize(1024);
      memset(&_sysexOutHdr, 0, sizeof(_sysexOutHdr));

      setrwFlags((hasOut ? 1 : 0) | (hasIn ? 2 : 0));
      setOpenFlags(rwFlags());

      if(muse_pipe(_wakeupFds) != 0)
      {
        fprintf(stderr, "MidiWinMMDevice: failed to create wakeup pipe for <%s>\n", n.toLocal8Bit().constData());
        _wakeupFds[0] = _wakeupFds[1] = -1;
      }
      else
        muse_pipe_set_nonblock(_wakeupFds[0]);
      }

MidiWinMMDevice::~MidiWinMMDevice()
      {
      close();
      // win_socketpair() (platform_pipe.h) hands back plain loopback
      // TCP sockets stored as int - closesocket() is the correct
      // counterpart (this file only builds under _WIN32, and
      // platform_pipe.h already pulled in winsock2.h).
      if(_wakeupFds[0] != -1)
        closesocket((SOCKET)_wakeupFds[0]);
      if(_wakeupFds[1] != -1)
        closesocket((SOCKET)_wakeupFds[1]);
      delete _rawInEvents;
      }

//---------------------------------------------------------
//   createWinMMMidiDevice
//---------------------------------------------------------

MidiDevice* MidiWinMMDevice::createWinMMMidiDevice(const QString& name, UINT inDeviceId, bool hasIn,
                                                    UINT outDeviceId, bool hasOut)
      {
      MidiWinMMDevice* dev = new MidiWinMMDevice(name, inDeviceId, hasIn, outDeviceId, hasOut);
      MusEGlobal::midiDevices.add(dev);
      return dev;
      }

//---------------------------------------------------------
//   midiInProc
//    Runs on an internal thread created by the Windows multimedia
//    subsystem - NOT a thread MusE controls. Keep this minimal: just
//    hand raw bytes off via the lock-free fifo. See WinMMRawInEvent's
//    comment (winmmmidi.h) for why we don't build a MidiRecordEvent or
//    call recordEvent() here.
//---------------------------------------------------------

void CALLBACK MidiWinMMDevice::midiInProc(HMIDIIN handle, UINT msg, DWORD_PTR instance,
                                           DWORD_PTR param1, DWORD_PTR param2)
      {
      (void)handle;
      (void)param2;
      MidiWinMMDevice* dev = (MidiWinMMDevice*)instance;
      if(!dev || !dev->_rawInEvents)
        return;

      // WINMM_MIDI_INPUT_DEBUG: temporary tracing for the "no MIDI input
      // signal" investigation (build6 did not fix it). Ask before removing.
      if(MusEGlobal::debugMsg)
        fprintf(stderr, "WINMM_MIDI_INPUT_DEBUG: midiInProc fired for <%s> msg=0x%x\n",
                dev->name().toLocal8Bit().constData(), (unsigned int)msg);

      if(msg == MIM_DATA)
      {
        WinMMRawInEvent ev;
        ev.status = (unsigned char)(param1 & 0xff);
        ev.data1  = (unsigned char)((param1 >> 8) & 0xff);
        ev.data2  = (unsigned char)((param1 >> 16) & 0xff);
        dev->_rawInEvents->put(ev);
      }
      else if(msg == MIM_LONGDATA)
      {
        MIDIHDR* hdr = (MIDIHDR*)param1;
        if(hdr && hdr->dwBytesRecorded > 0)
        {
          WinMMRawInEvent ev;
          ev.sysex.assign(hdr->lpData, hdr->lpData + hdr->dwBytesRecorded);
          dev->_rawInEvents->put(ev);
        }
        // WINMM_MIDI_INPUT_DEBUG: temporary tracing, see openIn(). Ask
        // before removing. Logged BEFORE the dwBytesRecorded reset
        // below so it shows exactly what triggered this callback.
        if(MusEGlobal::debugMsg)
          fprintf(stderr, "WINMM_MIDI_INPUT_DEBUG: MIM_LONGDATA <%s> dwBytesRecorded=%lu dwFlags=0x%lx\n",
                  dev->name().toLocal8Bit().constData(),
                  (unsigned long)(hdr ? hdr->dwBytesRecorded : 0),
                  (unsigned long)(hdr ? hdr->dwFlags : 0));
        // Re-queue the SAME buffer for the next chunk - it stays
        // "prepared" across multiple midiInAddBuffer() calls (no need
        // to call midiInPrepareHeader() again), and calling
        // midiInAddBuffer() from within the callback itself is
        // documented as safe. dwBytesRecorded MUST be reset first -
        // Microsoft's own MIDI recording sample does this - otherwise
        // the header is handed back with the previous cycle's stale
        // byte count still set, and at least some class-compliant
        // USB-MIDI drivers (confirmed: this is what was happening with
        // the user's CASIO keyboard) treat that as an already-complete
        // buffer and re-fire MIM_LONGDATA again almost immediately,
        // in a tight loop (measured: ~23000 MIM_LONGDATA callbacks in
        // under a minute, for a session where only ~38 real MIM_DATA
        // short messages - actual notes - were ever received). Beyond
        // just wasting CPU, every one of those spurious callbacks
        // called _rawInEvents->put(), and that fifo is a fixed-size
        // (256) ring buffer whose put() DROPS new entries once full
        // rather than overwriting old ones - so this flood was also
        // capable of silently discarding real note-on/off events
        // sitting behind it in the queue, independent of the separate
        // "MidiSeq thread never started" bug.
        if(hdr)
          hdr->dwBytesRecorded = 0;
        // Do NOT re-queue once closeIn() has started tearing this
        // device down. midiInReset() (called by closeIn()) itself
        // returns every pending buffer via this same MIM_LONGDATA path
        // - if we re-add the buffer in response to THAT, the device is
        // already stopped/reset, and at least this user's CASIO driver
        // immediately completes it again, forever: a tight callback
        // loop racing closeIn()'s midiInUnprepareHeader()/midiInClose()/
        // _inHandle=nullptr on another thread, which is a use-after-free
        // once _inHandle is closed and this callback is still firing.
        // Confirmed via WINMM_MIDI_INPUT_DEBUG tracing: a debug_log.txt
        // showed this callback firing nonstop with dwBytesRecorded=0
        // starting exactly at "deleting midiport controllers" (i.e.
        // app shutdown), consistent with the user's "crashes 3 times
        // out of 4 when quitting" report. Ask before removing this
        // comment.
        if(dev->_inHandle && !dev->_closingIn)
          midiInAddBuffer(dev->_inHandle, hdr, sizeof(MIDIHDR));
      }

      // Wake MidiSeq's poll() loop immediately instead of waiting for
      // its next timer tick - same purpose as ALSA's single sequencer
      // fd, one self-pipe per device here since WinMM has no
      // equivalent single fd covering every device.
      if(dev->_wakeupFds[1] != -1)
      {
        char b = 1;
        muse_pipe_write(dev->_wakeupFds[1], &b, 1);
      }
      }

//---------------------------------------------------------
//   openIn / openOut / closeIn / closeOut
//---------------------------------------------------------

bool MidiWinMMDevice::openIn()
      {
      if(_inHandle || !_hasIn)
        return _inHandle != nullptr;

      _closingIn = false;

      MMRESULT rv = midiInOpen(&_inHandle, _inDeviceId, (DWORD_PTR)&MidiWinMMDevice::midiInProc,
                                (DWORD_PTR)this, CALLBACK_FUNCTION);
      if(rv != MMSYSERR_NOERROR)
      {
        fprintf(stderr, "MidiWinMMDevice::openIn: midiInOpen failed for <%s>: %u\n",
                name().toLocal8Bit().constData(), (unsigned int)rv);
        _inHandle = nullptr;
        return false;
      }

      memset(&_sysexInHdr, 0, sizeof(_sysexInHdr));
      _sysexInHdr.lpData = (LPSTR)_sysexInBuf.data();
      _sysexInHdr.dwBufferLength = (DWORD)_sysexInBuf.size();
      if(midiInPrepareHeader(_inHandle, &_sysexInHdr, sizeof(MIDIHDR)) == MMSYSERR_NOERROR)
        midiInAddBuffer(_inHandle, &_sysexInHdr, sizeof(MIDIHDR));

      MMRESULT startRv = midiInStart(_inHandle);
      // WINMM_MIDI_INPUT_DEBUG: temporary tracing, see midiInProc(). Ask before removing.
      if(MusEGlobal::debugMsg)
        fprintf(stderr, "WINMM_MIDI_INPUT_DEBUG: openIn <%s> deviceId=%u midiInOpen=OK midiInStart=%u wakeupFds=(%d,%d)\n",
                name().toLocal8Bit().constData(), (unsigned int)_inDeviceId, (unsigned int)startRv,
                _wakeupFds[0], _wakeupFds[1]);
      return true;
      }

bool MidiWinMMDevice::openOut()
      {
      if(_outHandle || !_hasOut)
        return _outHandle != nullptr;

      MMRESULT rv = midiOutOpen(&_outHandle, _outDeviceId, 0, 0, CALLBACK_NULL);
      if(rv != MMSYSERR_NOERROR)
      {
        fprintf(stderr, "MidiWinMMDevice::openOut: midiOutOpen failed for <%s>: %u\n",
                name().toLocal8Bit().constData(), (unsigned int)rv);
        _outHandle = nullptr;
        return false;
      }
      return true;
      }

void MidiWinMMDevice::closeIn()
      {
      if(!_inHandle)
        return;
      // Set before midiInReset(): see the comment in midiInProc()'s
      // MIM_LONGDATA branch. Ask before removing.
      _closingIn = true;
      midiInStop(_inHandle);
      midiInReset(_inHandle);
      // Only unprepare if it's not (still) queued - after midiInReset()
      // all pending buffers are returned (as MIM_LONGDATA with
      // dwBytesRecorded possibly 0), so this is safe here.
      midiInUnprepareHeader(_inHandle, &_sysexInHdr, sizeof(MIDIHDR));
      midiInClose(_inHandle);
      _inHandle = nullptr;
      }

void MidiWinMMDevice::closeOut()
      {
      if(!_outHandle)
        return;
      // Marks any pending/in-flight sysex buffer MHDR_DONE, so
      // unpreparing it right after is safe.
      midiOutReset(_outHandle);
      if(_sysexOutHdr.dwFlags & MHDR_PREPARED)
        midiOutUnprepareHeader(_outHandle, &_sysexOutHdr, sizeof(MIDIHDR));
      midiOutClose(_outHandle);
      _outHandle = nullptr;
      }

//---------------------------------------------------------
//   open / close
//---------------------------------------------------------

QString MidiWinMMDevice::open()
      {
      _openFlags &= _rwFlags; // restrict to available bits

      _writeEnable = _readEnable = false;

      bool in_ok = true, out_ok = true;
      if((_openFlags & 2) && _hasIn)
        in_ok = openIn();
      if((_openFlags & 1) && _hasOut)
        out_ok = openOut();

      _readEnable = (_openFlags & 2) && in_ok;
      _writeEnable = (_openFlags & 1) && out_ok;

      _state = (in_ok && out_ok) ? QString("Open") : QString("Not ready");
      return _state;
      }

void MidiWinMMDevice::close()
      {
      _writeEnable = _readEnable = false;
      closeIn();
      closeOut();
      _state = QString("Closed");
      }

//---------------------------------------------------------
//   selectRfd
//---------------------------------------------------------

int MidiWinMMDevice::selectRfd()
      {
      return _wakeupFds[0];
      }

//---------------------------------------------------------
//   processInput
//    Called on MidiSeq's own thread (see midiseq.cpp's generic
//    per-device polling in MidiSeq::updatePollFd(), triggered here by
//    the self-pipe becoming readable). This is the ONE place a real
//    MidiRecordEvent gets constructed and handed to recordEvent(),
//    matching the "allocated and deleted in midiseq thread context"
//    contract documented in mpevent.h.
//---------------------------------------------------------

void MidiWinMMDevice::processInput()
      {
      // Drain the wakeup pipe.
      char buf[64];
      while(muse_pipe_read(_wakeupFds[0], buf, sizeof(buf)) > 0)
        ;

      // WINMM_MIDI_INPUT_DEBUG: temporary tracing, see midiInProc(). Ask before removing.
      if(MusEGlobal::debugMsg)
        fprintf(stderr, "WINMM_MIDI_INPUT_DEBUG: processInput() called for <%s>\n",
                name().toLocal8Bit().constData());

      // Real, current-frame timestamp for this whole batch - same
      // granularity/approach as alsaProcessMidiInput() (alsamidi.cpp),
      // which reads MusEGlobal::audio->curFrame() once per wakeup and
      // stamps every event drained in that batch with it. WinMM events
      // were previously constructed with a hardcoded time of 0
      // (regardless of when they actually arrived), which made every
      // recorded note-on/note-off land at tick 0 with near-zero
      // length - the "notes vanish/collapse on record-stop" bug.
      // curFrame() is explicitly documented (audio.cpp) as safe to call
      // from a thread other than the audio process thread, which is
      // exactly this (MidiSeq's thread). Ask before removing this
      // comment.
      const unsigned frame_ts = MusEGlobal::audio->curFrame();

      WinMMRawInEvent raw;
      while(_rawInEvents->get(raw))
      {
        MidiRecordEvent event;
        if(!raw.sysex.empty())
          event = MidiRecordEvent(frame_ts, _port, ME_SYSEX, raw.sysex.data(), (int)raw.sysex.size());
        else
        {
          int type = raw.status & 0xf0;
          int chan = raw.status & 0x0f;
          event = MidiRecordEvent(frame_ts, _port, chan, type, raw.data1, raw.data2);
        }
        recordEvent(event);
      }
      }

//---------------------------------------------------------
//   pbForwardShiftFrames
//    Buffer-based device (playback events are gathered once per audio
//    cycle in processMidi(), same as Jack midi/synths) - see the
//    comment on MidiDevice::pbForwardShiftFrames() in mididev.h.
//---------------------------------------------------------

unsigned int MidiWinMMDevice::pbForwardShiftFrames() const
      {
      return MusEGlobal::segmentSize;
      }

//---------------------------------------------------------
//   sendShortMessage / sendSysex
//    The only genuinely WinMM-specific part - actually handing bytes
//    to the driver. Equivalent role to MidiJackDevice::queueEvent(),
//    but WinMM has no sample-accurate buffer to reserve space in: a
//    message is simply sent as soon as processMidi() reaches it, best-
//    effort, the same way ALSA sequencer events are (non-realtime-
//    buffer based).
//---------------------------------------------------------

bool MidiWinMMDevice::sendShortMessage(int status, int a, int b)
      {
      if(!_outHandle)
        return false;
      DWORD msg = (DWORD)(status & 0xff) | ((DWORD)(a & 0xff) << 8) | ((DWORD)(b & 0xff) << 16);
      MMRESULT rv = midiOutShortMsg(_outHandle, msg);
      if(rv != MMSYSERR_NOERROR && MusEGlobal::debugMsg)
        fprintf(stderr, "MidiWinMMDevice::sendShortMessage: midiOutShortMsg failed: %u\n", (unsigned int)rv);
      return rv == MMSYSERR_NOERROR;
      }

bool MidiWinMMDevice::sendSysex(const unsigned char* data, int len)
      {
      if(!_outHandle || len <= 0)
        return false;

      // A minimal, synchronous implementation: prepare, send, and poll
      // (briefly, non-blocking-ish) for MHDR_DONE before unpreparing.
      // Real hardware sysex transmission can take a while (a MIDI byte
      // is ~320us at the standard 31.25kbaud rate, so a large dump can
      // take tens of milliseconds) - this is called from the audio
      // thread's processMidi(), so it deliberately does NOT block
      // waiting for completion. Simplification: rely on the OS/driver
      // to finish before this same device is asked to send another
      // sysex; if that ever isn't true in practice, the earlier buffer
      // simply won't have been unprepared yet and this call will
      // safely refuse to send (returns false) instead of corrupting a
      // still-in-flight MIDIHDR.
      if(_sysexOutHdr.dwFlags & MHDR_PREPARED)
      {
        if(!(_sysexOutHdr.dwFlags & MHDR_DONE))
          return false; // Previous sysex still in flight - drop this one rather than block.
        midiOutUnprepareHeader(_outHandle, &_sysexOutHdr, sizeof(MIDIHDR));
      }

      _sysexOutBuf.assign(data, data + len);
      memset(&_sysexOutHdr, 0, sizeof(_sysexOutHdr));
      _sysexOutHdr.lpData = (LPSTR)_sysexOutBuf.data();
      _sysexOutHdr.dwBufferLength = (DWORD)_sysexOutBuf.size();

      if(midiOutPrepareHeader(_outHandle, &_sysexOutHdr, sizeof(MIDIHDR)) != MMSYSERR_NOERROR)
        return false;
      MMRESULT rv = midiOutLongMsg(_outHandle, &_sysexOutHdr, sizeof(MIDIHDR));
      if(rv != MMSYSERR_NOERROR)
      {
        midiOutUnprepareHeader(_outHandle, &_sysexOutHdr, sizeof(MIDIHDR));
        return false;
      }
      return true;
      }

//---------------------------------------------------------
//   processEvent
//    Translates MusE's internal controller/RPN/NRPN/note-off-mode
//    conventions into plain MIDI messages. Deliberately mirrors
//    MidiJackDevice::processEvent()/queueEvent() (jackmidi.cpp) - same
//    semantics, adapted to call sendShortMessage()/sendSysex()
//    directly instead of reserving space in a Jack buffer.
//---------------------------------------------------------

bool MidiWinMMDevice::processEvent(const MidiPlayEvent& event)
      {
      int chn = event.channel();
      int a   = event.dataA();
      int b   = event.dataB();

      MidiInstrument::NoteOffMode nom = MidiInstrument::NoteOffAll;
      const int mport = midiPort();
      if(mport != -1)
      {
        if(MidiInstrument* mi = MusEGlobal::midiPorts[mport].instrument())
          nom = mi->noteOffMode();
      }

      if(event.type() == ME_NOTEON)
      {
        if(b == 0)
        {
          switch(nom)
          {
            case MidiInstrument::NoteOffAll:
              return processEvent(MidiPlayEvent(event.time(), event.port(), chn, ME_NOTEOFF, a, 0));
            case MidiInstrument::NoteOffNone:
            case MidiInstrument::NoteOffConvertToZVNoteOn:
              return sendShortMessage(ME_NOTEON | chn, a, b);
          }
        }
        return sendShortMessage(ME_NOTEON | chn, a, b);
      }
      else if(event.type() == ME_NOTEOFF)
      {
        switch(nom)
        {
          case MidiInstrument::NoteOffAll:
            return sendShortMessage(ME_NOTEOFF | chn, a, b);
          case MidiInstrument::NoteOffNone:
            return true;
          case MidiInstrument::NoteOffConvertToZVNoteOn:
            return sendShortMessage(ME_NOTEON | chn, a, 0);
        }
        return sendShortMessage(ME_NOTEOFF | chn, a, b);
      }
      else if(event.type() == ME_PROGRAM)
      {
        _curOutParamNums[chn].resetParamNums();
        _curOutParamNums[chn].setPROG(a);
        return sendShortMessage(ME_PROGRAM | chn, a, 0);
      }
      else if(event.type() == ME_PITCHBEND)
      {
        int v = a + 8192;
        return sendShortMessage(ME_PITCHBEND | chn, v & 0x7f, (v >> 7) & 0x7f);
      }
      else if(event.type() == ME_SYSEX)
      {
        resetCurOutParamNums();
        return sendSysex(event.constData(), event.len());
      }
      else if(event.type() == ME_CONTROLLER)
      {
        if((a | 0xff) == CTRL_POLYAFTER)
          return sendShortMessage(ME_POLYAFTER | chn, a & 0x7f, b & 0x7f);
        else if(a == CTRL_AFTERTOUCH)
          return sendShortMessage(ME_AFTERTOUCH | chn, b & 0x7f, 0);
        else if(a == CTRL_PITCH)
        {
          int v = b + 8192;
          return sendShortMessage(ME_PITCHBEND | chn, v & 0x7f, (v >> 7) & 0x7f);
        }
        else if(a == CTRL_PROGRAM)
        {
          _curOutParamNums[chn].resetParamNums();
          int hb = (b >> 16) & 0xff;
          int lb = (b >> 8) & 0xff;
          int pr = b & 0xff;
          _curOutParamNums[chn].setCurrentProg(pr, lb, hb);
          bool ok = true;
          if(hb != 0xff)
            ok = ok && sendShortMessage(ME_CONTROLLER | chn, CTRL_HBANK, hb);
          if(lb != 0xff)
            ok = ok && sendShortMessage(ME_CONTROLLER | chn, CTRL_LBANK, lb);
          if(pr != 0xff)
            ok = ok && sendShortMessage(ME_PROGRAM | chn, pr, 0);
          return ok;
        }
        else if(a == CTRL_MASTER_VOLUME)
        {
          unsigned char sysex[] = { 0x7f, 0x7f, 0x04, 0x01, 0x00, 0x00 };
          sysex[4] = b & 0x7f;
          sysex[5] = (b >> 7) & 0x7f;
          return sendSysex(sysex, 6);
        }
        else if(a == CTRL_RESET_ALL_CTRL)
        {
          _curOutParamNums[chn].resetParamNums();
          return sendShortMessage(ME_CONTROLLER | chn, a & 0x7f, b & 0x7f);
        }
        else if(a < CTRL_14_OFFSET)
        {
          if(a == CTRL_HRPN)
            _curOutParamNums[chn].setRPNH(b);
          else if(a == CTRL_LRPN)
            _curOutParamNums[chn].setRPNL(b);
          else if(a == CTRL_HNRPN)
            _curOutParamNums[chn].setNRPNH(b);
          else if(a == CTRL_LNRPN)
            _curOutParamNums[chn].setNRPNL(b);
          else if(a == CTRL_HBANK)
          {
            _curOutParamNums[chn].setBANKH(b);
            _curOutParamNums[chn].resetParamNums();
          }
          else if(a == CTRL_LBANK)
          {
            _curOutParamNums[chn].setBANKL(b);
            _curOutParamNums[chn].resetParamNums();
          }
          return sendShortMessage(ME_CONTROLLER | chn, a & 0x7f, b & 0x7f);
        }
        else if(a < CTRL_RPN_OFFSET)
        {
          int ctrlH = (a >> 8) & 0x7f;
          int ctrlL = a & 0x7f;
          int dataH = (b >> 7) & 0x7f;
          int dataL = b & 0x7f;
          bool ok = sendShortMessage(ME_CONTROLLER | chn, ctrlH, dataH);
          ok = ok && sendShortMessage(ME_CONTROLLER | chn, ctrlL, dataL);
          return ok;
        }
        else if(a < CTRL_NRPN_OFFSET)
        {
          int ctrlH = (a >> 8) & 0x7f;
          int ctrlL = a & 0x7f;
          int data = b & 0x7f;
          bool ok = true;
          if(ctrlH != _curOutParamNums[chn].RPNH || !MusEGlobal::config.midiOptimizeControllers)
          {
            _curOutParamNums[chn].setRPNH(ctrlH);
            ok = ok && sendShortMessage(ME_CONTROLLER | chn, CTRL_HRPN, ctrlH);
          }
          if(ctrlL != _curOutParamNums[chn].RPNL || !MusEGlobal::config.midiOptimizeControllers)
          {
            _curOutParamNums[chn].setRPNL(ctrlL);
            ok = ok && sendShortMessage(ME_CONTROLLER | chn, CTRL_LRPN, ctrlL);
          }
          if(data != _curOutParamNums[chn].DATAH || !MusEGlobal::config.midiOptimizeControllers)
          {
            _curOutParamNums[chn].setDATAH(data);
            ok = ok && sendShortMessage(ME_CONTROLLER | chn, CTRL_HDATA, data);
          }
          if(MusEGlobal::config.midiSendNullParameters)
          {
            _curOutParamNums[chn].setRPNH(0x7f);
            ok = ok && sendShortMessage(ME_CONTROLLER | chn, CTRL_HRPN, 0x7f);
            _curOutParamNums[chn].setRPNL(0x7f);
            ok = ok && sendShortMessage(ME_CONTROLLER | chn, CTRL_LRPN, 0x7f);
          }
          return ok;
        }
        else if(a < CTRL_INTERNAL_OFFSET)
        {
          int ctrlH = (a >> 8) & 0x7f;
          int ctrlL = a & 0x7f;
          int data = b & 0x7f;
          bool ok = true;
          if(ctrlH != _curOutParamNums[chn].NRPNH || !MusEGlobal::config.midiOptimizeControllers)
          {
            _curOutParamNums[chn].setNRPNH(ctrlH);
            ok = ok && sendShortMessage(ME_CONTROLLER | chn, CTRL_HNRPN, ctrlH);
          }
          if(ctrlL != _curOutParamNums[chn].NRPNL || !MusEGlobal::config.midiOptimizeControllers)
          {
            _curOutParamNums[chn].setNRPNL(ctrlL);
            ok = ok && sendShortMessage(ME_CONTROLLER | chn, CTRL_LNRPN, ctrlL);
          }
          if(data != _curOutParamNums[chn].DATAH || !MusEGlobal::config.midiOptimizeControllers)
          {
            _curOutParamNums[chn].setDATAH(data);
            ok = ok && sendShortMessage(ME_CONTROLLER | chn, CTRL_HDATA, data);
          }
          if(MusEGlobal::config.midiSendNullParameters)
          {
            _curOutParamNums[chn].setNRPNH(0x7f);
            ok = ok && sendShortMessage(ME_CONTROLLER | chn, CTRL_HNRPN, 0x7f);
            _curOutParamNums[chn].setNRPNL(0x7f);
            ok = ok && sendShortMessage(ME_CONTROLLER | chn, CTRL_LNRPN, 0x7f);
          }
          return ok;
        }
        else if(a < CTRL_RPN14_OFFSET)
          return false; // Unaccounted for internal controller.
        else if(a < CTRL_NRPN14_OFFSET)
        {
          int ctrlH = (a >> 8) & 0x7f;
          int ctrlL = a & 0x7f;
          int dataH = (b >> 7) & 0x7f;
          int dataL = b & 0x7f;
          bool ok = true;
          if(ctrlH != _curOutParamNums[chn].RPNH || !MusEGlobal::config.midiOptimizeControllers)
          {
            _curOutParamNums[chn].setRPNH(ctrlH);
            ok = ok && sendShortMessage(ME_CONTROLLER | chn, CTRL_HRPN, ctrlH);
          }
          if(ctrlL != _curOutParamNums[chn].RPNL || !MusEGlobal::config.midiOptimizeControllers)
          {
            _curOutParamNums[chn].setRPNL(ctrlL);
            ok = ok && sendShortMessage(ME_CONTROLLER | chn, CTRL_LRPN, ctrlL);
          }
          if(dataH != _curOutParamNums[chn].DATAH || !MusEGlobal::config.midiOptimizeControllers)
          {
            _curOutParamNums[chn].setDATAH(dataH);
            ok = ok && sendShortMessage(ME_CONTROLLER | chn, CTRL_HDATA, dataH);
          }
          if(dataL != _curOutParamNums[chn].DATAL || !MusEGlobal::config.midiOptimizeControllers)
          {
            _curOutParamNums[chn].setDATAL(dataL);
            ok = ok && sendShortMessage(ME_CONTROLLER | chn, CTRL_LDATA, dataL);
          }
          if(MusEGlobal::config.midiSendNullParameters)
          {
            _curOutParamNums[chn].setRPNH(0x7f);
            ok = ok && sendShortMessage(ME_CONTROLLER | chn, CTRL_HRPN, 0x7f);
            _curOutParamNums[chn].setRPNL(0x7f);
            ok = ok && sendShortMessage(ME_CONTROLLER | chn, CTRL_LRPN, 0x7f);
          }
          return ok;
        }
        else if(a < CTRL_NONE_OFFSET)
        {
          int ctrlH = (a >> 8) & 0x7f;
          int ctrlL = a & 0x7f;
          int dataH = (b >> 7) & 0x7f;
          int dataL = b & 0x7f;
          bool ok = true;
          if(ctrlH != _curOutParamNums[chn].NRPNH || !MusEGlobal::config.midiOptimizeControllers)
          {
            _curOutParamNums[chn].setNRPNH(ctrlH);
            ok = ok && sendShortMessage(ME_CONTROLLER | chn, CTRL_HNRPN, ctrlH);
          }
          if(ctrlL != _curOutParamNums[chn].NRPNL || !MusEGlobal::config.midiOptimizeControllers)
          {
            _curOutParamNums[chn].setNRPNL(ctrlL);
            ok = ok && sendShortMessage(ME_CONTROLLER | chn, CTRL_LNRPN, ctrlL);
          }
          if(dataH != _curOutParamNums[chn].DATAH || !MusEGlobal::config.midiOptimizeControllers)
          {
            _curOutParamNums[chn].setDATAH(dataH);
            ok = ok && sendShortMessage(ME_CONTROLLER | chn, CTRL_HDATA, dataH);
          }
          if(dataL != _curOutParamNums[chn].DATAL || !MusEGlobal::config.midiOptimizeControllers)
          {
            _curOutParamNums[chn].setDATAL(dataL);
            ok = ok && sendShortMessage(ME_CONTROLLER | chn, CTRL_LDATA, dataL);
          }
          if(MusEGlobal::config.midiSendNullParameters)
          {
            _curOutParamNums[chn].setNRPNH(0x7f);
            ok = ok && sendShortMessage(ME_CONTROLLER | chn, CTRL_HNRPN, 0x7f);
            _curOutParamNums[chn].setNRPNL(0x7f);
            ok = ok && sendShortMessage(ME_CONTROLLER | chn, CTRL_LNRPN, 0x7f);
          }
          return ok;
        }
        else
        {
          if(MusEGlobal::debugMsg)
            fprintf(stderr, "MidiWinMMDevice::processEvent: unknown controller type 0x%x\n", a);
          return true;
        }
      }
      else if(event.type() == ME_SONGPOS)
        return sendShortMessage(ME_SONGPOS, a & 0x7f, (a >> 7) & 0x7f);
      else if(event.type() == ME_CLOCK || event.type() == ME_START ||
              event.type() == ME_CONTINUE || event.type() == ME_STOP)
        return sendShortMessage(event.type(), 0, 0);
      else
      {
        if(MusEGlobal::debugMsg)
          fprintf(stderr, "MidiWinMMDevice::processEvent: event type %x not supported\n", event.type());
        return true; // Absorb the event.
      }
      }

//---------------------------------------------------------
//   processMidi
//    Same buffer-transfer/merge/dispatch pattern as
//    MidiAlsaDevice::processMidi()/MidiJackDevice::processMidi().
//---------------------------------------------------------

void MidiWinMMDevice::processMidi(unsigned int curFrame)
      {
      const bool do_stop = stopFlag();
      const bool do_process = _writeEnable && _outHandle;

      MidiPlayEvent buf_ev;

      if(do_stop || !do_process)
      {
        const unsigned int usr_buf_sz = eventBuffers(MidiDevice::UserBuffer)->getSize();
        for(unsigned int i = 0; i < usr_buf_sz; ++i)
        {
          if(eventBuffers(MidiDevice::UserBuffer)->get(buf_ev))
            _outUserEvents.addExclusive(buf_ev, false);
        }
        eventBuffers(MidiDevice::PlaybackBuffer)->clearRead();
        _outPlaybackEvents.clear();
        setStopFlag(false);
      }
      else
      {
        const unsigned int usr_buf_sz = eventBuffers(MidiDevice::UserBuffer)->getSize();
        for(unsigned int i = 0; i < usr_buf_sz; ++i)
        {
          if(eventBuffers(MidiDevice::UserBuffer)->get(buf_ev))
            _outUserEvents.insert(buf_ev);
        }

        const unsigned int pb_buf_sz = eventBuffers(MidiDevice::PlaybackBuffer)->getSize();
        for(unsigned int i = 0; i < pb_buf_sz; ++i)
        {
          if(eventBuffers(MidiDevice::PlaybackBuffer)->get(buf_ev))
            _outPlaybackEvents.insert(buf_ev);
        }
      }

      if(!do_process)
        return;

      iSeqMPEvent impe_pb = _outPlaybackEvents.begin();
      iSeqMPEvent impe_us = _outUserEvents.begin();
      bool using_pb;

      while(1)
      {
        if(impe_pb != _outPlaybackEvents.end() && impe_us != _outUserEvents.end())
          using_pb = *impe_pb < *impe_us;
        else if(impe_pb != _outPlaybackEvents.end())
          using_pb = true;
        else if(impe_us != _outUserEvents.end())
          using_pb = false;
        else
          break;

        const MidiPlayEvent& ev = using_pb ? *impe_pb : *impe_us;

        if(ev.time() >= (curFrame + MusEGlobal::segmentSize))
          break;

        processEvent(ev);

        if(using_pb)
          impe_pb = _outPlaybackEvents.erase(impe_pb);
        else
          impe_us = _outUserEvents.erase(impe_us);
      }
      }

//---------------------------------------------------------
//   initMidiWinMM / exitMidiWinMM
//---------------------------------------------------------

bool initMidiWinMM()
      {
      // The sequencer thread (MidiSeq) is what actually polls each
      // device's selectRfd() and drains recorded MIDI - MidiSeq itself
      // is generic/backend-agnostic, but MusEGlobal::midiSeq is never
      // constructed anywhere except as a side effect of initMidiAlsa()
      // calling initMidiSequencer() (see alsamidi.cpp). On Windows,
      // ALSA_SUPPORT is never defined so that call site is compiled out
      // entirely: MusEGlobal::midiSeq silently stayed nullptr for the
      // whole session. Every other part of the WinMM backend worked
      // (device enumeration, midiInOpen/midiInStart, midiInProc firing
      // on real hardware input, confirmed via WINMM_MIDI_INPUT_DEBUG
      // tracing) - but with no sequencer thread, nothing ever called
      // updatePollFd() or processInput(), so recorded input (and
      // anything else routed through MidiSeq, e.g. metronome/playback
      // timing) silently went nowhere. This mirrors initMidiAlsa()'s
      // own call (idempotent: initMidiSequencer() only constructs the
      // object if MusEGlobal::midiSeq is still null). Ask before
      // removing this comment.
      MusECore::initMidiSequencer();

      const UINT numIn = midiInGetNumDevs();
      const UINT numOut = midiOutGetNumDevs();

      if(MusEGlobal::debugMsg)
        fprintf(stderr, "initMidiWinMM: %u input(s), %u output(s)\n", numIn, numOut);

      for(UINT i = 0; i < numIn; ++i)
      {
        // Explicit wide (W) API/struct, regardless of whether this
        // build defines UNICODE - avoids any ambiguity about whether
        // szPname is CHAR[] or WCHAR[] (TCHAR depends on that define).
        MIDIINCAPSW caps;
        if(midiInGetDevCapsW(i, &caps, sizeof(caps)) != MMSYSERR_NOERROR)
          continue;
        const QString name = QString::fromWCharArray(caps.szPname);
        if(MusEGlobal::debugMsg)
          fprintf(stderr, "initMidiWinMM: input %u: %s\n", i, name.toLocal8Bit().constData());
        MidiWinMMDevice::createWinMMMidiDevice(name, i, true, 0, false);
      }

      for(UINT i = 0; i < numOut; ++i)
      {
        MIDIOUTCAPSW caps;
        if(midiOutGetDevCapsW(i, &caps, sizeof(caps)) != MMSYSERR_NOERROR)
          continue;
        const QString name = QString::fromWCharArray(caps.szPname);
        if(MusEGlobal::debugMsg)
          fprintf(stderr, "initMidiWinMM: output %u: %s\n", i, name.toLocal8Bit().constData());
        MidiWinMMDevice::createWinMMMidiDevice(name, 0, false, i, true);
      }

      return false; // false == no error, matches initMidiAlsa()/initMidiJack() convention.
      }

void exitMidiWinMM()
      {
      // Nothing global to clean up here - each MidiWinMMDevice's own
      // destructor closes its handles (see ~MidiWinMMDevice()), called
      // as part of the normal MusEGlobal::midiDevices teardown.
      }

} // namespace MusECore

#else // !_WIN32

namespace MusECore {
bool initMidiWinMM() { return false; }
void exitMidiWinMM() {}
} // namespace MusECore

#endif // _WIN32
