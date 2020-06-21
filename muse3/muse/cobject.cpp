//=========================================================
//	MusE
//	Linux Music Editor
//	$Id: cobject.cpp,v 1.4 2004/02/02 12:10:09 wschweer Exp $
//
//	(C) Copyright 1999/2000 Werner Schweer (ws@seh.de)
//
//	This program is free software; you can redistribute it and/or
//	modify it under the terms of the GNU General Public License
//	as published by the Free Software Foundation; version 2 of
//	the License, or (at your option) any later version.
//
//	This program is distributed in the hope that it will be useful,
//	but WITHOUT ANY WARRANTY; without even the implied warranty of
//	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	See the
//	GNU General Public License for more details.
//
//	You should have received a copy of the GNU General Public License
//	along with this program; if not, write to the Free Software
//	Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA	02110-1301, USA.
//
//=========================================================

#include "cobject.h"
#include "xml.h"
#include "gui.h"
#include "globals.h"
#include "app.h"
#include "shortcuts.h"
#include "songpos_toolbar.h"
#include "sig_tempo_toolbar.h"
#include "gconfig.h"
#include "helper.h"
#include "song.h"

#include <QMdiSubWindow>
#include <QToolBar>
#include <QMenuBar>
#include <QAction>
#include <QWidgetAction>
#include <QLabel>

// For debugging output: Uncomment the fprintf section.
#define ERROR_COBJECT(dev, format, args...)  fprintf(dev, format, ##args)
#define DEBUG_COBJECT(dev, format, args...) // fprintf(dev, format, ##args)

using std::list;
using MusEGlobal::muse;

namespace MusEGui {

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
	_initalizing = true;
	
	_isDeleting = false;
	if (initInited==false)
		initConfiguration();

	_type=t;

	setObjectName(QString(name));
	//setDockNestingEnabled(true); // Allow multiple rows.	Tim.
    setIconSize(QSize(MusEGlobal::config.iconSize, MusEGlobal::config.iconSize));

    subwinAction=new QAction(tr("As Subwindow"), this);
	subwinAction->setCheckable(true);
	connect(subwinAction, SIGNAL(toggled(bool)), SLOT(setIsMdiWin(bool)));

    shareAction=new QAction(tr("Shares Tools and Menu"), this);
	shareAction->setCheckable(true);
	connect(shareAction, SIGNAL(toggled(bool)), SLOT(shareToolsAndMenu(bool)));

	fullscreenAction=new QAction(tr("Fullscreen"), this);
	fullscreenAction->setCheckable(true);
	fullscreenAction->setChecked(false);
	fullscreenAction->setShortcut(shortcuts[SHRT_FULLSCREEN].key);
	connect(fullscreenAction, SIGNAL(toggled(bool)), SLOT(setFullscreen(bool)));

	mdisubwin=NULL;
	if (!MusEGlobal::unityWorkaround)
		_sharesToolsAndMenu=_defaultSubwin[_type] ? _sharesWhenSubwin[_type] : _sharesWhenFree[_type];
	else
		_sharesToolsAndMenu=false;
	
	if (_defaultSubwin[_type] && !MusEGlobal::unityWorkaround)
	{
		setIsMdiWin(true);
		_savedToolbarState=_toolbarNonsharedInit[_type];
	}

	if (_sharesToolsAndMenu)
		menuBar()->hide();

	subwinAction->setChecked(isMdiWin());
	shareAction->setChecked(_sharesToolsAndMenu);
	if (MusEGlobal::unityWorkaround)
	{
		shareAction->setEnabled(false);
		subwinAction->setEnabled(false);
	}
	fullscreenAction->setEnabled(!isMdiWin());
	
	if (mdisubwin)
 {
    mdisubwin->resize(_widthInit[_type], _heightInit[_type]);
    if(_type == ARRANGER)
      mdisubwin->setWindowState(Qt::WindowMaximized);
 }
	else
		resize(_widthInit[_type], _heightInit[_type]);
	

      //--------------------------------------------------
      //    Toolbar
      //--------------------------------------------------

// NOTICE: Please ensure that any tool bar object names here match the names
//          assigned in the 'toolbar' creation section of MusE::MusE(), 
//           or any other TopWin class.
//         This allows MusE::setCurrentMenuSharingTopwin() to do some magic
//          to retain the original toolbar layout. If it finds an existing
//          toolbar with the same object name, it /replaces/ it using insertToolBar(),
//          instead of /appending/ with addToolBar().
        
	QToolBar* undo_tools=addToolBar(tr("Undo/Redo tools"));
	undo_tools->setObjectName("Undo/Redo tools");
	undo_tools->addActions(MusEGlobal::undoRedo->actions());

        QToolBar* panic_toolbar = addToolBar(tr("Panic"));         
        panic_toolbar->setObjectName("Panic tool");
        panic_toolbar->addAction(MusEGlobal::panicAction);

        QToolBar* metronome_toolbar = addToolBar(tr("Metronome"));
        metronome_toolbar->setObjectName("Metronome tool");
        metronome_toolbar->addAction(MusEGlobal::metronomeAction);

        QToolBar* songpos_tb;
        songpos_tb = addToolBar(tr("Song Position"));
        songpos_tb->setObjectName("Song Position tool");
        songpos_tb->addWidget(new MusEGui::SongPosToolbarWidget(songpos_tb));
        songpos_tb->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        songpos_tb->setContextMenuPolicy(Qt::PreventContextMenu);

        addToolBarBreak();
        
        QToolBar* transport_toolbar = addToolBar(tr("Transport"));
        transport_toolbar->setObjectName("Transport tool");
        transport_toolbar->addActions(MusEGlobal::transportAction->actions());
        transport_toolbar->setIconSize(QSize(MusEGlobal::config.iconSize, MusEGlobal::config.iconSize));

        // Already has an object name.
        TempoToolbar* tempo_tb = new TempoToolbar(tr("Tempo"), this);
        addToolBar(tempo_tb);
        
        // Already has an object name.
        SigToolbar* sig_tb = new SigToolbar(tr("Signature"), this);
        addToolBar(sig_tb);
        
        connect(tempo_tb, SIGNAL(returnPressed()), SLOT(focusCanvas()));
        connect(tempo_tb, SIGNAL(escapePressed()), SLOT(focusCanvas()));
        connect(tempo_tb, SIGNAL(masterTrackChanged(bool)), MusEGlobal::song, SLOT(setMasterFlag(bool)));

        connect(sig_tb, SIGNAL(returnPressed()), SLOT(focusCanvas()));
        connect(sig_tb, SIGNAL(escapePressed()), SLOT(focusCanvas()));

 // NOTICE: It seems after the switch to Qt5, windows with a parent have stay-on-top behaviour.
 // But with the fix below, other TopWin destructors are not called when closing the app.
 // So there is now an additional fix in MusE::closeEvent() which deletes all parentless TopWin.
 //
 /* unconnect parent if window is not mdi */
 /* to make editor windows not stay on top */
 if(!isMdiWin())
 {
    setParent(0);
 }

}

TopWin::~TopWin()
{
  DEBUG_COBJECT(stderr, "TopWin dtor: %s\n", objectName().toLatin1().constData());
}

//---------------------------------------------------------
//	 readStatus
//---------------------------------------------------------

void TopWin::readStatus(MusECore::Xml& xml)
{
	int x=0, y=0, width=800, height=600;
	bool wsMinimized = false;
	bool wsMaximized = false;
	bool wsFullScreen = false;
	bool wsActive = false;
	
	for (;;)
	{
		MusECore::Xml::Token token = xml.parse();
		if (token == MusECore::Xml::Error || token == MusECore::Xml::End)
			break;
	
		QString tag = xml.s1();
		switch (token)
		{
			case MusECore::Xml::TagStart:
				if (tag == "x")
					x=xml.parseInt();
				else if (tag == "y")
					y=xml.parseInt();
				else if (tag == "width")
					width=xml.parseInt();
				else if (tag == "height")
					height=xml.parseInt();
				else if (tag == "wsMinimized")
					wsMinimized=xml.parseInt();
				else if (tag == "wsMaximized")
					wsMaximized=xml.parseInt();
				else if (tag == "wsFullScreen")
					wsFullScreen=xml.parseInt();
				else if (tag == "wsActive")
					wsActive=xml.parseInt();
				else if (tag == "toolbars")
				{
					if (!sharesToolsAndMenu())
					{
						if (!restoreState(QByteArray::fromHex(xml.parse1().toLatin1())))
						{
							fprintf(stderr,"ERROR: couldn't restore toolbars. trying default configuration...\n");
							if (!restoreState(_toolbarNonsharedInit[_type]))
								fprintf(stderr,"ERROR: couldn't restore default toolbars. this is not really a problem.\n");
						}
					}
					else
					{
						_savedToolbarState=QByteArray::fromHex(xml.parse1().toLatin1());
						if (_savedToolbarState.isEmpty())
							_savedToolbarState=_toolbarNonsharedInit[_type];
					}
				}
				else if (tag == "shares_menu")
				{
					shareToolsAndMenu(xml.parseInt());
				}
				else if (tag == "is_subwin")
				{
					setIsMdiWin(xml.parseInt());
				}
				else
					xml.unknown("TopWin");
				break;

			case MusECore::Xml::TagEnd:
				if (tag == "topwin")
				{
					const QRect geo(x, y, width, height);
					QFlags<Qt::WindowState> wstate;
					if(wsMinimized)
					  wstate |= Qt::WindowMinimized;
					if(wsMaximized)
					  wstate |= Qt::WindowMaximized;
					if(wsFullScreen)
					  wstate |= Qt::WindowFullScreen;
					if(wsActive)
					  wstate |= Qt::WindowActive;
					if (mdisubwin)
					{
						mdisubwin->setGeometry(geo);
						mdisubwin->setWindowState(wstate);
					}
					else
					{
						setGeometry(geo);
						setWindowState(wstate);
					}

					return;
				}
	
			default:
				break;
		}
	}
}

//---------------------------------------------------------
//	 writeStatus
//---------------------------------------------------------

void TopWin::writeStatus(int level, MusECore::Xml& xml) const
{
	xml.tag(level++, "topwin");

	// the order of these tags has a certain sense
	// changing it won't break muse, but it may break proper
	// restoring of the positions
	xml.intTag(level, "is_subwin", isMdiWin());

	QRect geo;
	QFlags<Qt::WindowState> wstate;
	if (mdisubwin)
	{
		wstate = mdisubwin->windowState();
		geo = mdisubwin->normalGeometry();
		// TESTED on Qt5.3: For MDI geo was invalid (0, 0, -1, -1) when window maximized.
		// This may be a reported Qt bug I read about.
		if(!geo.isValid())
		  geo = mdisubwin->geometry();
	}
	else
	{
		wstate = windowState();
		geo = normalGeometry();
		if(!geo.isValid())
		  geo = geometry();
	}
	// The order of geo first then state may be important here.
	xml.intTag(level, "x", geo.x());
	xml.intTag(level, "y", geo.y());
	xml.intTag(level, "width", geo.width());
	xml.intTag(level, "height", geo.height());
	if(wstate.testFlag(Qt::WindowMinimized))
	  xml.intTag(level, "wsMinimized", 1);
	if(wstate.testFlag(Qt::WindowMaximized))
	  xml.intTag(level, "wsMaximized", 1);
	if(wstate.testFlag(Qt::WindowFullScreen))
	  xml.intTag(level, "wsFullScreen", 1);
	if(wstate.testFlag(Qt::WindowActive))
	  xml.intTag(level, "wsActive", 1);

	xml.intTag(level, "shares_menu", sharesToolsAndMenu());

	if (!sharesToolsAndMenu())
		xml.strTag(level, "toolbars", saveState().toHex().data());
	else
		xml.strTag(level, "toolbars", _savedToolbarState.toHex().data());

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
    if (mdisubwin) {
        if (MusEGlobal::config.openMDIWinMaximized)
            mdisubwin->setWindowState(Qt::WindowMaximized);
		mdisubwin->show();
    }
	
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
	if (MusEGlobal::unityWorkaround)
		return;
	
	if (val)
	{
		if (!isMdiWin())
		{
			_savedToolbarState = saveState();
			int width_temp=width();
			int height_temp=height();
			bool vis=isVisible();
			
			QMdiSubWindow* subwin = createMdiWrapper();
			muse->addMdiSubWindow(subwin);
			subwin->resize(width_temp, height_temp);
			subwin->move(0,0);
			subwin->setVisible(vis);
			this->QMainWindow::show(); //bypass the delegation to the subwin
			
			// Due to bug in Oxygen and Breeze at least on *buntu 16.04 LTS and some other distros,
			//  force the style and stylesheet again. Otherwise the window freezes.
			if(MusEGlobal::config.fixFrozenMDISubWindows)
			{
				if(MusEGlobal::debugMsg)
				  fprintf(stderr, "TopWin::setIsMdiWin Calling updateThemeAndStyle()\n");
				MusEGui::updateThemeAndStyle(true);
			}

			if (_sharesToolsAndMenu == _sharesWhenFree[_type])
				shareToolsAndMenu(_sharesWhenSubwin[_type]);
			
			fullscreenAction->setEnabled(false);
			fullscreenAction->setChecked(false);
			subwinAction->setChecked(true);
			muse->updateWindowMenu();
			
			if(MusEGlobal::config.fixFrozenMDISubWindows)
				connect(subwin, SIGNAL(windowStateChanged(Qt::WindowStates,Qt::WindowStates)),
					        SLOT(windowStateChanged(Qt::WindowStates,Qt::WindowStates)));
		}
		else
		{
			if (MusEGlobal::debugMsg) printf("TopWin::setIsMdiWin(true) called, but window is already a MDI win\n");
		}
	}
	else
	{
		if (isMdiWin())
		{
			int width_temp=width();
			int height_temp=height();
			bool vis=isVisible();

			QMdiSubWindow* mdisubwin_temp=mdisubwin;
			mdisubwin=NULL;
			setParent(NULL);
			mdisubwin_temp->hide();
			delete mdisubwin_temp;
			
			resize(width_temp, height_temp);
			setVisible(vis);

			if (_sharesToolsAndMenu == _sharesWhenSubwin[_type])
				shareToolsAndMenu(_sharesWhenFree[_type]);
						
			fullscreenAction->setEnabled(true);
			subwinAction->setChecked(false);
			muse->updateWindowMenu();
		}
		else
		{
			if (MusEGlobal::debugMsg) printf("TopWin::setIsMdiWin(false) called, but window is not a MDI win\n");
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
	
	if (!_sharesToolsAndMenu || MusEGlobal::unityWorkaround)
		QMainWindow::addToolBar(toolbar);
	else
		toolbar->hide();
	
    toolbar->setIconSize(QSize(MusEGlobal::config.iconSize, MusEGlobal::config.iconSize));
}

QToolBar* TopWin::addToolBar(const QString& title)
{
	QToolBar* toolbar = new QToolBar(title, this);
	addToolBar(toolbar);
	return toolbar;
}


void TopWin::addToolBarBreak(Qt::ToolBarArea area)
{
  QMainWindow::addToolBarBreak(area);
  _toolbars.push_back(NULL);
}


void TopWin::shareToolsAndMenu(bool val)
{
	if (MusEGlobal::unityWorkaround)
		return;
	
	if (_sharesToolsAndMenu == val)
	{
		if (MusEGlobal::debugMsg) printf("TopWin::shareToolsAndMenu() called but has no effect\n");
		return;
	}
	
	
	_sharesToolsAndMenu = val;
	
	if (!val)
	{
		muse->shareMenuAndToolbarChanged(this, false);
		
		for (list<QToolBar*>::iterator it=_toolbars.begin(); it!=_toolbars.end(); it++)
			if (*it != NULL)
			{
				QMainWindow::addToolBar(*it);
				(*it)->show();
			}
			else
				QMainWindow::addToolBarBreak();
				
		restoreState(_savedToolbarState);
		_savedToolbarState.clear();
		
		menuBar()->show();
	}
	else
	{
		if (_savedToolbarState.isEmpty())	 // this check avoids overwriting a previously saved state 
			_savedToolbarState = saveState(); // (by setIsMdiWin) with a now incorrect (empty) state
		
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
//	 storeInitialState
//---------------------------------------------------------

void TopWin::storeInitialState() const
{
	if (mdisubwin)
	{
		_widthInit[_type] = mdisubwin->width();
		_heightInit[_type] = mdisubwin->height();
	}
	else
	{
		_widthInit[_type] = width();
		_heightInit[_type] = height();
	}
	
	if (sharesToolsAndMenu())
	{
		if (muse->getCurrentMenuSharingTopwin() == this)
			_toolbarSharedInit[_type] = muse->saveState();
	}
	else
		_toolbarNonsharedInit[_type] = saveState();
		  
	// Store class-specific view settings.
	storeInitialViewState();
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
//	 readConfiguration
//---------------------------------------------------------

void TopWin::readConfiguration(ToplevelType t, MusECore::Xml& xml)
{
	if (initInited==false)
		initConfiguration();

	for (;;)
	{
		MusECore::Xml::Token token = xml.parse();
		if (token == MusECore::Xml::Error || token == MusECore::Xml::End)
			break;
			
		const QString& tag = xml.s1();
		switch (token)
		{
			case MusECore::Xml::TagStart:
				if (tag == "width")
					_widthInit[t] = xml.parseInt();
				else if (tag == "height")
					_heightInit[t] = xml.parseInt();
				else if (tag == "nonshared_toolbars")
					_toolbarNonsharedInit[t] = QByteArray::fromHex(xml.parse1().toLatin1());
				else if (tag == "shared_toolbars")
					_toolbarSharedInit[t] = QByteArray::fromHex(xml.parse1().toLatin1());
				else if (tag == "shares_when_free")
					_sharesWhenFree[t] = xml.parseInt();
				else if (tag == "shares_when_subwin")
					_sharesWhenSubwin[t] = xml.parseInt();
				else if (tag == "default_subwin")
					_defaultSubwin[t] = xml.parseInt();
				else
					xml.unknown("TopWin");
				break;

			case MusECore::Xml::TagEnd:
				if (tag == "topwin")
					return;

			default:
				break;
		}
	}
}


//---------------------------------------------------------
//	 writeConfiguration
//---------------------------------------------------------

void TopWin::writeConfiguration(ToplevelType t, int level, MusECore::Xml& xml)
{
	if (!initInited)
	{
		printf ("WARNING: TopWin::writeConfiguration() called although the config hasn't been\n"
		        "				 initialized! writing default configuration\n");
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

void TopWin::finalizeInit()
{
	MusEGlobal::muse->topwinMenuInited(this);
	_initalizing=false;
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

void TopWin::setFullscreen(bool val)
{
	if (val)
		showFullScreen();
	else
		showNormal();
}

void TopWin::resize(int w, int h)
{
	QMainWindow::resize(w,h);
	
	if (isMdiWin())
		mdisubwin->resize(w,h);
}

void TopWin::resize(const QSize& s)
{
	resize(s.width(), s.height());
}

void TopWin::setWindowTitle (const QString& title)
{
	QMainWindow::setWindowTitle(title);
	muse->updateWindowMenu();
}

void TopWin::windowStateChanged(Qt::WindowStates oldState, Qt::WindowStates newState)
{
  // Due to bug in Oxygen and Breeze at least on *buntu 16.04 LTS and some other distros,
  //  force the style and stylesheet again. Otherwise the window freezes.
  // Ignore the Qt::WindowActive flag.
  if((oldState & (Qt::WindowNoState | Qt::WindowMinimized | Qt::WindowMaximized | Qt::WindowFullScreen)) !=
     (newState & (Qt::WindowNoState | Qt::WindowMinimized | Qt::WindowMaximized | Qt::WindowFullScreen)))
  {
    if(MusEGlobal::debugMsg)
      fprintf(stderr, "TopWin::windowStateChanged oldState:%d newState:%d Calling updateThemeAndStyle()\n", int(oldState), int(newState));
    MusEGui::updateThemeAndStyle(true);
  }
}

TopWin* ToplevelList::findType(TopWin::ToplevelType type) const
{
	for (ciToplevel i = begin(); i != end(); ++i) 
	{
		if((*i)->type() == type) 
			return (*i);
	}  
	return 0;
}

} // namespace MusEGui
