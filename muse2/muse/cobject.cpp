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
                        else if (tag == "toolbars") {
                              if (!restoreState(QByteArray::fromHex(xml.parse1().toAscii())))
                                    fprintf(stderr,"ERROR: couldn't restore toolbars. however, this is not really a problem.\n");
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
      
      xml.strTag(level, "toolbars", saveState().toHex().data());

      xml.tag(level, "/topwin");
      }

TopWin::TopWin(QWidget* parent, const char* name,
   Qt::WindowFlags f) : QMdiSubWindow(parent, f)
      {
      setObjectName(QString(name));
      //setAttribute(Qt::WA_DeleteOnClose);
      // Allow multiple rows.  Tim.
      //setDockNestingEnabled(true);
      //setIconSize(ICON_SIZE); FINDMICH
      
      menu_bar=0;
      }

void TopWin::setCentralWidget(QWidget* w)
{
  setWidget(w);
}

QMenuBar* TopWin::menuBar()
{
  if (!menu_bar)
  {
    menu_bar=new QMenuBar(this);
    menu_bar->hide();
  }
  return menu_bar;
}

QToolBar* TopWin::addToolBar(QString str)
{
  QToolBar* temp = new QToolBar();
  temp->setWindowTitle(str);
  
  addToolBar(temp);
  
  return temp;
}

void TopWin::addToolBar(QToolBar* toolbar)
{
  printf("adding toolbar %p. ",toolbar);
  toolbar->hide();
  toolbars.push_back(toolbar);
  printf("toolbars.size()=%i\n",toolbars.size());
}

void TopWin::addToolBarBreak()
{

}

QMenuBar* TopWin::getMenuBar()
{
  return menu_bar;
}

std::list<QToolBar*> TopWin::getToolbars()
{
  return toolbars;
}

bool TopWin::restoreState(QByteArray) { return true; }
QByteArray TopWin::saveState(int) const { return QByteArray(); }
