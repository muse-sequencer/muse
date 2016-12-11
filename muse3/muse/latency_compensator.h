//=========================================================
//  MusE
//  Linux Music Editor
//
//  latency_compensator.h
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
//=========================================================

#ifndef __LATENCY_COMPENSATOR_H__
#define __LATENCY_COMPENSATOR_H__

namespace MusECore {

class LatencyCompensator
{
  private:
    unsigned long _channels;
    unsigned long   _bufferSize; // Must be power of two.
    unsigned long*  _writePointers;
    unsigned long*  _delays; // One for each channel. In samples.
    float** _input;
    float** _output;
    float** _buffer;

  public:
    LatencyCompensator(unsigned long channels = 1, unsigned long bufferSize = 16384);
    virtual ~LatencyCompensator();
    
    void clear();
    void setBufferSize(unsigned long size);
    void setChannels(unsigned long channels);
    void run(unsigned long SampleCount, float** data);
};

} // namespace MusECore

#endif
