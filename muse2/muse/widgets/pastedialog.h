//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: pastedialog.h,v 1.1.1.1 2011/05/05 18:51:04 flo93 Exp $
//  (C) Copyright 2011 Florian Jung (flo93@sourceforge.net)
//
//  This program is free software; you can redistribute it and/or
//  modify it under the terms of the GNU General Public License
//  as published by the Free Software Foundation; version 2 of
//  the License, or (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//
//=========================================================

#ifndef __PASTEDIALOG_H__
#define __PASTEDIALOG_H__

#include "ui_pastedialogbase.h"
#include <QString>

namespace MusECore {
class Xml;
}

namespace MusEGui {

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

		static int insert_method;
		static int number;
		static int raster;
		static bool all_in_one_track;
		static bool clone;
		
		static void read_configuration(MusECore::Xml& xml);
		void write_configuration(int level, MusECore::Xml& xml);
		
	
	public slots:
		int exec();
};

} // namespace MusEGui

#endif

