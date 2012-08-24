//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: ./muse/waveedit/editgain.h $
//
//  Copyright (C) 1999-2011 by Werner Schweer and others
//
// Author: Mathias Lundgren <lunar_shuttle@users.sf.net>, (C) 2005
//
// Copyright: See COPYING file that comes with this distribution
//
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
//
// C++ Interface: editgain
//
// Description: 
//
//

#ifndef EDITGAIN_H
#define EDITGAIN_H

#include "ui_editgainbase.h"

class QDialog;

namespace MusEGui {

class EditGain : public QDialog, public Ui::EditGainBase
{
      Q_OBJECT
public:
    EditGain(QWidget* parent = 0, int initGainValue=100);

    ~EditGain();
    int getGain();

private:
    int gain;

private slots:
    void resetPressed();
    void applyPressed();
    void cancelPressed();
    void gainChanged(int value);
};

} // namespace MusEGui

#endif
