//=========================================================
//  MusE
//  Linux Music Editor
//  structure.h
//  (C) Copyright 2011 Florian Jung (flo93@users.sourceforge.net)
//=========================================================

#ifndef __STRUCTURE_H__
#define __STRUCTURE_H__

#include "undo.h"

Undo movePartsTotheRight(unsigned int startTick, int moveTick);
void adjustGlobalLists(Undo& operations, int startPos, int diff);
void globalCut();
void globalInsert();
void globalSplit();
#endif
