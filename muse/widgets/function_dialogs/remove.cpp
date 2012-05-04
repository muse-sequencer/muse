//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: remove.cpp,v 1.1.1.1 2011/05/05 18:51:04 flo93 Exp $
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
#include "remove.h"
#include "xml.h"

namespace MusEGui {

int Remove::range = 1;
int Remove::velo_threshold = 16;
bool Remove::velo_thres_used = 0;
int Remove::len_threshold = 12;
bool Remove::len_thres_used = 0;


Remove::Remove(QWidget* parent)
	: QDialog(parent)
{
	setupUi(this);
	range_group = new QButtonGroup;
	range_group->addButton(all_events_button,0);
	range_group->addButton(selected_events_button,1);
	range_group->addButton(looped_events_button,2);
	range_group->addButton(selected_looped_button,3);
}

void Remove::pull_values()
{
	range = range_group->checkedId();
	len_thres_used=len_checkbox->isChecked();
	len_threshold=len_spinbox->value();
	velo_thres_used=velo_checkbox->isChecked();
	velo_threshold=velo_spinbox->value();
}

void Remove::accept()
{
	pull_values();
	QDialog::accept();
}

int Remove::exec()
{
	if ((range < 0) || (range > 3)) range=0;
	
	range_group->button(range)->setChecked(true);
	len_checkbox->setChecked(len_thres_used);
	len_spinbox->setValue(len_threshold);
	velo_checkbox->setChecked(velo_thres_used);
	velo_spinbox->setValue(velo_threshold);
	
	return QDialog::exec();
}

void Remove::read_configuration(MusECore::Xml& xml)
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
				else if (tag == "velo_threshold")
					velo_threshold=xml.parseInt();
				else if (tag == "velo_thres_used")
					velo_thres_used=xml.parseInt();
				else if (tag == "len_threshold")
					len_threshold=xml.parseInt();
				else if (tag == "len_thres_used")
					len_thres_used=xml.parseInt();
				else
					xml.unknown("Erase");
				break;
				
			case MusECore::Xml::TagEnd:
				if (tag == "erase")
					return;
				
			default:
				break;
		}
	}
}

void Remove::write_configuration(int level, MusECore::Xml& xml)
{
	xml.tag(level++, "erase");
	xml.intTag(level, "range", range);
	xml.intTag(level, "velo_threshold", velo_threshold);
	xml.intTag(level, "velo_thres_used", velo_thres_used);
	xml.intTag(level, "len_threshold", len_threshold);
	xml.intTag(level, "len_thres_used", len_thres_used);
	xml.tag(level, "/erase");
}

} // namespace MusEGui

