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
#include "globals.h"
#include "app.h"

#include <QMdiSubWindow>
#include <QToolBar>
#include <QMenuBar>
#include <QAction>

using std::list;

TopWin::TopWin(QWidget* parent, const char* name, Qt::WindowFlags f)
                                               : QMainWindow(parent, f)
      {
      setObjectName(QString(name));
      //setAttribute(Qt::WA_DeleteOnClose);
      // Allow multiple rows.  Tim.
      //setDockNestingEnabled(true);
      setIconSize(ICON_SIZE);
      
      mdisubwin=NULL;
      _sharesToolsAndMenu=false;
      
      subwinAction=new QAction(tr("As subwindow"), this);
      subwinAction->setCheckable(true);
      subwinAction->setChecked(isMdiWin());
      connect(subwinAction, SIGNAL(toggled(bool)), SLOT(setIsMdiWin(bool)));
      }



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

void TopWin::hide()
{
  if (mdisubwin)
    mdisubwin->close();
  
  QMainWindow::hide();
}

void TopWin::show()
{
  if (mdisubwin)
    mdisubwin->show();
  
  QMainWindow::show();
}

void TopWin::setVisible(bool param)
{
  if (mdisubwin)
  {
    if (param)
      mdisubwin->show();
    else
      mdisubwin->close();
  }
  QMainWindow::setVisible(param);
}

QMdiSubWindow* TopWin::createMdiWrapper()
{
  if (mdisubwin==NULL)
  {
    mdisubwin = new QMdiSubWindow();
    mdisubwin->setWidget(this);
  }
  
  return mdisubwin;
}

void TopWin::setIsMdiWin(bool val)
{
  if (val)
  {
    if (!isMdiWin())
    {
      bool vis=isVisible();
      QMdiSubWindow* subwin = createMdiWrapper();
      muse->addMdiSubWindow(subwin);
      subwin->setVisible(vis);
      
      subwinAction->setChecked(true);
    }
    else
    {
      if (debugMsg) printf("TopWin::setIsMdiWin(true) called, but window is already a MDI win\n");
    }
  }
  else
  {
    if (isMdiWin())
    {
      bool vis=isVisible();
      QMdiSubWindow* mdisubwin_temp=mdisubwin;
      mdisubwin=NULL;
      setParent(NULL);
      mdisubwin_temp->hide();
      //TODO FINDMICH evtl noch ein signal emitten oder sowas?
      delete mdisubwin_temp;
      
      printf("unMDIfied, visible is %i\n",vis);
      setVisible(vis);
      
      subwinAction->setChecked(false);
    }
    else
    {
      if (debugMsg) printf("TopWin::setIsMdiWin(false) called, but window is not a MDI win\n");
    }
  }
}

bool TopWin::isMdiWin()
{
  return (mdisubwin!=NULL);
}

void TopWin::insertToolBar(QToolBar*, QToolBar*) { printf("ERROR: THIS SHOULD NEVER HAPPEN: TopWin::insertToolBar called, but it's not implemented! ignoring it\n"); }
void TopWin::insertToolBarBreak(QToolBar*) { printf("ERROR: THIS SHOULD NEVER HAPPEN: TopWin::insertToolBarBreak called, but it's not implemented! ignoring it\n"); }
void TopWin::removeToolBar(QToolBar*) { printf("ERROR: THIS SHOULD NEVER HAPPEN: TopWin::removeToolBar called, but it's not implemented! ignoring it\n"); }
void TopWin::removeToolBarBreak(QToolBar*) { printf("ERROR: THIS SHOULD NEVER HAPPEN: TopWin::removeToolBarBreak called, but it's not implemented! ignoring it\n"); }
void TopWin::addToolBar(Qt::ToolBarArea, QToolBar* tb) { printf("ERROR: THIS SHOULD NEVER HAPPEN: TopWin::addToolBar(Qt::ToolBarArea, QToolBar*) called, but it's not implemented!\nusing addToolBar(QToolBar*) instead\n"); addToolBar(tb);}

void TopWin::addToolBar(QToolBar* toolbar)
{
  _toolbars.push_back(toolbar);
  
  if (!_sharesToolsAndMenu)
    QMainWindow::addToolBar(toolbar);
}

QToolBar* TopWin::addToolBar(const QString& title)
{
  QToolBar* toolbar = new QToolBar(title, this);
  addToolBar(toolbar);
  return toolbar;
}


void TopWin::shareToolsAndMenu(bool val)
{
  _sharesToolsAndMenu = val;
  
  if (!val)
  {
    for (list<QToolBar*>::iterator it=_toolbars.begin(); it!=_toolbars.end(); it++)
      if (*it != NULL)
        QMainWindow::addToolBar(*it);
      else
        QMainWindow::addToolBarBreak();
    
    menuBar()->show();
  }
  else
  {
    for (list<QToolBar*>::iterator it=_toolbars.begin(); it!=_toolbars.end(); it++)
      if (*it != NULL)
        QMainWindow::removeToolBar(*it); // this does NOT delete the toolbar, which is good
    
    menuBar()->hide();
  }
  
  emit toolsAndMenuSharingChanged(val);
}

