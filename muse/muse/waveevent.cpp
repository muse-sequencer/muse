//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: waveevent.cpp,v 1.9.2.6 2009/12/20 05:00:35 terminator356 Exp $
//
//  (C) Copyright 2000-2003 Werner Schweer (ws@seh.de)
//=========================================================

#include "globals.h"
#include "event.h"
#include "waveevent.h"
#include "xml.h"
#include "wave.h"
#include <iostream>

// Added by Tim. p3.3.18
//#define WAVEEVENT_DEBUG
//#define USE_SAMPLERATE

//---------------------------------------------------------
//   WaveEvent
//---------------------------------------------------------

WaveEventBase::WaveEventBase(EventType t)
   : EventBase(t)
      {
      deleted = false;
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
                              SndFile* wf = getWave(xml.parse1(), true);
                              if (wf) {
                                    f = SndFileR(wf);
                                    }
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
//      if (offset)
//            wpos.setTick(wpos.tick() + offset);
      wpos.write(level, xml, "poslen");
      xml.intTag(level, "frame", _spos);  // offset in wave file

      //
      // waves in the project dirctory are stored
      // with relative path name, others with absolute path
      //
      QString path = f.dirPath();

      //if (path.contains(museProject)) {
      if (!forcePath && path.contains(museProject)) {
            // extract museProject.
            QString newName = f.path().remove(museProject+"/");
            xml.strTag(level, "file", newName);
            }
      else
            xml.strTag(level, "file", f.path());
      xml.etag(level, "event");
      }

//void WaveEventBase::read(unsigned offset, float** buffer, int channel, int n, bool overwrite)
void WaveEventBase::readAudio(unsigned offset, float** buffer, int channel, int n, bool doSeek, bool overwrite)
      {
      // Added by Tim. p3.3.17
      #ifdef WAVEEVENT_DEBUG
      printf("WaveEventBase::readAudio offset:%u channel:%d n:%d\n", offset, channel, n);
      #endif
      
      if (f.isNull())
            return;
      
      // Changed by Tim. p3.3.18 
      // FIXME: Removed until resampling is enabled.
      #ifdef USE_SAMPLERATE
      if(doSeek)
      #endif
        f.seek(offset + _spos, 0);
        
      f.read(channel, buffer, n, overwrite);
      }
