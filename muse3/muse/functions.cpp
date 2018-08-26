//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: functions.cpp,v 1.20.2.19 2011/05/05 20:10 flo93 Exp $
//  (C) Copyright 2011,2013 Florian Jung (flo93@sourceforge.net)
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

#include "functions.h"
#include "song.h"
#include "undo.h"
#include "helper.h"

#include "event.h"
#include "audio.h"
#include "gconfig.h"

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
#include "widgets/pasteeventsdialog.h"

#include <limits.h>
#include <iostream>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <math.h>

#include <QMimeData>
#include <QByteArray>
#include <QDrag>
#include <QMessageBox>
#include <QClipboard>
#include <QSet>


using namespace std;

using MusEGlobal::config;


namespace MusEGui {

FunctionDialogReturnVeloLen erase_items_dialog(const FunctionDialogMode& mode)
{
  erase_dialog->setElements(mode._buttons);
  if(!erase_dialog->exec())
    return FunctionDialogReturnVeloLen();
    
//   return FunctionDialogReturnVeloLen(!(MusEGui::erase_dialog->range & FUNCTION_RANGE_ONLY_SELECTED),
//                               MusEGui::erase_dialog->parts & FUNCTION_ALL_PARTS,
//                               MusEGui::erase_dialog->range & FUNCTION_RANGE_ONLY_BETWEEN_MARKERS,
//                               MusEGlobal::song->lPos(), MusEGlobal::song->rPos(),
//                               MusEGui::erase_dialog->velo_thres_used, MusEGui::erase_dialog->velo_threshold,
//                               MusEGui::erase_dialog->len_thres_used, MusEGui::erase_dialog->len_threshold);
//   const int range = erase_dialog->range;
//   const int parts = erase_dialog->parts;
//   const bool all_events = range == FunctionAllEventsButton || range == FunctionLoopedButton;
//   const bool range_events = range == FunctionLoopedButton || range == FunctionSelectedLoopedButton;
//   const bool all_parts = parts == FunctionAllPartsButton;

  const int flags = erase_dialog->_ret_flags;
  return FunctionDialogReturnVeloLen(flags & FunctionReturnAllEvents,
                                     flags & FunctionReturnAllParts,
                                     flags & FunctionReturnLooped,
                                     MusEGlobal::song->lPos(), MusEGlobal::song->rPos(),
                                     erase_dialog->velo_thres_used, erase_dialog->velo_threshold,
                                     erase_dialog->len_thres_used, erase_dialog->len_threshold);

  //   // TODO For now only do current part. Add support for 'all parts' option.
  //   if(range_items)
  //     editor->tagItems(!selected_items, false, true, MusEGlobal::song->lPos(), MusEGlobal::song->rPos());
  //   else
  //     editor->tagItems(!selected_items, false);
  //   
  //   erase_items(MusEGui::erase_dialog->velo_threshold, MusEGui::erase_dialog->velo_thres_used, 
  //               MusEGui::erase_dialog->len_threshold, MusEGui::erase_dialog->len_thres_used );
  // 
  //   return true;
}

FunctionDialogReturnCrescendo crescendo_items_dialog(const FunctionDialogMode& mode)
{
  crescendo_dialog->setElements(mode._buttons);
  if(!crescendo_dialog->exec())
    return FunctionDialogReturnCrescendo();
    
  const int flags = crescendo_dialog->_ret_flags;
  return FunctionDialogReturnCrescendo(flags & FunctionReturnAllEvents,
                                     flags & FunctionReturnAllParts,
                                     flags & FunctionReturnLooped,
                                     MusEGlobal::song->lPos(), MusEGlobal::song->rPos(),
                                     crescendo_dialog->start_val, crescendo_dialog->end_val,
                                     crescendo_dialog->absolute);
}

FunctionDialogReturnDelOverlaps deloverlaps_items_dialog(const FunctionDialogMode& mode)
{
  del_overlaps_dialog->setElements(mode._buttons);
  if(!del_overlaps_dialog->exec())
    return FunctionDialogReturnDelOverlaps();
    
  const int flags = del_overlaps_dialog->_ret_flags;
  return FunctionDialogReturnDelOverlaps(flags & FunctionReturnAllEvents,
                                     flags & FunctionReturnAllParts,
                                     flags & FunctionReturnLooped,
                                     MusEGlobal::song->lPos(), MusEGlobal::song->rPos());
}

FunctionDialogReturnGateTime gatetime_items_dialog(const FunctionDialogMode& mode)
{
  gatetime_dialog->setElements(mode._buttons);
  if(!gatetime_dialog->exec())
    return FunctionDialogReturnGateTime();
    
  const int flags = gatetime_dialog->_ret_flags;
  return FunctionDialogReturnGateTime(flags & FunctionReturnAllEvents,
                                     flags & FunctionReturnAllParts,
                                     flags & FunctionReturnLooped,
                                     MusEGlobal::song->lPos(), MusEGlobal::song->rPos(), 
                                     gatetime_dialog->rateVal, gatetime_dialog->offsetVal);
}

FunctionDialogReturnLegato legato_items_dialog(const FunctionDialogMode& mode)
{
  legato_dialog->setElements(mode._buttons);
  if(!legato_dialog->exec())
    return FunctionDialogReturnLegato();
    
  const int flags = legato_dialog->_ret_flags;
  return FunctionDialogReturnLegato(flags & FunctionReturnAllEvents,
                                     flags & FunctionReturnAllParts,
                                     flags & FunctionReturnLooped,
                                     MusEGlobal::song->lPos(), MusEGlobal::song->rPos(), 
                                     legato_dialog->min_len, legato_dialog->allow_shortening);
}

FunctionDialogReturnMove move_items_dialog(const FunctionDialogMode& mode)
{
  move_notes_dialog->setElements(mode._buttons);
  if(!move_notes_dialog->exec())
    return FunctionDialogReturnMove();
    
  const int flags = move_notes_dialog->_ret_flags;
  return FunctionDialogReturnMove(flags & FunctionReturnAllEvents,
                                     flags & FunctionReturnAllParts,
                                     flags & FunctionReturnLooped,
                                     MusEGlobal::song->lPos(), MusEGlobal::song->rPos(), 
                                     move_notes_dialog->amount);
}

FunctionDialogReturnQuantize quantize_items_dialog(const FunctionDialogMode& mode)
{
  quantize_dialog->setElements(mode._buttons);
  if(!quantize_dialog->exec())
    return FunctionDialogReturnQuantize();
    
  const int flags = quantize_dialog->_ret_flags;
  return FunctionDialogReturnQuantize(flags & FunctionReturnAllEvents,
                                     flags & FunctionReturnAllParts,
                                     flags & FunctionReturnLooped,
                                     MusEGlobal::song->lPos(), MusEGlobal::song->rPos(), 
                                     quantize_dialog->strength, quantize_dialog->threshold, 
                                     quantize_dialog->raster_index, quantize_dialog->swing, 
                                     quantize_dialog->quant_len);
}

FunctionDialogReturnSetLen setlen_items_dialog(const FunctionDialogMode& mode)
{
  set_notelen_dialog->setElements(mode._buttons);
  if(!set_notelen_dialog->exec())
    return FunctionDialogReturnSetLen();
    
  const int flags = set_notelen_dialog->_ret_flags;
  return FunctionDialogReturnSetLen(flags & FunctionReturnAllEvents,
                                     flags & FunctionReturnAllParts,
                                     flags & FunctionReturnLooped,
                                     MusEGlobal::song->lPos(), MusEGlobal::song->rPos(), 
                                     set_notelen_dialog->len);
}

FunctionDialogReturnTranspose transpose_items_dialog(const FunctionDialogMode& mode)
{
  transpose_dialog->setElements(mode._buttons);
  if(!transpose_dialog->exec())
    return FunctionDialogReturnTranspose();
    
  const int flags = transpose_dialog->_ret_flags;
  return FunctionDialogReturnTranspose(flags & FunctionReturnAllEvents,
                                     flags & FunctionReturnAllParts,
                                     flags & FunctionReturnLooped,
                                     MusEGlobal::song->lPos(), MusEGlobal::song->rPos(), 
                                     transpose_dialog->amount);
}

FunctionDialogReturnVelocity velocity_items_dialog(const FunctionDialogMode& mode)
{
  velocity_dialog->setElements(mode._buttons);
  if(!velocity_dialog->exec())
    return FunctionDialogReturnVelocity();
    
  const int flags = velocity_dialog->_ret_flags;
  return FunctionDialogReturnVelocity(flags & FunctionReturnAllEvents,
                                     flags & FunctionReturnAllParts,
                                     flags & FunctionReturnLooped,
                                     MusEGlobal::song->lPos(), MusEGlobal::song->rPos(), 
                                     velocity_dialog->rateVal, velocity_dialog->offsetVal);
}


} // namespace MusEGui


namespace MusECore {

// unit private functions:

bool read_eventlist_and_part(Xml& xml, EventList* el, int* part_id);

// -----------------------



set<const Part*> partlist_to_set(PartList* pl)
{
	set<const Part*> result;
	
	for (PartList::iterator it=pl->begin(); it!=pl->end(); it++)
		result.insert(it->second);
	
	return result;
}

set<const Part*> part_to_set(const Part* p)
{
	set<const Part*> result;
	result.insert(p);
	return result;
}

set<const Part*> get_all_parts()
{
	set<const Part*> result;
	
	TrackList* tracks=MusEGlobal::song->tracks();
	for (TrackList::const_iterator t_it=tracks->begin(); t_it!=tracks->end(); t_it++)
	{
		const PartList* parts=(*t_it)->cparts();
		for (ciPart p_it=parts->begin(); p_it!=parts->end(); p_it++)
			result.insert(p_it->second);
	}
	
	return result;
}

set<const Part*> get_all_selected_parts()
{
	set<const Part*> result;
	
	TrackList* tracks=MusEGlobal::song->tracks();
	for (TrackList::const_iterator t_it=tracks->begin(); t_it!=tracks->end(); t_it++)
	{
		const PartList* parts=(*t_it)->cparts();
		for (ciPart p_it=parts->begin(); p_it!=parts->end(); p_it++)
			if (p_it->second->selected())
				result.insert(p_it->second);
	}
	
	return result;
}

bool is_relevant(const Event& event, const Part* part, int range, RelevantSelectedEvents_t relevant)
{
	unsigned tick;
	
  switch(event.type())
  {
    case Note:
      if(!(relevant & NotesRelevant))
        return false;
    break;
    
    case Controller:
      if(!(relevant & ControllersRelevant))
        return false;
    break;
    
    case Sysex:
      if(!(relevant & SysexRelevant))
        return false;
    break;
    
    case Meta:
      if(!(relevant & MetaRelevant))
        return false;
    break;
    
    case Wave:
      if(!(relevant & WaveRelevant))
        return false;
    break;
  }
	
	switch (range)
	{
		case 0: return true;
		case 1: return event.selected();
		case 2: tick=event.tick()+part->tick(); return (tick >= MusEGlobal::song->lpos()) && (tick < MusEGlobal::song->rpos());
		case 3: return is_relevant(event,part,1, relevant) && is_relevant(event,part,2, relevant);
		default: cout << "ERROR: ILLEGAL FUNCTION CALL in is_relevant: range is illegal: "<<range<<endl;
		         return false;
	}
}


map<const Event*, const Part*> get_events(const set<const Part*>& parts, int range, RelevantSelectedEvents_t relevant)
{
	map<const Event*, const Part*> events;
	
	for (set<const Part*>::iterator part=parts.begin(); part!=parts.end(); part++)
		for (ciEvent event=(*part)->events().begin(); event!=(*part)->events().end(); event++)
			if (is_relevant(event->second, *part, range, relevant))
				events.insert(pair<const Event*, const Part*>(&event->second, *part));
	
	return events;
}


void untag_all_items()
{
	Part* part;
	PartList* pl;
	TrackList* tl = MusEGlobal::song->tracks();
	
	// We must clear all the tagged flags...
	for(ciTrack it = tl->begin(); it != tl->end(); ++it)
	{
		pl = (*it)->parts();
		for(ciPart ip = pl->begin(); ip != pl->end(); ++ip)
		{
			part = ip->second;
			part->setTagged(false);
			if(part->eventsTagged())
			{
				part->setEventsTagged(false);
				EventList& el = part->nonconst_events();
				for(iEvent ie = el.begin(); ie != el.end(); ie++)
					ie->second.setTagged(false);
			}
		}
	}
}


bool modify_notelen(const set<const Part*>& parts)
{
	if (!MusEGui::gatetime_dialog->exec())
		return false;
		
	modify_notelen(parts,MusEGui::gatetime_dialog->range,MusEGui::gatetime_dialog->rateVal,MusEGui::gatetime_dialog->offsetVal);
	
	return true;
}

bool modify_velocity(const set<const Part*>& parts)
{
	if (!MusEGui::velocity_dialog->exec())
		return false;
		
	modify_velocity(parts,MusEGui::velocity_dialog->range,MusEGui::velocity_dialog->rateVal,MusEGui::velocity_dialog->offsetVal);
	
	return true;
}

bool quantize_notes(const set<const Part*>& parts)
{
	if (!MusEGui::quantize_dialog->exec())
		return false;

  int raster = MusEGui::rasterVals[MusEGui::quantize_dialog->raster_index];
  quantize_notes(parts, MusEGui::quantize_dialog->range, (MusEGlobal::config.division*4)/raster,
	               MusEGui::quantize_dialog->quant_len, MusEGui::quantize_dialog->strength, MusEGui::quantize_dialog->swing,
	               MusEGui::quantize_dialog->threshold);
	
	return true;
}

// REMOVE Tim. citem. Removed.
// bool erase_notes(const set<const Part*>& parts)
// {
//   MusEGui::erase_dialog->setButtons();
// 	if (!MusEGui::erase_dialog->exec())
// 		return false;
// 		
// 	erase_notes(parts,MusEGui::erase_dialog->range, MusEGui::erase_dialog->velo_threshold, MusEGui::erase_dialog->velo_thres_used, 
// 	                                       MusEGui::erase_dialog->len_threshold, MusEGui::erase_dialog->len_thres_used );
// 	
// 	return true;
// }

bool delete_overlaps(const set<const Part*>& parts)
{
	if (!MusEGui::del_overlaps_dialog->exec())
		return false;
		
	delete_overlaps(parts,MusEGui::erase_dialog->_range);
	
	return true;
}

bool set_notelen(const set<const Part*>& parts)
{
	if (!MusEGui::set_notelen_dialog->exec())
		return false;
		
	set_notelen(parts,MusEGui::set_notelen_dialog->range,MusEGui::set_notelen_dialog->len);
	
	return true;
}

bool move_notes(const set<const Part*>& parts)
{
	if (!MusEGui::move_notes_dialog->exec())
		return false;
		
	move_notes(parts,MusEGui::move_notes_dialog->range,MusEGui::move_notes_dialog->amount);
	
	return true;
}

bool transpose_notes(const set<const Part*>& parts)
{
	if (!MusEGui::transpose_dialog->exec())
		return false;
		
	transpose_notes(parts,MusEGui::transpose_dialog->range,MusEGui::transpose_dialog->amount);
	
	return true;
}

bool crescendo(const set<const Part*>& parts)
{
	if (MusEGlobal::song->rpos() <= MusEGlobal::song->lpos())
	{
		QMessageBox::warning(NULL, QObject::tr("Error"), QObject::tr("Please first select the range for crescendo with the loop markers."));
		return false;
	}
	
	if (!MusEGui::crescendo_dialog->exec())
		return false;
		
	crescendo(parts,MusEGui::crescendo_dialog->range,MusEGui::crescendo_dialog->start_val,MusEGui::crescendo_dialog->end_val,MusEGui::crescendo_dialog->absolute);
	
	return true;
}

bool legato(const set<const Part*>& parts)
{
	if (!MusEGui::legato_dialog->exec())
		return false;
		
	legato(parts,MusEGui::legato_dialog->range, MusEGui::legato_dialog->min_len, !MusEGui::legato_dialog->allow_shortening);
	
	return true;
}



bool modify_notelen()
{
	if (!MusEGui::gatetime_dialog->exec())
		return false;
		
	set<const Part*> parts;
	if (MusEGui::gatetime_dialog->range & FUNCTION_RANGE_ONLY_SELECTED)
		parts=get_all_selected_parts();
	else
		parts=get_all_parts();
		
	modify_notelen(parts,MusEGui::gatetime_dialog->range & FUNCTION_RANGE_ONLY_BETWEEN_MARKERS, MusEGui::gatetime_dialog->rateVal,MusEGui::gatetime_dialog->offsetVal);
	
	return true;
}

bool modify_velocity()
{
	if (!MusEGui::velocity_dialog->exec())
		return false;
		
	set<const Part*> parts;
	if (MusEGui::velocity_dialog->range & FUNCTION_RANGE_ONLY_SELECTED)
		parts=get_all_selected_parts();
	else
		parts=get_all_parts();
		
	modify_velocity(parts,MusEGui::velocity_dialog->range & FUNCTION_RANGE_ONLY_BETWEEN_MARKERS,MusEGui::velocity_dialog->rateVal,MusEGui::velocity_dialog->offsetVal);
	
	return true;
}

bool quantize_notes()
{
	if (!MusEGui::quantize_dialog->exec())
		return false;
		
	set<const Part*> parts;
	if (MusEGui::quantize_dialog->range & FUNCTION_RANGE_ONLY_SELECTED)
		parts=get_all_selected_parts();
	else
		parts=get_all_parts();

  int raster = MusEGui::rasterVals[MusEGui::quantize_dialog->raster_index];
  quantize_notes(parts, MusEGui::quantize_dialog->range & FUNCTION_RANGE_ONLY_BETWEEN_MARKERS, (config.division*4)/raster,
	               MusEGui::quantize_dialog->quant_len, MusEGui::quantize_dialog->strength, MusEGui::quantize_dialog->swing,
	               MusEGui::quantize_dialog->threshold);
	
	return true;
}

// REMOVE Tim. citem. Removed.
// bool erase_notes()
// {
//   MusEGui::erase_dialog->setButtons();
// 	if (!MusEGui::erase_dialog->exec())
// 		return false;
// 		
// 	set<const Part*> parts;
// 	if (MusEGui::erase_dialog->range & FUNCTION_RANGE_ONLY_SELECTED)
// 		parts=get_all_selected_parts();
// 	else
// 		parts=get_all_parts();
// 		
// 	erase_notes(parts,MusEGui::erase_dialog->range & FUNCTION_RANGE_ONLY_BETWEEN_MARKERS, MusEGui::erase_dialog->velo_threshold, MusEGui::erase_dialog->velo_thres_used, 
// 	            MusEGui::erase_dialog->len_threshold, MusEGui::erase_dialog->len_thres_used );
// 	
// 	return true;
// }

bool delete_overlaps()
{
	if (!MusEGui::del_overlaps_dialog->exec())
		return false;
		
	set<const Part*> parts;
	if (MusEGui::del_overlaps_dialog->range & FUNCTION_RANGE_ONLY_SELECTED)
		parts=get_all_selected_parts();
	else
		parts=get_all_parts();
		
	delete_overlaps(parts,MusEGui::erase_dialog->_range & FUNCTION_RANGE_ONLY_BETWEEN_MARKERS);
	
	return true;
}

bool set_notelen()
{
	if (!MusEGui::set_notelen_dialog->exec())
		return false;
		
	set<const Part*> parts;
	if (MusEGui::set_notelen_dialog->range & FUNCTION_RANGE_ONLY_SELECTED)
		parts=get_all_selected_parts();
	else
		parts=get_all_parts();
		
	set_notelen(parts,MusEGui::set_notelen_dialog->range & FUNCTION_RANGE_ONLY_BETWEEN_MARKERS, MusEGui::set_notelen_dialog->len);
	
	return true;
}

bool move_notes()
{
	if (!MusEGui::move_notes_dialog->exec())
		return false;
		
	set<const Part*> parts;
	if (MusEGui::move_notes_dialog->range & FUNCTION_RANGE_ONLY_SELECTED)
		parts=get_all_selected_parts();
	else
		parts=get_all_parts();
		
	move_notes(parts,MusEGui::move_notes_dialog->range & FUNCTION_RANGE_ONLY_BETWEEN_MARKERS, MusEGui::move_notes_dialog->amount);
	
	return true;
}

bool transpose_notes()
{
	if (!MusEGui::transpose_dialog->exec())
		return false;
		
	set<const Part*> parts;
	if (MusEGui::transpose_dialog->range & FUNCTION_RANGE_ONLY_SELECTED)
		parts=get_all_selected_parts();
	else
		parts=get_all_parts();
		
	transpose_notes(parts,MusEGui::transpose_dialog->range & FUNCTION_RANGE_ONLY_BETWEEN_MARKERS, MusEGui::transpose_dialog->amount);
	
	return true;
}

bool crescendo()
{
	if (MusEGlobal::song->rpos() <= MusEGlobal::song->lpos())
	{
		QMessageBox::warning(NULL, QObject::tr("Error"), QObject::tr("Please first select the range for crescendo with the loop markers."));
		return false;
	}
	
	if (!MusEGui::crescendo_dialog->exec())
		return false;
		
	set<const Part*> parts;
	if (MusEGui::crescendo_dialog->range & FUNCTION_RANGE_ONLY_SELECTED)
		parts=get_all_selected_parts();
	else
		parts=get_all_parts();
		
	crescendo(parts,MusEGui::crescendo_dialog->range & FUNCTION_RANGE_ONLY_BETWEEN_MARKERS, MusEGui::crescendo_dialog->start_val,MusEGui::crescendo_dialog->end_val,MusEGui::crescendo_dialog->absolute);
	
	return true;
}

bool legato()
{
	if (!MusEGui::legato_dialog->exec())
		return false;
		
	set<const Part*> parts;
	if (MusEGui::legato_dialog->range & FUNCTION_RANGE_ONLY_SELECTED)
		parts=get_all_selected_parts();
	else
		parts=get_all_parts();
		
	legato(parts,MusEGui::legato_dialog->range & FUNCTION_RANGE_ONLY_BETWEEN_MARKERS, MusEGui::legato_dialog->min_len, !MusEGui::legato_dialog->allow_shortening);
	
	return true;
}






bool modify_velocity(const set<const Part*>& parts, int range, int rate, int offset)
{
	map<const Event*, const Part*> events = get_events(parts, range);
	Undo operations;
	
	if ( (!events.empty()) && ((rate!=100) || (offset!=0)) )
	{
		for (map<const Event*, const Part*>::iterator it=events.begin(); it!=events.end(); it++)
		{
			const Event& event=*(it->first);
			const Part* part=it->second;
			
			int velo = event.velo();

			velo = (velo * rate) / 100;
			velo += offset;

			if (velo <= 0)
				velo = 1;
			else if (velo > 127)
				velo = 127;
				
			if (event.velo() != velo)
			{
				Event newEvent = event.clone();
				newEvent.setVelo(velo);
				operations.push_back(UndoOp(UndoOp::ModifyEvent, newEvent, event, part, false, false));
			}
		}
		
		return MusEGlobal::song->applyOperationGroup(operations);
	}
	else
		return false;
}

bool modify_off_velocity(const set<const Part*>& parts, int range, int rate, int offset)
{
	map<const Event*, const Part*> events = get_events(parts, range);
	Undo operations;
	
	if ( (!events.empty()) && ((rate!=100) || (offset!=0)) )
	{
		for (map<const Event*, const Part*>::iterator it=events.begin(); it!=events.end(); it++)
		{
			const Event& event=*(it->first);
			const Part* part=it->second;
			
			int velo = event.veloOff();

			velo = (velo * rate) / 100;
			velo += offset;

			if (velo <= 0)
				velo = 1;
			else if (velo > 127)
				velo = 127;
				
			if (event.veloOff() != velo)
			{
				Event newEvent = event.clone();
				newEvent.setVeloOff(velo);
				operations.push_back(UndoOp(UndoOp::ModifyEvent, newEvent, event, part, false, false));
			}
		}

		return MusEGlobal::song->applyOperationGroup(operations);
	}
	else
		return false;
}

bool modify_notelen(const set<const Part*>& parts, int range, int rate, int offset)
{
	map<const Event*, const Part*> events = get_events(parts, range);
	Undo operations;
	map<const Part*, int> partlen;
	
	if ( (!events.empty()) && ((rate!=100) || (offset!=0)) )
	{
		for (map<const Event*, const Part*>::iterator it=events.begin(); it!=events.end(); it++)
		{
			const Event& event=*(it->first);
			const Part* part=it->second;

			unsigned int len = event.lenTick(); //prevent compiler warning: comparison signed/unsigned

			len = (len * rate) / 100;
			len += offset;

			if (len <= 0)
				len = 1;
			
			if ((event.tick()+len > part->lenTick()) && (!part->hasHiddenEvents()))
				partlen[part]=event.tick()+len; // schedule auto-expanding
				
			if (event.lenTick() != len)
			{
				Event newEvent = event.clone();
				newEvent.setLenTick(len);
				operations.push_back(UndoOp(UndoOp::ModifyEvent, newEvent, event, part, false, false));
			}
		}
		
		for (map<const Part*, int>::iterator it=partlen.begin(); it!=partlen.end(); it++)
			schedule_resize_all_same_len_clone_parts(it->first, it->second, operations);

		return MusEGlobal::song->applyOperationGroup(operations);
	}
	else
		return false;
}

bool set_notelen(const set<const Part*>& parts, int range, int len)
{
	return modify_notelen(parts, range, 0, len);
}

unsigned quantize_tick(unsigned tick, unsigned raster, int swing)
{
	//find out the nearest tick and the distance to it:
	//this is so complicated because this function supports
	//swing: if swing is 50, the resulting rhythm is not
	//"daa daa daa daa" but "daaaa da daaaa da"...
	int tick_dest1 = AL::sigmap.raster1(tick, raster*2); //round down
	int tick_dest2 = tick_dest1 + raster + raster*swing/100;
	int tick_dest3 = tick_dest1 + raster*2;

	int tick_diff1 = abs(tick_dest1 - (int)tick);
	int tick_diff2 = abs(tick_dest2 - (int)tick);
	int tick_diff3 = abs(tick_dest3 - (int)tick);
	
	if ((tick_diff3 <= tick_diff1) && (tick_diff3 <= tick_diff2)) //tick_dest3 is the nearest tick
		return tick_dest3;
	else if ((tick_diff2 <= tick_diff1) && (tick_diff2 <= tick_diff3)) //tick_dest2 is the nearest tick
		return tick_dest2;
	else
		return tick_dest1;
}

bool quantize_notes(const set<const Part*>& parts, int range, int raster, bool quant_len, int strength, int swing, int threshold)
{
	map<const Event*, const Part*> events = get_events(parts, range);
	Undo operations;
	
	if (!events.empty())
	{
		for (map<const Event*, const Part*>::iterator it=events.begin(); it!=events.end(); it++)
		{
			const Event& event=*(it->first);
			const Part* part=it->second;

			unsigned begin_tick = event.tick() + part->tick();
			int begin_diff = quantize_tick(begin_tick, raster, swing) - begin_tick;

			if (abs(begin_diff) > threshold)
				begin_tick = begin_tick + begin_diff*strength/100;


			unsigned len=event.lenTick();
			
			unsigned end_tick = begin_tick + len;
			int len_diff = quantize_tick(end_tick, raster, swing) - end_tick;
				
			if ((abs(len_diff) > threshold) && quant_len)
				len = len + len_diff*strength/100;

			if (len <= 0)
				len = 1;

				
			if ( (event.lenTick() != len) || (event.tick() + part->tick() != begin_tick) )
			{
				Event newEvent = event.clone();
				newEvent.setTick(begin_tick - part->tick());
				newEvent.setLenTick(len);
				operations.push_back(UndoOp(UndoOp::ModifyEvent, newEvent, event, part, false, false));
			}
		}
		
		return MusEGlobal::song->applyOperationGroup(operations);
	}
	else
		return false;
}

// REMOVE Tim. citem. Removed.
// bool erase_notes(const set<const Part*>& parts, int range, int velo_threshold, bool velo_thres_used, int len_threshold, bool len_thres_used)
// {
// 	map<const Event*, const Part*> events = get_events(parts, range);
// 	Undo operations;
// 	
// 	if (!events.empty())
// 	{
// 		for (map<const Event*, const Part*>::iterator it=events.begin(); it!=events.end(); it++)
// 		{
// 			const Event& event=*(it->first);
// 			const Part* part=it->second;
// 
// 			if ( (!velo_thres_used && !len_thres_used) ||
// 			     (velo_thres_used && event.velo() < velo_threshold) ||
// 			     (len_thres_used && int(event.lenTick()) < len_threshold) )
// 				operations.push_back(UndoOp(UndoOp::DeleteEvent, event, part, false, false));
// 		}
// 		
// 		return MusEGlobal::song->applyOperationGroup(operations);
// 	}
// 	else
// 		return false;
// }

bool erase_items(int velo_threshold, bool velo_thres_used, int len_threshold, bool len_thres_used)
{
  Undo operations;
  
  bool changed = false;
  Part* part;
  PartList* pl;
  TrackList* tl = MusEGlobal::song->tracks();

  for(ciTrack it = tl->begin(); it != tl->end(); ++it)
  {
    pl = (*it)->parts();
    for(ciPart ip = pl->begin(); ip != pl->end(); ++ip)
    {
      part = ip->second;
      part->setTagged(false);
      if(part->eventsTagged())
      {
        part->setEventsTagged(false);
        EventList& el = part->nonconst_events();
        for(iEvent ie = el.begin(); ie != el.end(); ie++)
        {
          Event& e = ie->second;
          if(e.tagged())
          {
            e.setTagged(false);
            // FIXME TODO Likely need agnostic Pos or frames rather than ticks if WaveCanvas is to use this.
            if ( e.type() != Note || (!velo_thres_used && !len_thres_used) ||
                   (velo_thres_used && e.velo() < velo_threshold) ||
                  (len_thres_used && int(e.lenTick()) < len_threshold) )
            {
              changed = true;
              operations.push_back(UndoOp(UndoOp::DeleteEvent, e, part, true, true));
            }
          }
        }
      }
    }
  }
  
  if(changed)
    return MusEGlobal::song->applyOperationGroup(operations);
  else
    return false;
}

bool crescendo_items(int start_val, int end_val, bool absolute)
{
  Undo operations;
  
  const Pos& from = MusEGlobal::song->lPos();
  const Pos& to = MusEGlobal::song->rPos();
  Pos pos;
  float curr_val;
  
  bool changed = false;
  if(to > from)
  {
    Part* part;
    PartList* pl;
    TrackList* tl = MusEGlobal::song->tracks();

    for(ciTrack it = tl->begin(); it != tl->end(); ++it)
    {
      pl = (*it)->parts();
      for(ciPart ip = pl->begin(); ip != pl->end(); ++ip)
      {
        part = ip->second;
        part->setTagged(false);
        if(part->eventsTagged())
        {
          part->setEventsTagged(false);
          EventList& el = part->nonconst_events();
          for(iEvent ie = el.begin(); ie != el.end(); ie++)
          {
            Event& e = ie->second;
            if(e.tagged())
            {
              e.setTagged(false);
              
              // This operation can only apply to notes.
              if(e.type() != Note)
                continue;

              pos = e.pos() + *part;
              curr_val = (float)start_val + (float)(end_val - start_val) * (pos - from).posValue() / (to - from).posValue();

              Event newEvent = e.clone();
              int velo = e.velo();

              if (absolute)
                velo=curr_val;
              else
                velo=curr_val*velo/100;

              if (velo > 127) velo=127;
              if (velo <= 0) velo=1;
              newEvent.setVelo(velo);
              
              changed = true;
              operations.push_back(UndoOp(UndoOp::ModifyEvent, newEvent, e, part, false, false));
            }
          }
        }
      }
    }
  }
  
  if(changed)
    return MusEGlobal::song->applyOperationGroup(operations);
  else
    return false;
}

bool delete_overlaps_items()
{
  Undo operations;
  
  bool changed = false;
  Part* part;
  PartList* pl;
  TrackList* tl = MusEGlobal::song->tracks();

  for(ciTrack it = tl->begin(); it != tl->end(); ++it)
  {
    pl = (*it)->parts();
    for(ciPart ip = pl->begin(); ip != pl->end(); ++ip)
    {
      part = ip->second;
      part->setTagged(false);
      if(part->eventsTagged())
      {
        part->setEventsTagged(false);
        EventList& el = part->nonconst_events();
        for(iEvent ie = el.begin(); ie != el.end(); ie++)
        {
          Event& e = ie->second;
          if(e.tagged())
          {
            e.setTagged(false);
            // FIXME TODO Likely need agnostic Pos or frames rather than ticks if WaveCanvas is to use this.
            if ( e.type() != Note || (!velo_thres_used && !len_thres_used) ||
                   (velo_thres_used && e.velo() < velo_threshold) ||
                  (len_thres_used && int(e.lenTick()) < len_threshold) )
            {
              changed = true;
              operations.push_back(UndoOp(UndoOp::DeleteEvent, e, part, true, true));
            }
          }
        }
      }
    }
  }
  
  if(changed)
    return MusEGlobal::song->applyOperationGroup(operations);
  else
    return false;
}

bool cut_items()
{
  QMimeData* drag = cut_or_copy_tagged_items_to_mime(true, true);

  if(drag)
  {
    QApplication::clipboard()->setMimeData(drag, QClipboard::Clipboard);
    return true;
  }
  
  return false;
}

bool transpose_notes(const set<const Part*>& parts, int range, signed int halftonesteps)
{
	map<const Event*, const Part*> events = get_events(parts, range);
	Undo operations;
	
	if ( (!events.empty()) && (halftonesteps!=0) )
	{
		for (map<const Event*, const Part*>::iterator it=events.begin(); it!=events.end(); it++)
		{
			const Event& event=*(it->first);
			const Part* part=it->second;

			Event newEvent = event.clone();
			int pitch = event.pitch()+halftonesteps;
			if (pitch > 127) pitch=127;
			if (pitch < 0) pitch=0;
			newEvent.setPitch(pitch);
			operations.push_back(UndoOp(UndoOp::ModifyEvent, newEvent, event, part, false, false));
		}
		
		return MusEGlobal::song->applyOperationGroup(operations);
	}
	else
		return false;
}

bool crescendo(const set<const Part*>& parts, int range, int start_val, int end_val, bool absolute)
{
	map<const Event*, const Part*> events = get_events(parts, range);
	Undo operations;
	
	int from=MusEGlobal::song->lpos();
	int to=MusEGlobal::song->rpos();
	
	if ( (!events.empty()) && (to>from) )
	{
		for (map<const Event*, const Part*>::iterator it=events.begin(); it!=events.end(); it++)
		{
			const Event& event=*(it->first);
			const Part* part=it->second;
			
			unsigned tick = event.tick() + part->tick();
			float curr_val= (float)start_val  +  (float)(end_val-start_val) * (tick-from) / (to-from);
			
			Event newEvent = event.clone();
			int velo = event.velo();

			if (absolute)
				velo=curr_val;
			else
				velo=curr_val*velo/100;

			if (velo > 127) velo=127;
			if (velo <= 0) velo=1;
			newEvent.setVelo(velo);
			operations.push_back(UndoOp(UndoOp::ModifyEvent, newEvent, event, part, false, false));
		}
		
		return MusEGlobal::song->applyOperationGroup(operations);
	}
	else
		return false;
}

bool move_notes(const set<const Part*>& parts, int range, signed int ticks)
{
	map<const Event*, const Part*> events = get_events(parts, range);
	Undo operations;
	map<const Part*, int> partlen;
	
	if ( (!events.empty()) && (ticks!=0) )
	{
		for (map<const Event*, const Part*>::iterator it=events.begin(); it!=events.end(); it++)
		{
			const Event& event=*(it->first);
			const Part* part=it->second;
			bool del=false;

			Event newEvent = event.clone();
			if ((signed)event.tick()+ticks < 0) //don't allow moving before the part's begin
				newEvent.setTick(0);
			else
				newEvent.setTick(event.tick()+ticks);
			
			if (newEvent.endTick() > part->lenTick()) //if exceeding the part's end:
			{
				if (part->hasHiddenEvents()) // auto-expanding is forbidden, clip
				{
					if (part->lenTick() > newEvent.tick())
						newEvent.setLenTick(part->lenTick() - newEvent.tick());
					else
						del=true; //if the new length would be <= 0, erase the note
				}
				else
					partlen[part]=newEvent.endTick(); // schedule auto-expanding
			}
			
			if (del==false)
				operations.push_back(UndoOp(UndoOp::ModifyEvent, newEvent, event, part, false, false));
			else
				operations.push_back(UndoOp(UndoOp::DeleteEvent, event, part, false, false));
		}
		
		for (map<const Part*, int>::iterator it=partlen.begin(); it!=partlen.end(); it++)
			schedule_resize_all_same_len_clone_parts(it->first, it->second, operations);
		
		return MusEGlobal::song->applyOperationGroup(operations);
	}
	else
		return false;
}


bool delete_overlaps(const set<const Part*>& parts, int range)
{
	map<const Event*, const Part*> events = get_events(parts, range);
	Undo operations;
	
	set<const Event*> deleted_events;
	
	if (!events.empty())
	{
		for (map<const Event*, const Part*>::iterator it1=events.begin(); it1!=events.end(); it1++)
		{
			const Event& event1=*(it1->first);
			const Part* part1=it1->second;
			
			// we may NOT optimize by letting it2 start at (it1 +1); this optimisation
			// is only allowed when events was sorted by time. it is, however, sorted
			// randomly by pointer.
			for (map<const Event*, const Part*>::iterator it2=events.begin(); it2!=events.end(); it2++)
			{
				const Event& event2=*(it2->first);
				const Part* part2=it2->second;
				
				if ( (part1->isCloneOf(part2)) &&          // part1 and part2 are the same or are duplicates
				     (&event1 != &event2) &&               // and event1 and event2 aren't the same
				     (deleted_events.find(&event2) == deleted_events.end()) ) //and event2 hasn't been deleted before
				{
					if ( (event1.pitch() == event2.pitch()) &&
					     (event1.tick() <= event2.tick()) &&
						   (event1.endTick() > event2.tick()) ) //they overlap
					{
						int new_len = event2.tick() - event1.tick();

						if (new_len==0)
						{
							operations.push_back(UndoOp(UndoOp::DeleteEvent, event1, part1, false, false));
							deleted_events.insert(&event1);
						}
						else
						{
							Event new_event1 = event1.clone();
							new_event1.setLenTick(new_len);
							
							operations.push_back(UndoOp(UndoOp::ModifyEvent, new_event1, event1, part1, false, false));
						}
					}
				}
			}
		}
		
		return MusEGlobal::song->applyOperationGroup(operations);
	}
	else
		return false;
}

bool legato(const set<const Part*>& parts, int range, int min_len, bool dont_shorten)
{
	map<const Event*, const Part*> events = get_events(parts, range);
	Undo operations;
	
	if (min_len<=0) min_len=1;
	
	if (!events.empty())
	{
		for (map<const Event*, const Part*>::iterator it1=events.begin(); it1!=events.end(); it1++)
		{
			const Event& event1=*(it1->first);
			const Part* part1=it1->second;
			
			unsigned len=INT_MAX;
			// we may NOT optimize by letting it2 start at (it1 +1); this optimisation
			// is only allowed when events was sorted by time. it is, however, sorted
			// randomly by pointer.
			for (map<const Event*, const Part*>::iterator it2=events.begin(); it2!=events.end(); it2++)
			{
				const Event& event2=*(it2->first);
				const Part* part2=it2->second;
				
				bool relevant = (event2.tick() >= event1.tick() + min_len);
				if (dont_shorten)
					relevant = relevant && (event2.tick() >= event1.endTick());
				
				if ( (part1->isCloneOf(part2)) &&           // part1 and part2 are the same or are duplicates
				      relevant &&                           // they're not too near (respect min_len and dont_shorten)
				     (event2.tick()-event1.tick() < len ) ) // that's the nearest relevant following note
					len=event2.tick()-event1.tick();
			}
			
			if (len==INT_MAX) len=event1.lenTick(); // if no following note was found, keep the length
			
			if (event1.lenTick() != len)
			{
				Event new_event1 = event1.clone();
				new_event1.setLenTick(len);
				
				operations.push_back(UndoOp(UndoOp::ModifyEvent, new_event1, event1, part1, false, false));
			}
		}
		
		return MusEGlobal::song->applyOperationGroup(operations);
	}
	else
		return false;
}



void copy_notes(const set<const Part*>& parts, int range)
{
	QMimeData* drag = selected_events_to_mime(parts,range);

	if (drag)
		QApplication::clipboard()->setMimeData(drag, QClipboard::Clipboard);
}

void copy_items()
{
	QMimeData* drag = cut_or_copy_tagged_items_to_mime();

	if (drag)
		QApplication::clipboard()->setMimeData(drag, QClipboard::Clipboard);
}

unsigned get_groupedevents_len(const QString& pt)
{
	unsigned maxlen=0;
	
	QByteArray pt_= pt.toLatin1();
	Xml xml(pt_.constData());
	for (;;) 
	{
		Xml::Token token = xml.parse();
		const QString& tag = xml.s1();
		switch (token) 
		{
			case Xml::Error:
			case Xml::End:
				return maxlen;
				
			case Xml::TagStart:
				if (tag == "eventlist")
				{
					EventList el;
					int part_id;
					if (read_eventlist_and_part(xml, &el, &part_id))
					{
						unsigned len = el.rbegin()->first;
						if (len > maxlen) maxlen=len;
					}
				}
				else
					xml.unknown("get_clipboard_len");
				break;
				
			case Xml::Attribut:
			case Xml::TagEnd:
			default:
				break;
		}
	}
	
	return maxlen; // see also the return statement above!
}

unsigned get_clipboard_len()
{
	QString tmp="x-muse-groupedeventlists"; // QClipboard::text() expects a QString&, not a QString :(
	QString s = QApplication::clipboard()->text(tmp, QClipboard::Clipboard);  // TODO CHECK Tim.
	
	return get_groupedevents_len(s);
}

bool paste_notes(const Part* paste_into_part)
{
	unsigned temp_begin = AL::sigmap.raster1(MusEGlobal::song->cpos(),0);
	unsigned temp_end = AL::sigmap.raster2(temp_begin + get_clipboard_len(), 0);
	MusEGui::paste_events_dialog->raster = temp_end - temp_begin;
	MusEGui::paste_events_dialog->into_single_part_allowed = (paste_into_part!=NULL);
	
	if (!MusEGui::paste_events_dialog->exec())
		return false;
		
	paste_notes(MusEGui::paste_events_dialog->max_distance, MusEGui::paste_events_dialog->always_new_part,
	            MusEGui::paste_events_dialog->never_new_part, MusEGui::paste_events_dialog->into_single_part ? paste_into_part : NULL,
	            MusEGui::paste_events_dialog->number, MusEGui::paste_events_dialog->raster);
	
	return true;
}

void paste_notes(int max_distance, bool always_new_part, bool never_new_part, const Part* paste_into_part, int amount, int raster)
{
	QString tmp="x-muse-groupedeventlists"; // QClipboard::text() expects a QString&, not a QString :(
	QString s = QApplication::clipboard()->text(tmp, QClipboard::Clipboard);
	paste_at(s, MusEGlobal::song->cpos(), max_distance, always_new_part, never_new_part, paste_into_part, amount, raster);
}

bool paste_items(const std::set<const Part*>& parts, const Part* paste_into_part)
{
	unsigned temp_begin = AL::sigmap.raster1(MusEGlobal::song->cpos(),0);
	unsigned temp_end = AL::sigmap.raster2(temp_begin + get_clipboard_len(), 0);
	MusEGui::paste_events_dialog->raster = temp_end - temp_begin;
	MusEGui::paste_events_dialog->into_single_part_allowed = (paste_into_part!=NULL);
	
	if (!MusEGui::paste_events_dialog->exec())
		return false;
		
	paste_items(parts, MusEGui::paste_events_dialog->max_distance, MusEGui::paste_events_dialog->always_new_part,
	            MusEGui::paste_events_dialog->never_new_part, MusEGui::paste_events_dialog->into_single_part ? paste_into_part : NULL,
	            MusEGui::paste_events_dialog->number, MusEGui::paste_events_dialog->raster);
	
	return true;
}

void paste_items(const set<const Part*>& parts, int max_distance, bool always_new_part, bool never_new_part, const Part* paste_into_part, int amount, int raster)
{
	QString tmp="x-muse-groupedeventlists"; // QClipboard::text() expects a QString&, not a QString :(
	QString s = QApplication::clipboard()->text(tmp, QClipboard::Clipboard);
	paste_items_at(parts, s, MusEGlobal::song->cpos(), max_distance, always_new_part, never_new_part, paste_into_part, amount, raster);
}

// if nothing is selected/relevant, this function returns NULL
QMimeData* selected_events_to_mime(const set<const Part*>& parts, int range)
{
    unsigned start_tick = INT_MAX; //will be the tick of the first event or INT_MAX if no events are there

    for (set<const Part*>::iterator part=parts.begin(); part!=parts.end(); part++)
        for (ciEvent ev=(*part)->events().begin(); ev!=(*part)->events().end(); ev++)
            if (is_relevant(ev->second, *part, range, AllEventsRelevant))
                if (ev->second.tick() < start_tick)
                    start_tick=ev->second.tick();

    if (start_tick == INT_MAX)
        return NULL;

    //---------------------------------------------------
    //    write events as XML into tmp file
    //---------------------------------------------------

    FILE* tmp = tmpfile();
    if (tmp == 0)
    {
        fprintf(stderr, "EventCanvas::getTextDrag() fopen failed: %s\n", strerror(errno));
        return 0;
    }

    Xml xml(tmp);
    int level = 0;

    for (set<const Part*>::iterator part=parts.begin(); part!=parts.end(); part++)
    {
        xml.tag(level++, "eventlist part_id=\"%d\"", (*part)->sn());
        for (ciEvent ev=(*part)->events().begin(); ev!=(*part)->events().end(); ev++)
            if (is_relevant(ev->second, *part, range, AllEventsRelevant))
                ev->second.write(level, xml, -start_tick);
        xml.etag(--level, "eventlist");
    }

    QMimeData *mimeData =  file_to_mimedata(tmp, "text/x-muse-groupedeventlists" );
    fclose(tmp);
    return mimeData;
}

// REMOVE Tim. citem. Added.
// // if nothing is selected/relevant, this function returns NULL
// QMimeData* tagged_items_to_mime(bool untag_when_done)
// {
//     unsigned start_tick = INT_MAX; //will be the tick of the first event or INT_MAX if no events are there
// 
//     Part* part;
//     PartList* pl;
//     TrackList* tl = MusEGlobal::song->tracks();
//     for(ciTrack it = tl->begin(); it != tl->end(); ++it)
//     {
//       pl = (*it)->parts();
//       for(ciPart ip = pl->begin(); ip != pl->end(); ++ip)
//       {
//         part = ip->second;
//         if(part->eventsTagged())
//         {
//           const EventList& el = part->events();
//           for(ciEvent ie = el.begin(); ie != el.end(); ie++)
//           {
//             const Event& e = ie->second;
//             if(e.tagged()) // && is_relevant(e, part, range, AllEventsRelevant))
//             {
//               if(e.tick() < start_tick)
//                 start_tick = e.tick();
//             }
//           }
//         }
//       }
//     }
//     
//     if (start_tick == INT_MAX)
//     {
//       if(untag_when_done)
//       {
//         // We must clear all the tagged flags...
//         for(ciTrack it = tl->begin(); it != tl->end(); ++it)
//         {
//           pl = (*it)->parts();
//           for(ciPart ip = pl->begin(); ip != pl->end(); ++ip)
//           {
//             part = ip->second;
//             part->setTagged(false);
//             if(part->eventsTagged())
//             {
//               part->setEventsTagged(false);
//               EventList& el = part->nonconst_events();
//               for(iEvent ie = el.begin(); ie != el.end(); ie++)
//                 ie->second.setTagged(false);
//             }
//           }
//         }
//       }
//       return NULL;
//     }
// 
//     //---------------------------------------------------
//     //    write events as XML into tmp file
//     //---------------------------------------------------
// 
//     FILE* tmp = tmpfile();
//     if (tmp == 0)
//     {
//         fprintf(stderr, "EventCanvas::getTextDrag() fopen failed: %s\n", strerror(errno));
//         return 0;
//     }
// 
//     Xml xml(tmp);
//     int level = 0;
// 
//     for(ciTrack it = tl->begin(); it != tl->end(); ++it)
//     {
//       pl = (*it)->parts();
//       for(ciPart ip = pl->begin(); ip != pl->end(); ++ip)
//       {
//         part = ip->second;
//         if(untag_when_done)
//           part->setTagged(false);
//         if(part->eventsTagged())
//         {
//           if(untag_when_done)
//             part->setEventsTagged(false);
//           xml.tag(level++, "eventlist part_id=\"%d\"", part->sn());
//           EventList& el = part->nonconst_events();
//           for(iEvent ie = el.begin(); ie != el.end(); ie++)
//           {
//             Event& e = ie->second;
//             if(e.tagged())
//             {
//               if(untag_when_done)
//                 e.setTagged(false);
//               
//               //if(is_relevant(e, part, range, AllEventsRelevant))
//                 e.write(level, xml, -start_tick);
//             }
//           }
//           xml.etag(--level, "eventlist");
//         }
//       }
//     }
//     
//     QMimeData *mimeData =  file_to_mimedata(tmp, "text/x-muse-groupedeventlists" );
//     fclose(tmp);
//     return mimeData;
// }

// REMOVE Tim. citem. Added.
// if nothing is selected/relevant, this function returns NULL
QMimeData* cut_or_copy_tagged_items_to_mime(bool cut_mode, bool untag_when_done)
{
    unsigned start_tick = INT_MAX; //will be the tick of the first event or INT_MAX if no events are there

    Undo operations;
  
    //bool do_cut = false;
    bool changed = false;
    Part* part;
    PartList* pl;
    TrackList* tl = MusEGlobal::song->tracks();
    for(ciTrack it = tl->begin(); it != tl->end(); ++it)
    {
      pl = (*it)->parts();
      for(ciPart ip = pl->begin(); ip != pl->end(); ++ip)
      {
        part = ip->second;
        // As an optimization, we only walk the events if the part says events are tagged.
        if(part->eventsTagged())
        {
          const EventList& el = part->events();
          for(ciEvent ie = el.begin(); ie != el.end(); ie++)
          {
            const Event& e = ie->second;
//             do_cut = false;
//             if(cut_mode)
//             {
//               // FIXME TODO Likely need agnostic Pos or frames rather than ticks if WaveCanvas is to use this.
//               do_cut = (e.type() != Note) || (!cut_velo_thres_used && !cut_len_thres_used) ||
//                        (cut_velo_thres_used && e.velo() < cut_velo_threshold) ||
//                        (cut_len_thres_used && int(e.lenTick()) < cut_len_threshold);
//             }
            
//             if(e.tagged() && (!cut_mode || do_cut)) // && is_relevant(e, part, range, AllEventsRelevant))
            if(e.tagged()) // && is_relevant(e, part, range, AllEventsRelevant))
            {
              if(e.tick() < start_tick)
                start_tick = e.tick();
            }
          }
        }
      }
    }
    
    if (start_tick == INT_MAX)
    {
      if(untag_when_done)
      {
        // We must clear all the tagged flags...
        for(ciTrack it = tl->begin(); it != tl->end(); ++it)
        {
          pl = (*it)->parts();
          for(ciPart ip = pl->begin(); ip != pl->end(); ++ip)
          {
            part = ip->second;
            part->setTagged(false);
            // As an optimization, we only walk the events if the part says events are tagged.
            if(part->eventsTagged())
            {
              part->setEventsTagged(false);
              EventList& el = part->nonconst_events();
              for(iEvent ie = el.begin(); ie != el.end(); ie++)
                ie->second.setTagged(false);
            }
          }
        }
      }
      return NULL;
    }

    //---------------------------------------------------
    //    write events as XML into tmp file
    //---------------------------------------------------

    FILE* tmp = tmpfile();
    if (tmp == 0)
    {
        fprintf(stderr, "EventCanvas::getTextDrag() fopen failed: %s\n", strerror(errno));
        return 0;
    }

    Xml xml(tmp);
    int level = 0;

    for(ciTrack it = tl->begin(); it != tl->end(); ++it)
    {
      pl = (*it)->parts();
      for(ciPart ip = pl->begin(); ip != pl->end(); ++ip)
      {
        part = ip->second;
        if(untag_when_done)
          part->setTagged(false);
        // As an optimization, we only walk the events if the part says events are tagged.
        if(part->eventsTagged())
        {
          if(untag_when_done)
            part->setEventsTagged(false);
          xml.tag(level++, "eventlist part_id=\"%d\"", part->sn());
          EventList& el = part->nonconst_events();
          for(iEvent ie = el.begin(); ie != el.end(); ie++)
          {
            Event& e = ie->second;
            if(e.tagged())
            {
              if(untag_when_done)
                e.setTagged(false);

//               do_cut = false;
//               if(cut_mode)
//               {
//                 // FIXME TODO Likely need agnostic Pos or frames rather than ticks if WaveCanvas is to use this.
//                 do_cut = (e.type() != Note) || (!cut_velo_thres_used && !cut_len_thres_used) ||
//                         (cut_velo_thres_used && e.velo() < cut_velo_threshold) ||
//                         (cut_len_thres_used && int(e.lenTick()) < cut_len_threshold);
//               }
                
              
              //if(is_relevant(e, part, range, AllEventsRelevant))
//               if(!cut_mode || do_cut)
                e.write(level, xml, -start_tick);
                
//               if(cut_mode && do_cut)
              if(cut_mode)
              {
                changed = true;
                operations.push_back(UndoOp(UndoOp::DeleteEvent, e, part, true, true));
              }
            }
          }
          xml.etag(--level, "eventlist");
        }
      }
    }
    
    QMimeData *mimeData =  file_to_mimedata(tmp, "text/x-muse-groupedeventlists" );
    fclose(tmp);
    
    if(changed)
      MusEGlobal::song->applyOperationGroup(operations);
    
    return mimeData;
}

// if nothing is selected/relevant, this function returns NULL
QMimeData* parts_to_mime(const set<const Part*>& parts)
{
	
	//---------------------------------------------------
	//    write events as XML into tmp file
	//---------------------------------------------------

	FILE* tmp = tmpfile();
	if (tmp == 0) 
	{
		fprintf(stderr, "EventCanvas::getTextDrag() fopen failed: %s\n", strerror(errno));
		return 0;
	}
	
	Xml xml(tmp);
	int level = 0;
	
    bool midi=false;
    bool wave=false;
	for (set<const Part*>::iterator part=parts.begin(); part!=parts.end(); part++)
	{
        if ((*part)->track()->type() == MusECore::Track::MIDI)
            midi=true;
        else
            wave=true;
        (*part)->write(level, xml, true, true);
    }
    QString mimeString = "text/x-muse-mixedpartlist";
    if (!midi)
        mimeString = "text/x-muse-wavepartlist";
    else if (!wave)
        mimeString = "text/x-muse-midipartlist";
    QMimeData *mimeData =  file_to_mimedata(tmp, mimeString );
    fclose(tmp);
    return mimeData;
}

//---------------------------------------------------
//    read datafile into mime Object
//---------------------------------------------------
QMimeData* file_to_mimedata(FILE *datafile, QString mimeType)
{

    fflush(datafile);
	struct stat f_stat;
    if (fstat(fileno(datafile), &f_stat) == -1)
	{
		fprintf(stderr, "copy_notes() fstat failed:<%s>\n",
		strerror(errno));
        fclose(datafile);
		return 0;
	}
	int n = f_stat.st_size;
	char* fbuf  = (char*)mmap(0, n+1, PROT_READ|PROT_WRITE,
    MAP_PRIVATE, fileno(datafile), 0);
	fbuf[n] = 0;

	QByteArray data(fbuf);

    QMimeData* md = new QMimeData();
    md->setData(mimeType, data);

	munmap(fbuf, n);

	return md;
}

bool read_eventlist_and_part(Xml& xml, EventList* el, int* part_id) // true on success, false on failure
{
	*part_id = -1;
	
	for (;;)
	{
		Xml::Token token = xml.parse();
		const QString& tag = xml.s1();
		switch (token)
		{
			case Xml::Error:
			case Xml::End:
				return false;
				
			case Xml::Attribut:
				if (tag == "part_id")
					*part_id = xml.s2().toInt();
				else
					printf("unknown attribute '%s' in read_eventlist_and_part(), ignoring it...\n", tag.toLatin1().data());
				break;
				
			case Xml::TagStart:
				if (tag == "event")
				{
					Event e(Note);
					e.read(xml);
					el->add(e);
				}
				else
					xml.unknown("read_eventlist_and_part");
				break;
				
			case Xml::TagEnd:
				if (tag == "eventlist")
					return true;
				
			default:
				break;
		}
	}
}

void paste_at(const QString& pt, int pos, int max_distance, bool always_new_part, bool never_new_part, const Part* paste_into_part, int amount, int raster)
{
	Undo operations;
	map<const Part*, unsigned> expand_map;
	map<const Part*, set<const Part*> > new_part_map;
	
	QByteArray pt_= pt.toLatin1();
	Xml xml(pt_.constData());
	for (;;) 
	{
		Xml::Token token = xml.parse();
		const QString& tag = xml.s1();
		switch (token) 
		{
			case Xml::Error:
			case Xml::End:
				goto out_of_paste_at_for;
				
			case Xml::TagStart:
				if (tag == "eventlist")
				{
					EventList el;
					int part_id;
		
					if (read_eventlist_and_part(xml, &el, &part_id))
					{
						const Part* dest_part;
						Track* dest_track;
						const Part* old_dest_part;
						
						if (paste_into_part == NULL)
							dest_part = partFromSerialNumber(part_id);
						else
							dest_part=paste_into_part;
						
						if (dest_part == NULL)
						{
							printf("ERROR: destination part wasn't found. ignoring these events\n");
						}
						else
						{
							dest_track=dest_part->track();
							old_dest_part=dest_part;
							unsigned first_paste_tick = el.begin()->first + pos;
							bool create_new_part = ( (dest_part->tick() > first_paste_tick) ||   // dest_part begins too late
									 ( ( (dest_part->endTick() + max_distance < first_paste_tick) || // dest_part is too far away
										                  always_new_part ) && !never_new_part ) );    // respect function arguments
							
							for (int i=0;i<amount;i++)
							{
								unsigned curr_pos = pos + i*raster;
								first_paste_tick = el.begin()->first + curr_pos;
								
								if (create_new_part)
								{
									dest_part = NULL;
									Part* newpart = dest_track->newPart();
									if(newpart)
									{
										newpart->setTick(AL::sigmap.raster1(first_paste_tick, config.division));
										new_part_map[old_dest_part].insert(dest_part);
										operations.push_back(UndoOp(UndoOp::AddPart, dest_part));
										dest_part = newpart;
									}
								}
								
								if(dest_part)
								{
									for (iEvent i = el.begin(); i != el.end(); ++i)
									{
										// If the destination part is a midi part, any midi event is allowed.
										// If the destination part is a wave part, any wave event is allowed.
										switch(i->second.type())
										{
											case Note:
											case Controller:
											case Sysex:
											case Meta:
												if(dest_part->type() == Pos::FRAMES)
													continue;
											break;
											
											case Wave:
												// FIXME TODO: To support pasting wave events, some code below must be made agnostic.
												//             For now just ignore wave events.
												//if(dest_part->type() == Pos::TICKS)
													continue;
											break;
										}
										
// FIXME TODO: To support pasting wave events, this code block and some code below it MUST be made position-agnostic.
										Event e = i->second.clone();
										int tick = e.tick() + curr_pos - dest_part->tick();
										if (tick<0)
										{
											printf("ERROR: trying to add event before current part! ignoring this event\n");
											continue;
										}

										e.setTick(tick);
										e.setSelected(true);  // No need to select clones, AddEvent operation below will take care of that.
										
										if (e.endTick() > dest_part->lenTick()) // event exceeds part?
										{
											if (dest_part->hasHiddenEvents()) // auto-expanding is forbidden?
											{
												if (e.tick() < dest_part->lenTick())
													e.setLenTick(dest_part->lenTick() - e.tick()); // clip
												else
													e.setLenTick(0); // don't insert that note at all
											}
											else
											{
												if (e.endTick() > expand_map[dest_part])
													expand_map[dest_part]=e.endTick();
											}
										}
										
	// REMOVE Tim. citem. Changed.
	// 									if (e.lenTick() != 0) operations.push_back(UndoOp(UndoOp::AddEvent,e, dest_part, false, false));
										// Don't add Note or Wave event types if they have no length.
										// Otherwise, controllers, sysex, and meta should all be allowed.
										//if ((e.type() != Note && e.type() != Wave) || e.lenTick() != 0)
										//	operations.push_back(UndoOp(UndoOp::AddEvent,e, dest_part, false, false));
										switch(e.type())
										{
											case Note:
												// Don't add Note event types if they have no length.
												// Notes are allowed to overlap. There is no DeleteEvent or ModifyEvent first.
												if(e.lenTick() != 0)
													operations.push_back(UndoOp(UndoOp::AddEvent, e, dest_part, false, false));
											break;
											
											case Wave:
												// Don't add Wave event types if they have no length.
												if(e.lenFrame() != 0)
												{
													EventList el;
													// Compare time, and wave position, path, and start position.
													dest_part->events().findSimilarType(e, el, true, false, false, false,
																															true, true, true);
													// Do NOT add the new wave event if it already exists at the position.
													// Don't event bother replacing it using DeletEvent or ModifyEvent.
													if(el.empty())
													{
														// REMOVE Tim. citem. Added. Diagnostic.
														fprintf(stderr, "paste_at: Adding new wave event: time:%u file:%s\n",
																		e.posValue(), e.sndFile().name().toLatin1().constData());
														operations.push_back(UndoOp(UndoOp::AddEvent, e, dest_part, false, false));
													}
													else
													{
														// Delete all but one of them. There shouldn't be more than one wave event
														//  at a time for a given wave event anyway.
														iEvent nie;
														for(iEvent ie = el.begin(); ie != el.end(); ++ie)
														{
															// Break on the second-last one, to leave one item intact.
															nie = ie;
															++nie;
															if(nie == el.end())
															{
																// REMOVE Tim. citem. Added. Diagnostic.
																fprintf(stderr, "paste_at: Leaving existing wave event intact: time:%u file:%s\n",
																				ie->second.posValue(), ie->second.sndFile().name().toLatin1().constData());
																break;
															}
															
															// REMOVE Tim. citem. Added. Diagnostic.
															fprintf(stderr, "paste_at: Deleting existing wave event: time:%u file:%s\n",
																			ie->second.posValue(), ie->second.sndFile().name().toLatin1().constData());
															
															operations.push_back(UndoOp(UndoOp::DeleteEvent, ie->second, dest_part, false, false));
														}
													}
													
												}
											break;
											
											case Controller:
											{
												EventList el;
												// Compare time and controller number (data A) only.
												dest_part->events().findSimilarType(e, el, true, true);
												// Delete them all. There shouldn't be more than one controller event
												//  at a time for a given controller number anyway.
												for(iEvent ie = el.begin(); ie != el.end(); ++ie)
												{
													
													// REMOVE Tim. citem. Added. Diagnostic.
													fprintf(stderr, "paste_at: Deleting existing controller event: time:%u number:%d\n",
																	ie->second.posValue(), ie->second.dataA());
													
													operations.push_back(UndoOp(UndoOp::DeleteEvent, ie->second, dest_part, true, true));
												}
												
												// REMOVE Tim. citem. Added. Diagnostic.
												if(!el.empty())
													fprintf(stderr, "paste_at: Adding replacement controller event: time:%u number:%d\n",
																	e.posValue(), e.dataA());
												
												// Do port controller values and clone parts. 
												operations.push_back(UndoOp(UndoOp::AddEvent, e, dest_part, true, true));
											}
											break;
											
											// Be careful with sysex and meta - there's no way to really determine if they are the same or not.
											case Sysex:
											{
												EventList el;
												// Compare time and sysex data only.
												dest_part->events().findSimilarType(e, el, true);
												// Do NOT add the new sysex if it already exists at the position.
												// Don't event bother replacing it using DeletEvent or ModifyEvent.
												if(el.empty())
												{
													// REMOVE Tim. citem. Added. Diagnostic.
													fprintf(stderr, "paste_at: Adding new sysex: time:%u len:%d\n",
																	e.posValue(), e.dataLen());
													operations.push_back(UndoOp(UndoOp::AddEvent, e, dest_part, false, false));
												}
												else
												{
													// Delete all but one of them. There shouldn't be more than one sysex event
													//  at a time for a given sysex anyway.
													iEvent nie;
													for(iEvent ie = el.begin(); ie != el.end(); ++ie)
													{
														// Break on the second-last one, to leave one item intact.
														nie = ie;
														++nie;
														if(nie == el.end())
														{
															// REMOVE Tim. citem. Added. Diagnostic.
															fprintf(stderr, "paste_at: Leaving existing sysex intact: time:%u len:%d\n",
																			ie->second.posValue(), ie->second.dataLen());
															break;
														}
														
														// REMOVE Tim. citem. Added. Diagnostic.
														fprintf(stderr, "paste_at: Deleting existing sysex event: time:%u len:%d\n",
																		ie->second.posValue(), ie->second.dataLen());
														
														operations.push_back(UndoOp(UndoOp::DeleteEvent, ie->second, dest_part, false, false));
													}
												}
											}
											break;
											
											case Meta:
											{
												EventList el;
												// Compare time and meta data only.
												dest_part->events().findSimilarType(e, el, true);
												// Do NOT add the new meta if it already exists at the position.
												// Don't event bother replacing it using DeletEvent or ModifyEvent.
												if(el.empty())
												{
													// REMOVE Tim. citem. Added. Diagnostic.
													fprintf(stderr, "paste_at: Adding new meta: time:%u len:%d\n",
																	e.posValue(), e.dataLen());
													operations.push_back(UndoOp(UndoOp::AddEvent, e, dest_part, false, false));
												}
												else
												{
													// Delete all but one of them. There shouldn't be more than one meta event
													//  at a time for a given meta anyway.
													iEvent nie;
													for(iEvent ie = el.begin(); ie != el.end(); ++ie)
													{
														// Break on the second-last one, to leave one item intact.
														nie = ie;
														++nie;
														if(nie == el.end())
														{
															// REMOVE Tim. citem. Added. Diagnostic.
															fprintf(stderr, "paste_at: Leaving existing meta intact: time:%u len:%d\n",
																			ie->second.posValue(), ie->second.dataLen());
															break;
														}
														
														// REMOVE Tim. citem. Added. Diagnostic.
														fprintf(stderr, "paste_at: Deleting existing meta event: time:%u len:%d\n",
																		ie->second.posValue(), ie->second.dataLen());
														
														operations.push_back(UndoOp(UndoOp::DeleteEvent, ie->second, dest_part, false, false));
													}
												}
											}
											break;
										}
									}
								}
							}
						}
					}
					else
					{
						printf("ERROR: reading eventlist from clipboard failed. ignoring this one...\n");
					}
				}
				else
					xml.unknown("paste_at");
				break;
				
			case Xml::Attribut:
			case Xml::TagEnd:
			default:
				break;
		}
	}
	
	out_of_paste_at_for:
	
	for (map<const Part*, unsigned>::iterator it = expand_map.begin(); it!=expand_map.end(); it++)
		if (it->second != it->first->lenTick())
			schedule_resize_all_same_len_clone_parts(it->first, it->second, operations);

	MusEGlobal::song->informAboutNewParts(new_part_map); // must be called before apply. otherwise
	                                                     // pointer changes (by resize) screw it up
	MusEGlobal::song->applyOperationGroup(operations);
	MusEGlobal::song->update(SC_SELECTION | SC_PART_SELECTION);
}

void paste_items_at(const std::set<const Part*>& parts, const QString& pt, int pos, int max_distance,
                    bool always_new_part, bool never_new_part,
                    const Part* paste_into_part, int amount, int raster)
{
	Undo operations;
	map<const Part*, unsigned> expand_map;
	map<const Part*, set<const Part*> > new_part_map;
	
	QByteArray pt_= pt.toLatin1();
	Xml xml(pt_.constData());
	for (;;) 
	{
		Xml::Token token = xml.parse();
		const QString& tag = xml.s1();
		switch (token) 
		{
			case Xml::Error:
			case Xml::End:
				goto out_of_paste_at_for;
				
			case Xml::TagStart:
				if (tag == "eventlist")
				{
					EventList el;
					int part_id;
		
					if (read_eventlist_and_part(xml, &el, &part_id))
					{
						const Part* dest_part;
						Track* dest_track;
						const Part* old_dest_part;
						
						if (paste_into_part == NULL)
							dest_part = partFromSerialNumber(part_id);
						else
							dest_part=paste_into_part;
						
						if (dest_part == NULL)
						{
							printf("ERROR: destination part wasn't found. ignoring these events\n");
						}
						else
						{
							// Paste into the destination part ONLY if it is included in the given set of parts,
							//  typically the parts used by an editor window instance's canvas. (WYSIWYG).
							// Override if paste_into_part is given, to allow 'Paste to current part' to work.
							if(paste_into_part || parts.find(dest_part) != parts.end())
							{
								dest_track=dest_part->track();
								old_dest_part=dest_part;
								unsigned first_paste_tick = el.begin()->first + pos;
								bool create_new_part = ( (dest_part->tick() > first_paste_tick) ||   // dest_part begins too late
										( ( (dest_part->endTick() + max_distance < first_paste_tick) || // dest_part is too far away
																				always_new_part ) && !never_new_part ) );    // respect function arguments
								
								for (int i=0;i<amount;i++)
								{
									unsigned curr_pos = pos + i*raster;
									first_paste_tick = el.begin()->first + curr_pos;
									
									if (create_new_part)
									{
										dest_part = NULL;
										Part* newpart = dest_track->newPart();
										if(newpart)
										{
											newpart->setTick(AL::sigmap.raster1(first_paste_tick, config.division));
											new_part_map[old_dest_part].insert(dest_part);
											operations.push_back(UndoOp(UndoOp::AddPart, dest_part));
											dest_part = newpart;
										}
									}
									
									if(dest_part)
									{
										for (iEvent i = el.begin(); i != el.end(); ++i)
										{
											// If the destination part is a midi part, any midi event is allowed.
											// If the destination part is a wave part, any wave event is allowed.
											switch(i->second.type())
											{
												case Note:
												case Controller:
												case Sysex:
												case Meta:
													if(dest_part->type() == Pos::FRAMES)
														continue;
												break;
												
												case Wave:
													// FIXME TODO: To support pasting wave events, some code below must be made agnostic.
													//             For now just ignore wave events.
													//if(dest_part->type() == Pos::TICKS)
														continue;
												break;
											}
											
	// FIXME TODO: To support pasting wave events, this code block and some code below it MUST be made position-agnostic.
											Event e = i->second.clone();
											int tick = e.tick() + curr_pos - dest_part->tick();
											if (tick<0)
											{
												printf("ERROR: trying to add event before current part! ignoring this event\n");
												continue;
											}

											e.setTick(tick);
											e.setSelected(true);  // No need to select clones, AddEvent operation below will take care of that.
											
											if (e.endTick() > dest_part->lenTick()) // event exceeds part?
											{
												if (dest_part->hasHiddenEvents()) // auto-expanding is forbidden?
												{
													if (e.tick() < dest_part->lenTick())
														e.setLenTick(dest_part->lenTick() - e.tick()); // clip
													else
														e.setLenTick(0); // don't insert that note at all
												}
												else
												{
													if (e.endTick() > expand_map[dest_part])
														expand_map[dest_part]=e.endTick();
												}
											}
											
		// REMOVE Tim. citem. Changed.
		// 									if (e.lenTick() != 0) operations.push_back(UndoOp(UndoOp::AddEvent,e, dest_part, false, false));
											// Don't add Note or Wave event types if they have no length.
											// Otherwise, controllers, sysex, and meta should all be allowed.
											//if ((e.type() != Note && e.type() != Wave) || e.lenTick() != 0)
											//	operations.push_back(UndoOp(UndoOp::AddEvent,e, dest_part, false, false));
											switch(e.type())
											{
												case Note:
													// Don't add Note event types if they have no length.
													// Notes are allowed to overlap. There is no DeleteEvent or ModifyEvent first.
													if(e.lenTick() != 0)
														operations.push_back(UndoOp(UndoOp::AddEvent, e, dest_part, false, false));
												break;
												
												case Wave:
													// Don't add Wave event types if they have no length.
													if(e.lenFrame() != 0)
													{
														EventList el;
														// Compare time, and wave position, path, and start position.
														dest_part->events().findSimilarType(e, el, true, false, false, false,
																																true, true, true);
														// Do NOT add the new wave event if it already exists at the position.
														// Don't event bother replacing it using DeletEvent or ModifyEvent.
														if(el.empty())
														{
															// REMOVE Tim. citem. Added. Diagnostic.
															fprintf(stderr, "paste_items_at: Adding new wave event: time:%u file:%s\n",
																			e.posValue(), e.sndFile().name().toLatin1().constData());
															operations.push_back(UndoOp(UndoOp::AddEvent, e, dest_part, false, false));
														}
														else
														{
															// Delete all but one of them. There shouldn't be more than one wave event
															//  at a time for a given wave event anyway.
															iEvent nie;
															for(iEvent ie = el.begin(); ie != el.end(); ++ie)
															{
																// Break on the second-last one, to leave one item intact.
																nie = ie;
																++nie;
																if(nie == el.end())
																{
																	// REMOVE Tim. citem. Added. Diagnostic.
																	fprintf(stderr, "paste_items_at: Leaving existing wave event intact: time:%u file:%s\n",
																					ie->second.posValue(), ie->second.sndFile().name().toLatin1().constData());
																	break;
																}
																
																// REMOVE Tim. citem. Added. Diagnostic.
																fprintf(stderr, "paste_items_at: Deleting existing wave event: time:%u file:%s\n",
																				ie->second.posValue(), ie->second.sndFile().name().toLatin1().constData());
																
																operations.push_back(UndoOp(UndoOp::DeleteEvent, ie->second, dest_part, false, false));
															}
														}
														
													}
												break;
												
												case Controller:
												{
													EventList el;
													// Compare time and controller number (data A) only.
													dest_part->events().findSimilarType(e, el, true, true);
													// Delete them all. There shouldn't be more than one controller event
													//  at a time for a given controller number anyway.
													for(iEvent ie = el.begin(); ie != el.end(); ++ie)
													{
														
														// REMOVE Tim. citem. Added. Diagnostic.
														fprintf(stderr, "paste_items_at: Deleting existing controller event: time:%u number:%d\n",
																		ie->second.posValue(), ie->second.dataA());
														
														operations.push_back(UndoOp(UndoOp::DeleteEvent, ie->second, dest_part, true, true));
													}
													
													// REMOVE Tim. citem. Added. Diagnostic.
													if(!el.empty())
														fprintf(stderr, "paste_items_at: Adding replacement controller event: time:%u number:%d\n",
																		e.posValue(), e.dataA());
													
													// Do port controller values and clone parts. 
													operations.push_back(UndoOp(UndoOp::AddEvent, e, dest_part, true, true));
												}
												break;
												
												// Be careful with sysex and meta - there's no way to really determine if they are the same or not.
												case Sysex:
												{
													EventList el;
													// Compare time and sysex data only.
													dest_part->events().findSimilarType(e, el, true);
													// Do NOT add the new sysex if it already exists at the position.
													// Don't event bother replacing it using DeletEvent or ModifyEvent.
													if(el.empty())
													{
														// REMOVE Tim. citem. Added. Diagnostic.
														fprintf(stderr, "paste_items_at: Adding new sysex: time:%u len:%d\n",
																		e.posValue(), e.dataLen());
														operations.push_back(UndoOp(UndoOp::AddEvent, e, dest_part, false, false));
													}
													else
													{
														// Delete all but one of them. There shouldn't be more than one sysex event
														//  at a time for a given sysex anyway.
														iEvent nie;
														for(iEvent ie = el.begin(); ie != el.end(); ++ie)
														{
															// Break on the second-last one, to leave one item intact.
															nie = ie;
															++nie;
															if(nie == el.end())
															{
																// REMOVE Tim. citem. Added. Diagnostic.
																fprintf(stderr, "paste_items_at: Leaving existing sysex intact: time:%u len:%d\n",
																				ie->second.posValue(), ie->second.dataLen());
																break;
															}
															
															// REMOVE Tim. citem. Added. Diagnostic.
															fprintf(stderr, "paste_items_at: Deleting existing sysex event: time:%u len:%d\n",
																			ie->second.posValue(), ie->second.dataLen());
															
															operations.push_back(UndoOp(UndoOp::DeleteEvent, ie->second, dest_part, false, false));
														}
													}
												}
												break;
												
												case Meta:
												{
													EventList el;
													// Compare time and meta data only.
													dest_part->events().findSimilarType(e, el, true);
													// Do NOT add the new meta if it already exists at the position.
													// Don't event bother replacing it using DeletEvent or ModifyEvent.
													if(el.empty())
													{
														// REMOVE Tim. citem. Added. Diagnostic.
														fprintf(stderr, "paste_items_at: Adding new meta: time:%u len:%d\n",
																		e.posValue(), e.dataLen());
														operations.push_back(UndoOp(UndoOp::AddEvent, e, dest_part, false, false));
													}
													else
													{
														// Delete all but one of them. There shouldn't be more than one meta event
														//  at a time for a given meta anyway.
														iEvent nie;
														for(iEvent ie = el.begin(); ie != el.end(); ++ie)
														{
															// Break on the second-last one, to leave one item intact.
															nie = ie;
															++nie;
															if(nie == el.end())
															{
																// REMOVE Tim. citem. Added. Diagnostic.
																fprintf(stderr, "paste_items_at: Leaving existing meta intact: time:%u len:%d\n",
																				ie->second.posValue(), ie->second.dataLen());
																break;
															}
															
															// REMOVE Tim. citem. Added. Diagnostic.
															fprintf(stderr, "paste_items_at: Deleting existing meta event: time:%u len:%d\n",
																			ie->second.posValue(), ie->second.dataLen());
															
															operations.push_back(UndoOp(UndoOp::DeleteEvent, ie->second, dest_part, false, false));
														}
													}
												}
												break;
											}
										}
									}
								}
							}
						}
					}
					else
					{
						printf("ERROR: reading eventlist from clipboard failed. ignoring this one...\n");
					}
				}
				else
					xml.unknown("paste_items_at");
				break;
				
			case Xml::Attribut:
			case Xml::TagEnd:
			default:
				break;
		}
	}
	
	out_of_paste_at_for:
	
	for (map<const Part*, unsigned>::iterator it = expand_map.begin(); it!=expand_map.end(); it++)
		if (it->second != it->first->lenTick())
			schedule_resize_all_same_len_clone_parts(it->first, it->second, operations);

	MusEGlobal::song->informAboutNewParts(new_part_map); // must be called before apply. otherwise
	                                                     // pointer changes (by resize) screw it up
	MusEGlobal::song->applyOperationGroup(operations);
	MusEGlobal::song->update(SC_SELECTION | SC_PART_SELECTION);
}

void select_all(const set<const Part*>& parts)
{
	Undo operations;
	operations.combobreaker=true;
	
	for (set<const Part*>::iterator part=parts.begin(); part!=parts.end(); part++)
		for (ciEvent ev_it=(*part)->events().begin(); ev_it!=(*part)->events().end(); ev_it++)
		{
			const Event& event=ev_it->second;
// REMOVE Tim. citem. Changed.
			operations.push_back(UndoOp(UndoOp::SelectEvent,event, *part, true, event.selected()));
// 			operations.push_back(UndoOp(UndoOp::SelectEvent,event, *part, true, event.selected()));
		}
// REMOVE Tim. citem. Changed.
// 	MusEGlobal::song->applyOperationGroup(operations);
	MusEGlobal::song->applyOperationGroup(operations, MusECore::Song::OperationExecuteUpdate);
}

void select_none(const set<const Part*>& parts)
{
	Undo operations;
	operations.combobreaker=true;
	
	for (set<const Part*>::iterator part=parts.begin(); part!=parts.end(); part++)
		for (ciEvent ev_it=(*part)->events().begin(); ev_it!=(*part)->events().end(); ev_it++)
		{
			const Event& event=ev_it->second;
// REMOVE Tim. citem. Changed.
			operations.push_back(UndoOp(UndoOp::SelectEvent,event, *part, false, event.selected()));
			//operations.push_back(UndoOp(UndoOp::SelectEvent,event, *part, false, event.selected(), false));
		}
// REMOVE Tim. citem. Changed.
// 	MusEGlobal::song->applyOperationGroup(operations);
	MusEGlobal::song->applyOperationGroup(operations, MusECore::Song::OperationExecuteUpdate);
}

void select_invert(const set<const Part*>& parts)
{
	Undo operations;
	operations.combobreaker=true;
	
	for (set<const Part*>::iterator part=parts.begin(); part!=parts.end(); part++)
		for (ciEvent ev_it=(*part)->events().begin(); ev_it!=(*part)->events().end(); ev_it++)
		{
			const Event& event=ev_it->second;
// REMOVE Tim. citem. Changed.
			operations.push_back(UndoOp(UndoOp::SelectEvent,event, *part, !event.selected(), event.selected()));
			//operations.push_back(UndoOp(UndoOp::SelectEvent,event, *part, !event.selected(), event.selected(), false));
		}
// REMOVE Tim. citem. Changed.
// 	MusEGlobal::song->applyOperationGroup(operations);
	MusEGlobal::song->applyOperationGroup(operations, MusECore::Song::OperationExecuteUpdate);
}

void select_in_loop(const set<const Part*>& parts)
{
	select_none(parts);
	Undo operations;
	operations.combobreaker=true;
	
	for (set<const Part*>::iterator part=parts.begin(); part!=parts.end(); part++)
		for (ciEvent ev_it=(*part)->events().begin(); ev_it!=(*part)->events().end(); ev_it++)
		{
			const Event& event=ev_it->second;
// REMOVE Tim. citem. Changed.
			operations.push_back(UndoOp(UndoOp::SelectEvent,event, *part,
         (event.tick()>=MusEGlobal::song->lpos() && event.endTick()<=MusEGlobal::song->rpos()), event.selected()));
			//operations.push_back(UndoOp(UndoOp::SelectEvent,event, *part,
      //   (event.tick()>=MusEGlobal::song->lpos() && event.endTick()<=MusEGlobal::song->rpos()), event.selected(), false));
		}
// REMOVE Tim. citem. Changed.
// 	MusEGlobal::song->applyOperationGroup(operations);
	MusEGlobal::song->applyOperationGroup(operations, MusECore::Song::OperationExecuteUpdate);
}

void select_not_in_loop(const set<const Part*>& parts)
{
	select_none(parts);
	Undo operations;
	operations.combobreaker=true;
	
	for (set<const Part*>::iterator part=parts.begin(); part!=parts.end(); part++)
		for (ciEvent ev_it=(*part)->events().begin(); ev_it!=(*part)->events().end(); ev_it++)
		{
			const Event& event=ev_it->second;
// REMOVE Tim. citem. Changed.
			operations.push_back(UndoOp(UndoOp::SelectEvent,event, *part,
        !(event.tick()>=MusEGlobal::song->lpos() && event.endTick()<=MusEGlobal::song->rpos()), event.selected()));
			//operations.push_back(UndoOp(UndoOp::SelectEvent,event, *part,
      //           !(event.tick()>=MusEGlobal::song->lpos() && event.endTick()<=MusEGlobal::song->rpos()), event.selected(), false));
		}
// REMOVE Tim. citem. Changed.
// 	MusEGlobal::song->applyOperationGroup(operations);
	MusEGlobal::song->applyOperationGroup(operations, MusECore::Song::OperationExecuteUpdate);
}

bool tracks_are_selected()
{
  const TrackList* tl = MusEGlobal::song->tracks();
  for(ciTrack it = tl->begin(); it != tl->end(); ++it)
    if((*it)->selected()) 
      return true;
  return false;
}

bool parts_are_selected()
{
  const TrackList* tl = MusEGlobal::song->tracks();
  for(ciTrack it = tl->begin(); it != tl->end(); ++it)
  {
    const PartList* pl = (*it)->cparts();
    for(ciPart ip = pl->begin(); ip != pl->end(); ++ip)
      if(ip->second->selected())
        return true;
  }
  return false;
}



void shrink_parts(int raster)
{
	Undo operations;
	
	unsigned min_len;
	if (raster<0) raster=MusEGlobal::config.division;
	if (raster>=0) min_len=raster; else min_len=MusEGlobal::config.division;
	
	TrackList* tracks = MusEGlobal::song->tracks();
	for (iTrack track = tracks->begin(); track != tracks->end(); track++)
		for (iPart part = (*track)->parts()->begin(); part != (*track)->parts()->end(); part++)
			if (part->second->selected())
			{
				unsigned len=0;
				
				for (ciEvent ev=part->second->events().begin(); ev!=part->second->events().end(); ev++)
					if (ev->second.endTick() > len)
						len=ev->second.endTick();
				
				if (raster) len=ceil((float)len/raster)*raster;
				if (len<min_len) len=min_len;
				
				if (len < part->second->lenTick())
					operations.push_back(UndoOp(UndoOp::ModifyPartLength, part->second, part->second->lenValue(), len, Pos::TICKS));
			}
	
	MusEGlobal::song->applyOperationGroup(operations);
}


void schedule_resize_all_same_len_clone_parts(const Part* part, unsigned new_len, Undo& operations)
{
	QSet<const Part*> already_done;
	
	for (Undo::iterator op_it=operations.begin(); op_it!=operations.end();op_it++)
		if (op_it->type==UndoOp::DeletePart)
      already_done.insert(op_it->part);
			
	unsigned old_len = part->lenValue();
	if (old_len!=new_len)
	{
		const Part* part_it=part;
		do
		{
			if (part_it->lenValue()==old_len && !already_done.contains(part_it))
				operations.push_back(UndoOp(UndoOp::ModifyPartLength, part_it, old_len, new_len, part->type()));
			
			part_it=part_it->nextClone();
		} while (part_it!=part);
	}
}

void expand_parts(int raster)
{
	Undo operations;
	
	unsigned min_len;
	if (raster<0) raster=MusEGlobal::config.division;
	if (raster>=0) min_len=raster; else min_len=MusEGlobal::config.division;

	TrackList* tracks = MusEGlobal::song->tracks();
	for (iTrack track = tracks->begin(); track != tracks->end(); track++)
		for (iPart part = (*track)->parts()->begin(); part != (*track)->parts()->end(); part++)
			if (part->second->selected())
			{
				unsigned len=part->second->lenTick();
				
				for (ciEvent ev=part->second->events().begin(); ev!=part->second->events().end(); ev++)
					if (ev->second.endTick() > len)
						len=ev->second.endTick();

				if (raster) len=ceil((float)len/raster)*raster;
				if (len<min_len) len=min_len;
								
				if (len > part->second->lenTick())
					operations.push_back(UndoOp(UndoOp::ModifyPartLength, part->second, part->second->lenValue(), len, Pos::TICKS));
			}
			
	MusEGlobal::song->applyOperationGroup(operations);
}

void clean_parts()
{
	Undo operations;
	set<const Part*> already_processed;
	
	TrackList* tracks = MusEGlobal::song->tracks();
	for (iTrack track = tracks->begin(); track != tracks->end(); track++)
		for (iPart part = (*track)->parts()->begin(); part != (*track)->parts()->end(); part++)
			if ((part->second->selected()) && (already_processed.find(part->second)==already_processed.end()))
			{ 
				// find out the length of the longest clone of this part;
				// avoid processing eventlist multiple times (because of
				// multiple clones)
				unsigned len=0;
				
				const Part* part_it=part->second;
				do
				{
					if (part_it->lenTick() > len)
						len=part_it->lenTick();
						
					already_processed.insert(part_it);
					part_it=part_it->nextClone();
				} while ((part_it!=part->second) && (part_it!=NULL));

				
				// erase all events exceeding the longest clone of this part
				// (i.e., erase all hidden events) or shorten them
				for (ciEvent ev=part->second->events().begin(); ev!=part->second->events().end(); ev++)
					if (ev->second.tick() >= len)
						operations.push_back(UndoOp(UndoOp::DeleteEvent, ev->second, part->second, true, true));
					else if (ev->second.endTick() > len)
					{
						Event new_event = ev->second.clone();
						new_event.setLenTick(len - ev->second.tick());
						
						operations.push_back(UndoOp(UndoOp::ModifyEvent, new_event, ev->second, part->second, true, true));
					}
			}
	
	MusEGlobal::song->applyOperationGroup(operations);
}


bool merge_with_next_part(const Part* oPart)
{
	const Track* track = oPart->track();
	
	if(track->type() != Track::WAVE && !track->isMidiTrack())
		return false;
	
	const PartList* pl   = track->cparts();
	const Part* nextPart = 0;

	for (ciPart ip = pl->begin(); ip != pl->end(); ++ip)
	{
			if (ip->second == oPart)
			{
				++ip;
				if (ip == pl->end())
						return false;
				nextPart = ip->second;
				break;
				}
			}
	
	if (nextPart)
	{
		set<const Part*> parts;
		parts.insert(oPart);
		parts.insert(nextPart);
		return merge_parts(parts);
	}
	else
		return false;
}

bool merge_selected_parts()
{
	set<const Part*> temp = get_all_selected_parts();
	return merge_parts(temp);
}

bool merge_parts(const set<const Part*>& parts)
{
	set<const Track*> tracks;
	for (set<const Part*>::iterator it=parts.begin(); it!=parts.end(); it++)
		tracks.insert( (*it)->track() );

	Undo operations;
	
	// process tracks separately
	for (set<const Track*>::iterator t_it=tracks.begin(); t_it!=tracks.end(); t_it++)
	{
		const Track* track=*t_it;

		unsigned begin=INT_MAX, end=0;
		const Part* first_part=NULL;
		
		// find begin of the first and end of the last part
		for (set<const Part*>::iterator it=parts.begin(); it!=parts.end(); it++)
			if ((*it)->track()==track)
			{
				const Part* p=*it;
				if (p->tick() < begin)
				{
					begin=p->tick();
					first_part=p;
				}
				
				if (p->endTick() > end)
					end=p->endTick();
			}
		
		if (begin==INT_MAX || end==0)
		{
			printf("THIS SHOULD NEVER HAPPEN: begin==INT_MAX || end==0 in merge_parts()\n");
			continue; // skip the actual work, as we cannot work under errornous conditions.
		}
		
		// create and prepare the new part
		Part* new_part = first_part->duplicateEmpty();
		new_part->setTick(begin);
		new_part->setLenTick(end-begin);
		
		// copy all events from the source parts into the new part
		for (set<const Part*>::iterator p_it=parts.begin(); p_it!=parts.end(); p_it++)
			if ((*p_it)->track()==track)
			{
				const EventList& old_el= (*p_it)->events();
				for (ciEvent ev_it=old_el.begin(); ev_it!=old_el.end(); ev_it++)
				{
					Event new_event=ev_it->second.clone();
					new_event.setTick( new_event.tick() + (*p_it)->tick() - new_part->tick() );
					new_part->addEvent(new_event);
				}
			}
		
		// delete all the source parts
		for (set<const Part*>::iterator it=parts.begin(); it!=parts.end(); it++)
			if ((*it)->track()==track)
				operations.push_back( UndoOp(UndoOp::DeletePart, *it) );
		// and add the new one
		operations.push_back( UndoOp(UndoOp::AddPart, new_part) );
	}
	
	return MusEGlobal::song->applyOperationGroup(operations);
}

bool split_part(const Part* part, int tick)
{
	int l1 = tick - part->tick();
	int l2 = part->lenTick() - l1;
	if (l1 <= 0 || l2 <= 0)
			return false;
	Part* p1;
	Part* p2;
	part->splitPart(tick, p1, p2);
	
	MusEGlobal::song->informAboutNewParts(part, p1);
	MusEGlobal::song->informAboutNewParts(part, p2);

	Undo operations;
	operations.push_back(UndoOp(UndoOp::DeletePart, part));
	operations.push_back(UndoOp(UndoOp::AddPart, p1));
	operations.push_back(UndoOp(UndoOp::AddPart, p2));
	return MusEGlobal::song->applyOperationGroup(operations);
}

bool delete_selected_parts()
{
	Undo operations;
	bool partSelected = false;

	TrackList* tl = MusEGlobal::song->tracks();

	for (iTrack it = tl->begin(); it != tl->end(); ++it)
	{
		PartList* pl = (*it)->parts();
		for (iPart ip = pl->begin(); ip != pl->end(); ++ip)
		{
				if (ip->second->selected())
				{
					operations.push_back(UndoOp(UndoOp::DeletePart,ip->second));
					partSelected = true;
					}
				}
		}
	
	MusEGlobal::song->applyOperationGroup(operations);
	
	return partSelected;
}

} // namespace MusECore
