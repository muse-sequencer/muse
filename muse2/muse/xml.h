//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: xml.h,v 1.8.2.3 2009/11/09 20:28:28 terminator356 Exp $
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

#ifndef __XML_H__
#define __XML_H__

#include <stdio.h>

#include <QString>

class QColor;
class QRect;
class QWidget;

namespace MusECore {

//---------------------------------------------------------
//   Xml
//    very simple XML-like parser
//---------------------------------------------------------

class Xml {
      FILE* f;
      int _line;
      int _col;
      QString _s1, _s2, _tag;
      int level;
      bool inTag;
      bool inComment;
      int _minorVersion;
      int _majorVersion;

      int c;            // current char
      char lbuffer[512];
      const char* bufptr;

      void next();
      void nextc();
      void token(int);
      void stoken();
      QString strip(const QString& s);
      void putLevel(int n);

   public:
      enum Token {Error, TagStart, TagEnd, Flag,
         Proc, Text, Attribut, End};
      int majorVersion() const { return _majorVersion; }
      int minorVersion() const { return _minorVersion; }
      void setVersion(int maj, int min) {
            _minorVersion = min;
            _majorVersion = maj;
            }
      Xml(FILE*);
      Xml(const char*);
      Token parse();
      QString parse(const QString&);
      QString parse1();
      int parseInt();
      unsigned int parseUInt();
      float parseFloat();
      double parseDouble();
      void unknown(const char*);
      int line() const    { return _line; }    // current line
      int col()  const    { return _col; }     // current col
      const QString& s1() { return _s1; }
      const QString& s2() { return _s2; }
      void dump(QString &dump);

      void header();
      void put(const char* format, ...);
      void put(int level, const char* format, ...);
      void nput(int level, const char* format, ...);
      void nput(const char* format, ...);
      void tag(int level, const char* format, ...);
      void etag(int level, const char* format, ...);
      void intTag(int level, const char* const name, int val);
      void uintTag(int level, const char* const name, unsigned int val);
      void doubleTag(int level, const char* const name, double val);
      void floatTag(int level, const char* const name, float val);
      void strTag(int level, const char* const name, const char* val);
      void strTag(int level, const char* const name, const QString& s);
      void colorTag(int level, const char* name, const QColor& color);
      void geometryTag(int level, const char* name, const QWidget* g);
      void qrectTag(int level, const char* name, const QRect& r);
      static QString xmlString(const QString&);
      static QString xmlString(const char*);

      void skip(const QString& tag);
      };

extern QRect readGeometry(Xml&, const QString&);

} // namespace MusECore

#endif

