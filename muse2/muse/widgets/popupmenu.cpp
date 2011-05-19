//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: popupmenu.cpp,v 1.1.1.1 2010/07/18 03:21:00 terminator356 Exp $
//
//  (C) Copyright 1999-2010 Werner Schweer (ws@seh.de)
//
//  PopupMenu sub-class of QMenu created by Tim.
//=========================================================

//#include <stdio.h>
#include <QMouseEvent>
#include <QHoverEvent>
#include <QAction>
#include <QPoint>
#include <QDesktopWidget>
#include <QApplication>
//#include <QTimer>

#include <stdio.h>
//#include <QStandardItemModel>

#include "popupmenu.h"
 
 
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

PopupMenu::~PopupMenu()
{
  //printf("PopupMenu::~PopupMenu\n");  
}

void PopupMenu::init()
{
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

void PopupMenu::clear()
{
  QList<QAction*> list = actions();
  for(int i = 0; i < list.size(); ++i)
  {
    QAction* act = list[i];
    QMenu* menu = act->menu();
    if(menu)
    {
      menu->clear();
      act->setMenu(0);  // CHECK: Is this OK?
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

QAction* PopupMenu::findActionFromData(QVariant v)
{
  QList<QAction*> list = actions();
  for(int i = 0; i < list.size(); ++i)
  {
    QAction* act = list[i];
    PopupMenu* menu = (PopupMenu*)act->menu();
    if(menu)
    {
      if(QAction* actm = menu->findActionFromData(v))
        return actm;
    }
    if(act->data() == v)
      return act;
  }
  return 0;
}
    
bool PopupMenu::event(QEvent* event)
{
  //printf("PopupMenu::event type:%d\n", event->type());   // REMOVE Tim.
  
  #ifndef POPUP_MENU_DISABLE_AUTO_SCROLL  
  if(event->type() == QEvent::MouseMove) 
  {
    QMouseEvent* e = static_cast<QMouseEvent*>(event);
    QPoint globPos = e->globalPos();
    //QPoint pos = e->pos();
    int dw = QApplication::desktop()->width();  // We want the whole thing if multiple monitors.
    
    //printf("PopupMenu::event MouseMove: pos x:%d y:%d  globPos x:%d y:%d\n", 
    //      pos.x(), pos.y(), globPos.x(), globPos.y());  // REMOVE Tim.
    
    /*
    //QAction* action = actionAt(globPos);
    QAction* action = actionAt(pos);
    if(action)
    { 
      QRect r = actionGeometry(action);
      //printf(" act x:%d y:%d w:%d h:%d  popup px:%d py:%d pw:%d ph:%d\n", 
      //      r.x(), r.y(), r.width(), r.height(), x(), y(), width(), height());  // REMOVE Tim.
            
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
  #endif      // POPUP_MENU_DISABLE_AUTO_SCROLL
  /*
  else
  if(event->type() == QEvent::HoverEnter) 
  {
    // Nope! Hovering over menu items did not invoke this.
    printf("PopupMenu::event hover\n");   // REMOVE Tim.
    QHoverEvent* he = static_cast<QHoverEvent*>(event);
    QPoint oldPos = he->oldPos();
    QPoint pos = he->pos();
    
    QAction* action = actionAt(pos);
     
    if(action)
    {
      QRect r = actionGeometry(action);
      printf("PopupMenu::event hover: act x:%d y:%d w:%d h:%d  popup px:%d py:%d pw:%d ph:%d\n", 
           r.x(), r.y(), r.width(), r.height(), x(), y(), width(), height());  // REMOVE Tim.
      printf(" pos x:%d y:%d  oldPos px:%d py:%d\n", 
           pos.x(), pos.y(), oldPos.x(), oldPos.y());  // REMOVE Tim.
           
           
    }
    
    
    //return true;
  }
  */ 
   
  return QMenu::event(event);
}

#ifndef POPUP_MENU_DISABLE_AUTO_SCROLL  
void PopupMenu::timerHandler()
{
  // printf("PopupMenu::timerHandler\n");   // REMOVE Tim.
  
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
    //       r.x(), r.y(), r.width(), r.height(), x(), y(), width(), height());  // REMOVE Tim.
    //printf("PopupMenu::popHovered x:%d y:%d w:%d h:%d px:%d py:%d pw:%d ph:%d dtw:%d\n", 
    //      r.x(), r.y(), r.width(), r.height(), x(), y(), width(), height(), dw);  // REMOVE Tim.
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
    #ifdef POPUP_MENU_DISABLE_STAY_OPEN    
    QMenu::mouseReleaseEvent(e);
    return;
    
    #else
    if(!_stayOpen)
    {
      QMenu::mouseReleaseEvent(e);
      return;
    }  
    
    //printf("PopupMenu::mouseReleaseEvent\n");  // REMOVE Tim.
    
    //Q_D(QMenu);
    //if (d->mouseEventTaken(e))
    //    return;

    //d->mouseDown = false;
    //QAction *action = d->actionAt(e->pos());
    QAction *action = actionAt(e->pos());
    
    //for(QWidget *caused = this; caused;) {
    //    if (QMenu *m = qobject_cast<QMenu*>(caused)) {
    //        QAction *currentAction = d->currentAction;
    //        if(currentAction && (!currentAction->isEnabled() || currentAction->menu() || currentAction->isSeparator()))
    //            currentAction = 0;
    //        caused = m->d_func()->causedPopup.widget;
    //        if (m->d_func()->eventLoop)
    //            m->d_func()->syncAction = currentAction; // synchronous operation
    //    } else {
    //        break;
    //    }
    //}
    
    //if (action && action == d->currentAction) {
    if (action && action == activeAction() && !action->isSeparator() && action->isEnabled()) 
    {
        //if (action->menu())
        //    action->menu()->d_func()->setFirstActionActive();
        //else
            //d->activateAction(action, QAction::Trigger);
            action->activate(QAction::Trigger);
    }
    else 
    //if (d->motions > 6) {
    //    d->hideUpToMenuBar();
    //  }
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
 
