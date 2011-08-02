//=========================================================
//  MusE
//  Linux Music Editor
//  steprec.cpp
//  (C) Copyright 2011 Florian Jung (flo93@users.sourceforge.net)
//=========================================================

#include "steprec.h"
#include "part.h"
#include "event.h"
#include "globals.h"
#include "functions.h"

#include "song.h"
#include "audio.h"

#include <set>

#define CHORD_TIMEOUT 75

StepRec::StepRec(bool* note_held_down_array)
{
	note_held_down=note_held_down_array;

	chord_timer=new QTimer(this);
	chord_timer->setSingleShot(true);
	chord_timer->setInterval(CHORD_TIMEOUT);
	chord_timer->stop();
	connect(chord_timer, SIGNAL(timeout()), SLOT(timeout()));
}

void StepRec::timeout()
{
	if (chord_timer_set_to_tick != song->cpos())
	{
		Pos p(chord_timer_set_to_tick, true);
		song->setPos(0, p, true, false, true);
	}
}

void StepRec::record(Part* part, int pitch, int len, int step, int velo, bool ctrl, bool shift)
{
	unsigned tick = song->cpos();
	unsigned lasttick=0;
	Undo operations;
	
	if (pitch!=rcSteprecNote) 
	{
		chord_timer->stop();

		// extend len of last note?
		EventList* events = part->events();
		if (ctrl)
		{
			for (iEvent i = events->begin(); i != events->end(); ++i)
			{
				Event ev = i->second;
				if (ev.isNote() && ev.pitch() == pitch && ((ev.tick() + ev.lenTick() + part->tick()) == tick))
				{
					Event e = ev.clone();
					e.setLenTick(ev.lenTick() + len);
					operations.push_back(UndoOp(UndoOp::ModifyEvent, e,ev, part, false, false));
					
					if (!shift)
					{
						chord_timer_set_to_tick = tick + step;
						chord_timer->start();
					}
					
					lasttick=tick+len - part->tick();
					goto steprec_record_foot;
				}
			}
		}

		if (tick<=part->endTick())
		{
			// if we already entered the note, delete it
			// if we would find a note after part->lenTick(), the above "if"
			// avoids this. this has to be avoided because then part->hasHiddenNotes() is true
			// which results in forbidding any action beyond its end
			EventRange range = events->equal_range(tick - part->tick());
			for (iEvent i = range.first; i != range.second; ++i)
			{
				Event ev = i->second;
				if (ev.isNote() && ev.pitch() == pitch)
				{
					audio->msgDeleteEvent(ev, part, true, false, false);

					if (!shift)
					{
						chord_timer_set_to_tick = tick + step;
						chord_timer->start();
					}
					
					return;
				}
			}
		}
		
				
		Event e(Note);
		e.setTick(tick - part->tick());
		e.setPitch(pitch);
		e.setVelo(velo);
		e.setLenTick(len);
		operations.push_back(UndoOp(UndoOp::AddEvent, e, part, false, false));
		lasttick=e.endTick();

		if (! (globalKeyState & Qt::ShiftModifier))
		{
			chord_timer_set_to_tick = tick + step;
			chord_timer->start();
		}
		
		goto steprec_record_foot; // this is actually unneccessary, but for clarity
	}
	else  // equals if (pitch==rcSteprecNote)
	{
		bool held_notes=false;
		if (note_held_down!=NULL)
		{
			for (int i=0;i<128;i++)
			if (note_held_down[i]) { held_notes=true; break; }
		}
		else
			held_notes=false;
			 

		if (held_notes)
		{
			chord_timer->stop();
			
			// extend len of last note(s)
			using std::set;
			
			set<Event*> extend_set;
			EventList* events = part->events();
			for (iEvent i = events->begin(); i != events->end(); ++i)
			{
				Event& ev = i->second;
				if (ev.isNote() && note_held_down[ev.pitch()] && ((ev.tick() + ev.lenTick() + part->tick()) == tick))
					extend_set.insert(&ev);
			}
			
			for (set<Event*>::iterator it=extend_set.begin(); it!=extend_set.end(); it++)
			{
				Event& ev=**it;
				Event e = ev.clone();
				e.setLenTick(ev.lenTick() + len);
				operations.push_back(UndoOp(UndoOp::ModifyEvent,e, ev, part, false, false));
			}

			if (!shift)
			{
				chord_timer_set_to_tick = tick + step;
				chord_timer->start();
			}
			
			lasttick=tick+len - part->tick();
			goto steprec_record_foot; // this is actually unneccessary, but for clarity
		}
		else // equals if (!held_notes)
		{
			chord_timer->stop();

			// simply proceed, inserting a rest
			Pos p(song->cpos() + step, true);
			song->setPos(0, p, true, false, true);
			
			return;
		}
	}
	
	steprec_record_foot:
	if (!((lasttick > part->lenTick()) && part->hasHiddenNotes())) // allowed?
	{
		if (lasttick > part->lenTick()) // we have to expand the part?
			schedule_resize_all_same_len_clone_parts(part, lasttick, operations);
		
		song->applyOperationGroup(operations);
	}
}
