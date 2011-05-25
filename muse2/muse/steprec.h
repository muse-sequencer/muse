//=========================================================
//  MusE
//  Linux Music Editor
//  steprec.h
//  (C) Copyright 2011 Florian Jung (flo93@users.sourceforge.net)
//=========================================================

#ifndef __STEPREC_H__
#define __STEPREC_H__

#include <QObject>
#include <QTimer>

#include "part.h"


class StepRec : public QObject
{
	Q_OBJECT

	public:
		StepRec(bool* note_held_down_array);
		
		void record(Part* part,  int pitch, int len, int step, int velo=80, bool ctrl=false, bool shift=false);
	
	private slots:
	  void timeout();
	
	private:
		QTimer* chord_timer;
		int chord_timer_set_to_tick;
		bool* note_held_down;
		
};

#endif
