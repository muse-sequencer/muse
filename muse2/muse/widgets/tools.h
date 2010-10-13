//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: tools.h,v 1.1.1.1 2003/10/27 18:54:49 wschweer Exp $
//  (C) Copyright 1999 Werner Schweer (ws@seh.de)
//=========================================================

#ifndef __TOOLS_H__
#define __TOOLS_H__

#include <q3toolbar.h>
//Added by qt3to4:
#include <QPixmap>

class Q3Action;
class Action;
class Q3MainWindow;

enum Tool { PointerTool=1, PencilTool=2, RubberTool=4, CutTool=8,
   ScoreTool=16, GlueTool=32, QuantTool=64, DrawTool=128, MuteTool=256};

const int arrangerTools = PointerTool | PencilTool | RubberTool | CutTool | GlueTool | MuteTool;

struct ToolB {
      QPixmap** icon;
      const char* tip;
      const char* ltip;
      };

extern ToolB toolList[];

//---------------------------------------------------------
//   EditToolBar
//---------------------------------------------------------

class EditToolBar : public Q3ToolBar {
      Q_OBJECT
      Action** actions;
      int nactions;

   private slots:
      void toolChanged(Q3Action* action);

   signals:
      void toolChanged(int);

   public slots:
      void set(int id);

   public:
      EditToolBar(Q3MainWindow*, int, const char* name = 0);
      ~EditToolBar();
      int curTool();
      };

#endif

