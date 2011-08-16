//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: cobject.h,v 1.3.2.1 2005/12/11 21:29:24 spamatica Exp $
//
//  (C) Copyright 1999/2000 Werner Schweer (ws@seh.de)
//=========================================================

#ifndef __COBJECT_H__
#define __COBJECT_H__

#include "config.h"

#include <QMainWindow>
#include <list>
#include <QByteArray>

class QMdiSubWindow;
class QFocusEvent;
class QToolBar;
class Xml;
class QAction;

//---------------------------------------------------------
//   TopWin
//---------------------------------------------------------

class TopWin : public QMainWindow
      {
      Q_OBJECT

   public:
      enum ToplevelType { PIANO_ROLL=0, LISTE, DRUM, MASTER, WAVE, //there shall be no
         LMASTER, CLIPLIST, MARKER, SCORE, ARRANGER,               //gaps in the enum!
#ifdef PATCHBAY
         M_PATCHBAY,
#endif /* PATCHBAY */
         TOPLEVELTYPE_LAST_ENTRY //this has to be always the last entry
         };

      ToplevelType type() const { return _type; }


      virtual void readStatus(Xml&);
      virtual void writeStatus(int, Xml&) const;

      static void readConfiguration(ToplevelType, Xml&);
      static void writeConfiguration(ToplevelType, int, Xml&);
      
      
      bool isMdiWin();

      TopWin(ToplevelType t, QWidget* parent=0, const char* name=0, Qt::WindowFlags f = Qt::Window);
         
      bool sharesToolsAndMenu() { return _sharesToolsAndMenu; }
      void shareToolsAndMenu(bool);
      const std::list<QToolBar*>& toolbars() { return _toolbars; }
      
      void addToolBar(QToolBar* toolbar);
      QToolBar* addToolBar(const QString& title);
         
  private:
      QMdiSubWindow* mdisubwin;
      bool _sharesToolsAndMenu;
      std::list<QToolBar*> _toolbars;

      void insertToolBar(QToolBar*, QToolBar*);
      void insertToolBarBreak(QToolBar*);
      void removeToolBar(QToolBar*);
      void removeToolBarBreak(QToolBar*);
      void addToolBar(Qt::ToolBarArea, QToolBar*);

      virtual QMdiSubWindow* createMdiWrapper();
      

  protected:
      QAction* subwinAction;

      ToplevelType _type;

      static int _widthInit[TOPLEVELTYPE_LAST_ENTRY];
      static int _heightInit[TOPLEVELTYPE_LAST_ENTRY];
      static QByteArray _toolbarNonsharedInit[TOPLEVELTYPE_LAST_ENTRY];
      static QByteArray _toolbarSharedInit[TOPLEVELTYPE_LAST_ENTRY];
      static bool initInited;
      
      void initTopwinState();

      bool initalizing; //if true, no state is saved
  
  public slots:
      virtual void hide();
      virtual void show();
      virtual void setVisible(bool);
      void setIsMdiWin(bool);
      void restoreMainwinState();
      void storeInitialState();
  
  signals:
      void toolsAndMenuSharingChanged(bool);
      };


typedef std::list <TopWin*> ToplevelList;
typedef ToplevelList::iterator iToplevel;
typedef ToplevelList::const_iterator ciToplevel;

#endif

