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

Xml::Xml()
      {
      level = 0;      
      }

Xml::Xml(QIODevice* device)
   : QTextStream(device)
      {
      setCodec("utf8");
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
      *this << "<?xml version=\"1.0\" encoding=\"utf8\"?>\n";
      }

//---------------------------------------------------------
//   put
//---------------------------------------------------------

void Xml::put(const char* format, ...)
      {
      va_list args;
      va_start(args, format);
      putLevel();
      char buffer[BS];
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
      char buffer[BS];
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
//   stag
//    <mops attribute="value">
//---------------------------------------------------------

void Xml::stag(const char* format, ...)
      {
      va_list args;
      va_start(args, format);
      putLevel();
      *this << '<';
      char buffer[BS];
      vsnprintf(buffer, BS, format, args);
    	*this << buffer;
      va_end(args);
      *this << '>' << endl;
      ++level;
      }

//---------------------------------------------------------
//   etag
//---------------------------------------------------------

void Xml::etag(const char* s)
      {
      putLevel();
      *this << "</" << s << '>' << endl;
      --level;
      }

//---------------------------------------------------------
//   tagE
//    <mops attribute="value"/>
//---------------------------------------------------------

void Xml::tagE(const char* format, ...)
      {
      va_list args;
      va_start(args, format);
      putLevel();
      *this << '<';
      char buffer[BS];
      vsnprintf(buffer, BS, format, args);
    	*this << buffer;
      va_end(args);
      *this << "/>" << endl;
      }

void Xml::tag(const char* name, int val)
      {
      putLevel();
      *this << "<" << name << ">" << val << "</" << name << ">\n";
      }

void Xml::tag(const char* name, unsigned val)
      {
      putLevel();
      *this << "<" << name << ">" << val << "</" << name << ">\n";
      }

void Xml::tag(const char* name, float val)
      {
      putLevel();
      *this << QString("<%1>%2</%3>\n").arg(name).arg(val).arg(name);
      }

void Xml::tag(const char* name, const double& val)
      {
      putLevel();
      QString s("<%1>%2</%3>\n");
      *this << s.arg(name).arg(val).arg(name);
      }

void Xml::tag(const char* name, const char* s)
      {
      tag(name, QString(s));
      }

//---------------------------------------------------------
//   colorTag
//---------------------------------------------------------

void Xml::tag(const char* name, const QColor& color)
      {
      putLevel();
      char buffer[BS];
      snprintf(buffer, BS, "<%s r=\"%d\" g=\"%d\" b=\"%d\" />\n",
	    name, color.red(), color.green(), color.blue());
    	*this << buffer;
      }

//---------------------------------------------------------
//   geometryTag
//---------------------------------------------------------

void Xml::tag(const char* name, const QWidget* g)
      {
      tag(name, QRect(g->pos(), g->size()));
      }

//---------------------------------------------------------
//   qrectTag
//---------------------------------------------------------

void Xml::tag(const char* name, const QRect& r)
      {
      putLevel();
   	*this << "<" << name;
      char buffer[BS];
      snprintf(buffer, BS, " x=\"%d\" y=\"%d\" w=\"%d\" h=\"%d\" />\n",
         r.x(), r.y(), r.width(), r.height());
    	*this << buffer;
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
//   strTag
//---------------------------------------------------------

void Xml::tag(const char* name, const QString& val)
      {
      putLevel();
      *this << "<" << name << ">";
      *this << xmlString(val) << "</" << name << ">\n";
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
                  	tag(name, v.toInt());
                        break;
                  case QVariant::Double:
                  	tag(name, v.toDouble());
                        break;
                  case QVariant::String:
                        tag(name, v.toString());
                        break;
                  case QVariant::Rect:
             		tag(name, v.toRect());
                        break;
                  case QVariant::Point:
                        {
                    	QPoint p = v.toPoint();
      			putLevel();
   				*this << "<" << name;
                        char buffer[BS];
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

