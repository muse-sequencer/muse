//=========================================================
//  MusE
//  Linux Music Editor
//  rtlog.h
//
//  Lock-free, non-blocking diagnostic logging queue, safe to call from ANY
//  real-time thread (JACK process callback, ALSA thread, other driver
//  backends, the audio prefetch thread's RT-context message senders, etc).
//  Deliberately has no dependency on any specific audio driver (e.g. NOT
//  built on jack_ringbuffer_t) - JACK support may not be compiled/linked
//  at all in some builds (ALSA-only etc), and this must work regardless.
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

#ifndef __RTLOG_H__
#define __RTLOG_H__

namespace MusECore {

// Producer side - RT-safe, call from any number of concurrent real-time
// threads (MPSC: multi-producer, single-consumer):
//
//   rtLog("track:%s empty_count:%d", name, count);
//
// Never blocks, never locks, never allocates. Formats into a fixed-size
// slot via vsnprintf (computational only - no I/O, no syscalls - but see
// the caveat in rtlog.cpp about glibc locale locking).
// If the queue is full because the consumer isn't draining fast enough,
// the message is silently dropped rather than blocking or spinning:
// losing a diagnostic line is acceptable, stalling a real-time thread
// is not.
void rtLog(const char* fmt, ...)
#if defined(__GNUC__) || defined(__clang__)
  __attribute__((format(printf, 1, 2)))
#endif
;

// Consumer side - call from exactly ONE non-RT thread only (e.g. a GUI
// heartbeat/idle timer). Never call this from an RT thread, and never
// call it concurrently from more than one thread.
// Drains and fprintf()s everything currently queued.
void rtLogFlush();

} // namespace MusECore

#endif
