//=============================================================================
//  MusE
//  Linux Music Editor
//  $Id:$
//
//  Copyright (C) 2002-2006 by Werner Schweer and others
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License version 2.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//=============================================================================

#include "tools.h"
#include "icons.h"
#include "shortcuts.h"

const char* toolList[TOOLS] = {
      "pointer", "pencil", "eraser", "scissor", "glue", 
      "quantize", "draw", "mute_parts"
      };

//---------------------------------------------------------
//   EditToolBar
//---------------------------------------------------------

EditToolBar::EditToolBar(QMainWindow* parent, int tools)
   : QToolBar(tr("Edit Tools"))
      {
      actionGroup = new QActionGroup(parent);
      actionGroup->setExclusive(true);

      bool first = true;
      for (unsigned i = 0; i < sizeof(toolList)/sizeof(*toolList); ++i) {
            if ((tools & (1 << i))==0)
                  continue;
            QAction* a = getAction(toolList[i], this);
            a->setData(1 << i);
            a->setCheckable(true);
            actionGroup->addAction(a);
            addAction(a);
            if (first) {
                  a->setChecked(true);
                  first = false;
                  }
            }
      connect(actionGroup, SIGNAL(triggered(QAction*)), SLOT(toolChanged(QAction*)));
      }

//---------------------------------------------------------
//   toolChanged
//---------------------------------------------------------

void EditToolBar::toolChanged(QAction* action)
      {
      emit toolChanged(action->data().toInt());
      }

//---------------------------------------------------------
//   set
//---------------------------------------------------------

void EditToolBar::set(int id)
      {
      QList<QAction*> actions = actionGroup->actions();
      int n = actions.size();

      for (int i = 0; i < n; ++i) {
            QAction* action = actions.at(i);
            if (action->data().toInt() == id) {
                  action->setChecked(true);
                  return;
                  }
            }
      }

//---------------------------------------------------------
//   curTool
//---------------------------------------------------------

int EditToolBar::curTool()
      {
      QList<QAction*> actions = actionGroup->actions();
      int n = actions.size();

      for (int i = 0; i < n; ++i) {
            QAction* a = actions.at(i);
            if (a->isChecked())
                  return a->data().toInt();
            }
      return -1;
      }

