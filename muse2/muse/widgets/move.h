//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: move.h,v 1.1.1.1 2011/05/05 18:51:04 flo93 Exp $
//  (C) Copyright 2011 Florian Jung (flo93@sourceforge.net)
//=========================================================

#ifndef __MOVE_H__
#define __MOVE_H__

#include "ui_movebase.h"

class QButtonGroup;

class Move : public QDialog, public Ui::MoveBase
{
	private:
		Q_OBJECT
		QButtonGroup* range_group;
		
	protected slots:
		void accept();
		void pull_values();

	public:
		Move(QWidget* parent = 0);

		int range;
		int amount;
		
	public slots:
		int exec();
};

#endif

