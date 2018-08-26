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
#include <QButtonGroup>
#include "function_dialog_base.h"
#include "xml.h"

namespace MusEGui {

int FunctionDialogBase::_range = 1;
int FunctionDialogBase::_parts = 0;
int FunctionDialogBase::_ret_flags = FunctionReturnNoFlags;
FunctionDialogElements_t FunctionDialogBase::_elements = FunctionDialogNoElements;

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
  
  _range = _range_group->checkedId();
  _parts = _parts_group->checkedId();
  
  //--------------------------------------------
  // Set convenience return flags.
  //--------------------------------------------
  
  set_return_flags();
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
  //--------------------------------------------
  // Show or hide the range and parts containers.
  // The containers need to be enabled first before
  //  their controls can be enabled in setupButton().
  //--------------------------------------------
  
  if(_range_container)
  {
    const bool show =
      _elements & (FunctionAllEventsButton | FunctionSelectedEventsButton |
                   FunctionLoopedButton | FunctionSelectedLoopedButton);
    _range_container->setEnabled(show);
    _range_container->setVisible(show);
  }
  
  if(_parts_container)
  {
    const bool show = _elements & (FunctionAllPartsButton | FunctionSelectedPartsButton);
    _parts_container->setEnabled(show);
    _parts_container->setVisible(show);
  }

  //--------------------------------------------
  // Now show or hide the individual controls in
  //  the range and parts containers.
  //--------------------------------------------
  
  setupButton(_range_group, FunctionAllEventsButton, _elements & FunctionAllEventsButton);
  setupButton(_range_group, FunctionSelectedEventsButton, _elements & FunctionSelectedEventsButton);
  setupButton(_range_group, FunctionLoopedButton, _elements & FunctionLoopedButton);
  setupButton(_range_group, FunctionSelectedLoopedButton, _elements & FunctionSelectedLoopedButton);

  setupButton(_parts_group, FunctionAllPartsButton, _elements & FunctionAllPartsButton);
  setupButton(_parts_group, FunctionSelectedPartsButton, _elements & FunctionSelectedPartsButton);

  //---------------------------------------
  // If the current range is invalid or the
  //  button is not visible, choose another.
  //---------------------------------------

  QAbstractButton* bt = _range_group->button(_range);
  if(!bt || !bt->isEnabled())
  {
    QList<QAbstractButton*> bl = _range_group->buttons();
    const int range_group_sz = bl.size();
    for(int i = 0; i < range_group_sz; ++i)
    {
      bt = bl.at(i);
      if(bt->isEnabled())
      {
        _range = _range_group->id(bt);
        break;
      }
    }
  }
  bt =_range_group->button(_range);
  if(bt)
    bt->setChecked(true);
  
  //---------------------------------------
  // If the current parts is invalid or the
  //  button is not visible, choose another.
  //---------------------------------------
  
  bt = _parts_group->button(_parts);
  if(!bt || !bt->isEnabled())
  {
    QList<QAbstractButton*> bl = _parts_group->buttons();
    const int parts_group_sz = bl.size();
    for(int i = 0; i < parts_group_sz; ++i)
    {
      bt = bl.at(i);
      if(bt->isEnabled())
      {
        _parts = _parts_group->id(bt);
        break;
      }
    }
  }
  bt = _parts_group->button(_parts);
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

bool FunctionDialogBase::read_configuration(const QString& tag, MusECore::Xml& xml)
{
  if (tag == "range")
  {
    _range=xml.parseInt();
    return true;
  }
  else if (tag == "parts")
  {
    _parts=xml.parseInt();
    return true;
  }
  
  return false;
        
//   for (;;)
//   {
//     MusECore::Xml::Token token = xml.parse();
//     if (token == MusECore::Xml::Error || token == MusECore::Xml::End)
//       break;
//       
//     const QString& tag = xml.s1();
//     switch (token)
//     {
//       case MusECore::Xml::TagStart:
//         if (tag == "range")
//           range=xml.parseInt();
//         else if (tag == "parts")
//           parts=xml.parseInt();
//         else
//           xml.unknown("Erase");
//         break;
//         
//       case MusECore::Xml::TagEnd:
//         if (tag == "erase")
//           return;
//         
//       default:
//         break;
//     }
//   }
}
 
void FunctionDialogBase::write_configuration(int level, MusECore::Xml& xml)
{
  xml.intTag(level, "range", _range);
  xml.intTag(level, "parts", _parts);
}

} // namespace MusEGui


