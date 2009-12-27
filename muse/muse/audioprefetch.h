//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: audioprefetch.h,v 1.3.2.2 2009/12/20 05:00:35 terminator356 Exp $
//
//  (C) Copyright 2001 Werner Schweer (ws@seh.de)
//=========================================================

#ifndef __AUDIOPREFETCH_H__
#define __AUDIOPREFETCH_H__

#include "thread.h"

//---------------------------------------------------------
//   AudioPrefetch
//---------------------------------------------------------

class AudioPrefetch : public Thread {
      unsigned writePos;
      unsigned seekPos; // remember last seek to optimize seeks

      virtual void processMsg1(const void*);
      //void prefetch();
      void prefetch(bool doSeek);
      void seek(unsigned pos);

   public:
      //AudioPrefetch(int prio, const char* name);
      AudioPrefetch(const char* name);
      
      ~AudioPrefetch();
      //virtual void start();
      virtual void start(int);

      void msgTick();
      void msgSeek(unsigned samplePos, bool force=false);
      volatile bool seekDone;
      };

extern AudioPrefetch* audioPrefetch;

#endif
