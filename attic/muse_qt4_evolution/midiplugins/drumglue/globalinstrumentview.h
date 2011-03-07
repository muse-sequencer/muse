//=========================================================
//  MusE
//  Linux Music Editor
//
//  (C) Copyright 2008 Robert Jonsson (rj@spamatica.se)
//  (C) Copyright 2005- Werner Schweer (ws@seh.de)
// Copyright: See COPYING file that comes with this distribution
//=========================================================

#ifndef __GLOBALINSTRUMENTVIEW_H__
#define __GLOBALINSTRUMENTVIEW_H__

#include "ui_globalinstrumentview.h"

#include "drumglue.h"
//---------------------------------------------------------
//   GlobalInstrumentView
//---------------------------------------------------------
class DrumGlue;

class GlobalInstrumentView : public QWidget, public Ui::GlobalInstrumentViewBase {
      Q_OBJECT

	  DrumGlue *drumGlue;
	  QString instrumentName;
	DrumInstrument *getCurrentOutputInstrument();
	  
   public:
      GlobalInstrumentView(DrumGlue*, QWidget* parent, QString name);
   private slots:
	  void addOutput();
	  void editOutput();
	  void removeOutput();
	  void updateInKey();
	  
   public slots:
	  void updateList();
      };

#endif

