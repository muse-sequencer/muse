//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: setlen.h,v 1.1.1.1 2011/05/05 18:51:04 flo93 Exp $
//  (C) Copyright 2011 Florian Jung (flo93@sourceforge.net)
//=========================================================

#ifndef __SETLEN_H__
#define __SETLEN_H__

#include "ui_setlenbase.h"

class QButtonGroup;

class Setlen : public QDialog, public Ui::SetlenBase
{
	private:
		Q_OBJECT
		QButtonGroup* range_group;
		
	protected slots:
		void accept();
		void pull_values();

	public:
		Setlen(QWidget* parent = 0);

		int range;
		int len;
		
	public slots:
		int exec();
};

#endif

