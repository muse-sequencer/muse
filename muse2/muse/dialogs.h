//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: dialogs.h,v 1.20.2.19 2011/05/05 20:10 flo93 Exp $
//  (C) Copyright 2011 Florian Jung (flo93@sourceforge.net)
//=========================================================

#ifndef __DIALOGS_H__
#define __DIALOGS_H__

class QWidget;
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
class PasteDialog;

class Xml;

extern GateTime* gatetime_dialog;
extern Velocity* velocity_dialog;
extern Quantize* quantize_dialog;
extern Remove* erase_dialog;
extern DelOverlaps* del_overlaps_dialog;
extern Setlen* set_notelen_dialog;
extern Move* move_notes_dialog;
extern Transpose* transpose_dialog;
extern Crescendo* crescendo_dialog;
extern Legato* legato_dialog;
extern PasteDialog* paste_dialog;

void init_function_dialogs(QWidget* parent);
void read_function_dialog_config(Xml& xml);
void write_function_dialog_config(int level, Xml& xml);

#endif
