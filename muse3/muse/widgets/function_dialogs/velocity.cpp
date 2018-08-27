//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: velocity.cpp,v 1.1.1.1 2003/10/27 18:55:04 wschweer Exp $
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

#include <QButtonGroup>
#include "velocity.h"
#include "xml.h"

namespace MusEGui {

FunctionReturnDialogFlags_t Velocity::_ret_flags = FunctionReturnNoFlags;
FunctionDialogElements_t Velocity::_elements = FunctionDialogNoElements;
int Velocity::_range = FunctionSelectedEventsButton;
int Velocity::_parts = FunctionSelectedPartsButton;
int Velocity::rateVal = 100;
int Velocity::offsetVal = 0;
      
//---------------------------------------------------------
//   Velocity
//---------------------------------------------------------

Velocity::Velocity(QWidget* parent)
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
      
      _range_group->addButton(allEvents, FunctionAllEventsButton);
      _range_group->addButton(selectedEvents,FunctionSelectedEventsButton);
      _range_group->addButton(loopedEvents, FunctionLoopedButton);
      _range_group->addButton(selectedLooped, FunctionSelectedLoopedButton);
      
      _parts_group->addButton(not_all_parts_button, FunctionSelectedPartsButton);
      _parts_group->addButton(all_parts_button, FunctionAllPartsButton);
      }

//---------------------------------------------------------
//   pullValues
//---------------------------------------------------------

void Velocity::pull_values()
      {
      //--------------------------------------------
      // Grab IDs or values from common base object
      //  (range and parts groups etc.)
      //--------------------------------------------
      
      FunctionDialogBase::pull_values();
      
      //--------------------------------------------
      // Grab this dialog's specific IDs or values.
      //--------------------------------------------
      
      rateVal   = rate->value();
      offsetVal = offset->value();
      }

void Velocity::setupDialog()
{
      //------------------------------------
      // Setup common base object items.
      //------------------------------------
      
      FunctionDialogBase::setupDialog();
      
      //------------------------------------
      // Setup this dialog's specific items.
      //------------------------------------
      
      rate->setValue(rateVal);
      offset->setValue(offsetVal);
}

void Velocity::read_configuration(MusECore::Xml& xml)
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
					
					if (tag == "range")
						_range = xml.parseInt();
					else if (tag == "parts")
						_parts = xml.parseInt();
					else if (tag == "rate")
					rateVal=xml.parseInt();
					else if (tag == "offset")
						offsetVal=xml.parseInt();
					else
						xml.unknown("ModVelo");
				}
				break;
				
			case MusECore::Xml::TagEnd:
				if (tag == "mod_velo")
					return;
				
			default:
				break;
		}
	}
}

void Velocity::write_configuration(int level, MusECore::Xml& xml)
{
  xml.tag(level++, "mod_velo");
  
  //-----------------------------------------
  // Write any common base settings.
  //-----------------------------------------
  
  FunctionDialogBase::write_configuration(level, xml);
  
  //-----------------------------------------
  // Write this dialog's specific settings.
  //-----------------------------------------
  
  xml.intTag(level, "range", _range);
  xml.intTag(level, "parts", _parts);
  xml.intTag(level, "offset", offsetVal);
  xml.intTag(level, "rate", rateVal);
  xml.tag(level, "/mod_velo");
}

} // namespace MusEGui
