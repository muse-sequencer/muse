//=========================================================
//  MusE
//  Linux Music Editor
//  warn_bad_timing.h
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
#ifndef __WARN_BAD_TIMING_H__
#define __WARN_BAD_TIMING_H__

#include "ui_warn_bad_timing.h"

namespace MusEGui {

class WarnBadTimingDialog : public QDialog, public Ui::warnBadTimingBase
{
    Q_OBJECT

public:
    WarnBadTimingDialog();
    bool dontAsk() const { return dontAskAgain->isChecked(); }
    void setLabelText(const QString& s) { label->setText(s); }  
};
  
} // namespace MusEGui

#endif
