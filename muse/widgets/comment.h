//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: comment.h,v 1.2 2004/02/08 18:30:00 wschweer Exp $
//  (C) Copyright 2001 Werner Schweer (ws@seh.de)
//=========================================================

#ifndef __COMMENT_H__
#define __COMMENT_H__

#include "ui_commentbase.h"

class Xml;
class Track;
class QWidget;

//---------------------------------------------------------
//   CommentBaseWidget
//   Wrapper around Ui::CommentBase
//---------------------------------------------------------

class CommentBaseWidget : public QWidget, public Ui::CommentBase
{
     Q_OBJECT

  public:
     CommentBaseWidget(QWidget *parent = 0) : QWidget(parent) { setupUi(this); }
};

//---------------------------------------------------------
//   Comment
//---------------------------------------------------------

class Comment : public CommentBaseWidget {
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

