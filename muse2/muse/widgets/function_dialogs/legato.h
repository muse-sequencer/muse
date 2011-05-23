//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: legato.h,v 1.1.1.1 2011/05/05 18:51:04 flo93 Exp $
//  (C) Copyright 2011 Florian Jung (flo93@sourceforge.net)
//=========================================================

#ifndef __LEGATO_H__
#define __LEGATO_H__

#include "ui_legatobase.h"

class QButtonGroup;
class Xml;

class Legato : public QDialog, public Ui::LegatoBase
{
	private:
		Q_OBJECT
		QButtonGroup* range_group;
		
	protected slots:
		void accept();
		void pull_values();

	public:
		Legato(QWidget* parent = 0);

		int range;
		int min_len;
		bool allow_shortening;
		
		void read_configuration(Xml& xml);
		void write_configuration(int level, Xml& xml);
		
		
	public slots:
		int exec();
};

#endif

