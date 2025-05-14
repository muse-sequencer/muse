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
//#include "gui.h"
#include "globals.h"
#include "app.h"
#include "shortcuts.h"
#include "songpos_toolbar.h"
#include "sig_tempo_toolbar.h"
#include "gconfig.h"
#include "helper.h"
#include "song.h"
#include "icons.h"
#include "rectoolbar.h"
#include "postoolbar.h"
#include "synctoolbar.h"

#include <QMenuBar>
//#include <QWidgetAction>
//#include <QLabel>

// Forwards from header:
#include <QMdiSubWindow>
//#include <QFocusEvent>
#include <QCloseEvent>
#include <QToolBar>
#include <QAction>
#include "xml.h"

// For debugging output: Uncomment the fprintf section.
#define ERROR_COBJECT(dev, format, args...)  fprintf(dev, format, ##args)
#define DEBUG_COBJECT(dev, format, args...) // fprintf(dev, format, ##args)
// For debugging song clearing and loading: Uncomment the fprintf section.
#define DEBUG_LOADING_AND_CLEARING(dev, format, args...) // fprintf(dev, format, ##args);

using std::list;
using MusEGlobal::muse;

namespace MusEGui {

int TopWin::_widthInit[TOPLEVELTYPE_LAST_ENTRY];
int TopWin::_heightInit[TOPLEVELTYPE_LAST_ENTRY];
QByteArray TopWin::_toolbarSharedInit[TOPLEVELTYPE_LAST_ENTRY];
QByteArray TopWin::_toolbarNonsharedInit[TOPLEVELTYPE_LAST_ENTRY];
bool TopWin::_openTabbed[TOPLEVELTYPE_LAST_ENTRY];
bool TopWin::initInited=false;

TopWin::TopWin(ToplevelType t, QWidget* parent, const char* name, Qt::WindowFlags f)
    : QMainWindow(parent, f)
{
    _initalizing = true;
    _isDeleting = false;

    if (!initInited)
        initConfiguration();

    _type=t;

    setObjectName(name ? QString(name) : "TopWin");
    //setDockNestingEnabled(true); // Allow multiple rows.	Tim.
    setIconSize(QSize(MusEGlobal::config.iconSize, MusEGlobal::config.iconSize));

    setWindowIcon(typeIcon(_type));

    setAttribute(Qt::WA_DeleteOnClose);

    subwinAction = new QAction(tr("Tabbed/Floating"), this);
    subwinAction->setCheckable(true);
    subwinAction->setStatusTip(tr("Display editor in a tab or in a separate window (preset in Global Settings->Editors)."));
    subwinAction->setShortcut(shortcuts[SHRT_TABBED_WIN].key);
    connect(subwinAction, &QAction::toggled, [this](bool v) { setIsMdiWin(v); } );

//    shareAction=new QAction(tr("Shares Tools and Menu"), this);
//    shareAction->setCheckable(true);
//    connect(shareAction, SIGNAL(toggled(bool)), SLOT(shareToolsAndMenu(bool)));

    fullscreenAction=new QAction(tr("Fullscreen"), this);
    fullscreenAction->setCheckable(true);
    fullscreenAction->setChecked(false);
    fullscreenAction->setShortcut(shortcuts[SHRT_FULLSCREEN].key);
    connect(fullscreenAction, &QAction::toggled, [this](bool v) { setFullscreen(v); } );


    mdisubwin = nullptr;

    _sharesToolsAndMenu=_openTabbed[_type];

    if (_openTabbed[_type] && !MusEGlobal::unityWorkaround)
    {
        setIsMdiWin(true);
        _savedToolbarState=_toolbarNonsharedInit[_type];
    }

    if (_sharesToolsAndMenu)
        menuBar()->hide();

    subwinAction->setChecked(isMdiWin());
//    shareAction->setChecked(_sharesToolsAndMenu);

    if (MusEGlobal::unityWorkaround)
    {
        _sharesToolsAndMenu=false;
//        shareAction->setEnabled(false);
        subwinAction->setEnabled(false);
    }

    fullscreenAction->setEnabled(!isMdiWin());

    if (_type == ARRANGER) {
//            shareAction->setEnabled(false);
        subwinAction->setEnabled(false);
    }

    if (!mdisubwin)
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

    QToolBar* undo_tools=addToolBar(tr("Undo/Redo"));
    undo_tools->setObjectName("Undo/Redo");
    undo_tools->addActions(MusEGlobal::undoRedo->actions());

    QToolBar* panic_toolbar = addToolBar(tr("Panic"));
    panic_toolbar->setObjectName("Panic tool");
    panic_toolbar->addAction(MusEGlobal::panicAction);

    QToolBar* metronome_toolbar = addToolBar(tr("Metronome"));
    metronome_toolbar->setObjectName("Metronome tool");
    metronome_toolbar->addAction(MusEGlobal::metronomeAction);

    QToolBar* songpos_tb = addToolBar(tr("Timeline"));
    songpos_tb->setObjectName("Timeline tool");
    songpos_tb->addWidget(new MusEGui::SongPosToolbarWidget(songpos_tb));
    songpos_tb->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    songpos_tb->setContextMenuPolicy(Qt::PreventContextMenu);

    QToolBar* transportToolbar = addToolBar(tr("Transport"));
    transportToolbar->setObjectName("Transport tool");
    transportToolbar->addActions(MusEGlobal::transportAction->actions());
    transportToolbar->setIconSize(QSize(MusEGlobal::config.iconSize, MusEGlobal::config.iconSize));

    RecToolbar *recToolbar = new RecToolbar(tr("Recording"), this);
    addToolBar(recToolbar);

    SyncToolbar *syncToolbar = new SyncToolbar(tr("Sync"), this);
    addToolBar(syncToolbar);

    addToolBarBreak();

    TempoToolbar* tempo_tb = new TempoToolbar(tr("Tempo"), this);
    addToolBar(tempo_tb);

    SigToolbar* sig_tb = new SigToolbar(tr("Signature"), this);
    addToolBar(sig_tb);

    PosToolbar *posToolbar = new PosToolbar(tr("Position"), this);
    addToolBar(posToolbar);

    connect(tempo_tb, &TempoToolbar::returnPressed, [this]() { focusCanvas(); } );
    connect(tempo_tb, &TempoToolbar::escapePressed, [this]() { focusCanvas(); } );
    connect(tempo_tb, &TempoToolbar::masterTrackChanged, [](bool v) { MusEGlobal::song->setMasterFlag(v); } );

    connect(sig_tb, &SigToolbar::returnPressed, [this]() { focusCanvas(); } );
    connect(sig_tb, &SigToolbar::escapePressed, [this]() { focusCanvas(); } );

    connect(posToolbar, &PosToolbar::returnPressed, [this]() { focusCanvas(); } );
    connect(posToolbar, &PosToolbar::escapePressed, [this]() { focusCanvas(); } );

// this is not (longer?) the case, to be tested on KDE (kybos)
// what about changing from MDI to top window later? then the parent remains anyway... (kybos)
//    // NOTICE: It seems after the switch to Qt5, windows with a parent have stay-on-top behaviour.
//    // But with the fix below, other TopWin destructors are not called when closing the app.
//    // So there is now an additional fix in MusE::closeEvent() which deletes all parentless TopWin.
//    //
//    /* unconnect parent if window is not mdi */
//    /* to make editor windows not stay on top */
//    if(!isMdiWin())
//    {
//        setParent(nullptr);
//    }

    DEBUG_LOADING_AND_CLEARING(stderr, "TopWin::TopWin:%p <%s>\n", this, typeName(_type).toLocal8Bit().constData());

#ifndef USE_SENDPOSTEDEVENTS_FOR_TOPWIN_CLOSE
    // Inform when destroyed.
    // We do not want the ArrangerView included here.
    if(_type != ARRANGER)
      MusEGlobal::muse->addPendingObjectDestruction(this);
#endif

}

TopWin::~TopWin()
{
    DEBUG_COBJECT(stderr, "TopWin dtor: %s\n", objectName().toLocal8Bit().constData());

    DEBUG_LOADING_AND_CLEARING(stderr, "~TopWin:%p <%s>\n", this, typeName(_type).toLocal8Bit().constData());

    // Toolbars must be deleted explicitly to avoid memory leakage and corruption.
    // For some reason (toolbar sharing?) they are reparented and thus
    // not destroyed by the original parent topwin when closed.
    for (auto& it : _toolbars) {
        if (it) {
            delete it;
            it = nullptr;
        }
    }

    if (mdisubwin)
        mdisubwin->close();
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
                    // We can only restore the state with version-compatible data.
                    // If toolbars were altered, 'alien' loaded data will not fit!
                    if(xml.isVersionEqualToLatest())
                    {
                      if (!restoreState(QByteArray::fromHex(xml.parse1().toLatin1())))
                      {
                          fprintf(stderr,"ERROR: couldn't restore toolbars. trying default configuration...\n");
                          if (!restoreState(_toolbarNonsharedInit[_type]))
                              fprintf(stderr,"ERROR: couldn't restore default toolbars. this is not really a problem.\n");
                      }
                    }
                    else
                      xml.parse1();
                }
                else
                {
                    // We can only restore the state with version-compatible data.
                    // If toolbars were altered, 'alien' loaded data will not fit!
                    if(xml.isVersionEqualToLatest())
                    {
                      _savedToolbarState = QByteArray::fromHex(xml.parse1().toLatin1());
                      if (_savedToolbarState.isEmpty())
                          _savedToolbarState=_toolbarNonsharedInit[_type];
                    }
                    else
                      xml.parse1();
                }
            }
//            else if (tag == "shares_menu")
//            {
//                shareToolsAndMenu(xml.parseInt());
//            }
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
                if (mdisubwin) {
                    QFlags<Qt::WindowState> wstate = Qt::WindowMaximized;
                    if (wsActive)
                        wstate |= Qt::WindowActive;
                    setWindowState(wstate);
                }
                else {
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

                    setGeometry(geo);
                    setWindowState(wstate);
                }

                return;
            }
            break;

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

//    xml.intTag(level, "shares_menu", sharesToolsAndMenu());

    if (sharesToolsAndMenu())
        xml.strTag(level, "toolbars", _savedToolbarState.toHex().data());
    else
        xml.strTag(level, "toolbars", saveState().toHex().data());

    xml.etag(--level, "topwin");
}

void TopWin::hide()
{
    if (mdisubwin)
        mdisubwin->hide();

    QMainWindow::hide();
}

void TopWin::show()
{
    if (mdisubwin)
        mdisubwin->showMaximized();

    QMainWindow::show();
}

void TopWin::setVisible(bool param)
{
    if (mdisubwin)
    {
        if (param)
            mdisubwin->show();
        else
            mdisubwin->hide();
    }

    QMainWindow::setVisible(param);
}

void TopWin::createMdiWrapper()
{
    if (mdisubwin == nullptr)
    {
        mdisubwin = new QMdiSubWindow();
        mdisubwin->setWidget(this);
        mdisubwin->setWindowIcon(typeIcon(_type));

        if (_type == ARRANGER) {
            mdisubwin->setWindowFlags(Qt::CustomizeWindowHint);
        } else {
            mdisubwin->setAttribute(Qt::WA_DeleteOnClose);
            mdisubwin->setWindowFlags(Qt::CustomizeWindowHint
                                      | Qt::WindowCloseButtonHint);
        }
    }
}

void TopWin::setIsMdiWin(bool val)
{
    if (MusEGlobal::unityWorkaround)
        return;

    if (!val && _type == ARRANGER)
        return;

    if (val)
    {
        if (!isMdiWin())
        {
            _savedToolbarState = saveState();

            createMdiWrapper();
            muse->addMdiSubWindow(mdisubwin);

            if (windowTitle().startsWith("MusE: "))
                setWindowTitle(windowTitle().mid(6));

            shareToolsAndMenu(true);

            fullscreenAction->setEnabled(false);
            fullscreenAction->setChecked(false);
            {
                const QSignalBlocker blocker(subwinAction);
                subwinAction->setChecked(true);
            }
            muse->updateWindowMenu();
            mdisubwin->showMaximized();
            muse->setActiveMdiSubWindow(mdisubwin);
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
            mdisubwin->setWidget(nullptr);
            mdisubwin->close();
            mdisubwin = nullptr;

            setParent(muse);
            setWindowFlags(Qt::Window);


            if (!windowTitle().startsWith("MusE: "))
                setWindowTitle(windowTitle().insert(0, "MusE: "));

            shareToolsAndMenu(false);

            fullscreenAction->setEnabled(true);
            {
                const QSignalBlocker blocker(subwinAction);
                subwinAction->setChecked(false);
            }
            muse->updateWindowMenu();
            QMainWindow::show();
        }
        else
        {
            if (MusEGlobal::debugMsg) printf("TopWin::setIsMdiWin(false) called, but window is not a MDI win\n");
        }
    }
}

bool TopWin::isMdiWin() const
{
    return (mdisubwin != nullptr);
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
    _toolbars.push_back(nullptr);
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

        for (const auto& it : _toolbars)
            if (it) {
                QMainWindow::addToolBar(it);
                it->show();
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

        for (const auto& it : _toolbars)
            if (it) {
                QMainWindow::removeToolBar(it); // this does NOT delete the toolbar, which is good
                it->setParent(nullptr);
            }

        menuBar()->hide();

        muse->shareMenuAndToolbarChanged(this, true);
    }
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



//initConfiguration() restores default tabbed configuration
void TopWin::initConfiguration()
{
    if (initInited)
        return;

    for (int i = 0; i < TOPLEVELTYPE_LAST_ENTRY; i++)
    {
        _widthInit[i] = 800;
        _heightInit[i] = 600;
        _openTabbed[i] = true;
    }

    initInited = true;
}

//---------------------------------------------------------
//	 readConfiguration
//---------------------------------------------------------

void TopWin::readConfiguration(ToplevelType t, MusECore::Xml& xml)
{
    if (!initInited)
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
            {
                // We can only restore the state with version-compatible data.
                // If toolbars were altered, 'alien' loaded data will not fit!
                if(xml.isVersionEqualToLatest())

                  _toolbarNonsharedInit[t] = QByteArray::fromHex(xml.parse1().toLatin1());
                else
                  xml.parse1();
            }
            else if (tag == "shared_toolbars")
            {
                // We can only restore the state with version-compatible data.
                // If toolbars were altered, 'alien' loaded data will not fit!
                if(xml.isVersionEqualToLatest())
                  _toolbarSharedInit[t] = QByteArray::fromHex(xml.parse1().toLatin1());

                else
                  xml.parse1();
            }
            else if (tag == "default_subwin")
                _openTabbed[t] = xml.parseInt();
            else
                xml.unknown("TopWin");
            break;

        case MusECore::Xml::TagEnd:
            if (tag == "topwin")
                return;
            break;

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
    xml.intTag(level, "default_subwin", _openTabbed[t]);
    xml.etag(--level, "topwin");
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
#ifdef MOVE_LISTEDIT_FROM_DOCK_TO_WINDOW_PULL1099
    case LISTE: return tr("List editor");
#else
    //case LISTE: return tr("List editor");
#endif
    case DRUM: return tr("Drum editor");
    case MASTER: return tr("Master track editor");
    case WAVE: return tr("Wave editor");
    case SCORE: return tr("Score editor");
    case ARRANGER: return tr("Arranger");
    default: return tr("<unknown toplevel type>");
    }
}

QIcon TopWin::typeIcon(ToplevelType t)
{
    switch (t)
    {
    case ARRANGER: return QIcon(*arrangerSVGIcon);
    case PIANO_ROLL: return QIcon(*pianorollSVGIcon);
    case DRUM: return QIcon(*drumeditSVGIcon);
    case MASTER: return QIcon(*mastereditSVGIcon);
    case WAVE: return QIcon(*waveeditorSVGIcon);
    case SCORE: return QIcon(*scoreeditSVGIcon);
    default: return QIcon();
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
    if (isMdiWin())
        return;

    QMainWindow::resize(w,h);
}

void TopWin::setWindowTitle (const QString& title)
{
    QMainWindow::setWindowTitle(title);
    muse->updateWindowMenu();
}

void TopWin::storeSettings() {}

void TopWin::setOpenInNewWin(bool newwin)
{
    if ( !(_openTabbed[_type]) || (_openTabbed[_type] && newwin) )
        setIsMdiWin(false);
    else
        setIsMdiWin(true);
}

//void TopWin::windowStateChanged(Qt::WindowStates oldState, Qt::WindowStates newState)
//{
//    // Due to bug in Oxygen and Breeze at least on *buntu 16.04 LTS and some other distros,
//    //  force the style and stylesheet again. Otherwise the window freezes.
//    // Ignore the Qt::WindowActive flag.
//    if((oldState & (Qt::WindowNoState | Qt::WindowMinimized | Qt::WindowMaximized | Qt::WindowFullScreen)) !=
//            (newState & (Qt::WindowNoState | Qt::WindowMinimized | Qt::WindowMaximized | Qt::WindowFullScreen)))
//    {
//        if(MusEGlobal::debugMsg)
//            fprintf(stderr, "TopWin::windowStateChanged oldState:%d newState:%d Calling updateThemeAndStyle()\n", int(oldState), int(newState));
//        MusEGui::updateThemeAndStyle(true);
//    }
//}

TopWin* ToplevelList::findType(TopWin::ToplevelType type) const
{
    for (ciToplevel i = begin(); i != end(); ++i)
    {
        if((*i)->type() == type)
            return (*i);
    }
    return nullptr;
}

} // namespace MusEGui
