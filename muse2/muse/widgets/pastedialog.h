//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: pastedialog.h,v 1.1.1.1 2011/05/05 18:51:04 flo93 Exp $
//  (C) Copyright 2011 Florian Jung (flo93@sourceforge.net)
//=========================================================

#ifndef __PASTEDIALOG_H__
#define __PASTEDIALOG_H__

#include "ui_pastedialogbase.h"
#include <QString>

class Xml;

class PasteDialog : public QDialog, public Ui::PasteDialogBase
{
 	Q_OBJECT
	protected:
		QButtonGroup* button_group;
		QString ticks_to_quarter_string(int ticks);
		
	protected slots:
		void accept();
		void pull_values();
		
		void raster_changed(int);
		void number_changed(int);

	public:
		PasteDialog(QWidget* parent = 0);

		int insert_method;
		int number;
		int raster;
		bool all_in_one_track;
		bool clone;
		
		void read_configuration(Xml& xml);
		void write_configuration(int level, Xml& xml);
		
	
	public slots:
		int exec();
};

#endif

