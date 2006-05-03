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

#ifndef __TOOLS_H__
#define __TOOLS_H__

class Action;

#include "awl/tcanvas.h"

const int arrangerTools = PointerTool | PencilTool | RubberTool | DrawTool
	| CutTool | GlueTool | MuteTool;

struct ToolB {
      QPixmap** icon;
      const char* tip;
      const char* ltip;
      };
const int TOOLS = 9;
extern ToolB toolList[TOOLS];

//---------------------------------------------------------
//   EditToolBar
//---------------------------------------------------------

class EditToolBar : public QToolBar {
      Q_OBJECT
      QActionGroup* actionGroup;

   private slots:
      void toolChanged(QAction* action);

   signals:
      void toolChanged(int);

   public slots:
      void set(int id);

   public:
      EditToolBar(QMainWindow*, int);
      int curTool();
      };

#endif

