//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: pasteeventsdialog.h,v 1.1.1.1 2011/05/05 18:51:04 flo93 Exp $
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

#ifndef __PASTEEVENTSDIALOG_H__
#define __PASTEEVENTSDIALOG_H__

#include "ui_pasteeventsdialogbase.h"
#include <QString>

namespace MusECore {
class Xml;
}

namespace MusEGui {

class PasteEventsDialog : public QDialog, public Ui::PasteEventsDialogBase
{
 	Q_OBJECT
	protected:
		QString ticks_to_quarter_string(int ticks);
		
	protected slots:
		void accept();
		void pull_values();
		
		void max_distance_changed(int);
		void raster_changed(int);
		void number_changed(int);

	public:
		PasteEventsDialog(QWidget* parent = 0);

		static int number;
		static int raster;
		static bool always_new_part;
		static bool never_new_part;
		static unsigned max_distance;
		static bool into_single_part;
		bool into_single_part_allowed;
		
		static void read_configuration(MusECore::Xml& xml);
		void write_configuration(int level, MusECore::Xml& xml);
		
	
	public slots:
		int exec();
};

} // namespace MusEGui

#endif

