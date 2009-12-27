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

#include "tracklistedit.h"
#include "track.h"
#include "song.h"
#include "al/pos.h"
#include "awl/posedit.h"

//---------------------------------------------------------
//   TrackListEditor
//---------------------------------------------------------

TrackListEditor::TrackListEditor(ListEdit* e, QWidget* parent)
   : ListWidget(parent)
      {
      listEdit = e;
      QWidget* cew = new QWidget;
      le.setupUi(cew);
      QVBoxLayout* layout = new QVBoxLayout;
      layout->addWidget(cew);
      setLayout(layout);

      QFontMetrics fm(le.partList->font());
      int zW = fm.width("0");
      le.partList->setColumnWidth(TICK_COL, zW * 8);
      le.partList->setColumnWidth(TIME_COL, zW * 14);
      track = 0;
      }

//---------------------------------------------------------
//   setup
//---------------------------------------------------------

void TrackListEditor::setup(const ListType& lt)
      {
      track = lt.track;
      le.trackName->setText(track->name());
      updateList();
      }

//---------------------------------------------------------
//   updateList
//---------------------------------------------------------

void TrackListEditor::updateList()
      {
      }
