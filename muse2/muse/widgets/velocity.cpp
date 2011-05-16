//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: velocity.cpp,v 1.1.1.1 2003/10/27 18:55:04 wschweer Exp $
//  (C) Copyright 2001 Werner Schweer (ws@seh.de)
//=========================================================

#include <QButtonGroup>
#include "velocity.h"
#include "xml.h"

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
      
      pullValues();
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

void Velocity::read_configuration(Xml& xml)
{
	for (;;)
	{
		Xml::Token token = xml.parse();
		if (token == Xml::Error || token == Xml::End)
			break;
			
		const QString& tag = xml.s1();
		switch (token)
		{
			case Xml::TagStart:
				if (tag == "range")
					range=xml.parseInt();
				else if (tag == "rate")
					rateVal=xml.parseInt();
				else if (tag == "offset")
					offsetVal=xml.parseInt();
				else
					xml.unknown("ModVelo");
				break;
				
			case Xml::TagEnd:
				if (tag == "mod_velo")
					return;
				
			default:
				break;
		}
	}
}

void Velocity::write_configuration(int level, Xml& xml)
{
	xml.tag(level++, "mod_velo");
	xml.intTag(level, "range", range);
	xml.intTag(level, "offset", offsetVal);
	xml.intTag(level, "rate", rateVal);
	xml.tag(level, "/mod_velo");
}
