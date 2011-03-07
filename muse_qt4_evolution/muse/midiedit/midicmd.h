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
#include "ui_midicmd.h"

class MidiEditor;
class MidiCmdDialog;

//---------------------------------------------------------
//   MidiCmdDialog
//    GUI base class for midiCmd
//---------------------------------------------------------

class MidiCmdDialog : public QDialog
      {
      Q_OBJECT

      QButtonGroup* rangeGroup;
      int _range;
      Ui::MidiCmd mc;

   protected:
      void accept();
      QVBoxLayout* layout;

   public:
      MidiCmdDialog();
      int range() const { return _range; }
      void setRange(int);
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

