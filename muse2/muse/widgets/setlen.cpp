//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: setlen.cpp,v 1.1.1.1 2011/05/05 18:51:04 flo93 Exp $
//  (C) Copyright 2011 Florian Jung (flo93@sourceforge.net)
//=========================================================

#include <QButtonGroup>
#include "setlen.h"


Setlen::Setlen(QWidget* parent)
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

