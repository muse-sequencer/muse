//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: deloverlaps.cpp,v 1.1.1.1 2011/05/05 18:51:04 flo93 Exp $
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
#include "deloverlaps.h"
#include "xml.h"

namespace MusEGui {

int DelOverlaps::range = 1;

DelOverlaps::DelOverlaps(QWidget* parent)
	: QDialog(parent)
{
	setupUi(this);
	range_group = new QButtonGroup;
	range_group->addButton(all_events_button,0);
	range_group->addButton(selected_events_button,1);
	range_group->addButton(looped_events_button,2);
	range_group->addButton(selected_looped_button,3);
}

void DelOverlaps::pull_values()
{
	range = range_group->checkedId();
}

void DelOverlaps::accept()
{
	pull_values();
	QDialog::accept();
}

int DelOverlaps::exec()
{
	if ((range < 0) || (range > 3)) range=0;
	
	range_group->button(range)->setChecked(true);
	
	return QDialog::exec();
}

void DelOverlaps::read_configuration(MusECore::Xml& xml)
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
				else
					xml.unknown("DelOverlaps");
				break;
				
			case MusECore::Xml::TagEnd:
				if (tag == "del_overlaps")
					return;
				
			default:
				break;
		}
	}
}

void DelOverlaps::write_configuration(int level, MusECore::Xml& xml)
{
	xml.tag(level++, "del_overlaps");
	xml.intTag(level, "range", range);
	xml.tag(level, "/del_overlaps");
}

} // namespace MusEGui
