//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: functions.cpp,v 1.20.2.19 2011/05/05 20:10 flo93 Exp $
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

#include "dialogs.h"
#include "widgets/function_dialogs/velocity.h"
#include "widgets/function_dialogs/quantize.h"
#include "widgets/function_dialogs/crescendo.h"
#include "widgets/function_dialogs/gatetime.h"
#include "widgets/function_dialogs/remove.h"
#include "widgets/function_dialogs/transpose.h"
#include "widgets/function_dialogs/setlen.h"
#include "widgets/function_dialogs/move.h"
#include "widgets/function_dialogs/deloverlaps.h"
#include "widgets/function_dialogs/legato.h"
#include "widgets/pastedialog.h"
#include "widgets/pasteeventsdialog.h"

#include "xml.h"

#include <iostream>

using namespace std;

MusEDialog::GateTime* gatetime_dialog=NULL;
MusEDialog::Velocity* velocity_dialog=NULL;
MusEDialog::Quantize* quantize_dialog=NULL;
MusEDialog::Remove* erase_dialog=NULL;
MusEDialog::DelOverlaps* del_overlaps_dialog=NULL;
MusEDialog::Setlen* set_notelen_dialog=NULL;
MusEDialog::Move* move_notes_dialog=NULL;
MusEDialog::Transpose* transpose_dialog=NULL;
MusEDialog::Crescendo* crescendo_dialog=NULL;
MusEDialog::Legato* legato_dialog=NULL;
MusEDialog::PasteDialog* paste_dialog=NULL;
MusEDialog::PasteEventsDialog* paste_events_dialog=NULL;

void init_function_dialogs(QWidget* parent)
{
	gatetime_dialog = new MusEDialog::GateTime(parent);
	velocity_dialog = new MusEDialog::Velocity(parent);
	quantize_dialog = new MusEDialog::Quantize(parent);
	erase_dialog = new MusEDialog::Remove(parent);
	del_overlaps_dialog = new MusEDialog::DelOverlaps(parent);
	set_notelen_dialog = new MusEDialog::Setlen(parent);
	move_notes_dialog = new MusEDialog::Move(parent);
	transpose_dialog = new MusEDialog::Transpose(parent);
	crescendo_dialog = new MusEDialog::Crescendo(parent);
	legato_dialog = new MusEDialog::Legato(parent);
	paste_dialog = new MusEDialog::PasteDialog(parent);
	paste_events_dialog = new MusEDialog::PasteEventsDialog(parent);
}

void read_function_dialog_config(Xml& xml)
{
	if (erase_dialog==NULL)
	{
		cout << "ERROR: THIS SHOULD NEVER HAPPEN: read_function_dialog_config() called, but\n"
		        "                                 dialogs are still uninitalized (NULL)!"<<endl;
		return;
	}
		
	for (;;)
	{
		Xml::Token token = xml.parse();
		if (token == Xml::Error || token == Xml::End)
			break;
			
		const QString& tag = xml.s1();
		switch (token)
		{
			case Xml::TagStart:
				if (tag == "mod_len")
					gatetime_dialog->read_configuration(xml);
				else if (tag == "mod_velo")
					velocity_dialog->read_configuration(xml);
				else if (tag == "quantize")
					quantize_dialog->read_configuration(xml);
				else if (tag == "erase")
					erase_dialog->read_configuration(xml);
				else if (tag == "del_overlaps")
					del_overlaps_dialog->read_configuration(xml);
				else if (tag == "setlen")
					set_notelen_dialog->read_configuration(xml);
				else if (tag == "move")
					move_notes_dialog->read_configuration(xml);
				else if (tag == "transpose")
					transpose_dialog->read_configuration(xml);
				else if (tag == "crescendo")
					crescendo_dialog->read_configuration(xml);
				else if (tag == "legato")
					legato_dialog->read_configuration(xml);
				else if (tag == "pastedialog")
					paste_dialog->read_configuration(xml);
				else if (tag == "pasteeventsdialog")
					paste_events_dialog->read_configuration(xml);
				else
					xml.unknown("dialogs");
				break;
				
			case Xml::TagEnd:
				if (tag == "dialogs")
					return;
				
			default:
				break;
		}
	}
}

void write_function_dialog_config(int level, Xml& xml)
{
	xml.tag(level++, "dialogs");

	gatetime_dialog->write_configuration(level, xml);
	velocity_dialog->write_configuration(level, xml);
	quantize_dialog->write_configuration(level, xml);
	erase_dialog->write_configuration(level, xml);
	del_overlaps_dialog->write_configuration(level, xml);
	set_notelen_dialog->write_configuration(level, xml);
	move_notes_dialog->write_configuration(level, xml);
	transpose_dialog->write_configuration(level, xml);
	crescendo_dialog->write_configuration(level, xml);
	legato_dialog->write_configuration(level, xml);
	paste_dialog->write_configuration(level, xml);
	paste_events_dialog->write_configuration(level, xml);

	xml.tag(level, "/dialogs");
}
