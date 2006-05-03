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

#ifndef __COMBOBOX_H__
#define __COMBOBOX_H__

class Q3PopupMenu;

//---------------------------------------------------------
//   ComboBox
//---------------------------------------------------------

class ComboBox : public QLabel {
      Q_OBJECT
      Q_PROPERTY(int id READ id WRITE setId)

      int _id;
      int _currentItem;
      Q3PopupMenu* list;
      virtual void mousePressEvent(QMouseEvent*);

   private slots:
      void activatedIntern(int);

   signals:
      void activated(int val, int id);

   public:
      ComboBox(QWidget* parent, const char* name = 0);
      ~ComboBox();
      void setCurrentItem(int);
      void insertItem(const QString& s, int id = -1, int idx=-1);
      int id() const       { return _id; }
      void setId(int i)    { _id = i; }
      };

#endif



