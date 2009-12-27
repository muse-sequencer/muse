//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: header.cpp,v 1.1.1.1 2003/10/27 18:55:05 wschweer Exp $
//  (C) Copyright 2000 Werner Schweer (ws@seh.de)
//=========================================================

#include "header.h"
#include "xml.h"
#include <qstringlist.h>

//---------------------------------------------------------
//   readStatus
//---------------------------------------------------------

void Header::readStatus(Xml& xml)
      {
      for (;;) {
            Xml::Token token = xml.parse();
            const QString& tag = xml.s1();
            switch (token) {
                  case Xml::Error:
                  case Xml::End:
                        return;
                  case Xml::Text:
                        {
                        QStringList l = QStringList::split(QString(" "), tag);
                        int index = count();
                        for (QStringList::Iterator it = l.begin(); it != l.end(); ++it) {
                              int section = (*it).toInt();
                              moveSection(section, index);
                              --index;
                              }
                        }
                        break;
                  case Xml::TagStart:
                        xml.unknown("Header");
                        break;
                  case Xml::TagEnd:
                        if (tag == name())
                              return;
                  default:
                        break;
                  }
            }
      }

//---------------------------------------------------------
//   writeStatus
//---------------------------------------------------------

void Header::writeStatus(int level, Xml& xml) const
      {
      xml.nput(level, "<%s> ", name());
      int n = count() - 1;
      for (int i = n; i >= 0; --i)
            xml.nput("%d ", mapToSection(i));
      xml.put("</%s>", name());
      }

