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

#ifndef __MIDICMD_H__
#define __MIDICMD_H__

#include "citem.h"

class MidiEditor;
class MidiCmdDialog;

//---------------------------------------------------------
//   MidiCmdDialog
//---------------------------------------------------------

class MidiCmdDialog : public QDialog
      {

   public:
      MidiCmdDialog();
      int range() const { return 0; }
      void setRange(int /*val*/) { }
      };
      
//---------------------------------------------------------
//   MidiCmd
//    abstract base class for midi commands
//---------------------------------------------------------

class MidiCmd {
      int modified;

   protected:
      MidiEditor* editor;
      int range;

      // convenience classes for derived classes
      bool itemInRange(CItem* item);
      void changeEvent(Event oldEvent, Event newEvent, Part* part);

      // virtual functions provided by derived classes
      virtual void process(CItemList*) = 0;
      virtual MidiCmdDialog* guiDialog() { return 0; }

   public:
      MidiCmd(MidiEditor*);
      virtual ~MidiCmd() {}
      void processEvents(CItemList*);
      };


#endif

