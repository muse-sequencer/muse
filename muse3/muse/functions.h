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

#include <map>
#include <set>
#include "part.h"
#include "dialogs.h"
#include "type_defs.h"
// REMOVE Tim. citem. Added.
#include "event_tag_list.h"
#include "pos.h"
#include "function_dialog_consts.h"

#include <QWidget>

class QString;
class QMimeData;

#define FUNCTION_RANGE_ONLY_SELECTED 1
#define FUNCTION_RANGE_ONLY_BETWEEN_MARKERS 2
#define FUNCTION_ALL_PARTS 1

namespace MusEGui {
  
struct FunctionDialogMode{
  FunctionDialogElements_t _buttons;
  MusECore::Pos  _pos0;
  MusECore::Pos  _pos1;
  
  FunctionDialogMode() : _buttons(FunctionAllEventsButton | FunctionSelectedEventsButton |
                                  FunctionLoopedButton | FunctionSelectedLoopedButton) { }
  FunctionDialogMode(MusEGui::FunctionDialogElements_t buttons = 
                             FunctionAllEventsButton | FunctionSelectedEventsButton |
                             FunctionLoopedButton | FunctionSelectedLoopedButton,
                     const MusECore::Pos& pos0 = MusECore::Pos(), const MusECore::Pos& pos1 = MusECore::Pos()) :
    _buttons(buttons), _pos0(pos0), _pos1(pos1) { }
};

class FunctionDialogReturnBase
{
  public:
  bool _valid;
  bool _allEvents;
  bool _allParts;
  bool _range;
  MusECore::Pos _pos0;
  MusECore::Pos _pos1;

  FunctionDialogReturnBase() :_valid(false), _allEvents(false), _allParts(false), _range(false) { }
  FunctionDialogReturnBase(bool allEvents, bool allParts,
                        bool useRange = false,
                        const MusECore::Pos& pos0 = MusECore::Pos(), const MusECore::Pos& pos1 = MusECore::Pos()) :
    _valid(true), _allEvents(allEvents),
    _allParts(allParts), _range(useRange), _pos0(pos0), _pos1(pos1) { }
};

class FunctionDialogReturnErase : public FunctionDialogReturnBase
{
  public:
  bool _veloThresUsed;
  int  _veloThreshold;
  bool _lenThresUsed;
  int  _lenThreshold;

  FunctionDialogReturnErase() : FunctionDialogReturnBase(),
      _veloThresUsed(false), _veloThreshold(0),
      _lenThresUsed(false), _lenThreshold(0) { }
  FunctionDialogReturnErase(bool allEvents, bool allParts,
                        bool useRange = false,
                        const MusECore::Pos& pos0 = MusECore::Pos(), const MusECore::Pos& pos1 = MusECore::Pos(),
                        bool veloThresUsed = false, int veloThreshold = 0,
                        bool lenThresUsed = false, int lenThreshold = 0) :
          FunctionDialogReturnBase(allEvents, allParts, useRange, pos0, pos1), 
    _veloThresUsed(veloThresUsed), _veloThreshold(veloThreshold),
    _lenThresUsed(lenThresUsed), _lenThreshold(lenThreshold) { }
};
  
class FunctionDialogReturnCrescendo : public FunctionDialogReturnBase
{
  public:

  int _start_val;
  int _end_val;
  bool _absolute;

  FunctionDialogReturnCrescendo() : FunctionDialogReturnBase(),
      _start_val(0), _end_val(0), _absolute(false) { }
  FunctionDialogReturnCrescendo(bool allEvents, bool allParts,
                        bool useRange = false,
                        const MusECore::Pos& pos0 = MusECore::Pos(), const MusECore::Pos& pos1 = MusECore::Pos(),
                        int startVal = 0, int endVal = 0,
                        bool absolute = false) :
          FunctionDialogReturnBase(allEvents, allParts, useRange, pos0, pos1), 
    _start_val(startVal), _end_val(endVal),
    _absolute(absolute) { }
};
  
class FunctionDialogReturnDelOverlaps : public FunctionDialogReturnBase
{
  public:

  FunctionDialogReturnDelOverlaps() : FunctionDialogReturnBase() { }
  FunctionDialogReturnDelOverlaps(bool allEvents, bool allParts,
                        bool useRange = false,
                        const MusECore::Pos& pos0 = MusECore::Pos(), const MusECore::Pos& pos1 = MusECore::Pos()) :
          FunctionDialogReturnBase(allEvents, allParts, useRange, pos0, pos1) { }
};
  
class FunctionDialogReturnGateTime : public FunctionDialogReturnBase
{
  public:

  int _rateVal;
  int _offsetVal;

  FunctionDialogReturnGateTime() : FunctionDialogReturnBase(), _rateVal(0), _offsetVal(0) { }
  FunctionDialogReturnGateTime(bool allEvents, bool allParts,
                        bool useRange = false,
                        const MusECore::Pos& pos0 = MusECore::Pos(), const MusECore::Pos& pos1 = MusECore::Pos(),
                        int rateVal = 0, int offsetVal = 0) :
          FunctionDialogReturnBase(allEvents, allParts, useRange, pos0, pos1), 
    _rateVal(rateVal), _offsetVal(offsetVal) { }
};
  
class FunctionDialogReturnLegato : public FunctionDialogReturnBase
{
  public:

  int _min_len;
  bool _allow_shortening;

  FunctionDialogReturnLegato() : FunctionDialogReturnBase(),
      _min_len(0), _allow_shortening(false) { }
  FunctionDialogReturnLegato(bool allEvents, bool allParts,
                        bool useRange = false,
                        const MusECore::Pos& pos0 = MusECore::Pos(), const MusECore::Pos& pos1 = MusECore::Pos(),
                        int minLen = 0, bool allowShortening = false) :
          FunctionDialogReturnBase(allEvents, allParts, useRange, pos0, pos1), 
    _min_len(minLen), _allow_shortening(allowShortening) { }
};
  
class FunctionDialogReturnMove : public FunctionDialogReturnBase
{
  public:

  int _amount;

  FunctionDialogReturnMove() : FunctionDialogReturnBase(), _amount(0) { }
  FunctionDialogReturnMove(bool allEvents, bool allParts,
                        bool useRange = false,
                        const MusECore::Pos& pos0 = MusECore::Pos(), const MusECore::Pos& pos1 = MusECore::Pos(),
                        int amount = 0) :
          FunctionDialogReturnBase(allEvents, allParts, useRange, pos0, pos1), 
    _amount(amount) { }
};
  
class FunctionDialogReturnQuantize : public FunctionDialogReturnBase
{
  public:

  int _strength;
  int _threshold;
  int _raster_index;
  int _swing;
  bool _quant_len;

  FunctionDialogReturnQuantize() : FunctionDialogReturnBase(),
      _strength(0), _threshold(0), _raster_index(0), _swing(0), _quant_len(false) { }
  FunctionDialogReturnQuantize(bool allEvents, bool allParts,
                        bool useRange = false,
                        const MusECore::Pos& pos0 = MusECore::Pos(), const MusECore::Pos& pos1 = MusECore::Pos(),
                        int strength = 0, int threshold = 0, int rasterIndex = 0,
                        int swing = 0, bool quantLen = false) :
          FunctionDialogReturnBase(allEvents, allParts, useRange, pos0, pos1), 
    _strength(strength), _threshold(threshold), _raster_index(rasterIndex),
    _swing(swing), _quant_len(quantLen) { }
};
  
class FunctionDialogReturnSetLen : public FunctionDialogReturnBase
{
  public:

  int _len;

  FunctionDialogReturnSetLen() : FunctionDialogReturnBase(), _len(0) { }
  FunctionDialogReturnSetLen(bool allEvents, bool allParts,
                        bool useRange = false,
                        const MusECore::Pos& pos0 = MusECore::Pos(), const MusECore::Pos& pos1 = MusECore::Pos(),
                        int len = 0) :
          FunctionDialogReturnBase(allEvents, allParts, useRange, pos0, pos1), 
    _len(len) { }
};
  
class FunctionDialogReturnTranspose : public FunctionDialogReturnBase
{
  public:

  int _amount;

  FunctionDialogReturnTranspose() : FunctionDialogReturnBase(), _amount(0) { }
  FunctionDialogReturnTranspose(bool allEvents, bool allParts,
                        bool useRange = false,
                        const MusECore::Pos& pos0 = MusECore::Pos(), const MusECore::Pos& pos1 = MusECore::Pos(),
                        int amount = 0) :
          FunctionDialogReturnBase(allEvents, allParts, useRange, pos0, pos1), 
    _amount(amount) { }
};
  
class FunctionDialogReturnVelocity : public FunctionDialogReturnBase
{
  public:

  int _rateVal;
  int _offsetVal;

  FunctionDialogReturnVelocity() : FunctionDialogReturnBase(), _rateVal(0), _offsetVal(0) { }
  FunctionDialogReturnVelocity(bool allEvents, bool allParts,
                        bool useRange = false,
                        const MusECore::Pos& pos0 = MusECore::Pos(), const MusECore::Pos& pos1 = MusECore::Pos(),
                        int rateVal = 0, int offsetVal = 0) :
          FunctionDialogReturnBase(allEvents, allParts, useRange, pos0, pos1), 
    _rateVal(rateVal), _offsetVal(offsetVal) { }
};
  
  //the below functions automatically open the dialog
  //they return true if you click "ok" and false if "abort"
  FunctionDialogReturnErase       erase_items_dialog      (const FunctionDialogMode&);
  FunctionDialogReturnCrescendo   crescendo_items_dialog  (const FunctionDialogMode&);
  FunctionDialogReturnDelOverlaps deloverlaps_items_dialog(const FunctionDialogMode&);
  FunctionDialogReturnGateTime    gatetime_items_dialog   (const FunctionDialogMode&);
  FunctionDialogReturnLegato      legato_items_dialog     (const FunctionDialogMode&);
  FunctionDialogReturnMove        move_items_dialog       (const FunctionDialogMode&);
  FunctionDialogReturnQuantize    quantize_items_dialog   (const FunctionDialogMode&);
  FunctionDialogReturnSetLen      setlen_items_dialog     (const FunctionDialogMode&);
  FunctionDialogReturnTranspose   transpose_items_dialog  (const FunctionDialogMode&);
  FunctionDialogReturnVelocity    velocity_items_dialog   (const FunctionDialogMode&);
  
} // namespace MusEGui



namespace MusECore {
class Undo;

std::set<const Part*> partlist_to_set(PartList* pl);
std::set<const Part*> part_to_set(const Part* p);
std::map<const Event*, const Part*> get_events(const std::set<const Part*>& parts,
         int range, MusECore::RelevantSelectedEvents_t relevant = MusECore::NotesRelevant);

//these functions simply do their job, non-interactively
bool modify_velocity(const std::set<const Part*>& parts, int range, int rate, int offset=0);
bool modify_off_velocity(const std::set<const Part*>& parts, int range, int rate, int offset=0);
bool modify_notelen(const std::set<const Part*>& parts, int range, int rate, int offset=0);
bool quantize_notes(const std::set<const Part*>& parts, int range, int raster, bool len=false, int strength=100, int swing=0, int threshold=0);
bool erase_notes(const std::set<const Part*>& parts, int range, int velo_threshold=0, bool velo_thres_used=false, int len_threshold=0, bool len_thres_used=false);
bool delete_overlaps(const std::set<const Part*>& parts, int range);
bool set_notelen(const std::set<const Part*>& parts, int range, int len);
bool move_notes(const std::set<const Part*>& parts, int range, signed int ticks);
bool transpose_notes(const std::set<const Part*>& parts, int range, signed int halftonesteps);
bool crescendo(const std::set<const Part*>& parts, int range, int start_val, int end_val, bool absolute);
bool legato(const std::set<const Part*>& parts, int range, int min_len=1, bool dont_shorten=false);




// bool modify_velocity_items(int rate, int offset=0);
// bool modify_off_velocity_items(int rate, int offset=0);
// bool modify_notelen_items(int rate, int offset=0);
// bool quantize_items(int raster, bool quant_len=false, int strength=100, int swing=0, int threshold=0);
// bool erase_items(int velo_threshold=0, bool velo_thres_used=false, int len_threshold=0, bool len_thres_used=false);
// bool delete_overlaps_items();
// bool set_notelen_items(int len);
// bool move_items(signed int ticks);
// bool transpose_items(signed int halftonesteps);
// bool crescendo_items(int start_val, int end_val, bool absolute);
// bool legato_items(int min_len=1, bool dont_shorten=false);


bool modify_velocity_items(TagEventList* tag_list, int rate, int offset=0);
bool modify_off_velocity_items(TagEventList* tag_list, int rate, int offset=0);
bool modify_notelen_items(TagEventList* tag_list, int rate, int offset=0);
bool quantize_items(TagEventList* tag_list, int raster, bool quant_len=false, int strength=100, int swing=0, int threshold=0);
bool erase_items(TagEventList* tag_list, int velo_threshold=0, bool velo_thres_used=false, int len_threshold=0, bool len_thres_used=false);
bool delete_overlaps_items(TagEventList* tag_list);
bool set_notelen_items(TagEventList* tag_list, int len);
bool move_items(TagEventList* tag_list, signed int ticks);
bool transpose_items(TagEventList* tag_list, signed int halftonesteps);
bool crescendo_items(TagEventList* tag_list, int start_val, int end_val, bool absolute);
bool legato_items(TagEventList* tag_list, int min_len=1, bool dont_shorten=false);


//------------------------------------------------------------------------
// NOTE: The copy/cut/paste functions are for event lists such as found in
//  the various editors like pianoroll and drum editor.
// A different set of copy and paste functions are used for the
//  part canvas (in the arranger). Use them for copying and pasting parts.
// TODO TODO Unify those part copy/cut/paste routines into these routines !
//------------------------------------------------------------------------

// void copy_items();
void copy_items(TagEventList* tag_list);
// bool cut_items();
bool cut_items(TagEventList* tag_list);
// QMimeData* cut_or_copy_tagged_items_to_mime(bool cut_mode = false /*, bool untag_when_done = true*/);
QMimeData* cut_or_copy_tagged_items_to_mime(TagEventList* tag_list, bool cut_mode = false);

bool paste_items(const std::set<const Part*>& parts, const Part* paste_into_part=NULL); // shows a dialog
void paste_items(const std::set<const Part*>& parts, int max_distance=3072,
                 // Options. Default is erase target existing controllers first + erase wysiwyg.
                 const FunctionOptionsStruct& options = FunctionOptionsStruct(),
                 // Paste into this part instead of the original part(s).
                 const Part* paste_into_part=NULL,
                 // Number of copies to paste.
                 int amount=1,
                 // Separation between copies.
                 int raster=3072,
                 // Choose which events to paste.
                 RelevantSelectedEvents_t relevant = AllEventsRelevant,
                 // If pasting controllers, paste into this controller number if not -1.
                 // If the source has multiple controllers, user will be asked which one to paste.
                 int paste_to_ctrl_num = -1
                 );

//void paste_items_at(const std::set<const Part*>& parts, const QString& pt, int pos, int max_distance=3072,
void paste_items_at(
  // List of parts from which to look for original part(s).
  const std::set<const Part*>& parts,
  // Text Xml list of parts and events to paste.
  const QString& pt,
  // Position to paste at.
  const Pos& pos,
  // Distance at which destination part is too far away
  //  (too soon, to the left) so a new part will be created.
  int max_distance=3072,
  // Erase target existing controllers first + erase wysiwyg.
  const FunctionOptionsStruct& options = FunctionOptionsStruct(),
  // Paste into this part instead of the original part(s).
  const Part* paste_into_part=NULL,
  // Number of copies to paste.
  int amount=1,
  // Separation between copies.
  int raster=3072,
  // Choose which events to paste.
  RelevantSelectedEvents_t relevant = AllEventsRelevant,
  // If pasting controllers, paste into this controller number if not -1.
  // If the source has multiple controllers, user will be asked which one to paste.
  int paste_to_ctrl_num = -1
  );

void paste_items_at(
  // List of parts from which to look for original part(s).
  const std::set<const Part*>& parts,
  // List of parts and events to paste.
  const TagEventList* tag_list,
  // Position to paste at.
  const Pos& pos,
  // Distance at which destination part is too far away
  //  (too soon, to the left) so a new part will be created.
  int max_distance=3072,
  // Erase target existing controllers first + erase wysiwyg.
  const FunctionOptionsStruct& options = FunctionOptionsStruct(),
  // Paste into this part instead of the original part(s).
  const Part* paste_into_part=NULL,
  // Number of copies to paste.
  int amount=1,
  // Separation between copies.
  int raster=3072,
  // Choose which events to paste.
  RelevantSelectedEvents_t relevant = AllEventsRelevant,
  // If pasting controllers, paste into this controller number if not -1.
  // If the source has multiple controllers, user will be asked which one to paste.
  int paste_to_ctrl_num = -1
  );

// Ensures that all events are untagged. Useful for aborting dialog etc.
// void untag_all_items();




//the below functions automatically open the dialog
//they return true if you click "ok" and false if "abort"
// REMOVE Tim. citem. Removed.
// bool modify_velocity(const std::set<const Part*>& parts);
// bool modify_notelen(const std::set<const Part*>& parts);
// bool quantize_notes(const std::set<const Part*>& parts);
// bool set_notelen(const std::set<const Part*>& parts);
// bool move_notes(const std::set<const Part*>& parts);
// bool transpose_notes(const std::set<const Part*>& parts);
// bool crescendo(const std::set<const Part*>& parts);
// bool erase_notes(const std::set<const Part*>& parts);
// bool delete_overlaps(const std::set<const Part*>& parts);
// bool legato(const std::set<const Part*>& parts);
// 
// //the below functions operate on selected parts
// bool modify_velocity();
// bool modify_notelen();
// bool quantize_notes();
// bool set_notelen();
// bool move_notes();
// bool transpose_notes();
// bool crescendo();
// bool erase_notes();
// bool delete_overlaps();
// bool legato();


//functions for copy'n'paste
void copy_notes(const std::set<const Part*>& parts, int range);
bool paste_notes(const Part* paste_into_part=NULL); // shows a dialog
void paste_notes(int max_distance=3072,
                 bool always_new_part=false, bool never_new_part=false,
                 const Part* paste_into_part=NULL, int amount=1, int raster=3072);
QMimeData* selected_events_to_mime(const std::set<const Part*>& parts, int range);
QMimeData* parts_to_mime(const std::set<const Part*>& parts);
void paste_at(const QString& pt, int pos, int max_distance=3072,
              bool always_new_part=false, bool never_new_part=false,
              const Part* paste_into_part=NULL, int amount=1, int raster=3072);

//functions for selections
void select_all(const std::set<const Part*>& parts);
void select_none(const std::set<const Part*>& parts);
void select_invert(const std::set<const Part*>& parts);
void select_in_loop(const std::set<const Part*>& parts);
void select_not_in_loop(const std::set<const Part*>& parts);
bool tracks_are_selected();
bool parts_are_selected();

//functions for parts
void shrink_parts(int raster=-1); //negative values mean "config.division"
void expand_parts(int raster=-1);
void schedule_resize_all_same_len_clone_parts(const Part* part, unsigned new_len, Undo& operations);
void clean_parts();
bool merge_with_next_part(const Part* part);
bool merge_selected_parts();
bool merge_parts(const std::set<const Part*>& parts);
bool split_part(const Part* part, int tick);
bool delete_selected_parts();

// internal
QMimeData* file_to_mimedata(FILE *datafile, QString mimeType);

} // namespace MusECore

#endif
