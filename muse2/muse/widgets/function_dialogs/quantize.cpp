//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: quantize.cpp,v 1.1.1.1 2011/05/05 18:51:04 flo93 Exp $
//  (C) Copyright 2011 Florian Jung (flo93@sourceforge.net)
//=========================================================

#include <QButtonGroup>
#include "quantize.h"
#include "xml.h"

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

void Quantize::read_configuration(Xml& xml)
{
	for (;;)
	{
		Xml::Token token = xml.parse();
		if (token == Xml::Error || token == Xml::End)
			break;
			
		const QString& tag = xml.s1();
		switch (token)
		{
			case Xml::TagStart:
				if (tag == "range")
					range=xml.parseInt();
				else if (tag == "strength")
					strength=xml.parseInt();
				else if (tag == "threshold")
					threshold=xml.parseInt();
				else if (tag == "raster")
					raster_power2=xml.parseInt();
				else if (tag == "swing")
					swing=xml.parseInt();
				else if (tag == "quant_len")
					quant_len=xml.parseInt();
				else
					xml.unknown("Quantize");
				break;
				
			case Xml::TagEnd:
				if (tag == "quantize")
					return;
				
			default:
				break;
		}
	}
}

void Quantize::write_configuration(int level, Xml& xml)
{
	xml.tag(level++, "quantize");
	xml.intTag(level, "range", range);
	xml.intTag(level, "strength", strength);
	xml.intTag(level, "threshold", threshold);
	xml.intTag(level, "raster", raster_power2);
	xml.intTag(level, "swing", swing);
	xml.intTag(level, "quant_len", quant_len);
	xml.tag(level, "/quantize");
}
