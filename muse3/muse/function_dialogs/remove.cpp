//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: remove.cpp,v 1.1.1.1 2011/05/05 18:51:04 flo93 Exp $
//  (C) Copyright 2011 Florian Jung (flo93@sourceforge.net)
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

#include <QButtonGroup>
#include "remove.h"
#include "xml.h"

namespace MusEGui {

FunctionReturnDialogFlags_t Remove::_ret_flags = FunctionReturnNoFlags;
FunctionDialogElements_t Remove::_elements = FunctionDialogNoElements;
int Remove::_range = FunctionSelectedEventsButton;
int Remove::_parts = FunctionSelectedPartsButton;
int Remove::velo_threshold = 16;
bool Remove::velo_thres_used = false;
int Remove::len_threshold = 12;
bool Remove::len_thres_used = false;


// REMOVE Tim. citem. Changed.
// Remove::Remove(QWidget* parent)
//  : QDialog(parent)

Remove::Remove(QWidget* parent)
  : FunctionDialogBase(parent)
{
  setupUi(this);
  
  //--------------------------------------------
  // Set range and parts containers if available.
  //--------------------------------------------
  
  _range_container = rangeBox;
  _parts_container = partsBox;
  
//  range_group = new QButtonGroup;
//  range_group->addButton(all_events_button,0);
//  range_group->addButton(selected_events_button,1);
//  range_group->addButton(looped_events_button,2);
//  range_group->addButton(selected_looped_button,3);
//  parts_group = new QButtonGroup;
//  parts_group ->addButton(not_all_parts_button,0);
//  parts_group ->addButton(all_parts_button,1);
  
  //--------------------------------------------
  // Add element widgets to range and parts groups.
  //--------------------------------------------
  
  _range_group->addButton(all_events_button, FunctionAllEventsButton);
  _range_group->addButton(selected_events_button,FunctionSelectedEventsButton);
  _range_group->addButton(looped_events_button, FunctionLoopedButton);
  _range_group->addButton(selected_looped_button, FunctionSelectedLoopedButton);
  
  _parts_group->addButton(not_all_parts_button, FunctionSelectedPartsButton);
  _parts_group->addButton(all_parts_button, FunctionAllPartsButton);
}

// REMOVE Tim. citem. Removed.
// Remove::~Remove()
// {
//  delete parts_group;
//  delete range_group;
// }

void Remove::pull_values()
{
  // REMOVE Tim. citem. Changed.
  //  range = range_group->checkedId();
  //  parts = parts_group->checkedId();
  
  //--------------------------------------------
  // Grab IDs or values from common base object
  //  (range and parts groups etc.)
  //--------------------------------------------
  
  FunctionDialogBase::pull_values();
  
  //--------------------------------------------
  // Grab this dialog's specific IDs or values.
  //--------------------------------------------
  
  len_thres_used=len_checkbox->isChecked();
  len_threshold=len_spinbox->value();
  velo_thres_used=velo_checkbox->isChecked();
  velo_threshold=velo_spinbox->value();
}

// REMOVE Tim. citem. Removed.
// void Remove::accept()
// {
// // REMOVE Tim. citem. Changed.
// //   pull_values();
// //  QDialog::accept();
//   FunctionDialogBase::accept();
// }

// REMOVE Tim. citem. Changed.
// int Remove::exec(bool showAllEventsButton,
//              bool showSelectedEventsButton,
//              bool showLoopedButton,
//              bool showSelectedLoopedButton,
//              bool showPartsButtons)
// {
//   const int range_group_sz = range_group->buttons().size();
//   const int parts_group_sz = parts_group->buttons().size();
// 
//   if ((range < 0) || (range >= range_group_sz)) range=0;
//   if ((parts < 0) || (parts >= parts_group_sz)) parts=0;
// 
//   all_events_button->setEnabled(showAllEventsButton);
//   all_events_button->setVisible(showAllEventsButton);
// 
//   selected_events_button->setEnabled(showSelectedEventsButton);
//   selected_events_button->setVisible(showSelectedEventsButton);
// 
//   looped_events_button->setEnabled(showLoopedButton);
//   looped_events_button->setVisible(showLoopedButton);
// 
//   selected_looped_button->setEnabled(showSelectedLoopedButton);
//   selected_looped_button->setVisible(showSelectedLoopedButton);
// 
//   len_checkbox->setChecked(len_thres_used);
//   len_spinbox->setValue(len_threshold);
//   velo_checkbox->setChecked(velo_thres_used);
//   velo_spinbox->setValue(velo_threshold);
// 
//   partsBox->setEnabled(showPartsButtons);
//   partsBox->setVisible(showPartsButtons);
// 
//   // If the current button is not visible, choose another.
//   if(!range_group->button(range)->isEnabled())
//   {
//     for(int i = 0; i < range_group_sz; ++i)
//     {
//       if(range_group->button(i)->isEnabled())
//       {
//         range = i;
//         break;
//       }
//     }
//   }
//   if(!parts_group->button(parts)->isEnabled())
//   {
//     for(int i = 0; i < parts_group_sz; ++i)
//     {
//       if(parts_group->button(i)->isEnabled())
//       {
//         parts = i;
//         break;
//       }
//     }
//   }
// 
//   range_group->button(range)->setChecked(true);
//   parts_group->button(parts)->setChecked(true);
// 
//   return QDialog::exec();
// }

void Remove::setupDialog()
{
  //------------------------------------
  // Setup common base object items.
  //------------------------------------
  
  FunctionDialogBase::setupDialog();
  
  //------------------------------------
  // Setup this dialog's specific items.
  //------------------------------------
  
  len_checkbox->setChecked(len_thres_used);
  len_spinbox->setValue(len_threshold);
  velo_checkbox->setChecked(velo_thres_used);
  velo_spinbox->setValue(velo_threshold);
}

// int Remove::exec()
// {
//   //-----------------------------------------
//   // Set up the dialog according to the
//   //  various settings:
//   //-----------------------------------------
//   
//   setupDialog();
// 
// //   //-----------------------------------------
// //   // Check whether the boxes should be hidden
// //   //  if no elements are enabled in them:
// //   //-----------------------------------------
// //   
// //   QAbstractButton* bt;
// // 
// //   QList<QAbstractButton*> gl = _range_group->buttons();
// //   int group_sz = gl.size();
// //   bool found = false;
// //   for(int i = 0; i < group_sz; ++i)
// //   {
// //     bt = gl.at(i);
// //     if(bt->isEnabled())
// //     {
// //       found = true;
// //       break;
// //     }
// //   }
// //   rangeBox->setEnabled(found);
// //   rangeBox->setVisible(found);
// //   
// //   gl = _parts_group->buttons();
// //   group_sz = gl.size();
// //   found = false;
// //   for(int i = 0; i < group_sz; ++i)
// //   {
// //     bt = gl.at(i);
// //     if(bt->isEnabled())
// //     {
// //       found = true;
// //       break;
// //     }
// //   }
// //   partsBox->setEnabled(found);
// //   partsBox->setVisible(found);
//   
//   return FunctionDialogBase::exec();
// }

void Remove::read_configuration(MusECore::Xml& xml)
{
	for (;;)
	{
		MusECore::Xml::Token token = xml.parse();
		if (token == MusECore::Xml::Error || token == MusECore::Xml::End)
			break;
			
		const QString& tag = xml.s1();
		switch (token)
		{
			case MusECore::Xml::TagStart:
// REMOVE Tim. citem. Changed.
// 				if (tag == "range")
// 					_range=xml.parseInt();
// 				else if (tag == "parts")
// 					_parts=xml.parseInt();
        
				//-----------------------------------------
				// Handle any common base settings.
				//-----------------------------------------
				
				if(!FunctionDialogBase::read_configuration(tag, xml))
				{
					//-----------------------------------------
					// Handle this dialog's specific settings.
					//-----------------------------------------
					
					if (tag == "range")
						_range = xml.parseInt();
					else if (tag == "parts")
						_parts = xml.parseInt();
					else if (tag == "velo_threshold")
						velo_threshold=xml.parseInt();
					else if (tag == "velo_thres_used")
						velo_thres_used=xml.parseInt();
					else if (tag == "len_threshold")
						len_threshold=xml.parseInt();
					else if (tag == "len_thres_used")
						len_thres_used=xml.parseInt();
					else
						xml.unknown("Erase");
				}
				break;
				
			case MusECore::Xml::TagEnd:
				if (tag == "erase")
					return;
				
			default:
				break;
		}
	}
}

void Remove::write_configuration(int level, MusECore::Xml& xml)
{
  xml.tag(level++, "erase");
  
// REMOVE Tim. citem. Changed.
//  xml.intTag(level, "range", range);
//  xml.intTag(level, "parts", parts);
  
  //-----------------------------------------
  // Write any common base settings.
  //-----------------------------------------
  
  FunctionDialogBase::write_configuration(level, xml);
  
  //-----------------------------------------
  // Write this dialog's specific settings.
  //-----------------------------------------
  
  xml.intTag(level, "range", _range);
  xml.intTag(level, "parts", _parts);
  xml.intTag(level, "velo_threshold", velo_threshold);
  xml.intTag(level, "velo_thres_used", velo_thres_used);
  xml.intTag(level, "len_threshold", len_threshold);
  xml.intTag(level, "len_thres_used", len_thres_used);
  
  xml.tag(level, "/erase");
}

} // namespace MusEGui

