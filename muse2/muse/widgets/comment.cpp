//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: comment.cpp,v 1.2 2004/02/08 18:30:00 wschweer Exp $
//  (C) Copyright 2001 Werner Schweer (ws@seh.de)
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

#include "comment.h"
#include "song.h"
#include "track.h"

#include <QWidget>

namespace MusEGui {

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
      setText(textentry->toPlainText());
      }

//---------------------------------------------------------
//   TrackComment
//---------------------------------------------------------

TrackComment::TrackComment(MusECore::Track* t, QWidget* parent)
   : Comment(parent)
      {
      setAttribute(Qt::WA_DeleteOnClose);
      setWindowTitle(tr("MusE: Track Comment"));
      track = t;
      connect(MusEGlobal::song, SIGNAL(songChanged(int)), SLOT(songChanged(int)));
      textentry->setText(track->comment());
      textentry->moveCursor(QTextCursor::End);
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

      // check if track still exists:
      MusECore::TrackList* tl = MusEGlobal::song->tracks();
      MusECore::iTrack it;
      for (it = tl->begin(); it != tl->end(); ++it) {
            if (track == *it)
                  break;
            }
      if (it == tl->end()) {
            close();
            return;
            }
      label2->setText(track->name());
      if (track->comment() != textentry->toPlainText()) {
            disconnect(textentry, SIGNAL(textChanged()), this, SLOT(textChanged()));
            textentry->setText(track->comment());
            textentry->moveCursor(QTextCursor::End);
            connect(textentry, SIGNAL(textChanged()), this, SLOT(textChanged()));
            }
      }

//---------------------------------------------------------
//   setText
//---------------------------------------------------------

void TrackComment::setText(const QString& s)
      {
      track->setComment(s);
      MusEGlobal::song->update(SC_TRACK_MODIFIED);
      }

} // namespace MusEGui
