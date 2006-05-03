//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: tb1.h,v 1.12 2006/01/25 16:24:33 wschweer Exp $
//  (C) Copyright 1999 Werner Schweer (ws@seh.de)
//=========================================================

#ifndef __TB1_H__
#define __TB1_H__

namespace Awl {
      class PosLabel;
      class PitchLabel;
      };
namespace AL {
      class Pos;
      };

class RasterCombo;
class QuantCombo;

enum { RANGE_ALL, RANGE_SELECTED, RANGE_LOOPED };

//---------------------------------------------------------
//   Toolbar1
//---------------------------------------------------------

class Toolbar1 : public QToolBar {
      QToolButton* solo;
      Awl::PosLabel* pos;
      Awl::PitchLabel* pitch;
      QuantCombo* quant;
      QComboBox* toList;
      RasterCombo* raster;
      Q_OBJECT

   public slots:
      void setTime(const AL::Pos&, bool);
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
      Toolbar1(int r=96, int q=96, bool showPitch=true);
      void setSolo(bool val);
      void setPitchMode(bool flag);
      void setApplyTo(int);
      };

#endif

