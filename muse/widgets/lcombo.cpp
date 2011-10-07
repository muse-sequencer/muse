//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: lcombo.cpp,v 1.1.1.1.2.3 2009/07/01 22:14:56 spamatica Exp $
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

#include "lcombo.h"

#include <QHBoxLayout>
#include <QLabel>

namespace MusEGui {

//---------------------------------------------------------
//   LabelCombo
//---------------------------------------------------------

LabelCombo::LabelCombo(const QString& txt, QWidget* parent,
   const char* name) : QWidget(parent)
      {
      setObjectName(name);
//      setFixedHeight(20);
      QHBoxLayout* layout = new QHBoxLayout(this);
      QLabel* label = new QLabel(txt, this);
      //box = new QComboBox(false, this);
      box = new QComboBox(this);
      box->setEditable(false);
      ///layout->addStretch();
      layout->addSpacing(5);
      layout->addWidget(label);
      layout->addSpacing(5);
      layout->addWidget(box);
      layout->addSpacing(5);
      ///layout->addStretch();
      connect(box, SIGNAL(activated(int)), SIGNAL(activated(int)));
      }

void LabelCombo::setCurrentIndex(int i) 
{ 
  int rc = box->model()->rowCount();
  if(rc == 0)
    return;
  int r = i % rc;
  int c = i / rc;
  if(c >= box->model()->columnCount())
    return;
  if(box->modelColumn() != c)
    box->setModelColumn(c);
  if(box->currentIndex() != r)  
    box->setCurrentIndex(r); 
} 

} // namespace MusEGui
