//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: tools.h,v 1.1.1.1 2003/10/27 18:54:49 wschweer Exp $
//  (C) Copyright 1999 Werner Schweer (ws@seh.de)
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

#ifndef __TOOLS_H__
#define __TOOLS_H__

#include <QToolBar>


class QAction;
class QPixmap;
class QWidget;

namespace MusEGui {

class Action;

enum Tool {
   PointerTool=1,
   PencilTool=2,
   RubberTool=4,
   CutTool=8,
   GlueTool=16,
   RangeTool=32,
   PanTool=64,
   ZoomTool=128,
   ScoreTool=256,
   QuantTool=512,
   DrawTool=1024,
   MuteTool=2048,
   AutomationTool=4096,
   CursorTool=8192
};

const int arrangerTools = PointerTool | PencilTool | RubberTool | CutTool | GlueTool | MuteTool |
                          AutomationTool | PanTool | ZoomTool;

struct ToolB {
      QPixmap** icon;
      const char* tip;
      const char* ltip;
      };

extern ToolB toolList[];
extern const unsigned gNumberOfTools;

//---------------------------------------------------------
//   EditToolBar
//---------------------------------------------------------

class EditToolBar : public QToolBar {
      Q_OBJECT
    
      Action** actions;
      int nactions;

   private slots:
      void toolChanged(QAction* action);

   signals:
      void toolChanged(int);

   public slots:
      void set(int id);

   public:
      //EditToolBar(QMainWindow*, int, const char* name = 0);
      EditToolBar(QWidget* /*parent*/, int /*tools*/, const char* name = 0);  // Needs a parent !
      ~EditToolBar();
      int curTool();
      };

} // namespace MusEGui

#endif

