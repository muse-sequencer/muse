//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: xml.cpp,v 1.17.2.6 2009/12/07 20:48:45 spamatica Exp $
//
//  (C) Copyright 2000 Werner Schweer (ws@seh.de)
//  (C) Copyright 2011 Tim E. Real (terminator356 on sourceforge)
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

#include <stdarg.h>
#include <QByteArray>

#include "xml.h"

namespace MusECore {

const int Xml::_latestMajorVersion = 4;   // Latest known songfile major version (as of this release)
const int Xml::_latestMinorVersion = 0;   // Latest known songfile minor version (as of this release)

//---------------------------------------------------------
//  Note:
//    this code is a Q/D hack for reading/parsing and
//    writing XML-Files
//    - can only handle the XML subset used by MusE
//    - may not handle malformed XML (eg. when manually
//      editing MusE output)

//---------------------------------------------------------
//   Xml
//---------------------------------------------------------

Xml::Xml(FILE* _f)
      {
      f          = _f;
      _destStr   = nullptr;
      _destIODev = nullptr;
      _line      = 0;
      _col       = 0;
      level      = 0;
      inTag      = false;
      inComment  = false;
      lbuffer[0] = 0;
      bufptr     = lbuffer;
      _minorVersion = -1;
      _majorVersion = -1;
      }

Xml::Xml(const char* buf)
      {
      f         = nullptr;
      _destStr  = nullptr;
      _destIODev = nullptr;
      _line     = 0;
      _col      = 0;
      level     = 0;
      inTag     = false;
      inComment = false;
      bufptr    = buf;
      _minorVersion = -1;
      _majorVersion = -1;
      }

Xml::Xml(QString* s)
      {
      f         = nullptr;
      _destIODev = nullptr;
      _line     = 0;
      _col      = 0;
      level     = 0;
      inTag     = false;
      inComment = false;
      lbuffer[0] = 0;
      bufptr     = lbuffer;
      _destStr   = s;
      _minorVersion = -1;
      _majorVersion = -1;
      }

Xml::Xml(QIODevice* d)
      {
      f         = nullptr;
      _destStr   = nullptr;
      _destIODev = d;
      _line     = 0;
      _col      = 0;
      level     = 0;
      inTag     = false;
      inComment = false;
      lbuffer[0] = 0;
      bufptr     = lbuffer;
      _minorVersion = -1;
      _majorVersion = -1;
      }


//---------------------------------------------------------
//   BEGIN Read functions:
//---------------------------------------------------------

      
//---------------------------------------------------------
//   next
//---------------------------------------------------------

void Xml::next()
      {
      if (*bufptr == 0) {
        
            if((!f && !_destIODev) ||
               (f && fgets(lbuffer, 512, f) == nullptr) ||
               (_destIODev && _destIODev->readLine(lbuffer, 512) <= 0))
            {
              c = EOF;
              return;
            }
                  
            bufptr = lbuffer;
            }
      c = *bufptr++;
      if (c == '\n') {
            ++_line;
            _col = -1;
            }
      ++_col;
      }

//---------------------------------------------------------
//   nextc
//    get next non space character
//---------------------------------------------------------

void Xml::nextc()
      {
      next();
      while (c == ' ' || c == '\t' || c == '\n')
            next();
      }

//---------------------------------------------------------
//   token
//    read token into _s2
//---------------------------------------------------------

void Xml::token(int cc)
      {
      QByteArray buffer;
      int i = 0;
      for (; i < 9999999;) {   // Stop at a reasonably large amount 10 million.
            if (c == ' ' || c == '\t' || c == cc || c == '\n' || c == EOF)
                  break;
            buffer.append(c);
            i++;
            next();
            }
      // No! Not in Qt6. Both Qt5 and Qt6 QByteArray always maintain a terminating null, but in Qt6:
      // "Note: Any null ('\0') bytes in the byte array will be included in this string,
      //   converted to Unicode null characters (U+0000). This behavior is different from Qt 5.x.
      //buffer.append(char(0));
      _s2 = buffer;     // deep copy !?
      }

//---------------------------------------------------------
//   stoken
//    read string token into _s2
//---------------------------------------------------------

void Xml::stoken()
      {
      QByteArray buffer;
      int i = 0;
      buffer.append(c);
      i++;
      next();

      for (;i < 10000000*4-1;) {  // Stop at a reasonably large amount 10 million.
            if (c == '"') {
                  buffer.append(c);
                  i++;
                  next();
                  break;
                  }
            if (c == '&') {
                  char entity[6];
                  int k = 0;
                  for (; k < 6; ++k) {
                        next();
                        if (c == EOF)
                             break;
                        else if (c == ';') {
                              entity[k] = 0;
                              if (strcmp(entity, "quot") == 0)
                                    c = '"';
                              else if (strcmp(entity, "amp") == 0)
                                    c = '&';
                              else if (strcmp(entity, "lt") == 0)
                                    c = '<';
                              else if (strcmp(entity, "gt") == 0)
                                    c = '>';
                              else if (strcmp(entity, "apos") == 0)
                                    c = '\'';
                              else
                                    entity[k] = c;
                              break;
                              }
                        else
                              entity[k] = c;
                        }
                  if (c == EOF || k == 6) {
                        // dump entity
                        int n = 0;
                        buffer.append('&');
                        i++;
                        for (;(i < 511) && (n < k); ++i, ++n)
                              buffer.append(entity[n]);
                        }
                  else {
                        buffer.append(c);
                        i++;
                     }
                  }
            else if(c != EOF)
            {
              buffer.append(c);
              i++;
            }
            if (c == EOF)
                  break;
            next();
            }
      // No! Not in Qt6. Both Qt5 and Qt6 QByteArray always maintain a terminating null, but in Qt6:
      // "Note: Any null ('\0') bytes in the byte array will be included in this string,
      //   converted to Unicode null characters (U+0000). This behavior is different from Qt 5.x.
      //buffer.append(char(0));
      _s2 = buffer;
      }

//---------------------------------------------------------
//   strip
//    strip `"` from string
//---------------------------------------------------------

QString Xml::strip(const QString& s)
      {
      int l = s.length();
      if (l >= 2 && s[0] == '"')
            return s.mid(1, l-2);
      return s;
      }

//---------------------------------------------------------
//   parse
//---------------------------------------------------------

Xml::Token Xml::parse()
      {
      QByteArray buffer;
      int idx = 0;

 again:
      bool endFlag = false;
      nextc();
      if (c == EOF) {
            //if (level > 0 || MusEGlobal::debugMsg)
            if (level > 0)
              fprintf(stderr, "WARNING: unexpected EOF reading xml file at level %d, line %d, <%s><%s><%s>\n",
                level, _line, _tag.toLocal8Bit().constData(), _s1.toLocal8Bit().constData(), _s2.toLocal8Bit().constData());
            return level == 0 ? End : Error;
            }

      _s1.clear();
      if (inTag) {
            //-------------------
            // parse Attributes
            //-------------------
            if (c == '/') {
                  nextc();
                  token('>');
                  if (c != '>') {
                        fprintf(stderr, "Xml: unexpected char '%c', expected '>'\n", c);
                        goto error;
                        }
                  _s1   = _tag;
                  inTag = false;
                  --level;
                  return TagEnd;
                  }
            _s2.clear();
            token('=');
            _s1 = _s2;
            nextc();      // skip space
            if (c == EOF) {
                  //if (level > 0 || MusEGlobal::debugMsg)
                  if (level > 0)
                    fprintf(stderr, "WARNING: unexpected EOF reading xml file at level %d, line %d, <%s><%s><%s>\n",
                      level, _line, _tag.toLocal8Bit().constData(),
                      _s1.toLocal8Bit().constData(), _s2.toLocal8Bit().constData());
                  return level == 0 ? End : Error;
                  }
            if (c == '"')
                  stoken();
            else
                  token('>');
            if (c == '>')
                  inTag = false;
            else
                  --bufptr;
            _s2 = strip(_s2);
            return Attribut;
            }
      if (c == '<') {
            //--------------
            // parse Tag
            //--------------
            next();
            if (c == '/') {
                  endFlag = true;
                  next();
                  }
            if (c == '?') {
                  next();
                  idx = 0;
                  buffer.clear();
                  for (;;) {
                        if (c == '?' || c == EOF || c == '>')
                              break;
                        
                        buffer.append(c);
                        idx++;
                        
                        // TODO: check overflow
                        next();
                        }
                  
                  // No! Not in Qt6. Both Qt5 and Qt6 QByteArray always maintain a terminating null, but in Qt6:
                  // "Note: Any null ('\0') bytes in the byte array will be included in this string,
                  //   converted to Unicode null characters (U+0000). This behavior is different from Qt 5.x.
                  //buffer.append(char(0));
                  _s1 = QString(buffer);

                  if (c == EOF) {
                        fprintf(stderr, "XML: unexpected EOF\n");
                        goto error;
                        }
                  nextc();
                  if (c != '>') {
                        fprintf(stderr, "XML: '>' expected\n");
                        goto error;
                        }
                  next();
                  return Proc;
                  }
            else if (c == '!') {    // process comment or declaration
                  bool check_start = true;
                  bool startc = false;
                  bool startc_2 = false;
                  bool endc = false;
                  bool endc_2 = false;
                  for(;;) {
                        next();
                        if(c == '-')
                        {
                          if (check_start)
                          {
                            if(startc)
                            {
                              startc_2 = true;
                              check_start = false;
                            }
                            else
                            {
                              startc = true;
                            }
                            continue;
                          }
                          // Check for end -- only if start -- was found.
                          else if(startc_2)
                          {
                            if(endc)
                              endc_2 = true;
                            else
                              endc = true;
                            continue;
                          }
                        }
                        else
                        {
                          if (check_start)
                          {
                            check_start = false;
                            // It's a declaration, such as 'DOCTYPE' reference to dtd...
                          }
                          // Check for '>' if declaration (no start --) or comment (start and end -- found).
                          if (c == '>' && (!startc_2 || endc_2))
                                break;
                          if (c == EOF) {
                                fprintf(stderr, "XML: unexpected EOF in comment\n");
                                goto error;
                                }
                          // It's not a '-' character so that destroys the start of any comment end sequence.
                          endc = endc_2 = false;
                        }
                        }
                  goto again;
                  }
            idx = 0;
            buffer.clear();
            for (;;) {
                  if (c == '/' || c == ' ' || c == '\t' || c == '>' || c == '\n' || c == EOF)
                        break;
                  // TODO: check overflow
                  
                  buffer.append(c);
                  idx++;
                  
                  next();
                  }
            
            // No! Not in Qt6. Both Qt5 and Qt6 QByteArray always maintain a terminating null, but in Qt6:
            // "Note: Any null ('\0') bytes in the byte array will be included in this string,
            //   converted to Unicode null characters (U+0000). This behavior is different from Qt 5.x.
            //buffer.append(char(0));
            _s1 = QString(buffer);

            // skip white space:
            while (c == ' ' || c == '\t' || c == '\n')
                  next();
            if (c == '/') {
                  nextc();
                  if (c == '>')
                        return Flag;
                  fprintf(stderr, "XML: '>' expected\n");
                  goto error;
                  }
            if (c == '?') {
                  nextc();
                  if (c == '>')
                        return Proc;
                  fprintf(stderr, "XML: '>' expected\n");
                  goto error;
                  }
            if (c == '>') {
                  if (endFlag) {
                        --level;
                        return TagEnd;
                        }
                  else {
                        ++level;
                        return TagStart;
                        }
                  }
            else {
                  _tag = _s1;
                  --bufptr;
                  inTag = true;
                  ++level;
                  if (!endFlag) {
                        return TagStart;
                        }
                  fprintf(stderr, "XML: endFlag expected\n");
                  goto error;
                  }
            }
      else {
            //--------------
            // parse Text
            //--------------
            if (level == 0) {
                  fprintf(stderr, "XML: level = 0\n");
                  goto error;
                  }
            idx = 0;
            buffer.clear();
            for (;;) {
                  if (c == EOF || c == '<')
                        break;
                  if (c == '&') {
                        next();
                        if (c == '<') {         // be tolerant with old muse files
                              buffer.append('&');
                              idx++;
                              continue;
                              }
                              
                        QByteArray name;
                        int name_idx = 0;
                        name.append(c);
                        name_idx++;
                     
                        for (; name_idx < 9999999;) {   // Stop at a reasonably large amount 10 million.
                              next();
                              if (c == ';')
                                    break;
                              name.append(c);
                              name_idx++;
                              }
                              
                        // No! Not in Qt6. Both Qt5 and Qt6 QByteArray always maintain a terminating null, but in Qt6:
                        // "Note: Any null ('\0') bytes in the byte array will be included in this string,
                        //   converted to Unicode null characters (U+0000). This behavior is different from Qt 5.x.
                        //name.append(char(0));

                        if (strcmp(name, "lt") == 0)
                              c = '<';
                        else if (strcmp(name, "gt") == 0)
                              c = '>';
                        else if (strcmp(name, "apos") == 0)
                              c = '\'';
                        else if (strcmp(name, "quot") == 0)
                              c = '"';
                        else if (strcmp(name, "amp") == 0)
                              c = '&';
                        else
                              c = '?';
                        
                        }

                  buffer.append(c);
                  idx++;
                  
                  next();
                  }
                  
            // No! Not in Qt6. Both Qt5 and Qt6 QByteArray always maintain a terminating null, but in Qt6:
            // "Note: Any null ('\0') bytes in the byte array will be included in this string,
            //   converted to Unicode null characters (U+0000). This behavior is different from Qt 5.x.
            //buffer.append(char(0));
            _s1 = QString(buffer);

            if (c == '<')
                  --bufptr;
            return Text;
            }
error:
      fprintf(stderr, "XML Parse Error at line %d col %d\n", _line, _col+1);
      return Error;
      }

//---------------------------------------------------------
//   parse(QString)
//---------------------------------------------------------

QString Xml::parse(const QString& tag)
      {
      QString a;

      for (;;) {
            switch (parse()) {
                  case Error:
                  case End:
                        return a;
                  default:
                  case TagStart:
                  case Attribut:
                        break;
                  case Text:
                        a = _s1;
                        break;
                  case TagEnd:
                        if (_s1 == tag)
                              return a;
                        break;
                  }
            }
      return a;
      }

//---------------------------------------------------------
//   parse1
//---------------------------------------------------------

QString Xml::parse1()
      {
      return parse(_s1.simplified());
      }

//---------------------------------------------------------
//   parseInt
//---------------------------------------------------------

int Xml::parseInt()
      {
      QString s(parse1().simplified());
      bool ok;
      int base = 10;
      if (s.startsWith("0x") || s.startsWith("0X")) {
            base = 16;
            s = s.mid(2);
            }
      int n = s.toInt(&ok, base);
      return n;
      }

//---------------------------------------------------------
//   parseLongLong
//---------------------------------------------------------

long long Xml::parseLongLong()
      {
      QString s(parse1().simplified());
      bool ok;
      int base = 10;
      if (s.startsWith("0x") || s.startsWith("0X")) {
            base = 16;
            s = s.mid(2);
            }
      long long n = s.toLongLong(&ok, base);
      return n;
      }

//---------------------------------------------------------
//   parseULongLong
//---------------------------------------------------------

unsigned long long Xml::parseULongLong()
      {
      QString s(parse1().simplified());
      bool ok;
      int base = 10;
      if (s.startsWith("0x") || s.startsWith("0X")) {
            base = 16;
            s = s.mid(2);
            }
      unsigned long long n = s.toULongLong(&ok, base);
      return n;
      }

//---------------------------------------------------------
//   parseLongInt
//---------------------------------------------------------

long int Xml::parseLongInt()
      {
      QString s(parse1().simplified());
      bool ok;
      int base = 10;
      if (s.startsWith("0x") || s.startsWith("0X")) {
            base = 16;
            s = s.mid(2);
            }
      long int n = s.toLong(&ok, base);
      return n;
      }

//---------------------------------------------------------
//   parseUInt
//---------------------------------------------------------

unsigned int Xml::parseUInt()
      {
      QString s(parse1().simplified());
      bool ok;
      int base = 10;
      if (s.startsWith("0x") || s.startsWith("0X")) {
            base = 16;
            s = s.mid(2);
            }
      unsigned int n = s.toUInt(&ok, base);
      return n;
      }

//---------------------------------------------------------
//   parseLongUInt
//---------------------------------------------------------

unsigned long int Xml::parseLongUInt()
      {
      QString s(parse1().simplified());
      bool ok;
      int base = 10;
      if (s.startsWith("0x") || s.startsWith("0X")) {
            base = 16;
            s = s.mid(2);
            }
      unsigned long int n = s.toULong(&ok, base);
      return n;
      }

//---------------------------------------------------------
//   parseFloat
//---------------------------------------------------------

float Xml::parseFloat()
      {
      QString s(parse1().simplified());
      return s.toFloat();
      }

//---------------------------------------------------------
//   parseDouble
//---------------------------------------------------------

double Xml::parseDouble()
      {
      QString s(parse1().simplified());
      return s.toDouble();
      }

//---------------------------------------------------------
//   Xml::skip
//---------------------------------------------------------

void Xml::skip(const QString& etag)
      {
      for (;;) {
            Token token = parse();
            const QString& tag = s1();
            switch (token) {
                  case Xml::Error:
                  case Xml::End:
                        return;
                  case Xml::Text:
                        break;
                  case Xml::TagEnd:
                        if (tag == etag)
                              return;
                        break;
                  case Xml::TagStart:
                        skip(tag);
                        break;
                  default:
                        break;
                  }
            }
      }

void Xml::dump(QString &dump)
      {
      if(f)
      {
        fpos_t pos;
        fgetpos(f, &pos);
        rewind(f);
        while(fgets(lbuffer, 512, f) != nullptr)
            dump.append(lbuffer);
        fsetpos(f, &pos);
      }
      else if(_destIODev)
      {
        // Only if not sequential.
        if(!_destIODev->isSequential())
        {
          const qint64 pos = _destIODev->pos();
          _destIODev->seek(0);
          while(_destIODev->read(lbuffer, 512) > 0)
              dump.append(lbuffer);
          _destIODev->seek(pos);
        }
      }
      else if(_destStr)
      {
        dump.append(*_destStr);
      }
      }

//---------------------------------------------------------
//   Basic functions:
//---------------------------------------------------------

//---------------------------------------------------------
//   readGeometry
//---------------------------------------------------------

QRect readGeometry(Xml& xml, const QString& name)
      {
      QRect r(0, 0, 50, 50);
      int val;

      for (;;) {
            Xml::Token token = xml.parse();
            if (token == Xml::Error || token == Xml::End)
                  break;
            QString tag = xml.s1();
            switch (token) {
                  case Xml::TagStart:
                        xml.parse1();
                        break;
                  case Xml::Attribut:
                        val = xml.s2().toInt();
                        if (tag == "x")
                              r.setX(val);
                        else if (tag == "y")
                              r.setY(val);
                        else if (tag == "w")
                              r.setWidth(val);
                        else if (tag == "h")
                              r.setHeight(val);
                        break;
                  case Xml::TagEnd:
                        if (tag == name)
                              return r;
                  default:
                        break;
                  }
            }
      return r;
      }


//---------------------------------------------------------
//   readColor
//---------------------------------------------------------

QColor readColor(Xml& xml)
       {
       int val, r=0, g=0, b=0;

      for (;;) {
            Xml::Token token = xml.parse();
            if (token != Xml::Attribut)
                  break;
            QString tag = xml.s1();
            switch (token) {
                  case Xml::Attribut:
                        val = xml.s2().toInt();
                        if (tag == "r")
                              r = val;
                        else if (tag == "g")
                              g = val;
                        else if (tag == "b")
                              b = val;
                        break;
                  default:
                        break;
                  }
            }

      return QColor(r, g, b);
      }
      
//---------------------------------------------------------
//   unknown
//---------------------------------------------------------

void Xml::unknown(const char* s)
      {
      fprintf(stderr, "%s: unknown tag <%s> at line %d\n",
         s, _s1.toLocal8Bit().constData(), _line+1);
      parse1();
      }

      
//---------------------------------------------------------
//   BEGIN Write functions:
//---------------------------------------------------------


//---------------------------------------------------------
//   header
//---------------------------------------------------------

void Xml::header()
      {
      const char* s = "<?xml version=\"1.0\" encoding=\"utf8\"?>\n";
      if(f)
        fprintf(f, "%s", s);
      else
      {
        if(_destIODev)
          _destIODev->write(s);
        else if(_destStr)
          _destStr->append(s);
      }
      }

//---------------------------------------------------------
//   putFileVersion
//---------------------------------------------------------

int Xml::putFileVersion(int level)
{
  nput(level++, "<muse version=\"%d.%d\">\n", latestMajorVersion(), latestMinorVersion());
  return level;
}

//---------------------------------------------------------
//   put
//---------------------------------------------------------

void Xml::put(const char* format, ...)
      {
      va_list args;
      va_start(args, format);

      if(f)
      {
        vfprintf(f, format, args);
        va_end(args);
        putc('\n', f);
      }
      else
      {
        const QString s = QString::vasprintf(format, args) + '\n';
        va_end(args);
        if(_destIODev)
          _destIODev->write(s.toUtf8());
        else if(_destStr)
          _destStr->append(s);
      }
      }

void Xml::put(int level, const char* format, ...)
      {
      va_list args;
      va_start(args, format);
      putLevel(level);

      if(f)
      {
        vfprintf(f, format, args);
        va_end(args);
        putc('\n', f);
      }
      else
      {
        const QString s = QString::vasprintf(format, args) + '\n';
        va_end(args);
        if(_destIODev)
          _destIODev->write(s.toUtf8());
        else if(_destStr)
          _destStr->append(s);
      }
      }

//---------------------------------------------------------
//   nput
//---------------------------------------------------------

void Xml::nput(int level, const char* format, ...)
      {
      va_list args;
      va_start(args, format);
      putLevel(level);

      if(f)
      {
        vfprintf(f, format, args);
        va_end(args);
      }
      else
      {
        const QString s = QString::vasprintf(format, args);
        va_end(args);
        if(_destIODev)
          _destIODev->write(s.toUtf8());
        else if(_destStr)
          _destStr->append(s);
      }
      }

void Xml::nput(const char* format, ...)
      {
      va_list args;
      va_start(args, format);

      if(f)
      {
        vfprintf(f, format, args);
        va_end(args);
      }
      else
      {
        const QString s = QString::vasprintf(format, args);
        va_end(args);
        if(_destIODev)
          _destIODev->write(s.toUtf8());
        else if(_destStr)
          _destStr->append(s);
      }
      }

//---------------------------------------------------------
//   tag
//---------------------------------------------------------

void Xml::tag(int level, const char* format, ...)
      {
      va_list args;
      va_start(args, format);
      putLevel(level);

      if(f)
      {
        putc('<', f);
        vfprintf(f, format, args);
        va_end(args);
        putc('>', f);
        putc('\n', f);
      }
      else
      {
        const QString s = '<' + QString::vasprintf(format, args) + ">\n";
        va_end(args);
        if(_destIODev)
          _destIODev->write(s.toUtf8());
        else if(_destStr)
          _destStr->append(s);
      }
      }

void Xml::tag(int level, const QString& name)
{
      putLevel(level);

      if(f)
      {
        fprintf(f, "<%s>\n", name.toUtf8().constData());
      }
      else
      {
        const QString s = QString("<%1>\n").arg(name);
        if(_destIODev)
          _destIODev->write(s.toUtf8());
        else if(_destStr)
          _destStr->append(s);
      }
}

//---------------------------------------------------------
//   etag
//---------------------------------------------------------

void Xml::etag(int level, const char* format, ...)
      {
      va_list args;
      va_start(args, format);
      putLevel(level);

      if(f)
      {
        putc('<', f);
        putc('/', f);
        vfprintf(f, format, args);
        va_end(args);
        putc('>', f);
        putc('\n', f);
      }
      else
      {
        const QString s = "</" + QString::vasprintf(format, args) + ">\n";
        va_end(args);
        if(_destIODev)
          _destIODev->write(s.toUtf8());
        else if(_destStr)
          _destStr->append(s);
      }
      }

void Xml::emptyTag(int level, const QString& name)
{
      putLevel(level);

      if(f)
      {
        fprintf(f, "<%s />\n", name.toUtf8().constData());
      }
      else
      {
        const QString s = QString("<%1 />\n").arg(name);
        if(_destIODev)
          _destIODev->write(s.toUtf8());
        else if(_destStr)
          _destStr->append(s);
      }
}

void Xml::putLevel(int n)
      {
      if(f)
      {
        for (int i = 0; i < n*2; ++i)
              putc(' ', f);
      }
      else if(_destIODev)
      {
        for (int i = 0; i < n*2; ++i)
              _destIODev->putChar(' ');
      }
      else if(_destStr)
      {
        for (int i = 0; i < n*2; ++i)
              _destStr->append(' ');
      }
      }

void Xml::intTag(int level, const char* name, int val)
      {
      putLevel(level);
      if(f)
      {
        fprintf(f, "<%s>%d</%s>\n", name, val, name);
      }
      else
      {
        const QString s = QString("<%1>%2</%3>\n").arg(name).arg(val).arg(name);
        if(_destIODev)
        _destIODev->write(s.toUtf8());
        else if(_destStr)
          _destStr->append(s);
      }
      }

void Xml::longIntTag(int level, const char* name, long int val)
      {
      putLevel(level);
      fprintf(f, "<%s>%ld</%s>\n", name, val, name);
      }

void Xml::uintTag(int level, const char* name, unsigned int val)
      {
      putLevel(level);
      if(f)
      {
        fprintf(f, "<%s>%u</%s>\n", name, val, name);
      }
      else
      {
        const QString s = QString("<%1>%2</%3>\n").arg(name).arg(val).arg(name);
        if(_destIODev)
        _destIODev->write(s.toUtf8());
        else if(_destStr)
          _destStr->append(s);
      }
      }

void Xml::longLongTag(int level, const char* name, long long val)
      {
      putLevel(level);
      if(f)
      {
        fprintf(f, "<%s>%lld</%s>\n", name, val, name);
      }
      else
      {
        const QString s = QString("<%1>%2</%3>\n").arg(name).arg(val).arg(name);
        if(_destIODev)
        _destIODev->write(s.toUtf8());
        else if(_destStr)
          _destStr->append(s);
      }
      }

void Xml::uLongLongTag(int level, const char* name, unsigned long long val)
      {
      putLevel(level);
      if(f)
      {
        fprintf(f, "<%s>%llu</%s>\n", name, val, name);
      }
      else
      {
        const QString s = QString("<%1>%2</%3>\n").arg(name).arg(val).arg(name);
        if(_destIODev)
        _destIODev->write(s.toUtf8());
        else if(_destStr)
          _destStr->append(s);
      }
      }

void Xml::longUintTag(int level, const char* name, unsigned long int val)
      {
      putLevel(level);
      fprintf(f, "<%s>%lu</%s>\n", name, val, name);
      }

void Xml::floatTag(int level, const char* name, float val)
      {
      putLevel(level);
      if(f)
      {
        // using QString to format decimal values since we know that
        // toLatin1 will make a string with decimal point instead of
        // decimal comma that some locales use
        QString s("<%1>%2</%3>\n");
        fprintf(f, "%s", s.arg(name).arg(val).arg(name).toUtf8().constData());
      }
      else
      {
        const QString s = QString("<%1>%2</%3>\n").arg(name).arg(val).arg(name);
        if(_destIODev)
          _destIODev->write(s.toUtf8());
        else if(_destStr)
          _destStr->append(s);
      }
      }

void Xml::doubleTag(int level, const char* name, double val)
      {
      putLevel(level);
      if(f)
      {
        // using QString to format decimal values since we know that
        // toLatin1 will make a string with decimal point instead of
        // decimal comma that some locales use
        QString s("<%1>%2</%3>\n");
        fprintf(f, "%s", s.arg(name).arg(val).arg(name).toUtf8().constData());
      }
      else
      {
        const QString s = QString("<%1>%2</%3>\n").arg(name).arg(val).arg(name);
        if(_destIODev)
          _destIODev->write(s.toUtf8());
        else if(_destStr)
          _destStr->append(s);
      }
      }

void Xml::strTag(int level, const char* name, const char* val)
      {
      putLevel(level);
      if(f)
      {
        fprintf(f, "<%s>", name);
        if (val) {
              while (*val) {
                    switch(*val) {
                          case '&': fprintf(f, "&amp;"); break;
                          case '<': fprintf(f, "&lt;"); break;
                          case '>': fprintf(f, "&gt;"); break;
                          case '\'': fprintf(f, "&apos;"); break;
                          case '"': fprintf(f, "&quot;"); break;
                          default: fputc(*val, f); break;
                          }
                    ++val;
                    }
              }
        fprintf(f, "</%s>\n", name);
      }
      else
      {
        QString s = QString("<%1>").arg(name);
        if (val) {
              while (*val) {
                    switch(*val) {
                          case '&': s.append("&amp;"); break;
                          case '<': s.append("&lt;"); break;
                          case '>': s.append("&gt;"); break;
                          case '\'': s.append("&apos;"); break;
                          case '"': s.append("&quot;"); break;
                          default: s.append(*val); break;
                          }
                    ++val;
                    }
              }
        s.append(QString("</%1>\n").arg(name));
        if(_destIODev)
          _destIODev->write(s.toUtf8());
        else if(_destStr)
          _destStr->append(s);
      }
      }

//---------------------------------------------------------
//   colorTag
//---------------------------------------------------------

void Xml::colorTag(int level, const char* name, const QColor& color)
      {
      putLevel(level);
      if(f)
      {
        fprintf(f, "<%s r=\"%d\" g=\"%d\" b=\"%d\"></%s>\n",
	      name, color.red(), color.green(), color.blue(), name);
      }
      else
      {
        const QString s = QString("<%1 r=\"%2\" g=\"%3\" b=\"%4\"></%5>\n")
          .arg(name).arg(color.red()).arg(color.green()).arg(color.blue()).arg(name);
        if(_destIODev)
          _destIODev->write(s.toUtf8());
        else if(_destStr)
          _destStr->append(s);
      }
      }

void Xml::colorTag(int level, const QString& name, const QColor& color)
{
  colorTag(level, name.toUtf8().constData(), color);
}

//---------------------------------------------------------
//   geometryTag
//---------------------------------------------------------

void Xml::geometryTag(int level, const char* name, const QWidget* g)
      {
      qrectTag(level, name, QRect(g->pos(), g->size()));
      }

//---------------------------------------------------------
//   qrectTag
//---------------------------------------------------------

void Xml::qrectTag(int level, const char* name, const QRect& r)
      {
      putLevel(level);
      if(f)
      {
        fprintf(f, "<%s x=\"%d\" y=\"%d\" w=\"%d\" h=\"%d\"></%s>\n",
           name, r.x(), r.y(), r.width(), r.height(), name);
      }
      else
      {
        const QString s = 
          QString("<%1 x=\"%2\" y=\"%3\" w=\"%4\" h=\"%5\"></%6>\n")
           .arg(name).arg(r.x()).arg(r.y()).arg(r.width()).arg(r.height()).arg(name);
          
        if(_destIODev)
          _destIODev->write(s.toUtf8());
        else if(_destStr)
          _destStr->append(s);
      }
      }

//---------------------------------------------------------
//   strTag
//---------------------------------------------------------

void Xml::strTag(int level, const char* name, const QString& val)
      {
      strTag(level, name, val.toUtf8().constData());
      }

void Xml::strTag(int level, const QString& name, const QString& val)
{
  strTag(level, name.toUtf8().constData(), val.toUtf8().constData());
}
      
//---------------------------------------------------------
//   xmlString
//---------------------------------------------------------

QString Xml::xmlString(const char* s)
      {
      return Xml::xmlString(QString(s));
      }

//---------------------------------------------------------
//   xmlString
//---------------------------------------------------------

QString Xml::xmlString(const QString& ss)
      {
      QString s(ss);
      s.replace('&', "&amp;");
      s.replace('<', "&lt;");
      s.replace('>', "&gt;");
      s.replace('\'', "&apos;");
      s.replace('"', "&quot;");
      return s;
      }

} // namespace MusECore
