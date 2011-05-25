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
	       
         if (pitch!=rcSteprecNote) {
					  chord_timer->stop();
					  
            
            //
            // extend len of last note?
            //
            EventList* events = part->events();
            if (ctrl) {
                  for (iEvent i = events->begin(); i != events->end(); ++i) {
                        Event ev = i->second;
                        if (!ev.isNote())
                              continue;
                        if (ev.pitch() == pitch && ((ev.tick() + ev.lenTick()) == tick)) {
                              Event e = ev.clone();
                              e.setLenTick(ev.lenTick() + len);
                              // Indicate do undo, and do not do port controller values and clone parts. 
                              audio->msgChangeEvent(ev, e, part, true, false, false);
                              
                              if (!shift) {
                                    chord_timer_set_to_tick = tick + step;
                                    chord_timer->start();
                                    }
                              return;
                              }
                        }
                  }

            //
            // if we already entered the note, delete it
            //
            EventRange range = events->equal_range(tick);
            for (iEvent i = range.first; i != range.second; ++i) {
                  Event ev = i->second;
                  if (ev.isNote() && ev.pitch() == pitch) {
                        // Indicate do undo, and do not do port controller values and clone parts. 
                        //audio->msgDeleteEvent(ev, part);
                        audio->msgDeleteEvent(ev, part, true, false, false);

                        if (!shift) {
                              chord_timer_set_to_tick = tick + step;
                              chord_timer->start();
                              }
                        
                        return;
                        }
                  }
                  
            Event e(Note);
            e.setTick(tick - part->tick());
            e.setPitch(pitch);
            e.setVelo(velo);
            e.setLenTick(len);
            // Indicate do undo, and do not do port controller values and clone parts. 
            //audio->msgAddEvent(e, part);
            audio->msgAddEvent(e, part, true, false, false);
            
            if (! (globalKeyState & Qt::ShiftModifier)) {
                  chord_timer_set_to_tick = tick + step;
                  chord_timer->start();
                  }
            }
         else { // equals if (pitch==rcSteprecNote)
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
                for (iEvent i = events->begin(); i != events->end(); ++i) {
                      Event& ev = i->second;
                      if (!ev.isNote())
                            continue;

                      if (note_held_down[ev.pitch()] && ((ev.tick() + ev.lenTick()) == tick))
                            extend_set.insert(&ev);
                      }
                for (set<Event*>::iterator it=extend_set.begin(); it!=extend_set.end(); it++)
                {
                    Event& ev=**it;
                    Event e = ev.clone();
                    e.setLenTick(ev.lenTick() + len);
                    // Indicate do undo, and do not do port controller values and clone parts. 
                    audio->msgChangeEvent(ev, e, part, true, false, false);
                }

                if (!shift) {
                      chord_timer_set_to_tick = tick + step;
                      chord_timer->start();
                      }
                return;
               
            }
            else // equals if (!held_notes)
            {
              chord_timer->stop();

              //simply proceed, inserting a rest
              Pos p(song->cpos() + step, true);
              song->setPos(0, p, true, false, true);
            }
            }
}
