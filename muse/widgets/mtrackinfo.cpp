//=========================================================
//  MusE
//  Linux Music Editor
//  (C) Copyright 2010 Werner Schweer and others (ws@seh.de)
//  (C) Copyright 2011 Tim E. Real (terminator356 on sourceforge)
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
  recEchoButton->setIcon((selected && ((MusECore::MidiTrack*)selected)->recEcho()) ? QIcon(*midiThruOnIcon) : QIcon(*midiThruOffIcon));
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

  recordButton->setFocusPolicy(Qt::NoFocus);
  progRecButton->setFocusPolicy(Qt::NoFocus);
  volRecButton->setFocusPolicy(Qt::NoFocus);
  panRecButton->setFocusPolicy(Qt::NoFocus);
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
  {
    trackNameLabel->setObjectName(selected->cname());
    
    /*QPalette pal;
    QColor c;
    //pal.setColor(trackNameLabel->backgroundRole(), QColor(0, 160, 255)); // Med blue
    if(selected->type() == MusECore::Track::DRUM)
      c = MusEGlobal::config.drumTrackLabelBg; 
    else  
      c = MusEGlobal::config.midiTrackLabelBg; 
    
    QLinearGradient gradient(trackNameLabel->geometry().topLeft(), trackNameLabel->geometry().bottomLeft());
    //gradient.setColorAt(0, c.darker());
    //gradient.setColorAt(0, c);
    //gradient.setColorAt(1, c.darker());
    gradient.setColorAt(0, c.lighter());
    gradient.setColorAt(1, c);
    //palette.setBrush(QPalette::Button, gradient);
    //palette.setBrush(QPalette::Window, gradient);
    pal.setBrush(trackNameLabel->backgroundRole(), gradient);
    trackNameLabel->setPalette(pal);  */
  } 
  //else
  //{
  //  pal.setColor(trackNameLabel->backgroundRole(), MusEGlobal::config.midiTrackLabelBg); 
  //  trackNameLabel->setPalette(pal);
  //}  
  
  //trackNameLabel->setStyleSheet(QString("background-color: ") + QColor(0, 160, 255).name()); // Med blue
  trackNameLabel->setWordWrap(true);
  trackNameLabel->setAutoFillBackground(true);
  trackNameLabel->setTextFormat(Qt::PlainText);
  trackNameLabel->setLineWidth(2);
  trackNameLabel->setFrameStyle(QFrame::Sunken | QFrame::StyledPanel);
  trackNameLabel->setSizePolicy(QSizePolicy(QSizePolicy::Ignored, QSizePolicy::Minimum));
  
  setLabelText();
  setLabelFont();

  //setSizePolicy(QSizePolicy(QSizePolicy::Maximum, QSizePolicy::Expanding));
  
  connect(iPatch, SIGNAL(released()), SLOT(instrPopup()));

  //pop = new QMenu(iPatch);
  //pop->setCheckable(false); // not needed in Qt4

  // Removed by Tim. p3.3.9
  //connect(iName, SIGNAL(returnPressed()), SLOT(iNameChanged()));
  
  connect(iOutputChannel, SIGNAL(valueChanged(int)), SLOT(iOutputChannelChanged(int)));
  ///connect(iInputChannel, SIGNAL(textChanged(const QString&)), SLOT(iInputChannelChanged(const QString&)));
  connect(iHBank, SIGNAL(valueChanged(int)), SLOT(iProgHBankChanged()));
  connect(iLBank, SIGNAL(valueChanged(int)), SLOT(iProgLBankChanged()));
  connect(iProgram, SIGNAL(valueChanged(int)), SLOT(iProgramChanged()));
  connect(iHBank, SIGNAL(ctrlDoubleClicked()), SLOT(iProgramDoubleClicked()));
  connect(iLBank, SIGNAL(ctrlDoubleClicked()), SLOT(iProgramDoubleClicked()));
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
  ///connect(iInput, SIGNAL(textChanged(const QString&)), SLOT(iInputPortChanged(const QString&)));
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
  
  // TODO: Works OK, but disabled for now, until we figure out what to do about multiple out routes and display values...
  // Enabled (for Midi Port to Audio Input routing). p4.0.14 Tim.
  //oRButton->setEnabled(false);
  //oRButton->setVisible(false);
  connect(oRButton, SIGNAL(pressed()), SLOT(outRoutesPressed()));
  
  connect(MusEGlobal::song, SIGNAL(songChanged(int)), SLOT(songChanged(int)));
  connect(MusEGlobal::muse, SIGNAL(configChanged()), SLOT(configChanged()));
  
  connect(MusEGlobal::heartBeatTimer, SIGNAL(timeout()), SLOT(heartBeat()));
}

//---------------------------------------------------------
//   heartBeat
//---------------------------------------------------------

void MidiTrackInfo::heartBeat()
{
  ///if(!showTrackinfoFlag || !selected)
  if(!isVisible() || !isEnabled() || !selected)
    return;
  switch(selected->type()) 
  {
    case MusECore::Track::MIDI:
    case MusECore::Track::DRUM:
    {
      MusECore::MidiTrack* track = (MusECore::MidiTrack*)selected;
      
      int outChannel = track->outChannel();
      int outPort    = track->outPort();
      ///int ichMask    = track->inChannelMask();
      //int iptMask    = track->inPortMask();
      ///unsigned int iptMask    = track->inPortMask();
      
      MusECore::MidiPort* mp = &MusEGlobal::midiPorts[outPort];
      
      // Set record echo.
      //if(recEchoButton->isChecked() != track->recEcho())
      //{
      //  recEchoButton->blockSignals(true);
      //  recEchoButton->setChecked(track->recEcho());
      //  recEchoButton->blockSignals(false);
      //}
      
      // Check for detection of midi general activity on chosen channels...
      int mpt = 0;
      //int mch = 0;
      MusECore::RouteList* rl = track->inRoutes();
      
      MusECore::ciRoute r = rl->begin();
      //for( ; mpt < MIDI_PORTS; ++mpt)
      for( ; r != rl->end(); ++r)
      {
        //if(!r->isValid() || ((r->type != Route::ALSA_MIDI_ROUTE) && (r->type != Route::JACK_MIDI_ROUTE)))
        //if(!r->isValid() || (r->type != Route::MIDI_DEVICE_ROUTE))
        if(!r->isValid() || (r->type != MusECore::Route::MIDI_PORT_ROUTE))   // p3.3.49
          continue;
        
        // NOTE: TODO: Code for channelless events like sysex, ** IF we end up using the 'special channel 17' method.
        //if(r->channel == -1)
        if(r->channel == -1 || r->channel == 0)  // p3.3.50
          continue;
        
        // No port assigned to the device?
        //mpt = r->device->midiPort();
        mpt = r->midiPort;              // p3.3.49
        if(mpt < 0 || mpt >= MIDI_PORTS)
          continue;
        
        //for(; mch < MIDI_CHANNELS; ++mch)
        //{
          //if(MusEGlobal::midiPorts[mpt].syncInfo().actDetect(mch) && (iptMask & (1 << mpt)) && (ichMask & (1 << mch)) )
          //if((iptMask & bitShiftLU[mpt]) && (MusEGlobal::midiPorts[mpt].syncInfo().actDetectBits() & ichMask) )
          //if(MusEGlobal::midiPorts[mpt].syncInfo().actDetectBits() & bitShiftLU[r->channel]) 
          if(MusEGlobal::midiPorts[mpt].syncInfo().actDetectBits() & r->channel)          // p3.3.50 Use new channel mask.
          {
            //if(iChanTextLabel->paletteBackgroundColor() != green)
            //  iChanTextLabel->setPaletteBackgroundColor(green);
            //if(iChanDetectLabel->pixmap() != greendotIcon)
            if(!_midiDetect)
            {
              //printf("Arranger::midiTrackInfoHeartBeat setting green icon\n");
          
              _midiDetect = true;
              //iChanDetectLabel->setPixmap(*greendotIcon);
              iChanDetectLabel->setPixmap(*redLedIcon);
            }  
            break;  
          }
        //}
      }
      // No activity detected?
      //if(mch == MIDI_CHANNELS)
      //if(mpt == MIDI_PORTS)
      if(r == rl->end())
      {
        //if(iChanTextLabel->paletteBackgroundColor() != darkGreen)
        //  iChanTextLabel->setPaletteBackgroundColor(darkGreen);
        //if(iChanDetectLabel->pixmap() != darkgreendotIcon)
        if(_midiDetect)
        {
          //printf("Arranger::midiTrackInfoHeartBeat setting darkgreen icon\n");
          
          _midiDetect = false;
          //iChanDetectLabel->setPixmap(*darkgreendotIcon);
          iChanDetectLabel->setPixmap(*darkRedLedIcon);
        }  
      }
      
      int nprogram = mp->hwCtrlState(outChannel, MusECore::CTRL_PROGRAM);
      if(nprogram == MusECore::CTRL_VAL_UNKNOWN)
      {
        if(program != MusECore::CTRL_VAL_UNKNOWN)
        {
          //printf("Arranger::midiTrackInfoHeartBeat setting program to unknown\n");
          
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
          QString name = instr->getPatchName(outChannel, nprogram, MusEGlobal::song->mtype(), track->type() == MusECore::Track::DRUM);
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
        // p4.0.27 The optimizing below, to avoid repeatedly calling getPatchName, generally worked OK. 
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
  
              //int hb, lb, pr;
              //if (program == MusECore::CTRL_VAL_UNKNOWN) {
              //      hb = lb = pr = 0;
              //      iPatch->setText("---");
              //      }
              //else 
              //{
                    MusECore::MidiInstrument* instr = mp->instrument();
                    QString name = instr->getPatchName(outChannel, program, MusEGlobal::song->mtype(), track->type() == MusECore::Track::DRUM);
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
              //}
              
              //printf("Arranger::midiTrackInfoHeartBeat setting program\n");
            
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
      //{
        //v = mc->initVal();
        //if(v == MusECore::CTRL_VAL_UNKNOWN)
        //  v = 0;
        v = mn - 1;
      //}  
      else
        // Auto bias...
        v -= mc->bias();
      if(volume != v)
      {
        volume = v;
        if(iLautst->value() != v)
        {
          //printf("Arranger::midiTrackInfoHeartBeat setting volume\n");
          
          iLautst->blockSignals(true);
          //iLautst->setRange(mn - 1, mc->maxVal());
          iLautst->setValue(v);
          iLautst->blockSignals(false);
        }
      }
      
      mc = mp->midiController(MusECore::CTRL_PANPOT);
      mn = mc->minVal();
      v = mp->hwCtrlState(outChannel, MusECore::CTRL_PANPOT);
      if(v == MusECore::CTRL_VAL_UNKNOWN)
      //{
        //v = mc->initVal();
        //if(v == MusECore::CTRL_VAL_UNKNOWN)
        //  v = 0;
        v = mn - 1;
      //}  
      else
        // Auto bias...
        v -= mc->bias();
      if(pan != v)
      {
        pan = v;
        if(iPan->value() != v)
        {
          //printf("Arranger::midiTrackInfoHeartBeat setting pan\n");
          
          iPan->blockSignals(true);
          //iPan->setRange(mn - 1, mc->maxVal());
          iPan->setValue(v);
          iPan->blockSignals(false);
        }  
      }  
      
      // Does it include a midi controller value adjustment? Then handle it...
      //if(flags & SC_MIDI_CONTROLLER)
      //  seek();

      /*
      if(iTransp->value() != track->transposition)
        iTransp->setValue(track->transposition);
      if(iAnschl->value() != track->velocity)
        iAnschl->setValue(track->velocity);
      if(iVerz->value() != track->delay)
        iVerz->setValue(track->delay);
      if(iLen->value() != track->len)
        iLen->setValue(track->len);
      if(iKompr->value() != track->compression)
        iKompr->setValue(track->compression);
      */  
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
      //printf("MidiTrackInfo::configChanged\n");
      
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

void MidiTrackInfo::songChanged(int type)
{
  // Is it simply a midi controller value adjustment? Forget it.
  if(type == SC_MIDI_CONTROLLER)
    return;
  if(type == SC_SELECTION)
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
        //pal.setColor(trackNameLabel->backgroundRole(), QColor(0, 160, 255)); // Med blue
        if(track->type() == MusECore::Track::DRUM)
          c = MusEGlobal::config.drumTrackLabelBg; 
        else  
          c = MusEGlobal::config.midiTrackLabelBg; 
        
        QLinearGradient gradient(trackNameLabel->geometry().topLeft(), trackNameLabel->geometry().bottomLeft());
        //gradient.setColorAt(0, c.darker());
        //gradient.setColorAt(0, c);
        //gradient.setColorAt(1, c.darker());
        gradient.setColorAt(0, c);
        gradient.setColorAt(0.5, c.lighter());
        gradient.setColorAt(1, c);
        //palette.setBrush(QPalette::Button, gradient);
        //palette.setBrush(QPalette::Window, gradient);
        pal.setBrush(trackNameLabel->backgroundRole(), gradient);
        trackNameLabel->setPalette(pal);
      }  
}
  
//---------------------------------------------------------
//   setLabelFont
//---------------------------------------------------------

void MidiTrackInfo::setLabelFont()
{
      //if(!selected)
      //  return;
      //MusECore::MidiTrack* track = (MusECore::MidiTrack*)selected;
        
      // Use the new font #6 I created just for these labels (so far).
      // Set the label's font.
      trackNameLabel->setFont(MusEGlobal::config.fonts[6]);
      // Dealing with a horizontally constrained label. Ignore vertical. Use a minimum readable point size.
      MusECore::autoAdjustFontSize(trackNameLabel, trackNameLabel->text(), false, true, MusEGlobal::config.fonts[6].pointSize(), 5); 
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
            // Changed by T356.
            //track->setOutChannel(channel);
            MusEGlobal::audio->msgIdle(true);
            //audio->msgSetTrackOutChannel(track, channel);
            track->setOutChanAndUpdate(channel);
            MusEGlobal::audio->msgIdle(false);
            
            // may result in adding/removing mixer strip:
            //MusEGlobal::song->update(-1);
            //MusEGlobal::song->update(SC_MIDI_TRACK_PROP);
            MusEGlobal::audio->msgUpdateSoloStates();                   // p4.0.14
            //MusEGlobal::song->update(SC_MIDI_TRACK_PROP | SC_ROUTE);  //
            MusEGlobal::song->update(SC_MIDI_TRACK_PROP);               //
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
      // Changed by T356.
      //track->setOutPort(port_num);
      MusEGlobal::audio->msgIdle(true);
      //audio->msgSetTrackOutPort(track, port_num);
      track->setOutPortAndUpdate(port_num);
      MusEGlobal::audio->msgIdle(false);
      
      //MusEGlobal::song->update(SC_MIDI_TRACK_PROP);  
      MusEGlobal::audio->msgUpdateSoloStates();                   // p4.0.14
      //MusEGlobal::song->update(SC_MIDI_TRACK_PROP | SC_ROUTE);  //
      MusEGlobal::song->update(SC_MIDI_TRACK_PROP);               //
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
  
  //RoutePopupMenu* pup = MusEGlobal::muse->getRoutingPopupMenu();
  RoutePopupMenu* pup = new RoutePopupMenu();
  pup->exec(QCursor::pos(), selected, false);
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
  
  //RoutePopupMenu* pup = MusEGlobal::muse->getRoutingPopupMenu();
  RoutePopupMenu* pup = new RoutePopupMenu();
  pup->exec(QCursor::pos(), selected, true);
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
        if(mp->hwCtrlState(channel, MusECore::CTRL_PROGRAM) != MusECore::CTRL_VAL_UNKNOWN)
          MusEGlobal::audio->msgSetHwCtrlState(mp, channel, MusECore::CTRL_PROGRAM, MusECore::CTRL_VAL_UNKNOWN);
        return;  
      }
      
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
      iPatch->setText(instr->getPatchName(channel, program, MusEGlobal::song->mtype(), track->type() == MusECore::Track::DRUM));
//      updateTrackInfo();
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
        if(mp->hwCtrlState(channel, MusECore::CTRL_PROGRAM) != MusECore::CTRL_VAL_UNKNOWN)
          MusEGlobal::audio->msgSetHwCtrlState(mp, channel, MusECore::CTRL_PROGRAM, MusECore::CTRL_VAL_UNKNOWN);
        return;  
      }
      
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
      iPatch->setText(instr->getPatchName(channel, program, MusEGlobal::song->mtype(), track->type() == MusECore::Track::DRUM));
//      updateTrackInfo();
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
        program = MusECore::CTRL_VAL_UNKNOWN;
        iHBank->blockSignals(true);
        iLBank->blockSignals(true);
        iHBank->setValue(0);
        iLBank->setValue(0);
        iHBank->blockSignals(false);
        iLBank->blockSignals(false);
        
        if(mp->hwCtrlState(channel, MusECore::CTRL_PROGRAM) != MusECore::CTRL_VAL_UNKNOWN)
          MusEGlobal::audio->msgSetHwCtrlState(mp, channel, MusECore::CTRL_PROGRAM, MusECore::CTRL_VAL_UNKNOWN);
        return;  
      }
      else
      {
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
        iPatch->setText(instr->getPatchName(channel, program, MusEGlobal::song->mtype(), track->type() == MusECore::Track::DRUM));
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
      MusEGlobal::song->update(SC_MIDI_CONTROLLER);
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
      MusEGlobal::song->update(SC_MIDI_TRACK_PROP);  
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
      MusEGlobal::song->update(SC_MIDI_TRACK_PROP);  
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
      MusEGlobal::song->update(SC_MIDI_TRACK_PROP);  
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
      MusEGlobal::song->update(SC_MIDI_TRACK_PROP);  
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
      MusEGlobal::song->update(SC_MIDI_TRACK_PROP);  
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
      MusEGlobal::song->update(SC_MIDI_CONTROLLER);
    }

//---------------------------------------------------------
//   instrPopupActivated
//---------------------------------------------------------

void MidiTrackInfo::instrPopupActivated(QAction* act)
{
  //printf("MidiTrackInfo::instrPopupActivated\n"); 
  
  if(act && selected) 
  {
    int rv = act->data().toInt();
    if(rv != -1)
    {
      MusECore::MidiTrack* track = (MusECore::MidiTrack*)selected;
      int channel = track->outChannel();
      int port    = track->outPort();
      MusECore::MidiPlayEvent ev(0, port, channel, MusECore::ME_CONTROLLER, MusECore::CTRL_PROGRAM, rv);
      MusEGlobal::audio->msgPlayMidiEvent(&ev);
      updateTrackInfo(-1);
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
      MusECore::MidiTrack* track = (MusECore::MidiTrack*)selected;
      int channel = track->outChannel();
      int port    = track->outPort();
      MusECore::MidiInstrument* instr = MusEGlobal::midiPorts[port].instrument();
      //QMenu* pup = new QMenu;
      PopupMenu* pup = new PopupMenu(true);
      
      instr->populatePatchPopup(pup, channel, MusEGlobal::song->mtype(), track->type() == MusECore::Track::DRUM);
      //populatePatchPopup(instr, pup, channel, MusEGlobal::song->mtype(), track->type() == MusECore::Track::DRUM);

      if(pup->actions().count() == 0)
      {
        delete pup;
        return;
      }  
      
      connect(pup, SIGNAL(triggered(QAction*)), SLOT(instrPopupActivated(QAction*)));
      //connect(pup, SIGNAL(hovered(QAction*)), SLOT(instrPopupActivated(QAction*)));
      
      QAction *act = pup->exec(iPatch->mapToGlobal(QPoint(10,5)));
      if(act) 
      {
        int rv = act->data().toInt();
        if(rv != -1)
        {
          MusECore::MidiPlayEvent ev(0, port, channel, MusECore::ME_CONTROLLER, MusECore::CTRL_PROGRAM, rv);
          MusEGlobal::audio->msgPlayMidiEvent(&ev);
          updateTrackInfo(-1);
        }  
      }
            
      delete pup;      
      }

//---------------------------------------------------------
//   recEchoToggled
//---------------------------------------------------------

void MidiTrackInfo::recEchoToggled(bool v)
{
  if(!selected)
    return;
  MusECore::MidiTrack* track = (MusECore::MidiTrack*)selected;
  track->setRecEcho(v);
  MusEGlobal::song->update(SC_MIDI_TRACK_PROP);  
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
      //int kiv = lrint(_knob->value());
      if(kiv == MusECore::CTRL_VAL_UNKNOWN)
        kiv = 0;
      //else
      //{  
        //if(kiv < mctrl->minVal())
        //  kiv = mctrl->minVal();
        //if(kiv > mctrl->maxVal())
        //  kiv = mctrl->maxVal();
        //kiv += mctrl->bias();
      //}    
      
      //MusECore::MidiPlayEvent ev(0, port, chan, MusECore::ME_CONTROLLER, num, kiv);
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
  
  MusEGlobal::song->update(SC_MIDI_CONTROLLER);
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
      //int kiv = lrint(_knob->value());
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
  
  MusEGlobal::song->update(SC_MIDI_CONTROLLER);
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
      //int kiv = lrint(_knob->value());
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
  
  MusEGlobal::song->update(SC_MIDI_CONTROLLER);
}


//---------------------------------------------------------
//   updateTrackInfo
//---------------------------------------------------------

void MidiTrackInfo::updateTrackInfo(int flags)
{
      // Is it simply a midi controller value adjustment? Forget it.
      if(flags == SC_MIDI_CONTROLLER)
        return;
      if(flags == SC_SELECTION)
        return;
        
      if(!selected)
        return;
      MusECore::MidiTrack* track = (MusECore::MidiTrack*)selected;
      
      setLabelText();
      setLabelFont();
        
      if(flags & (SC_MIDI_TRACK_PROP))  
      {
        iTransp->blockSignals(true);
        iAnschl->blockSignals(true);
        iVerz->blockSignals(true);
        iLen->blockSignals(true);
        iKompr->blockSignals(true);
        iTransp->setValue(track->transposition);
        iAnschl->setValue(track->velocity);
        iVerz->setValue(track->delay);
        iLen->setValue(track->len);
        iKompr->setValue(track->compression);
        iTransp->blockSignals(false);
        iAnschl->blockSignals(false);
        iVerz->blockSignals(false);
        iLen->blockSignals(false);
        iKompr->blockSignals(false);
        
        int outChannel = track->outChannel();
        ///int inChannel  = track->inChannelMask();
        int outPort    = track->outPort();
        //int inPort     = track->inPortMask();
        ///unsigned int inPort     = track->inPortMask();
  
        iOutput->blockSignals(true);
        //iInput->clear();
        iOutput->clear();
  
        int item_idx = 0;
        for (int i = 0; i < MIDI_PORTS; ++i) {
              MusECore::MidiDevice* md = MusEGlobal::midiPorts[i].device(); 
              if(!md)  // In the case of this combo box, don't bother listing empty ports.             p4.0.41
                continue;
              //if(!(md->rwFlags() & 1 || md->isSynti()) && (i != outPort))  
              if(!(md->rwFlags() & 1) && (i != outPort))   // Only writeable ports, or current one.    p4.0.41
                continue;
              QString name;
              name.sprintf("%d:%s", i+1, MusEGlobal::midiPorts[i].portname().toLatin1().constData());
              iOutput->insertItem(item_idx, name, i);
              if (i == outPort)
                    iOutput->setCurrentIndex(item_idx);
              item_idx++;
              }
        iOutput->blockSignals(false);
        
        //iInput->setText(bitmap2String(inPort));
        ///iInput->setText(u32bitmap2String(inPort));
        
        //iInputChannel->setText(bitmap2String(inChannel));
  
        // Removed by Tim. p3.3.9
        //if (iName->text() != selected->name()) {
        //      iName->setText(selected->name());
        //      iName->home(false);
        //      }
        
        iOutputChannel->blockSignals(true);
        iOutputChannel->setValue(outChannel+1);
        iOutputChannel->blockSignals(false);
        ///iInputChannel->setText(bitmap2String(inChannel));
        
        // Set record echo.
        if(recEchoButton->isChecked() != track->recEcho())
        {
          recEchoButton->blockSignals(true);
          recEchoButton->setChecked(track->recEcho());
          recEchoButton->blockSignals(false);
        }
        recEchoButton->setIcon(track->recEcho() ? QIcon(*midiThruOnIcon) : QIcon(*midiThruOffIcon));
        //recEchoButton->setIconSize(midiThruOnIcon->size());  
      }
        
        int outChannel = track->outChannel();
        int outPort    = track->outPort();
        MusECore::MidiPort* mp = &MusEGlobal::midiPorts[outPort];
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
            //iPatch->setText(QString("<unknown>"));
            iPatch->setText(tr("<unknown>"));
          else
          {
            MusECore::MidiInstrument* instr = mp->instrument();
            iPatch->setText(instr->getPatchName(outChannel, nprogram, MusEGlobal::song->mtype(), track->type() == MusECore::Track::DRUM));
          }         
        }
        else
        //if (program != nprogram) 
        {
              program = nprogram;

              //int hb, lb, pr;
              //if (program == MusECore::CTRL_VAL_UNKNOWN) {
              //      hb = lb = pr = 0;
              //      iPatch->setText("---");
              //      }
              //else 
              //{
                    MusECore::MidiInstrument* instr = mp->instrument();
                    iPatch->setText(instr->getPatchName(outChannel, program, MusEGlobal::song->mtype(), track->type() == MusECore::Track::DRUM));

                    int hb = ((program >> 16) & 0xff) + 1;
                    if (hb == 0x100)
                          hb = 0;
                    int lb = ((program >> 8) & 0xff) + 1;
                    if (lb == 0x100)
                          lb = 0;
                    int pr = (program & 0xff) + 1;
                    if (pr == 0x100)
                          pr = 0;
              //}
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
        //{
          //v = mc->initVal();
          //if(v == MusECore::CTRL_VAL_UNKNOWN)
          //  v = 0;
          v = mn - 1;
        //}  
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
        //{
          //v = mc->initVal();
          //if(v == MusECore::CTRL_VAL_UNKNOWN)
          //  v = 0;
          v = mn - 1;
        //}  
        else
          // Auto bias...
          v -= mc->bias();
        iPan->blockSignals(true);
        iPan->setRange(mn - 1, mc->maxVal());
        iPan->setValue(v);
        iPan->blockSignals(false);
      //}
      
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
  //printf("MidiTrackInfo::resizeEvent\n");  
  QWidget::resizeEvent(ev);
  setLabelText();  
  setLabelFont();
}  

} // namespace MusEGui
