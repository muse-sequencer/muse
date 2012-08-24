//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: functions.h,v 1.20.2.19 2011/05/05 20:10 flo93 Exp $
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

#ifndef __FUNCTIONS_H__
#define __FUNCTIONS_H__

#include <set>
#include "part.h"
#include "dialogs.h"
#include <QWidget>

class QString;
class QMimeData;

#define FUNCTION_RANGE_ONLY_SELECTED 1
#define FUNCTION_RANGE_ONLY_BETWEEN_MARKERS 2



namespace MusECore {
class Undo;

std::set<Part*> partlist_to_set(PartList* pl);
std::set<Part*> part_to_set(Part* p);
std::map<Event*, Part*> get_events(const std::set<Part*>& parts, int range);

//these functions simply do their job, non-interactively
bool modify_velocity(const std::set<Part*>& parts, int range, int rate, int offset=0);
bool modify_off_velocity(const std::set<Part*>& parts, int range, int rate, int offset=0);
bool modify_notelen(const std::set<Part*>& parts, int range, int rate, int offset=0);
bool quantize_notes(const std::set<Part*>& parts, int range, int raster, bool len=false, int strength=100, int swing=0, int threshold=0);
bool erase_notes(const std::set<Part*>& parts, int range, int velo_threshold=0, bool velo_thres_used=false, int len_threshold=0, bool len_thres_used=false);
bool delete_overlaps(const std::set<Part*>& parts, int range);
bool set_notelen(const std::set<Part*>& parts, int range, int len);
bool move_notes(const std::set<Part*>& parts, int range, signed int ticks);
bool transpose_notes(const std::set<Part*>& parts, int range, signed int halftonesteps);
bool crescendo(const std::set<Part*>& parts, int range, int start_val, int end_val, bool absolute);
bool legato(const std::set<Part*>& parts, int range, int min_len=1, bool dont_shorten=false);


//the below functions automatically open the dialog
//they return true if you click "ok" and false if "abort"
bool modify_velocity(const std::set<Part*>& parts);
bool modify_notelen(const std::set<Part*>& parts);
bool quantize_notes(const std::set<Part*>& parts);
bool set_notelen(const std::set<Part*>& parts);
bool move_notes(const std::set<Part*>& parts);
bool transpose_notes(const std::set<Part*>& parts);
bool crescendo(const std::set<Part*>& parts);
bool erase_notes(const std::set<Part*>& parts);
bool delete_overlaps(const std::set<Part*>& parts);
bool legato(const std::set<Part*>& parts);

//the below functions operate on selected parts
bool modify_velocity();
bool modify_notelen();
bool quantize_notes();
bool set_notelen();
bool move_notes();
bool transpose_notes();
bool crescendo();
bool erase_notes();
bool delete_overlaps();
bool legato();


//functions for copy'n'paste
void copy_notes(const std::set<Part*>& parts, int range);
bool paste_notes(Part* paste_into_part=NULL); // shows a dialog
void paste_notes(int max_distance=3072, bool always_new_part=false, bool never_new_part=false, Part* paste_into_part=NULL, int amount=1, int raster=3072);
QMimeData* selected_events_to_mime(const std::set<Part*>& parts, int range);
void paste_at(const QString& pt, int pos, int max_distance=3072, bool always_new_part=false, bool never_new_part=false, Part* paste_into_part=NULL, int amount=1, int raster=3072);

//functions for selections
void select_all(const std::set<Part*>& parts);
void select_none(const std::set<Part*>& parts);
void select_invert(const std::set<Part*>& parts);
void select_in_loop(const std::set<Part*>& parts);
void select_not_in_loop(const std::set<Part*>& parts);

//functions for parts
void shrink_parts(int raster=-1); //negative values mean "config.division"
void expand_parts(int raster=-1);
void schedule_resize_all_same_len_clone_parts(Part* part, unsigned new_len, Undo& operations);
void clean_parts();
bool merge_selected_parts();
bool merge_parts(const std::set<Part*>& parts);

} // namespace MusECore

#endif
