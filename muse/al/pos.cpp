//=============================================================================
//  AL
//  Audio Utility Library
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

#include "al.h"
#include "pos.h"
#include "xml.h"
#include "tempo.h"
#include "sig.h"

namespace AL {

//---------------------------------------------------------
//   Pos
//---------------------------------------------------------

Pos::Pos()
      {
      _type   = TICKS;
      _tick   = 0;
      _frame  = 0;
      sn      = -1;
      }

Pos::Pos(const Pos& p)
      {
      _type  = p._type;
      sn     = p.sn;
      _tick  = p._tick;
      _frame = p._frame;
      }

Pos::Pos(unsigned t, TType timeType)
      {
 	_type = timeType;
      if (_type == TICKS)
            _tick   = t;
      else
            _frame = t;
      sn = -1;
      }

Pos::Pos(const QString& s)
      {
      int m, b, t;
      sscanf(s.toLatin1().data(), "%04d.%02d.%03d", &m, &b, &t);
      _tick = sigmap.bar2tick(m, b, t);
      _type = TICKS;
      sn    = -1;
      }

Pos::Pos(int measure, int beat, int tick)
      {
      _tick = sigmap.bar2tick(measure, beat, tick);
      _type = TICKS;
      sn    = -1;
      }

Pos::Pos(int min, int sec, int frame, int subframe)
      {
      double time = min * 60.0 + sec;

      double f = frame + subframe/100.0;
      switch (AL::mtcType) {
            case 0:     // 24 frames sec
                  time += f * 1.0/24.0;
                  break;
            case 1:     // 25
                  time += f * 1.0/25.0;
                  break;
            case 2:     // 30 drop frame
                  time += f * 1.0/30.0;
                  break;
            case 3:     // 30 non drop frame
                  time += f * 1.0/30.0;
                  break;
            }
      _type  = FRAMES;
      _frame = lrint(time * AL::sampleRate);
      sn     = -1;
      }

//---------------------------------------------------------
//   setType
//---------------------------------------------------------

void Pos::setType(TType t)
      {
      if (t == _type)
            return;

      if (_type == TICKS) {
            // convert from ticks to frames
            _frame = tempomap.tick2frame(_tick, _frame, &sn);
            }
      else {
            // convert from frames to ticks
            _tick = tempomap.frame2tick(_frame, _tick, &sn);
            }
      _type = t;
      }

//---------------------------------------------------------
//   operator+=
//---------------------------------------------------------

Pos& Pos::operator+=(const Pos& a)
      {
      if (_type == FRAMES)
            _frame += a.frame();
      else
            _tick += a.tick();
      sn = -1;          // invalidate cached values
      return *this;
      }

//---------------------------------------------------------
//   operator-=
//---------------------------------------------------------

Pos& Pos::operator-=(const Pos& a)
      {
      if (_type == FRAMES)
            _frame -= a.frame();
      else
            _tick -= a.tick();
      sn = -1;          // invalidate cached values
      return *this;
      }

//---------------------------------------------------------
//   operator+=
//---------------------------------------------------------

Pos& Pos::operator+=(int a)
      {
      if (_type == FRAMES)
            _frame += a;
      else
            _tick += a;
      sn = -1;          // invalidate cached values
      return *this;
      }

//---------------------------------------------------------
//   operator-=
//---------------------------------------------------------

Pos& Pos::operator-=(int a)
      {
      if (_type == FRAMES)
            _frame -= a;
      else
            _tick -= a;
      sn = -1;          // invalidate cached values
      return *this;
      }

Pos operator+(const Pos& a, int b)
      {
      Pos c(a);
      return c += b;
      }

Pos operator-(const Pos& a, int b)
      {
      Pos c(a);
      return c -= b;
      }

Pos operator+(const Pos& a, const Pos& b)
      {
      Pos c(a);
      return c += b;
      }

Pos operator-(const Pos& a, const Pos& b)
      {
      Pos c(a);
      return c -= b;
      }

bool Pos::operator>=(const Pos& s) const
      {
      if (_type == FRAMES)
            return _frame >= s.frame();
      else
            return _tick >= s.tick();
      }

bool Pos::operator>(const Pos& s) const
      {
      if (_type == FRAMES)
            return _frame > s.frame();
      else
            return _tick > s.tick();
      }

bool Pos::operator<(const Pos& s) const
      {
      if (_type == FRAMES)
            return _frame < s.frame();
      else
            return _tick < s.tick();
      }

bool Pos::operator<=(const Pos& s) const
      {
      if (_type == FRAMES)
            return _frame <= s.frame();
      else
            return _tick <= s.tick();
      }

bool Pos::operator==(const Pos& s) const
      {
      if (_type == FRAMES)
            return _frame == s.frame();
      else
            return _tick == s.tick();
      }

bool Pos::operator!=(const Pos& s) const
      {
      if (_type == FRAMES)
            return _frame != s.frame();
      else
            return _tick != s.tick();
      }

//---------------------------------------------------------
//   tick
//---------------------------------------------------------

unsigned Pos::tick() const
      {
      if (_type == FRAMES)
            _tick = tempomap.frame2tick(_frame, _tick, &sn);
      return _tick;
      }

//---------------------------------------------------------
//   frame
//---------------------------------------------------------

unsigned Pos::frame() const
      {
      if (_type == TICKS)
            _frame = tempomap.tick2frame(_tick, _frame, &sn);
      return _frame;
      }

//---------------------------------------------------------
//   setTick
//---------------------------------------------------------

void Pos::setTick(unsigned pos)
      {
      _tick = pos;
      sn    = -1;
      if (_type == FRAMES)
            _frame = tempomap.tick2frame(pos, &sn);
      }

//---------------------------------------------------------
//   setFrame
//---------------------------------------------------------

void Pos::setFrame(unsigned pos)
      {
      _frame = pos;
      sn     = -1;
      if (_type == TICKS)
            _tick = tempomap.frame2tick(pos, &sn);
      }

//---------------------------------------------------------
//   write
//---------------------------------------------------------

void Pos::write(Xml& xml, const char* name) const
      {
      if (_type == TICKS)
            xml.tagE("%s tick=\"%d\"", name, _tick);
      else
            xml.tagE("%s frame=\"%d\"", name, _frame);
      }

//---------------------------------------------------------
//   read
//---------------------------------------------------------

void Pos::read(QDomNode node)
      {
      sn = -1;

      QDomElement e = node.toElement();
      QString s;
      s = e.attribute("tick");
      if (!s.isEmpty()) {
            _tick = s.toInt();
            _type = TICKS;
            }
      s = e.attribute("frame");
      if (!s.isEmpty()) {
            _frame = s.toInt();
            _type = FRAMES;
            }
      }

//---------------------------------------------------------
//   PosLen
//---------------------------------------------------------

PosLen::PosLen()
      {
      _lenTick  = 0;
      _lenFrame = 0;
      sn        = -1;
      }

PosLen::PosLen(const PosLen& p)
  : Pos(p)
      {
      _lenTick  = p._lenTick;
      _lenFrame = p._lenFrame;
      sn = -1;
      }

//---------------------------------------------------------
//   dump
//---------------------------------------------------------

void PosLen::dump(int n) const
      {
      Pos::dump(n);
      printf("  Len(");
      switch(type()) {
            case FRAMES:
                  printf("samples=%d)\n", _lenFrame);
                  break;
            case TICKS:
                  printf("ticks=%d)\n", _lenTick);
                  break;
            }
      }

void Pos::dump(int /*n*/) const
      {
      printf("Pos(%s, sn=%d, ", type() == FRAMES ? "Frames" : "Ticks", sn);
      switch(type()) {
            case FRAMES:
                  printf("samples=%d)", _frame);
                  break;
            case TICKS:
                  printf("ticks=%d)", _tick);
                  break;
            }
      }

//---------------------------------------------------------
//   write
//---------------------------------------------------------

void PosLen::write(Xml& xml, const char* name) const
      {
      if (type() == TICKS)
            xml.tagE("%s tick=\"%d\" len=\"%d\"", name, tick(), _lenTick);
      else
            xml.tagE("%s sample=\"%d\" len=\"%d\"", name, frame(), _lenFrame);
      }

//---------------------------------------------------------
//   read
//---------------------------------------------------------

void PosLen::read(QDomNode node)
      {
      QDomElement e = node.toElement();
      QString s;
      s = e.attribute("tick");
      if (!s.isEmpty()) {
            setType(TICKS);
            setTick(s.toInt());
            }
      s = e.attribute("sample");
      if (!s.isEmpty()) {
            setType(FRAMES);
            setFrame(s.toInt());
            }
      s = e.attribute("len");
      if (!s.isEmpty()) {
            int n = s.toInt();
            if (type() == TICKS)
                  setLenTick(n);
            else
                  setLenFrame(n);
            }
      }

//---------------------------------------------------------
//   setLenTick
//---------------------------------------------------------

void PosLen::setLenTick(unsigned len)
      {
      _lenTick = len;
      sn       = -1;
      if (type() == FRAMES)
            _lenFrame = tempomap.tick2frame(len, &sn);
      else
            _lenTick = len;
      }

//---------------------------------------------------------
//   setLenFrame
//---------------------------------------------------------

void PosLen::setLenFrame(unsigned len)
      {
      sn      = -1;
      if (type() == TICKS)
            _lenTick = tempomap.frame2tick(len, &sn);
      else
            _lenFrame = len;
      }

//---------------------------------------------------------
//   lenTick
//---------------------------------------------------------

unsigned PosLen::lenTick() const
      {
      if (type() == FRAMES)
            _lenTick = tempomap.frame2tick(_lenFrame, _lenTick, &sn);
      return _lenTick;
      }

//---------------------------------------------------------
//   lenFrame
//---------------------------------------------------------

unsigned PosLen::lenFrame() const
      {
      if (type() == TICKS)
            _lenFrame = tempomap.tick2frame(_lenTick, _lenFrame, &sn);
      return _lenFrame;
      }

//---------------------------------------------------------
//   end
//---------------------------------------------------------

Pos PosLen::end() const
      {
      Pos pos(*this);
      pos.invalidSn();
      switch(type()) {
            case FRAMES:
                  pos.setFrame(pos.frame() + _lenFrame);
                  break;
            case TICKS:
                  pos.setTick(pos.tick() + _lenTick);
                  break;
            }
      return pos;
      }

//---------------------------------------------------------
//   setPos
//---------------------------------------------------------

void PosLen::setPos(const Pos& pos)
      {
      switch(pos.type()) {
            case FRAMES:
                  setFrame(pos.frame());
                  break;
            case TICKS:
                  setTick(pos.tick());
                  break;
            }
      }

//---------------------------------------------------------
//   mbt
//---------------------------------------------------------

void Pos::mbt(int* bar, int* beat, int* tk) const
      {
      sigmap.tickValues(tick(), bar, beat, (unsigned*)tk);
      }

//---------------------------------------------------------
//   msf
//---------------------------------------------------------

void Pos::msf(int* min, int* sec, int* fr, int* subFrame) const
      {
      double time = double(frame()) / double(AL::sampleRate);
      *min  = int(time) / 60;
      *sec  = int(time) % 60;
      double rest = time - (*min * 60 + *sec);
      switch(AL::mtcType) {
            case 0:     // 24 frames sec
                  rest *= 24;
                  break;
            case 1:     // 25
                  rest *= 25;
                  break;
            case 2:     // 30 drop frame
                  rest *= 30;
                  break;
            case 3:     // 30 non drop frame
                  rest *= 30;
                  break;
            }
      *fr = int(rest);
      *subFrame = int((rest- *fr)*100);
      }

//---------------------------------------------------------
//   timesig
//---------------------------------------------------------

TimeSignature Pos::timesig() const
      {
      return sigmap.timesig(tick());
      }

//---------------------------------------------------------
//   snap
//    raster = 1  no snap
//    raster = 0  snap to measure
//    all other raster values snap to raster tick
//---------------------------------------------------------

void Pos::snap(int raster)
      {
      setTick(sigmap.raster(tick(), raster));
      }

void Pos::upSnap(int raster)
      {
      setTick(sigmap.raster2(tick(), raster));
      }

void Pos::downSnap(int raster)
      {
      setTick(sigmap.raster1(tick(), raster));
      }

Pos Pos::snaped(int raster) const
      {
      return Pos(sigmap.raster(tick(), raster));
      }

Pos Pos::upSnaped(int raster) const
      {
      return Pos(sigmap.raster2(tick(), raster));
      }

Pos Pos::downSnaped(int raster) const
      {
      return Pos(sigmap.raster1(tick(), raster));
      }
}

