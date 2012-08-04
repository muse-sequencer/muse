//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: header.h,v 1.1.1.1 2003/10/27 18:55:03 wschweer Exp $
//  (C) Copyright 2000 Werner Schweer (ws@seh.de)
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

#ifndef __HEADER_H__
#define __HEADER_H__

#include <QHeaderView>
#include <QAction>
#include <QString>

class QStandardItemModel;

namespace MusECore {
class Xml;
}

namespace MusEGui {

class Header : public QHeaderView {
      Q_OBJECT

      QStandardItemModel *itemModel;

   public:
      Header(QWidget* parent=0, const char* name=0);
      void writeStatus(int level, MusECore::Xml&) const;
      void readStatus(MusECore::Xml&);
      void setColumnLabel( const QString & s, int col, int width = -1 );
      void setToolTip(int col, const QString &text);
      void setWhatsThis(int col, const QString &text);
      void mousePressEvent ( QMouseEvent * e );
    private slots:
      void changeColumns(QAction* a);
};

} // namespace MusEGui

#endif

