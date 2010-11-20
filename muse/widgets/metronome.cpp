//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: metronome.cpp,v 1.2.2.1 2009/12/20 05:00:35 terminator356 Exp $
//
//  (C) Copyright 2001 Werner Schweer (ws@seh.de)
//=========================================================

#include <stdio.h>
#include "metronome.h"

#include <QMenu>
#include "globals.h"
#include "song.h"
#include "track.h"
#include "audio.h"

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

      measureNote->setValue(measureClickNote);
      measureVelocity->setValue(measureClickVelo);
      beatNote->setValue(beatClickNote);
      beatVelocity->setValue(beatClickVelo);
      midiChannel->setValue(clickChan+1);
      midiPort->setValue(clickPort+1);

      /*
      precountBars->setValue(preMeasures);
      precountEnable->setChecked(precountEnableFlag);
      precountFromMastertrack->setChecked(precountFromMastertrackFlag);
      precountSigZ->setValue(::precountSigZ);
      precountSigN->setValue(::precountSigN);
      precountPrerecord->setChecked(::precountPrerecord);
      precountPreroll->setChecked(::precountPreroll);
      */

      midiClick->setChecked(midiClickFlag);
      audioBeep->setChecked(audioClickFlag);
      }

//---------------------------------------------------------
//   audioBeepRoutesClicked
//---------------------------------------------------------

void MetronomeConfig::audioBeepRoutesClicked()
{
      if(song->outputs()->size() == 0)
        return;
        
      QMenu* pup = new QMenu;
      
      OutputList* ol = song->outputs();

      int nn = 0;
      for(iAudioOutput iao = ol->begin(); iao != ol->end(); ++iao)
      {
        QAction* action = pup->addAction(QT_TR_NOOP((*iao)->name()));
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
        for(iAudioOutput iao = ol->begin(); iao != ol->end(); ++iao)
        {
          //if(((*iao)->name() == s) && (n == nn))
          if (nn == clickaction->data())
          {
            //(*iao)->setSendMetronome();
            audio->msgSetSendMetronome(*iao, clickaction->isChecked());
            //song->update(SC_ROUTE);
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
      measureClickNote   = measureNote->value();
      measureClickVelo   = measureVelocity->value();
      beatClickNote      = beatNote->value();
      beatClickVelo      = beatVelocity->value();
      clickChan          = midiChannel->value() - 1;
      clickPort          = midiPort->value() - 1;
      preMeasures        = precountBars->value();
      /*
      precountEnableFlag = precountEnable->isChecked();
      precountFromMastertrackFlag = precountFromMastertrack->isChecked();
      ::precountSigZ     = precountSigZ->value();
      ::precountSigN     = precountSigN->value();
      ::precountPrerecord = precountPrerecord->isChecked();
      ::precountPreroll  = precountPreroll->isChecked();
      */
      midiClickFlag      = midiClick->isChecked();
      audioClickFlag     = audioBeep->isChecked();
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

void MetronomeConfig::precountEnableChanged(bool flag)
      {
      /*
      precountBars->setEnabled(flag);
      precountFromMastertrack->setEnabled(flag);
      precountSigZ->setEnabled(flag && !precountFromMastertrack->isChecked());
      precountSigN->setEnabled(flag && !precountFromMastertrack->isChecked());
      */
      }

void MetronomeConfig::precountFromMastertrackChanged(bool flag)
      {
    /*
      precountSigZ->setEnabled(!flag);
      precountSigN->setEnabled(!flag);
      */
      }

void MetronomeConfig::beepVolumeChanged(int volume)
      {
      // this value is directly applied, not using th Apply button, it just seems more usable this way.
      audioClickVolume=volume/100.0;
      }
