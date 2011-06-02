//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: tb1.h,v 1.2 2004/01/11 18:55:37 wschweer Exp $
//  (C) Copyright 1999 Werner Schweer (ws@seh.de)
//=========================================================

#ifndef __TB1_H__
#define __TB1_H__

#include <QToolBar>     

class QToolButton;
class QTableWidget;

class PosLabel;
class PitchLabel;
class Track;
class LabelCombo;

//---------------------------------------------------------
//   Toolbar1
//---------------------------------------------------------

class Toolbar1 : public QToolBar {       
      Q_OBJECT
    
      QToolButton* solo;
      PosLabel* pos;
      PitchLabel* pitch;
      LabelCombo* raster;
      QTableWidget* rlist;
      bool showPitch;
      
   private slots:
      void _rasterChanged(int);

   public slots:
      void setTime(unsigned);
      void setPitch(int);
      void setInt(int);
      void setRaster(int);

   signals:
      void rasterChanged(int);
      void soloChanged(bool);

   public:
      //Toolbar1(QMainWindow* parent = 0, int r=96,
      Toolbar1(QWidget* parent, int r=96,     
         bool showPitch=true);
      void setSolo(bool val);
      void setPitchMode(bool flag);
      };

#endif
