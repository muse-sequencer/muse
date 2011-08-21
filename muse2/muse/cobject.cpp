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

int TopWin::_widthInit[TOPLEVELTYPE_LAST_ENTRY];
int TopWin::_heightInit[TOPLEVELTYPE_LAST_ENTRY];
QByteArray TopWin::_toolbarSharedInit[TOPLEVELTYPE_LAST_ENTRY];
QByteArray TopWin::_toolbarNonsharedInit[TOPLEVELTYPE_LAST_ENTRY];
bool TopWin::_sharesWhenFree[TOPLEVELTYPE_LAST_ENTRY];
bool TopWin::_sharesWhenSubwin[TOPLEVELTYPE_LAST_ENTRY];
bool TopWin::_defaultSubwin[TOPLEVELTYPE_LAST_ENTRY];
bool TopWin::initInited=false;

TopWin::TopWin(ToplevelType t, QWidget* parent, const char* name, Qt::WindowFlags f)
                                               : QMainWindow(parent, f)
      {
      if (initInited==false)
        initConfiguration();
      
      initalizing=true;
      
      _type=t;
      
      
      
      setObjectName(QString(name));
      // Allow multiple rows.  Tim.
      //setDockNestingEnabled(true);
      setIconSize(ICON_SIZE);
      
      subwinAction=new QAction(tr("As subwindow"), this);
      subwinAction->setCheckable(true);
      connect(subwinAction, SIGNAL(toggled(bool)), SLOT(setIsMdiWin(bool)));

      shareAction=new QAction(tr("Shares tools and menu"), this);
      shareAction->setCheckable(true);
      connect(shareAction, SIGNAL(toggled(bool)), SLOT(shareToolsAndMenu(bool)));

      mdisubwin=NULL;
      _sharesToolsAndMenu=_defaultSubwin[_type] ? _sharesWhenSubwin[_type] : _sharesWhenFree[_type];
      if (_defaultSubwin[_type])
        setIsMdiWin(true);
      
      
      subwinAction->setChecked(isMdiWin());
      shareAction->setChecked(_sharesToolsAndMenu);
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
                        if (tag == "geometry_state") {
                              if (!restoreGeometry(QByteArray::fromHex(xml.parse1().toAscii())))
                                    fprintf(stderr,"ERROR: couldn't restore geometry. however, this is probably not really a problem.\n");
                              }
                        else if (tag == "toolbars") {
                              if (!restoreState(QByteArray::fromHex(xml.parse1().toAscii())))
                                    fprintf(stderr,"ERROR: couldn't restore toolbars. however, this is not really a problem.\n");
                              }
                        else if (tag == "shares_menu") {
                              shareToolsAndMenu(xml.parseInt());
                              }
                        else if (tag == "is_subwin") {
                              setIsMdiWin(xml.parseInt());
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
      
      // the order of these tags has a certain sense
      // changing it won't break muse, but it may break proper
      // restoring of the positions
      xml.intTag(level, "is_subwin", isMdiWin());
      xml.strTag(level, "geometry_state", saveGeometry().toHex().data());
      xml.intTag(level, "shares_menu", sharesToolsAndMenu());
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
      
      if (_sharesToolsAndMenu == _sharesWhenFree[_type])
        shareToolsAndMenu(_sharesWhenSubwin[_type]);
      
      subwinAction->setChecked(true);
      muse->updateWindowMenu();
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
      delete mdisubwin_temp;
      
      setVisible(vis);

      if (_sharesToolsAndMenu == _sharesWhenSubwin[_type])
        shareToolsAndMenu(_sharesWhenFree[_type]);
            
      subwinAction->setChecked(false);
      muse->updateWindowMenu();
    }
    else
    {
      if (debugMsg) printf("TopWin::setIsMdiWin(false) called, but window is not a MDI win\n");
    }
  }
}

bool TopWin::isMdiWin() const
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
    muse->shareMenuAndToolbarChanged(this, false);
    
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
      {
        QMainWindow::removeToolBar(*it); // this does NOT delete the toolbar, which is good
        (*it)->setParent(NULL);
      }
    
    menuBar()->hide();
    
    muse->shareMenuAndToolbarChanged(this, true);
  }
  
  shareAction->setChecked(val);
}



//---------------------------------------------------------
//   storeInitialState
//---------------------------------------------------------

void TopWin::storeInitialState() const
      {
        if (initalizing)
          printf("THIS SHOULD NEVER HAPPEN: STORE INIT STATE CALLED WHILE INITING! please IMMEDIATELY report that to flo!\n");

        _widthInit[_type] = width();
        _heightInit[_type] = height();
        if (sharesToolsAndMenu())
        {
          if (muse->getCurrentMenuSharingTopwin() == this)
            _toolbarSharedInit[_type] = muse->saveState();
        }
        else
          _toolbarNonsharedInit[_type] = saveState();
      }



//initConfiguration() restores default "traditional muse" configuration
void TopWin::initConfiguration()
{
  if (initInited==false)
  {
    for (int i=0;i<TOPLEVELTYPE_LAST_ENTRY;i++)
    {
      _widthInit[i]=800;
      _heightInit[i]=600;
      _sharesWhenFree[i]=false;
      _sharesWhenSubwin[i]=true;
      _defaultSubwin[i]=false;
    }
    
    _defaultSubwin[ARRANGER]=true;
    
    initInited=true;
  }    
}

//---------------------------------------------------------
//   readConfiguration
//---------------------------------------------------------

void TopWin::readConfiguration(ToplevelType t, Xml& xml)
      {
      if (initInited==false)
        initConfiguration();
      
      for (;;) {
            Xml::Token token = xml.parse();
            if (token == Xml::Error || token == Xml::End)
                  break;
            const QString& tag = xml.s1();
            switch (token) {
                  case Xml::TagStart:
                        if (tag == "width")
                              _widthInit[t] = xml.parseInt();
                        else if (tag == "height")
                              _heightInit[t] = xml.parseInt();
                        else if (tag == "nonshared_toolbars")
                              _toolbarNonsharedInit[t] = QByteArray::fromHex(xml.parse1().toAscii());
                        else if (tag == "shared_toolbars")
                              _toolbarSharedInit[t] = QByteArray::fromHex(xml.parse1().toAscii());
                        else if (tag == "shares_when_free")
                              _sharesWhenFree[t] = xml.parseInt();
                        else if (tag == "shares_when_subwin")
                              _sharesWhenSubwin[t] = xml.parseInt();
                        else if (tag == "default_subwin")
                              _defaultSubwin[t] = xml.parseInt();
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
//   writeConfiguration
//---------------------------------------------------------

void TopWin::writeConfiguration(ToplevelType t, int level, Xml& xml)
      {
      if (!initInited)
      {
        printf ("WARNING: TopWin::writeConfiguration() called although the config hasn't been\n"
                "         initalized! writing default configuration\n");
        initConfiguration();
      }
      xml.tag(level++, "topwin");
      xml.intTag(level, "width", _widthInit[t]);
      xml.intTag(level, "height", _heightInit[t]);
      xml.strTag(level, "nonshared_toolbars", _toolbarNonsharedInit[t].toHex().data());
      xml.strTag(level, "shared_toolbars", _toolbarSharedInit[t].toHex().data());
      xml.intTag(level, "shares_when_free", _sharesWhenFree[t]);
      xml.intTag(level, "shares_when_subwin", _sharesWhenSubwin[t]);
      xml.intTag(level, "default_subwin", _defaultSubwin[t]);
      xml.etag(level, "topwin");
      }

void TopWin::initTopwinState()
{
  if (sharesToolsAndMenu())
  {
    if (this == muse->getCurrentMenuSharingTopwin())
      muse->restoreState(_toolbarSharedInit[_type]);
  }
  else
    restoreState(_toolbarNonsharedInit[_type]);
}

void TopWin::restoreMainwinState()
{
  if (sharesToolsAndMenu())
    initTopwinState();
}

QString TopWin::typeName(ToplevelType t)
{
  switch (t)
  {
    case PIANO_ROLL: return tr("Piano roll");
    case LISTE: return tr("List editor");
    case DRUM: return tr("Drum editor");
    case MASTER: return tr("Master track editor");
    case LMASTER: return tr("Master track list editor");
    case WAVE: return tr("Wave editor");
    case CLIPLIST: return tr("Clip list");
    case MARKER: return tr("Marker view");
    case SCORE: return tr("Score editor");
    case ARRANGER: return tr("Arranger");
    default: return tr("<unknown toplevel type>");
  }
}
