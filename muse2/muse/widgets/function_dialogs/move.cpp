//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: move.cpp,v 1.1.1.1 2011/05/05 18:51:04 flo93 Exp $
//  (C) Copyright 2011 Florian Jung (flo93@sourceforge.net)
//=========================================================

#include <QButtonGroup>
#include "move.h"
#include "xml.h"

Move::Move(QWidget* parent)
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


void Move::read_configuration(Xml& xml)
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
				else if (tag == "amount")
					amount=xml.parseInt();
				else
					xml.unknown("Move");
				break;
				
			case Xml::TagEnd:
				if (tag == "move")
					return;
				
			default:
				break;
		}
	}
}

void Move::write_configuration(int level, Xml& xml)
{
	xml.tag(level++, "move");
	xml.intTag(level, "range", range);
	xml.intTag(level, "amount", amount);
	xml.tag(level, "/move");
}
