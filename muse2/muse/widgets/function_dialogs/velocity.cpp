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

int Velocity::range = 1;
int Velocity::rateVal = 100;
int Velocity::offsetVal = 0;
      
//---------------------------------------------------------
//   Velocity
//---------------------------------------------------------

Velocity::Velocity(QWidget* parent)
   : QDialog(parent)
      {
      setupUi(this);
      rangeGroup = new QButtonGroup;
      rangeGroup->addButton(allEvents,0);
      rangeGroup->addButton(selectedEvents,1);
      rangeGroup->addButton(loopedEvents,2);
      rangeGroup->addButton(selectedLooped,3);
      }

//---------------------------------------------------------
//   accept
//---------------------------------------------------------

void Velocity::accept()
      {
      pullValues();
      QDialog::accept();
      }

//---------------------------------------------------------
//   pullValues
//---------------------------------------------------------

void Velocity::pullValues()
      {
      range     = rangeGroup->checkedId();
      rateVal   = rate->value();
      offsetVal = offset->value();
      }

//---------------------------------------------------------
//   exec
//---------------------------------------------------------

int Velocity::exec()
      {
      rangeGroup->button(range)->setChecked(true);
      rate->setValue(rateVal);
      offset->setValue(offsetVal);
      
      return QDialog::exec();
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
				if (tag == "range")
					range=xml.parseInt();
				else if (tag == "rate")
					rateVal=xml.parseInt();
				else if (tag == "offset")
					offsetVal=xml.parseInt();
				else
					xml.unknown("ModVelo");
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
	xml.intTag(level, "range", range);
	xml.intTag(level, "offset", offsetVal);
	xml.intTag(level, "rate", rateVal);
	xml.tag(level, "/mod_velo");
}

} // namespace MusEGui
