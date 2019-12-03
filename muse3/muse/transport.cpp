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

#include "posedit.h"
#include "sigedit.h"

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
#include "globaldefs.h"
#include "pixmap_button.h"
#include "tempolabel.h"
#include "operations.h"
#include "tempo.h"

namespace MusEGui {

//---------------------------------------------------------
//   toolButton
//---------------------------------------------------------

// Unused since switch to svg.
// static QToolButton* newButton(const QPixmap* pm, const QString& tt, 
//                               bool toggle=false, QWidget* parent=0)
//       {
//       QToolButton* button = new QToolButton(parent);
//       button->setFixedHeight(25);
//       button->setIcon(QIcon(*pm));
//       button->setCheckable(toggle);
//       button->setToolTip(tt);
//       button->setFocusPolicy(Qt::NoFocus);
//       return button;
//       }

static QToolButton* newButton(const QIcon* icon, const QString& tt,
                              bool toggle=false, QWidget* parent=0)
      {
      QToolButton* button = new QToolButton(parent);
      button->setFixedHeight(25);
      button->setIcon(*icon);
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
    this->setStyleSheet("background-color:" + MusEGlobal::config.transportHandleColor.name());
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
      //_curVal = 0.0;
//       _curVal = -1.0;
//       _extern = false;
      
      QBoxLayout* vb1 = new QVBoxLayout;
      vb1->setContentsMargins(0, 0, 0, 0);
      vb1->setSpacing(0);

      QBoxLayout* vb2 = new QVBoxLayout;
      vb2->setContentsMargins(0, 0, 0, 0);
      vb2->setSpacing(0);

      QBoxLayout* hb1 = new QHBoxLayout;
      hb1->setContentsMargins(0, 0, 0, 0);
      hb1->setSpacing(0);

      QFrame* f = new QFrame;
      f->setFrameStyle(QFrame::Panel | QFrame::Sunken);
      f->setLineWidth(1);

      _masterButton = new IconButton(masterTrackOnSVGIcon, masterTrackOffSVGIcon, 0, 0, false, true);
      _masterButton->setContentsMargins(0, 0, 0, 0);
      _masterButton->setCheckable(true);
      _masterButton->setToolTip(tr("Use mastertrack tempo"));
      _masterButton->setSizePolicy(QSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum));
      _masterButton->setFocusPolicy(Qt::NoFocus);
      connect(_masterButton, SIGNAL(toggled(bool)), SLOT(masterToggled(bool)));
      hb1->addWidget(_masterButton);
      
      l3 = new QLabel(tr("Tempo/Sig"));
      l3->setFont(MusEGlobal::config.fonts[2]);
      vb2->addWidget(l3);
      l1 = new TempoEdit();
      l1->setContentsMargins(0, 0, 0, 0);
      l1->setFocusPolicy(Qt::StrongFocus);
      l1->setToolTip(tr("Mastertrack tempo at current position, or fixed tempo"));
      hb1->addWidget(l1);
      vb2->addLayout(hb1);
      
      l2 = new SigEdit(this);
      l2->setContentsMargins(0, 0, 0, 0);
      l2->setFocusPolicy(Qt::StrongFocus);
      l2->setToolTip(tr("Time signature at current position"));

      vb2->addWidget(l2);

      f->setLayout(vb2);
      vb1->addWidget(f);

      l1->setAlignment(Qt::AlignCenter);
      l1->setSizePolicy(QSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed));
      l2->setSizePolicy(QSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed));
      l3->setAlignment(Qt::AlignCenter);
      l3->setSizePolicy(QSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed));

      connect(l1, SIGNAL(tempoChanged(double)), SLOT(newTempo(double)));
      connect(l2, SIGNAL(valueChanged(const MusECore::TimeSignature&)), SIGNAL(sigChanged(const MusECore::TimeSignature&)));
      connect(MusEGlobal::muse, SIGNAL(configChanged()), SLOT(configChanged()));

      connect(l1, SIGNAL(returnPressed()), SIGNAL(returnPressed()));
      connect(l1, SIGNAL(escapePressed()), SIGNAL(escapePressed()));
      connect(l2, SIGNAL(returnPressed()), SIGNAL(returnPressed()));
      connect(l2, SIGNAL(escapePressed()), SIGNAL(escapePressed()));
      
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
//   masterToggled
//---------------------------------------------------------

void TempoSig::masterToggled(bool val)
{
  emit masterTrackChanged(val);
}

bool TempoSig::masterTrack() const
{
  return _masterButton->isChecked();
}

void TempoSig::setMasterTrack(bool on)
{
  _masterButton->blockSignals(true);
  _masterButton->setChecked(on);
  _masterButton->blockSignals(false);
}

//---------------------------------------------------------
//   setExternalMode
//---------------------------------------------------------

void TempoSig::setExternalMode(bool on)
{
  l1->setExternalMode(on);
}

//---------------------------------------------------------
//   newTempo
//---------------------------------------------------------

void TempoSig::newTempo(double t)
      {
        emit tempoChanged(int ((1000000.0 * 60.0) / t));
      }

//---------------------------------------------------------
//   setTempo
//---------------------------------------------------------

void TempoSig::setTempo(int tempo)
      {
      l1->setValue((1000000.0 * 60.0)/tempo);
      }

//---------------------------------------------------------
//   setTimesig
//---------------------------------------------------------

void TempoSig::setTimesig(int a, int b)
      {
      l2->setValue(MusECore::TimeSignature(a, b));
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

      QToolButton* b1 = newButton(punchinSVGIcon, tr("Punch in"), true);
      QToolButton* b2 = newButton(loopSVGIcon, tr("Loop"), true);
      b2->setShortcut(shortcuts[SHRT_TOGGLE_LOOP].key);

      QToolButton* b3 = newButton(punchoutSVGIcon, tr("Punch out"), true);
      button2->addWidget(b1);
      button2->addWidget(b2);
      button2->addWidget(b3);
      b1->setToolTip(tr("Punch in"));
      b2->setToolTip(tr("Loop"));
      b3->setToolTip(tr("Punch out"));
      b1->setWhatsThis(tr("Punch in"));
      b2->setWhatsThis(tr("Loop"));
      b3->setWhatsThis(tr("Punch out"));

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

      QVBoxLayout *marken = new QVBoxLayout;
      marken->setSpacing(0);
      marken->setContentsMargins(0, 0, 0, 0);

      tl1 = new PosEdit(0);
      tl1->setSizePolicy(QSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed));
      tl1->setFocusPolicy(Qt::NoFocus);

      marken->addWidget(tl1);

      l5 = new QLabel(tr("Left Mark"));
      l5->setFont(MusEGlobal::config.fonts[2]);
      l5->setAlignment(Qt::AlignCenter);
      marken->addWidget(l5);

      tl2 = new PosEdit(0);
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
      
      time1 = new PosEdit(0);
      time2 = new PosEdit(0);
      time2->setSmpte(true);
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

      buttons[0] = newButton(rewindToStartSVGIcon, tr("Rewind to Start"));
      buttons[0]->setWhatsThis(tr("Click this button to rewind to start position"));

      buttons[1] = newButton(rewindSVGIcon, tr("Rewind"));
      buttons[1]->setAutoRepeat(true);
      buttons[1]->setWhatsThis(tr("Click this button to rewind"));

      buttons[2] = newButton(fastForwardSVGIcon, tr("Forward"));
      buttons[2]->setAutoRepeat(true);
      buttons[2]->setWhatsThis(tr("Click this button to forward current play position"));

      buttons[3] = newButton(stopSVGIcon, tr("Stop"), true);
      buttons[3]->setChecked(true);     // set STOP
      buttons[3]->setWhatsThis(tr("Click this button to stop playback"));

      buttons[4] = newButton(playSVGIcon, tr("Play"), true);
      buttons[4]->setWhatsThis(tr("Click this button to start playback"));

      buttons[5] = newButton(recMasterSVGIcon, tr("Record"), true);
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

      clickButton = new IconButton(metronomeOnSVGIcon, metronomeOffSVGIcon, 0, 0, false, true);
      clickButton->setContentsMargins(0, 0, 0, 0);
      clickButton->setCheckable(true);
      clickButton->setToolTip(tr("Metronome on/off"));
      clickButton->setSizePolicy(QSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum));
        
      syncButton = new IconButton(externSyncOnSVGIcon, externSyncOffSVGIcon, 0, 0, false, true);
      syncButton->setContentsMargins(0, 0, 0, 0);
      syncButton->setCheckable(true);
      syncButton->setToolTip(tr("External sync on/off"));
      syncButton->setSizePolicy(QSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum));
        
      jackTransportButton = new IconButton(jackTransportOnSVGIcon, jackTransportOffSVGIcon, 0, 0, false, true);
      jackTransportButton->setContentsMargins(0, 0, 0, 0);
      jackTransportButton->setCheckable(true);
      jackTransportButton->setToolTip(tr("Jack Transport on/off"));
      jackTransportButton->setSizePolicy(QSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum));

      transportMasterButton = new IconButton(transportMasterOnSVGIcon, transportMasterOffSVGIcon, 0, 0, false, true);
      transportMasterButton->setContentsMargins(0, 0, 0, 0);
      transportMasterButton->setCheckable(true);
      transportMasterButton->setToolTip(tr("Transport master (on) or slave (off)"));
      transportMasterButton->setSizePolicy(QSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum));

      clickButton->setChecked(MusEGlobal::song->click());
      syncButton->setChecked(MusEGlobal::extSyncFlag);
      jackTransportButton->setChecked(MusEGlobal::useJackTransport);
      transportMasterButton->setChecked(MusEGlobal::transportMasterState);
      clickButton->setFocusPolicy(Qt::NoFocus);
      syncButton->setFocusPolicy(Qt::NoFocus);
      jackTransportButton->setFocusPolicy(Qt::NoFocus);
      transportMasterButton->setFocusPolicy(Qt::NoFocus);

      button1->addStretch();
      button1->addWidget(clickButton);
      button1->addWidget(syncButton);
      button1->addWidget(jackTransportButton);
      button1->addWidget(transportMasterButton);
      button1->addStretch();
      
      connect(clickButton, SIGNAL(toggled(bool)), MusEGlobal::song, SLOT(setClick(bool)));
      connect(syncButton, SIGNAL(toggled(bool)), SLOT(extSyncClicked(bool)));
      connect(jackTransportButton, SIGNAL(toggled(bool)), SLOT(useJackTransportClicked(bool)));
      connect(transportMasterButton, &IconButton::toggled, [this](bool v) { transportMasterClicked(v); } );

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
      box5->addStretch();
      box5->addWidget(tempo);
      box5->addStretch();

      hbox->addLayout(box5);
      
      //-----------------------------------------------------

      connect(tl1,   SIGNAL(valueChanged(const MusECore::Pos&)), SLOT(lposChanged(const MusECore::Pos&)));
      connect(tl2,   SIGNAL(valueChanged(const MusECore::Pos&)), SLOT(rposChanged(const MusECore::Pos&)));
      connect(time1, SIGNAL(valueChanged(const MusECore::Pos&)), SLOT(cposChanged(const MusECore::Pos&)));
      connect(time2, SIGNAL(valueChanged(const MusECore::Pos&)), SLOT(cposChanged(const MusECore::Pos&)));

      connect(slider,SIGNAL(valueChanged(int)),  SLOT(cposChanged(int)));
      connect(MusEGlobal::song, SIGNAL(posChanged(int, unsigned, bool)), SLOT(setPos(int, unsigned, bool)));
      connect(tempo, SIGNAL(tempoChanged(int)), MusEGlobal::song, SLOT(setTempo(int)));
      connect(tempo, SIGNAL(sigChanged(const MusECore::TimeSignature&)), SLOT(sigChange(const MusECore::TimeSignature&)));
      connect(tempo, SIGNAL(masterTrackChanged(bool)), MusEGlobal::song, SLOT(setMasterFlag(bool)));
      connect(tempo, SIGNAL(escapePressed()), SLOT(setFocus()));
      connect(tempo, SIGNAL(returnPressed()), SLOT(setFocus()));
      connect(MusEGlobal::song, SIGNAL(playChanged(bool)), SLOT(setPlay(bool)));
      connect(MusEGlobal::song, SIGNAL(songChanged(MusECore::SongChangedStruct_t)), this, SLOT(songChanged(MusECore::SongChangedStruct_t)));
      connect(MusEGlobal::muse, SIGNAL(configChanged()), SLOT(configChanged()));


      this->setLayout(hbox);
      righthandle = new Handle(this);
      hbox->addWidget(righthandle);
      
      songChanged(SC_EVERYTHING);
      syncChanged(MusEGlobal::extSyncFlag);
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
      tempo->setTempo(t);
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
                  if (!MusEGlobal::extSyncFlag)
                    setTempo(MusEGlobal::tempomap.tempo(v));
                  
                  {
                  int z, n;
                  MusEGlobal::sigmap.timesig(v, z, n);
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
      MusEGlobal::song->setPos(MusECore::Song::CPOS, tick);
      }

//---------------------------------------------------------
//   cposChanged
//---------------------------------------------------------

void Transport::cposChanged(const MusECore::Pos& pos)
      {
      MusEGlobal::song->setPos(MusECore::Song::CPOS, pos.tick());
      }

//---------------------------------------------------------
//   lposChanged
//---------------------------------------------------------

void Transport::lposChanged(const MusECore::Pos& pos)
      {
      MusEGlobal::song->setPos(MusECore::Song::LPOS, pos.tick());
      }

//---------------------------------------------------------
//   rposChanged
//---------------------------------------------------------

void Transport::rposChanged(const MusECore::Pos& pos)
      {
      MusEGlobal::song->setPos(MusECore::Song::RPOS, pos.tick());
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
      tempo->setMasterTrack(f);
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
//   setSyncFlag
//---------------------------------------------------------

void Transport::setSyncFlag(bool f)
      {
      syncButton->blockSignals(true);
      syncButton->setChecked(f);
      syncButton->blockSignals(false);
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

void Transport::songChanged(MusECore::SongChangedStruct_t flags)
      {
      slider->setRange(0, MusEGlobal::song->len());
      int cpos  = MusEGlobal::song->cpos();
      if (flags & (SC_MASTER | SC_TEMPO)) {
            if(!MusEGlobal::extSyncFlag)
              setTempo(MusEGlobal::tempomap.tempo(cpos));
            }
      if (flags & SC_SIG) {
            int z, n;
            MusEGlobal::sigmap.timesig(cpos, z, n);
            setTimesig(z, n);
            }
      if (flags & SC_MASTER)
      {
            tempo->setMasterTrack(MusEGlobal::tempomap.masterFlag());
      }
      if (flags & SC_EXTERNAL_MIDI_SYNC)
      {
            syncChanged(MusEGlobal::extSyncFlag);
      }
      if (flags & SC_USE_JACK_TRANSPORT)
      {
            jackSyncChanged(MusEGlobal::useJackTransport);
      }
      if (flags & SC_TRANSPORT_MASTER)
      {
            transportMasterChanged(MusEGlobal::transportMasterState);
      }
      }

//---------------------------------------------------------
//   syncChanged
//---------------------------------------------------------

void Transport::syncChanged(bool flag)
      {
      syncButton->blockSignals(true);
      syncButton->setChecked(flag);
      syncButton->blockSignals(false);
      buttons[0]->setEnabled(!flag);      // goto start
      buttons[1]->setEnabled(!flag);      // rewind
      buttons[2]->setEnabled(!flag);      // forward
      buttons[3]->setEnabled(!flag);      // stop
      buttons[4]->setEnabled(!flag);      // play
      slider->setEnabled(!flag);
      tempo->setExternalMode(flag);
      if(!flag)
        // Update or initialize the value - this might be the first time setting it.
        tempo->setTempo(MusEGlobal::tempomap.tempo(MusEGlobal::song->cpos()));
     
      MusEGlobal::playAction->setEnabled(!flag);
      MusEGlobal::startAction->setEnabled(!flag);
      MusEGlobal::stopAction->setEnabled(!flag);
      MusEGlobal::rewindAction->setEnabled(!flag);
      MusEGlobal::forwardAction->setEnabled(!flag);
      }

void Transport::jackSyncChanged(bool flag)
      {
      jackTransportButton->blockSignals(true);
      jackTransportButton->setChecked(flag);
      jackTransportButton->blockSignals(false);
      }

void Transport::transportMasterChanged(bool flag)
      {
      transportMasterButton->blockSignals(true);
      transportMasterButton->setChecked(flag);
      transportMasterButton->blockSignals(false);
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
      
void Transport::sigChange(const MusECore::TimeSignature& sig)
{
  // Add will replace if found. 
  MusEGlobal::song->applyOperation(MusECore::UndoOp(MusECore::UndoOp::AddSig,
                            MusEGlobal::song->cPos().tick(), sig.z, sig.n));
}

//---------------------------------------------------------
//   extSyncClicked
//---------------------------------------------------------

void Transport::extSyncClicked(bool v)
{
  MusECore::PendingOperationList operations;
  operations.add(MusECore::PendingOperationItem(&MusEGlobal::extSyncFlag, v, MusECore::PendingOperationItem::SetExternalSyncFlag));
  MusEGlobal::audio->msgExecutePendingOperations(operations, true);
}

//---------------------------------------------------------
//   useJackTransportClicked
//---------------------------------------------------------

void Transport::useJackTransportClicked(bool v)
{
  MusECore::PendingOperationList operations;
  operations.add(MusECore::PendingOperationItem(&MusEGlobal::useJackTransport, v, MusECore::PendingOperationItem::SetUseJackTransport));
  MusEGlobal::audio->msgExecutePendingOperations(operations, true);
}

//---------------------------------------------------------
//   transportMasterClicked
//---------------------------------------------------------

void Transport::transportMasterClicked(bool /*v*/)
{
//   MusECore::PendingOperationList operations;
//   operations.add(MusECore::PendingOperationItem(&MusEGlobal::useJackTransport, v, MusECore::PendingOperationItem::SetUseJackTransport));
//   MusEGlobal::audio->msgExecutePendingOperations(operations, true);
}

void Transport::keyPressEvent(QKeyEvent* ev)
{
  switch (ev->key())
  {
    case Qt::Key_Escape:
      ev->accept();
      // Yield the focus to the transport window.
      setFocus();
      return;
    break;

    default:
    break;
  }

  // Let some other higher up window handle it if needed.
  ev->ignore();
  QWidget::keyPressEvent(ev);
}


} // namespace MusEGui
