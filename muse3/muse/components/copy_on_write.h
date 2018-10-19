//=========================================================
//  MusE
//  Linux Music Editor
//    copy_on_write.h
//  (C) Copyright 2012 Tim E. Real (terminator356 on users dot sourceforge dot net)
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

#ifndef __COPY_ON_WRITE_H__
#define __COPY_ON_WRITE_H__

#include "ui_copy_on_write_base.h"

namespace MusEGui {

class CopyOnWriteDialog : public QDialog, public Ui::CopyOnWriteDialogBase
{
    Q_OBJECT

public:
    CopyOnWriteDialog(QWidget* parent = 0);
    void addProjDirFile(const QString&);
};

} // namespace MusEGui


#endif 
