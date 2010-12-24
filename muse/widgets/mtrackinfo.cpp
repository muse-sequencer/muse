//=========================================================
//  MusE
//  Linux Music Editor
//  (C) Copyright 2010 Werner Schweer and others (ws@seh.de)
//=========================================================

#include <QTimer>
#include <QMessageBox>

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

//---------------------------------------------------------
//   setTrack
//---------------------------------------------------------

void MidiTrackInfo::setTrack(Track* t)
{
  selected = t;
  //updateTrackInfo(-1);
}

//---------------------------------------------------------
//   midiTrackInfo
//---------------------------------------------------------

MidiTrackInfo::MidiTrackInfo(QWidget* parent, Track* sel_track) : QWidget(parent)
{ 
  setupUi(this); 
  _midiDetect = false; 
  
  selected = sel_track;
  
  // Since program covers 3 controls at once, it is in 'midi controller' units rather than 'gui control' units.
  //program  = -1;
  program  = CTRL_VAL_UNKNOWN;
  pan      = -65;
  volume   = -1;
  
  //iChanDetectLabel->setPixmap(*darkgreendotIcon);
  iChanDetectLabel->setPixmap(*darkRedLedIcon);
  
  QIcon recEchoIconSet;
  //recEchoIconSet.addPixmap(*recEchoIconOn, QIcon::Normal, QIcon::On);
  //recEchoIconSet.addPixmap(*recEchoIconOff, QIcon::Normal, QIcon::Off);
  recEchoIconSet.addPixmap(*midiConnectorRedBorderIcon, QIcon::Normal, QIcon::On);
  recEchoIconSet.addPixmap(*edit_midiIcon, QIcon::Normal, QIcon::Off);
  recEchoButton->setIcon(recEchoIconSet);
  //recEchoButton->setIcon(QIcon(*edit_midiIcon));
  //recEchoButton->setIconSize(edit_midiIcon->size());  
  
  
  // MusE-2: AlignCenter and WordBreak are set in the ui(3) file, but not supported by QLabel. Turn them on here.
  trackNameLabel->setAlignment(Qt::AlignCenter);
  //Qt::TextWordWrap is not available for alignment in Qt4 - Orcan
  // MusE-2 Tested: TextWrapAnywhere actually works, but in fact it takes precedence 
  //  over word wrap, so I found it is not really desirable. Maybe with a user setting...
  //trackNameLabel->setAlignment(Qt::AlignCenter | Qt::TextWordWrap | Qt::TextWrapAnywhere);
  //trackNameLabel->setSizePolicy(QSizePolicy(QSizePolicy::Ignored, QSizePolicy::Minimum));
  
  if(selected)
    trackNameLabel->setObjectName(selected->cname());
  QPalette pal;
  pal.setColor(trackNameLabel->backgroundRole(), QColor(0, 160, 255)); // Med blue
  trackNameLabel->setPalette(pal);
  trackNameLabel->setWordWrap(true);
  trackNameLabel->setAutoFillBackground(true);
  trackNameLabel->setTextFormat(Qt::PlainText);
  trackNameLabel->setLineWidth(2);
  trackNameLabel->setFrameStyle(QFrame::Sunken | QFrame::StyledPanel);
  trackNameLabel->setSizePolicy(QSizePolicy(QSizePolicy::Ignored, QSizePolicy::Minimum));
  
  // Added by Tim. p3.3.9
  setLabelText();
  setLabelFont();

  connect(iPatch, SIGNAL(released()), SLOT(instrPopup()));

  ///pop = new QMenu(iPatch);
  //pop->setCheckable(false); // not needed in Qt4

  // Removed by Tim. p3.3.9
  //connect(iName, SIGNAL(returnPressed()), SLOT(iNameChanged()));
  
  connect(iOutputChannel, SIGNAL(valueChanged(int)), SLOT(iOutputChannelChanged(int)));
  ///connect(iInputChannel, SIGNAL(textChanged(const QString&)), SLOT(iInputChannelChanged(const QString&)));
  connect(iHBank, SIGNAL(valueChanged(int)), SLOT(iProgHBankChanged()));
  connect(iLBank, SIGNAL(valueChanged(int)), SLOT(iProgLBankChanged()));
  connect(iProgram, SIGNAL(valueChanged(int)), SLOT(iProgramChanged()));
  connect(iHBank, SIGNAL(doubleClicked()), SLOT(iProgramDoubleClicked()));
  connect(iLBank, SIGNAL(doubleClicked()), SLOT(iProgramDoubleClicked()));
  connect(iProgram, SIGNAL(doubleClicked()), SLOT(iProgramDoubleClicked()));
  connect(iLautst, SIGNAL(valueChanged(int)), SLOT(iLautstChanged(int)));
  connect(iLautst, SIGNAL(doubleClicked()), SLOT(iLautstDoubleClicked()));
  connect(iTransp, SIGNAL(valueChanged(int)), SLOT(iTranspChanged(int)));
  connect(iAnschl, SIGNAL(valueChanged(int)), SLOT(iAnschlChanged(int)));
  connect(iVerz, SIGNAL(valueChanged(int)), SLOT(iVerzChanged(int)));
  connect(iLen, SIGNAL(valueChanged(int)), SLOT(iLenChanged(int)));
  connect(iKompr, SIGNAL(valueChanged(int)), SLOT(iKomprChanged(int)));
  connect(iPan, SIGNAL(valueChanged(int)), SLOT(iPanChanged(int)));
  connect(iPan, SIGNAL(doubleClicked()), SLOT(iPanDoubleClicked()));
  connect(iOutput, SIGNAL(activated(int)), SLOT(iOutputPortChanged(int)));
  ///connect(iInput, SIGNAL(textChanged(const QString&)), SLOT(iInputPortChanged(const QString&)));
  connect(recordButton, SIGNAL(clicked()), SLOT(recordClicked()));
  connect(progRecButton, SIGNAL(clicked()), SLOT(progRecClicked()));
  connect(volRecButton, SIGNAL(clicked()), SLOT(volRecClicked()));
  connect(panRecButton, SIGNAL(clicked()), SLOT(panRecClicked()));
  connect(recEchoButton, SIGNAL(toggled(bool)), SLOT(recEchoToggled(bool)));
  connect(iRButton, SIGNAL(pressed()), SLOT(inRoutesPressed()));
  
  // TODO: Works OK, but disabled for now, until we figure out what to do about multiple out routes and display values...
  oRButton->setEnabled(false);
  oRButton->setVisible(false);
  connect(oRButton, SIGNAL(pressed()), SLOT(outRoutesPressed()));
  
  connect(heartBeatTimer, SIGNAL(timeout()), SLOT(heartBeat()));
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
    case Track::MIDI:
    case Track::DRUM:
    {
      MidiTrack* track = (MidiTrack*)selected;
      
      int outChannel = track->outChannel();
      int outPort    = track->outPort();
      ///int ichMask    = track->inChannelMask();
      //int iptMask    = track->inPortMask();
      ///unsigned int iptMask    = track->inPortMask();
      
      MidiPort* mp = &midiPorts[outPort];
      
      // Set record echo.
      if(recEchoButton->isChecked() != track->recEcho())
      {
        recEchoButton->blockSignals(true);
        recEchoButton->setChecked(track->recEcho());
        recEchoButton->blockSignals(false);
      }
      
      // Check for detection of midi general activity on chosen channels...
      int mpt = 0;
      //int mch = 0;
      RouteList* rl = track->inRoutes();
      
      ciRoute r = rl->begin();
      //for( ; mpt < MIDI_PORTS; ++mpt)
      for( ; r != rl->end(); ++r)
      {
        //if(!r->isValid() || ((r->type != Route::ALSA_MIDI_ROUTE) && (r->type != Route::JACK_MIDI_ROUTE)))
        //if(!r->isValid() || (r->type != Route::MIDI_DEVICE_ROUTE))
        if(!r->isValid() || (r->type != Route::MIDI_PORT_ROUTE))   // p3.3.49
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
          //if(midiPorts[mpt].syncInfo().actDetect(mch) && (iptMask & (1 << mpt)) && (ichMask & (1 << mch)) )
          //if((iptMask & bitShiftLU[mpt]) && (midiPorts[mpt].syncInfo().actDetectBits() & ichMask) )
          //if(midiPorts[mpt].syncInfo().actDetectBits() & bitShiftLU[r->channel]) 
          if(midiPorts[mpt].syncInfo().actDetectBits() & r->channel)          // p3.3.50 Use new channel mask.
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
      
      int nprogram = mp->hwCtrlState(outChannel, CTRL_PROGRAM);
      if(nprogram == CTRL_VAL_UNKNOWN)
      {
        if(program != CTRL_VAL_UNKNOWN)
        {
          //printf("Arranger::midiTrackInfoHeartBeat setting program to unknown\n");
          
          program = CTRL_VAL_UNKNOWN;
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
          
        nprogram = mp->lastValidHWCtrlState(outChannel, CTRL_PROGRAM);
        if(nprogram == CTRL_VAL_UNKNOWN) 
        {
          //const char* n = "<unknown>";
          const QString n(tr("<unknown>"));
          //if(strcmp(iPatch->text().toLatin1().constData(), n) != 0)
          if(iPatch->text() != n)
          {
            //printf("Arranger::midiTrackInfoHeartBeat setting patch <unknown>\n");
          
            iPatch->setText(n);
          }  
        }
        else
        {
          MidiInstrument* instr = mp->instrument();
          QString name = instr->getPatchName(outChannel, nprogram, song->mtype(), track->type() == Track::DRUM);
          if(name.isEmpty())
          {
            const QString n("???");
            if(iPatch->text() != n)
              iPatch->setText(n);
          }
          else
          if(iPatch->text() != name)
          {
            //printf("Arranger::midiTrackInfoHeartBeat setting patch name\n");
          
            iPatch->setText(name);
          }  
        }         
      }
      else
      if(program != nprogram) 
      {
            program = nprogram;

            //int hb, lb, pr;
            //if (program == CTRL_VAL_UNKNOWN) {
            //      hb = lb = pr = 0;
            //      iPatch->setText("---");
            //      }
            //else 
            //{
                  MidiInstrument* instr = mp->instrument();
                  QString name = instr->getPatchName(outChannel, program, song->mtype(), track->type() == Track::DRUM);
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

      MidiController* mc = mp->midiController(CTRL_VOLUME);
      int mn = mc->minVal();
      int v = mp->hwCtrlState(outChannel, CTRL_VOLUME);
      if(v == CTRL_VAL_UNKNOWN)
      //{
        //v = mc->initVal();
        //if(v == CTRL_VAL_UNKNOWN)
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
      
      mc = mp->midiController(CTRL_PANPOT);
      mn = mc->minVal();
      v = mp->hwCtrlState(outChannel, CTRL_PANPOT);
      if(v == CTRL_VAL_UNKNOWN)
      //{
        //v = mc->initVal();
        //if(v == CTRL_VAL_UNKNOWN)
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
    
    case Track::WAVE:
    case Track::AUDIO_OUTPUT:
    case Track::AUDIO_INPUT:
    case Track::AUDIO_GROUP:
    case Track::AUDIO_AUX:
    case Track::AUDIO_SOFTSYNTH:
    break;
  }  
}

//---------------------------------------------------------
//   songChanged
//---------------------------------------------------------

void MidiTrackInfo::songChanged(int type)
      {
      // Is it simply a midi controller value adjustment? Forget it.
      if(type != SC_MIDI_CONTROLLER)
      {
/*        
        unsigned endTick = song->len();
        int offset  = AL::sigmap.ticksMeasure(endTick);
        hscroll->setRange(-offset, endTick + offset);  //DEBUG
        canvas->setOrigin(-offset, 0);
        time->setOrigin(-offset, 0);
  
        int bar, beat;
        unsigned tick;
        AL::sigmap.tickValues(endTick, &bar, &beat, &tick);
        if (tick || beat)
              ++bar;
        lenEntry->blockSignals(true);
        lenEntry->setValue(bar);
        lenEntry->blockSignals(false);
  
        trackSelectionChanged();
        canvas->partsChanged();
        typeBox->setCurrentIndex(int(song->mtype()));
        if (type & SC_SIG)
              time->redraw();
        if (type & SC_TEMPO)
              setGlobalTempo(tempomap.globalTempo());
              
        if(type & SC_TRACK_REMOVED)
        {
          AudioStrip* w = (AudioStrip*)(trackInfo->getWidget(2));
          if(w)
          {
            Track* t = w->getTrack();
            if(t)
            {
              TrackList* tl = song->tracks();
              iTrack it = tl->find(t);
              if(it == tl->end())
              {
                delete w;
                trackInfo->addWidget(0, 2);
                selected = 0;
              } 
            }   
          } 
        }
*/
      
      }
            
      updateTrackInfo(type);
    }

//---------------------------------------------------------
//   setLabelText
//---------------------------------------------------------

void MidiTrackInfo::setLabelText()
{
      MidiTrack* track = (MidiTrack*)selected;
      if(track)
        trackNameLabel->setText(track->name());
      else  
        trackNameLabel->setText(QString());
}
  
//---------------------------------------------------------
//   setLabelFont
//---------------------------------------------------------

void MidiTrackInfo::setLabelFont()
{
      if(!selected)
        return;
      MidiTrack* track = (MidiTrack*)selected;
        
      // Use the new font #6 I created just for these labels (so far).
      // Set the label's font.
      trackNameLabel->setFont(config.fonts[6]);
      // Dealing with a horizontally constrained label. Ignore vertical. Use a minimum readable point size.
      autoAdjustFontSize(trackNameLabel, trackNameLabel->text(), false, true, config.fonts[6].pointSize(), 5); 
}
  
//---------------------------------------------------------
//   iOutputChannelChanged
//---------------------------------------------------------

void MidiTrackInfo::iOutputChannelChanged(int channel)
      {
      --channel;
      if(!selected)
        return;
      MidiTrack* track = (MidiTrack*)selected;
      if (channel != track->outChannel()) {
            // Changed by T356.
            //track->setOutChannel(channel);
            audio->msgIdle(true);
            //audio->msgSetTrackOutChannel(track, channel);
            track->setOutChanAndUpdate(channel);
            audio->msgIdle(false);
            
            // may result in adding/removing mixer strip:
            //song->update(-1);
            song->update(SC_MIDI_CHANNEL);
            }
      }

//---------------------------------------------------------
//   iOutputPortChanged
//---------------------------------------------------------

void MidiTrackInfo::iOutputPortChanged(int index)
      {
      if(!selected)
        return;
      MidiTrack* track = (MidiTrack*)selected;
      if (index == track->outPort())
            return;
      // Changed by T356.
      //track->setOutPort(index);
      audio->msgIdle(true);
      //audio->msgSetTrackOutPort(track, index);
      track->setOutPortAndUpdate(index);
      audio->msgIdle(false);
      
      ///list->redraw();
      emit outputPortChanged(index);  
      }

//---------------------------------------------------------
//   routingPopupMenuActivated
//---------------------------------------------------------

//void MidiTrackInfo::routingPopupMenuActivated(int n)
void MidiTrackInfo::routingPopupMenuActivated(QAction* act)
{
  ///if(!midiTrackInfo || gRoutingPopupMenuMaster != midiTrackInfo || !selected || !selected->isMidiTrack())
  if((gRoutingPopupMenuMaster != this) || !selected || !selected->isMidiTrack())
    return;
  muse->routingPopupMenuActivated(selected, act->data().toInt());
}

#if 0
//---------------------------------------------------------
//   routingPopupViewActivated
//---------------------------------------------------------

void MidiTrackInfo::routingPopupViewActivated(const QModelIndex& mdi)
{
  ///if(!midiTrackInfo || gRoutingPopupMenuMaster != midiTrackInfo || !selected || !selected->isMidiTrack())
  if(gRoutingPopupMenuMaster != this || !selected || !selected->isMidiTrack())
    return;
  muse->routingPopupMenuActivated(selected, mdi.data().toInt());
}
#endif

//---------------------------------------------------------
//   inRoutesPressed
//---------------------------------------------------------

void MidiTrackInfo::inRoutesPressed()
{
  if(!selected)
    return;
  if(!selected->isMidiTrack())
    return;
  
  PopupMenu* pup = muse->prepareRoutingPopupMenu(selected, false);
  //PopupView* pup = muse->prepareRoutingPopupView(selected, false);

  if(!pup) {
    int ret = QMessageBox::warning(this, tr("No inputs"),
                                   tr("There are no midi inputs.\n"
                                      "Do you want to open the midi configuration dialog?"),
                                   QMessageBox::Ok | QMessageBox::Cancel,
                                   QMessageBox::Ok);
    if (ret == QMessageBox::Ok) {
        printf("open config midi ports\n");
        muse->configMidiPorts();
    }
    return;
  }
  
  ///gRoutingPopupMenuMaster = midiTrackInfo;
  gRoutingPopupMenuMaster = this;
  connect(pup, SIGNAL(triggered(QAction*)), SLOT(routingPopupMenuActivated(QAction*)));
  //connect(pup, SIGNAL(activated(const QModelIndex&)), SLOT(routingPopupViewActivated(const QModelIndex&)));
  connect(pup, SIGNAL(aboutToHide()), muse, SLOT(routingPopupMenuAboutToHide()));
  //connect(pup, SIGNAL(aboutToHide()), muse, SLOT(routingPopupViewAboutToHide()));
  pup->popup(QCursor::pos());
  //pup->setVisible(true);
  iRButton->setDown(false);     
  return;
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
  
  PopupMenu* pup = muse->prepareRoutingPopupMenu(selected, true);
  if(!pup)
    return;
  
  ///gRoutingPopupMenuMaster = midiTrackInfo;
  gRoutingPopupMenuMaster = this;
  connect(pup, SIGNAL(triggered(QAction*)), SLOT(routingPopupMenuActivated(QAction*)));
  connect(pup, SIGNAL(aboutToHide()), muse, SLOT(routingPopupMenuAboutToHide()));
  pup->popup(QCursor::pos());
  oRButton->setDown(false);     
  return;
}

//---------------------------------------------------------
//   iProgHBankChanged
//---------------------------------------------------------

void MidiTrackInfo::iProgHBankChanged()
      {
      if(!selected)
        return;
      MidiTrack* track = (MidiTrack*)selected;
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

      MidiPort* mp = &midiPorts[port];
      if(prog == 0xff && hbank == 0xff && lbank == 0xff)
      {
        program = CTRL_VAL_UNKNOWN;
        if(mp->hwCtrlState(channel, CTRL_PROGRAM) != CTRL_VAL_UNKNOWN)
          audio->msgSetHwCtrlState(mp, channel, CTRL_PROGRAM, CTRL_VAL_UNKNOWN);
        return;  
      }
      
      int np = mp->hwCtrlState(channel, CTRL_PROGRAM);
      if(np == CTRL_VAL_UNKNOWN)
      {
        np = mp->lastValidHWCtrlState(channel, CTRL_PROGRAM);
        if(np != CTRL_VAL_UNKNOWN)
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
      MidiPlayEvent ev(0, port, channel, ME_CONTROLLER, CTRL_PROGRAM, program);
      audio->msgPlayMidiEvent(&ev);
      
      MidiInstrument* instr = mp->instrument();
      iPatch->setText(instr->getPatchName(channel, program, song->mtype(), track->type() == Track::DRUM));
//      updateTrackInfo();
      }

//---------------------------------------------------------
//   iProgLBankChanged
//---------------------------------------------------------

void MidiTrackInfo::iProgLBankChanged()
      {
      if(!selected)
        return;
      MidiTrack* track = (MidiTrack*)selected;
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

      MidiPort* mp = &midiPorts[port];
      if(prog == 0xff && hbank == 0xff && lbank == 0xff)
      {
        program = CTRL_VAL_UNKNOWN;
        if(mp->hwCtrlState(channel, CTRL_PROGRAM) != CTRL_VAL_UNKNOWN)
          audio->msgSetHwCtrlState(mp, channel, CTRL_PROGRAM, CTRL_VAL_UNKNOWN);
        return;  
      }
      
      int np = mp->hwCtrlState(channel, CTRL_PROGRAM);
      if(np == CTRL_VAL_UNKNOWN)
      {
        np = mp->lastValidHWCtrlState(channel, CTRL_PROGRAM);
        if(np != CTRL_VAL_UNKNOWN)
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
      MidiPlayEvent ev(0, port, channel, ME_CONTROLLER, CTRL_PROGRAM, program);
      audio->msgPlayMidiEvent(&ev);
      
      MidiInstrument* instr = mp->instrument();
      iPatch->setText(instr->getPatchName(channel, program, song->mtype(), track->type() == Track::DRUM));
//      updateTrackInfo();
      }

//---------------------------------------------------------
//   iProgramChanged
//---------------------------------------------------------

void MidiTrackInfo::iProgramChanged()
      {
      if(!selected)
        return;
      MidiTrack* track = (MidiTrack*)selected;
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

      MidiPort *mp = &midiPorts[port];
      if(prog == 0xff)
      {
        program = CTRL_VAL_UNKNOWN;
        iHBank->blockSignals(true);
        iLBank->blockSignals(true);
        iHBank->setValue(0);
        iLBank->setValue(0);
        iHBank->blockSignals(false);
        iLBank->blockSignals(false);
        
        if(mp->hwCtrlState(channel, CTRL_PROGRAM) != CTRL_VAL_UNKNOWN)
          audio->msgSetHwCtrlState(mp, channel, CTRL_PROGRAM, CTRL_VAL_UNKNOWN);
        return;  
      }
      else
      {
        int np = mp->hwCtrlState(channel, CTRL_PROGRAM);
        if(np == CTRL_VAL_UNKNOWN)
        {
          np = mp->lastValidHWCtrlState(channel, CTRL_PROGRAM);
          if(np != CTRL_VAL_UNKNOWN)
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
        MidiPlayEvent ev(0, port, channel, ME_CONTROLLER, CTRL_PROGRAM, program);
        audio->msgPlayMidiEvent(&ev);
        
        MidiInstrument* instr = mp->instrument();
        iPatch->setText(instr->getPatchName(channel, program, song->mtype(), track->type() == Track::DRUM));
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
      MidiTrack* track = (MidiTrack*)selected;
      int outPort = track->outPort();
      int chan = track->outChannel();
      MidiPort* mp = &midiPorts[outPort];
      MidiController* mctl = mp->midiController(CTRL_VOLUME);
      if((val < mctl->minVal()) || (val > mctl->maxVal()))
      {
        if(mp->hwCtrlState(chan, CTRL_VOLUME) != CTRL_VAL_UNKNOWN)
          audio->msgSetHwCtrlState(mp, chan, CTRL_VOLUME, CTRL_VAL_UNKNOWN);
      }  
      else
      {
        val += mctl->bias();
        
        MidiPlayEvent ev(0, outPort, chan,
          ME_CONTROLLER, CTRL_VOLUME, val);
        audio->msgPlayMidiEvent(&ev);
      }  
      song->update(SC_MIDI_CONTROLLER);
    }

//---------------------------------------------------------
//   iTranspChanged
//---------------------------------------------------------

void MidiTrackInfo::iTranspChanged(int val)
      {
      if(!selected)
        return;
      MidiTrack* track = (MidiTrack*)selected;
      track->transposition = val;
      }

//---------------------------------------------------------
//   iAnschlChanged
//---------------------------------------------------------

void MidiTrackInfo::iAnschlChanged(int val)
      {
      if(!selected)
        return;
      MidiTrack* track = (MidiTrack*)selected;
      track->velocity = val;
      }

//---------------------------------------------------------
//   iVerzChanged
//---------------------------------------------------------

void MidiTrackInfo::iVerzChanged(int val)
      {
      if(!selected)
        return;
      MidiTrack* track = (MidiTrack*)selected;
      track->delay = val;
      }

//---------------------------------------------------------
//   iLenChanged
//---------------------------------------------------------

void MidiTrackInfo::iLenChanged(int val)
      {
      if(!selected)
        return;
      MidiTrack* track = (MidiTrack*)selected;
      track->len = val;
      }

//---------------------------------------------------------
//   iKomprChanged
//---------------------------------------------------------

void MidiTrackInfo::iKomprChanged(int val)
      {
      if(!selected)
        return;
      MidiTrack* track = (MidiTrack*)selected;
      track->compression = val;
      }

//---------------------------------------------------------
//   iPanChanged
//---------------------------------------------------------

void MidiTrackInfo::iPanChanged(int val)
    {
      if(!selected)
        return;
      MidiTrack* track = (MidiTrack*)selected;
      int port    = track->outPort();
      int chan = track->outChannel();
      MidiPort* mp = &midiPorts[port];  
      MidiController* mctl = mp->midiController(CTRL_PANPOT);
      if((val < mctl->minVal()) || (val > mctl->maxVal()))
      {
        if(mp->hwCtrlState(chan, CTRL_PANPOT) != CTRL_VAL_UNKNOWN)
          audio->msgSetHwCtrlState(mp, chan, CTRL_PANPOT, CTRL_VAL_UNKNOWN);
      }  
      else
      {
        val += mctl->bias();
        
        // Realtime Change:
        MidiPlayEvent ev(0, port, chan,
          ME_CONTROLLER, CTRL_PANPOT, val);
        audio->msgPlayMidiEvent(&ev);
      }  
      song->update(SC_MIDI_CONTROLLER);
    }

//---------------------------------------------------------
//   instrPopup
//---------------------------------------------------------

void MidiTrackInfo::instrPopup()
      {
      if(!selected)
        return;
      MidiTrack* track = (MidiTrack*)selected;
      int channel = track->outChannel();
      int port    = track->outPort();
      MidiInstrument* instr = midiPorts[port].instrument();
      QMenu* pup = new QMenu;
      ///instr->populatePatchPopup(pop, channel, song->mtype(), track->type() == Track::DRUM);
      instr->populatePatchPopup(pup, channel, song->mtype(), track->type() == Track::DRUM);

      ///if(pop->actions().count() == 0)
      ///  return;
      if(pup->actions().count() == 0)
      {
        delete pup;
        return;
      }  
      
      ///QAction *act = pop->exec(iPatch->mapToGlobal(QPoint(10,5)));
      QAction *act = pup->exec(iPatch->mapToGlobal(QPoint(10,5)));
      if (act) {
            int rv = act->data().toInt();
            MidiPlayEvent ev(0, port, channel, ME_CONTROLLER, CTRL_PROGRAM, rv);
            audio->msgPlayMidiEvent(&ev);
            updateTrackInfo(-1);
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
  MidiTrack* track = (MidiTrack*)selected;
  track->setRecEcho(v);
  
  //song->update(SC_???);
}

//---------------------------------------------------------
//   iProgramDoubleClicked
//---------------------------------------------------------

void MidiTrackInfo::iProgramDoubleClicked()
{
  if(!selected)
    return;
  MidiTrack* track = (MidiTrack*)selected;
  int port = track->outPort();
  int chan = track->outChannel();
  MidiPort* mp = &midiPorts[port];  
  MidiController* mctl = mp->midiController(CTRL_PROGRAM);
  
  if(!track || !mctl)
      return;
  
  int lastv = mp->lastValidHWCtrlState(chan, CTRL_PROGRAM);
  int curv = mp->hwCtrlState(chan, CTRL_PROGRAM);
  
  if(curv == CTRL_VAL_UNKNOWN)
  {
    // If no value has ever been set yet, use the current knob value 
    //  (or the controller's initial value?) to 'turn on' the controller.
    if(lastv == CTRL_VAL_UNKNOWN)
    {
      int kiv = mctl->initVal();
      //int kiv = lrint(_knob->value());
      if(kiv == CTRL_VAL_UNKNOWN)
        kiv = 0;
      //else
      //{  
        //if(kiv < mctrl->minVal())
        //  kiv = mctrl->minVal();
        //if(kiv > mctrl->maxVal())
        //  kiv = mctrl->maxVal();
        //kiv += mctrl->bias();
      //}    
      
      //MidiPlayEvent ev(0, port, chan, ME_CONTROLLER, num, kiv);
      MidiPlayEvent ev(0, port, chan, ME_CONTROLLER, CTRL_PROGRAM, kiv);
      audio->msgPlayMidiEvent(&ev);
    }
    else
    {
      MidiPlayEvent ev(0, port, chan, ME_CONTROLLER, CTRL_PROGRAM, lastv);
      audio->msgPlayMidiEvent(&ev);
    }
  }  
  else
  {
    if(mp->hwCtrlState(chan, CTRL_PROGRAM) != CTRL_VAL_UNKNOWN)
      audio->msgSetHwCtrlState(mp, chan, CTRL_PROGRAM, CTRL_VAL_UNKNOWN);
  }
  
  song->update(SC_MIDI_CONTROLLER);
}

//---------------------------------------------------------
//   iLautstDoubleClicked
//---------------------------------------------------------

void MidiTrackInfo::iLautstDoubleClicked()
{
  if(!selected)
    return;
  MidiTrack* track = (MidiTrack*)selected;
  int port = track->outPort();
  int chan = track->outChannel();
  MidiPort* mp = &midiPorts[port];  
  MidiController* mctl = mp->midiController(CTRL_VOLUME);
  
  if(!track || !mctl)
      return;
  
  int lastv = mp->lastValidHWCtrlState(chan, CTRL_VOLUME);
  int curv = mp->hwCtrlState(chan, CTRL_VOLUME);
  
  if(curv == CTRL_VAL_UNKNOWN)
  {
    // If no value has ever been set yet, use the current knob value 
    //  (or the controller's initial value?) to 'turn on' the controller.
    if(lastv == CTRL_VAL_UNKNOWN)
    {
      int kiv = mctl->initVal();
      //int kiv = lrint(_knob->value());
      if(kiv == CTRL_VAL_UNKNOWN)
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
      
      MidiPlayEvent ev(0, port, chan, ME_CONTROLLER, CTRL_VOLUME, kiv);
      audio->msgPlayMidiEvent(&ev);
    }
    else
    {
      MidiPlayEvent ev(0, port, chan, ME_CONTROLLER, CTRL_VOLUME, lastv);
      audio->msgPlayMidiEvent(&ev);
    }
  }  
  else
  {
    if(mp->hwCtrlState(chan, CTRL_VOLUME) != CTRL_VAL_UNKNOWN)
      audio->msgSetHwCtrlState(mp, chan, CTRL_VOLUME, CTRL_VAL_UNKNOWN);
  }
  
  song->update(SC_MIDI_CONTROLLER);
}

//---------------------------------------------------------
//   iPanDoubleClicked
//---------------------------------------------------------

void MidiTrackInfo::iPanDoubleClicked()
{
  if(!selected)
    return;
  MidiTrack* track = (MidiTrack*)selected;
  int port = track->outPort();
  int chan = track->outChannel();
  MidiPort* mp = &midiPorts[port];  
  MidiController* mctl = mp->midiController(CTRL_PANPOT);
  
  if(!track || !mctl)
      return;
  
  int lastv = mp->lastValidHWCtrlState(chan, CTRL_PANPOT);
  int curv = mp->hwCtrlState(chan, CTRL_PANPOT);
  
  if(curv == CTRL_VAL_UNKNOWN)
  {
    // If no value has ever been set yet, use the current knob value 
    //  (or the controller's initial value?) to 'turn on' the controller.
    if(lastv == CTRL_VAL_UNKNOWN)
    {
      int kiv = mctl->initVal();
      //int kiv = lrint(_knob->value());
      if(kiv == CTRL_VAL_UNKNOWN)
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
      
      MidiPlayEvent ev(0, port, chan, ME_CONTROLLER, CTRL_PANPOT, kiv);
      audio->msgPlayMidiEvent(&ev);
    }
    else
    {
      MidiPlayEvent ev(0, port, chan, ME_CONTROLLER, CTRL_PANPOT, lastv);
      audio->msgPlayMidiEvent(&ev);
    }
  }  
  else
  {
    if(mp->hwCtrlState(chan, CTRL_PANPOT) != CTRL_VAL_UNKNOWN)
      audio->msgSetHwCtrlState(mp, chan, CTRL_PANPOT, CTRL_VAL_UNKNOWN);
  }
  
  song->update(SC_MIDI_CONTROLLER);
}


//---------------------------------------------------------
//   updateTrackInfo
//---------------------------------------------------------

void MidiTrackInfo::updateTrackInfo(int flags)
{
      if(!selected)
        return;
      MidiTrack* track = (MidiTrack*)selected;
      
      // Is it simply a midi controller value adjustment? Forget it.
      if(flags == SC_MIDI_CONTROLLER)
        return;
        
      // p3.3.47 Update the routing popup menu if anything relevant changes.
      //if(gRoutingPopupMenuMaster == midiTrackInfo && selected && (flags & (SC_ROUTE | SC_CHANNELS | SC_CONFIG))) 
      if(flags & (SC_ROUTE | SC_CHANNELS | SC_CONFIG))     // p3.3.50
        // Use this handy shared routine.
        //muse->updateRouteMenus(selected);
        ///muse->updateRouteMenus(selected, midiTrackInfo);   // p3.3.50
        muse->updateRouteMenus(selected, this); 
      
      // Added by Tim. p3.3.9
      setLabelText();
      setLabelFont();
        
      //{
        int outChannel = track->outChannel();
        ///int inChannel  = track->inChannelMask();
        int outPort    = track->outPort();
        //int inPort     = track->inPortMask();
        ///unsigned int inPort     = track->inPortMask();
  
        //iInput->clear();
        iOutput->clear();
  
        for (int i = 0; i < MIDI_PORTS; ++i) {
              QString name;
              name.sprintf("%d:%s", i+1, midiPorts[i].portname().toLatin1().constData());
              iOutput->insertItem(i, name);
              if (i == outPort)
                    iOutput->setCurrentIndex(i);
              }
        //iInput->setText(bitmap2String(inPort));
        ///iInput->setText(u32bitmap2String(inPort));
        
        //iInputChannel->setText(bitmap2String(inChannel));
  
        // Removed by Tim. p3.3.9
        //if (iName->text() != selected->name()) {
        //      iName->setText(selected->name());
        //      iName->home(false);
        //      }
        
        iOutputChannel->setValue(outChannel+1);
        ///iInputChannel->setText(bitmap2String(inChannel));
        
        // Set record echo.
        if(recEchoButton->isChecked() != track->recEcho())
        {
          recEchoButton->blockSignals(true);
          recEchoButton->setChecked(track->recEcho());
          recEchoButton->blockSignals(false);
        }
        
        MidiPort* mp = &midiPorts[outPort];
        int nprogram = mp->hwCtrlState(outChannel, CTRL_PROGRAM);
        if(nprogram == CTRL_VAL_UNKNOWN)
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
          
          program = CTRL_VAL_UNKNOWN;
          nprogram = mp->lastValidHWCtrlState(outChannel, CTRL_PROGRAM);
          if(nprogram == CTRL_VAL_UNKNOWN) 
            //iPatch->setText(QString("<unknown>"));
            iPatch->setText(tr("<unknown>"));
          else
          {
            MidiInstrument* instr = mp->instrument();
            iPatch->setText(instr->getPatchName(outChannel, nprogram, song->mtype(), track->type() == Track::DRUM));
          }         
        }
        else
        //if (program != nprogram) 
        {
              program = nprogram;

              //int hb, lb, pr;
              //if (program == CTRL_VAL_UNKNOWN) {
              //      hb = lb = pr = 0;
              //      iPatch->setText("---");
              //      }
              //else 
              //{
                    MidiInstrument* instr = mp->instrument();
                    iPatch->setText(instr->getPatchName(outChannel, program, song->mtype(), track->type() == Track::DRUM));

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
        
        MidiController* mc = mp->midiController(CTRL_VOLUME);
        int mn = mc->minVal();
        int v = mp->hwCtrlState(outChannel, CTRL_VOLUME);
        volume = v;
        if(v == CTRL_VAL_UNKNOWN)
        //{
          //v = mc->initVal();
          //if(v == CTRL_VAL_UNKNOWN)
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
        
        mc = mp->midiController(CTRL_PANPOT);
        mn = mc->minVal();
        v = mp->hwCtrlState(outChannel, CTRL_PANPOT);
        pan = v;
        if(v == CTRL_VAL_UNKNOWN)
        //{
          //v = mc->initVal();
          //if(v == CTRL_VAL_UNKNOWN)
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
      
      
      // Does it include a midi controller value adjustment? Then handle it...
      //if(flags & SC_MIDI_CONTROLLER)
      //  seek();

      // Is it simply a midi controller value adjustment? Forget it.
      //if(flags != SC_MIDI_CONTROLLER)
      //{
        iTransp->setValue(track->transposition);
        iAnschl->setValue(track->velocity);
        iVerz->setValue(track->delay);
        iLen->setValue(track->len);
        iKompr->setValue(track->compression);
      //}
}

//---------------------------------------------------------
//   progRecClicked
//---------------------------------------------------------

void MidiTrackInfo::progRecClicked()
      {
      if(!selected)
        return;
      MidiTrack* track = (MidiTrack*)selected;
      int portno       = track->outPort();
      int channel      = track->outChannel();
      MidiPort* port   = &midiPorts[portno];
      int program      = port->hwCtrlState(channel, CTRL_PROGRAM);
      if(program == CTRL_VAL_UNKNOWN || program == 0xffffff) 
        return;

      unsigned tick = song->cpos();
      Event a(Controller);
      a.setTick(tick);
      a.setA(CTRL_PROGRAM);
      a.setB(program);

      song->recordEvent(track, a);
      }

//---------------------------------------------------------
//   volRecClicked
//---------------------------------------------------------

void MidiTrackInfo::volRecClicked()
      {
      if(!selected)
        return;
      MidiTrack* track = (MidiTrack*)selected;
      int portno       = track->outPort();
      int channel      = track->outChannel();
      MidiPort* port   = &midiPorts[portno];
      int volume       = port->hwCtrlState(channel, CTRL_VOLUME);
      if(volume == CTRL_VAL_UNKNOWN) 
        return;

      unsigned tick = song->cpos();
      Event a(Controller);
      a.setTick(tick);
      a.setA(CTRL_VOLUME);
      a.setB(volume);

      song->recordEvent(track, a);
      }

//---------------------------------------------------------
//   panRecClicked
//---------------------------------------------------------

void MidiTrackInfo::panRecClicked()
      {
      if(!selected)
        return;
      MidiTrack* track = (MidiTrack*)selected;
      int portno       = track->outPort();
      int channel      = track->outChannel();
      MidiPort* port   = &midiPorts[portno];
      int pan          = port->hwCtrlState(channel, CTRL_PANPOT);
      if(pan == CTRL_VAL_UNKNOWN) 
        return;

      unsigned tick = song->cpos();
      Event a(Controller);
      a.setTick(tick);
      a.setA(CTRL_PANPOT);
      a.setB(pan);

      song->recordEvent(track, a);
      }

//---------------------------------------------------------
//   recordClicked
//---------------------------------------------------------

void MidiTrackInfo::recordClicked()
      {
      if(!selected)
        return;
      MidiTrack* track = (MidiTrack*)selected;
      int portno       = track->outPort();
      int channel      = track->outChannel();
      MidiPort* port   = &midiPorts[portno];
      unsigned tick = song->cpos();
      
      int program = port->hwCtrlState(channel, CTRL_PROGRAM);
      if(program != CTRL_VAL_UNKNOWN && program != 0xffffff) 
      {
        Event a(Controller);
        a.setTick(tick);
        a.setA(CTRL_PROGRAM);
        a.setB(program);
        song->recordEvent(track, a);
      }
      int volume = port->hwCtrlState(channel, CTRL_VOLUME);
      if(volume != CTRL_VAL_UNKNOWN) 
      {
        Event a(Controller);
        a.setTick(tick);
        a.setA(CTRL_VOLUME);
        a.setB(volume);
        song->recordEvent(track, a);
      }
      int pan = port->hwCtrlState(channel, CTRL_PANPOT);
      if(pan != CTRL_VAL_UNKNOWN) 
      {
        Event a(Controller);
        a.setTick(tick);
        a.setA(CTRL_PANPOT);
        a.setB(pan);
        song->recordEvent(track, a);
      }
    }

