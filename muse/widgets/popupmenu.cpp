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

//#include <stdio.h>
#include <QMouseEvent>
#include <QHoverEvent>
#include <QAction>
#include <QPoint>
#include <QDesktopWidget>
#include <QApplication>
//#include <QTimer>

//#include <stdio.h>
//#include <QStandardItemModel>

#include "popupmenu.h"
#include "gconfig.h"
#include "route.h"
 
 
namespace MusEGui {

//======================
// PopupMenu
//======================

//PopupMenu::PopupMenu() 
//{
//  init();
//}

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
  //printf("PopupMenu::init this:%p\n", this);   

  // Menus will trigger! Set to make sure our trigger handlers ignore menus.
  menuAction()->setData(-1);
  
  //_stayOpen = false;
  moveDelta = 0;
  
  #ifndef POPUP_MENU_DISABLE_AUTO_SCROLL  
  timer = new QTimer(this);
  timer->setInterval(100);
  timer->setSingleShot(false);
  connect(this, SIGNAL(hovered(QAction*)), SLOT(popHovered(QAction*)));
  connect(timer, SIGNAL(timeout()), SLOT(timerHandler()));
  #endif   // POPUP_MENU_DISABLE_AUTO_SCROLL
}

// NOTE: Tested all RoutePopupMenu and PopupMenu dtors and a couple of action dtors from our 
//  PixmapButtonsHeaderWidgetAction and PixmapButtonsWidgetAction: 
// This does not appear to be required any more. All submenus and actions are being deleted now.  p4.0.43 
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
  //printf("PopupMenu::event type:%d\n", event->type());   
  
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
      if(_stayOpen)
      //if(_stayOpen && MusEGlobal::config.popupsDefaultStayOpen)
      {
        QKeyEvent* e = static_cast<QKeyEvent*>(event);
        if(e->modifiers() == Qt::NoModifier && e->key() == Qt::Key_Space)
        {
          QAction* act = activeAction();
          if(act)
          {
            act->trigger();
            event->accept();
            return true;    // We handled it.
          }
        }  
      }
    }
    break;
    #endif   // POPUP_MENU_DISABLE_STAY_OPEN
    
    #ifndef POPUP_MENU_DISABLE_AUTO_SCROLL  
    case QEvent::MouseMove:
    {
      QMouseEvent* e = static_cast<QMouseEvent*>(event);
      QPoint globPos = e->globalPos();
      //QPoint pos = e->pos();
      int dw = QApplication::desktop()->width();  // We want the whole thing if multiple monitors.
      
      //printf("PopupMenu::event MouseMove: pos x:%d y:%d  globPos x:%d y:%d\n", 
      //      pos.x(), pos.y(), globPos.x(), globPos.y());  
      
      /*
      //QAction* action = actionAt(globPos);
      QAction* action = actionAt(pos);
      if(action)
      { 
        QRect r = actionGeometry(action);
        //printf(" act x:%d y:%d w:%d h:%d  popup px:%d py:%d pw:%d ph:%d\n", 
        //      r.x(), r.y(), r.width(), r.height(), x(), y(), width(), height());  
              
        //action->hover();      
      }
      */
      
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
      
      event->ignore();               // Pass it on
      //return QMenu::event(event);  
    }  
    break;
    #endif  // POPUP_MENU_DISABLE_AUTO_SCROLL  
    
    default:
    break;
  }
  
  return QMenu::event(event);
}

#ifndef POPUP_MENU_DISABLE_AUTO_SCROLL  
void PopupMenu::timerHandler()
{
  // printf("PopupMenu::timerHandler\n");   
  
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

void PopupMenu::popHovered(QAction* action)
{
  //timer->stop();
  
  //moveDelta = 0;
  if(action)
  {  
    int dw = QApplication::desktop()->width();  // We want the whole thing if multiple monitors.
    
    QRect r = actionGeometry(action);
    //printf("PopupMenu::popHovered x:%d y:%d w:%d h:%d px:%d py:%d pw:%d ph:%d\n", 
    //       r.x(), r.y(), r.width(), r.height(), x(), y(), width(), height());  
    //printf("PopupMenu::popHovered x:%d y:%d w:%d h:%d px:%d py:%d pw:%d ph:%d dtw:%d\n", 
    //      r.x(), r.y(), r.width(), r.height(), x(), y(), width(), height(), dw);  
    //int x = r.x() + ctrlSubPop->x();
    if(x() + r.x() < 0)
      //setGeometry(0, y(), width(), height());      
      //scroll(-x, 0);      
      //move(-r.x() + 32, y());   // Allow some of left column to show so that mouse can move over it.  
      //move(-r.x() + r.width(), y());   // Allow some of left column to show so that mouse can move over it.  
      //moveDelta = x() - r.x() + 32;
      move(-r.x(), y());   
    else
    if(r.x() + r.width() + x() > dw)
      //setGeometry(1200 - r.x() - r.width(), y(), width(), height());      
      //scroll(-x + 1200, 0);      
      //move(dw - r.x() - r.width() - 32, y());  // Allow some of right column to show so that mouse can move over it.       
      //move(dw - r.x(), y());  // Allow some of right column to show so that mouse can move over it.       
      //moveDelta = x() + dw - r.x() - r.width() - 32;
      move(dw - r.x() - r.width(), y());  
  }
      
  //if(moveDelta == 0)
  //  timer->stop();
    
}
#endif    // POPUP_MENU_DISABLE_AUTO_SCROLL

void PopupMenu::mouseReleaseEvent(QMouseEvent *e)
{
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
      if (action && action->menu() != NULL  &&  action->isCheckable())
        action->activate(QAction::Trigger);

      QMenu::mouseReleaseEvent(e);

      if (action && action->menu() != NULL  &&  action->isCheckable())
        close();

      return;
    }  
    
    //printf("PopupMenu::mouseReleaseEvent\n");  
    if (action) 
      action->activate(QAction::Trigger);
    else 
      QMenu::mouseReleaseEvent(e);
      
    #endif   // POPUP_MENU_DISABLE_STAY_OPEN 
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
