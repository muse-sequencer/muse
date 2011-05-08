//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: functions.cpp,v 1.20.2.19 2011/05/05 20:10 flo93 Exp $
//  (C) Copyright 2011 Florian Jung (flo93@sourceforge.net)
//=========================================================

#include "functions.h"
#include "song.h"

#include "event.h"
#include "audio.h"
#include "gconfig.h"

#include <iostream>

using namespace std;

GateTime* gatetime_dialog=NULL;
Velocity* velocity_dialog=NULL;
Quantize* quantize_dialog=NULL;

void init_function_dialogs(QWidget* parent)
{
	gatetime_dialog = new GateTime(parent);
	velocity_dialog = new Velocity(parent);
	quantize_dialog = new Quantize(parent);
}

set<Part*> partlist_to_set(PartList* pl)
{
	set<Part*> result;
	
	for (PartList::iterator it=pl->begin(); it!=pl->end(); it++)
		result.insert(it->second);
	
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
	               quantize_dialog->strength, quantize_dialog->swing, quantize_dialog->threshold);
	
	return true;
}



void modify_velocity(const set<Part*>& parts, int range, int rate, int offset)
{
	map<Event*, Part*> events;
	
	for (set<Part*>::iterator part=parts.begin(); part!=parts.end(); part++)
		for (iEvent event=(*part)->events()->begin(); event!=(*part)->events()->end(); event++)
			if (is_relevant(event->second, *part, range))
				events.insert(pair<Event*, Part*>(&event->second, *part));
	
	
	song->startUndo();
	
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
			// Indicate no undo, and do not do port controller values and clone parts. 
			audio->msgChangeEvent(event, newEvent, part, false, false, false);
		}
	}
	
	song->endUndo(SC_EVENT_MODIFIED);
}

void modify_notelen(const set<Part*>& parts, int range, int rate, int offset)
{
	map<Event*, Part*> events;
	
	for (set<Part*>::iterator part=parts.begin(); part!=parts.end(); part++)
		for (iEvent event=(*part)->events()->begin(); event!=(*part)->events()->end(); event++)
			if (is_relevant(event->second, *part, range))
				events.insert(pair<Event*, Part*>(&event->second, *part));
	
	
	song->startUndo();
	
	for (map<Event*, Part*>::iterator it=events.begin(); it!=events.end(); it++)
	{
		Event& event=*(it->first);
		Part* part=it->second;

		unsigned int len = event.lenTick(); //prevent compiler warning: comparison singed/unsigned

		len = (len * rate) / 100;
		len += offset;

		if (len <= 0)
			len = 1;
			
		if (event.lenTick() != len)
		{
			Event newEvent = event.clone();
			newEvent.setLenTick(len);
			// Indicate no undo, and do not do port controller values and clone parts. 
			audio->msgChangeEvent(event, newEvent, part, false, false, false);
		}
	}
	
	song->endUndo(SC_EVENT_MODIFIED);
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

void quantize_notes(const set<Part*>& parts, int range, int raster, int strength, int swing, int threshold)
{
	map<Event*, Part*> events;
	
	for (set<Part*>::iterator part=parts.begin(); part!=parts.end(); part++)
		for (iEvent event=(*part)->events()->begin(); event!=(*part)->events()->end(); event++)
			if (is_relevant(event->second, *part, range))
				events.insert(pair<Event*, Part*>(&event->second, *part));
	
	
	song->startUndo();
	
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
			
		if (abs(len_diff) > threshold)
			len = len + len_diff*strength/100;

		if (len <= 0)
			len = 1;

			
		if ( (event.lenTick() != len) || (event.tick() + part->tick() != begin_tick) )
		{
			Event newEvent = event.clone();
			newEvent.setTick(begin_tick - part->tick());
			newEvent.setLenTick(len);
			// Indicate no undo, and do not do port controller values and clone parts. 
			audio->msgChangeEvent(event, newEvent, part, false, false, false);
		}
	}
	
	song->endUndo(SC_EVENT_MODIFIED);
}


