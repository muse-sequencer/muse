//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: pastedialog.cpp,v 1.1.1.1 2011/05/05 18:51:04 flo93 Exp $
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
#include "pastedialog.h"
#include "xml.h"
#include "gconfig.h"

using MusEGlobal::config;

namespace MusEGui {

int PasteDialog::insert_method = 0;
int PasteDialog::number = 1;
int PasteDialog::raster = 384;
bool PasteDialog::all_in_one_track = 0;
bool PasteDialog::clone = 0;
  
PasteDialog::PasteDialog(QWidget* parent)
	: QDialog(parent)
{
	setupUi(this);
	button_group = new QButtonGroup;
	button_group->addButton(merge_button,0);
	button_group->addButton(move_all_button,1);
	button_group->addButton(move_some_button,2);
	
	raster_spinbox->setSingleStep(config.division);
	
	connect(raster_spinbox, SIGNAL(valueChanged(int)), this, SLOT(raster_changed(int)));
	connect(n_spinbox, SIGNAL(valueChanged(int)), this, SLOT(number_changed(int)));
}

void PasteDialog::pull_values()
{
	insert_method = button_group->checkedId();
	number = n_spinbox->value();
	raster = raster_spinbox->value();
	all_in_one_track = all_in_one_track_checkbox->isChecked();
	clone = clone_checkbox->isChecked();
}

void PasteDialog::accept()
{
	pull_values();
	QDialog::accept();
}

int PasteDialog::exec()
{
	if ((insert_method < 0) || (insert_method > 2)) insert_method=0;
	
	button_group->button(insert_method)->setChecked(true);
	n_spinbox->setValue(number);
	raster_spinbox->setValue(raster);
	all_in_one_track_checkbox->setChecked(all_in_one_track);
	clone_checkbox->setChecked(clone);
	
	return QDialog::exec();
}

QString PasteDialog::ticks_to_quarter_string(int ticks)
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

void PasteDialog::raster_changed(int r)
{
	raster_quarters->setText(ticks_to_quarter_string(r));
	insert_quarters->setText(ticks_to_quarter_string(r*n_spinbox->value()));
}

void PasteDialog::number_changed(int n)
{
	insert_quarters->setText(ticks_to_quarter_string(n*raster_spinbox->value()));
}


void PasteDialog::read_configuration(MusECore::Xml& xml)
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
				if (tag == "insert_method")
					insert_method=xml.parseInt();
				else if (tag == "number")
					number=xml.parseInt();
				else if (tag == "raster")
					raster=xml.parseInt();
				else if (tag == "clone")
					clone=xml.parseInt();
				else if (tag == "all_in_one_track")
					all_in_one_track=xml.parseInt();
				else
					xml.unknown("PasteDialog");
				break;
				
			case MusECore::Xml::TagEnd:
				if (tag == "pastedialog")
					return;
				
			default:
				break;
		}
	}
}

void PasteDialog::write_configuration(int level, MusECore::Xml& xml)
{
	xml.tag(level++, "pastedialog");
	xml.intTag(level, "insert_method", insert_method);
	xml.intTag(level, "number", number);
	xml.intTag(level, "raster", raster);
	xml.intTag(level, "clone", clone);
	xml.intTag(level, "all_in_one_track", all_in_one_track);
	xml.tag(level, "/pastedialog");
}

} // namespace MusEGui
