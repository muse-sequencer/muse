//=========================================================
//  MusE
//  Linux Music Editor
//  (C) Copyright 2010 Werner Schweer and others (ws@seh.de)
//  (C) Copyright 2011-2016 Tim E. Real (terminator356 on sourceforge)
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

#include <QTimer>
#include <QMessageBox>
#include <QLinearGradient>
#include <QPalette>
#include <QColor>

#include <math.h>
#include <string.h>

#include "mtrackinfo.h"
#include "song.h"
#include "globals.h"
#include "config.h"
#include "gconfig.h"
#include "midiport.h"
#include "minstrument.h"
#include "mididev.h"
#include "utils.h"
#include "audio.h"
#include "midi.h"
#include "midictrl.h"
#include "icons.h"
#include "app.h"
#include "route.h"
#include "popupmenu.h"
#include "routepopup.h"
#include "lv2host.h"

namespace MusEGui {

//---------------------------------------------------------
//   setTrack
//---------------------------------------------------------

void MidiTrackInfo::setTrack(MusECore::Track* t)
{
  if(!t)
  {
    selected = 0;
    return;
  }  
  
  if(!t->isMidiTrack())
    return;
  selected = t;

  trackNameLabel->setObjectName(selected->cname());
  
  /*QPalette pal;
  if(selected->type() == MusECore::Track::DRUM) {
    pal.setColor(trackNameLabel->backgroundRole(), MusEGlobal::config.drumTrackLabelBg); 
    iOutputChannel->setEnabled(false);
  } else  {
    pal.setColor(trackNameLabel->backgroundRole(), MusEGlobal::config.midiTrackLabelBg);
    iOutputChannel->setEnabled(true);
  }
  trackNameLabel->setPalette(pal); */
  //setLabelText();
  
  updateTrackInfo(-1);
}

//---------------------------------------------------------
//   midiTrackInfo
//---------------------------------------------------------

MidiTrackInfo::MidiTrackInfo(QWidget* parent, MusECore::Track* sel_track) : QWidget(parent)
{ 
  setupUi(this); 
  _midiDetect = false; 
  heartBeatCounter = 0;
  _blockHeartbeatCount = 0;
  
  selected = sel_track;
  
  // Since program covers 3 controls at once, it is in 'midi controller' units rather than 'gui control' units.
  //program  = -1;
  program  = MusECore::CTRL_VAL_UNKNOWN;
  pan      = -65;
  volume   = -1;
  
  setFont(MusEGlobal::config.fonts[1]);
  
  //iChanDetectLabel->setPixmap(*darkgreendotIcon);
  iChanDetectLabel->setPixmap(*darkRedLedIcon);
  
  recEchoButton->setFocusPolicy(Qt::NoFocus);
  recEchoButton->setIcon((selected && ((MusECore::MidiTrack*)selected)->recMonitor()) ? QIcon(*midiThruOnIcon) : QIcon(*midiThruOffIcon));
  recEchoButton->setIconSize(midiThruOnIcon->size());  
  //recEchoButton->setOffPixmap(midiThruOffIcon);
  //recEchoButton->setOnPixmap(midiThruOnIcon);
  
  iRButton->setFocusPolicy(Qt::NoFocus);
  iRButton->setIcon(QIcon(*routesMidiInIcon));
  iRButton->setIconSize(routesMidiInIcon->size());  
  //iRButton->setOffPixmap(routesMidiInIcon);
  
  oRButton->setFocusPolicy(Qt::NoFocus);
  oRButton->setIcon(QIcon(*routesMidiOutIcon));
  oRButton->setIconSize(routesMidiOutIcon->size());  
  //oRButton->setOffPixmap(routesMidiOutIcon);

  updateRouteButtons();
  
  recordButton->setFocusPolicy(Qt::NoFocus);
  progRecButton->setFocusPolicy(Qt::NoFocus);
  volRecButton->setFocusPolicy(Qt::NoFocus);
  panRecButton->setFocusPolicy(Qt::NoFocus);
  instrPushButton->setFocusPolicy(Qt::NoFocus);
  iPatch->setFocusPolicy(Qt::NoFocus);

  iOutput->setFocusPolicy(Qt::NoFocus);
  iOutputChannel->setFocusPolicy(Qt::StrongFocus);
  iHBank->setFocusPolicy(Qt::StrongFocus);
  iLBank->setFocusPolicy(Qt::StrongFocus);
  iProgram->setFocusPolicy(Qt::StrongFocus);
  iHBank->setFocusPolicy(Qt::StrongFocus);
  iLBank->setFocusPolicy(Qt::StrongFocus);
  iProgram->setFocusPolicy(Qt::StrongFocus);
  iLautst->setFocusPolicy(Qt::StrongFocus);
  iLautst->setFocusPolicy(Qt::StrongFocus);
  iTransp->setFocusPolicy(Qt::StrongFocus);
  iAnschl->setFocusPolicy(Qt::StrongFocus);
  iVerz->setFocusPolicy(Qt::StrongFocus);
  iLen->setFocusPolicy(Qt::StrongFocus);
  iKompr->setFocusPolicy(Qt::StrongFocus);
  iPan->setFocusPolicy(Qt::StrongFocus);
  
  // MusE-2: AlignCenter and WordBreak are set in the ui(3) file, but not supported by QLabel. Turn them on here.
  trackNameLabel->setAlignment(Qt::AlignCenter);
  //Qt::TextWordWrap is not available for alignment in Qt4 - Orcan
  // MusE-2 Tested: TextWrapAnywhere actually works, but in fact it takes precedence 
  //  over word wrap, so I found it is not really desirable. Maybe with a user setting...
  //trackNameLabel->setAlignment(Qt::AlignCenter | Qt::TextWordWrap | Qt::TextWrapAnywhere);
  //trackNameLabel->setSizePolicy(QSizePolicy(QSizePolicy::Ignored, QSizePolicy::Minimum));
  
  if(selected)
    trackNameLabel->setObjectName(selected->cname());
  
  //trackNameLabel->setStyleSheet(QString("background-color: ") + QColor(0, 160, 255).name()); // Med blue
//   trackNameLabel->setWordWrap(true);
  trackNameLabel->setAutoFillBackground(true);
  trackNameLabel->setTextFormat(Qt::PlainText);
  trackNameLabel->setLineWidth(2);
  trackNameLabel->setFrameStyle(QFrame::Sunken | QFrame::StyledPanel);
  trackNameLabel->setSizePolicy(QSizePolicy(QSizePolicy::Ignored, QSizePolicy::Minimum));
  
  setLabelText();
  setLabelFont();

  MusECore::MidiInstrument* minstr = NULL; 
  MusECore::MidiTrack* track = static_cast<MusECore::MidiTrack*>(selected);
  if(track)
    minstr = MusEGlobal::midiPorts[track->outPort()].instrument();
  if(minstr)
  {
    instrPushButton->setText(minstr->iname());
    if(minstr->isSynti())
      instrPushButton->setEnabled(false);
    else
      instrPushButton->setEnabled(true);
  }
  else
    instrPushButton->setText(tr("<unknown>"));
  
  //setSizePolicy(QSizePolicy(QSizePolicy::Maximum, QSizePolicy::Expanding));
  
  connect(instrPushButton, SIGNAL(released()), SLOT(instrPopup()));
  connect(iPatch, SIGNAL(released()), SLOT(patchPopup()));

  connect(iOutputChannel, SIGNAL(valueChanged(int)), SLOT(iOutputChannelChanged(int)));
  connect(iHBank, SIGNAL(valueChanged(int)), SLOT(iProgHBankChanged()));
  connect(iLBank, SIGNAL(valueChanged(int)), SLOT(iProgLBankChanged()));
  connect(iProgram, SIGNAL(valueChanged(int)), SLOT(iProgramChanged()));
  connect(iHBank, SIGNAL(ctrlDoubleClicked()), SLOT(iProgHBankDoubleCLicked()));
  connect(iLBank, SIGNAL(ctrlDoubleClicked()), SLOT(iProgLBankDoubleCLicked()));
  connect(iProgram, SIGNAL(ctrlDoubleClicked()), SLOT(iProgramDoubleClicked()));
  connect(iLautst, SIGNAL(valueChanged(int)), SLOT(iLautstChanged(int)));
  connect(iLautst, SIGNAL(ctrlDoubleClicked()), SLOT(iLautstDoubleClicked()));
  connect(iTransp, SIGNAL(valueChanged(int)), SLOT(iTranspChanged(int)));
  connect(iAnschl, SIGNAL(valueChanged(int)), SLOT(iAnschlChanged(int)));
  connect(iVerz, SIGNAL(valueChanged(int)), SLOT(iVerzChanged(int)));
  connect(iLen, SIGNAL(valueChanged(int)), SLOT(iLenChanged(int)));
  connect(iKompr, SIGNAL(valueChanged(int)), SLOT(iKomprChanged(int)));
  connect(iPan, SIGNAL(valueChanged(int)), SLOT(iPanChanged(int)));
  connect(iPan, SIGNAL(ctrlDoubleClicked()), SLOT(iPanDoubleClicked()));
  connect(iOutput, SIGNAL(activated(int)), SLOT(iOutputPortChanged(int)));
  connect(recordButton, SIGNAL(clicked()), SLOT(recordClicked()));
  connect(progRecButton, SIGNAL(clicked()), SLOT(progRecClicked()));
  connect(volRecButton, SIGNAL(clicked()), SLOT(volRecClicked()));
  connect(panRecButton, SIGNAL(clicked()), SLOT(panRecClicked()));
  connect(recEchoButton, SIGNAL(toggled(bool)), SLOT(recEchoToggled(bool)));
  connect(iRButton, SIGNAL(pressed()), SLOT(inRoutesPressed()));

  connect(iOutputChannel, SIGNAL(returnPressed()), SIGNAL(returnPressed()));
  connect(iHBank, SIGNAL(returnPressed()), SIGNAL(returnPressed()));
  connect(iLBank, SIGNAL(returnPressed()), SIGNAL(returnPressed()));
  connect(iProgram, SIGNAL(returnPressed()), SIGNAL(returnPressed()));
  connect(iHBank, SIGNAL(returnPressed()), SIGNAL(returnPressed()));
  connect(iLBank, SIGNAL(returnPressed()), SIGNAL(returnPressed()));
  connect(iProgram, SIGNAL(returnPressed()), SIGNAL(returnPressed()));
  connect(iLautst, SIGNAL(returnPressed()), SIGNAL(returnPressed()));
  connect(iLautst, SIGNAL(returnPressed()), SIGNAL(returnPressed()));
  connect(iTransp, SIGNAL(returnPressed()), SIGNAL(returnPressed()));
  connect(iAnschl, SIGNAL(returnPressed()), SIGNAL(returnPressed()));
  connect(iVerz, SIGNAL(returnPressed()), SIGNAL(returnPressed()));
  connect(iLen, SIGNAL(returnPressed()), SIGNAL(returnPressed()));
  connect(iKompr, SIGNAL(returnPressed()), SIGNAL(returnPressed()));
  connect(iPan, SIGNAL(returnPressed()), SIGNAL(returnPressed()));
  
  connect(iOutputChannel, SIGNAL(escapePressed()), SIGNAL(escapePressed()));
  connect(iHBank, SIGNAL(escapePressed()), SIGNAL(escapePressed()));
  connect(iLBank, SIGNAL(escapePressed()), SIGNAL(escapePressed()));
  connect(iProgram, SIGNAL(escapePressed()), SIGNAL(escapePressed()));
  connect(iHBank, SIGNAL(escapePressed()), SIGNAL(escapePressed()));
  connect(iLBank, SIGNAL(escapePressed()), SIGNAL(escapePressed()));
  connect(iProgram, SIGNAL(escapePressed()), SIGNAL(escapePressed()));
  connect(iLautst, SIGNAL(escapePressed()), SIGNAL(escapePressed()));
  connect(iLautst, SIGNAL(escapePressed()), SIGNAL(escapePressed()));
  connect(iTransp, SIGNAL(escapePressed()), SIGNAL(escapePressed()));
  connect(iAnschl, SIGNAL(escapePressed()), SIGNAL(escapePressed()));
  connect(iVerz, SIGNAL(escapePressed()), SIGNAL(escapePressed()));
  connect(iLen, SIGNAL(escapePressed()), SIGNAL(escapePressed()));
  connect(iKompr, SIGNAL(escapePressed()), SIGNAL(escapePressed()));
  connect(iPan, SIGNAL(escapePressed()), SIGNAL(escapePressed()));
  connect(oRButton, SIGNAL(pressed()), SLOT(outRoutesPressed()));
  
  connect(MusEGlobal::song, SIGNAL(songChanged(MusECore::SongChangedFlags_t)), SLOT(songChanged(MusECore::SongChangedFlags_t)));
  connect(MusEGlobal::muse, SIGNAL(configChanged()), SLOT(configChanged()));
  
  connect(MusEGlobal::heartBeatTimer, SIGNAL(timeout()), SLOT(heartBeat()));
}

//---------------------------------------------------------
//   heartBeat
//---------------------------------------------------------

void MidiTrackInfo::heartBeat()
{
  if(_blockHeartbeatCount < 0)  // error
  {
    fprintf(stderr, "ERROR: MidiTrackInfo::heartBeat: _blockHeartbeatCount is < 0, resetting to 0\n");
    _blockHeartbeatCount = 0;
  }
  
  if(_blockHeartbeatCount > 0 || !isVisible() || !isEnabled() || !selected)
    return;
  switch(selected->type()) 
  {
    case MusECore::Track::MIDI:
    case MusECore::Track::DRUM:
    case MusECore::Track::NEW_DRUM:
    {
      MusECore::MidiTrack* track = (MusECore::MidiTrack*)selected;
      
      int outChannel = track->outChannel();
      int outPort    = track->outPort();
      MusECore::MidiPort* mp = &MusEGlobal::midiPorts[outPort];
      
      // Check for detection of midi general activity on chosen channels...
      int mpt = 0;
      MusECore::RouteList* rl = track->inRoutes();
      
      MusECore::ciRoute r = rl->begin();
      for( ; r != rl->end(); ++r)
      {
        if(r->type != MusECore::Route::MIDI_PORT_ROUTE || !r->isValid())
          continue;
        
        // NOTE: TODO: Code for channelless events like sysex, ** IF we end up using the 'special channel 17' method.
        //if(r->channel == -1)
//         if(r->channel == -1 || r->channel == 0)
//           continue;
        
        // No port assigned to the device?
        mpt = r->midiPort;
        if(mpt < 0 || mpt >= MIDI_PORTS)
          continue;

        const int det_bits = MusEGlobal::midiPorts[mpt].syncInfo().actDetectBits();
        if(!det_bits)
          continue;
        if(r->channel == -1 || (det_bits & (1 << r->channel)))
        {
          if(!_midiDetect)
          {
            _midiDetect = true;
            iChanDetectLabel->setPixmap(*redLedIcon);
          }  
          break;  
        }
      }
      // No activity detected?
      if(r == rl->end())
      {
        if(_midiDetect)
        {
          _midiDetect = false;
          iChanDetectLabel->setPixmap(*darkRedLedIcon);
        }  
      }
      
      int nprogram = mp->hwCtrlState(outChannel, MusECore::CTRL_PROGRAM);
      if(nprogram == MusECore::CTRL_VAL_UNKNOWN)
      {
        if(program != MusECore::CTRL_VAL_UNKNOWN)
        {
          program = MusECore::CTRL_VAL_UNKNOWN;
          if(iHBank->value() != 0)
          {
            iHBank->blockSignals(true);
            iHBank->setValue(0);
            iHBank->blockSignals(false);
          }
          if(iLBank->value() != 0)
          {
            iLBank->blockSignals(true);
            iLBank->setValue(0);
            iLBank->blockSignals(false);
          }
          if(iProgram->value() != 0)
          {
            iProgram->blockSignals(true);
            iProgram->setValue(0);
            iProgram->blockSignals(false);
          }
        }
          
        nprogram = mp->lastValidHWCtrlState(outChannel, MusECore::CTRL_PROGRAM);
        if(nprogram == MusECore::CTRL_VAL_UNKNOWN) 
        {
          const QString n(tr("<unknown>"));
          if(iPatch->text() != n)
            iPatch->setText(n);
        }
        else
        {
          MusECore::MidiInstrument* instr = mp->instrument();
          const QString name = instr->getPatchName(outChannel, nprogram, track->isDrumTrack(), true); // Include default.
          if(name.isEmpty())
          {
            const QString n("???");
            if(iPatch->text() != n)
              iPatch->setText(n);
          }
          else if(iPatch->text() != name)
            iPatch->setText(name);
        }         
      }
      else
      {
        // The optimizing below, to avoid repeatedly calling getPatchName, generally worked OK. 
        // But Fluidsynth revealed a flaw. When loading a song, updateTrackInfo is called which correctly 
        //  sets program = nprogram. But a synth will not receive midistate sysexes until later. 
        // With Fluidsynth, that messed up our optimizing because the soundfont has not loaded yet. 
        // fluid_synth_get_channel_preset returns 0 in FluidSynth::getPatchName which returns <unknown> 
        //  when asked by updateTrackInfo, which then sets the patch box text to <unknown>. Then nothing 
        //  happens here because program = nprogram.
        // I don't like the idea of calling getPatchName here at a high rate. 
        // So force an update of program at a slower rate here. 
        //
        // The alternative is to have a system where the synth can signal the host when a change has happened.
        // Or an 'isValidPatch' function, or make getPatchName (and several others) return 0, so that updateTrackInfo 
        //  can ignore it. Oops. No! Even if we make updateTrackInfo ignore it, then the same thing happens here. 
        // Thats is, program = nprogram but the text is still wrong.  Not much choice but to do this for now...
        if(++heartBeatCounter >= 20)
          heartBeatCounter = 0;
        if(program != nprogram || heartBeatCounter == 0) 
        {
              program = nprogram;
  
              MusECore::MidiInstrument* instr = mp->instrument();
              const QString name = instr->getPatchName(outChannel, program, track->isDrumTrack(), true); // Include default.
              if(iPatch->text() != name)
                iPatch->setText(name);

              int hb = ((program >> 16) & 0xff) + 1;
              if (hb == 0x100)
                    hb = 0;
              int lb = ((program >> 8) & 0xff) + 1;
              if (lb == 0x100)
                    lb = 0;
              int pr = (program & 0xff) + 1;
              if (pr == 0x100)
                    pr = 0;
            
              if(iHBank->value() != hb)
              {
                iHBank->blockSignals(true);
                iHBank->setValue(hb);
                iHBank->blockSignals(false);
              }
              if(iLBank->value() != lb)
              {
                iLBank->blockSignals(true);
                iLBank->setValue(lb);
                iLBank->blockSignals(false);
              }
              if(iProgram->value() != pr)
              {
                iProgram->blockSignals(true);
                iProgram->setValue(pr);
                iProgram->blockSignals(false);
              }
              
        }
      }

      MusECore::MidiController* mc = mp->midiController(MusECore::CTRL_VOLUME);
      int mn = mc->minVal();
      int v = mp->hwCtrlState(outChannel, MusECore::CTRL_VOLUME);
      if(v == MusECore::CTRL_VAL_UNKNOWN)
        v = mn - 1;
      else
        // Auto bias...
        v -= mc->bias();
      if(volume != v)
      {
        volume = v;
        if(iLautst->value() != v)
        {
          iLautst->blockSignals(true);
          iLautst->setValue(v);
          iLautst->blockSignals(false);
        }
      }
      
      mc = mp->midiController(MusECore::CTRL_PANPOT);
      mn = mc->minVal();
      v = mp->hwCtrlState(outChannel, MusECore::CTRL_PANPOT);
      if(v == MusECore::CTRL_VAL_UNKNOWN)
        v = mn - 1;
      else
        // Auto bias...
        v -= mc->bias();
      if(pan != v)
      {
        pan = v;
        if(iPan->value() != v)
        {
          iPan->blockSignals(true);
          iPan->setValue(v);
          iPan->blockSignals(false);
        }  
      }  
      
      if(iTransp->value() != track->transposition)
      {
        iTransp->blockSignals(true);
        iTransp->setValue(track->transposition);
        iTransp->blockSignals(false);
      }
      
      if(iAnschl->value() != track->velocity)
      {
        iAnschl->blockSignals(true);
        iAnschl->setValue(track->velocity);
        iAnschl->blockSignals(false);
      }
      
      if(iVerz->value() != track->delay)
      {
        iVerz->blockSignals(true);
        iVerz->setValue(track->delay);
        iVerz->blockSignals(false);
      }
      
      if(iLen->value() != track->len)
      {
        iLen->blockSignals(true);
        iLen->setValue(track->len);
        iLen->blockSignals(false);
      }
      
      if(iKompr->value() != track->compression)
      {
        iKompr->blockSignals(true);
        iKompr->setValue(track->compression);
        iKompr->blockSignals(false);
      }
    }  
    break;
    
    case MusECore::Track::WAVE:
    case MusECore::Track::AUDIO_OUTPUT:
    case MusECore::Track::AUDIO_INPUT:
    case MusECore::Track::AUDIO_GROUP:
    case MusECore::Track::AUDIO_AUX:
    case MusECore::Track::AUDIO_SOFTSYNTH:
    break;
  }  
}

//---------------------------------------------------------
//   configChanged
//---------------------------------------------------------

void MidiTrackInfo::configChanged()
      {
      //if (MusEGlobal::config.canvasBgPixmap.isEmpty()) {
      //      canvas->setBg(MusEGlobal::config.partCanvasBg);
      //      canvas->setBg(QPixmap());
      //}
      //else {
      //      canvas->setBg(QPixmap(MusEGlobal::config.canvasBgPixmap));
      //}
      
      setFont(MusEGlobal::config.fonts[1]);
      //updateTrackInfo(type);
      }

//---------------------------------------------------------
//   songChanged
//---------------------------------------------------------

void MidiTrackInfo::songChanged(MusECore::SongChangedFlags_t type)
{
  if(type & SC_TRACK_SELECTION)
  {
    MusECore::Track* t = MusEGlobal::song->selectedTrack();
    if(!t)
      selected = 0;
    else if(t->isMidiTrack())
    {
      selected = t;
      trackNameLabel->setObjectName(selected->cname());
    }
  }

  if(type == SC_SELECTION || type == SC_PART_SELECTION || type == SC_TRACK_SELECTION)
    return;
  if(!isVisible())
    return;
  updateTrackInfo(type);
}      

//---------------------------------------------------------
//   setLabelText
//---------------------------------------------------------

void MidiTrackInfo::setLabelText()
{
      MusECore::MidiTrack* track = (MusECore::MidiTrack*)selected;
      if(track)
        trackNameLabel->setText(track->name());
      else  
        trackNameLabel->setText(QString());
      
      if(track)
      {  
        QPalette pal;
        QColor c;
        if(track->type() == MusECore::Track::DRUM)
          c = MusEGlobal::config.drumTrackLabelBg; 
        else if (track->type() == MusECore::Track::MIDI)
          c = MusEGlobal::config.midiTrackLabelBg; 
        else if (track->type() == MusECore::Track::NEW_DRUM)
          c = MusEGlobal::config.newDrumTrackLabelBg;
        else
          printf("THIS SHOULD NEVER HAPPEN: track is not a MIDI track in MidiTrackInfo::setLabelText()!\n");
        
        QLinearGradient gradient(trackNameLabel->geometry().topLeft(), trackNameLabel->geometry().bottomLeft());
        gradient.setColorAt(0, c);
        gradient.setColorAt(0.5, c.lighter());
        gradient.setColorAt(1, c);
        pal.setBrush(trackNameLabel->backgroundRole(), gradient);
        trackNameLabel->setPalette(pal);
      }  
}
  
//---------------------------------------------------------
//   setLabelFont
//---------------------------------------------------------

void MidiTrackInfo::setLabelFont()
{
      // Use the new font #6 I created just for these labels (so far).
      // Set the label's font.
      trackNameLabel->setFont(MusEGlobal::config.fonts[6]);

      // Dealing with a horizontally constrained label. Ignore vertical. Use a minimum readable point size.
//       MusECore::autoAdjustFontSize(trackNameLabel, trackNameLabel->text(), false, true, MusEGlobal::config.fonts[6].pointSize(), 5);
//       const bool need_word_wrap =
//         !MusECore::autoAdjustFontSize(trackNameLabel,
//                                      trackNameLabel->text(), false, true,
//                                      MusEGlobal::config.fonts[6].pointSize(), 6);

      QFont fnt(MusEGlobal::config.fonts[6]);
//       const bool need_word_wrap =
//         !MusECore::autoAdjustFontSize(trackNameLabel,
//                                      trackNameLabel->text(), false, true,
//                                      fnt, 6);
      const bool need_word_wrap =
        !MusECore::autoAdjustFontSize(trackNameLabel,
                                     trackNameLabel->text(), fnt, false, true,
                                     fnt.pointSize(), 6);
      // TODO: Add an updateStyleSheet() method.

      if(trackNameLabel->wordWrap() != need_word_wrap)
        trackNameLabel->setWordWrap(need_word_wrap);
}
  
//---------------------------------------------------------
//   iOutputChannelChanged
//---------------------------------------------------------

void MidiTrackInfo::iOutputChannelChanged(int channel)
      {
      --channel;
      if(!selected)
        return;
      MusECore::MidiTrack* track = (MusECore::MidiTrack*)selected;
      if (channel != track->outChannel()) {
            ++_blockHeartbeatCount;
            MusECore::MidiTrack::ChangedType_t changed = MusECore::MidiTrack::NothingChanged;
            changed |= track->setOutChanAndUpdate(channel, false);
            MusEGlobal::audio->msgIdle(false);
            
            MusEGlobal::audio->msgUpdateSoloStates();
            MusEGlobal::song->update(SC_ROUTE | ((changed & MusECore::MidiTrack::DrumMapChanged) ? SC_DRUMMAP : 0));
            --_blockHeartbeatCount;
            }
      }

//---------------------------------------------------------
//   iOutputPortChanged
//---------------------------------------------------------

void MidiTrackInfo::iOutputPortChanged(int index)
      {
      if(!selected)
        return;
      int port_num = iOutput->itemData(index).toInt();
      if(port_num < 0 || port_num >= MIDI_PORTS)
        return;
      MusECore::MidiTrack* track = (MusECore::MidiTrack*)selected;
      if (port_num == track->outPort())
            return;
      ++_blockHeartbeatCount;
      MusECore::MidiTrack::ChangedType_t changed = MusECore::MidiTrack::NothingChanged;
      MusEGlobal::audio->msgIdle(true);
      changed |= track->setOutPortAndUpdate(port_num, false);
      MusEGlobal::audio->msgIdle(false);
      
      MusEGlobal::audio->msgUpdateSoloStates();
      MusEGlobal::song->update(SC_ROUTE | ((changed & MusECore::MidiTrack::DrumMapChanged) ? SC_DRUMMAP : 0));
      --_blockHeartbeatCount;
      }

//---------------------------------------------------------
//   inRoutesPressed
//---------------------------------------------------------

void MidiTrackInfo::inRoutesPressed()
{
  if(!selected)
    return;
  if(!selected->isMidiTrack())
    return;
  
  RoutePopupMenu* pup = new RoutePopupMenu();
  pup->exec(QCursor::pos(), MusECore::Route(selected), false);
  delete pup;
  iRButton->setDown(false);     
}

//---------------------------------------------------------
//   outRoutesPressed
//---------------------------------------------------------

void MidiTrackInfo::outRoutesPressed()
{
  if(!selected)
    return;
  if(!selected->isMidiTrack())
    return;
  
  RoutePopupMenu* pup = new RoutePopupMenu();
  pup->exec(QCursor::pos(), MusECore::Route(selected), true);
  delete pup;
  oRButton->setDown(false);     
}

//---------------------------------------------------------
//   iProgHBankChanged
//---------------------------------------------------------

void MidiTrackInfo::iProgHBankChanged()
      {
      if(!selected)
        return;
      MusECore::MidiTrack* track = (MusECore::MidiTrack*)selected;
      int channel = track->outChannel();
      int port    = track->outPort();
      int hbank   = iHBank->value();
      int lbank   = iLBank->value();
      int prog    = iProgram->value();

      if (hbank > 0 && hbank < 129)
            hbank -= 1;
      else
            hbank = 0xff;
      if (lbank > 0 && lbank < 129)
            lbank -= 1;
      else
            lbank = 0xff;
      if (prog > 0 && prog < 129)
            prog -= 1;
      else
            prog = 0xff;

      MusECore::MidiPort* mp = &MusEGlobal::midiPorts[port];
      if(prog == 0xff && hbank == 0xff && lbank == 0xff)
      {
        program = MusECore::CTRL_VAL_UNKNOWN;
        ++_blockHeartbeatCount;
        if(mp->hwCtrlState(channel, MusECore::CTRL_PROGRAM) != MusECore::CTRL_VAL_UNKNOWN)
          MusEGlobal::audio->msgSetHwCtrlState(mp, channel, MusECore::CTRL_PROGRAM, MusECore::CTRL_VAL_UNKNOWN);
        --_blockHeartbeatCount;
        return;  
      }
      
      ++_blockHeartbeatCount;
      int np = mp->hwCtrlState(channel, MusECore::CTRL_PROGRAM);
      if(np == MusECore::CTRL_VAL_UNKNOWN)
      {
        np = mp->lastValidHWCtrlState(channel, MusECore::CTRL_PROGRAM);
        if(np != MusECore::CTRL_VAL_UNKNOWN)
        {
          lbank = (np & 0xff00) >> 8; 
          prog = np & 0xff;
          if(prog == 0xff)
            prog = 0;
          int ilbnk = lbank;
          int iprog = prog;
          if(ilbnk == 0xff)
            ilbnk = -1;
          ++ilbnk;
          ++iprog;
          iLBank->blockSignals(true);
          iProgram->blockSignals(true);
          iLBank->setValue(ilbnk);
          iProgram->setValue(iprog);
          iLBank->blockSignals(false);
          iProgram->blockSignals(false);
        }
      }
      
      if(prog == 0xff && (hbank != 0xff || lbank != 0xff))
      {
        prog = 0;
        iProgram->blockSignals(true);
        iProgram->setValue(1);
        iProgram->blockSignals(false);
      }  
      program = (hbank << 16) + (lbank << 8) + prog;
      MusECore::MidiPlayEvent ev(0, port, channel, MusECore::ME_CONTROLLER, MusECore::CTRL_PROGRAM, program);
      MusEGlobal::audio->msgPlayMidiEvent(&ev);
      
      MusECore::MidiInstrument* instr = mp->instrument();
      iPatch->setText(instr->getPatchName(channel, program, track->isDrumTrack(), true)); // Include default.
//      updateTrackInfo();
      
      --_blockHeartbeatCount;
      }

//---------------------------------------------------------
//   iProgLBankChanged
//---------------------------------------------------------

void MidiTrackInfo::iProgLBankChanged()
      {
      if(!selected)
        return;
      MusECore::MidiTrack* track = (MusECore::MidiTrack*)selected;
      int channel = track->outChannel();
      int port    = track->outPort();
      int hbank   = iHBank->value();
      int lbank   = iLBank->value();
      int prog    = iProgram->value();

      if (hbank > 0 && hbank < 129)
            hbank -= 1;
      else
            hbank = 0xff;
      if (lbank > 0 && lbank < 129)
            lbank -= 1;
      else
            lbank = 0xff;
      if (prog > 0 && prog < 129)
            prog -= 1;
      else
            prog = 0xff;

      MusECore::MidiPort* mp = &MusEGlobal::midiPorts[port];
      if(prog == 0xff && hbank == 0xff && lbank == 0xff)
      {
        program = MusECore::CTRL_VAL_UNKNOWN;
        ++_blockHeartbeatCount;
        if(mp->hwCtrlState(channel, MusECore::CTRL_PROGRAM) != MusECore::CTRL_VAL_UNKNOWN)
          MusEGlobal::audio->msgSetHwCtrlState(mp, channel, MusECore::CTRL_PROGRAM, MusECore::CTRL_VAL_UNKNOWN);
        --_blockHeartbeatCount;
        return;  
      }
      
      ++_blockHeartbeatCount;
      int np = mp->hwCtrlState(channel, MusECore::CTRL_PROGRAM);
      if(np == MusECore::CTRL_VAL_UNKNOWN)
      {
        np = mp->lastValidHWCtrlState(channel, MusECore::CTRL_PROGRAM);
        if(np != MusECore::CTRL_VAL_UNKNOWN)
        {
          hbank = (np & 0xff0000) >> 16; 
          prog = np & 0xff;
          if(prog == 0xff)
            prog = 0;
          int ihbnk = hbank;
          int iprog = prog;
          if(ihbnk == 0xff)
            ihbnk = -1;
          ++ihbnk;
          ++iprog;
          iHBank->blockSignals(true);
          iProgram->blockSignals(true);
          iHBank->setValue(ihbnk);
          iProgram->setValue(iprog);
          iHBank->blockSignals(false);
          iProgram->blockSignals(false);
        }
      }
      
      if(prog == 0xff && (hbank != 0xff || lbank != 0xff))
      {
        prog = 0;
        iProgram->blockSignals(true);
        iProgram->setValue(1);
        iProgram->blockSignals(false);
      }  
      program = (hbank << 16) + (lbank << 8) + prog;
      MusECore::MidiPlayEvent ev(0, port, channel, MusECore::ME_CONTROLLER, MusECore::CTRL_PROGRAM, program);
      MusEGlobal::audio->msgPlayMidiEvent(&ev);
      
      MusECore::MidiInstrument* instr = mp->instrument();
      iPatch->setText(instr->getPatchName(channel, program, track->isDrumTrack(), true)); // Include default.
//      updateTrackInfo();
      
      --_blockHeartbeatCount;
      }

//---------------------------------------------------------
//   iProgramChanged
//---------------------------------------------------------

void MidiTrackInfo::iProgramChanged()
      {
      if(!selected)
        return;
      MusECore::MidiTrack* track = (MusECore::MidiTrack*)selected;
      int channel = track->outChannel();
      int port    = track->outPort();
      int hbank   = iHBank->value();
      int lbank   = iLBank->value();
      int prog    = iProgram->value();

      if (hbank > 0 && hbank < 129)
            hbank -= 1;
      else
            hbank = 0xff;
      if (lbank > 0 && lbank < 129)
            lbank -= 1;
      else
            lbank = 0xff;
      if (prog > 0 && prog < 129)
            prog -= 1;
      else
            prog = 0xff;

      MusECore::MidiPort *mp = &MusEGlobal::midiPorts[port];
      if(prog == 0xff)
      {
        ++_blockHeartbeatCount;
        program = MusECore::CTRL_VAL_UNKNOWN;
        iHBank->blockSignals(true);
        iLBank->blockSignals(true);
        iHBank->setValue(0);
        iLBank->setValue(0);
        iHBank->blockSignals(false);
        iLBank->blockSignals(false);
        
        if(mp->hwCtrlState(channel, MusECore::CTRL_PROGRAM) != MusECore::CTRL_VAL_UNKNOWN)
          MusEGlobal::audio->msgSetHwCtrlState(mp, channel, MusECore::CTRL_PROGRAM, MusECore::CTRL_VAL_UNKNOWN);
        --_blockHeartbeatCount;
        return;
      }
      else
      {
        ++_blockHeartbeatCount;
        int np = mp->hwCtrlState(channel, MusECore::CTRL_PROGRAM);
        if(np == MusECore::CTRL_VAL_UNKNOWN)
        {
          np = mp->lastValidHWCtrlState(channel, MusECore::CTRL_PROGRAM);
          if(np != MusECore::CTRL_VAL_UNKNOWN)
          {
            hbank = (np & 0xff0000) >> 16;
            lbank = (np & 0xff00) >> 8; 
            int ihbnk = hbank;
            int ilbnk = lbank;
            if(ihbnk == 0xff)
              ihbnk = -1;
            if(ilbnk == 0xff)
              ilbnk = -1;
            ++ihbnk;
            ++ilbnk;
            iHBank->blockSignals(true);
            iLBank->blockSignals(true);
            iHBank->setValue(ihbnk);
            iLBank->setValue(ilbnk);
            iHBank->blockSignals(false);
            iLBank->blockSignals(false);
          }
        }
        program = (hbank << 16) + (lbank << 8) + prog;
        MusECore::MidiPlayEvent ev(0, port, channel, MusECore::ME_CONTROLLER, MusECore::CTRL_PROGRAM, program);
        MusEGlobal::audio->msgPlayMidiEvent(&ev);
        
        MusECore::MidiInstrument* instr = mp->instrument();
        iPatch->setText(instr->getPatchName(channel, program, track->isDrumTrack(), true)); // Include default.
        
        --_blockHeartbeatCount;
      }
        
//      updateTrackInfo();
      }

//---------------------------------------------------------
//   iLautstChanged
//---------------------------------------------------------

void MidiTrackInfo::iLautstChanged(int val)
    {
      if(!selected)
        return;
      MusECore::MidiTrack* track = (MusECore::MidiTrack*)selected;
      int outPort = track->outPort();
      int chan = track->outChannel();
      MusECore::MidiPort* mp = &MusEGlobal::midiPorts[outPort];
      MusECore::MidiController* mctl = mp->midiController(MusECore::CTRL_VOLUME);
      if((val < mctl->minVal()) || (val > mctl->maxVal()))
      {
        if(mp->hwCtrlState(chan, MusECore::CTRL_VOLUME) != MusECore::CTRL_VAL_UNKNOWN)
          MusEGlobal::audio->msgSetHwCtrlState(mp, chan, MusECore::CTRL_VOLUME, MusECore::CTRL_VAL_UNKNOWN);
      }  
      else
      {
        val += mctl->bias();
        
        MusECore::MidiPlayEvent ev(0, outPort, chan,
          MusECore::ME_CONTROLLER, MusECore::CTRL_VOLUME, val);
        MusEGlobal::audio->msgPlayMidiEvent(&ev);
      }  
    }

//---------------------------------------------------------
//   iTranspChanged
//---------------------------------------------------------

void MidiTrackInfo::iTranspChanged(int val)
      {
      if(!selected)
        return;
      MusECore::MidiTrack* track = (MusECore::MidiTrack*)selected;
      track->transposition = val;
      }

//---------------------------------------------------------
//   iAnschlChanged
//---------------------------------------------------------

void MidiTrackInfo::iAnschlChanged(int val)
      {
      if(!selected)
        return;
      MusECore::MidiTrack* track = (MusECore::MidiTrack*)selected;
      track->velocity = val;
      }

//---------------------------------------------------------
//   iVerzChanged
//---------------------------------------------------------

void MidiTrackInfo::iVerzChanged(int val)
      {
      if(!selected)
        return;
      MusECore::MidiTrack* track = (MusECore::MidiTrack*)selected;
      track->delay = val;
      }

//---------------------------------------------------------
//   iLenChanged
//---------------------------------------------------------

void MidiTrackInfo::iLenChanged(int val)
      {
      if(!selected)
        return;
      MusECore::MidiTrack* track = (MusECore::MidiTrack*)selected;
      track->len = val;
      }

//---------------------------------------------------------
//   iKomprChanged
//---------------------------------------------------------

void MidiTrackInfo::iKomprChanged(int val)
      {
      if(!selected)
        return;
      MusECore::MidiTrack* track = (MusECore::MidiTrack*)selected;
      track->compression = val;
      }

//---------------------------------------------------------
//   iPanChanged
//---------------------------------------------------------

void MidiTrackInfo::iPanChanged(int val)
    {
      if(!selected)
        return;
      MusECore::MidiTrack* track = (MusECore::MidiTrack*)selected;
      int port    = track->outPort();
      int chan = track->outChannel();
      MusECore::MidiPort* mp = &MusEGlobal::midiPorts[port];  
      MusECore::MidiController* mctl = mp->midiController(MusECore::CTRL_PANPOT);
      if((val < mctl->minVal()) || (val > mctl->maxVal()))
      {
        if(mp->hwCtrlState(chan, MusECore::CTRL_PANPOT) != MusECore::CTRL_VAL_UNKNOWN)
          MusEGlobal::audio->msgSetHwCtrlState(mp, chan, MusECore::CTRL_PANPOT, MusECore::CTRL_VAL_UNKNOWN);
      }  
      else
      {
        val += mctl->bias();
        
        // Realtime Change:
        MusECore::MidiPlayEvent ev(0, port, chan,
          MusECore::ME_CONTROLLER, MusECore::CTRL_PANPOT, val);
        MusEGlobal::audio->msgPlayMidiEvent(&ev);
      }  
    }

//---------------------------------------------------------
//   patchPopupActivated
//---------------------------------------------------------

void MidiTrackInfo::patchPopupActivated(QAction* act)
{
  if(act && selected) 
  {
    MusECore::MidiTrack* track = static_cast<MusECore::MidiTrack*>(selected);
    const int channel = track->outChannel();
    const int port    = track->outPort();
    MusECore::MidiInstrument* instr = MusEGlobal::midiPorts[port].instrument();
    if(act->data().type() == QVariant::Int || act->data().type() == QVariant::UInt)
    {
      bool ok;
      int rv = act->data().toInt(&ok);
      if(ok && rv != -1)
      {
          ++_blockHeartbeatCount;
          // If the chosen patch's number is don't care (0xffffff),
          //  then by golly since we "don't care" let's just set it to '-/-/1'.
          // 0xffffff cannot be a valid patch number... yet...
          if(rv == MusECore::CTRL_PROGRAM_VAL_DONT_CARE)
            rv = 0xffff00;
          MusECore::MidiPlayEvent ev(0, port, channel, MusECore::ME_CONTROLLER, MusECore::CTRL_PROGRAM, rv);
          MusEGlobal::audio->msgPlayMidiEvent(&ev);
          updateTrackInfo(-1);
          --_blockHeartbeatCount;
      }
    }
    else if(instr->isSynti() && act->data().canConvert<void *>())
    {
      MusECore::SynthI *si = static_cast<MusECore::SynthI *>(instr);
      MusECore::Synth *s = si->synth();
#ifdef LV2_SUPPORT
      //only for lv2 synths call applyPreset function.
      if(s && s->synthType() == MusECore::Synth::LV2_SYNTH)
      {
          MusECore::LV2SynthIF *sif = static_cast<MusECore::LV2SynthIF *>(si->sif());
          //be pedantic about checks
          if(sif)
          {
            MusECore::MidiPort* mp = &MusEGlobal::midiPorts[port];
            if(mp)
            {
                if(mp->hwCtrlState(channel, MusECore::CTRL_PROGRAM) != MusECore::CTRL_VAL_UNKNOWN)
                  MusEGlobal::audio->msgSetHwCtrlState(mp, channel, MusECore::CTRL_PROGRAM, MusECore::CTRL_VAL_UNKNOWN);
                sif->applyPreset(act->data().value<void *>());
            }
          }
      }
#endif
    }
  }
}

//---------------------------------------------------------
//   instrPopup
//---------------------------------------------------------

void MidiTrackInfo::instrPopup()
      {
      if(!selected)
        return;
      MusECore::MidiTrack* track = static_cast<MusECore::MidiTrack*>(selected);
      int port = track->outPort();
      MusECore::MidiInstrument* instr = MusEGlobal::midiPorts[port].instrument();
      PopupMenu* pup = new PopupMenu(false);
      
      MusECore::MidiInstrument::populateInstrPopup(pup, instr, false);

      if(pup->actions().count() == 0)
      {
        delete pup;
        return;
      }  
      
      QAction *act = pup->exec(instrPushButton->mapToGlobal(QPoint(10,5)));
      if(act) 
      {
        QString s = act->text();
        for (MusECore::iMidiInstrument i = MusECore::midiInstruments.begin(); i != MusECore::midiInstruments.end(); ++i) 
        {
          if((*i)->iname() == s) 
          {
            MusEGlobal::audio->msgIdle(true); // Make it safe to edit structures
            MusEGlobal::midiPorts[port].changeInstrument(*i);
            MusEGlobal::audio->msgIdle(false);
            // Make sure device initializations are sent if necessary.
            MusEGlobal::audio->msgInitMidiDevices(false);  // false = Don't force
            MusEGlobal::song->update(SC_MIDI_INSTRUMENT);
            break;
          }
        }
      }
      delete pup;      
      }

//---------------------------------------------------------
//   patchPopup
//---------------------------------------------------------

void MidiTrackInfo::patchPopup()
      {
      if(!selected)
        return;
      MusECore::MidiTrack* track = (MusECore::MidiTrack*)selected;
      int channel = track->outChannel();
      int port    = track->outPort();
      MusECore::MidiInstrument* instr = MusEGlobal::midiPorts[port].instrument();
      PopupMenu* pup = new PopupMenu(true);
      
      instr->populatePatchPopup(pup, channel, track->isDrumTrack());

      if(pup->actions().count() == 0)
      {
        delete pup;
        return;
      }  
      
      connect(pup, SIGNAL(triggered(QAction*)), SLOT(patchPopupActivated(QAction*)));

      pup->exec(iPatch->mapToGlobal(QPoint(10,5)));
      delete pup;      
      }   

//---------------------------------------------------------
//   recEchoToggled
//---------------------------------------------------------

void MidiTrackInfo::recEchoToggled(bool v)
{
  if(!selected)
    return;
  selected->setRecMonitor(v);
  MusEGlobal::song->update(SC_TRACK_REC_MONITOR);
}

//---------------------------------------------------------
//   iProgHBankDoubleCLicked
//---------------------------------------------------------

void MidiTrackInfo::iProgHBankDoubleCLicked()
{
  if(!selected)
    return;
  MusECore::MidiTrack* track = (MusECore::MidiTrack*)selected;
  int port = track->outPort();
  int chan = track->outChannel();
  MusECore::MidiPort* mp = &MusEGlobal::midiPorts[port];  
  MusECore::MidiController* mctl = mp->midiController(MusECore::CTRL_PROGRAM);
  
  if(!track || !mctl)
      return;
  
  int lastv = mp->lastValidHWCtrlState(chan, MusECore::CTRL_PROGRAM);
  int curv = mp->hwCtrlState(chan, MusECore::CTRL_PROGRAM);
  
  if(curv == MusECore::CTRL_VAL_UNKNOWN)
  {
    // If no value has ever been set yet, use the current knob value 
    //  (or the controller's initial value?) to 'turn on' the controller.
    if(lastv == MusECore::CTRL_VAL_UNKNOWN)
    {
      int kiv = mctl->initVal();
      if(kiv == MusECore::CTRL_VAL_UNKNOWN)
        kiv = 0;

      ++_blockHeartbeatCount;
      MusECore::MidiPlayEvent ev(0, port, chan, MusECore::ME_CONTROLLER, MusECore::CTRL_PROGRAM, kiv);
      MusEGlobal::audio->msgPlayMidiEvent(&ev);
      --_blockHeartbeatCount;
    }
    else
    {
      // TODO
//       int hbank = (lastv >> 16) & 0xff;
//       if(hbank == 0xff)
//         lastv &= 0xffff;
//       else
//         lastv |= 0xff0000;
      
      ++_blockHeartbeatCount;
      MusECore::MidiPlayEvent ev(0, port, chan, MusECore::ME_CONTROLLER, MusECore::CTRL_PROGRAM, lastv);
      MusEGlobal::audio->msgPlayMidiEvent(&ev);
      --_blockHeartbeatCount;
    }
  }  
  else
  {      
    // TODO
//     int lasthb = 0xff;
//     if(lastv != MusECore::CTRL_VAL_UNKNOWN)
//       lasthb = (lastv >> 16) & 0xff;
//     
//     int hbank = (curv >> 16) & 0xff;
//     if(hbank == 0xff)
//     {
//       curv &= 0xffff;
//       if(lasthb != 0xff)
//         curv |= lasthb;
//     }
//     else
//       curv |= 0xff0000;
    
    if(mp->hwCtrlState(chan, MusECore::CTRL_PROGRAM) != MusECore::CTRL_VAL_UNKNOWN)
    {
      ++_blockHeartbeatCount;
      MusEGlobal::audio->msgSetHwCtrlState(mp, chan, MusECore::CTRL_PROGRAM, MusECore::CTRL_VAL_UNKNOWN);
      --_blockHeartbeatCount;
    }
  }
}

//---------------------------------------------------------
//   iProgLBankDoubleCLicked
//---------------------------------------------------------

void MidiTrackInfo::iProgLBankDoubleCLicked()
{
  if(!selected)
    return;
  MusECore::MidiTrack* track = (MusECore::MidiTrack*)selected;
  int port = track->outPort();
  int chan = track->outChannel();
  MusECore::MidiPort* mp = &MusEGlobal::midiPorts[port];  
  MusECore::MidiController* mctl = mp->midiController(MusECore::CTRL_PROGRAM);
  
  if(!track || !mctl)
      return;
  
  int lastv = mp->lastValidHWCtrlState(chan, MusECore::CTRL_PROGRAM);
  int curv = mp->hwCtrlState(chan, MusECore::CTRL_PROGRAM);
  
  if(curv == MusECore::CTRL_VAL_UNKNOWN)
  {
    // If no value has ever been set yet, use the current knob value 
    //  (or the controller's initial value?) to 'turn on' the controller.
    if(lastv == MusECore::CTRL_VAL_UNKNOWN)
    {
      int kiv = mctl->initVal();
      if(kiv == MusECore::CTRL_VAL_UNKNOWN)
        kiv = 0xff0000;
      
      //MusECore::MidiPlayEvent ev(0, port, chan, MusECore::ME_CONTROLLER, num, kiv);
      MusECore::MidiPlayEvent ev(0, port, chan, MusECore::ME_CONTROLLER, MusECore::CTRL_PROGRAM, kiv);
      MusEGlobal::audio->msgPlayMidiEvent(&ev);
    }
    else
    {
// TODO
//       int lbank = (lastv >> 8) & 0xff;
//       if(lbank == 0xff)
//         lastv &= 0xff00ff;
//       else
//       {
//         lastv |= 0xffff00;
//         //int hbank = (lastv >> 16) & 0xff;
//         //if(hbank != 0xff)
//         //  lastv |= 0xff0000;
//       }
      
      MusECore::MidiPlayEvent ev(0, port, chan, MusECore::ME_CONTROLLER, MusECore::CTRL_PROGRAM, lastv);
      MusEGlobal::audio->msgPlayMidiEvent(&ev);
    }
  }  
  else
  {
// TODO
//     //int lasthb = 0xff;
//     int lastlb = 0xff;
//     if(lastv != MusECore::CTRL_VAL_UNKNOWN)
//     {
//       //lasthb = (lastv >> 16) & 0xff;
//       lastlb = (lastv >> 8) & 0xff;
//     }
//     
//     int lbank = (curv >> 8) & 0xff;
//     if(lbank == 0xff)
//     {
//       curv &= 0xff00ff;
//       if(lastlb != 0xff)
//         curv |= lastlb;
//     }
//     else
//     {
//       curv |= 0xffff00;
//       //int hbank = (curv >> 16) & 0xff;
//       //if(hbank != 0xff)
//       //  curv |= 0xff0000;
//     }
      
    if(mp->hwCtrlState(chan, MusECore::CTRL_PROGRAM) != MusECore::CTRL_VAL_UNKNOWN)
      MusEGlobal::audio->msgSetHwCtrlState(mp, chan, MusECore::CTRL_PROGRAM, MusECore::CTRL_VAL_UNKNOWN);
//     MusECore::MidiPlayEvent ev(0, port, chan, MusECore::ME_CONTROLLER, MusECore::CTRL_PROGRAM, curv);
//     MusEGlobal::audio->msgPlayMidiEvent(&ev);
  }
}

//---------------------------------------------------------
//   iProgramDoubleClicked
//---------------------------------------------------------

void MidiTrackInfo::iProgramDoubleClicked()
{
  if(!selected)
    return;
  MusECore::MidiTrack* track = (MusECore::MidiTrack*)selected;
  int port = track->outPort();
  int chan = track->outChannel();
  MusECore::MidiPort* mp = &MusEGlobal::midiPorts[port];  
  MusECore::MidiController* mctl = mp->midiController(MusECore::CTRL_PROGRAM);
  
  if(!track || !mctl)
      return;
  
  int lastv = mp->lastValidHWCtrlState(chan, MusECore::CTRL_PROGRAM);
  int curv = mp->hwCtrlState(chan, MusECore::CTRL_PROGRAM);
  
  if(curv == MusECore::CTRL_VAL_UNKNOWN)
  {
    // If no value has ever been set yet, use the current knob value 
    //  (or the controller's initial value?) to 'turn on' the controller.
    if(lastv == MusECore::CTRL_VAL_UNKNOWN)
    {
      int kiv = mctl->initVal();
      if(kiv == MusECore::CTRL_VAL_UNKNOWN)
        kiv = 0xffff00;
      
      MusECore::MidiPlayEvent ev(0, port, chan, MusECore::ME_CONTROLLER, MusECore::CTRL_PROGRAM, kiv);
      MusEGlobal::audio->msgPlayMidiEvent(&ev);
    }
    else
    {
      MusECore::MidiPlayEvent ev(0, port, chan, MusECore::ME_CONTROLLER, MusECore::CTRL_PROGRAM, lastv);
      MusEGlobal::audio->msgPlayMidiEvent(&ev);
    }
  }  
  else
  {
    if(mp->hwCtrlState(chan, MusECore::CTRL_PROGRAM) != MusECore::CTRL_VAL_UNKNOWN)
      MusEGlobal::audio->msgSetHwCtrlState(mp, chan, MusECore::CTRL_PROGRAM, MusECore::CTRL_VAL_UNKNOWN);
  }
}

//---------------------------------------------------------
//   iLautstDoubleClicked
//---------------------------------------------------------

void MidiTrackInfo::iLautstDoubleClicked()
{
  if(!selected)
    return;
  MusECore::MidiTrack* track = (MusECore::MidiTrack*)selected;
  int port = track->outPort();
  int chan = track->outChannel();
  MusECore::MidiPort* mp = &MusEGlobal::midiPorts[port];  
  MusECore::MidiController* mctl = mp->midiController(MusECore::CTRL_VOLUME);
  
  if(!track || !mctl)
      return;
  
  int lastv = mp->lastValidHWCtrlState(chan, MusECore::CTRL_VOLUME);
  int curv = mp->hwCtrlState(chan, MusECore::CTRL_VOLUME);
  
  if(curv == MusECore::CTRL_VAL_UNKNOWN)
  {
    // If no value has ever been set yet, use the current knob value 
    //  (or the controller's initial value?) to 'turn on' the controller.
    if(lastv == MusECore::CTRL_VAL_UNKNOWN)
    {
      int kiv = mctl->initVal();
      if(kiv == MusECore::CTRL_VAL_UNKNOWN)
        // Set volume to 78% of range, so that if range is 0 - 127, then value is 100.
        kiv = lround(double(mctl->maxVal() - mctl->minVal()) * 0.7874);
      else
      {  
        if(kiv < mctl->minVal())
          kiv = mctl->minVal();
        if(kiv > mctl->maxVal())
          kiv = mctl->maxVal();
        kiv += mctl->bias();
      }    
      
      MusECore::MidiPlayEvent ev(0, port, chan, MusECore::ME_CONTROLLER, MusECore::CTRL_VOLUME, kiv);
      MusEGlobal::audio->msgPlayMidiEvent(&ev);
    }
    else
    {
      MusECore::MidiPlayEvent ev(0, port, chan, MusECore::ME_CONTROLLER, MusECore::CTRL_VOLUME, lastv);
      MusEGlobal::audio->msgPlayMidiEvent(&ev);
    }
  }  
  else
  {
    if(mp->hwCtrlState(chan, MusECore::CTRL_VOLUME) != MusECore::CTRL_VAL_UNKNOWN)
      MusEGlobal::audio->msgSetHwCtrlState(mp, chan, MusECore::CTRL_VOLUME, MusECore::CTRL_VAL_UNKNOWN);
  }
}

//---------------------------------------------------------
//   iPanDoubleClicked
//---------------------------------------------------------

void MidiTrackInfo::iPanDoubleClicked()
{
  if(!selected)
    return;
  MusECore::MidiTrack* track = (MusECore::MidiTrack*)selected;
  int port = track->outPort();
  int chan = track->outChannel();
  MusECore::MidiPort* mp = &MusEGlobal::midiPorts[port];  
  MusECore::MidiController* mctl = mp->midiController(MusECore::CTRL_PANPOT);
  
  if(!track || !mctl)
      return;
  
  int lastv = mp->lastValidHWCtrlState(chan, MusECore::CTRL_PANPOT);
  int curv = mp->hwCtrlState(chan, MusECore::CTRL_PANPOT);
  
  if(curv == MusECore::CTRL_VAL_UNKNOWN)
  {
    // If no value has ever been set yet, use the current knob value 
    //  (or the controller's initial value?) to 'turn on' the controller.
    if(lastv == MusECore::CTRL_VAL_UNKNOWN)
    {
      int kiv = mctl->initVal();
      if(kiv == MusECore::CTRL_VAL_UNKNOWN)
        // Set volume to 50% of range, so that if range is 0 - 127, then value is 64.
        kiv = lround(double(mctl->maxVal() - mctl->minVal()) * 0.5);
      else
      {  
        if(kiv < mctl->minVal())
          kiv = mctl->minVal();
        if(kiv > mctl->maxVal())
          kiv = mctl->maxVal();
        kiv += mctl->bias();
      }    
      
      MusECore::MidiPlayEvent ev(0, port, chan, MusECore::ME_CONTROLLER, MusECore::CTRL_PANPOT, kiv);
      MusEGlobal::audio->msgPlayMidiEvent(&ev);
    }
    else
    {
      MusECore::MidiPlayEvent ev(0, port, chan, MusECore::ME_CONTROLLER, MusECore::CTRL_PANPOT, lastv);
      MusEGlobal::audio->msgPlayMidiEvent(&ev);
    }
  }  
  else
  {
    if(mp->hwCtrlState(chan, MusECore::CTRL_PANPOT) != MusECore::CTRL_VAL_UNKNOWN)
      MusEGlobal::audio->msgSetHwCtrlState(mp, chan, MusECore::CTRL_PANPOT, MusECore::CTRL_VAL_UNKNOWN);
  }
}


//---------------------------------------------------------
//   updateTrackInfo
//---------------------------------------------------------

void MidiTrackInfo::updateTrackInfo(MusECore::SongChangedFlags_t flags)
{
      if(flags == SC_SELECTION || flags == SC_PART_SELECTION || flags == SC_TRACK_SELECTION)
        return;
        
      if(!selected)
        return;
      MusECore::MidiTrack* track = static_cast<MusECore::MidiTrack*>(selected);

      ++_blockHeartbeatCount;
      
      setLabelText();
      setLabelFont();

      const int outChannel = track->outChannel();
      const int outPort    = track->outPort();
      MusECore::MidiPort* mp = &MusEGlobal::midiPorts[outPort];
      MusECore::MidiInstrument* instr = mp->instrument();
      
      // If a port's instrument changed. Also if a track's output port changed, the instrument changed.
      if(flags & (SC_MIDI_INSTRUMENT | SC_ROUTE))  
      {
        if(instr)
        {
          instrPushButton->setText(instr->iname());
          if(instr->isSynti())
            instrPushButton->setEnabled(false);
          else
            instrPushButton->setEnabled(true);
        }
        else
          instrPushButton->setText(tr("<unknown>"));
      }
        
      if (flags & SC_ROUTE) {
        updateRouteButtons();
      }
      if (flags & SC_CONFIG) {
        iOutput->blockSignals(true);
        iOutput->clear();
  
        int item_idx = 0;
        for (int i = 0; i < MIDI_PORTS; ++i) {
              MusECore::MidiDevice* md = MusEGlobal::midiPorts[i].device(); 
              if(!md)  // In the case of this combo box, don't bother listing empty ports.
                continue;
              if(!(md->rwFlags() & 1) && (i != outPort))   // Only writeable ports, or current one.
                continue;
              QString name = QString("%1:%2").arg(i + 1).arg(MusEGlobal::midiPorts[i].portname());
              iOutput->insertItem(item_idx, name, i);
              if (i == outPort)
                    iOutput->setCurrentIndex(item_idx);
              item_idx++;
              }
        iOutput->blockSignals(false);
        
        iOutputChannel->blockSignals(true);
        iOutputChannel->setValue(outChannel+1);
        iOutputChannel->blockSignals(false);
      }
      
      if(flags & (SC_TRACK_REC_MONITOR))
      {
        // Set record echo.
        if(recEchoButton->isChecked() != track->recMonitor())
        {
          recEchoButton->blockSignals(true);
          recEchoButton->setChecked(track->recMonitor());
          recEchoButton->blockSignals(false);
        }
        recEchoButton->setIcon(track->recMonitor() ? QIcon(*midiThruOnIcon) : QIcon(*midiThruOffIcon));
      }
        
      int nprogram = mp->hwCtrlState(outChannel, MusECore::CTRL_PROGRAM);
      if(nprogram == MusECore::CTRL_VAL_UNKNOWN)
      {
        iHBank->blockSignals(true);
        iLBank->blockSignals(true);
        iProgram->blockSignals(true);
        iHBank->setValue(0);
        iLBank->setValue(0);
        iProgram->setValue(0);
        iHBank->blockSignals(false);
        iLBank->blockSignals(false);
        iProgram->blockSignals(false);
        
        program = MusECore::CTRL_VAL_UNKNOWN;
        nprogram = mp->lastValidHWCtrlState(outChannel, MusECore::CTRL_PROGRAM);
        if(nprogram == MusECore::CTRL_VAL_UNKNOWN)
          iPatch->setText(tr("<unknown>"));
        else
        {
          if(instr)
            iPatch->setText(instr->getPatchName(outChannel, nprogram, track->isDrumTrack(), true)); // Include default.
        }         
      }
      else
      {
            program = nprogram;

            iPatch->setText(instr->getPatchName(outChannel, program, track->isDrumTrack(), true)); // Include default.

            int hb = ((program >> 16) & 0xff) + 1;
            if (hb == 0x100)
                  hb = 0;
            int lb = ((program >> 8) & 0xff) + 1;
            if (lb == 0x100)
                  lb = 0;
            int pr = (program & 0xff) + 1;
            if (pr == 0x100)
                  pr = 0;
                  
            iHBank->blockSignals(true);
            iLBank->blockSignals(true);
            iProgram->blockSignals(true);

            iHBank->setValue(hb);
            iLBank->setValue(lb);
            iProgram->setValue(pr);

            iHBank->blockSignals(false);
            iLBank->blockSignals(false);
            iProgram->blockSignals(false);
      }
      
      MusECore::MidiController* mc = mp->midiController(MusECore::CTRL_VOLUME);
      int mn = mc->minVal();
      int v = mp->hwCtrlState(outChannel, MusECore::CTRL_VOLUME);
      volume = v;
      if(v == MusECore::CTRL_VAL_UNKNOWN)
        v = mn - 1;
      else
        // Auto bias...
        v -= mc->bias();
      iLautst->blockSignals(true);
      iLautst->setRange(mn - 1, mc->maxVal());
      iLautst->setValue(v);
      iLautst->blockSignals(false);
      
      mc = mp->midiController(MusECore::CTRL_PANPOT);
      mn = mc->minVal();
      v = mp->hwCtrlState(outChannel, MusECore::CTRL_PANPOT);
      pan = v;
      if(v == MusECore::CTRL_VAL_UNKNOWN)
        v = mn - 1;
      else
        // Auto bias...
        v -= mc->bias();
      iPan->blockSignals(true);
      iPan->setRange(mn - 1, mc->maxVal());
      iPan->setValue(v);
      iPan->blockSignals(false);
      
      --_blockHeartbeatCount;
}

//---------------------------------------------------------
//   progRecClicked
//---------------------------------------------------------

void MidiTrackInfo::progRecClicked()
      {
      if(!selected)
        return;
      MusECore::MidiTrack* track = (MusECore::MidiTrack*)selected;
      int portno       = track->outPort();
      int channel      = track->outChannel();
      MusECore::MidiPort* port   = &MusEGlobal::midiPorts[portno];
      int program      = port->hwCtrlState(channel, MusECore::CTRL_PROGRAM);
      if(program == MusECore::CTRL_VAL_UNKNOWN || program == 0xffffff) 
        return;

      unsigned tick = MusEGlobal::song->cpos();
      MusECore::Event a(MusECore::Controller);
      a.setTick(tick);
      a.setA(MusECore::CTRL_PROGRAM);
      a.setB(program);

      MusEGlobal::song->recordEvent(track, a);
      }

//---------------------------------------------------------
//   volRecClicked
//---------------------------------------------------------

void MidiTrackInfo::volRecClicked()
      {
      if(!selected)
        return;
      MusECore::MidiTrack* track = (MusECore::MidiTrack*)selected;
      int portno       = track->outPort();
      int channel      = track->outChannel();
      MusECore::MidiPort* port   = &MusEGlobal::midiPorts[portno];
      int volume       = port->hwCtrlState(channel, MusECore::CTRL_VOLUME);
      if(volume == MusECore::CTRL_VAL_UNKNOWN) 
        return;

      unsigned tick = MusEGlobal::song->cpos();
      MusECore::Event a(MusECore::Controller);
      a.setTick(tick);
      a.setA(MusECore::CTRL_VOLUME);
      a.setB(volume);

      MusEGlobal::song->recordEvent(track, a);
      }

//---------------------------------------------------------
//   panRecClicked
//---------------------------------------------------------

void MidiTrackInfo::panRecClicked()
      {
      if(!selected)
        return;
      MusECore::MidiTrack* track = (MusECore::MidiTrack*)selected;
      int portno       = track->outPort();
      int channel      = track->outChannel();
      MusECore::MidiPort* port   = &MusEGlobal::midiPorts[portno];
      int pan          = port->hwCtrlState(channel, MusECore::CTRL_PANPOT);
      if(pan == MusECore::CTRL_VAL_UNKNOWN) 
        return;

      unsigned tick = MusEGlobal::song->cpos();
      MusECore::Event a(MusECore::Controller);
      a.setTick(tick);
      a.setA(MusECore::CTRL_PANPOT);
      a.setB(pan);

      MusEGlobal::song->recordEvent(track, a);
      }

//---------------------------------------------------------
//   recordClicked
//---------------------------------------------------------

void MidiTrackInfo::recordClicked()
      {
      if(!selected)
        return;
      MusECore::MidiTrack* track = (MusECore::MidiTrack*)selected;
      int portno       = track->outPort();
      int channel      = track->outChannel();
      MusECore::MidiPort* port   = &MusEGlobal::midiPorts[portno];
      unsigned tick = MusEGlobal::song->cpos();
      
      int program = port->hwCtrlState(channel, MusECore::CTRL_PROGRAM);
      if(program != MusECore::CTRL_VAL_UNKNOWN && program != 0xffffff) 
      {
        MusECore::Event a(MusECore::Controller);
        a.setTick(tick);
        a.setA(MusECore::CTRL_PROGRAM);
        a.setB(program);
        MusEGlobal::song->recordEvent(track, a);
      }
      int volume = port->hwCtrlState(channel, MusECore::CTRL_VOLUME);
      if(volume != MusECore::CTRL_VAL_UNKNOWN) 
      {
        MusECore::Event a(MusECore::Controller);
        a.setTick(tick);
        a.setA(MusECore::CTRL_VOLUME);
        a.setB(volume);
        MusEGlobal::song->recordEvent(track, a);
      }
      int pan = port->hwCtrlState(channel, MusECore::CTRL_PANPOT);
      if(pan != MusECore::CTRL_VAL_UNKNOWN) 
      {
        MusECore::Event a(MusECore::Controller);
        a.setTick(tick);
        a.setA(MusECore::CTRL_PANPOT);
        a.setB(pan);
        MusEGlobal::song->recordEvent(track, a);
      }
    }

void MidiTrackInfo::resizeEvent(QResizeEvent* ev)
{
  QWidget::resizeEvent(ev);
  setLabelText();  
  setLabelFont();
}  

void MidiTrackInfo::updateRouteButtons()
{
  if(!selected)
    return;
  
  if (iRButton)
  {
      if (selected->noInRoute())
      {
        iRButton->setStyleSheet("background-color:red;");
        iRButton->setToolTip(MusEGlobal::noInputRoutingToolTipWarn);
      }
      else
      {
        iRButton->setStyleSheet("");
        iRButton->setToolTip(MusEGlobal::inputRoutingToolTipBase);
      }
  }

  if (oRButton)
  {
    if (selected->noOutRoute())
    {
      oRButton->setStyleSheet("background-color:red;");
      oRButton->setToolTip(MusEGlobal::noOutputRoutingToolTipWarn);
    }
    else
    {
      oRButton->setStyleSheet("");
      oRButton->setToolTip(MusEGlobal::outputRoutingToolTipBase);
    }
  }
}

} // namespace MusEGui
