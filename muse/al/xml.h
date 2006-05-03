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

#ifndef __XML_H__
#define __XML_H__

namespace AL {

static const int BS = 2048;

//---------------------------------------------------------
//   Xml
//---------------------------------------------------------

class Xml : public QTextStream {
    	char buffer[BS];
      int level;

   public:
      Xml(QIODevice*);
      void header();
      void putLevel();
      void put(const char* format, ...);
      void nput(const char* format, ...);
      void tag(const char* format, ...);
      void etag(const char* format, ...);
      void tagE(const char* format, ...);
      void tdata(const QString&);
      void intTag(const char* const name, int val);
      void doubleTag(const char* const name, double val);
      void floatTag(const char* const name, float val);
      void strTag(const char* const name, const char* val);
      void strTag(const char* const name, const QString& s);
      void colorTag(const char* name, const QColor& color);
      void geometryTag(const char* name, const QWidget* g);
      void qrectTag(const char* name, const QRect& r);
      void writeProperties(const QObject*);
      };

extern QRect readGeometry(QDomNode);
extern void readProperties(QObject* o, QDomNode node);
}

#endif

