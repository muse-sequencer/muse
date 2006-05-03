//=============================================================================
//  MusE
//  Linux Music Editor
//  $Id:$
//
//  Copyright (C) 2002-2006 by Werner Schweer and others
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License version 2.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//=============================================================================

#ifndef __GENSET_H__
#define __GENSET_H__

#include "ui_genset.h"
#include "event.h"
#include "mpevent.h"

//---------------------------------------------------------
//   GlobalSettingsConfig
//---------------------------------------------------------

class GlobalSettingsConfig : public QDialog, Ui::GlobalSettingsDialogBase {
      Q_OBJECT

      QButtonGroup* startSongGroup;

   private slots:
      void apply();
      void ok();
      void cancel();
      void mixerCurrent1();
      void mixerCurrent2();
      void bigtimeCurrent();
      void arrangerCurrent();
      void transportCurrent();
      void recordStopToggled(bool);
      void recordRecordToggled(bool);
      void recordGotoLeftMarkToggled(bool);
      void recordPlayToggled(bool);
      void midiEventReceived(MidiEvent);

   public:
      GlobalSettingsConfig(QWidget* parent=0);
      };

#endif
