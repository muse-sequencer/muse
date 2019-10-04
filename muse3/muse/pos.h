//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: pos.h,v 1.8 2004/07/14 15:27:26 wschweer Exp $
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

#ifndef __POS_H__
#define __POS_H__

#include "large_int.h"

class QString;

namespace MusECore {

class Xml;

//---------------------------------------------------------
//   Pos
//    depending on type _tick or _frame is a cached
//    value. When the tempomap changes, all cached values
//    are invalid. Sn is used to check for tempomap
//    changes.
//---------------------------------------------------------

class Pos {
   public:
      enum TType { TICKS, FRAMES };

   private:
      TType _type;
      mutable int sn;
      mutable unsigned _tick;
      mutable unsigned _frame;
      // This flag indicates the frame and tick are separate
      //  and are not influenced by the tempomap at all.
      // Frame and tick can be set independently without being
      //  altered by the tempomap.
      // When this flag is set, the _type is effectively ignored
      //  for several functions and the serial number is set to -1.
      // It allows storing independent frame and tick values
      //  conveniently housed in a Pos.
      // It is mainly for use during external midi sync,
      //  where frame and tick are completely independent.
      // When asked for frame or tick, Pos will simply return
      //  the stored frame or tick verbosely, without conversion
      //  using the tempomap.
      bool _lock;

   public:
      Pos();
      Pos(const Pos&);
      Pos(int measure, int beat, int tick);

      //------------------------------------------------------------------------------------------
      // To make it easier to work with automatic section increment/decrement over/under-flow,
      //  these constructors accept integers which can be negative. If the end result is less than zero
      //  or greater than UINT_MAX, the result is adjusted accordingly.
      //------------------------------------------------------------------------------------------

      // Construct a position from a minute-second-frame-subframe value. If round_mode is up or nearest, any
      //  fractional frame in the result is either rounded up or to the nearest frame.
//       // round_mode should normally be left as up. If ticks is true, the resulting frame is further
      // round_mode should normally be left as down. If ticks is true, the resulting frame is further
      //  rounded up or to the nearest tick, and the type is set to TICKS.
//       Pos(int min, int sec, int frame, int subframe, bool ticks = false, LargeIntRoundMode round_mode = LargeIntRoundUp);
      Pos(int min, int sec, int frame, int subframe, bool ticks = false, LargeIntRoundMode round_mode = LargeIntRoundDown);
      // Construct a position from a time value. If round_mode is up or nearest, any fractional frame in the result
      //  is either rounded up or to the nearest frame. round_mode should normally be left as down. If ticks
      //  is true, the resulting frame is further rounded up or to the nearest tick, and the type
      //  is set to TICKS.
      Pos(int hour, int min, int sec, int msec, int usec, bool ticks = false, LargeIntRoundMode round_mode = LargeIntRoundDown);
      
      Pos(unsigned, bool ticks=true);
      Pos(const QString&);
      void dump(int n = 0) const;
      void mbt(int* bar, int* beat, int* tk) const;
//       // msf resolution is normally less than ticks or frames. Round down by default.
//       void msf(int* min, int* sec, int* fr, int* subFrame, LargeIntRoundMode round_mode = LargeIntRoundDown) const;
      // If hour is given, hours and minutes will be split accordingly, with min being a maximum of 59.
      // Otherwise if hour is null, min will be unlimited.
      void msf(int* hour, int* min, int* sec, int* fr, int* subFrame, LargeIntRoundMode round_mode = LargeIntRoundUp) const;
// REMOVE Tim. clip. Added.
      // msmu resolution is normally greater than ticks or frames. Round up by default.
      // If hour is given, hours and minutes will be split accordingly, with min being a maximum of 59.
      // Otherwise if hour is null, min will be unlimited.
      void msmu(int* hour, int* min, int* sec, int* msec, int* usec, LargeIntRoundMode round_mode = LargeIntRoundUp) const;

      void invalidSn()  { sn = -1; }
      // Returns whether the serial number is the same as the tempomap serial number.
      bool snValid() const;

      TType  type() const     { return _type; }
      void   setType(TType t);

      bool lock() const { return _lock; }
      void setLock(bool v);

      Pos& operator=(const Pos&);
      
      Pos& operator+=(Pos a);
      Pos& operator+=(int a);
      Pos& operator-=(Pos a);
      Pos& operator-=(int a);
      Pos& operator++();
      Pos& operator--();

      bool operator>=(const Pos& s) const;
      bool operator>(const Pos& s) const;
      bool operator<(const Pos& s) const;
      bool operator<=(const Pos& s) const;
      bool operator==(const Pos& s) const;
      bool operator!=(const Pos& s) const;

      friend Pos operator+(Pos a, Pos b);
      friend Pos operator+(Pos a, int b);
      friend Pos operator-(Pos a, Pos b);
      friend Pos operator-(Pos a, int b);

      unsigned tick(LargeIntRoundMode round_mode = LargeIntRoundDown) const;
      unsigned frame(LargeIntRoundMode round_mode = LargeIntRoundUp) const;
      unsigned posValue() const;
      unsigned posValue(TType time_type) const;
      void setTick(unsigned, LargeIntRoundMode round_mode = LargeIntRoundUp);
      void setFrame(unsigned, LargeIntRoundMode round_mode = LargeIntRoundDown);
      void setPosValue(unsigned val);
      void setPosValue(unsigned val, TType time_type);
      // This is not the same as assigning or =.
      // The type is kept and the value converted if required.
      void setPos(const Pos&);
      // This is a convenience function that sets both tick and frame at once.
      // It allows setting independent tick and frame at once.
      // They will both be valid until the next tempomap serial number change
      //  ie. whenever tempo changes. Certain situations benefit from this,
      //  for example passing around audio transport tick and frame which may
      //  need to be separate especially during external sync.
      // The important thing is that calling tick() or frame() will NOT cause it
      //  to try and recalculate using the tempomap, until the next tempo change.
      void setTickAndFrame(unsigned tick, unsigned frame);
      void setTickAndFrame(const Pos);
      static unsigned convert(unsigned val, TType from_type, TType to_type);

      void write(int level, Xml&, const char*) const;
      void read(Xml& xml, const char*);
      bool isValid() const { return true; }
      static bool isValid(int m, int b, int t);
      static bool isValid(int, int, int, int);
      };

//---------------------------------------------------------
//   PosLen
//---------------------------------------------------------

class PosLen : public Pos {
      mutable unsigned _lenTick;
      mutable unsigned _lenFrame;
      mutable int sn;

   public:
      PosLen(bool ticks = true, unsigned pos = 0, unsigned len = 0);
      PosLen(const PosLen&);
      void dump(int n = 0) const;

      PosLen& operator=(const PosLen&);
      
      void write(int level, Xml&, const char*) const;
      void read(Xml& xml, const char*);
      void setLenTick(unsigned);
      void setLenFrame(unsigned);
      void setLenValue(unsigned val);
      void setLenValue(unsigned val, TType time_type);     
      // This is not the same as assigning or =.
      // The type is kept and the value converted if required.
      void setLen(const PosLen&);
      unsigned lenTick() const;
      unsigned lenFrame() const;
      unsigned lenValue() const;
      unsigned lenValue(TType time_type) const;
      Pos end() const;
      void setEnd(const Pos&);
      void setEndValue(unsigned val);
      void setEndValue(unsigned val, TType time_type);     
      unsigned endTick() const    { return end().tick(); }
      unsigned endFrame() const   { return end().frame(); }
      unsigned endValue() const;
      unsigned endValue(TType time_type) const;
      void setPos(const Pos&);
      static unsigned convertLen(unsigned val, unsigned len, TType from_type, TType to_type);
      };

} // namespace MusECore

#endif
