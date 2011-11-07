//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: metronome.cpp,v 1.2.2.1 2009/12/20 05:00:35 terminator356 Exp $
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

#include <stdio.h>
#include "metronome.h"

#include <QMenu>
#include "globals.h"
#include "song.h"
#include "track.h"
#include "audio.h"

namespace MusEGui {

//---------------------------------------------------------
//   MetronomeConfig
//---------------------------------------------------------

MetronomeConfig::MetronomeConfig(QDialog* parent)
   : QDialog(parent)
      {
      setupUi(this);
      connect(buttonApply, SIGNAL(clicked()), SLOT(apply()));
      connect(midiClick, SIGNAL(toggled(bool)), SLOT(midiClickChanged(bool)));
      connect(precountEnable, SIGNAL(toggled(bool)), SLOT(precountEnableChanged(bool)));
      connect(precountFromMastertrack, SIGNAL(toggled(bool)),
         SLOT(precountFromMastertrackChanged(bool)));
      connect(audioBeepRoutesButton, SIGNAL(clicked()), SLOT(audioBeepRoutesClicked()));
      connect(volumeSlider, SIGNAL(valueChanged(int)), SLOT(beepVolumeChanged(int)));

      measureNote->setValue(MusEGlobal::measureClickNote);
      measureVelocity->setValue(MusEGlobal::measureClickVelo);
      beatNote->setValue(MusEGlobal::beatClickNote);
      beatVelocity->setValue(MusEGlobal::beatClickVelo);
      midiChannel->setValue(MusEGlobal::clickChan+1);
      midiPort->setValue(MusEGlobal::clickPort+1);

      /*
      precountBars->setValue(MusEGlobal::preMeasures);
      precountEnable->setChecked(precountEnableFlag);
      precountFromMastertrack->setChecked(precountFromMastertrackFlag);
      precountSigZ->setValue(::precountSigZ);
      precountSigN->setValue(::precountSigN);
      precountPrerecord->setChecked(::precountPrerecord);
      precountPreroll->setChecked(::precountPreroll);
      */

      midiClick->setChecked(MusEGlobal::midiClickFlag);
      audioBeep->setChecked(MusEGlobal::audioClickFlag);
      }

//---------------------------------------------------------
//   audioBeepRoutesClicked
//---------------------------------------------------------

void MetronomeConfig::audioBeepRoutesClicked()
{
      if(MusEGlobal::song->outputs()->size() == 0)
        return;
        
      QMenu* pup = new QMenu;
      
      MusECore::OutputList* ol = MusEGlobal::song->outputs();

      int nn = 0;
      for(MusECore::iAudioOutput iao = ol->begin(); iao != ol->end(); ++iao)
      {
        QAction* action = pup->addAction((*iao)->name());
        action->setCheckable(true);
        action->setData(nn);
        if((*iao)->sendMetronome())
          action->setChecked(true);
        ++nn;  
      }  
      
      QAction* clickaction = pup->exec(QCursor::pos());
      if (clickaction)
      {
        //QString s(pup->text(n));
        nn = 0;
        for(MusECore::iAudioOutput iao = ol->begin(); iao != ol->end(); ++iao)
        {
          //if(((*iao)->name() == s) && (n == nn))
          if (nn == clickaction->data())
          {
            //(*iao)->setSendMetronome();
	    MusEGlobal::audio->msgSetSendMetronome(*iao, clickaction->isChecked());
            //MusEGlobal::song->update(SC_ROUTE);
            break;
          }
          ++nn;
        }
      }
      
      delete pup;
      audioBeepRoutesButton->setDown(false);     // pup->exec() catches mouse release event
}

//---------------------------------------------------------
//   accept
//---------------------------------------------------------

void MetronomeConfig::accept()
      {
      apply();
      QDialog::accept();
      }

//---------------------------------------------------------
//   apply
//---------------------------------------------------------

void MetronomeConfig::apply()
      {
      MusEGlobal::measureClickNote   = measureNote->value();
      MusEGlobal::measureClickVelo   = measureVelocity->value();
      MusEGlobal::beatClickNote      = beatNote->value();
      MusEGlobal::beatClickVelo      = beatVelocity->value();
      MusEGlobal::clickChan          = midiChannel->value() - 1;
      MusEGlobal::clickPort          = midiPort->value() - 1;
      MusEGlobal::preMeasures        = precountBars->value();
      /*
      precountEnableFlag = precountEnable->isChecked();
      precountFromMastertrackFlag = precountFromMastertrack->isChecked();
      ::precountSigZ     = precountSigZ->value();
      ::precountSigN     = precountSigN->value();
      ::precountPrerecord = precountPrerecord->isChecked();
      ::precountPreroll  = precountPreroll->isChecked();
      */
      MusEGlobal::midiClickFlag      = midiClick->isChecked();
      MusEGlobal::audioClickFlag     = audioBeep->isChecked();
      //audioVolumeChanged = volumeSlider->value();
      }

//---------------------------------------------------------
//   reject
//---------------------------------------------------------

void MetronomeConfig::reject()
      {
      QDialog::reject();
      }

//---------------------------------------------------------
//   midiClickChanged
//---------------------------------------------------------

void MetronomeConfig::midiClickChanged(bool flag)
      {
      measureNote->setEnabled(flag);
      measureVelocity->setEnabled(flag);
      beatNote->setEnabled(flag);
      beatVelocity->setEnabled(flag);
      midiChannel->setEnabled(flag);
      midiPort->setEnabled(flag);
      }

void MetronomeConfig::precountEnableChanged(bool /*flag*/)
      {
      /*
      precountBars->setEnabled(flag);
      precountFromMastertrack->setEnabled(flag);
      precountSigZ->setEnabled(flag && !precountFromMastertrack->isChecked());
      precountSigN->setEnabled(flag && !precountFromMastertrack->isChecked());
      */
      }

void MetronomeConfig::precountFromMastertrackChanged(bool /*flag*/)
      {
    /*
      precountSigZ->setEnabled(!flag);
      precountSigN->setEnabled(!flag);
      */
      }

void MetronomeConfig::beepVolumeChanged(int volume)
      {
      // this value is directly applied, not using th Apply button, it just seems more usable this way.
      MusEGlobal::audioClickVolume=volume/100.0;
      }

} // namespace MusEGui

