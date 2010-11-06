//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: popupmenu.h,v 1.1.1.1 2010/07/18 03:18:00 terminator356 Exp $
//
//  (C) Copyright 1999-2010 Werner Schweer (ws@seh.de)
//
//  PopupMenu sub-class of QMenu created by Tim.
//=========================================================

#ifndef __POPUPMENU_H__
#define __POPUPMENU_H__

#include <QMenu>
//#include <QMouseEvent>
//#include <QColumnView>

class QWidget;
class QMouseEvent;
class QVariant;
class QAction;
//class QStandardItemModel;

class PopupMenu : public QMenu
{
  Q_OBJECT
  
  protected:
    void mouseReleaseEvent(QMouseEvent *);
  
  public:
    PopupMenu(QWidget* parent=0);
    ~PopupMenu();
    void clear();
    QAction* findActionFromData(QVariant);
};


/*
class PopupView : public QColumnView
{
  Q_OBJECT
  private:  
    QStandardItemModel* _model;
    
  protected:
  
  public:
    PopupView(QWidget* parent=0);
    ~PopupView();
    
    void clear();
    QStandardItemModel* model() { return _model; }
};
*/


#endif

