//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: tools.h,v 1.1.1.1 2003/10/27 18:54:49 wschweer Exp $
//  (C) Copyright 1999 Werner Schweer (ws@seh.de)
//=========================================================

#ifndef __TOOLS_H__
#define __TOOLS_H__

#include <QToolBar>

class Action;

class QAction;
class QPixmap;
class QWidget;

enum Tool {
   PointerTool=1,
   PencilTool=2,
   RubberTool=4,
   CutTool=8,
   ScoreTool=16,
   GlueTool=32,
   QuantTool=64,
   DrawTool=128,
   MuteTool=256,
   AutomationTool=512,
   CursorTool=1024
};

const int arrangerTools = PointerTool | PencilTool | RubberTool | CutTool | GlueTool | MuteTool | AutomationTool;

struct ToolB {
      QPixmap** icon;
      const char* tip;
      const char* ltip;
      };

extern ToolB toolList[];

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

#endif

