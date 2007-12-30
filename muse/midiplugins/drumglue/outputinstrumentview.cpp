//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: filtergui.cpp,v 1.4 2005/11/06 17:49:34 wschweer Exp $
//
//  (C) Copyright 2005 Werner Schweer (ws@seh.de)
//=========================================================

#include "outputinstrumentview.h"
#include "drumglue.h"
#include <QtGui>
//---------------------------------------------------------
//   OutputInstrumentView
//---------------------------------------------------------

OutputInstrumentView::OutputInstrumentView(DrumOutputInstrument* doi, QWidget* parent)
  : QDialog(parent)
      {
      outputInstrument= doi;
      setupUi(this);

	  midiOutputSpinBox->setValue(outputInstrument->outKey);
	  highRangeSlider->setValue(outputInstrument->highestVelocity);
	  lowRangeSlider->setValue(outputInstrument->lowestVelocity);
	  preferWhenFastCheckBox->setChecked(outputInstrument->preferFast);
	  highProbabiltyCheckBox->setChecked(outputInstrument->prefer);


	  connect(midiOutputSpinBox,SIGNAL(valueChanged(int)),this, SLOT(update()));
	  connect(highRangeSlider,SIGNAL(valueChanged(int)),this, SLOT(update()));
	  connect(lowRangeSlider,SIGNAL(valueChanged(int)),this, SLOT(update()));
	  connect(preferWhenFastCheckBox,SIGNAL(stateChanged(int)),this, SLOT(update()));
	  connect(highProbabiltyCheckBox,SIGNAL(stateChanged(int)),this, SLOT(update()));
      }


void OutputInstrumentView::update()
{
	outputInstrument->outKey = midiOutputSpinBox->value();
	outputInstrument->highestVelocity = highRangeSlider->value();
	outputInstrument->lowestVelocity = lowRangeSlider->value();
	outputInstrument->preferFast = preferWhenFastCheckBox->isChecked();
	outputInstrument->prefer = highProbabiltyCheckBox->isChecked();

}
