//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: popupmenu.cpp,v 1.1.1.1 2010/07/18 03:21:00 terminator356 Exp $
//
//  (C) Copyright 1999-2010 Werner Schweer (ws@seh.de)
//
//  PopupMenu sub-class of QMenu created by Tim.
//=========================================================

#include <QMouseEvent>
#include <QAction>
#include <stdio.h>
//#include <QStandardItemModel>

#include "popupmenu.h"
 
//======================
// PopupMenu
//======================

PopupMenu::PopupMenu(QWidget* parent) 
          : QMenu(parent)
{
  // Menus will trigger! Set to make sure our trigger handlers ignore menus.
  menuAction()->setData(-1);
}

PopupMenu::~PopupMenu()
{
  printf("PopupMenu::~PopupMenu\n");  // REMOVE Tim.
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
    
void PopupMenu::mouseReleaseEvent(QMouseEvent *e)
{
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
 
