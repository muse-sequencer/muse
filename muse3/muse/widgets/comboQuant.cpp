//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: comboQuant.cpp,v 1.1.1.1 2003/10/27 18:54:52 wschweer Exp $
//  (C) Copyright 2001 Werner Schweer (ws@seh.de)
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

#include <QTableWidget>
#include <QTableWidgetItem>
#include <QHeaderView>

#include "comboQuant.h"

namespace MusEGui {

static int quantTable[] = {
      1, 16, 32,  64, 128, 256,  512, 1024,
      1, 24, 48,  96, 192, 384,  768, 1536,
      1, 36, 72, 144, 288, 576, 1152, 2304
      };

static const char* quantStrings[] = {
      QT_TRANSLATE_NOOP("MusEGui::ComboQuant", "Off"), "64T", "32T", "16T", "8T", "4T", "2T", "1T",
      QT_TRANSLATE_NOOP("MusEGui::ComboQuant", "Off"), "64",  "32",  "16",  "8",  "4",  "2",  "1",
      QT_TRANSLATE_NOOP("MusEGui::ComboQuant", "Off"), "64.", "32.", "16.", "8.", "4.", "2.", "1."
      };

//---------------------------------------------------------
//   ComboQuant
//---------------------------------------------------------

ComboQuant::ComboQuant(QWidget* parent)
   : QComboBox(parent)
      {
      ///Q3ListBox* qlist = new Q3ListBox(this);
      ///qlist->setMinimumWidth(95);
      //setListBox(qlist); ddskrjo
      ///qlist->setColumnMode(3);
      
      
      qlist = new QTableWidget(8, 3);     
      qlist->verticalHeader()->setDefaultSectionSize(22);                      
      qlist->horizontalHeader()->setDefaultSectionSize(32);                      
      qlist->setSelectionMode(QAbstractItemView::SingleSelection);                      
      qlist->verticalHeader()->hide();                        
      qlist->horizontalHeader()->hide();                      
      
      qlist->setMinimumWidth(96);
      
      setView(qlist);               
      
      ///for (int i = 0; i < 24; i++)
      ///      qlist->insertItem(tr(quantStrings[i]), i);
      for (int j = 0; j < 3; j++)                           
        for (int i = 0; i < 8; i++)
          qlist->setItem(i, j, new QTableWidgetItem(tr(quantStrings[i + j * 8])));
       
            
      connect(this, SIGNAL(activated(int)), SLOT(activated(int)));
      }

//---------------------------------------------------------
//   activated
//---------------------------------------------------------

void ComboQuant::activated(int /*index*/)
      {
      ///emit valueChanged(quantTable[index]);
      emit valueChanged(quantTable[qlist->currentRow() + qlist->currentColumn() * 8]);
      }

//---------------------------------------------------------
//   setQuant
//---------------------------------------------------------

void ComboQuant::setValue(int val)
      {
      for (int i = 0; i < 24; i++) {
            if (val == quantTable[i]) {
                  setCurrentIndex(i);
                  return;
                  }
            }
            
      for (unsigned i = 0; i < sizeof(quantTable)/sizeof(*quantTable); i++) {
            if (val == quantTable[i]) {
                  setCurrentIndex(i);
                  return;
                  }
            }
      printf("ComboQuant::setValue(%d) not defined\n", val);
      setCurrentIndex(0);
      }

} // namespace MusEGui
