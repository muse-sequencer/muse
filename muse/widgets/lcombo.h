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

#include <QAbstractItemView>
#include <QComboBox>
#include <QVariant>

class QString;

namespace MusEGui {

//---------------------------------------------------------
//   LabelCombo
//---------------------------------------------------------

class LabelCombo : public QWidget {
      Q_OBJECT
      QComboBox* box;
      

   signals:
      void activated(int);

   public slots:
      void clearFocus() { box->clearFocus(); }
      void setCurrentIndex(int i);

   public:
      LabelCombo(const QString& label, QWidget* parent,
         const char* name=0);
      void addItem(const QString& txt, const QVariant & userData = QVariant()) { box->addItem(txt, userData); }
      void insertItem(int index, const QString& txt, const QVariant & userData = QVariant()) { box->insertItem(index, txt, userData); }
      //void setListBox(Q3ListBox* lb) { box->setListBox(lb); } // ddskrjo
      void setView(QAbstractItemView* v) { box->setModel(v->model()); box->setView(v); } // p4.0.3
      void setFocusPolicy ( Qt::FocusPolicy fp ) { box->setFocusPolicy(fp); }
      };

} // namespace MusEGui

#endif
