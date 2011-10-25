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

   public:
      Pos();
      Pos(const Pos&);
      Pos(int,int,int);
      Pos(int,int,int,int);
      Pos(unsigned, bool ticks=true);
      Pos(const QString&);
      void dump(int n = 0) const;
      void mbt(int*, int*, int*) const;
      void msf(int*, int*, int*, int*) const;

      void invalidSn()  { sn = -1; }

      TType  type() const     { return _type; }
      void   setType(TType t);

      Pos& operator+=(Pos a);
      Pos& operator+=(int a);

      bool operator>=(const Pos& s) const;
      bool operator>(const Pos& s) const;
      bool operator<(const Pos& s) const;
      bool operator<=(const Pos& s) const;
      bool operator==(const Pos& s) const;

      friend Pos operator+(Pos a, Pos b);
      friend Pos operator+(Pos a, int b);

      unsigned tick() const;
      unsigned frame() const;
      void setTick(unsigned);
      void setFrame(unsigned);

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
      PosLen();
      PosLen(const PosLen&);
      void dump(int n = 0) const;

      void write(int level, Xml&, const char*) const;
      void read(Xml& xml, const char*);
      void setLenTick(unsigned);
      void setLenFrame(unsigned);
      unsigned lenTick() const;
      unsigned lenFrame() const;
      Pos end() const;
      unsigned endTick() const    { return end().tick(); }
      unsigned endFrame() const   { return end().frame(); }
      void setPos(const Pos&);
      };

} // namespace MusECore

#endif
