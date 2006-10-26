//=============================================================================
//  MusE
//  Linux Music Editor
//  $Id:$
//
//  Copyright (C) 2002-2006 by Werner Schweer and others
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License version 2.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//=============================================================================

#ifndef __SYNC_H__
#define __SYNC_H__

#include "mtc.h"

extern bool debugSync;
extern MTC mtcOffset;
extern bool extSyncFlag;
extern bool genMTCSync;       // output MTC Sync
extern bool genMCSync;        // output MidiClock Sync
extern bool genMMC;           // output Midi Machine Control
extern bool acceptMTC;
extern bool acceptMC;
extern bool acceptMMC;

#endif

