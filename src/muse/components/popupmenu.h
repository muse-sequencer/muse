//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: popupmenu.h,v 1.1.1.1 2010/07/18 03:18:00 terminator356 Exp $
//
//  (C) Copyright 1999-2010 Werner Schweer (ws@seh.de)
//
//  PopupMenu sub-class of QMenu created by Tim.
//  (C) Copyright 2010-2015 Tim E. Real (terminator356 A T sourceforge D O T net)
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

#ifndef __POPUPMENU_H__
#define __POPUPMENU_H__

// Just in case Qt ever adds these features natively, we would need to turn our features off!
//#define POPUP_MENU_DISABLE_STAY_OPEN    
//#define POPUP_MENU_DISABLE_AUTO_SCROLL

#include <QMenu>
#ifndef POPUP_MENU_DISABLE_AUTO_SCROLL  
  #include <QTimer>
#endif

#include <QVariant>
#include <QPointer>
#include <QAction>


// Forward declarations:
class QWidget;
class QMouseEvent;
class QContextMenuEvent;
class QHideEvent;
class QEvent;

namespace MusEGui {

/** offers a QMenu-like menu, which stays open once the user
 *  clicked a checkable action. */
class PopupMenu : public QMenu
{
Q_OBJECT
  
    bool _stayOpen;
    #ifndef POPUP_MENU_DISABLE_AUTO_SCROLL  
    QTimer* timer;
    #endif
    int moveDelta;
    PopupMenu* _cur_menu; // For auto-breakup.
    int _cur_menu_count;
    int _max_items_in_breakup;

    QMenu* _contextMenu;
    QAction* _lastHoveredAction;
    QPointer<QAction> _highlightedAction;
    
    void init();
    void showContextMenu(const QPoint&);
    // Auto-breakup a too-wide menu.
    // If a new menu is created, parentText will be used as the parent item's text.
    PopupMenu* getMenu(const QString& parentText);
    
  private slots:
    void popHovered(QAction*);

    #ifndef POPUP_MENU_DISABLE_AUTO_SCROLL  
    void timerHandler();
    #endif
  
  protected:
    virtual void mouseReleaseEvent(QMouseEvent*);
    virtual void mousePressEvent(QMouseEvent*);
    virtual void contextMenuEvent(QContextMenuEvent*);
    virtual void hideEvent(QHideEvent*);    
    virtual bool event(QEvent*);
    virtual void closeUp();
    
    // For auto-breakup of a too-wide menu. Virtual.
    virtual PopupMenu* cloneMenu(const QString& title, QWidget* parent = 0, bool stayOpen = false, bool showTooltips = false);

  public: signals:
    void aboutToShowContextMenu(PopupMenu* menu, QAction* menuAction, QMenu* ctxMenu);
    
  public:
    PopupMenu(bool stayOpen);
    PopupMenu(QWidget* parent=0, bool stayOpen = false);
    PopupMenu(const QString& title, QWidget* parent = 0, bool stayOpen = false);
    ~PopupMenu();
    QAction* findActionFromData(const QVariant&) const;
    bool stayOpen() const { return _stayOpen; }
    void clearAllChecks() const;
    
    QMenu* contextMenu();
    void hideContextMenu();
    static PopupMenu* contextMenuFocus();
    static QAction* contextMenuFocusAction();

    // Need to catch these to auto-breakup a too-big menu.
    QAction* addAction(const QString& text);
    QAction* addAction(const QIcon& icon, const QString& text);
    QAction* addAction(const QString& text, const QObject* receiver, const char* member, const QKeySequence& shortcut = 0);
    QAction* addAction(const QIcon& icon, const QString& text, const QObject* receiver, const char* member, const QKeySequence& shortcut = 0);
    void     addAction(QAction* action);
    QAction* addMenu(QMenu* menu);
    QMenu*   addMenu(const QString &title);
    QMenu*   addMenu(const QIcon &icon, const QString &title);
};

// A handy structure for use with PopupMenu context menu action data.
// The variant holds the ORIGINAL data as set by the programmer.
class PopupMenuContextData {
  private:
    PopupMenu* _menu;
    QAction* _action;
    QVariant _variant;
    
  public:
    PopupMenuContextData() : _menu(0), _action(0), _variant(0) { }
    PopupMenuContextData(const PopupMenuContextData& o) : _menu(o._menu), _action(o._action), _variant(o._variant) { }
    PopupMenuContextData(PopupMenu* menu, QAction* action, QVariant var) : _menu(menu), _action(action), _variant(var) { }
  
    inline PopupMenu* menu() const { return _menu; }
    inline QAction* action() const { return _action; }
    inline QVariant varValue() const { return _variant; }
};
  
} // namespace MusEGui

Q_DECLARE_METATYPE(MusEGui::PopupMenuContextData)

#endif

