//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: crescendo.cpp,v 1.1.1.1 2011/05/05 18:51:04 flo93 Exp $
//  (C) Copyright 2011 Florian Jung (flo93@sourceforge.net)
//=========================================================

#include <QButtonGroup>
#include "crescendo.h"
#include "xml.h"

Crescendo::Crescendo(QWidget* parent)
	: QDialog(parent)
{
	setupUi(this);
	range_group = new QButtonGroup;
	range_group->addButton(looped_events_button,2);
	range_group->addButton(selected_looped_button,3);
	
	connect(absolute_button, SIGNAL(toggled(bool)), SLOT(absolute_changed(bool)));
	
	pull_values();
}

void Crescendo::pull_values()
{
	range = range_group->checkedId();
	start_val = start_spinbox->value();
	end_val = end_spinbox->value();
	absolute = absolute_button->isChecked();
}

void Crescendo::accept()
{
	pull_values();
	QDialog::accept();
}

int Crescendo::exec()
{
	if ((range < 2) || (range > 3)) range=3;
	
	range_group->button(range)->setChecked(true);
	start_spinbox->setValue(start_val);
	end_spinbox->setValue(end_val);
	absolute_button->setChecked(absolute);
	absolute_changed(absolute);
	
	return QDialog::exec();
}

void Crescendo::read_configuration(Xml& xml)
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
				else if (tag == "start")
					start_val=xml.parseInt();
				else if (tag == "end")
					end_val=xml.parseInt();
				else if (tag == "absolute")
					absolute=xml.parseInt();
				else
					xml.unknown("Crescendo");
				break;
				
			case Xml::TagEnd:
				if (tag == "crescendo")
					return;
				
			default:
				break;
		}
	}
}

void Crescendo::write_configuration(int level, Xml& xml)
{
	xml.tag(level++, "crescendo");
	xml.intTag(level, "range", range);
	xml.intTag(level, "start", start_val);
	xml.intTag(level, "end", end_val);
	xml.intTag(level, "absolute", absolute);
	xml.tag(level, "/crescendo");
}

void Crescendo::absolute_changed(bool val)
{
	if (val)
	{
		start_spinbox->setMaximum(127);
		start_spinbox->setSuffix("");
		end_spinbox->setMaximum(127);
		end_spinbox->setSuffix("");
	}
	else
	{
		start_spinbox->setMaximum(12700);
		start_spinbox->setSuffix(" %");
		end_spinbox->setMaximum(12700);
		end_spinbox->setSuffix(" %");
	}
}
