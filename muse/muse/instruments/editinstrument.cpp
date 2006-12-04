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

#include "editinstrument.h"
#include "minstrument.h"
#include "ctrl.h"
#include "midictrl.h"

//---------------------------------------------------------
//   EditInstrument
//---------------------------------------------------------

EditInstrument::EditInstrument(QWidget* parent)
   : QMainWindow(parent)
      {
      setupUi(this);
      // populate instrument list
      for (iMidiInstrument i = midiInstruments.begin(); i != midiInstruments.end(); ++i) {
            QListWidgetItem* item = new QListWidgetItem((*i)->iname());
            QVariant v = qVariantFromValue((void*)(*i));
            item->setData(Qt::UserRole, v);
            instrumentList->addItem(item);
            }
      instrumentList->setItemSelected(instrumentList->item(0), true);
      connect(instrumentList, SIGNAL(currentItemChanged(QListWidgetItem*,QListWidgetItem*)),
         SLOT(instrumentChanged(QListWidgetItem*)));
      connect(patchView, SIGNAL(currentItemChanged(QTreeWidgetItem*,QTreeWidgetItem*)),
         SLOT(patchChanged(QTreeWidgetItem*)));
      instrumentChanged(instrumentList->item(0));
      connect(listController, SIGNAL(currentItemChanged(QListWidgetItem*,QListWidgetItem*)),
         SLOT(controllerChanged(QListWidgetItem*)));
      connect(instrumentName, SIGNAL(textChanged(const QString&)), SLOT(instrumentNameChanged(const QString&)));
      connect(fileSaveAsAction, SIGNAL(triggered()), SLOT(fileSaveAs()));
      connect(fileSaveAction, SIGNAL(triggered()), SLOT(fileSave()));
      }

//---------------------------------------------------------
//   instrumentChanged
//---------------------------------------------------------

void EditInstrument::instrumentChanged(QListWidgetItem* sel)
      {
      patchView->clear();
      listController->clear();

      if (sel == 0)
            return;
      // populate patch list

      MidiInstrument* instrument = (MidiInstrument*)sel->data(Qt::UserRole).value<void*>();
      instrumentName->setText(instrument->iname());
      std::vector<PatchGroup>* pg = instrument->groups();
      for (std::vector<PatchGroup>::iterator g = pg->begin(); g != pg->end(); ++g) {
            QTreeWidgetItem* item = new QTreeWidgetItem;
            item->setText(0, g->name);
            QVariant v = QVariant::fromValue((void*)0);
            item->setData(0, Qt::UserRole, v);
            patchView->addTopLevelItem(item);
            for (iPatch p = g->patches.begin(); p != g->patches.end(); ++p) {
                  Patch* patch = *p;
                  QTreeWidgetItem* sitem = new QTreeWidgetItem;
                  sitem->setText(0, patch->name);
                  QVariant v = QVariant::fromValue((void*)(patch));
                  sitem->setData(0, Qt::UserRole, v);
                  item->addChild(sitem);
                  }
            }
      MidiControllerList* cl = instrument->controller();
      for (iMidiController ic = cl->begin(); ic != cl->end(); ++ic) {
            MidiController* c = *ic;
            QListWidgetItem* item = new QListWidgetItem(c->name());
            QVariant v = qVariantFromValue((void*)(c));
            item->setData(Qt::UserRole, v);
            listController->addItem(item);
            }
      listController->setItemSelected(listController->item(0), true);
      controllerChanged(listController->item(0));
      }

//---------------------------------------------------------
//   controllerChanged
//---------------------------------------------------------

void EditInstrument::controllerChanged(QListWidgetItem* sel)
      {
      if (sel == 0 || sel->data(Qt::UserRole).value<void*>() == 0) {
            // patchNameEdit->setText("");
            return;
            }
      MidiController* c = (MidiController*)sel->data(Qt::UserRole).value<void*>();
      entryName->setText(c->name());
      MidiController::ControllerType type = c->type();
      switch(type) {
            case MidiController::Controller7:
                  spinBoxHCtrlNo->setEnabled(false);
                  break;
            case MidiController::Controller14:
            case MidiController::RPN:
            case MidiController::NRPN:
            case MidiController::RPN14:
            case MidiController::NRPN14:
                  spinBoxHCtrlNo->setEnabled(true);
                  break;
            case MidiController::Pitch:
            case MidiController::Program:
            case MidiController::Velo:
                  break;
            }

      int ctrlH = (c->num() >> 8) & 0x7f;
      int ctrlL = c->num() & 0x7f;
      spinBoxType->setCurrentIndex(int(type));
      spinBoxHCtrlNo->setValue(ctrlH);
      spinBoxLCtrlNo->setValue(ctrlL);
      spinBoxMin->setValue(c->minVal());
      spinBoxMax->setValue(c->maxVal());
      spinBoxDefault->setValue(c->initVal());
      }

//---------------------------------------------------------
//   patchChanged
//---------------------------------------------------------

void EditInstrument::patchChanged(QTreeWidgetItem* sel)
      {
      if (sel == 0 || sel->data(0, Qt::UserRole).value<void*>() == 0) {
            patchNameEdit->setText("");
            return;
            }
      Patch* p = (Patch*)sel->data(0, Qt::UserRole).value<void*>();
      patchNameEdit->setText(p->name);
      spinBoxHBank->setValue(p->hbank);
      spinBoxLBank->setValue(p->lbank);
      spinBoxProgram->setValue(p->prog);
      checkBoxDrum->setChecked(p->drumMap);
      checkBoxGM->setChecked(p->typ & 1);
      checkBoxGS->setChecked(p->typ & 2);
      checkBoxXG->setChecked(p->typ & 4);
      }

//---------------------------------------------------------
//   fileNew
//---------------------------------------------------------

void EditInstrument::fileNew()
      {

      }

//---------------------------------------------------------
//   fileSave
//---------------------------------------------------------

void EditInstrument::fileSave()
      {

      }

//---------------------------------------------------------
//   fileSaveAs
//---------------------------------------------------------

void EditInstrument::fileSaveAs()
      {
      QListWidgetItem* item = instrumentList->currentItem();
      if (item == 0)
            return;
      MidiInstrument* instrument = (MidiInstrument*)item->data(Qt::UserRole).value<void*>();
      QString path = QDir::homePath() + "/MusE/instruments";
      path += QString("/%1.idf").arg(instrument->iname());
      QString s = QFileDialog::getSaveFileName(this,
         tr("MusE: Save Instrument Definition"),
         path,
         tr("Instrument Definition (*.idf)"));
      }

//---------------------------------------------------------
//   fileExit
//---------------------------------------------------------

void EditInstrument::fileExit()
      {
      }

//---------------------------------------------------------
//   instrumentNameChanged
//---------------------------------------------------------

void EditInstrument::instrumentNameChanged(const QString& s)
      {
      QListWidgetItem* item = instrumentList->currentItem();
      if (item == 0)
            return;
      if (s != item->text()) {
            item->setText(s);
            MidiInstrument* instrument = (MidiInstrument*)item->data(Qt::UserRole).value<void*>();
            instrument->setDirty(true);
            }
      }

