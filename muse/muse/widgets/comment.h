//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: comment.h,v 1.5 2005/11/04 12:03:48 wschweer Exp $
//  (C) Copyright 2001 Werner Schweer (ws@seh.de)
//=========================================================

#ifndef __COMMENT_H__
#define __COMMENT_H__

#include "ui_comment.h"

namespace AL {
      class Xml;
      };
using AL::Xml;

class Track;

//---------------------------------------------------------
//   Comment
//---------------------------------------------------------

class Comment : public QWidget, public Ui::CommentBase {
      Q_OBJECT

   private:
      virtual void setText(const QString& s) = 0;

   private slots:
      void textChanged();

   public:
      Comment(QWidget* parent);
      };

//---------------------------------------------------------
//   TrackComment
//---------------------------------------------------------

class TrackComment : public Comment {
      Track* track;
      Q_OBJECT

   private:
      virtual void setText(const QString& s);

   private slots:
      void songChanged(int);

   public:
      TrackComment(Track*, QWidget*);
      };

#endif

