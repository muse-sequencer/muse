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

#ifndef __SELECTFILTER_H__
#define __SELECTFILTER_H__

//---------------------------------------------------------
//   SelectFilter
//---------------------------------------------------------

class SelectFilter :public QObject
      {
      QWidget* w;
      Track* t;

   protected:
      bool eventFilter(QObject*, QEvent* ev)
            {
            if (ev->type() == QEvent::MouseButtonPress) {
                  w->setFocus();
                  song->selectTrack(t);
                  }
            return false;
            }
   public:
      SelectFilter(QObject* parent, QWidget* widget, Track* track)
         : QObject(parent) {
            w = widget;
            t = track;
            }
      };

#endif

