//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: lcombo.h,v 1.1.1.1.2.3 2009/07/01 22:14:56 spamatica Exp $
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

#ifndef __LCOMBO_H__
#define __LCOMBO_H__

#include <QWidget>
#include <QComboBox>
#include <QVariant>

class QAbstractItemView;
class QString;
class QModelIndex;

namespace MusEGui {

//---------------------------------------------------------
//   LabelCombo
//---------------------------------------------------------

class LabelCombo : public QWidget {
      Q_OBJECT
      QComboBox* box;
      

   signals:
      void activated(int);
      void activated(const QModelIndex&);

   private slots:
      void box_activated(int idx);
     
   public slots:
      void clearFocus();
      void setCurrentIndex(int i);
      void setCurrentModelIndex(const QModelIndex& mdl_idx);

   public:
      LabelCombo(const QString& label, QWidget* parent,
         const char* name=0);
      void addItem(const QString& txt, const QVariant & userData = QVariant());
      void insertItem(int index, const QString& txt, const QVariant & userData = QVariant());

      QAbstractItemView *view() const;

      void setView(QAbstractItemView* v);
      void setFocusPolicy ( Qt::FocusPolicy fp );

      QVariant itemData(int index, int role = Qt::UserRole) const;
      int findData(
        const QVariant &data, int role = Qt::UserRole,
        Qt::MatchFlags flags = static_cast<Qt::MatchFlags>(Qt::MatchExactly|Qt::MatchCaseSensitive)) const;
      int maxVisibleItems() const;
      void setMaxVisibleItems(int maxItems);
      QComboBox::SizeAdjustPolicy sizeAdjustPolicy() const;
      void setSizeAdjustPolicy(QComboBox::SizeAdjustPolicy policy);

      int currentIndex() const;
      QModelIndex currentModelIndex() const;
      };

} // namespace MusEGui

#endif
