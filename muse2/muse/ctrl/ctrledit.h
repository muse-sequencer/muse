//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: ctrledit.h,v 1.4.2.1 2008/05/21 00:28:53 terminator356 Exp $
//  (C) Copyright 1999 Werner Schweer (ws@seh.de)
//=========================================================

#ifndef __CTRL_EDIT_H__
#define __CTRL_EDIT_H__

#include <QWidget>

#include "ctrlcanvas.h"
#include "song.h"

class MidiEditor;
class CtrlView;
class CtrlPanel;
class Xml;

#define CTRL_PANEL_FIXED_WIDTH 40
//---------------------------------------------------------
//   CtrlEdit
//---------------------------------------------------------

class CtrlEdit : public QWidget {
      CtrlCanvas* canvas;
      CtrlPanel* panel;

      Q_OBJECT

   private slots:
      void destroy();

   public slots:
      void setTool(int tool);
      void setXPos(int val)           { canvas->setXPos(val); }
      void setXMag(int val)           { canvas->setXMag(val); }
      void setCanvasWidth(int w);
   signals:
      void timeChanged(unsigned);
      void destroyedCtrl(CtrlEdit*);
      void enterCanvas();
      void yposChanged(int);

   public:
      CtrlEdit(QWidget*, MidiEditor* e, int xmag,
         bool expand = false, const char* name = 0);
      void readStatus(Xml&);
      void writeStatus(int, Xml&);
      };

#endif

