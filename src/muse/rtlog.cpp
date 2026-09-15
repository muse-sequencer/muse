//=========================================================
//  MusE
//  Linux Music Editor
//  rtlog.cpp
//
//  Lock-free MPSC (multi-producer, single-consumer) diagnostic logging
//  queue for real-time threads.
//
//  Design: a bounded ring buffer using Dmitry Vyukov's lock-free MPMC
//  algorithm (per-cell sequence numbers instead of a single shared
//  head/tail pair), used here in its MPSC form - any number of producer
//  threads may call rtLog() concurrently and safely; rtLogFlush() must
//  only ever be called from a single non-RT thread.
//
//  Multiple concurrent RT producers are a real scenario here, not a
//  theoretical one: the JACK process callback, an ALSA thread, and the
//  audio prefetch thread's RT-context senders (AudioPrefetch::msgSeek())
//  can all legitimately want to log around the same time.
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

#include "rtlog.h"

#include <cstdio>
#include <cstdarg>
#include <cstdint>
#include <cstddef>
#include <atomic>

namespace MusECore {

namespace {

// Must be a power of two (see '& RTLOG_MASK' usage below).
constexpr size_t RTLOG_CAPACITY = 512;
constexpr size_t RTLOG_MASK     = RTLOG_CAPACITY - 1;
// Fixed message length - no heap allocation, ever.
constexpr size_t RTLOG_MSG_LEN  = 200;

struct Cell
{
  std::atomic<size_t> sequence;
  char text[RTLOG_MSG_LEN];
};

Cell g_cells[RTLOG_CAPACITY];
std::atomic<size_t> g_enqueuePos{0};
std::atomic<size_t> g_dequeuePos{0};

struct RtLogInit
{
  // Cell's default construction already zero-initializes 'sequence' to 0,
  // but the algorithm requires sequence[i] == i for every slot up front.
  RtLogInit()
  {
    for(size_t i = 0; i < RTLOG_CAPACITY; ++i)
      g_cells[i].sequence.store(i, std::memory_order_relaxed);
  }
};
// Runs once before main(), via static initialization. rtLog()/rtLogFlush()
// must not be called before static initialization has completed anyway
// (true of any global in any translation unit), so this is safe.
RtLogInit g_rtLogInit;

} // namespace

void rtLog(const char* fmt, ...)
{
  size_t pos = g_enqueuePos.load(std::memory_order_relaxed);
  Cell* cell = nullptr;

  // Claim a slot. Lock-free: multiple producer threads may race here
  // concurrently and safely - that is the entire point of this loop.
  for(;;)
  {
    cell = &g_cells[pos & RTLOG_MASK];
    const size_t seq = cell->sequence.load(std::memory_order_acquire);
    const intptr_t diff = (intptr_t)seq - (intptr_t)pos;

    if(diff == 0)
    {
      // Slot looks free. Try to claim it.
      if(g_enqueuePos.compare_exchange_weak(pos, pos + 1, std::memory_order_relaxed))
        break;
      // CAS failed (another producer claimed it first). compare_exchange_weak
      // already refreshed 'pos' to the current value - just retry.
    }
    else if(diff < 0)
    {
      // Queue is full - the consumer isn't keeping up. Drop this message
      // rather than blocking or spinning indefinitely: losing a diagnostic
      // line is acceptable, stalling a real-time thread is not.
      return;
    }
    else
    {
      // Another producer already claimed this slot; refresh and retry.
      pos = g_enqueuePos.load(std::memory_order_relaxed);
    }
  }

  // We now exclusively own this cell - no other thread will touch it until
  // we publish below. vsnprintf is computational only (writes into our
  // already-owned fixed buffer): no locks, no syscalls, no heap allocation.
  // Caveat: some libc implementations take an internal lock the first time
  // to initialize locale/conversion state; in practice this is a one-time,
  // negligible cost, not a per-call blocking risk, but worth knowing if you
  // are chasing sub-microsecond determinism rather than avoiding dropouts.
  va_list args;
  va_start(args, fmt);
  vsnprintf(cell->text, RTLOG_MSG_LEN, fmt, args);
  va_end(args);

  // Publish: make the message visible to the consumer.
  cell->sequence.store(pos + 1, std::memory_order_release);
}

void rtLogFlush()
{
  // Single-consumer only - do not call this from more than one thread,
  // and never from an RT thread (fprintf() below is blocking I/O).
  size_t pos = g_dequeuePos.load(std::memory_order_relaxed);

  for(;;)
  {
    Cell& cell = g_cells[pos & RTLOG_MASK];
    const size_t seq = cell.sequence.load(std::memory_order_acquire);
    const intptr_t diff = (intptr_t)seq - (intptr_t)(pos + 1);

    if(diff == 0)
    {
      // Message ready - print it.
      fprintf(stderr, "%s\n", cell.text);

      // Free the slot for producers to reuse one lap later.
      cell.sequence.store(pos + RTLOG_CAPACITY, std::memory_order_release);
      ++pos;
      g_dequeuePos.store(pos, std::memory_order_relaxed);
    }
    else if(diff < 0)
    {
      // Nothing more queued right now.
      break;
    }
    else
    {
      // Shouldn't normally happen with a single consumer, but stay safe
      // rather than looping on stale state.
      pos = g_dequeuePos.load(std::memory_order_relaxed);
    }
  }
}

} // namespace MusECore
