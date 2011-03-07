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

#include "app.h"
#include "driver/mididev.h"
#include "midictrl.h"
#include "midictrledit.h"
#include "xml.h"
#include "filedialog.h"
#include "globals.h"

MidiControllerEditDialog* midiControllerEditDialog;

static MidiController predefinedMidiController[] = {
      MidiController("Pitch", 0x40000, -10000, +10000),
      MidiController("Program", 0x40001, 0, 127),
      MidiController("BankSel", 0x10000, 0, 16383)
      };

enum {
      COL_NAME = 0, COL_TYPE,
      COL_HNUM, COL_LNUM, COL_MIN, COL_MAX
      };

//---------------------------------------------------------
//   addControllerToView
//---------------------------------------------------------

void MidiControllerEditDialog::addControllerToView(MidiController* mctrl)
      {
      QString hnum;
      hnum.setNum(mctrl->num());
      QString lnum("---");
      QString min;
      min.setNum(mctrl->minVal());
      QString max;
      max.setNum(mctrl->maxVal());
      new Q3ListViewItem(viewController,
               mctrl->name(),
               int2ctrlType(mctrl->type()),
               hnum, lnum, min, max
               );
      }

//---------------------------------------------------------
//   MidiControllerEditDialog
//---------------------------------------------------------

MidiControllerEditDialog::MidiControllerEditDialog(QWidget* parent, const char* name, bool modal, Qt::WFlags fl)
   : MidiControllerEditDialogBase(parent, name, modal, fl)
      {
      viewController->setColumnAlignment(COL_HNUM, AlignCenter);
      viewController->setColumnAlignment(COL_LNUM, AlignCenter);
      viewController->setColumnAlignment(COL_MIN,  AlignCenter);
      viewController->setColumnAlignment(COL_MAX,  AlignCenter);
      viewController->setColumnWidthMode(COL_NAME, Q3ListView::Maximum);

      // populate list of predefined controller
      int size = sizeof(predefinedMidiController) / sizeof(*predefinedMidiController);
      for (int i = 0; i < size; ++i)
            listController->insertItem(predefinedMidiController[i].name());
      listController->setSelected(0, true);

      // populate ports pulldown
      for (int i = 0; i < MIDI_PORTS; ++i) {
            MidiPort* port = &midiPorts[i];
            MidiDevice* dev = port->device();
            QString name;
            name.sprintf("%d(%s)", port->portno()+1,
                     dev ? dev->name().latin1() : "none");
            midiPortsList->insertItem(name, i);
            }

      reject();   // populate list
      viewController->setCurrentItem(viewController->firstChild());

      connect(buttonNew,    SIGNAL(clicked()), SLOT(ctrlAdd()));
      connect(buttonDelete, SIGNAL(clicked()), SLOT(ctrlDelete()));
      connect(entryName,    SIGNAL(textChanged(const QString&)), SLOT(nameChanged(const QString&)));
      connect(comboType,    SIGNAL(activated(const QString&)), SLOT(typeChanged(const QString&)));
      connect(spinboxHCtrlNo, SIGNAL(valueChanged(int)), SLOT(valueHChanged(int)));
      connect(spinboxLCtrlNo, SIGNAL(valueChanged(int)), SLOT(valueLChanged(int)));
      connect(spinboxMin, SIGNAL(valueChanged(int)), SLOT(minChanged(int)));
      connect(spinboxMax, SIGNAL(valueChanged(int)), SLOT(maxChanged(int)));
      connect(viewController, SIGNAL(selectionChanged()), SLOT(controllerChanged()));

      controllerChanged(viewController->currentItem());
      }

//---------------------------------------------------------
//   reject
//---------------------------------------------------------

void MidiControllerEditDialog::reject()
      {
      viewController->clear();
      for (iMidiController i = midiControllerList.begin();
         i != midiControllerList.end(); ++i) {
            addControllerToView(&*i);
            }
      MidiControllerEditDialogBase::reject();
      }

//---------------------------------------------------------
//   ctrlAdd
//---------------------------------------------------------

void MidiControllerEditDialog::ctrlAdd()
      {
      Q3ListBoxItem* item = listController->selectedItem();
      if (item == 0)
            return;
      QString name = item->text();
      int size = sizeof(predefinedMidiController) / sizeof(*predefinedMidiController);
      for (int i = 0; i < size; ++i) {
            MidiController* c = &predefinedMidiController[i];
            if (c->name() != name)
                  continue;
            MidiController::ControllerType t = c->type();
            QString type = int2ctrlType(t);
            QString min, max;
            min.setNum(c->minVal());
            max.setNum(c->maxVal());

            QString hno, lno;
            int h = (c->num() >> 14) & 0x7f;
            int l = c->num() & 0x7f;

            switch(t) {
                  case MidiController::Controller7:
                        hno = "---";
                        lno.setNum(l);
                        break;
                  case MidiController::RPN:
                  case MidiController::NRPN:
                  case MidiController::Controller14:
                        hno.setNum(h);
                        lno.setNum(l);
                        break;
                  case MidiController::Pitch:
                  case MidiController::Program:
                        hno = "---";
                        lno = "---";
                  default:
                        break;
                  }

            Q3ListViewItem* item = new Q3ListViewItem(viewController,
               name, type, hno, lno, min, max);

            viewController->setCurrentItem(item);
            controllerChanged(item);
            break;
            }
      }

//---------------------------------------------------------
//   ctrlDelete
//---------------------------------------------------------

void MidiControllerEditDialog::ctrlDelete()
      {
      Q3ListViewItem* item = viewController->currentItem();
      if (item == 0)
            return;
      delete item;
      }

//---------------------------------------------------------
//   accept
//---------------------------------------------------------

void MidiControllerEditDialog::accept()
      {
      midiControllerList.clear();

      Q3ListViewItem* item = viewController->firstChild();
      int hval = item->text(COL_HNUM).toInt();
      int lval = item->text(COL_LNUM).toInt();

      while (item) {
            MidiController c;
            c.setName(item->text(COL_NAME));
            MidiController::ControllerType type = ctrlType2Int(item->text(COL_TYPE));

            switch(type) {
                  case MidiController::Controller7:
                        c.setNum(hval);
                        break;
                  case MidiController::Controller14:
                        c.setNum((hval << 8 | lval) | 0x10000);
                        break;
                  case MidiController::RPN:
                        c.setNum((hval << 8 | lval) | 0x20000);
                        break;
                  case MidiController::NRPN:
                        c.setNum((hval << 8 | lval) | 0x30000);
                        break;
                  case MidiController::Pitch:
                        c.setNum(CTRL_PITCH);
                        break;
                  default:
                        break;
                  }
            c.setMinVal(item->text(COL_MIN).toInt());
            c.setMaxVal(item->text(COL_MAX).toInt());
            midiControllerList.push_back(c);
            item = item->nextSibling();
            }
      MidiControllerEditDialogBase::accept();
      }

//---------------------------------------------------------
//   nameChanged
//---------------------------------------------------------

void MidiControllerEditDialog::nameChanged(const QString& s)
      {
      Q3ListViewItem* item = viewController->currentItem();
      if (item == 0)
            return;
      item->setText(COL_NAME, s);
      }

//---------------------------------------------------------
//   typeChanged
//---------------------------------------------------------

void MidiControllerEditDialog::typeChanged(const QString& s)
      {
      Q3ListViewItem* item = viewController->currentItem();
      if (item == 0)
            return;
      item->setText(COL_TYPE, s);
      item->setText(COL_MIN, QString("0"));
      switch(ctrlType2Int(s)) {
            case 2:     // RPN
            case 3:     // NRPN
            case MidiController::Controller14:
                  item->setText(COL_MAX, QString("16383"));
                  break;
            case MidiController::Controller7:
            case MidiController::Program:
                  item->setText(COL_MAX, QString("127"));
                  break;
            case MidiController::Pitch:
                  item->setText(COL_MIN, QString("-5000"));
                  item->setText(COL_MAX, QString("+5000"));
                  break;
            default:
                  break;
            }
      controllerChanged(item);
      }

//---------------------------------------------------------
//   valueHChanged
//---------------------------------------------------------

void MidiControllerEditDialog::valueHChanged(int val)
      {
      Q3ListViewItem* item = viewController->currentItem();
      if (item == 0)
            return;
      QString s;
      s.setNum(val);
      item->setText(COL_HNUM, s);
      }

//---------------------------------------------------------
//   valueLChanged
//---------------------------------------------------------

void MidiControllerEditDialog::valueLChanged(int val)
      {
      Q3ListViewItem* item = viewController->currentItem();
      if (item == 0)
            return;
      QString s;
      s.setNum(val);
      item->setText(COL_LNUM, s);
      }

//---------------------------------------------------------
//   controllerChanged
//---------------------------------------------------------

void MidiControllerEditDialog::controllerChanged()
      {
      Q3ListViewItem* item = viewController->selectedItem();
      controllerChanged(item);
      }

void MidiControllerEditDialog::controllerChanged(Q3ListViewItem* item)
      {
      if (item == 0) {
            entryName->setEnabled(false);
            comboType->setEnabled(false);
            spinboxHCtrlNo->setEnabled(false);
            spinboxLCtrlNo->setEnabled(false);
            spinboxMin->setEnabled(false);
            spinboxMax->setEnabled(false);
            return;
            }

      entryName->blockSignals(true);
      comboType->blockSignals(true);
      spinboxHCtrlNo->blockSignals(true);
      spinboxLCtrlNo->blockSignals(true);
      spinboxMin->blockSignals(true);
      spinboxMax->blockSignals(true);

      entryName->setEnabled(true);
      entryName->setText(item->text(COL_NAME));
      comboType->setCurrentItem(int(ctrlType2Int(item->text(COL_TYPE))));
      switch (ctrlType2Int(item->text(COL_TYPE))) {
            case MidiController::Controller7:
                  comboType->setEnabled(true);
                  spinboxHCtrlNo->setEnabled(false);
                  spinboxLCtrlNo->setEnabled(true);
                  spinboxMin->setEnabled(true);
                  spinboxMax->setEnabled(true);

                  spinboxHCtrlNo->setValue(item->text(COL_HNUM).toInt());
                  spinboxMin->setValue(item->text(COL_MIN).toInt());
                  spinboxMax->setValue(item->text(COL_MAX).toInt());
                  item->setText(COL_LNUM, QString("---"));
                  break;

            case MidiController::Controller14:
            case MidiController::RPN:
            case MidiController::NRPN:
                  comboType->setEnabled(true);
                  spinboxHCtrlNo->setEnabled(true);
                  spinboxLCtrlNo->setEnabled(true);
                  spinboxMin->setEnabled(true);
                  spinboxMax->setEnabled(true);

                  spinboxHCtrlNo->setValue(item->text(COL_HNUM).toInt());
                  spinboxLCtrlNo->setValue(item->text(COL_LNUM).toInt());
                  spinboxMin->setValue(item->text(COL_MIN).toInt());
                  spinboxMax->setValue(item->text(COL_MAX).toInt());
                  break;

            case MidiController::Pitch:
            case MidiController::Program:
                  comboType->setEnabled(true);
                  spinboxHCtrlNo->setEnabled(false);
                  spinboxLCtrlNo->setEnabled(false);
                  spinboxMin->setEnabled(true);
                  spinboxMax->setEnabled(true);
                  break;
            default:
                  break;
            }
      entryName->blockSignals(false);
      comboType->blockSignals(false);
      spinboxHCtrlNo->blockSignals(false);
      spinboxLCtrlNo->blockSignals(false);
      spinboxMin->blockSignals(false);
      spinboxMax->blockSignals(false);
      }

//---------------------------------------------------------
//   minChanged
//---------------------------------------------------------

void MidiControllerEditDialog::minChanged(int val)
      {
      Q3ListViewItem* item = viewController->currentItem();
      if (item == 0)
            return;
      QString s;
      s.setNum(val);
      item->setText(COL_MIN, s);
      }

//---------------------------------------------------------
//   maxChanged
//---------------------------------------------------------

void MidiControllerEditDialog::maxChanged(int val)
      {
      Q3ListViewItem* item = viewController->currentItem();
      if (item == 0)
            return;
      QString s;
      s.setNum(val);
      item->setText(COL_MAX, s);
      }

//---------------------------------------------------------
//   configMidiController
//---------------------------------------------------------

void configMidiController()
      {
      if (midiControllerEditDialog == 0)
            midiControllerEditDialog = new MidiControllerEditDialog();
      midiControllerEditDialog->show();
      }
