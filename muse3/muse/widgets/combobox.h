//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: combobox.h,v 1.3 2004/02/29 12:12:36 wschweer Exp $
//  (C) Copyright 2004 Werner Schweer (ws@seh.de)
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

#ifndef __COMBOBOX_H__
#define __COMBOBOX_H__

#include <QToolButton>

class QMenu;
class QSignalMapper;

namespace MusEGui {

//---------------------------------------------------------
//   ComboBox
//---------------------------------------------------------

class ComboBox : public QToolButton {
      Q_OBJECT

      int _currentItem;
      QList<int> itemlist;

      QMenu* menu;
      virtual void mousePressEvent(QMouseEvent*);
      virtual void wheelEvent(QWheelEvent*);

      QSignalMapper* autoTypeSignalMapper;

   private slots:
      void activatedIntern(int id);

   signals:
      void activated(int id);

   public:
      ComboBox(QWidget* parent = 0, const char* name = 0);
      ~ComboBox();
      void setCurrentItem(int);
      void addAction(const QString& s, int id = -1);
      };

} // namespace MusEGui

#endif



