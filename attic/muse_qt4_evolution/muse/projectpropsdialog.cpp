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

#include "projectpropsdialog.h"
#include "song.h"

#include "al/al.h"
#include "al/tempo.h"

//---------------------------------------------------------
//   ProjectPropsDialog
//---------------------------------------------------------

ProjectPropsDialog::ProjectPropsDialog(QWidget* parent)
   : QDialog(parent)
      {
      setupUi(this);
      comment->setPlainText(song->comment());
      projectName->setText(song->projectName());
      createdDate->setDateTime(song->createDate());
      int n = AL::tempomap.tick2frame(song->len()) / AL::sampleRate;
      QTime time;
      time = time.addSecs(n);
      length->setTime(time);
      }

//---------------------------------------------------------
//   accept
//---------------------------------------------------------

void ProjectPropsDialog::accept()
      {
      song->setComment(comment->toPlainText());
      song->dirty = true;
      QDialog::accept();
      }

//---------------------------------------------------------
//   reject
//---------------------------------------------------------

void ProjectPropsDialog::reject()
      {
      comment->setPlainText(song->comment());
      QDialog::reject();
      }

