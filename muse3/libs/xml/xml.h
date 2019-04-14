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
#include <QColor>
#include <QRect>
#include <QWidget>
#include <QIODevice>

namespace MusECore {

//---------------------------------------------------------
//   Xml
//    very simple XML-like parser
//---------------------------------------------------------

class Xml {
      // When constructed with a FILE* parameter, this will be valid.
      FILE* f;
      // When constructed with a QString* parameter, this will be valid.
      QString* _destStr;
      // When constructed with a QIODevice* parameter, this will be valid.
      QIODevice* _destIODev;
      int _line;
      int _col;
      QString _s1, _s2, _tag;
      int level;
      bool inTag;
      bool inComment;
      // NOTICE: Whenever the MusE song file structure changes - even for a small reason, bump these up!
      //         To preserve user data please try to add song-loading converters to accompany any changes so no data is lost.
      //         The user will be automatically informed and warned (via GUI) of loading an OLD or FUTURE song file,
      //          and that the song might be converted if saved!
      // TODO: Always make this equal the MusE version? 
      //       But how do we bump up the MusE version safely before releasing, for testing/coding purposes?
      static const int _latestMinorVersion;   // Latest known songfile minor version (as of this release)
      static const int _latestMajorVersion;   // Latest known songfile major version (as of this release)
      int _minorVersion;                      // Currently loaded songfile minor version
      int _majorVersion;                      // Currently loaded songfile major version

      char c;            // current char
      char lbuffer[512];
      // When constructed with a const char* parameter, this will be valid.
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
      int latestMajorVersion() const { return _latestMajorVersion; }
      int latestMinorVersion() const { return _latestMinorVersion; }
      int majorVersion() const { return _majorVersion; }
      int minorVersion() const { return _minorVersion; }
      bool isVersionEqualToLatest() const { return _majorVersion == _latestMajorVersion && _minorVersion == _latestMinorVersion; }
      bool isVersionLessThanLatest() const { return _majorVersion < _latestMajorVersion || _minorVersion < _latestMinorVersion; }
      bool isVersionGreaterThanLatest() const { return _majorVersion > _latestMajorVersion || _minorVersion > _latestMinorVersion; }
      void setVersion(int maj, int min) {
            _minorVersion = min;
            _majorVersion = maj;
            }
      // For writing and reading a FILE. Constructs an xml from a FILE.
      Xml(FILE*);
      // For reading from a char array only. Writing may cause error. Constructs an xml from a char array.
      Xml(const char*);
      // For writing to a QString only. Reading may cause error. Constructs an xml from a QString.
      Xml(QString*);
      // For writing and reading a QIODevice. Constructs an xml from a QIODevice.
      Xml(QIODevice*);

      Token parse();
      QString parse(const QString&);
      QString parse1();
      int parseInt();
      unsigned int parseUInt();
      long long parseLongLong();
      unsigned long long parseULongLong();
      float parseFloat();
      double parseDouble();
      void unknown(const char*);
      int line() const    { return _line; }    // current line
      int col()  const    { return _col; }     // current col
      const QString& s1() { return _s1; }
      const QString& s2() { return _s2; }
      void dump(QString &dump);

      void header();
      // Returns level incremented by one.
      int putFileVersion(int level);
      void put(const char* format, ...);
      void put(int level, const char* format, ...);
      void nput(int level, const char* format, ...);
      void nput(const char* format, ...);
      void tag(int level, const char* format, ...);
      void etag(int level, const char* format, ...);
      void intTag(int level, const char* const name, int val);
      void uintTag(int level, const char* const name, unsigned int val);
      void longLongTag(int level, const char* const name, long long val);
      void uLongLongTag(int level, const char* const name, unsigned long long val);
      void doubleTag(int level, const char* const name, double val);
      void floatTag(int level, const char* const name, float val);
      void strTag(int level, const char* const name, const char* val);
      void strTag(int level, const char* const name, const QString& s);
      void strTag(int level, const QString& name, const QString& val);
      void colorTag(int level, const char* name, const QColor& color);
      void colorTag(int level, const QString& name, const QColor& color);
      void geometryTag(int level, const char* name, const QWidget* g);
      void qrectTag(int level, const char* name, const QRect& r);
      static QString xmlString(const QString&);
      static QString xmlString(const char*);

      void skip(const QString& tag);
      };

  //---------------------------------------------------------
  //   Basic functions:
  //---------------------------------------------------------

  QRect readGeometry(Xml& xml, const QString& name);
  QColor readColor(Xml& xml);


} // namespace MusECore

#endif

