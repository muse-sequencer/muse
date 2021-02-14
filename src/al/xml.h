//=============================================================================
//  AL
//  Audio Utility Library
//  $Id:$
//
//  Copyright (C) 1999-2011 by Werner Schweer and others
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License
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
//=============================================================================

#ifndef __XML_H__
#define __XML_H__

#include <QTextStream>

class QColor;
class QDomNode;
class QRect;
class QWidget;

namespace AL {

//---------------------------------------------------------
//   Xml
//---------------------------------------------------------

class Xml : public QTextStream {
      int level;

   public:
      Xml();
      Xml(QIODevice*);

      void header();
      void putLevel();

      void put(const QString&);

      void stag(const QString&);
      void etag(const char*);

      void tagE(const QString&);

      void tag(const char* name, int);
      void tag(const char* name, unsigned);
      void tag(const char* name, const double& val);
      void tag(const char* name, float val);
      void tag(const char* name, const QString& s);
      void tag(const char* name, const QColor& color);
      void tag(const char* name, const QWidget* g);
      void tag(const char* name, const QRect& r);

      void dump(int n, const unsigned char*);
      void writeProperties(const QObject*);

      static QString xmlString(const QString&);
      };

extern QRect readGeometry(QDomNode);
extern void readProperties(QObject* o, QDomNode node);
extern void domError(QDomNode node);
extern void domNotImplemented(QDomNode node);
}

#endif

