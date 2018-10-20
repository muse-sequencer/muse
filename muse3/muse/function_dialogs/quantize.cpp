//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: quantize.cpp,v 1.1.1.1 2011/05/05 18:51:04 flo93 Exp $
//  (C) Copyright 2011 Florian Jung (flo93@sourceforge.net)
//
//  This program is free software; you can redistribute it and/or
//  modify it under the terms of the GNU General Public License
//  as published by the Free Software Foundation; version 2 of
//  the License, or (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//
//=========================================================

#include <QButtonGroup>
#include "quantize.h"
#include "xml.h"

namespace MusEGui {

int Quantize::range = 1;
int Quantize::strength = 90;
int Quantize::threshold = 0;
int Quantize::raster_index = 3;
int Quantize::swing = 0;
bool Quantize::quant_len = 1;


int rasterVals[] = {
  1, // Whole note divisor
  2, // Half note divisor
  4, // 4th note divisor
  6, // 4thT divisor
  8, // 8th divisor
  12,//8thT divisor
  16,// ...
  24,
  32,
  48,
  64
};

Quantize::Quantize(QWidget* parent)
	: QDialog(parent)
{
	setupUi(this);
	range_group = new QButtonGroup;
	range_group->addButton(all_events_button,0);
	range_group->addButton(selected_events_button,1);
	range_group->addButton(looped_events_button,2);
	range_group->addButton(selected_looped_button,3);
}

void Quantize::pull_values()
{
	range = range_group->checkedId();
	strength = strength_spinbox->value();
	threshold = threshold_spinbox->value();
	raster_index = raster_combobox->currentIndex();
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
  raster_combobox->setCurrentIndex(raster_index);
	len_checkbox->setChecked(quant_len);
	swing_spinbox->setValue(swing);
	
	return QDialog::exec();
}

void Quantize::read_configuration(MusECore::Xml& xml)
{
	for (;;)
	{
		MusECore::Xml::Token token = xml.parse();
		if (token == MusECore::Xml::Error || token == MusECore::Xml::End)
			break;
			
		const QString& tag = xml.s1();
		switch (token)
		{
			case MusECore::Xml::TagStart:
				if (tag == "range")
					range=xml.parseInt();
				else if (tag == "strength")
					strength=xml.parseInt();
				else if (tag == "threshold")
					threshold=xml.parseInt();
				else if (tag == "raster")
					raster_index=xml.parseInt();
				else if (tag == "swing")
					swing=xml.parseInt();
				else if (tag == "quant_len")
					quant_len=xml.parseInt();
				else
					xml.unknown("Quantize");
				break;
				
			case MusECore::Xml::TagEnd:
				if (tag == "quantize")
					return;
				
			default:
				break;
		}
	}
}

void Quantize::write_configuration(int level, MusECore::Xml& xml)
{
	xml.tag(level++, "quantize");
	xml.intTag(level, "range", range);
	xml.intTag(level, "strength", strength);
	xml.intTag(level, "threshold", threshold);
  xml.intTag(level, "raster", raster_index);
	xml.intTag(level, "swing", swing);
	xml.intTag(level, "quant_len", quant_len);
	xml.tag(level, "/quantize");
}

} // namespace MusEGui

