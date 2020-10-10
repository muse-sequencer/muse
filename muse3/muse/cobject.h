//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: cobject.h,v 1.3.2.1 2005/12/11 21:29:24 spamatica Exp $
//
//  (C) Copyright 1999/2000 Werner Schweer (ws@seh.de)
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

#ifndef __COBJECT_H__
#define __COBJECT_H__

#include "config.h"
#include "script_delivery.h"

#include <QMainWindow>
#include <list>
#include <QByteArray>
#include <QString>


// Forward declarations:
class QMdiSubWindow;
class QFocusEvent;
class QCloseEvent;
class QToolBar;
class QAction;

namespace MusECore {
class Xml;
}

namespace MusEGui {

//---------------------------------------------------------
//   TopWin
//---------------------------------------------------------

class TopWin : public QMainWindow
      {
      Q_OBJECT

   public:
      enum ToplevelType { PIANO_ROLL=0, DRUM, MASTER, WAVE, SCORE, ARRANGER, // no gaps in the enum!
#ifdef PATCHBAY
         M_PATCHBAY,
#endif /* PATCHBAY */
         TOPLEVELTYPE_LAST_ENTRY //this has to be always the last entry
         };

      ToplevelType type() const { return _type; }
      static QString typeName(ToplevelType t);
      static QIcon typeIcon(ToplevelType t);

      bool initalizing() const { return _initalizing; }
      bool deleting() const { return _isDeleting; }
      
      virtual void readStatus(MusECore::Xml&);
      virtual void writeStatus(int, MusECore::Xml&) const;

      static void readConfiguration(ToplevelType, MusECore::Xml&);
      static void writeConfiguration(ToplevelType, int, MusECore::Xml&);
      
      virtual void storeSettings();
      
      bool isMdiWin() const;
      QMdiSubWindow* getMdiWin() const { return mdisubwin; }

      TopWin(ToplevelType t, QWidget* parent=nullptr, const char* name=nullptr, Qt::WindowFlags f = Qt::Window);
      virtual ~TopWin();
         
      bool sharesToolsAndMenu() const { return _sharesToolsAndMenu; }
      const std::list<QToolBar*>& toolbars() { return _toolbars; }
      
      virtual void addToolBar(QToolBar* toolbar);
      virtual QToolBar* addToolBar(const QString& title);
      virtual void addToolBarBreak(Qt::ToolBarArea area = Qt::TopToolBarArea);
      virtual void insertToolBar(QToolBar*, QToolBar*);
      virtual void insertToolBarBreak(QToolBar*);
      virtual void removeToolBar(QToolBar*);
      virtual void removeToolBarBreak(QToolBar*);
      virtual void addToolBar(Qt::ToolBarArea, QToolBar*);
      
      void resize(int w, int h);
//      void resize(const QSize&);
 
//      static bool _sharesWhenFree[TOPLEVELTYPE_LAST_ENTRY];
//      static bool _sharesMenusToolbars[TOPLEVELTYPE_LAST_ENTRY];
      static bool _openTabbed[TOPLEVELTYPE_LAST_ENTRY];
 
  private:
      QMdiSubWindow* mdisubwin;
      bool _sharesToolsAndMenu;
      std::list<QToolBar*> _toolbars;
      bool _initalizing;

      void createMdiWrapper();
      
      static void initConfiguration();

  protected:
      QAction* subwinAction;
//      QAction* shareAction;
      QAction* fullscreenAction;

      ToplevelType _type;

      static int _widthInit[TOPLEVELTYPE_LAST_ENTRY];
      static int _heightInit[TOPLEVELTYPE_LAST_ENTRY];
      static QByteArray _toolbarNonsharedInit[TOPLEVELTYPE_LAST_ENTRY];
      static QByteArray _toolbarSharedInit[TOPLEVELTYPE_LAST_ENTRY];
      static bool initInited;
      
      QByteArray _savedToolbarState;

      MusECore::ScriptReceiver _scriptReceiver;

      // Set if close has been called on a TopWin having the WA_DeleteOnClose attribute.
      // The TopWins and any children should ignore any signals such as songChanged
      //  which may cause a crash while deleting.
      bool _isDeleting;  
      
      void finalizeInit();
      void initTopwinState();

  private slots:
      void setFullscreen(bool);
  
  public slots:
      virtual void hide();
      virtual void show();
      virtual void setVisible(bool);
      void setIsMdiWin(bool);
      void shareToolsAndMenu(bool);
      void restoreMainwinState();
      void storeInitialState() const;
      virtual void storeInitialViewState() const { }
      virtual void setWindowTitle (const QString&);
      virtual void focusCanvas() { }
//      virtual void windowStateChanged(Qt::WindowStates oldState, Qt::WindowStates newState);
      };


//---------------------------------------------------------
//   ToplevelList
//---------------------------------------------------------

typedef std::list<TopWin*>::iterator iToplevel;
typedef std::list<TopWin*>::const_iterator ciToplevel;

class ToplevelList : public std::list<TopWin* > {
   public:
        TopWin* findType(TopWin::ToplevelType) const;
      };

} // namespace MusEGui

#endif

