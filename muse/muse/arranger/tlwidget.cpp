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
#include "track.h"
#include "tlwidget.h"
#include "tlwlayout.h"
#include "tllineedit.h"
#include "icons.h"
#include "arranger.h"
#include "widgets/simplebutton.h"
#include "muse.h"
#include "trelement.h"
#include "gconfig.h"
#include "widgets/utils.h"
#include "trackdrag.h"
#include "synth.h"
#include "widgets/outportcombo.h"
#include "audio.h"

//---------------------------------------------------------
//   TLWidget
//---------------------------------------------------------

TLWidget::TLWidget(Track* t, TrGroupList* tgl)
      {
//      setAttribute(Qt::WA_OpaquePaintEvent);
//      setAttribute(Qt::WA_NoSystemBackground);
//      setAttribute(Qt::WA_StaticContents);
      setMouseTracking(true);

      state   = S_NORMAL;
      off     = 0;
      tel     = tgl;
      _track  = t;
      outPort = 0;

      bgColor       = _track->ccolor();
      selectBgColor = bgColor.light();

      QPalette p(palette());
      p.setColor(QPalette::Window, bgColor);
      p.setColor(QPalette::Base, bgColor);
      setPalette(p);

      l = new TLWidgetLayout(this);
      configChanged();

      connect(_track, SIGNAL(selectionChanged(bool)), SLOT(selectionChanged()));
      setAcceptDrops(true);
      connect(muse, SIGNAL(configChanged()), SLOT(configChanged()));
      }

//---------------------------------------------------------
//   selectionChanged
//---------------------------------------------------------

void TLWidget::selectionChanged()
      {
      update();
      }

//---------------------------------------------------------
//   nameChanged
//---------------------------------------------------------

void TLWidget::nameChanged(QString s)
      {
      song->changeTrackName(_track, s);
      nameEdit->setCursorPosition(0);
      }

//---------------------------------------------------------
//   selectTL
//---------------------------------------------------------

void TLWidget::select()
      {
      setFocus();
      song->selectTrack(_track);
      }

//---------------------------------------------------------
//   labelPlusClicked
//---------------------------------------------------------

void TLWidget::labelPlusClicked()
      {
      emit plusClicked(this);
      }

//---------------------------------------------------------
//   mousePressEvent
//---------------------------------------------------------

void TLWidget::mousePressEvent(QMouseEvent* ev)
      {
      int button = ev->button();
      if (button == Qt::RightButton) {
            QMenu* menu = new QMenu(this);
            QAction* a;
            a = menu->addAction(QIcon(*deleteIcon), tr("Delete Track"));
            a->setData(0);
            if (_track->type() == Track::MIDI || _track->type() == Track::WAVE) {
                  a = menu->addAction(tr("Copy Track"));
                  a->setData(1);
                  }
            if (_track->type() == Track::AUDIO_SOFTSYNTH) {
                  SynthI* s = (SynthI*) _track;
                  if (s->hasGui()) {
                        menu->addSeparator();
                        a = menu->addAction(tr("Show Gui"));
                        a->setData(2);
                        a->setCheckable(true);
                        a->setChecked(s->guiVisible());
                        }
                  }
            else if (_track->type() == Track::MIDI_SYNTI) {
                  MidiSynti* s = (MidiSynti*) _track;
                  if (s->hasGui()) {
                        menu->addSeparator();
                        a = menu->addAction(tr("Show Gui"));
                        a->setData(3);
                        a->setCheckable(true);
                        a->setChecked(s->guiVisible());
                        }
                  }

            a = menu->exec(ev->globalPos());
            if (!a)
                  return;
            int rv = a->data().toInt();
            switch (rv) {
                  default:
                        break;
                  case 0:
                        song->removeTrack(_track);
                        break;
                  case 1:
                        {
				int idx = song->tracks()->index(_track);
                        if (_track->type() == Track::MIDI) {
	                        MidiTrack* t = new MidiTrack;
      	                  t->clone((MidiTrack*)_track);
                  	      song->insertTrack(t, idx);
                              }
                        else {
	                        WaveTrack* t = new WaveTrack;
      	                  t->clone((WaveTrack*)_track);
                  	      song->insertTrack(t, idx);
                              }
                        }
                        break;
                  case 2:
                        {
                        SynthI* s = (SynthI*) _track;
                        s->showGui(!s->guiVisible());
                        }
                        break;
                  case 3:
                        {
                        MidiSynti* s = (MidiSynti*) _track;
                        s->showGui(!s->guiVisible());
                        }
                        break;
                  }
            return;
            }
      int y = ev->pos().y();
      int wh = height();
      starty = ev->globalPos().y();
      if (y > (wh - splitWidth)) {
            state  = S_DRAGBOTTOM;
            emit startDrag(trackIdx);
            }
      else {
            state = S_DRAG;
            startDragPos  = ev->pos();
            startDragTime = QTime::currentTime();
            select();
            }
      }

//---------------------------------------------------------
//   mouseReleaseEvent
//---------------------------------------------------------

void TLWidget::mouseReleaseEvent(QMouseEvent*)
      {
      state  = S_NORMAL;
      }

//---------------------------------------------------------
//   mouseMoveEvent
//---------------------------------------------------------

void TLWidget::mouseMoveEvent(QMouseEvent* ev)
      {
      QPoint pos(ev->pos());

      if (state == S_DRAGBOTTOM)
            emit drag(trackIdx, ev->globalPos().y() - starty);
      else if (state == S_DRAG) {
            int t = startDragTime.msecsTo(QTime::currentTime());
            bool dragActive = (startDragPos - pos).manhattanLength() >
                     QApplication::startDragDistance()
                     || t > QApplication::startDragTime();
            if (dragActive) {
                  QDrag* d = new TrackDrag(_track, this);
                  d->start(Qt::MoveAction);
                  state = S_NORMAL;
                  }
            }
      else {
            int y = pos.y();
            int wh = height();
            if (y > (wh - splitWidth))
                  setCursor(Qt::SizeVerCursor);
            else
                  setCursor(Qt::ArrowCursor);
            }
      }

//---------------------------------------------------------
//   configChanged
//---------------------------------------------------------

void TLWidget::configChanged()
      {
      l->clear();
      wlist.clear();
      nameEdit   = 0;
      record     = 0;
      off        = 0;
      outChannel = 0;
      outPort    = 0;

      label = new QLabel;
      l->addWidget(label);
      label->setIndent(3);
      //label->setAlignment(Qt::AlignCener);

      label->setFont(*config.fonts[1]);
      label->setText(_track->cname());
      label->setFixedHeight(11);

      plus = newPlusButton();
      l->addWidget(plus);
      plus->setToolTip(tr("Add Subtrack"));
      connect(plus, SIGNAL(clicked()), SLOT(labelPlusClicked()));

      for (iTrGroup i = tel->begin(); i != tel->end(); ++i) {
            TrElementList& el = *i;
            iTrElement k;
            for (k = el.begin(); k != el.end(); ++k) {
                  int id = (*k)->id;
                  switch(id) {
                        case TR_NAME:
                              nameEdit = new TLLineEdit(_track->name(), this);
//                        	nameEdit->setBackgroundRole(QPalette::Window);
                              nameEdit->setFixedHeight(trackRowHeight);
                              connect(nameEdit, SIGNAL(contentChanged(QString)), SLOT(nameChanged(QString)));
                              connect(nameEdit, SIGNAL(mousePress()), SLOT(select()));
                              connect(_track, SIGNAL(nameChanged(const QString&)), nameEdit, SLOT(setText(const QString&)));
                              nameEdit->setToolTip(_track->clname() + " Name");
                              l->addWidget(nameEdit);
                              wlist.push_back(nameEdit);
                              break;

                        case TR_INSTRUMENT:
                              {
                              instrument = new QComboBox(this);
                              instrument->setFixedHeight(trackRowHeight);
                              MidiOutPort* op = (MidiOutPort*)_track;
                              MidiInstrument* mi = op->instrument();
                              int idx = 0;
                              int curIdx = 0;
                              for (iMidiInstrument i = midiInstruments.begin(); i != midiInstruments.end(); ++i, ++idx) {
                                    instrument->addItem((*i)->iname());
                                    if ((*i)->iname() == mi->iname())
                                          curIdx = idx;
                                    }
                              instrument->setCurrentIndex(curIdx);
                              connect(instrument, SIGNAL(activated(int)), SLOT(instrumentSelected(int)));
                              connect(op, SIGNAL(instrumentChanged()), SLOT(instrumentChanged()));
                              l->addWidget(instrument);
                              wlist.push_back(instrument);
                              }
                              break;

                        case TR_PATCH:
                              {
                              // Ctrl* ctrl = _track->getController(CTRL_PROGRAM);
                              }
                              break;

                        case TR_OFF:
                              {
                              off = newOffButton();
                              off->setFixedSize(trackRowHeight, trackRowHeight);
                              off->setChecked(_track->off());
//      				off->setAutoFillBackground(true);
                              connect(off, SIGNAL(clicked(bool)), SLOT(offToggled(bool)));
                              connect(_track, SIGNAL(offChanged(bool)), this, SLOT(updateOffState()));
                              l->addWidget(off);
                              wlist.push_back(off);
                              }
                              break;

                        case TR_DRUMMAP:
                              {
                              SimpleButton* dm = newDrumMapButton();
                              MidiChannel* mt = (MidiChannel*)_track;
                              dm->setFixedSize(trackRowHeight, trackRowHeight);
                              dm->setChecked(mt->useDrumMap());
                              connect(dm, SIGNAL(clicked(bool)), SLOT(drumMapToggled(bool)));
                              connect(mt, SIGNAL(useDrumMapChanged(bool)), dm, SLOT(setChecked(bool)));
                              l->addWidget(dm);
                              wlist.push_back(dm);
                              }
                              break;

                        case TR_MUTE:
                              {
                              SimpleButton* mute = newMuteButton();
                              mute->setFixedSize(trackRowHeight, trackRowHeight);
                              mute->setChecked(_track->isMute());
                              connect(mute, SIGNAL(clicked(bool)), SLOT(muteToggled(bool)));
                              connect(_track, SIGNAL(muteChanged(bool)), mute, SLOT(setChecked(bool)));
                              l->addWidget(mute);
                              wlist.push_back(mute);
                              }
                              break;

                        case TR_SOLO:
                              {
                              SimpleButton* solo = newSoloButton();
                              solo->setFixedSize(trackRowHeight, trackRowHeight);
                              solo->setChecked(_track->solo());
                              connect(solo, SIGNAL(clicked(bool)), SLOT(soloToggled(bool)));
                              connect(_track, SIGNAL(soloChanged(bool)), solo, SLOT(setChecked(bool)));
                              l->addWidget(solo);
                              wlist.push_back(solo);
                              }
                              break;

                        case TR_MONITOR:
                              {
                              SimpleButton* monitor = newMonitorButton();
                              monitor->setFixedSize(trackRowHeight+4, trackRowHeight);
                              monitor->setChecked(_track->monitor());
                              connect(monitor, SIGNAL(clicked(bool)), SLOT(monitorToggled(bool)));
                              connect(_track, SIGNAL(monitorChanged(bool)), monitor, SLOT(setChecked(bool)));
                              l->addWidget(monitor);
                              wlist.push_back(monitor);
                              }
                              break;

                        case TR_RECORD:
                              {
                              record  = newRecordButton();
                              record->setFixedSize(trackRowHeight, trackRowHeight);
                              record->setChecked(_track->recordFlag());
//      				record->setAutoFillBackground(true);
                              connect(record, SIGNAL(clicked(bool)), SLOT(recordToggled(bool)));
                              connect(_track, SIGNAL(recordChanged(bool)), record, SLOT(setChecked(bool)));
                              l->addWidget(record);
                              wlist.push_back(record);
                              }
                              break;

                        case TR_AREAD:
                              {
                              SimpleButton* b = newAutoReadButton();
                              b->setFixedSize(trackRowHeight, trackRowHeight);
                              b->setChecked(_track->autoRead());
                              connect(b, SIGNAL(clicked(bool)), SLOT(autoReadToggled(bool)));
                              connect(_track, SIGNAL(autoReadChanged(bool)), b, SLOT(setChecked(bool)));
                              l->addWidget(b);
                              wlist.push_back(b);
                              }
                              break;

                        case TR_AWRITE:
                              {
                              SimpleButton* b = newAutoWriteButton();
                              b->setFixedSize(trackRowHeight, trackRowHeight);
                              b->setChecked(_track->autoWrite());
                              connect(b, SIGNAL(clicked(bool)), SLOT(autoWriteToggled(bool)));
                              connect(_track, SIGNAL(autoWriteChanged(bool)), b, SLOT(setChecked(bool)));
                              l->addWidget(b);
                              wlist.push_back(b);
                              }
                              break;

                        case TR_OCHANNEL:
                              {
                              outChannel = new QSpinBox(this);
                              outChannel->setFixedSize(45, trackRowHeight);
                              outChannel->setRange(1, 16);
                              MidiChannel* midiChannel = ((MidiTrack*)_track)->channel();
                              if (midiChannel)
                                 outChannel->setValue(midiChannel->channelNo()+1);
                              outChannel->setToolTip(tr("Midi Output Channel"));
                              l->addWidget(outChannel);
                              wlist.push_back(outChannel);
                              connect(outChannel, SIGNAL(valueChanged(int)), SLOT(outChannelChanged(int)));
//TD                              connect((MidiTrack*)_track, SIGNAL(outChannelChanged(int)), SLOT(setOutChannel(int)));
                              }
                              break;

                        default:
                              printf("TLWidget:: unknown element %d\n", id);
                              break;
                        }
                  }
            }
      updateOffState();
      selectionChanged();     // update selection state
      l->update();
      }

//---------------------------------------------------------
//   recordToggled
//---------------------------------------------------------

void TLWidget::recordToggled(bool val)
      {
      song->setRecordFlag(_track, !val);
      }

//---------------------------------------------------------
//   muteToggled
//---------------------------------------------------------

void TLWidget::muteToggled(bool val)
      {
      song->setMute(_track, val);
      }

//---------------------------------------------------------
//   monitorToggled
//---------------------------------------------------------

void TLWidget::monitorToggled(bool val)
      {
      _track->setMonitor(val);
      }

//---------------------------------------------------------
//   drumMapToggled
//---------------------------------------------------------

void TLWidget::drumMapToggled(bool val)
      {
      ((MidiChannel*)_track)->setUseDrumMap(val);
      }

//---------------------------------------------------------
//   offToggled
//---------------------------------------------------------

void TLWidget::offToggled(bool val)
      {
      song->setOff(_track, !val);
      }

//---------------------------------------------------------
//   soloToggled
//---------------------------------------------------------

void TLWidget::soloToggled(bool val)
      {
      song->setSolo(_track, val);
      }

//---------------------------------------------------------
//   autoReadToggled
//---------------------------------------------------------

void TLWidget::autoReadToggled(bool val)
      {
      song->setAutoRead(_track, val);
      }

//---------------------------------------------------------
//   autoWriteToggled
//---------------------------------------------------------

void TLWidget::autoWriteToggled(bool val)
      {
      song->setAutoWrite(_track, val);
      }

//---------------------------------------------------------
//   setOutPort
//---------------------------------------------------------

void TLWidget::setOutPort(int n)
      {
      outPort->setCurrentIndex(n);
      }

//---------------------------------------------------------
//   outChannelChanged
//---------------------------------------------------------

void TLWidget::outChannelChanged(int n)
      {
      n -= 1;
      MidiChannel* mc = ((MidiTrack*)_track)->channel();
      if (mc == 0)		// no route to port?
            return;
      MidiOutPort* mp = mc->port();
      int id = mc->channelNo();
      if (id == n)
            return;
      audio->msgRemoveRoute(Route(_track, -1, Route::TRACK), Route(mc,-1, Route::TRACK));
      audio->msgAddRoute(Route(_track, -1, Route::TRACK), Route(mp->channel(n),-1, Route::TRACK));
      song->update(SC_ROUTE);
      }

//---------------------------------------------------------
//   setOutChannel
//---------------------------------------------------------

void TLWidget::setOutChannel(int)
      {
//TODO3      outChannel->setValue(n + 1);
      }

//---------------------------------------------------------
//   dragEnterEvent
//---------------------------------------------------------

void TLWidget::dragEnterEvent(QDragEnterEvent* event)
      {
      if (TrackDrag::canDecode(event))
            event->acceptProposedAction();
      }

//---------------------------------------------------------
//   dropEvent
//---------------------------------------------------------

void TLWidget::dropEvent(QDropEvent* event)
      {
      Track* t;
      TrackDrag::decode(event, t);
      if (_track != t)
            emit moveTrack(t, _track);
      event->acceptProposedAction();
      }

//---------------------------------------------------------
//   updateOffState
//---------------------------------------------------------

void TLWidget::updateOffState()
      {
      bool val = !_track->off();
      std::vector<QWidget*>::iterator i = wlist.begin();
      for (; i != wlist.end(); ++i) {
            if ((*i) != off)
                  (*i)->setEnabled(val);
            }
      if (off)
            off->setChecked(!val);
      }

//---------------------------------------------------------
//   instrumentSelected
//---------------------------------------------------------

void TLWidget::instrumentSelected(int n)
      {
      MidiOutPort* op = (MidiOutPort*)_track;
      int idx = 0;
      for (iMidiInstrument i = midiInstruments.begin(); i != midiInstruments.end(); ++i, ++idx) {
            if (idx == n) {
                  op->setInstrument(*i);
                  break;
                  }
            }
      }

//---------------------------------------------------------
//   instrumentChanged
//---------------------------------------------------------

void TLWidget::instrumentChanged()
      {
      MidiOutPort* op = (MidiOutPort*)_track;
      MidiInstrument* mi = op->instrument();
      int idx = 0;
      for (iMidiInstrument i = midiInstruments.begin(); i != midiInstruments.end(); ++i, ++idx) {
            if (*i == mi) {
                  instrument->setCurrentIndex(idx);
                  break;
                  }
            }
      }

//---------------------------------------------------------
//   paintEvent
//---------------------------------------------------------

void TLWidget::paintEvent(QPaintEvent* ev)
      {
      QPainter p(this);
      QRect r(ev->rect());
      QColor color(_track->selected() ? selectBgColor : bgColor);
      p.fillRect(r, color);

// does not work if scrolled:
//      if (r==rect()) // only draw on full redraw
            paintHLine(p, r.x(), r.x() + r.width(), height() - splitWidth);
      QPalette pl = nameEdit->palette();
      pl.setColor(QPalette::Window, color);
      pl.setColor(QPalette::Base, color);
      nameEdit->setPalette(pl);
      
      // The selected track will get a 4 pixel red bar to the left
//      if ( _track->selected() && r==rect() /* only draw on full redraw */) {
      if ( _track->selected()) {
            QColor color(200,10,10);
            QRect qr(0, 0, 4, r.height()-splitWidth); 
            p.fillRect(qr, color);
            }
      }
