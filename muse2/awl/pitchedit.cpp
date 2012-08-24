//=============================================================================
//  Awl
//  Audio Widget Library
//  $Id:$
//
//  Copyright (C) 1999-2011 by Werner Schweer and others
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License
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
//=============================================================================

#include <stdio.h>

#include "pitchedit.h"
#include "utils.h"

#include <QKeyEvent>

namespace MusEGlobal {
extern QObject* song; // TODO FINDME this is a really dirty hack!
}

namespace Awl {

//---------------------------------------------------------
//   PitchEdit
//---------------------------------------------------------

PitchEdit::PitchEdit(QWidget* parent)
  : QSpinBox(parent)
      {
      setRange(0, 127);
      deltaMode = false;
      connect(MusEGlobal::song, SIGNAL(midiNote(int, int)), SLOT(midiNote(int,int)));
      }

//---------------------------------------------------------
//   keyPressEvent
//---------------------------------------------------------

void PitchEdit::keyPressEvent(QKeyEvent* ev)
      {
      if (ev->key() == Qt::Key_Return)
            emit returnPressed();
      else if (ev->key() == Qt::Key_Escape)
            emit escapePressed();
      }

//---------------------------------------------------------
//   mapValueToText
//---------------------------------------------------------

QString PitchEdit::textFromValue(int v) const
      {
      if (deltaMode) {
            QString s;
            s.setNum(v);
            return s;
            }
      else
            return pitch2string(v);
      }

//---------------------------------------------------------
//   mapTextToValue
//---------------------------------------------------------

int PitchEdit::valueFromText(bool* ok) const
      {
printf("AwlPitchEdit: mapTextToValue: not impl.\n");
      if (ok)
            *ok = false;
      return 0;
      }

//---------------------------------------------------------
//   setDeltaMode
//---------------------------------------------------------

void PitchEdit::setDeltaMode(bool val)
      {
      deltaMode = val;
      if (deltaMode)
            setRange(-127, 127);
      else
            setRange(0, 127);
      }

void PitchEdit::midiNote(int pitch, int velo)
{
	if (hasFocus() && velo)
	  setValue(pitch);
}


}


