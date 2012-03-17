//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: arrangercolumns.h, flo93 $
//
//  (C) Copyright 2012 Florian Jung (florian.a.jung@web.de)
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

#ifndef __ARRANGERCOLUMNS_H__
#define __ARRANGERCOLUMNS_H__

#include "ui_arrangercolumnsbase.h"
#include <QDialog>

namespace MusEGui {

class ArrangerColumns : public QDialog, private Ui::ArrangerColumnsBase
{
	Q_OBJECT
	
	public:
		ArrangerColumns(QWidget* parent);
	
	private slots:
		void ctrlTypeChanged(int idx);
		void somethingChanged();
		void initList();
		void itemSelected(int idx);
		void addEntry();
		void delEntry();
		
		QString getListEntryString(int idx);
	
	private:
		bool ignoreSomethingChanged;
};

} // namespace MusEGui

#endif

