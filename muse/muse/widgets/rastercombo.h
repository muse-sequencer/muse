//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: rastercombo.h,v 1.1 2006/01/24 21:19:28 wschweer Exp $
//  (C) Copyright 2006 Werner Schweer (ws@seh.de)
//=========================================================

#ifndef __RASTERCOMBO_H__
#define __RASTERCOMBO_H__

//---------------------------------------------------------
//   RasterCombo
//---------------------------------------------------------

class RasterCombo : public QComboBox {
      Q_OBJECT

   private slots:
      void _rasterChanged(int);

   public slots:
      void setRaster(int);

   signals:
      void rasterChanged(int);

   public:
      RasterCombo(QWidget* parent = 0);
      int raster() const;
      };

#endif

