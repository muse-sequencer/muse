//=========================================================
//  MusE
//  Linux Music Editor
//    tracks_duplicate.h
//  (C) Copyright 2011 Tim E. Real (terminator356 on sourceforge.net)
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

#ifndef __TRACKS_DUPLICATE_H__
#define __TRACKS_DUPLICATE_H__

#include "ui_tracks_duplicate_base.h"

namespace MusEGui {
  
class DuplicateTracksDialog : public QDialog, public Ui::DuplicateTracksBase
{
  Q_OBJECT
  protected:
          
  protected slots:
    //void accept();

  public slots:
    //int exec();

  public:
    DuplicateTracksDialog(bool audio, bool midi, bool drum, QWidget* parent = 0);

    int copies() const { return copiesSpinBox->value(); }
    bool copyStdCtrls() const { return standardCtrlsCheckBox->isChecked(); }
    bool copyPlugins() const { return pluginsCheckBox->isChecked(); }
    bool copyPluginCtrls() const { return pluginCtrlsCheckBox->isChecked(); }
    
    bool allRoutes() const { return allRoutesRadioButton->isChecked(); }
    bool defaultRoutes() const { return defaultRoutesRadioButton->isChecked(); }
    
    bool copyParts() const { return copyPartsCheckBox->isChecked(); }
};
  
} // namespace MusEGui

#endif

