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

#ifndef __ACTION_H__
#define __ACTION_H__

//---------------------------------------------------------
//   Action
//---------------------------------------------------------

class Action : public QAction {
      int _id;

   public:
      Action(QObject* parent, int i, const char* name = 0, bool toggle = false)
         : QAction(parent, name, toggle) {
            _id = i;
            }
      void setId(int i) { _id = i; }
      int id() const    { return _id; }
      };


#endif

