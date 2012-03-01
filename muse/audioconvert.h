//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: audioconvert.h,v 1.1.1.1 2009/12/28 16:07:33 terminator356 Exp $
//
//  (C) Copyright 1999-2009 Werner Schweer (ws@seh.de)
//
//  Audio converter module created by Tim 
//  (C) Copyright 2009-2011 Tim E. Real (terminator356 A T sourceforge D O T net)
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

#ifndef __AUDIOCONVERT_H__
#define __AUDIOCONVERT_H__

#include <map>

#ifdef RUBBERBAND_SUPPORT
#include <RubberBandStretcher.h>
#endif

#include <samplerate.h>
#include <sys/types.h>


namespace MusECore {
class EventBase;
class EventList;
class SndFileR;


//---------------------------------------------------------
//   AudioConverter
//---------------------------------------------------------

class AudioConverter
{
   protected:   
      int _refCount;
      off_t _sfCurFrame;
      
   public:   
      AudioConverter();
      ~AudioConverter();
      
      AudioConverter* reference();
      static AudioConverter* release(AudioConverter* cv);
      
      off_t readAudio(MusECore::SndFileR& sf, unsigned offset, float** buffer, 
                      int channels, int frames, bool doSeek, bool overwrite);
      
      virtual bool isValid() = 0;
      virtual void reset() = 0;
      virtual void setChannels(int ch) = 0;
      virtual off_t process(MusECore::SndFileR& sf, float** buffer, 
                            int channels, int frames, bool overwrite) = 0; // Interleaved buffer if stereo.
};

//---------------------------------------------------------
//   SRCAudioConverter
//---------------------------------------------------------

class SRCAudioConverter : public AudioConverter
{
      int _type;
      int _channels;
      SRC_STATE* _src_state;
   
   public:   
      SRCAudioConverter(int channels, int type);
      ~SRCAudioConverter();
      
      virtual bool isValid() { return _src_state != 0; }
      virtual void reset();
      virtual void setChannels(int ch);
      virtual off_t process(MusECore::SndFileR& sf, float** buffer, 
                            int channels, int frames, bool overwrite); // Interleaved buffer if stereo.
};

#ifdef RUBBERBAND_SUPPORT

//---------------------------------------------------------
//   RubberBandAudioConverter
//---------------------------------------------------------

class RubberBandAudioConverter : public AudioConverter
{
      int _options;
      int _channels;
      RubberBandStretcher* _rbs;
   
   public:   
      RubberBandAudioConverter(int channels, int options);
      ~RubberBandAudioConverter();
      
      virtual bool isValid() { return _rbs != 0; }
      virtual void reset();
      virtual void setChannels(int ch);
      virtual off_t process(MusECore::SndFileR& sf, float** buffer, 
                            int channels, int frames, bool overwrite); // Interleaved buffer if stereo.
};

#endif // RUBBERBAND_SUPPORT

//---------------------------------------------------------
//   AudioConvertMap
//---------------------------------------------------------

typedef std::map<EventBase*, AudioConverter*, std::less<EventBase*> >::iterator iAudioConvertMap;
typedef std::map<EventBase*, AudioConverter*, std::less<EventBase*> >::const_iterator ciAudioConvertMap;

class AudioConvertMap : public std::map<EventBase*, AudioConverter*, std::less<EventBase*> > 
{
   public:
      void remapEvents(const EventList*);  
      iAudioConvertMap addEvent(EventBase*);
      void removeEvent(EventBase*);
      iAudioConvertMap getConverter(EventBase*);
};

} // namespace MusECore

#endif

