//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: audioconvert.cpp,v 1.1.1.1 2009/12/28 16:07:33 terminator356 Exp $
//
//  (C) Copyright 1999-2009 Werner Schweer (ws@seh.de)
//
//  Audio converter module created by Tim
//  (C) Copyright 2009-2020 Tim E. Real (terminator356 A T sourceforge D O T net)
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
//
//=========================================================

#include <cstring>
#include <stdio.h>

#include "audioconvert.h"
#include "muse_math.h"

// For debugging output: Uncomment the fprintf section.
#define ERROR_AUDIOCONVERT(dev, format, args...) fprintf(dev, format, ##args)
#define DEBUG_AUDIOCONVERT(dev, format, args...) // fprintf(dev, format, ##args)

namespace MusECore {

//---------------------------------------------------------
//   AudioConverter
//---------------------------------------------------------

AudioConverter::AudioConverter(int systemSampleRate) :
  _systemSampleRate(systemSampleRate), _channels(0), _refCount(1)
{
  DEBUG_AUDIOCONVERT(stderr, "AudioConverter::AudioConverter this:%p\n", this);
}

AudioConverter::~AudioConverter()
{
  DEBUG_AUDIOCONVERT(stderr, "AudioConverter::~AudioConverter this:%p\n", this);
}

AudioConverterHandle AudioConverter::reference()
{
  _refCount += 1;
  DEBUG_AUDIOCONVERT(stderr, "AudioConverter::reference this:%p current refcount:%d\n", this, _refCount);
  return this;
}

// Static.
AudioConverterHandle AudioConverter::release(AudioConverter* cv)
{
  if(!cv)
    return 0;
  cv->_refCount -= 1;
  DEBUG_AUDIOCONVERT(stderr, "AudioConverter::release converter:%p current refcount:%d\n", cv, cv->_refCount);
  if(cv->_refCount <= 0)
  {
    DEBUG_AUDIOCONVERT(stderr, "AudioConverter::release deleting converter:%p\n", cv);
    delete cv;
    cv = 0;
  }
  return cv;  
}


} // namespace MusECore
