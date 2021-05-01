//=========================================================
//  MusE
//  Linux Music Editor
//  choose_sysex.cpp
//  (C) Copyright 2014 Tim E. Real (terminator356 at users.sourceforge.net)
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

#include <QDialog>
#include <QWidget>
#include <QListWidgetItem>
#include <QVariant>
#include <qtextstream.h>

#include "choose_sysex.h"
#include "minstrument.h"

namespace MusECore {
extern QString sysex2string(int len, unsigned char* data);
}

namespace MusEGui {
  
ChooseSysexDialog::ChooseSysexDialog(QWidget* parent, MusECore::MidiInstrument* instr)
  : QDialog(parent)
{
  setupUi(this);
  sysexList->clear();
  _sysex = nullptr;
  _instr = instr;
  if(_instr)
  {
    foreach(const MusECore::SysEx* s, _instr->sysex()) 
    {
      if(!s) 
        continue;
      QListWidgetItem* item = new QListWidgetItem(s->name);
      QVariant v = QVariant::fromValue((void*)s);
      item->setData(Qt::UserRole, v);
      sysexList->addItem(item);
    }
  }
  
  if(sysexList->item(0))
    sysexList->item(0)->setSelected(true);
  
  connect(sysexList, SIGNAL(currentItemChanged(QListWidgetItem*,QListWidgetItem*)),
      SLOT(sysexChanged(QListWidgetItem*, QListWidgetItem*)));
  
  sysexChanged(sysexList->item(0), 0);
}
  
//---------------------------------------------------------
//   sysexChanged
//---------------------------------------------------------

void ChooseSysexDialog::sysexChanged(QListWidgetItem* sel, QListWidgetItem*)
{
  if(!sel) 
  {
    hexLabel->setText("");
    commentLabel->setText("");
    return;
  }
  MusECore::SysEx* sx = (MusECore::SysEx*)sel->data(Qt::UserRole).value<void*>();
  if(!sx)
  {
    hexLabel->setText("");
    commentLabel->setText("");
    return;
  }
  hexLabel->setText(MusECore::sysex2string(sx->dataLen, sx->data));
  commentLabel->setText(sx->comment);
}
      
//---------------------------------------------------------
//   accept
//---------------------------------------------------------

void ChooseSysexDialog::accept()
{
  _sysex = nullptr;
  QListWidgetItem* item = sysexList->currentItem();
  if(item)
    _sysex = (MusECore::SysEx*)item->data(Qt::UserRole).value<void*>();
    
  QDialog::accept();
}

} // namespace MusEGui
