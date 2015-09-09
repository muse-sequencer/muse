//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: popupmenu.cpp,v 1.1.1.1 2010/07/18 03:21:00 terminator356 Exp $
//
//  (C) Copyright 1999-2010 Werner Schweer (ws@seh.de)
//
//  PopupMenu sub-class of QMenu created by Tim.
//  (C) Copyright 2010-2011 Tim E. Real (terminator356 A T sourceforge D O T net)
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

#include <QMouseEvent>
#include <QHoverEvent>
#include <QAction>
#include <QPoint>
#include <QList>
#include <QVariant>
#include <QDesktopWidget>
#include <QApplication>
//#include <QMetaMethod>
#include <QStyle>

#include "popupmenu.h"
#include "gconfig.h"
#include "route.h"


namespace MusEGui {

//======================
// PopupMenu
//======================

PopupMenu::PopupMenu(bool stayOpen) 
   : _stayOpen(stayOpen)
{
   init();
}

PopupMenu::PopupMenu(QWidget* parent, bool stayOpen) 
   : QMenu(parent), _stayOpen(stayOpen)
{
   init();
}

PopupMenu::PopupMenu(const QString& title, QWidget* parent, bool stayOpen)
   : QMenu(title, parent), _stayOpen(stayOpen)
{
   init();
}

void PopupMenu::init()
{
   // REMOVE Tim. Persistent routes. Added. TESTING
   //setWindowFlags((windowFlags() ^ Qt::Popup) | Qt::Window);
  
   _contextMenu = 0;
   _lastHoveredAction = 0;
   _highlightedAction = 0;
   if(MusEGlobal::config.scrollableSubMenus)
   {
      setStyleSheet("QMenu { menu-scrollable: 1; }");
      return;
   }
   // Menus will trigger! Set to make sure our trigger handlers ignore menus.
   menuAction()->setData(-1);

   _cur_menu = this;
   _cur_menu_count = 1;
   _cur_item_width = 0;
   _cur_col_count = 0;

   //_stayOpen = false;
   moveDelta = 0;

   connect(this, SIGNAL(hovered(QAction*)), SLOT(popHovered(QAction*)));
   
#ifndef POPUP_MENU_DISABLE_AUTO_SCROLL
   timer = new QTimer(this);
   timer->setInterval(100);
   timer->setSingleShot(false);
   connect(timer, SIGNAL(timeout()), SLOT(timerHandler()));
#endif   // POPUP_MENU_DISABLE_AUTO_SCROLL
}

PopupMenu::~PopupMenu()
{
   if(_contextMenu)
     delete _contextMenu;
   _contextMenu = 0;
}

// NOTE: Tested all RoutePopupMenu and PopupMenu dtors and a couple of action dtors from our 
//  PixmapButtonsHeaderWidgetAction and PixmapButtonsWidgetAction: 
// This does not appear to be required any more. All submenus and actions are being deleted now.  
/*
void PopupMenu::clear()
{
  //printf("PopupMenu::clear this:%p\n", this);

  QList<QAction*> list = actions();
  for(int i = 0; i < list.size(); ++i)
  {
    QAction* act = list[i];
    QMenu* menu = act->menu();
    if(menu)
    {
      menu->clear();    // Recursive.
      act->setMenu(0);  // CHECK: Is this OK?
      //printf("  deleting menu:%p\n", menu);
      delete menu;
    }
  }
  
  // Now let QT remove and delete this menu's actions.
  QMenu::clear();
  
  #ifndef POPUP_MENU_DISABLE_AUTO_SCROLL
  connect(this, SIGNAL(hovered(QAction*)), SLOT(popHovered(QAction*)));
  connect(timer, SIGNAL(timeout()), SLOT(timerHandler()));
  #endif    // POPUP_MENU_DISABLE_AUTO_SCROLL
}
*/

void PopupMenu::clearAllChecks() const
{
   QList<QAction*> list = actions();
   for(int i = 0; i < list.size(); ++i)
   {
      QAction* act = list[i];
      PopupMenu* menu = static_cast <PopupMenu*>(act->menu());
      if(menu)
         menu->clearAllChecks();     // Recursive.
      if(act->isCheckable())
      {
         act->blockSignals(true);
         act->setChecked(false);
         act->blockSignals(false);
      }
   }
}

QAction* PopupMenu::findActionFromData(const QVariant& v) const
{
   QList<QAction*> list = actions();
   for(int i = 0; i < list.size(); ++i)
   {
      QAction* act = list[i];
      PopupMenu* menu = (PopupMenu*)act->menu();
      if(menu)
      {
         if(QAction* actm = menu->findActionFromData(v))      // Recursive.
            return actm;
      }

      // "Operator == Compares this QVariant with v and returns true if they are equal,
      //   otherwise returns false. In the case of custom types, their equalness operators
      //   are not called. Instead the values' addresses are compared."
      //
      // Take care of struct Route first. Insert other future custom structures here too !
      if(act->data().canConvert<MusECore::Route>() && v.canConvert<MusECore::Route>())
      {
         if(act->data().value<MusECore::Route>() == v.value<MusECore::Route>())
            return act;
      }
      else
         if(act->data() == v)
            return act;
   }
   return 0;
}

bool PopupMenu::event(QEvent* event)
{
  // REMOVE Tim. Persistent routes. Added.
  fprintf(stderr, "PopupMenu::event:%p activePopupWidget:%p this:%p class:%s event type:%d\n", 
          event, QApplication::activePopupWidget(), this, metaObject()->className(), event->type()); 
   if(MusEGlobal::config.scrollableSubMenus)
   {
      return QMenu::event(event);
   }
   
   switch(event->type())
   {
#ifndef POPUP_MENU_DISABLE_STAY_OPEN
   case QEvent::MouseButtonDblClick:
   {
      if(_stayOpen)
         //if(_stayOpen && MusEGlobal::config.popupsDefaultStayOpen)
      {
         QMouseEvent* e = static_cast<QMouseEvent*>(event);
         if(e->modifiers() == Qt::NoModifier)
         {
            event->accept();
            // Convert into a return press, which selects the item and closes the menu.
            // Note that with double click, it's a press followed by release followed by double click.
            // That would toggle our item twice eg on->off->on, which is hopefully OK.
            QKeyEvent ke(QEvent::KeyPress, Qt::Key_Return, Qt::NoModifier);
            //ke.ignore();   // Pass it on
            return QMenu::event(&ke);
         }
      }
   }
   break;
   
   case QEvent::KeyPress:
   {
//       if(_stayOpen)
//          //if(_stayOpen && MusEGlobal::config.popupsDefaultStayOpen)
//       {
//          QKeyEvent* e = static_cast<QKeyEvent*>(event);
//          if(e->modifiers() == Qt::NoModifier && e->key() == Qt::Key_Space)
//          {
//             QAction* act = activeAction();
//             if(act)
//             {
//                act->trigger();
//                event->accept();
//                return true;    // We handled it.
//             }
//          }
//       }
     
     QKeyEvent* e = static_cast<QKeyEvent*>(event);
     switch(e->key())
     {
        case Qt::Key_Space:
          if (!style()->styleHint(QStyle::SH_Menu_SpaceActivatesItem, 0, this))
              break;
            // for motif, fall through
#ifdef QT_KEYPAD_NAVIGATION
        case Qt::Key_Select:
#endif
        case Qt::Key_Return:
        case Qt::Key_Enter:
        {
          if(_stayOpen && (MusEGlobal::config.popupsDefaultStayOpen || (e->modifiers() & Qt::ControlModifier)))
          {
            //if(act && act->menu() && act->isCheckable())
            if(QAction* act = activeAction())
            {
              // REMOVE Tim. Persistent routes. Added.
              fprintf(stderr, "   Return/Enter pressed: triggering action:%p\n", act); 
              act->trigger();
              event->accept();
              //if(!_stayOpen || (!MusEGlobal::config.popupsDefaultStayOpen && (e->modifiers() & Qt::ControlModifier) == 0))
                closeUp();
              return true;    // We handled it.
            }
          }
        }
        break;
       
        default:
        break;
     }
   }
   break;
#endif   // POPUP_MENU_DISABLE_STAY_OPEN

#ifndef POPUP_MENU_DISABLE_AUTO_SCROLL
   case QEvent::MouseMove:
   {
      QMouseEvent* e = static_cast<QMouseEvent*>(event);
      QPoint globPos = e->globalPos();
      int dw = QApplication::desktop()->width();  // We want the whole thing if multiple monitors.
      if(x() < 0 && globPos.x() <= 0)   // If on the very first pixel (or beyond)
      {
         moveDelta = 32;
         if(!timer->isActive())
            timer->start();
         event->accept();
         return true;
      }
      else
         if(x() + width() >= dw && globPos.x() >= (dw -1))   // If on the very last pixel (or beyond)
         {
            moveDelta = -32;
            if(!timer->isActive())
               timer->start();
            event->accept();
            return true;
         }

      if(timer->isActive())
         timer->stop();
      
      //event->accept();
      //return true;
      
      //event->ignore();               // Pass it on
      //return QMenu::event(event);
   }
      break;
#endif  // POPUP_MENU_DISABLE_AUTO_SCROLL

   default:
      break;
   }

   event->ignore();
   return QMenu::event(event);
}

void PopupMenu::closeUp()
{
  // Code adapted from QWidget::mousePressEvent()...
  // NG. This didn't work
//   if((windowType() == Qt::Popup))
//   {
//     QWidget* w;
//     while((w = QApplication::activePopupWidget()) && w != this)
//     {
//       w->close();
//       if(QApplication::activePopupWidget() == w) // widget does not want to disappear
//         w->hide(); // hide at least
//     }
//     close();
//   }
  
  if(isVisible())
  {
    fprintf(stderr, "PopupMenu::closeUp() this:%p closing\n", this);
    close();
  }
  
  QAction* act = menuAction();
  if(act)
  {
    fprintf(stderr, "PopupMenu::closeUp() this:%p menuAction:%p\n", this, act);
    const int sz = act->associatedWidgets().size();
    for(int i = 0; i < sz; ++i)
    {
      fprintf(stderr, "   associated widget#:%d\n", i);
      if(PopupMenu* pup = qobject_cast<PopupMenu*>(act->associatedWidgets().at(i)))
      {
        fprintf(stderr, "   associated popup:%p\n", pup);
        //if(pup->isVisible())
        //{
          fprintf(stderr, "   closing...\n");
          pup->closeUp();
        //}
      }
    }
  }
  //else
//   if(isVisible())
//   {
//     fprintf(stderr, "PopupMenu::closeUp() this:%p closing\n", this);
//     close();
//   }
}

#ifndef POPUP_MENU_DISABLE_AUTO_SCROLL  
void PopupMenu::timerHandler()
{
   //if(!isVisible() || !hasFocus())
   if(!isVisible())
   {
      timer->stop();
      return;
   }

   int dw = QApplication::desktop()->width();  // We want the whole thing if multiple monitors.
   int nx = x() + moveDelta;
   if(moveDelta < 0 && nx + width() < dw)
   {
      timer->stop();
      nx = dw - width();
   }
   else
      if(moveDelta > 0 && nx > 0)
      {
         timer->stop();
         nx = 0;
      }

   move(nx, y());
}
#endif    // POPUP_MENU_DISABLE_AUTO_SCROLL

void PopupMenu::popHovered(QAction* action)
{  
  fprintf(stderr, "PopupMenu::popHovered action text:%s\n", action->text().toLatin1().constData()); // REMOVE Tim. Persistent routes. Added.
   _lastHoveredAction = action;
   hideContextMenu();  
#ifndef POPUP_MENU_DISABLE_AUTO_SCROLL  
   if(action)
   {
      int dw = QApplication::desktop()->width();  // We want the whole thing if multiple monitors.
      QRect r = actionGeometry(action);
      if(x() + r.x() < 0)
         move(-r.x(), y());
      else
         if(r.x() + r.width() + x() > dw)
            move(dw - r.x() - r.width(), y());
   }
#endif    // POPUP_MENU_DISABLE_AUTO_SCROLL
}

void PopupMenu::mousePressEvent(QMouseEvent* e)
{
  fprintf(stderr, "PopupMenu::mousePressEvent this:%p\n", this);  // REMOVE Tim. Persistent routes. Added.
  // REMOVE Tim. Persistent routes. Added. Just a test. 
//   e->ignore();
//   QMenu::mousePressEvent(e);
//   return;
  
  if (_contextMenu && _contextMenu->isVisible())
    _contextMenu->hide();
  e->ignore();
  QMenu::mousePressEvent(e);
}

void PopupMenu::mouseReleaseEvent(QMouseEvent *e)
{
  fprintf(stderr, "PopupMenu::mouseReleaseEvent this:%p\n", this);  // REMOVE Tim. Persistent routes. Added.
  // REMOVE Tim. Persistent routes. Added. Just a test. 
//   e->ignore();
//   QMenu::mouseReleaseEvent(e);
//   return;
   
   if(_contextMenu && _contextMenu->isVisible())
     return;
     
// REMOVE Tim. Persistent routes. Removed. Why not stay-open scrollable menus?
//    if(MusEGlobal::config.scrollableSubMenus)
//    {
//      QMenu::mouseReleaseEvent(e);
//      return;
//    }
   
   QAction* action = actionAt(e->pos());
   if (!(action && action == activeAction() && !action->isSeparator() && action->isEnabled()))
      action=NULL;

#ifdef POPUP_MENU_DISABLE_STAY_OPEN
   if (action && action->menu() != NULL  &&  action->isCheckable())
      action->activate(QAction::Trigger);

   QMenu::mouseReleaseEvent(e);

   if (action && action->menu() != NULL  &&  action->isCheckable())
      close();

   return;

#else
   // Check for Ctrl to stay open.
   if(!_stayOpen || (!MusEGlobal::config.popupsDefaultStayOpen && (e->modifiers() & Qt::ControlModifier) == 0))
   {
      fprintf(stderr, "PopupMenu::mouseReleaseEvent: not stay open\n");  // REMOVE Tim. Persistent routes. Added.
      if (action && action->menu() != NULL  &&  action->isCheckable())
         action->activate(QAction::Trigger);

      e->ignore(); // REMOVE Tim. Persistent routes. Added. Just a test.
      QMenu::mouseReleaseEvent(e);

      if (action && action->menu() != NULL  &&  action->isCheckable())
         close();

      return;
   }

   fprintf(stderr, "PopupMenu::mouseReleaseEvent: stay open\n");  // REMOVE Tim. Persistent routes. Added.
   if (action)
      action->activate(QAction::Trigger);
   else
      QMenu::mouseReleaseEvent(e);

#endif   // POPUP_MENU_DISABLE_STAY_OPEN
}

//-----------------------------------------
// getMenu
// Auto-breakup a too-wide menu.
// NOTE This is a necessary catch-all because X doesn't like too-wide menus, but risky because
//       some callers might depend on all the items being in one menu (using ::actions or ::addActions).
//      So try to avoid that. The overwhelming rule of thumb is that too-wide menus are bad design anyway.
//------------------------------------------
PopupMenu* PopupMenu::getMenu()
{  
   // We want the whole thing if multiple monitors.
   // Resonable to assume if X can show this desktop, it can show a menu with the same width?
   int dw = QApplication::desktop()->width();
   // If we're still only at one column, not much we can do - some item(s) must have had reeeeally long text.
   // Not to worry. Hopefully the auto-scroll will handle it!
   // Use columnCount() + 2 to catch well BEFORE it widens beyond the edge, and leave room for many <More...>
   // TESTED: Not only does the system not appear to like too-wide menus, but also the column count was observed
   //          rolling over to zero repeatedly after it reached 15, simply when adding actions! The action width was 52
   //          the number of items when it first rolled over was around 480 = 884, well below my desktop width of 1366.
   //         Apparently there is a limit on the number of columns - whatever, it made the col count limit necessary:
   if((_cur_col_count > 1 && ((_cur_col_count + 2) * _cur_item_width) >= dw) || _cur_col_count >= 8)
   {
      // This menu is too wide. So make a new one...
      _cur_item_width = 0;
      _cur_col_count = 1;
      QString s(tr("<More...> %1").arg(_cur_menu_count));
      _cur_menu = new PopupMenu(s, this, _stayOpen);
      ++_cur_menu_count;
      addMenu(_cur_menu);
   }
   return _cur_menu;
}

//----------------------------------------------------
// Need to catch these to auto-breakup a too-big menu...
//----------------------------------------------------

QAction* PopupMenu::addAction(const QString& text)
{
   if(MusEGlobal::config.scrollableSubMenus)
   {
      return QMenu::addAction(text);
   }
   QAction* act = static_cast<QMenu*>(getMenu())->addAction(text);
   int w = _cur_menu->actionGeometry(act).width();
   if(w > _cur_item_width)
      _cur_item_width = w;
   int c = _cur_menu->columnCount();
   if(c > _cur_col_count)
      _cur_col_count = c;
   return act;
}

QAction* PopupMenu::addAction(const QIcon& icon, const QString& text)
{
   if(MusEGlobal::config.scrollableSubMenus)
   {
      return QMenu::addAction(icon, text);
   }
   QAction* act = static_cast<QMenu*>(getMenu())->addAction(icon, text);
   int w = _cur_menu->actionGeometry(act).width();
   if(w > _cur_item_width)
      _cur_item_width = w;
   int c = _cur_menu->columnCount();
   if(c > _cur_col_count)
      _cur_col_count = c;
   return act;
}

QAction* PopupMenu::addAction(const QString& text, const QObject* receiver, const char* member, const QKeySequence& shortcut)
{
   if(MusEGlobal::config.scrollableSubMenus)
   {
      return QMenu::addAction(text, receiver, member, shortcut);
   }
   QAction* act = static_cast<QMenu*>(getMenu())->addAction(text, receiver, member, shortcut);
   int w = _cur_menu->actionGeometry(act).width();
   if(w > _cur_item_width)
      _cur_item_width = w;
   int c = _cur_menu->columnCount();
   if(c > _cur_col_count)
      _cur_col_count = c;
   return act;
}

QAction* PopupMenu::addAction(const QIcon& icon, const QString& text, const QObject* receiver, const char* member, const QKeySequence& shortcut)
{
   if(MusEGlobal::config.scrollableSubMenus)
   {
      return QMenu::addAction(icon, text, receiver, member, shortcut);
   }
   QAction* act = static_cast<QMenu*>(getMenu())->addAction(icon, text, receiver, member, shortcut);
   int w = _cur_menu->actionGeometry(act).width();
   if(w > _cur_item_width)
      _cur_item_width = w;
   int c = _cur_menu->columnCount();
   if(c > _cur_col_count)
      _cur_col_count = c;
   return act;
}

void PopupMenu::addAction(QAction* action)
{
   if(MusEGlobal::config.scrollableSubMenus)
   {
      return QMenu::addAction(action);
   }
   static_cast<QMenu*>(getMenu())->addAction(action);
   int w = _cur_menu->actionGeometry(action).width();
   if(w > _cur_item_width)
      _cur_item_width = w;
   int c = _cur_menu->columnCount();
   if(c > _cur_col_count)
      _cur_col_count = c;
}

//----------------
// Context menu
//----------------

// PopupMenuContextData::PopupMenuContextData()
// : _menu(0L), _action(0L)
// {
// }
// 
// PopupMenuContextData::PopupMenuContextData(const PopupMenuContextData& o)
// : _menu(o._menu), _action(o._action)
// {
// }
// 
// PopupMenuContextData::PopupMenuContextData(QPointer<PopupMenu> menu,QPointer<QAction> action)
// : _menu(menu), _action(action)
// {
// }

static void PopupMenuSetActionData(QMenu *context_menu, PopupMenu* menu, QAction* menuAction) 
{
  const QList<QAction*>actions = context_menu->actions();
  for(int i = 0; i < actions.count(); i++)
  {
    QVariant e = actions[i]->data();
    // If it's already a PopupMenuContextData, just update the values.
    if(e.canConvert<PopupMenuContextData>())
      actions[i]->setData(QVariant::fromValue(PopupMenuContextData(menu, menuAction, e.value<PopupMenuContextData>().varValue())));
    // Otherwise bring in the ORIGINAL supplied variant data.
    else
      actions[i]->setData(QVariant::fromValue(PopupMenuContextData(menu, menuAction, e)));
  }
}

QMenu* PopupMenu::contextMenu()
{
  if(!_contextMenu)
    _contextMenu = new QMenu(this);
  return _contextMenu;
}

void PopupMenu::hideContextMenu()
{
  if(!_contextMenu || !_contextMenu->isVisible())
    return;
  _contextMenu->hide();
}

void PopupMenu::showContextMenu(const QPoint &pos)
{
  _highlightedAction = activeAction();
  if(!_highlightedAction)
  {
    PopupMenuSetActionData(_contextMenu, 0, 0);
    return;
  }
  emit aboutToShowContextMenu(this, _highlightedAction, _contextMenu);
  PopupMenuSetActionData(_contextMenu, this, _highlightedAction);
  if(QMenu* subMenu = _highlightedAction->menu())
    QTimer::singleShot(100, subMenu, SLOT(hide()));
  _contextMenu->popup(mapToGlobal(pos));
}

PopupMenu* PopupMenu::contextMenuFocus()
{
  return qobject_cast<PopupMenu*>(QApplication::activePopupWidget());
}

QAction* PopupMenu::contextMenuFocusAction()
{
  if(PopupMenu* menu = qobject_cast<PopupMenu*>(QApplication::activePopupWidget())) 
  {
    if(!menu->_lastHoveredAction) 
      return 0;
    QVariant var = menu->_lastHoveredAction->data();
    PopupMenuContextData ctx = var.value<PopupMenuContextData>();
    Q_ASSERT(ctx.menu() == menu);
    return ctx.action();
  }
  return 0;
}

void PopupMenu::contextMenuEvent(QContextMenuEvent* e)
{
  if(_contextMenu)
  {
    if(e->reason() == QContextMenuEvent::Mouse)
      showContextMenu(e->pos());
    else if(activeAction())
      showContextMenu(actionGeometry(activeAction()).center());

    e->accept();
    return;
  }
  QMenu::contextMenuEvent(e);
}

void PopupMenu::hideEvent(QHideEvent *e)
{
  // REMOVE Tim. Persistent routes. Added. Just a test.
//   e->ignore();
//   QMenu::hideEvent(e);
//   return;
  
  if(_contextMenu && _contextMenu->isVisible())
  {
    // we need to block signals here when the ctxMenu is showing
    // to prevent the QPopupMenu::activated(int) signal from emitting
    // when hiding with a context menu, the user doesn't expect the
    // menu to actually do anything.
    // since hideEvent gets called very late in the process of hiding
    // (deep within QWidget::hide) the activated(int) signal is the
    // last signal to be emitted, even after things like aboutToHide()
    // AJS
    bool blocked = blockSignals(true);
    _contextMenu->hide();
    blockSignals(blocked);
  }
  QMenu::hideEvent(e);
}

/*
//======================
// PopupView
//======================

PopupView::PopupView(QWidget* parent) 
          : QColumnView(parent)
{
  _model= new QStandardItemModel(this);
  // FIXME: After clearing, then re-filling, no items seen.
  // But if setModel is called FOR THE FIRST TIME after clearing the model,
  //  then it works. Calling setModel any time after that does not work.
  setModel(_model);
}

PopupView::~PopupView()
{
  // Make sure to clear the popup so that any child popups are also deleted !
  //popup->clear();
}

void PopupView::clear()
{
  _model->clear();
}
*/ 

} // namespace MusEGui
