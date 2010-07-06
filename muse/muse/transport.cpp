//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: transport.cpp,v 1.8.2.3 2009/07/01 10:39:42 spamatica Exp $
//
//  (C) Copyright 1999/2000 Werner Schweer (ws@seh.de)
//=========================================================

#include <qvbox.h>
#include <qlabel.h>
#include <qslider.h>
#include <qpushbutton.h>
#include <qframe.h>
#include <qwhatsthis.h>
#include <qtooltip.h>
#include <qlayout.h>
#include <qtoolbutton.h>
#include <qcombobox.h>
#include <qaction.h>

#include "song.h"
#include "transport.h"
#include "doublelabel.h"
#include "siglabel.h"
#include "globals.h"
#include "icons.h"
#include "posedit.h"
#include "sync.h"
#include "shortcuts.h"
#include "gconfig.h"
#include "app.h"

static const char* recordTransportText   = QT_TR_NOOP("Click this button to enable recording");
static const char* stopTransportText     = QT_TR_NOOP("Click this button to stop playback");
static const char* playTransportText     = QT_TR_NOOP("Click this button to start playback");
static const char* startTransportText    = QT_TR_NOOP("Click this button to rewind to start position");
static const char* frewindTransportText  = QT_TR_NOOP("Click this button to rewind");
static const char* fforwardTransportText = QT_TR_NOOP("Click this button to forward current play position");

//---------------------------------------------------------
//   toolButton
//---------------------------------------------------------

static QToolButton* newButton(QWidget* parent, const QString& s,
   const QString& tt, bool toggle=false, int height=25)
      {
      QToolButton* button = new QToolButton(parent);
      button->setFixedHeight(height);
      button->setText(s);
      button->setToggleButton(toggle);
      QToolTip::add(button, tt);
      return button;
      }

static QToolButton* newButton(QWidget* parent, const QPixmap* pm,
   const QString& tt, bool toggle=false)
      {
      QToolButton* button = new QToolButton(parent);
      button->setFixedHeight(25);
      button->setPixmap(*pm);
      button->setToggleButton(toggle);
      QToolTip::add(button, tt);
      return button;
      }

//---------------------------------------------------------
//    Handle
//    erlaubt das Verschieben eines Root-Windows mit der
//    Maus
//---------------------------------------------------------

Handle::Handle(QWidget* root, QWidget* r)
   : QWidget(root)
      {
      rootWin = r;
      setFixedWidth(20);
      setCursor(pointingHandCursor);
      setBackgroundColor(config.transportHandleColor);
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
//    Widget fï¿½r Tempo + Signature
//---------------------------------------------------------

TempoSig::TempoSig(QWidget* parent)
   : QWidget(parent, "TempoSig")
      {
      QBoxLayout* vb1 = new QVBoxLayout(this);
      vb1->setAutoAdd(true);

      QFrame* f = new QFrame(this);
      f->setFrameStyle(QFrame::Panel | QFrame::Sunken);
      f->setLineWidth(1);

      QBoxLayout* vb2 = new QVBoxLayout(f);
      vb2->setAutoAdd(true);

      l1 = new DoubleLabel(120.0, 20.0, 400.0, f);
      
      l1->setSpecialText(QString("extern"));
      l2 = new SigLabel(4, 4, f);

      l3 = new QLabel(tr("Tempo/Sig"), this);
      l3->setFont(config.fonts[2]);

      l1->setBackgroundMode(PaletteLight);
      l1->setAlignment(AlignCenter);
      l1->setSizePolicy(QSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed));
      l2->setBackgroundMode(PaletteLight);
      l2->setAlignment(AlignCenter);
      l2->setSizePolicy(QSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed));
      l3->setAlignment(AlignCenter);
      l3->setSizePolicy(QSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed));

      connect(l1, SIGNAL(valueChanged(double,int)), SLOT(setTempo(double)));
      connect(l2, SIGNAL(valueChanged(int,int)), SIGNAL(sigChanged(int,int)));
      connect(muse, SIGNAL(configChanged()), SLOT(configChanged()));
      }

//---------------------------------------------------------
//   configChanged
//---------------------------------------------------------

void TempoSig::configChanged()
      {
      l3->setFont(config.fonts[2]);
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
      buttons[5]->setOn(flag);
      buttons[5]->blockSignals(false);
      }

//---------------------------------------------------------
//   Transport
//---------------------------------------------------------

Transport::Transport(QWidget*, const char* name)
  // : QWidget(0, name, WStyle_Customize | WType_TopLevel | WStyle_Tool
  //| WStyle_NoBorder | WStyle_StaysOnTop)
   : QWidget(0, name, WStyle_Customize | WType_TopLevel | WStyle_NoBorder | WStyle_StaysOnTop)
      {
      setCaption(QString("Muse: Transport"));
      setSizePolicy(QSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum));

      QHBoxLayout* hbox = new QHBoxLayout(this, 2, 5);

      hbox->setAutoAdd(true);
      lefthandle = new Handle(this, this);
  
      //-----------------------------------------------------
      //    Record & Cycle Mode
      //-----------------------------------------------------

      QVBox* box1 = new QVBox(this);
      recMode     = new QComboBox(box1);
      recMode->setFocusPolicy(QWidget::NoFocus);
      recMode->insertItem(tr("Overdub"), Song::REC_OVERDUP);
      recMode->insertItem(tr("Replace"), Song::REC_REPLACE);
      recMode->setCurrentItem(song->recMode());
      l2 = new QLabel(tr("Rec Mode"), box1);
      l2->setFont(config.fonts[2]);
      l2->setAlignment(AlignCenter);
      connect(recMode, SIGNAL(activated(int)), SLOT(setRecMode(int)));

      cycleMode = new QComboBox(box1);
      cycleMode->setFocusPolicy(QWidget::NoFocus);
      cycleMode->insertItem(tr("Normal"),  Song::CYCLE_NORMAL);
      cycleMode->insertItem(tr("Mix"),     Song::CYCLE_MIX);
      cycleMode->insertItem(tr("Replace"), Song::CYCLE_REPLACE);
      cycleMode->setCurrentItem(song->cycleMode());
      l3 = new QLabel(tr("Cycle Rec"), box1);
      l3->setFont(config.fonts[2]);
      l3->setAlignment(AlignCenter);
      connect(cycleMode, SIGNAL(activated(int)), SLOT(setCycleMode(int)));

      //-----------------------------------------------------
      //  loop flags
      //-----------------------------------------------------

      QVBox* button2 = new QVBox(this);
      button2->setMargin(3);

      QToolButton* b1 = newButton(button2, punchinIcon, tr("punchin"), true);
      QToolButton* b2 = newButton(button2, loopIcon, tr("loop"), true);
      b2->setAccel(shortcuts[SHRT_TOGGLE_LOOP].key);

      QToolButton* b3 = newButton(button2, punchoutIcon, tr("punchout"), true);
      QToolTip::add(b1, tr("Punch In"));
      QToolTip::add(b2, tr("Loop"));
      QToolTip::add(b3, tr("Punch Out"));
      QWhatsThis::add(b1, tr("Punch In"));

      connect(b1, SIGNAL(toggled(bool)), song, SLOT(setPunchin(bool)));
      connect(b2, SIGNAL(toggled(bool)), song, SLOT(setLoop(bool)));
      connect(b3, SIGNAL(toggled(bool)), song, SLOT(setPunchout(bool)));

      b1->setOn(song->punchin());
      b2->setOn(song->loop());
      b3->setOn(song->punchout());

      connect(song, SIGNAL(punchinChanged(bool)),  b1, SLOT(setOn(bool)));
      connect(song, SIGNAL(punchoutChanged(bool)), b3, SLOT(setOn(bool)));
      connect(song, SIGNAL(loopChanged(bool)),     b2, SLOT(setOn(bool)));

      //-----------------------------------------------------
      //  left right mark
      //-----------------------------------------------------

      QVBox* marken = new QVBox(this);
      tl1 = new PosEdit(marken);
      l5 = new QLabel(tr("Left Mark"), marken);
      l5->setFont(config.fonts[2]);
      l5->setAlignment(AlignCenter);
      tl2 = new PosEdit(marken);
      l6 = new QLabel(tr("Right Mark"), marken);
      l6->setFont(config.fonts[2]);
      l6->setAlignment(AlignCenter);

      //-----------------------------------------------------
      //  Transport Buttons
      //-----------------------------------------------------

      QVBox* box4 = new QVBox(this);
      box4->setSizePolicy(QSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum));
      box4->setSpacing(3);
      QHBox* hbox1 = new QHBox(box4);
      hbox1->setSizePolicy(QSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum));
      time1 = new PosEdit(hbox1);
      time2 = new PosEdit(hbox1);
      time2->setSmpte(true);
      time1->setSizePolicy(QSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed));
      time2->setSizePolicy(QSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed));

      slider = new QSlider(0, 200000, 1000, 0, Horizontal, box4);

      tb = new QHBox(box4);
      tb->setSizePolicy(QSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum));

      buttons[0] = newButton(tb, startIcon, tr("rewind to start"));
      QWhatsThis::add(buttons[0], tr(startTransportText));

      buttons[1] = newButton(tb, frewindIcon, tr("rewind"));
      buttons[1]->setAutoRepeat(true);
      QWhatsThis::add(buttons[1], tr(frewindTransportText));

      buttons[2] = newButton(tb, fforwardIcon, tr("forward"));
      buttons[2]->setAutoRepeat(true);
      QWhatsThis::add(buttons[2], tr(fforwardTransportText));

      buttons[3] = newButton(tb, stopIcon, tr("stop"), true);
      buttons[3]->setOn(true);     // set STOP
      QWhatsThis::add(buttons[3], tr(stopTransportText));

      buttons[4] = newButton(tb, playIcon, tr("play"), true);
      QWhatsThis::add(buttons[4], tr(playTransportText));

      buttons[5] = newButton(tb, record_on_Icon, tr("record"), true);
      QWhatsThis::add(buttons[5], tr(recordTransportText));

      for (int i = 0; i < 6; ++i)
            buttons[i]->setSizePolicy(QSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed));


      connect(buttons[3], SIGNAL(toggled(bool)), SLOT(stopToggled(bool)));
      connect(buttons[4], SIGNAL(toggled(bool)), SLOT(playToggled(bool)));

      connect(buttons[5], SIGNAL(toggled(bool)), song, SLOT(setRecord(bool)));
      connect(song, SIGNAL(recordChanged(bool)), SLOT(setRecord(bool)));
      connect(buttons[0], SIGNAL(clicked()), song, SLOT(rewindStart()));
      connect(buttons[1], SIGNAL(clicked()), song, SLOT(rewind()));
      connect(buttons[2], SIGNAL(clicked()), song, SLOT(forward()));

      //-----------------------------------------------------
      //  AQ - Click - Sync
      //-----------------------------------------------------

      QVBox* button1 = new QVBox(this);
      button1->setMargin(1);

      quantizeButton = newButton(button1, tr("AC"), tr("quantize during record"), true,19);
      clickButton    = newButton(button1, tr("Click"), tr("metronom click on/off"), true,19);
      clickButton->setAccel(shortcuts[SHRT_TOGGLE_METRO].key);

      syncButton     = newButton(button1, tr("Sync"), tr("external sync on/off"), true,19);
      jackTransportButton     = newButton(button1, tr("Jack"), tr("Jack transport sync on/off"), true,19);

      quantizeButton->setOn(song->quantize());
      clickButton->setOn(song->click());
      syncButton->setOn(extSyncFlag.value());
      jackTransportButton->setOn(useJackTransport.value());

      connect(quantizeButton, SIGNAL(toggled(bool)), song, SLOT(setQuantize(bool)));
      connect(clickButton, SIGNAL(toggled(bool)), song, SLOT(setClick(bool)));

      connect(syncButton, SIGNAL(toggled(bool)), &extSyncFlag, SLOT(setValue(bool)));
      connect(jackTransportButton, SIGNAL(toggled(bool)),&useJackTransport, SLOT(setValue(bool)));
      connect(&extSyncFlag, SIGNAL(valueChanged(bool)), SLOT(syncChanged(bool)));
      connect(&useJackTransport, SIGNAL(valueChanged(bool)), SLOT(jackSyncChanged(bool)));

      connect(song, SIGNAL(quantizeChanged(bool)), this, SLOT(setQuantizeFlag(bool)));
      connect(song, SIGNAL(clickChanged(bool)), this, SLOT(setClickFlag(bool)));

      //-----------------------------------------------------
      //  Tempo/Sig
      //-----------------------------------------------------

      QVBox* box5  = new QVBox(this);
      tempo        = new TempoSig(box5);
      tempo->setSizePolicy(QSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed));
      masterButton = newButton(box5, tr("Master"), tr("use master track"), true);
      masterButton->setSizePolicy(QSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed));

      connect(masterButton, SIGNAL(toggled(bool)), song, SLOT(setMasterFlag(bool)));

      //-----------------------------------------------------

      connect(tl1,   SIGNAL(valueChanged(const Pos&)), SLOT(lposChanged(const Pos&)));
      connect(tl2,   SIGNAL(valueChanged(const Pos&)), SLOT(rposChanged(const Pos&)));
      connect(time1, SIGNAL(valueChanged(const Pos&)), SLOT(cposChanged(const Pos&)));
      connect(time2, SIGNAL(valueChanged(const Pos&)), SLOT(cposChanged(const Pos&)));

      connect(slider,SIGNAL(valueChanged(int)),  SLOT(cposChanged(int)));
      connect(song, SIGNAL(posChanged(int, unsigned, bool)), SLOT(setPos(int, unsigned, bool)));
      connect(tempo, SIGNAL(tempoChanged(int)), song, SLOT(setTempo(int)));
      connect(tempo, SIGNAL(sigChanged(int, int)), song, SLOT(setSig(int, int)));
      connect(song, SIGNAL(playChanged(bool)), SLOT(setPlay(bool)));
      connect(song, SIGNAL(songChanged(int)), this, SLOT(songChanged(int)));
      connect(muse, SIGNAL(configChanged()), SLOT(configChanged()));
      righthandle = new Handle(this, this);
      }

//---------------------------------------------------------
//   configChanged
//---------------------------------------------------------

void Transport::configChanged()
      {
      l2->setFont(config.fonts[2]);
      l3->setFont(config.fonts[2]);
      l5->setFont(config.fonts[2]);
      l6->setFont(config.fonts[2]);
      lefthandle->setBackgroundColor(config.transportHandleColor);
      righthandle->setBackgroundColor(config.transportHandleColor);
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
      }

//---------------------------------------------------------
//   setHandleColor
//---------------------------------------------------------

void Transport::setHandleColor(QColor c)
      {
     	lefthandle->setBackgroundColor(c);
     	righthandle->setBackgroundColor(c);
      }

//---------------------------------------------------------
//   setTimesig
//---------------------------------------------------------

void Transport::setTimesig(int z, int n)
      {
      tempo->setTimesig(z, n);
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
                  slider->blockSignals(true);
                  slider->setValue(v);
                  slider->blockSignals(false);
                  if (song->masterFlag())
                        setTempo(tempomap.tempo(v));
                  {
                  int z, n;
                  sigmap.timesig(v, z, n);
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
      buttons[3]->blockSignals(true);
      buttons[4]->blockSignals(true);
      buttons[3]->setOn(!f);
      buttons[4]->setOn(f);
      buttons[3]->blockSignals(false);
      buttons[4]->blockSignals(false);
      }

//---------------------------------------------------------
//   setMasterFlag
//---------------------------------------------------------

void Transport::setMasterFlag(bool f)
      {
      masterButton->setOn(f);
      }

//---------------------------------------------------------
//   setClickFlag
//---------------------------------------------------------

void Transport::setClickFlag(bool f)
      {
      clickButton->blockSignals(true);
      clickButton->setOn(f);
      clickButton->blockSignals(false);
      }

//---------------------------------------------------------
//   setQuantizeFlag
//---------------------------------------------------------

void Transport::setQuantizeFlag(bool f)
      {
      quantizeButton->setOn(f);
      }

//---------------------------------------------------------
//   setSyncFlag
//---------------------------------------------------------

void Transport::setSyncFlag(bool f)
      {
      syncButton->setOn(f);
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
      // Is it simply a midi controller value adjustment? Forget it.
      if(flags == SC_MIDI_CONTROLLER)
        return;
    
      slider->setRange(0, song->len());
      int cpos  = song->cpos();
      int t = tempomap.tempo(cpos);
      if (flags & (SC_MASTER | SC_TEMPO)) {
            if (extSyncFlag.value())
                  setTempo(0);
            else
                  setTempo(t);
            }
      if (flags & SC_SIG) {
            int z, n;
            sigmap.timesig(cpos, z, n);
            setTimesig(z, n);
            }
      if (flags & SC_MASTER)
            masterButton->setOn(song->masterFlag());
      }

//---------------------------------------------------------
//   syncChanged
//---------------------------------------------------------

void Transport::syncChanged(bool flag)
      {
      syncButton->setOn(flag);
      buttons[0]->setEnabled(!flag);      // goto start
      buttons[1]->setEnabled(!flag);      // rewind
      buttons[2]->setEnabled(!flag);      // forward
      buttons[3]->setEnabled(!flag);      // stop
      buttons[4]->setEnabled(!flag);      // play
      slider->setEnabled(!flag);
      masterButton->setEnabled(!flag);
      if (flag) {
            masterButton->setOn(false);
            song->setMasterFlag(false);
            tempo->setTempo(0);         // slave mode: show "extern"
            }
      else
            tempo->setTempo(tempomap.tempo(song->cpos()));
      playAction->setEnabled(!flag);
      startAction->setEnabled(!flag);
      stopAction->setEnabled(!flag);
      rewindAction->setEnabled(!flag);
      forwardAction->setEnabled(!flag);
      }

void Transport::jackSyncChanged(bool flag)
      {
      jackTransportButton->setOn(flag);
      }
//---------------------------------------------------------
//   stopToggled
//---------------------------------------------------------

void Transport::stopToggled(bool val)
      {
      if (val)
            song->setStop(true);
      else {
            buttons[3]->blockSignals(true);
            buttons[3]->setOn(true);
            buttons[3]->blockSignals(false);
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
            buttons[4]->blockSignals(true);
            buttons[4]->setOn(true);
            buttons[4]->blockSignals(false);
            }
      }

