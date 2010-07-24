//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: popupmenu.h,v 1.1.1.1 2010/07/18 03:18:00 terminator356 Exp $
//
//  (C) Copyright 1999-2010 Werner Schweer (ws@seh.de)
//
//  PopupMenu sub-class of QPopupMenu created by Tim.
//=========================================================



//=========================================================
//
// NOTICE: This sub-class of QPopupMenu *automatically* deletes
//          and *clears* any sub popup menus, when clear() is called.
//         Therefore a parent widget is *not* necessary when 
//          creating sub popup menus to add to the popup.
//
//=========================================================



#ifndef __POPUPMENU_H__
#define __POPUPMENU_H__

#include <qpopupmenu.h>
//#include <qmenudata.h>

class QWidget;
class QMouseEvent;

//class MenuData : public QMenuData
//{
  //friend class QMenuBar;
//  friend class QPopupMenu;
//  friend class PopupMenu;
  
//  Q_OBJECT
  //private:  
//};

/*
// Internal class to get access to protected QMenuData members.
class MenuData : public QMenuData
{
    friend class QPopupMenu;
    friend class QMenuData;
    friend class PopupMenu;
    
private:    

public:
    MenuData() : QMenuData() { }
    virtual ~MenuData() { }
};
*/

//class Q_EXPORT PopupMenu : public QPopupMenu
class PopupMenu : public QPopupMenu
//class PopupMenu : public QPopupMenu, public MenuData
{
  friend class QMenuData;
  //friend class QMenuBar;
  friend class QPopupMenu;
  friend class QMenuBar;
  //friend class MenuData;
  
  Q_OBJECT
  private:  
    // QPopupMenu::d is private, so this is our own private.
    //QPopupMenuPrivate *d;
    
    //virtual void setFirstItemActive();
    //void hideAllPopups();
    //void hidePopups();
    bool tryMenuBar(QMouseEvent *);
    //bool tryMouseEvent(QPopupMenu *, QMouseEvent *);
    bool tryMouseEvent(PopupMenu *, QMouseEvent *);
    //void byeMenuBar();
    void actSig(int, bool = FALSE);
    virtual void menuDelPopup(QPopupMenu *);
  
  protected:
    //int            actItem;
    
    //void mousePressEvent(QMouseEvent *);
    void mouseReleaseEvent(QMouseEvent *);
  
  public:
    PopupMenu(QWidget* parent=0, const char* name=0);
    ~PopupMenu();
};

#endif




/****************************************************************************
**
** Definition of QPopupMenu class
**
** Created : 941128
**
** Copyright (C) 1992-2008 Trolltech ASA.  All rights reserved.
**
** This file is part of the widgets module of the Qt GUI Toolkit.
**
** This file may be used under the terms of the GNU General
** Public License versions 2.0 or 3.0 as published by the Free
** Software Foundation and appearing in the files LICENSE.GPL2
** and LICENSE.GPL3 included in the packaging of this file.
** Alternatively you may (at your option) use any later version
** of the GNU General Public License if such license has been
** publicly approved by Trolltech ASA (or its successors, if any)
** and the KDE Free Qt Foundation.
**
** Please review the following information to ensure GNU General
** Public Licensing requirements will be met:
** http://trolltech.com/products/qt/licenses/licensing/opensource/.
** If you are unsure which license is appropriate for your use, please
** review the following information:
** http://trolltech.com/products/qt/licenses/licensing/licensingoverview
** or contact the sales department at sales@trolltech.com.
**
** This file may be used under the terms of the Q Public License as
** defined by Trolltech ASA and appearing in the file LICENSE.QPL
** included in the packaging of this file.  Licensees holding valid Qt
** Commercial licenses may use this file in accordance with the Qt
** Commercial License Agreement provided with the Software.
**
** This file is provided "AS IS" with NO WARRANTY OF ANY KIND,
** INCLUDING THE WARRANTIES OF DESIGN, MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE. Trolltech reserves all rights not granted
** herein.
**
**********************************************************************/

/*
#ifndef __POPUPMENU_H__
#define __POPUPMENU_H__

#ifndef QT_H
#include <qframe.h>
#include <qmenudata.h>
#endif // QT_H

#ifndef QT_NO_POPUPMENU
class QPopupMenuPrivate;

class Q_EXPORT PopupMenu : public QFrame, public QMenuData
{
    Q_OBJECT
    Q_PROPERTY( bool checkable READ isCheckable WRITE setCheckable )
public:
    PopupMenu( QWidget* parent=0, const char* name=0 );
    ~PopupMenu();

    void        popup( const QPoint & pos, int indexAtPoint = -1 ); // open
    void        updateItem( int id );

    virtual void        setCheckable( bool );
    bool        isCheckable() const;

    void        setFont( const QFont & );
    void        show();
    void        hide();

    int         exec();
    int         exec( const QPoint & pos, int indexAtPoint = 0 ); // modal

    virtual void        setActiveItem( int );
    QSize       sizeHint() const;

    int         idAt( int index ) const { return QMenuData::idAt( index ); }
    int         idAt( const QPoint& pos ) const;

    bool        customWhatsThis() const;

    int         insertTearOffHandle( int id=-1, int index=-1 );

    void        activateItemAt( int index );
    QRect       itemGeometry( int index );


signals:
    void        activated( int itemId );
    void        highlighted( int itemId );
    void        activatedRedirect( int itemId ); // to parent menu
    void        highlightedRedirect( int itemId );
    void        aboutToShow();
    void        aboutToHide();

protected:
    int         itemHeight( int ) const;
    int         itemHeight( QMenuItem* mi ) const;
    void        drawItem( QPainter* p, int tab, QMenuItem* mi,
                   bool act, int x, int y, int w, int h);

    void        drawContents( QPainter * );

    void        closeEvent( QCloseEvent *e );
    void        paintEvent( QPaintEvent * );
    void        mousePressEvent( QMouseEvent * );
    void        mouseReleaseEvent( QMouseEvent * );
    void        mouseMoveEvent( QMouseEvent * );
    void        keyPressEvent( QKeyEvent * );
    void        focusInEvent( QFocusEvent * );
    void        focusOutEvent( QFocusEvent * );
    void        timerEvent( QTimerEvent * );
    void        leaveEvent( QEvent * );
    void        styleChange( QStyle& );
    void        enabledChange( bool );
    int         columns() const;

    bool        focusNextPrevChild( bool next );

    int         itemAtPos( const QPoint &, bool ignoreSeparator = TRUE ) const;

private slots:
    void        subActivated( int itemId );
    void        subHighlighted( int itemId );
#ifndef QT_NO_ACCEL
    void        accelActivated( int itemId );
    void        accelDestroyed();
#endif
    void        popupDestroyed( QObject* );
    void        modalActivation( int );

    void        subMenuTimer();
    void        subScrollTimer();
    void        allowAnimation();
    void     toggleTearOff();

    void        performDelayedChanges();

private:
    void        updateScrollerState();
    void        menuContentsChanged();
    void        menuStateChanged();
    void        performDelayedContentsChanged();
    void        performDelayedStateChanged();
    void        menuInsPopup( PopupMenu * );
    void        menuDelPopup( PopupMenu * );
    void        frameChanged();

    void        actSig( int, bool = FALSE );
    void        hilitSig( int );
    virtual void setFirstItemActive();
    void        hideAllPopups();
    void        hidePopups();
    bool        tryMenuBar( QMouseEvent * );
    void        byeMenuBar();

    QSize       updateSize(bool force_recalc=FALSE, bool do_resize=TRUE);
    void        updateRow( int row );
#ifndef QT_NO_ACCEL
    void        updateAccel( QWidget * );
    void        enableAccel( bool );
#endif
    QPopupMenuPrivate  *d;
#ifndef QT_NO_ACCEL
    QAccel     *autoaccel;
#endif

#if defined(Q_WS_MAC) && !defined(QMAC_QMENUBAR_NO_NATIVE)
    bool macPopupMenu(const QPoint &, int);
    uint mac_dirty_popup : 1;
#endif

    int popupActive;
    int tab;
    uint accelDisabled : 1;
    uint checkable : 1;
    uint connectModalRecursionSafety : 1;
    uint tornOff : 1;
    uint pendingDelayedContentsChanges : 1;
    uint pendingDelayedStateChanges : 1;
    int maxPMWidth;
    int ncols;
    bool        snapToMouse;
    bool        tryMouseEvent( PopupMenu *, QMouseEvent * );

    friend class QMenuData;
    friend class QMenuBar;

    void connectModal(PopupMenu* receiver, bool doConnect);

private:        // Disabled copy constructor and operator=
#if defined(Q_DISABLE_COPY)
    PopupMenu( const PopupMenu & );
    PopupMenu &operator=( const PopupMenu & );
#endif
};


#endif // QT_NO_POPUPMENU

#endif // QPOPUPMENU_H
*/