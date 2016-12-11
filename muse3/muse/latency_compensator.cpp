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

#include <string.h>

#include "latency_compensator.h"

#define DELAY_BUFFER_SIZE 16384

namespace MusECore {

LatencyCompensator::LatencyCompensator(unsigned long channels, unsigned long bufferSize)
  : _channels(channels), _bufferSize(bufferSize)
{
  _buffer = new float*[channels];
  _delays = new unsigned long[channels];
  _writePointers = new unsigned long[channels];

  for(int i = 0; i < _channels; ++i)
  {
    _buffer[i] = new float[_bufferSize];
    memset(_buffer[i],  0, sizeof(float) * _bufferSize);
    _delays[i] = 0;
    _writePointers[i] = 0;
  }
}

LatencyCompensator::~LatencyCompensator()
{
  for(int i = 0; i < _channels; ++i)
    delete [] _buffer[i];
  delete [] _buffer;
  delete [] _delays;
  delete [] _writePointers;
}

void LatencyCompensator::clear()
{
  for(int i = 0; i < _channels; ++i)
    memset(_buffer[i],  0, sizeof(float) * _bufferSize);
}

void LatencyCompensator::setBufferSize(unsigned long size)
{
  _bufferSize = size;
  for(int i = 0; i < _channels; ++i)
  {
    delete [] _buffer[i];
    _buffer[i] = new float[_bufferSize];
    memset(_buffer[i],  0, sizeof(float) * _bufferSize);
  }
}

void LatencyCompensator::setChannels(unsigned long channels)
{
  for(int i = 0; i < _channels; ++i)
    delete [] _buffer[i];
  delete [] _buffer;
  delete [] _delays;
  delete [] _writePointers;

  _buffer = new float*[channels];
  _delays = new unsigned long[channels];
  _writePointers = new unsigned long[channels];

  for(int i = 0; i < _channels; ++i)
  {
    _buffer[i] = new float[_bufferSize];
    memset(_buffer[i],  0, sizeof(float) * _bufferSize);
    _delays[i] = 0;
    _writePointers[i] = 0;
  }
}

void LatencyCompensator::run(unsigned long SampleCount, float** data)
{
  float inputSample;
  unsigned long readOffset;
  unsigned long bufsz_mask;
  unsigned long writeOffset;
  unsigned long i;

  bufsz_mask = _bufferSize - 1;
  
  float* input;
  float* output;
  float* buf;
  
  for(int ch = 0; ch < _channels; ++ch)
  {
    input = data[ch];
    output = data[ch];
    buf = _buffer[ch];

    writeOffset = _writePointers[ch];
    readOffset = writeOffset + _bufferSize - _delays[ch];
    
    for(i = 0; i < SampleCount; i++) 
    {
      inputSample = *(input++);

      *(output++) = (buf[((i + readOffset) & bufsz_mask)]);
      //*(output++) = (buf[((i + readOffset) % _bufferSize)]);

      buf[((i + writeOffset) & bufsz_mask)] = inputSample;
      //buf[((i + writeOffset) % _bufferSize)] = inputSample;
    }
    
    _writePointers[ch] = (_writePointers[ch] + SampleCount) & bufsz_mask;
    //_writePointers[ch] = (_writePointers[ch] + SampleCount) % _bufferSize;
  }
}
  
} // namespace MusECore
