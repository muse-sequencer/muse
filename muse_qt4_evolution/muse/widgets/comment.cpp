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

#include "comment.h"
#include "song.h"
#include "track.h"

//---------------------------------------------------------
//   Comment
//---------------------------------------------------------

Comment::Comment(QWidget* parent)
   : QWidget(parent)
      {
      setupUi(this);
      }

//---------------------------------------------------------
//   textChanged
//---------------------------------------------------------

void Comment::textChanged()
      {
      setText(textentry->document()->toPlainText());
      }

//---------------------------------------------------------
//   TrackComment
//---------------------------------------------------------

TrackComment::TrackComment(Track* t, QWidget* parent)
   : Comment(parent)
      {
      setWindowTitle(tr("MusE: Track Comment"));
      track = t;
      connect(song, SIGNAL(songChanged(int)), SLOT(songChanged(int)));
      textentry->setPlainText(track->comment());
      connect(textentry, SIGNAL(textChanged()), SLOT(textChanged()));
      label1->setText(tr("Track Comment:"));
      label2->setText(track->name());
      }

//---------------------------------------------------------
//   songChanged
//---------------------------------------------------------

void TrackComment::songChanged(int flags)
      {
      if ((flags & (SC_TRACK_INSERTED|SC_TRACK_REMOVED|SC_TRACK_MODIFIED)) == 0)
            return;
      if (!song->trackExists(track)) {
            close();
            return;
            }
      label2->setText(track->name());
      QString txt(textentry->document()->toPlainText());
      if (track->comment() != txt) {
            disconnect(textentry, SIGNAL(textChanged()), this, SLOT(textChanged()));
            textentry->setPlainText(track->comment());
//            textentry->setCursorPosition(1000, 1000);
            connect(textentry, SIGNAL(textChanged()), this, SLOT(textChanged()));
            }
      }

//---------------------------------------------------------
//   setText
//---------------------------------------------------------

void TrackComment::setText(const QString& s)
      {
      track->setComment(s);
      song->update(SC_TRACK_MODIFIED);
      }

