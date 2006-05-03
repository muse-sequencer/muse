//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: outportcombo.cpp,v 1.3 2005/11/04 12:03:48 wschweer Exp $
//  (C) Copyright 2005 Werner Schweer
//=========================================================

#include "outportcombo.h"
#include "midiport.h"
#include "song.h"

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

