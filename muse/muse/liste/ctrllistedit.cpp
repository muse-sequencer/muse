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

#include "ctrllistedit.h"
#include "ctrl.h"

//---------------------------------------------------------
//   CtrlListEditor
//---------------------------------------------------------

CtrlListEditor::CtrlListEditor(QWidget* parent)
   : ListWidget(parent)
      {
      QWidget* cew = new QWidget;
      le.setupUi(cew);
      QVBoxLayout* layout = new QVBoxLayout;
      layout->addWidget(cew);
      setLayout(layout);
      le.minValue->setRange(-10000000.0, 100000000.0);
      le.maxValue->setRange(-10000000.0, 100000000.0);
      le.defaultValue->setRange(-10000000.0, 100000000.0);
      le.minValue->setSingleStep(1.0);
      le.maxValue->setSingleStep(1.0);
      le.defaultValue->setSingleStep(1.0);
      }

//---------------------------------------------------------
//   setup
//---------------------------------------------------------

void CtrlListEditor::setup(const ListType& lt)
      {
      Ctrl* c = lt.ctrl;
      le.controllerName->setText(c->name());
      le.discreteCheckBox->setChecked(c->type() & Ctrl::DISCRETE);
      le.logarithmicCheckBox->setChecked(c->type() & Ctrl::LOG);
      le.floatCheckBox->setChecked(!(c->type() & Ctrl::INT));
      if (c->type() & Ctrl::INT) {
            le.minValue->setDecimals(0);
            le.minValue->setValue(c->minVal().i);
            le.maxValue->setDecimals(0);
            le.maxValue->setValue(c->maxVal().i);
            le.defaultValue->setDecimals(0);
            le.defaultValue->setValue(c->getDefault().i);
            }
      else {
            le.minValue->setDecimals(1);
            le.minValue->setValue(c->minVal().f);
            le.maxValue->setDecimals(1);
            le.maxValue->setValue(c->maxVal().f);
            le.defaultValue->setDecimals(1);
            le.defaultValue->setValue(c->getDefault().f);
            }
      le.ctrlList->clear();
      int idx = 0;
      for (iCtrlVal i = c->begin(); i != c->end(); ++i, ++idx) {
            CVal v = i.value();
            QTreeWidgetItem* item = new QTreeWidgetItem;
            item->setText(0, QString("%1").arg(i.key()));
            item->setText(1, QString("%1").arg(i.key()));
            if (c->type() & Ctrl::INT)
                  item->setText(2, QString("%1").arg(v.i));
            else
                  item->setText(2, QString("%1").arg(v.f));
            le.ctrlList->insertTopLevelItem(idx, item);
            }
      }

