//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: trackinfo.cpp,v 1.10.2.15 2009/11/14 03:37:48 terminator356 Exp $
//  (C) Copyright 1999-2004 Werner Schweer (ws@seh.de)
//=========================================================

#include <QLayout>
#include <QComboBox>
#include <QToolButton>
#include <QLabel>
#include <QPalette>
#include <QColor>
#include <QMenu>
#include <QMessageBox>
//#include <q3hbox.h>
#include <QCheckBox>
#include <QPushButton>
//#include <q3widgetstack.h>
#include <QLineEdit>
#include <QTimer>
//#include <QModelIndex>
//Added by qt3to4:
#include <QPixmap>
#include <math.h>
#include <string.h>

#include "song.h"
#include "globals.h"
#include "spinbox.h"
#include "config.h"
#include "gconfig.h"
#include "arranger.h"
#include "midiport.h"
#include "mididev.h"
#include "utils.h"
#include "tlist.h"
#include "alayout.h"
#include "audio.h"
#include "mixer/amixer.h"
#include "midi.h"
#include "midictrl.h"
#include "xpm/muse_leftside_logo.xpm"
#include "mixer/astrip.h"
#include "icons.h"
#include "app.h"
#include "route.h"
#include "popupmenu.h"


//---------------------------------------------------------
//   midiTrackInfoHeartBeat
//---------------------------------------------------------

void Arranger::midiTrackInfoHeartBeat()
{
  if(!showTrackinfoFlag || !selected)
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
      if(midiTrackInfo->recEchoButton->isOn() != track->recEcho())
      {
        midiTrackInfo->recEchoButton->blockSignals(true);
        midiTrackInfo->recEchoButton->setOn(track->recEcho());
        midiTrackInfo->recEchoButton->blockSignals(false);
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
            //if(midiTrackInfo->iChanTextLabel->paletteBackgroundColor() != green)
            //  midiTrackInfo->iChanTextLabel->setPaletteBackgroundColor(green);
            //if(midiTrackInfo->iChanDetectLabel->pixmap() != greendotIcon)
            if(!midiTrackInfo->_midiDetect)
            {
              //printf("Arranger::midiTrackInfoHeartBeat setting green icon\n");
          
              midiTrackInfo->_midiDetect = true;
              //midiTrackInfo->iChanDetectLabel->setPixmap(*greendotIcon);
              midiTrackInfo->iChanDetectLabel->setPixmap(*redLedIcon);
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
        //if(midiTrackInfo->iChanTextLabel->paletteBackgroundColor() != darkGreen)
        //  midiTrackInfo->iChanTextLabel->setPaletteBackgroundColor(darkGreen);
        //if(midiTrackInfo->iChanDetectLabel->pixmap() != darkgreendotIcon)
        if(midiTrackInfo->_midiDetect)
        {
          //printf("Arranger::midiTrackInfoHeartBeat setting darkgreen icon\n");
          
          midiTrackInfo->_midiDetect = false;
          //midiTrackInfo->iChanDetectLabel->setPixmap(*darkgreendotIcon);
          midiTrackInfo->iChanDetectLabel->setPixmap(*darkRedLedIcon);
        }  
      }
      
      int nprogram = mp->hwCtrlState(outChannel, CTRL_PROGRAM);
      if(nprogram == CTRL_VAL_UNKNOWN)
      {
        if(program != CTRL_VAL_UNKNOWN)
        {
          //printf("Arranger::midiTrackInfoHeartBeat setting program to unknown\n");
          
          program = CTRL_VAL_UNKNOWN;
          if(midiTrackInfo->iHBank->value() != 0)
          {
            midiTrackInfo->iHBank->blockSignals(true);
            midiTrackInfo->iHBank->setValue(0);
            midiTrackInfo->iHBank->blockSignals(false);
          }
          if(midiTrackInfo->iLBank->value() != 0)
          {
            midiTrackInfo->iLBank->blockSignals(true);
            midiTrackInfo->iLBank->setValue(0);
            midiTrackInfo->iLBank->blockSignals(false);
          }
          if(midiTrackInfo->iProgram->value() != 0)
          {
            midiTrackInfo->iProgram->blockSignals(true);
            midiTrackInfo->iProgram->setValue(0);
            midiTrackInfo->iProgram->blockSignals(false);
          }
        }
          
        nprogram = mp->lastValidHWCtrlState(outChannel, CTRL_PROGRAM);
        if(nprogram == CTRL_VAL_UNKNOWN) 
        {
          //const char* n = "<unknown>";
          const QString n(tr("<unknown>"));
          //if(strcmp(midiTrackInfo->iPatch->text().latin1(), n) != 0)
          if(midiTrackInfo->iPatch->text() != n)
          {
            //printf("Arranger::midiTrackInfoHeartBeat setting patch <unknown>\n");
          
            midiTrackInfo->iPatch->setText(n);
          }  
        }
        else
        {
          MidiInstrument* instr = mp->instrument();
          const char* name = instr->getPatchName(outChannel, nprogram, song->mtype(), track->type() == Track::DRUM);
          if(!name)
          {
            if(midiTrackInfo->iPatch->text() != ("???"))
              midiTrackInfo->iPatch->setText("???");
          }
          else
          if(strcmp(midiTrackInfo->iPatch->text().latin1(), name) != 0)
          {
            //printf("Arranger::midiTrackInfoHeartBeat setting patch name\n");
          
            midiTrackInfo->iPatch->setText(name);
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
                  const char* name = instr->getPatchName(outChannel, program, song->mtype(), track->type() == Track::DRUM);
                  if(strcmp(midiTrackInfo->iPatch->text().latin1(), name) != 0)
                    midiTrackInfo->iPatch->setText(QString(name));

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
          
            if(midiTrackInfo->iHBank->value() != hb)
            {
              midiTrackInfo->iHBank->blockSignals(true);
              midiTrackInfo->iHBank->setValue(hb);
              midiTrackInfo->iHBank->blockSignals(false);
            }
            if(midiTrackInfo->iLBank->value() != lb)
            {
              midiTrackInfo->iLBank->blockSignals(true);
              midiTrackInfo->iLBank->setValue(lb);
              midiTrackInfo->iLBank->blockSignals(false);
            }
            if(midiTrackInfo->iProgram->value() != pr)
            {
              midiTrackInfo->iProgram->blockSignals(true);
              midiTrackInfo->iProgram->setValue(pr);
              midiTrackInfo->iProgram->blockSignals(false);
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
        if(midiTrackInfo->iLautst->value() != v)
        {
          //printf("Arranger::midiTrackInfoHeartBeat setting volume\n");
          
          midiTrackInfo->iLautst->blockSignals(true);
          //midiTrackInfo->iLautst->setRange(mn - 1, mc->maxVal());
          midiTrackInfo->iLautst->setValue(v);
          midiTrackInfo->iLautst->blockSignals(false);
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
        if(midiTrackInfo->iPan->value() != v)
        {
          //printf("Arranger::midiTrackInfoHeartBeat setting pan\n");
          
          midiTrackInfo->iPan->blockSignals(true);
          //midiTrackInfo->iPan->setRange(mn - 1, mc->maxVal());
          midiTrackInfo->iPan->setValue(v);
          midiTrackInfo->iPan->blockSignals(false);
        }  
      }  
      
      // Does it include a midi controller value adjustment? Then handle it...
      //if(flags & SC_MIDI_CONTROLLER)
      //  seek();

      /*
      if(midiTrackInfo->iTransp->value() != track->transposition)
        midiTrackInfo->iTransp->setValue(track->transposition);
      if(midiTrackInfo->iAnschl->value() != track->velocity)
        midiTrackInfo->iAnschl->setValue(track->velocity);
      if(midiTrackInfo->iVerz->value() != track->delay)
        midiTrackInfo->iVerz->setValue(track->delay);
      if(midiTrackInfo->iLen->value() != track->len)
        midiTrackInfo->iLen->setValue(track->len);
      if(midiTrackInfo->iKompr->value() != track->compression)
        midiTrackInfo->iKompr->setValue(track->compression);
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
//   showTrackInfo
//---------------------------------------------------------

void Arranger::showTrackInfo(bool flag)
      {
      showTrackinfoFlag = flag;
      trackInfo->setShown(flag);
      infoScroll->setShown(flag);
      updateTrackInfo(-1);
      }

//---------------------------------------------------------
//   genTrackInfo
//---------------------------------------------------------

void Arranger::genTrackInfo(QWidget* parent)
      {
      trackInfo = new WidgetStack(parent, "trackInfoStack");

      noTrackInfo          = new QWidget(trackInfo);
      QPixmap *noInfoPix   = new QPixmap(160, 1000); //muse_leftside_logo_xpm);
      const QPixmap *logo  = new QPixmap(muse_leftside_logo_xpm);
      noInfoPix->fill(noTrackInfo->paletteBackgroundColor() );
      copyBlt(noInfoPix, 10, 0, logo, 0,0, logo->width(), logo->height());
      noTrackInfo->setPaletteBackgroundPixmap(*noInfoPix);
      noTrackInfo->setGeometry(0, 0, 65, 200);
      noTrackInfo->setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding));

      midiTrackInfo = new MidiTrackInfo(trackInfo);
      trackInfo->addWidget(noTrackInfo,   0);
      trackInfo->addWidget(midiTrackInfo, 1);
      trackInfo->addWidget(0, 2);

      genMidiTrackInfo();
      }

//---------------------------------------------------------
//   updateTrackInfo
//---------------------------------------------------------

void Arranger::updateTrackInfo(int flags)
      {
      if (!showTrackinfoFlag) {
            switchInfo(-1);
            return;
            }
      if (selected == 0) {
            switchInfo(0);
            return;
            }
      if (selected->isMidiTrack()) {
            switchInfo(1);
            updateMidiTrackInfo(flags);
            }
      else {
            switchInfo(2);
            }
      }

//---------------------------------------------------------
//   switchInfo
//---------------------------------------------------------

void Arranger::switchInfo(int n)
      {
      if (n == 2) {
            AudioStrip* w = (AudioStrip*)(trackInfo->getWidget(2));
            if (w == 0 || selected != w->getTrack()) {
                  if (w)
                        delete w;
                  w = new AudioStrip(trackInfo, (AudioTrack*)selected);
                  connect(song, SIGNAL(songChanged(int)), w, SLOT(songChanged(int)));
                  connect(muse, SIGNAL(configChanged()), w, SLOT(configChanged()));
                  w->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed));
                  trackInfo->addWidget(w, 2);
                  w->show();
                  tgrid->activate();
                  tgrid->update();   // muse-2 Qt4
                  }
            }
      if (trackInfo->curIdx() == n)
            return;
      trackInfo->raiseWidget(n);
      tgrid->activate();
      tgrid->update();   // muse-2 Qt4
      }

//---------------------------------------------------------
//   setTrackInfoLabelText
//---------------------------------------------------------

void Arranger::setTrackInfoLabelText()
{
      MidiTrack* track = (MidiTrack*)selected;
      if(track)
        midiTrackInfo->trackNameLabel->setText(track->name());
      else  
        midiTrackInfo->trackNameLabel->setText(QString());
}
  
//---------------------------------------------------------
//   setTrackInfoLabelFont
//---------------------------------------------------------

void Arranger::setTrackInfoLabelFont()
{
      MidiTrack* track = (MidiTrack*)selected;
      if(!track)
        return;
        
      // Use the new font #6 I created just for these labels (so far).
      // Set the label's font.
      midiTrackInfo->trackNameLabel->setFont(config.fonts[6]);
      // Dealing with a horizontally constrained label. Ignore vertical. Use a minimum readable point size.
      autoAdjustFontSize(midiTrackInfo->trackNameLabel, midiTrackInfo->trackNameLabel->text(), false, true, config.fonts[6].pointSize(), 5); 
}
  
// Removed by Tim. p3.3.9

/*
//---------------------------------------------------------
//   iNameChanged
//---------------------------------------------------------

void Arranger::iNameChanged()
      {
      QString txt = midiTrackInfo->iName->text();
      if (txt == selected->name())
            return;
      
      TrackList* tl = song->tracks();
      for(iTrack i = tl->begin(); i != tl->end(); ++i) 
      {
        if((*i)->name() == txt) 
        {
          // Restore the text.
          midiTrackInfo->iName->blockSignals(true);
          midiTrackInfo->iName->setText(selected->name());
          midiTrackInfo->iName->blockSignals(false);
          
          QMessageBox::critical(this,
              tr("MusE: bad trackname"),
              tr("please choose a unique track name"),
              QMessageBox::Ok,
              QMessageBox::NoButton,
              QMessageBox::NoButton);
          
          midiTrackInfo->iName->setFocus();
          return;
        }
      }
      
      //Track* track = selected->clone();
      Track* track = selected->clone(false);
      selected->setName(txt);
      audio->msgChangeTrack(track, selected);
      }
*/

//---------------------------------------------------------
//   iOutputChannelChanged
//---------------------------------------------------------

void Arranger::iOutputChannelChanged(int channel)
      {
      --channel;
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

/*
//---------------------------------------------------------
//   iKanalChanged
//---------------------------------------------------------

void Arranger::iInputChannelChanged(const QString& s)
      {
      MidiTrack* track = (MidiTrack*)selected;
      int val = string2bitmap(s);
      if (val != track->inChannelMask()) {
            track->setInChannelMask(val);
            list->redraw();
            }
      }
*/

//---------------------------------------------------------
//   iOutputPortChanged
//---------------------------------------------------------

void Arranger::iOutputPortChanged(int index)
      {
      MidiTrack* track = (MidiTrack*)selected;
      if (index == track->outPort())
            return;
      // Changed by T356.
      //track->setOutPort(index);
      audio->msgIdle(true);
      //audio->msgSetTrackOutPort(track, index);
      track->setOutPortAndUpdate(index);
      audio->msgIdle(false);
      
      list->redraw();
      }

/*
//---------------------------------------------------------
//   iInputPortChanged
//---------------------------------------------------------

void Arranger::iInputPortChanged(const QString& s)
      {
      // Changed by Tim. p3.3.8
      //int val = string2bitmap(s);
      unsigned int val = string2u32bitmap(s);
      
      MidiTrack* track = (MidiTrack*)selected;
      if (val == track->inPortMask())
            return;
      track->setInPortMask(val);
      list->redraw();
      }
*/

//---------------------------------------------------------
//   routingPopupMenuActivated
//---------------------------------------------------------

//void Arranger::routingPopupMenuActivated(int n)
void Arranger::routingPopupMenuActivated(QAction* act)
{
  if(!midiTrackInfo || gRoutingPopupMenuMaster != midiTrackInfo || !selected || !selected->isMidiTrack())
    return;
  muse->routingPopupMenuActivated(selected, act->data().toInt());
}

#if 0
//---------------------------------------------------------
//   routingPopupViewActivated
//---------------------------------------------------------

void Arranger::routingPopupViewActivated(const QModelIndex& mdi)
{
  if(!midiTrackInfo || gRoutingPopupMenuMaster != midiTrackInfo || !selected || !selected->isMidiTrack())
    return;
  muse->routingPopupMenuActivated(selected, mdi.data().toInt());
}
#endif

//---------------------------------------------------------
//   inRoutesPressed
//---------------------------------------------------------

void Arranger::inRoutesPressed()
{
  if(!selected)
    return;
  if(!selected->isMidiTrack())
    return;
  
  PopupMenu* pup = muse->prepareRoutingPopupMenu(selected, false);
  //PopupView* pup = muse->prepareRoutingPopupView(selected, false);
  if(!pup)
    return;
  
  gRoutingPopupMenuMaster = midiTrackInfo;
  connect(pup, SIGNAL(triggered(QAction*)), SLOT(routingPopupMenuActivated(QAction*)));
  //connect(pup, SIGNAL(activated(const QModelIndex&)), SLOT(routingPopupViewActivated(const QModelIndex&)));
  connect(pup, SIGNAL(aboutToHide()), muse, SLOT(routingPopupMenuAboutToHide()));
  //connect(pup, SIGNAL(aboutToHide()), muse, SLOT(routingPopupViewAboutToHide()));
  pup->popup(QCursor::pos());
  //pup->setVisible(true);
  midiTrackInfo->iRButton->setDown(false);     
  return;
}

//---------------------------------------------------------
//   outRoutesPressed
//---------------------------------------------------------

void Arranger::outRoutesPressed()
{
  if(!selected)
    return;
  if(!selected->isMidiTrack())
    return;
  
  PopupMenu* pup = muse->prepareRoutingPopupMenu(selected, true);
  if(!pup)
    return;
  
  gRoutingPopupMenuMaster = midiTrackInfo;
  connect(pup, SIGNAL(triggered(QAction*)), SLOT(routingPopupMenuActivated(QAction*)));
  connect(pup, SIGNAL(aboutToHide()), muse, SLOT(routingPopupMenuAboutToHide()));
  pup->popup(QCursor::pos());
  midiTrackInfo->oRButton->setDown(false);     
  return;
}

//---------------------------------------------------------
//   iProgHBankChanged
//---------------------------------------------------------

void Arranger::iProgHBankChanged()
      {
      MidiTrack* track = (MidiTrack*)selected;
      int channel = track->outChannel();
      int port    = track->outPort();
      int hbank   = midiTrackInfo->iHBank->value();
      int lbank   = midiTrackInfo->iLBank->value();
      int prog    = midiTrackInfo->iProgram->value();

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
          midiTrackInfo->iLBank->blockSignals(true);
          midiTrackInfo->iProgram->blockSignals(true);
          midiTrackInfo->iLBank->setValue(ilbnk);
          midiTrackInfo->iProgram->setValue(iprog);
          midiTrackInfo->iLBank->blockSignals(false);
          midiTrackInfo->iProgram->blockSignals(false);
        }
      }
      
      if(prog == 0xff && (hbank != 0xff || lbank != 0xff))
      {
        prog = 0;
        midiTrackInfo->iProgram->blockSignals(true);
        midiTrackInfo->iProgram->setValue(1);
        midiTrackInfo->iProgram->blockSignals(false);
      }  
      program = (hbank << 16) + (lbank << 8) + prog;
      MidiPlayEvent ev(0, port, channel, ME_CONTROLLER, CTRL_PROGRAM, program);
      audio->msgPlayMidiEvent(&ev);
      
      MidiInstrument* instr = mp->instrument();
      const char* name = instr->getPatchName(channel, program, song->mtype(), track->type() == Track::DRUM);
      midiTrackInfo->iPatch->setText(QString(name));
//      updateTrackInfo();
      }

//---------------------------------------------------------
//   iProgLBankChanged
//---------------------------------------------------------

void Arranger::iProgLBankChanged()
      {
      MidiTrack* track = (MidiTrack*)selected;
      int channel = track->outChannel();
      int port    = track->outPort();
      int hbank   = midiTrackInfo->iHBank->value();
      int lbank   = midiTrackInfo->iLBank->value();
      int prog    = midiTrackInfo->iProgram->value();

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
          midiTrackInfo->iHBank->blockSignals(true);
          midiTrackInfo->iProgram->blockSignals(true);
          midiTrackInfo->iHBank->setValue(ihbnk);
          midiTrackInfo->iProgram->setValue(iprog);
          midiTrackInfo->iHBank->blockSignals(false);
          midiTrackInfo->iProgram->blockSignals(false);
        }
      }
      
      if(prog == 0xff && (hbank != 0xff || lbank != 0xff))
      {
        prog = 0;
        midiTrackInfo->iProgram->blockSignals(true);
        midiTrackInfo->iProgram->setValue(1);
        midiTrackInfo->iProgram->blockSignals(false);
      }  
      program = (hbank << 16) + (lbank << 8) + prog;
      MidiPlayEvent ev(0, port, channel, ME_CONTROLLER, CTRL_PROGRAM, program);
      audio->msgPlayMidiEvent(&ev);
      
      MidiInstrument* instr = mp->instrument();
      const char* name = instr->getPatchName(channel, program, song->mtype(), track->type() == Track::DRUM);
      midiTrackInfo->iPatch->setText(QString(name));
//      updateTrackInfo();
      }

//---------------------------------------------------------
//   iProgramChanged
//---------------------------------------------------------

void Arranger::iProgramChanged()
      {
      MidiTrack* track = (MidiTrack*)selected;
      int channel = track->outChannel();
      int port    = track->outPort();
      int hbank   = midiTrackInfo->iHBank->value();
      int lbank   = midiTrackInfo->iLBank->value();
      int prog    = midiTrackInfo->iProgram->value();

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
        midiTrackInfo->iHBank->blockSignals(true);
        midiTrackInfo->iLBank->blockSignals(true);
        midiTrackInfo->iHBank->setValue(0);
        midiTrackInfo->iLBank->setValue(0);
        midiTrackInfo->iHBank->blockSignals(false);
        midiTrackInfo->iLBank->blockSignals(false);
        
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
            midiTrackInfo->iHBank->blockSignals(true);
            midiTrackInfo->iLBank->blockSignals(true);
            midiTrackInfo->iHBank->setValue(ihbnk);
            midiTrackInfo->iLBank->setValue(ilbnk);
            midiTrackInfo->iHBank->blockSignals(false);
            midiTrackInfo->iLBank->blockSignals(false);
          }
        }
        program = (hbank << 16) + (lbank << 8) + prog;
        MidiPlayEvent ev(0, port, channel, ME_CONTROLLER, CTRL_PROGRAM, program);
        audio->msgPlayMidiEvent(&ev);
        
        MidiInstrument* instr = mp->instrument();
        const char* name = instr->getPatchName(channel, program, song->mtype(), track->type() == Track::DRUM);
        midiTrackInfo->iPatch->setText(QString(name));
      }
        
//      updateTrackInfo();
      }

//---------------------------------------------------------
//   iLautstChanged
//---------------------------------------------------------

void Arranger::iLautstChanged(int val)
    {
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

void Arranger::iTranspChanged(int val)
      {
      MidiTrack* track = (MidiTrack*)selected;
      track->transposition = val;
      }

//---------------------------------------------------------
//   iAnschlChanged
//---------------------------------------------------------

void Arranger::iAnschlChanged(int val)
      {
      MidiTrack* track = (MidiTrack*)selected;
      track->velocity = val;
      }

//---------------------------------------------------------
//   iVerzChanged
//---------------------------------------------------------

void Arranger::iVerzChanged(int val)
      {
      MidiTrack* track = (MidiTrack*)selected;
      track->delay = val;
      }

//---------------------------------------------------------
//   iLenChanged
//---------------------------------------------------------

void Arranger::iLenChanged(int val)
      {
      MidiTrack* track = (MidiTrack*)selected;
      track->len = val;
      }

//---------------------------------------------------------
//   iKomprChanged
//---------------------------------------------------------

void Arranger::iKomprChanged(int val)
      {
      MidiTrack* track = (MidiTrack*)selected;
      track->compression = val;
      }

//---------------------------------------------------------
//   iPanChanged
//---------------------------------------------------------

void Arranger::iPanChanged(int val)
    {
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

void Arranger::instrPopup()
      {
      MidiTrack* track = (MidiTrack*)selected;
      int channel = track->outChannel();
      int port    = track->outPort();
      MidiInstrument* instr = midiPorts[port].instrument();
      instr->populatePatchPopup(pop, channel, song->mtype(), track->type() == Track::DRUM);

      if(pop->count() == 0)
        return;
      int rv = pop->exec(midiTrackInfo->iPatch->mapToGlobal(QPoint(10,5)));
      if (rv != -1) {
            MidiPlayEvent ev(0, port, channel, ME_CONTROLLER, CTRL_PROGRAM, rv);
            audio->msgPlayMidiEvent(&ev);
            updateTrackInfo(-1);
            }
      }

//---------------------------------------------------------
//   recEchoToggled
//---------------------------------------------------------

void Arranger::recEchoToggled(bool v)
{
  MidiTrack* track = (MidiTrack*)selected;
  track->setRecEcho(v);
  
  //song->update(SC_???);
}

//---------------------------------------------------------
//   iProgramDoubleClicked
//---------------------------------------------------------

void Arranger::iProgramDoubleClicked()
{
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

void Arranger::iLautstDoubleClicked()
{
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

void Arranger::iPanDoubleClicked()
{
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
//   genMidiTrackInfo
//---------------------------------------------------------

void Arranger::genMidiTrackInfo()                                                                                                             
      {
      //midiTrackInfo->iChanDetectLabel->setPixmap(*darkgreendotIcon);
      midiTrackInfo->iChanDetectLabel->setPixmap(*darkRedLedIcon);
      
      QIcon recEchoIconSet;
      recEchoIconSet.setPixmap(*recEchoIconOn, QIcon::Automatic, QIcon::Normal, QIcon::On);
      recEchoIconSet.setPixmap(*recEchoIconOff, QIcon::Automatic, QIcon::Normal, QIcon::Off);
      midiTrackInfo->recEchoButton->setIconSet(recEchoIconSet);
      
      
      // MusE-2: AlignCenter and WordBreak are set in the ui(3) file, but not supported by QLabel. Turn them on here.
      midiTrackInfo->trackNameLabel->setAlignment(Qt::AlignCenter | Qt::TextWordWrap);
      // MusE-2 Tested: TextWrapAnywhere actually works, but in fact it takes precedence 
      //  over word wrap, so I found it is not really desirable. Maybe with a user setting...
      //midiTrackInfo->trackNameLabel->setAlignment(Qt::AlignCenter | Qt::TextWordWrap | Qt::TextWrapAnywhere);
      //midiTrackInfo->trackNameLabel->setSizePolicy(QSizePolicy(QSizePolicy::Ignored, QSizePolicy::Minimum));
      
      QPalette pal;
      pal.setColor(midiTrackInfo->trackNameLabel->backgroundRole(), QColor(0, 160, 255)); // Med blue
      midiTrackInfo->trackNameLabel->setPalette(pal);
      midiTrackInfo->trackNameLabel->setWordWrap(true);
      midiTrackInfo->trackNameLabel->setAutoFillBackground(true);
      
      // Added by Tim. p3.3.9
      setTrackInfoLabelText();
      setTrackInfoLabelFont();

      connect(midiTrackInfo->iPatch, SIGNAL(released()), SLOT(instrPopup()));

      pop = new Q3PopupMenu(midiTrackInfo->iPatch);
      pop->setCheckable(false);

      // Removed by Tim. p3.3.9
      //connect(midiTrackInfo->iName, SIGNAL(returnPressed()), SLOT(iNameChanged()));
      
      connect(midiTrackInfo->iOutputChannel, SIGNAL(valueChanged(int)), SLOT(iOutputChannelChanged(int)));
      ///connect(midiTrackInfo->iInputChannel, SIGNAL(textChanged(const QString&)), SLOT(iInputChannelChanged(const QString&)));
      connect(midiTrackInfo->iHBank, SIGNAL(valueChanged(int)), SLOT(iProgHBankChanged()));
      connect(midiTrackInfo->iLBank, SIGNAL(valueChanged(int)), SLOT(iProgLBankChanged()));
      connect(midiTrackInfo->iProgram, SIGNAL(valueChanged(int)), SLOT(iProgramChanged()));
      connect(midiTrackInfo->iHBank, SIGNAL(doubleClicked()), SLOT(iProgramDoubleClicked()));
      connect(midiTrackInfo->iLBank, SIGNAL(doubleClicked()), SLOT(iProgramDoubleClicked()));
      connect(midiTrackInfo->iProgram, SIGNAL(doubleClicked()), SLOT(iProgramDoubleClicked()));
      connect(midiTrackInfo->iLautst, SIGNAL(valueChanged(int)), SLOT(iLautstChanged(int)));
      connect(midiTrackInfo->iLautst, SIGNAL(doubleClicked()), SLOT(iLautstDoubleClicked()));
      connect(midiTrackInfo->iTransp, SIGNAL(valueChanged(int)), SLOT(iTranspChanged(int)));
      connect(midiTrackInfo->iAnschl, SIGNAL(valueChanged(int)), SLOT(iAnschlChanged(int)));
      connect(midiTrackInfo->iVerz, SIGNAL(valueChanged(int)), SLOT(iVerzChanged(int)));
      connect(midiTrackInfo->iLen, SIGNAL(valueChanged(int)), SLOT(iLenChanged(int)));
      connect(midiTrackInfo->iKompr, SIGNAL(valueChanged(int)), SLOT(iKomprChanged(int)));
      connect(midiTrackInfo->iPan, SIGNAL(valueChanged(int)), SLOT(iPanChanged(int)));
      connect(midiTrackInfo->iPan, SIGNAL(doubleClicked()), SLOT(iPanDoubleClicked()));
      connect(midiTrackInfo->iOutput, SIGNAL(activated(int)), SLOT(iOutputPortChanged(int)));
      ///connect(midiTrackInfo->iInput, SIGNAL(textChanged(const QString&)), SLOT(iInputPortChanged(const QString&)));
      connect(midiTrackInfo->recordButton, SIGNAL(clicked()), SLOT(recordClicked()));
      connect(midiTrackInfo->progRecButton, SIGNAL(clicked()), SLOT(progRecClicked()));
      connect(midiTrackInfo->volRecButton, SIGNAL(clicked()), SLOT(volRecClicked()));
      connect(midiTrackInfo->panRecButton, SIGNAL(clicked()), SLOT(panRecClicked()));
      connect(midiTrackInfo->recEchoButton, SIGNAL(toggled(bool)), SLOT(recEchoToggled(bool)));
      connect(midiTrackInfo->iRButton, SIGNAL(pressed()), SLOT(inRoutesPressed()));
      
      // TODO: Works OK, but disabled for now, until we figure out what to do about multiple out routes and display values...
      midiTrackInfo->oRButton->setEnabled(false);
      connect(midiTrackInfo->oRButton, SIGNAL(pressed()), SLOT(outRoutesPressed()));
      
      connect(heartBeatTimer, SIGNAL(timeout()), SLOT(midiTrackInfoHeartBeat()));
      }

//---------------------------------------------------------
//   updateMidiTrackInfo
//---------------------------------------------------------

void Arranger::updateMidiTrackInfo(int flags)
{
      MidiTrack* track = (MidiTrack*)selected;
      
      // Is it simply a midi controller value adjustment? Forget it.
      if(flags == SC_MIDI_CONTROLLER)
        return;
        
      // p3.3.47 Update the routing popup menu if anything relevant changes.
      //if(gRoutingPopupMenuMaster == midiTrackInfo && selected && (flags & (SC_ROUTE | SC_CHANNELS | SC_CONFIG))) 
      if(flags & (SC_ROUTE | SC_CHANNELS | SC_CONFIG))     // p3.3.50
        // Use this handy shared routine.
        //muse->updateRouteMenus(selected);
        muse->updateRouteMenus(selected, midiTrackInfo);   // p3.3.50
      
      // Added by Tim. p3.3.9
      setTrackInfoLabelText();
      setTrackInfoLabelFont();
        
      //{
        int outChannel = track->outChannel();
        ///int inChannel  = track->inChannelMask();
        int outPort    = track->outPort();
        //int inPort     = track->inPortMask();
        ///unsigned int inPort     = track->inPortMask();
  
        //midiTrackInfo->iInput->clear();
        midiTrackInfo->iOutput->clear();
  
        for (int i = 0; i < MIDI_PORTS; ++i) {
              QString name;
              name.sprintf("%d:%s", i+1, midiPorts[i].portname().latin1());
              midiTrackInfo->iOutput->insertItem(name, i);
              if (i == outPort)
                    midiTrackInfo->iOutput->setCurrentItem(i);
              }
        //midiTrackInfo->iInput->setText(bitmap2String(inPort));
        ///midiTrackInfo->iInput->setText(u32bitmap2String(inPort));
        
        //midiTrackInfo->iInputChannel->setText(bitmap2String(inChannel));
  
        // Removed by Tim. p3.3.9
        //if (midiTrackInfo->iName->text() != selected->name()) {
        //      midiTrackInfo->iName->setText(selected->name());
        //      midiTrackInfo->iName->home(false);
        //      }
        
        midiTrackInfo->iOutputChannel->setValue(outChannel+1);
        ///midiTrackInfo->iInputChannel->setText(bitmap2String(inChannel));
        
        // Set record echo.
        if(midiTrackInfo->recEchoButton->isOn() != track->recEcho())
        {
          midiTrackInfo->recEchoButton->blockSignals(true);
          midiTrackInfo->recEchoButton->setOn(track->recEcho());
          midiTrackInfo->recEchoButton->blockSignals(false);
        }
        
        MidiPort* mp = &midiPorts[outPort];
        int nprogram = mp->hwCtrlState(outChannel, CTRL_PROGRAM);
        if(nprogram == CTRL_VAL_UNKNOWN)
        {
          midiTrackInfo->iHBank->blockSignals(true);
          midiTrackInfo->iLBank->blockSignals(true);
          midiTrackInfo->iProgram->blockSignals(true);
          midiTrackInfo->iHBank->setValue(0);
          midiTrackInfo->iLBank->setValue(0);
          midiTrackInfo->iProgram->setValue(0);
          midiTrackInfo->iHBank->blockSignals(false);
          midiTrackInfo->iLBank->blockSignals(false);
          midiTrackInfo->iProgram->blockSignals(false);
          
          program = CTRL_VAL_UNKNOWN;
          nprogram = mp->lastValidHWCtrlState(outChannel, CTRL_PROGRAM);
          if(nprogram == CTRL_VAL_UNKNOWN) 
            //midiTrackInfo->iPatch->setText(QString("<unknown>"));
            midiTrackInfo->iPatch->setText(tr("<unknown>"));
          else
          {
            MidiInstrument* instr = mp->instrument();
            const char* name = instr->getPatchName(outChannel, nprogram, song->mtype(), track->type() == Track::DRUM);
            midiTrackInfo->iPatch->setText(QString(name));
          }         
        }
        else
        //if (program != nprogram) 
        {
              program = nprogram;

              //int hb, lb, pr;
              //if (program == CTRL_VAL_UNKNOWN) {
              //      hb = lb = pr = 0;
              //      midiTrackInfo->iPatch->setText("---");
              //      }
              //else 
              //{
                    MidiInstrument* instr = mp->instrument();
                    const char* name = instr->getPatchName(outChannel, program, song->mtype(), track->type() == Track::DRUM);
                    midiTrackInfo->iPatch->setText(QString(name));

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
              midiTrackInfo->iHBank->blockSignals(true);
              midiTrackInfo->iLBank->blockSignals(true);
              midiTrackInfo->iProgram->blockSignals(true);

              midiTrackInfo->iHBank->setValue(hb);
              midiTrackInfo->iLBank->setValue(lb);
              midiTrackInfo->iProgram->setValue(pr);

              midiTrackInfo->iHBank->blockSignals(false);
              midiTrackInfo->iLBank->blockSignals(false);
              midiTrackInfo->iProgram->blockSignals(false);
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
        midiTrackInfo->iLautst->blockSignals(true);
        midiTrackInfo->iLautst->setRange(mn - 1, mc->maxVal());
        midiTrackInfo->iLautst->setValue(v);
        midiTrackInfo->iLautst->blockSignals(false);
        
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
        midiTrackInfo->iPan->blockSignals(true);
        midiTrackInfo->iPan->setRange(mn - 1, mc->maxVal());
        midiTrackInfo->iPan->setValue(v);
        midiTrackInfo->iPan->blockSignals(false);
      //}
      
      
      // Does it include a midi controller value adjustment? Then handle it...
      //if(flags & SC_MIDI_CONTROLLER)
      //  seek();

      // Is it simply a midi controller value adjustment? Forget it.
      //if(flags != SC_MIDI_CONTROLLER)
      //{
        midiTrackInfo->iTransp->setValue(track->transposition);
        midiTrackInfo->iAnschl->setValue(track->velocity);
        midiTrackInfo->iVerz->setValue(track->delay);
        midiTrackInfo->iLen->setValue(track->len);
        midiTrackInfo->iKompr->setValue(track->compression);
      //}
}

/*
//---------------------------------------------------------
//   seek
//    change values akkording to seek position
//---------------------------------------------------------

void Arranger::seek()
      {
      if (!showTrackinfoFlag || !selected)
            return;
      switch(selected->type()) {
            case Track::MIDI:
            case Track::DRUM:
                  {
                    MidiTrack* track = (MidiTrack*)selected;
                    int outPort      = track->outPort();
                    int outChannel   = track->outChannel();
                    MidiPort* mp = &midiPorts[outPort];
  
                    // int nprogram = mp->getCtrl(outChannel, tick, CTRL_PROGRAM);
                    int nprogram = mp->hwCtrlState(outChannel, CTRL_PROGRAM);
                    if(nprogram == CTRL_VAL_UNKNOWN)
                    {
                      midiTrackInfo->iHBank->blockSignals(true);
                      midiTrackInfo->iLBank->blockSignals(true);
                      midiTrackInfo->iProgram->blockSignals(true);
                      midiTrackInfo->iHBank->setValue(0);
                      midiTrackInfo->iLBank->setValue(0);
                      midiTrackInfo->iProgram->setValue(0);
                      midiTrackInfo->iHBank->blockSignals(false);
                      midiTrackInfo->iLBank->blockSignals(false);
                      midiTrackInfo->iProgram->blockSignals(false);
                      
                      program = CTRL_VAL_UNKNOWN;
                      nprogram = mp->lastValidHWCtrlState(outChannel, CTRL_PROGRAM);
                      if(nprogram == CTRL_VAL_UNKNOWN) 
                        midiTrackInfo->iPatch->setText(QString("<unknown>"));
                      else
                      {
                        MidiInstrument* instr = mp->instrument();
                        const char* name = instr->getPatchName(outChannel, nprogram, song->mtype(), track->type() == Track::DRUM);
                        midiTrackInfo->iPatch->setText(QString(name));
                      }         
                    }
                    else
                    if (program != nprogram) {
                          program = nprogram;
  
                          //int hb, lb, pr;
                          //if (program == CTRL_VAL_UNKNOWN) {
                          //      hb = lb = pr = 0;
                          //      midiTrackInfo->iPatch->setText("---");
                          //      }
                          //else 
                          //{
                                MidiInstrument* instr = mp->instrument();
                                const char* name = instr->getPatchName(outChannel, program, song->mtype(), track->type() == Track::DRUM);
                                midiTrackInfo->iPatch->setText(QString(name));
  
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
                          midiTrackInfo->iHBank->blockSignals(true);
                          midiTrackInfo->iLBank->blockSignals(true);
                          midiTrackInfo->iProgram->blockSignals(true);
  
                          midiTrackInfo->iHBank->setValue(hb);
                          midiTrackInfo->iLBank->setValue(lb);
                          midiTrackInfo->iProgram->setValue(pr);
  
                          midiTrackInfo->iHBank->blockSignals(false);
                          midiTrackInfo->iLBank->blockSignals(false);
                          midiTrackInfo->iProgram->blockSignals(false);
                          }
  
                    //int nvolume = midiPorts[outPort].getCtrl(outChannel, tick, CTRL_VOLUME);
                    // int npan = midiPorts[outPort].getCtrl(outChannel, tick, CTRL_PANPOT);
                    MidiController* mc = mp->midiController(CTRL_VOLUME);
                    int v = mp->hwCtrlState(outChannel, CTRL_VOLUME);
                    if(v != volume) 
                    {
                      volume = v;
                      if(v == CTRL_VAL_UNKNOWN)
                        v = mc->minVal() - 1;
                      else
                        v -= mc->bias();
                      midiTrackInfo->iLautst->blockSignals(true);
                      midiTrackInfo->iLautst->setValue(v);
                      midiTrackInfo->iLautst->blockSignals(false);
                    }  
                    mc = mp->midiController(CTRL_PANPOT);
                    v = mp->hwCtrlState(outChannel, CTRL_PANPOT);
                    if(v != pan) 
                    {
                      pan = v;
                      if(v == CTRL_VAL_UNKNOWN)
                      //{
                        //v = mc->initVal();
                        //if(v == CTRL_VAL_UNKNOWN)
                        //  v = 0;
                        v = mc->minVal() - 1;
                      //}  
                      else
                        // Auto bias...
                        v -= mc->bias();
                      midiTrackInfo->iPan->blockSignals(true);
                      midiTrackInfo->iPan->setValue(v);
                      midiTrackInfo->iPan->blockSignals(false);
                    }
                
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
*/
