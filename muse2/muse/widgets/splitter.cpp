//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: splitter.cpp,v 1.1.1.1 2003/10/27 18:54:59 wschweer Exp $
//  (C) Copyright 1999 Werner Schweer (ws@seh.de)
//=========================================================

#include "splitter.h"
#include "xml.h"
#include <qstringlist.h>
//Added by qt3to4:
#include <Q3ValueList>

//---------------------------------------------------------
//   Splitter
//---------------------------------------------------------

Splitter::Splitter(Qt::Orientation o, QWidget* parent, const char* name)
   : QSplitter(o, parent, name)
      {
      setOpaqueResize(true);
      }

//---------------------------------------------------------
//   saveConfiguration
//---------------------------------------------------------

void Splitter::writeStatus(int level, Xml& xml)
      {
      Q3ValueList<int> vl = sizes();
      //xml.nput(level++, "<%s>", name());
      xml.nput(level++, "<%s>", Xml::xmlString(name()).latin1());
      Q3ValueListIterator<int> ivl = vl.begin();
      for (; ivl != vl.end(); ++ivl) {
            xml.nput("%d ", *ivl);
            }
      //xml.nput("</%s>\n", name());
      xml.nput("</%s>\n", Xml::xmlString(name()).latin1());
      }

//---------------------------------------------------------
//   loadConfiguration
//---------------------------------------------------------

void Splitter::readStatus(Xml& xml)
      {
      Q3ValueList<int> vl;

      for (;;) {
            Xml::Token token = xml.parse();
            const QString& tag = xml.s1();
            switch (token) {
                  case Xml::Error:
                  case Xml::End:
                        return;
                  case Xml::TagStart:
                        xml.unknown("Splitter");
                        break;
                  case Xml::Text:
                        {
                        QStringList sl = QStringList::split(' ', tag);
                        for (QStringList::Iterator it = sl.begin(); it != sl.end(); ++it) {
                              int val = (*it).toInt();
                              vl.append(val);
                              }
                        }
                        break;
                  case Xml::TagEnd:
                        if (tag == name()) {
                              setSizes(vl);
                              return;
                              }
                  default:
                        break;
                  }
            }
      }
