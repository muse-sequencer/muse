//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: comment.h,v 1.2 2004/02/08 18:30:00 wschweer Exp $
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

#ifndef __COMMENT_H__
#define __COMMENT_H__

#include "ui_commentbase.h"

class QWidget;


namespace MusECore {
class Xml;
class Track;
}

namespace MusEGui {

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
      Q_OBJECT
    
      MusECore::Track* track;
      

   private:
      virtual void setText(const QString& s);

   private slots:
      void songChanged(int);

   public:
      TrackComment(MusECore::Track*, QWidget*);
      };

} // namespace MusEGui

#endif

