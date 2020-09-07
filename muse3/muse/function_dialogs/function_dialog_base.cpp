//=========================================================
//  MusE
//  Linux Music Editor
//
//  function_dialog_base.cpp
//  (C) Copyright 2018 Tim E. Real (terminator356 on users dot sourceforge dot net)
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

#include <QAbstractButton>
#include "function_dialog_base.h"

// NOTE: To cure circular dependencies these includes are at the bottom.
#include <QWidget>
#include <QButtonGroup>
#include "xml.h"

namespace MusEGui {

FunctionDialogBase::FunctionDialogBase(QWidget* parent)
  : QDialog(parent)
{
  _range_container = NULL;
  _parts_container = NULL;
  _range_group = new QButtonGroup;
  _parts_group = new QButtonGroup;
  _range_group->setExclusive(true);
  _parts_group->setExclusive(true);
}

FunctionDialogBase::~FunctionDialogBase()
{
  delete _parts_group;
  delete _range_group;
}

void FunctionDialogBase::pull_values()
{
  //--------------------------------------------
  // Grab current IDs from range and parts groups.
  //--------------------------------------------
  
  setCurRange(_range_group->checkedId());
  setCurParts(_parts_group->checkedId());
  
  //--------------------------------------------
  // Set convenience return flags.
  //--------------------------------------------
  
  setReturnFlags(calc_return_flags());
}

void FunctionDialogBase::accept()
{
  pull_values();
  QDialog::accept();
}

void FunctionDialogBase::setupButton(QButtonGroup* group, int buttonID, bool show)
{
  QAbstractButton* bt = group->button(buttonID);
  if(!bt)
    return;
  bt->setEnabled(show);
  bt->setVisible(show);
}

void FunctionDialogBase::setupDialog()
{
  const FunctionDialogElements_t elem = elements();
  
  //--------------------------------------------
  // Show or hide the range and parts containers.
  // The containers need to be enabled first before
  //  their controls can be enabled in setupButton().
  //--------------------------------------------

  if(_range_container)
  {
    const bool show =
      elem & (FunctionAllEventsButton | FunctionSelectedEventsButton |
                   FunctionLoopedButton | FunctionSelectedLoopedButton);
    _range_container->setEnabled(show);
    _range_container->setVisible(show);
  }
  
  if(_parts_container)
  {
    const bool show = elem & (FunctionAllPartsButton | FunctionSelectedPartsButton);
    _parts_container->setEnabled(show);
    _parts_container->setVisible(show);
  }

  //--------------------------------------------
  // Now show or hide the individual controls in
  //  the range and parts containers.
  //--------------------------------------------
  
  setupButton(_range_group, FunctionAllEventsButton, elem & FunctionAllEventsButton);
  setupButton(_range_group, FunctionSelectedEventsButton, elem & FunctionSelectedEventsButton);
  setupButton(_range_group, FunctionLoopedButton, elem & FunctionLoopedButton);
  setupButton(_range_group, FunctionSelectedLoopedButton, elem & FunctionSelectedLoopedButton);

  setupButton(_parts_group, FunctionAllPartsButton, elem & FunctionAllPartsButton);
  setupButton(_parts_group, FunctionSelectedPartsButton, elem & FunctionSelectedPartsButton);

  //---------------------------------------
  // If the current range is invalid or the
  //  button is not visible, choose another.
  //---------------------------------------

  QAbstractButton* bt = _range_group->button(curRange());
  if(!bt || !bt->isEnabled())
  {
    QList<QAbstractButton*> bl = _range_group->buttons();
    const int range_group_sz = bl.size();
    for(int i = 0; i < range_group_sz; ++i)
    {
      bt = bl.at(i);
      if(bt->isEnabled())
      {
        setCurRange(_range_group->id(bt));
        break;
      }
    }
  }
  bt =_range_group->button(curRange());
  if(bt)
    bt->setChecked(true);
  
  //---------------------------------------
  // If the current parts is invalid or the
  //  button is not visible, choose another.
  //---------------------------------------
  
  bt = _parts_group->button(curParts());
  if(!bt || !bt->isEnabled())
  {
    QList<QAbstractButton*> bl = _parts_group->buttons();
    const int parts_group_sz = bl.size();
    for(int i = 0; i < parts_group_sz; ++i)
    {
      bt = bl.at(i);
      if(bt->isEnabled())
      {
        setCurParts(_parts_group->id(bt));
        break;
      }
    }
  }
  bt = _parts_group->button(curParts());
  if(bt)
    bt->setChecked(true);
}

int FunctionDialogBase::exec()
{
  //-----------------------------------
  // Set up the dialog according to the
  //  various settings.
  //-----------------------------------

  setupDialog();

  //-----------------------------------
  // Now execute the dialog.
  //-----------------------------------

  return QDialog::exec();
}

bool FunctionDialogBase::read_configuration(const QString& /*tag*/, MusECore::Xml& /*xml*/)
{
//   if (tag == "range")
//   {
//     setCurRange(xml.parseInt());
//     return true;
//   }
//   else if (tag == "parts")
//   {
//     setCurParts(xml.parseInt());
//     return true;
//   }
  
  return false;
}
 
void FunctionDialogBase::write_configuration(int /*level*/, MusECore::Xml& /*xml*/)
{
//   xml.intTag(level, "range", curRange());
//   xml.intTag(level, "parts", curParts());
}

} // namespace MusEGui


