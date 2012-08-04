//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: transport.cpp,v 1.8.2.3 2009/07/01 10:39:42 spamatica Exp $
//
//  (C) Copyright 1999/2000 Werner Schweer (ws@seh.de)
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

#include <QAction>
#include <QComboBox>
#include <QMouseEvent>
#include <QSlider>
#include <QToolButton>
#include <QHBoxLayout>
#include <QVBoxLayout>

#include "awl/posedit.h"

#include "song.h"
#include "transport.h"
#include "doublelabel.h"
#include "siglabel.h"
#include "globals.h"
#include "icons.h"
#include "sync.h"
#include "shortcuts.h"
#include "gconfig.h"
#include "app.h"
#include "audio.h"

namespace MusEGui {

//---------------------------------------------------------
//   toolButton
//---------------------------------------------------------

static QToolButton* newButton(const QString& s, const QString& tt, 
                              bool toggle=false, int height=25, QWidget* parent=0)
      {
      QToolButton* button = new QToolButton(parent);
      button->setFixedHeight(height);
      button->setText(s);
      button->setCheckable(toggle);
      button->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Maximum);
      button->setFocusPolicy(Qt::NoFocus);
      button->setToolTip(tt);
      return button;
      }

static QToolButton* newButton(const QPixmap* pm, const QString& tt, 
                              bool toggle=false, QWidget* parent=0)
      {
      QToolButton* button = new QToolButton(parent);
      button->setFixedHeight(25);
      button->setIcon(QIcon(*pm));
      button->setCheckable(toggle);
      button->setToolTip(tt);
      button->setFocusPolicy(Qt::NoFocus);
      return button;
      }

//---------------------------------------------------------
//    Handle
//    allows moving a root-window with the mouse
//---------------------------------------------------------

Handle::Handle(QWidget* r, QWidget* parent)
   : QWidget(parent)
      {
      rootWin = r;
      setFixedWidth(20);
      setCursor(Qt::PointingHandCursor);
      QPalette palette;
      palette.setColor(this->backgroundRole(), MusEGlobal::config.transportHandleColor);
      this->setPalette(palette);
      setAutoFillBackground(true);
      }

//---------------------------------------------------------
//   mouseMoveEvent
//---------------------------------------------------------

void Handle::mouseMoveEvent(QMouseEvent* ev)
      {
      rootWin->move(ev->globalX()-dx, ev->globalY() - dy);
      }

//---------------------------------------------------------
//   mousePressEvent
//---------------------------------------------------------

void Handle::mousePressEvent(QMouseEvent* ev)
      {
      rootWin->raise();
      dx = ev->globalX() - rootWin->x();
      dy = ev->globalY() - rootWin->y();
      }

//---------------------------------------------------------
//   TempoSig
//    Widget for Tempo + Signature
//---------------------------------------------------------

TempoSig::TempoSig(QWidget* parent)
  : QWidget(parent)
      {
      QBoxLayout* vb1 = new QVBoxLayout;
      vb1->setContentsMargins(0, 0, 0, 0);
      vb1->setSpacing(0);

      QBoxLayout* vb2 = new QVBoxLayout;
      vb2->setContentsMargins(0, 0, 0, 0);
      vb2->setSpacing(0);


      QFrame* f = new QFrame;
      f->setFrameStyle(QFrame::Panel | QFrame::Sunken);
      f->setLineWidth(1);

      // ORCAN get rid of l1 l2 last arguments (parent)?
      l1 = new MusEGui::DoubleLabel(120.0, 20.0, 400.0, 0);
      l1->setFocusPolicy(Qt::NoFocus);
      l1->setSpecialText(QString("extern"));
      vb2->addWidget(l1);
      
      l2 = new MusEGui::SigLabel(4, 4, 0);
      l2->setFocusPolicy(Qt::NoFocus);
      vb2->addWidget(l2);

      f->setLayout(vb2);
      vb1->addWidget(f);

      l3 = new QLabel(tr("Tempo/Sig"));
      l3->setFont(MusEGlobal::config.fonts[2]);
      vb1->addWidget(l3);

      l1->setBackgroundRole(QPalette::Light);
      l1->setAlignment(Qt::AlignCenter);
      l1->setSizePolicy(QSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed));
      l2->setBackgroundRole(QPalette::Light);
      l2->setAlignment(Qt::AlignCenter);
      l2->setSizePolicy(QSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed));
      l3->setAlignment(Qt::AlignCenter);
      l3->setSizePolicy(QSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed));

      connect(l1, SIGNAL(valueChanged(double,int)), SLOT(setTempo(double)));
      ///connect(l2, SIGNAL(valueChanged(int,int)), SIGNAL(sigChanged(int,int)));
      connect(l2, SIGNAL(valueChanged(const AL::TimeSignature&)), SIGNAL(sigChanged(const AL::TimeSignature&)));
      connect(MusEGlobal::muse, SIGNAL(configChanged()), SLOT(configChanged()));

      this->setLayout(vb1);
      }

//---------------------------------------------------------
//   configChanged
//---------------------------------------------------------

void TempoSig::configChanged()
      {
      l3->setFont(MusEGlobal::config.fonts[2]);
      }

//---------------------------------------------------------
//   setTempo
//---------------------------------------------------------

void TempoSig::setTempo(double t)
      {
      int tempo = int ((1000000.0 * 60.0)/t);
      emit tempoChanged(tempo);
      }

//---------------------------------------------------------
//   setTempo
//---------------------------------------------------------

void TempoSig::setTempo(int tempo)
      {
      double t;
      if(tempo == 0)
        t = l1->off() - 1.0;
      else  
        t = (1000000.0 * 60.0)/tempo;
      
      l1->blockSignals(true);
      l1->setValue(t);
      l1->blockSignals(false);
      }

//---------------------------------------------------------
//   setTimesig
//---------------------------------------------------------

void TempoSig::setTimesig(int a, int b)
      {
      l2->setValue(a, b);
      }

//---------------------------------------------------------
//   setRecord
//---------------------------------------------------------

void Transport::setRecord(bool flag)
      {
      buttons[5]->blockSignals(true);
      buttons[5]->setChecked(flag);
      buttons[5]->blockSignals(false);
      }

//---------------------------------------------------------
//   Transport
//---------------------------------------------------------

Transport::Transport(QWidget* parent, const char* name)
  : QWidget(parent, Qt::Window | Qt::WindowStaysOnTopHint | Qt::FramelessWindowHint )  // Possibly also Qt::X11BypassWindowManagerHint
      {
      setObjectName(name);
      setWindowTitle(QString("Muse: Transport"));
      setSizePolicy(QSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum));

      QHBoxLayout* hbox = new QHBoxLayout;
      hbox->setContentsMargins(2, 2, 2, 2);

      lefthandle = new Handle(this);
      hbox->addWidget(lefthandle);

      //-----------------------------------------------------
      //    Record & Cycle Mode
      //-----------------------------------------------------

      QVBoxLayout *box1 = new QVBoxLayout;
      recMode     = new QComboBox;
      recMode->setFocusPolicy(Qt::NoFocus);
      recMode->insertItem(MusECore::Song::REC_OVERDUP, tr("Overdub"));
      recMode->insertItem(MusECore::Song::REC_REPLACE, tr("Replace"));
      recMode->setCurrentIndex(MusEGlobal::song->recMode());

      box1->addWidget(recMode);

      l2 = new QLabel(tr("Rec Mode"));
      l2->setFont(MusEGlobal::config.fonts[2]);
      l2->setAlignment(Qt::AlignCenter);
      connect(recMode, SIGNAL(activated(int)), SLOT(setRecMode(int)));
      box1->addWidget(l2);

      cycleMode = new QComboBox;
      cycleMode->setFocusPolicy(Qt::NoFocus);
      cycleMode->insertItem(MusECore::Song::CYCLE_NORMAL,  tr("Normal"));
      cycleMode->insertItem(MusECore::Song::CYCLE_MIX,     tr("Mix"));
      cycleMode->insertItem(MusECore::Song::CYCLE_REPLACE, tr("Replace"));
      cycleMode->setCurrentIndex(MusEGlobal::song->cycleMode());

      box1->addWidget(cycleMode);

      l3 = new QLabel(tr("Cycle Rec"));
      l3->setFont(MusEGlobal::config.fonts[2]);
      l3->setAlignment(Qt::AlignCenter);
      connect(cycleMode, SIGNAL(activated(int)), SLOT(setCycleMode(int)));
      box1->addWidget(l3);

      box1->setSpacing(0);
      hbox->addLayout(box1);

      //-----------------------------------------------------
      //  loop flags
      //-----------------------------------------------------

      QVBoxLayout *button2 = new QVBoxLayout;
      button2->setSpacing(0);

      QToolButton* b1 = newButton(punchinIcon, tr("punchin"), true);
      QToolButton* b2 = newButton(loopIcon, tr("loop"), true);
      b2->setShortcut(shortcuts[SHRT_TOGGLE_LOOP].key);

      QToolButton* b3 = newButton(punchoutIcon, tr("punchout"), true);
      button2->addWidget(b1);
      button2->addWidget(b2);
      button2->addWidget(b3);
      b1->setToolTip(tr("Punch In"));
      b2->setToolTip(tr("Loop"));
      b3->setToolTip(tr("Punch Out"));
      b1->setWhatsThis(tr("Punch In"));
      b2->setWhatsThis(tr("Loop"));
      b3->setWhatsThis(tr("Punch Out"));

      connect(b1, SIGNAL(toggled(bool)), MusEGlobal::song, SLOT(setPunchin(bool)));
      connect(b2, SIGNAL(toggled(bool)), MusEGlobal::song, SLOT(setLoop(bool)));
      connect(b3, SIGNAL(toggled(bool)), MusEGlobal::song, SLOT(setPunchout(bool)));

      b1->setChecked(MusEGlobal::song->punchin());
      b2->setChecked(MusEGlobal::song->loop());
      b3->setChecked(MusEGlobal::song->punchout());

      connect(MusEGlobal::song, SIGNAL(punchinChanged(bool)),  b1, SLOT(setChecked(bool)));
      connect(MusEGlobal::song, SIGNAL(punchoutChanged(bool)), b3, SLOT(setChecked(bool)));
      connect(MusEGlobal::song, SIGNAL(loopChanged(bool)),     b2, SLOT(setChecked(bool)));

      hbox->addLayout(button2);

      //-----------------------------------------------------
      //  left right mark
      //-----------------------------------------------------

      // ORCAN: should we change PosEdit constructor so we can call it without a parent argument?
      QVBoxLayout *marken = new QVBoxLayout;
      marken->setSpacing(0);
      marken->setContentsMargins(0, 0, 0, 0);

      tl1 = new Awl::PosEdit(0);
      tl1->setMinimumSize(105,0);
      tl1->setSizePolicy(QSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed));
      tl1->setFocusPolicy(Qt::NoFocus);

      marken->addWidget(tl1);

      l5 = new QLabel(tr("Left Mark"));
      l5->setFont(MusEGlobal::config.fonts[2]);
      l5->setAlignment(Qt::AlignCenter);
      marken->addWidget(l5);

      tl2 = new Awl::PosEdit(0);
      tl2->setMinimumSize(105,0);
      tl2->setSizePolicy(QSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed));
      marken->addWidget(tl2);
      tl2->setFocusPolicy(Qt::NoFocus);

      l6 = new QLabel(tr("Right Mark"));
      l6->setFont(MusEGlobal::config.fonts[2]);
      l6->setAlignment(Qt::AlignCenter);
      marken->addWidget(l6);

      hbox->addLayout(marken);

      //-----------------------------------------------------
      //  Transport Buttons
      //-----------------------------------------------------

      QVBoxLayout *box4 = new QVBoxLayout;
      box4->setSpacing(0);
      box4->setContentsMargins(0, 0, 0, 0);

      QHBoxLayout *hbox1 = new QHBoxLayout;
      hbox1->setContentsMargins(0, 0, 0, 0);
      
      time1 = new Awl::PosEdit(0);
      time2 = new Awl::PosEdit(0);
      time2->setSmpte(true);
      time1->setMinimumSize(105,0);
      time2->setMinimumSize(105,0);
      time1->setSizePolicy(QSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed));
      time2->setSizePolicy(QSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed));
      time1->setFocusPolicy(Qt::NoFocus);
      time2->setFocusPolicy(Qt::NoFocus);

      hbox1->addWidget(time1);
      hbox1->addWidget(time2);
      box4->addLayout(hbox1);

      slider = new QSlider;
      slider->setMinimum(0);
      slider->setMaximum(200000);
      slider->setPageStep(1000);
      slider->setValue(0);
      slider->setOrientation(Qt::Horizontal);
      slider->setFocusPolicy(Qt::NoFocus);

      box4->addWidget(slider);

      tb = new QHBoxLayout;
      tb->setSpacing(0);

      buttons[0] = newButton(startIcon, tr("rewind to start"));
      buttons[0]->setWhatsThis(tr("Click this button to rewind to start position"));

      buttons[1] = newButton(frewindIcon, tr("rewind"));
      buttons[1]->setAutoRepeat(true);
      buttons[1]->setWhatsThis(tr("Click this button to rewind"));

      buttons[2] = newButton(fforwardIcon, tr("forward"));
      buttons[2]->setAutoRepeat(true);
      buttons[2]->setWhatsThis(tr("Click this button to forward current play position"));

      buttons[3] = newButton(stopIcon, tr("stop"), true);
      buttons[3]->setChecked(true);     // set STOP
      buttons[3]->setWhatsThis(tr("Click this button to stop playback"));

      buttons[4] = newButton(playIcon, tr("play"), true);
      buttons[4]->setWhatsThis(tr("Click this button to start playback"));

      buttons[5] = newButton(record_on_Icon, tr("record"), true);
      buttons[5]->setWhatsThis(tr("Click this button to enable recording"));

      for (int i = 0; i < 6; ++i)
        {
            buttons[i]->setSizePolicy(QSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed));
            tb->addWidget(buttons[i]);
        }
      connect(buttons[3], SIGNAL(toggled(bool)), SLOT(stopToggled(bool)));
      connect(buttons[4], SIGNAL(toggled(bool)), SLOT(playToggled(bool)));

      connect(buttons[5], SIGNAL(toggled(bool)), MusEGlobal::song, SLOT(setRecord(bool)));
      connect(MusEGlobal::song, SIGNAL(recordChanged(bool)), SLOT(setRecord(bool)));
      connect(buttons[0], SIGNAL(clicked()), MusEGlobal::song, SLOT(rewindStart()));
      connect(buttons[1], SIGNAL(clicked()), MusEGlobal::song, SLOT(rewind()));
      connect(buttons[2], SIGNAL(clicked()), MusEGlobal::song, SLOT(forward()));

      box4->addLayout(tb);
      hbox->addLayout(box4);

      //-----------------------------------------------------
      //  AQ - Click - Sync
      //-----------------------------------------------------

      QVBoxLayout *button1 = new QVBoxLayout;
      button1->setContentsMargins(0, 0, 0, 0);
      button1->setSpacing(0);

      quantizeButton = newButton(tr("AC"), tr("quantize during record"), true,19);

      clickButton    = newButton(tr("Click"), tr("metronom click on/off"), true,19);
      clickButton->setShortcut(shortcuts[SHRT_TOGGLE_METRO].key);

      syncButton     = newButton(tr("Sync"), tr("external sync on/off"), true,19);

      jackTransportButton     = newButton(tr("Jack"), tr("Jack transport sync on/off"), true,19);

      quantizeButton->setChecked(MusEGlobal::song->quantize());
      clickButton->setChecked(MusEGlobal::song->click());
      syncButton->setChecked(MusEGlobal::extSyncFlag.value());
      jackTransportButton->setChecked(MusEGlobal::useJackTransport.value());
      quantizeButton->setFocusPolicy(Qt::NoFocus);
      clickButton->setFocusPolicy(Qt::NoFocus);
      syncButton->setFocusPolicy(Qt::NoFocus);
      jackTransportButton->setFocusPolicy(Qt::NoFocus);

      button1->addWidget(quantizeButton);
      button1->addWidget(clickButton);
      button1->addWidget(syncButton);
      button1->addWidget(jackTransportButton);

      connect(quantizeButton, SIGNAL(toggled(bool)), MusEGlobal::song, SLOT(setQuantize(bool)));
      connect(clickButton, SIGNAL(toggled(bool)), MusEGlobal::song, SLOT(setClick(bool)));

      connect(syncButton, SIGNAL(toggled(bool)), &MusEGlobal::extSyncFlag, SLOT(setValue(bool)));
      connect(jackTransportButton, SIGNAL(toggled(bool)),&MusEGlobal::useJackTransport, SLOT(setValue(bool)));
      connect(&MusEGlobal::extSyncFlag, SIGNAL(valueChanged(bool)), SLOT(syncChanged(bool)));
      connect(&MusEGlobal::useJackTransport, SIGNAL(valueChanged(bool)), SLOT(jackSyncChanged(bool)));

      connect(MusEGlobal::song, SIGNAL(quantizeChanged(bool)), this, SLOT(setQuantizeFlag(bool)));
      connect(MusEGlobal::song, SIGNAL(clickChanged(bool)), this, SLOT(setClickFlag(bool)));

      hbox->addLayout(button1);

      //-----------------------------------------------------
      //  Tempo/Sig
      //-----------------------------------------------------

      QVBoxLayout *box5 = new QVBoxLayout;
      box5->setSpacing(0);
      box5->setContentsMargins(0, 0, 0, 0);


      tempo        = new TempoSig;
      tempo->setSizePolicy(QSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed));
      tempo->setFocusPolicy(Qt::NoFocus);
      box5->addWidget(tempo);

      masterButton = newButton(tr("Master"), tr("use master track"), true);
      masterButton->setSizePolicy(QSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed));
      masterButton->setFocusPolicy(Qt::NoFocus);
      box5->addWidget(masterButton);

      connect(masterButton, SIGNAL(toggled(bool)), MusEGlobal::song, SLOT(setMasterFlag(bool)));

      hbox->addLayout(box5);
      
      //-----------------------------------------------------

      connect(tl1,   SIGNAL(valueChanged(const MusECore::Pos&)), SLOT(lposChanged(const MusECore::Pos&)));
      connect(tl2,   SIGNAL(valueChanged(const MusECore::Pos&)), SLOT(rposChanged(const MusECore::Pos&)));
      connect(time1, SIGNAL(valueChanged(const MusECore::Pos&)), SLOT(cposChanged(const MusECore::Pos&)));
      connect(time2, SIGNAL(valueChanged(const MusECore::Pos&)), SLOT(cposChanged(const MusECore::Pos&)));

      connect(slider,SIGNAL(valueChanged(int)),  SLOT(cposChanged(int)));
      connect(MusEGlobal::song, SIGNAL(posChanged(int, unsigned, bool)), SLOT(setPos(int, unsigned, bool)));
      connect(tempo, SIGNAL(tempoChanged(int)), MusEGlobal::song, SLOT(setTempo(int)));
      connect(tempo, SIGNAL(sigChanged(const AL::TimeSignature&)), SLOT(sigChange(const AL::TimeSignature&)));
      connect(MusEGlobal::song, SIGNAL(playChanged(bool)), SLOT(setPlay(bool)));
      connect(MusEGlobal::song, SIGNAL(songChanged(int)), this, SLOT(songChanged(int)));
      connect(MusEGlobal::muse, SIGNAL(configChanged()), SLOT(configChanged()));


      this->setLayout(hbox);
      righthandle = new Handle(this);
      hbox->addWidget(righthandle);
      }

//---------------------------------------------------------
//   configChanged
//---------------------------------------------------------

void Transport::configChanged()
      {
      l2->setFont(MusEGlobal::config.fonts[2]);
      l3->setFont(MusEGlobal::config.fonts[2]);
      l5->setFont(MusEGlobal::config.fonts[2]);
      l6->setFont(MusEGlobal::config.fonts[2]);

      QPalette pal;
      pal.setColor(lefthandle->backgroundRole(), MusEGlobal::config.transportHandleColor);
      lefthandle->setPalette(pal);
      righthandle->setPalette(pal);
      }

//---------------------------------------------------------
//   setTempo
//---------------------------------------------------------

void Transport::setTempo(int t)
      {
      static int tempoVal = -1;
      if (t != tempoVal) {
            tempo->setTempo(t);
            tempoVal = t;
            }
      blockSignals(true);
      // Make sure positional controls are updated
      unsigned v = MusEGlobal::song->cpos();
      time2->setValue(v); // time2 is SMPTE, it only need tempo updates.
      blockSignals(false);
      }

//---------------------------------------------------------
//   setHandleColor
//---------------------------------------------------------

void Transport::setHandleColor(QColor c)
      {
	QPalette pal;
	pal.setColor(lefthandle->backgroundRole(), c);
	lefthandle->setPalette(pal);
	righthandle->setPalette(pal);
      }

//---------------------------------------------------------
//   setTimesig
//---------------------------------------------------------

void Transport::setTimesig(int z, int n)
      {
      blockSignals(true);
      tempo->setTimesig(z, n);
      
      // Make sure positional controls are updated
      unsigned v = MusEGlobal::song->cpos();
      time1->setValue(v); // time2 is SMPTE. It only need tempo updates.
      
      v = MusEGlobal::song->lpos();      
      tl1->setValue(v);
      v = MusEGlobal::song->rpos();      
      tl2->setValue(v);
      
      blockSignals(false);
      }

//---------------------------------------------------------
//   setPos
//---------------------------------------------------------

void Transport::setPos(int idx, unsigned v, bool)
      {
      switch (idx) {
            case 0:
                  time1->setValue(v);
                  time2->setValue(v);
                  if((unsigned) slider->value() != v)
                  {
                    slider->blockSignals(true);
                    slider->setValue(v);
                    slider->blockSignals(false);
                  }  
                  if (MusEGlobal::song->masterFlag())
                        setTempo(MusEGlobal::tempomap.tempo(v));
                  {
                  int z, n;
                  ///sigmap.timesig(v, z, n);
                  AL::sigmap.timesig(v, z, n);
                  setTimesig(z, n);
                  }
                  break;
            case 1:
                  tl1->setValue(v);
                  break;
            case 2:
                  tl2->setValue(v);
                  break;
            }
      }

//---------------------------------------------------------
//   cposChanged
//---------------------------------------------------------

void Transport::cposChanged(int tick)
      {
      MusEGlobal::song->setPos(0, tick);
      }

//---------------------------------------------------------
//   cposChanged
//---------------------------------------------------------

void Transport::cposChanged(const MusECore::Pos& pos)
      {
      MusEGlobal::song->setPos(0, pos.tick());
      }

//---------------------------------------------------------
//   lposChanged
//---------------------------------------------------------

void Transport::lposChanged(const MusECore::Pos& pos)
      {
      MusEGlobal::song->setPos(1, pos.tick());
      }

//---------------------------------------------------------
//   rposChanged
//---------------------------------------------------------

void Transport::rposChanged(const MusECore::Pos& pos)
      {
      MusEGlobal::song->setPos(2, pos.tick());
      }

//---------------------------------------------------------
//   setPlay
//---------------------------------------------------------

void Transport::setPlay(bool f)
      {
      buttons[3]->blockSignals(true);
      buttons[4]->blockSignals(true);
      buttons[3]->setChecked(!f);
      buttons[4]->setChecked(f);
      buttons[3]->blockSignals(false);
      buttons[4]->blockSignals(false);
      }

//---------------------------------------------------------
//   setMasterFlag
//---------------------------------------------------------

void Transport::setMasterFlag(bool f)
      {
      masterButton->setChecked(f);
      }

//---------------------------------------------------------
//   setClickFlag
//---------------------------------------------------------

void Transport::setClickFlag(bool f)
      {
      clickButton->blockSignals(true);
      clickButton->setChecked(f);
      clickButton->blockSignals(false);
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
      MusEGlobal::song->setRecMode(id);
      }

//---------------------------------------------------------
//   toggleCycleMode
//---------------------------------------------------------

void Transport::setCycleMode(int id)
      {
      MusEGlobal::song->setCycleMode(id);
      }

//---------------------------------------------------------
//   songChanged
//---------------------------------------------------------

void Transport::songChanged(int flags)
      {
      // Is it simply a midi controller value adjustment? Forget it.
      if(flags == SC_MIDI_CONTROLLER)
        return;
    
      slider->setRange(0, MusEGlobal::song->len());
      int cpos  = MusEGlobal::song->cpos();
      int t = MusEGlobal::tempomap.tempo(cpos);
      if (flags & (SC_MASTER | SC_TEMPO)) {
            if (MusEGlobal::extSyncFlag.value())
                  setTempo(0);
            else
                  setTempo(t);
            }
      if (flags & SC_SIG) {
            int z, n;
            ///sigmap.timesig(cpos, z, n);
            AL::sigmap.timesig(cpos, z, n);
            setTimesig(z, n);
            }
      if (flags & SC_MASTER)
            masterButton->setChecked(MusEGlobal::song->masterFlag());
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
            MusEGlobal::song->setMasterFlag(false);
            tempo->setTempo(0);         // slave mode: show "extern"
            }
      else
            tempo->setTempo(MusEGlobal::tempomap.tempo(MusEGlobal::song->cpos()));
      MusEGlobal::playAction->setEnabled(!flag);
      MusEGlobal::startAction->setEnabled(!flag);
      MusEGlobal::stopAction->setEnabled(!flag);
      MusEGlobal::rewindAction->setEnabled(!flag);
      MusEGlobal::forwardAction->setEnabled(!flag);
      }

void Transport::jackSyncChanged(bool flag)
      {
      jackTransportButton->setChecked(flag);
      }
//---------------------------------------------------------
//   stopToggled
//---------------------------------------------------------

void Transport::stopToggled(bool val)
      {
      if (val)
            MusEGlobal::song->setStop(true);
      else {
            buttons[3]->blockSignals(true);
            buttons[3]->setChecked(true);
            buttons[3]->blockSignals(false);
            }
      }

//---------------------------------------------------------
//   playToggled
//---------------------------------------------------------

void Transport::playToggled(bool val)
      {
      if (val)
	MusEGlobal::song->setPlay(true);
      else {
            buttons[4]->blockSignals(true);
            buttons[4]->setChecked(true);
            buttons[4]->blockSignals(false);
            }
      }
      
void Transport::sigChange(const AL::TimeSignature& sig)
{
  MusEGlobal::audio->msgAddSig(MusEGlobal::song->cPos().tick(), sig.z, sig.n);  // Add will replace if found. 
}


} // namespace MusEGui
