//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: gatetime.cpp,v 1.1.1.1 2003/10/27 18:54:37 wschweer Exp $
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
#include <QDialog>

#include "gatetime.h"

#include "xml.h"
#include "song.h"

namespace MusEGui {

int GateTime::range = 1;
int GateTime::rateVal = 100;
int GateTime::offsetVal = 0;
      
//---------------------------------------------------------
//   GateTime
//---------------------------------------------------------

GateTime::GateTime(QWidget* parent)
   : QDialog(parent)
      {
      setupUi(this);
      rangeGroup = new QButtonGroup(rangeBox);
      rangeGroup->addButton(allButton, 0);
      rangeGroup->addButton(selButton, 1);
      rangeGroup->addButton(loopButton, 2);
      rangeGroup->addButton(sloopButton, 3);
      rangeGroup->setExclusive(true);
      }

//---------------------------------------------------------
//   accept
//---------------------------------------------------------

void GateTime::accept()
      {
      pullValues();
      QDialog::accept();
      }

//---------------------------------------------------------
//   pullValues
//---------------------------------------------------------

void GateTime::pullValues()
      {
      range     = rangeGroup->checkedId();
      rateVal   = rate->value();
      offsetVal = offset->value();
      }

//---------------------------------------------------------
//   exec
//---------------------------------------------------------

int GateTime::exec()
      {
      rangeGroup->button(range)->setChecked(true);
      rate->setValue(rateVal);
      offset->setValue(offsetVal);
      
      return QDialog::exec();
      }


void GateTime::read_configuration(MusECore::Xml& xml)
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
					xml.unknown("ModLen");
				break;
				
			case MusECore::Xml::TagEnd:
				if (tag == "mod_len")
					return;
				
			default:
				break;
		}
	}
}

void GateTime::write_configuration(int level, MusECore::Xml& xml)
{
	xml.tag(level++, "mod_len");
	xml.intTag(level, "range", range);
	xml.intTag(level, "offset", offsetVal);
	xml.intTag(level, "rate", rateVal);
	xml.tag(level, "/mod_len");
}

} // namespace MusEGui
