//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: editinstrument.cpp,v 1.6 2005/12/12 22:03:44 wschweer Exp $
//
//  (C) Copyright 2005 Werner Schweer (ws@seh.de)
//=========================================================

#include "editinstrument.h"
#include "minstrument.h"

//---------------------------------------------------------
//   EditInstrument
//---------------------------------------------------------

EditInstrument::EditInstrument(QWidget* parent)
   : QMainWindow(parent)
      {
      setupUi(this);
      // populate instrument list
      for (iMidiInstrument i = midiInstruments.begin(); i != midiInstruments.end(); ++i)
            instrumentList->addItem((*i)->iname());
      instrumentList->setItemSelected(instrumentList->item(0), true);
      connect(instrumentList, SIGNAL(currentItemChanged(QListWidgetItem*,QListWidgetItem*)),
         SLOT(instrumentChanged(QListWidgetItem*)));
      connect(patchView, SIGNAL(currentItemChanged(QTreeWidgetItem*,QTreeWidgetItem*)),
         SLOT(patchChanged(QTreeWidgetItem*)));
      instrumentChanged(instrumentList->item(0));
      }

//---------------------------------------------------------
//   instrumentChanged
//---------------------------------------------------------

void EditInstrument::instrumentChanged(QListWidgetItem* sel)
      {
      if (sel == 0)
            return;
      QString iname = sel->text();
      iMidiInstrument i = midiInstruments.begin();
      for (; i != midiInstruments.end(); ++i) {
            if ((*i)->iname() == iname)
                  break;
            }
      patchView->clear();
      if (i == midiInstruments.end())
            return;
      // populate patch list
      std::vector<PatchGroup>* pg = (*i)->groups();
      for (std::vector<PatchGroup>::iterator g = pg->begin(); g != pg->end(); ++g) {
            QTreeWidgetItem* item = new QTreeWidgetItem;
            item->setText(0, g->name);
            patchView->addTopLevelItem(item);
            for (iPatch p = g->patches.begin(); p != g->patches.end(); ++p) {
                  QTreeWidgetItem* sitem = new QTreeWidgetItem;
                  sitem->setText(0, (*p)->name);
                  item->addChild(sitem);
                  }
            }
      }

//---------------------------------------------------------
//   patchChanged
//---------------------------------------------------------

void EditInstrument::patchChanged(QTreeWidgetItem* sel)
      {
      if (sel == 0) {
            textLabelPatchName->setText("");
            return;
            }
      textLabelPatchName->setText(sel->text(0));
      }

//---------------------------------------------------------
//   fileNew
//---------------------------------------------------------

void EditInstrument::fileNew()
      {

      }

//---------------------------------------------------------
//   fileOpen
//---------------------------------------------------------

void EditInstrument::fileOpen()
      {
      printf("fileOpen\n");
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

      }

//---------------------------------------------------------
//   fileExit
//---------------------------------------------------------

void EditInstrument::fileExit()
      {

      }

