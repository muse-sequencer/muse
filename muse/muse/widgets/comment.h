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

