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
#include "helper.h"
#include "xml_statistics.h"
#include "utils.h"

#include "event.h"
#include "audio.h"
#include "gconfig.h"
#include "sig.h"
#include "muse_time.h"

#include "function_dialogs/velocity.h"
#include "function_dialogs/quantize.h"
#include "function_dialogs/crescendo.h"
#include "function_dialogs/gatetime.h"
#include "function_dialogs/remove.h"
#include "function_dialogs/transpose.h"
#include "function_dialogs/setlen.h"
#include "function_dialogs/move.h"
#include "function_dialogs/deloverlaps.h"
#include "function_dialogs/legato.h"
#include "components/pasteeventsdialog.h"

#include <limits.h>
#include <iostream>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <stdint.h>
#ifdef _WIN32
#include "mman.h"
#include "mman.c"
#else
#include <sys/mman.h>
#endif
#include "muse_math.h"

#include <QTemporaryFile>
#include <QByteArray>
#include <QDrag>
#include <QMessageBox>
#include <QClipboard>
#include <QSet>
#include <QUuid>

// Forwards from header:
#include <QMimeData>
#include "track.h"
#include "part.h"
#include "ctrl.h"

using namespace std;

using MusEGlobal::config;


namespace MusEGui {

FunctionDialogReturnErase erase_items_dialog(const FunctionDialogMode& mode)
{
  erase_dialog->setElements(mode._buttons);
  if(!erase_dialog->exec())
    return FunctionDialogReturnErase();
    
  const int flags = erase_dialog->_ret_flags;
  return FunctionDialogReturnErase(flags & FunctionReturnAllEvents,
                                     flags & FunctionReturnAllParts,
                                     flags & FunctionReturnLooped,
                                     MusEGlobal::song->lPos(), MusEGlobal::song->rPos(),
                                     erase_dialog->velo_thres_used, erase_dialog->velo_threshold,
                                     erase_dialog->len_thres_used, erase_dialog->len_threshold);
}

FunctionDialogReturnCrescendo crescendo_items_dialog(const FunctionDialogMode& mode)
{
  if (MusEGlobal::song->rPos() <= MusEGlobal::song->lPos())
  {
    QMessageBox::warning(nullptr, QObject::tr("Error"), QObject::tr("Please first select the range for crescendo with the loop markers."));
    return FunctionDialogReturnCrescendo();
  }
  
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

bool read_eventlist_and_part(
  Xml& xml, EventList* el, QUuid* part_id,
  PosLen* poslenResult = nullptr, int* numEvents = nullptr,
	const Part** destPart = nullptr, RelevantSelectedEvents_t relevant = AllEventsRelevant, int ctrlNum = 0);

// -----------------------

typedef map<const Part*, unsigned> expand_map_t;
typedef map<const Part*, set<const Part*> > new_part_map_t;
    

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


bool modify_velocity(const set<const Part*>& parts, int range, int rate, int offset)
{
	map<const Event*, const Part*> events = get_events(parts, range);
	Undo operations;
	
	if ( (!events.empty()) && ((rate!=100) || (offset!=0)) )
	{
		for (map<const Event*, const Part*>::iterator it=events.begin(); it!=events.end(); it++)
		{
			const Event& event=*(it->first);
			// This operation can only apply to notes.
			if(event.type() != Note)
			  continue;
      
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
			// This operation can only apply to notes.
			if(event.type() != Note)
			  continue;
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
			// This operation can only apply to notes.
			if(event.type() != Note)
			  continue;
			const Part* part=it->second;

			unsigned int len = event.lenTick(); //prevent compiler warning: comparison signed/unsigned

			len = (len * rate) / 100;
			len += offset;

			if (len <= 0)
				len = 1;
			
			if ((event.tick()+len > part->lenTick()) && (!(part->hasHiddenEvents() & Part::RightEventsHidden)))
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
	int tick_dest1 = MusEGlobal::sigmap.raster1(tick, raster*2); //round down
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
			// This operation can only apply to notes.
			if(event.type() != Note)
			  continue;
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

bool erase_notes(const set<const Part*>& parts, int range, int velo_threshold, bool velo_thres_used, int len_threshold, bool len_thres_used)
{
	map<const Event*, const Part*> events = get_events(parts, range);
	Undo operations;
	
	if (!events.empty())
	{
		for (map<const Event*, const Part*>::iterator it=events.begin(); it!=events.end(); it++)
		{
			// This operation can apply to any event...
			const Event& event=*(it->first);
			const Part* part=it->second;

			if ( (!velo_thres_used && !len_thres_used) ||
			     (velo_thres_used && event.velo() < velo_threshold) ||
			     (len_thres_used && int(event.lenTick()) < len_threshold) )
				operations.push_back(UndoOp(UndoOp::DeleteEvent, event, part, false, false));
		}
		
		return MusEGlobal::song->applyOperationGroup(operations);
	}
	else
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
			// This operation can only apply to notes.
			if(event.type() != Note)
			  continue;
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
			// This operation can only apply to notes.
			if(event.type() != Note)
			  continue;
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
			// This operation can only apply to notes.
			if(event.type() != Note)
			  continue;
      
			const Part* part=it->second;
			bool del=false;

			Event newEvent = event.clone();
			if ((signed)event.tick()+ticks < 0) //don't allow moving before the part's begin
				newEvent.setTick(0);
			else
				newEvent.setTick(event.tick()+ticks);
			
			if (newEvent.endTick() > part->lenTick()) //if exceeding the part's end:
			{
				if (part->hasHiddenEvents() & Part::RightEventsHidden) // auto-expanding is forbidden, clip
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
			// This operation can only apply to notes.
			if(event1.type() != Note)
			  continue;
			const Part* part1=it1->second;
			
			// we may NOT optimize by letting it2 start at (it1 +1); this optimisation
			// is only allowed when events was sorted by time. it is, however, sorted
			// randomly by pointer.
			for (map<const Event*, const Part*>::iterator it2=events.begin(); it2!=events.end(); it2++)
			{
				const Event& event2=*(it2->first);
				// This operation can only apply to notes.
				if(event2.type() != Note)
				  continue;
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
							operations.push_back(UndoOp(UndoOp::DeleteEvent, event2, part2, false, false));
							deleted_events.insert(&event2);
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
			// This operation can only apply to notes.
			if(event1.type() != Note)
			  continue;
			const Part* part1=it1->second;
			
			unsigned len=INT_MAX;
			// we may NOT optimize by letting it2 start at (it1 +1); this optimisation
			// is only allowed when events was sorted by time. it is, however, sorted
			// randomly by pointer.
			for (map<const Event*, const Part*>::iterator it2=events.begin(); it2!=events.end(); it2++)
			{
				const Event& event2=*(it2->first);
				// This operation can only apply to notes.
				if(event2.type() != Note)
				  continue;
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
        return nullptr;

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
        xml.tag(level++, QString("eventlist part_id=\"%1\"").arg((*part)->uuid().toString()));
        for (ciEvent ev=(*part)->events().begin(); ev!=(*part)->events().end(); ev++)
            if (is_relevant(ev->second, *part, range, AllEventsRelevant))
                ev->second.write(level, xml, -start_tick);
        xml.etag(--level, "eventlist");
    }

    QMimeData *mimeData =  file_to_mimedata(tmp, "text/x-muse-groupedeventlists" );
    fclose(tmp);
    return mimeData;
}

void copy_notes(const set<const Part*>& parts, int range)
{
	QMimeData* drag = selected_events_to_mime(parts,range);

	if (drag)
		QApplication::clipboard()->setMimeData(drag, QClipboard::Clipboard);
}

unsigned get_groupedevents_len(const QString& pt)
{
	unsigned maxlen=0;
	
	QByteArray pt_= pt.toUtf8();
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
					QUuid part_id;
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
	QString s = QApplication::clipboard()->text(tmp, QClipboard::Clipboard);
	
	return get_groupedevents_len(s);
}

bool paste_notes(const Part* paste_into_part)
{
	unsigned temp_begin = MusEGlobal::sigmap.raster1(MusEGlobal::song->cpos(),0);
	unsigned temp_end = MusEGlobal::sigmap.raster2(temp_begin + get_clipboard_len(), 0);
	MusEGui::paste_events_dialog->raster = temp_end - temp_begin;
	MusEGui::paste_events_dialog->into_single_part_allowed = (paste_into_part!=nullptr);
	
	if (!MusEGui::paste_events_dialog->exec())
		return false;
		
	paste_notes(MusEGui::paste_events_dialog->max_distance, MusEGui::paste_events_dialog->always_new_part,
	            MusEGui::paste_events_dialog->never_new_part, MusEGui::paste_events_dialog->into_single_part ? paste_into_part : nullptr,
	            MusEGui::paste_events_dialog->number, MusEGui::paste_events_dialog->raster);
	
	return true;
}

void paste_notes(int max_distance, bool always_new_part, bool never_new_part, const Part* paste_into_part, int amount, int raster)
{
	QString tmp="x-muse-groupedeventlists"; // QClipboard::text() expects a QString&, not a QString :(
	QString s = QApplication::clipboard()->text(tmp, QClipboard::Clipboard);
	paste_at(s, MusEGlobal::song->cpos(), max_distance, always_new_part, never_new_part, paste_into_part, amount, raster);
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

	XmlWriteStatistics stats;
	Xml xml(tmp);
	int level = 0;

	for (set<const Part*>::iterator part=parts.begin(); part!=parts.end(); part++)
	{
        (*part)->write(level, xml, true, true, &stats);
    }
    QString mimeString = "text/x-muse-mixedpartlist";
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

// true on success, false on failure
// If poslenResult is valid then destPart, relevant and ctrlNum are meaningful.
bool read_eventlist_and_part(
  Xml& xml, EventList* el, QUuid* part_id,
  PosLen* poslenResult, int* numEvents,
	const Part** destPart, RelevantSelectedEvents_t relevant, int ctrlNum)
{
	*part_id = QUuid();

	PosLen res;
	bool wave = false;
	if(poslenResult && destPart && *destPart)
		wave = (*destPart)->partType() == Part::WavePartType;
	res.setType(wave ? Pos::FRAMES : Pos::TICKS);

	int e_found = 0;
	bool first_found = false;
	unsigned start_time = 0;
	unsigned end_time = 0;


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
				{
					*part_id = QUuid(xml.s2());
					if(poslenResult && destPart && !*destPart)
					{
						*destPart = partFromSerialNumber(*part_id);
						if(!*destPart)
							return false;
						wave = (*destPart)->partType() == Part::WavePartType;
						res.setType(wave ? Pos::FRAMES : Pos::TICKS);
					}
				}
				else
					printf("unknown attribute '%s' in read_eventlist_and_part(), ignoring it...\n", tag.toLocal8Bit().data());
				break;

			case Xml::TagStart:
				if (tag == "event")
				{
					Event e(Note);
					e.read(xml);


					// Only events of the given type are considered.
					const EventType et = e.type();
					switch(et)
					{
						case Note:
							if(!poslenResult || (!wave && (relevant & NotesRelevant)))
							{
								// Don't add Note event types if they have no length.
								// Hm, it is possible for zero-length events to be
								//  accidentally present in the list. So the user should
								//  at least be allowed to cut and paste them. The EventList
								//  class will probably be correcting this condition anyway
								//  when adding the event to the list.
								//if(e.lenValue() == 0)
								//  continue;
								if(!first_found)
								{
									start_time = e.posValue();
									first_found = true;
								}
								if(e.endPosValue() > end_time)
									end_time = e.endPosValue();
								++e_found;
								el->add(e);
							}
						break;

						case Wave:
							if(!poslenResult || (wave && (relevant & WaveRelevant)))
							{
								// Don't add Wave event types if they have no length.
								//if(e.lenValue() == 0)
								//  continue;
								if(!first_found)
								{
									start_time = e.posValue();
									first_found = true;
								}
								if(e.endPosValue() > end_time)
									end_time = e.endPosValue();
								++e_found;
								el->add(e);
							}
						break;

						case Controller:
						case Meta:
						case Sysex:
							if(!poslenResult || (!wave &&
								  ((et == Controller && (relevant & ControllersRelevant) && ((ctrlNum < 0 || e.dataA() == ctrlNum))) ||
								  (et == Meta && (relevant & MetaRelevant)) ||
								  (et == Sysex && (relevant & SysexRelevant)))))
							{
// 							switch(et)
// 							{
// 								case Controller:
// 									if((relevant & ControllersRelevant) == NoEventsRelevant)
// 										continue;
// 									if(ctrlNum >= 0 && e.dataA() != ctrlNum)
// 										continue;
// 								break;
//
// 								case Meta:
// 									if((relevant & MetaRelevant) == NoEventsRelevant)
// 										continue;
// 								break;
//
// 								case Sysex:
// 									if((relevant & SysexRelevant) == NoEventsRelevant)
// 										continue;
// 								break;
//
// 								default:
// 									continue;
// 								break;
// 							}
								// For these events, even if duplicates are already found at this position,
								//  the range is still the same, which simplifies this code - go ahead and count it...

								if(!first_found)
								{
									start_time = e.posValue();
									first_found = true;
								}
								// For these events, minimum 1 unit time, to qualify as a valid 'range'.
								if((e.posValue() + 1) > end_time)
									end_time = e.posValue() + 1;
								++e_found;
								el->add(e);
							}
						break;
					}

				}
				else
					xml.unknown("read_eventlist_and_part");
				break;

			case Xml::TagEnd:
				if (tag == "eventlist")
				{
					res.setPosValue(start_time);
					res.setLenValue(end_time - start_time);
					if(poslenResult)
						*poslenResult = res;
					if(numEvents)
						*numEvents = e_found;
					return true;
				}

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
	
	QByteArray pt_= pt.toUtf8();
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
					QUuid part_id;

					if (read_eventlist_and_part(xml, &el, &part_id))
					{
						const Part* dest_part;
						Track* dest_track;
						const Part* old_dest_part;
						
						if (paste_into_part == nullptr)
							dest_part = partFromSerialNumber(part_id);
						else
							dest_part=paste_into_part;
						
						if (dest_part == nullptr)
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
									dest_part = nullptr;
									Part* newpart = dest_track->newPart();
									if(newpart)
									{
										newpart->setTick(MusEGlobal::sigmap.raster1(first_paste_tick, config.division));
										dest_part = newpart;
										new_part_map[old_dest_part].insert(dest_part);
										operations.push_back(UndoOp(UndoOp::AddPart, dest_part));
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
											if (dest_part->hasHiddenEvents() & Part::RightEventsHidden) // auto-expanding is forbidden?
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
																break;
															}
															
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
													operations.push_back(UndoOp(UndoOp::DeleteEvent, ie->second, dest_part, true, true));
												}
												
												// Do port controller values and clone parts. 
												operations.push_back(UndoOp(UndoOp::AddEvent, e, dest_part, true, true));
											}
											break;
											
											case Sysex:
											{
												EventList el;
												// Compare time and sysex data only.
												dest_part->events().findSimilarType(e, el, true);
												// Do NOT add the new sysex if it already exists at the position.
												// Don't even bother replacing it using DeletEvent or ModifyEvent.
												if(el.empty())
												{
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
															break;
														}
														
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
												// Don't even bother replacing it using DeletEvent or ModifyEvent.
												if(el.empty())
												{
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
															break;
														}
														
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

void select_all(const set<const Part*>& parts)
{
	Undo operations;
	operations.combobreaker=true;
	
	for (set<const Part*>::iterator part=parts.begin(); part!=parts.end(); part++)
		for (ciEvent ev_it=(*part)->events().begin(); ev_it!=(*part)->events().end(); ev_it++)
		{
			const Event& event=ev_it->second;
			operations.push_back(UndoOp(UndoOp::SelectEvent,event, *part, true, event.selected()));
		}
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
			operations.push_back(UndoOp(UndoOp::SelectEvent,event, *part, false, event.selected()));
		}
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
			operations.push_back(UndoOp(UndoOp::SelectEvent,event, *part, !event.selected(), event.selected()));
		}
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
			operations.push_back(UndoOp(UndoOp::SelectEvent,event, *part,
         (event.tick()>=MusEGlobal::song->lpos() && event.endTick()<=MusEGlobal::song->rpos()), event.selected()));
		}
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
			operations.push_back(UndoOp(UndoOp::SelectEvent,event, *part,
        !(event.tick()>=MusEGlobal::song->lpos() && event.endTick()<=MusEGlobal::song->rpos()), event.selected()));
		}
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
					operations.push_back(UndoOp(UndoOp::ModifyPartLength, part->second, part->second->lenValue(), len, 0, Pos::TICKS));
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
				operations.push_back(UndoOp(UndoOp::ModifyPartLength, part_it, old_len, new_len, 0, part->type()));
			
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
					operations.push_back(UndoOp(UndoOp::ModifyPartLength, part->second, part->second->lenValue(), len, 0, Pos::TICKS));
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
				} while ((part_it!=part->second) && (part_it!=nullptr));

				
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
		const Part* first_part=nullptr;
		
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
			continue; // skip the actual work, as we cannot work under errorneous conditions.
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

bool delete_selected_parts(Undo& operations)
{
	bool partSelected = false;
	TrackList* tl = MusEGlobal::song->tracks();
	for (iTrack it = tl->begin(); it != tl->end(); ++it)
	{
		//if(!(*it)->isVisible())
			//continue;
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
	return partSelected;
}

bool delete_selected_parts()
{
	Undo operations;
  const bool partSelected = delete_selected_parts(operations);
	MusEGlobal::song->applyOperationGroup(operations);
	return partSelected;
}

bool delete_selected_audio_automation(Undo& operations)
{
  bool changed = false;
  TrackList* tl = MusEGlobal::song->tracks();
  for (iTrack it = tl->begin(); it != tl->end(); ++it)
  {
    if((*it)->isMidiTrack() /*|| !(*it)->isVisible()*/)
      continue;
    AudioTrack* at = static_cast<AudioTrack*>(*it);
    for(iCtrlList icl = at->controller()->begin(); icl != at->controller()->end(); ++icl)
    {
      if(!icl->second->isVisible() || icl->second->dontShow())
        continue;
      for(iCtrl ic = icl->second->begin(); ic != icl->second->end(); ++ic)
      {
        if(!ic->second.selected())
          continue;
        operations.push_back(UndoOp(
          UndoOp::DeleteAudioCtrlVal, at, icl->second->id(), ic->first, double(0), double(0), double(0)));
        changed = true;
      }
    }
  }
  return changed;
}

bool delete_selected_audio_automation()
{
  Undo operations;
  const bool changed = delete_selected_audio_automation(operations);
  MusEGlobal::song->applyOperationGroup(operations);
  return changed;
}

bool delete_selected_parts_and_audio_automation()
{
  Undo operations;
  bool changed = delete_selected_parts(operations) || delete_selected_audio_automation(operations);
  MusEGlobal::song->applyOperationGroup(operations);
  return changed;
}

//=============================================================================
// BEGIN item-based functions
//=============================================================================


// For erasing existing target controller events before pasting source controller events.
typedef std::pair<unsigned long /*t0*/, unsigned long /*t1*/ > PasteEraseMapInsertPair_t;
typedef std::map<unsigned long /*t0*/, unsigned long /*t1*/> PasteEraseMap_t;
typedef PasteEraseMap_t::iterator iPasteEraseMap;
typedef PasteEraseMap_t::const_iterator ciPasteEraseMap;
typedef std::pair<int /*ctlnum*/, PasteEraseMap_t > PasteEraseCtlMapPair_t;
typedef std::map<int /*ctlnum*/, PasteEraseMap_t> PasteEraseCtlMap_t;
typedef PasteEraseCtlMap_t::iterator iPasteEraseCtlMap;
typedef PasteEraseCtlMap_t::const_iterator ciPasteEraseCtlMap;

class PasteEraseCtlMap : public PasteEraseCtlMap_t
{
  private:
    bool _erase_controllers_wysiwyg;
    bool _erase_controllers_inclusive;

  public:
    PasteEraseCtlMap(bool erase_controllers_wysiwyg,
                     bool erase_controllers_inclusive) :
                     _erase_controllers_wysiwyg(erase_controllers_wysiwyg),
                     _erase_controllers_inclusive(erase_controllers_inclusive) { }
    
    // Add an item. All ctl_time must be sorted beforehand.
    // Be sure to call tidy() after all items have been added.
    void add(int ctl_num, unsigned int ctl_time,
             unsigned int len_val);
    // Tidy up the very last items in the list.
    void tidy();
};

void PasteEraseCtlMap::add(int ctl_num, unsigned int ctl_time,
                           unsigned int len_val)
{
  unsigned long ctl_end_time;

  if(len_val > 0)
    ctl_end_time = ctl_time + len_val;
  else
    ctl_end_time = ctl_time + 1;

  iPasteEraseCtlMap icm = find(ctl_num);
  if(icm == end())
  {
    PasteEraseMap_t new_tmap;
    new_tmap.insert(PasteEraseMapInsertPair_t(ctl_time, ctl_end_time));
    insert(PasteEraseCtlMapPair_t(ctl_num, new_tmap));
  }
  else
  {
    PasteEraseMap_t& tmap = icm->second;
    // The event times must be sorted already. So this would always return end().
    //iPasteEraseMap itm = tmap.upper_bound(ctl_time);
    iPasteEraseMap itm = tmap.end();

    if(itm != tmap.begin())
    {
      --itm;

      iPasteEraseMap prev_itm_2 = tmap.end();
      if(itm != tmap.begin())
      {
        prev_itm_2 = itm;
        --prev_itm_2;
      }

      if((itm->second >= ctl_time) || _erase_controllers_inclusive)
      {
        if(_erase_controllers_inclusive)
                    itm->second = ctl_time;
        
        if(prev_itm_2 != tmap.end())
        {
          if((prev_itm_2->second >= itm->first) || _erase_controllers_inclusive)
          {
            prev_itm_2->second = itm->second;
            tmap.erase(itm);
          }
        }

        tmap.insert(PasteEraseMapInsertPair_t(ctl_time, ctl_end_time));
      }
      else
      {
        // If we want wysiwyg pasting, we erase existing events up to
        //  the end-time of the last tmap item which ended a contiguous
        //  'cluster' of items. Otherwise we ONLY erase UP TO AND INCLUDING
        //  the start-time of that last tmap item. So we 'truncate' that
        //  last item in the cluster by setting the end-time to the start-time,
        //  so that the gathering routine below knows to erase that last
        //  single-time position.
        
        if(!_erase_controllers_wysiwyg)
          itm->second = itm->first + 1;

        if(prev_itm_2 != tmap.end())
        {
          if(prev_itm_2->second >= itm->first)
          {
            prev_itm_2->second = itm->second;
            tmap.erase(itm);
          }
        }

        tmap.insert(PasteEraseMapInsertPair_t(ctl_time, ctl_end_time));
      }
    }
  }
}

void PasteEraseCtlMap::tidy()
{
  // Tidy up the very last items in the list.
  for(iPasteEraseCtlMap icm = begin(); icm != end(); ++icm)
  {
    PasteEraseMap_t& tmap = icm->second;
    iPasteEraseMap itm = tmap.end();
    if(itm != tmap.begin())
    {
      --itm;
      
      if(!_erase_controllers_wysiwyg)
        itm->second = itm->first + 1;
      
      if(itm != tmap.begin())
      {
        iPasteEraseMap itm_2 = itm;
        --itm_2;
        if((itm_2->second >= itm->second) || _erase_controllers_inclusive)
        {
          itm_2->second = itm->second;
          tmap.erase(itm);
        }
      }
    }
  }
}

bool erase_items(TagEventList* tag_list, int velo_threshold, bool velo_thres_used, int len_threshold, bool len_thres_used)
{
  Undo operations;
  
  const Part* part;
    
  for(ciTagEventList itl = tag_list->begin(); itl != tag_list->end(); ++itl)
  {
    part = itl->part();
    const EventList& el = itl->evlist();
    for(ciEvent ie = el.begin(); ie != el.end(); ie++)
    {
      // This operation can apply to any event.
      const Event& e = ie->second;

      // FIXME TODO Likely need agnostic Pos or frames rather than ticks if WaveCanvas is to use this.
      if ( e.type() != Note || (!velo_thres_used && !len_thres_used) ||
              (velo_thres_used && e.velo() < velo_threshold) ||
            (len_thres_used && int(e.lenTick()) < len_threshold) )
      {
        operations.push_back(UndoOp(UndoOp::DeleteEvent, e, part, true, true));
      }
    }
  }
  
  return MusEGlobal::song->applyOperationGroup(operations);
}

bool crescendo_items(TagEventList* tag_list, int start_val, int end_val, bool absolute)
{
  const Pos& from = MusEGlobal::song->lPos();
  const Pos& to = MusEGlobal::song->rPos();
  if(to <= from)
    return false;
  
  Undo operations;
  Pos pos;
  float curr_val;
  unsigned int pos_val = (to - from).posValue();
  const Part* part;
    
  for(ciTagEventList itl = tag_list->begin(); itl != tag_list->end(); ++itl)
  {
    part = itl->part();
    const EventList& el = itl->evlist();
    for(ciEvent ie = el.begin(); ie != el.end(); ie++)
    {
      const Event& e = ie->second;

      // This operation can only apply to notes.
      if(e.type() != Note)
        continue;

      pos = e.pos() + *part;
      curr_val = (float)start_val + (float)(end_val - start_val) * (pos - from).posValue() / pos_val;

      Event newEvent = e.clone();
      int velo = e.velo();

      if (absolute)
        velo=curr_val;
      else
        velo=curr_val*velo/100;

      if (velo > 127) velo=127;
      if (velo <= 0) velo=1;
      newEvent.setVelo(velo);
      
      operations.push_back(UndoOp(UndoOp::ModifyEvent, newEvent, e, part, false, false));
    }
  }
  
  return MusEGlobal::song->applyOperationGroup(operations);
}

bool delete_overlaps_items(TagEventList* tag_list)
{
  Undo operations;
  
  set<const Event*> deleted_events;
  int new_len;
  Event new_event1;
  const Part* part;
    
  for(ciTagEventList itl = tag_list->begin(); itl != tag_list->end(); ++itl)
  {
    part = itl->part();
    const EventList& el = itl->evlist();
    for(ciEvent ie = el.begin(); ie != el.end(); ie++)
    {
      const Event& e = ie->second;

      // This operation can only apply to notes.
      if(e.type() != Note)
        continue;
      
      // Has this event already been scheduled for deletion? Ignore it.
      if(deleted_events.find(&e) != deleted_events.end())
        continue;
            
      ciEvent ie2 = ie;
      ++ie2;
      for( ; ie2 != el.end(); ++ie2)
      {
        const Event& e2 = ie2->second;
        // This operation can only apply to notes.
        if(e2.type() != Note)
          continue;
        
        // Do e2 and e point to the same event? Or has e2 already been scheduled for deletion? Ignore it.
        if(e == e2 || deleted_events.find(&e2) != deleted_events.end())
          continue;
        
        if ( (e.pitch() == e2.pitch()) &&
            (e.tick() <= e2.tick()) &&
            (e.endTick() > e2.tick()) ) //they overlap
        {
          new_len = e2.tick() - e.tick();

          if(new_len==0)
          {
            operations.push_back(UndoOp(UndoOp::DeleteEvent, e2, part, false, false));
            deleted_events.insert(&e2);
          }
          else
          {
            new_event1 = e.clone();
            new_event1.setLenTick(new_len);
            
            operations.push_back(UndoOp(UndoOp::ModifyEvent, new_event1, e, part, false, false));
            
            // After resizing the event, it should not be necessary to continue with any further
            //  events in this loop since any more sorted events will come at or AFTER e2's position
            //  which we have just resized the end of e to.
            break;
          }
        }
      }
    }
  }
  
  return MusEGlobal::song->applyOperationGroup(operations);
}

bool modify_notelen_items(TagEventList* tag_list, int rate, int offset)
{
  if(rate == 100 && offset == 0)
    return false;
    
  Undo operations;
  
  unsigned int len;
  map<const Part*, int> partlen;
  Event newEvent;
  const Part* part;
    
  for(ciTagEventList itl = tag_list->begin(); itl != tag_list->end(); ++itl)
  {
    part = itl->part();
    const EventList& el = itl->evlist();
    for(ciEvent ie = el.begin(); ie != el.end(); ie++)
    {
      const Event& e = ie->second;

      // This operation can only apply to notes.
      if(e.type() != Note)
        continue;
      
      len = e.lenTick(); //prevent compiler warning: comparison signed/unsigned

      len = (len * rate) / 100;
      len += offset;

      if (len <= 0)
        len = 1;
      
      if ((e.tick()+len > part->lenTick()) && (!(part->hasHiddenEvents() & Part::RightEventsHidden)))
        partlen[part] = e.tick() + len; // schedule auto-expanding
        
      if (e.lenTick() != len)
      {
        newEvent = e.clone();
        newEvent.setLenTick(len);
        operations.push_back(UndoOp(UndoOp::ModifyEvent, newEvent, e, part, false, false));
      }
    }
    
    for (map<const Part*, int>::iterator it=partlen.begin(); it!=partlen.end(); it++)
      schedule_resize_all_same_len_clone_parts(it->first, it->second, operations);
  }
  
  return MusEGlobal::song->applyOperationGroup(operations);
}

bool legato_items(TagEventList* tag_list, int min_len, bool dont_shorten)
{
  Undo operations;
  
  if (min_len<=0) min_len=1;
  
  unsigned len = INT_MAX;
  bool relevant;
  Event new_event1;
  const Part* part;
    
  for(ciTagEventList itl = tag_list->begin(); itl != tag_list->end(); ++itl)
  {
    part = itl->part();
    const EventList& el = itl->evlist();
    for(ciEvent ie = el.begin(); ie != el.end(); ie++)
    {
      const Event& e = ie->second;

      // This operation can only apply to notes.
      if(e.type() != Note)
        continue;
      
      ciEvent ie2 = ie;
      ++ie2;
      for( ; ie2 != el.end(); ++ie2)
      {
        const Event& e2 = ie2->second;
        // This operation can only apply to notes.
        if(e2.type() != Note)
          continue;

        relevant = (e2.tick() >= e.tick() + min_len);
        if (dont_shorten)
          relevant = relevant && (e2.tick() >= e.endTick());
        
        if ( relevant &&                      // they're not too near (respect min_len and dont_shorten)
              (e2.tick() - e.tick() < len ) )  // that's the nearest relevant following note
          len = e2.tick() - e.tick();
      }            
      
      if (len==INT_MAX) len = e.lenTick(); // if no following note was found, keep the length
      
      if (e.lenTick() != len)
      {
        new_event1 = e.clone();
        new_event1.setLenTick(len);
        
        operations.push_back(UndoOp(UndoOp::ModifyEvent, new_event1, e, part, false, false));
      }
    }
  }
  
  return MusEGlobal::song->applyOperationGroup(operations);
}

bool move_items(TagEventList* tag_list, signed int ticks)
{
  if(ticks == 0)
    return false;
  
  Undo operations;
  map<const Part*, int> partlen;
  
  bool del;
  Event newEvent;
  const Part* part;
    
  for(ciTagEventList itl = tag_list->begin(); itl != tag_list->end(); ++itl)
  {
    part = itl->part();
    const EventList& el = itl->evlist();
    for(ciEvent ie = el.begin(); ie != el.end(); ie++)
    {
      const Event& e = ie->second;

      del = false;
      
      // This operation can only apply to notes.
      if(e.type() != Note)
        continue;
      
      newEvent = e.clone();
      if ((signed)e.tick() + ticks < 0) //don't allow moving before the part's begin
        newEvent.setTick(0);
      else
        newEvent.setTick(e.tick() + ticks);
      
      if (newEvent.endTick() > part->lenTick()) //if exceeding the part's end:
      {
        if (part->hasHiddenEvents() & Part::RightEventsHidden) // auto-expanding is forbidden, clip
        {
          if (part->lenTick() > newEvent.tick())
            newEvent.setLenTick(part->lenTick() - newEvent.tick());
          else
            del = true; //if the new length would be <= 0, erase the note
        }
        else
          partlen[part] = newEvent.endTick(); // schedule auto-expanding
      }
      
      if (del == false)
        operations.push_back(UndoOp(UndoOp::ModifyEvent, newEvent, e, part, false, false));
      else
        operations.push_back(UndoOp(UndoOp::DeleteEvent, e, part, false, false));
    }
    
    for (map<const Part*, int>::iterator it=partlen.begin(); it!=partlen.end(); it++)
      schedule_resize_all_same_len_clone_parts(it->first, it->second, operations);
  }
  
  return MusEGlobal::song->applyOperationGroup(operations);
}

bool quantize_items(TagEventList* tag_list, int raster_idx, bool quant_len, int strength, int swing, int threshold)
{
  const int rv = MusEGui::functionQuantizeRasterVals[raster_idx];
  if(rv <= 0)
    return false;
  
  const int raster = (MusEGlobal::config.division*4) / rv;
  
  Undo operations;
  
  unsigned begin_tick;
  int begin_diff;
  unsigned len;
  unsigned end_tick;
  int len_diff;
  Event newEvent;
  const Part* part;
    
  for(ciTagEventList itl = tag_list->begin(); itl != tag_list->end(); ++itl)
  {
    part = itl->part();
    const EventList& el = itl->evlist();
    for(ciEvent ie = el.begin(); ie != el.end(); ie++)
    {
      const Event& e = ie->second;

      // This operation can only apply to notes.
      if(e.type() != Note)
        continue;
      
      begin_tick = e.tick() + part->tick();
      begin_diff = quantize_tick(begin_tick, raster, swing) - begin_tick;

      if (abs(begin_diff) > threshold)
        begin_tick = begin_tick + begin_diff*strength/100;

      len = e.lenTick();
      
      end_tick = begin_tick + len;
      len_diff = quantize_tick(end_tick, raster, swing) - end_tick;
        
      if ((abs(len_diff) > threshold) && quant_len)
        len = len + len_diff*strength/100;

      if (len <= 0)
        len = 1;

        
      if ( (e.lenTick() != len) || (e.tick() + part->tick() != begin_tick) )
      {
        newEvent = e.clone();
        newEvent.setTick(begin_tick - part->tick());
        newEvent.setLenTick(len);
        operations.push_back(UndoOp(UndoOp::ModifyEvent, newEvent, e, part, false, false));
      }
    }
  }
  
  return MusEGlobal::song->applyOperationGroup(operations);
}

bool set_notelen_items(TagEventList* tag_list, int len)
{
  return modify_notelen_items(tag_list, 0, len);
}

bool transpose_items(TagEventList* tag_list, signed int halftonesteps)
{
  if(halftonesteps == 0)
    return false;
  
  Undo operations;
  
  Event newEvent;
  int pitch;
  const Part* part;
    
  for(ciTagEventList itl = tag_list->begin(); itl != tag_list->end(); ++itl)
  {
    part = itl->part();
    const EventList& el = itl->evlist();
    for(ciEvent ie = el.begin(); ie != el.end(); ie++)
    {
      const Event& e = ie->second;

      // This operation can only apply to notes.
      if(e.type() != Note)
        continue;
      
      newEvent = e.clone();
      pitch = e.pitch() + halftonesteps;
      if (pitch > 127) pitch = 127;
      if (pitch < 0) pitch = 0;
      newEvent.setPitch(pitch);
      operations.push_back(UndoOp(UndoOp::ModifyEvent, newEvent, e, part, false, false));
    }
  }
  
  return MusEGlobal::song->applyOperationGroup(operations);
}

bool modify_velocity_items(TagEventList* tag_list, int rate, int offset)
{
  if(rate == 100 && offset == 0)
    return false;
  
  Undo operations;
  int velo;
  Event newEvent;
  const Part* part;
    
  for(ciTagEventList itl = tag_list->begin(); itl != tag_list->end(); ++itl)
  {
    part = itl->part();
    const EventList& el = itl->evlist();
    for(ciEvent ie = el.begin(); ie != el.end(); ie++)
    {
      const Event& e = ie->second;

      // This operation can only apply to notes.
      if(e.type() != Note)
        continue;
      
      velo = e.velo();

      velo = (velo * rate) / 100;
      velo += offset;

      if (velo <= 0)
        velo = 1;
      else if (velo > 127)
        velo = 127;
        
      if (e.velo() != velo)
      {
        newEvent = e.clone();
        newEvent.setVelo(velo);
        operations.push_back(UndoOp(UndoOp::ModifyEvent, newEvent, e, part, false, false));
      }
    }
  }
  
  return MusEGlobal::song->applyOperationGroup(operations);
}

bool modify_off_velocity_items(TagEventList* tag_list, int rate, int offset)
{
  if(rate == 100 && offset == 0)
    return false;
  
  Undo operations;
  int velo;
  Event newEvent;
  const Part* part;

  for(ciTagEventList itl = tag_list->begin(); itl != tag_list->end(); ++itl)
  {
    part = itl->part();
    const EventList& el = itl->evlist();
    for(ciEvent ie = el.begin(); ie != el.end(); ie++)
    {
      const Event& e = ie->second;
      
      // This operation can only apply to notes.
      if(e.type() != Note)
        continue;
      
      velo = e.veloOff();

      velo = (velo * rate) / 100;
      velo += offset;

      if (velo <= 0)
        velo = 1;
      else if (velo > 127)
        velo = 127;
        
      if (e.veloOff() != velo)
      {
        newEvent = e.clone();
        newEvent.setVeloOff(velo);
        operations.push_back(UndoOp(UndoOp::ModifyEvent, newEvent, e, part, false, false));
      }
    }
  }
  
  return MusEGlobal::song->applyOperationGroup(operations);
}

bool readAudioAutomation(MusECore::Xml& xml, MusECore::PasteCtrlTrackMap* pctm)
{
  QUuid trackUuid;
  MusECore::PasteCtrlListList pcll;

  for (;;) {
        MusECore::Xml::Token token = xml.parse();
        const QString& tag = xml.s1();
        switch (token) {
              case MusECore::Xml::Error:
              case MusECore::Xml::End:
                    return false;
              case MusECore::Xml::TagStart:
                    if (tag == "controller")
                    {
                      MusECore::PasteCtrlListStruct pcls;
                      if(!pcls._ctrlList.read(xml) || pcls._ctrlList.id() < 0)
                        return false;
                      if(!pcls._ctrlList.empty())
                        pcls._minFrame = pcls._ctrlList.cbegin()->first;
                      pcll.add(pcls._ctrlList.id(), pcls);
                    }
                    else
                      xml.unknown("readAudioAutomation");
                    break;
              case MusECore::Xml::Attribut:
                    if (tag == "trackUuid")
                      trackUuid = QUuid(xml.s2());
                    else
                      fprintf(stderr, "readAudioAutomation unknown tag %s\n", tag.toLocal8Bit().constData());
                    break;
              case MusECore::Xml::TagEnd:
                    if (tag == "audioTrackAutomation")
                    {
                      if(!trackUuid.isNull())
                        pctm->add(trackUuid, pcll);
                      return true;
                    }
              default:
                    break;
              }
        }
  return false;
}

void parseArrangerPasteXml(
  const QString& pt, MusECore::Track* track,
  bool clone, bool /*toTrack*/, set<MusECore::Track*>* affected_tracks,
  std::set<MusECore::Part*>* partList,
  MusECore::XmlReadStatistics* stats,
  MusECore::PasteCtrlTrackMap* pctm,
  unsigned int* minPos, bool* minPosValid)
{
      QByteArray ba = pt.toUtf8();
      const char* ptxt = ba.constData();
      MusECore::Xml xml(ptxt);
      unsigned int  minPartPos=0;
      unsigned int  minCtrlPos=0;
      bool minPartPosValid = false;
      bool minCtrlPosValid = false;
      int  notDone = 0;
      int  done = 0;
      bool end = false;
      if(minPos)
        *minPos = 0;
      if(minPosValid)
        *minPosValid = false;

      // NOTE: Here the xml just holds a collection of copied objects.
      // There is no top-level tag containing the objects thus no TagEnd, only End.
      for (;;) {
            MusECore::Xml::Token token = xml.parse();
            const QString& tag = xml.s1();
            switch (token) {
                  case MusECore::Xml::Error:
                        end = true;
                        break;
                  case MusECore::Xml::End:
                        {
                          if(minPos && (minPartPosValid || minCtrlPosValid))
                          {
                            bool haveMinCtrlPos = false;
                            if(pctm && minCtrlPosValid)
                            {
                              *minPos = MusECore::Pos::convert(minCtrlPos, MusECore::Pos::FRAMES, MusECore::Pos::TICKS);
                              haveMinCtrlPos = true;
                              if(minPosValid)
                                *minPosValid = true;
                            }
                            if(partList && minPartPosValid && (!haveMinCtrlPos || minPartPos < *minPos))
                            {
                              *minPos = minPartPos;
                              if(minPosValid)
                                *minPosValid = true;
                            }
                          }
                        }
                        end = true;
                        break;
                  case MusECore::Xml::TagStart:
                        if (tag == "part") {
                              if(!partList)
                                xml.skip(tag);
                              else
                              {
                                // Read the part.
                                MusECore::Part* p = 0;
                                p = MusECore::Part::readFromXml(xml, track, stats, clone, false);

                                // If it could not be created...
                                if(!p)
                                {
                                  // Increment the number of parts not done and break.
                                  ++notDone;
                                  break;
                                }

                                // Increment the number of parts done.
                                ++done;

                                p->setSelected(true);
                                partList->insert(p);

                                if (!minPartPosValid || p->tick() < minPartPos)
                                {
                                  minPartPos = p->tick();
                                  minPartPosValid = true;
                                }

                                if (affected_tracks)
                                  affected_tracks->insert(p->track());

                              }
                            }
                        else if(tag == "audioTrackAutomation")
                        {
                              if(!pctm)
                                xml.skip(tag);
                              else
                              {
                                if(readAudioAutomation(xml, pctm))
                                {
                                  if(!pctm->empty() && (!minCtrlPosValid || pctm->_minFrame < minCtrlPos))
                                  {
                                    minCtrlPos = pctm->_minFrame;
                                    minCtrlPosValid = true;
                                  }
                                }
                              }
                        }
                        else
                              xml.unknown("parseArrangerPasteXml");
                        break;
                  case MusECore::Xml::TagEnd:
                        break;
                  default:
                              end = true;
                        break;
                }
                if(end)
                  break;
            }


      if(notDone)
      {
        int tot = notDone + done;
        QMessageBox::critical(nullptr, QString("MusE"),
           (tot > 1  ?  QObject::tr("%n part(s) out of %1 could not be pasted.\nLikely the selected track is the wrong type.","",notDone).arg(tot)
                     :  QObject::tr("%n part(s) could not be pasted.\nLikely the selected track is the wrong type.","",notDone)));
      }
}

void pasteAudioAutomation(MusECore::AudioTrack* track, int ctrlId, /*bool fitToRange,*/ int amount, int raster)
{
  QClipboard* cb  = QApplication::clipboard();
  const QMimeData* md = cb->mimeData(QClipboard::Clipboard);

  const QString pfx("text/");
  QString mxpl("x-muse-mixedpartlist");

  if(!md->hasFormat(pfx + mxpl))
  {
    QMessageBox::critical(nullptr, QString("MusE"),
      QObject::tr("Cannot paste: wrong data type"));
    return;
  }

  const QString txt = cb->text(mxpl, QClipboard::Clipboard);
  if (txt.isEmpty())
    return;

  MusECore::XmlReadStatistics stats;
  MusECore::PasteCtrlTrackMap pctm;

  unsigned int minPos = 0;
  bool minPosValid = false;
  parseArrangerPasteXml(txt, track, false, false, nullptr, nullptr, &stats, &pctm, &minPos, &minPosValid);
  if(!minPosValid)
    return;

  const bool multipleCtrls = pctm.size() > 1 || pctm.cbegin()->second.size() > 1;

  if(multipleCtrls)
  {
    QMessageBox::critical(nullptr, QString("MusE"),
      QObject::tr("Cannot paste automation: Clipboard has multiple controllers.\nOnly one is allowed."));
    return;

// TODO: Dialog to choose which controller to paste !
//           const MusECore::TrackList* tl = MusEGlobal::song->tracks();
//           int ctrlId;
//           unsigned int ctrlFrame;
//           for(MusECore::iPasteCtrlTrackMap ipctm = pctm.begin(); ipctm != pctm.end(); ++ipctm)
//           {
//             const QUuid& trackUuid = ipctm->first;
//             // We can only work with controller data that has a track here.
//             if(trackUuid.isNull())
//               continue;
//             const MusECore::Track* t = tl->findUuid(trackUuid);
//             if(!t || t->isMidiTrack())
//               continue;
//             const MusECore::AudioTrack* at = static_cast<const MusECore::AudioTrack*>(t);
//             const MusECore::CtrlListList* track_cll = at->controller();
//
//             MusECore::PasteCtrlListList& pcll = ipctm->second;
//             for(MusECore::ciPasteCtrlListList ipcll = pcll.cbegin(); ipcll != pcll.cend(); ++ipcll)
//             {
//               const MusECore::PasteCtrlListStruct& pcls = ipcll->second;
//               const MusECore::CtrlList& cl = pcls._ctrlList;
//               if(cl.empty())
//                 continue;
//
//             }
//           }
  }


  const MusECore::CtrlList::PasteEraseOptions eraseOpts = MusEGlobal::config.audioCtrlGraphPasteEraseOptions;
  unsigned int ctrlFrame;
  MusECore::iPasteCtrlTrackMap ipctm = pctm.begin();

  MusECore::PasteCtrlListList& pcll = ipctm->second;
  MusECore::ciPasteCtrlListList ipcll = pcll.cbegin();
  const MusECore::PasteCtrlListStruct& pcls = ipcll->second;
  const MusECore::CtrlList& cl = pcls._ctrlList;
  if(cl.empty())
    return;

  const MusECore::CtrlListList* track_cll = track->controller();
  // Find the controller we are trying to paste to.
  MusECore::ciCtrlList track_icl = track_cll->find(ctrlId);
  // We can only work with existing controllers here.
  if(track_icl == track_cll->end())
    return;
  const MusECore::CtrlList* track_cl = track_icl->second;

  MusECore::Undo operations;
  unsigned int startPos=MusEGlobal::song->vcpos();
  set<MusECore::Track*> affected_tracks;

  // TODO TODO:
  //deselectAll(&operations);

  CtrlList* addCtrlList = new CtrlList(*track_cl, CtrlList::ASSIGN_PROPERTIES);
  CtrlList* eraseCtrlList = new CtrlList(*track_cl, CtrlList::ASSIGN_PROPERTIES);

  for (int i = 0; i < amount; ++i)
  {
    unsigned int groupStartFrame, groupEndFrame;
    MusECore::MuseCount_t ctrlTick;
    double newValue;
    bool isGroupStart = true;
    bool isGroupEnd = false;
    for(MusECore::ciCtrl ic = cl.cbegin(); ic != cl.cend(); ++ic)
    {
      ctrlFrame = ic->first;
      const MusECore::CtrlVal& cv = ic->second;
      newValue = normalizedValueFromRange(cv.value(), &cl); // represent volume between 0 and 1

      //if(fitToRange)
      {
        newValue = normalizedValueToRange(newValue, track_cl);
      }

      // Even in PasteNoErase mode, we must still erase any existing
      //  items at the locations that we wish to paste to.
      if(eraseOpts == MusECore::CtrlList::PasteNoErase)
        isGroupEnd = true;
      // In PasteErase mode, check for group end.
      else if(eraseOpts == MusECore::CtrlList::PasteErase && cv.groupEnd())
        isGroupEnd = true;

      // Check for missing group end and force it if necessary.
      // Also, in PasteEraseRange mode this will catch the last
      //  item and force it to be group end.
      MusECore::ciCtrl ic_next = ic;
      ++ic_next;
      if(ic_next == cl.cend())
        isGroupEnd = true;

      // Convert the position.
      ctrlTick = MusECore::Pos::convert(ctrlFrame, MusECore::Pos::FRAMES, MusECore::Pos::TICKS);
      // Subtract the minimum position and add the paste position, and limit to zero just in case.
      ctrlTick -= (MusECore::MuseCount_t)minPos;
      ctrlTick += (MusECore::MuseCount_t)startPos + i * raster;
      if(ctrlTick < 0)
        ctrlTick = 0;
      // Convert the position back again.
      ctrlFrame = MusECore::Pos::convert(ctrlTick, MusECore::Pos::TICKS, MusECore::Pos::FRAMES);

      if(isGroupStart)
      {
        groupStartFrame = ctrlFrame;
        isGroupStart = false;
      }
      // This is ignored in PasteNoErase mode since isGroupEnd will not be set.
      // In that case erasing items is already done, above.
      if(isGroupEnd)
      {
        groupEndFrame = ctrlFrame;

        MusECore::ciCtrl icEraseStart = track_cl->lower_bound(groupStartFrame);
        MusECore::ciCtrl icEraseEnd = track_cl->upper_bound(groupEndFrame);
        //eraseCtrlList->insert(icEraseStart, icEraseEnd);
        for(MusECore::ciCtrl ice = icEraseStart; ice != icEraseEnd; ++ice)
        {
          // Only if the existing value and desired 'add' value are not precisely equal. Otherwise ignore.
          if(ice->first != ctrlFrame || ice->second.value() != newValue)
            eraseCtrlList->add(ice->first, ice->second);
        }

        // Reset for next iteration.
        isGroupStart = true;
        isGroupEnd = false;
      }

      MusECore::ciCtrl ic_existing = track_cl->find(ctrlFrame);
      // Only if the existing value and desired 'add' value are not precisely equal. Otherwise ignore.
      if(ic_existing == track_cl->cend() || ic_existing->second.value() != newValue)
        addCtrlList->add(ctrlFrame, newValue, /*cv._selected*/ true,
                         // Force a discrete point if the destination cl is discrete or the given value is discrete.
                         track_cl->mode() == CtrlList::DISCRETE || cv.discrete(), cv.groupEnd());
    }
  }
  // If nothing was changed, delete and ignore.
  if(eraseCtrlList->empty())
  {
    delete eraseCtrlList;
    eraseCtrlList = nullptr;
  }
  if(addCtrlList->empty())
  {
    delete addCtrlList;
    addCtrlList = nullptr;
  }
  if(eraseCtrlList || addCtrlList)
  {
    //MusEGlobal::song->endAudioCtrlMoveMode(operations);
    operations.push_back(MusECore::UndoOp(
      MusECore::UndoOp::ModifyAudioCtrlValList, track, ctrlId, eraseCtrlList, addCtrlList));
  }

  //MusECore::Pos p(endPos, true);
  //MusEGlobal::song->setPos(MusECore::Song::CPOS, p);

  MusEGlobal::song->applyOperationGroup(operations);
}

void processArrangerPasteObjects(
  MusECore::Undo& operations,
  unsigned int pos,
  unsigned int* finalPosPtr,
  std::set<MusECore::Part*>* partList,
  MusECore::PasteCtrlTrackMap* pctm,
  unsigned int minPos)
{
      unsigned int  finalPos = pos;
      const MusECore::CtrlList::PasteEraseOptions eraseOpts = MusEGlobal::config.audioCtrlGraphPasteEraseOptions;

      //--------------------
      //  Process parts
      //--------------------
      if(partList)
      {
        MusECore::Part* pt;
        MusECore::MuseCount_t newPos;
        for(std::set<MusECore::Part*>::const_iterator ip = partList->cbegin(); ip != partList->cend(); )
        {
          pt = *ip;
          // We can only work with parts that have a track here.
          if(!pt->track())
          {
            delete pt;
            ip = partList->erase(ip);
            continue;
          }
          // Subtract the minimum position and add the paste position, and limit to zero just in case.
          newPos =
            (MusECore::MuseCount_t)pt->tick() -
            (MusECore::MuseCount_t)minPos +
            (MusECore::MuseCount_t)pos;
          if(newPos < 0)
            newPos = 0;
          pt->setTick(newPos);
          if(newPos + pt->lenTick() > finalPos)
            finalPos = newPos + pt->lenTick();
          operations.push_back(MusECore::UndoOp(MusECore::UndoOp::AddPart, pt));
          ++ip;
        }
      }

      //---------------------------
      //  Process audio automation
      //---------------------------
      if(pctm)
      {
        const MusECore::TrackList* tl = MusEGlobal::song->tracks();
        int ctrlId;
        unsigned int ctrlFrame;
        for(MusECore::iPasteCtrlTrackMap ipctm = pctm->begin(); ipctm != pctm->end(); ++ipctm)
        {
          const QUuid& trackUuid = ipctm->first;
          // We can only work with controller data that has a track here.
          if(trackUuid.isNull())
            continue;
          const MusECore::Track* t = tl->findUuid(trackUuid);
          if(!t || t->isMidiTrack())
            continue;
          const MusECore::AudioTrack* at = static_cast<const MusECore::AudioTrack*>(t);
          const MusECore::CtrlListList* track_cll = at->controller();

          MusECore::PasteCtrlListList& pcll = ipctm->second;
          for(MusECore::ciPasteCtrlListList ipcll = pcll.cbegin(); ipcll != pcll.cend(); ++ipcll)
          {
            const MusECore::PasteCtrlListStruct& pcls = ipcll->second;
            const MusECore::CtrlList& cl = pcls._ctrlList;
            if(cl.empty())
              continue;

            ctrlId = ipcll->first;
            MusECore::ciCtrlList track_icl = track_cll->find(ctrlId);
            // We can only work with existing controllers here.
            if(track_icl == track_cll->end())
              continue;
            const MusECore::CtrlList* track_cl = track_icl->second;

            CtrlList* addCtrlList = new CtrlList(*track_cl, CtrlList::ASSIGN_PROPERTIES);
            CtrlList* eraseCtrlList = new CtrlList(*track_cl, CtrlList::ASSIGN_PROPERTIES);

            unsigned int groupStartFrame, groupEndFrame;
            MusECore::MuseCount_t ctrlTick;
            bool isGroupStart = true;
            bool isGroupEnd = false;
            for(MusECore::ciCtrl ic = cl.cbegin(); ic != cl.cend(); ++ic)
            {
              ctrlFrame = ic->first;
              const MusECore::CtrlVal& cv = ic->second;
              // Even in PasteNoErase mode, we must still erase any existing
              //  items at the locations that we wish to paste to.
              if(eraseOpts == MusECore::CtrlList::PasteNoErase)
                isGroupEnd = true;
              // In PasteErase mode, check for group end.
              else if(eraseOpts == MusECore::CtrlList::PasteErase && cv.groupEnd())
                isGroupEnd = true;

              // Check for missing group end and force it if necessary.
              // Also, in PasteEraseRange mode this will catch the last
              //  item and force it to be group end.
              MusECore::ciCtrl ic_next = ic;
              ++ic_next;
              if(ic_next == cl.cend())
                isGroupEnd = true;

              // Convert the position.
              ctrlTick = MusECore::Pos::convert(ctrlFrame, MusECore::Pos::FRAMES, MusECore::Pos::TICKS);
              // Subtract the minimum position and add the paste position, and limit to zero just in case.
              ctrlTick -= (MusECore::MuseCount_t)minPos;
              ctrlTick += (MusECore::MuseCount_t)pos;
              if(ctrlTick < 0)
                ctrlTick = 0;
              // Convert the position back again.
              ctrlFrame = MusECore::Pos::convert(ctrlTick, MusECore::Pos::TICKS, MusECore::Pos::FRAMES);

              if(isGroupStart)
              {
                groupStartFrame = ctrlFrame;
                isGroupStart = false;
              }
              // This is ignored in PasteNoErase mode since isGroupEnd will not be set.
              // In that case erasing items is already done, above.
              if(isGroupEnd)
              {
                groupEndFrame = ctrlFrame;

                MusECore::ciCtrl icEraseStart = track_cl->lower_bound(groupStartFrame);
                MusECore::ciCtrl icEraseEnd = track_cl->upper_bound(groupEndFrame);
                for(MusECore::ciCtrl ice = icEraseStart; ice != icEraseEnd; ++ice)
                {
                  // Only if the existing value and desired 'add' value are not precisely equal. Otherwise ignore.
                  if(ice->first != ctrlFrame || ice->second.value() != cv.value())
                    eraseCtrlList->add(ice->first, ice->second);
                }

                // Reset for next iteration.
                isGroupStart = true;
                isGroupEnd = false;
              }

              MusECore::ciCtrl ic_existing = track_cl->find(ctrlFrame);
              // Only if the existing value and desired 'add' value are not precisely equal. Otherwise ignore.
              if(ic_existing == track_cl->cend() || ic_existing->second.value() != cv.value())
                addCtrlList->add(ctrlFrame, cv.value(), /*cv._selected*/ true,
                                 // Force a discrete point if the destination cl is discrete or the given value is discrete.
                                 track_cl->mode() == CtrlList::DISCRETE || cv.discrete(), cv.groupEnd());
            }
            // If nothing was changed, delete and ignore.
            if(eraseCtrlList->empty())
            {
              delete eraseCtrlList;
              eraseCtrlList = nullptr;
            }
            if(addCtrlList->empty())
            {
              delete addCtrlList;
              addCtrlList = nullptr;
            }
            if(eraseCtrlList || addCtrlList)
            {
              operations.push_back(MusECore::UndoOp(
                MusECore::UndoOp::ModifyAudioCtrlValList, at, ctrlId, eraseCtrlList, addCtrlList));
            }
          }
        }
      }

      if (finalPosPtr) *finalPosPtr=finalPos;
}

void copy_items(TagEventList* tag_list)
{
 QMimeData* drag = cut_or_copy_tagged_items_to_mime(tag_list);

 if (drag)
   QApplication::clipboard()->setMimeData(drag, QClipboard::Clipboard);
}

bool cut_items(TagEventList* tag_list)
{
  QMimeData* drag = cut_or_copy_tagged_items_to_mime(tag_list, true);

  if(drag)
  {
    QApplication::clipboard()->setMimeData(drag, QClipboard::Clipboard);
    return true;
  }
  
  return false;
}

// if nothing is selected/relevant, this function returns NULL
QMimeData* cut_or_copy_tagged_items_to_mime(TagEventList* tag_list, bool cut_mode)
{
    if(tag_list->empty())
      return nullptr;
  
    QTemporaryFile tmp;
    if(!tmp.open())
    {
        fprintf(stderr, "cut_or_copy_tagged_items_to_mime(): ERROR: Failed to open temporary file\n");
        return nullptr;
    }
    
    const Pos start_pos = tag_list->globalStats().evrange();

    Undo operations;
  
    bool changed = false;
    const Part* part;

    //---------------------------------------------------
    //    write events as XML into tmp file
    //---------------------------------------------------

    Xml xml(&tmp);
    int level = 0;

    for(ciTagEventList itl = tag_list->begin(); itl != tag_list->end(); ++itl)
    {
      part = itl->part();
      const EventList& el = itl->evlist();
      if(el.empty())
        continue;
      
      xml.tag(level++, QString("eventlist part_id=\"%1\"").arg(part->uuid().toString()));
      for(ciEvent ie = el.begin(); ie != el.end(); ie++)
      {
        const Event& e = ie->second;
        Event ne = e.clone();
        ne.setPos(ne.pos() - start_pos);
        ne.write(level, xml, Pos(0, e.pos().type() == Pos::TICKS));
        if(cut_mode)
        {
          changed = true;
          operations.push_back(UndoOp(UndoOp::DeleteEvent, e, part, true, true));
        }
      }
      xml.etag(--level, "eventlist");
    }
    
    tmp.flush();
    tmp.seek(0);
    const QByteArray data = tmp.readAll();
    QMimeData* mimeData = new QMimeData();
    mimeData->setData("text/x-muse-groupedeventlists", data);

    if(changed)
      MusEGlobal::song->applyOperationGroup(operations);
    
    return mimeData;
}

bool paste_items(const std::set<const Part*>& parts, const Part* paste_into_part)
{
	unsigned temp_begin = MusEGlobal::sigmap.raster1(MusEGlobal::song->cpos(),0);
	unsigned temp_end = MusEGlobal::sigmap.raster2(temp_begin + get_clipboard_len(), 0);
	MusEGui::paste_events_dialog->raster = temp_end - temp_begin;
	MusEGui::paste_events_dialog->into_single_part_allowed = (paste_into_part!=nullptr);
	
	if (!MusEGui::paste_events_dialog->exec())
		return false;
		
	paste_items(parts, MusEGui::paste_events_dialog->max_distance,
							FunctionOptionsStruct(
								(MusEGui::paste_events_dialog->ctrl_erase ?             FunctionEraseItems : FunctionNoOptions)
								| (MusEGui::paste_events_dialog->ctrl_erase_wysiwyg ?   FunctionEraseItemsWysiwyg : FunctionNoOptions)
								| (MusEGui::paste_events_dialog->ctrl_erase_inclusive ? FunctionEraseItemsInclusive : FunctionNoOptions)
								| (MusEGui::paste_events_dialog->always_new_part ?      FunctionPasteAlwaysNewPart : FunctionNoOptions)
								| (MusEGui::paste_events_dialog->never_new_part ?       FunctionPasteNeverNewPart : FunctionNoOptions)),
							MusEGui::paste_events_dialog->into_single_part ? paste_into_part : nullptr,
							MusEGui::paste_events_dialog->number, MusEGui::paste_events_dialog->raster,
							AllEventsRelevant,
							-1 /*paste to ctrl num*/
							);
	
	return true;
}

void paste_items(const set<const Part*>& parts, int max_distance,
								 const FunctionOptionsStruct& options,
								 const Part* paste_into_part, int amount, int raster,
								 RelevantSelectedEvents_t relevant,
								 int paste_to_ctrl_num
								 )
{
	QString tmp="x-muse-groupedeventlists"; // QClipboard::text() expects a QString&, not a QString :(
	QString s = QApplication::clipboard()->text(tmp, QClipboard::Clipboard);
	paste_items_at(parts, s, MusEGlobal::song->cPos(), max_distance, options,
								paste_into_part, amount, raster, relevant, paste_to_ctrl_num
								);
}

void pasteEventList(
  const EventList& el,
  const Pos& pos,
  Part* dest_part,
  Undo& operations,
  Undo& add_operations,
  expand_map_t& expand_map,
  new_part_map_t& new_part_map,
  // The source part where the event list came from, in case
  //  the erase_source argument is true.
  const Part* source_part = nullptr,
  // Whether to erase ('cut') the source events after pasting.
  bool erase_source = false,
  // The starting position and length of the range of events in el. For speed this must be know beforehand.
  const PosLen& eventlist_range = PosLen(),
  // The number of events in el. For speed this must be know beforehand.
  int num_events = 0,

  int max_distance=3072,
  // Options. Default is erase target existing controllers first + erase wysiwyg.
  const FunctionOptionsStruct& options = FunctionOptionsStruct(),
  // Number of copies to paste.
  int amount=1,
  // Separation between copies.
  int raster=3072,
  // Choose which events to paste.
  const RelevantSelectedEvents_t relevant = AllEventsRelevant,
  // If pasting controllers, paste into this controller number if not -1.
  // If the source has multiple controllers, user will be asked which one to paste.
  int paste_to_ctrl_num = -1
  )
{
  const bool wave_mode = dest_part->partType() == Part::WavePartType;

  if(num_events <= 0)
    return;
  
  const bool always_new_part             = options._flags & FunctionPasteAlwaysNewPart;
  const bool never_new_part              = options._flags & FunctionPasteNeverNewPart;
  const bool erase_controllers           = options._flags & FunctionEraseItems;
  const bool erase_controllers_wysiwyg   = options._flags & FunctionEraseItemsWysiwyg;
  const bool erase_controllers_inclusive = options._flags & FunctionEraseItemsInclusive;
  
  const Pos::TType time_type = wave_mode ? Pos::FRAMES : Pos::TICKS;
  Track* dest_track = nullptr;
  const Part* old_dest_part = nullptr;
  
  // Be sure to subtract the position of the very first event of interest.
  // This is exactly what the copy/cut functions do before they write the results
  //  to an output file. But here the events in the directly-passed source list
  //  cannot be time-modified beforehand. So here we subtract this start position:

  const unsigned pos_value = pos.posValue(time_type);
  unsigned dest_part_pos_value = dest_part->posValue(time_type);
  unsigned dest_part_end_value = dest_part->end().posValue(time_type);
  dest_track=dest_part->track();
  old_dest_part=dest_part;
  unsigned first_paste_pos_value = pos_value;
  bool create_new_part = ( (first_paste_pos_value < dest_part_pos_value) || // dest_part begins too late
      ( ( (dest_part_end_value + max_distance < first_paste_pos_value) ||   // dest_part is too far away
                          always_new_part ) && !never_new_part ) );    // respect function arguments

  for (int i=0;i<amount;i++)
  {
    unsigned curr_pos = pos_value + i*raster;
    first_paste_pos_value = curr_pos;

    if (create_new_part)
    {
      dest_part = nullptr;
      Part* newpart = dest_track->newPart();
      if(newpart)
      {
        // TODO: Shouldn't we snap to frames for wave parts? But snap to what exactly?
        const unsigned pos_tick = Pos(first_paste_pos_value, !wave_mode).tick();
        const unsigned rast_pos_tick = MusEGlobal::sigmap.raster1(pos_tick, config.division);
        newpart->setTick(rast_pos_tick);
        const unsigned len_rast_off_value = pos_tick >= rast_pos_tick ? pos_tick - rast_pos_tick : 0;
        // TODO: Probably length should be properly converted ???
        newpart->setLenValue(eventlist_range.lenValue() + len_rast_off_value, time_type);
        dest_part = newpart;
        dest_part_pos_value = dest_part->posValue(time_type);
        dest_part_end_value = dest_part->end().posValue(time_type);
        new_part_map[old_dest_part].insert(dest_part);
        add_operations.push_back(UndoOp(UndoOp::AddPart, dest_part));
      }
    }
    
    if(!dest_part)
      continue;
    
    // This will be filled as we go.
    PasteEraseCtlMap ctl_map(erase_controllers_wysiwyg, erase_controllers_inclusive);
  
    const unsigned dest_part_len_value = dest_part->lenValue(time_type);
    for (ciEvent i = el.cbegin(); i != el.cend(); ++i)
    {
      const Event& old_e = i->second;

      // If the destination part is a midi part, any midi event is allowed.
      // If the destination part is a wave part, any wave event is allowed.
      switch(old_e.type())
      {
        case Note:
          if(!(relevant & NotesRelevant) || dest_part->type() == Pos::FRAMES)
            continue;
        break;

        case Controller:
          if(!(relevant & ControllersRelevant) || dest_part->type() == Pos::FRAMES ||
              (paste_to_ctrl_num >= 0 && paste_to_ctrl_num != old_e.dataA()))
            continue;
        break;

        case Sysex:
          if(!(relevant & SysexRelevant) || dest_part->type() == Pos::FRAMES)
            continue;
        break;

        case Meta:
          if(!(relevant & MetaRelevant) || dest_part->type() == Pos::FRAMES)
            continue;
        break;
        
        case Wave:
          if(!(relevant & WaveRelevant) || dest_part->type() == Pos::TICKS)
            continue;
        break;
      }
      
      Event e = old_e.clone();
      unsigned tick = e.posValue(time_type) + curr_pos;

      // Be sure to subtract the position of the very first event of interest.
      const unsigned sp_val = eventlist_range.posValue(time_type);
      if(tick >= sp_val)
        tick -= sp_val;
      else
      {
        printf("WARNING: paste_items_at(): Should not happen: event pos value: %u less than start pos value: %u\n",
                tick, sp_val);
        tick = 0;
      }

      if (tick < dest_part_pos_value)
      //if (tick.posValue() < 0)
      {
        printf("ERROR: paste_items_at(): trying to add event before current part! ignoring this event\n");
        continue;
      }
      tick -= dest_part_pos_value;

      e.setPosValue(tick, time_type);
      e.setSelected(true);  // No need to select clones, AddEvent operation below will take care of that.
      
      // Don't bother with expansion if these are new parts.
      if (!create_new_part && e.endPosValue() > dest_part_len_value) // event exceeds part?
      {
        if (dest_part->hasHiddenEvents() & Part::RightEventsHidden) // auto-expanding is forbidden?
        {
          if (e.posValue(time_type) < dest_part_len_value)
            e.setLenValue(dest_part_len_value - e.posValue(time_type), time_type); // clip
          else
            e.setLenValue(0, time_type); // don't insert that note at all
        }
        else
        {
          if (e.endPosValue() > expand_map[dest_part])
            expand_map[dest_part]=e.endPosValue();
        }
      }
      
      switch(e.type())
      {
        case Note:
          // Don't add Note event types if they have no length.
          // Notes are allowed to overlap. There is no DeleteEvent or ModifyEvent first.
          //if(e.lenTick() != 0)
          //{
            // If this is a fresh new part, to avoid double operation warnings when undoing
            //  just add the event directly to the part instead of an operation.
            if(create_new_part)
              ((Part*)dest_part)->addEvent(e);
            else
              add_operations.push_back(UndoOp(UndoOp::AddEvent, e, dest_part, false, false));
          //}
        break;
        
        case Wave:
          // Don't add Wave event types if they have no length.
          //if(e.lenFrame() != 0)
          //{
            // If this is a fresh new part, to avoid double operation warnings when undoing
            //  just add the event directly to the part instead of an operation.
            if(create_new_part)
            {
              ((Part*)dest_part)->addEvent(e);
            }
            else
            {
              EventList s_el;
              // Compare time, and wave position, path, and start position.
              dest_part->events().findSimilarType(e, s_el, true, false, false, false,
                                                  true, true, true);
              // Do NOT add the new wave event if it already exists at the position.
              // Don't event bother replacing it using DeletEvent or ModifyEvent.
              if(s_el.empty())
              {
                add_operations.push_back(UndoOp(UndoOp::AddEvent, e, dest_part, false, false));
              }
              else
              {
                // Delete all but one of them. There shouldn't be more than one wave event
                //  at a time for a given wave event anyway.
                ciEvent nie;
                for(ciEvent ie = s_el.cbegin(); ie != s_el.cend(); ++ie)
                {
                  // Break on the second-last one, to leave one item intact.
                  nie = ie;
                  ++nie;
                  if(nie == s_el.end())
                  {
                    break;
                  }
                  
                  // If we are 'cutting' the source events, and the source and destination parts
                  //  are the same, and the cut and erase events are the same, don't push the
                  //  deletes here. Let the cutting section at the end of the routine take do it.
                  if(!erase_source || source_part != dest_part || el.findId(ie->second) == el.end())
                    operations.push_back(UndoOp(UndoOp::DeleteEvent, ie->second, dest_part, false, false));
                }
              }
            }
          //}
        break;
        
        case Controller:
        {
          // HACK Grab the event length since we use it for indicating
          //       the visual width when tagging controller items.
          const unsigned int len_val = e.lenValue();
          // Be sure to reset this always since we use it for the above hack.
          e.setLenValue(0);
          
          // If this is a fresh new part, to avoid double DeleteMidiCtrlVal warnings when undoing
          //  just add the event directly to the part instead of an operation.
          if(create_new_part)
          {
            ((Part*)dest_part)->addEvent(e);
          }
          else
          {
            // If we are erasing existing controller values first, this block will
            //  take care of all of the erasures. But even if we are NOT specifically
            //  erasing first, we still MUST erase any existing controller values found
            //  at that exact time value. So that is done in the next block.
            if(erase_controllers)
            {
              ctl_map.add(e.dataA(), e.posValue(), len_val);
            }
            else
            // Here we are not specifically erasing first. But we still MUST erase any
            //  existing controller values found at that exact time value.
            {
              EventList s_el;
              // Compare time and controller number (data A) only.
              dest_part->events().findSimilarType(e, s_el, true, true);
              // Delete them all. There shouldn't be more than one controller event
              //  at a time for a given controller number anyway.
              for(ciEvent ie = s_el.cbegin(); ie != s_el.cend(); ++ie)
              {
                // If we are 'cutting' the source events, and the source and destination parts
                //  are the same, and the cut and erase events are the same, don't push the
                //  deletes here. Let the cutting section at the end of the routine take do it.
                if(!erase_source || source_part != dest_part || el.findId(ie->second) == el.end())
                  // Do port controller values and clone parts. 
                  operations.push_back(UndoOp(UndoOp::DeleteEvent, ie->second, dest_part, true, true));
              }
            }
            // Do port controller values and clone parts. 
            add_operations.push_back(UndoOp(UndoOp::AddEvent, e, dest_part, true, true));
          }
        }
        break;
        
        case Sysex:
        {
          // If this is a fresh new part, to avoid double operation warnings when undoing
          //  just add the event directly to the part instead of an operation.
          if(create_new_part)
          {
            ((Part*)dest_part)->addEvent(e);
          }
          else
          {
            EventList s_el;
            // Compare time and sysex data only.
            dest_part->events().findSimilarType(e, s_el, true);
            // Do NOT add the new sysex if it already exists at the position.
            // Don't event bother replacing it using DeletEvent or ModifyEvent.
            if(s_el.empty())
            {
              add_operations.push_back(UndoOp(UndoOp::AddEvent, e, dest_part, false, false));
            }
            else
            {
              // Delete all but one of them. There shouldn't be more than one sysex event
              //  at a time for a given sysex anyway.
              ciEvent nie;
              for(ciEvent ie = s_el.cbegin(); ie != s_el.cend(); ++ie)
              {
                // Break on the second-last one, to leave one item intact.
                nie = ie;
                ++nie;
                if(nie == s_el.end())
                {
                  break;
                }
                
                // If we are 'cutting' the source events, and the source and destination parts
                //  are the same, and the cut and erase events are the same, don't push the
                //  deletes here. Let the cutting section at the end of the routine take do it.
                if(!erase_source || source_part != dest_part || el.findId(ie->second) == el.end())
                  operations.push_back(UndoOp(UndoOp::DeleteEvent, ie->second, dest_part, false, false));
              }
            }
          }
        }
        break;
        
        case Meta:
        {
          // If this is a fresh new part, to avoid double operation warnings when undoing
          //  just add the event directly to the part instead of an operation.
          if(create_new_part)
          {
            ((Part*)dest_part)->addEvent(e);
          }
          else
          {
            EventList s_el;
            // Compare time and meta data only.
            dest_part->events().findSimilarType(e, s_el, true);
            // Do NOT add the new meta if it already exists at the position.
            // Don't event bother replacing it using DeletEvent or ModifyEvent.
            if(s_el.empty())
            {
              add_operations.push_back(UndoOp(UndoOp::AddEvent, e, dest_part, false, false));
            }
            else
            {
              // Delete all but one of them. There shouldn't be more than one meta event
              //  at a time for a given meta anyway.
              ciEvent nie;
              for(ciEvent ie = s_el.cbegin(); ie != s_el.cend(); ++ie)
              {
                // Break on the second-last one, to leave one item intact.
                nie = ie;
                ++nie;
                if(nie == s_el.end())
                {
                  break;
                }
                
                // If we are 'cutting' the source events, and the source and destination parts
                //  are the same, and the cut and erase events are the same, don't push the
                //  deletes here. Let the cutting section at the end of the routine take do it.
                if(!erase_source || source_part != dest_part || el.findId(ie->second) == el.end())
                  operations.push_back(UndoOp(UndoOp::DeleteEvent, ie->second, dest_part, false, false));
              }
            }
          }
        }
        break;
      }
    }
    
    // If this is not a fresh new part, gather the operations to
    //  erase existing controller events in the destination part.
    if(erase_controllers && !create_new_part && dest_part && !ctl_map.empty())
    {
      // Tidy up the very last items in the list.
      ctl_map.tidy();
      
      unsigned e_pos;
      const EventList& er_el = dest_part->events();
      for(ciEvent ie = er_el.cbegin(); ie != er_el.cend(); ++ie)
      {
        const Event& er_e = ie->second;
        if(er_e.type() != Controller)
          continue;
        
        ciPasteEraseCtlMap icm = ctl_map.find(er_e.dataA());
        if(icm == ctl_map.end())
          continue;
        
        const PasteEraseMap_t& tmap = icm->second;
        e_pos = er_e.posValue();
        ciPasteEraseMap itm = tmap.upper_bound(e_pos);
        if(itm == tmap.begin())
          continue;
        
        --itm;
        if(e_pos >= itm->first && e_pos < itm->second)
        {
          // If we are 'cutting' the source events, and the source and destination parts
          //  are the same, and the cut and erase events are the same, don't push the
          //  deletes here. Let the cutting section at the end of the routine take do it.
          if(!erase_source || source_part != dest_part || el.findId(er_e) == el.end())
            operations.push_back(UndoOp(UndoOp::DeleteEvent, er_e, dest_part, true, true));
        }
      }
    }
  }

  // Do we want to cut the items as well?
  if(erase_source && source_part)
  {
    for(ciEvent i = el.cbegin(); i != el.cend(); ++i)
    {
      const Event& old_e = i->second;
      operations.push_back(UndoOp(UndoOp::DeleteEvent, old_e, source_part, true, true));
    }
  }
}

void paste_items_at(const std::set<const Part*>& parts, const QString& pt, const Pos& pos, int max_distance,
                    const FunctionOptionsStruct& options,
                    const Part* paste_into_part, int amount, int raster,
                    RelevantSelectedEvents_t relevant,
                    int paste_to_ctrl_num)
{
  // To maximize speed and minimize memory use, the processing below 
  //  can only find any delete operations AFTER it has gathered
  //  add operations. So we keep two separate operations lists and
  //  combine them later so that all the deletes come BEFORE all the adds.
  Undo add_operations, operations;
  
  map<const Part*, unsigned> expand_map;
  map<const Part*, set<const Part*> > new_part_map;

  QByteArray pt_= pt.toUtf8();
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
          QUuid part_id;
          PosLen poslen;
					int numevents = 0;
          const Part* dest_part = paste_into_part;

          if (!read_eventlist_and_part(xml, &el, &part_id, &poslen, &numevents, &dest_part, relevant, paste_to_ctrl_num))
          {
            printf("ERROR: reading eventlist from clipboard failed. ignoring this one...\n");
            break;
          }
          
          if (dest_part == nullptr)
          {
            printf("ERROR: destination part wasn't found. ignoring these events\n");
            break;
          }
          
          // Paste into the destination part ONLY if it is included in the given set of parts,
          //  typically the parts used by an editor window instance's canvas. (WYSIWYG).
          // Override if paste_into_part is given, to allow 'Paste to current part' to work.
          if(!paste_into_part && parts.find(dest_part) == parts.end())
            break;
          
          const bool wave_mode = dest_part->partType() == Part::WavePartType;
        
          FindMidiCtlsList_t ctrlList;
          el.findControllers(wave_mode, &ctrlList);
          int ctrlsFound = 0;
          if(!ctrlList.empty())
            ctrlsFound = ctrlList.size();
          if(paste_to_ctrl_num >= 0 && ctrlsFound > 0)
          {
            // TODO Dialog for choosing which controller to paste.
          }

          pasteEventList(
            el, pos, ((Part*)dest_part), operations, add_operations,
            expand_map, new_part_map, nullptr, false,
            poslen, numevents, max_distance, options, amount, raster, relevant, paste_to_ctrl_num);
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
  
  // Push any part resizing operations onto the operations list now, before merging
  //  the add operations.
  for (map<const Part*, unsigned>::iterator it = expand_map.begin(); it!=expand_map.end(); it++)
    if (it->second != it->first->lenValue())
      schedule_resize_all_same_len_clone_parts(it->first, it->second, operations);

  // Now merge the add operations into the operations so that all of the 'deletes' come first.
  //add_operations.splice(add_operations.begin(), delete_operations);
  for(ciUndoOp iuo = add_operations.begin(); iuo != add_operations.end(); ++iuo)
    operations.push_back(*iuo);
  
  MusEGlobal::song->informAboutNewParts(new_part_map); // must be called before apply. otherwise
                                                      // pointer changes (by resize) screw it up
  
  MusEGlobal::song->applyOperationGroup(operations);
  MusEGlobal::song->update(SC_SELECTION | SC_PART_SELECTION);
}

void paste_items_at(
  const std::set<const Part*>& parts,
  const TagEventList* tag_list,
  const Pos& pos,
  int max_distance,
  const FunctionOptionsStruct& options,
  const Part* paste_into_part,
  int amount,
  int raster,
  RelevantSelectedEvents_t relevant,
  int paste_to_ctrl_num
  )
  {
    const bool cut_mode                    = options._flags & FunctionCutItems;

    // To maximize speed and minimize memory use, the processing below 
    //  can only find any delete operations AFTER it has gathered
    //  add operations. So we keep two separate operations lists and
    //  combine them later so that all the deletes come BEFORE all the adds.
    Undo add_operations, operations;
    
    expand_map_t expand_map;
    new_part_map_t new_part_map;

    // At this point the tag list's event list will still have any controller
    //  visual lengths HACK applied.
    // Those lengths will be reset below. But for now we could use them...
    FindMidiCtlsList_t globalCtrlList;
    int globalCtrlsFound = 0;
    if(!globalCtrlList.empty())
      globalCtrlsFound = globalCtrlList.size();
    if(paste_to_ctrl_num >= 0)
    {
      tag_list->globalCtlStats(&globalCtrlList, paste_to_ctrl_num);
      if(!globalCtrlList.empty())
        globalCtrlsFound = globalCtrlList.size();
      if(globalCtrlsFound > 0)
      {
        // Prompt user to choose controller...
        
      }
    }
    
    for(ciTagEventList itl = tag_list->cbegin(); itl != tag_list->cend(); ++itl)
    {
      const Part* dest_part = nullptr;
      const Part* src_part = itl->part();

      if (paste_into_part == nullptr)
        // Paste to original source part.
        dest_part = src_part;
      else
        // Paste to specific part.
        dest_part=paste_into_part;

      if (dest_part == nullptr)
      {
        printf("paste_items_at(): ERROR: destination part wasn't found. ignoring these events\n");
        continue;
      }
      
      // Paste into the destination part ONLY if it is included in the given set of parts,
      //  typically the parts used by an editor window instance's canvas. (WYSIWYG).
      // Override if paste_into_part is given, to allow 'Paste to current part' to work.
      if(!paste_into_part && parts.find(dest_part) == parts.end())
        continue;
        
      // Grab the event list and find the number of relevant events.
      const EventList& el = itl->evlist();

      const bool wave_mode = dest_part->partType() == Part::WavePartType;
      int num_events = 0;
      const PosLen el_range = el.evrange(wave_mode, relevant, &num_events, paste_to_ctrl_num);

      pasteEventList(
        el, pos, ((Part*)dest_part), operations, add_operations,
        expand_map, new_part_map, src_part, cut_mode, el_range, num_events, max_distance, options,
        amount, raster, relevant, paste_to_ctrl_num);
    }

    // Push any part resizing operations onto the operations list now, before merging
    //  the add operations.
    for (map<const Part*, unsigned>::iterator it = expand_map.begin(); it!=expand_map.end(); it++)
      if (it->second != it->first->lenValue())
        schedule_resize_all_same_len_clone_parts(it->first, it->second, operations);

    // Now merge the add operations into the operations so that all of the 'deletes' come first.
    //add_operations.splice(add_operations.begin(), delete_operations);
    for(ciUndoOp iuo = add_operations.begin(); iuo != add_operations.end(); ++iuo)
      operations.push_back(*iuo);
    
    MusEGlobal::song->informAboutNewParts(new_part_map); // must be called before apply. otherwise
                                                        // pointer changes (by resize) screw it up
    
    MusEGlobal::song->applyOperationGroup(operations);
    MusEGlobal::song->update(SC_SELECTION | SC_PART_SELECTION);
}

//---------------------------------------------------------
//   getSelectedParts
//---------------------------------------------------------
PartList* getSelectedParts()
{
  PartList* parts1;
  PartList* parts2;

  parts1 = getSelectedMidiParts();
  parts2 = getSelectedWaveParts();

  for (ciPart p = parts2->begin(); p != parts2->end(); ++p) {
    parts1->add(p->second);
  }

  return parts1;
}

PartList* getSelectedMidiParts()
      {
      PartList* parts = new PartList();

      /*
            If a part is selected, edit that.
            If a track is selected, edit the first
             part of the track, the rest are
             'ghost parts'
            When multiple parts are selected, then edit the first,
              the rest are 'ghost parts'
      */


       // collect marked parts
      for (ciMidiTrack t = MusEGlobal::song->midis()->begin(); t != MusEGlobal::song->midis()->end(); ++t) {
            PartList* pl = (*t)->parts();
            for (iPart p = pl->begin(); p != pl->end(); ++p) {
                  if (p->second->selected()) {
                        parts->add(p->second);
                        }
                  }
            }
      // if no part is selected, then search for selected track
      // and collect all parts of this track

      if (parts->empty()) {
            for (ciMidiTrack t = MusEGlobal::song->midis()->begin(); t != MusEGlobal::song->midis()->end(); ++t) {
                  if ((*t)->selected()) {
                        PartList* pl = (*t)->parts();
                        for (iPart p = pl->begin(); p != pl->end(); ++p)
                              parts->add(p->second);
                        break;
                        }
                  }
            }

      return parts;
      }

PartList* getSelectedWaveParts()
      {
      PartList* parts = new PartList();

      /*
            If a part is selected, edit that.
            If a track is selected, edit the first
             part of the track, the rest are
             'ghost parts'
            When multiple parts are selected, then edit the first,
              the rest are 'ghost parts'
      */

      // collect selected parts
      for (ciWaveTrack t = MusEGlobal::song->waves()->begin(); t != MusEGlobal::song->waves()->end(); ++t) {
            PartList* pl = (*t)->parts();
            for (ciPart p = pl->begin(); p != pl->end(); ++p) {
                  if (p->second->selected()) {
                        parts->add(p->second);
                        }
                  }
            }
      // if no parts are selected, then search the selected track
      // and collect all parts in this track

      if (parts->empty()) {
            for (ciWaveTrack t = MusEGlobal::song->waves()->begin(); t != MusEGlobal::song->waves()->end(); ++t) {
                  if ((*t)->selected()) {
                        PartList* pl = (*t)->parts();
                        for (ciPart p = pl->begin(); p != pl->end(); ++p)
                              parts->add(p->second);
                        break;
                        }
                  }
            }
      return parts;
}

// REMOVE Tim. wave. Removed. (From upstream 20230615).
// //---------------------------------------------------------
// //   resize_part
// //---------------------------------------------------------
//
// void resize_part(
//   Track* track, Part* originalPart, unsigned int newTickPosOrLen, MusECore::ResizeDirection resizeDirection,
//   bool doClones, bool dragEvents)
//       {
//
//       // Are the events to be offset?
//       const bool use_events_offset =
//         (resizeDirection == MusECore::ResizeDirection::RESIZE_TO_THE_RIGHT && dragEvents) ||
//         (resizeDirection == MusECore::ResizeDirection::RESIZE_TO_THE_LEFT && !dragEvents);
//
//       // Under this condition we MUST do all clones. The RULE is that event times, which are relative to the part start,
//       //   must be the same in all clones.
//       if(use_events_offset)
//         doClones = true;
//
//       switch(track->type()) {
//             case Track::WAVE:
//             case Track::MIDI:
//             case Track::DRUM:
//                   {
//                   Undo operations;
//
// #ifdef ALLOW_LEFT_HIDDEN_EVENTS
//                   const Pos::TType newPosOrLenType = Pos::TType::TICKS;
//                   const unsigned int origPosValue = originalPart->posValue();
//                   const unsigned int newOrigPosValue = Pos::convert(newTickPosOrLen, newPosOrLenType, originalPart->type());
//                   // The amount to shift a part. The int64_t cast ensures we preserve the unsigned range.
//                   const int64_t origPosValueDiff = (int64_t)newOrigPosValue - (int64_t)origPosValue;
//                   const unsigned int origPosValueConverted = originalPart->posValue(newPosOrLenType);
//                   const unsigned int newOrigEndPosValue =
//                     Pos::convert(origPosValueConverted + newTickPosOrLen, newPosOrLenType, originalPart->type());
//                   const unsigned int newOrigLenValue = newOrigEndPosValue - origPosValue;
//
//                   const unsigned int origLenValue = originalPart->lenValue();
//                   const int64_t origLenValueDiff = (int64_t)newOrigLenValue - (int64_t)origLenValue;
//
//                   int64_t events_offset = 0L;
//                   if(use_events_offset)
//                   {
//                     switch(resizeDirection)
//                     {
//                       case MusECore::ResizeDirection::RESIZE_TO_THE_RIGHT:
//                         events_offset = origLenValueDiff;
//                       break;
//                       case MusECore::ResizeDirection::RESIZE_TO_THE_LEFT:
//                         events_offset = -origPosValueDiff;
//                       break;
//                     }
//                   }
//
//                   auto currentPart = originalPart;
//
//                   do
//                   {
//                       if(resizeDirection == MusECore::ResizeDirection::RESIZE_TO_THE_RIGHT)
//                       {
//                         const unsigned int pos_val = currentPart->posValue(originalPart->type());
//                         const unsigned int new_end_pos_val = Pos::convert(pos_val + newOrigLenValue, originalPart->type(), currentPart->type());
//                         const unsigned int new_len_val = new_end_pos_val - pos_val;
//                         operations.push_back(
//                           UndoOp(UndoOp::ModifyPartLength, currentPart,
//                                 currentPart->lenValue(),
//                                 new_len_val,
//                                 // The amount to shift all events in the part.
//                                 events_offset,
//                                 // The position type of the amount to shift all events in the part.
//                                 originalPart->type()));
//                       }
//                       else if(resizeDirection == MusECore::ResizeDirection::RESIZE_TO_THE_LEFT)
//                       {
//                           const unsigned int pos_val = currentPart->posValue(originalPart->type());
//                           const unsigned int end_pos_val = currentPart->endValue(originalPart->type());
//                           unsigned int new_pos_val, new_len_val;
//                           if((int64_t)pos_val + origPosValueDiff < 0L)
//                           {
//                             new_pos_val = 0;
//                             new_len_val = Pos::convert((int64_t)end_pos_val - ((int64_t)pos_val + origPosValueDiff),
//                                                       originalPart->type(), currentPart->type()) - new_pos_val;
//                           }
//                           else
//                           {
//                             new_pos_val = Pos::convert((int64_t)pos_val + origPosValueDiff, originalPart->type(), currentPart->type());
//                             new_len_val = currentPart->endValue() - new_pos_val;
//                           }
//                           operations.push_back(
//                             UndoOp(UndoOp::ModifyPartStart, currentPart,
//                                     currentPart->posValue(),
//                                     new_pos_val,
//                                     currentPart->lenValue(),
//                                     new_len_val,
//                                     // The amount to shift all events in the part.
//                                     events_offset,
//                                     // The position type of the amount to shift all events in the part.
//                                     originalPart->type()));
//                       }
//
//                       currentPart = currentPart->nextClone();
//
//                   } while (doClones && (currentPart != originalPart));
// #else
//                   const Pos::TType newPosOrLenType = Pos::TType::TICKS;
//                   const unsigned int origPosValue = originalPart->posValue();
//                   const unsigned int newOrigPosValue = Pos::convert(newTickPosOrLen, newPosOrLenType, originalPart->type());
//                   // The amount to shift a part. The int64_t cast ensures we preserve the unsigned range.
//                   const int64_t origPosValueDiff = (int64_t)newOrigPosValue - (int64_t)origPosValue;
//                   const Pos::TType events_offset_time_type = originalPart->type();
//                   const unsigned int origPosValueConverted = originalPart->posValue(newPosOrLenType);
//                   const unsigned int newOrigEndPosValue = Pos::convert(origPosValueConverted + newTickPosOrLen, newPosOrLenType, originalPart->type());
//                   const unsigned int newOrigLenValue = newOrigEndPosValue - origPosValue;
//
//                   const unsigned int origLenValue = originalPart->lenValue();
//                   const int64_t origLenValueDiff = (int64_t)newOrigLenValue - (int64_t)origLenValue;
//
//                   int64_t events_offset = 0L;
//                   if(use_events_offset)
//                   {
//                     switch(resizeDirection)
//                     {
//                       case MusECore::ResizeDirection::RESIZE_TO_THE_RIGHT:
//                         events_offset = origLenValueDiff;
//                       break;
//                       case MusECore::ResizeDirection::RESIZE_TO_THE_LEFT:
//                         events_offset = -origPosValueDiff;
//                       break;
//                     }
//                   }
//
//                   // Check to see if the events offset would move events before time zero, and limit if so.
//                   if(use_events_offset)
//                   {
//                     const EventList& el = originalPart->events();
//                     const ciEvent iev = el.cbegin();
//                     if(iev != el.cend())
//                     {
//                       const Event first_ev = iev->second;
//
//                       // In case the event and part pos types differ, the event dominates.
//                       unsigned int new_part_pos_val;
//                       switch(resizeDirection)
//                       {
//                         // If resizing to the right, just use the original part position, converted.
//                         case MusECore::ResizeDirection::RESIZE_TO_THE_RIGHT:
//                           new_part_pos_val = originalPart->posValue(first_ev.pos().type());
//                         break;
//                         // If resizing to the left, use the new part position, converted.
//                         case MusECore::ResizeDirection::RESIZE_TO_THE_LEFT:
//                           new_part_pos_val = Pos::convert(newTickPosOrLen, newPosOrLenType, first_ev.pos().type());
//                         break;
//                       }
//
//                       const unsigned int old_abs_ev_pos_val =
//                         Pos::convert(first_ev.posValue() + new_part_pos_val, first_ev.pos().type(), events_offset_time_type);
//
//                       if((int64_t)old_abs_ev_pos_val + events_offset < 0L)
//                         events_offset = -(int64_t)old_abs_ev_pos_val;
//
//                       const unsigned int new_abs_ev_pos_val =
//                         Pos::convert((int64_t)old_abs_ev_pos_val + events_offset, events_offset_time_type, first_ev.pos().type());
//
//                       if(new_abs_ev_pos_val < new_part_pos_val)
//                         events_offset = -(int64_t)first_ev.pos().posValue();
//                     }
//                   }
//
//                   auto currentPart = originalPart;
//
//                   do
//                   {
//                       if(resizeDirection == MusECore::ResizeDirection::RESIZE_TO_THE_RIGHT)
//                       {
//                         const unsigned int pos_val = currentPart->posValue(originalPart->type());
//                         const unsigned int new_end_pos_val = Pos::convert(pos_val + newOrigLenValue, originalPart->type(), currentPart->type());
//                         const unsigned int new_len_val = new_end_pos_val - pos_val;
//                         operations.push_back(
//                           UndoOp(UndoOp::ModifyPartLength, currentPart,
//                                 currentPart->lenValue(),
//                                 new_len_val,
//                                 // The amount to shift all events in the part.
//                                 events_offset,
//                                 // The position type of the amount to shift all events in the part.
//                                 originalPart->type()));
//                       }
//                       else if(resizeDirection == MusECore::ResizeDirection::RESIZE_TO_THE_LEFT)
//                       {
//                           const unsigned int pos_val = currentPart->posValue(originalPart->type());
//                           const unsigned int end_pos_val = currentPart->endValue(originalPart->type());
//                           unsigned int new_pos_val, new_len_val;
//                           if((int64_t)pos_val + origPosValueDiff < 0L)
//                           {
//                             new_pos_val = 0;
//                             new_len_val = Pos::convert((int64_t)end_pos_val - ((int64_t)pos_val + origPosValueDiff),
//                                                       originalPart->type(), currentPart->type()) - new_pos_val;
//                           }
//                           else
//                           {
//                             new_pos_val = Pos::convert((int64_t)pos_val + origPosValueDiff, originalPart->type(), currentPart->type());
//                             new_len_val = currentPart->endValue() - new_pos_val;
//                           }
//                           operations.push_back(
//                             UndoOp(UndoOp::ModifyPartStart, currentPart,
//                                     currentPart->posValue(),
//                                     new_pos_val,
//                                     currentPart->lenValue(),
//                                     new_len_val,
//                                     // The amount to shift all events in the part.
//                                     events_offset,
//                                     // The position type of the amount to shift all events in the part.
//                                     originalPart->type()));
//                       }
//
//                       currentPart = currentPart->nextClone();
//
//                   } while (doClones && (currentPart != originalPart));
// #endif
//
//                   MusEGlobal::song->applyOperationGroup(operations);
//                   break;
//                 }
//
//             default:
//                   break;
//             }
//       }

// REMOVE Tim. wave. Removed. (Original from 20200908).
// //---------------------------------------------------------
// //   resize_part
// //---------------------------------------------------------
//
// void resize_part(
//   Track* track, Part* originalPart, unsigned int newTickPosOrLen, MusECore::ResizeDirection resizeDirection,
//   bool doClones, bool dragEvents, bool autoExpandWaves)
//       {
//       if(resizeDirection != MusECore::ResizeDirection::RESIZE_TO_THE_RIGHT &&
//         resizeDirection != MusECore::ResizeDirection::RESIZE_TO_THE_LEFT)
//         return;
//
//       const MusECore::Part::PartType originalPartType = originalPart->partType();
//
//       // Are the events to be offset?
//       const bool useEventsOffset =
//         (resizeDirection == MusECore::ResizeDirection::RESIZE_TO_THE_RIGHT && dragEvents) ||
//         (resizeDirection == MusECore::ResizeDirection::RESIZE_TO_THE_LEFT && !dragEvents);
//
//       // Under this condition we MUST do all clones. The RULE is that event times, which are relative to the part start,
//       //   must be the same in all clones.
//       if(useEventsOffset || (autoExpandWaves && originalPartType == MusECore::Part::WavePartType))
//         doClones = true;
//
//       switch(track->type()) {
//             case Track::WAVE:
//             case Track::MIDI:
//             case Track::DRUM:
//                   {
//                   Undo operations;
//
// #ifdef ALLOW_LEFT_HIDDEN_EVENTS
//                   const Pos::TType newPosOrLenType = Pos::TType::TICKS;
//                   const MuseCount_t origPosValue = MUSE_TIME_UINT_TO_INT64 originalPart->posValue();
//                   const MuseCount_t newOrigPosValue = MUSE_TIME_UINT_TO_INT64 Pos::convert(
//                     newTickPosOrLen, newPosOrLenType, originalPart->type());
//                   // The amount to shift a part. The MuseCount_t cast ensures we preserve the unsigned range.
//                   const MuseCount_t origPosValueDiff = newOrigPosValue - origPosValue;
//                   const MuseCount_t origPosValueConverted = MUSE_TIME_UINT_TO_INT64 originalPart->posValue(newPosOrLenType);
//                   const MuseCount_t newOrigEndPosValue = MUSE_TIME_UINT_TO_INT64 Pos::convert(
//                     origPosValueConverted + newTickPosOrLen, newPosOrLenType, originalPart->type());
//                   const MuseCount_t newOrigLenValue = newOrigEndPosValue - origPosValue;
//
//                   const MuseCount_t origLenValue = MUSE_TIME_UINT_TO_INT64 originalPart->lenValue();
//                   const MuseCount_t origLenValueDiff = newOrigLenValue - origLenValue;
//
//                   MuseCount_t eventsOffset = 0L;
//                   if(useEventsOffset)
//                   {
//                     switch(resizeDirection)
//                     {
//                       case MusECore::ResizeDirection::RESIZE_TO_THE_RIGHT:
//                         eventsOffset = origLenValueDiff;
//                       break;
//                       case MusECore::ResizeDirection::RESIZE_TO_THE_LEFT:
//                         eventsOffset = -origPosValueDiff;
//                       break;
//                     }
//                   }
//
//                   auto currentPart = originalPart;
//
//                   do
//                   {
//                       if(resizeDirection == MusECore::ResizeDirection::RESIZE_TO_THE_RIGHT ||
//                         resizeDirection == MusECore::ResizeDirection::RESIZE_TO_THE_LEFT)
//                       {
//                         MuseCount_t finEventsOffset = eventsOffset;
//                         const MuseCount_t posValOrigType = MUSE_TIME_UINT_TO_INT64 currentPart->posValue(originalPart->type());
//                         const MuseCount_t endValOrigType = MUSE_TIME_UINT_TO_INT64 currentPart->endValue(originalPart->type());
//                         MuseCount_t newPosVal, newEndVal, newLenVal;
//
//                         if(resizeDirection == MusECore::ResizeDirection::RESIZE_TO_THE_RIGHT)
//                         {
//                           newPosVal = MUSE_TIME_UINT_TO_INT64 currentPart->posValue();
//                           newEndVal =
//                             MUSE_TIME_UINT_TO_INT64 Pos::convert(posValOrigType + newOrigLenValue, originalPart->type(), currentPart->type());
//                           newLenVal = newEndVal - newPosVal;
//                         }
//                         else if(resizeDirection == MusECore::ResizeDirection::RESIZE_TO_THE_LEFT)
//                         {
//                             if(posValOrigType + origPosValueDiff < 0L)
//                             {
//                               newPosVal = 0;
//                               newLenVal = MUSE_TIME_UINT_TO_INT64 Pos::convert(endValOrigType - (posValOrigType + origPosValueDiff),
//                                                         originalPart->type(), currentPart->type());
//                               newEndVal = newLenVal;
//                             }
//                             else
//                             {
//                               newPosVal =
//                                 MUSE_TIME_UINT_TO_INT64 Pos::convert(posValOrigType + origPosValueDiff, originalPart->type(), currentPart->type());
//                               newEndVal = currentPart->endValue();
//                               newLenVal = newEndVal - newPosVal;
//                             }
//                         }
//
//                           switch(currentPart->partType())
//                           {
//                             // NOTE: Special for wave events: It is time-costly to clone a wave event,
//                             //        and therefore costly to compose a swappable replacement event list.
//                             //       There is noticeable delay when doing operations by cloning them.
//                             //       So use this non-cloning technique which uses C++17 in-place key modifications.
//                             //       But if we ever add controller events or other frequent events to wave parts
//                             //        (likely, desirable) where there might be millions of events (performances)
//                             //        to shift, then this technique might bog down the realtime thread a bit.
//                             case Part::WavePartType:
//                             {
//                               const EventList& el = currentPart->events();
//                               const int sz = el.size();
//
//                               for(ciEvent ie = el.cbegin(); ie != el.cend(); ++ie)
//                               {
//                                 const Event& e = ie->second;
//
//                                 // Only for a single wave event.
//                                 //if(sz == 1 && e.type() == EventType::Wave)
//                                 {
//                                   // Clear this. We won't be using it.
//                                   finEventsOffset = 0;
//
//                                   // In this section we modify the event using the ModifyEventProperties operation.
//                                   // But ONLY for the given part not any of its clone parts, because the
//                                   //  ModifyEventProperties operation automatically processes all the clone parts for us.
//                                   if(currentPart == originalPart)
//                                   {
//                                     MuseCount_t newEPosVal, newELenVal;
//                                     int new_espos = e.spos();
//
//                                     const MuseCount_t currPosValEtype = MUSE_TIME_UINT_TO_INT64 currentPart->posValue(e.pos().type());
//                                     const MuseCount_t newPosValEtype =
//                                       MUSE_TIME_UINT_TO_INT64 Pos::convert(newPosVal, currentPart->type(), e.pos().type());
//
//                                     const MuseCount_t currEndPosEtype =
//                                       MUSE_TIME_UINT_TO_INT64 currentPart->endValue(e.pos().type());
//                                     const MuseCount_t newEndPosEtype =
//                                       MUSE_TIME_UINT_TO_INT64 Pos::convert(newPosVal + newLenVal, currentPart->type(), e.pos().type());
//
//                                     const MuseCount_t currAbsEPosVal =
//                                       currPosValEtype + MUSE_TIME_UINT_TO_INT64 e.posValue();
//                                     const MuseCount_t currAbsEPosFrames =
//                                       MUSE_TIME_UINT_TO_INT64 Pos::convert(currAbsEPosVal, e.pos().type(), Pos::FRAMES);
//
//                                     const MuseCount_t wavSamplesConv = e.sndFile().samplesConverted();
//                                     const MuseCount_t minAbsEPosFrames = currAbsEPosFrames - e.spos();
//                                     const MuseCount_t maxAbsEEndFrames = minAbsEPosFrames + wavSamplesConv;
//
//                                     MuseCount_t minAbsEPosEtype =
//                                       MUSE_TIME_UINT_TO_INT64 Pos::convert(minAbsEPosFrames, Pos::FRAMES, e.pos().type());
//                                     MuseCount_t maxAbsEEndEType =
//                                       MUSE_TIME_UINT_TO_INT64 Pos::convert(maxAbsEEndFrames, Pos::FRAMES, e.pos().type());
//
//                                     // Account for movement of the right border caused by resizing the left border.
//                                     if(resizeDirection == MusECore::ResizeDirection::RESIZE_TO_THE_LEFT)
//                                     {
//                                       const MuseCount_t endposDiffEType = newEndPosEtype - currEndPosEtype;
//                                       minAbsEPosEtype += endposDiffEType;
//                                       maxAbsEEndEType += endposDiffEType;
//                                     };
//
//                                     // Account for dragging events with the borders.
//                                     if(dragEvents)
//                                     {
//                                       const MuseCount_t dragOffsetEType = (resizeDirection == MusECore::ResizeDirection::RESIZE_TO_THE_LEFT ?
//                                         newPosValEtype - currPosValEtype : newEndPosEtype - currEndPosEtype);
//                                       minAbsEPosEtype += dragOffsetEType;
//                                       maxAbsEEndEType += dragOffsetEType;
//                                     }
//
//                                     // Can any of or all of the event be expanded into the view of the part?
//                                     // If not, leave the event alone and move it normally without auto expand, below.
//                                     // Only for a single wave event.
//                                     if(autoExpandWaves && sz == 1 && e.type() == EventType::Wave &&
//                                         minAbsEPosEtype < newEndPosEtype &&
//                                         maxAbsEEndEType >= newPosValEtype)
//                                     {
//                                       const MuseCount_t desiredMinAbsEpos = minAbsEPosEtype;
//
//                                       // Limit the new event borders to the new part borders.
//                                       if(minAbsEPosEtype < newPosValEtype)
//                                         minAbsEPosEtype = newPosValEtype;
//                                       if(maxAbsEEndEType > newEndPosEtype)
//                                         maxAbsEEndEType = newEndPosEtype;
//
//                                       // Adjusting the wave's spos offset value in frames...
//                                       // Special consideration for an empty or zero length wave. Reset the spos to zero.
//                                       // TODO Really reset it to zero? An empty wave with an spos offset might be legitimate,
//                                       //       user might load a wave into the event later and want it to be at that spos ?
//                                       //if(wav_samples_conv == 0L) {
//                                       //  new_espos = 0;
//                                       //}
//                                       //else
//                                       {
//                                         const MuseCount_t desiredMinAbsEposFrames =
//                                           MUSE_TIME_UINT_TO_INT64 Pos::convert(desiredMinAbsEpos, e.pos().type(), Pos::FRAMES);
//                                         const MuseCount_t minAbsEposFrames =
//                                           MUSE_TIME_UINT_TO_INT64 Pos::convert(minAbsEPosEtype, e.pos().type(), Pos::FRAMES);
//                                         new_espos = minAbsEposFrames - desiredMinAbsEposFrames;
//
//                                         // Limit the wave's spos offset value to zero and above.
//                                         // Although the spos could be made to work with negative values, we don't
//                                         //  want that right here, only the lower limit of what is available.
//                                         if(new_espos < 0)
//                                           new_espos = 0;
//                                       }
//
//                                       // Final new event position relative to the part.
//                                       newEPosVal = minAbsEPosEtype - newPosValEtype;
//                                       // Final new event length.
//                                       newELenVal = maxAbsEEndEType - minAbsEPosEtype;
//                                     }
//                                     else
//                                     {
//                                       MuseCount_t currAbsEPosOrigType = MUSE_TIME_UINT_TO_INT64 Pos::convert(
//                                             currAbsEPosVal, e.pos().type(), originalPart->type());
//                                       newEPosVal = MUSE_TIME_UINT_TO_INT64 Pos::convert(
//                                                      currAbsEPosOrigType + eventsOffset, originalPart->type(), e.pos().type()) - currPosValEtype;
//                                       newELenVal = MUSE_TIME_UINT_TO_INT64 e.lenValue();
//                                       new_espos = e.spos();
//                                     }
//
//                                     operations.push_back(
//                                       UndoOp(UndoOp::ModifyEventProperties, currentPart,
//                                               e,
//                                               newEPosVal,
//                                               newELenVal,
//                                               new_espos,
//                                               // Let the part operations handle port controller operations.
//                                               false,
//                                               // Let the part operations handle clone port controller operations.
//                                               false));
//                                   }
//                                 }
//                               }
//                             }
//                             break;
//
//                             case Part::MidiPartType:
//                             break;
//                           }
//
//                           operations.push_back(
//                             UndoOp(UndoOp::ModifyPartStart, currentPart,
//                                     currentPart->posValue(),
//                                     newPosVal,
//                                     currentPart->lenValue(),
//                                     newLenVal,
//                                     // The amount to shift all events in the part.
//                                     finEventsOffset,
//                                     // The position type of the amount to shift all events in the part.
//                                     originalPart->type()));
//                       }
//
//                       currentPart = currentPart->nextClone();
//
//                   } while (doClones && (currentPart != originalPart));
// #else
//                   const Pos::TType newPosOrLenType = Pos::TType::TICKS;
//                   const unsigned int origPosValue = originalPart->posValue();
//                   const unsigned int newOrigPosValue = Pos::convert(newTickPosOrLen, newPosOrLenType, originalPart->type());
//                   // The amount to shift a part. The int64_t cast ensures we preserve the unsigned range.
//                   const int64_t origPosValueDiff = (int64_t)newOrigPosValue - (int64_t)origPosValue;
//                   const Pos::TType events_offset_time_type = originalPart->type();
//                   const unsigned int origPosValueConverted = originalPart->posValue(newPosOrLenType);
//                   const unsigned int newOrigEndPosValue = Pos::convert(origPosValueConverted + newTickPosOrLen, newPosOrLenType, originalPart->type());
//                   const unsigned int newOrigLenValue = newOrigEndPosValue - origPosValue;
//
//                   const unsigned int origLenValue = originalPart->lenValue();
//                   const int64_t origLenValueDiff = (int64_t)newOrigLenValue - (int64_t)origLenValue;
//
//                   int64_t events_offset = 0L;
//                   if(use_events_offset)
//                   {
//                     switch(resizeDirection)
//                     {
//                       case MusECore::ResizeDirection::RESIZE_TO_THE_RIGHT:
//                         events_offset = origLenValueDiff;
//                       break;
//                       case MusECore::ResizeDirection::RESIZE_TO_THE_LEFT:
//                         events_offset = -origPosValueDiff;
//                       break;
//                     }
//                   }
//
//                   // Check to see if the events offset would move events before time zero, and limit if so.
//                   if(use_events_offset)
//                   {
//                     const EventList& el = originalPart->events();
//                     const ciEvent iev = el.cbegin();
//                     if(iev != el.cend())
//                     {
//                       const Event first_ev = iev->second;
//
//                       // In case the event and part pos types differ, the event dominates.
//                       unsigned int new_part_pos_val;
//                       switch(resizeDirection)
//                       {
//                         // If resizing to the right, just use the original part position, converted.
//                         case MusECore::ResizeDirection::RESIZE_TO_THE_RIGHT:
//                           new_part_pos_val = originalPart->posValue(first_ev.pos().type());
//                         break;
//                         // If resizing to the left, use the new part position, converted.
//                         case MusECore::ResizeDirection::RESIZE_TO_THE_LEFT:
//                           new_part_pos_val = Pos::convert(newTickPosOrLen, newPosOrLenType, first_ev.pos().type());
//                         break;
//                       }
//
//                       const unsigned int old_abs_ev_pos_val =
//                         Pos::convert(first_ev.posValue() + new_part_pos_val, first_ev.pos().type(), events_offset_time_type);
//
//                       if((int64_t)old_abs_ev_pos_val + events_offset < 0L)
//                         events_offset = -(int64_t)old_abs_ev_pos_val;
//
//                       const unsigned int new_abs_ev_pos_val =
//                         Pos::convert((int64_t)old_abs_ev_pos_val + events_offset, events_offset_time_type, first_ev.pos().type());
//
//                       if(new_abs_ev_pos_val < new_part_pos_val)
//                         events_offset = -(int64_t)first_ev.pos().posValue();
//                     }
//                   }
//
//                   auto currentPart = originalPart;
//
//                   do
//                   {
//                       if(resizeDirection == MusECore::ResizeDirection::RESIZE_TO_THE_RIGHT)
//                       {
//                         const unsigned int pos_val = currentPart->posValue(originalPart->type());
//                         const unsigned int new_end_pos_val = Pos::convert(pos_val + newOrigLenValue, originalPart->type(), currentPart->type());
//                         const unsigned int new_len_val = new_end_pos_val - pos_val;
//                         operations.push_back(
//                           UndoOp(UndoOp::ModifyPartLength, currentPart,
//                                 currentPart->lenValue(),
//                                 new_len_val,
//                                 // The amount to shift all events in the part.
//                                 events_offset,
//                                 // The position type of the amount to shift all events in the part.
//                                 originalPart->type()));
//                       }
//                       else if(resizeDirection == MusECore::ResizeDirection::RESIZE_TO_THE_LEFT)
//                       {
//                           const unsigned int pos_val = currentPart->posValue(originalPart->type());
//                           const unsigned int end_pos_val = currentPart->endValue(originalPart->type());
//                           unsigned int new_pos_val, new_len_val;
//                           if((int64_t)pos_val + origPosValueDiff < 0L)
//                           {
//                             new_pos_val = 0;
//                             new_len_val = Pos::convert((int64_t)end_pos_val - ((int64_t)pos_val + origPosValueDiff),
//                                                       originalPart->type(), currentPart->type()) - new_pos_val;
//                           }
//                           else
//                           {
//                             new_pos_val = Pos::convert((int64_t)pos_val + origPosValueDiff, originalPart->type(), currentPart->type());
//                             new_len_val = currentPart->endValue() - new_pos_val;
//                           }
//                           operations.push_back(
//                             UndoOp(UndoOp::ModifyPartStart, currentPart,
//                                     currentPart->posValue(),
//                                     new_pos_val,
//                                     currentPart->lenValue(),
//                                     new_len_val,
//                                     // The amount to shift all events in the part.
//                                     events_offset,
//                                     // The position type of the amount to shift all events in the part.
//                                     originalPart->type()));
//                       }
//
//                       currentPart = currentPart->nextClone();
//
//                   } while (doClones && (currentPart != originalPart));
// #endif
//
//                   MusEGlobal::song->applyOperationGroup(operations);
//                   break;
//                 }
//
//             default:
//                   break;
//             }
//       }

} // namespace MusECore
