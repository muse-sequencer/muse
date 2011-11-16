//=========================================================
//  MusE
//  Linux Music Editor
//  structure.h
//  (C) Copyright 2011 Florian Jung (flo93@users.sourceforge.net)
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

#ifndef __STRUCTURE_H__
#define __STRUCTURE_H__

#include "undo.h"
#include <set>

namespace MusECore {
Undo movePartsTotheRight(unsigned int startTick, int moveTick, bool only_selected=false, std::set<Track*>* tracklist=NULL);
Undo partSplitter(unsigned int tick, bool onlySelectedTracks=false);
void adjustGlobalLists(Undo& operations, int startPos, int diff);
void globalCut(bool onlySelectedTracks=false);
void globalInsert(bool onlySelectedTracks=false);
void globalSplit(bool onlySelectedTracks=false);
}

#endif
