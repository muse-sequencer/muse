//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: functions.cpp,v 1.20.2.19 2011/05/05 20:10 flo93 Exp $
//  (C) Copyright 2011 Florian Jung (flo93@sourceforge.net)
//=========================================================

#include "functions.h"
#include "song.h"
#include "undo.h"

#include "event.h"
#include "audio.h"
#include "gconfig.h"

#include <values.h>
#include <iostream>
#include <errno.h>
#include <values.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <math.h>

#include <QMimeData>
#include <QByteArray>
#include <QDrag>
#include <QMessageBox>
#include <QClipboard>

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


using namespace std;

GateTime* gatetime_dialog=NULL;
Velocity* velocity_dialog=NULL;
Quantize* quantize_dialog=NULL;
Remove* erase_dialog=NULL;
DelOverlaps* del_overlaps_dialog=NULL;
Setlen* set_notelen_dialog=NULL;
Move* move_notes_dialog=NULL;
Transpose* transpose_dialog=NULL;
Crescendo* crescendo_dialog=NULL;
Legato* legato_dialog=NULL;

void init_function_dialogs(QWidget* parent)
{
	gatetime_dialog = new GateTime(parent);
	velocity_dialog = new Velocity(parent);
	quantize_dialog = new Quantize(parent);
	erase_dialog = new Remove(parent);
	del_overlaps_dialog = new DelOverlaps(parent);
	set_notelen_dialog = new Setlen(parent);
	move_notes_dialog = new Move(parent);
	transpose_dialog = new Transpose(parent);
	crescendo_dialog = new Crescendo(parent);
	legato_dialog = new Legato(parent);
}

set<Part*> partlist_to_set(PartList* pl)
{
	set<Part*> result;
	
	for (PartList::iterator it=pl->begin(); it!=pl->end(); it++)
		result.insert(it->second);
	
	return result;
}

set<Part*> part_to_set(Part* p)
{
	set<Part*> result;
	result.insert(p);
	return result;
}

set<Part*> get_all_parts()
{
	set<Part*> result;
	
	TrackList* tracks=song->tracks();
	for (TrackList::const_iterator t_it=tracks->begin(); t_it!=tracks->end(); t_it++)
	{
		const PartList* parts=(*t_it)->cparts();
		for (ciPart p_it=parts->begin(); p_it!=parts->end(); p_it++)
			result.insert(p_it->second);
	}
	
	return result;
}

set<Part*> get_all_selected_parts()
{
	set<Part*> result;
	
	TrackList* tracks=song->tracks();
	for (TrackList::const_iterator t_it=tracks->begin(); t_it!=tracks->end(); t_it++)
	{
		const PartList* parts=(*t_it)->cparts();
		for (ciPart p_it=parts->begin(); p_it!=parts->end(); p_it++)
			if (p_it->second->selected())
				result.insert(p_it->second);
	}
	
	return result;
}

bool is_relevant(const Event& event, const Part* part, int range)
{
	unsigned tick;
	
	if (event.type()!=Note) return false;
	
	switch (range)
	{
		case 0: return true;
		case 1: return event.selected();
		case 2: tick=event.tick()+part->tick(); return (tick >= song->lpos()) && (tick < song->rpos());
		case 3: return is_relevant(event,part,1) && is_relevant(event,part,2);
		default: cout << "ERROR: ILLEGAL FUNCTION CALL in is_relevant: range is illegal: "<<range<<endl;
		         return false;
	}
}


map<Event*, Part*> get_events(const set<Part*>& parts, int range)
{
	map<Event*, Part*> events;
	
	for (set<Part*>::iterator part=parts.begin(); part!=parts.end(); part++)
		for (iEvent event=(*part)->events()->begin(); event!=(*part)->events()->end(); event++)
			if (is_relevant(event->second, *part, range))
				events.insert(pair<Event*, Part*>(&event->second, *part));
	
	return events;
}




bool modify_notelen(const set<Part*>& parts)
{
	if (!gatetime_dialog->exec())
		return false;
		
	modify_notelen(parts,gatetime_dialog->range,gatetime_dialog->rateVal,gatetime_dialog->offsetVal);
	
	return true;
}

bool modify_velocity(const set<Part*>& parts)
{
	if (!velocity_dialog->exec())
		return false;
		
	modify_velocity(parts,velocity_dialog->range,velocity_dialog->rateVal,velocity_dialog->offsetVal);
	
	return true;
}

bool quantize_notes(const set<Part*>& parts)
{
	if (!quantize_dialog->exec())
		return false;
		
	quantize_notes(parts, quantize_dialog->range, (config.division*4)/(1<<quantize_dialog->raster_power2),
	               quantize_dialog->quant_len, quantize_dialog->strength, quantize_dialog->swing,
	               quantize_dialog->threshold);
	
	return true;
}

bool erase_notes(const set<Part*>& parts)
{
	if (!erase_dialog->exec())
		return false;
		
	erase_notes(parts,erase_dialog->range, erase_dialog->velo_threshold, erase_dialog->velo_thres_used, 
	                                       erase_dialog->len_threshold, erase_dialog->len_thres_used );
	
	return true;
}

bool delete_overlaps(const set<Part*>& parts)
{
	if (!del_overlaps_dialog->exec())
		return false;
		
	delete_overlaps(parts,erase_dialog->range);
	
	return true;
}

bool set_notelen(const set<Part*>& parts)
{
	if (!set_notelen_dialog->exec())
		return false;
		
	set_notelen(parts,set_notelen_dialog->range,set_notelen_dialog->len);
	
	return true;
}

bool move_notes(const set<Part*>& parts)
{
	if (!move_notes_dialog->exec())
		return false;
		
	move_notes(parts,move_notes_dialog->range,move_notes_dialog->amount);
	
	return true;
}

bool transpose_notes(const set<Part*>& parts)
{
	if (!transpose_dialog->exec())
		return false;
		
	transpose_notes(parts,transpose_dialog->range,transpose_dialog->amount);
	
	return true;
}

bool crescendo(const set<Part*>& parts)
{
	if (song->rpos() <= song->lpos())
	{
		QMessageBox::warning(NULL, QObject::tr("Error"), QObject::tr("Please first select the range for crescendo with the loop markers."));
		return false;
	}
	
	if (!crescendo_dialog->exec())
		return false;
		
	crescendo(parts,crescendo_dialog->range,crescendo_dialog->start_val,crescendo_dialog->end_val,crescendo_dialog->absolute);
	
	return true;
}

bool legato(const set<Part*>& parts)
{
	if (!legato_dialog->exec())
		return false;
		
	legato(parts,legato_dialog->range, legato_dialog->min_len, !legato_dialog->allow_shortening);
	
	return true;
}



bool modify_notelen()
{
	if (!gatetime_dialog->exec())
		return false;
		
	set<Part*> parts;
	if (gatetime_dialog->range & FUNCTION_RANGE_ONLY_SELECTED)
		parts=get_all_selected_parts();
	else
		parts=get_all_parts();
		
	modify_notelen(parts,gatetime_dialog->range & FUNCTION_RANGE_ONLY_BETWEEN_MARKERS, gatetime_dialog->rateVal,gatetime_dialog->offsetVal);
	
	return true;
}

bool modify_velocity()
{
	if (!velocity_dialog->exec())
		return false;
		
	set<Part*> parts;
	if (velocity_dialog->range & FUNCTION_RANGE_ONLY_SELECTED)
		parts=get_all_selected_parts();
	else
		parts=get_all_parts();
		
	modify_velocity(parts,velocity_dialog->range & FUNCTION_RANGE_ONLY_BETWEEN_MARKERS,velocity_dialog->rateVal,velocity_dialog->offsetVal);
	
	return true;
}

bool quantize_notes()
{
	if (!quantize_dialog->exec())
		return false;
		
	set<Part*> parts;
	if (quantize_dialog->range & FUNCTION_RANGE_ONLY_SELECTED)
		parts=get_all_selected_parts();
	else
		parts=get_all_parts();
		
	quantize_notes(parts, quantize_dialog->range & FUNCTION_RANGE_ONLY_BETWEEN_MARKERS, (config.division*4)/(1<<quantize_dialog->raster_power2),
	               quantize_dialog->quant_len, quantize_dialog->strength, quantize_dialog->swing,
	               quantize_dialog->threshold);
	
	return true;
}

bool erase_notes()
{
	if (!erase_dialog->exec())
		return false;
		
	set<Part*> parts;
	if (erase_dialog->range & FUNCTION_RANGE_ONLY_SELECTED)
		parts=get_all_selected_parts();
	else
		parts=get_all_parts();
		
	erase_notes(parts,erase_dialog->range & FUNCTION_RANGE_ONLY_BETWEEN_MARKERS, erase_dialog->velo_threshold, erase_dialog->velo_thres_used, 
	            erase_dialog->len_threshold, erase_dialog->len_thres_used );
	
	return true;
}

bool delete_overlaps()
{
	if (!del_overlaps_dialog->exec())
		return false;
		
	set<Part*> parts;
	if (del_overlaps_dialog->range & FUNCTION_RANGE_ONLY_SELECTED)
		parts=get_all_selected_parts();
	else
		parts=get_all_parts();
		
	delete_overlaps(parts,erase_dialog->range & FUNCTION_RANGE_ONLY_BETWEEN_MARKERS);
	
	return true;
}

bool set_notelen()
{
	if (!set_notelen_dialog->exec())
		return false;
		
	set<Part*> parts;
	if (set_notelen_dialog->range & FUNCTION_RANGE_ONLY_SELECTED)
		parts=get_all_selected_parts();
	else
		parts=get_all_parts();
		
	set_notelen(parts,set_notelen_dialog->range & FUNCTION_RANGE_ONLY_BETWEEN_MARKERS, set_notelen_dialog->len);
	
	return true;
}

bool move_notes()
{
	if (!move_notes_dialog->exec())
		return false;
		
	set<Part*> parts;
	if (move_notes_dialog->range & FUNCTION_RANGE_ONLY_SELECTED)
		parts=get_all_selected_parts();
	else
		parts=get_all_parts();
		
	move_notes(parts,move_notes_dialog->range & FUNCTION_RANGE_ONLY_BETWEEN_MARKERS, move_notes_dialog->amount);
	
	return true;
}

bool transpose_notes()
{
	if (!transpose_dialog->exec())
		return false;
		
	set<Part*> parts;
	if (transpose_dialog->range & FUNCTION_RANGE_ONLY_SELECTED)
		parts=get_all_selected_parts();
	else
		parts=get_all_parts();
		
	transpose_notes(parts,transpose_dialog->range & FUNCTION_RANGE_ONLY_BETWEEN_MARKERS, transpose_dialog->amount);
	
	return true;
}

bool crescendo()
{
	if (song->rpos() <= song->lpos())
	{
		QMessageBox::warning(NULL, QObject::tr("Error"), QObject::tr("Please first select the range for crescendo with the loop markers."));
		return false;
	}
	
	if (!crescendo_dialog->exec())
		return false;
		
	set<Part*> parts;
	if (crescendo_dialog->range & FUNCTION_RANGE_ONLY_SELECTED)
		parts=get_all_selected_parts();
	else
		parts=get_all_parts();
		
	crescendo(parts,crescendo_dialog->range & FUNCTION_RANGE_ONLY_BETWEEN_MARKERS, crescendo_dialog->start_val,crescendo_dialog->end_val,crescendo_dialog->absolute);
	
	return true;
}

bool legato()
{
	if (!legato_dialog->exec())
		return false;
		
	set<Part*> parts;
	if (legato_dialog->range & FUNCTION_RANGE_ONLY_SELECTED)
		parts=get_all_selected_parts();
	else
		parts=get_all_parts();
		
	legato(parts,legato_dialog->range & FUNCTION_RANGE_ONLY_BETWEEN_MARKERS, legato_dialog->min_len, !legato_dialog->allow_shortening);
	
	return true;
}






bool modify_velocity(const set<Part*>& parts, int range, int rate, int offset)
{
	map<Event*, Part*> events = get_events(parts, range);
	Undo operations;
	
	if ( (!events.empty()) && ((rate!=100) || (offset!=0)) )
	{
		for (map<Event*, Part*>::iterator it=events.begin(); it!=events.end(); it++)
		{
			Event& event=*(it->first);
			Part* part=it->second;
			
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
		
		return song->applyOperationGroup(operations);
	}
	else
		return false;
}

bool modify_off_velocity(const set<Part*>& parts, int range, int rate, int offset)
{
	map<Event*, Part*> events = get_events(parts, range);
	Undo operations;
	
	if ( (!events.empty()) && ((rate!=100) || (offset!=0)) )
	{
		for (map<Event*, Part*>::iterator it=events.begin(); it!=events.end(); it++)
		{
			Event& event=*(it->first);
			Part* part=it->second;
			
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

		return song->applyOperationGroup(operations);
	}
	else
		return false;
}

bool modify_notelen(const set<Part*>& parts, int range, int rate, int offset)
{
	map<Event*, Part*> events = get_events(parts, range);
	Undo operations;
	map<Part*, int> partlen;
	
	if ( (!events.empty()) && ((rate!=100) || (offset!=0)) )
	{
		for (map<Event*, Part*>::iterator it=events.begin(); it!=events.end(); it++)
		{
			Event& event=*(it->first);
			Part* part=it->second;

			unsigned int len = event.lenTick(); //prevent compiler warning: comparison singed/unsigned

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
		
		for (map<Part*, int>::iterator it=partlen.begin(); it!=partlen.end(); it++)
			schedule_resize_all_same_len_clone_parts(it->first, it->second, operations);

		return song->applyOperationGroup(operations);
	}
	else
		return false;
}

bool set_notelen(const set<Part*>& parts, int range, int len)
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

	int tick_diff1 = tick_dest1 - tick;
	int tick_diff2 = tick_dest2 - tick;
	int tick_diff3 = tick_dest3 - tick;
	
	if ((abs(tick_diff1) <= abs(tick_diff2)) && (abs(tick_diff1) <= abs(tick_diff3))) //tick_dest1 is the nearest tick
		return tick_dest1;
	else if ((abs(tick_diff2) <= abs(tick_diff1)) && (abs(tick_diff2) <= abs(tick_diff3))) //tick_dest2 is the nearest tick
		return tick_dest2;
	else
		return tick_dest3;
}

bool quantize_notes(const set<Part*>& parts, int range, int raster, bool quant_len, int strength, int swing, int threshold)
{
	map<Event*, Part*> events = get_events(parts, range);
	Undo operations;
	
	if (!events.empty())
	{
		for (map<Event*, Part*>::iterator it=events.begin(); it!=events.end(); it++)
		{
			Event& event=*(it->first);
			Part* part=it->second;

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
		
		return song->applyOperationGroup(operations);
	}
	else
		return false;
}

bool erase_notes(const set<Part*>& parts, int range, int velo_threshold, bool velo_thres_used, int len_threshold, bool len_thres_used)
{
	map<Event*, Part*> events = get_events(parts, range);
	Undo operations;
	
	if (!events.empty())
	{
		for (map<Event*, Part*>::iterator it=events.begin(); it!=events.end(); it++)
		{
			Event& event=*(it->first);
			Part* part=it->second;

			if ( (!velo_thres_used && !len_thres_used) ||
			     (velo_thres_used && event.velo() < velo_threshold) ||
			     (len_thres_used && int(event.lenTick()) < len_threshold) )
				operations.push_back(UndoOp(UndoOp::DeleteEvent, event, part, false, false));
		}
		
		return song->applyOperationGroup(operations);
	}
	else
		return false;
}

bool transpose_notes(const set<Part*>& parts, int range, signed int halftonesteps)
{
	map<Event*, Part*> events = get_events(parts, range);
	Undo operations;
	
	if ( (!events.empty()) && (halftonesteps!=0) )
	{
		for (map<Event*, Part*>::iterator it=events.begin(); it!=events.end(); it++)
		{
			Event& event=*(it->first);
			Part* part=it->second;

			Event newEvent = event.clone();
			int pitch = event.pitch()+halftonesteps;
			if (pitch > 127) pitch=127;
			if (pitch < 0) pitch=0;
			newEvent.setPitch(pitch);
			operations.push_back(UndoOp(UndoOp::ModifyEvent, newEvent, event, part, false, false));
		}
		
		return song->applyOperationGroup(operations);
	}
	else
		return false;
}

bool crescendo(const set<Part*>& parts, int range, int start_val, int end_val, bool absolute)
{
	map<Event*, Part*> events = get_events(parts, range);
	Undo operations;
	
	int from=song->lpos();
	int to=song->rpos();
	
	if ( (!events.empty()) && (to>from) )
	{
		for (map<Event*, Part*>::iterator it=events.begin(); it!=events.end(); it++)
		{
			Event& event=*(it->first);
			Part* part=it->second;
			
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
		
		return song->applyOperationGroup(operations);
	}
	else
		return false;
}

bool move_notes(const set<Part*>& parts, int range, signed int ticks)
{
	map<Event*, Part*> events = get_events(parts, range);
	Undo operations;
	map<Part*, int> partlen;
	
	if ( (!events.empty()) && (ticks!=0) )
	{
		for (map<Event*, Part*>::iterator it=events.begin(); it!=events.end(); it++)
		{
			Event& event=*(it->first);
			Part* part=it->second;
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
		
		for (map<Part*, int>::iterator it=partlen.begin(); it!=partlen.end(); it++)
			schedule_resize_all_same_len_clone_parts(it->first, it->second, operations);
		
		return song->applyOperationGroup(operations);
	}
	else
		return false;
}


bool delete_overlaps(const set<Part*>& parts, int range)
{
	map<Event*, Part*> events = get_events(parts, range);
	Undo operations;
	
	set<Event*> deleted_events;
	
	if (!events.empty())
	{
		for (map<Event*, Part*>::iterator it1=events.begin(); it1!=events.end(); it1++)
		{
			Event& event1=*(it1->first);
			Part* part1=it1->second;
			
			// we may NOT optimize by letting it2 start at (it1 +1); this optimisation
			// is only allowed when events was sorted by time. it is, however, sorted
			// randomly by pointer.
			for (map<Event*, Part*>::iterator it2=events.begin(); it2!=events.end(); it2++)
			{
				Event& event2=*(it2->first);
				Part* part2=it2->second;
				
				if ( (part1->events()==part2->events()) && // part1 and part2 are the same or are duplicates
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
		
		return song->applyOperationGroup(operations);
	}
	else
		return false;
}

bool legato(const set<Part*>& parts, int range, int min_len, bool dont_shorten)
{
	map<Event*, Part*> events = get_events(parts, range);
	Undo operations;
	
	if (min_len<=0) min_len=1;
	
	if (!events.empty())
	{
		for (map<Event*, Part*>::iterator it1=events.begin(); it1!=events.end(); it1++)
		{
			Event& event1=*(it1->first);
			Part* part1=it1->second;
			
			unsigned len=MAXINT;
			// we may NOT optimize by letting it2 start at (it1 +1); this optimisation
			// is only allowed when events was sorted by time. it is, however, sorted
			// randomly by pointer.
			for (map<Event*, Part*>::iterator it2=events.begin(); it2!=events.end(); it2++)
			{
				Event& event2=*(it2->first);
				Part* part2=it2->second;
				
				bool relevant = (event2.tick() >= event1.tick() + min_len);
				if (dont_shorten)
					relevant = relevant && (event2.tick() >= event1.endTick());
				
				if ( (part1->events()==part2->events()) &&  // part1 and part2 are the same or are duplicates
				      relevant &&                           // they're not too near (respect min_len and dont_shorten)
				     (event2.tick()-event1.tick() < len ) ) // that's the nearest relevant following note
					len=event2.tick()-event1.tick();
			}
			
			if (len==MAXINT) len=event1.lenTick(); // if no following note was found, keep the length
			
			if (event1.lenTick() != len)
			{
				Event new_event1 = event1.clone();
				new_event1.setLenTick(len);
				
				operations.push_back(UndoOp(UndoOp::ModifyEvent, new_event1, event1, part1, false, false));
			}
		}
		
		return song->applyOperationGroup(operations);
	}
	else
		return false;
}



void copy_notes(const set<Part*>& parts, int range)
{
	QMimeData* drag = selected_events_to_mime(parts,range);

	if (drag)
		QApplication::clipboard()->setMimeData(drag, QClipboard::Clipboard);
}

void paste_notes(Part* dest_part)
{
	QString tmp="x-muse-eventlist"; // QClipboard::text() expects a QString&, not a QString :(
	QString s = QApplication::clipboard()->text(tmp, QClipboard::Clipboard);  // TODO CHECK Tim.
	paste_at(dest_part, s, song->cpos());
}

QMimeData* selected_events_to_mime(const set<Part*>& parts, int range)
{
	map<Event*, Part*> events=get_events(parts,range);

	//---------------------------------------------------
	//   generate event list from selected events
	//---------------------------------------------------

	EventList el;
	unsigned startTick = MAXINT; //will be the tick of the first event or MAXINT if no events are there

	for (map<Event*, Part*>::iterator it=events.begin(); it!=events.end(); it++)
	{
		Event& e   = *it->first;
		
		if (e.tick() < startTick)
			startTick = e.tick();
			
		el.add(e);
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
	
	xml.tag(level++, "eventlist");
	for (ciEvent e = el.begin(); e != el.end(); ++e)
		e->second.write(level, xml, -startTick);
	xml.etag(--level, "eventlist");

	//---------------------------------------------------
	//    read tmp file into drag Object
	//---------------------------------------------------

	fflush(tmp);
	struct stat f_stat;
	if (fstat(fileno(tmp), &f_stat) == -1)
	{
		fprintf(stderr, "PianoCanvas::copy() fstat failed:<%s>\n",
		strerror(errno));
		fclose(tmp);
		return 0;
	}
	int n = f_stat.st_size;
	char* fbuf  = (char*)mmap(0, n+1, PROT_READ|PROT_WRITE,
	MAP_PRIVATE, fileno(tmp), 0);
	fbuf[n] = 0;

	QByteArray data(fbuf);
	QMimeData* md = new QMimeData();

	md->setData("text/x-muse-eventlist", data);

	munmap(fbuf, n);
	fclose(tmp);

	return md;
}

void paste_at(Part* dest_part, const QString& pt, int pos)
{
	Undo operations;
	unsigned newpartlen=dest_part->lenTick();
	
	Xml xml(pt.toLatin1().constData());
	for (;;) 
	{
		Xml::Token token = xml.parse();
		const QString& tag = xml.s1();
		switch (token) 
		{
			case Xml::Error:
			case Xml::End:
				goto end_of_paste_at;
				
			case Xml::TagStart:
				if (tag == "eventlist")
				{
					EventList el;
					el.read(xml, "eventlist", true);
					for (iEvent i = el.begin(); i != el.end(); ++i)
					{
						Event e = i->second;
						int tick = e.tick() + pos - dest_part->tick();
						if (tick<0)
						{
							printf("ERROR: trying to add event before current part!\n");
							goto end_of_paste_at;
						}

						e.setTick(tick);
						e.setSelected(true);
						
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
								if (e.endTick() > newpartlen)
									newpartlen=e.endTick();
							}
						}
						
						if (e.lenTick() != 0) operations.push_back(UndoOp(UndoOp::AddEvent,e, dest_part, false, false));
					}
					
					if (newpartlen != dest_part->lenTick())
						schedule_resize_all_same_len_clone_parts(dest_part, newpartlen, operations);

					song->applyOperationGroup(operations);
					goto end_of_paste_at;
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
	
	end_of_paste_at:
	song->update(SC_SELECTION);
}

void select_all(const std::set<Part*>& parts)
{
	for (set<Part*>::iterator part=parts.begin(); part!=parts.end(); part++)
		for (iEvent ev_it=(*part)->events()->begin(); ev_it!=(*part)->events()->end(); ev_it++)
		{
			Event& event=ev_it->second;
			event.setSelected(true);
		}
	song->update(SC_SELECTION);
}

void select_none(const std::set<Part*>& parts)
{
	for (set<Part*>::iterator part=parts.begin(); part!=parts.end(); part++)
		for (iEvent ev_it=(*part)->events()->begin(); ev_it!=(*part)->events()->end(); ev_it++)
		{
			Event& event=ev_it->second;
			event.setSelected(false);
		}
	song->update(SC_SELECTION);
}

void select_invert(const std::set<Part*>& parts)
{
	for (set<Part*>::iterator part=parts.begin(); part!=parts.end(); part++)
		for (iEvent ev_it=(*part)->events()->begin(); ev_it!=(*part)->events()->end(); ev_it++)
		{
			Event& event=ev_it->second;
			event.setSelected(!event.selected());
		}
	song->update(SC_SELECTION);
}

void select_in_loop(const std::set<Part*>& parts)
{
	select_none(parts);
	for (set<Part*>::iterator part=parts.begin(); part!=parts.end(); part++)
		for (iEvent ev_it=(*part)->events()->begin(); ev_it!=(*part)->events()->end(); ev_it++)
		{
			Event& event=ev_it->second;
			event.setSelected((event.tick()>=song->lpos() && event.endTick()<=song->rpos()));
		}
	song->update(SC_SELECTION);
}

void select_not_in_loop(const std::set<Part*>& parts)
{
	select_none(parts);
	for (set<Part*>::iterator part=parts.begin(); part!=parts.end(); part++)
		for (iEvent ev_it=(*part)->events()->begin(); ev_it!=(*part)->events()->end(); ev_it++)
		{
			Event& event=ev_it->second;
			event.setSelected(!(event.tick()>=song->lpos() && event.endTick()<=song->rpos()));
		}
	song->update(SC_SELECTION);
}


void shrink_parts(int raster)
{
	Undo operations;
	
	unsigned min_len;
	if (raster<0) raster=config.division;
	if (raster>=0) min_len=raster; else min_len=config.division;
	
	TrackList* tracks = song->tracks();
	for (iTrack track = tracks->begin(); track != tracks->end(); track++)
		for (iPart part = (*track)->parts()->begin(); part != (*track)->parts()->end(); part++)
			if (part->second->selected())
			{
				EventList* events=part->second->events();
				unsigned len=0;
				
				for (iEvent ev=events->begin(); ev!=events->end(); ev++)
					if (ev->second.endTick() > len)
						len=ev->second.endTick();
				
				if (raster) len=ceil((float)len/raster)*raster;
				if (len<min_len) len=min_len;
				
				if (len < part->second->lenTick())
				{
					MidiPart* new_part = new MidiPart(*(MidiPart*)part->second);
					new_part->setLenTick(len);
					operations.push_back(UndoOp(UndoOp::ModifyPart, part->second, new_part, true, false));
				}
			}
	
	song->applyOperationGroup(operations);
}

void internal_schedule_expand_part(Part* part, int raster, Undo& operations)
{
	EventList* events=part->events();
	unsigned len=part->lenTick();
	
	for (iEvent ev=events->begin(); ev!=events->end(); ev++)
		if (ev->second.endTick() > len)
			len=ev->second.endTick();

	if (raster) len=ceil((float)len/raster)*raster;
					
	if (len > part->lenTick())
	{
		MidiPart* new_part = new MidiPart(*(MidiPart*)part);
		new_part->setLenTick(len);
		operations.push_back(UndoOp(UndoOp::ModifyPart, part, new_part, true, false));
	}
}

void schedule_resize_all_same_len_clone_parts(Part* part, unsigned new_len, Undo& operations)
{
	unsigned old_len=part->lenTick();
	if (old_len!=new_len)
	{
		Part* part_it=part;
		do
		{
			if (part_it->lenTick()==old_len)
			{
				MidiPart* new_part = new MidiPart(*(MidiPart*)part_it);
				new_part->setLenTick(new_len);
				operations.push_back(UndoOp(UndoOp::ModifyPart, part_it, new_part, true, false));
			}
			
			part_it=part_it->nextClone();
		} while (part_it!=part);
	}
}

void expand_parts(int raster)
{
	Undo operations;
	
	unsigned min_len;
	if (raster<0) raster=config.division;
	if (raster>=0) min_len=raster; else min_len=config.division;

	TrackList* tracks = song->tracks();
	for (iTrack track = tracks->begin(); track != tracks->end(); track++)
		for (iPart part = (*track)->parts()->begin(); part != (*track)->parts()->end(); part++)
			if (part->second->selected())
			{
				EventList* events=part->second->events();
				unsigned len=part->second->lenTick();
				
				for (iEvent ev=events->begin(); ev!=events->end(); ev++)
					if (ev->second.endTick() > len)
						len=ev->second.endTick();

				if (raster) len=ceil((float)len/raster)*raster;
				if (len<min_len) len=min_len;
								
				if (len > part->second->lenTick())
				{
					MidiPart* new_part = new MidiPart(*(MidiPart*)part->second);
					new_part->setLenTick(len);
					operations.push_back(UndoOp(UndoOp::ModifyPart, part->second, new_part, true, false));
				}
			}
			
	song->applyOperationGroup(operations);
}

void clean_parts()
{
	Undo operations;
	set<Part*> already_processed;
	
	TrackList* tracks = song->tracks();
	for (iTrack track = tracks->begin(); track != tracks->end(); track++)
		for (iPart part = (*track)->parts()->begin(); part != (*track)->parts()->end(); part++)
			if ((part->second->selected()) && (already_processed.find(part->second)==already_processed.end()))
			{ 
				// find out the length of the longest clone of this part;
				// avoid processing eventlist multiple times (because of
				// multiple clones)
				unsigned len=0;
				
				Part* part_it=part->second;
				do
				{
					if (part_it->lenTick() > len)
						len=part_it->lenTick();
						
					already_processed.insert(part_it);
					part_it=part_it->nextClone();
				} while ((part_it!=part->second) && (part_it!=NULL));

				
				// erase all events exceeding the longest clone of this part
				// (i.e., erase all hidden events) or shorten them
				EventList* el = part->second->events();
				for (iEvent ev=el->begin(); ev!=el->end(); ev++)
					if (ev->second.tick() >= len)
						operations.push_back(UndoOp(UndoOp::DeleteEvent, ev->second, part->second, true, true));
					else if (ev->second.endTick() > len)
					{
						Event new_event = ev->second.clone();
						new_event.setLenTick(len - ev->second.tick());
						
						operations.push_back(UndoOp(UndoOp::ModifyEvent, new_event, ev->second, part->second, true, true));
					}
			}
	
	song->applyOperationGroup(operations);
}


void read_function_dialog_config(Xml& xml)
{
	if (erase_dialog==NULL)
	{
		cout << "ERROR: THIS SHOULD NEVER HAPPEN: read_function_dialog_config() called, but\n"
		        "                                 dialogs are still uninitalized (NULL)!"<<endl;
		return;
	}
		
	for (;;)
	{
		Xml::Token token = xml.parse();
		if (token == Xml::Error || token == Xml::End)
			break;
			
		const QString& tag = xml.s1();
		switch (token)
		{
			case Xml::TagStart:
				if (tag == "mod_len")
					gatetime_dialog->read_configuration(xml);
				else if (tag == "mod_velo")
					velocity_dialog->read_configuration(xml);
				else if (tag == "quantize")
					quantize_dialog->read_configuration(xml);
				else if (tag == "erase")
					erase_dialog->read_configuration(xml);
				else if (tag == "del_overlaps")
					del_overlaps_dialog->read_configuration(xml);
				else if (tag == "setlen")
					set_notelen_dialog->read_configuration(xml);
				else if (tag == "move")
					move_notes_dialog->read_configuration(xml);
				else if (tag == "transpose")
					transpose_dialog->read_configuration(xml);
				else if (tag == "crescendo")
					crescendo_dialog->read_configuration(xml);
				else if (tag == "legato")
					legato_dialog->read_configuration(xml);
				else
					xml.unknown("function_dialogs");
				break;
				
			case Xml::TagEnd:
				if (tag == "dialogs")
					return;
				
			default:
				break;
		}
	}
}

void write_function_dialog_config(int level, Xml& xml)
{
	xml.tag(level++, "dialogs");

	gatetime_dialog->write_configuration(level, xml);
	velocity_dialog->write_configuration(level, xml);
	quantize_dialog->write_configuration(level, xml);
	erase_dialog->write_configuration(level, xml);
	del_overlaps_dialog->write_configuration(level, xml);
	set_notelen_dialog->write_configuration(level, xml);
	move_notes_dialog->write_configuration(level, xml);
	transpose_dialog->write_configuration(level, xml);
	crescendo_dialog->write_configuration(level, xml);
	legato_dialog->write_configuration(level, xml);

	xml.tag(level, "/dialogs");
}
