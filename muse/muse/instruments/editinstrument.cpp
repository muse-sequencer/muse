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
#include "al/xml.h"
#include "gconfig.h"

extern int string2sysex(const QString& s, unsigned char** data);
extern QString sysex2string(int len, unsigned char* data);

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
         SLOT(instrumentChanged(QListWidgetItem*,QListWidgetItem*)));
      connect(patchView, SIGNAL(currentItemChanged(QTreeWidgetItem*,QTreeWidgetItem*)),
         SLOT(patchChanged(QTreeWidgetItem*, QTreeWidgetItem*)));
      instrumentChanged(instrumentList->item(0), instrumentList->item(0));
      connect(listController, SIGNAL(currentItemChanged(QListWidgetItem*,QListWidgetItem*)),
         SLOT(controllerChanged(QListWidgetItem*, QListWidgetItem*)));
      connect(sysexList, SIGNAL(currentItemChanged(QListWidgetItem*,QListWidgetItem*)),
         SLOT(sysexChanged(QListWidgetItem*, QListWidgetItem*)));
      connect(instrumentName, SIGNAL(textChanged(const QString&)), SLOT(instrumentNameChanged(const QString&)));
      connect(fileSaveAsAction, SIGNAL(triggered()), SLOT(fileSaveAs()));
      connect(fileSaveAction, SIGNAL(triggered()), SLOT(fileSave()));
      connect(fileNewAction, SIGNAL(triggered()), SLOT(fileNew()));

      connect(deletePatch, SIGNAL(clicked()), SLOT(deletePatchClicked()));
      connect(newPatch, SIGNAL(clicked()), SLOT(newPatchClicked()));
      connect(newGroup, SIGNAL(clicked()), SLOT(newGroupClicked()));
      connect(newCategory, SIGNAL(clicked()), SLOT(newCategoryClicked()));
      connect(deleteController, SIGNAL(clicked()), SLOT(deleteControllerClicked()));
      connect(newController, SIGNAL(clicked()), SLOT(newControllerClicked()));
      connect(deleteSysex, SIGNAL(clicked()), SLOT(deleteSysexClicked()));
      connect(newSysex, SIGNAL(clicked()), SLOT(newSysexClicked()));

      connect(ctrlType,SIGNAL(activated(int)), SLOT(ctrlTypeChanged(int)));
      }

//---------------------------------------------------------
//   fileNew
//---------------------------------------------------------

void EditInstrument::fileNew()
      {
      for (int i = 1;; ++i) {
            QString s = QString("Instrument-%1").arg(i);
            bool found = false;
            for (iMidiInstrument i = midiInstruments.begin(); i != midiInstruments.end(); ++i) {
                  if (s == (*i)->iname()) {
                        found = true;
                        break;
                        }
                  }
            if (!found) {
                  MidiInstrument* ni = new MidiInstrument(s);
                  midiInstruments.append(ni);
                  QListWidgetItem* item = new QListWidgetItem(ni->iname());
                  QVariant v = qVariantFromValue((void*)(ni));
                  item->setData(Qt::UserRole, v);
                  instrumentList->addItem(item);
                  instrumentList->setCurrentItem(item);
                  break;
                  }
            }
      }

//---------------------------------------------------------
//   fileSave
//---------------------------------------------------------

void EditInstrument::fileSave()
      {
      QListWidgetItem* item = instrumentList->currentItem();
      if (item == 0)
            return;
      MidiInstrument* instrument = (MidiInstrument*)item->data(Qt::UserRole).value<void*>();
      if (instrument->filePath().isEmpty())
            fileSaveAs();
      else {
            QFile f(instrument->filePath());
            if (!f.open(QIODevice::WriteOnly))
                  fileSaveAs();
            else {
                  f.close();
                  if (fileSave(instrument, instrument->filePath()))
                        instrument->setDirty(false);
                  }
            }
      }

//---------------------------------------------------------
//   fileSave
//---------------------------------------------------------

bool EditInstrument::fileSave(MidiInstrument* instrument, const QString& name)
      {
      QFile f(name);
      if (!f.open(QIODevice::WriteOnly)) {
            QString s("Creating file failed: ");
            s += strerror(errno);
            QMessageBox::critical(this,
               tr("MusE: Create file failed"), s);
            return false;
            }
      Xml xml(&f);
      instrument->write(xml);
      f.close();
      if (f.error()) {
            QString s = QString("Write File\n") + f.fileName() + QString("\nfailed: ")
               + f.errorString();
            QMessageBox::critical(this, tr("MusE: Write File failed"), s);
            return false;
            }
      return true;
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
      QString path = QDir::homePath() + "/" + config.instrumentPath;
      path += QString("/%1.idf").arg(instrument->iname());
      QString s = QFileDialog::getSaveFileName(this,
         tr("MusE: Save Instrument Definition"),
         path,
         tr("Instrument Definition (*.idf)"));
      if (s.isEmpty())
            return;
      instrument->setFilePath(s);
      if (fileSave(instrument, s))
            instrument->setDirty(false);
      }

//---------------------------------------------------------
//   closeEvent
//---------------------------------------------------------

void EditInstrument::closeEvent(QCloseEvent* ev)
      {
      QListWidgetItem* item = instrumentList->currentItem();
      if (item) {
            MidiInstrument* instrument = (MidiInstrument*)item->data(Qt::UserRole).value<void*>();
            if (checkDirty(instrument)) {
                  ev->ignore();
                  return;
                  }
            }
      QMainWindow::closeEvent(ev);
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

//---------------------------------------------------------
//   deletePatchClicked
//---------------------------------------------------------

void EditInstrument::deletePatchClicked()
      {
      QListWidgetItem* item = instrumentList->currentItem();
      if (item == 0)
            return;
      MidiInstrument* instrument = (MidiInstrument*)item->data(Qt::UserRole).value<void*>();
      QTreeWidgetItem* pi = patchView->currentItem();
      if (pi == 0)
            return;
      void* p = pi->data(0, Qt::UserRole).value<void*>();
      if (p == 0)
            return;
      Patch* patch = (Patch*)p;
      std::vector<PatchGroup>* pg = instrument->groups();
      for (std::vector<PatchGroup>::iterator g = pg->begin(); g != pg->end(); ++g) {
            for (iPatch p = g->patches.begin(); p != g->patches.end(); ++p) {
                  if (patch == *p) {
                        g->patches.erase(p);
                        delete pi;
                        instrument->setDirty(true);
                        return;
                        }
                  }
            }
      printf("fatal: patch not found\n");      
      }

//---------------------------------------------------------
//   newPatchClicked
//---------------------------------------------------------

void EditInstrument::newPatchClicked()
      {
      QListWidgetItem* item = instrumentList->currentItem();
      if (item == 0)
            return;
      MidiInstrument* instrument = (MidiInstrument*)item->data(Qt::UserRole).value<void*>();
      std::vector<PatchGroup>* pg = instrument->groups();
      QString patchName;
      for (int i = 1;; ++i) {
            patchName = QString("Patch-%1").arg(i);
            bool found = false;

            for (std::vector<PatchGroup>::iterator g = pg->begin(); g != pg->end(); ++g) {
                  for (iPatch p = g->patches.begin(); p != g->patches.end(); ++p) {
                        if ((*p)->name == patchName) {
                              found = true;
                              break;
                              }
                        }
                  if (found)
                        break;
                  }
            if (!found)
                  break;
            }

      //
      // search current patch group
      //
      PatchGroup* pGroup = 0;
      QTreeWidgetItem* pi = patchView->currentItem();
      if (pi == 0)
            return;
      if (pi->data(0, Qt::UserRole).value<void*>())
            pi = pi->parent();
      for (std::vector<PatchGroup>::iterator g = pg->begin(); g != pg->end(); ++g) {
            if (g->name == pi->text(0)) {
                  pGroup = &*g;
                  break;
                  }            
            }
      if (pGroup == 0) {
            printf("group not found\n");
            return;
            }
      Patch* patch = new Patch;
      patch->name = patchName;
      pGroup->patches.push_back(patch);
      QTreeWidgetItem* sitem = new QTreeWidgetItem;
      sitem->setText(0, patch->name);
      QVariant v = QVariant::fromValue((void*)(patch));
      sitem->setData(0, Qt::UserRole, v);

      pi->addChild(sitem);
      patchView->setCurrentItem(sitem);
      instrument->setDirty(true);
      }

//---------------------------------------------------------
//   newGroupClicked
//---------------------------------------------------------

void EditInstrument::newGroupClicked()
      {
      QListWidgetItem* item = instrumentList->currentItem();
      if (item == 0)
            return;
      MidiInstrument* instrument = (MidiInstrument*)item->data(Qt::UserRole).value<void*>();
      std::vector<PatchGroup>* pg = instrument->groups();
      QString groupName;
      for (int i = 1;; ++i) {
            groupName = QString("Group-%1").arg(i);
            bool found = false;

            for (std::vector<PatchGroup>::iterator g = pg->begin(); g != pg->end(); ++g) {
                  if (g->name == groupName) {
                        found = true;
                        break;
                        }
                  }
            if (!found)
                  break;
            }

      PatchGroup pGroup;
      pGroup.name = groupName;
      pg->push_back(pGroup);

      QTreeWidgetItem* sitem = new QTreeWidgetItem;
      sitem->setText(0, groupName);
      QVariant v = QVariant::fromValue((void*)0);
      sitem->setData(0, Qt::UserRole, v);
      patchView->addTopLevelItem(sitem);
      patchView->setCurrentItem(sitem);
      instrument->setDirty(true);
      }

//---------------------------------------------------------
//   newCategoryClicked
//---------------------------------------------------------

void EditInstrument::newCategoryClicked()
      {
      QListWidgetItem* item = instrumentList->currentItem();
      if (item == 0)
            return;
      MidiInstrument* instrument = (MidiInstrument*)item->data(Qt::UserRole).value<void*>();
      bool ok;
      QString cat = QInputDialog::getText(this,
            tr("MusE: Enter new Category"),
            tr("Enter new Category:"),
            QLineEdit::Normal, "", &ok
            );
      if (ok && !cat.isEmpty()) {
            category->addItem(cat);
            instrument->addCategory(cat);
            instrument->setDirty(true);
            }
      }

//---------------------------------------------------------
//   deleteControllerClicked
//---------------------------------------------------------

void EditInstrument::deleteControllerClicked()
      {
      QListWidgetItem* item = instrumentList->currentItem();
      QListWidgetItem* item2 = listController->currentItem();
      if (item == 0 || item2 == 0)
            return;
      MidiInstrument* instrument = (MidiInstrument*)item->data(Qt::UserRole).value<void*>();
      MidiController* ctrl       = (MidiController*)item2->data(Qt::UserRole).value<void*>();
      MidiControllerList* cl     = instrument->controller();
      cl->removeAll(ctrl);
      delete item2;
      instrument->setDirty(true);
      }

//---------------------------------------------------------
//   newControllerClicked
//---------------------------------------------------------

void EditInstrument::newControllerClicked()
      {
      QListWidgetItem* item = instrumentList->currentItem();
      if (item == 0)
            return;
      MidiInstrument* instrument = (MidiInstrument*)item->data(Qt::UserRole).value<void*>();

      QString ctrlName;
      MidiControllerList* cl = instrument->controller();
      for (int i = 1;; ++i) {
            ctrlName = QString("Controller-%d").arg(i);
      
            bool found = false;
            for (iMidiController ic = cl->begin(); ic != cl->end(); ++ic) {
                  MidiController* c = *ic;
                  if (c->name() == ctrlName) {
                        found = true;
                        break;
                        }
                  }
            if (!found)
                  break;
            }

      MidiController* ctrl = new MidiController();
      ctrl->setName(ctrlName);
      item = new QListWidgetItem(ctrlName);
      QVariant v = qVariantFromValue((void*)(ctrl));
      item->setData(Qt::UserRole, v);
      listController->addItem(item);
      listController->setCurrentItem(item);
      instrument->setDirty(true);
      }

//---------------------------------------------------------
//   deleteSysexClicked
//---------------------------------------------------------

void EditInstrument::deleteSysexClicked()
      {
      QListWidgetItem* item = instrumentList->currentItem();
      QListWidgetItem* item2 = sysexList->currentItem();
      if (item == 0 || item2 == 0)
            return;
      MidiInstrument* instrument = (MidiInstrument*)item->data(Qt::UserRole).value<void*>();
      SysEx* sysex  = (SysEx*)item2->data(Qt::UserRole).value<void*>();
      QList<SysEx*> sl = instrument->sysex();
      instrument->removeSysex(sysex);
      delete item2;
      instrument->setDirty(true);
      }

//---------------------------------------------------------
//   newSysexClicked
//---------------------------------------------------------

void EditInstrument::newSysexClicked()
      {
      QListWidgetItem* item = instrumentList->currentItem();
      if (item == 0)
            return;
      MidiInstrument* instrument = (MidiInstrument*)item->data(Qt::UserRole).value<void*>();

      QString sysexName;
      for (int i = 1;; ++i) {
            sysexName = QString("Sysex-%1").arg(i);
      
            bool found = false;
            foreach(const SysEx* s, instrument->sysex()) {
                  if (s->name == sysexName) {
                        found = true;
                        break;
                        }
                  }
            if (!found)
                  break;
            }
      SysEx* nsysex = new SysEx;
      nsysex->name = sysexName;
      instrument->addSysex(nsysex);

      item = new QListWidgetItem(sysexName);
      QVariant v = QVariant::fromValue((void*)nsysex);
      item->setData(Qt::UserRole, v);
      sysexList->addItem(item);
      sysexList->setCurrentItem(item);
      instrument->setDirty(true);
      }

//---------------------------------------------------------
//   instrumentChanged
//---------------------------------------------------------

void EditInstrument::instrumentChanged(QListWidgetItem* sel, QListWidgetItem* old)
      {
      patchView->clear();
      listController->clear();
      category->clear();
      sysexList->clear();

      if (sel == 0)
            return;
      if (old) {
            MidiInstrument* oi = (MidiInstrument*)old->data(Qt::UserRole).value<void*>();
            checkDirty(oi);
            oi->setDirty(false);
            }

      // populate patch list

      MidiInstrument* instrument = (MidiInstrument*)sel->data(Qt::UserRole).value<void*>();
      instrument->setDirty(false);

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
                  QVariant v = QVariant::fromValue((void*)patch);
                  sitem->setData(0, Qt::UserRole, v);
                  item->addChild(sitem);
                  }
            }
      MidiControllerList* cl = instrument->controller();
      for (iMidiController ic = cl->begin(); ic != cl->end(); ++ic) {
            MidiController* c = *ic;
            QListWidgetItem* item = new QListWidgetItem(c->name());
            QVariant v = QVariant::fromValue((void*)c);
            item->setData(Qt::UserRole, v);
            listController->addItem(item);
            }

      category->addItems(instrument->categories());

      foreach(const SysEx* s, instrument->sysex()) {
            QListWidgetItem* item = new QListWidgetItem(s->name);
            QVariant v = QVariant::fromValue((void*)s);
            item->setData(Qt::UserRole, v);
            sysexList->addItem(item);
            }

      sysexList->setItemSelected(sysexList->item(0), true);
      sysexChanged(sysexList->item(0), 0);

      if (!cl->empty()) {
            listController->setItemSelected(listController->item(0), true);
            controllerChanged(listController->item(0), 0);
            }
      }

//---------------------------------------------------------
//   patchChanged
//---------------------------------------------------------

void EditInstrument::patchChanged(QTreeWidgetItem* sel, QTreeWidgetItem* old)
      {
      if (old && old->data(0, Qt::UserRole).value<void*>()) {
            QListWidgetItem* item = instrumentList->currentItem();
            if (item == 0)
                  return;
            MidiInstrument* instrument = (MidiInstrument*)item->data(Qt::UserRole).value<void*>();
            Patch* p = (Patch*)old->data(0, Qt::UserRole).value<void*>();
            if (p->name != patchNameEdit->text()) {
                  p->name = patchNameEdit->text();
                  instrument->setDirty(true);
printf("patch mod 1\n");
                  }
            if (p->hbank != spinBoxHBank->value()) {
                  p->hbank = spinBoxHBank->value();
                  instrument->setDirty(true);
printf("patch mod 2\n");
                  }
            if (p->lbank != spinBoxLBank->value()) {
                  p->hbank = spinBoxHBank->value();
                  instrument->setDirty(true);
printf("patch mod 3\n");
                  }
            if (p->prog != spinBoxProgram->value()) {
                  p->prog = spinBoxProgram->value();
                  instrument->setDirty(true);
printf("patch mod 4\n");
                  }
            // there is no logical xor in c++
            bool a = p->typ & 1;
            bool b = p->typ & 2;
            bool c = p->typ & 4;
            bool aa = checkBoxGM->isChecked();
            bool bb = checkBoxGS->isChecked();
            bool cc = checkBoxXG->isChecked();
            if ((a ^ aa) || (b ^ bb) || (c ^ cc)) {
                  int value = 0;
                  if (checkBoxGM->isChecked())
                        value |= 1;
                  if (checkBoxGS->isChecked())
                        value |= 2;
                  if (checkBoxXG->isChecked())
                        value |= 4;
                  p->typ = value;
                  instrument->setDirty(true);
                  }
            if (p->categorie != category->currentIndex()) {
                  p->categorie = category->currentIndex();
                  instrument->setDirty(true);
                  }
            }
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
      category->setCurrentIndex(p->categorie);
      }

//---------------------------------------------------------
//   controllerChanged
//---------------------------------------------------------

void EditInstrument::controllerChanged(QListWidgetItem* sel, QListWidgetItem* old)
      {
      if (old) {
            QListWidgetItem* item = instrumentList->currentItem();
            if (item == 0)
                  return;
            MidiInstrument* instrument = (MidiInstrument*)item->data(Qt::UserRole).value<void*>();
            MidiController* oc = (MidiController*)old->data(Qt::UserRole).value<void*>();
            int ctrlH = spinBoxHCtrlNo->value();
            int ctrlL = spinBoxLCtrlNo->value();
            MidiController::ControllerType type = (MidiController::ControllerType)ctrlType->currentIndex();
            int num = MidiController::genNum(type, ctrlH, ctrlL);

            if (num != oc->num()) {
                  oc->setNum(num);
                  instrument->setDirty(true);
                  }
            if (spinBoxMin->value() != oc->minVal()) {
                  oc->setMinVal(spinBoxMin->value());
                  instrument->setDirty(true);
                  }
            if (spinBoxMax->value() != oc->maxVal()) {
                  oc->setMaxVal(spinBoxMax->value());
                  instrument->setDirty(true);
                  }
            if (spinBoxDefault->value() != oc->initVal()) {
                  oc->setInitVal(spinBoxDefault->value());
                  instrument->setDirty(true);
                  }
            }
      if (sel == 0 || sel->data(Qt::UserRole).value<void*>() == 0) {
            // patchNameEdit->setText("");
            return;
            }
      MidiController* c = (MidiController*)sel->data(Qt::UserRole).value<void*>();
      entryName->setText(c->name());
      int ctrlH = (c->num() >> 8) & 0x7f;
      int ctrlL = c->num() & 0x7f;
      int type = int(c->type());
      ctrlType->setCurrentIndex(type);
      ctrlTypeChanged(type);
      spinBoxHCtrlNo->setValue(ctrlH);
      spinBoxLCtrlNo->setValue(ctrlL);
      spinBoxMin->setValue(c->minVal());
      spinBoxMax->setValue(c->maxVal());
      spinBoxDefault->setRange(c->minVal()-1, c->maxVal());
      spinBoxDefault->setValue(c->initVal());
      }

//---------------------------------------------------------
//   sysexChanged
//---------------------------------------------------------

void EditInstrument::sysexChanged(QListWidgetItem* sel, QListWidgetItem* old)
      {
      if (old) {
            QListWidgetItem* item = instrumentList->currentItem();
            if (item == 0)
                  return;
            MidiInstrument* instrument = (MidiInstrument*)item->data(Qt::UserRole).value<void*>();
            SysEx* so = (SysEx*)old->data(Qt::UserRole).value<void*>();
            if (sysexName->text() != so->name) {
                  so->name = sysexName->text();
                  instrument->setDirty(true);
printf("sysex mod 1\n");
                  }
            if (sysexComment->toPlainText() != so->comment) {
                  so->comment = sysexComment->toPlainText();
                  instrument->setDirty(true);
printf("sysex mod 2\n");
                  }
            unsigned char* data;
            int len = string2sysex(sysexData->toPlainText(), &data);
            if (so->dataLen != len || !memcmp(data, so->data, len)) {
                  delete so->data;
                  so->data = data;
                  so->dataLen = len;
                  }
            }
      if (sel == 0) {
            sysexName->setText("");
            sysexComment->setText("");
            sysexData->setText("");
            sysexName->setEnabled(false);
            sysexComment->setEnabled(false);
            sysexData->setEnabled(false);
            return;
            }
      sysexName->setEnabled(true);
      sysexComment->setEnabled(true);
      sysexData->setEnabled(true);

      SysEx* sx = (SysEx*)sel->data(Qt::UserRole).value<void*>();
      sysexName->setText(sx->name);
      sysexComment->setText(sx->comment);
      sysexData->setText(sysex2string(sx->dataLen, sx->data));
      }

//---------------------------------------------------------
//   checkDirty
//    return true on Abort
//---------------------------------------------------------

bool EditInstrument::checkDirty(MidiInstrument* i)
      {
      if (!i->dirty())
            return false;
      int n = QMessageBox::warning(this, tr("MusE"),
         tr("The current Instrument contains unsaved data\n"
         "Save Current Instrument?"),
         tr("&Save"), tr("&Nosave"), tr("&Abort"), 0, 2);
      if (n == 0) {
            if (i->filePath().isEmpty())
                  fileSaveAs();
            else {
                  QFile f(i->filePath());
                  if (!f.open(QIODevice::WriteOnly))
                        fileSaveAs();
                  else {
                        f.close();
                        if (fileSave(i, i->filePath()))
                              i->setDirty(false);
                        }
                  }
            return false;
            }
      return n == 2;
      }

//---------------------------------------------------------
//   ctrlTypeChanged
//---------------------------------------------------------

void EditInstrument::ctrlTypeChanged(int idx)
      {
      MidiController::ControllerType t = (MidiController::ControllerType)idx;
      switch (t) {
            case MidiController::RPN:
            case MidiController::NRPN:
            case MidiController::Controller7:
                  spinBoxHCtrlNo->setEnabled(false);
                  spinBoxLCtrlNo->setEnabled(true);
                  break;
            case MidiController::Controller14:
            case MidiController::RPN14:
            case MidiController::NRPN14:
                  spinBoxHCtrlNo->setEnabled(true);
                  spinBoxLCtrlNo->setEnabled(true);
                  break;
            case MidiController::Pitch:
            case MidiController::Program:
                  spinBoxHCtrlNo->setEnabled(false);
                  spinBoxLCtrlNo->setEnabled(false);
                  break;
            default:
                  break;
            }      
      }

