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

#include <stdio.h>
#include <stdarg.h>

#include <QByteArray>
#include <QString>
#include <QColor>
#include <QWidget>
#include <QRect>

#include "xml.h"

namespace MusECore {

//---------------------------------------------------------
//  Note:
//    this code is a Q/D hack for reading/parsing and
//    writing XML-Files
//    - can only handle the XML subset used by MusE
//    - may not handle misformed XML (eg. when manually
//      editing MusE output)

//---------------------------------------------------------
//   Xml
//---------------------------------------------------------

Xml::Xml(FILE* _f)
      {
      f          = _f;
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
      f         = 0;
      _line     = 0;
      _col      = 0;
      level     = 0;
      inTag     = false;
      inComment = false;
      bufptr    = buf;
      _minorVersion = -1;
      _majorVersion = -1;
      }

//---------------------------------------------------------
//   next
//---------------------------------------------------------

void Xml::next()
      {
      if (*bufptr == 0) {
            if (f == 0 || fgets(lbuffer, 512, f) == 0) {
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
            buffer[i++] = c;
            next();
            }
      buffer[i] = 0;
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
      buffer[i] = c;
      ++i;
      next();

      for (;i < 10000000*4-1;) {  // Stop at a reasonably large amount 10 million.
            if (c == '"') {
                  buffer[i++] = c;
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
                                    c = '\\';
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
                        buffer[i++] = '&';
                        for (;(i < 511) && (n < k); ++i, ++n)
                              buffer[i] = entity[n];
                        }
                  else
                        buffer[i++] = c;
                  }
            else if(c != EOF)
              buffer[i++] = c;
            if (c == EOF)
                  break;
            next();
            }
      buffer[i] = 0;
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
            printf("unexpected EOF reading *.med file at level %d, line %d, <%s><%s><%s>\n",
               level, _line, _tag.toLatin1().constData(), _s1.toLatin1().constData(), _s2.toLatin1().constData());
            return level == 0 ? End : Error;
            }

      _s1 = QString("");
      if (inTag) {
            //-------------------
            // parse Attributes
            //-------------------
            if (c == '/') {
                  nextc();
                  token('>');
                  if (c != '>') {
                        printf("Xml: unexpected char '%c', expected '>'\n", c);
                        goto error;
                        }
                  _s1   = _tag;
                  inTag = false;
                  --level;
                  return TagEnd;
                  }
            _s2 = QString("");
            token('=');
            _s1 = _s2;
            nextc();      // skip space
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
                  for (;;) {
                        if (c == '?' || c == EOF || c == '>')
                              break;
                        
                        buffer[idx++] = c;
                        
                        // TODO: check overflow
                        next();
                        }
                  
                  buffer[idx] = 0;
                  
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
            else if (c == '!') {    // process comment
                  bool endc = false;
                  for(;;) {
                        next();
                        if (c == '>' && endc)
                              break;
                        endc = c == '-';
                        if (c == EOF) {
                              fprintf(stderr, "XML: unexpected EOF in comment\n");
                              goto error;
                              }
                        }
                  goto again;
                  }
            idx = 0;
            for (;;) {
                  if (c == '/' || c == ' ' || c == '\t' || c == '>' || c == '\n' || c == EOF)
                        break;
                  // TODO: check overflow
                  
                  buffer[idx++] = c;
                  
                  next();
                  }
            
            buffer[idx] = 0;
            
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
            for (;;) {
                  if (c == EOF || c == '<')
                        break;
                  if (c == '&') {
                        next();
                        if (c == '<') {         // be tolerant with old muse files
                              buffer[idx++] = '&';
                              
                              continue;
                              }
                              
                        QByteArray name;
                        int name_idx = 0;
                        
                        name[name_idx++] = c;
                        
                        for (; name_idx < 9999999;) {   // Stop at a reasonably large amount 10 million.
                              next();
                              if (c == ';')
                                    break;
                              
                              name[name_idx++] = c;
                              }
                              
                        name[name_idx] = 0;
                        
                        if (strcmp(name, "lt") == 0)
                              c = '<';
                        else if (strcmp(name, "gt") == 0)
                              c = '>';
                        else if (strcmp(name, "apos") == 0)
                              c = '\\';
                        else if (strcmp(name, "quot") == 0)
                              c = '"';
                        else if (strcmp(name, "amp") == 0)
                              c = '&';
                        else
                              c = '?';
                        }

                  buffer[idx++] = c;
                  
                  next();
                  }
                  
            buffer[idx] = 0;
            
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
//   parseUInt
//---------------------------------------------------------
// Added by Tim. p3.3.8

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
//   unknown
//---------------------------------------------------------

void Xml::unknown(const char* s)
      {
      printf("%s: unknown tag <%s> at line %d\n",
         s, _s1.toLatin1().constData(), _line+1);
      parse1();
      }

//---------------------------------------------------------
//   header
//---------------------------------------------------------

void Xml::header()
      {
      fprintf(f, "<?xml version=\"1.0\"?>\n");
      }

//---------------------------------------------------------
//   put
//---------------------------------------------------------

void Xml::put(const char* format, ...)
      {
      va_list args;
      va_start(args, format);

      vfprintf(f, format, args);
      va_end(args);
      putc('\n', f);
      }

void Xml::put(int level, const char* format, ...)
      {
      va_list args;
      va_start(args, format);
      putLevel(level);
      vfprintf(f, format, args);
      va_end(args);
      putc('\n', f);
      }

//---------------------------------------------------------
//   nput
//---------------------------------------------------------

void Xml::nput(int level, const char* format, ...)
      {
      va_list args;
      va_start(args, format);
      putLevel(level);
      vfprintf(f, format, args);
      va_end(args);
      }

void Xml::nput(const char* format, ...)
      {
      va_list args;
      va_start(args, format);
      vfprintf(f, format, args);
      va_end(args);
      }

//---------------------------------------------------------
//   tag
//---------------------------------------------------------

void Xml::tag(int level, const char* format, ...)
      {
      va_list args;
      va_start(args, format);
      putLevel(level);
      putc('<', f);
      vfprintf(f, format, args);
      va_end(args);
      putc('>', f);
      putc('\n', f);
      }

//---------------------------------------------------------
//   etag
//---------------------------------------------------------

void Xml::etag(int level, const char* format, ...)
      {
      va_list args;
      va_start(args, format);
      putLevel(level);
      putc('<', f);
      putc('/', f);
      vfprintf(f, format, args);
      va_end(args);
      putc('>', f);
      putc('\n', f);
      }

void Xml::putLevel(int n)
      {
      for (int i = 0; i < n*2; ++i)
            putc(' ', f);
      }

void Xml::intTag(int level, const char* name, int val)
      {
      putLevel(level);
      fprintf(f, "<%s>%d</%s>\n", name, val, name);
      }

void Xml::uintTag(int level, const char* name, unsigned int val)
      {
      putLevel(level);
      fprintf(f, "<%s>%u</%s>\n", name, val, name);
      }

void Xml::floatTag(int level, const char* name, float val)
      {
      putLevel(level);
      QString s("<%1>%2</%3>\n");
      fprintf(f, "%s", s.arg(name).arg(val).arg(name).toLatin1().constData());
      }

void Xml::doubleTag(int level, const char* name, double val)
      {
      putLevel(level);
      QString s("<%1>%2</%3>\n");
      fprintf(f, "%s", s.arg(name).arg(val).arg(name).toLatin1().constData());
      }

void Xml::strTag(int level, const char* name, const char* val)
      {
      putLevel(level);
      fprintf(f, "<%s>", name);
      if (val) {
            while (*val) {
                  switch(*val) {
                        case '&': fprintf(f, "&amp;"); break;
                        case '<': fprintf(f, "&lt;"); break;
                        case '>': fprintf(f, "&gt;"); break;
                        case '\\': fprintf(f, "&apos;"); break;
                        case '"': fprintf(f, "&quot;"); break;
                        default: fputc(*val, f); break;
                        }
                  ++val;
                  }
            }
      fprintf(f, "</%s>\n", name);
      }

//---------------------------------------------------------
//   colorTag
//---------------------------------------------------------

void Xml::colorTag(int level, const char* name, const QColor& color)
      {
      putLevel(level);
      fprintf(f, "<%s r=\"%d\" g=\"%d\" b=\"%d\"></%s>\n",
	    name, color.red(), color.green(), color.blue(), name);
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
      fprintf(f, "<%s x=\"%d\" y=\"%d\" w=\"%d\" h=\"%d\"></%s>\n",
         name, r.x(), r.y(), r.width(), r.height(), name);
      }

//---------------------------------------------------------
//   strTag
//---------------------------------------------------------

void Xml::strTag(int level, const char* name, const QString& val)
      {
      strTag(level, name, val.toLatin1().constData());
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

void Xml::dump(QString &dump)
      {
      if (f == 0)
          return;
      fpos_t pos;
      fgetpos(f, &pos);
      rewind(f);
      while(fgets(lbuffer, 512, f) != 0)
          dump.append(lbuffer);
       fsetpos(f, &pos);
      }

} // namespace MusECore
