//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: tools.h,v 1.7 2006/01/28 19:11:20 wschweer Exp $
//  (C) Copyright 1999 Werner Schweer (ws@seh.de)
//=========================================================

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

