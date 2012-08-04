//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: midictrledit.cpp,v 1.1.1.1.2.2 2008/08/18 00:15:24 terminator356 Exp $
//
//  (C) Copyright 2003 Werner Schweer (ws@seh.de)
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

#include <stdio.h>

#include "app.h"
#include "midiport.h"
#include "mididev.h"
#include "midictrl.h"
#include "midictrledit.h"
#include "minstrument.h"
#include "song.h"
#include "xml.h"
#include "filedialog.h"
#include "globals.h"

namespace MusEGui {

MidiControllerEditDialog* midiControllerEditDialog;

static MidiController predefinedMidiController[] = {
      MidiController(QString("Pitch"), 0x40000, -8192, +8191, 0),
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
      QString lnum;
      QString min;
      QString max;
      int n = mctrl->num();
      int h = (n >> 8) & 0x7f;
      int l = n & 0x7f;
      MidiController::ControllerType t = midiControllerType(n);
      switch(t)
      {
          case MidiController::Controller7:
                hnum = "---";
                lnum.setNum(l);
                min.setNum(mctrl->minVal());
                max.setNum(mctrl->maxVal());
                break;
          case MidiController::RPN:
          case MidiController::NRPN:
          case MidiController::RPN14:
          case MidiController::NRPN14:
          case MidiController::Controller14:
                hnum.setNum(h);
                lnum.setNum(l);
                min.setNum(mctrl->minVal());
                max.setNum(mctrl->maxVal());
                break;
          case MidiController::Pitch:
                hnum = "---";
                lnum = "---";
                min.setNum(mctrl->minVal());
                max.setNum(mctrl->maxVal());
                break;
          default:
                hnum = "---";
                lnum = "---";
                min.setNum(0);
                max.setNum(0);
                break;
      }
      
      new Q3ListViewItem(viewController,
               mctrl->name(),
               int2ctrlType(t),
               hnum, lnum, min, max
               );
               
      }
//---------------------------------------------------------
//   MidiControllerEditDialog
//---------------------------------------------------------

MidiControllerEditDialog::MidiControllerEditDialog(QWidget* parent, const char* name, bool modal, Qt::WFlags fl)
   : MidiControllerEditDialogBase(parent, name, modal, fl)
      {
      _lastPort = midiPortsList->currentItem();
      viewController->setColumnAlignment(COL_HNUM, Qt::AlignCenter);
      viewController->setColumnAlignment(COL_LNUM, Qt::AlignCenter);
      viewController->setColumnAlignment(COL_MIN,  Qt::AlignCenter);
      viewController->setColumnAlignment(COL_MAX,  Qt::AlignCenter);
      viewController->setColumnWidthMode(COL_NAME, Q3ListView::Maximum);

      // populate list of predefined controller
      updatePredefinedList();

      // populate ports pulldown
      updateMidiPortsList();      
      connect(buttonNew,    SIGNAL(clicked()), SLOT(ctrlAdd()));
      connect(buttonDelete, SIGNAL(clicked()), SLOT(ctrlDelete()));
      connect(entryName,    SIGNAL(textChanged(const QString&)), SLOT(nameChanged(const QString&)));
      connect(comboType,    SIGNAL(activated(const QString&)), SLOT(typeChanged(const QString&)));
      connect(spinboxHCtrlNo, SIGNAL(valueChanged(int)), SLOT(valueHChanged(int)));
      connect(spinboxLCtrlNo, SIGNAL(valueChanged(int)), SLOT(valueLChanged(int)));
      connect(spinboxMin, SIGNAL(valueChanged(int)), SLOT(minChanged(int)));
      connect(spinboxMax, SIGNAL(valueChanged(int)), SLOT(maxChanged(int)));
      connect(viewController, SIGNAL(selectionChanged()), SLOT(controllerChanged()));
      connect(buttonApply,  SIGNAL(clicked()), SLOT(apply()));
      connect(midiPortsList, SIGNAL(activated(int)), SLOT(portChanged(int)));

      updateViewController();
      _modified = false;
      buttonApply->setEnabled(false);
      connect(MusEGlobal::song, SIGNAL(songChanged(int)), SLOT(songChanged(int)));
      }

//---------------------------------------------------------
//   songChanged
//---------------------------------------------------------

void MidiControllerEditDialog::songChanged(int flags)
{
  // Is it simply a midi controller value adjustment? Forget it.
  if(flags == SC_MIDI_CONTROLLER)
    return;
    
  if(flags & (SC_CONFIG | SC_MIDI_CONTROLLER))
  {
    midiPortsList->blockSignals(true);
    updatePredefinedList();
    updateMidiPortsList();
    updateViewController();
    midiPortsList->blockSignals(false);
  }  
}

//---------------------------------------------------------
//   updatePredefinedList
//---------------------------------------------------------

void MidiControllerEditDialog::updatePredefinedList()
{
      listController->clear();
      int size = sizeof(predefinedMidiController) / sizeof(*predefinedMidiController);
      for (int i = 0; i < size; ++i)
            listController->insertItem(predefinedMidiController[i].name());
      listController->setSelected(0, true);
}

//---------------------------------------------------------
//   updateMidiPortsList
//---------------------------------------------------------

void MidiControllerEditDialog::updateMidiPortsList()
{
      midiPortsList->clear();
      for (int i = 0; i < MIDI_PORTS; ++i) {
            MidiPort* port = &midiPorts[i];
            MidiDevice* dev = port->device();
            QString name;
            name.sprintf("%d(%s)", port->portno()+1,
                     dev ? dev->name().toLatin1() : "none");
            midiPortsList->insertItem(name, i);
            }
      _lastPort = midiPortsList->currentItem();
}

//---------------------------------------------------------
//   updateViewController
//---------------------------------------------------------

void MidiControllerEditDialog::updateViewController()
{
      int mpidx = midiPortsList->currentItem();
      
      viewController->clear();
      MidiInstrument* mi = midiPorts[mpidx].instrument();
      MidiControllerList* mcl = mi->controller();
      for (iMidiController i = mcl->begin(); i != mcl->end(); ++i) 
            addControllerToView(*i);
            
      viewController->blockSignals(true);      
      viewController->setCurrentItem(viewController->firstChild());
      controllerChanged(viewController->currentItem());
      viewController->blockSignals(false);      
      
      setModified(false);      
}
      
//---------------------------------------------------------
//   reject
//---------------------------------------------------------

void MidiControllerEditDialog::setModified(bool v)
{
  if(v == _modified)
    return;
    
  _modified = v;  
  
  if(v)
  {
    buttonApply->setEnabled(true);
  }
  else
  {
    buttonApply->setEnabled(false);
  }
}

//---------------------------------------------------------
//   reject
//---------------------------------------------------------

void MidiControllerEditDialog::reject()
      {
      // Restore the list before closing this dialog.
      updateViewController();      
      //setModified(false);      
      
      MidiControllerEditDialogBase::reject();
      }

//---------------------------------------------------------
//   apply
//---------------------------------------------------------
      
void MidiControllerEditDialog::apply()
{
      int mpidx = midiPortsList->currentItem();
      MidiInstrument* mi = midiPorts[mpidx].instrument();
      MidiControllerList* mcl = mi->controller();
      mcl->clear();
      

      Q3ListViewItem* item = viewController->firstChild();
      int hval;
      int lval;
      while (item) {
            hval = item->text(COL_HNUM).toInt();
            lval = item->text(COL_LNUM).toInt();
            MidiController* c = new MidiController();
            c->setName(item->text(COL_NAME));
            
            MidiController::ControllerType type = ctrlType2Int(item->text(COL_TYPE));

            switch(type) {
                  case MidiController::Controller7:
                        c->setNum(lval);
                        break;
                  case MidiController::Controller14:
                        c->setNum((hval << 8 | lval) | CTRL_14_OFFSET);
                        break;
                  case MidiController::RPN:
                        c->setNum((hval << 8 | lval) | CTRL_RPN_OFFSET);
                        break;
                  case MidiController::NRPN:
                        c->setNum((hval << 8 | lval) | CTRL_NRPN_OFFSET);
                        break;
                  case MidiController::RPN14:
                        c->setNum((hval << 8 | lval) | CTRL_RPN14_OFFSET);
                        break;
                  case MidiController::NRPN14:
                        c->setNum((hval << 8 | lval) | CTRL_NRPN14_OFFSET);
                        break;
                  case MidiController::Program:
                        c->setNum(CTRL_PROGRAM);
                        break;
                  case MidiController::Pitch:
                        c->setNum(CTRL_PITCH);
                        break;
                  default:
                        break;
                  }
            if(type == MidiController::Program)
            {
              c->setMinVal(0);
              c->setMaxVal(0xffffff);
            }
            else
            {
              c->setMinVal(item->text(COL_MIN).toInt());
              c->setMaxVal(item->text(COL_MAX).toInt());
            }
            
            mcl->push_back(c);
            
            item = item->nextSibling();
            }
            
      MusEGlobal::song->update(SC_CONFIG | SC_MIDI_CONTROLLER);
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
            MidiController::ControllerType t = midiControllerType(c->num());
            QString type = int2ctrlType(t);
            QString min, max;
            QString hno, lno;
            int h = (c->num() >> 8) & 0x7f;
            int l = c->num() & 0x7f;

            switch(t) {
                  case MidiController::Controller7:
                        min.setNum(c->minVal());
                        max.setNum(c->maxVal());
                        hno = "---";
                        lno.setNum(l);
                        break;
                  case MidiController::RPN:
                  case MidiController::NRPN:
                  case MidiController::RPN14:
                  case MidiController::NRPN14:
                  case MidiController::Controller14:
                        min.setNum(c->minVal());
                        max.setNum(c->maxVal());
                        hno.setNum(h);
                        lno.setNum(l);
                        break;
                  case MidiController::Pitch:
                        min.setNum(c->minVal());
                        max.setNum(c->maxVal());
                        hno = "---";
                        lno = "---";
                        break;
                  default:
                        hno = "---";
                        lno = "---";
                        min.setNum(0);
                        max.setNum(0);
                        break;
                  }

            Q3ListViewItem* item = new Q3ListViewItem(viewController,
               name, type, hno, lno, min, max);

            viewController->blockSignals(true);
            viewController->setCurrentItem(item);
            controllerChanged(item);
            viewController->blockSignals(false);
            
            setModified(true);      
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
      
      setModified(true);
      }

//---------------------------------------------------------
//   accept
//---------------------------------------------------------

void MidiControllerEditDialog::accept()
      {
      apply();
      MidiControllerEditDialogBase::accept();
      }

//---------------------------------------------------------
//   portChanged
//---------------------------------------------------------

void MidiControllerEditDialog::portChanged(int n)
      {
        if(n == _lastPort)
          return;
        _lastPort = n;
          
        //listController->blockSignals(true); DELETETHIS
        //midiPortsList->blockSignals(true);
        //viewController->blockSignals(true);
        //updatePredefinedList();
        //updateMidiPortsList();
        //reject();   // populate list
        updateViewController();
        //viewController->setCurrentItem(viewController->firstChild()); DELETETHIS
        //controllerChanged(viewController->currentItem());
        //listController->blockSignals(false);
        //midiPortsList->blockSignals(false);
        //viewController->blockSignals(false);
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
      
      setModified(true);      
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
      switch(ctrlType2Int(s)) {
            case MidiController::Controller14:
            case MidiController::RPN14:
            case MidiController::NRPN14:
                  item->setText(COL_LNUM, QString("0"));
                  item->setText(COL_HNUM, QString("0"));
                  item->setText(COL_MIN, QString("0"));
                  item->setText(COL_MAX, QString("16383"));
                  break;
            case MidiController::Controller7:
                  item->setText(COL_MIN, QString("0"));
                  item->setText(COL_MAX, QString("127"));
                  item->setText(COL_LNUM, QString("0"));
                  item->setText(COL_HNUM, QString("---"));
                  break;
            case MidiController::RPN:
            case MidiController::NRPN:
                  item->setText(COL_MIN, QString("0"));
                  item->setText(COL_MAX, QString("127"));
                  item->setText(COL_LNUM, QString("0"));
                  item->setText(COL_HNUM, QString("0"));
                  break;
                  
            case MidiController::Program:
            
                  item->setText(COL_MIN, QString("---"));
                  item->setText(COL_MAX, QString("---"));
                  item->setText(COL_LNUM, QString("---"));
                  item->setText(COL_HNUM, QString("---"));
                  break;
            case MidiController::Pitch:
                  item->setText(COL_MIN, QString("-8192"));
                  item->setText(COL_MAX, QString("8191"));
                  item->setText(COL_LNUM, QString("---"));
                  item->setText(COL_HNUM, QString("---"));
                  break;
            default:
                  break;
            }
      
      setModified(true);      
      
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
      
      setModified(true);      
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
      
      setModified(true);      
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
                  spinboxHCtrlNo->setValue(0);
                  spinboxLCtrlNo->setValue(item->text(COL_LNUM).toInt());
                  spinboxMin->setRange(0, 127);
                  spinboxMax->setRange(0, 127);
                  spinboxMin->setValue(item->text(COL_MIN).toInt());
                  spinboxMax->setValue(item->text(COL_MAX).toInt());
                  break;

            case MidiController::RPN:
            case MidiController::NRPN:
                  comboType->setEnabled(true);
                  spinboxHCtrlNo->setEnabled(true);
                  spinboxLCtrlNo->setEnabled(true);
                  spinboxMin->setEnabled(true);
                  spinboxMax->setEnabled(true);
                  spinboxHCtrlNo->setValue(item->text(COL_HNUM).toInt());
                  spinboxLCtrlNo->setValue(item->text(COL_LNUM).toInt());
                  spinboxMin->setRange(0, 127);
                  spinboxMax->setRange(0, 127);
                  spinboxMin->setValue(item->text(COL_MIN).toInt());
                  spinboxMax->setValue(item->text(COL_MAX).toInt());
                  break;
            case MidiController::Controller14:
            case MidiController::RPN14:
            case MidiController::NRPN14:
                  comboType->setEnabled(true);
                  spinboxHCtrlNo->setEnabled(true);
                  spinboxLCtrlNo->setEnabled(true);
                  spinboxMin->setEnabled(true);
                  spinboxMax->setEnabled(true);

                  spinboxHCtrlNo->setValue(item->text(COL_HNUM).toInt());
                  spinboxLCtrlNo->setValue(item->text(COL_LNUM).toInt());
                  spinboxMin->setRange(0, 16383);
                  spinboxMax->setRange(0, 16383);
                  spinboxMin->setValue(item->text(COL_MIN).toInt());
                  spinboxMax->setValue(item->text(COL_MAX).toInt());
                  break;

            case MidiController::Pitch:
                  comboType->setEnabled(true);
                  spinboxHCtrlNo->setEnabled(false);
                  spinboxLCtrlNo->setEnabled(false);
                  spinboxMin->setEnabled(true);
                  spinboxMax->setEnabled(true);
                  spinboxHCtrlNo->setValue(0);
                  spinboxLCtrlNo->setValue(0);
                  spinboxMin->setRange(-8192, 8191);
                  spinboxMax->setRange(-8192, 8191);
                  spinboxMin->setValue(item->text(COL_MIN).toInt());
                  spinboxMax->setValue(item->text(COL_MAX).toInt());
                  break;
                  
            case MidiController::Program:
                  comboType->setEnabled(true);
                  spinboxHCtrlNo->setEnabled(false);
                  spinboxLCtrlNo->setEnabled(false);
                  spinboxMin->setEnabled(false);
                  spinboxMax->setEnabled(false);
                  spinboxHCtrlNo->setValue(0);
                  spinboxLCtrlNo->setValue(0);
                  spinboxMin->setRange(0, 0);
                  spinboxMax->setRange(0, 0);
                  spinboxMin->setValue(0);
                  spinboxMax->setValue(0);
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
      
      if(val > item->text(COL_MAX).toInt())
      {
        spinboxMax->blockSignals(true);
        spinboxMax->setValue(val);
        item->setText(COL_MAX, s);
        spinboxMax->blockSignals(false);
      }  
      setModified(true);      
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
      
      if(val < item->text(COL_MIN).toInt())
      {
        spinboxMin->blockSignals(true);
        spinboxMin->setValue(val);
        item->setText(COL_MIN, s);
        spinboxMin->blockSignals(false);
      }  
      setModified(true);      
      }

//---------------------------------------------------------
//   configMidiController
//---------------------------------------------------------

void configMidiController()
    {
      if (midiControllerEditDialog == 0)
      {
            midiControllerEditDialog = new MidiControllerEditDialog();
            midiControllerEditDialog->show();
      }
      else
      {
        if(midiControllerEditDialog->isShown())
          midiControllerEditDialog->hide();
        else      
          midiControllerEditDialog->show();
      }
    }

} // namespace MusEGui
