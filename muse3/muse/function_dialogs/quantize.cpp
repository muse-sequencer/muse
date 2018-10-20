//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: quantize.cpp,v 1.1.1.1 2011/05/05 18:51:04 flo93 Exp $
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
#include "quantize.h"
#include "xml.h"

namespace MusEGui {

FunctionReturnDialogFlags_t Quantize::_ret_flags = FunctionReturnNoFlags;
FunctionDialogElements_t Quantize::_elements = FunctionDialogNoElements;
int Quantize::_range = FunctionSelectedEventsButton;
int Quantize::_parts = FunctionSelectedPartsButton;
int Quantize::strength = 90;
int Quantize::threshold = 0;
int Quantize::raster_index = 3;
int Quantize::swing = 0;
bool Quantize::quant_len = 1;

Quantize::Quantize(QWidget* parent)
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
  
  _range_group->addButton(all_events_button, FunctionAllEventsButton);
  _range_group->addButton(selected_events_button,FunctionSelectedEventsButton);
  _range_group->addButton(looped_events_button, FunctionLoopedButton);
  _range_group->addButton(selected_looped_button, FunctionSelectedLoopedButton);
  
  _parts_group->addButton(not_all_parts_button, FunctionSelectedPartsButton);
  _parts_group->addButton(all_parts_button, FunctionAllPartsButton);
}

void Quantize::pull_values()
{
  //--------------------------------------------
  // Grab IDs or values from common base object
  //  (range and parts groups etc.)
  //--------------------------------------------
  
  FunctionDialogBase::pull_values();
  
  //--------------------------------------------
  // Grab this dialog's specific IDs or values.
  //--------------------------------------------
  
	strength = strength_spinbox->value();
	threshold = threshold_spinbox->value();
	raster_index = raster_combobox->currentIndex();
	quant_len = len_checkbox->isChecked();
	swing = swing_spinbox->value();
}

void Quantize::setupDialog()
{
  //------------------------------------
  // Setup common base object items.
  //------------------------------------
  
  FunctionDialogBase::setupDialog();
  
  //------------------------------------
  // Setup this dialog's specific items.
  //------------------------------------
  
	strength_spinbox->setValue(strength);
	threshold_spinbox->setValue(threshold);
  raster_combobox->setCurrentIndex(raster_index);
	len_checkbox->setChecked(quant_len);
	swing_spinbox->setValue(swing);
}

void Quantize::read_configuration(MusECore::Xml& xml)
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
					else if (tag == "strength")
						strength=xml.parseInt();
					else if (tag == "threshold")
						threshold=xml.parseInt();
					else if (tag == "raster")
						raster_index=xml.parseInt();
					else if (tag == "swing")
						swing=xml.parseInt();
					else if (tag == "quant_len")
						quant_len=xml.parseInt();
					else
						xml.unknown("Quantize");
				}
				break;
				
			case MusECore::Xml::TagEnd:
				if (tag == "quantize")
					return;
				
			default:
				break;
		}
	}
}

void Quantize::write_configuration(int level, MusECore::Xml& xml)
{
  xml.tag(level++, "quantize");
  
  //-----------------------------------------
  // Write any common base settings.
  //-----------------------------------------
  
  FunctionDialogBase::write_configuration(level, xml);
  
  //-----------------------------------------
  // Write this dialog's specific settings.
  //-----------------------------------------
  
  xml.intTag(level, "range", _range);
  xml.intTag(level, "parts", _parts);
  xml.intTag(level, "strength", strength);
  xml.intTag(level, "threshold", threshold);
  xml.intTag(level, "raster", raster_index);
  xml.intTag(level, "swing", swing);
  xml.intTag(level, "quant_len", quant_len);
  xml.tag(level, "/quantize");
}

} // namespace MusEGui

