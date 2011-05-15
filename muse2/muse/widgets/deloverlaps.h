//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: deloverlaps.h,v 1.1.1.1 2011/05/05 18:51:04 flo93 Exp $
//  (C) Copyright 2011 Florian Jung (flo93@sourceforge.net)
//=========================================================

#ifndef __DELOVERLAPS_H__
#define __DELOVERLAPS__H__

#include "ui_deloverlapsbase.h"

class QButtonGroup;

class DelOverlaps : public QDialog, public Ui::DelOverlapsBase
{
	private:
		Q_OBJECT
		QButtonGroup* range_group;
		
	protected slots:
		void accept();
		void pull_values();

	public:
		DelOverlaps(QWidget* parent = 0);

		int range;
		
	public slots:
		int exec();
};

#endif

