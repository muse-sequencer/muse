//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: metronomgui.h,v 1.3 2005/10/05 17:02:03 lunar_shuttle Exp $
//
//  (C) Copyright 2005 Werner Schweer (ws@seh.de)
//=========================================================

#ifndef __METRONOMGUI_H__
#define __METRONOMGUI_H__

#include "ui_metronogui.h"

class Metronom;

//---------------------------------------------------------
//   MetronomGui
//---------------------------------------------------------

class MetronomGui : public QDialog, public Ui::MetronomBase {
      Q_OBJECT
      Metronom* metronom;

   private slots:
      void beatNoteChanged(int);
      void measureVelocityChanged(int);
      void measureNoteChanged(int);
      void beatVelocityChanged(int);

   public:
      MetronomGui(Metronom*, QWidget* parent=0);
      void init();
      };

#endif

