//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: cobject.cpp,v 1.4 2004/02/02 12:10:09 wschweer Exp $
//
//  (C) Copyright 1999/2000 Werner Schweer (ws@seh.de)
//=========================================================

#include "cobject.h"
#include "xml.h"
#include "gui.h"

//---------------------------------------------------------
//   readStatus
//---------------------------------------------------------

void TopWin::readStatus(Xml& xml)
      {
      for (;;) {
            Xml::Token token = xml.parse();
            if (token == Xml::Error || token == Xml::End)
                  break;
            QString tag = xml.s1();
            switch (token) {
                  case Xml::TagStart:
                        if (tag == "geometry") {
                              QRect r(readGeometry(xml, tag));
                              resize(r.size());
                              move(r.topLeft());
                              }
                        else
                              xml.unknown("TopWin");
                        break;
                  case Xml::TagEnd:
                        if (tag == "topwin")
                              return;
                  default:
                        break;
                  }
            }
      }

//---------------------------------------------------------
//   writeStatus
//---------------------------------------------------------

void TopWin::writeStatus(int level, Xml& xml) const
      {
      xml.tag(level++, "topwin");
      xml.tag(level++, "geometry x=\"%d\" y=\"%d\" w=\"%d\" h=\"%d\"",
            geometry().x(),
            geometry().y(),
            geometry().width(),
            geometry().height());
      xml.tag(level--, "/geometry");
      xml.tag(level, "/topwin");
      }

TopWin::TopWin(QWidget* parent, const char* name,
   Qt::WindowFlags f) : QMainWindow(parent, f)
      {
      setObjectName(QString(name));
      //setAttribute(Qt::WA_DeleteOnClose);
      // Allow multiple rows.  Tim.
      //setDockNestingEnabled(true);
      setIconSize(ICON_SIZE);
      }

