//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: remove.h,v 1.1.1.1 2011/05/05 18:51:04 flo93 Exp $
//  (C) Copyright 2011 Florian Jung (flo93@sourceforge.net)
//=========================================================

#ifndef __REMOVE_H__
#define __REMOVE_H__

#include "ui_removebase.h"

class QButtonGroup;
class Xml;

class Remove : public QDialog, public Ui::RemoveBase
{
	private:
		Q_OBJECT
		QButtonGroup* range_group;
		
	protected slots:
		void accept();
		void pull_values();

	public:
		Remove(QWidget* parent = 0);

		int range;
		
		void read_configuration(Xml& xml);
		void write_configuration(int level, Xml& xml);
		
		
	public slots:
		int exec();
};

#endif

