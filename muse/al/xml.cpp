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

#include "xml.h"


namespace AL {

//---------------------------------------------------------
//   Xml
//---------------------------------------------------------

Xml::Xml(QIODevice* device)
   : QTextStream(device)
      {
      level = 0;
      }

//---------------------------------------------------------
//   header
//---------------------------------------------------------

void Xml::header()
      {
      *this << "<?xml version=\"1.0\"?>\n";
      }

//---------------------------------------------------------
//   put
//---------------------------------------------------------

void Xml::put(const char* format, ...)
      {
      va_list args;
      va_start(args, format);
      putLevel();
      vsnprintf(buffer, BS, format, args);
      va_end(args);
    	*this << buffer;
      *this << '\n';
      }

//---------------------------------------------------------
//   nput
//---------------------------------------------------------

void Xml::nput(const char* format, ...)
      {
      va_list args;
      va_start(args, format);
      vsnprintf(buffer, BS, format, args);
    	*this << buffer;
      va_end(args);
      }

//---------------------------------------------------------
//   tdata
//---------------------------------------------------------

void Xml::tdata(const QString& s)
      {
      putLevel();
      *this << s << endl;
      }

//---------------------------------------------------------
//   tag
//---------------------------------------------------------

void Xml::tag(const char* format, ...)
      {
      va_list args;
      va_start(args, format);
      putLevel();
      *this << '<';
      vsnprintf(buffer, BS, format, args);
    	*this << buffer;
      va_end(args);
      *this << '>' << endl;
      ++level;
      }

void Xml::tagE(const char* format, ...)
      {
      va_list args;
      va_start(args, format);
      putLevel();
      *this << '<';
      vsnprintf(buffer, BS, format, args);
    	*this << buffer;
      va_end(args);
      *this << "/>" << endl;
      }

//---------------------------------------------------------
//   etag
//---------------------------------------------------------

void Xml::etag(const char* format, ...)
      {
      va_list args;
      va_start(args, format);
      putLevel();
      *this << "</";
      vsnprintf(buffer, BS, format, args);
    	*this << buffer;
      va_end(args);
      *this << '>' << endl;
      --level;
      }

void Xml::putLevel()
      {
      for (int i = 0; i < level*2; ++i)
            *this << ' ';
      }

void Xml::intTag(const char* name, int val)
      {
      putLevel();
      *this << "<" << name << ">" << val << "</" << name << ">\n";
      }

void Xml::floatTag(const char* name, float val)
      {
      putLevel();
      *this << QString("<%1>%2</%3>\n").arg(name).arg(val).arg(name);
      }

void Xml::doubleTag(const char* name, double val)
      {
      putLevel();
      QString s("<%1>%2</%3>\n");
      *this << s.arg(name).arg(val).arg(name);
      }

void Xml::strTag(const char* name, const char* val)
      {
      putLevel();
      *this << "<" << name << ">";
      if (val) {
            while (*val) {
                  switch(*val) {
                        case '&':
                              *this << "&amp;";
                              break;
                        case '<':
                              *this << "&lt;";
                              break;
                        case '>':
                              *this << "&gt;";
                              break;
                        case '"':
                              *this << "&quot;";
                              break;
                        case '\'':
                              *this << "&apos;";
                              break;
                        default:
                              *this << *val;
                              break;
                        }
                  ++val;
                  }
            }
      *this << "</" << name << ">\n";
      }

//---------------------------------------------------------
//   colorTag
//---------------------------------------------------------

void Xml::colorTag(const char* name, const QColor& color)
      {
      putLevel();
      snprintf(buffer, BS, "<%s r=\"%d\" g=\"%d\" b=\"%d\" />\n",
	    name, color.red(), color.green(), color.blue());
    	*this << buffer;
      }

//---------------------------------------------------------
//   geometryTag
//---------------------------------------------------------

void Xml::geometryTag(const char* name, const QWidget* g)
      {
      qrectTag(name, QRect(g->pos(), g->size()));
      }

//---------------------------------------------------------
//   qrectTag
//---------------------------------------------------------

void Xml::qrectTag(const char* name, const QRect& r)
      {
      putLevel();
   	*this << "<" << name;
      snprintf(buffer, BS, " x=\"%d\" y=\"%d\" w=\"%d\" h=\"%d\" />\n",
         r.x(), r.y(), r.width(), r.height());
    	*this << buffer;
      }

//---------------------------------------------------------
//   strTag
//---------------------------------------------------------

void Xml::strTag(const char* name, const QString& val)
      {
      strTag(name, val.toLatin1().data());
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
      // first muse propertie in widget hierarchy
      //
  	int from = meta->indexOfProperty("muse") + 1;
     	int n    = meta->propertyCount();
      for (int i = from; i < n; ++i) {
            QMetaProperty p  = meta->property(i);
          	if (!p.isScriptable())
            	continue;
            const char* name = p.name();
            QVariant v       = p.read(o);
            switch(v.type()) {
            	case QVariant::Bool:
            	case QVariant::Int:
                  	intTag(name, v.toInt());
                        break;
                  case QVariant::Double:
                  	doubleTag(name, v.toDouble());
                        break;
                  case QVariant::String:
                        strTag(name, v.toString());
                        break;
                  case QVariant::Rect:
             		qrectTag(name, v.toRect());
                        break;
                  case QVariant::Point:
                        {
                    	QPoint p = v.toPoint();
      			putLevel();
   				*this << "<" << name;
      			snprintf(buffer, BS, " x=\"%d\" y=\"%d\" />\n",
         			   p.x(), p.y());
    				*this << buffer;
                        }
                        break;

                  default:
                        printf("MusE:%s type %d not implemented\n",
                           meta->className(), v.type());
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
      int idx = meta->indexOfProperty(tag.toLatin1().data());
      if (idx == -1) {
            printf("MusE:%s: unknown tag %s\n",
               meta->className(), tag.toLatin1().data());
            return; 
            }
      QMetaProperty p = meta->property(idx);
      QVariant v;
      switch(p.type()) {
            case QVariant::Int:
            case QVariant::Bool:
                  v.setValue(e.text().toInt());
                  break;
            case QVariant::Double:
                  v.setValue(e.text().toDouble());
                  break;
            case QVariant::String:
                  v.setValue(e.text());
                  break;
            case QVariant::Rect:
                  v.setValue(AL::readGeometry(node));
                  break;
            case QVariant::Point:
                  {
			int x = e.attribute("x","0").toInt();
			int y = e.attribute("y","0").toInt();
			v.setValue(QPoint(x, y));
             	}
                  break;
            default:
                  printf("MusE:%s type %d not implemented\n",
                     meta->className(), p.type());
                  return;
            }
      if (p.isWritable())
            p.write(o, v);
      }
}

