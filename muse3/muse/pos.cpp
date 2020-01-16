//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: pos.cpp,v 1.11.2.1 2006/09/19 19:07:08 spamatica Exp $
//
//  (C) Copyright 2000 Werner Schweer (ws@seh.de)
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

#include "muse_math.h"
#include <stdint.h>

#include "pos.h"
#include "xml.h"
#include "tempo.h"
#include "globals.h"
#include "sync.h"
#include "sig.h"

namespace MusEGlobal {
extern int mtcType;
}

namespace MusECore {


//---------------------------------------------------------
//   Pos
//---------------------------------------------------------

Pos::Pos()
      {
      _type   = TICKS;
      _tick   = 0;
      _frame  = 0;
      _lock   = false;
      sn      = -1;
      }

Pos::Pos(const Pos& p)
      {
      _type = p._type;
      sn    = p.sn;
      _tick = p._tick;
      _frame = p._frame;
      _lock  = p._lock;
      }

Pos::Pos(unsigned t, bool ticks)
      {
      if (ticks) {
            _type   = TICKS;
            _tick   = t;
            }
      else {
            _type  = FRAMES;
            _frame = t;
            }
      sn = -1;
      _lock = false;
      }

Pos::Pos(const QString& s)
      {
      int m, b, t;
      sscanf(s.toLatin1(), "%04d.%02d.%03d", &m, &b, &t);
      _tick = MusEGlobal::sigmap.bar2tick(m, b, t);
      _type = TICKS;
      sn    = -1;
      _lock = false;
      }

Pos::Pos(int measure, int beat, int tick)
      {
      _tick = MusEGlobal::sigmap.bar2tick(measure, beat, tick);
      _type = TICKS;
      sn    = -1;
      _lock = false;
      }

Pos::Pos(int min, int sec, int frame, int subframe, bool ticks, LargeIntRoundMode round_mode)
      {
      _lock = false;

      const int64_t sr = (int64_t)MusEGlobal::sampleRate;
      int64_t time = sr * (min * 60L + sec);
      const int64_t f = sr * (frame * 100L + subframe);
      int64_t divisor = 2400L;
      switch(MusEGlobal::mtcType) {
            case 0:     // 24 frames sec
                  divisor = 2400L;
                  break;
            case 1:     // 25
                  divisor = 2500L;
                  break;
            case 2:     // 30 drop frame
                  divisor = 3000L;
                  break;
            case 3:     // 30 non drop frame
                  divisor = 3000L;
                  break;
            }
            
      time += f / divisor;

      if(time < 0)
        time = 0;
      //else if(time > INT_MAX)
      //  time = INT_MAX;

      if(round_mode == LargeIntRoundUp && (f % divisor) != 0)
        ++time;
      else if(round_mode == LargeIntRoundNearest && (f % divisor) >= divisor / 2)
        ++time;
      
      _frame = time;
      if (ticks) {
            _type   = TICKS;
            // Convert from frames to ticks
            _tick = MusEGlobal::tempomap.frame2tick(_frame, &sn, round_mode);
            }
      else {
            _type  = FRAMES;
            sn = -1;
      }

      }

Pos::Pos(int hour, int min, int sec, int msec, int usec, bool ticks, LargeIntRoundMode round_mode)
{
      _lock = false;

      const int64_t sr = (int64_t)MusEGlobal::sampleRate;
      const int64_t uf = sr * (1000L * msec + usec);
      const int64_t mf =  uf / 1000000L;
      
      int64_t f = sr * (hour * 3600L + min * 60L + sec) + mf;

      if(f < 0)
        f = 0;
      //else if(f > INT_MAX)
      //  f = INT_MAX;

      // REMOVE Tim. clip. Added. Diagnostics.
      fprintf(stderr, "Pos: uf:%ld uf mod 1000000:%ld round_mode:%d\n", uf, uf % 1000000L, round_mode);

      if(round_mode == LargeIntRoundUp && (uf % 1000000L) != 0)
        ++f;
      else if(round_mode == LargeIntRoundNearest && (uf % 1000000L) >= 500000L)
        ++f;
      
      _frame = f;
      if (ticks) {
            _type   = TICKS;
            // Convert from frames to ticks
            _tick = MusEGlobal::tempomap.frame2tick(_frame, &sn, round_mode);
            }
      else {
            _type  = FRAMES;
            sn = -1;
      }
}

//---------------------------------------------------------
//   setType
//---------------------------------------------------------

void Pos::setType(TType t)
      {
      if (t == _type)
            return;

      if(!_lock) {
        if (_type == TICKS) {
              // convert from ticks to frames
              _frame = MusEGlobal::tempomap.tick2frame(_tick, _frame, &sn);
              }
        else {
              // convert from frames to ticks
              _tick = MusEGlobal::tempomap.frame2tick(_frame, _tick, &sn);
              }
      }
      _type = t;
      }

//---------------------------------------------------------
//   setLock
//---------------------------------------------------------

void Pos::setLock(bool v)
{
  _lock = v;
  sn = -1;
}

//---------------------------------------------------------
//   snValid
//---------------------------------------------------------

bool Pos::snValid() const
{
  return sn == MusEGlobal::tempomap.tempoSN();
}

//---------------------------------------------------------
//   operator=
//---------------------------------------------------------

Pos& Pos::operator=(const Pos& p)
      {
      _type = p._type;
      sn    = p.sn;
      _tick = p._tick;
      _frame = p._frame;
      _lock = p._lock;
      return *this;
      }

//---------------------------------------------------------
//   operator+=
//---------------------------------------------------------

Pos& Pos::operator+=(Pos a)
      {
      switch(_type) {
            case FRAMES:
                  _frame += a.frame();
                  break;
            case TICKS:
                  _tick += a.tick();
                  break;
            }
      sn = -1;          // invalidate cached values
      return *this;
      }

//---------------------------------------------------------
//   operator+=
//---------------------------------------------------------

Pos& Pos::operator+=(int a)
      {
      switch(_type) {
            case FRAMES:
                  _frame += a;
                  break;
            case TICKS:
                  _tick += a;
                  break;
            }
      sn = -1;          // invalidate cached values
      return *this;
      }

//---------------------------------------------------------
//   operator-=
//---------------------------------------------------------

Pos& Pos::operator-=(Pos a)
      {
      switch(_type) {
            case FRAMES:
                  _frame -= a.frame();
                  break;
            case TICKS:
                  _tick -= a.tick();
                  break;
            }
      sn = -1;          // invalidate cached values
      return *this;
      }

//---------------------------------------------------------
//   operator-=
//---------------------------------------------------------

Pos& Pos::operator-=(int a)
      {
      switch(_type) {
            case FRAMES:
                  _frame -= a;
                  break;
            case TICKS:
                  _tick -= a;
                  break;
            }
      sn = -1;          // invalidate cached values
      return *this;
      }

Pos& Pos::operator++()
{
      switch(_type) {
            case FRAMES:
                  ++_frame;
                  break;
            case TICKS:
                  ++_tick;
                  break;
            }
      sn = -1;          // invalidate cached values
      return *this;
}

Pos& Pos::operator--()
{
      switch(_type) {
            case FRAMES:
                  --_frame;
                  break;
            case TICKS:
                  --_tick;
                  break;
            }
      sn = -1;          // invalidate cached values
      return *this;
}

Pos operator+(Pos a, int b)
      {
      Pos c = a;
      c.setType(a.type());
      return c += b;
      }

Pos operator+(Pos a, Pos b)
      {
      Pos c = a;
      return c += b;
      }

Pos operator-(Pos a, int b)
      {
      Pos c = a;
      c.setType(a.type());
      return c -= b;
      }

Pos operator-(Pos a, Pos b)
      {
      Pos c = a;
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

unsigned Pos::tick(LargeIntRoundMode round_mode) const
      {
      if(!_lock) {
        if (_type == FRAMES)
              _tick = MusEGlobal::tempomap.frame2tick(_frame, _tick, &sn, round_mode);
      }
      return _tick;
      }

//---------------------------------------------------------
//   frame
//---------------------------------------------------------

unsigned Pos::frame(LargeIntRoundMode round_mode) const
      {
      if(!_lock) {
        if (_type == TICKS)
              _frame = MusEGlobal::tempomap.tick2frame(_tick, _frame, &sn, round_mode);
      }
      return _frame;
      }

//---------------------------------------------------------
//   posValue
//---------------------------------------------------------

unsigned Pos::posValue() const
{
  switch(type())
  {
    case FRAMES:
      return _frame;
    case TICKS:
      return _tick;
  }
  return tick();
}
      
unsigned Pos::posValue(TType time_type) const
{
  switch(time_type)
  {
    case FRAMES:
      if(!_lock) {
        if (_type == TICKS)
              _frame = MusEGlobal::tempomap.tick2frame(_tick, _frame, &sn);
      }
      return _frame;
    case TICKS:
      if(!_lock) {
        if (_type == FRAMES)
              _tick = MusEGlobal::tempomap.frame2tick(_frame, _tick, &sn);
      }
      return _tick;
  }
  return tick();
}
      
//---------------------------------------------------------
//   setTick
//---------------------------------------------------------

void Pos::setTick(unsigned pos, LargeIntRoundMode round_mode)
      {
      _tick = pos;
      sn    = -1;
      if(!_lock) {
        if (_type == FRAMES)
              _frame = MusEGlobal::tempomap.tick2frame(pos, &sn, round_mode);
      }
      }

//---------------------------------------------------------
//   setFrame
//---------------------------------------------------------

void Pos::setFrame(unsigned pos, LargeIntRoundMode round_mode)
      {
      _frame = pos;
      sn     = -1;
      if(!_lock) {
        if (_type == TICKS)
              _tick = MusEGlobal::tempomap.frame2tick(pos, &sn, round_mode);
      }
      }

//---------------------------------------------------------
//   setPosValue
//---------------------------------------------------------

void Pos::setPosValue(unsigned val)
{
  sn = -1;
  switch(type()) {
    case FRAMES:
          _frame = val;
          break;
    case TICKS:
          _tick = val;
          break;
  }
}
      
void Pos::setPosValue(unsigned val, TType time_type)
{
  sn = -1;
  switch(time_type) {
    case FRAMES:
          _frame = val;
          if(!_lock) {
            if (_type == TICKS)
                  _tick = MusEGlobal::tempomap.frame2tick(_frame, &sn);
          }
          break;
    case TICKS:
          _tick = val;
          if(!_lock) {
            if (_type == FRAMES)
                  _frame = MusEGlobal::tempomap.tick2frame(_tick, &sn);
          }
          break;
  }
}

void Pos::setPos(const Pos& s)
{
  sn = -1;
  switch(s.type()) {
    case FRAMES:
          _frame = s.posValue();
          if(_lock) {
            _tick = s.tick();
          }
          else {
            if (_type == TICKS)
                  _tick = MusEGlobal::tempomap.frame2tick(_frame, &sn);
          }
          break;
    case TICKS:
          _tick = s.posValue();
          if(_lock) {
            _frame = s.frame();
          }
          else {
            if (_type == FRAMES)
                  _frame = MusEGlobal::tempomap.tick2frame(_tick, &sn);
          }
          break;
  }
}

//---------------------------------------------------------
//   setTickAndFrame
//---------------------------------------------------------

void Pos::setTickAndFrame(unsigned tick, unsigned frame)
{
  sn = MusEGlobal::tempomap.tempoSN();
  _tick = tick;
  _frame = frame;
}

void Pos::setTickAndFrame(const Pos p)
{
  sn = MusEGlobal::tempomap.tempoSN();
  _tick = p.tick();
  _frame = p.frame();
}

//---------------------------------------------------------
//   convert (static)
//---------------------------------------------------------

unsigned Pos::convert(unsigned val, TType from_type, TType to_type)
{
  switch(from_type) {
    case FRAMES:
      switch(to_type) 
      {
        case FRAMES: return val; 
        case TICKS:  return MusEGlobal::tempomap.frame2tick(val);
      }
    break;
          
    case TICKS:
      switch(to_type) 
      {
        case FRAMES: return MusEGlobal::tempomap.tick2frame(val);
        case TICKS:  return val; 
      }
    break;
  }
  return val;
}

//---------------------------------------------------------
//   write
//---------------------------------------------------------

void Pos::write(int level, Xml& xml, const char* name) const
      {
      xml.nput(level++, "<%s ", name);

      switch(_type) {
            case TICKS:
                  xml.nput("tick=\"%d\"", _tick);
                  break;
            case FRAMES:
                  xml.nput("frame=\"%d\"", _frame);
                  break;
            }
      xml.put(" />", name);
      }

//---------------------------------------------------------
//   read
//---------------------------------------------------------

void Pos::read(Xml& xml, const char* name)
      {
      sn = -1;
      for (;;) {
            Xml::Token token = xml.parse();
            const QString& tag = xml.s1();
            switch (token) {
                  case Xml::Error:
                  case Xml::End:
                        return;

                  case Xml::TagStart:
                        xml.unknown(name);
                        break;

                  case Xml::Attribut:
                        if (tag == "tick") {
                              _tick = xml.s2().toInt();
                              _type = TICKS;
                              }
                        else if (tag == "frame" || tag == "sample") {
                              // For now, the conversion only has a TEMPORARY effect during song loading.
                              // See comments in Song::read at the "samplerate" tag.
                              _frame = MusEGlobal::convertFrame4ProjectSampleRate(xml.s2().toInt());
                              _type = FRAMES;
                              }
                        else
                              xml.unknown(name);
                        break;

                  case Xml::TagEnd:
                        if (tag == name)
                              return;
                  default:
                        break;
                  }
            }
      }

//---------------------------------------------------------
//   PosLen
//---------------------------------------------------------

PosLen::PosLen(bool ticks, unsigned pos, unsigned len)
  : Pos(pos, ticks)
      {
      if (ticks) {
            _lenTick  = len;
            _lenFrame = 0;
            }
      else {
            _lenTick  = 0;
            _lenFrame = len;
            }
      sn        = -1;
      }

PosLen::PosLen(const PosLen& p)
  : Pos(p)
      {
      _lenTick  = p._lenTick;
      _lenFrame = p._lenFrame;
      sn = -1;
      }

PosLen& PosLen::operator=(const PosLen& p)
      {
      Pos::operator=(p);
      _lenTick  = p._lenTick;
      _lenFrame = p._lenFrame;
      sn = -1;
      return *this;
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

void PosLen::write(int level, Xml& xml, const char* name) const
      {
      xml.nput(level++, "<%s ", name);

      switch(type()) {
            case TICKS:
                  xml.nput("tick=\"%d\" len=\"%d\"", tick(), _lenTick);
                  break;
            case FRAMES:
                  xml.nput("sample=\"%d\" len=\"%d\"", frame(), _lenFrame);
                  break;
            }
      xml.put(" />", name);
      }

//---------------------------------------------------------
//   read
//---------------------------------------------------------

void PosLen::read(Xml& xml, const char* name)
      {
      sn = -1;
      for (;;) {
            Xml::Token token = xml.parse();
            const QString& tag = xml.s1();
            switch (token) {
                  case Xml::Error:
                  case Xml::End:
                        return;

                  case Xml::TagStart:
                        xml.unknown(name);
                        break;

                  case Xml::Attribut:
                        if (tag == "tick") {
                              setType(TICKS);
                              setTick(xml.s2().toInt());
                              }
                        else if (tag == "sample") {
                              setType(FRAMES);
                              // For now, the conversion only has a TEMPORARY effect during song loading.
                              // See comments in Song::read at the "samplerate" tag.
                              setFrame(MusEGlobal::convertFrame4ProjectSampleRate(xml.s2().toInt()));
                              }
                        else if (tag == "len") {
                              int n = xml.s2().toInt();
                              switch(type()) {
                                    case TICKS:
                                          setLenTick(n);
                                          break;
                                    case FRAMES:
                                          // For now, the conversion only has a TEMPORARY effect during song loading.
                                          // See comments in Song::read at the "samplerate" tag.
                                          setLenFrame(MusEGlobal::convertFrame4ProjectSampleRate(n));
                                          break;
                                    }
                              }
                        else
                              xml.unknown(name);
                        break;

                  case Xml::TagEnd:
                        if (tag == name)
                              return;
                  default:
                        break;
                  }
            }
      }

//---------------------------------------------------------
//   setLenTick
//---------------------------------------------------------

void PosLen::setLenTick(unsigned len)
      {
      _lenTick = len;
      sn       = -1;
      _lenFrame = MusEGlobal::tempomap.deltaTick2frame(tick(), tick() + len, &sn);
      }

//---------------------------------------------------------
//   setLenFrame
//---------------------------------------------------------

void PosLen::setLenFrame(unsigned len)
      {
      _lenFrame = len;
      sn      = -1;
      _lenTick = MusEGlobal::tempomap.deltaFrame2tick(frame(), frame() + len, &sn);
      }

//---------------------------------------------------------
//   setLenValue
//---------------------------------------------------------

void PosLen::setLenValue(unsigned val)
{
  sn      = -1;
  switch(type())
  {
    case FRAMES:
        _lenFrame = val;
      break;
    case TICKS:
        _lenTick = val;
      break;
  }
}

void PosLen::setLenValue(unsigned val, TType time_type)
{
  sn      = -1;
  switch(time_type)
  {
    case FRAMES:
        _lenFrame = val;
        if (type() == TICKS)
          _lenTick = MusEGlobal::tempomap.deltaFrame2tick(frame(), frame() + _lenFrame, &sn);
      break;
    case TICKS:
        _lenTick = val;
        if (type() == FRAMES)
          _lenFrame = MusEGlobal::tempomap.deltaTick2frame(tick(), tick() + _lenTick, &sn);
      break;
  }
}

void PosLen::setLen(const PosLen& s)
{
  sn      = -1;
  switch(s.type())
  {
    case FRAMES:
        _lenFrame = s.lenValue();
        if (type() == TICKS)
          _lenTick = MusEGlobal::tempomap.deltaFrame2tick(frame(), frame() + _lenFrame, &sn);
      break;
    case TICKS:
        _lenTick = s.lenValue();
        if (type() == FRAMES)
          _lenFrame = MusEGlobal::tempomap.deltaTick2frame(tick(), tick() + _lenTick, &sn);
      break;
  }
}

//---------------------------------------------------------
//   convertLen (static)
//---------------------------------------------------------

unsigned PosLen::convertLen(unsigned val, unsigned len, TType from_type, TType to_type)
{
  switch(from_type) 
  {
    case FRAMES:
      switch(to_type) 
      {
        case FRAMES: return val; 
        case TICKS:  return MusEGlobal::tempomap.deltaFrame2tick(val, val + len);
      }
    break;
          
    case TICKS:
      switch(to_type) 
      {
        case FRAMES: return MusEGlobal::tempomap.deltaTick2frame(val, val + len);
        case TICKS:  return val; 
      }
    break;
  }
  return len;
}

//---------------------------------------------------------
//   lenTick
//---------------------------------------------------------

unsigned PosLen::lenTick() const
      {
      if (type() == FRAMES)
            _lenTick = MusEGlobal::tempomap.deltaFrame2tick(frame(), frame() + _lenFrame, &sn);
      return _lenTick;
      }

//---------------------------------------------------------
//   lenFrame
//---------------------------------------------------------

unsigned PosLen::lenFrame() const
      {
      if (type() == TICKS)
            _lenFrame = MusEGlobal::tempomap.deltaTick2frame(tick(), tick() + _lenTick, &sn); 
      return _lenFrame;
      }

//---------------------------------------------------------
//   lenValue
//---------------------------------------------------------

unsigned PosLen::lenValue() const
      {
        switch(type())
        {
          case FRAMES:
              return _lenFrame;
          case TICKS:
              return _lenTick;
        }
        return lenTick();
      }

unsigned PosLen::lenValue(TType time_type) const
      {
        switch(time_type)
        {
          case FRAMES:
                if (type() == TICKS)
                      _lenFrame = MusEGlobal::tempomap.deltaTick2frame(tick(), tick() + _lenTick, &sn); 
                return _lenFrame;
          case TICKS:
                if (type() == FRAMES)
                      _lenTick = MusEGlobal::tempomap.deltaFrame2tick(frame(), frame() + _lenFrame, &sn);
                return _lenTick;
        }
        return lenTick();
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

void PosLen::setEnd(const Pos& pos)
{
      switch(pos.type()) {
            case FRAMES:
                  if(pos.frame() > frame())
                    setLenFrame(pos.frame() - frame());
                  else
                    setLenFrame(0);
                  break;
            case TICKS:
                  if(pos.tick() > tick())
                    setLenTick(pos.tick() - tick());
                  else
                    setLenTick(0);
                  break;
            }
}

//---------------------------------------------------------
//   endValue
//---------------------------------------------------------

unsigned PosLen::endValue() const
      {
      switch(type()) {
            case FRAMES:
                  return frame() + lenFrame();
                  break;
            case TICKS:
                  return tick() + lenTick();
                  break;
            }
      return 0;
      }

unsigned PosLen::endValue(TType time_type) const
{
      switch(time_type) {
            case FRAMES:
                  return frame() + lenFrame();
                  break;
            case TICKS:
                  return tick() + lenTick();
                  break;
            }
      return 0;
}

void PosLen::setEndValue(unsigned val)
{
      switch(type()) {
            case FRAMES:
                  if(val > frame())
                    setLenFrame(val - frame());
                  else
                    setLenFrame(0);
                  break;
            case TICKS:
                  if(val > tick())
                    setLenTick(val - tick());
                  else
                    setLenTick(0);
                  break;
            }
}

void PosLen::setEndValue(unsigned val, TType time_type)
{
      switch(time_type) {
            case FRAMES:
                  if(val > frame())
                    setLenFrame(val - frame());
                  else
                    setLenFrame(0);
                  break;
            case TICKS:
                  if(val > tick())
                    setLenTick(val - tick());
                  else
                    setLenTick(0);
                  break;
            }
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
      MusEGlobal::sigmap.tickValues(tick(), bar, beat, (unsigned*)tk);
      }

//---------------------------------------------------------
//   msf
//---------------------------------------------------------

void Pos::msf(int* hour, int* min, int* sec, int* fr, int* subFrame, LargeIntRoundMode round_mode) const
      {
      const uint64_t sr = (uint64_t)MusEGlobal::sampleRate;
      const unsigned int f = frame();
      const unsigned int time = f / sr;
      if(hour)
      {
        *hour = time / 3600;
        if(min)
          *min  = (time / 60) % 60;
      }
      else if(min)
        *min  = time / 60;

      if(sec)
        *sec  = time % 60;

      unsigned int rest_fact = 24;
      switch(MusEGlobal::mtcType) {
            case 0:     // 24 frames sec
                  rest_fact = 24;
                  break;
            case 1:     // 25
                  rest_fact = 25;
                  break;
            case 2:     // 30 drop frame
                  rest_fact = 30;
                  break;
            case 3:     // 30 non drop frame
                  rest_fact = 30;
                  break;
            }

      const uint64_t ss = (uint64_t)(f % sr) * 100 * rest_fact;
      uint64_t sf = ss / sr;
      
      if(round_mode == LargeIntRoundUp && (ss % sr) != 0)
        ++sf;
      else if(round_mode == LargeIntRoundNearest && (ss % sr) >= sr / 2)
        ++sf;
      
      if(subFrame)
        *subFrame = sf % 100;
      if(fr)
        *fr = sf / 100;
      }

//---------------------------------------------------------
//   msmu
//---------------------------------------------------------

void Pos::msmu(int* hour, int* min, int* sec, int* msec, int* usec, LargeIntRoundMode round_mode) const
      {
      const uint64_t sr = (uint64_t)MusEGlobal::sampleRate;
      const unsigned int f = frame();
      const unsigned int time = f / sr;
      if(hour)
      {
        *hour = time / 3600;
        if(min)
          *min  = (time / 60) % 60;
      }
      else if(min)
        *min  = time / 60;

      if(sec)
        *sec  = time % 60;

      const uint64_t uf = (uint64_t)(f % sr) * 1000000;
      uint64_t us = uf / sr;

      if(round_mode == LargeIntRoundUp && (uf % sr) != 0)
        ++us;
      else if(round_mode == LargeIntRoundNearest && (uf % sr) >= sr / 2)
        ++us;

      if(usec)
        *usec = us % 1000;
      if(msec)
        *msec = us / 1000;
      }

//---------------------------------------------------------
//   isValid
//---------------------------------------------------------

bool Pos::isValid(int,int,int)
      {
      return true;
      }

//---------------------------------------------------------
//   isValid
//---------------------------------------------------------

bool Pos::isValid(int,int,int,int)
      {
      return true;
      }

} // namespace MusECore
