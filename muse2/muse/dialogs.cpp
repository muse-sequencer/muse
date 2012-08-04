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

namespace MusEGui {

GateTime* gatetime_dialog=NULL;
Velocity* velocity_dialog=NULL;
Quantize* quantize_dialog=NULL;
Remove* erase_dialog=NULL;
DelOverlaps* del_overlaps_dialog=NULL;
Setlen* set_notelen_dialog=NULL;
Move* move_notes_dialog=NULL;
Transpose* transpose_dialog=NULL;
Crescendo* crescendo_dialog=NULL;
Legato* legato_dialog=NULL;
PasteDialog* paste_dialog=NULL;
PasteEventsDialog* paste_events_dialog=NULL;

void init_function_dialogs(QWidget* parent)
{
	gatetime_dialog = new GateTime(parent);
	velocity_dialog = new Velocity(parent);
	quantize_dialog = new Quantize(parent);
	erase_dialog = new Remove(parent);
	del_overlaps_dialog = new DelOverlaps(parent);
	set_notelen_dialog = new Setlen(parent);
	move_notes_dialog = new Move(parent);
	transpose_dialog = new Transpose(parent);
	crescendo_dialog = new Crescendo(parent);
	legato_dialog = new Legato(parent);
	paste_dialog = new PasteDialog(parent);
	paste_events_dialog = new PasteEventsDialog(parent);
}

void retranslate_function_dialogs()
{
	gatetime_dialog->retranslateUi(gatetime_dialog);
	velocity_dialog->retranslateUi(velocity_dialog);
	quantize_dialog->retranslateUi(quantize_dialog);
	erase_dialog->retranslateUi(erase_dialog);
	del_overlaps_dialog->retranslateUi(del_overlaps_dialog);
	set_notelen_dialog->retranslateUi(set_notelen_dialog);
	move_notes_dialog->retranslateUi(move_notes_dialog);
	transpose_dialog->retranslateUi(transpose_dialog);
	crescendo_dialog->retranslateUi(crescendo_dialog);
	legato_dialog->retranslateUi(legato_dialog);
	paste_dialog->retranslateUi(paste_dialog);
	paste_events_dialog->retranslateUi(paste_events_dialog);
}

void read_function_dialog_config(MusECore::Xml& xml)
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
				
			case MusECore::Xml::TagEnd:
				if (tag == "dialogs")
					return;
				
			default:
				break;
		}
	}
}

void write_function_dialog_config(int level, MusECore::Xml& xml)
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

}
