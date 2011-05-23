//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: legato.cpp,v 1.1.1.1 2011/05/05 18:51:04 flo93 Exp $
//  (C) Copyright 2011 Florian Jung (flo93@sourceforge.net)
//=========================================================

#include <QButtonGroup>
#include "legato.h"
#include "xml.h"

Legato::Legato(QWidget* parent)
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

void Legato::pull_values()
{
	range = range_group->checkedId();
	min_len = len_spinbox->value();
	allow_shortening = allow_shorten_checkbox->isChecked();
}

void Legato::accept()
{
	pull_values();
	QDialog::accept();
}

int Legato::exec()
{
	if ((range < 0) || (range > 3)) range=0;
	
	range_group->button(range)->setChecked(true);
	len_spinbox->setValue(min_len);
	allow_shorten_checkbox->setChecked(allow_shortening);
	
	return QDialog::exec();
}

void Legato::read_configuration(Xml& xml)
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
				else if (tag == "min_len")
					min_len=xml.parseInt();
				else if (tag == "allow_shortening")
					allow_shortening=xml.parseInt();
				else
					xml.unknown("Legato");
				break;
				
			case Xml::TagEnd:
				if (tag == "legato")
					return;
				
			default:
				break;
		}
	}
}

void Legato::write_configuration(int level, Xml& xml)
{
	xml.tag(level++, "legato");
	xml.intTag(level, "range", range);
	xml.intTag(level, "min_len", min_len);
	xml.intTag(level, "allow_shortening", allow_shortening);
	xml.tag(level, "/legato");
}
