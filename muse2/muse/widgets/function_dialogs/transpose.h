//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: transpose.h,v 1.1.1.1 2011/05/05 18:51:04 flo93 Exp $
//  (C) Copyright 2011 Florian Jung (flo93@sourceforge.net)
//=========================================================

#ifndef __TRANSPOSE_H__
#define __TRANSPOSE_H__

#include "ui_transposebase.h"

class QButtonGroup;
class Xml;

class Transpose : public QDialog, public Ui::TransposeBase
{
		Q_OBJECT
		QButtonGroup* range_group;
		
	protected slots:
		void accept();
		void pull_values();

	public:
		Transpose(QWidget* parent = 0);

		int range;
		int amount;
		
		void read_configuration(Xml& xml);
		void write_configuration(int level, Xml& xml);
		
		
	public slots:
		int exec();
};

#endif

