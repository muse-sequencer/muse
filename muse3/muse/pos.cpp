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

#include <cmath>
#include <stdint.h>

#include "pos.h"
#include "xml.h"
#include "tempo.h"
#include "globals.h"
///#include "sig.h"
#include "al/sig.h"

namespace MusEGlobal {
extern int mtcType;
}

namespace MusECore {


//REMOVE Tim. samplerate. Added.
// //---------------------------------------------------------
// //   MuseFrame
// //---------------------------------------------------------
// 
// MuseFrame::MuseFrame(unsigned frame)
// {
//   _frame = double(MusEGlobal::projectSampleRate) / double(MusEGlobal::sampleRate);
// }
// 
// MuseFrame::operator unsigned() const
// { 
//   return double(MusEGlobal::sampleRate) / double(MusEGlobal::projectSampleRate);
// }
// 
// MuseFrame::operator=(const MuseFrame& other)
// {
//   _frame = other._frame;
// }
  
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
      _type = p._type;
      sn    = p.sn;
      _tick = p._tick;
      _frame = p._frame;
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
      }

Pos::Pos(const QString& s)
      {
      int m, b, t;
      sscanf(s.toLatin1(), "%04d.%02d.%03d", &m, &b, &t);
      _tick = AL::sigmap.bar2tick(m, b, t);
      _type = TICKS;
      sn    = -1;
      }

Pos::Pos(int measure, int beat, int tick)
      {
      _tick = AL::sigmap.bar2tick(measure, beat, tick);
      _type = TICKS;
      sn    = -1;
      }

Pos::Pos(int min, int sec, int frame, int subframe)
      {
      uint64_t time = (uint64_t)MusEGlobal::sampleRate * (min * 60UL + sec);
      const uint64_t f = (uint64_t)MusEGlobal::sampleRate * (frame * 100UL + subframe);
      switch(MusEGlobal::mtcType) {
            case 0:     // 24 frames sec
                  time += f / 2400UL;
                  break;
            case 1:     // 25
                  time += f / 2500UL;
                  break;
            case 2:     // 30 drop frame
                  time += f / 3000UL;
                  break;
            case 3:     // 30 non drop frame
                  time += f / 3000UL;
                  break;
            }
      _type  = FRAMES;
      _frame = time;
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
            _frame = MusEGlobal::tempomap.tick2frame(_tick, _frame, &sn);
            }
      else {
            // convert from frames to ticks
            _tick = MusEGlobal::tempomap.frame2tick(_frame, _tick, &sn);
            }
      _type = t;
      }

// //REMOVE Tim. samplerate. Added.
// //---------------------------------------------------------
// //   convertFrame4SampleRate
// //---------------------------------------------------------
// 
// unsigned Pos::convertFrame4SampleRate(unsigned frame) const
// {
//   return double(frame) * double(MusEGlobal::sampleRate) / double(MusEGlobal::projectSampleRate);
// }

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

// REMOVE Tim. samplerate. Changed.
//---------------------------------------------------------
//   tick
//---------------------------------------------------------

unsigned Pos::tick() const
      {
      if (_type == FRAMES)
            _tick = MusEGlobal::tempomap.frame2tick(_frame, _tick, &sn);
      return _tick;
      }

//---------------------------------------------------------
//   frame
//---------------------------------------------------------

unsigned Pos::frame() const
      {
      if (_type == TICKS)
            _frame = MusEGlobal::tempomap.tick2frame(_tick, _frame, &sn);
      return _frame;
      }

//---------------------------------------------------------
//   tick
//---------------------------------------------------------

// unsigned Pos::tick() const
//       {
//       if (_type == FRAMES)
//             _tick = MusEGlobal::tempomap.frame2tick(MusEGlobal::convertFrame4ProjectSampleRate(_frame), _tick, &sn);
//       return _tick;
//       }
// 
// //---------------------------------------------------------
// //   frame
// //---------------------------------------------------------
// 
// unsigned Pos::frame() const
//       {
//       if (_type == TICKS)
//             _frame = MusEGlobal::tempomap.tick2frame(_tick, _frame, &sn);
//       return MusEGlobal::convertFrame4ProjectSampleRate(_frame);
//       }

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
  return _tick;
}
      
unsigned Pos::posValue(TType time_type) const
{
  switch(time_type)
  {
    case FRAMES:
      if (_type == TICKS)
            _frame = MusEGlobal::tempomap.tick2frame(_tick, _frame, &sn);
      return _frame;
    case TICKS:
      if (_type == FRAMES)
            _tick = MusEGlobal::tempomap.frame2tick(_frame, _tick, &sn);
      return _tick;
  }
  return _tick;
}
      
//---------------------------------------------------------
//   setTick
//---------------------------------------------------------

void Pos::setTick(unsigned pos)
      {
      _tick = pos;
      sn    = -1;
      if (_type == FRAMES)
            _frame = MusEGlobal::tempomap.tick2frame(pos, &sn);
      }

//---------------------------------------------------------
//   setFrame
//---------------------------------------------------------

void Pos::setFrame(unsigned pos)
      {
      _frame = pos;
      sn     = -1;
      if (_type == TICKS)
            _tick = MusEGlobal::tempomap.frame2tick(pos, &sn);
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
          if (_type == TICKS)
                _tick = MusEGlobal::tempomap.frame2tick(_frame, &sn);
          break;
    case TICKS:
          _tick = val;
          if (_type == FRAMES)
                _frame = MusEGlobal::tempomap.tick2frame(_tick, &sn);
          break;
  }
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
//REMOVE Tim. samplerate. Changed.
//                         else if (tag == "frame") {
//                               _frame = xml.s2().toInt();
//                               _type = FRAMES;
//                               }
//                         else if (tag == "sample") {   // obsolete
//                               _frame = xml.s2().toInt();
//                               _type = FRAMES;
//                               }
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
//REMOVE Tim. samplerate. Changed.
//                               setFrame(xml.s2().toInt());
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
//REMOVE Tim. samplerate. Changed.
//                                           setLenFrame(n);
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
        return _lenTick;
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
        return _lenTick;
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
      AL::sigmap.tickValues(tick(), bar, beat, (unsigned*)tk);
      }

//---------------------------------------------------------
//   msf
//---------------------------------------------------------

void Pos::msf(int* min, int* sec, int* fr, int* subFrame) const
      {
      double time = double(frame()) / double(MusEGlobal::sampleRate);
      *min  = int(time) / 60;
      *sec  = int(time) % 60;
      double rest = time - (*min * 60 + *sec);
      switch(MusEGlobal::mtcType) {
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
