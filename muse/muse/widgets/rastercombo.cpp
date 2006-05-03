//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: rastercombo.cpp,v 1.1 2006/01/24 21:19:28 wschweer Exp $
//  (C) Copyright 2006 Werner Schweer (ws@seh.de)
//=========================================================

#include "rastercombo.h"

static int rasterTable[] = {
      //------                8    4     2
      1, 6, 12, 24, 48,  96, 192, 384,  768, 1536,
      4,  8, 16, 32,  64, 128, 256,  512, 1024,
      9, 18, 36, 72, 144, 288, 576, 1152, 2304
      };

static const char* rasterStrings[] = {
      QT_TR_NOOP("Off"), "3pp", "6pp", "64",  "32",  "16",  "8",  "4",  "2",  "1",
      "2pp", "5pp", "64T", "32T", "16T", "8T", "4T", "2T", "1T",
      "4pp", "7pp", "64.", "32.", "16.", "8.", "4.", "2.", "1."
      };

//---------------------------------------------------------
//   RasterCombo
//---------------------------------------------------------

RasterCombo::RasterCombo(QWidget* parent)
   : QComboBox(parent)
      {
      for (unsigned i = 0; i < sizeof(rasterStrings)/sizeof(*rasterStrings); i++)
            addItem(tr(rasterStrings[i]), i);
      connect(this, SIGNAL(activated(int)), SLOT(_rasterChanged(int)));
      }

//---------------------------------------------------------
//   _rasterChanged
//---------------------------------------------------------

void RasterCombo::_rasterChanged(int idx)
      {
      emit rasterChanged(rasterTable[idx]);
      }

//---------------------------------------------------------
//   raster
//---------------------------------------------------------

int RasterCombo::raster() const
      {
      return rasterTable[currentIndex()];
      }

//---------------------------------------------------------
//   setRaster
//---------------------------------------------------------

void RasterCombo::setRaster(int val)
      {
      for (unsigned i = 0; i < sizeof(rasterTable)/sizeof(*rasterTable); i++) {
            if (val == rasterTable[i]) {
                  setCurrentIndex(i);
                  return;
                  }
            }
      printf("setRaster(%d) not defined\n", val);
      setCurrentIndex(0);
      }


