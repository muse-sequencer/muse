//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: ctrleditor.h,v 1.5 2006/01/28 19:11:20 wschweer Exp $
//  (C) Copyright 2005 Werner Schweer (ws@seh.de)
//=========================================================

#ifndef __CTRL_EDITOR_H__
#define __CTRL_EDITOR_H__

#include "al/pos.h"

class Ctrl;
class TimeCanvas;
class Track;

//---------------------------------------------------------
//   CtrlEditor
//---------------------------------------------------------

class CtrlEditor {
      bool _drawCtrlName;
      int dragx, dragy;
      int lselected;    // cached pixel position of current value
      AL::Pos selected; // current selected controller value

      bool drawRuler;
      QPoint ruler1;
      QPoint ruler2;

      virtual Ctrl* ctrl()     const = 0;
      virtual TimeCanvas* tc() const = 0;
      virtual int cheight()    const = 0;
      virtual Track* track()   const = 0;
      virtual Track* ctrlTrack()   const = 0;

   protected:
      int singlePitch;
      void populateControllerMenu(QMenu* ctrlMenu);

   public:
      CtrlEditor();
      virtual ~CtrlEditor() {}
      void paint(QPainter& p, const QRect& r);
      void setDrawCtrlName(bool val) { _drawCtrlName = val; }
      void mousePress(const QPoint&, int);
      void mouseRelease();
      void mouseMove(const QPoint& pos);
      };

#endif

