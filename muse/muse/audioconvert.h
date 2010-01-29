//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: audioconvert.h,v 1.1.1.1 2009/12/28 16:07:33 terminator356 Exp $
//
//  (C) Copyright 1999-2009 Werner Schweer (ws@seh.de)
//
//  Audio converter module created by Tim terminator356
//=========================================================

#ifndef __AUDIOCONVERT_H__
#define __AUDIOCONVERT_H__

//#include <map>

#ifdef RUBBERBAND_SUPPORT
#include <RubberBandStretcher.h>
#endif

#include <samplerate.h>
#include <sys/types.h>

//#include "eventbase.h"
//class EventBase;
//class EventList;

class SndFileR;

//---------------------------------------------------------
//   AudioConverter
//---------------------------------------------------------

class AudioConverter
{
      int _refCount;
      
   public:   
      AudioConverter();
      ~AudioConverter();
      
      AudioConverter* reference();
      static AudioConverter* release(AudioConverter* cv);
      
      off_t readAudio(SndFileR& /*sf*/, off_t /*sfCurFrame*/, unsigned /*offset*/, float** /*buffer*/, 
                      int /*channels*/, int /*frames*/, bool /*doSeek*/, bool /*overwrite*/);
      
      virtual bool isValid() = 0;
      virtual void reset() = 0;
      virtual void setChannels(int ch) = 0;
      virtual off_t process(SndFileR& /*sf*/, off_t /*sfCurFrame*/, float** /*buffer*/, 
                            int /*channels*/, int /*frames*/, bool /*overwrite*/) = 0; // Interleaved buffer if stereo.
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
      virtual off_t process(SndFileR& /*sf*/, off_t /*sfCurFrame*/, float** /*buffer*/, 
                            int /*channels*/, int /*frames*/, bool /*overwrite*/); // Interleaved buffer if stereo.
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
      virtual off_t process(SndFileR& /*sf*/, off_t /*sfCurFrame*/, float** /*buffer*/, 
                            int /*channels*/, int /*frames*/, bool /*overwrite*/); // Interleaved buffer if stereo.
};

#endif // RUBBERBAND_SUPPORT

//---------------------------------------------------------
//   AudioConvertMap
//---------------------------------------------------------

/*
typedef std::map<EventBase*, AudioConverter*, std::less<EventBase*> >::iterator iAudioConvertMap;
typedef std::map<EventBase*, AudioConverter*, std::less<EventBase*> >::const_iterator ciAudioConvertMap;

//typedef std::map<EventBase*, AudioConverter*, std::less<EventBase*> > AudioConvertMap;
class AudioConvertMap : public std::map<EventBase*, AudioConverter*, std::less<EventBase*> > 
{
   public:
      void remapEvents(const EventList*);  
      iAudioConvertMap addEventBase(const EventBase*);
      AudioConverter* findConverter(const EventBase*);
};
*/

#endif

