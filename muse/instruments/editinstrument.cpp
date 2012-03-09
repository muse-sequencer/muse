//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: editinstrument.cpp,v 1.2.2.6 2009/05/31 05:12:12 terminator356 Exp $
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
#include <errno.h>

#include <QCloseEvent>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QInputDialog>
#include <QMessageBox>
#include <QLineEdit>
#include <QWhatsThis>

#include "editinstrument.h"
#include "minstrument.h"
#include "globals.h"
#include "song.h"
#include "xml.h"
#include "midictrl.h"
#include "gconfig.h"
#include "icons.h"

namespace MusEGui {

enum {
      COL_NAME = 0, COL_TYPE,
      COL_HNUM, COL_LNUM, COL_MIN, COL_MAX, COL_DEF
      };

//---------------------------------------------------------
//   EditInstrument
//---------------------------------------------------------

EditInstrument::EditInstrument(QWidget* parent, Qt::WFlags fl)
   : QMainWindow(parent, fl)
      {
      setupUi(this);
      fileNewAction->setIcon(QIcon(*filenewIcon));
      fileOpenAction->setIcon(QIcon(*openIcon));
      fileSaveAction->setIcon(QIcon(*saveIcon));
      fileSaveAsAction->setIcon(QIcon(*saveasIcon));
      fileExitAction->setIcon(QIcon(*exitIcon));
      viewController->setSelectionMode(QAbstractItemView::SingleSelection);
      toolBar->addAction(QWhatsThis::createAction(this));
      Help->addAction(QWhatsThis::createAction(this));

      // populate instrument list
      // Populate common controller list.
      for(int i = 0; i < 128; ++i)
      {
        QListWidgetItem *lci = new QListWidgetItem(MusECore::midiCtrlName(i));
        listController->addItem(lci);
      }
      oldMidiInstrument = 0;
      oldPatchItem = 0;
      for (MusECore::iMidiInstrument i = MusECore::midiInstruments.begin(); i != MusECore::midiInstruments.end(); ++i) {
            // Imperfect, crude way of ignoring internal instruments, soft synths etc. If it has a gui, 
            //  it must be an internal instrument. But this will still allow some vst instruments (without a gui) 
            //  to show up in the list.
            // Hmm, try file path instead...
            //if((*i)->hasGui())
            if((*i)->filePath().isEmpty())
              continue;
              
            QListWidgetItem* item = new QListWidgetItem((*i)->iname());
            QVariant v = qVariantFromValue((void*)(*i));
            item->setData(Qt::UserRole, v);
            instrumentList->addItem(item);
            }
      instrumentList->setSelectionMode(QAbstractItemView::SingleSelection);
      if(instrumentList->item(0))
        instrumentList->setCurrentItem(instrumentList->item(0));
      //DELETETHIS
      //oldMidiInstrument = (MusECore::MidiInstrument*)((ListBoxData*)instrumentList->item(0))->data();
      //oldMidiInstrument = (ListBoxData*)instrumentList->item(0);
      //oldMidiInstrument = (ListBoxData*)instrumentList->selectedItem();
      
//      MusECore::MidiInstrument* wip = (MusECore::MidiInstrument*)((ListBoxData*)instrumentList->item(0))->data();
//      if(wip)
        // Assignment
//        workingInstrument.assign( *wip );
      
      
      connect(instrumentList, SIGNAL(itemSelectionChanged()), SLOT(instrumentChanged()));
      connect(patchView, SIGNAL(itemSelectionChanged()), SLOT(patchChanged()));
      
      changeInstrument();
      
      connect(viewController, SIGNAL(itemSelectionChanged()), SLOT(controllerChanged()));
      
      connect(instrumentName, SIGNAL(returnPressed()), SLOT(instrumentNameReturn()));
      connect(instrumentName, SIGNAL(lostFocus()), SLOT(instrumentNameReturn()));
      
      connect(patchNameEdit, SIGNAL(returnPressed()), SLOT(patchNameReturn()));
      connect(patchNameEdit, SIGNAL(lostFocus()), SLOT(patchNameReturn()));
      connect(patchDelete, SIGNAL(clicked()), SLOT(deletePatchClicked()));
      connect(patchNew, SIGNAL(clicked()), SLOT(newPatchClicked()));
      connect(patchNewGroup, SIGNAL(clicked()), SLOT(newGroupClicked()));
      
      connect(patchButton, SIGNAL(clicked()), SLOT(patchButtonClicked()));
      connect(defPatchH, SIGNAL(valueChanged(int)), SLOT(defPatchChanged(int)));
      connect(defPatchL, SIGNAL(valueChanged(int)), SLOT(defPatchChanged(int)));
      connect(defPatchProg, SIGNAL(valueChanged(int)), SLOT(defPatchChanged(int)));
      connect(deleteController, SIGNAL(clicked()), SLOT(deleteControllerClicked()));
      connect(newController, SIGNAL(clicked()), SLOT(newControllerClicked()));
      connect(addController, SIGNAL(clicked()), SLOT(addControllerClicked()));
      connect(listController, SIGNAL(itemDoubleClicked(QListWidgetItem*)), SLOT(addControllerClicked()));
      connect(ctrlType,SIGNAL(activated(int)), SLOT(ctrlTypeChanged(int)));
      connect(ctrlName, SIGNAL(returnPressed()), SLOT(ctrlNameReturn()));
      connect(ctrlName, SIGNAL(lostFocus()), SLOT(ctrlNameReturn()));
      connect(spinBoxHCtrlNo, SIGNAL(valueChanged(int)), SLOT(ctrlHNumChanged(int)));
      connect(spinBoxLCtrlNo, SIGNAL(valueChanged(int)), SLOT(ctrlLNumChanged(int)));
      connect(spinBoxMin, SIGNAL(valueChanged(int)), SLOT(ctrlMinChanged(int)));
      connect(spinBoxMax, SIGNAL(valueChanged(int)), SLOT(ctrlMaxChanged(int)));
      connect(spinBoxDefault, SIGNAL(valueChanged(int)), SLOT(ctrlDefaultChanged(int)));
      connect(nullParamSpinBoxH, SIGNAL(valueChanged(int)), SLOT(ctrlNullParamHChanged(int)));
      connect(nullParamSpinBoxL, SIGNAL(valueChanged(int)), SLOT(ctrlNullParamLChanged(int)));
      
      connect(tabWidget3, SIGNAL(currentChanged(QWidget*)), SLOT(tabChanged(QWidget*)));
      //connect(sysexList, SIGNAL(selectionChanged()), SLOT(sysexChanged())); DELETETHIS or is it needed later?
      //connect(deleteSysex, SIGNAL(clicked()), SLOT(deleteSysexClicked()));
      //connect(newSysex, SIGNAL(clicked()), SLOT(newSysexClicked()));
      }

//---------------------------------------------------------
//   helpWhatsThis
//---------------------------------------------------------

void EditInstrument::helpWhatsThis()
{
  whatsThis();
}

//---------------------------------------------------------
//   fileNew
//---------------------------------------------------------

void EditInstrument::fileNew()
      {
      // Allow these to update...
      instrumentNameReturn();
      patchNameReturn();
      ctrlNameReturn();
      
      for (int i = 1;; ++i) {
            QString s = QString("Instrument-%1").arg(i);
            bool found = false;
            for (MusECore::iMidiInstrument i = MusECore::midiInstruments.begin(); i != MusECore::midiInstruments.end(); ++i) {
                  if (s == (*i)->iname()) {
                        found = true;
                        break;
                        }
                  }
            if (!found) {
                        MusECore::MidiInstrument* oi = 0;
                        if(oldMidiInstrument)
                          oi = (MusECore::MidiInstrument*)oldMidiInstrument->data(Qt::UserRole).value<void*>();
                        MusECore::MidiInstrument* wip = &workingInstrument;
                        if(checkDirty(wip)) // No save was chosen. Restore the actual instrument name.
                        {
                          if(oi)
                          {
                            oldMidiInstrument->setText(oi->iname());
                            
                            // No file path? Only a new unsaved instrument can do that. So delete it.
                            if(oi->filePath().isEmpty())
                              // Delete the list item and the instrument.
                              deleteInstrument(oldMidiInstrument);
                            
                          }  
                        }
                        workingInstrument.setDirty(false);
                        
                  MusECore::MidiInstrument* ni = new MusECore::MidiInstrument(s);
                  MusECore::midiInstruments.push_back(ni);
                  QListWidgetItem* item = new QListWidgetItem(ni->iname());
                  
                  workingInstrument.assign( *ni );

                  QVariant v = qVariantFromValue((void*)(ni));
                  item->setData(Qt::UserRole, v);
                  instrumentList->addItem(item);
                  
                  oldMidiInstrument = 0;
                  
                  instrumentList->blockSignals(true);
                  instrumentList->setCurrentItem(item);
                  instrumentList->blockSignals(false);
                  
                  changeInstrument();
                  
                  // We have our new instrument! So set dirty to true.
                  workingInstrument.setDirty(true);
                  
                  break;
                  }
            }

      }

//---------------------------------------------------------
//   fileOpen
//---------------------------------------------------------

void EditInstrument::fileOpen() //DELETETHIS?
      {
      }

//---------------------------------------------------------
//   fileSave
//---------------------------------------------------------

void EditInstrument::fileSave()
{
      if (workingInstrument.filePath().isEmpty())
      {
        saveAs();
        return;
      }      
      
      // Do not allow a direct overwrite of a 'built-in' muse instrument.
      QFileInfo qfi(workingInstrument.filePath());
      if(qfi.absolutePath() == MusEGlobal::museInstruments)
      {
        saveAs();
        return;
      }
      
      FILE* f = fopen(workingInstrument.filePath().toLatin1().constData(), "w");
      if(f == 0)
      {
        saveAs();
        return;
      }  
      
      // Allow these to update...
      instrumentNameReturn();
      patchNameReturn();
      ctrlNameReturn();
      
      if(fclose(f) != 0)
      {
        QString s = QString("Creating file:\n") + workingInstrument.filePath() + QString("\nfailed: ")
          + QString(strerror(errno) );
        QMessageBox::critical(this, tr("MusE: Create file failed"), s);
        return;
      }
      
      if(fileSave(&workingInstrument, workingInstrument.filePath()))
        workingInstrument.setDirty(false);
}

//---------------------------------------------------------
//   fileSave
//---------------------------------------------------------

bool EditInstrument::fileSave(MusECore::MidiInstrument* instrument, const QString& name)
    {
      //QFile f(name);                                  DELETETHIS
      //if (!f.open(QIODevice::WriteOnly)) {
      //      QString s("Creating file failed: ");
      //      s += strerror(errno);
      //      QMessageBox::critical(this,
      //         tr("MusE: Create file failed"), s);
      //      return false;
      //      }
      //MusECore::Xml xml(&f);
      
      FILE* f = fopen(name.toAscii().constData(), "w");
      if(f == 0)
      {
        QString s("Creating file failed: ");
        s += QString(strerror(errno));
        QMessageBox::critical(this,
            tr("MusE: Create file failed"), s);
        return false;
      }
            
      MusECore::Xml xml(f);
      
      updateInstrument(instrument);
      
      instrument->write(0, xml);
      
      // Assign the working instrument values to the actual current selected instrument...
      if(oldMidiInstrument)
      {
        MusECore::MidiInstrument* oi = (MusECore::MidiInstrument*)oldMidiInstrument->data(Qt::UserRole).value<void*>();
        if(oi)
        {
          oi->assign(workingInstrument);
          
          // Now signal the rest of the app so stuff can change...
          MusEGlobal::song->update(SC_CONFIG | SC_MIDI_CONTROLLER);
        }  
      }
      
      if(fclose(f) != 0)
      {
            QString s = QString("Write File\n") + name + QString("\nfailed: ")
               + QString(strerror(errno));
            QMessageBox::critical(this, tr("MusE: Write File failed"), s);
            return false;
      }
      return true;
    }

//---------------------------------------------------------
//   saveAs
//---------------------------------------------------------

void EditInstrument::saveAs()
    {
      // Allow these to update...
      instrumentNameReturn();
      patchNameReturn();
      ctrlNameReturn();
      
      //QListWidgetItem* item = instrumentList->currentItem();              DELETETHIS
//      ListBoxData* item = (ListBoxData*)instrumentList->selectedItem();
//      if (item == 0)
//            return;
      //MusECore::MidiInstrument* instrument = (MusECore::MidiInstrument*)item->data(Qt::UserRole).value<void*>();
//      MusECore::MidiInstrument* instrument = (MusECore::MidiInstrument*)item->data();
      
      //QString path = QDir::homePath() + "/" + MusEGlobal::config.instrumentPath;
      //QString path = QDir::homeDirPath() + "/" + MusEGlobal::museGlobalShare;
      //QString path = MusEGlobal::museInstruments;
      QString path = MusEGlobal::museUserInstruments;
      
      if(!QDir(MusEGlobal::museUserInstruments).exists())
      {
        printf("MusE Error! User instrument directory: %s does not exist. Should be created at startup!\n", MusEGlobal::museUserInstruments.toLatin1().constData());
        
        //path = MusEGlobal::museUser; DELETETHIS?
        //path = MusEGlobal::configPath;  
      }
        
      if (workingInstrument.filePath().isEmpty())
            path += QString("/%1.idf").arg(workingInstrument.iname());
      else {
            QFileInfo fi(workingInstrument.filePath());
            
            // Prompt for a new instrument name if the name has not been changed, to avoid duplicates.
            if(oldMidiInstrument)
            {
              MusECore::MidiInstrument* oi = (MusECore::MidiInstrument*)oldMidiInstrument->data(Qt::UserRole).value<void*>();
              if(oi)
              {
                if(oi->iname() == workingInstrument.iname())
                {
                  // Prompt only if it's a user instrument, to avoid duplicates in the user instrument dir.
                  // This will still allow a user instrument to override a built-in instrument with the same name.
                  if(fi.absolutePath() != MusEGlobal::museInstruments)
                  {
                    //QMessageBox::critical(this,             DELETETHIS???
                    //    tr("MusE: Bad instrument name"),
                    //    tr("Please change the instrument name to a new unique name before saving, to avoid duplicates"),
                    //    QMessageBox::Ok,
                    //    QMessageBox::NoButton,
                    //    QMessageBox::NoButton);
                    printf("EditInstrument::saveAs Error: Instrument name %s already used!\n", workingInstrument.iname().toLatin1().constData());
                    return;    
                  }  
                }
              }
            }  
            path += QString("/%1.idf").arg(fi.baseName());
           }

      QString s = QFileDialog::getSaveFileName(this, tr("MusE: Save Instrument Definition").toLatin1().constData(), 
         path, tr("Instrument Definition (*.idf)"));
      if (s.isEmpty())
            return;
      workingInstrument.setFilePath(s);
      
      if(fileSave(&workingInstrument, s))
        workingInstrument.setDirty(false);
    }

//---------------------------------------------------------
//   fileSaveAs
//---------------------------------------------------------

void EditInstrument::fileSaveAs()
    {
      // Is this a new unsaved instrument? Just do a normal save.
      if(workingInstrument.filePath().isEmpty())
      {
        saveAs();
        return;
      }      
      
      // Allow these to update...
      instrumentNameReturn();
      patchNameReturn();
      ctrlNameReturn();
      
      MusECore::MidiInstrument* oi = 0;
      if(oldMidiInstrument)
        oi = (MusECore::MidiInstrument*)oldMidiInstrument->data(Qt::UserRole).value<void*>();
        
      int res = checkDirty(&workingInstrument, true);
      switch(res)
      {
        // No save:
        case 1:
          workingInstrument.setDirty(false);
          if(oi)
          {
            oldMidiInstrument->setText(oi->iname());
            
            // No file path? Only a new unsaved instrument can do that. So delete it.
            if(oi->filePath().isEmpty())
            {
              // Delete the list item and the instrument.
              deleteInstrument(oldMidiInstrument);
              oldMidiInstrument = 0;
            }
            
            changeInstrument();
            
          }
          return;
        break;
        
        // Abort:
        case 2: 
          return;
        break;
          
        // Save:
        case 0:
            workingInstrument.setDirty(false);
        break;
      }
      
      //QListWidgetItem* item = instrumentList->currentItem(); DELETETHIS
//      ListBoxData* item = (ListBoxData*)instrumentList->selectedItem();
//      if (item == 0)
//            return;
      //MusECore::MidiInstrument* instrument = (MusECore::MidiInstrument*)item->data(Qt::UserRole).value<void*>();
//      MusECore::MidiInstrument* instrument = (MusECore::MidiInstrument*)item->data();
      
      bool isuser = false;
      QString so;
      if(workingInstrument.iname().isEmpty())
        so = QString("Instrument");
      else  
        so = workingInstrument.iname();
        
      for(int i = 1;; ++i) 
      {
        QString s = so + QString("-%1").arg(i);
        
        bool found = false;
        for(MusECore::iMidiInstrument imi = MusECore::midiInstruments.begin(); imi != MusECore::midiInstruments.end(); ++imi) 
        {
          if(s == (*imi)->iname()) 
          {
            // Allow override of built-in instrument names.
            QFileInfo fi((*imi)->filePath());
            if(fi.absolutePath() == MusEGlobal::museInstruments)
              break;
            found = true;
            break;
          }
        }
        if(found) 
          continue;  
        
        bool ok;
        s = QInputDialog::getText(this, tr("MusE: Save instrument as"), tr("Enter a new unique instrument name:"), 
                                  QLineEdit::Normal, s, &ok);
        if(!ok) 
          return;
        if(s.isEmpty())
        {
          --i;
          continue;  
        }  
          
        isuser = false;
        bool builtin = false;
        found = false;
        for(MusECore::iMidiInstrument imi = MusECore::midiInstruments.begin(); imi != MusECore::midiInstruments.end(); ++imi) 
        {
          // If an instrument with the same name is found...
          if((*imi)->iname() == s) 
          {
            // If it's not the same name as the working instrument, and it's not an internal instrument (soft synth etc.)...
            if(s != workingInstrument.iname() && !(*imi)->filePath().isEmpty())
            {
              QFileInfo fi((*imi)->filePath());
              // Allow override of built-in and user instruments:
              // If it's a user instrument, not a built-in instrument...
              if(fi.absolutePath() == MusEGlobal::museUserInstruments)
              {
                // No choice really but to overwrite the destination instrument file!
                // Can not have two user files containing the same instrument name.
                if(QMessageBox::question(this,
                    tr("MusE: Save instrument as"),
                    tr("The user instrument '%1' already exists. This will overwrite its .idf instrument file.\nAre you sure?").arg(s),
                    QMessageBox::Ok | QMessageBox::Default,
                    QMessageBox::Cancel | QMessageBox::Escape,
                    Qt::NoButton) == QMessageBox::Ok)
                {
                  // Set the working instrument's file path to the found instrument's path.
                  workingInstrument.setFilePath((*imi)->filePath());
                  // Mark as overwriting a user instrument.
                  isuser = true;
                }  
                else
                {
                  found = true;
                  break;
                }
              }

              // Assign the found instrument name to the working instrument name.
              workingInstrument.setIName(s);
              
              // Find the instrument in the list and set the old instrument to the item.
              oldMidiInstrument = instrumentList->findItems(s, Qt::MatchExactly)[0];
              
              // Mark as a built-in instrument.
              builtin = true;
              break;
            }  
            found = true;
            break;
          }
        }
        if(found)
        { 
          so = s;
          i = 0;
          continue;  
        }  
        
        so = s;
        
        // If the name does not belong to a built-in instrument...
        if(!builtin)
        {
          MusECore::MidiInstrument* ni = new MusECore::MidiInstrument();
          ni->assign(workingInstrument);
          ni->setIName(so);
          ni->setFilePath(QString());
          MusECore::midiInstruments.push_back(ni);
          QListWidgetItem* item = new QListWidgetItem(so);
          
          workingInstrument.assign( *ni );

          QVariant v = qVariantFromValue((void*)(ni));
          item->setData(Qt::UserRole, v);
          instrumentList->addItem(item);
          
          oldMidiInstrument = 0;
          
          instrumentList->blockSignals(true);
          instrumentList->setCurrentItem(item);
          instrumentList->blockSignals(false);
          
          changeInstrument();
          
          // We have our new instrument! So set dirty to true.
          workingInstrument.setDirty(true);
        }  
          
        break;
      }
      
      QString path = MusEGlobal::museUserInstruments;
      
      if(!QDir(MusEGlobal::museUserInstruments).exists())
      {
        printf("MusE Error! User instrument directory: %s does not exist. Should be created at startup!\n", MusEGlobal::museUserInstruments.toLatin1().constData());
        
        //path = MusEGlobal::museUser; DELETETHIS
        //path = MusEGlobal::configPath;  
      }
      path += QString("/%1.idf").arg(so);
            
      QString sfn;
      // If we are overwriting a user instrument just force the path.
      if(isuser)
        sfn = path;
      else  
      {
        sfn = QFileDialog::getSaveFileName(this, tr("MusE: Save Instrument Definition").toLatin1().constData(),
           path, tr("Instrument Definition (*.idf)"));
        if (sfn.isEmpty())
              return;
        workingInstrument.setFilePath(sfn);
      }  
      
      if(fileSave(&workingInstrument, sfn))
        workingInstrument.setDirty(false);
    }

//---------------------------------------------------------
//   fileExit
//---------------------------------------------------------

void EditInstrument::fileExit()
      {

      }

//---------------------------------------------------------
//   closeEvent
//---------------------------------------------------------

void EditInstrument::closeEvent(QCloseEvent* ev)
      {
      // Allow these to update...
      instrumentNameReturn();
      patchNameReturn();
      ctrlNameReturn();
      
      //QListWidgetItem* item = instrumentList->currentItem(); DELETETHIS
      
//      ListBoxData* item = (ListBoxData*)instrumentList->selectedItem();
      
//      if(item)
//      {
        //MusECore::MidiInstrument* instrument = (MusECore::MidiInstrument*)item->data(Qt::UserRole).value<void*>();
//        MusECore::MidiInstrument* instrument = (MusECore::MidiInstrument*)item->data();
//        int res = checkDirty(instrument, true);
        MusECore::MidiInstrument* oi = 0;
        if(oldMidiInstrument)
          oi = (MusECore::MidiInstrument*)oldMidiInstrument->data(Qt::UserRole).value<void*>();
          
        int res = checkDirty(&workingInstrument, true);
        switch(res)
        {
          // No save:
          case 1:
            workingInstrument.setDirty(false);
            if(oi)
            {
              oldMidiInstrument->setText(oi->iname());
              
              // No file path? Only a new unsaved instrument can do that. So delete it.
              if(oi->filePath().isEmpty())
              {
                // Delete the list item and the instrument.
                deleteInstrument(oldMidiInstrument);
                oldMidiInstrument = 0;
              }
              
              changeInstrument();
              
            }  
          break;
          
          // Abort:
          case 2: 
            ev->ignore();
            return;
          break;
            
          // Save:
          case 0:
              workingInstrument.setDirty(false);
          break;
          
        }
      
      QMainWindow::closeEvent(ev);
      }

//---------------------------------------------------------
//   changeInstrument
//---------------------------------------------------------

void EditInstrument::changeInstrument()
{
  QListWidgetItem* sel = instrumentList->currentItem();

  if(!sel)
    return;

  oldMidiInstrument = sel;
  // Assignment
  
  // Assign will 'delete' any existing patches, groups, or controllers.
  workingInstrument.assign( *((MusECore::MidiInstrument*)sel->data(Qt::UserRole).value<void*>()) );
  
  workingInstrument.setDirty(false);
  
  // populate patch list
  patchView->blockSignals(true);
  for (int i = 0; i < patchView->topLevelItemCount(); ++i)
    qDeleteAll(patchView->topLevelItem(i)->takeChildren());
  patchView->clear();
  patchView->blockSignals(false);

  for (int i = 0; i < viewController->topLevelItemCount(); ++i)
    qDeleteAll(viewController->topLevelItem(i)->takeChildren());
  viewController->clear();

  instrumentName->blockSignals(true);
  instrumentName->setText(workingInstrument.iname());
  instrumentName->blockSignals(false);
  
  nullParamSpinBoxH->blockSignals(true);
  nullParamSpinBoxL->blockSignals(true);
  int nv = workingInstrument.nullSendValue();
  if(nv == -1)
  {
    nullParamSpinBoxH->setValue(-1);
    nullParamSpinBoxL->setValue(-1);
  }  
  else
  {
    int nvh = (nv >> 8) & 0xff;
    int nvl = nv & 0xff;
    if(nvh == 0xff)
      nullParamSpinBoxH->setValue(-1);
    else  
      nullParamSpinBoxH->setValue(nvh & 0x7f);
    if(nvl == 0xff)
      nullParamSpinBoxL->setValue(-1);
    else  
      nullParamSpinBoxL->setValue(nvl & 0x7f);
  }
  nullParamSpinBoxH->blockSignals(false);
  nullParamSpinBoxL->blockSignals(false);
  
  MusECore::PatchGroupList* pg = workingInstrument.groups();
  for (MusECore::ciPatchGroup g = pg->begin(); g != pg->end(); ++g) {
        MusECore::PatchGroup* pgp = *g; 
        if(pgp)
        {
          QTreeWidgetItem* item = new QTreeWidgetItem(patchView);
          
          item->setText(0, pgp->name);
          QVariant v = qVariantFromValue((void*)(pgp));
          item->setData(0, Qt::UserRole, v);
          
          for (MusECore::ciPatch p = pgp->patches.begin(); p != pgp->patches.end(); ++p) 
          {
            MusECore::Patch* patch = *p;
            if(patch)
            {
              QTreeWidgetItem* sitem = new QTreeWidgetItem(item);
              sitem->setText(0, patch->name);
              QVariant v = QVariant::fromValue((void*)patch);
              sitem->setData(0, Qt::UserRole, v);
            }  
          }  
        }
      }  
  
  oldPatchItem = 0;
  
  QTreeWidgetItem* fc = patchView->topLevelItem(0);
  if(fc)
  {
    // This may cause a patchChanged call.
    //if(patchView->selectedItem() != fc) DELETETHIS
    patchView->blockSignals(true);
    fc->setSelected(true);
    patchView->blockSignals(false);
    //else  
    //  patchChanged(); DELETETHIS

    //patchView->firstChild()->setSelected(true); DELETETHIS
    //patchView->triggerUpdate(true);
  }
      
  patchChanged();
  
//      oldPatchItem = (ListViewData*)patchView->selectedItem(); DELETETHIS
      //patchChanged();
//      if(oldPatchItem)
//      {
//        if(oldPatchItem->parent())
//          patchNameEdit->setText( ((MusECore::Patch*)oldPatchItem->data())->name );
//        else  
//          patchNameEdit->setText( ((MusECore::PatchGroup*)oldPatchItem->data())->name );
//      }    
      
  //MusECore::MidiControllerList* cl = instrument->controller();
  
  MusECore::MidiControllerList* cl = workingInstrument.controller();
  for (MusECore::ciMidiController ic = cl->begin(); ic != cl->end(); ++ic) {
        MusECore::MidiController* c = ic->second;
        //QListWidgetItem* item = new QListWidgetItem(c->name()); DELETETHIS
     //   ListBoxData* item = new ListBoxData(c->name());
        //QVariant v = QVariant::fromValue((void*)c);
        //item->setData(Qt::UserRole, v);
    //    item->setData((void*)c);
    //    listController->insertItem(item);
        
        addControllerToView(c);
        }
  
  
  //listController->setItemSelected(listController->item(0), true); DELETETHIS
  
//  oldController = 0;
  
  //ListBoxData* ci = (ListBoxData*)listController->item(0);

  QTreeWidgetItem *ci = viewController->topLevelItem(0);

  if(ci)
  {
    // This may cause a controllerChanged call. DELETETHIS
    //if(listController->selectedItem != ci)
  //  listController->blockSignals(true);
  //  listController->setSelected(ci, true);
  //  listController->blockSignals(false);
    //else  
    //  controllerChanged();
    
    viewController->blockSignals(true);
    ci->setSelected(true);
    viewController->blockSignals(false);
  }  
  
  controllerChanged();
  
  //oldController = (ListBoxData*)listController->selectedItem(); DELETETHIS
  
  
  //controllerChanged(listController->item(0), 0);
  //controllerChanged();
  
/* DELETETHIS
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
*/


}

//---------------------------------------------------------
//   instrumentChanged
//---------------------------------------------------------

void EditInstrument::instrumentChanged()
      {
      QListWidgetItem* sel = instrumentList->currentItem();

      if(!sel)
        return;
           
        MusECore::MidiInstrument* oi = 0;
        if(oldMidiInstrument)
          oi = (MusECore::MidiInstrument*)oldMidiInstrument->data(Qt::UserRole).value<void*>();
        MusECore::MidiInstrument* wip = &workingInstrument;
        // Returns true if aborted.
        if(checkDirty(wip))
        {
          // No save was chosen. Abandon changes, or delete if it is new...
          if(oi)
          {
            oldMidiInstrument->setText(oi->iname());
            
            // No file path? Only a new unsaved instrument can do that. So delete it.
            if(oi->filePath().isEmpty())
            {
              // Delete the list item and the instrument.
              deleteInstrument(oldMidiInstrument);
              oldMidiInstrument = 0;
            }
            
          }  
        }
        workingInstrument.setDirty(false);

        changeInstrument();
      }

//---------------------------------------------------------
//   instrumentNameReturn
//---------------------------------------------------------

void EditInstrument::instrumentNameReturn()
{
  QListWidgetItem* item = instrumentList->currentItem();

  if (item == 0)
        return;
  QString s = instrumentName->text();
  
  if(s == item->text()) 
    return;
  
  MusECore::MidiInstrument* curins = (MusECore::MidiInstrument*)item->data(Qt::UserRole).value<void*>();
  
  for(MusECore::iMidiInstrument i = MusECore::midiInstruments.begin(); i != MusECore::midiInstruments.end(); ++i) 
  {
    if((*i) != curins && s == (*i)->iname()) 
    {
      instrumentName->blockSignals(true);
      // Grab the last valid name from the item text, since the instrument has not been updated yet.
      instrumentName->setText(item->text());
      instrumentName->blockSignals(false);
      
      QMessageBox::critical(this,
          tr("MusE: Bad instrument name"),
          tr("Please choose a unique instrument name.\n(The name might be used by a hidden instrument.)"),
          QMessageBox::Ok,
          Qt::NoButton,
          Qt::NoButton);
          
      return;
    }
  }      
  
  item->setText(s);
  workingInstrument.setIName(s);
  workingInstrument.setDirty(true);
}

//---------------------------------------------------------
//   deleteInstrument
//---------------------------------------------------------

void EditInstrument::deleteInstrument(QListWidgetItem* item)
{
  if(!item)
    return;

  MusECore::MidiInstrument* ins = (MusECore::MidiInstrument*)item->data(Qt::UserRole).value<void*>();
  
  // Delete the list item.
  // Test this: Is this going to change the current selection?
  instrumentList->blockSignals(true);
  delete item;
  instrumentList->blockSignals(false);
  
  // Test this: Neccessary? DELETETHIS
  // if(curritem)
  //  instrumentList->setCurrentItem(curritem);
  
  if(!ins)
    return;
        
  // Remove the instrument from the list.
  MusECore::midiInstruments.remove(ins);
  
  // Delete the instrument.
  delete ins;
}

//---------------------------------------------------------
//   tabChanged
//   Added so that patch list is updated when switching tabs, 
//    so that 'Program' default values and text are current in controller tab. 
//---------------------------------------------------------

void EditInstrument::tabChanged(QWidget* w)
{
  if(!w)
    return;
    
  // If we're switching to the Patches tab, just ignore.
  if(QString(w->objectName()) == QString("patchesTab"))
    return;
    
  if(oldPatchItem)
  {
    // Don't bother calling patchChanged, just update the patch or group.
    if(oldPatchItem->QTreeWidgetItem::parent())
      updatePatch(&workingInstrument, (MusECore::Patch*)oldPatchItem->data(0, Qt::UserRole).value<void*>());
    else
      updatePatchGroup(&workingInstrument, (MusECore::PatchGroup*)oldPatchItem->data(0, Qt::UserRole).value<void*>());
  }
  
  // We're still on the same item. No need to set oldPatchItem as in patchChanged...
  
  // If we're switching to the Controller tab, update the default patch button text in case a patch changed...
  if(QString(w->objectName()) == QString("controllerTab"))
  {
    QTreeWidgetItem* sel = viewController->currentItem();
        
    if(!sel || !sel->data(0, Qt::UserRole).value<void*>()) 
      return;
        
    MusECore::MidiController* c = (MusECore::MidiController*)sel->data(0, Qt::UserRole).value<void*>();
    MusECore::MidiController::ControllerType type = MusECore::midiControllerType(c->num());
        
    // Grab the controller number from the actual values showing
    //  and set the patch button text.
    if(type == MusECore::MidiController::Program)
      setDefaultPatchName(getDefaultPatchNumber());
  }  
}

//---------------------------------------------------------
//   patchNameReturn
//---------------------------------------------------------

void EditInstrument::patchNameReturn()
{
  QTreeWidgetItem* item = patchView->currentItem();
  
  if (item == 0)
        return;
  
  QString s = patchNameEdit->text();
  
  if(item->text(0) == s)
    return;
  
  MusECore::PatchGroupList* pg = workingInstrument.groups();
  for(MusECore::iPatchGroup g = pg->begin(); g != pg->end(); ++g) 
  {
    MusECore::PatchGroup* pgp = *g;
    // If the item has a parent, it's a patch item.
    if(item->QTreeWidgetItem::parent())
    {
      MusECore::Patch* curp = (MusECore::Patch*)item->data(0, Qt::UserRole).value<void*>();
      for(MusECore::iPatch p = pgp->patches.begin(); p != pgp->patches.end(); ++p) 
      {
        if((*p) != curp && (*p)->name == s) 
        {
          patchNameEdit->blockSignals(true);
          // Grab the last valid name from the item text, since the patch has not been updated yet.
          patchNameEdit->setText(item->text(0));
          patchNameEdit->blockSignals(false);
          
          QMessageBox::critical(this,
              tr("MusE: Bad patch name"),
              tr("Please choose a unique patch name"),
              QMessageBox::Ok,
              Qt::NoButton,
              Qt::NoButton);
              
          return;
        }
      }  
    }
    else
    // The item has no parent. It's a patch group item.
    {
      MusECore::PatchGroup* curpg = (MusECore::PatchGroup*)item->data(0, Qt::UserRole).value<void*>();
      if(pgp != curpg && pgp->name == s) 
      {
        patchNameEdit->blockSignals(true);
        // Grab the last valid name from the item text, since the patch group has not been updated yet.
        patchNameEdit->setText(item->text(0));
        patchNameEdit->blockSignals(false);
        
        QMessageBox::critical(this,
            tr("MusE: Bad patchgroup name"),
            tr("Please choose a unique patchgroup name"),
            QMessageBox::Ok,
            Qt::NoButton,
            Qt::NoButton);
            
        return;
      }
    }
  }
  
    item->setText(0, s);
    workingInstrument.setDirty(true);
    
  // DELETETHIS
  // Since the name of the patch/group in the working instrument will be updated later,
  //  there's no need to do manually set the name here now. 
  /*
  // If the item has a parent, it's a patch item.
  if(item->parent())
  {
    MusECore::Patch* p = item->data();
    if(s != p->name)
    {
      item->setText(s);
      p->name = s;
      workingInstrument.setDirty(true);
      //patchView->triggerUpdate(true);
    }
  }
  else
  // The item has no parent. It's a patch group item.
  {
    MusECore::PatchGroup* pg = (MusECore::PatchGroup*)item->data();
    if(s != pg->name)
    {
      item->setText(s);
      pg->name = s;
      workingInstrument.setDirty(true);
      //patchView->triggerUpdate(true);
    }
  }
  */
}

//---------------------------------------------------------
//   patchChanged
//---------------------------------------------------------
void EditInstrument::patchChanged()
    {
      if(oldPatchItem)
      {
            if(oldPatchItem->parent())
                    updatePatch(&workingInstrument, (MusECore::Patch*)oldPatchItem->data(0, Qt::UserRole).value<void*>());
            else
                    updatePatchGroup(&workingInstrument, (MusECore::PatchGroup*)oldPatchItem->data(0, Qt::UserRole).value<void*>());
      }
      
      QTreeWidgetItem* sel = patchView->selectedItems().size() ? patchView->selectedItems()[0] : 0;
      oldPatchItem = sel;
      
      if(!sel || !sel->data(0, Qt::UserRole).value<void*>())
      {
        patchNameEdit->setText("");
        spinBoxHBank->setEnabled(false);
        spinBoxLBank->setEnabled(false);
        spinBoxProgram->setEnabled(false);
        checkBoxDrum->setEnabled(false);
        checkBoxGM->setEnabled(false);
        checkBoxGS->setEnabled(false);
        checkBoxXG->setEnabled(false);
        return;
      }
      
      // If the item has a parent, it's a patch item.
      if(sel->parent())
      {
        MusECore::Patch* p = (MusECore::Patch*)sel->data(0, Qt::UserRole).value<void*>();
        patchNameEdit->setText(p->name);
        spinBoxHBank->setEnabled(true);
        spinBoxLBank->setEnabled(true);
        spinBoxProgram->setEnabled(true);
        checkBoxDrum->setEnabled(true);
        checkBoxGM->setEnabled(true);
        checkBoxGS->setEnabled(true);
        checkBoxXG->setEnabled(true);
        
        int hb = ((p->hbank + 1) & 0xff);
        int lb = ((p->lbank + 1) & 0xff);
        int pr = ((p->prog + 1) & 0xff);
        spinBoxHBank->setValue(hb);
        spinBoxLBank->setValue(lb);
        spinBoxProgram->setValue(pr);
        checkBoxDrum->setChecked(p->drum);
        checkBoxGM->setChecked(p->typ & 1);
        checkBoxGS->setChecked(p->typ & 2);
        checkBoxXG->setChecked(p->typ & 4);
      }  
      else
      // The item is a patch group item.
      {
        patchNameEdit->setText( ((MusECore::PatchGroup*)sel->data(0, Qt::UserRole).value<void*>())->name );
        spinBoxHBank->setEnabled(false);
        spinBoxLBank->setEnabled(false);
        spinBoxProgram->setEnabled(false);
        checkBoxDrum->setEnabled(false);
        checkBoxGM->setEnabled(false);
        checkBoxGS->setEnabled(false);
        checkBoxXG->setEnabled(false);
      }  
    }

//---------------------------------------------------------
//   defPatchChanged
//---------------------------------------------------------

void EditInstrument::defPatchChanged(int)
{
      QTreeWidgetItem* item = viewController->currentItem();
      
      if (!item)
            return;
        
      MusECore::MidiController* c = (MusECore::MidiController*)item->data(0, Qt::UserRole).value<void*>();
      
      int val = getDefaultPatchNumber();
      
      c->setInitVal(val);
      
      setDefaultPatchName(val);
      
      item->setText(COL_DEF, getPatchItemText(val));
      workingInstrument.setDirty(true);
}

//---------------------------------------------------------
//   patchButtonClicked
//---------------------------------------------------------

void EditInstrument::patchButtonClicked()
{
      QMenu* patchpopup = new QMenu;
      
      MusECore::PatchGroupList* pg = workingInstrument.groups();
      
      if (pg->size() > 1) {
            for (MusECore::ciPatchGroup i = pg->begin(); i != pg->end(); ++i) {
                  MusECore::PatchGroup* pgp = *i;
                  QMenu* pm = patchpopup->addMenu(pgp->name);
                  //pm->setCheckable(false);//Qt4 doc says this is unnecessary
                  pm->setFont(MusEGlobal::config.fonts[0]);
                  const MusECore::PatchList& pl = pgp->patches;
                  for (MusECore::ciPatch ipl = pl.begin(); ipl != pl.end(); ++ipl) {
                        const MusECore::Patch* mp = *ipl;
                              int id = ((mp->hbank & 0xff) << 16)
                                         + ((mp->lbank & 0xff) << 8) + (mp->prog & 0xff);
                              QAction *ac1 = pm->addAction(mp->name);
                              ac1->setData(id);
                        }
                  }
            }
      else if (pg->size() == 1 ){
            // no groups
            const MusECore::PatchList& pl = pg->front()->patches;
            for (MusECore::ciPatch ipl = pl.begin(); ipl != pl.end(); ++ipl) {
                  const MusECore::Patch* mp = *ipl;
                        int id = ((mp->hbank & 0xff) << 16)
                                 + ((mp->lbank & 0xff) << 8) + (mp->prog & 0xff);
                        QAction *ac2 = patchpopup->addAction(mp->name);
                        ac2->setData(id);
                  }
            }

      if(patchpopup->actions().count() == 0)
      {
        delete patchpopup;
        return;
      }
      
      QAction* act = patchpopup->exec(patchButton->mapToGlobal(QPoint(10,5)));
      if(!act)
      {
        delete patchpopup;
        return;
      }
      
      int rv = act->data().toInt();
      delete patchpopup;

      if (rv != -1) 
      {
        setDefaultPatchControls(rv);
        
        QTreeWidgetItem* item = viewController->currentItem();

        if(item)
        {
          MusECore::MidiController* c = (MusECore::MidiController*)item->data(0, Qt::UserRole).value<void*>();
          c->setInitVal(rv);
          
          item->setText(COL_DEF, getPatchItemText(rv));
        }
        workingInstrument.setDirty(true);
      }
            
}

//---------------------------------------------------------
//   addControllerToView
//---------------------------------------------------------

QTreeWidgetItem* EditInstrument::addControllerToView(MusECore::MidiController* mctrl)
{
      QString hnum;
      QString lnum;
      QString min;
      QString max;
      QString def;
      int defval = mctrl->initVal();
      int n = mctrl->num();
      int h = (n >> 8) & 0x7f;
      int l = n & 0x7f;
      if((n & 0xff) == 0xff)
        l = -1;
        
      MusECore::MidiController::ControllerType t = MusECore::midiControllerType(n);
      switch(t)
      {
          case MusECore::MidiController::Controller7:
                hnum = "---";
                if(l == -1)
                  lnum = "*";
                else  
                  lnum.setNum(l);
                min.setNum(mctrl->minVal());
                max.setNum(mctrl->maxVal());
                if(defval == MusECore::CTRL_VAL_UNKNOWN)
                  def = "---";
                else
                  def.setNum(defval);
                break;
          case MusECore::MidiController::RPN:
          case MusECore::MidiController::NRPN:
          case MusECore::MidiController::RPN14:
          case MusECore::MidiController::NRPN14:
          case MusECore::MidiController::Controller14:
                hnum.setNum(h);
                if(l == -1)
                  lnum = "*";
                else  
                  lnum.setNum(l);
                min.setNum(mctrl->minVal());
                max.setNum(mctrl->maxVal());
                if(defval == MusECore::CTRL_VAL_UNKNOWN)
                  def = "---";
                else
                  def.setNum(defval);
                break;
          case MusECore::MidiController::Pitch:
                hnum = "---";
                lnum = "---";
                min.setNum(mctrl->minVal());
                max.setNum(mctrl->maxVal());
                if(defval == MusECore::CTRL_VAL_UNKNOWN)
                  def = "---";
                else
                  def.setNum(defval);
                break;
          case MusECore::MidiController::Program:
                hnum = "---";
                lnum = "---";
                min = "---";
                max = "---";
                def = getPatchItemText(defval); 
                break;
                
          default:
                hnum = "---";
                lnum = "---";
                min = "---";
                max = "---";
                def = "---";
                break;
      }

      	QTreeWidgetItem* ci =  new QTreeWidgetItem(viewController, QStringList() <<  mctrl->name() << int2ctrlType(t) << hnum << lnum << min << max << def);
      QVariant v = qVariantFromValue((void*)(mctrl));
      ci->setData(0, Qt::UserRole, v);
      
      return ci;
}
      
//---------------------------------------------------------
//   controllerChanged
//---------------------------------------------------------

void EditInstrument::controllerChanged()
      {
	QTreeWidgetItem* sel = viewController->selectedItems().size() ? viewController->selectedItems()[0] : 0;
      
	if(!sel || !sel->data(0, Qt::UserRole).value<void*>()) 
      {
        ctrlName->blockSignals(true);
        ctrlName->setText("");
        ctrlName->blockSignals(false);
        return;
      }
      
      MusECore::MidiController* c = (MusECore::MidiController*)sel->data(0, Qt::UserRole).value<void*>();
      
      ctrlName->blockSignals(true);
      ctrlName->setText(c->name());
      ctrlName->blockSignals(false);
      
      int ctrlH = (c->num() >> 8) & 0x7f;
      int ctrlL = c->num() & 0x7f;
      if((c->num() & 0xff) == 0xff)
        ctrlL = -1;
        
      MusECore::MidiController::ControllerType type = MusECore::midiControllerType(c->num());
      
      ctrlType->blockSignals(true);
      ctrlType->setCurrentIndex(type);
      ctrlType->blockSignals(false);
      
      spinBoxHCtrlNo->blockSignals(true);
      spinBoxLCtrlNo->blockSignals(true);
      spinBoxMin->blockSignals(true);
      spinBoxMax->blockSignals(true);
      spinBoxDefault->blockSignals(true);
     
      switch (type) {
            case MusECore::MidiController::Controller7:
                  spinBoxHCtrlNo->setEnabled(false);
                  spinBoxLCtrlNo->setEnabled(true);
                  spinBoxHCtrlNo->setValue(0);
                  spinBoxLCtrlNo->setValue(ctrlL);
                  spinBoxMin->setEnabled(true);
                  spinBoxMax->setEnabled(true);
                  spinBoxMin->setRange(-128, 127);
                  spinBoxMax->setRange(-128, 127);
                  spinBoxMin->setValue(c->minVal());
                  spinBoxMax->setValue(c->maxVal());
                  enableDefaultControls(true, false);
                  break;
            case MusECore::MidiController::RPN:
            case MusECore::MidiController::NRPN:
                  spinBoxHCtrlNo->setEnabled(true);
                  spinBoxLCtrlNo->setEnabled(true);
                  spinBoxHCtrlNo->setValue(ctrlH);
                  spinBoxLCtrlNo->setValue(ctrlL);
                  spinBoxMin->setEnabled(true);
                  spinBoxMax->setEnabled(true);
                  spinBoxMin->setRange(-128, 127);
                  spinBoxMax->setRange(-128, 127);
                  spinBoxMin->setValue(c->minVal());
                  spinBoxMax->setValue(c->maxVal());
                  enableDefaultControls(true, false);
                  break;
            case MusECore::MidiController::Controller14:
            case MusECore::MidiController::RPN14:
            case MusECore::MidiController::NRPN14:
                  spinBoxHCtrlNo->setEnabled(true);
                  spinBoxLCtrlNo->setEnabled(true);
                  spinBoxHCtrlNo->setValue(ctrlH);
                  spinBoxLCtrlNo->setValue(ctrlL);
                  spinBoxMin->setEnabled(true);
                  spinBoxMax->setEnabled(true);
                  spinBoxMin->setRange(-16384, 16383);
                  spinBoxMax->setRange(-16384, 16383);
                  spinBoxMin->setValue(c->minVal());
                  spinBoxMax->setValue(c->maxVal());
                  enableDefaultControls(true, false);
                  break;
            case MusECore::MidiController::Pitch:
                  spinBoxHCtrlNo->setEnabled(false);
                  spinBoxLCtrlNo->setEnabled(false);
                  spinBoxHCtrlNo->setValue(0);
                  spinBoxLCtrlNo->setValue(0);
                  spinBoxMin->setEnabled(true);
                  spinBoxMax->setEnabled(true);
                  spinBoxMin->setRange(-8192, 8191);
                  spinBoxMax->setRange(-8192, 8191);
                  spinBoxMin->setValue(c->minVal());
                  spinBoxMax->setValue(c->maxVal());
                  enableDefaultControls(true, false);
                  break;
            case MusECore::MidiController::Program:
                  spinBoxHCtrlNo->setEnabled(false);
                  spinBoxLCtrlNo->setEnabled(false);
                  spinBoxHCtrlNo->setValue(0);
                  spinBoxLCtrlNo->setValue(0);
                  spinBoxMin->setEnabled(false);
                  spinBoxMax->setEnabled(false);
                  spinBoxMin->setRange(0, 0);
                  spinBoxMax->setRange(0, 0);
                  spinBoxMin->setValue(0);
                  spinBoxMax->setValue(0);
                  enableDefaultControls(false, true);
                  break;
            default:
                  spinBoxHCtrlNo->setEnabled(false);
                  spinBoxLCtrlNo->setEnabled(false);
                  spinBoxMin->setEnabled(false);
                  spinBoxMax->setEnabled(false);
                  enableDefaultControls(false, false);
                  break;
            }      
            
      if(type == MusECore::MidiController::Program)
      {
        spinBoxDefault->setRange(0, 0);
        spinBoxDefault->setValue(0);
        setDefaultPatchControls(c->initVal());
      }
      else
      {
        spinBoxDefault->setRange(c->minVal() - 1, c->maxVal());
        if(c->initVal() == MusECore::CTRL_VAL_UNKNOWN)
          spinBoxDefault->setValue(spinBoxDefault->minimum());
        else  
          spinBoxDefault->setValue(c->initVal());
      }
      
      spinBoxHCtrlNo->blockSignals(false);
      spinBoxLCtrlNo->blockSignals(false);
      spinBoxMin->blockSignals(false);
      spinBoxMax->blockSignals(false);
      spinBoxDefault->blockSignals(false);
      }

//---------------------------------------------------------
//   ctrlNameReturn
//---------------------------------------------------------

void EditInstrument::ctrlNameReturn()
{
      QTreeWidgetItem* item = viewController->currentItem();

      if (item == 0)
            return;
      MusECore::MidiController* c = (MusECore::MidiController*)item->data(0, Qt::UserRole).value<void*>();
      
      QString cName = ctrlName->text();
      
      if(c->name() == cName)
        return;

      MusECore::MidiControllerList* cl = workingInstrument.controller();
      for(MusECore::ciMidiController ic = cl->begin(); ic != cl->end(); ++ic) 
      {
        MusECore::MidiController* mc = ic->second;
        if(mc != c && mc->name() == cName) 
        {
          ctrlName->blockSignals(true);
          ctrlName->setText(c->name());
          ctrlName->blockSignals(false);
          
          QMessageBox::critical(this,
              tr("MusE: Bad controller name"),
              tr("Please choose a unique controller name"),
              QMessageBox::Ok,
              Qt::NoButton,
              Qt::NoButton);
              
          return;
        }
      }
      
      c->setName(ctrlName->text());
      item->setText(COL_NAME, ctrlName->text());
      workingInstrument.setDirty(true);
}

//---------------------------------------------------------
//   ctrlTypeChanged
//---------------------------------------------------------

void EditInstrument::ctrlTypeChanged(int idx)
      {
      QTreeWidgetItem* item = viewController->currentItem();
      
      if (item == 0)
            return;
      
      MusECore::MidiController::ControllerType t = (MusECore::MidiController::ControllerType)idx;
      MusECore::MidiController* c = (MusECore::MidiController*)item->data(0, Qt::UserRole).value<void*>();
      if(t == MusECore::midiControllerType(c->num()))
         return;
      
      item->setText(COL_TYPE, ctrlType->currentText());
      
      int hnum = 0, lnum = 0;
      
      spinBoxMin->blockSignals(true);
      spinBoxMax->blockSignals(true);
      spinBoxDefault->blockSignals(true);
      
      switch (t) {
            case MusECore::MidiController::Controller7:
                  spinBoxHCtrlNo->setEnabled(false);
                  spinBoxLCtrlNo->setEnabled(true);
                  spinBoxMin->setEnabled(true);
                  spinBoxMax->setEnabled(true);
                  enableDefaultControls(true, false);
                  spinBoxMin->setRange(-128, 127);
                  spinBoxMax->setRange(-128, 127);
                  
                  spinBoxMin->setValue(0);
                  spinBoxMax->setValue(127);
                  spinBoxDefault->setRange(spinBoxMin->value() - 1, spinBoxMax->value());
                  
                  spinBoxDefault->setValue(spinBoxDefault->minimum());
                  lnum = spinBoxLCtrlNo->value();

                  if(lnum == -1)
                    item->setText(COL_LNUM, QString("*"));
                  else  
                    item->setText(COL_LNUM, QString().setNum(lnum));
                  item->setText(COL_HNUM, QString("---"));
                  item->setText(COL_MIN, QString().setNum(spinBoxMin->value()));
                  item->setText(COL_MAX, QString().setNum(spinBoxMax->value()));
                  item->setText(COL_DEF, QString("---"));
                  break;

            case MusECore::MidiController::RPN:
            case MusECore::MidiController::NRPN:
                  spinBoxHCtrlNo->setEnabled(true);
                  spinBoxLCtrlNo->setEnabled(true);
                  spinBoxMin->setEnabled(true);
                  spinBoxMax->setEnabled(true);
                  enableDefaultControls(true, false);
                  spinBoxMin->setRange(-128, 127);
                  spinBoxMax->setRange(-128, 127);
                  
                  spinBoxMin->setValue(0);
                  spinBoxMax->setValue(127);
                  spinBoxDefault->setRange(spinBoxMin->value() - 1, spinBoxMax->value());
                  spinBoxDefault->setValue(spinBoxDefault->minimum());
                  
                  hnum = spinBoxHCtrlNo->value();
                  lnum = spinBoxLCtrlNo->value();

                  if(lnum == -1)
                    item->setText(COL_LNUM, QString("*"));
                  else  
                    item->setText(COL_LNUM, QString().setNum(lnum));
                  item->setText(COL_HNUM, QString().setNum(hnum));
                  item->setText(COL_MIN, QString().setNum(spinBoxMin->value()));
                  item->setText(COL_MAX, QString().setNum(spinBoxMax->value()));
                  item->setText(COL_DEF, QString("---"));
                  break;

            case MusECore::MidiController::Controller14:
            case MusECore::MidiController::RPN14:
            case MusECore::MidiController::NRPN14:
                  spinBoxHCtrlNo->setEnabled(true);
                  spinBoxLCtrlNo->setEnabled(true);
                  spinBoxMin->setEnabled(true);
                  spinBoxMax->setEnabled(true);
                  enableDefaultControls(true, false);
                  spinBoxMin->setRange(-16384, 16383);
                  spinBoxMax->setRange(-16384, 16383);
                  
                  spinBoxMin->setValue(0);
                  spinBoxMax->setValue(16383);
                  spinBoxDefault->setRange(spinBoxMin->value() - 1, spinBoxMax->value());
                  spinBoxDefault->setValue(spinBoxDefault->minimum());
                  
                  hnum = spinBoxHCtrlNo->value();
                  lnum = spinBoxLCtrlNo->value();
                  if(lnum == -1)
                    item->setText(COL_LNUM, QString("*"));
                  else  
                    item->setText(COL_LNUM, QString().setNum(lnum));
                  item->setText(COL_HNUM, QString().setNum(hnum));
                  item->setText(COL_MIN, QString().setNum(spinBoxMin->value()));
                  item->setText(COL_MAX, QString().setNum(spinBoxMax->value()));
                  item->setText(COL_DEF, QString("---"));
                  break;

            case MusECore::MidiController::Pitch:
                  spinBoxHCtrlNo->setEnabled(false);
                  spinBoxLCtrlNo->setEnabled(false);
                  spinBoxMin->setEnabled(true);
                  spinBoxMax->setEnabled(true);
                  enableDefaultControls(true, false);
                  spinBoxMin->setRange(-8192, 8191);
                  spinBoxMax->setRange(-8192, 8191);
                  
                  spinBoxMin->setValue(-8192);
                  spinBoxMax->setValue(8191);
                  spinBoxDefault->setRange(spinBoxMin->value() - 1, spinBoxMax->value());
                  spinBoxDefault->setValue(spinBoxDefault->minimum());
                  
                    item->setText(COL_LNUM, QString("---"));
                    item->setText(COL_HNUM, QString("---"));
                    item->setText(COL_MIN, QString().setNum(spinBoxMin->value()));
                    item->setText(COL_MAX, QString().setNum(spinBoxMax->value()));
                    item->setText(COL_DEF, QString("---"));
                  break;

            case MusECore::MidiController::Program:
                  spinBoxHCtrlNo->setEnabled(false);
                  spinBoxLCtrlNo->setEnabled(false);
                  spinBoxMin->setEnabled(false);
                  spinBoxMax->setEnabled(false);
                  enableDefaultControls(false, true);
                  spinBoxMin->setRange(0, 0);
                  spinBoxMax->setRange(0, 0);
                  
                  spinBoxMin->setValue(0);
                  spinBoxMax->setValue(0);
                  spinBoxDefault->setRange(0, 0);
                  spinBoxDefault->setValue(0);
                  
                    item->setText(COL_LNUM, QString("---"));
                    item->setText(COL_HNUM, QString("---"));
                    item->setText(COL_MIN, QString("---"));
                    item->setText(COL_MAX, QString("---"));
                    item->setText(COL_DEF, QString("---"));
                  break;

            // Shouldn't happen...
            default:
                  spinBoxHCtrlNo->setEnabled(false);
                  spinBoxLCtrlNo->setEnabled(false);
                  spinBoxMin->setEnabled(false);
                  spinBoxMax->setEnabled(false);
                  enableDefaultControls(false, false);
                  
                  spinBoxMin->blockSignals(false);
                  spinBoxMax->blockSignals(false);
                  return;
                  
                  break;
            }      
            
      spinBoxMin->blockSignals(false);
      spinBoxMax->blockSignals(false);
      spinBoxDefault->blockSignals(false);
      
      c->setNum(MusECore::MidiController::genNum(t, hnum, lnum));
      
      setDefaultPatchControls(0xffffff);
      if(t == MusECore::MidiController::Program)
      {
        c->setMinVal(0);
        c->setMaxVal(0xffffff);
        c->setInitVal(0xffffff);
      }
      else
      {
        c->setMinVal(spinBoxMin->value());
        c->setMaxVal(spinBoxMax->value());
        if(spinBoxDefault->value() == spinBoxDefault->minimum())
          c->setInitVal(MusECore::CTRL_VAL_UNKNOWN);
        else  
          c->setInitVal(spinBoxDefault->value());
      }  
      
      
      /* DELETETHIS
      
      if(rng != 0)
      {
        int mn = c->minVal();
        int mx = c->maxVal();
        //if(val > item->text(COL_MAX).toInt())
        if(mx > max)
        {
          c->setMaxVal(max);
          spinBoxMax->blockSignals(true);
          spinBoxMax->setValue(max);
          spinBoxMax->blockSignals(false);
          if(item)
            item->setText(COL_MAX, QString().setNum(max));
        }  
        //else
        if(mn < min)
        {
          c->setMinVal(min);
          spinBoxMin->blockSignals(true);
          spinBoxMin->setValue(min);
          spinBoxMin->blockSignals(false);
          if(item)
            item->setText(COL_MIN, QString().setNum(min));
        }  
        //else
        if(mx - mn > rng)
        {
          //mx = val + rng;
          c->setMinVal(0);
          c->setMaxVal(rng);
          spinBoxMin->blockSignals(true);
          spinBoxMax->blockSignals(true);
          spinBoxMin->setValue(0);
          spinBoxMax->setValue(rng);
          spinBoxMin->blockSignals(false);
          spinBoxMax->blockSignals(false);
          if(item)
          {
            item->setText(COL_MIN, QString().setNum(0));
            item->setText(COL_MAX, QString().setNum(rng));
          }  
        }  
        
        spinBoxDefault->blockSignals(true);
        
        spinBoxDefault->setRange(spinBoxMin->value() - 1, spinBoxMax->value());
        int inval = c->initVal();
        if(inval == MusECore::CTRL_VAL_UNKNOWN)
          spinBoxDefault->setValue(spinBoxDefault->minimum());
        else  
        {
          if(inval < c->minVal())
          {
            c->setInitVal(c->minVal());
            spinBoxDefault->setValue(c->minVal());
          }
          else  
          if(inval > c->maxVal())
          {  
            c->setInitVal(c->maxVal());
            spinBoxDefault->setValue(c->maxVal());
          }  
        } 
         
        //spinBoxDefault->setRange(c->minVal() - 1, c->maxVal());
        spinBoxDefault->blockSignals(false);
        
      }
        
      */
      
      workingInstrument.setDirty(true);
      }

//---------------------------------------------------------
//   ctrlHNumChanged
//---------------------------------------------------------

void EditInstrument::ctrlHNumChanged(int val)
      {
      QTreeWidgetItem* item = viewController->currentItem();
      
      if (item == 0)
            return;
      QString s;
      s.setNum(val);
      MusECore::MidiController* c = (MusECore::MidiController*)item->data(0, Qt::UserRole).value<void*>();
      int n = c->num() & 0x7fff00ff;
      c->setNum(n | ((val & 0xff) << 8));
      item->setText(COL_HNUM, s);
      workingInstrument.setDirty(true);
      }

//---------------------------------------------------------
//   ctrlLNumChanged
//---------------------------------------------------------

void EditInstrument::ctrlLNumChanged(int val)
      {
      QTreeWidgetItem* item = viewController->currentItem();
      
      if (item == 0)
            return;
      MusECore::MidiController* c = (MusECore::MidiController*)item->data(0, Qt::UserRole).value<void*>();
      int n = c->num() & ~0xff;
      c->setNum(n | (val & 0xff));
      if(val == -1)
        item->setText(COL_LNUM, QString("*"));
      else  
      {
        QString s;
        s.setNum(val);
        item->setText(COL_LNUM, s);
      }  
      workingInstrument.setDirty(true);
      }

//---------------------------------------------------------
//   ctrlMinChanged
//---------------------------------------------------------

void EditInstrument::ctrlMinChanged(int val)
{
      QTreeWidgetItem* item = viewController->currentItem();
      
      if (item == 0)
            return;
        
      QString s;
      s.setNum(val);
      item->setText(COL_MIN, s);
      
      MusECore::MidiController* c = (MusECore::MidiController*)item->data(0, Qt::UserRole).value<void*>();
      c->setMinVal(val);
      
      int rng = 0;
      switch(MusECore::midiControllerType(c->num()))
      {
            case MusECore::MidiController::Controller7:
            case MusECore::MidiController::RPN:
            case MusECore::MidiController::NRPN:
                  rng = 127;
                  break;
            case MusECore::MidiController::Controller14:
            case MusECore::MidiController::RPN14:
            case MusECore::MidiController::NRPN14:
            case MusECore::MidiController::Pitch:
                  rng = 16383;
                  break;
            default: 
                  break;      
      }
      
      int mx = c->maxVal();
      
      if(val > mx)
      {
        c->setMaxVal(val);
        spinBoxMax->blockSignals(true);
        spinBoxMax->setValue(val);
        spinBoxMax->blockSignals(false);
        item->setText(COL_MAX, s);
      }  
      else
      if(mx - val > rng)
      {
        mx = val + rng;
        c->setMaxVal(mx);
        spinBoxMax->blockSignals(true);
        spinBoxMax->setValue(mx);
        spinBoxMax->blockSignals(false);
        item->setText(COL_MAX, QString().setNum(mx));
      }  
      
      spinBoxDefault->blockSignals(true);
      
      spinBoxDefault->setRange(spinBoxMin->value() - 1, spinBoxMax->value());
      
      int inval = c->initVal();
      if(inval == MusECore::CTRL_VAL_UNKNOWN)
        spinBoxDefault->setValue(spinBoxDefault->minimum());
      else  
      {
        if(inval < c->minVal())
        {
          c->setInitVal(c->minVal());
          spinBoxDefault->setValue(c->minVal());
        }
        else  
        if(inval > c->maxVal())
        {  
          c->setInitVal(c->maxVal());
          spinBoxDefault->setValue(c->maxVal());
        }  
      } 
      
      spinBoxDefault->blockSignals(false);
      
      workingInstrument.setDirty(true);
}

//---------------------------------------------------------
//   ctrlMaxChanged
//---------------------------------------------------------

void EditInstrument::ctrlMaxChanged(int val)
{
      QTreeWidgetItem* item = viewController->currentItem();
      
      if (item == 0)
            return;
        
      QString s;
      s.setNum(val);
      item->setText(COL_MAX, s);
      
      MusECore::MidiController* c = (MusECore::MidiController*)item->data(0, Qt::UserRole).value<void*>();
      c->setMaxVal(val);
      
      int rng = 0;
      switch(MusECore::midiControllerType(c->num()))
      {
            case MusECore::MidiController::Controller7:
            case MusECore::MidiController::RPN:
            case MusECore::MidiController::NRPN:
                  rng = 127;
                  break;
            case MusECore::MidiController::Controller14:
            case MusECore::MidiController::RPN14:
            case MusECore::MidiController::NRPN14:
            case MusECore::MidiController::Pitch:
                  rng = 16383;
                  break;
            default: 
                  break;      
      }
      
      int mn = c->minVal();
      
      if(val < mn)
      {
        c->setMinVal(val);
        spinBoxMin->blockSignals(true);
        spinBoxMin->setValue(val);
        spinBoxMin->blockSignals(false);
        item->setText(COL_MIN, s);
      }  
      else
      if(val - mn > rng)
      {
        mn = val - rng;
        c->setMinVal(mn);
        spinBoxMin->blockSignals(true);
        spinBoxMin->setValue(mn);
        spinBoxMin->blockSignals(false);
        item->setText(COL_MIN, QString().setNum(mn));
      }  
      
      spinBoxDefault->blockSignals(true);
      
      spinBoxDefault->setRange(spinBoxMin->value() - 1, spinBoxMax->value());
      
      int inval = c->initVal();
      if(inval == MusECore::CTRL_VAL_UNKNOWN)
        spinBoxDefault->setValue(spinBoxDefault->minimum());
      else  
      {
        if(inval < c->minVal())
        {
          c->setInitVal(c->minVal());
          spinBoxDefault->setValue(c->minVal());
        }
        else  
        if(inval > c->maxVal())
        {  
          c->setInitVal(c->maxVal());
          spinBoxDefault->setValue(c->maxVal());
        }  
      } 
      
      spinBoxDefault->blockSignals(false);
      
      workingInstrument.setDirty(true);
}

//---------------------------------------------------------
//   ctrlDefaultChanged
//---------------------------------------------------------

void EditInstrument::ctrlDefaultChanged(int val)
{
      QTreeWidgetItem* item = viewController->currentItem();

      if (item == 0)
            return;
        
      MusECore::MidiController* c = (MusECore::MidiController*)item->data(0, Qt::UserRole).value<void*>();
      
      if(val == c->minVal() - 1)
      {
        c->setInitVal(MusECore::CTRL_VAL_UNKNOWN);
        item->setText(COL_DEF, QString("---"));
      }  
      else
      {
        c->setInitVal(val);
        item->setText(COL_DEF, QString().setNum(val));
      }
      workingInstrument.setDirty(true);
}

//---------------------------------------------------------
//   ctrlNullParamHChanged
//---------------------------------------------------------

void EditInstrument::ctrlNullParamHChanged(int nvh)
{
  int nvl = nullParamSpinBoxL->value();
  if(nvh == -1)
  {
    nullParamSpinBoxL->blockSignals(true);
    nullParamSpinBoxL->setValue(-1);
    nullParamSpinBoxL->blockSignals(false);
    nvl = -1;
  }
  else
  {
    if(nvl == -1)
    {
      nullParamSpinBoxL->blockSignals(true);
      nullParamSpinBoxL->setValue(0);
      nullParamSpinBoxL->blockSignals(false);
      nvl = 0;
    }
  }
  if(nvh == -1 && nvl == -1)
    workingInstrument.setNullSendValue(-1);
  else  
    workingInstrument.setNullSendValue((nvh << 8) | nvl);
  workingInstrument.setDirty(true);
}

//---------------------------------------------------------
//   ctrlNullParamLChanged
//---------------------------------------------------------

void EditInstrument::ctrlNullParamLChanged(int nvl)
{
  int nvh = nullParamSpinBoxH->value();
  if(nvl == -1)
  {
    nullParamSpinBoxH->blockSignals(true);
    nullParamSpinBoxH->setValue(-1);
    nullParamSpinBoxH->blockSignals(false);
    nvh = -1;
  }
  else
  {
    if(nvh == -1)
    {
      nullParamSpinBoxH->blockSignals(true);
      nullParamSpinBoxH->setValue(0);
      nullParamSpinBoxH->blockSignals(false);
      nvh = 0;
    }
  }
  if(nvh == -1 && nvl == -1)
    workingInstrument.setNullSendValue(-1);
  else  
    workingInstrument.setNullSendValue((nvh << 8) | nvl);
  workingInstrument.setDirty(true);
}

//---------------------------------------------------------
//   deletePatchClicked
//---------------------------------------------------------

void EditInstrument::deletePatchClicked()
      {
      QTreeWidgetItem* pi = patchView->currentItem();

      if (pi == 0)
            return;
      
      // If the item has a parent item, it's a patch item...
      if(pi->parent())
      {
        MusECore::PatchGroup* group = (MusECore::PatchGroup*)(pi->parent())->data(0, Qt::UserRole).value<void*>();
        
        // If there is an allocated patch in the data, delete it.
        MusECore::Patch* patch = (MusECore::Patch*)pi->data(0, Qt::UserRole).value<void*>();
        if(patch)
        {
          if(group)
            group->patches.remove(patch);

          delete patch;
        }  
      }
      else
      // The item has no parent item, it's a patch group item...
      {
        // Is there an allocated patch group in the data?
        MusECore::PatchGroup* group = (MusECore::PatchGroup*)pi->data(0, Qt::UserRole).value<void*>();
        if(group)
        {
          
          MusECore::PatchGroupList* pg = workingInstrument.groups();
          for(MusECore::iPatchGroup ipg = pg->begin(); ipg != pg->end(); ++ipg)
          {
            
            if(*ipg == group)
            {
              pg->erase(ipg);
              break;
            }  
          }
          
              const MusECore::PatchList& pl = group->patches;
              for(MusECore::ciPatch ip = pl.begin(); ip != pl.end(); ++ip)
              {
                // Delete the patch.
                if(*ip)
                  delete *ip;  
              }
              
          // Now delete the group.
          delete group;
          
        }  
      }
      
      // Now delete the patch or group item (and any child patch items) from the list view tree.
      // !!! This will trigger a patchChanged call. 
      patchView->blockSignals(true);
      delete pi;
      if(patchView->currentItem())
        patchView->currentItem()->setSelected(true);
      patchView->blockSignals(false);
      
      oldPatchItem = 0;
      patchChanged();
      
      workingInstrument.setDirty(true);
      }

//---------------------------------------------------------
//   newPatchClicked
//---------------------------------------------------------

void EditInstrument::newPatchClicked()
      {
      if(oldPatchItem)
      {
        if(oldPatchItem->parent())
          updatePatch(&workingInstrument, (MusECore::Patch*)oldPatchItem->data(0, Qt::UserRole).value<void*>());
        else  
          updatePatchGroup(&workingInstrument, (MusECore::PatchGroup*)oldPatchItem->data(0, Qt::UserRole).value<void*>());
      }  

      MusECore::PatchGroupList* pg = workingInstrument.groups();
      QString patchName;
      for (int i = 1;; ++i) {
            patchName = QString("Patch-%1").arg(i);
            bool found = false;

            for (MusECore::iPatchGroup g = pg->begin(); g != pg->end(); ++g) {
                  MusECore::PatchGroup* pgp = *g;
                  for (MusECore::iPatch p = pgp->patches.begin(); p != pgp->patches.end(); ++p) {
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

      // search current patch group
      QTreeWidgetItem* pi = patchView->currentItem();

      if (pi == 0)
            return;
      
      MusECore::Patch* selpatch = 0;
      
      // If there is a parent item then pi is a patch item, and there must be a parent patch group item.
      if(pi->parent())
      {
        // Remember the current selected patch.
        selpatch = (MusECore::Patch*)pi->data(0, Qt::UserRole).value<void*>();
        
        pi = pi->parent();
      }
      
      MusECore::PatchGroup* group = (MusECore::PatchGroup*)pi->data(0, Qt::UserRole).value<void*>();
      if(!group)
        return;
        
      
      // Create a new Patch, then store its pointer in a new patch item, 
      //  to be added later to the patch group only upon save...
      //Patch patch;
      //patch.name = patchName;
      MusECore::Patch* patch = new MusECore::Patch;
      int hb  = -1;
      int lb  = -1;
      int prg = 0;
      patch->hbank = hb;
      patch->lbank = lb;
      patch->prog = prg;
      patch->typ = -1;                     
      patch->drum = false;
      
      if(selpatch)
      {
        hb  = selpatch->hbank;
        lb  = selpatch->lbank;
        prg = selpatch->prog;
        patch->typ = selpatch->typ;                     
        patch->drum = selpatch->drum;                     
      }
      
      bool found = false;
      
      // The 129 is to accommodate -1 values. Yes, it may cause one extra redundant loop but hey, 
      //  if it hasn't found an available patch number by then, another loop won't matter.
      for(int k = 0; k < 129; ++k)
      {
        for(int j = 0; j < 129; ++j)
        {
          for(int i = 0; i < 128; ++i) 
          {
            found = false;

            for(MusECore::iPatchGroup g = pg->begin(); g != pg->end(); ++g) 
            {
              MusECore::PatchGroup* pgp = *g;
              for(MusECore::iPatch ip = pgp->patches.begin(); ip != pgp->patches.end(); ++ip) 
              {
                MusECore::Patch* p = *ip;
                if((p->prog  == ((prg + i) & 0x7f)) && 
                   ((p->lbank == -1 && lb == -1) || (p->lbank == ((lb + j) & 0x7f))) && 
                   ((p->hbank == -1 && hb == -1) || (p->hbank == ((hb + k) & 0x7f)))) 
                {
                  found = true;
                  break;
                }
              }
              if(found)
                break;
            }
            
            if(!found)
            {
              patch->prog  = (prg + i) & 0x7f;
              if(lb == -1)
                patch->lbank = -1;
              else  
                patch->lbank = (lb + j) & 0x7f;
                
              if(hb == -1)
                patch->hbank = -1;
              else    
                patch->hbank = (hb + k) & 0x7f;
                
              break;
            } 
              
          }
          if(!found)
            break;
        }  
        if(!found)
          break;
      }  
      
      patch->name = patchName;

      group->patches.push_back(patch);

      QTreeWidgetItem* sitem = new QTreeWidgetItem(pi);
      sitem->setText(0, patchName);
      
      patchNameEdit->setText(patchName);
      
      QVariant v = qVariantFromValue((void*)(patch));
      sitem->setData(0, Qt::UserRole, v);
      
      // May cause patchChanged call.
      patchView->blockSignals(true);
      sitem->setSelected(true);
      patchView->scrollToItem((QTreeWidgetItem*)sitem, QAbstractItemView::EnsureVisible);
      patchView->blockSignals(false);
      
      spinBoxHBank->setEnabled(true);
      spinBoxLBank->setEnabled(true);
      spinBoxProgram->setEnabled(true);
      checkBoxDrum->setEnabled(true);
      checkBoxGM->setEnabled(true);
      checkBoxGS->setEnabled(true);
      checkBoxXG->setEnabled(true);
      
      oldPatchItem = 0;
      patchChanged();
      
      workingInstrument.setDirty(true);
      }

//---------------------------------------------------------
//   newGroupClicked
//---------------------------------------------------------

void EditInstrument::newGroupClicked()
      {
      if(oldPatchItem)
      {
        if(oldPatchItem->parent())
          updatePatch(&workingInstrument, (MusECore::Patch*)oldPatchItem->data(0, Qt::UserRole).value<void*>());
        else  
          updatePatchGroup(&workingInstrument, (MusECore::PatchGroup*)oldPatchItem->data(0, Qt::UserRole).value<void*>());
      }  
      
      MusECore::PatchGroupList* pg = workingInstrument.groups();
      QString groupName;
      for (int i = 1;; ++i) {
            groupName = QString("Group-%1").arg(i);
            bool found = false;

            for (MusECore::ciPatchGroup g = pg->begin(); g != pg->end(); ++g) {
                  if ((*g)->name == groupName) {
                        found = true;
                        break;
                        }
                  }
            if (!found)
                  break;
            }

      // Create a new PatchGroup, then store its pointer in a new patch group item, 
      //  to be added later to the instrument only upon save...
      MusECore::PatchGroup* group = new MusECore::PatchGroup;
      group->name = groupName;
      
      pg->push_back(group);
      
      QTreeWidgetItem* sitem = new QTreeWidgetItem(patchView);
      sitem->setText(0, groupName);
      
      patchNameEdit->setText(groupName);
      
      // Set the list view item's data. 
      QVariant v = qVariantFromValue((void*)(group));
      sitem->setData(0, Qt::UserRole, v);
      //sitem->setAuxData((void*)pgp);
      
      // May cause patchChanged call.
      patchView->blockSignals(true);
      sitem->setSelected(true);
      patchView->blockSignals(false);
      
      oldPatchItem = sitem;
      
      spinBoxHBank->setEnabled(false);
      spinBoxLBank->setEnabled(false);
      spinBoxProgram->setEnabled(false);
      checkBoxDrum->setEnabled(false);
      checkBoxGM->setEnabled(false);
      checkBoxGS->setEnabled(false);
      checkBoxXG->setEnabled(false);
      
      workingInstrument.setDirty(true);
      }

//---------------------------------------------------------
//   deleteControllerClicked
//---------------------------------------------------------

void EditInstrument::deleteControllerClicked()
      {
      QTreeWidgetItem* item = viewController->currentItem();
      
      if(!item)
        return;
      
      MusECore::MidiController* ctrl = (MusECore::MidiController*)item->data(0, Qt::UserRole).value<void*>();
      if(!ctrl)
        return;
        
      workingInstrument.controller()->erase(ctrl->num());   
      // Now delete the controller.
      delete ctrl;
      
      // Now remove the controller item from the list.
      // This may cause a controllerChanged call.
      viewController->blockSignals(true);
      delete item;
      if(viewController->currentItem())
        viewController->currentItem()->setSelected(true);
      viewController->blockSignals(false);
      
      controllerChanged();
      
      workingInstrument.setDirty(true);
      }

//---------------------------------------------------------
//   newControllerClicked
//---------------------------------------------------------

void EditInstrument::newControllerClicked()
      {      
      QString cName;
      MusECore::MidiControllerList* cl = workingInstrument.controller();
      for (int i = 1;; ++i) {
            cName = QString("Controller-%1").arg(i);
            bool found = false;
            for (MusECore::iMidiController ic = cl->begin(); ic != cl->end(); ++ic) {
                  MusECore::MidiController* c = ic->second;
                  if (c->name() == cName) {
                        found = true;
                        break;
                        }
                  }
            if (!found)
              break;
          }

      MusECore::MidiController* ctrl = new MusECore::MidiController();
      ctrl->setNum(MusECore::CTRL_MODULATION);
      ctrl->setMinVal(0);
      ctrl->setMaxVal(127);
      ctrl->setInitVal(MusECore::CTRL_VAL_UNKNOWN);

      QTreeWidgetItem* ci = viewController->currentItem();
      
      // To allow for quick multiple successive controller creation.
      // If there's a current controller item selected, copy initial values from it.
      bool found = false;
      if(ci)
      {
        MusECore::MidiController* selctl = (MusECore::MidiController*)ci->data(0, Qt::UserRole).value<void*>();
        
        // Auto increment controller number.
        int l = selctl->num() & 0x7f;
        int h = selctl->num() & 0xffffff00;
          
        // Ignore internal controllers and wild cards.
        if(((h & 0xff0000) != 0x40000) && ((selctl->num() & 0xff) != 0xff))
        {
          // Assign.
          *ctrl = *selctl;
          
          for (int i = 1; i < 128; ++i) 
          {
            int j = ((i + l) & 0x7f) | h;
            found = false;
            for (MusECore::iMidiController ic = cl->begin(); ic != cl->end(); ++ic) 
            {
              MusECore::MidiController* c = ic->second;
              if(c->num() == j) 
              {
                found = true;
                break;
              }
            }
            if(!found)
            {
              ctrl->setNum(j);
              break;
            }  
          }  
        }
      }  
      
      ctrl->setName(cName);
      
      workingInstrument.controller()->add(ctrl);   
      QTreeWidgetItem* item = addControllerToView(ctrl);
      
      viewController->blockSignals(true);
      item->setSelected(true);
      viewController->blockSignals(false);
      
      controllerChanged();

      workingInstrument.setDirty(true);
      }

//---------------------------------------------------------
//   addControllerClicked
//---------------------------------------------------------

void EditInstrument::addControllerClicked()
{
  QListWidgetItem* idx = listController->currentItem();
  if(idx == 0)
    return;
  
  int lnum = -1;
  QString name = listController->currentItem()->text();
  for(int i = 0; i < 128; i++)
  {
    if(MusECore::midiCtrlName(i) == name)
    {
      lnum = i;
      break;
    }  
  }
  if(lnum == -1)
  {
    printf("Add controller: Controller not found: %s\n", name.toLatin1().constData());
    return;
  }
  
  int num = MusECore::MidiController::genNum(MusECore::MidiController::Controller7, 0, lnum);
    
  MusECore::MidiControllerList* cl = workingInstrument.controller();
  for(MusECore::iMidiController ic = cl->begin(); ic != cl->end(); ++ic) 
  {
    MusECore::MidiController* c = ic->second;
    if(c->name() == name)
    {
      QMessageBox::critical(this,
          tr("MusE: Cannot add common controller"),
          tr("A controller named '%1' already exists.").arg(name),
          QMessageBox::Ok,
          Qt::NoButton,
          Qt::NoButton);
          
      return;      
    }
    
    if(c->num() == num)
    {
      QMessageBox::critical(this,
          tr("MusE: Cannot add common controller"),
          tr("A controller number %1 already exists.").arg(num),
          QMessageBox::Ok,
          Qt::NoButton,
          Qt::NoButton);
          
      return;      
    }
  }
  
  MusECore::MidiController* ctrl = new MusECore::MidiController();
  ctrl->setNum(num);
  ctrl->setMinVal(0);
  ctrl->setMaxVal(127);
  ctrl->setInitVal(MusECore::CTRL_VAL_UNKNOWN);
  ctrl->setName(name);
  
  workingInstrument.controller()->add(ctrl);   
  
  QTreeWidgetItem* item = addControllerToView(ctrl);
  
  viewController->blockSignals(true);
  item->setSelected(true);
  viewController->blockSignals(false);
  
  controllerChanged();
  
  workingInstrument.setDirty(true);
}

/* DELETETHIS or later needed???
//---------------------------------------------------------
//   deleteSysexClicked
//---------------------------------------------------------

void EditInstrument::deleteSysexClicked()
      {
      //QListWidgetItem* item = instrumentList->currentItem();
      //QListWidgetItem* item2 = sysexList->currentItem();
      //if (item == 0 || item2 == 0)
      //      return;
      
      //MusECore::MidiInstrument* instrument = (MusECore::MidiInstrument*)item->data(Qt::UserRole).value<void*>();
      //SysEx* sysex  = (SysEx*)item2->data(Qt::UserRole).value<void*>();
      //QList<SysEx*> sl = instrument->sysex();
      //instrument->removeSysex(sysex);
      //delete item2;
      //instrument->setDirty(true);
      
      
      
      ListBoxData* item = (ListBoxData*)sysexList->selectedItem();
      if(!item)
        return;
      
      EventList* el = (EventList*)item->data();
      if(!el)
        return;
        
      }
*/

//---------------------------------------------------------
//   updatePatchGroup
//---------------------------------------------------------

void EditInstrument::updatePatchGroup(MusECore::MidiInstrument* instrument, MusECore::PatchGroup* pg)
      {
	QString a = pg->name;
	QString b = patchNameEdit->text();
      if (pg->name != patchNameEdit->text()) {
            pg->name = patchNameEdit->text();
            instrument->setDirty(true);
            }
      }

//---------------------------------------------------------
//   updatePatch
//---------------------------------------------------------

void EditInstrument::updatePatch(MusECore::MidiInstrument* instrument, MusECore::Patch* p)
      {
      if (p->name != patchNameEdit->text()) {
            p->name = patchNameEdit->text();
            instrument->setDirty(true);
            }
      
      signed char hb = (spinBoxHBank->value() - 1) & 0xff;
      if (p->hbank != hb) {
            p->hbank = hb;
            
            instrument->setDirty(true);
            }
      
      signed char lb = (spinBoxLBank->value() - 1) & 0xff;
      if (p->lbank != lb) {
            p->lbank = lb;
            
            instrument->setDirty(true);
            }
      
      signed char pr = (spinBoxProgram->value() - 1) & 0xff;
      if (p->prog != pr) {
            p->prog = pr;
            
            instrument->setDirty(true);
            }
            
      if (p->drum != checkBoxDrum->isChecked()) {
            p->drum = checkBoxDrum->isChecked();
            instrument->setDirty(true);
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
      }

/* DELETETHIS
//---------------------------------------------------------
//   updateController
//---------------------------------------------------------

void EditInstrument::updateController(MusECore::MidiInstrument* instrument, MusECore::MidiController* oc)
      {
      printf("updateController: A\n");
      
      int ctrlH = spinBoxHCtrlNo->value();
      int ctrlL = spinBoxLCtrlNo->value();
      //MusECore::MidiController::ControllerType type = (MusECore::MidiController::ControllerType)ctrlType->currentIndex();
      MusECore::MidiController::ControllerType type = (MusECore::MidiController::ControllerType)ctrlType->currentItem();
      int num = MusECore::MidiController::genNum(type, ctrlH, ctrlL);
      //int num = (ctrlH << 8) & 0x7f + ctrlL & 0x7f;

      printf("updateController: B\n");
      
      if (num != oc->num()) {
            
            printf("updateController: num changed, setting dirty. num:%d c->num:%d\n", num, oc->num());
            
            oc->setNum(num);
            instrument->setDirty(true);
            }
            
      if(type != MusECore::MidiController::Pitch && type != MusECore::MidiController::Program)
      {
        if (spinBoxMin->value() != oc->minVal()) {
              
              printf("updateController: min changed, setting dirty. min:%d c->min:%d\n", spinBoxMin->value(), oc->minVal());
              
              oc->setMinVal(spinBoxMin->value());
              instrument->setDirty(true);
              }
        if (spinBoxMax->value() != oc->maxVal()) {
              
              printf("updateController: max changed, setting dirty. num:%d max:%d c->max:%d\n", num, spinBoxMax->value(), oc->maxVal());
              
              oc->setMaxVal(spinBoxMax->value());
              instrument->setDirty(true);
              }
        
        int dv = spinBoxDefault->value(); 
        if(dv == oc->minVal() - 1)
          dv = MusECore::CTRL_VAL_UNKNOWN;
          
        //if (spinBoxDefault->value() != oc->initVal()) {
        if(dv != oc->initVal()) {
              //oc->setInitVal(spinBoxDefault->value());
              oc->setInitVal(dv);
              
              printf("updateController: default changed, setting dirty. def:%d c->init:%d\n", dv, oc->initVal());
              
              instrument->setDirty(true);
              }
      }
      
      
      printf("updateController: C\n");
      
      //if (moveWithPart->isChecked() ^ oc->moveWithPart()) {
      //      oc->setMoveWithPart(moveWithPart->isChecked());
      //      instrument->setDirty(true);
      //      }
      if (ctrlName->text() != oc->name()) {
            oc->setName(ctrlName->text());
            
            printf("updateController: name changed, setting dirty. name:%s c->name:%s\n", ctrlName->text().toLatin1().constData(), oc->name().toLatin1().constData());
            
            instrument->setDirty(true);
            }
      //if (ctrlComment->toPlainText() != oc->comment()) {
      //      oc->setComment(ctrlComment->toPlainText());
      //      instrument->setDirty(true);
      //      }
      
      printf("updateController: D\n");
      
      }
*/

//---------------------------------------------------------
//   updateInstrument
//---------------------------------------------------------

void EditInstrument::updateInstrument(MusECore::MidiInstrument* instrument)
      {
      QTreeWidgetItem* patchItem = patchView->currentItem();

      if (patchItem) 
      {      
        // If the item has a parent, it's a patch item.
        if(patchItem->parent())
          updatePatch(instrument, (MusECore::Patch*)patchItem->data(0, Qt::UserRole).value<void*>());
        else
          updatePatchGroup(instrument, (MusECore::PatchGroup*)patchItem->data(0, Qt::UserRole).value<void*>());
              
      }
    }

//---------------------------------------------------------
//   checkDirty
//    return true on Abort
//---------------------------------------------------------

int EditInstrument::checkDirty(MusECore::MidiInstrument* i, bool isClose)
      {
      updateInstrument(i);
      if (!i->dirty())
            return 0;

      int n;
      if(isClose) 
        n = QMessageBox::warning(this, tr("MusE"),
         tr("The current Instrument contains unsaved data\n"
         "Save Current Instrument?"),
         tr("&Save"), tr("&Nosave"), tr("&Abort"), 0, 2);
      else   
        n = QMessageBox::warning(this, tr("MusE"),
         tr("The current Instrument contains unsaved data\n"
         "Save Current Instrument?"),
         tr("&Save"), tr("&Nosave"), 0, 1);
      if (n == 0) {
            if (i->filePath().isEmpty())
            {
                  saveAs();
            }      
            else {
                  FILE* f = fopen(i->filePath().toLatin1().constData(), "w");
                  if(f == 0)
                        saveAs();
                  else {
                        if(fclose(f) != 0)
                          printf("EditInstrument::checkDirty: Error closing file\n");
                          
                        if(fileSave(i, i->filePath()))
                              i->setDirty(false);
                        }
                  }
            return 0;
            }
      return n;
      }

//---------------------------------------------------------
//    getPatchItemText
//---------------------------------------------------------

QString EditInstrument::getPatchItemText(int val)
{
  QString s;
  if(val == MusECore::CTRL_VAL_UNKNOWN)
    s = "---";
  else
  {
    int hb = ((val >> 16) & 0xff) + 1;
    if (hb == 0x100)
      hb = 0;
    int lb = ((val >> 8) & 0xff) + 1;
    if (lb == 0x100)
      lb = 0;
    int pr = (val & 0xff) + 1;
    if (pr == 0x100)
      pr = 0;
    s.sprintf("%d-%d-%d", hb, lb, pr);
  } 
  
  return s;
}                 

//---------------------------------------------------------
//    enableDefaultControls
//---------------------------------------------------------

void EditInstrument::enableDefaultControls(bool enVal, bool enPatch)
{
  spinBoxDefault->setEnabled(enVal);
  patchButton->setEnabled(enPatch);
  if(!enPatch)
  {
    patchButton->blockSignals(true);
    patchButton->setText("---");
    patchButton->blockSignals(false);
  }
  defPatchH->setEnabled(enPatch);
  defPatchL->setEnabled(enPatch);
  defPatchProg->setEnabled(enPatch);
}

//---------------------------------------------------------
//    setDefaultPatchName
//---------------------------------------------------------

void EditInstrument::setDefaultPatchName(int val)
{
  patchButton->blockSignals(true);
  patchButton->setText(getPatchName(val));
  patchButton->blockSignals(false);
}

//---------------------------------------------------------
//    getDefaultPatchNumber
//---------------------------------------------------------

int EditInstrument::getDefaultPatchNumber()
{      
  int hval = defPatchH->value() - 1;
  int lval = defPatchL->value() - 1;
  int prog = defPatchProg->value() - 1;
  if(hval == -1)
    hval = 0xff;
  if(lval == -1)
    lval = 0xff;
  if(prog == -1)
    prog = 0xff;
    
  return ((hval & 0xff) << 16) + ((lval & 0xff) << 8) + (prog & 0xff); 
}
      
//---------------------------------------------------------
//    setDefaultPatchNumbers
//---------------------------------------------------------

void EditInstrument::setDefaultPatchNumbers(int val)
{
  int hb;
  int lb;
  int pr;
  
  if(val == MusECore::CTRL_VAL_UNKNOWN)
    hb = lb = pr = 0;
  else
  {
    hb = ((val >> 16) & 0xff) + 1;
    if (hb == 0x100)
      hb = 0;
    lb = ((val >> 8) & 0xff) + 1;
    if (lb == 0x100)
      lb = 0;
    pr = (val & 0xff) + 1;
    if (pr == 0x100)
      pr = 0;
  } 
    
  defPatchH->blockSignals(true);
  defPatchL->blockSignals(true);
  defPatchProg->blockSignals(true);
  defPatchH->setValue(hb);  
  defPatchL->setValue(lb);  
  defPatchProg->setValue(pr);
  defPatchH->blockSignals(false);
  defPatchL->blockSignals(false);
  defPatchProg->blockSignals(false);
}

//---------------------------------------------------------
//    setDefaultPatchControls
//---------------------------------------------------------

void EditInstrument::setDefaultPatchControls(int val)
{
  setDefaultPatchNumbers(val);
  setDefaultPatchName(val);
}

//---------------------------------------------------------
//   getPatchName
//---------------------------------------------------------

QString EditInstrument::getPatchName(int prog)
{
      int pr = prog & 0xff;
      if(prog == MusECore::CTRL_VAL_UNKNOWN || pr == 0xff)
            return "---";
      
      int hbank = (prog >> 16) & 0xff;
      int lbank = (prog >> 8) & 0xff;

      MusECore::PatchGroupList* pg = workingInstrument.groups();
      
      for(MusECore::ciPatchGroup i = pg->begin(); i != pg->end(); ++i) {
            const MusECore::PatchList& pl = (*i)->patches;
            for (MusECore::ciPatch ipl = pl.begin(); ipl != pl.end(); ++ipl) {
                  const MusECore::Patch* mp = *ipl;
                  if (//(mp->typ & tmask) && DELETETHIS
                    (pr == mp->prog)
                    //&& ((drum && mode != MT_GM) ||  DELETETHIS
                    //   (mp->drum == drumchan))   
                    
                    //&& (hbank == mp->hbank || !hb || mp->hbank == -1)
                    //&& (lbank == mp->lbank || !lb || mp->lbank == -1))
                    && (hbank == mp->hbank || mp->hbank == -1)
                    && (lbank == mp->lbank || mp->lbank == -1))
                        return mp->name;
                  }
            }
      return "---";
}

} // namespace MusEGui
