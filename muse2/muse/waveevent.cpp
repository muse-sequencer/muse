//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: waveevent.cpp,v 1.9.2.6 2009/12/20 05:00:35 terminator356 Exp $
//
//  (C) Copyright 2000-2003 Werner Schweer (ws@seh.de)
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

#include "globals.h"
#include "event.h"
#include "waveevent.h"
#include "xml.h"
#include "wave.h"
#include <iostream>
#include <math.h>

//#define WAVEEVENT_DEBUG
//#define WAVEEVENT_DEBUG_PRC

namespace MusECore {

//---------------------------------------------------------
//   WaveEvent
//---------------------------------------------------------

WaveEventBase::WaveEventBase(EventType t)
   : EventBase(t)
      {
      deleted = false;
      _spos = 0;
      }

//---------------------------------------------------------
//   WaveEventBase::clone
//---------------------------------------------------------

EventBase* WaveEventBase::clone() 
{ 
  return new WaveEventBase(*this); 
}

//---------------------------------------------------------
//   WaveEvent::mid
//---------------------------------------------------------

EventBase* WaveEventBase::mid(unsigned b, unsigned e)
      {
      WaveEventBase* ev = new WaveEventBase(*this);
      unsigned fr = frame();
      unsigned start = fr - b;
      if(b > fr)
      {  
        start = 0;
        ev->setSpos(spos() + b - fr);
      }
      unsigned end = endFrame();
      
      if (e < end)
            end = e;

      ev->setFrame(start);
      ev->setLenFrame(end - b - start);
      return ev;
      }

//---------------------------------------------------------
//   dump
//---------------------------------------------------------

void WaveEventBase::dump(int n) const
      {
      EventBase::dump(n);
      }

//---------------------------------------------------------
//   WaveEventBase::read
//---------------------------------------------------------

void WaveEventBase::read(Xml& xml)
      {
      for (;;) {
            Xml::Token token = xml.parse();
            const QString& tag = xml.s1();
            switch (token) {
                  case Xml::Error:
                  case Xml::End:
                  case Xml::Attribut:
                        return;
                  case Xml::TagStart:
                        if (tag == "poslen")
                              PosLen::read(xml, "poslen");
                        else if (tag == "frame")
                              _spos = xml.parseInt();
                        else if (tag == "file") {
                              SndFileR wf = getWave(xml.parse1(), true);
                              if (wf) f = wf;
                              }
                        else
                              xml.unknown("Event");
                        break;
                  case Xml::TagEnd:
                        if (tag == "event") {
                              Pos::setType(FRAMES);   // DEBUG
                              return;
                              }
                  default:
                        break;
                  }
            }
      }

//---------------------------------------------------------
//   write
//---------------------------------------------------------

//void WaveEventBase::write(int level, Xml& xml, const Pos& offset) const
void WaveEventBase::write(int level, Xml& xml, const Pos& offset, bool forcePath) const
      {
      if (f.isNull())
            return;
      xml.tag(level++, "event");
      PosLen wpos(*this);
      wpos += offset;
      wpos.write(level, xml, "poslen");
      xml.intTag(level, "frame", _spos);  // offset in wave file

      //
      // waves in the project dirctory are stored
      // with relative path name, others with absolute path
      //
      QString path = f.dirPath();

      if (!forcePath && path.contains(MusEGlobal::museProject)) {
            // extract MusEGlobal::museProject.
            QString newName = f.path().remove(MusEGlobal::museProject+"/");
            xml.strTag(level, "file", newName);
            }
      else
            xml.strTag(level, "file", f.path());
      xml.etag(level, "event");
      }

void WaveEventBase::readAudio(WavePart* part, unsigned firstFrame, float** buffer, int channel, int nFrames, XTick fromXTick, XTick toXTick, bool doSeek, bool overwrite)
{
// TODO: doSeek is unreliable! it does not respect moving the part or event
// offset is the sample position to read from. usually, that's the last offset + the last n

// TODO: this will horribly break with clone parts! each clone must have its own audiostream (and streamPosition)!.
  
  #ifdef WAVEEVENT_DEBUG_PRC
  printf("WaveEventBase::readAudio audConv:%p sfCurFrame:%ld offset:%u channel:%d n:%d\n", audConv, sfCurFrame, offset, channel, n);
  #endif
  
  
  if (doSeek || firstFrame != streamPosition)
  {
    if (!doSeek && firstFrame >= streamPosition && firstFrame <= streamPosition+STREAM_SEEK_THRESHOLD)
    {
      // reading some frames from the stream into /dev/null is enough.
      fprintf(stderr, "DEBUG: WaveEventBase::readAudio has to drop frames, diff is %u, threshold is %u!\n",(int)(firstFrame-streamPosition),STREAM_SEEK_THRESHOLD);
      
      int len = audiostream.readAudio(NULL, channel, firstFrame-streamPosition, false);
      streamPosition += len;
    }
    else
    {
      // we need to seek.
      // this might cause crackles and thus is only rarely done.
      fprintf(stderr, "DEBUG: WaveEventBase::readAudio has to seek, diff is %u, threshold is %u!\n",(int)(firstFrame-streamPosition),STREAM_SEEK_THRESHOLD);

      streamPosition = audiostream.seek(firstFrame,fromXTick);
    }
  }
  
  // update the stretch keyframe
  audiostream.setNextStretchingKeyframe(firstFrame+nFrames, toXTick);
  
  streamPosition += audiostream.readAudio(buffer, channel, nFrames, overwrite);
  // now streamPosition == firstFrame+nFrames 
}

} // namespace MusECore
