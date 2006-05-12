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

#include "song.h"
#include "transport.h"
#include "widgets/doublelabel.h"
#include "widgets/siglabel.h"
#include "globals.h"
#include "icons.h"
#include "awl/posedit.h"
#include "sync.h"
#include "shortcuts.h"
#include "gconfig.h"
#include "muse.h"
#include "al/sig.h"
#include "al/tempo.h"

static const char* recordTransportText   = QT_TR_NOOP("Click this button to enable recording");
static const char* stopTransportText     = QT_TR_NOOP("Click this button to stop playback");
static const char* playTransportText     = QT_TR_NOOP("Click this button to start playback");
static const char* startTransportText    = QT_TR_NOOP("Click this button to rewind to start position");
static const char* frewindTransportText  = QT_TR_NOOP("Click this button to rewind");
static const char* fforwardTransportText = QT_TR_NOOP("Click this button to forward current play position");

//---------------------------------------------------------
//   Transport
//---------------------------------------------------------

Transport::Transport()
      {
      setupUi(this);

      connect(recMode, SIGNAL(activated(int)), SLOT(setRecMode(int)));
      connect(cycleMode, SIGNAL(activated(int)), SLOT(setCycleMode(int)));

      //-----------------------------------------------------
      //  loop flags
      //-----------------------------------------------------

      connect(punchinButton,  SIGNAL(toggled(bool)), song, SLOT(setPunchin(bool)));
      connect(punchoutButton, SIGNAL(toggled(bool)), song, SLOT(setPunchout(bool)));
      connect(loopButton,     SIGNAL(toggled(bool)), song, SLOT(setLoop(bool)));

      connect(song, SIGNAL(punchinChanged(bool)),  punchinButton, SLOT(setChecked(bool)));
      connect(song, SIGNAL(punchoutChanged(bool)), punchoutButton, SLOT(setChecked(bool)));
      connect(song, SIGNAL(loopChanged(bool)),     loopButton, SLOT(setChecked(bool)));

      //-----------------------------------------------------
      //  Transport Buttons
      //-----------------------------------------------------

      time2->setSmpte(true);

      buttons[0] = tb1;
      tb1->setWhatsThis(tr(startTransportText));

      buttons[1] = tb2;
      tb2->setAutoRepeat(true);
      tb2->setWhatsThis(tr(frewindTransportText));

      buttons[2] = tb3;
      tb3->setAutoRepeat(true);
      tb3->setWhatsThis(tr(fforwardTransportText));

      buttons[3] = tb4;
      tb4->setChecked(true);     // set STOP
      tb4->setWhatsThis(tr(stopTransportText));

      buttons[4] = tb5;
      tb5->setWhatsThis(tr(playTransportText));

      buttons[5] = tb6;
      tb6->setIcon(QIcon(*record_on_Icon));
      tb6->setWhatsThis(tr(recordTransportText));

      connect(buttons[0], SIGNAL(clicked()),     song, SLOT(rewindStart()));
      connect(buttons[1], SIGNAL(clicked()),     song, SLOT(rewind()));
      connect(buttons[2], SIGNAL(clicked()),     song, SLOT(forward()));
      connect(buttons[3], SIGNAL(clicked(bool)), SLOT(stopToggled(bool)));
      connect(buttons[4], SIGNAL(clicked(bool)), SLOT(playToggled(bool)));
      connect(buttons[5], SIGNAL(clicked(bool)), song, SLOT(setRecord(bool)));

      connect(song, SIGNAL(recordChanged(bool)), SLOT(setRecord(bool)));

      //-----------------------------------------------------
      //  AQ - Click - Sync
      //-----------------------------------------------------

      clickButton->setShortcut(shortcuts[SHRT_TOGGLE_METRO].key);

      connect(quantizeButton, SIGNAL(clicked(bool)), song, SLOT(setQuantize(bool)));
      connect(clickButton, SIGNAL(clicked(bool)), song, SLOT(setClick(bool)));
      connect(syncButton, SIGNAL(clicked(bool)), SLOT(syncButtonClicked(bool)));
//TD      connect(&extSyncFlag, SIGNAL(valueChanged(bool)), SLOT(syncChanged(bool)));

      connect(song, SIGNAL(quantizeChanged(bool)), this, SLOT(setQuantizeFlag(bool)));
      connect(song, SIGNAL(clickChanged(bool)), clickButton, SLOT(setChecked(bool)));

      //-----------------------------------------------------
      //  Tempo/Sig
      //-----------------------------------------------------

      connect(masterButton, SIGNAL(clicked(bool)), song, SLOT(setMasterFlag(bool)));

      //-----------------------------------------------------

      connect(tl1,   SIGNAL(valueChanged(const Pos&)), SLOT(lposChanged(const Pos&)));
      connect(tl2,   SIGNAL(valueChanged(const Pos&)), SLOT(rposChanged(const Pos&)));
      connect(time1, SIGNAL(valueChanged(const Pos&)), SLOT(cposChanged(const Pos&)));
      connect(time2, SIGNAL(valueChanged(const Pos&)), SLOT(cposChanged(const Pos&)));

      slider->setRange(0, song->len());
      connect(song, SIGNAL(lenChanged(const AL::Pos&)), SLOT(setLen(const AL::Pos&)));
      connect(slider,SIGNAL(sliderMoved(int)),                     SLOT(cposChanged(int)));
      connect(song,  SIGNAL(posChanged(int,const AL::Pos&, bool)), SLOT(setPos(int,const AL::Pos&)));
      connect(tempo, SIGNAL(tempoChanged(int)),      song,         SLOT(setTempo(int)));
      connect(song,  SIGNAL(playChanged(bool)),                    SLOT(setPlay(bool)));
      connect(song,  SIGNAL(songChanged(int)), this,               SLOT(songChanged(int)));
      setValues();
      }

//---------------------------------------------------------
//   setValues
//---------------------------------------------------------

void Transport::setValues()
      {
      punchinButton->setChecked(song->punchin());
      loopButton->setChecked(song->loop());
      punchoutButton->setChecked(song->punchout());
      quantizeButton->setChecked(song->quantize());
      clickButton->setChecked(song->click());
      syncButton->setChecked(extSyncFlag);
      setPos(0, song->cpos());
      setPos(1, song->lpos());
      setPos(2, song->rpos());
      }

//---------------------------------------------------------
//   setLen
//	song len changed
//---------------------------------------------------------

void Transport::setLen(const AL::Pos& len)
      {
	slider->setRange(0, len.tick());
      }

//---------------------------------------------------------
//   setTimesig
//---------------------------------------------------------

void Transport::setTimesig(int /*z*/, int /*n*/)
      {
//TD      tempo->setTimesig(z, n);
      }

//---------------------------------------------------------
//   setPos
//---------------------------------------------------------

void Transport::setPos(int idx, const AL::Pos& pos)
      {
      switch (idx) {
            case 0:
                  time1->setValue(pos);
                  time2->setValue(pos);
                  slider->setValue(pos.tick());
                  {
            	int tp = AL::tempomap.tempo(pos.tick());
                  AL::TimeSignature sig = AL::sigmap.timesig(pos.tick());
                  int z = sig.z;
                  int n = sig.n;
                  setTimesig(z, n);
            	tempo->setTempo(tp);
                  }
                  break;
            case 1:
                  tl1->setValue(pos);
                  break;
            case 2:
                  tl2->setValue(pos);
                  break;
            }
      }

//---------------------------------------------------------
//   cposChanged
//---------------------------------------------------------

void Transport::cposChanged(int tick)
      {
      song->setPos(0, tick);
      }

//---------------------------------------------------------
//   cposChanged
//---------------------------------------------------------

void Transport::cposChanged(const Pos& pos)
      {
      song->setPos(0, pos.tick());
      }

//---------------------------------------------------------
//   lposChanged
//---------------------------------------------------------

void Transport::lposChanged(const Pos& pos)
      {
      song->setPos(1, pos.tick());
      }

//---------------------------------------------------------
//   rposChanged
//---------------------------------------------------------

void Transport::rposChanged(const Pos& pos)
      {
      song->setPos(2, pos.tick());
      }

//---------------------------------------------------------
//   setPlay
//---------------------------------------------------------

void Transport::setPlay(bool f)
      {
      buttons[3]->setChecked(!f);
      buttons[4]->setChecked(f);
      }

//---------------------------------------------------------
//   setMasterFlag
//---------------------------------------------------------

void Transport::setMasterFlag(bool f)
      {
      masterButton->setChecked(f);
      }

//---------------------------------------------------------
//   setQuantizeFlag
//---------------------------------------------------------

void Transport::setQuantizeFlag(bool f)
      {
      quantizeButton->setChecked(f);
      }

//---------------------------------------------------------
//   setSyncFlag
//---------------------------------------------------------

void Transport::setSyncFlag(bool f)
      {
      syncButton->setChecked(f);
      }

//---------------------------------------------------------
//   toggleRecMode
//---------------------------------------------------------

void Transport::setRecMode(int id)
      {
      song->setRecMode(id);
      }

//---------------------------------------------------------
//   toggleCycleMode
//---------------------------------------------------------

void Transport::setCycleMode(int id)
      {
      song->setCycleMode(id);
      }

//---------------------------------------------------------
//   songChanged
//---------------------------------------------------------

void Transport::songChanged(int flags)
      {
      int cpos  = song->cpos();
      if (flags & (SC_MASTER | SC_TEMPO)) {
            if (extSyncFlag) {
                  tempo->setTempo(0);
                  }
            else {
      		int t = AL::tempomap.tempo(cpos);
                  tempo->setTempo(t);
                  }
            }
      if (flags & SC_SIG) {
            AL::TimeSignature sig = AL::sigmap.timesig(cpos);
            int z = sig.z;
            int n = sig.n;
            setTimesig(z, n);
            }
      if (flags & SC_MASTER)
            masterButton->setChecked(song->masterFlag());
      }

//---------------------------------------------------------
//   syncChanged
//---------------------------------------------------------

void Transport::syncChanged(bool flag)
      {
      syncButton->setChecked(flag);
      buttons[0]->setEnabled(!flag);      // goto start
      buttons[1]->setEnabled(!flag);      // rewind
      buttons[2]->setEnabled(!flag);      // forward
      buttons[3]->setEnabled(!flag);      // stop
      buttons[4]->setEnabled(!flag);      // play
      slider->setEnabled(!flag);
      masterButton->setEnabled(!flag);
      if (flag) {
            masterButton->setChecked(false);
            song->setMasterFlag(false);
            tempo->setValue(0);         // slave mode: show "extern"
            }
      else
            tempo->setValue(AL::tempomap.tempo(song->cpos()));
      muse->playAction->setEnabled(!flag);
      muse->startAction->setEnabled(!flag);
      muse->stopAction->setEnabled(!flag);
      muse->rewindAction->setEnabled(!flag);
      muse->forwardAction->setEnabled(!flag);
      }

//---------------------------------------------------------
//   stopToggled
//---------------------------------------------------------

void Transport::stopToggled(bool val)
      {
      if (val)
            song->setStop(true);
      else {
            buttons[3]->setChecked(true);
            }
      }

//---------------------------------------------------------
//   playToggled
//---------------------------------------------------------

void Transport::playToggled(bool val)
      {
      if (val)
            song->setPlay(true);
      else {
            buttons[4]->setChecked(true);
            }
      }

//---------------------------------------------------------
//   setRecord
//---------------------------------------------------------

void Transport::setRecord(bool flag)
      {
      buttons[5]->setChecked(flag);
      }

//---------------------------------------------------------
//   syncButtonClicked
//---------------------------------------------------------

void Transport::syncButtonClicked(bool flag)
      {
	extSyncFlag = flag;
      }

//---------------------------------------------------------
//   syncChanged
//---------------------------------------------------------

void Transport::syncChanged()
      {
      syncButton->setChecked(extSyncFlag);
      }

//---------------------------------------------------------
//   closeEvent
//---------------------------------------------------------

void Transport::closeEvent(QCloseEvent *ev)
      {
      emit closed();
      QWidget::closeEvent(ev);
      }

