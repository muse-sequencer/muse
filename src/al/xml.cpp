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

#include "xml.h"
#include "al.h"

#include <QtXml/QDomElement>
#include <QMetaProperty>
#include <QMetaType>
#include <QWidget>
#include <QColor>
#include <QRect>

namespace AL {

//---------------------------------------------------------
//   Xml
//---------------------------------------------------------

Xml::Xml()
      {
      level = 0;
      }

Xml::Xml(QIODevice* device)
   : QTextStream(device)
      {
      //setCodec("utf8");
      setEncoding(QStringConverter::Utf8);
      level = 0;
      }

//---------------------------------------------------------
//   putLevel
//---------------------------------------------------------

void Xml::putLevel()
      {
      for (int i = 0; i < level*2; ++i)
            *this << ' ';
      }

//---------------------------------------------------------
//   header
//---------------------------------------------------------

void Xml::header()
      {
      *this << "<?xml version=\"1.0\" encoding=\"utf8\"?>" <<
      Qt::endl;
      }

//---------------------------------------------------------
//   put
//---------------------------------------------------------

void Xml::put(const QString& s)
      {
      putLevel();
    	*this << xmlString(s) <<
    	Qt::endl;
      }

//---------------------------------------------------------
//   stag
//    <mops attribute="value">
//---------------------------------------------------------

void Xml::stag(const QString& s)
      {
      putLevel();
      *this << '<' << s << '>' <<
      Qt::endl;
      ++level;
      }

//---------------------------------------------------------
//   etag
//---------------------------------------------------------

void Xml::etag(const char* s)
      {
      putLevel();
      *this << "</" << s << '>' <<
      Qt::endl;
      --level;
      }

//---------------------------------------------------------
//   tagE
//    <mops attribute="value"/>
//---------------------------------------------------------

void Xml::tagE(const QString& s)
      {
      putLevel();
      *this << '<' << s << "/>" <<
      Qt::endl;
      }

void Xml::tag(const char* name, int val)
      {
      putLevel();
      *this << '<' << name << '>' << val << "</" << name << '>' <<
      Qt::endl;
      }

void Xml::tag(const char* name, unsigned val)
      {
      putLevel();
      *this << '<' << name << '>' << val << "</" << name << '>' <<
      Qt::endl;
      }

void Xml::tag(const char* name, float val)
      {
      putLevel();
      *this << '<' << name << '>' << val << "</" << name << '>' <<
      Qt::endl;
      }

void Xml::tag(const char* name, const double& val)
      {
      putLevel();
      *this << '<' << name << '>' << val << "</" << name << '>' <<
      Qt::endl;
      }

void Xml::tag(const char* name, const QString& val)
      {
      putLevel();
      *this << "<" << name << ">" << xmlString(val) << "</" << name << '>' <<
      Qt::endl;
      }

void Xml::tag(const char* name, const QColor& color)
      {
      putLevel();
    	*this << QString("<%1 r=\"%2\" g=\"%3\" b=\"%4\"/>")
         .arg(name).arg(color.red()).arg(color.green()).arg(color.blue()) <<
         Qt::endl;
      }

void Xml::tag(const char* name, const QWidget* g)
      {
      tag(name, QRect(g->pos(), g->size()));
      }

void Xml::tag(const char* name, const QRect& r)
      {
      putLevel();
   	*this << "<" << name;
      *this << QString(" x=\"%1\" y=\"%2\" w=\"%3\" h=\"%4\"/>")
         .arg(r.x()).arg(r.y()).arg(r.width()).arg(r.height()) <<
         Qt::endl;
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

//---------------------------------------------------------
//   readGeometry
//---------------------------------------------------------

QRect readGeometry(QDomNode node)
      {
      QDomElement e = node.toElement();
      int x = e.attribute("x","0").toInt();
      int y = e.attribute("y","0").toInt();
      int w = e.attribute("w","50").toInt();
      int h = e.attribute("h","50").toInt();
      return QRect(x, y, w, h);
      }

//---------------------------------------------------------
//   writeProperties
//---------------------------------------------------------

void Xml::writeProperties(const QObject* o)
      {
      const QMetaObject* meta = o->metaObject();

  	//
      // start from dummy "muse" property, assuming this is the
      // first muse property in widget hierarchy
      //
  	int from = meta->indexOfProperty("muse") + 1;
     	int n    = meta->propertyCount();
      for (int i = from; i < n; ++i) {
            QMetaProperty p  = meta->property(i);
          	if (!p.isScriptable())
            	continue;
            const char* name = p.name();
            QVariant v       = p.read(o);
            switch(v.typeId()) {
            	case QMetaType::Bool:
            	case QMetaType::Int:
                  	tag(name, v.toInt());
                        break;
                  case QMetaType::Double:
                  	tag(name, v.toDouble());
                        break;
                  case QMetaType::QString:
                        tag(name, v.toString());
                        break;
                  case QMetaType::QRect:
             		tag(name, v.toRect());
                        break;
                  case QMetaType::QPoint:
                        {
                    	QPoint p = v.toPoint();
      			putLevel();
   				*this << "<" << name << QString(" x=\"%1\" y=\"%2\" />")
         			   .arg(p.x()).arg(p.y()) <<
         			   Qt::endl;
                        }
                        break;

                  default:
                        printf("MusE:%s type %d not implemented\n",
                           meta->className(), v.typeId());
                        break;
               	}
            }
      }

//---------------------------------------------------------
//   readProperties
//---------------------------------------------------------

void readProperties(QObject* o, QDomNode node)
      {
      const QMetaObject* meta = o->metaObject();

      QDomElement e = node.toElement();
      QString tag(e.tagName());
      int idx = meta->indexOfProperty(tag.toLatin1().constData());
      if (idx == -1) {
            printf("MusE:%s: unknown tag %s\n",
               meta->className(), tag.toLocal8Bit().constData());
            return;
            }
      QMetaProperty p = meta->property(idx);
      QVariant v;
      switch(p.typeId()) {
            case QMetaType::Int:
            case QMetaType::Bool:
                  v.setValue(e.text().toInt());
                  break;
            case QMetaType::Double:
                  v.setValue(e.text().toDouble());
                  break;
            case QMetaType::QString:
                  v.setValue(e.text());
                  break;
            case QMetaType::QRect:
                  v.setValue(AL::readGeometry(node));
                  break;
            case QMetaType::QPoint:
                  {
			int x = e.attribute("x","0").toInt();
			int y = e.attribute("y","0").toInt();
			v.setValue(QPoint(x, y));
             	}
                  break;
            default:
                  printf("MusE:%s type %d not implemented\n",
                     meta->className(), p.typeId());
                  return;
            }
      if (p.isWritable())
            p.write(o, v);
      }

//---------------------------------------------------------
//   dump
//---------------------------------------------------------

void Xml::dump(int len, const unsigned char* p)
      {
      putLevel();
      int col = 0;
      setFieldWidth(5);
      setNumberFlags(numberFlags() | QTextStream::ShowBase);
      setIntegerBase(16);
      for (int i = 0; i < len; ++i, ++col) {
            if (col >= 16) {
                  setFieldWidth(0);
                  *this << Qt::endl;
                  col = 0;
                  putLevel();
                  setFieldWidth(5);
                  }
            *this << (p[i] & 0xff);
            }
      if (col)
            *this << Qt::endl << Qt::dec;
      setFieldWidth(0);
      setIntegerBase(10);
      }

//---------------------------------------------------------
//   domError
//---------------------------------------------------------

void domError(QDomNode node)
      {
      QDomElement e = node.toElement();
      QString tag(e.tagName());
      QString s;
      QDomNode dn(node);
      while (!dn.parentNode().isNull()) {
            dn = dn.parentNode();
            const QDomElement e = dn.toElement();
            const QString k(e.tagName());
            if (!s.isEmpty())
                  s += ":";
            s += k;
            }
      fprintf(stderr, "%s: Unknown Node <%s>, type %d\n",
         s.toLocal8Bit().constData(), tag.toLocal8Bit().constData(), node.nodeType());
      if (node.isText()) {
            fprintf(stderr, "  text node <%s>\n", node.toText().data().toLocal8Bit().constData());
            }
      }

//---------------------------------------------------------
//   domNotImplemented
//---------------------------------------------------------

void domNotImplemented(QDomNode node)
      {
      if (!AL::debugMsg)
            return;
      QDomElement e = node.toElement();
      QString tag(e.tagName());
      QString s;
      QDomNode dn(node);
      while (!dn.parentNode().isNull()) {
            dn = dn.parentNode();
            const QDomElement e = dn.toElement();
            const QString k(e.tagName());
            if (!s.isEmpty())
                  s += ":";
            s += k;
            }
      fprintf(stderr, "%s: Node not implemented: <%s>, type %d\n",
         s.toLocal8Bit().constData(), tag.toLocal8Bit().constData(), node.nodeType());
      if (node.isText()) {
            fprintf(stderr, "  text node <%s>\n", node.toText().data().toLocal8Bit().constData());
            }
      }
}

