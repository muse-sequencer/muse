//=========================================================
//  MusE
//  Linux Music Editor
//  steprec.h
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

#ifndef __STEPREC_H__
#define __STEPREC_H__

#include <QObject>
#include <QTimer>

#include "part.h"

namespace MusECore {

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
		unsigned int chord_timer_set_to_tick; 
		bool* note_held_down;
};

} // namespace MusECore

#endif
