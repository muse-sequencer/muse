//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: crescendo.h,v 1.1.1.1 2011/05/05 18:51:04 flo93 Exp $
//  (C) Copyright 2011 Florian Jung (flo93@sourceforge.net)
//=========================================================

#ifndef __CRESCENDO_H__
#define __CRESCENDO_H__

#include "ui_crescendobase.h"

class QButtonGroup;
class Xml;

class Crescendo : public QDialog, public Ui::CrescendoBase
{
 	Q_OBJECT
	private:
		
		QButtonGroup* range_group;
		
	protected slots:
		void accept();
		void pull_values();

	public:
		Crescendo(QWidget* parent = 0);

		int range;
		int start_val;
		int end_val;
		bool absolute;
		
		void read_configuration(Xml& xml);
		void write_configuration(int level, Xml& xml);
		
		
	public slots:
		int exec();
	
	private slots:
		void absolute_changed(bool);
};

#endif

