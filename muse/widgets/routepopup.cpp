//=========================================================
//  MusE
//  Linux Music Editor
//
//  RoutePopupMenu.cpp 
//  (C) Copyright 2011 Tim E. Real (terminator356 A T sourceforge D O T net)
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

#include "app.h"
#include "routepopup.h"
#include "midiport.h"
#include "mididev.h"
#include "audio.h"
#include "driver/audiodev.h"
#include "song.h"
#include "track.h"
#include "synth.h"
#include "route.h"
#include "icons.h"
#include "menutitleitem.h"
#include "popupmenu.h"

//---------------------------------------------------------
//   addMenuItem
//---------------------------------------------------------

int RoutePopupMenu::addMenuItem(AudioTrack* track, Track* route_track, PopupMenu* lb, int id, int channel, int channels, bool isOutput)
{
  // totalInChannels is only used by syntis.
  int toch = ((AudioTrack*)track)->totalOutChannels();
  // If track channels = 1, it must be a mono synth. And synti channels cannot be changed by user.
  if(track->channels() == 1)
    toch = 1;
  
  // Don't add the last stray mono route if the track is stereo.
  //if(route_track->channels() > 1 && (channel+1 == chans))
  //  return id;
    
  RouteList* rl = isOutput ? track->outRoutes() : track->inRoutes();
  
  QAction* act;
  
  QString s(route_track->name());
  
  act = lb->addAction(s);
  act->setCheckable(true);
  
  int ach = channel;
  int bch = -1;
  
  Route r(route_track, isOutput ? ach : bch, channels);
  
  r.remoteChannel = isOutput ? bch : ach;
  
  act->setData(qVariantFromValue(r));   
  
  for(ciRoute ir = rl->begin(); ir != rl->end(); ++ir) 
  {
    if(ir->type == Route::TRACK_ROUTE && ir->track == route_track && ir->remoteChannel == r.remoteChannel)
    {
      int tcompch = r.channel;
      if(tcompch == -1)
        tcompch = 0;
      int tcompchs = r.channels;
      if(tcompchs == -1)
        tcompchs = isOutput ? track->channels() : route_track->channels();
      
      int compch = ir->channel;
      if(compch == -1)
        compch = 0;
      int compchs = ir->channels;
      if(compchs == -1)
        compchs = isOutput ? track->channels() : ir->track->channels();
      
      if(compch == tcompch && compchs == tcompchs) 
      {
        act->setChecked(true);
        break;
      }
    }  
  }
  return ++id;      
}

//---------------------------------------------------------
//   addAuxPorts
//---------------------------------------------------------

int RoutePopupMenu::addAuxPorts(AudioTrack* t, PopupMenu* lb, int id, int channel, int channels, bool isOutput)
      {
      AuxList* al = song->auxs();
      for (iAudioAux i = al->begin(); i != al->end(); ++i) {
            Track* track = *i;
            if (t == track)
                  continue;
            id = addMenuItem(t, track, lb, id, channel, channels, isOutput);
            }
      return id;      
      }

//---------------------------------------------------------
//   addInPorts
//---------------------------------------------------------

int RoutePopupMenu::addInPorts(AudioTrack* t, PopupMenu* lb, int id, int channel, int channels, bool isOutput)
      {
      InputList* al = song->inputs();
      for (iAudioInput i = al->begin(); i != al->end(); ++i) {
            Track* track = *i;
            if (t == track)
                  continue;
            id = addMenuItem(t, track, lb, id, channel, channels, isOutput);
            }
      return id;      
      }

//---------------------------------------------------------
//   addOutPorts
//---------------------------------------------------------

int RoutePopupMenu::addOutPorts(AudioTrack* t, PopupMenu* lb, int id, int channel, int channels, bool isOutput)
      {
      OutputList* al = song->outputs();
      for (iAudioOutput i = al->begin(); i != al->end(); ++i) {
            Track* track = *i;
            if (t == track)
                  continue;
            id = addMenuItem(t, track, lb, id, channel, channels, isOutput);
            }
      return id;      
      }

//---------------------------------------------------------
//   addGroupPorts
//---------------------------------------------------------

int RoutePopupMenu::addGroupPorts(AudioTrack* t, PopupMenu* lb, int id, int channel, int channels, bool isOutput)
      {
      GroupList* al = song->groups();
      for (iAudioGroup i = al->begin(); i != al->end(); ++i) {
            Track* track = *i;
            if (t == track)
                  continue;
            id = addMenuItem(t, track, lb, id, channel, channels, isOutput);
            }
      return id;      
      }

//---------------------------------------------------------
//   addWavePorts
//---------------------------------------------------------

int RoutePopupMenu::addWavePorts(AudioTrack* t, PopupMenu* lb, int id, int channel, int channels, bool isOutput)
      {
      WaveTrackList* al = song->waves();
      for (iWaveTrack i = al->begin(); i != al->end(); ++i) {
            Track* track = *i;
            if (t == track)
                  continue;
            id = addMenuItem(t, track, lb, id, channel, channels, isOutput);
            }
      return id;      
      }

//---------------------------------------------------------
//   addSyntiPorts
//---------------------------------------------------------

int RoutePopupMenu::addSyntiPorts(AudioTrack* t, PopupMenu* lb, int id, 
                         int channel, int channels, bool isOutput)
{
      RouteList* rl = isOutput ? t->outRoutes() : t->inRoutes();
      
      QAction* act;
      
      SynthIList* al = song->syntis();
      for (iSynthI i = al->begin(); i != al->end(); ++i) 
      {
            Track* track = *i;
            if (t == track)
                  continue;
            int toch = ((AudioTrack*)track)->totalOutChannels();
            // If track channels = 1, it must be a mono synth. And synti channels cannot be changed by user.
            if(track->channels() == 1)
              toch = 1;
            
            // totalInChannels is only used by syntis.
            int chans = (!isOutput || track->type() != Track::AUDIO_SOFTSYNTH) ? toch : ((AudioTrack*)track)->totalInChannels();
            
            int tchans = (channels != -1) ? channels: t->channels();
            if(tchans == 2)
            {
              // Ignore odd numbered left-over mono channel.
              //chans = chans & ~1;
              //if(chans != 0)
                chans -= 1;
            }
            
            if(chans > 0)
            {
              PopupMenu* chpup = new PopupMenu(lb, true);
              chpup->setTitle(track->name());
              for(int ch = 0; ch < chans; ++ch)
              {
                char buffer[128];
                if(tchans == 2)
                  snprintf(buffer, 128, "%s %d,%d", chpup->tr("Channel").toLatin1().constData(), ch+1, ch+2);
                else  
                  snprintf(buffer, 128, "%s %d", chpup->tr("Channel").toLatin1().constData(), ch+1);
                act = chpup->addAction(QString(buffer));
                act->setCheckable(true);
                
                int ach = (channel == -1) ? ch : channel;
                int bch = (channel == -1) ? -1 : ch;
                
                Route rt(track, (t->type() != Track::AUDIO_SOFTSYNTH || isOutput) ? ach : bch, tchans);
                rt.remoteChannel = (t->type() != Track::AUDIO_SOFTSYNTH || isOutput) ? bch : ach;
                
                act->setData(qVariantFromValue(rt));   
                
                for(ciRoute ir = rl->begin(); ir != rl->end(); ++ir) 
                {
                  if(ir->type == Route::TRACK_ROUTE && ir->track == track && ir->remoteChannel == rt.remoteChannel)
                  {
                    int tcompch = rt.channel;
                    if(tcompch == -1)
                      tcompch = 0;
                    int tcompchs = rt.channels;
                    if(tcompchs == -1)
                      tcompchs = isOutput ? t->channels() : track->channels();
                    
                    int compch = ir->channel;
                    if(compch == -1)
                      compch = 0;
                    int compchs = ir->channels;
                    if(compchs == -1)
                      compchs = isOutput ? t->channels() : ir->track->channels();
                    
                    if(compch == tcompch && compchs == tcompchs) 
                    {
                      act->setChecked(true);
                      break;
                    }
                  }
                }  
                ++id;
              }
            
              lb->addMenu(chpup);
            }
      }
      return id;      
}

//---------------------------------------------------------
//   addMultiChannelOutPorts
//---------------------------------------------------------

int RoutePopupMenu::addMultiChannelPorts(AudioTrack* t, PopupMenu* pup, int id, bool isOutput)
{
  int toch = t->totalOutChannels();
  // If track channels = 1, it must be a mono synth. And synti channels cannot be changed by user.
  if(t->channels() == 1)
    toch = 1;
  
  // Number of allocated buffers is always MAX_CHANNELS or more, even if _totalOutChannels is less. 
  // totalInChannels is only used by syntis.
  int chans = (isOutput || t->type() != Track::AUDIO_SOFTSYNTH) ? toch : t->totalInChannels();

  if(chans > 1)
    pup->addAction(new MenuTitleItem("<Mono>", pup)); 
  
  //
  // If it's more than one channel, create a sub-menu. If it's just one channel, don't bother with a sub-menu...
  //

  PopupMenu* chpup = pup;
  
  for(int ch = 0; ch < chans; ++ch)
  {
    // If more than one channel, create the sub-menu.
    if(chans > 1)
      chpup = new PopupMenu(pup, true);
    
    if(isOutput)
    {
      switch(t->type()) 
      {
        
        case Track::AUDIO_INPUT:
        case Track::WAVE:
        case Track::AUDIO_GROUP:
        case Track::AUDIO_SOFTSYNTH:
        case Track::AUDIO_AUX:        
              id = addWavePorts(t, chpup, id, ch, 1, isOutput);
              id = addOutPorts(t, chpup, id, ch, 1, isOutput);
              id = addGroupPorts(t, chpup, id, ch, 1, isOutput);
              id = addSyntiPorts(t, chpup, id, ch, 1, isOutput);
              break;
        default:
              break;
      }
    }
    else
    {
      switch(t->type()) 
      {
        
        case Track::AUDIO_OUTPUT:
              id = addWavePorts(t, chpup, id, ch, 1, isOutput);
              id = addInPorts(t, chpup, id, ch, 1, isOutput);
              id = addGroupPorts(t, chpup, id, ch, 1, isOutput);
              id = addAuxPorts(t, chpup, id, ch, 1, isOutput);
              id = addSyntiPorts(t, chpup, id, ch, 1, isOutput);
              break;
        case Track::WAVE:
        case Track::AUDIO_SOFTSYNTH:
        case Track::AUDIO_GROUP:
              id = addWavePorts(t, chpup, id, ch, 1, isOutput);
              id = addInPorts(t, chpup, id, ch, 1, isOutput);
              id = addGroupPorts(t, chpup, id, ch, 1, isOutput);
              id = addAuxPorts(t, chpup, id, ch, 1, isOutput);     
              id = addSyntiPorts(t, chpup, id, ch, 1, isOutput);
              break;
        default:
              break;
      }
    }
      
    // If more than one channel, add the created sub-menu.
    if(chans > 1)
    {
      char buffer[128];
      snprintf(buffer, 128, "%s %d", pup->tr("Channel").toLatin1().constData(), ch+1);
      chpup->setTitle(QString(buffer));
      pup->addMenu(chpup);
    }  
  } 
       
  // For stereo listing, ignore odd numbered left-over channels.
  chans -= 1;
  if(chans > 0)
  {
    // Ignore odd numbered left-over channels.
    //int schans = (chans & ~1) - 1;
    
    pup->addSeparator();
    pup->addAction(new MenuTitleItem("<Stereo>", pup));
  
    //
    // If it's more than two channels, create a sub-menu. If it's just two channels, don't bother with a sub-menu...
    //
    
    chpup = pup;
    if(chans <= 2)
      // Just do one iteration.
      chans = 1;
    
    for(int ch = 0; ch < chans; ++ch)
    {
      // If more than two channels, create the sub-menu.
      if(chans > 2)
        chpup = new PopupMenu(pup, true);
      
      if(isOutput)
      {
        switch(t->type()) 
        {
          case Track::AUDIO_INPUT:
          case Track::WAVE:
          case Track::AUDIO_GROUP:
          case Track::AUDIO_SOFTSYNTH:
          case Track::AUDIO_AUX:                                          
                id = addWavePorts(t, chpup, id, ch, 2, isOutput);     
                id = addOutPorts(t, chpup, id, ch, 2, isOutput);
                id = addGroupPorts(t, chpup, id, ch, 2, isOutput);
                id = addSyntiPorts(t, chpup, id, ch, 2, isOutput);
                break;
          default:
                break;
        }
      }    
      else
      {
        switch(t->type()) 
        {
          case Track::AUDIO_OUTPUT:
                id = addWavePorts(t, chpup, id, ch, 2, isOutput);
                id = addInPorts(t, chpup, id, ch, 2, isOutput);
                id = addGroupPorts(t, chpup, id, ch, 2, isOutput);
                id = addAuxPorts(t, chpup, id, ch, 2, isOutput);
                id = addSyntiPorts(t, chpup, id, ch, 2, isOutput);
                break;
          case Track::WAVE:
          case Track::AUDIO_SOFTSYNTH:
          case Track::AUDIO_GROUP:
                id = addWavePorts(t, chpup, id, ch, 2, isOutput);
                id = addInPorts(t, chpup, id, ch, 2, isOutput);
                id = addGroupPorts(t, chpup, id, ch, 2, isOutput);
                id = addAuxPorts(t, chpup, id, ch, 2, isOutput);     
                id = addSyntiPorts(t, chpup, id, ch, 2, isOutput);
                break;
          default:
                break;
        }
      }
      
      // If more than two channels, add the created sub-menu.
      if(chans > 2)
      {
        char buffer[128];
        snprintf(buffer, 128, "%s %d,%d", pup->tr("Channel").toLatin1().constData(), ch+1, ch+2);
        chpup->setTitle(QString(buffer));
        pup->addMenu(chpup);
      }  
    } 
  }
  
  return id;
}

//---------------------------------------------------------
//   nonSyntiTrackAddSyntis
//---------------------------------------------------------

int RoutePopupMenu::nonSyntiTrackAddSyntis(AudioTrack* t, PopupMenu* lb, int id, bool isOutput)
{
      RouteList* rl = isOutput ? t->outRoutes() : t->inRoutes();
      
      QAction* act;
      SynthIList* al = song->syntis();
      for (iSynthI i = al->begin(); i != al->end(); ++i) 
      {
            Track* track = *i;
            if (t == track)
                  continue;
            
            int toch = ((AudioTrack*)track)->totalOutChannels();
            // If track channels = 1, it must be a mono synth. And synti channels cannot be changed by user.
            if(track->channels() == 1)
              toch = 1;
            
            // totalInChannels is only used by syntis.
            int chans = (!isOutput || track->type() != Track::AUDIO_SOFTSYNTH) ? toch : ((AudioTrack*)track)->totalInChannels();
            
            //int schans = synti->channels();
            //if(schans < chans)
            //  chans = schans;
//            int tchans = (channels != -1) ? channels: t->channels();
//            if(tchans == 2)
//            {
              // Ignore odd numbered left-over mono channel.
              //chans = chans & ~1;
              //if(chans != 0)
//                chans -= 1;
//            }
            //int tchans = (channels != -1) ? channels: t->channels();
            
            if(chans > 0)
            {
              PopupMenu* chpup = new PopupMenu(lb, true);
              chpup->setTitle(track->name());
              if(chans > 1)
                chpup->addAction(new MenuTitleItem("<Mono>", chpup));
              
              for(int ch = 0; ch < chans; ++ch)
              {
                char buffer[128];
                snprintf(buffer, 128, "%s %d", chpup->tr("Channel").toLatin1().constData(), ch+1);
                act = chpup->addAction(QString(buffer));
                act->setCheckable(true);
                
                int ach = ch;
                int bch = -1;
                
                Route rt(track, isOutput ? bch : ach, 1);
                
                rt.remoteChannel = isOutput ? ach : bch;
                
                act->setData(qVariantFromValue(rt));   
                
                for(ciRoute ir = rl->begin(); ir != rl->end(); ++ir) 
                {
                  if(ir->type == Route::TRACK_ROUTE && ir->track == track && ir->remoteChannel == rt.remoteChannel)
                  {
                    int tcompch = rt.channel;
                    if(tcompch == -1)
                      tcompch = 0;
                    int tcompchs = rt.channels;
                    if(tcompchs == -1)
                      tcompchs = isOutput ? t->channels() : track->channels();
                    
                    int compch = ir->channel;
                    if(compch == -1)
                      compch = 0;
                    int compchs = ir->channels;
                    if(compchs == -1)
                      compchs = isOutput ? t->channels() : ir->track->channels();
                    
                    if(compch == tcompch && compchs == tcompchs) 
                    {
                      act->setChecked(true);
                      break;
                    }
                  }
                }
                ++id;
              }
            
              chans -= 1;
              if(chans > 0)
              {
                // Ignore odd numbered left-over channels.
                //int schans = (chans & ~1) - 1;
                
                chpup->addSeparator();
                chpup->addAction(new MenuTitleItem("<Stereo>", chpup)); 
              
                for(int ch = 0; ch < chans; ++ch)
                {
                  char buffer[128];
                  snprintf(buffer, 128, "%s %d,%d", chpup->tr("Channel").toLatin1().constData(), ch+1, ch+2);
                  act = chpup->addAction(QString(buffer));
                  act->setCheckable(true);
                  
                  int ach = ch;
                  int bch = -1;
                  
                  Route rt(track, isOutput ? bch : ach, 2);
                  
                  rt.remoteChannel = isOutput ? ach : bch;
                  
                  act->setData(qVariantFromValue(rt));   
                  
                  for(ciRoute ir = rl->begin(); ir != rl->end(); ++ir) 
                  {
                    if(ir->type == Route::TRACK_ROUTE && ir->track == track && ir->remoteChannel == rt.remoteChannel)
                    {
                      int tcompch = rt.channel;
                      if(tcompch == -1)
                        tcompch = 0;
                      int tcompchs = rt.channels;
                      if(tcompchs == -1)
                        tcompchs = isOutput ? t->channels() : track->channels();
                      
                      int compch = ir->channel;
                      if(compch == -1)
                        compch = 0;
                      int compchs = ir->channels;
                      if(compchs == -1)
                        compchs = isOutput ? t->channels() : ir->track->channels();
                      
                      if(compch == tcompch && compchs == tcompchs) 
                      {
                        act->setChecked(true);
                        break;
                      }
                    }  
                  }
                  ++id;
                }
              }
              
              lb->addMenu(chpup);
            }
      }
      return id;      
}

//---------------------------------------------------------
//   addMidiPorts
//---------------------------------------------------------

int RoutePopupMenu::addMidiPorts(AudioTrack* t, PopupMenu* pup, int id, bool isOutput)
{
  QAction* act;
  for(int i = 0; i < MIDI_PORTS; ++i)
  {
    MidiPort* mp = &midiPorts[i];
    MidiDevice* md = mp->device();
    
    // This is desirable, but could lead to 'hidden' routes unless we add more support
    //  such as removing the existing routes when user changes flags.
    // So for now, just list all valid ports whether read or write.
    if(!md)
      continue;
    //if(!(md->rwFlags() & (isOutput ? 1 : 2)))
    //  continue;
          
    // Do not list synth devices!
    if(md->isSynti())
      continue;
          
   RouteList* rl = isOutput ? t->outRoutes() : t->inRoutes();
    
    PopupMenu* subp = new PopupMenu(pup, true);
    subp->setTitle(md->name()); 
    
    int chanmask = 0;
    // To reduce number of routes required, from one per channel to just one containing a channel mask. 
    // Look for the first route to this midi port. There should always be only a single route for each midi port, now.
    for(ciRoute ir = rl->begin(); ir != rl->end(); ++ir)   
    {
      if(ir->type == Route::MIDI_PORT_ROUTE && ir->midiPort == i) 
      {
        // We have a route to the midi port. Grab the channel mask.
        chanmask = ir->channel;
        break;
      }
    }
    
    for(int ch = 0; ch < MIDI_CHANNELS; ++ch) 
    {
      act = subp->addAction(QString("Channel %1").arg(ch+1));
      act->setCheckable(true);
      
      int chbit = 1 << ch;
      Route srcRoute(i, chbit);    // In accordance with channel mask, use the bit position.
      
      act->setData(qVariantFromValue(srcRoute));   
      
      if(chanmask & chbit)                  // Is the channel already set? Show item check mark.
        act->setChecked(true);
      
      ++id;
    }
    
    //gid = MIDI_PORTS * MIDI_CHANNELS + i;           // Make sure each 'toggle' item gets a unique id.
    act = subp->addAction(QString("Toggle all"));
    //act->setCheckable(true);
    Route togRoute(i, (1 << MIDI_CHANNELS) - 1);    // Set all channel bits.
    act->setData(qVariantFromValue(togRoute));   
    ++id;
    
    pup->addMenu(subp);
  }    
  return id;      
}


//======================
// RoutePopupMenu
//======================

RoutePopupMenu::RoutePopupMenu(QWidget* parent, Track* track, bool isOutput) 
               : _track(track), _isOutMenu(isOutput)
{
  _pup = new PopupMenu(parent, true);
  init();
}

RoutePopupMenu::RoutePopupMenu(const QString& title, QWidget* parent, Track* track, bool isOutput)
               : _track(track), _isOutMenu(isOutput)
{
  _pup = new PopupMenu(title, parent, true);
  init();        
}

RoutePopupMenu::~RoutePopupMenu()
{
  //printf("RoutePopupMenu::~RoutePopupMenu\n");  
  // Make sure to clear which clears and deletes any sub popups.
  _pup->clear();
  delete _pup;
}

void RoutePopupMenu::init()
{
  connect(song, SIGNAL(songChanged(int)), SLOT(songChanged(int)));
}

void RoutePopupMenu::songChanged(int val)
{
  if(val & (SC_ROUTE | SC_CHANNELS | SC_CONFIG))     
    updateRouteMenus();             
}

void RoutePopupMenu::updateRouteMenus()    
{
  // NOTE: The purpose of this routine is to make sure the items actually reflect
  //  the routing status. 
  // In case for some reason a route could not be added (or removed). 
  // Then the item will be properly un-checked (or checked) here.
  
  //printf("RoutePopupMenu::updateRouteMenus\n");  
  
  if(!_track || !_pup || _pup->actions().isEmpty() || !_pup->isVisible())  
    return;
    
  RouteList* rl = _isOutMenu ? _track->outRoutes() : _track->inRoutes();

  // Clear all the action check marks.
  _pup->clearAllChecks();    
    
  // Take care of Midi Port to Audio Input routes first...  
  if(_isOutMenu && _track->isMidiTrack())
  {
    int port = ((MidiTrack*)_track)->outPort();
    if(port >= 0 && port < MIDI_PORTS)
    {
      MidiPort* mp = &midiPorts[port];
      RouteList* mprl = mp->outRoutes();
      for (ciRoute ir = mprl->begin(); ir != mprl->end(); ++ir) 
      {
        if(ir->type == Route::TRACK_ROUTE && ir->track && ir->track->type() == Track::AUDIO_INPUT)
        {
          for(int ch = 0; ch < MIDI_CHANNELS; ++ch)
          {
            int chbits = 1 << ch;
            if(ir->channel & chbits)
            {
              Route r(ir->track, chbits);
              //printf("RoutePopupMenu::updateRouteMenus MidiPort to AudioInput chbits:%d\n", chbits);  
              QAction* act = _pup->findActionFromData(qVariantFromValue(r));  
              if(act)
                act->setChecked(true);
            }  
          }  
        }  
      }  
    }
  }

  // Now check the ones that are found in the route list.
  for(ciRoute irl = rl->begin(); irl != rl->end(); ++irl) 
  {
    // Do MidiTrack to MidiPort routes...
    if(irl->type == Route::MIDI_PORT_ROUTE)
    {
      //printf("RoutePopupMenu::updateRouteMenus MIDI_PORT_ROUTE\n");  
      for(int ch = 0; ch < MIDI_CHANNELS; ++ch)
      {
        int chbits = 1 << ch;
        if(irl->channel & chbits)
        {
          Route r(irl->midiPort, chbits);
          QAction* act = _pup->findActionFromData(qVariantFromValue(r));  
          if(act)
            act->setChecked(true);
        }    
      }    
    }
    else
    // Do all other routes...
    {
      //printf("RoutePopupMenu::updateRouteMenus other irl type:%d\n", irl->type);  
      QAction* act = _pup->findActionFromData(qVariantFromValue(*irl));  
      if(act)
        act->setChecked(true);
    }
  }
}      

void RoutePopupMenu::popupActivated(QAction* action)
{
      if(!action || !_track || !_pup || _pup->actions().isEmpty())
        return;
        
      if(_track->isMidiTrack())
      {
        RouteList* rl = _isOutMenu ? _track->outRoutes() : _track->inRoutes();
        
        // Take care of Route data items first...
        if(qVariantCanConvert<Route>(action->data()))    
        {
          Route aRoute = action->data().value<Route>();
          
          // Support Midi Port to Audio Input track routes. 
          if(aRoute.type == Route::TRACK_ROUTE && aRoute.track && aRoute.track->type() == Track::AUDIO_INPUT)
          {
            //if(gIsOutRoutingPopupMenu)    // Try to avoid splitting like this. 
            {
              int chbit = aRoute.channel;
              int port = ((MidiTrack*)_track)->outPort();
              if(port < 0 || port >= MIDI_PORTS)
                return;
              
              MidiPort* mp = &midiPorts[port];
              //MidiDevice* md = mp->device();
              
              // This is desirable, but could lead to 'hidden' routes unless we add more support
              //  such as removing the existing routes when user changes flags.
              // So for now, just list all valid ports whether read or write.
              //if(!md)
              //  return;
              //if(!(md->rwFlags() & (gIsOutRoutingPopupMenu ? 1 : 2)))
              //  return;
              
              Route bRoute(port, chbit);
              
              int chmask = 0;                   
              RouteList* mprl = _isOutMenu ? mp->outRoutes() : mp->inRoutes();
              ciRoute ir = mprl->begin();
              for (; ir != mprl->end(); ++ir) 
              {
                if(ir->type == Route::TRACK_ROUTE && ir->track == aRoute.track)    // Is there already a route to this port?
                {
                      chmask = ir->channel;  // Grab the channel mask.
                      break;
                }      
              }
              if ((chmask & chbit) == chbit)             // Is the channel's bit(s) set?
              {
                // disconnect
                if(_isOutMenu)
                  audio->msgRemoveRoute(bRoute, aRoute);
                else
                  audio->msgRemoveRoute(aRoute, bRoute);
              }
              else 
              {
                // connect
                if(_isOutMenu)
                  audio->msgAddRoute(bRoute, aRoute);
                else
                  audio->msgAddRoute(aRoute, bRoute);
              }
              
              audio->msgUpdateSoloStates();
              song->update(SC_ROUTE);
              
            }
            return;
          }
          else if(aRoute.type == Route::MIDI_PORT_ROUTE)
          {
            int chbit = aRoute.channel;
            Route bRoute(_track, chbit);
            int mdidx = aRoute.midiPort;
    
            MidiPort* mp = &midiPorts[mdidx];
            MidiDevice* md = mp->device();
            //if(!md)    // Rem. Allow connections to ports with no device.
            //  return;
            
            //if(!(md->rwFlags() & 2))
            //if(!(md->rwFlags() & (gIsOutRoutingPopupMenu ? 1 : 2)))
            if(md && !(md->rwFlags() & (_isOutMenu ? 1 : 2)))   
                return;
            
            int chmask = 0;                   
            ciRoute iir = rl->begin();
            for (; iir != rl->end(); ++iir) 
            {
              if(iir->type == Route::MIDI_PORT_ROUTE && iir->midiPort == mdidx)    // Is there already a route to this port?
              {
                    chmask = iir->channel;  // Grab the channel mask.
                    break;
              }      
            }
            if ((chmask & chbit) == chbit)             // Is the channel's bit(s) set?
            {
              // disconnect
              if(_isOutMenu)
                audio->msgRemoveRoute(bRoute, aRoute);
              else
                audio->msgRemoveRoute(aRoute, bRoute);
            }
            else 
            {
              // connect
              if(_isOutMenu)
                audio->msgAddRoute(bRoute, aRoute);
              else
                audio->msgAddRoute(aRoute, bRoute);
            }
            
            audio->msgUpdateSoloStates();
            song->update(SC_ROUTE);
          }  
        }  
        else
        // ... now take care of integer data items.
        if(qVariantCanConvert<int>(action->data()))    
        {
          int n = action->data().value<int>();
          if(!_isOutMenu && n == 0)
            muse->configMidiPorts();
          return;  
        }
      }
      else
      {
        AudioTrack* t = (AudioTrack*)_track;
        RouteList* rl = _isOutMenu ? t->outRoutes() : t->inRoutes();
        
        if(!qVariantCanConvert<Route>(action->data()))    
          return;  
          
        if(_isOutMenu)
        {  
          Route dstRoute = action->data().value<Route>();             
          Route srcRoute(t, dstRoute.channel, dstRoute.channels);    
          srcRoute.remoteChannel = dstRoute.remoteChannel;
  
          // check if route src->dst exists:
          ciRoute irl = rl->begin();
          for (; irl != rl->end(); ++irl) {
                if (*irl == dstRoute)
                      break;
                }
          if (irl != rl->end()) {
                // disconnect if route exists
                audio->msgRemoveRoute(srcRoute, dstRoute);
                }
          else {
                // connect if route does not exist
                audio->msgAddRoute(srcRoute, dstRoute);
                }
          audio->msgUpdateSoloStates();
          song->update(SC_ROUTE);
        }  
        else
        {
          Route srcRoute = action->data().value<Route>();             
          
          // Support Midi Port to Audio Input routes. 
          if(_track->type() == Track::AUDIO_INPUT && srcRoute.type == Route::MIDI_PORT_ROUTE)
          {
            int chbit = srcRoute.channel;
            Route dstRoute(t, chbit);
            int mdidx = srcRoute.midiPort;
            int chmask = 0;                   
            ciRoute iir = rl->begin();
            for (; iir != rl->end(); ++iir) 
            {
              if(iir->type == Route::MIDI_PORT_ROUTE && iir->midiPort == mdidx)    // Is there already a route to this port?
              {
                chmask = iir->channel;  // Grab the channel mask.
                break;
              }      
            }
            
            if ((chmask & chbit) == chbit)             // Is the channel's bit(s) set?
            {
              //printf("routingPopupMenuActivated: removing src route ch:%d dst route ch:%d\n", srcRoute.channel, dstRoute.channel); 
              audio->msgRemoveRoute(srcRoute, dstRoute);
            }
            else 
            {
              //printf("routingPopupMenuActivated: adding src route ch:%d dst route ch:%d\n", srcRoute.channel, dstRoute.channel); 
              audio->msgAddRoute(srcRoute, dstRoute);
            }
            
            audio->msgUpdateSoloStates();
            song->update(SC_ROUTE);
            return;
          }
          
          Route dstRoute(t, srcRoute.channel, srcRoute.channels);     
          dstRoute.remoteChannel = srcRoute.remoteChannel;
  
          ciRoute irl = rl->begin();
          for (; irl != rl->end(); ++irl) {
                if (*irl == srcRoute)
                      break;
                }
          if (irl != rl->end()) {
                // disconnect
                audio->msgRemoveRoute(srcRoute, dstRoute);
                }
          else {
                // connect
                audio->msgAddRoute(srcRoute, dstRoute);
                }
          audio->msgUpdateSoloStates();
          song->update(SC_ROUTE);
        }
        
       
      }
      //else
      //{
      //}
}

void RoutePopupMenu::prepare()
{
  _pup->disconnect();
  _pup->clear();
   
  if(!_track)
    return;
   
  connect(_pup, SIGNAL(triggered(QAction*)), SLOT(popupActivated(QAction*)));
  
  if(_track->isMidiTrack())
  {
    RouteList* rl = _isOutMenu ? _track->outRoutes() : _track->inRoutes();
    
    int gid = 0;
    QAction* act = 0;
    
    if(_isOutMenu)   
    {
      // Support Midi Port to Audio Input track routes. 
      int port = ((MidiTrack*)_track)->outPort();
      if(port >= 0 && port < MIDI_PORTS)
      {
        MidiPort* mp = &midiPorts[port];
        
        // Do not list synth devices! Requiring valid device is desirable, 
        //  but could lead to 'hidden' routes unless we add more support
        //  such as removing the existing routes when user changes flags.
        // So for now, just list all valid ports whether read or write.
        if(mp->device() && !mp->device()->isSynti())  
        {
          RouteList* mprl = mp->outRoutes();
          int chbits = 1 << ((MidiTrack*)_track)->outChannel();
          //MidiDevice* md = mp->device();
          //if(!md)
          //  continue;
          
          _pup->addSeparator();
          _pup->addAction(new MenuTitleItem(tr("Soloing chain"), _pup)); 
          PopupMenu* subp = new PopupMenu(_pup, true);
          subp->setTitle(tr("Audio returns")); 
          _pup->addMenu(subp);
          
          InputList* al = song->inputs();
          for (ciAudioInput i = al->begin(); i != al->end(); ++i) 
          {
            Track* t = *i;
            QString s(t->name());
            act = subp->addAction(s);
            act->setCheckable(true);
            Route r(t, chbits);
            act->setData(qVariantFromValue(r));   
            for(ciRoute ir = mprl->begin(); ir != mprl->end(); ++ir) 
            {
              if(ir->type == Route::TRACK_ROUTE && ir->track == t && (ir->channel & chbits))
              {
                act->setChecked(true);
                break;
              }  
            }
            ++gid;      
          }
        }     
      }  
    }
    else
    {
      // Warn if no devices available. Add an item to open midi config. 
      int pi = 0;
      for( ; pi < MIDI_PORTS; ++pi)
      {
        MidiDevice* md = midiPorts[pi].device();
        //if(md && !md->isSynti() && (md->rwFlags() & 2))
        //if(md && (md->rwFlags() & 2))   // p4.0.27
        if(md && (md->rwFlags() & 2 || md->isSynti()) )  // p4.0.27
          break;
      }
      if(pi == MIDI_PORTS)
      {
        act = _pup->addAction(tr("Warning: No midi input devices!"));
        act->setCheckable(false);
        act->setData(-1);
        _pup->addSeparator();
      }
      act = _pup->addAction(QIcon(*settings_midiport_softsynthsIcon), tr("Open midi config..."));
      act->setCheckable(false);
      act->setData(gid);
      _pup->addSeparator();
      ++gid;
      
      _pup->addAction(new MenuTitleItem("Midi input ports", _pup)); 
      
      for(int i = 0; i < MIDI_PORTS; ++i)
      {
        // NOTE: Could possibly list all devices, bypassing ports, but no, let's stick with ports.
        MidiPort* mp = &midiPorts[i];
        MidiDevice* md = mp->device();
        //if(!md)
        //  continue;
        
        // Do not list synth devices!
        ///if(md && md->isSynti())
        ///  continue;
        ///if(md && !(md->rwFlags() & 2))
        ///  continue;
        // p4.0.27 Go ahead. Synths esp MESS send out stuff. 
        if( md && !(md->rwFlags() & 2) && !md->isSynti() )
          continue;
          
        //printf("MusE::prepareRoutingPopupMenu adding submenu portnum:%d\n", i);
        
        int chanmask = 0;
        // To reduce number of routes required, from one per channel to just one containing a channel mask. 
        // Look for the first route to this midi port. There should always be only a single route for each midi port, now.
        ciRoute ir = rl->begin();
        for( ; ir != rl->end(); ++ir)   
        {
          if(ir->type == Route::MIDI_PORT_ROUTE && ir->midiPort == i) 
          {
            // We have a route to the midi port. Grab the channel mask.
            chanmask = ir->channel;
            break;
          }
        }
        // List ports with no device, but with routes to this track, in the main popup.
        if(!md && ir == rl->end())
          continue;
        
        PopupMenu* subp = new PopupMenu(_pup, true);
        subp->setTitle(QString("%1:").arg(i+1) + (md ? md->name() : tr("<none>"))); 
        
        for(int ch = 0; ch < MIDI_CHANNELS; ++ch) 
        {
          act = subp->addAction(QString("Channel %1").arg(ch+1));
          act->setCheckable(true);
          int chbit = 1 << ch;
          Route srcRoute(i, chbit);    // In accordance with channel mask, use the bit position.
          act->setData(qVariantFromValue(srcRoute));   
          if(chanmask & chbit)                  // Is the channel already set? Show item check mark.
            act->setChecked(true);
          ++gid;  
        }
        //gid = MIDI_PORTS * MIDI_CHANNELS + i;           // Make sure each 'toggle' item gets a unique id.
        act = subp->addAction(tr("Toggle all"));
        //act->setCheckable(true);
        Route togRoute(i, (1 << MIDI_CHANNELS) - 1);    // Set all channel bits.
        act->setData(qVariantFromValue(togRoute));   
        ++gid;
        _pup->addMenu(subp);
      }
      
      #if 0
      // p4.0.17 List ports with no device and no in routes, in a separate popup.
      PopupMenu* morep = new PopupMenu(pup, true);
      morep->setTitle(tr("More...")); 
      for(int i = 0; i < MIDI_PORTS; ++i)
      {
        MidiPort* mp = &midiPorts[i];
        if(mp->device())
          continue;
        
        PopupMenu* subp = new PopupMenu(morep, true);
        subp->setTitle(QString("%1:").arg(i) + tr("<none>")); 
        
        // MusE-2: Check this - needed with QMenu? Help says no. No - verified, it actually causes double triggers!
        //connect(subp, SIGNAL(triggered(QAction*)), pup, SIGNAL(triggered(QAction*)));
        //connect(subp, SIGNAL(aboutToHide()), pup, SIGNAL(aboutToHide()));
        
        iRoute ir = rl->begin();
        for( ; ir != rl->end(); ++ir)   
        {
          if(ir->type == Route::MIDI_PORT_ROUTE && ir->midiPort == i) 
            break;
        }
        if(ir != rl->end())
          continue;
        
        for(int ch = 0; ch < MIDI_CHANNELS; ++ch) 
        {
          act = subp->addAction(QString("Channel %1").arg(ch+1));
          act->setCheckable(true);
          act->setData(gid);
          
          int chbit = 1 << ch;
          Route srcRoute(i, chbit);    // In accordance with new channel mask, use the bit position.
          
          gRoutingMenuMap.insert( pRouteMenuMap(gid, srcRoute) );
          
          //if(chanmask & chbit)                  // Is the channel already set? Show item check mark.
          //  act->setChecked(true);
          
          ++gid;  
        }
        //gid = MIDI_PORTS * MIDI_CHANNELS + i;           // Make sure each 'toggle' item gets a unique id.
        act = subp->addAction(QString("Toggle all"));
        //act->setCheckable(true);
        act->setData(gid);
        Route togRoute(i, (1 << MIDI_CHANNELS) - 1);    // Set all channel bits.
        gRoutingMenuMap.insert( pRouteMenuMap(gid, togRoute) );
        ++gid;
        morep->addMenu(subp);
      }      
      pup->addMenu(morep);
      #endif
      
    }
    return;
  }
  else
  {
    AudioTrack* t = (AudioTrack*)_track;
    int channel = t->channels();
    if(_isOutMenu)   
    {
      RouteList* orl = t->outRoutes();

      QAction* act = 0;
      int gid = 0;
      gid = 0;
      
      switch(_track->type()) 
      {
        case Track::AUDIO_OUTPUT:
        {
          for(int i = 0; i < channel; ++i) 
          {
            char buffer[128];
            snprintf(buffer, 128, "%s %d", tr("Channel").toLatin1().constData(), i+1);
            MenuTitleItem* titel = new MenuTitleItem(QString(buffer), _pup);
            _pup->addAction(titel); 
  
            if(!checkAudioDevice())
            { 
              _pup->clear();
              return;
            }
            std::list<QString> ol = audioDevice->inputPorts();
            for(std::list<QString>::iterator ip = ol.begin(); ip != ol.end(); ++ip) 
            {
              act = _pup->addAction(*ip);
              act->setCheckable(true);
              
              Route dst(*ip, true, i, Route::JACK_ROUTE);
              act->setData(qVariantFromValue(dst));   
              ++gid;
              for(ciRoute ir = orl->begin(); ir != orl->end(); ++ir) 
              {
                if(*ir == dst) 
                {
                  act->setChecked(true);
                  break;
                }
              }
            }
            if(i+1 != channel)
              _pup->addSeparator();
          }      
          
          //
          // Display using separate menu for audio inputs:
          //
          _pup->addSeparator();
          _pup->addAction(new MenuTitleItem(tr("Soloing chain"), _pup)); 
          PopupMenu* subp = new PopupMenu(_pup, true);
          subp->setTitle(tr("Audio returns")); 
          _pup->addMenu(subp);
          gid = addInPorts(t, subp, gid, -1, -1, true);  
          //
          // Display all in the same menu:
          //
          //_pup->addSeparator();
          //MenuTitleItem* title = new MenuTitleItem(tr("Audio returns"), _pup);
          //_pup->addAction(title); 
          //gid = addInPorts(t, _pup, gid, -1, -1, true);  
        }
        break;
        case Track::AUDIO_SOFTSYNTH:
              gid = addMultiChannelPorts(t, _pup, gid, true);
        break;
        
        case Track::AUDIO_INPUT:
        case Track::WAVE:
        case Track::AUDIO_GROUP:
        case Track::AUDIO_AUX:
              gid = addWavePorts(        t, _pup, gid, -1, -1, true);  
              gid = addOutPorts(         t, _pup, gid, -1, -1, true);
              gid = addGroupPorts(       t, _pup, gid, -1, -1, true);
              gid = nonSyntiTrackAddSyntis(t, _pup, gid, true);
        break;
        default:
              _pup->clear();
              return;
      }
    }
    else
    {
      if(_track->type() == Track::AUDIO_AUX)
        return;
        
      RouteList* irl = t->inRoutes();
  
      QAction* act = 0;
      int gid = 0;
      gid = 0;
      
      switch(_track->type()) 
      {
        case Track::AUDIO_INPUT:
        {
          for(int i = 0; i < channel; ++i) 
          {
            char buffer[128];
            snprintf(buffer, 128, "%s %d", tr("Channel").toLatin1().constData(), i+1);
            MenuTitleItem* titel = new MenuTitleItem(QString(buffer), _pup);
            _pup->addAction(titel); 
  
            if(!checkAudioDevice())
            { 
              _pup->clear();
              return;
            }
            std::list<QString> ol = audioDevice->outputPorts();
            for(std::list<QString>::iterator ip = ol.begin(); ip != ol.end(); ++ip) 
            {
              act = _pup->addAction(*ip);
              act->setCheckable(true);
              
              Route dst(*ip, true, i, Route::JACK_ROUTE);
              act->setData(qVariantFromValue(dst));   
              ++gid;
              for(ciRoute ir = irl->begin(); ir != irl->end(); ++ir) 
              {
                if(*ir == dst) 
                {
                  act->setChecked(true);
                  break;
                }
              }
            }
            if(i+1 != channel)
              _pup->addSeparator();
          }
          
          //
          // Display using separate menus for midi ports and audio outputs:
          //
          _pup->addSeparator();
          _pup->addAction(new MenuTitleItem(tr("Soloing chain"), _pup)); 
          PopupMenu* subp = new PopupMenu(_pup, true);
          subp->setTitle(tr("Audio sends")); 
          _pup->addMenu(subp);
          gid = addOutPorts(t, subp, gid, -1, -1, false);  
          subp = new PopupMenu(_pup, true);
          subp->setTitle(tr("Midi port sends")); 
          _pup->addMenu(subp);
          addMidiPorts(t, subp, gid, false);
          //
          // Display all in the same menu:
          //
          //_pup->addAction(new MenuTitleItem(tr("Audio sends"), _pup)); 
          //gid = addOutPorts(t, _pup, gid, -1, -1, false);  
          //_pup->addSeparator();
          //_pup->addAction(new MenuTitleItem(tr("Midi sends"), _pup)); 
          //addMidiPorts(t, _pup, gid, false);
        }
        break;
        case Track::AUDIO_OUTPUT:
              gid = addWavePorts( t, _pup, gid, -1, -1, false);
              gid = addInPorts(   t, _pup, gid, -1, -1, false);
              gid = addGroupPorts(t, _pup, gid, -1, -1, false);
              gid = addAuxPorts(  t, _pup, gid, -1, -1, false);
              gid = nonSyntiTrackAddSyntis(t, _pup, gid, false);
              break;
        case Track::WAVE:
              gid = addWavePorts( t, _pup, gid, -1, -1, false);  
              gid = addInPorts(   t, _pup, gid, -1, -1, false);
              gid = addGroupPorts(t, _pup, gid, -1, -1, false);  
              gid = addAuxPorts(  t, _pup, gid, -1, -1, false);  
              gid = nonSyntiTrackAddSyntis(t, _pup, gid, false); 
              break;
        case Track::AUDIO_GROUP:
              gid = addWavePorts( t, _pup, gid, -1, -1, false);
              gid = addInPorts(   t, _pup, gid, -1, -1, false);
              gid = addGroupPorts(t, _pup, gid, -1, -1, false);
              gid = addAuxPorts(  t, _pup, gid, -1, -1, false);  
              gid = nonSyntiTrackAddSyntis(t, _pup, gid, false);
              break;
        
        case Track::AUDIO_SOFTSYNTH:
              gid = addMultiChannelPorts(t, _pup, gid, false);
              break;
        default:
              _pup->clear();
              return;
      }  
    }  
  }
}

void RoutePopupMenu::exec(Track* track, bool isOutput)
{
  if(track)
  {
    _track = track;
    _isOutMenu = isOutput;
  }  
  prepare();
  _pup->exec();
}

void RoutePopupMenu::exec(const QPoint& p, Track* track, bool isOutput)
{
  if(track)
  {
    _track = track;
    _isOutMenu = isOutput;
  }  
  prepare();
  _pup->exec(p);
}

void RoutePopupMenu::popup(const QPoint& p, Track* track, bool isOutput)
{
  if(track)
  {
    _track = track;
    _isOutMenu = isOutput;
  }  
  prepare();
  _pup->popup(p);
}

