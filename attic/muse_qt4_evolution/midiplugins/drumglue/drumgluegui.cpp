//=========================================================
//  MusE
//  Linux Music Editor
//
//  (C) Copyright 2008 Robert Jonsson (rj@spamatica.se)
//  (C) Copyright 2005- Werner Schweer (ws@seh.de)
// Copyright: See COPYING file that comes with this distribution
//=========================================================

#include "drumgluegui.h"
#include "drumglue.h"
#include "globalinstrumentview.h"

//---------------------------------------------------------
//   DrumGlueGui
//---------------------------------------------------------

DrumGlueGui::DrumGlueGui(DrumGlue* f, QWidget* parent)
  : QDialog(parent)
      {
      drumGlue = f;
      setupUi(this);
      instrumentsTabWidget->clear();
      
      connect (addInstrumentButton, SIGNAL(clicked()), this, SLOT(addInstrument()));
      }

//---------------------------------------------------------
//   init
//---------------------------------------------------------
void DrumGlueGui::init()
      {
      foreach(DrumInstrument *di, drumGlue->drumInstruments) {
			GlobalInstrumentView *giView = new GlobalInstrumentView(drumGlue,this, di->name);
      		instrumentsTabWidget->addTab(giView, di->name);
      		}
      }

//---------------------------------------------------------
//   addInstrument
//---------------------------------------------------------
void DrumGlueGui::addInstrument()
	  {
		bool ok;
		QString text = QInputDialog::getText(this, tr("Instrument name"),
											tr("Name of instrument:"), QLineEdit::Normal,
											"", &ok);
		if (ok && !text.isEmpty()) {
				DrumInstrument *di = new DrumInstrument();
				di->name = text;
				drumGlue->drumInstruments.append(di);
				GlobalInstrumentView *giView = new GlobalInstrumentView(drumGlue,this, text);
				instrumentsTabWidget->addTab(giView, text);
			}
	  }

//---------------------------------------------------------
//   removeInstrument
//---------------------------------------------------------
void DrumGlueGui::removeInstrument()
	  {
 		int ret = QMessageBox::warning(this, tr("Remove instrument"),
					tr("Are you sure you want to remove current instrument?"),
					QMessageBox::No,
					QMessageBox::Yes);
		if (ret == QMessageBox::Yes)
			instrumentsTabWidget->removeTab(instrumentsTabWidget->currentIndex());
	  }
