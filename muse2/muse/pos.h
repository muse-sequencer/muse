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
      enum TType { FRAMES=0, TICKS=1 };

   private:
      TType _type;
      mutable int sn;
      mutable unsigned _tick;
      mutable unsigned _frame;
      bool _nullFlag;

   public:
      Pos();
      Pos(const Pos&);
      Pos(int,int,int);
      Pos(int,int,int,int);
      Pos(unsigned, bool ticks=true);
      Pos(unsigned, TType time_type = TICKS);
      Pos(const QString&);
      Pos(bool is_null);
      void dump(int n = 0) const;
      void mbt(int*, int*, int*) const;
      void msf(int*, int*, int*, int*) const;

      void invalidSn()  { sn = -1; }
      bool nullFlag() const { return _nullFlag; }
      void setNullFlag(bool f) { _nullFlag = f; }
      
      TType  type() const     { return _type; }
      void   setType(TType time_type);

      Pos& operator+=(Pos a);
      Pos& operator+=(int a);
      Pos& operator-=(Pos a);
      Pos& operator-=(int a);

      bool operator>=(const Pos& s) const;
      bool operator>(const Pos& s) const;
      bool operator<(const Pos& s) const;
      bool operator<=(const Pos& s) const;
      bool operator==(const Pos& s) const;

      friend Pos operator+(Pos a, Pos b);
      friend Pos operator+(Pos a, int b);
      friend Pos operator-(Pos a, Pos b);
      friend Pos operator-(Pos a, int b);

      unsigned tick() const;
      unsigned frame() const;
      unsigned posValue(TType time_type) const;
      void setTick(unsigned);
      void setFrame(unsigned);
      void setPosValue(unsigned val, TType time_type);     
      void setPos(const Pos&);

      void write(int level, Xml&, const char*) const;
      void read(Xml& xml, const char*);
      bool isValid() const; 
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
      PosLen(unsigned pos, unsigned len, TType time_type = TICKS);
      PosLen(const Pos& pos, unsigned len);
      PosLen(const Pos& start_pos, const Pos& end_pos);
      void dump(int n = 0) const;

      void write(int level, Xml&, const char*) const;
      void read(Xml& xml, const char*);
      void setLenTick(unsigned);
      void setLenFrame(unsigned);
      void setLenValue(unsigned val, TType time_type);     
      void setLen(const Pos& start_pos, const Pos& end_pos);
      void setLen(const PosLen& len);     
      void setPosLen(const Pos& pos, const Pos& end_pos);
      unsigned lenTick() const;
      unsigned lenFrame() const;
      unsigned lenValue(TType time_type) const;
      Pos end() const;
      unsigned endTick() const    { return end().tick(); }
      unsigned endFrame() const   { return end().frame(); }
      //void setPos(const Pos&);   // REMOVE Tim.
      };

} // namespace MusECore

#endif
