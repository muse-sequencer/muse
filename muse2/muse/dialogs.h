//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: dialogs.h,v 1.20.2.19 2011/05/05 20:10 flo93 Exp $
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

#ifndef __DIALOGS_H__
#define __DIALOGS_H__

class QWidget;
namespace MusEDialog
{
	class GateTime;
	class Velocity;
	class Quantize;
	class Remove;
	class DelOverlaps;
	class Setlen;
	class Move;
	class Transpose;
	class Crescendo;
	class Legato;
}
class PasteDialog; //FINDMICHJETZT put into namespace

class Xml;

extern MusEDialog::GateTime* gatetime_dialog;
extern MusEDialog::Velocity* velocity_dialog;
extern MusEDialog::Quantize* quantize_dialog;
extern MusEDialog::Remove* erase_dialog;
extern MusEDialog::DelOverlaps* del_overlaps_dialog;
extern MusEDialog::Setlen* set_notelen_dialog;
extern MusEDialog::Move* move_notes_dialog;
extern MusEDialog::Transpose* transpose_dialog;
extern MusEDialog::Crescendo* crescendo_dialog;
extern MusEDialog::Legato* legato_dialog;
extern PasteDialog* paste_dialog; //FINDMICHJETZT

void init_function_dialogs(QWidget* parent);
void read_function_dialog_config(Xml& xml);
void write_function_dialog_config(int level, Xml& xml);

#endif
