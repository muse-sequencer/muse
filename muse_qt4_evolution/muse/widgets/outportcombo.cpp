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

#include "outportcombo.h"
#include "song.h"
#include "midioutport.h"

//---------------------------------------------------------
//   OutportCombo
//---------------------------------------------------------

OutportCombo::OutportCombo(QWidget* parent)
   : QComboBox(parent)
      {
      setToolTip(tr("Midi Output Port"));
      populate();
      // midiPort names may change, when inserting/deleting syntis
      connect(song, SIGNAL(trackAdded(Track*,int)), SLOT(populate()));
      connect(song, SIGNAL(trackRemoved(Track*)), SLOT(populate()));
      }

//---------------------------------------------------------
//   populate
//---------------------------------------------------------

void OutportCombo::populate()
      {
      int cur = currentIndex();
      clear();
      MidiOutPortList* mpl = song->midiOutPorts();
      for (iMidiOutPort i = mpl->begin(); i != mpl->end(); ++i)
            addItem((*i)->name());
      setCurrentIndex(cur);
      }

