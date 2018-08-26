//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: crescendo.cpp,v 1.1.1.1 2011/05/05 18:51:04 flo93 Exp $
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
#include "crescendo.h"
#include "xml.h"

namespace MusEGui {

int Crescendo::start_val = 80;
int Crescendo::end_val = 130;
bool Crescendo::absolute = 0;
  
Crescendo::Crescendo(QWidget* parent)
	: FunctionDialogBase(parent)
{
	setupUi(this);
  
  //--------------------------------------------
  // Set range and parts containers if available.
  //--------------------------------------------
  
  _range_container = rangeBox;
  _parts_container = partsBox;
  
  //--------------------------------------------
  // Add element widgets to range and parts groups.
  //--------------------------------------------
  
  _range_group->addButton(looped_events_button, FunctionLoopedButton);
  _range_group->addButton(selected_looped_button, FunctionSelectedLoopedButton);
  
  _parts_group->addButton(not_all_parts_button, FunctionSelectedPartsButton);
  _parts_group->addButton(all_parts_button, FunctionAllPartsButton);
  
	
	connect(absolute_button, SIGNAL(toggled(bool)), SLOT(absolute_changed(bool)));
}

void Crescendo::pull_values()
{
  //--------------------------------------------
  // Grab IDs or values from common base object
  //  (range and parts groups etc.)
  //--------------------------------------------
  
  FunctionDialogBase::pull_values();
  
  //--------------------------------------------
  // Grab this dialog's specific IDs or values.
  //--------------------------------------------
  
	start_val = start_spinbox->value();
	end_val = end_spinbox->value();
	absolute = absolute_button->isChecked();
}

void Crescendo::setupDialog()
{
  //------------------------------------
  // Setup common base object items.
  //------------------------------------
  
  FunctionDialogBase::setupDialog();
  
  //------------------------------------
  // Setup this dialog's specific items.
  //------------------------------------
  
  start_spinbox->setValue(start_val);
  end_spinbox->setValue(end_val);
  absolute_button->setChecked(absolute);
}


void Crescendo::read_configuration(MusECore::Xml& xml)
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
				        
				//-----------------------------------------
				// Handle any common base settings.
				//-----------------------------------------
				
				if(!FunctionDialogBase::read_configuration(tag, xml))
				{
					//-----------------------------------------
					// Handle this dialog's specific settings.
					//-----------------------------------------
					
					if (tag == "start")
						start_val=xml.parseInt();
					else if (tag == "end")
						end_val=xml.parseInt();
					else if (tag == "absolute")
						absolute=xml.parseInt();
					else
						xml.unknown("Crescendo");
				}
				break;
				
			case MusECore::Xml::TagEnd:
				if (tag == "crescendo")
					return;
				
			default:
				break;
		}
	}
}

void Crescendo::write_configuration(int level, MusECore::Xml& xml)
{
	xml.tag(level++, "crescendo");
  
  //-----------------------------------------
  // Write any common base settings.
  //-----------------------------------------
  
  FunctionDialogBase::write_configuration(level, xml);
  
  //-----------------------------------------
  // Write this dialog's specific settings.
  //-----------------------------------------
  
	xml.intTag(level, "start", start_val);
	xml.intTag(level, "end", end_val);
	xml.intTag(level, "absolute", absolute);
	xml.tag(level, "/crescendo");
}

void Crescendo::absolute_changed(bool val)
{
	if (val)
	{
		start_spinbox->setMaximum(127);
		start_spinbox->setSuffix("");
		end_spinbox->setMaximum(127);
		end_spinbox->setSuffix("");
	}
	else
	{
		start_spinbox->setMaximum(12700);
		start_spinbox->setSuffix(" %");
		end_spinbox->setMaximum(12700);
		end_spinbox->setSuffix(" %");
	}
}

} // namespace MusEGui
