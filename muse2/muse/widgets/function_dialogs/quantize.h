//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: quantize.h,v 1.1.1.1 2011/05/05 18:51:04 flo93 Exp $
//  (C) Copyright 2011 Florian Jung (flo93@sourceforge.net)
//=========================================================

#ifndef __QUANTIZE_H__
#define __QUANTIZE_H__

#include "ui_quantbase.h"

class QButtonGroup;
class Xml;

class Quantize : public QDialog, public Ui::QuantBase
{
	private:
		Q_OBJECT
		QButtonGroup* range_group;
		
	protected slots:
		void accept();
		void pull_values();

	public:
		Quantize(QWidget* parent = 0);

		int range;
		int strength;
		int threshold;
		int raster_power2;
		int swing;
		bool quant_len;
		
		void read_configuration(Xml& xml);
		void write_configuration(int level, Xml& xml);
		
		
	public slots:
		int exec();
};

#endif

