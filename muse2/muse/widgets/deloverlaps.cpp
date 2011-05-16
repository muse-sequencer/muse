//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: deloverlaps.cpp,v 1.1.1.1 2011/05/05 18:51:04 flo93 Exp $
//  (C) Copyright 2011 Florian Jung (flo93@sourceforge.net)
//=========================================================

#include <QButtonGroup>
#include "deloverlaps.h"
#include "xml.h"


DelOverlaps::DelOverlaps(QWidget* parent)
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

void DelOverlaps::read_configuration(Xml& xml)
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
					xml.unknown("DelOverlaps");
				break;
				
			case Xml::TagEnd:
				if (tag == "del_overlaps")
					return;
				
			default:
				break;
		}
	}
}

void DelOverlaps::write_configuration(int level, Xml& xml)
{
	xml.tag(level++, "del_overlaps");
	xml.intTag(level, "range", range);
	xml.tag(level, "/del_overlaps");
}
