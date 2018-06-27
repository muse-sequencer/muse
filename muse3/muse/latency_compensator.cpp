//===================================================================
//  MusE
//  Linux Music Editor
//
//  latency_compensator.cpp
//  (C) Copyright 2016 Tim E. Real (terminator356 on users dot sourceforge dot net)
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
//===================================================================

#include <cstring>

#include "latency_compensator.h"

namespace MusECore {

LatencyCompensator::LatencyCompensator(int channels, unsigned long bufferSize)
  : _channels(channels), _bufferSize(bufferSize)
{
  _buffer = new float*[_channels];
  _readPointers = new unsigned long[_channels];

  for(int i = 0; i < _channels; ++i)
  {
    _buffer[i] = new float[_bufferSize];
    std::memset(_buffer[i],  0, sizeof(float) * _bufferSize);
    _readPointers[i] = 0;
  }
}

LatencyCompensator::~LatencyCompensator()
{
  if(_buffer)
  {
    for(int i = 0; i < _channels; ++i)
      delete [] _buffer[i];
    delete [] _buffer;
  }
  if(_readPointers)
    delete [] _readPointers;
}

void LatencyCompensator::clear()
{
  for(int i = 0; i < _channels; ++i)
    std::memset(_buffer[i],  0, sizeof(float) * _bufferSize);
}

void LatencyCompensator::setBufferSize(unsigned long size)
{
  _bufferSize = size;
  if(_buffer)
  {
    for(int i = 0; i < _channels; ++i)
    {
      delete [] _buffer[i];
      _buffer[i] = new float[_bufferSize];
      std::memset(_buffer[i],  0, sizeof(float) * _bufferSize);
      _readPointers[i] = 0;
    }
  }
}

void LatencyCompensator::setChannels(int channels)
{
  if(_buffer)
  {
    for(int i = 0; i < _channels; ++i)
      delete [] _buffer[i];
    delete [] _buffer;
  }
  if(_readPointers)
    delete [] _readPointers;

  _channels = channels;
  if(_channels > 0)
  {
    _buffer = new float*[_channels];
    _readPointers = new unsigned long[_channels];

    for(int i = 0; i < _channels; ++i)
    {
      _buffer[i] = new float[_bufferSize];
      std::memset(_buffer[i],  0, sizeof(float) * _bufferSize);
      _readPointers[i] = 0;
    }
  }
}

// void LatencyCompensator::run(unsigned long SampleCount, float** data)
// {
//   float inputSample;
//   unsigned long readOffset;
//   unsigned long bufsz_mask;
//   unsigned long writeOffset;
//   unsigned long i;
// 
//   bufsz_mask = _bufferSize - 1;
//   
//   float* input;
//   float* output;
//   float* buf;
//   
//   for(int ch = 0; ch < _channels; ++ch)
//   {
//     input = data[ch];
//     output = data[ch];
//     buf = _buffer[ch];
// 
//     writeOffset = _writePointers[ch];
//     readOffset = writeOffset + _bufferSize - _delays[ch];
//     //readOffset = writeOffset + _bufferSize - (_delays[ch] & bufsz_mask);
//     
//     for(i = 0; i < SampleCount; i++) 
//     {
//       inputSample = *(input++);
// 
//       *(output++) = (buf[((i + readOffset) & bufsz_mask)]);
//       //*(output++) = (buf[((i + readOffset) % _bufferSize)]);
// 
//       buf[((i + writeOffset) & bufsz_mask)] = inputSample;
//       //buf[((i + writeOffset) % _bufferSize)] = inputSample;
//     }
//     
//     _writePointers[ch] = (_writePointers[ch] + SampleCount) & bufsz_mask;
//     //_writePointers[ch] = (_writePointers[ch] + SampleCount) % _bufferSize;
//   }
// }
  
void LatencyCompensator::read(unsigned long sampleCount, float** data)
{
  unsigned long bufsz_mask = _bufferSize - 1;
  unsigned long read_position, i, idx;
  float *output, *buf;
  
  for(int ch = 0; ch < _channels; ++ch)
  {
    output = data[ch];
    buf = _buffer[ch];
    read_position = _readPointers[ch];
    for(i = 0; i < sampleCount; i++) 
    {
      idx = (i + read_position) & bufsz_mask;
      *(output++) = buf[idx];
      // Clear the data so that the next time around, simple addition of data can be used.
      // Like the erase head of a (imaginary) multi-record-head latency-correction tape loop
      //  mechanism, where in this simulation the erase head is just after the read head.
      buf[idx] = 0.0f;
    }
    _readPointers[ch] = (_readPointers[ch] + sampleCount) & bufsz_mask;
  }
}

void LatencyCompensator::read(int channel, unsigned long sampleCount, float* data)
{
  if(channel >= _channels)
    return;
  
  unsigned long bufsz_mask = _bufferSize - 1;
  unsigned long read_position, i, idx;
  float *output, *buf;
  
  output = data;
  buf = _buffer[channel];
  read_position = _readPointers[channel];
  for(i = 0; i < sampleCount; i++) 
  {
    idx = (i + read_position) & bufsz_mask;
    *(output++) = buf[idx];
    // Clear the data so that the next time around, simple addition of data can be used.
    // Like the erase head of a (imaginary) multi-record-head latency-correction tape loop
    //  mechanism, where in this simulation the erase head is just after the read head.
    buf[idx] = 0.0f;
  }
  _readPointers[channel] = (_readPointers[channel] + sampleCount) & bufsz_mask;
}

void LatencyCompensator::write(unsigned long sampleCount, const unsigned long* const writeOffsets, const float* const* data)
{
  unsigned long bufsz_mask = _bufferSize - 1;
  unsigned long write_position, i;
  const float *input;
  float *buf;
  
  for(int ch = 0; ch < _channels; ++ch)
  {
    input = data[ch];
    buf = _buffer[ch];
    write_position = _readPointers[ch] + writeOffsets[ch];
    for(i = 0; i < sampleCount; i++) 
      buf[((i + write_position) & bufsz_mask)] += *(input++);
  }
}

void LatencyCompensator::write(int channel, unsigned long sampleCount, unsigned long writeOffset, const float* const data)
{
  if(channel >= _channels)
    return;
  
  unsigned long bufsz_mask = _bufferSize - 1;
  unsigned long write_position, i;
  const float *input;
  float *buf;
  
  input = data;
  buf = _buffer[channel];
  write_position = _readPointers[channel] + writeOffset;
  for(i = 0; i < sampleCount; i++) 
    buf[((i + write_position) & bufsz_mask)] += *(input++);
}
    
void LatencyCompensator::write(unsigned long sampleCount, unsigned long writeOffset, const float* const* data)
{
  unsigned long bufsz_mask = _bufferSize - 1;
  unsigned long write_position, i;
  const float *input;
  float *buf;
  
  for(int ch = 0; ch < _channels; ++ch)
  {
    input = data[ch];
    buf = _buffer[ch];
    write_position = _readPointers[ch] + writeOffset;
    for(i = 0; i < sampleCount; i++) 
      buf[((i + write_position) & bufsz_mask)] += *(input++);
  }
}

} // namespace MusECore
