//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: pasteeventsdialog.cpp,v 1.1.1.1 2011/05/05 18:51:04 flo93 Exp $
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
#include "pasteeventsdialog.h"
#include "xml.h"
#include "gconfig.h"

using MusEGlobal::config;

namespace MusEGui {

int PasteEventsDialog::number = 1;
int PasteEventsDialog::raster = 384;
bool PasteEventsDialog::always_new_part = 0;
bool PasteEventsDialog::never_new_part = 0;
unsigned PasteEventsDialog::max_distance = 3072;
bool PasteEventsDialog::into_single_part = 0;
  
PasteEventsDialog::PasteEventsDialog(QWidget* parent)
	: QDialog(parent)
{
	setupUi(this);
	
	raster_spinbox->setSingleStep(config.division);
	
	connect(raster_spinbox, SIGNAL(valueChanged(int)), this, SLOT(raster_changed(int)));
	connect(n_spinbox, SIGNAL(valueChanged(int)), this, SLOT(number_changed(int)));
	connect(max_distance_spinbox, SIGNAL(valueChanged(int)), this, SLOT(max_distance_changed(int)));
	
	into_single_part_allowed=true;
}

void PasteEventsDialog::pull_values()
{
	into_single_part = all_into_selected_part_checkbox->isChecked();
	always_new_part = always_new_button->isChecked();
	never_new_part = never_new_button->isChecked();
	
	int temp = max_distance_spinbox->value();
	if (temp < 0)
		max_distance = 0;
	else
		max_distance = unsigned(temp);
	
	number = n_spinbox->value();
	raster = raster_spinbox->value();
}

void PasteEventsDialog::accept()
{
	pull_values();
	QDialog::accept();
}

int PasteEventsDialog::exec()
{
	all_into_selected_part_checkbox->setChecked(into_single_part && into_single_part_allowed);
	all_into_selected_part_checkbox->setEnabled(into_single_part_allowed);
	into_single_part_allowed=true;
	
	if (always_new_part)
		always_new_button->setChecked(true);
	else if (never_new_part)
		never_new_button->setChecked(true);
	else
		sometimes_into_new_button->setChecked(true);
	
	max_distance_spinbox->setValue(max_distance);
	
	n_spinbox->setValue(number);
	raster_spinbox->setValue(raster);
	
	return QDialog::exec();
}

QString PasteEventsDialog::ticks_to_quarter_string(int ticks)
{
	if (ticks % config.division == 0)
	{
		return tr("%n quarter(s)", "", ticks/config.division);
	}
	else
	{
		double quarters = (double) ticks/config.division;
		bool one = ( quarters > 0.995 && quarters < 1.005 );
		if (one)
			return tr("%1 quarter", "for floating-point arguments like 1.5").arg(quarters, 0, 'f', 2);
		else
			return tr("%1 quarters", "for floating-point arguments like 1.5").arg(quarters, 0, 'f', 2);
	}
}

void PasteEventsDialog::max_distance_changed(int d)
{
	max_distance_quarters->setText(ticks_to_quarter_string(d));
}

void PasteEventsDialog::raster_changed(int r)
{
	raster_quarters->setText(ticks_to_quarter_string(r));
	insert_quarters->setText(ticks_to_quarter_string(r*n_spinbox->value()));
}

void PasteEventsDialog::number_changed(int n)
{
	insert_quarters->setText(ticks_to_quarter_string(n*raster_spinbox->value()));
}


void PasteEventsDialog::read_configuration(MusECore::Xml& xml)
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
				if (tag == "number")
					number=xml.parseInt();
				else if (tag == "raster")
					raster=xml.parseInt();
				else if (tag == "always_new_part")
					always_new_part=xml.parseInt();
				else if (tag == "never_new_part")
					never_new_part=xml.parseInt();
				else if (tag == "max_distance")
					max_distance=xml.parseInt();
				else if (tag == "into_single_part")
					into_single_part=xml.parseInt();
				else
					xml.unknown("PasteEventsDialog");
				break;
				
			case MusECore::Xml::TagEnd:
				if (tag == "pasteeventsdialog")
					return;
				
			default:
				break;
		}
	}
}

void PasteEventsDialog::write_configuration(int level, MusECore::Xml& xml)
{
	xml.tag(level++, "pasteeventsdialog");
	xml.intTag(level, "number", number);
	xml.intTag(level, "raster", raster);
	xml.intTag(level, "always_new_part", always_new_part);
	xml.intTag(level, "never_new_part", never_new_part);
	xml.intTag(level, "max_distance", max_distance);
	xml.intTag(level, "into_single_part", into_single_part);
	xml.tag(level, "/pasteeventsdialog");
}

} // namespace MusEGui
