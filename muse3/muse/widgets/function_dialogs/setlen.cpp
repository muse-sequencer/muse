//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: setlen.cpp,v 1.1.1.1 2011/05/05 18:51:04 flo93 Exp $
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
#include "setlen.h"
#include "xml.h"

namespace MusEGui {

int Setlen::range = 1;
int Setlen::len = 384;
  
Setlen::Setlen(QWidget* parent)
	: QDialog(parent)
{
	setupUi(this);
	range_group = new QButtonGroup;
	range_group->addButton(all_events_button,0);
	range_group->addButton(selected_events_button,1);
	range_group->addButton(looped_events_button,2);
	range_group->addButton(selected_looped_button,3);
}

void Setlen::pull_values()
{
	range = range_group->checkedId();
	len = len_spinbox->value();
}

void Setlen::accept()
{
	pull_values();
	QDialog::accept();
}

int Setlen::exec()
{
	if ((range < 0) || (range > 3)) range=0;
	
	range_group->button(range)->setChecked(true);
	len_spinbox->setValue(len);
	
	return QDialog::exec();
}

void Setlen::read_configuration(MusECore::Xml& xml)
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
				else if (tag == "len")
					len=xml.parseInt();
				else
					xml.unknown("SetLen");
				break;
				
			case MusECore::Xml::TagEnd:
				if (tag == "setlen")
					return;
				
			default:
				break;
		}
	}
}

void Setlen::write_configuration(int level, MusECore::Xml& xml)
{
	xml.tag(level++, "setlen");
	xml.intTag(level, "range", range);
	xml.intTag(level, "len", len);
	xml.tag(level, "/setlen");
}

} // namespace MusEGui
