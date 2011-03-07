//=========================================================
//  MusE
//  Linux Music Editor
//
//  (C) Copyright 2008 Robert Jonsson (rj@spamatica.se)
//  (C) Copyright 2005- Werner Schweer (ws@seh.de)
// Copyright: See COPYING file that comes with this distribution
//=========================================================

#ifndef __OUTPUTINSTRUMENTVIEW_H__
#define __OUTPUTINSTRUMENTVIEW_H__

#include "ui_outputinstrumentview.h"

#include "drumglue.h"
//---------------------------------------------------------
//   OutputInstrumentView
//---------------------------------------------------------
class DrumGlue;

class OutputInstrumentView : public QDialog, public Ui::OutputInstrumentViewBase {
      Q_OBJECT
      DrumOutputInstrument *outputInstrument;
   public:
      OutputInstrumentView(DrumOutputInstrument*, QWidget* parent);
   private slots:
      void update();
      };

#endif

