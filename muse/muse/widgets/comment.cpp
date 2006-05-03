//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: comment.cpp,v 1.5 2005/11/04 12:03:48 wschweer Exp $
//  (C) Copyright 2001 Werner Schweer (ws@seh.de)
//=========================================================

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

