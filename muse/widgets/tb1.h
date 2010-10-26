//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: tb1.h,v 1.2 2004/01/11 18:55:37 wschweer Exp $
//  (C) Copyright 1999 Werner Schweer (ws@seh.de)
//=========================================================

#ifndef __TB1_H__
#define __TB1_H__

//#include <qwidget.h>
//#include <q3toolbar.h>

#include <QWidget>      // p4.0.4
#include <QToolBar>     //

class PosLabel;
class PitchLabel;
class Track;
class LabelCombo;
class QToolButton;

//---------------------------------------------------------
//   Toolbar1
//---------------------------------------------------------

//class Toolbar1 : public Q3ToolBar {
class Toolbar1 : public QToolBar {       // p4.0.4
      QToolButton* solo;
      PosLabel* pos;
      PitchLabel* pitch;
      LabelCombo* quant;
      LabelCombo* raster;
      bool showPitch;
      Q_OBJECT

   private slots:
      void _rasterChanged(int);
      void _quantChanged(int);

   public slots:
      void setTime(unsigned);
      void setPitch(int);
      void setInt(int);
      void setRaster(int);
      void setQuant(int);

   signals:
      void rasterChanged(int);
      void quantChanged(int);
      void soloChanged(bool);
      void toChanged(int);

   public:
      //Toolbar1(QMainWindow* parent = 0, int r=96,
      Toolbar1(QWidget* parent, int r=96,     
         int q=96, bool showPitch=true);
      void setSolo(bool val);
      void setPitchMode(bool flag);
      };

#endif
