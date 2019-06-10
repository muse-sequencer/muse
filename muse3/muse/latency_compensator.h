//=========================================================
//  MusE
//  Linux Music Editor
//
//  latency_compensator.h
//  (C) Copyright 2016, 2019 Tim E. Real (terminator356 on users dot sourceforge dot net)
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

#ifndef __LATENCY_COMPENSATOR_H__
#define __LATENCY_COMPENSATOR_H__

namespace MusECore {

class LatencyCompensator
{
  private:
    int _channels;
    unsigned long   _bufferSize; // Must be power of two.
    unsigned long   _bufferSizeMask;
    unsigned long*  _readPointers;
    float** _buffer;
    // Channels that have been peeked and require an advance.
    bool* _peekedChannels;

  public:
    LatencyCompensator(unsigned long bufferSize = 16384) : 
      _channels(0), _bufferSize(bufferSize), _bufferSizeMask(bufferSize - 1), _readPointers(0), _buffer(0), _peekedChannels(0) { }
    LatencyCompensator(int channels, unsigned long bufferSize = 16384);
    virtual ~LatencyCompensator();
    
    void clear();
    void setBufferSize(unsigned long size);
    void setChannels(int channels);
    
    // Write a block of data on each channel at the given write offsets (from the read position).
    // All writes are additive. Read will clear the blocks.
    void write(unsigned long sampleCount, const unsigned long* const writeOffsets, const float* const* data);
    // Convenient single channel version of write.
    void write(int channel, unsigned long sampleCount, unsigned long writeOffset, const float* const data);
    // Convenient version of write with common write offset for all channels.
    void write(unsigned long sampleCount, unsigned long writeOffset, const float* const* data);
    
    // Read a block of data on each channel.
    // The internal read pointer is advanced to the next position.
    // The block is cleared after a read.
    void read(unsigned long sampleCount, float** data);
    // Convenient single channel version of read.
    void read(int channel, unsigned long sampleCount, float* data);
    
    // Read (peek) a block of data on each channel.
    // The internal read pointer is NOT advanced to the next position.
    // The block is NOT cleared after a peek.
    // This allows further data to be written after a peek, before a full read.
    void peek(unsigned long sampleCount, float** data);
    // Convenient single channel version of peek.
    void peek(int channel, unsigned long sampleCount, float* data);
    
    // Manually advances all channels' read pointers, but only if they were peeked.
    // This also clears the blocks.
    void advance(unsigned long sampleCount);
    // Manually advances a channel's read pointer, but only if it was peeked.
    // This also clears the block.
    void advance(int channel, unsigned long sampleCount);
};

} // namespace MusECore

#endif
