//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: genset.h,v 1.3 2004/01/25 09:55:17 wschweer Exp $
//
//  (C) Copyright 2001 Werner Schweer (ws@seh.de)
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

#ifndef __GENSET_H__
#define __GENSET_H__

#include "ui_gensetbase.h"

#include <QShowEvent>

namespace MusEWidget {

//---------------------------------------------------------
//   GlobalSettingsConfig
//---------------------------------------------------------

class GlobalSettingsConfig : public QDialog, public Ui::GlobalSettingsDialogBase {
      Q_OBJECT

   private slots:
      void updateSettings();
      void apply();
      void ok();
      void cancel();
      void mixerCurrent();
      void mixer2Current();
      void bigtimeCurrent();
      void arrangerCurrent();
      void transportCurrent();
      void selectInstrumentsPath();
      void defaultInstrumentsPath();

    protected:
      void showEvent(QShowEvent*);
      QButtonGroup *startSongGroup;
      
   public:
      GlobalSettingsConfig(QWidget* parent=0);
      };

} // namespace MusEWidget

#endif
