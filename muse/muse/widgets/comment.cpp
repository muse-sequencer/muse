//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: comment.cpp,v 1.2 2004/02/08 18:30:00 wschweer Exp $
//  (C) Copyright 2001 Werner Schweer (ws@seh.de)
//=========================================================

#include "comment.h"
#include "song.h"
#include "track.h"
#include <qmultilineedit.h>
#include <qlabel.h>

//---------------------------------------------------------
//   Comment
//---------------------------------------------------------

Comment::Comment(QWidget* parent, const char* name)
   : CommentBase(parent, name)
      {
      }

//---------------------------------------------------------
//   textChanged
//---------------------------------------------------------

void Comment::textChanged()
      {
      setText(textentry->text());
      }

//---------------------------------------------------------
//   TrackComment
//---------------------------------------------------------

TrackComment::TrackComment(Track* t, QWidget* parent, const char* name)
   : Comment(parent, name)
      {
      setCaption(tr("MusE: Track Comment"));
      track = t;
      connect(song, SIGNAL(songChanged(int)), SLOT(songChanged(int)));
      textentry->setText(track->comment());
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
      TrackList* tl = song->tracks();
      iTrack it;
      for (it = tl->begin(); it != tl->end(); ++it) {
            if (track == *it)
                  break;
            }
      if (it == tl->end()) {
            close(true);
            return;
            }
      label2->setText(track->name());
      if (track->comment() != textentry->text()) {
            disconnect(textentry, SIGNAL(textChanged()), this, SLOT(textChanged()));
            textentry->setText(track->comment());
            textentry->setCursorPosition(1000, 1000);
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

