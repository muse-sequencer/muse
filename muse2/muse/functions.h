//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: functions.h,v 1.20.2.19 2011/05/05 20:10 flo93 Exp $
//  (C) Copyright 2011 Florian Jung (flo93@sourceforge.net)
//=========================================================

#ifndef __FUNCTIONS_H__
#define __FUNCTIONS_H__

#include "velocity.h"
#include "quantize.h"
#include "gatetime.h"

#include <set>
#include "part.h"


extern GateTime* gatetime_dialog;
extern Velocity* velocity_dialog;
extern Quantize* quantize_dialog;

void init_function_dialogs(QWidget* parent);


std::set<Part*> partlist_to_set(PartList* pl);

//these functions simply do their job, non-interactively
void modify_velocity(const std::set<Part*>& parts, int range, int rate, int offset=0);
void modify_notelen(const std::set<Part*>& parts, int range, int rate, int offset=0);
void quantize_notes(const std::set<Part*>& parts, int range, int raster, int strength=100, int swing=0, int threshold=0);
void erase_notes(const std::set<Part*>& parts, int range);

//the below functions automatically open the dialog
//they return true if you click "ok" and false if "abort"
bool modify_velocity(const std::set<Part*>& parts);
bool modify_notelen(const std::set<Part*>& parts);
bool quantize_notes(const std::set<Part*>& parts);


#endif
