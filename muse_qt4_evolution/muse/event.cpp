//=============================================================================
//  MusE
//  Linux Music Editor
//  $Id:$
//
//  Copyright (C) 2002-2006 by Werner Schweer and others
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License version 2.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//=============================================================================

#include "event.h"
#include "eventbase.h"
#include "waveevent.h"
#include "midieventbase.h"

using AL::Pos;

//---------------------------------------------------------
//   Event
//---------------------------------------------------------

EventBase::EventBase(EventType t)
      {
      _type     = t;
      Pos::setType(_type == Wave ? AL::FRAMES : AL::TICKS);
      refCount  = 0;
      _selected = false;
      _recorded = false;
      }

EventBase::EventBase(const EventBase& ev)
  : PosLen(ev)
      {
      refCount  = 0;
      _selected = ev._selected;
      _type     = ev._type;
      _recorded = ev._recorded;
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

Event Event::clone() const
      {
      return Event(ev->clone());
      }

Event::Event(EventType t)
	{
	if (t == Wave)
      	ev = new WaveEventBase(t);
	else
      	ev = new MidiEventBase(t);
	++(ev->refCount);
      }

Event::Event(const Event& e)
	{
	ev = e.ev;
      if (ev)
      	++(ev->refCount);
	}

Event::Event(EventBase* eb)
	{
	ev = eb;
      ++(ev->refCount);
      }

Event::~Event()
	{
	if (ev && --(ev->refCount) == 0)
      	delete ev;
	}

bool Event::empty() const
	{
      return ev == 0;
      }

EventType Event::type() const
	{
      return ev ? ev->type() : Note;
      }

void Event::setType(EventType t)
	{
	if (ev && --(ev->refCount) == 0)
      	delete ev;
	if (t == Wave)
      	ev = new WaveEventBase(t);
	else
      	ev = new MidiEventBase(t);
	++(ev->refCount);
      }

Event& Event::operator=(const Event& e)
	{
	if (ev == e.ev)
      	return *this;
	if (ev && --(ev->refCount) == 0)
      	delete ev;
	ev = e.ev;
      if (ev)
      	++(ev->refCount);
	return *this;
      }

bool Event::operator==(const Event& e) const {
      return *ev == *e.ev;
      }


int Event::getRefCount() const    { return ev->getRefCount(); }
bool Event::selected() const      { return ev->_selected;     }
void Event::setSelected(bool val) { ev->_selected = val;      }
bool Event::recorded() const      { return ev->_recorded;     }
void Event::setRecorded(bool val) { ev->_recorded = val;      }
void Event::move(int offset)      { ev->move(offset);         }

void Event::read(QDomNode node)   { ev->read(node); }
void Event::write(Xml& xml, const Pos& o) const { ev->write(xml, o); }
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
void Event::setSndFile(SndFileR& sf) { ev->setSndFile(sf);   }
void Event::read(unsigned offset, float** bpp, int channels, int nn)
      {
      ev->read(offset, bpp, channels, nn);
      }

void Event::setTick(unsigned val)       { ev->setTick(val); }
unsigned Event::tick() const            { return ev->tick(); }
Pos Event::pos() const                  { return *ev;  }
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
QString Event::eventTypeName() const    { return ev->eventTypeName(); }


//---------------------------------------------------------
//   eventTypeName
//---------------------------------------------------------

QString EventBase::eventTypeName() const
      {
      switch(type()) {
            case Note:       return QString("Note");
            case Controller: return QString("Ctrl");
            case Sysex:      return QString("Sysex");
            case PAfter:     return QString("PAfter");
            case CAfter:     return QString("CAfter");
            case Meta:       return QString("Meta");
            default:         return QString("??");
            }
      }

