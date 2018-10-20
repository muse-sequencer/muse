//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: move.cpp,v 1.1.1.1 2011/05/05 18:51:04 flo93 Exp $
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
#include "move.h"
#include "xml.h"

namespace MusEGui {

int Move::range = 1;
int Move::amount = 0;
  
Move::Move(QWidget* parent)
	: QDialog(parent)
{
	setupUi(this);
	range_group = new QButtonGroup;
	range_group->addButton(all_events_button,0);
	range_group->addButton(selected_events_button,1);
	range_group->addButton(looped_events_button,2);
	range_group->addButton(selected_looped_button,3);
}

void Move::pull_values()
{
	range = range_group->checkedId();
	amount = amount_spinbox->value();
}

void Move::accept()
{
	pull_values();
	QDialog::accept();
}

int Move::exec()
{
	if ((range < 0) || (range > 3)) range=0;
	
	range_group->button(range)->setChecked(true);
	amount_spinbox->setValue(amount);
	
	return QDialog::exec();
}


void Move::read_configuration(MusECore::Xml& xml)
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
				else if (tag == "amount")
					amount=xml.parseInt();
				else
					xml.unknown("Move");
				break;
				
			case MusECore::Xml::TagEnd:
				if (tag == "move")
					return;
				
			default:
				break;
		}
	}
}

void Move::write_configuration(int level, MusECore::Xml& xml)
{
	xml.tag(level++, "move");
	xml.intTag(level, "range", range);
	xml.intTag(level, "amount", amount);
	xml.tag(level, "/move");
}

} // namespace MusEGui
