//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: splitter.cpp,v 1.1.1.1 2003/10/27 18:54:59 wschweer Exp $
//  (C) Copyright 1999 Werner Schweer (ws@seh.de)
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

#include "splitter.h"
#include "xml.h"

#include <QList>
//#include <QStringList>

namespace MusEGui {

//---------------------------------------------------------
//   Splitter
//---------------------------------------------------------

Splitter::Splitter(QWidget* parent, const char* name)
   : QSplitter(parent)
      {
      if(name)
        setObjectName(name);
      setOpaqueResize(true);
      }

Splitter::Splitter(Qt::Orientation o, QWidget* parent, const char* name)
   : QSplitter(o, parent)
      {
      if(name)
        setObjectName(name);
      setOpaqueResize(true);
      }

//---------------------------------------------------------
//   saveConfiguration
//---------------------------------------------------------

void Splitter::writeStatus(int level, MusECore::Xml& xml)
      {
      QList<int> vl = sizes();
      //xml.nput(level++, "<%s>", name());
      xml.nput(level++, "<%s>", MusECore::Xml::xmlString(objectName()).toUtf8().constData());
      QList<int>::iterator ivl = vl.begin();
      for (; ivl != vl.end(); ++ivl) {
            xml.nput("%d ", *ivl);
            }
      //xml.nput("</%s>\n", name());
      xml.nput("</%s>\n", MusECore::Xml::xmlString(objectName()).toUtf8().constData());
      }

//---------------------------------------------------------
//   loadConfiguration
//---------------------------------------------------------

void Splitter::readStatus(MusECore::Xml& xml)
      {
      QList<int> vl;

      for (;;) {
            MusECore::Xml::Token token = xml.parse();
            const QString& tag = xml.s1();
            switch (token) {
                  case MusECore::Xml::Error:
                  case MusECore::Xml::End:
                        return;
                  case MusECore::Xml::TagStart:
                        xml.unknown("Splitter");
                        break;
                  case MusECore::Xml::Text:
                        {
                        //QStringList sl = QStringList::split(' ', tag);
                        QStringList sl = tag.split(QString(" "), Qt::SkipEmptyParts);
                        for (QStringList::Iterator it = sl.begin(); it != sl.end(); ++it) {
                              int val = (*it).toInt();
                              vl.append(val);
                              }

                        // fix for allowing the arranger split to work nicely with old songs
                        // that have only two splitters, we add a first splitter with the
                        // standard strip width 53 pixels
                        if (objectName() == "split") {
                          if (vl.size() < 3) {
                            vl.prepend(53);
                          }
                        }
                        //
                        //
                        }
                        break;
                  case MusECore::Xml::TagEnd:
                        if (tag == objectName()) {
                              setSizes(vl);
                              return;
                              }
                  default:
                        break;
                  }
            }
      }

} // namespace MusEGui
