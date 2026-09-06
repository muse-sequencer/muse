//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: popupmenu.cpp,v 1.1.1.1 2010/07/18 03:21:00 terminator356 Exp $
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

#include <QPoint>
#include <QList>
#include <QVariant>
#include <QDesktopWidget>
#include <QApplication>
#include <QStyle>
#include <QFont>
#include <QStyleOptionMenuItem>
#include <QWidgetAction>
#include <QScreen>
#if QT_VERSION >= 0x050a00
#include <QGuiApplication>
#else
#include <QDesktopWidget>
#endif
#include "menutitleitem.h"

#include "popupmenu.h"
#include "gconfig.h"
#include "route.h"

// Forwards from header:
#include <QWidget>
#include <QMouseEvent>
//#include <QContextMenuEvent>
//#include <QHideEvent>
#include <QEvent>

// For debugging output: Uncomment the fprintf section.
#define DEBUG_PRST_ROUTES(dev, format, args...) // fprintf(dev, format, ##args);


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
   _contextMenu = 0;
   _lastHoveredAction = 0;
   _highlightedAction = 0;
   // Menus will trigger! Set to make sure our trigger handlers ignore menus.
   menuAction()->setData(-1);
   _cur_menu = this;
   _cur_menu_count = 1;
   _max_items_in_breakup = 0;
   moveDelta = 0;
   timer = 0;

   connect(this, SIGNAL(hovered(QAction*)), SLOT(popHovered(QAction*)));

   if(MusEGlobal::config.scrollableSubMenus)
   {
      setStyleSheet("QMenu { menu-scrollable: 1; }");
      return;
   }
   
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

void PopupMenu::clearAllChecks() const
{
   QList<QAction*> list = actions();
   for(int i = 0; i < list.size(); ++i)
   {
      QAction* act = list[i];
      if(PopupMenu* menu = qobject_cast<PopupMenu*>(act->menu()))
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
      if(PopupMenu* menu = qobject_cast<PopupMenu*>(act->menu()))
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
  DEBUG_PRST_ROUTES(stderr, "PopupMenu::event:%p activePopupWidget:%p this:%p class:%s event type:%d\n", 
          event, QApplication::activePopupWidget(), this, metaObject()->className(), event->type()); 
   
   switch(event->type())
   {
#ifndef POPUP_MENU_DISABLE_STAY_OPEN
   case QEvent::MouseButtonDblClick:
   {
      if(_stayOpen)
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
     QKeyEvent* e = static_cast<QKeyEvent*>(event);
     switch(e->key())
     {
        case Qt::Key_Space:
          if (!style()->styleHint(QStyle::SH_Menu_SpaceActivatesItem, 0, this))
              break;
        // NOTE: Error suppressor for new gcc 7 'fallthrough' level 3 and 4:
        // FALLTHROUGH
        case Qt::Key_Select:
        case Qt::Key_Return:
        case Qt::Key_Enter:
        {
          if(QAction* act = activeAction())
          {
            const bool stay_open = _stayOpen && (MusEGlobal::config.popupsDefaultStayOpen || (e->modifiers() & Qt::ControlModifier));
            // Stay open? Or does the action have a submenu, but also a checkbox of its own?
            if(stay_open || (act->isEnabled() && act->menu() && act->isCheckable()))
            {
              act->trigger();  // Trigger the action. 
              event->accept();
              if(!stay_open)
                closeUp();
              return true;     // We handled it.
            }
            // Otherwise let ancestor QMenu handle it...
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
      if(!MusEGlobal::config.scrollableSubMenus)
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
      }
   }
   break;
#endif  // POPUP_MENU_DISABLE_AUTO_SCROLL

   default:
      break;
   }

   return QMenu::event(event);
}

void PopupMenu::closeUp()
{
  if(isVisible())
  {
    DEBUG_PRST_ROUTES(stderr, "PopupMenu::closeUp() this:%p closing\n", this);
    close();
  }
  
  QAction* act = menuAction();
  if(act)
  {
    DEBUG_PRST_ROUTES(stderr, "PopupMenu::closeUp() this:%p menuAction:%p\n", this, act);
    const int sz = act->associatedWidgets().size();
    for(int i = 0; i < sz; ++i)
    {
      DEBUG_PRST_ROUTES(stderr, "   associated widget#:%d\n", i);
      if(PopupMenu* pup = qobject_cast<PopupMenu*>(act->associatedWidgets().at(i)))
      {
        DEBUG_PRST_ROUTES(stderr, "   associated popup:%p\n", pup);
        DEBUG_PRST_ROUTES(stderr, "   closing...\n");
        pup->closeUp();
      }
    }
  }
}

PopupMenu* PopupMenu::cloneMenu(const QString& title, QWidget* parent, bool stayOpen, bool showTooltips)
{
  PopupMenu* m = new PopupMenu(title, parent, stayOpen);
  m->setToolTipsVisible(showTooltips);

  // Explicitly mirror the palette or styles to be completely safe across custom engines
  m->setStyle(this->style());
  m->setPalette(this->palette());

  return m;
}

#ifndef POPUP_MENU_DISABLE_AUTO_SCROLL  
void PopupMenu::timerHandler()
{
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
  DEBUG_PRST_ROUTES(stderr, "PopupMenu::popHovered action text:%s\n", action->text().toLocal8Bit().constData());
   _lastHoveredAction = action;
   hideContextMenu();  
#ifndef POPUP_MENU_DISABLE_AUTO_SCROLL  
   if(action && !MusEGlobal::config.scrollableSubMenus)
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
  DEBUG_PRST_ROUTES(stderr, "PopupMenu::mousePressEvent this:%p\n", this);
  if (_contextMenu && _contextMenu->isVisible())
    _contextMenu->hide();
  e->ignore();
  QMenu::mousePressEvent(e);
}

void PopupMenu::mouseReleaseEvent(QMouseEvent *e)
{
  DEBUG_PRST_ROUTES(stderr, "PopupMenu::mouseReleaseEvent this:%p\n", this);
   if(_contextMenu && _contextMenu->isVisible())
     return;
     
// Removed by Tim. Why not stay-open scrollable menus?
//    if(MusEGlobal::config.scrollableSubMenus)
//    {
//      QMenu::mouseReleaseEvent(e);
//      return;
//    }
   
   QAction* action = actionAt(e->pos());
   if (!(action && action == activeAction() && !action->isSeparator() && action->isEnabled()))
      action=nullptr;

#ifdef POPUP_MENU_DISABLE_STAY_OPEN
   if (action && action->menu() != nullptr  &&  action->isCheckable())
      action->activate(QAction::Trigger);

   QMenu::mouseReleaseEvent(e);

   if (action && action->menu() != nullptr  &&  action->isCheckable())
      close();

   return;

#else
   
  // Check for Ctrl to stay open.
  const bool stay_open = _stayOpen && (MusEGlobal::config.popupsDefaultStayOpen || (e->modifiers() & Qt::ControlModifier));
  // Stay open? Or does the action have a submenu, but also a checkbox of its own?
  if(action && (stay_open || (action->isEnabled() && action->menu() && action->isCheckable())))
  {
    DEBUG_PRST_ROUTES(stderr, "PopupMenu::mouseReleaseEvent: stay open\n");
    action->trigger();  // Trigger the action. 
    e->accept();
    if(!stay_open)
      closeUp();
    return;     // We handled it.
  }
  // Otherwise let ancestor QMenu handle it...
  e->ignore();
  QMenu::mouseReleaseEvent(e);

#endif   // POPUP_MENU_DISABLE_STAY_OPEN
}

void PopupMenu::rebuildMenu(PopupMenu* menu)
{
  if(!menu)
    return;

  PopupMenu *curmenu = menu;

  int menu_count = 1;

  // Force the menu to initialize its style and layout properties. (Fonts, padding, etc.)
  curmenu->ensurePolished();

  const QList<QAction*> acts = curmenu->actions();
  const int acts_sz = acts.size();

  // Nothing to do?
  if(acts_sz == 0)
    return;

  const int screenH = _screenSize.height();

  int vmargin = curmenu->style()->pixelMetric(QStyle::PM_MenuVMargin, nullptr, curmenu);
  int fwidth = curmenu->style()->pixelMetric(QStyle::PM_MenuPanelWidth, nullptr, curmenu);

  // Start the height off with the 'decorations'.
  int totalHeight = 2 * vmargin + 2 * fwidth;

  QSize itemSize;
  QSize contentSize;

  for(int i = 0; i < acts_sz; ++i)
  {
    QAction* act = acts[i];

    bool handled = false;

    if(qobject_cast<MenuTitleItem*>(act))
    {
      // We want these title items to be in the top level. Don't touch. Mark as handled.
      handled = true;
      // If the current menu is one that we created.
      if(curmenu != menu)
      {
        // Force the next item to create a new menu to continue where the old one left off.
        totalHeight = screenH;
      }
      // Otherwise allow the item to participate in the height calculations.
      else if(act->isVisible())
      {
        // NOTE: Using actionGeometry() was observed to be quite slow.
        //       For a thousand item menu, 10 seconds to open instead of a few like with other techniques.
        //
        //       But... It is expected that the ratio of title items to actions will be low.
        //       A menu designed with many titles and few actions should be avoided, even if the cause
        //        is unintended sparse dynamic population of actions.

        // Get the geometry from the original top menu.
        const QRect ag = menu->actionGeometry(act);

        // Valid height? (If it's a widget action, the widget might not have been created yet).
        if(ag.height() > 0)
         totalHeight += ag.height();
      }
    }
    else if(qobject_cast<QWidgetAction*>(act))
    {
      // NOTE: Since actionGeometry is slow, let's not try to use it here because it is possible
      //        that many or all of the actions might be widget actions (other than MenuTitleItem).
      //       Also, the widget might not have been created yet.
    }
    // Skip hidden actions.
    else if(act->isVisible())
    {
      QStyleOptionMenuItem option;
      option.initFrom(curmenu);

      if(act->isSeparator())
      {
        option.menuItemType = QStyleOptionMenuItem::Separator;

        itemSize = curmenu->style()->sizeFromContents(QStyle::CT_MenuItem, &option, QSize(0, 0), curmenu);
      }
      else
      {
        option.font = curmenu->font();
        option.fontMetrics = curmenu->fontMetrics();
        option.text = act->text();

        contentSize = option.fontMetrics.size(Qt::TextSingleLine, option.text);
        itemSize = curmenu->style()->sizeFromContents(QStyle::CT_MenuItem, &option, contentSize, curmenu);
      }

      totalHeight += itemSize.height();

      if(totalHeight + 100 >= screenH)
      {
        const QString txt = act->text();

        // This menu is too wide. So make a new one...
        QString s;
        if(txt.isEmpty())
          s = tr("<More...> ") + QString(menu_count);
        else
        {
          // Let's do the elide based on number of characters rather than a pixel width,
          //  so that it is consistent at different fonts. Say, 20 characters?
          s = QString("%1 ...").arg(txt.left(20));
        }

        // FIXME: Should be parentWidget() instead of 'this'. But then sub actions don't respond. What's wrong?
        curmenu = cloneMenu(s, this, _stayOpen, toolTipsVisible());
        //curmenu = cloneMenu(s, parentWidget(), _stayOpen, toolTipsVisible());
        // Make the parent text stand out with bold.
        QFont fnt = curmenu->font();
        fnt.setBold(true);
        curmenu->menuAction()->setFont(fnt);
        ++menu_count;
        insertMenu(act, curmenu);

        // Move the given action from the original menu to the current menu.
        menu->removeAction(act);
        curmenu->addAction(act);
        handled = true;

        // Force the new menu to initialize its style and layout properties.
        curmenu->ensurePolished();

        // Get these again, from the new menu.
        vmargin = curmenu->style()->pixelMetric(QStyle::PM_MenuVMargin, nullptr, curmenu);
        fwidth = curmenu->style()->pixelMetric(QStyle::PM_MenuPanelWidth, nullptr, curmenu);

        // Restart the total height accumulator with the new menu's 'decorations'.
        totalHeight = 2 * vmargin + 2 * fwidth;

        option.initFrom(curmenu);
        option.font = curmenu->font();
        option.fontMetrics = curmenu->fontMetrics();
        option.text = act->text();

        if(act->isSeparator())
        {
          option.menuItemType = QStyleOptionMenuItem::Separator;

          itemSize = curmenu->style()->sizeFromContents(QStyle::CT_MenuItem, &option, QSize(0, 0), curmenu);
        }
        else
        {
          option.font = curmenu->font();
          option.fontMetrics = curmenu->fontMetrics();
          option.text = act->text();

          contentSize = option.fontMetrics.size(Qt::TextSingleLine, option.text);
          itemSize = curmenu->style()->sizeFromContents(QStyle::CT_MenuItem, &option, contentSize, curmenu);
        }

        // Add that to the total height.
        totalHeight += itemSize.height();
      }
    }

    // If not already added and if the current menu is one that we created.
    if(!handled && curmenu != menu)
    {
      // Move the given action from the original menu to the current menu.
      menu->removeAction(act);
      curmenu->addAction(act);
    }

    if(PopupMenu* submenu = qobject_cast<PopupMenu*>(act->menu()))
    {
      if(submenu != curmenu)
        rebuildMenu(submenu);      // Recursive.
    }
  }
}

QAction *PopupMenu::exec()
{
  return exec(pos());
}

QAction *PopupMenu::exec(const QPoint &p, QAction *action)
{
  _screenSize = QSize(200, 200);

  // Version 5.10 and above use QGuiApplication::screenAt().
#if QT_VERSION >= 0x050a00
  const QScreen *scr = QGuiApplication::screenAt(p);
  if(scr)
    _screenSize = scr->availableSize();
#else
  const QDesktopWidget *dw = QApplication::desktop();
  if(dw)
    _screenSize = dw->availableGeometry(p).size();
#endif

  emit aboutToPopup();

  rebuildMenu(this);

  return QMenu::exec(p, action);
}

void PopupMenu::popup(const QPoint &p, QAction *atAction)
{
  _screenSize = QSize(200, 200);

  // Version 5.10 and above use QGuiApplication::screenAt().
#if QT_VERSION >= 0x050a00
  const QScreen *scr = QGuiApplication::screenAt(p);
  if(scr)
    _screenSize = scr->availableSize();
#else
  const QDesktopWidget *dw = QApplication::desktop();
  if(dw)
    _screenSize = dw->availableGeometry(p).size();
#endif

  emit aboutToPopup();

  rebuildMenu(this);

  QMenu::popup(p, atAction);
}

//----------------
// Context menu
//----------------

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
  if(_contextMenu && _contextMenu->isVisible())
  {
    // "we need to block signals here when the ctxMenu is showing
    // to prevent the QPopupMenu::activated(int) signal from emitting
    // when hiding with a context menu, the user doesn't expect the
    // menu to actually do anything.
    // since hideEvent gets called very late in the process of hiding
    // (deep within QWidget::hide) the activated(int) signal is the
    // last signal to be emitted, even after things like aboutToHide()
    // AJS"
    bool blocked = blockSignals(true);
    _contextMenu->hide();
    blockSignals(blocked);
  }
  QMenu::hideEvent(e);
}

} // namespace MusEGui
