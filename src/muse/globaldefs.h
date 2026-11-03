//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: globaldefs.h,v 1.3.2.1 2009/05/03 04:14:00 terminator356 Exp $
//
//  (C) Copyright 2000 Werner Schweer (ws@seh.de)
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

#ifndef __GLOBALDEFS_H__
#define __GLOBALDEFS_H__

#include <cstdint>
#include <cstdlib>

namespace MusECore {

//---------------------------------------------------------
//   museAlignedMalloc / museAlignedFree
//    Portable 16-byte-aligned allocation for SIMD audio buffers, used
//    throughout this codebase (see AudioTrack::init_buffers() and many
//    other call sites). Originally _aligned_malloc()/_aligned_free() on
//    _WIN32, posix_memalign()/free() elsewhere.
//
//    A real _aligned_malloc()/free() mismatch bug (fixed: every release
//    site now goes through museAlignedFree() instead of a bare free())
//    was confirmed via Windows page heap as "corrupted start stamp".
//    After fixing every such site, page heap still deterministically
//    reports a *different* corruption ("corrupted suffix pattern") on
//    the exact same kind of buffer, every run, always at the exact same
//    byte offset relative to the block's start (2071 = the requested
//    2048-byte payload plus 23 bytes - precisely the maximum padding
//    _aligned_malloc(ptr, 16) can add for a 16-byte alignment request).
//    That precision, and total insensitivity to unrelated code fixes
//    (including a confirmed-clean ThreadSanitizer pass), points at an
//    interaction between MinGW-w64 UCRT's _aligned_malloc()/
//    _aligned_free() bookkeeping and Windows page heap's own guard
//    bytes, not an application-level bug. This portable manual
//    implementation (over-allocate, align, stash the real pointer just
//    before the aligned one - the same technique posix_memalign uses
//    internally) sidesteps the CRT's own aligned allocator entirely, as
//    a test of that hypothesis.
//---------------------------------------------------------

static inline void* museAlignedMalloc(size_t alignment, size_t size)
{
  // Room for the requested size, alignment slack, and the stashed
  // original-pointer slot right before the aligned address.
  void* raw = malloc(size + alignment - 1 + sizeof(void*));
  if(!raw)
    return nullptr;
  uintptr_t rawAddr = (uintptr_t)raw + sizeof(void*);
  uintptr_t alignedAddr = (rawAddr + alignment - 1) & ~(uintptr_t)(alignment - 1);
  ((void**)alignedAddr)[-1] = raw;
  return (void*)alignedAddr;
}

static inline void museAlignedFree(void* ptr)
{
  if(ptr)
    free(((void**)ptr)[-1]);
}

// Midi Type
//    MT_GM  - General Midi
//    MT_GS  - Roland GS
//    MT_XG  - Yamaha XG
//    MT_GM2 - General Midi Level 2

enum MType { MT_UNKNOWN=0, MT_GM, MT_GS, MT_XG, MT_GM2 };

enum AutomationType {
      AUTO_OFF, AUTO_READ, AUTO_TOUCH, AUTO_WRITE,
      AUTO_LATCH
      };

// Record events ring buffer size
#define MIDI_REC_FIFO_SIZE  256

// Absolute max number of plugins in mixer rack (if we ever want to increase PipelineDepth).
// Used to determine the index where special blocks (dssi ladspa controls) appear in the list of controllers.
// The special block(s) must appear AFTER any rack plugins, so we need this variable to help us
//  leave some room in case we ever want to increase the number of rack plugins.
const int MAX_PLUGINS  = 8;

// plugins in mixer rack, max up to MAX_PLUGINS
const int PipelineDepth = 8;

// max audio channels
const int MAX_CHANNELS = 2;

// max Number of Midi Ports
const int MIDI_PORTS   = 200;

// Midi channels per Port
const int MUSE_MIDI_CHANNELS = 16;

const double MIN_TEMPO_VAL = 20.0;
const double MAX_TEMPO_VAL = 400.0;

// Some non controller IDs to work with.
enum NonControllerId {
  NCTL_UNKNOWN_ID = -1,
  NCTL_TRACK_MUTE = 0,
  NCTL_TRACK_SOLO,
  // Various track properties.
  NCTL_TRACKPROP_TRANSPOSE,
  NCTL_TRACKPROP_DELAY,
  NCTL_TRACKPROP_LENGTH,
  NCTL_TRACKPROP_VELOCITY,
  NCTL_TRACKPROP_COMPRESS };

} // namespace MusECore


namespace MusEGui {
  
enum EditInstrumentTabType {
  EditInstrumentPatches=0,
  EditInstrumentDrumMaps=1, 
  EditInstrumentControllers=2, 
  EditInstrumentSysex=3, 
  EditInstrumentInitSeq=4 };

enum class MidiEventColorMode {
  blueEvents,
  pitchColorEvents,
  velocityColorEvents,
  lastInList
};

// The default amount of space before bar # 1 (or the start of a part).
const int DefaultCanvasXOrigin = -16;

} // namespace MusEGui


#endif

