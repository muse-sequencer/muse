//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: event.cpp,v 1.8.2.5 2009/12/20 05:00:35 terminator356 Exp $
//
//  (C) Copyright 2000-2003 Werner Schweer (ws@seh.de)
//=========================================================

#include <stdio.h>
// #include <memory.h>
#include "audioconvert.h"
#include "event.h"
#include "eventbase.h"
#include "waveevent.h"
#include "midievent.h"
//#include "globals.h"

// Added by Tim. p3.3.20
//#define USE_SAMPLERATE

//---------------------------------------------------------
//   Event
//---------------------------------------------------------

EventBase::EventBase(EventType t)
      {
      _type     = t;
      Pos::setType(_type == Wave ? FRAMES : TICKS);
      refCount  = 0;
      _selected = false;
      }

EventBase::EventBase(const EventBase& ev)
  : PosLen(ev)
      {
      refCount  = 0;
      _selected = ev._selected;
      _type     = ev._type;
      }

//---------------------------------------------------------
//   move
//---------------------------------------------------------

void EventBase::move(int tickOffset)
      {
      setTick(tick() + tickOffset);
      }

//---------------------------------------------------------
//   dump
//---------------------------------------------------------

void EventBase::dump(int n) const
      {
      for (int i = 0; i < n; ++i)
            putchar(' ');
      printf("Event %p refs:%d ", this, refCount);
      PosLen::dump(n+2);
      }

//---------------------------------------------------------
//   clone
//---------------------------------------------------------

Event Event::clone()
      {
      // p3.3.31
      printf("Event::clone() this:%p\n", this);
      
      // p3.3.31
      //return Event(ev->clone());
      #ifdef USE_SAMPLERATE
      return Event(ev->clone(), _audConv);
      #else
      return Event(ev->clone());
      #endif
      }

Event::Event() 
{ 
  ev = 0; 
  _sfCurFrame = 0;
  _audConv = 0;
}

Event::Event(EventType t) {
            _sfCurFrame = 0;
            _audConv = 0;
            
            if (t == Wave)
                  ev = new WaveEventBase(t);
            else
                  ev = new MidiEventBase(t);
            ++(ev->refCount);
            }
Event::Event(const Event& e) {
            _sfCurFrame = 0;
            _audConv = 0;
            
            ev = e.ev;
            if(ev)
              ++(ev->refCount);
            
            #ifdef USE_SAMPLERATE
            //_audConv = AudioConverter::getAudioConverter(e._audConv);
            if(e._audConv)
              _audConv = e._audConv->reference();
            #endif
          }
Event::Event(EventBase* eb) {
            _sfCurFrame = 0;
            _audConv = 0;
            
            ev = eb;
            ++(ev->refCount);
            
            #ifdef USE_SAMPLERATE
            if(!ev->sndFile().isNull())
              //_audConv = AudioConverter::getAudioConverter(eb, SRC_SINC_MEDIUM_QUALITY);
              //_audConv = new AudioConverter(ev->sndFile().channels(), SRC_SINC_MEDIUM_QUALITY);
              _audConv = new SRCAudioConverter(ev->sndFile().channels(), SRC_SINC_MEDIUM_QUALITY);
            #endif  
            }
#ifdef USE_SAMPLERATE
Event::Event(EventBase* eb, AudioConverter* cv) {
            _sfCurFrame = 0;
            _audConv = 0;
            
            ev = eb;
            ++(ev->refCount);
            
            if(cv)
              _audConv = cv->reference();
            }
#endif  
            
Event::~Event() {
            if (ev && --(ev->refCount) == 0) {
                  delete ev;
                  ev=0;
                  }

            #ifdef USE_SAMPLERATE
            AudioConverter::release(_audConv);
            #endif
            }

bool Event::empty() const      { return ev == 0; }
EventType Event::type() const  { return ev ? ev->type() : Note;  }

void Event::setType(EventType t) {
            if (ev && --(ev->refCount) == 0) {
                  delete ev;
                  ev = 0;
                  }
            if (t == Wave)
                  ev = new WaveEventBase(t);
            else
                  ev = new MidiEventBase(t);
            ++(ev->refCount);
            }

Event& Event::operator=(const Event& e) {
            /*
            if (ev == e.ev)
                  return *this;
            if (ev && --(ev->refCount) == 0) {
                  delete ev;
                  ev = 0;
                  }
            ev = e.ev;
            if (ev)
                  ++(ev->refCount);
            return *this;
            */
            
            if (ev != e.ev)
            {
              if (ev && --(ev->refCount) == 0) {
                    delete ev;
                    ev = 0;
                    }
              ev = e.ev;
              if (ev)
                    ++(ev->refCount);
            }      
            
            #ifdef USE_SAMPLERATE
            if (_audConv != e._audConv)
            {
              if(_audConv)
                AudioConverter::release(_audConv);
              //_audConv = AudioConverter::getAudioConverter(e._audConv);
              _audConv = e._audConv->reference();
            }      
            #endif
            return *this;
            }

bool Event::operator==(const Event& e) const {
            return ev == e.ev;
            }

int Event::getRefCount() const    { return ev->getRefCount(); }
bool Event::selected() const      { return ev->_selected; }
void Event::setSelected(bool val) { ev->_selected = val; }
void Event::move(int offset)      { ev->move(offset); }

//void Event::read(Xml& xml)            { ev->read(xml); }
void Event::read(Xml& xml)            
{ 
  ev->read(xml); 
  
  #ifdef USE_SAMPLERATE
  if(!ev->sndFile().isNull())
  {
    if(_audConv)
    {
      _audConv->setChannels(ev->sndFile().channels());
    }
    else
    {
      //int srcerr;
      //if(debugMsg)
      //  printf("Event::read Creating samplerate converter with %d channels\n", ev->sndFile().channels());
      //_src_state = src_new(SRC_SINC_MEDIUM_QUALITY, ev->sndFile().channels(), &srcerr);
//      _audConv = new AudioConverter(ev->sndFile().channels(), SRC_SINC_MEDIUM_QUALITY);
      _audConv = new SRCAudioConverter(ev->sndFile().channels(), SRC_SINC_MEDIUM_QUALITY);
      //if(!_src_state)      
      //if(!_audConv)      
      //  printf("Event::read Creation of samplerate converter with %d channels failed:%s\n", ev->sndFile().channels(), src_strerror(srcerr));
    }  
  }  
  #endif
}


//void Event::write(int a, Xml& xml, const Pos& o) const { ev->write(a, xml, o); }
void Event::write(int a, Xml& xml, const Pos& o, bool forceWavePaths) const { ev->write(a, xml, o, forceWavePaths); }
void Event::dump(int n) const     { ev->dump(n); }
Event Event::mid(unsigned a, unsigned b) { return Event(ev->mid(a, b)); }

bool Event::isNote() const                   { return ev->isNote();        }
bool Event::isNoteOff() const                { return ev->isNoteOff();     }
bool Event::isNoteOff(const Event& e) const  { return ev->isNoteOff(e); }
int Event::dataA() const                     { return ev->dataA();  }
int Event::pitch() const                     { return ev->dataA();  }
void Event::setA(int val)                    { ev->setA(val);       }
void Event::setPitch(int val)                { ev->setA(val);       }
int Event::dataB() const                     { return ev->dataB();  }
int Event::velo() const                      { return ev->dataB();  }
void Event::setB(int val)                    { ev->setB(val);       }
void Event::setVelo(int val)                 { ev->setB(val);       }
int Event::dataC() const                     { return ev->dataC();  }
int Event::veloOff() const                   { return ev->dataC();  }
void Event::setC(int val)                    { ev->setC(val);       }
void Event::setVeloOff(int val)              { ev->setC(val);       }

const unsigned char* Event::data() const     { return ev->data();    }
int Event::dataLen() const                   { return ev->dataLen(); }
void Event::setData(const unsigned char* data, int len) { ev->setData(data, len); }
const EvData Event::eventData() const        { return ev->eventData(); }

const QString Event::name() const            { return ev->name();  }
void Event::setName(const QString& s)        { ev->setName(s);     }
int Event::spos() const                      { return ev->spos();  }
void Event::setSpos(int s)                   { ev->setSpos(s);     }
SndFileR Event::sndFile() const              { return ev->sndFile(); }

//void Event::setSndFile(SndFileR& sf) { ev->setSndFile(sf);   }
void Event::setSndFile(SndFileR& sf) 
{ 
  ev->setSndFile(sf);   
  
  #ifdef USE_SAMPLERATE
  //if(_audConv)
//  if(_audConv && !sf.isNull())
//  { 
    //_audConv->setSndFile(sf);
    //if(sf.isNull())
    //  AudioConverter::release(_audConv);
    //else
//      _audConv->setChannels(sf.channels());
//  }  
  
  if(_audConv)
  { 
    // Do we release? Or keep the converter around, while gaining speed since no rapid creation/destruction. 
    //if(sf.isNull())
    //  _audConv = AudioConverter::release(_audConv);
    //else
    //  _audConv->setChannels(sf.channels());
    if(!sf.isNull())
      _audConv->setChannels(sf.channels());
  }
  else
  {
    if(!sf.isNull())
      _audConv = new SRCAudioConverter(ev->sndFile().channels(), SRC_SINC_MEDIUM_QUALITY);
  }
  #endif
}

//void Event::read(unsigned offset, float** bpp, int channels, int nn, bool overwrite)
void Event::readAudio(unsigned offset, float** bpp, int channels, int nn, bool doSeek, bool overwrite)
      {
        //ev->read(offset, bpp, channels, nn, overwrite);
        //ev->readAudio(offset, bpp, channels, nn, doSeek, overwrite);
        //_sfCurFrame = ev->readAudio(_src_state, _sfCurFrame, offset, bpp, channels, nn, doSeek, overwrite);
        _sfCurFrame = ev->readAudio(_audConv, _sfCurFrame, offset, bpp, channels, nn, doSeek, overwrite);
      }
void Event::setTick(unsigned val)       { ev->setTick(val); }
unsigned Event::tick() const            { return ev->tick(); }
unsigned Event::frame() const           { return ev->frame(); }
void Event::setFrame(unsigned val)      { ev->setFrame(val); }
void Event::setLenTick(unsigned val)    { ev->setLenTick(val); }
void Event::setLenFrame(unsigned val)   { ev->setLenFrame(val); }
unsigned Event::lenTick() const         { return ev->lenTick(); }
unsigned Event::lenFrame() const        { return ev->lenFrame(); }
Pos Event::end() const                  { return ev->end(); }
unsigned Event::endTick() const         { return ev->end().tick(); }
unsigned Event::endFrame() const        { return ev->end().frame(); }
void Event::setPos(const Pos& p)        { ev->setPos(p); }

