//=========================================================
//  MusE
//  Linux Music Editor
//  automation_mode_toolbar.h
//  (C) Copyright 2022 Tim E. Real (terminator356 on users dot sourceforge dot net)
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
#ifndef AUTOMATION_MODE_TOOLBAR_H
#define AUTOMATION_MODE_TOOLBAR_H

#include <QToolBar>

class QAction;
class QActionGroup;
class QWidget;

namespace MusEGui {

//---------------------------------------------------------
//   AutomationModeToolBar
//---------------------------------------------------------

class AutomationModeToolBar : public QToolBar {
      Q_OBJECT

   QActionGroup* interpolateActions;
   QActionGroup* boxActions;
   QAction* optimizeAction;

   private slots:
      void interpolateModeChange(QAction* action);
      void boxModeChange(QAction* action);
      void optimizeChange(bool v);
   public slots:
      void setInterpolateMode(int id);
      void setBoxMode(int id);
      void setOptimize(bool v);

   signals:
      void interpolateModeChanged(int);
      void boxModeChanged(int);
      void optimizeChanged(bool);

   public:
      AutomationModeToolBar(QWidget* /*parent*/, const char* name = 0);  // Needs a parent !
      virtual ~AutomationModeToolBar();
      };

} // namespace MusEGui

#endif // AUTOMATION_MODE_TOOLBAR_H

