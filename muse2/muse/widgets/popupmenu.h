//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: popupmenu.h,v 1.1.1.1 2010/07/18 03:18:00 terminator356 Exp $
//
//  (C) Copyright 1999-2010 Werner Schweer (ws@seh.de)
//
//  PopupMenu sub-class of QMenu created by Tim.
//  (C) Copyright 2010-2011 Tim E. Real (terminator356 A T sourceforge D O T net)
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

//#include <QMouseEvent>
//#include <QColumnView>

class QWidget;
class QMouseEvent;
class QVariant;
class QAction;
class QEvent;
//class QTimer;
//class QStandardItemModel;

class PopupMenu : public QMenu
{
  Q_OBJECT
  
    bool _stayOpen;
    #ifndef POPUP_MENU_DISABLE_AUTO_SCROLL  
    QTimer* timer;
    #endif
    int moveDelta;
    void init();
    
  private slots:
    #ifndef POPUP_MENU_DISABLE_AUTO_SCROLL  
    void popHovered(QAction*);
    void timerHandler();
    #endif
  
  protected:
    void mouseReleaseEvent(QMouseEvent *);
    bool event(QEvent*);   
  
  public:
    //PopupMenu();
    PopupMenu(bool stayOpen);
    PopupMenu(QWidget* parent=0, bool stayOpen = false);
    PopupMenu(const QString& title, QWidget* parent = 0, bool stayOpen = false);
    ~PopupMenu();
    void clear();
    QAction* findActionFromData(const QVariant&) const;
    bool stayOpen() const { return _stayOpen; }
    void clearAllChecks() const;
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

