//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: dialogs.h,v 1.20.2.19 2011/05/05 20:10 flo93 Exp $
//  (C) Copyright 2011 Florian Jung (flo93@sourceforge.net)
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
