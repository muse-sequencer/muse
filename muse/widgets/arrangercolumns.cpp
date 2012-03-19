//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: arrangercolumns.cpp, flo93 $
//
//  (C) Copyright 2012 Florian Jung (florian.a.jung@web.de)
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

#include "arrangercolumns.h"
#include "midictrl.h"
#include "arranger.h"

namespace MusEGui {

ArrangerColumns::ArrangerColumns(QWidget* parent) : QDialog(parent)
{
	ignoreSomethingChanged=true;
	
	setupUi(this);
	
	initList();
	
	connect(ctrlType,SIGNAL(currentIndexChanged(int)), SLOT(ctrlTypeChanged(int)));
	// connect(ctrlType,SIGNAL(activated(int)), SLOT(somethingChanged())); // called by ctrlTypeChanged
	connect(nameEdit,SIGNAL(textEdited(const QString&)), SLOT(somethingChanged()));
	connect(spinBoxHCtrlNo,SIGNAL(valueChanged(int)), SLOT(somethingChanged()));
	connect(spinBoxLCtrlNo,SIGNAL(valueChanged(int)), SLOT(somethingChanged()));
	connect(affectBeginButton,SIGNAL(toggled(bool)), SLOT(somethingChanged()));
	connect(affectCposButton,SIGNAL(toggled(bool)), SLOT(somethingChanged()));
	connect(listWidget,SIGNAL(currentRowChanged(int)), SLOT(itemSelected(int)));
	connect(addBtn,SIGNAL(clicked()), SLOT(addEntry()));
	connect(delBtn,SIGNAL(clicked()), SLOT(delEntry()));
	
	if (listWidget->count()!=0)
		listWidget->setCurrentRow(0);
	else
		itemSelected(-1);
	
	ctrlTypeChanged(ctrlType->currentIndex());
	
	ignoreSomethingChanged=false;
}

void ArrangerColumns::ctrlTypeChanged(int idx)
{
	MusECore::MidiController::ControllerType t = (MusECore::MidiController::ControllerType)idx;

	switch (t)
	{
		case MusECore::MidiController::Controller7:
			spinBoxHCtrlNo->setEnabled(false);
			spinBoxLCtrlNo->setEnabled(true);
			break;

		case MusECore::MidiController::RPN:
		case MusECore::MidiController::NRPN:
		case MusECore::MidiController::Controller14:
		case MusECore::MidiController::RPN14:
		case MusECore::MidiController::NRPN14:
			spinBoxHCtrlNo->setEnabled(true);
			spinBoxLCtrlNo->setEnabled(true);
			break;

		default:
			spinBoxHCtrlNo->setEnabled(false);
			spinBoxLCtrlNo->setEnabled(false);
			break;
	}      

	somethingChanged();
}

void ArrangerColumns::somethingChanged()
{
	if (ignoreSomethingChanged) return;
	
	int row=listWidget->currentRow();
	if (row!=-1)
	{
		int hnum = spinBoxHCtrlNo->value();
		int lnum = spinBoxLCtrlNo->value();
		MusECore::MidiController::ControllerType t = (MusECore::MidiController::ControllerType)ctrlType->currentIndex();
		int ctrl_number = MusECore::MidiController::genNum(t, hnum, lnum);

		Arranger::new_custom_columns[row].name=nameEdit->text();
		Arranger::new_custom_columns[row].ctrl=ctrl_number;
		Arranger::new_custom_columns[row].affected_pos=(affectBeginButton->isChecked() ? Arranger::custom_col_t::AFFECT_BEGIN : Arranger::custom_col_t::AFFECT_CPOS);
		
		listWidget->currentItem()->setText(getListEntryString(row));
	}
}

QString ArrangerColumns::getListEntryString(int row)
{
	return "\""+Arranger::new_custom_columns[row].name+"\": "+MusECore::midiCtrlNumString(Arranger::new_custom_columns[row].ctrl, true);
}

void ArrangerColumns::initList()
{
	listWidget->clear();
	
	for (unsigned int i=0;i<Arranger::new_custom_columns.size(); i++)
		listWidget->addItem(getListEntryString(i));
}

void ArrangerColumns::itemSelected(int i)
{
	ignoreSomethingChanged=true;
	
	if (i==-1)
	{
		frame->setEnabled(false);
		delBtn->setEnabled(false);
	}
	else
	{
		frame->setEnabled(true);
		delBtn->setEnabled(true);
		
		nameEdit->setText(Arranger::new_custom_columns[i].name);
		int num=Arranger::new_custom_columns[i].ctrl;
		ctrlType->setCurrentIndex(MusECore::midiControllerType(num));
		if (spinBoxHCtrlNo->isEnabled())
			spinBoxHCtrlNo->setValue((num & 0xFF00)>>8);
		else
			spinBoxHCtrlNo->setValue(0);
			
		if (spinBoxLCtrlNo->isEnabled())
			spinBoxLCtrlNo->setValue(num & 0xFF);
		else
			spinBoxLCtrlNo->setValue(0);
		
		affectBeginButton->setChecked(Arranger::new_custom_columns[i].affected_pos == Arranger::custom_col_t::AFFECT_BEGIN);
		affectCposButton->setChecked(Arranger::new_custom_columns[i].affected_pos == Arranger::custom_col_t::AFFECT_CPOS);
	}
	
	ignoreSomethingChanged=false;
}

void ArrangerColumns::addEntry()
{
	Arranger::new_custom_columns.push_back(Arranger::custom_col_t(0,QString("?")));
	listWidget->addItem(getListEntryString(listWidget->count()));
	listWidget->setCurrentRow(listWidget->count()-1);
}

void ArrangerColumns::delEntry()
{
	int row=listWidget->currentRow();
	
	if (row!=-1)
	{
		std::vector<Arranger::custom_col_t>::iterator it=Arranger::new_custom_columns.begin();
		advance(it, row);
		Arranger::new_custom_columns.erase(it);

		initList();
		
		if (listWidget->count()>0)
		{
			if (listWidget->count()<=row)
				row=listWidget->count()-1;
			
			listWidget->setCurrentRow(row);
			itemSelected(row);
		}
		else
			itemSelected(-1);
	}
}

} // namespace MusEGui

