//=========================================================
//  MusE
//  Linux Music Editor
//  steprec.cpp
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

#include "steprec.h"
#include "part.h"
#include "event.h"
#include "globals.h"
#include "functions.h"

#include "song.h"
#include "audio.h"

#include <set>

#define CHORD_TIMEOUT 75

namespace MusECore {

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
	if (chord_timer_set_to_tick != MusEGlobal::song->cpos())
	{
		Pos p(chord_timer_set_to_tick, true);
    MusEGlobal::song->setPos(MusECore::Song::CPOS, p, true, false, true);
	}
}

void StepRec::moveon(int step)
{
    unsigned tick = MusEGlobal::song->cpos();

    chord_timer->stop();
    chord_timer_set_to_tick = tick + step;
    chord_timer->start();
}

void StepRec::record(const Part* part, int pitch, int len, int step, int velo, bool ctrl, bool shift, int incoming_pitch)
{
	unsigned tick = MusEGlobal::song->cpos();
	unsigned lasttick=0;
	Undo operations;
	
	if (tick < part->tick()) //insert note before the part to insert?
	{
		if (MusEGlobal::debugMsg)
		printf("StepRec::record(): tick (%i) is before part (begin tick is %i), ignoring...\n",tick, part->tick());
		return;
	}
	
	// if incoming_pitch wasn't specified, set it to pitch
    if (incoming_pitch == 1337)
        incoming_pitch=pitch;
	
    if (incoming_pitch != MusEGlobal::rcSteprecNote)
	{
		chord_timer->stop();

		// extend len of last note?
		const EventList& events = part->events();
		if (ctrl)
		{
			for (ciEvent i = events.begin(); i != events.end(); ++i)
			{
				const Event& ev = i->second;
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
			// avoids this. this has to be avoided because then part->hasHiddenEvents() is true
			// which results in forbidding any action beyond its end
			         cEventRange range = events.equal_range(tick - part->tick());
			for (ciEvent i = range.first; i != range.second; ++i)
			{
				const Event& ev = i->second;
				if (ev.isNote() && ev.pitch() == pitch)
				{
					MusEGlobal::song->applyOperation(UndoOp(UndoOp::DeleteEvent,ev, part, true,true));

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

		if (! (MusEGlobal::globalKeyState & Qt::ShiftModifier))
		{
			chord_timer_set_to_tick = tick + step;
			chord_timer->start();
		}
		
		goto steprec_record_foot; // this is actually unnecessary, but for clarity
	}
	else  // equals if (incoming_pitch==MusEGlobal::rcSteprecNote)
	{
		bool held_notes=false;
        if (note_held_down != nullptr)
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
			
			set<const Event*> extend_set;
			const EventList& events = part->events();
			for (ciEvent i = events.begin(); i != events.end(); ++i)
			{
				const Event& ev = i->second;
				if (ev.isNote() && note_held_down[ev.pitch()] && ((ev.tick() + ev.lenTick() + part->tick()) == tick))
					extend_set.insert(&ev);
			}
			
			for (set<const Event*>::iterator it=extend_set.begin(); it!=extend_set.end(); it++)
			{
				const Event& ev=**it;
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
			goto steprec_record_foot; // this is actually unnecessary, but for clarity
		}
		else // equals if (!held_notes)
		{
			chord_timer->stop();

			// simply proceed, inserting a rest
			Pos p(MusEGlobal::song->cpos() + step, true);
            MusEGlobal::song->setPos(MusECore::Song::CPOS, p, true, false, true);
			
			return;
		}
	}
	
	steprec_record_foot:
	if (!((lasttick > part->lenTick()) && (part->hasHiddenEvents() & Part::RightEventsHidden))) // allowed?
	{
		if (lasttick > part->lenTick()) // we have to expand the part?
			schedule_resize_all_same_len_clone_parts(part, lasttick, operations);
		
		MusEGlobal::song->applyOperationGroup(operations);
	}
}

} // namespace MusECore
