//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: quantize.cpp,v 1.1.1.1 2011/05/05 18:51:04 flo93 Exp $
//  (C) Copyright 2011 Florian Jung (flo93@sourceforge.net)
//=========================================================

#include <QButtonGroup>
#include "quantize.h"


Quantize::Quantize(QWidget* parent)
	: QDialog(parent)
{
	setupUi(this);
	range_group = new QButtonGroup;
	range_group->addButton(all_events_button,0);
	range_group->addButton(selected_events_button,1);
	range_group->addButton(looped_events_button,2);
	range_group->addButton(selected_looped_button,3);
	
	pull_values();
}

void Quantize::pull_values()
{
	range = range_group->checkedId();
	strength = strength_spinbox->value();
	threshold = threshold_spinbox->value();
	raster_power2 = raster_combobox->currentIndex();
	quant_len = len_checkbox->isChecked();
	swing = swing_spinbox->value();
}

void Quantize::accept()
{
	pull_values();
	QDialog::accept();
}

int Quantize::exec()
{
	if ((range < 0) || (range > 3)) range=0;
	
	range_group->button(range)->setChecked(true);
	strength_spinbox->setValue(strength);
	threshold_spinbox->setValue(threshold);
	raster_combobox->setCurrentIndex(raster_power2);
	len_checkbox->setChecked(quant_len);
	swing_spinbox->setValue(swing);
	
	return QDialog::exec();
}

