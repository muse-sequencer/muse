//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: remove.cpp,v 1.1.1.1 2011/05/05 18:51:04 flo93 Exp $
//  (C) Copyright 2011 Florian Jung (flo93@sourceforge.net)
//=========================================================

#include <QButtonGroup>
#include "remove.h"
#include "xml.h"

Remove::Remove(QWidget* parent)
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

void Remove::pull_values()
{
	range = range_group->checkedId();
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
	
	return QDialog::exec();
}

void Remove::read_configuration(Xml& xml)
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
				else
					xml.unknown("Erase");
				break;
				
			case Xml::TagEnd:
				if (tag == "erase")
					return;
				
			default:
				break;
		}
	}
}

void Remove::write_configuration(int level, Xml& xml)
{
	xml.tag(level++, "erase");
	xml.intTag(level, "range", range);
	xml.tag(level, "/erase");
}
