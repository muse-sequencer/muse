//=========================================================
//  MusE
//  Linux Music Editor
//
//  RoutePopupMenu.cpp 
//  (C) Copyright 2011-2015 Tim E. Real (terminator356 A T sourceforge D O T net)
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
//=============================================================================

#include <QBitArray>
#include <QByteArray>
#include <qtextcodec.h>
#include <QActionGroup>
#include <QLayout>
#include <QApplication>
#include <QStyle>
#include <list>

#include "app.h"
#include "routepopup.h"
#include "gconfig.h"
#include "midiport.h"
#include "mididev.h"
#include "audio.h"
#include "driver/audiodev.h"
#include "song.h"
#include "synth.h"
#include "icons.h"
#include "menutitleitem.h"

#include "globaldefs.h"
#include "gconfig.h"
#include "globals.h"

// Forwards from header:
#include <QWidget>
#include <QAction>
#include <QPoint>
#include <QResizeEvent>
#include "track.h"
#include "operations.h"

// For debugging output: Uncomment the fprintf section.
#define DEBUG_PRST_ROUTES(dev, format, args...) // fprintf(dev, format, ##args);
#define DEBUG_PRST_ROUTES_2(dev, format, args...) // fprintf(dev, format, ##args);

#define _USE_CUSTOM_WIDGET_ACTIONS_ 

// REMOVE Tim. Persistent routes. Added. Make this permanent later if it works OK and makes good sense.
#define _USE_SIMPLIFIED_SOLO_CHAIN_

// REMOVE Tim. Persistent routes. Added. Make this permanent later if it works OK and makes good sense.
#define _USE_MIDI_ROUTE_PER_CHANNEL_

// Undefine if and when multiple output routes are added to midi tracks.
#define _USE_MIDI_TRACK_SINGLE_OUT_PORT_CHAN_

#define _SHOW_CANONICAL_NAMES_ 0x1000
#define _SHOW_FIRST_ALIASES_  0x1001
#define _SHOW_SECOND_ALIASES_ 0x1002

#define _ALIASES_WIDGET_ACTION_ 0x2000
#define _OPEN_MIDI_CONFIG_ 0x2001
#define _OPEN_ROUTING_DIALOG_ 0x2002
#define _GROUPING_CHANNELS_WIDGET_ACTION_ 0x2003

namespace MusEGui {


void RoutePopupMenu::addGroupingChannelsAction(PopupMenu* lb)
{
  RoutingMatrixWidgetAction* name_wa = new RoutingMatrixWidgetAction(2, 0, 0, this, tr("Channel grouping:"));
  name_wa->setArrayStayOpen(true);
  name_wa->setData(_GROUPING_CHANNELS_WIDGET_ACTION_);
  name_wa->array()->setColumnsExclusive(true);
  name_wa->array()->setExclusiveToggle(false);
  name_wa->array()->headerSetVisible(false);
  name_wa->array()->setText(0, tr("Mono "));
  name_wa->array()->setText(1, tr("Stereo "));
  switch(MusEGlobal::config.routerGroupingChannels)
  {
    case 1:
      name_wa->array()->setValue(0, true);
    break;
    case 2:
      name_wa->array()->setValue(1, true);
    break;
    default:
    break;
  }
  // Must rebuild array after text changes.
  name_wa->updateChannelArray();
  lb->addAction(name_wa);
  lb->addSeparator();
}         
  
//---------------------------------------------------------
//   addMenuItem
//---------------------------------------------------------

int RoutePopupMenu::addMenuItem(MusECore::AudioTrack* track, MusECore::Track* route_track, PopupMenu* lb, 
                                int id, int channel, int /*channels*/, bool isOutput)
{
  if(route_track->isMidiTrack())
    return ++id;
  
  MusECore::RouteList* rl = isOutput ? track->outRoutes() : track->inRoutes();
  
  const bool circ_route = (isOutput ? track : route_track)->isCircularRoute(isOutput ? route_track : track);
  
  MusECore::RouteCapabilitiesStruct t_caps = track->routeCapabilities();
  MusECore::RouteCapabilitiesStruct rt_caps = route_track->routeCapabilities();
  const int t_chans = isOutput ? t_caps._trackChannels._outChannels : t_caps._trackChannels._inChannels;
  const int rt_chans = isOutput ? rt_caps._trackChannels._inChannels : rt_caps._trackChannels._outChannels;
  
  // Support Audio Output Track to Audio Input Track 'Omni' routes.
  if(isOutput && track->type() == MusECore::Track::AUDIO_OUTPUT && route_track->type() == MusECore::Track::AUDIO_INPUT)
  {
    if(channel != -1 || !t_caps._trackChannels._outRoutable || !rt_caps._trackChannels._inRoutable)
      return ++id;
  }
  else
  if(!isOutput && track->type() == MusECore::Track::AUDIO_INPUT && route_track->type() == MusECore::Track::AUDIO_OUTPUT)
  {
    if(channel != -1 || !t_caps._trackChannels._inRoutable || !rt_caps._trackChannels._outRoutable)
      return ++id;
  }
  else
  {
    if(t_chans <= 0)
      return ++id;
    if(rt_chans <= 0)
      return ++id;
  }
  
#ifndef _USE_CUSTOM_WIDGET_ACTIONS_

  // Is it an omnibus route?
  if(channel == -1)
  {
    QAction* act = lb->addAction(route_track->displayName());
    act->setCheckable(true);
    MusECore::Route r(route_track, -1);
    act->setData(QVariant::fromValue(r));   
    for(MusECore::ciRoute ir = rl->begin(); ir != rl->end(); ++ir) 
    {
      if(ir->type == MusECore::Route::TRACK_ROUTE && ir->track == route_track && 
          ir->remoteChannel == r.channel && 
          ir->channel == r.remoteChannel && 
          ir->channels == r.channels)
      {
        act->setChecked(true);
        break;
      }  
    }
    if(!act->isChecked() && circ_route)  // If circular route exists, allow user to break it, otherwise forbidden.
      act->setEnabled(false);
  }
  else
#endif
    
  {
    
#ifdef _USE_CUSTOM_WIDGET_ACTIONS_
    
    QAction* act = lb->addAction(route_track->displayName());
    act->setCheckable(true);
    MusECore::Route r(route_track, -1);
    act->setData(QVariant::fromValue(r));   
    if(rl->contains(r))
      act->setChecked(true);
    
    if(rt_chans != 0 && t_chans != 0)
    {
      RoutePopupMenu* subp = new RoutePopupMenu(_route, this, isOutput, _broadcastChanges);
      subp->addAction(new MenuTitleItem(tr("Channels"), this));
      act->setMenu(subp);
      QActionGroup* act_group = new QActionGroup(this);
      act_group->setExclusive(false);
      for(int row = 0; row < rt_chans; ++row)
      {
        //RoutingMatrixWidgetAction* wa = new RoutingMatrixWidgetAction(t_chans, redLedIcon, darkRedLedIcon, this, QString::number(row + 1));
        RoutingMatrixWidgetAction* wa = new RoutingMatrixWidgetAction(t_chans, nullptr, nullptr, this, QString::number(row + 1));
        wa->setFont(wa->smallFont());
        wa->array()->headerSetVisible(row == 0);
        r.channel = row;
        wa->setData(QVariant::fromValue(r)); // Ignore the routing channel and channels - our action holds the channels.
        for(int col = 0; col < t_chans; ++col)
        {  
          for(MusECore::ciRoute ir = rl->begin(); ir != rl->end(); ++ir) 
          {
            if(ir->type == MusECore::Route::TRACK_ROUTE && ir->track == route_track && 
                ir->remoteChannel == row && 
                ir->channel == col && 
                ir->channels == 1)
            {
              wa->array()->setValue(col, true);
              break;
            }  
          }
        }
        // Must rebuild array after text changes.
        wa->updateChannelArray();
//         subp->addAction(wa);
        act_group->addAction(wa);
      }
      subp->addActions(act_group->actions());
    }
    
    if(!act->isChecked() && circ_route)  // If circular route exists, allow user to break it, otherwise forbidden.
      act->setEnabled(false);
    
#else
    
    // It's not an omnibus route. Add the individual channels...
    PopupMenu* subp = new PopupMenu(this, true);
    subp->setTitle(route_track->displayName());
    subp->addAction(new MenuTitleItem(tr("Channels"), this));
    QActionGroup* act_group = new QActionGroup(this);
    act_group->setExclusive(false);
    for(int i = 0; i < rt_chans; ++i)
    {
//       QAction* act = subp->addAction(QString::number(i + 1));
      QAction* act = act_group->addAction(QString::number(i + 1));
      act->setCheckable(true);
      MusECore::Route r(route_track, i, 1);
      r.remoteChannel = channel;
      act->setData(QVariant::fromValue(r));
      
      for(MusECore::ciRoute ir = rl->begin(); ir != rl->end(); ++ir) 
      {
        if(ir->type == MusECore::Route::TRACK_ROUTE && ir->track == route_track && 
            ir->remoteChannel == r.channel && 
            ir->channel == r.remoteChannel && 
            ir->channels == r.channels)
        {
          act->setChecked(true);
          break;
        }  
      }
      if(!act->isChecked() && circ_route)  // If circular route exists, allow user to break it, otherwise forbidden.
        act->setEnabled(false);
    }
    subp->addActions(act_group->actions());
    lb->addMenu(subp);
#endif
    
  }
  
  return ++id;
}  

//---------------------------------------------------------
//   addAuxPorts
//---------------------------------------------------------

int RoutePopupMenu::addAuxPorts(MusECore::AudioTrack* t, PopupMenu* lb, int id, int channel, int channels, bool isOutput)
      {
      MusECore::AuxList* al = MusEGlobal::song->auxs();
      for (MusECore::iAudioAux i = al->begin(); i != al->end(); ++i) {
            MusECore::Track* track = *i;
            if (t == track)
                  continue;
            id = addMenuItem(t, track, lb, id, channel, channels, isOutput);
            }
      return id;      
      }

//---------------------------------------------------------
//   addInPorts
//---------------------------------------------------------

int RoutePopupMenu::addInPorts(MusECore::AudioTrack* t, PopupMenu* lb, int id, int channel, int channels, bool isOutput)
      {
      MusECore::InputList* al = MusEGlobal::song->inputs();
      for (MusECore::iAudioInput i = al->begin(); i != al->end(); ++i) {
            MusECore::Track* track = *i;
            if (t == track)
                  continue;
            id = addMenuItem(t, track, lb, id, channel, channels, isOutput);
            }
      return id;      
      }

//---------------------------------------------------------
//   addOutPorts
//---------------------------------------------------------

int RoutePopupMenu::addOutPorts(MusECore::AudioTrack* t, PopupMenu* lb, int id, int channel, int channels, bool isOutput)
      {
      MusECore::OutputList* al = MusEGlobal::song->outputs();
      for (MusECore::iAudioOutput i = al->begin(); i != al->end(); ++i) {
            MusECore::Track* track = *i;
            if (t == track)
                  continue;
            id = addMenuItem(t, track, lb, id, channel, channels, isOutput);
            }
      return id;      
      }

//---------------------------------------------------------
//   addGroupPorts
//---------------------------------------------------------

int RoutePopupMenu::addGroupPorts(MusECore::AudioTrack* t, PopupMenu* lb, int id, int channel, int channels, bool isOutput)
      {
      MusECore::GroupList* al = MusEGlobal::song->groups();
      for (MusECore::iAudioGroup i = al->begin(); i != al->end(); ++i) {
            MusECore::Track* track = *i;
            if (t == track)
                  continue;
            id = addMenuItem(t, track, lb, id, channel, channels, isOutput);
            }
      return id;      
      }

//---------------------------------------------------------
//   addWavePorts
//---------------------------------------------------------

int RoutePopupMenu::addWavePorts(MusECore::AudioTrack* t, PopupMenu* lb, int id, int channel, int channels, bool isOutput)
      {
      MusECore::WaveTrackList* al = MusEGlobal::song->waves();
      for (MusECore::iWaveTrack i = al->begin(); i != al->end(); ++i) {
            MusECore::Track* track = *i;
            if (t == track)
                  continue;
            id = addMenuItem(t, track, lb, id, channel, channels, isOutput);
            }
      return id;      
      }

void RoutePopupMenu::addMidiTracks(MusECore::Track* t, PopupMenu* pup, bool isOutput)
{
  const MusECore::RouteList* const rl = isOutput ? t->outRoutes() : t->inRoutes();
  const MusECore::MidiTrackList* const mtracks = MusEGlobal::song->midis();
  for(MusECore::ciMidiTrack imt = mtracks->begin(); imt != mtracks->end(); ++imt)
  {
    MusECore::MidiTrack* const mt = *imt;
    QAction* act = pup->addAction(mt->displayName());
    act->setCheckable(true);
    const MusECore::Route r(mt, -1);
    act->setData(QVariant::fromValue(r));   
    if(rl->contains(r))
      act->setChecked(true);
  }
}

void RoutePopupMenu::addMidiPorts(MusECore::Track* t, PopupMenu* pup, bool isOutput, bool show_synths, bool want_writable)
{

#ifdef _USE_CUSTOM_WIDGET_ACTIONS_

  const MusECore::RouteList* const rl = isOutput ? t->outRoutes() : t->inRoutes();
  MusECore::MidiDevice* md;
  
  bool is_first_pass = true;
  QActionGroup* act_group = nullptr;
  // Order the entire listing by device type.
  for(int dtype = 0; dtype <= MusECore::MidiDevice::SYNTH_MIDI; ++dtype)
  {
  // Currently only midi port output to midi track input supports 'Omni' routes.
#ifdef _USE_MIDI_TRACK_SINGLE_OUT_PORT_CHAN_
    if(isOutput)
    {
      // Count the number of required rows. 
      int rows = 0;
      for(int i = 0; i < MusECore::MIDI_PORTS; ++i)
      {
        md = MusEGlobal::midiPorts[i].device();
        // This is desirable, but could lead to 'hidden' routes unless we add more support such as removing the existing routes when user changes flags.
        // So for now, just list all valid ports whether read or write.
        //if(!md)
        //  continue;
        if(!md || !(md->rwFlags() & (want_writable ? 1 : 2)))  // If this is an output click we are looking for midi writeable here.
          continue;
        // Do not list synth devices!
        if(!show_synths && md->isSynti())
          continue;
        // We only want the sorted device type.
        if(md->deviceType() != dtype)
          continue;
        ++rows;      
      }
      if(rows == 0)
        continue;
      
      if(is_first_pass)
      {
        is_first_pass = false;
        act_group = new QActionGroup(this);
        act_group->setExclusive(true);
      }
      
      int row = 0;
      MusECore::Route r(-1);   // Midi port route.
      for(int i = 0; i < MusECore::MIDI_PORTS; ++i)
      {
        md = MusEGlobal::midiPorts[i].device();
        // This is desirable, but could lead to 'hidden' routes unless we add more support such as removing the existing routes when user changes flags.
        // So for now, just list all valid ports whether read or write.
        //if(!md)
        //  continue;
        if(!md || !(md->rwFlags() & (want_writable ? 1 : 2)))  // If this is an output click we are looking for midi writeable here.
          continue;
        // Do not list synth devices!
        if(!show_synths && md->isSynti())
          continue;
        // We only want the sorted device type.
        if(md->deviceType() != dtype)
          continue;
        
        //RoutingMatrixWidgetAction* wa = new RoutingMatrixWidgetAction(
        //  MusECore::MUSE_MIDI_CHANNELS, redLedIcon, darkRedLedIcon, this, QString("%1:%2").arg(i + 1).arg(md->name()));
        RoutingMatrixWidgetAction* wa = new RoutingMatrixWidgetAction(
          MusECore::MUSE_MIDI_CHANNELS, nullptr, nullptr, this, QString("%1:%2").arg(i + 1).arg(md->name()));
        if(row == 0)
        {
          switch(dtype)
          {
            case MusECore::MidiDevice::ALSA_MIDI:
              wa->array()->headerSetTitle(tr("ALSA devices"));
            break;
            
            case MusECore::MidiDevice::JACK_MIDI:
              wa->array()->headerSetTitle(tr("JACK devices"));
            break;

            case MusECore::MidiDevice::SYNTH_MIDI:
              wa->array()->headerSetTitle(tr("Synth devices"));
            break;
          }
          wa->array()->setArrayTitle(tr("Channels"));
          wa->array()->headerSetVisible(true);
        }
        else
          wa->array()->headerSetVisible(false);
          
        r.midiPort = i;
        wa->setData(QVariant::fromValue(r));   
        
        wa->array()->setColumnsExclusive(true);
        MusECore::MidiTrack* mt = static_cast<MusECore::MidiTrack*>(t);
        if(i == mt->outPort())
          wa->array()->setValue(mt->outChannel(), true);
        // Must rebuild array after text changes.
        wa->updateChannelArray();
        
        // Make it easy for the user: Show the device's jack ports as well.
        // This is reasonable for midi devices since they are hidden away.
        // (Midi devices were made tracks, and midi ports eliminated, in the old MusE-2 muse_evolution branch!)
        switch(md->deviceType())
        {
          case MusECore::MidiDevice::JACK_MIDI:
          {
            const MusECore::Route md_r(md, -1);
            RoutePopupMenu* subp = new RoutePopupMenu(md_r, this, isOutput, _broadcastChanges);
            addJackPorts(md_r, subp);
            wa->setMenu(subp);
          }
          break;

          default:
          break;
        }
        
        act_group->addAction(wa);
        ++row;
      }
      pup->addActions(act_group->actions());
    }
    else 
    
#endif // _USE_MIDI_TRACK_SINGLE_OUT_PORT_CHAN_
    
    {
      // Count the number of required rows. 
      int rows = 0;
      for(int i = 0; i < MusECore::MIDI_PORTS; ++i)
      {
        md = MusEGlobal::midiPorts[i].device();
        // This is desirable, but could lead to 'hidden' routes unless we add more support such as removing the existing routes when user changes flags.
        // So for now, just list all valid ports whether read or write.
        //if(!md)
        //  continue;
        if(!md || !(md->rwFlags() & (want_writable ? 1 : 2)))  // If this is an input click we are looking for midi readable here.
          continue;
        // Do not list synth devices!
        if(!show_synths && md->isSynti())
          continue;
        // We only want the sorted device type.
        if(md->deviceType() != dtype)
          continue;
        ++rows;      
      }
      if(rows == 0)
        continue;
      
      // It's an input. Allow 'Omni' routes'...
      int row = 0;
      for(int i = 0; i < MusECore::MIDI_PORTS; ++i)
      {
        md = MusEGlobal::midiPorts[i].device();
        // This is desirable, but could lead to 'hidden' routes unless we add more support such as removing the existing routes when user changes flags.
        // So for now, just list all valid ports whether read or write.
        //if(!md)
        //  continue;
        if(!md || !(md->rwFlags() & (want_writable ? 1 : 2)))  // If this is an input click we are looking for midi readable here.
          continue;
        // Do not list synth devices!
        if(!show_synths && md->isSynti())
          continue;
        // We only want the sorted device type.
        if(md->deviceType() != dtype)
          continue;
        
        MusECore::Route r(i, -1);
        //RoutingMatrixWidgetAction* wa = new RoutingMatrixWidgetAction(
        //  MusECore::MUSE_MIDI_CHANNELS, redLedIcon, darkRedLedIcon, this, QString("%1:%2").arg(i + 1).arg(md->name()));
        RoutingMatrixWidgetAction* wa = new RoutingMatrixWidgetAction(
          MusECore::MUSE_MIDI_CHANNELS, nullptr, nullptr, this, QString("%1:%2").arg(i + 1).arg(md->name()));
        if(row == 0)
        {
          wa->array()->setCheckBoxTitle(tr("Omni"));
          switch(dtype)
          {
            case MusECore::MidiDevice::ALSA_MIDI:
              wa->array()->headerSetTitle(tr("ALSA devices"));
            break;
            
            case MusECore::MidiDevice::JACK_MIDI:
              wa->array()->headerSetTitle(tr("JACK devices"));
            break;

            case MusECore::MidiDevice::SYNTH_MIDI:
              wa->array()->headerSetTitle(tr("Synth devices"));
            break;
          }
          wa->array()->setArrayTitle(tr("Channels"));
          wa->array()->headerSetVisible(true);
        }
        else
          wa->array()->headerSetVisible(false);
        
        wa->setHasCheckBox(true);
        if(rl->contains(r))
          wa->setCheckBoxChecked(true);
        wa->setData(QVariant::fromValue(r)); // Ignore the routing channel and channels - our action holds the channels.
      
#ifdef _USE_MIDI_ROUTE_PER_CHANNEL_
        for(int col = 0; col < MusECore::MUSE_MIDI_CHANNELS; ++col)
        {  
          r.channel = col;
          if(rl->contains(r))
            wa->array()->setValue(col, true);
        }
#else  // _USE_MIDI_ROUTE_PER_CHANNEL_    
        int chans = 0;
        // Is there already a route?
        for(MusECore::ciRoute ir = rl->begin(); ir != rl->end(); ++ir)
        {
          switch(ir->type)
          {
            case MusECore::Route::MIDI_PORT_ROUTE:
              if(ir->midiPort == i)
                chans = ir->channel; // Grab the channels.
            break;  
            case MusECore::Route::TRACK_ROUTE:
            case MusECore::Route::JACK_ROUTE:
            case MusECore::Route::MIDI_DEVICE_ROUTE:
            break;  
          }
          if(chans != 0)
            break;
        }
        if(chans != 0 && chans != -1)
        {
          for(int col = 0; col < MusECore::MUSE_MIDI_CHANNELS; ++col)
          {
            if(chans & (1 << col))
              wa->array()->setValue(col, true);
          }
        }
#endif // _USE_MIDI_ROUTE_PER_CHANNEL_

        // Must rebuild array after text changes.
        wa->updateChannelArray();
        
        // Make it easy for the user: Show the device's jack ports as well.
        // This is reasonable for midi devices since they are hidden away.
        // (Midi devices were made tracks, and midi ports eliminated, in the old MusE-2 muse_evolution branch!)
        switch(md->deviceType())
        {
          case MusECore::MidiDevice::JACK_MIDI:
          {
            //PopupMenu* subp = new PopupMenu(this, true);
            const MusECore::Route md_r(md, -1);
            RoutePopupMenu* subp = new RoutePopupMenu(md_r, this, isOutput, _broadcastChanges);
            addJackPorts(md_r, subp);
            wa->setMenu(subp);
          }
          break;
          
          default:
          break;
        }
        
        pup->addAction(wa);
        ++row;
      }
    }
        
#else // _USE_CUSTOM_WIDGET_ACTIONS_
        
    pup->addAction(new MenuTitleItem(qApp->translate("@default", QT_TRANSLATE_NOOP("@default", "Output port/device")), pup)); 
    for(int i = 0; i < MIDI_PORTS; ++i)
    {
      MusECore::MidiPort* mp = &MusEGlobal::midiPorts[i];
      MusECore::MidiDevice* md = mp->device();
      
      // This is desirable, but could lead to 'hidden' routes unless we add more support
      //  such as removing the existing routes when user changes flags.
      // So for now, just list all valid ports whether read or write.
      //if(!md)
      //  continue;
      if(!md || !(md->rwFlags() & (want_writable ? 1 : 2)))  // If this is an input click we are looking for midi outputs here.
        continue;
            
      // Do not list synth devices!
      if(!show_synths && md->isSynti())
        continue;
      // We only want the sorted device type.
      if(md->deviceType() != dtype)
        continue;
            
      MusECore::RouteList* rl = isOutput ? t->outRoutes() : t->inRoutes();
      
      int chanmask = 0;
      // To reduce number of routes required, from one per channel to just one containing a channel mask. 
      // Look for the first route to this midi port. There should always be only a single route for each midi port, now.
      for(MusECore::ciRoute ir = rl->begin(); ir != rl->end(); ++ir)   
      {
        if(ir->type == MusECore::Route::MIDI_PORT_ROUTE && ir->midiPort == i) 
        {
          // We have a route to the midi port. Grab the channel mask.
          chanmask = ir->channel;
          break;
        }
      }
      
      PopupMenu* subp = new PopupMenu(pup, true);
      subp->setTitle(md->name()); 
      QAction* act;
      
      for(int ch = 0; ch < MusECore::MUSE_MIDI_CHANNELS; ++ch) 
      {
        act = subp->addAction(QString("Channel %1").arg(ch+1));
        act->setCheckable(true);
        
        int chbit = 1 << ch;
        MusECore::Route srcRoute(i, chbit);    // In accordance with channel mask, use the bit position.
        
        act->setData(QVariant::fromValue(srcRoute));   
        
        if(chanmask & chbit)                  // Is the channel already set? Show item check mark.
          act->setChecked(true);
      }
      act = subp->addAction(QString("Toggle all"));
      //act->setCheckable(true);
      MusECore::Route togRoute(i, (1 << MusECore::MUSE_MIDI_CHANNELS) - 1);    // Set all channel bits.
      act->setData(QVariant::fromValue(togRoute));   
      pup->addMenu(subp);
    }    
  
#endif // _USE_CUSTOM_WIDGET_ACTIONS_    

  }
  
  return;
}

//---------------------------------------------------------
//   addSynthPorts
//---------------------------------------------------------

int RoutePopupMenu::addSynthPorts(MusECore::AudioTrack* t, PopupMenu* lb, int id, int channel, int channels, bool isOutput)
{
      MusECore::SynthIList* al = MusEGlobal::song->syntis();
      for (MusECore::iSynthI i = al->begin(); i != al->end(); ++i) {
            MusECore::Track* track = *i;
            if (t == track)
                  continue;
            id = addMenuItem(t, track, lb, id, channel, channels, isOutput);
            }
      return id;      
}

//---------------------------------------------------------
//   addJackPorts
//---------------------------------------------------------

void RoutePopupMenu::addJackPorts(const MusECore::Route& route, PopupMenu* lb)
{
  if(!MusEGlobal::checkAudioDevice())
    return;

  MusECore::RouteList* rl = nullptr;
  int channels = -1;
  std::list<QString> ol;
  MusECore::RouteCapabilitiesStruct rcaps;
  switch(route.type)
  {
    case MusECore::Route::TRACK_ROUTE:
      ol = _isOutMenu ? MusEGlobal::audioDevice->inputPorts() : MusEGlobal::audioDevice->outputPorts();
      rl = _isOutMenu ? route.track->outRoutes() : route.track->inRoutes();
      rcaps = route.track->routeCapabilities();
      channels = _isOutMenu ? rcaps._jackChannels._outChannels : rcaps._jackChannels._inChannels;
    break;
    
    case MusECore::Route::MIDI_DEVICE_ROUTE:
      ol = _isOutMenu ? MusEGlobal::audioDevice->inputPorts(true) : MusEGlobal::audioDevice->outputPorts(true);
      rl = _isOutMenu ? route.device->outRoutes() : route.device->inRoutes();
    break;
    
    case MusECore::Route::JACK_ROUTE:
    case MusECore::Route::MIDI_PORT_ROUTE:
      return;
    break;
  }
  
  const int sz = ol.size();
  if(sz != 0)
  {
    
#ifdef _USE_CUSTOM_WIDGET_ACTIONS_
    
    //RoutingMatrixWidgetAction* name_wa = new RoutingMatrixWidgetAction(2, redLedIcon, darkRedLedIcon, this, tr("Show aliases:"));
    RoutingMatrixWidgetAction* name_wa = new RoutingMatrixWidgetAction(2, nullptr, nullptr, this, tr("Show aliases:"));
    name_wa->setArrayStayOpen(true);
    name_wa->setData(_ALIASES_WIDGET_ACTION_);
    name_wa->array()->setColumnsExclusive(true);
    name_wa->array()->setExclusiveToggle(true);
    name_wa->array()->headerSetVisible(false);
    name_wa->array()->setText(0, tr("First  "));
    name_wa->array()->setText(1, tr("Second "));
    switch(MusEGlobal::config.preferredRouteNameOrAlias)
    {
      case MusEGlobal::RoutePreferFirstAlias:
        name_wa->array()->setValue(0, true);
      break;
      case MusEGlobal::RoutePreferSecondAlias:
        name_wa->array()->setValue(1, true);
      break;
      case MusEGlobal::RoutePreferCanonicalName:
      break;
    }
    // Must rebuild array after text changes.
    name_wa->updateChannelArray();
    lb->addAction(name_wa);
    lb->addSeparator();
#else
    QActionGroup* act_grp = new QActionGroup(this);
    act_grp->setExclusive(true);
    act = act_grp->addAction(tr("Show names"));
    act->setCheckable(true);
    act->setData(_SHOW_CANONICAL_NAMES_);
    if(MusEGlobal::config.preferredRouteNameOrAlias == MusEGlobal::RoutePreferCanonicalName)
      act->setChecked(true); 
    act = act_grp->addAction(tr("Show first aliases"));
    act->setCheckable(true);
    act->setData(_SHOW_FIRST_ALIASES_);
    if(MusEGlobal::config.preferredRouteNameOrAlias == MusEGlobal::RoutePreferFirstAlias)
      act->setChecked(true); 
    act = act_grp->addAction(tr("Show second aliases"));
    act->setCheckable(true);
    act->setData(_SHOW_SECOND_ALIASES_);
    if(MusEGlobal::config.preferredRouteNameOrAlias == MusEGlobal::RoutePreferSecondAlias)
      act->setChecked(true); 
    lb->addActions(act_grp->actions());
    lb->addSeparator();
#endif                
    
#ifdef _USE_CUSTOM_WIDGET_ACTIONS_
    
    QActionGroup* act_group = new QActionGroup(this);
    act_group->setExclusive(false);
    int row = 0;
    for(std::list<QString>::iterator ip = ol.begin(); ip != ol.end(); ++ip)
    {
      QByteArray ba = (*ip).toLatin1();
      const char* port_name = ba.constData();
      void* const port = MusEGlobal::audioDevice->findPort(port_name);
      if(port)
      {
        //RoutingMatrixWidgetAction* wa = new RoutingMatrixWidgetAction(channels == -1 ? 1 : channels, redLedIcon, darkRedLedIcon, this);
        RoutingMatrixWidgetAction* wa = new RoutingMatrixWidgetAction(channels == -1 ? 1 : channels, nullptr, nullptr, this);
        if(row == 0)
        {
          wa->array()->headerSetTitle(tr("Jack ports"));
          if(channels == -1)
          {
            wa->array()->setArrayTitle(tr("Connect"));
            wa->array()->headerSetVisible(false);
          }
          else
          {
            wa->array()->setArrayTitle(tr("Channels"));
            wa->array()->headerSetVisible(true);
          }
        }
        else
          wa->array()->headerSetVisible(false);

        char good_name[ROUTE_PERSISTENT_NAME_SIZE];
        
        // Get the preferred display name.
        MusEGlobal::audioDevice->portName(port, good_name, ROUTE_PERSISTENT_NAME_SIZE, MusEGlobal::config.preferredRouteNameOrAlias);
        wa->setActionText(good_name);
        
        // Get a good routing name.
        MusEGlobal::audioDevice->portName(port, good_name, ROUTE_PERSISTENT_NAME_SIZE);
        MusECore::Route r(MusECore::Route::JACK_ROUTE, -1, port, -1, -1, -1, good_name);
        
        wa->setData(QVariant::fromValue(r));
        if(channels == -1)
        {
          if(rl->contains(r))
            wa->array()->setValue(0, true);
        }
        else
        {
          for(int i = 0; i < channels; ++i) 
          {
            r.channel = i;
            if(rl->contains(r))
              wa->array()->setValue(i, true);
          }
        }
        // Must rebuild array after text changes.
        wa->updateChannelArray();
//         lb->addAction(wa);
        act_group->addAction(wa);
        ++row;
      }
    }  
    lb->addActions(act_group->actions());

#else
    
    QAction* act = 0;
    if(channels == -1)
    {
      if(!MusEGlobal::checkAudioDevice())
      { 
        clear();
        return;
      }
      for(std::list<QString>::iterator ip = ol.begin(); ip != ol.end(); ++ip) 
      {
        act = lb->addAction(*ip);
        act->setCheckable(true);
        
        QByteArray ba = (*ip).toLatin1();
        const char* port_name = ba.constData();
        char good_name[ROUTE_PERSISTENT_NAME_SIZE];
        void* const port = MusEGlobal::audioDevice->findPort(port_name);
        if(port)
        {
          MusEGlobal::audioDevice->portName(port, good_name, ROUTE_PERSISTENT_NAME_SIZE);
          port_name = good_name;
        }
        MusECore::Route dst(MusECore::Route::JACK_ROUTE, -1, NULL, -1, -1, -1, port_name);
        
        act->setData(QVariant::fromValue(dst));   
        if(rl->exists(r))
          act->setChecked(true);
      }      
    }
    else
    {
      for(int i = 0; i < channels; ++i) 
      {
        QString chBuffer = tr("Channel") + QString(" ") + QString::number(i + 1);
        MenuTitleItem* titel = new MenuTitleItem(chBuffer, this);
        lb->addAction(titel); 

        if(!MusEGlobal::checkAudioDevice())
        { 
          clear();
          return;
        }
        for(std::list<QString>::iterator ip = ol.begin(); ip != ol.end(); ++ip) 
        {
          act = lb->addAction(*ip);
          act->setCheckable(true);
          
          QByteArray ba = (*ip).toLatin1();
          const char* port_name = ba.constData();
          char good_name[ROUTE_PERSISTENT_NAME_SIZE];
          void* const port = MusEGlobal::audioDevice->findPort(port_name);
          if(port)
          {
            MusEGlobal::audioDevice->portName(port, good_name, ROUTE_PERSISTENT_NAME_SIZE);
            port_name = good_name;
          }
          MusECore::Route dst(MusECore::Route::JACK_ROUTE, -1, NULL, i, -1, -1, port_name);
          
          act->setData(QVariant::fromValue(dst));   
          for(MusECore::ciRoute ir = rl->begin(); ir != rl->end(); ++ir) 
          {
            if(*ir == dst) 
            {
              act->setChecked(true);
              break;
            }
          }
        }
        if(i+1 != channels)
          lb->addSeparator();
      }
    }
#endif                

  }
  
  QList<QAction*> act_list;
  int row = 0;
  for(MusECore::iRoute ir = rl->begin(); ir != rl->end(); ++ir)
  {
    switch(ir->type)
    {
      case MusECore::Route::JACK_ROUTE:
        if(ir->jackPort == nullptr && MusEGlobal::audioDevice->findPort(ir->persistentJackPortName) == nullptr)
        {
          //RoutingMatrixWidgetAction* wa = new RoutingMatrixWidgetAction(channels == -1 ? 1 : channels, redLedIcon, darkRedLedIcon, 
          RoutingMatrixWidgetAction* wa = new RoutingMatrixWidgetAction(channels == -1 ? 1 : channels, nullptr, nullptr,
                                                                        this, ir->persistentJackPortName);
          wa->setEnabled(false);
          if(row == 0)
          {
            wa->array()->headerSetTitle(tr("Jack ports"));
            if(channels == -1)
            {
              wa->array()->setArrayTitle(tr("Connect"));
              wa->array()->headerSetVisible(false);
            }
            else
            {
              wa->array()->setArrayTitle(tr("Channels"));
              wa->array()->headerSetVisible(true);
            }
          }
          else
            wa->array()->headerSetVisible(false);

          MusECore::Route r(MusECore::Route::JACK_ROUTE, -1, nullptr, -1, -1, -1, ir->persistentJackPortName);
          wa->setData(QVariant::fromValue(r));

          if(channels == -1)
            wa->array()->setValue(0, true);
          else
          {
            for(int i = 0; i < channels; ++i) 
            {
              if(i == ir->channel)
                wa->array()->setValue(i, true);
            }
          }
          // Must rebuild array after text changes.
          wa->updateChannelArray();
          act_list.append(wa);
          ++row;
        }
      break;  
      
      case MusECore::Route::TRACK_ROUTE:
      case MusECore::Route::MIDI_DEVICE_ROUTE:
      case MusECore::Route::MIDI_PORT_ROUTE:
      break;  
    }
  }

  if(!act_list.isEmpty())
  {
    RoutePopupMenu* subp = new RoutePopupMenu(route, this, _isOutMenu, _broadcastChanges);
    subp->setTitle(tr("Unavailable"));
    const int sz = act_list.size();
    for(int i = 0; i < sz; ++i)
      subp->addAction(act_list.at(i));
    lb->addMenu(subp);
  }
}

//======================
// RoutePopupMenu
//======================

RoutePopupMenu::RoutePopupMenu(QWidget* parent, bool isOutput, bool broadcastChanges)
               : PopupMenu(parent, true), _isOutMenu(isOutput), _broadcastChanges(broadcastChanges)
{
  init();
}

RoutePopupMenu::RoutePopupMenu(const MusECore::Route& route, QWidget* parent, bool isOutput, bool broadcastChanges)
               : PopupMenu(parent, true), _route(route), _isOutMenu(isOutput), _broadcastChanges(broadcastChanges)
{
  init();
}

RoutePopupMenu::RoutePopupMenu(const MusECore::Route& route, const QString& title, QWidget* parent, bool isOutput, bool broadcastChanges)
               //: PopupMenu(title, parent, true), _track(track), _isOutMenu(isOutput)
               : PopupMenu(title, parent, true), _route(route), _isOutMenu(isOutput), _broadcastChanges(broadcastChanges)
{
  init();        
}

void RoutePopupMenu::init()
{
  _hoverIsFromMouse = false;
  connect(this, SIGNAL(hovered(QAction*)), SLOT(routePopupHovered(QAction*)));
  connect(MusEGlobal::song, SIGNAL(songChanged(MusECore::SongChangedStruct_t)), SLOT(songChanged(MusECore::SongChangedStruct_t)));
}

bool RoutePopupMenu::event(QEvent* event)
{
  DEBUG_PRST_ROUTES(stderr, "RoutePopupMenu::event:%p activePopupWidget:%p this:%p class:%s event type:%d\n",
          event, QApplication::activePopupWidget(), this, metaObject()->className(), event->type());
  
  switch(event->type())
  {
    // Technical difficulties:
    // "mouseReleaseEvent() is called when a mouse button is released. A widget receives mouse release events 
    //   when it has received the corresponding mouse press event. This means that if the user presses the mouse 
    //   inside your widget, then drags the mouse somewhere else before releasing the mouse button, your widget 
    //   receives the release event. There is one exception: if a popup menu appears while the mouse button is held down, 
    //   this popup immediately steals the mouse events."
    // Unfortunately that's exactly what we don't want. The mouse release events are not being passed to the higher-up menu
    //  if we hold the mouse down and move over another menu item which has a submenu - the (delayed) appearance of that 
    //  submenu steals the release. Oddly, if the mouse is moved further - even just once - within the new item, 
    //  the release event IS passed on. So to avoid dealing with that distinction, let's just pass on all release events.
    // Should be OK under normal usage, since it makes some sense that no mouse events should be reaching a submenu anyway - 
    //  they have no effect since the cursor position is outside of them !
    // NOTE: If a submenu OVERLAPS its higher-up menu, this could be a big problem. In general how to deal with overlapping popups
    //        when we wish to be able to click on items in both a menu and its submenu. Overlapping will only happen with too-wide 
    //        menus which should be rare for routing, but we could also defer to the advanced router when the popup becomes too wide.
    case QEvent::MouseButtonRelease:
    case QEvent::MouseButtonPress:
// Removed. Causes very high CPU usage spikes.
// I think I remember adding MouseMove simply for 'good luck' rather than any real usefulness.
// Tested OK /without/ this on KUBUNTU 15.10, we don't seem to need it. Retest on 14.04 LTS...
//     case QEvent::MouseMove:
    {
      QMouseEvent* mev = static_cast<QMouseEvent*>(event);
      DEBUG_PRST_ROUTES(stderr, "RoutePopupMenu::event type:%d x:%d y:%d gx:%d gy:%d sx:%f sy:%f wx:%f wy:%f lx:%f ly:%f\n", 
              mev->type(),
              mev->pos().x(), mev->pos().y(), 
              mev->globalPos().x(), mev->globalPos().y(), 
              mev->screenPos().x(), mev->screenPos().y(), 
              mev->windowPos().x(), mev->windowPos().y(), 
              mev->localPos().x(), mev->localPos().y());
      
      QMenu* target_menu = nullptr;
      const int sz = QApplication::topLevelWidgets().size();
      for(int i = 0; i < sz; ++i)
      {
        QWidget* w = QApplication::topLevelWidgets().at(i);
        DEBUG_PRST_ROUTES(stderr, "   checking top level widget:%p\n", w);
        if(QMenu* menu = qobject_cast<QMenu*>(w))
        {
          if(menu->windowType() != Qt::Popup)
            continue;
          DEBUG_PRST_ROUTES(stderr, "   checking hit in menu:%p visible:%d modal:%d\n", menu, menu->isVisible(), menu->isModal());
          if(!menu->isVisible() || !menu->geometry().contains(mev->globalPos()))
            continue;
          DEBUG_PRST_ROUTES(stderr, "   hit\n");
          // If we hit the submenu it means the submenu is partially or wholly obscuring the other menu.
          // We must honour the submenu in this case even if it is only slightly obscuring the other menu.
          if(menu == this)
          {
            DEBUG_PRST_ROUTES(stderr, "   menu is this\n");
            return PopupMenu::event(mev);
          }
          // The menu is a good target - the mouse is within it and it is not obscured.
          // Regardless, afterward continue watching for THIS menu...
          if(!target_menu)
            target_menu = menu;
        }
      }
      
      if(target_menu)
      {
        DEBUG_PRST_ROUTES(stderr, "   target_menu:%p\n", target_menu);
        QMouseEvent new_mev(mev->type(), 
                            //mev->windowPos(), // Relative to the widget the mouse is actually over (menu variable).
                            QPointF(target_menu->mapFromGlobal(mev->globalPos())),
                            mev->screenPos(),
                            mev->button(),
                            mev->buttons(),
                            mev->modifiers());
        new_mev.setAccepted(mev->isAccepted());
        new_mev.setTimestamp(mev->timestamp());
        QApplication::sendEvent(target_menu, &new_mev);
        return true;
      }
      
      DEBUG_PRST_ROUTES(stderr, "   no target popup found\n");
    }
    break;
    
    case QEvent::KeyPress:
    {
      QKeyEvent* e = static_cast<QKeyEvent*>(event);
      switch(e->key())
      {
          case Qt::Key_Space:
            if (!style()->styleHint(QStyle::SH_Menu_SpaceActivatesItem, nullptr, this))
                break;
          // NOTE: Error suppressor for new gcc 7 'fallthrough' level 3 and 4:
          // FALLTHROUGH
          case Qt::Key_Select:
          case Qt::Key_Return:
          case Qt::Key_Enter:
          {
            if(activeAction() && (!contextMenu() || !contextMenu()->isVisible()))
            {
              if(RoutingMatrixWidgetAction* mwa = qobject_cast<RoutingMatrixWidgetAction*>(activeAction()))
              {
                bool accept = false;
                if(mwa->hasCheckBox() && mwa->isSelected())
                {
                  mwa->setCheckBoxChecked(!mwa->checkBoxChecked());
                }
                else if(mwa->array()->columns() != 0 && mwa->array()->activeColumn() != -1)
                {
                  mwa->array()->setValue(mwa->array()->activeColumn(), !mwa->array()->value(mwa->array()->activeColumn()));
                  // Reset any other switch bars besides this one which are part of a QActionGroup.
                  // Since they are all part of an action group, force them to be exclusive regardless of their exclusivity settings.
                  QActionGroup* act_group = mwa->actionGroup();
                  if(act_group && act_group->isExclusive())
                  {
                    const int sz = act_group->actions().size();
                    for(int i = 0; i < sz; ++i) 
                    {
                      if(RoutingMatrixWidgetAction* act = qobject_cast<RoutingMatrixWidgetAction*>(act_group->actions().at(i)))
                      {
                        if(act != mwa)
                        {
                          // Set any column to false, and exclusiveColumns and exclusiveToggle to true which will reset all columns.
                          act->array()->setValues(0, false, true, true);
                          //update();  // Redraw the indicators.
                          act->updateCreatedWidgets();  // Redraw the indicators.
                        }
                      }
                    }  
                  }
                  if(mwa->arrayStayOpen())
                    accept = true;
                }
                else
                {
                  // Nothing selected. Do nothing. TODO: Select the first available item, like QMenu does...
                  e->accept();
                  return true; // We handled it.
                }
                
                mwa->updateCreatedWidgets();
                e->accept();
                mwa->trigger();  // Trigger the action. 
                // Check for Ctrl to stay open.
                if(!accept && (!stayOpen() || (!MusEGlobal::config.popupsDefaultStayOpen && (e->modifiers() & Qt::ControlModifier) == 0)))
                  closeUp(); // Close all the popups.
                return true; // We handled it.
              }
              // Otherwise let ancestor PopupMenu handle it...
            }
          }
          break;
        
          default:
          break;
      }
    }
    break;
   
    default:
    break;
  }
  
  return PopupMenu::event(event);
}

void RoutePopupMenu::resizeEvent(QResizeEvent* e)
{
  DEBUG_PRST_ROUTES(stderr, "RoutePopupMenu::resizeEvent\n");
  e->ignore();
  PopupMenu::resizeEvent(e);
}

void RoutePopupMenu::mouseReleaseEvent(QMouseEvent* e)
{
  DEBUG_PRST_ROUTES(stderr, "RoutePopupMenu::mouseReleaseEvent this:%p x:%d y:%d\n", this, e->pos().x(), e->pos().y());
  if(contextMenu() && contextMenu()->isVisible())
    return;
  
  DEBUG_PRST_ROUTES_2(stderr, "RoutePopupMenu::mouseReleaseEvent begin: this:%p active action:%p\n", this, activeAction()); 
  
  bool activate = false;
  bool accept = false;

  QAction* action = actionAt(e->pos());
  RoutingMatrixWidgetAction* act_mwa = qobject_cast<RoutingMatrixWidgetAction*>(action);
  
//   RoutingMatrixWidgetAction* mwa = 0;
//   QAction* action = actionAt(e->pos());
//   if(action)
//   {
//     mwa = qobject_cast<RoutingMatrixWidgetAction*>(action);
//     if(mwa)
//     {
//       RoutePopupHit hit = mwa->hitTest(e->pos(), RoutePopupHit::HitTestClick);
//       switch(hit._type)
//       {
//         case RoutePopupHit::HitChannel:
//         {
//           mwa->array()->setValue(hit._value, !mwa->array()->value(hit._value));
//           
//           // Reset any other switch bars besides this one which are part of a QActionGroup.
//           // Since they are all part of an action group, force them to be exclusive regardless of their exclusivity settings.
//           QActionGroup* act_group = mwa->actionGroup();
//           if(act_group && act_group->isExclusive())
//           {
//             const int sz = act_group->actions().size();
//             for(int i = 0; i < sz; ++i) 
//             {
//               if(RoutingMatrixWidgetAction* act = qobject_cast<RoutingMatrixWidgetAction*>(act_group->actions().at(i)))
//               {
//                 if(act != mwa)
//                 {
//                   // Set any column to false, and exclusiveColumns and exclusiveToggle to true which will reset all columns.
//                   act->array()->setValues(0, false, true, true);
//                   act->updateCreatedWidgets(); // Redraw the indicators.
//                 }
//               }
//             }  
//           }
//             
//           if(mwa->arrayStayOpen())
//             accept = true;
//           activate = true;
//         }
//         break;
//         
//         case RoutePopupHit::HitMenuItem:
//           mwa->setCheckBoxChecked(!mwa->checkBoxChecked());
//           activate = true;
//         break;
//         
//         case RoutePopupHit::HitChannelBar:
//         case RoutePopupHit::HitSpace:
//           accept = true;
//         break;
//         
//         case RoutePopupHit::HitNone:
//         break;
//       }
//     }
//   }

  int ch_hit_clk_idx_min = -1;
  int ch_hit_clk_idx_max = -1;
  int ch_hit_clk_ch_start = -1;
  bool ch_hit_clk_val = false;
  QActionGroup* ch_hit_clk_act_group = nullptr;
  
  const int sz = actions().size();
  for(int i = 0; i < sz; ++i)
  {
    if(RoutingMatrixWidgetAction* mwa = qobject_cast<RoutingMatrixWidgetAction*>(actions().at(i)))
    {
      bool do_upd = false;
      // Sanity check: Only activate the item(s) if the action truly is the active one.
      //if(mwa == activeAction())
      if(mwa == action)
      {
        RoutePopupHit hit = mwa->hitTest(e->pos(), RoutePopupHit::HitTestClick);
        switch(hit._type)
        {
          case RoutePopupHit::HitChannel:
          {
            // Support grouping together of channels.
            ch_hit_clk_idx_min = i;
            ch_hit_clk_idx_max = ch_hit_clk_idx_min + MusEGlobal::config.routerGroupingChannels;
            if(ch_hit_clk_idx_max > sz)
              ch_hit_clk_idx_min = sz - MusEGlobal::config.routerGroupingChannels;
            ch_hit_clk_ch_start = hit._value - (i - ch_hit_clk_idx_min);
            const int ch_diff = mwa->array()->columns() - (ch_hit_clk_ch_start + MusEGlobal::config.routerGroupingChannels);
            if(ch_diff < 0)
            {
              ch_hit_clk_idx_min += ch_diff;
              ch_hit_clk_idx_max += ch_diff;
              ch_hit_clk_ch_start += ch_diff;
            }
            
            ch_hit_clk_act_group = mwa->actionGroup();
            ch_hit_clk_val = !mwa->array()->value(hit._value);
            
            DEBUG_PRST_ROUTES_2(stderr, "RoutePopupMenu::mouseReleaseEvent i:%d hit._value:%d ch_hit_clk_idx_min:%d ch_hit_clk_idx_max:%d ch_hit_clk_ch_start:%d ch_hit_clk_val:%d\n",
                                i, hit._value, ch_hit_clk_idx_min, ch_hit_clk_idx_max, ch_hit_clk_ch_start, ch_hit_clk_val); 
            
            if(mwa->array()->value(hit._value) != ch_hit_clk_val)
            {
              DEBUG_PRST_ROUTES_2(stderr, "   calling mwa->array()->setValue\n"); 
              mwa->array()->setValue(hit._value, ch_hit_clk_val);
              do_upd = true;
            }
            if(mwa->setMenuItemPressed(false) || mwa->array()->setPressedColumn(-1))
              do_upd = true;
            
//             // Reset any other switch bars besides this one which are part of a QActionGroup.
//             // Since they are all part of an action group, force them to be exclusive regardless of their exclusivity settings.
//             QActionGroup* act_group = mwa->actionGroup();
//             if(act_group && act_group->isExclusive())
//             {
//               const int sz = act_group->actions().size();
//               for(int i = 0; i < sz; ++i) 
//               {
//                 if(RoutingMatrixWidgetAction* act = qobject_cast<RoutingMatrixWidgetAction*>(act_group->actions().at(i)))
//                 {
//                   if(act != mwa)
//                   {
//                     // Set any column to false, and exclusiveColumns and exclusiveToggle to true which will reset all columns.
//                     act->array()->setValues(0, false, true, true);
//                     act->updateCreatedWidgets(); // Redraw the indicators.
//                   }
//                 }
//               }  
//             }
              
            if(mwa->arrayStayOpen())
              accept = true;
            activate = true;
            
//             // Directly execute the trigger handler.
//             e->accept();
//             routePopupActivated(mwa);
          }
          break;
          
          case RoutePopupHit::HitMenuItem:
          {
            const bool chk = !mwa->checkBoxChecked();
            if(mwa->checkBoxChecked() != chk)
            {
              mwa->setCheckBoxChecked(chk);
              do_upd = true;
            }
            activate = true;
            
//             // Directly execute the trigger handler.
//             routePopupActivated(mwa);
          }
          break;
          
          case RoutePopupHit::HitChannelBar:
          case RoutePopupHit::HitSpace:
            accept = true;
            
//             e->accept();
          break;
          
          case RoutePopupHit::HitNone:
          break;
        }        
      }
      if(do_upd)
        mwa->updateCreatedWidgets();
    }  
  }  
  
  
  
  for(int i = 0; i < sz; ++i)
  {
    if(RoutingMatrixWidgetAction* mwa = qobject_cast<RoutingMatrixWidgetAction*>(actions().at(i)))
    {
      bool do_upd = false;
      // Sanity check: Only activate the item(s) which are not the active one.
      //if(mwa != activeAction())
      if(mwa != action)
      {
        DEBUG_PRST_ROUTES_2(stderr, "RoutePopupMenu::mouseReleaseEvent i:%d this:%p inactive mwa:%p\n", i, this, mwa); 
        
        if(ch_hit_clk_act_group && ch_hit_clk_act_group == mwa->actionGroup())
        {
          DEBUG_PRST_ROUTES_2(stderr, "   ch_hit_clk_act_group && ch_hit_clk_act_group == mwa->actionGroup()\n"); 
          if(ch_hit_clk_act_group->isExclusive())
          {
            // Reset any other switch bars besides this one which are part of a QActionGroup.
            // Since they are all part of an action group, force them to be exclusive regardless of their exclusivity settings.
            // Set any column to false, and exclusiveColumns and exclusiveToggle to true which will reset all columns.
            DEBUG_PRST_ROUTES_2(stderr, "   calling mwa->array()->setValues (reset)\n"); 
            mwa->array()->setValues(0, false, true, true);
            do_upd = true;
          }
          else if(i >= ch_hit_clk_idx_min && i < ch_hit_clk_idx_max)
          {
            const int ch = ch_hit_clk_ch_start + (i - ch_hit_clk_idx_min);
//             mwa->array()->setValue(ch, !mwa->array()->value(ch));
            if(mwa->array()->value(ch) != ch_hit_clk_val)
            {
              DEBUG_PRST_ROUTES_2(stderr, "   calling mwa->array()->setValue ch:%d\n", ch); 
              mwa->array()->setValue(ch, ch_hit_clk_val);
              do_upd = true;
            }
            
//             // Directly execute the trigger handler.
//             e->accept();
//             routePopupActivated(mwa);
            
            DEBUG_PRST_ROUTES_2(stderr, "   ch:%d active col:%d pressed col:%d\n", 
                              ch_hit_clk_ch_start + (i - ch_hit_clk_idx_min), mwa->array()->activeColumn(), mwa->array()->pressedColumn()); 
          }
        }
//         else 
        if(mwa->setMenuItemPressed(false) || mwa->array()->setPressedColumn(-1))
          do_upd = true;

      }
      if(do_upd)
        mwa->updateCreatedWidgets();
    }
  }
  
  if(!action || !act_mwa)
  {
    e->ignore();
    // Defer to PopupMenu, where we handle regular actions with checkboxes.
    PopupMenu::mouseReleaseEvent(e);
    DEBUG_PRST_ROUTES_2(stderr, "RoutePopupMenu::mouseReleaseEvent defer end: this:%p active action:%p\n", this, activeAction()); 
    return;
  }

  if(accept)
  {
    DEBUG_PRST_ROUTES_2(stderr, "RoutePopupMenu::mouseReleaseEvent accept\n"); 
    e->accept();
    if(activate)
    {
      DEBUG_PRST_ROUTES_2(stderr, "RoutePopupMenu::mouseReleaseEvent accept: directly executing trigger handler routePopupActivated(act_mwa)\n"); 
      // Directly execute the trigger handler.
      routePopupActivated(act_mwa);
    }
    DEBUG_PRST_ROUTES_2(stderr, "RoutePopupMenu::mouseReleaseEvent accept end: this:%p active action:%p\n", this, activeAction()); 
    return;
  }
  
  // Check for Ctrl to stay open.
  if(!stayOpen() || (!MusEGlobal::config.popupsDefaultStayOpen && (e->modifiers() & Qt::ControlModifier) == 0))
  {
    DEBUG_PRST_ROUTES_2(stderr, "RoutePopupMenu::mouseReleaseEvent No stay-open\n"); 
    e->ignore();
    // If this is the active popup widget let the ancestor activate and close it, otherwise we must close this manually.
// //     QMenu* m = qobject_cast<QMenu*>(QApplication::activePopupWidget());
// //     if(m == this)
// //       PopupMenu::mouseReleaseEvent(e);
// //     else
// //     {
    
      if(activate)
      {
        DEBUG_PRST_ROUTES_2(stderr, "   activate true: directly executing trigger handler routePopupActivated(act_mwa)\n"); 
        // Directly execute the trigger handler.
        routePopupActivated(act_mwa);
      }
      
      // Close all the popups.
      closeUp();
// //     }
    return;
  }

  e->accept();
  if(activate)
  {
    DEBUG_PRST_ROUTES_2(stderr, "   activate true: directly executing trigger handler routePopupActivated(act_mwa)\n"); 
    routePopupActivated(act_mwa);
  }
  DEBUG_PRST_ROUTES_2(stderr, "RoutePopupMenu::mouseReleaseEvent end: this:%p active action:%p\n", this, activeAction()); 
}

void RoutePopupMenu::mousePressEvent(QMouseEvent* e)
{
  DEBUG_PRST_ROUTES_2(stderr, "RoutePopupMenu::mousePressEvent begin: this:%p active action:%p\n", this, activeAction()); 
//   e->ignore();
//   PopupMenu::mousePressEvent(e);
//   DEBUG_PRST_ROUTES_2(stderr, "RoutePopupMenu::mousePressEvent after begin: this:%p active action:%p\n", this, activeAction()); 
  
  DEBUG_PRST_ROUTES(stderr, "RoutePopupMenu::mousePressEvent this:%p x:%d y:%d\n", this, e->pos().x(), e->pos().y());

  RoutingMatrixWidgetAction* act_mwa = qobject_cast<RoutingMatrixWidgetAction*>(actionAt(e->pos()));
  
  bool accept = false;
  
  int ch_hit_clk_idx_min = -1;
  int ch_hit_clk_idx_max = -1;
  int ch_hit_clk_ch_start = -1;
  QActionGroup* ch_hit_clk_act_group = nullptr;

  const int sz = actions().size();
  for(int i = 0; i < sz; ++i)
  {
    if(RoutingMatrixWidgetAction* mwa = qobject_cast<RoutingMatrixWidgetAction*>(actions().at(i)))
    {
      bool do_upd = false;
      // Sanity check: Only activate the item(s) if the action truly is the active one.
      DEBUG_PRST_ROUTES_2(stderr, "RoutePopupMenu::mousePressEvent this:%p mwa:%p activeAction:%p\n", this, mwa, activeAction()); 
      //if(mwa == activeAction())
      if(mwa == act_mwa)
      {
        DEBUG_PRST_ROUTES_2(stderr, "  is active\n"); 
        RoutePopupHit hit = mwa->hitTest(e->pos(), RoutePopupHit::HitTestClick);
        switch(hit._type)
        {
          case RoutePopupHit::HitChannel:
          {
            // Support grouping together of channels.
            ch_hit_clk_idx_min = i;
            ch_hit_clk_idx_max = ch_hit_clk_idx_min + MusEGlobal::config.routerGroupingChannels;
            if(ch_hit_clk_idx_max > sz)
              ch_hit_clk_idx_min = sz - MusEGlobal::config.routerGroupingChannels;
            ch_hit_clk_ch_start = hit._value - (i - ch_hit_clk_idx_min);
            const int ch_diff = mwa->array()->columns() - (ch_hit_clk_ch_start + MusEGlobal::config.routerGroupingChannels);
            if(ch_diff < 0)
            {
              ch_hit_clk_idx_min += ch_diff;
              ch_hit_clk_idx_max += ch_diff;
              ch_hit_clk_ch_start += ch_diff;
            }
            // Set the pressed value. If the column was already active, update again 
            //  so that pressed colour overrides highlighted colour.
            if(mwa->array()->setPressedColumn(hit._value) || mwa->array()->activeColumn() == hit._value)
              do_upd = true;
            ch_hit_clk_act_group = mwa->actionGroup();
            DEBUG_PRST_ROUTES_2(stderr, "   HitChannel ch:%d active col:%d pressed col:%d\n", 
                             hit._value, mwa->array()->activeColumn(), mwa->array()->pressedColumn()); 
            accept = true;
          }
          break;
          
          case RoutePopupHit::HitMenuItem:
            if(mwa->setMenuItemPressed(true))
              do_upd = true;
            accept = true;
          break;
          
          case RoutePopupHit::HitChannelBar:
          case RoutePopupHit::HitSpace:
            if(mwa->setMenuItemPressed(false) || mwa->array()->setPressedColumn(-1))
              do_upd = true;
            accept = true;
          break;
          
          case RoutePopupHit::HitNone:
            if(mwa->setMenuItemPressed(false) || mwa->array()->setPressedColumn(-1))
              do_upd = true; // TODO Close the menu instead of letting QMenu do it (below)?
          break;
        }
      }
      if(do_upd)
        mwa->updateCreatedWidgets();
    }
  }

  for(int i = 0; i < sz; ++i)
  {
    if(RoutingMatrixWidgetAction* mwa = qobject_cast<RoutingMatrixWidgetAction*>(actions().at(i)))
    {
      bool do_upd = false;
      // Sanity check: Only activate the item(s) which are not the active one.
      //if(mwa != activeAction())
      if(mwa != act_mwa)
      {
        DEBUG_PRST_ROUTES_2(stderr, "RoutePopupMenu::mousePressEvent this:%p inactive mwa:%p\n", this, mwa); 
        if(ch_hit_clk_act_group && 
           !ch_hit_clk_act_group->isExclusive() && 
           ch_hit_clk_act_group == mwa->actionGroup() &&
           i >= ch_hit_clk_idx_min && 
           i < ch_hit_clk_idx_max)
        {
          if(mwa->array()->setPressedColumn(ch_hit_clk_ch_start + (i - ch_hit_clk_idx_min)))
            do_upd = true;
          DEBUG_PRST_ROUTES_2(stderr, "   ch:%d active col:%d pressed col:%d\n", 
                             ch_hit_clk_ch_start + (i - ch_hit_clk_idx_min), mwa->array()->activeColumn(), mwa->array()->pressedColumn()); 
        }
        else if(mwa->array()->setPressedColumn(-1))
          do_upd = true;
      }
      if(do_upd)
        mwa->updateCreatedWidgets();
    }
  }
  
  
  if(accept)
  {
// //     e->accept();
// //     PopupMenu::mousePressEvent(e);
// //     return;
  }

  DEBUG_PRST_ROUTES_2(stderr, "RoutePopupMenu::mousePressEvent before end: this:%p active action:%p\n", this, activeAction()); 
  e->ignore();
  PopupMenu::mousePressEvent(e);
  DEBUG_PRST_ROUTES_2(stderr, "RoutePopupMenu::mousePressEvent end: this:%p active action:%p\n", this, activeAction()); 
}

void RoutePopupMenu::mouseMoveEvent(QMouseEvent* e)
{
  DEBUG_PRST_ROUTES(stderr, "RoutePopupMenu::mouseMoveEvent this:%p\n", this);

  RoutingMatrixWidgetAction* act_mwa = qobject_cast<RoutingMatrixWidgetAction*>(actionAt(e->pos()));
  
  // Inform the hover handler that it was a mouse hover.
  _hoverIsFromMouse = true;
  // Ignore the event and pass it on. Let any new active action and hover signal be generated before the code below is run.
  e->ignore();
  PopupMenu::mouseMoveEvent(e);
  // Clear the flag.
  _hoverIsFromMouse = false;

  int ch_hit_hvr_idx_min = -1;
  int ch_hit_hvr_idx_max = -1;
  int ch_hit_hvr_ch_start = -1;
  QActionGroup* ch_hit_hvr_act_group = nullptr;

  int ch_hit_clk_idx_min = -1;
  int ch_hit_clk_idx_max = -1;
  int ch_hit_clk_ch_start = -1;
  QActionGroup* ch_hit_clk_act_group = nullptr;
  
  const int sz = actions().size();
  for(int i = 0; i < sz; ++i)
  {
    if(RoutingMatrixWidgetAction* mwa = qobject_cast<RoutingMatrixWidgetAction*>(actions().at(i)))
    {
      bool do_upd = false;
      // Sanity check: Only activate the item(s) if the action truly is the active one.
      //if(mwa == activeAction())
      if(mwa == act_mwa)
      {
        RoutePopupHit hit = mwa->hitTest(e->pos(), RoutePopupHit::HitTestHover);
        switch(hit._type)
        {
          case RoutePopupHit::HitChannel:
          {
            // Update the current 'last' hover info.
            _lastHoveredHit = hit;
            // Support grouping together of channels.
            ch_hit_hvr_idx_min = i;
            ch_hit_hvr_idx_max = ch_hit_hvr_idx_min + MusEGlobal::config.routerGroupingChannels;
            if(ch_hit_hvr_idx_max > sz)
              ch_hit_hvr_idx_min = sz - MusEGlobal::config.routerGroupingChannels;
            ch_hit_hvr_ch_start = hit._value - (i - ch_hit_hvr_idx_min);
            const int ch_diff = mwa->array()->columns() - (ch_hit_hvr_ch_start + MusEGlobal::config.routerGroupingChannels);
            if(ch_diff < 0)
            {
              ch_hit_hvr_idx_min += ch_diff;
              ch_hit_hvr_idx_max += ch_diff;
              ch_hit_hvr_ch_start += ch_diff;
            }
            // Set the values.
            if(mwa->isSelected())
            {
              mwa->setSelected(false);
              do_upd = true;
            }
            if(mwa->array()->activeColumn() != hit._value)
            {
              DEBUG_PRST_ROUTES(stderr, "   Setting active column:%d\n", hit._value);
              mwa->array()->setActiveColumn(hit._value);
              do_upd = true;
            }
            ch_hit_hvr_act_group = mwa->actionGroup();
          }
          break;
          
          case RoutePopupHit::HitMenuItem:
            // Update the current 'last' hover info.
            _lastHoveredHit = hit;
            if(!mwa->isSelected())
            {
              mwa->setSelected(true);
              do_upd = true;
            }
            if(mwa->array()->activeColumn() != -1)
            {
              mwa->array()->setActiveColumn(-1);
              do_upd = true;
            }
          break; 
          
          case RoutePopupHit::HitChannelBar:
          case RoutePopupHit::HitSpace:
          case RoutePopupHit::HitNone:
            if(mwa->isSelected())
            {
              mwa->setSelected(false);
              do_upd = true;
            }
            if(mwa->array()->activeColumn() != -1)
            {
              mwa->array()->setActiveColumn(-1);
              do_upd = true;
            }
          break;
        }
//       }
//       else
//       {
//         if(mwa->isSelected())
//         {
//           mwa->setSelected(false);
//           do_upd = true;
//         }
//         if(mwa->array()->activeColumn() != -1)
//         {
//           mwa->array()->setActiveColumn(-1);
//           do_upd = true;
//         }
//       }
        
        if(e->buttons() != Qt::NoButton)
        {
          RoutePopupHit hit = mwa->hitTest(e->pos(), RoutePopupHit::HitTestClick);
          switch(hit._type)
          {
            case RoutePopupHit::HitChannel:
            {
              // Support grouping together of channels.
              ch_hit_clk_idx_min = i;
              ch_hit_clk_idx_max = ch_hit_clk_idx_min + MusEGlobal::config.routerGroupingChannels;
              if(ch_hit_clk_idx_max > sz)
                ch_hit_clk_idx_min = sz - MusEGlobal::config.routerGroupingChannels;
              ch_hit_clk_ch_start = hit._value - (i - ch_hit_clk_idx_min);
              const int ch_diff = mwa->array()->columns() - (ch_hit_clk_ch_start + MusEGlobal::config.routerGroupingChannels);
              if(ch_diff < 0)
              {
                ch_hit_clk_idx_min += ch_diff;
                ch_hit_clk_idx_max += ch_diff;
                ch_hit_clk_ch_start += ch_diff;
              }
              // Set the value.
              if(mwa->array()->setPressedColumn(hit._value))
                do_upd = true;
              ch_hit_clk_act_group = mwa->actionGroup();
            }
            break;
            
            case RoutePopupHit::HitMenuItem:
              if(mwa->setMenuItemPressed(true))
                do_upd = true;
            break;
            
            case RoutePopupHit::HitChannelBar:
            case RoutePopupHit::HitSpace:
            case RoutePopupHit::HitNone:
              if(mwa->setMenuItemPressed(false) || mwa->array()->setPressedColumn(-1))
                do_upd = true;
            break;
          }
        }
      }
      if(do_upd)
        mwa->updateCreatedWidgets();
    }
  }

  
  for(int i = 0; i < sz; ++i)
  {
    if(RoutingMatrixWidgetAction* mwa = qobject_cast<RoutingMatrixWidgetAction*>(actions().at(i)))
    {
      bool do_upd = false;
      // Sanity check: Only activate the item(s) which are not the active one.
      //if(mwa != activeAction())
      if(mwa != act_mwa)
      {
        if(mwa->isSelected())
        {
          mwa->setSelected(false);
          do_upd = true;
        }
        
        if(ch_hit_hvr_act_group && 
           !ch_hit_hvr_act_group->isExclusive() && 
           ch_hit_hvr_act_group == mwa->actionGroup() &&
           i >= ch_hit_hvr_idx_min && i < ch_hit_hvr_idx_max)
        {
          const int ch = ch_hit_hvr_ch_start + (i - ch_hit_hvr_idx_min);
          if(mwa->array()->activeColumn() != ch)
          {
            DEBUG_PRST_ROUTES(stderr, "   Setting inactive column:%d\n", ch);
            mwa->array()->setActiveColumn(ch);
            do_upd = true;
          }
        }
        else if(mwa->array()->activeColumn() != -1)
        {
          mwa->array()->setActiveColumn(-1);
          do_upd = true;
        }
//       }

        if(e->buttons() != Qt::NoButton && 
           ch_hit_clk_act_group && 
           !ch_hit_clk_act_group->isExclusive() && 
           ch_hit_clk_act_group == mwa->actionGroup() &&          
           i >= ch_hit_clk_idx_min && 
           i < ch_hit_clk_idx_max)
        {
          if(mwa->array()->setPressedColumn(ch_hit_clk_ch_start + (i - ch_hit_clk_idx_min)))
            do_upd = true;
        }
        else if(mwa->array()->setPressedColumn(-1))
          do_upd = true;
      }
      
      if(do_upd)
        mwa->updateCreatedWidgets();
    }
  }
  
  
}

void RoutePopupMenu::routePopupHovered(QAction* action)
{  
   DEBUG_PRST_ROUTES(stderr, "RoutePopupMenu::popHovered this:%p action:%p _hoverIsFromMouse:%d text:%s\n", this, action, _hoverIsFromMouse, action->text().toLatin1().constData());
  
  // Ignore if this hover was from mouse.
  // Also, we get this hovered signal even if the hovered action is from another popup, so ignore it. 
  if(!_hoverIsFromMouse && actions().contains(action))
  {
    const int sz = actions().size();
    for(int i = 0; i < sz; ++i)
    {
      if(RoutingMatrixWidgetAction* mwa = qobject_cast<RoutingMatrixWidgetAction*>(actions().at(i)))
      {
        bool do_upd = false;
        if(mwa == action)
        {
          switch(_lastHoveredHit._type)
          {
            case RoutePopupHit::HitChannel:
            {
              if(mwa->isSelected())
              {
                mwa->setSelected(false);
                do_upd = true;
              }
              const int cols = mwa->array()->columns();
              if(cols != 0)
              {
                int col = _lastHoveredHit._value; // The column.
                if(col >= cols)
                {
                  col = cols - 1;  // Clip it.
                  _lastHoveredHit._value = col; // Adjust the current 'last' column setting.
                }
                if(mwa->array()->activeColumn() != col)
                {
                  mwa->array()->setActiveColumn(col);
                  do_upd = true;
                }
              }
            }
            break;
            
            case RoutePopupHit::HitMenuItem:
              if(mwa->hasCheckBox() && !mwa->isSelected())
              {
                mwa->setSelected(true);
                do_upd = true;
              }
              if(mwa->array()->activeColumn() != -1)
              {
                mwa->array()->setActiveColumn(-1);
                do_upd = true;
              }
            break; 
            
            case RoutePopupHit::HitChannelBar:
            case RoutePopupHit::HitSpace:
            case RoutePopupHit::HitNone:
              // If it has a checkbox (or there is no channel bar) select the checkbox/text area.
              if(mwa->hasCheckBox() || mwa->array()->columns() == 0)
              {
                // Update the current 'last' hover info.
                _lastHoveredHit._type = RoutePopupHit::HitMenuItem;
                _lastHoveredHit._action = mwa;
                _lastHoveredHit._value = 0;
                if(!mwa->isSelected())
                {
                  mwa->setSelected(true);
                  do_upd = true;
                }
              }
              // Otherwise select the first available channel bar column.
              else
              {
                // Update the current 'last' hover info.
                _lastHoveredHit._type = RoutePopupHit::HitChannel;
                _lastHoveredHit._action = mwa;
                _lastHoveredHit._value = 0;
                if(mwa->array()->activeColumn() != 0)
                {
                  mwa->array()->setActiveColumn(0);
                  do_upd = true;
                }
              }
            break;
          }
        }
        else
        {
          if(mwa->isSelected())
          {
            mwa->setSelected(false); // Reset the checkbox/text active area.
            do_upd = true;
          }
          if(mwa->array()->activeColumn() != -1)
          {
            mwa->array()->setActiveColumn(-1); // Reset any active column.
            do_upd = true;
          }
        }
        
        if(do_upd)
          mwa->updateCreatedWidgets();
      }
    }
  }

  // Clear the flag.
  //_hoverIsFromMouse = false;
}

void RoutePopupMenu::keyPressEvent(QKeyEvent* e)
{
  if(activeAction())
  {
    if(RoutingMatrixWidgetAction* mwa = qobject_cast<RoutingMatrixWidgetAction*>(activeAction()))
    {
      bool do_upd = false;
      bool key_accepted = false;
      const int key = e->key();
      switch(key)
      {
        case Qt::Key_Left:
        {
          switch(_lastHoveredHit._type)
          {
            case RoutePopupHit::HitMenuItem:
            // Allow the menu to close.
            break;
            
            case RoutePopupHit::HitChannel:
              // If we're on the first available channel and there's no checkbox, allow the menu to close.
              if(_lastHoveredHit._value == 0 && !mwa->hasCheckBox())
                break;
            // Fall through.  
            case RoutePopupHit::HitChannelBar:
            case RoutePopupHit::HitSpace:
            case RoutePopupHit::HitNone:
            {
              // Get the next available item to the left.
              RoutePopupHit hit = mwa->previousHit(_lastHoveredHit);
              switch(hit._type)
              {
                case RoutePopupHit::HitChannel:
                {
                  if(mwa->isSelected())
                  {
                    mwa->setSelected(false);
                    do_upd = true;
                  }
                  if(mwa->array()->activeColumn() != hit._value)
                  {
                    mwa->array()->setActiveColumn(hit._value);
                    do_upd = true;
                  }
                  // Update the current 'last' hover info.
                  _lastHoveredHit = hit;
                  key_accepted = true;
                }
                break;
                
                case RoutePopupHit::HitMenuItem:
                  if(!mwa->isSelected())
                  {
                    mwa->setSelected(true);
                    do_upd = true;
                  }
                  if(mwa->array()->activeColumn() != -1)
                  {
                    mwa->array()->setActiveColumn(-1);
                    do_upd = true;
                  }
                  // Update the current 'last' hover info.
                  _lastHoveredHit = hit;
                  key_accepted = true;
                break; 
                
                case RoutePopupHit::HitChannelBar:
                case RoutePopupHit::HitSpace:
                case RoutePopupHit::HitNone:
                  if(mwa->isSelected())
                  {
                    mwa->setSelected(false);
                    do_upd = true;
                  }
                  if(mwa->array()->activeColumn() != -1)
                  {
                    mwa->array()->setActiveColumn(-1);
                    do_upd = true;
                  }
                  // Update the current 'last' hover info.
                  _lastHoveredHit = hit;
                  key_accepted = true;
                break;
              }              
            }
            break;
          }
        }  
        break;
        
        case Qt::Key_Right:
        {
          switch(_lastHoveredHit._type)
          {
            case RoutePopupHit::HitChannel:
              // If we're on the last available channel, allow any submenu to open.
              if(mwa->array()->columns() != 0 && _lastHoveredHit._value == mwa->array()->columns() - 1)
                break;
            // Fall through.  
            case RoutePopupHit::HitMenuItem:
            case RoutePopupHit::HitChannelBar:
            case RoutePopupHit::HitSpace:
            case RoutePopupHit::HitNone:
            {
              // Get the next available item to the right.
              RoutePopupHit hit = mwa->nextHit(_lastHoveredHit);
              switch(hit._type)
              {
                case RoutePopupHit::HitChannel:
                {
                  if(mwa->isSelected())
                  {
                    mwa->setSelected(false);
                    do_upd = true;
                  }
                  if(mwa->array()->activeColumn() != hit._value)
                  {
                    mwa->array()->setActiveColumn(hit._value);
                    do_upd = true;
                  }
                  // Update the current 'last' hover info.
                  _lastHoveredHit = hit;
                  key_accepted = true;
                }
                break;
                
                case RoutePopupHit::HitMenuItem:
                  if(!mwa->isSelected())
                  {
                    mwa->setSelected(true);
                    do_upd = true;
                  }
                  if(mwa->array()->activeColumn() != -1)
                  {
                    mwa->array()->setActiveColumn(-1);
                    do_upd = true;
                  }
                  // Update the current 'last' hover info.
                  _lastHoveredHit = hit;
                  key_accepted = true;
                break; 
                
                case RoutePopupHit::HitChannelBar:
                case RoutePopupHit::HitSpace:
                case RoutePopupHit::HitNone:
                  if(mwa->isSelected())
                  {
                    mwa->setSelected(false);
                    do_upd = true;
                  }
                  if(mwa->array()->activeColumn() != -1)
                  {
                    mwa->array()->setActiveColumn(-1);
                    do_upd = true;
                  }
                  // Update the current 'last' hover info.
                  _lastHoveredHit = hit;
                    key_accepted = true;
                break;
              }
            }
          }
        }
        break;
        
        default:
        break;
      }
      
      if(do_upd)
        mwa->updateCreatedWidgets();
      if(key_accepted)
      {
        e->accept();
        return;
      }
    }
  }
  
  e->ignore();
  PopupMenu::keyPressEvent(e);
}

void RoutePopupMenu::songChanged(MusECore::SongChangedStruct_t val)
{
  DEBUG_PRST_ROUTES(stderr, "RoutePopupMenu::songChanged flags:%ld", (long int)val._flags);
  if(val & (SC_ROUTE | SC_CHANNELS | SC_CONFIG))
    updateRouteMenus();
  if(val & SC_PORT_ALIAS_PREFERENCE)
    preferredPortAliasChanged();
  if(val & SC_ROUTER_CHANNEL_GROUPING)
    routerChannelGroupingChanged();
}

bool RoutePopupMenu::updateItemTexts(PopupMenu* menu)
{
  if(!menu)
    menu = this;
  QList<QAction*> list = menu->actions();
  const int sz = list.size();
  bool changed = false;
  for(int i = 0; i < sz; ++i)
  {
    QAction* act = list.at(i);
    if(RoutingMatrixWidgetAction* wa = qobject_cast<RoutingMatrixWidgetAction*>(act))
    {
      // Take care of struct Route first. Insert other future custom structures here too !
      if(act->data().canConvert<MusECore::Route>())
      {
        const MusECore::Route r = act->data().value<MusECore::Route>();
        switch(r.type)
        {
          case MusECore::Route::JACK_ROUTE:
          {
            if(MusEGlobal::checkAudioDevice())
            {
              char good_name[ROUTE_PERSISTENT_NAME_SIZE];
              const char* port_name = r.persistentJackPortName;
              void* const port = MusEGlobal::audioDevice->findPort(port_name);
              if(port)
              {
                MusEGlobal::audioDevice->portName(port, good_name, ROUTE_PERSISTENT_NAME_SIZE, MusEGlobal::config.preferredRouteNameOrAlias);
                const QString str(good_name);
                if(wa->actionText() != str)
                {
                  wa->setActionText(str);
                  changed = true;
                }
              }
              if(changed)
              {
//                 wa->updateChannelArray();
              }
            }
          }
          break;
          
          case MusECore::Route::TRACK_ROUTE:
          case MusECore::Route::MIDI_DEVICE_ROUTE:
          case MusECore::Route::MIDI_PORT_ROUTE:
          break;
        }
      }
      
      
    }
    else
    // Take care of struct Route first. Insert other future custom structures here too !
    if(act->data().canConvert<MusECore::Route>())
    {
      const MusECore::Route r = act->data().value<MusECore::Route>();
      switch(r.type)
      {
        case MusECore::Route::JACK_ROUTE:
          act->setText(r.displayName(MusEGlobal::config.preferredRouteNameOrAlias));
        break;
        
        case MusECore::Route::TRACK_ROUTE:
        case MusECore::Route::MIDI_DEVICE_ROUTE:
        case MusECore::Route::MIDI_PORT_ROUTE:
        break;
      }
    }
  }
  
  return changed;
}

// Updates item texts and the 'preferred alias action'. Returns true if any action was changed.
bool RoutePopupMenu::preferredPortAliasChanged()
{
  QList<QAction*> list = actions();
  const int sz = list.size();
  bool changed = false;
  for(int i = 0; i < sz; ++i)
  {
    QAction* act = list.at(i);
    // Check for custom widget action.
    if(RoutingMatrixWidgetAction* wa = qobject_cast<RoutingMatrixWidgetAction*>(act))
    {
      // Check for Route data type.
      // Take care of struct Route first. Add other future custom structures here too.
      if(act->data().canConvert<MusECore::Route>())
      {
        const MusECore::Route r = act->data().value<MusECore::Route>();
        switch(r.type)
        {
          case MusECore::Route::JACK_ROUTE:
          {
            if(MusEGlobal::checkAudioDevice())
            {
              char good_name[ROUTE_PERSISTENT_NAME_SIZE];
              const char* port_name = r.persistentJackPortName;
              void* const port = MusEGlobal::audioDevice->findPort(port_name);
              if(port)
              {
                MusEGlobal::audioDevice->portName(port, good_name, ROUTE_PERSISTENT_NAME_SIZE, MusEGlobal::config.preferredRouteNameOrAlias);
                const QString str(good_name);
                if(wa->actionText() != str)
                {
                  wa->setActionText(str);
                  changed = true;
                }
              }
            }
          }
          break;
          
          case MusECore::Route::TRACK_ROUTE:
          case MusECore::Route::MIDI_DEVICE_ROUTE:
          case MusECore::Route::MIDI_PORT_ROUTE:
          break;
        }
      }
      // No Route data type. Check for int data IDs.
      // Handle future data types above, before this in case those types might be convertible to int.
      else
      {
        bool ok = false;
        const int n = act->data().toInt(&ok);
        if(ok)
        {
          switch(n)
          {
  #ifdef _USE_CUSTOM_WIDGET_ACTIONS_
            // Check for the 'preferred port alias' action.
            case _ALIASES_WIDGET_ACTION_:
            {
              int v; 
              if(wa->array()->value(0))
                v = MusEGlobal::RoutePreferFirstAlias;
              else if(wa->array()->value(1))
                v = MusEGlobal::RoutePreferSecondAlias;
              else 
                v = MusEGlobal::RoutePreferCanonicalName;
              
              if(v != MusEGlobal::config.preferredRouteNameOrAlias)
              {
                DEBUG_PRST_ROUTES(stderr, "RoutePopupMenu::preferredPortAliasChanged setting alias array preferredRouteNameOrAlias:%d\n", 
                        MusEGlobal::config.preferredRouteNameOrAlias);
                switch(MusEGlobal::config.preferredRouteNameOrAlias)
                {
                  case MusEGlobal::RoutePreferFirstAlias:
                    wa->array()->setValue(0, true);
                  break;
                  case MusEGlobal::RoutePreferSecondAlias:
                    wa->array()->setValue(1, true);
                  break;
                  case MusEGlobal::RoutePreferCanonicalName:
                    // Just set any column to false to clear this exclusive array.
                    wa->array()->setValue(0, false);
                  break;
                }
                changed = true;
              }
            }
            break;
  #endif
            
            default:
            break;
          }
        }
      }
    }
    // Not a custom widget action. Check for Route data type.
    // Take care of struct Route first. Add other future custom structures here too.
    else if(act->data().canConvert<MusECore::Route>())
    {
      const MusECore::Route r = act->data().value<MusECore::Route>();
      switch(r.type)
      {
        case MusECore::Route::JACK_ROUTE:
        {
          const QString rname = r.displayName(MusEGlobal::config.preferredRouteNameOrAlias);
          if(act->text() != rname)
          {
            act->setText(rname);
            changed = true;
          }
        }
        break;
        
        case MusECore::Route::TRACK_ROUTE:
        case MusECore::Route::MIDI_DEVICE_ROUTE:
        case MusECore::Route::MIDI_PORT_ROUTE:
        break;
      }
    }
  }
  
  return changed;
}

bool RoutePopupMenu::routerChannelGroupingChanged()
{
  QList<QAction*> list = actions();
  const int sz = list.size();
  bool changed = false;
  for(int i = 0; i < sz; ++i)
  {
    QAction* act = list.at(i);
    // Check for custom widget action.
    if(RoutingMatrixWidgetAction* wa = qobject_cast<RoutingMatrixWidgetAction*>(act))
    {
      // Check for Route data type.
      // Take care of struct Route first. Add other future custom structures here too.
      if(act->data().canConvert<MusECore::Route>())
      {
        // Nothing to do here yet.
      }
      // No Route data type. Check for int data IDs.
      // Handle future data types above, before this in case those types might be convertible to int.
      else
      {
        bool ok = false;
        const int n = act->data().toInt(&ok);
        if(ok)
        {
          switch(n)
          {
  #ifdef _USE_CUSTOM_WIDGET_ACTIONS_
            // Check for the 'grouping channels' action.
            case _GROUPING_CHANNELS_WIDGET_ACTION_:
            {
              int v; 
              if(wa->array()->value(0))
                v = 1;
              else 
                v = 2;
              
              if(v != MusEGlobal::config.routerGroupingChannels)
              {
                DEBUG_PRST_ROUTES(stderr, "RoutePopupMenu::routerChannelGroupingChanged setting array routerGroupingChannels:%d\n", 
                        MusEGlobal::config.routerGroupingChannels);
                switch(MusEGlobal::config.routerGroupingChannels)
                {
                  case 1:
                    wa->array()->setValue(0, true);
                    changed = true;
                  break;
                  case 2:
                    wa->array()->setValue(1, true);
                    changed = true;
                  break;
                  default:
                  break;
                }
              }
            }
            break;
  #endif
            
            default:
            break;
          }
        }
      }
    }
    // Not a custom widget action. Check for Route data type.
    // Take care of struct Route first. Add other future custom structures here too.
    else if(act->data().canConvert<MusECore::Route>())
    {
      // Nothing to do here yet.
    }
  }
  return changed;
}

PopupMenu* RoutePopupMenu::cloneMenu(const QString& title, QWidget* parent, bool /*stayOpen*/, bool showTooltips)
{
  PopupMenu* m = new RoutePopupMenu(_route, title, parent, _isOutMenu, _broadcastChanges);
  m->setToolTipsVisible(showTooltips);
  return m;
}

void RoutePopupMenu::updateRouteMenus()    
{
  // NOTE: The purpose of this routine is to make sure the items actually reflect
  //  the routing status. 
  // In case for some reason a route could not be added (or removed). 
  // Then the item will be properly un-checked (or checked) here.
  
  // TODO Fix this up a bit. It doesn't quite respond to complete removal, and other situations are a bit odd.
  //      Best to ignore it for now since it only half-works.    p4.0.42
  
/*
  //printf("RoutePopupMenu::updateRouteMenus\n");  
  
  if(!_track || actions().isEmpty() || !isVisible())  
    return;
    
  MusECore::RouteList* rl = _isOutMenu ? _track->outRoutes() : _track->inRoutes();

  // Clear all the action check marks.
  clearAllChecks();    
    
  // Take care of Midi Port to Audio Input routes first...  
  if(_isOutMenu && _track->isMidiTrack())
  {
    int port = ((MusECore::MidiTrack*)_track)->outPort();
    if(port >= 0 && port < MIDI_PORTS)
    {
      MusECore::MidiPort* mp = &MusEGlobal::midiPorts[port];
      MusECore::RouteList* mprl = mp->outRoutes();
      for (MusECore::ciRoute ir = mprl->begin(); ir != mprl->end(); ++ir) 
      {
        if(ir->type == MusECore::Route::TRACK_ROUTE && ir->track && ir->track->type() == MusECore::Track::AUDIO_INPUT)
        {
          for(int ch = 0; ch < MusECore::MUSE_MIDI_CHANNELS; ++ch)
          {
            int chbits = 1 << ch;
            if(ir->channel & chbits)
            {
              MusECore::Route r(ir->track, chbits);
              //printf("RoutePopupMenu::updateRouteMenus MusECore::MidiPort to AudioInput chbits:%d\n", chbits);  // 
              QAction* act = findActionFromData(QVariant::fromValue(r));  
              if(act)
              {  
                //printf("  ... Found\n");  // 
                act->setChecked(true);
              }  
            }  
          }  
        }  
      }  
    }
  }

  // Now check the ones that are found in the route list.
  for(MusECore::ciRoute irl = rl->begin(); irl != rl->end(); ++irl) 
  {
    // Do MidiTrack to MidiPort routes...
    if(irl->type == MusECore::Route::MIDI_PORT_ROUTE)
    {
      
#ifdef _USE_CUSTOM_WIDGET_ACTIONS_      
      
      // Widget action handles channels. Look for route with channels ignored and set to zero. 
      MusECore::Route r(irl->midiPort, 0);
      QAction* act = findActionFromData(QVariant::fromValue(r));  
      if(act)
      {  
        //printf("RoutePopupMenu::updateRouteMenus found MidiTrack to MidiPort irl type:%d\n", irl->type);  // 
        // Check for custom widget actions first.
        PixmapButtonsWidgetAction* mc_wa = dynamic_cast<PixmapButtonsWidgetAction*>(act);
        if(mc_wa)
        {  
          //printf("  ... Found custom, setting current state\n");  // 
          mc_wa->setCurrentState(irl->channel);  // Set all channels at once.
        }  
      }
      
#else
      //printf("RoutePopupMenu::updateRouteMenus MIDI_PORT_ROUTE\n");  
      for(int ch = 0; ch < MusECore::MUSE_MIDI_CHANNELS; ++ch)
      {
        int chbits = 1 << ch;
        if(irl->channel & chbits)
        {
          MusECore::Route r(irl->midiPort, chbits);
          //printf("RoutePopupMenu::updateRouteMenus MidiTrack to MidiPort irl type:%d\n", irl->type);  // 
          // If act is a PixmapButtonsWidgetAction, route channel is ignored and is zero.
          QAction* act = findActionFromData(QVariant::fromValue(r));  
          if(act)
          {  
            //printf("  ... Found\n");  // 
            act->setChecked(true);
          }  
        }    
      }    
#endif // _USE_CUSTOM_WIDGET_ACTIONS_

    }
    else
    // Do all other routes...
    {
      
#ifdef _USE_CUSTOM_WIDGET_ACTIONS_      
      
      // Do MidiPort to MidiTrack routes...
      if(irl->type == MusECore::Route::TRACK_ROUTE && irl->track && irl->track->type() == MusECore::Track::AUDIO_INPUT)
      {
        // Widget action handles channels. Look for route with channels ignored and set to zero. 
        MusECore::Route r(irl->track, 0);
        QAction* act = findActionFromData(QVariant::fromValue(r));  
        if(act)
        {  
          // Check for custom widget actions first.
          PixmapButtonsWidgetAction* mc_wa = dynamic_cast<PixmapButtonsWidgetAction*>(act);
          if(mc_wa)
          {  
            //printf("RoutePopupMenu::updateRouteMenus found custom irl type:%d\n", irl->type);  // 
            mc_wa->setCurrentState(irl->channel);  // Set all channels at once.
            continue;
          }  
        }
      }

#endif // _USE_CUSTOM_WIDGET_ACTIONS_
      
      //printf("RoutePopupMenu::updateRouteMenus other irl type:%d\n", irl->type);
      if(act)
      {  
        //printf("RoutePopupMenu::updateRouteMenus found other irl type:%d\n", irl->type);  // 
        act->setChecked(true);
      }
    }
  }
*/  
}      

void RoutePopupMenu::trackRouteActivated(QAction* action, MusECore::Route& rem_route, MusECore::PendingOperationList& operations)
{
  // Check for custom routing matrix action.
  RoutingMatrixWidgetAction* matrix_wa = qobject_cast<RoutingMatrixWidgetAction*>(action);
  if(!matrix_wa)
    return;
  
  switch(rem_route.type)
  {
    case MusECore::Route::TRACK_ROUTE:
    {
      // Make sure the track still exists.
      if(MusEGlobal::song->tracks()->find(rem_route.track) == MusEGlobal::song->tracks()->end())
        return;
      
      DEBUG_PRST_ROUTES(stderr, "RoutePopupMenu::trackRouteActivated:\n");

      MusECore::Track* track = _route.track;

      MusECore::TrackList* tracks = MusEGlobal::song->tracks();
      for(MusECore::iTrack it = tracks->begin(); it != tracks->end(); ++it)
      {
        MusECore::Track* t = *it;
        // Track types must be same.
        if((track->isMidiTrack() && !t->isMidiTrack()) || (t->type() != track->type()))
          continue;
        // We are looking for the given track alone if unselected, or else all selected tracks.
        // Ignore other tracks if broadcasting changes is disabled.
        if(t != track && (!_broadcastChanges || !t->selected() || !track->selected()))
          continue;


        const int cols = matrix_wa->array()->columns();
        for(int col = 0; col < cols; ++col)
        {
          MusECore::Route this_route(t, col, 1);
          rem_route.channels = 1;

          const MusECore::Route& src = _isOutMenu ? this_route : rem_route;
          const MusECore::Route& dst = _isOutMenu ? rem_route : this_route;

          DEBUG_PRST_ROUTES(stderr, "RoutePopupMenu::trackRouteActivated: checking operations\n");
          const bool val = matrix_wa->array()->value(col);
          // Connect if route does not exist. Allow it to reconnect a partial route.
          if(val && MusECore::routeCanConnect(src, dst))
          {
            DEBUG_PRST_ROUTES(stderr, "RoutePopupMenu::trackRouteActivated: adding AddRoute operation\n");
            operations.add(MusECore::PendingOperationItem(src, dst, MusECore::PendingOperationItem::AddRoute));
          }
          // Disconnect if route exists. Allow it to reconnect a partial route.
          else if(!val && MusECore::routeCanDisconnect(src, dst))
          {
            DEBUG_PRST_ROUTES(stderr, "RoutePopupMenu::trackRouteActivated: adding DeleteRoute operation\n");
            operations.add(MusECore::PendingOperationItem(src, dst, MusECore::PendingOperationItem::DeleteRoute));
          }
        }
      }
    } 
    break;  
    
    case MusECore::Route::JACK_ROUTE:
    case MusECore::Route::MIDI_DEVICE_ROUTE:
    case MusECore::Route::MIDI_PORT_ROUTE:
      return;
    break;  
  }
}

void RoutePopupMenu::jackRouteActivated(QAction* action, const MusECore::Route& route, const MusECore::Route& rem_route, MusECore::PendingOperationList& operations)
{
  // Check for custom routing matrix action.
  RoutingMatrixWidgetAction* matrix_wa = qobject_cast<RoutingMatrixWidgetAction*>(action);
  if(!matrix_wa)
    return;
  
  if(!MusEGlobal::checkAudioDevice())
    return;

  DEBUG_PRST_ROUTES(stderr, "RoutePopupMenu::jackRouteActivated: Matrix\n");

  const int cols = matrix_wa->array()->columns();
  const char* const port_name = rem_route.persistentJackPortName;
  void* const port = MusEGlobal::audioDevice->findPort(port_name);
  if(port)
  {
    for(int col = 0; col < cols; ++col)
    {
      MusECore::Route this_route(route);
      switch(route.type)
      {
        case MusECore::Route::MIDI_DEVICE_ROUTE:
          this_route.channel = -1;
        break;

        case MusECore::Route::TRACK_ROUTE:
        {
          this_route.channel = col;

          MusECore::Track* track = route.track;
          if(track)
          {
            MusECore::TrackList* tracks = MusEGlobal::song->tracks();
            for(MusECore::iTrack it = tracks->begin(); it != tracks->end(); ++it)
            {
              MusECore::Track* t = *it;
              // Track types must be same.
              if((track->isMidiTrack() && !t->isMidiTrack()) || (t->type() != track->type()))
                continue;
              // We are looking for the given track alone if unselected, or else all selected tracks.
              // Ignore other tracks if broadcasting changes is disabled.
              if(t != track && (!_broadcastChanges || !t->selected() || !track->selected()))
                continue;

              this_route.track = t;
              // TODO Lazy identical copy of code below. Streamline somehow...
              const MusECore::Route r_route(port);
              const MusECore::Route& src = _isOutMenu ? this_route : r_route;
              const MusECore::Route& dst = _isOutMenu ? r_route : this_route;
              const bool val = matrix_wa->array()->value(col);
              DEBUG_PRST_ROUTES_2(stderr, "RoutePopupMenu::jackRouteActivated: checking operations col:%d val:%d\n", col, val);
              // Connect if route does not exist. Allow it to reconnect a partial route.
              if(val && MusECore::routeCanConnect(src, dst))
              {
                DEBUG_PRST_ROUTES_2(stderr, "RoutePopupMenu::jackRouteActivated: adding AddRoute operation\n");
                operations.add(MusECore::PendingOperationItem(src, dst, MusECore::PendingOperationItem::AddRoute));
              }
              // Disconnect if route exists. Allow it to reconnect a partial route.
              else if(!val && MusECore::routeCanDisconnect(src, dst))
              {
                DEBUG_PRST_ROUTES_2(stderr, "RoutePopupMenu::jackRouteActivated: adding DeleteRoute operation\n");
                operations.add(MusECore::PendingOperationItem(src, dst, MusECore::PendingOperationItem::DeleteRoute));
              }

            }
          }
          // We took care of it. Continue with the next column.
          continue;

        }
        break;

        case MusECore::Route::MIDI_PORT_ROUTE:
          if(route.midiPort == -1)
            return;
          if(MusECore::MidiDevice* md = MusEGlobal::midiPorts[route.midiPort].device())
          {
            this_route.type = MusECore::Route::MIDI_DEVICE_ROUTE;
            this_route.device = md;
            this_route.channel = -1;
          }
          else
            return;
        break;

        case MusECore::Route::JACK_ROUTE:
        break;
      }

      const MusECore::Route r_route(port);
      const MusECore::Route& src = _isOutMenu ? this_route : r_route;
      const MusECore::Route& dst = _isOutMenu ? r_route : this_route;

      const bool val = matrix_wa->array()->value(col);
      DEBUG_PRST_ROUTES_2(stderr, "RoutePopupMenu::jackRouteActivated: checking operations col:%d val:%d\n", col, val);
      // Connect if route does not exist. Allow it to reconnect a partial route.
      if(val && MusECore::routeCanConnect(src, dst))
      {
        DEBUG_PRST_ROUTES_2(stderr, "RoutePopupMenu::jackRouteActivated: adding AddRoute operation\n");
        operations.add(MusECore::PendingOperationItem(src, dst, MusECore::PendingOperationItem::AddRoute));
      }
      // Disconnect if route exists. Allow it to reconnect a partial route.
      else if(!val && MusECore::routeCanDisconnect(src, dst))
      {
        DEBUG_PRST_ROUTES_2(stderr, "RoutePopupMenu::jackRouteActivated: adding DeleteRoute operation\n");
        operations.add(MusECore::PendingOperationItem(src, dst, MusECore::PendingOperationItem::DeleteRoute));
      }
    }
  }
}

void RoutePopupMenu::audioTrackPopupActivated(QAction* action, MusECore::Route& rem_route, MusECore::PendingOperationList& operations)
{
  DEBUG_PRST_ROUTES(stderr, "RoutePopupMenu::audioTrackPopupActivated: action text:%s checked:%d name:%s\n", 
          action->text().toLatin1().constData(), action->isChecked(), 
          action->objectName().toLatin1().constData());
  
  MusECore::Track* track = _route.track;
  // Check for custom routing matrix action.
  RoutingMatrixWidgetAction* matrix_wa = qobject_cast<RoutingMatrixWidgetAction*>(action);
  if(matrix_wa)
  {
    DEBUG_PRST_ROUTES(stderr, "RoutePopupMenu::audioTrackPopupActivated: Matrix\n");
    
    switch(rem_route.type)
    {
      case MusECore::Route::JACK_ROUTE:
        jackRouteActivated(action, _route, rem_route, operations);
      break;
      
      case MusECore::Route::TRACK_ROUTE:
        trackRouteActivated(action, rem_route, operations);
      break;
      
      case MusECore::Route::MIDI_DEVICE_ROUTE:
      break;  
      case MusECore::Route::MIDI_PORT_ROUTE:
      break;  
    }
  }
#ifndef _USE_SIMPLIFIED_SOLO_CHAIN_
  // Support Midi Port to Audio Input routes. 
  else if(!_isOutMenu && track->type() == MusECore::Track::AUDIO_INPUT && rem_route.type == MusECore::Route::MIDI_PORT_ROUTE)
  {
    MusECore::TrackList* tracks = MusEGlobal::song->tracks();
    for(MusECore::iTrack it = tracks->begin(); it != tracks->end(); ++it)
    {
      MusECore::Track* t = *it;
      // Track types must be same.
      if((track->isMidiTrack() && !t->isMidiTrack()) || (t->type() != track->type()))
        continue;
      // We are looking for the given track alone if unselected, or else all selected tracks.
      // Ignore other tracks if broadcasting changes is disabled.
      if(t != track && (!_broadcastChanges || !t->selected() || !track->selected()))
        continue;

      MusECore::RouteList* rl = _isOutMenu ? t->outRoutes() : t->inRoutes();
      // Check for custom midi channel select action.
      PixmapButtonsWidgetAction* cs_wa = qobject_cast<PixmapButtonsWidgetAction*>(action);
      if(cs_wa)
      {
        const QBitArray ba = cs_wa->currentState();
        const int ba_sz = ba.size();
        int chbits = 0;
        for(int mch = 0; mch < MusECore::MUSE_MIDI_CHANNELS && mch < ba_sz; ++mch)
        {
          if(ba.at(mch))
            chbits |= (1 << mch);
        }

        rem_route.channel = chbits;  // Restore the desired route channels from the custom widget action state.
        int mdidx = rem_route.midiPort;

        int chmask = 0;
        MusECore::ciRoute iir = rl->begin();
        for (; iir != rl->end(); ++iir)
          if(iir->type == MusECore::Route::MIDI_PORT_ROUTE && iir->midiPort == mdidx) {   // Is there already a route to this port?
            chmask = iir->channel;  // Grab the channel mask.
            break;
          }

        // Only if something changed...
        if(chmask != chbits)
        {
          if(chmask != 0)
          {
            // Disconnect all existing channels.
            MusECore::Route dstRoute(t, chmask);
            operations.add(MusECore::PendingOperationItem(*iir, dstRoute, MusECore::PendingOperationItem::DeleteRoute));
          }
          if(chbits != 0)
          {
            // Connect desired channels.
            MusECore::Route dstRoute(t, chbits);
            operations.add(MusECore::PendingOperationItem(rem_route, dstRoute, MusECore::PendingOperationItem::AddRoute));
          }
        }
      }
      else
      {
        int chbit = rem_route.channel;
        MusECore::Route dstRoute(t, chbit);
        int mdidx = rem_route.midiPort;
        int chmask = 0;
        MusECore::ciRoute iir = rl->begin();
        for (; iir != rl->end(); ++iir)
        {
          if(iir->type == MusECore::Route::MIDI_PORT_ROUTE && iir->midiPort == mdidx)    // Is there already a route to this port?
          {
            chmask = iir->channel;  // Grab the channel mask.
            break;
          }
        }

        if ((chmask & chbit) == chbit)             // Is the channel's bit(s) set?
          operations.add(MusECore::PendingOperationItem(rem_route, dstRoute, MusECore::PendingOperationItem::DeleteRoute));
        else
          operations.add(MusECore::PendingOperationItem(rem_route, dstRoute, MusECore::PendingOperationItem::AddRoute));
      }
    }
  }
#endif // _USE_SIMPLIFIED_SOLO_CHAIN_  
  else
  {
    // Make sure the track still exists.
    if(MusEGlobal::song->tracks()->find(rem_route.track) == MusEGlobal::song->tracks()->end())
      return;
    
    MusECore::TrackList* tracks = MusEGlobal::song->tracks();
    for(MusECore::iTrack it = tracks->begin(); it != tracks->end(); ++it)
    {
      MusECore::Track* t = *it;
      // Track types must be same.
      if((track->isMidiTrack() && !t->isMidiTrack()) || (t->type() != track->type()))
        continue;
      // We are looking for the given track alone if unselected, or else all selected tracks.
      // Ignore other tracks if broadcasting changes is disabled.
      if(t != track && (!_broadcastChanges || !t->selected() || !track->selected()))
        continue;


      MusECore::Route this_route(t, rem_route.channel, rem_route.channels);
      this_route.remoteChannel = rem_route.remoteChannel;

      const MusECore::Route& src = _isOutMenu ? this_route : rem_route;
      const MusECore::Route& dst = _isOutMenu ? rem_route : this_route;

      // Connect if route does not exist. Allow it to reconnect a partial route.
      if(action->isChecked() && MusECore::routeCanConnect(src, dst))
      {
        DEBUG_PRST_ROUTES(stderr, "RoutePopupMenu::audioTrackPopupActivated: Route: adding AddRoute operation\n");
        operations.add(MusECore::PendingOperationItem(src, dst, MusECore::PendingOperationItem::AddRoute));
      }
      // Disconnect if route exists. Allow it to reconnect a partial route.
      else if(!action->isChecked() && MusECore::routeCanDisconnect(src, dst))
      {
        DEBUG_PRST_ROUTES(stderr, "RoutePopupMenu::audioTrackPopupActivated: Route: adding DeleteRoute operation\n");
        operations.add(MusECore::PendingOperationItem(src, dst, MusECore::PendingOperationItem::DeleteRoute));
      }
    }
  }
}

void RoutePopupMenu::midiTrackPopupActivated(QAction* action, MusECore::Route& rem_route, MusECore::PendingOperationList& operations)
{
  DEBUG_PRST_ROUTES(stderr, "RoutePopupMenu::midiTrackPopupActivated: action text:%s checked:%d name:%s\n", 
          action->text().toLatin1().constData(), action->isChecked(),
          action->objectName().toLatin1().constData());
  
  MusECore::Track* track = _route.track;
  if(rem_route.type == MusECore::Route::TRACK_ROUTE && rem_route.track &&
    // Make sure the track still exists.
    MusEGlobal::song->tracks()->find(rem_route.track) != MusEGlobal::song->tracks()->end() &&
    rem_route.track->type() == MusECore::Track::AUDIO_INPUT)
  {

    MusECore::MidiTrackList* tracks = MusEGlobal::song->midis();
    for(MusECore::iMidiTrack it = tracks->begin(); it != tracks->end(); ++it)
    {
      MusECore::MidiTrack* mt = *it;
      // We are looking for the given track alone if unselected, or else all selected tracks.
      // Ignore other tracks if broadcasting changes is disabled.
      if(mt != track && (!_broadcastChanges || !mt->selected() || !track->selected()))
        continue;

  #ifdef _USE_SIMPLIFIED_SOLO_CHAIN_
      // Support Midi Track to Audio Input track soloing chain routes.
      // Support omni routes only, because if channels are supported, the graphical router becomes more complicated.
      if(_isOutMenu && rem_route.channel == -1)
      {
        const MusECore::Route this_route(mt, -1);
        operations.add(MusECore::PendingOperationItem(this_route, rem_route,
                                                      action->isChecked() ?
                                                        MusECore::PendingOperationItem::AddRoute :
                                                        MusECore::PendingOperationItem::DeleteRoute));
      }
  #else
      // Support Midi Port to Audio Input track routes.
      int chbit = rem_route.channel;
      int port = mt->outPort();
      if(port < 0 || port >= MIDI_PORTS)
        return;

      MusECore::MidiPort* mp = &MusEGlobal::midiPorts[port];
      MusECore::Route bRoute(port, chbit);

      int chmask = 0;
      MusECore::RouteList* mprl = _isOutMenu ? mp->outRoutes() : mp->inRoutes();
      MusECore::ciRoute ir = mprl->begin();
      for (; ir != mprl->end(); ++ir)
        if(ir->type == MusECore::Route::TRACK_ROUTE && ir->track == rem_route.track) {   // Is there already a route to this port?
          chmask = ir->channel;  // Grab the channel mask.
          break;
        }
      if ((chmask & chbit) == chbit)             // Is the channel's bit(s) set?
      {
        // disconnect
        if(_isOutMenu)
          operations.add(MusECore::PendingOperationItem(bRoute, rem_route, MusECore::PendingOperationItem::DeleteRoute));
        else
          operations.add(MusECore::PendingOperationItem(rem_route, bRoute, MusECore::PendingOperationItem::DeleteRoute));
      }
      else
      {
        // connect
        if(_isOutMenu)
          operations.add(MusECore::PendingOperationItem(bRoute, rem_route, MusECore::PendingOperationItem::AddRoute));
        else
          operations.add(MusECore::PendingOperationItem(rem_route, bRoute, MusECore::PendingOperationItem::AddRoute));
      }
  #endif

    }
  }
  // Midi track to Midi Port routes.
  else if(rem_route.type == MusECore::Route::MIDI_PORT_ROUTE)
  {
    // Check for custom routing matrix action.
    RoutingMatrixWidgetAction* matrix_wa = qobject_cast<RoutingMatrixWidgetAction*>(action);

    MusECore::MidiTrack::ChangedType_t changed = MusECore::MidiTrack::NothingChanged;

    MusECore::MidiTrackList* tracks = MusEGlobal::song->midis();
    for(MusECore::iMidiTrack it = tracks->begin(); it != tracks->end(); ++it)
    {
      MusECore::MidiTrack* mt = *it;
      // We are looking for the given track alone if unselected, or else all selected tracks.
      // Ignore other tracks if broadcasting changes is disabled.
      if(mt != track && (!_broadcastChanges || !mt->selected() || !track->selected()))
        continue;

      if(matrix_wa)
      {
        DEBUG_PRST_ROUTES(stderr, "RoutePopupMenu::midiTrackPopupActivated midi port: matrix:\n");

  #ifdef _USE_MIDI_ROUTE_PER_CHANNEL_
        const int cols = matrix_wa->array()->columns();
        switch(rem_route.type)
        {
          case MusECore::Route::MIDI_PORT_ROUTE:
          {
            if(rem_route.isValid() && rem_route.midiPort != -1)
            {
              // Do channel routes...
              for(int col = 0; col < cols && col < MusECore::MUSE_MIDI_CHANNELS; ++col)
              {
                const bool val = matrix_wa->array()->value(col);

  #ifdef _USE_MIDI_TRACK_SINGLE_OUT_PORT_CHAN_
                // In this case the channel bar (and any other channel bars in a QActionGroup) will be in exclusive mode...
                if(_isOutMenu)
                {
                  if(val)
                  {
                    const bool p_changed = rem_route.midiPort != mt->outPort();
                    const bool c_changed = col != mt->outChannel();
                    if(p_changed || c_changed)
                    {
                      // Avoid repeated idlings. And remember to un-idle outside of the loop!
                      if(!MusEGlobal::audio->isIdle())
                        MusEGlobal::audio->msgIdle(true);

                      if(p_changed && c_changed)
                        changed |= mt->setOutPortAndChannelAndUpdate(rem_route.midiPort, col, false);
                      else if(p_changed)
                        changed |= mt->setOutPortAndUpdate(rem_route.midiPort, false);
                      else if(c_changed)
                        changed |= mt->setOutChanAndUpdate(col, false);
                    }
                    break;
                  }
                  continue;
                }

  #endif
                MusECore::Route this_route(mt, col);
                rem_route.channel = col;
                const MusECore::Route& src = _isOutMenu ? this_route : rem_route;
                const MusECore::Route& dst = _isOutMenu ? rem_route : this_route;
                DEBUG_PRST_ROUTES(stderr, "RoutePopupMenu::midiTrackPopupActivated: checking operations\n");
                // Connect if route does not exist. Allow it to reconnect a partial route.
                if(val && MusECore::routeCanConnect(src, dst))
                {
                  DEBUG_PRST_ROUTES(stderr, "RoutePopupMenu::midiTrackPopupActivated: adding AddRoute operation\n");
                  operations.add(MusECore::PendingOperationItem(src, dst, MusECore::PendingOperationItem::AddRoute));
                }
                // Disconnect if route exists. Allow it to reconnect a partial route.
                else if(!val && MusECore::routeCanDisconnect(src, dst))
                {
                  DEBUG_PRST_ROUTES(stderr, "RoutePopupMenu::midiTrackPopupActivated: adding DeleteRoute operation\n");
                  operations.add(MusECore::PendingOperationItem(src, dst, MusECore::PendingOperationItem::DeleteRoute));
                }
              }
            }

            // Do Omni route...
            if(matrix_wa->hasCheckBox())
            {
              const bool cb_val = matrix_wa->checkBoxChecked();
              MusECore::Route this_route(mt);
              rem_route.channel = -1;
              const MusECore::Route& src = _isOutMenu ? this_route : rem_route;
              const MusECore::Route& dst = _isOutMenu ? rem_route : this_route;
              DEBUG_PRST_ROUTES(stderr, "RoutePopupMenu::midiTrackPopupActivated: Omni checking operations\n");
              // Connect if route does not exist. Allow it to reconnect a partial route.
              if(cb_val && MusECore::routeCanConnect(src, dst))
              {
                DEBUG_PRST_ROUTES(stderr, "RoutePopupMenu::midiTrackPopupActivated: Omni adding AddRoute operation\n");
                operations.add(MusECore::PendingOperationItem(src, dst, MusECore::PendingOperationItem::AddRoute));
              }
              // Disconnect if route exists. Allow it to reconnect a partial route.
              else if(!cb_val && MusECore::routeCanDisconnect(src, dst))
              {
                DEBUG_PRST_ROUTES(stderr, "RoutePopupMenu::midiTrackPopupActivated: Omni adding DeleteRoute operation\n");
                operations.add(MusECore::PendingOperationItem(src, dst, MusECore::PendingOperationItem::DeleteRoute));
              }
            }
          }
          break;

          case MusECore::Route::TRACK_ROUTE:
          case MusECore::Route::JACK_ROUTE:
          case MusECore::Route::MIDI_DEVICE_ROUTE:
          break;
        }
  #else
        MusECore::RouteList* rl = _isOutMenu ? mt->outRoutes() : mt->inRoutes();
        const int cols = matrix_wa->array()->columns();
        switch(rem_route.type)
        {
          case MusECore::Route::MIDI_PORT_ROUTE:
          {
            if(rem_route.isValid() && rem_route.midiPort != -1)
            {
              int chmask = 0;
              // Is there already a route?
              for(MusECore::ciRoute ir = rl->begin(); ir != rl->end(); ++ir)
              {
                switch(ir->type)
                {
                  case MusECore::Route::MIDI_PORT_ROUTE:
                    if(ir->midiPort == rem_route.midiPort)
                      chmask = ir->channel; // Grab the channels.
                  break;
                  case MusECore::Route::TRACK_ROUTE:
                  case MusECore::Route::JACK_ROUTE:
                  case MusECore::Route::MIDI_DEVICE_ROUTE:
                  break;
                }
                if(chmask != 0)
                  break;
              }

              int chbits = 0;
              for(int col = 0; col < cols; ++col)
              {
                if(matrix_wa->array()->value(col))
                  chbits |= (1 << col);
              }

              // Only if something changed...
              if(chmask != chbits)
              {
                if(chmask != 0)
                {
                  MusECore::Route bRoute(mt, chmask);
                  // Disconnect all existing channels.
                  if(_isOutMenu)
                    operations.add(MusECore::PendingOperationItem(bRoute, r, MusECore::PendingOperationItem::DeleteRoute));
                  else
                    operations.add(MusECore::PendingOperationItem(r, bRoute, MusECore::PendingOperationItem::DeleteRoute));
                }
                if(chbits != 0)
                {
                  // Connect desired channels.
                  MusECore::Route bRoute(mt, chbits);
                  if(_isOutMenu)
                    operations.add(MusECore::PendingOperationItem(bRoute, r, MusECore::PendingOperationItem::AddRoute));
                  else
                    operations.add(MusECore::PendingOperationItem(r, bRoute, MusECore::PendingOperationItem::AddRoute));
                }
              }
            }
          }
          break;

          case MusECore::Route::TRACK_ROUTE:
          case MusECore::Route::JACK_ROUTE:
          case MusECore::Route::MIDI_DEVICE_ROUTE:
          break;
        }
  #endif

      }
      else
      {

  #ifdef _USE_MIDI_ROUTE_PER_CHANNEL_

        MusECore::Route this_route(mt, rem_route.channel);
        const MusECore::Route& src = _isOutMenu ? this_route : rem_route;
        const MusECore::Route& dst = _isOutMenu ? rem_route : this_route;
        // Connect if route does not exist. Allow it to reconnect a partial route.
        if(action->isChecked() && MusECore::routeCanConnect(src, dst))
        {
          DEBUG_PRST_ROUTES(stderr, "RoutePopupMenu::midiTrackPopupActivated: Route: adding AddRoute operation\n");
          operations.add(MusECore::PendingOperationItem(src, dst, MusECore::PendingOperationItem::AddRoute));
        }
        // Disconnect if route exists. Allow it to reconnect a partial route.
        else if(!action->isChecked() && MusECore::routeCanDisconnect(src, dst))
        {
          DEBUG_PRST_ROUTES(stderr, "RoutePopupMenu::midiTrackPopupActivated: Route: adding DeleteRoute operation\n");
          operations.add(MusECore::PendingOperationItem(src, dst, MusECore::PendingOperationItem::DeleteRoute));
        }

  #else
        int chbit = rem_route.channel;
        MusECore::Route bRoute(mt, chbit);
        int mdidx = rem_route.midiPort;

        MusECore::MidiPort* mp = &MusEGlobal::midiPorts[mdidx];
        MusECore::MidiDevice* md = mp->device();
        //if(!md)    // Rem. Allow connections to ports with no device.
        //  return;

        if(md && !(md->rwFlags() & (_isOutMenu ? 1 : 2)))
            return;

        int chmask = 0;
        MusECore::ciRoute iir = rl->begin();
        for (; iir != rl->end(); ++iir)
        {
          if(iir->type == MusECore::Route::MIDI_PORT_ROUTE && iir->midiPort == mdidx)    // Is there already a route to this port?
          {
                chmask = iir->channel;  // Grab the channel mask.
                break;
          }
        }
        if ((chmask & chbit) == chbit)             // Is the channel's bit(s) set?
        {
          // disconnect
          if(_isOutMenu)
            operations.add(MusECore::PendingOperationItem(bRoute, rem_route, MusECore::PendingOperationItem::DeleteRoute));
          else
            operations.add(MusECore::PendingOperationItem(rem_route, bRoute, MusECore::PendingOperationItem::DeleteRoute));
        }
        else
        {
          // connect
          if(_isOutMenu)
            operations.add(MusECore::PendingOperationItem(bRoute, rem_route, MusECore::PendingOperationItem::AddRoute));
          else
            operations.add(MusECore::PendingOperationItem(rem_route, bRoute, MusECore::PendingOperationItem::AddRoute));
        }
  #endif

      }
    }

    // If we are idling, we made some changes. Make sure to un-idle and update.
    if(MusEGlobal::audio->isIdle())
    {
      MusEGlobal::audio->msgIdle(false);
      MusEGlobal::audio->msgUpdateSoloStates();
      MusEGlobal::song->update(SC_ROUTE | ((changed & MusECore::MidiTrack::DrumMapChanged) ? SC_DRUMMAP : 0));
    }
  }
  // Midi device to Jack port routes - via Midi Track.
  // NOTE: When a submenu's action is activated, its top-most menu always gets the activation call.
  // Try this simple (ideal) method, otherwise it requires we abandon the '_route' member and store TWO
  //  routes in EACH action: In this case one is the midi device route and the other is the jack route.
  else if(rem_route.type == MusECore::Route::JACK_ROUTE)
  {
    DEBUG_PRST_ROUTES(stderr, "RoutePopupMenu::midiTrackPopupActivated rem_route is JACK_ROUTE\n");
    if(QAction* act = activeAction())
    {
      DEBUG_PRST_ROUTES(stderr, "RoutePopupMenu::midiTrackPopupActivated active action:%p\n", act);
      if(act->data().canConvert<MusECore::Route>())
      {
        const MusECore::Route route = act->data().value<MusECore::Route>();
        DEBUG_PRST_ROUTES(stderr, "RoutePopupMenu::midiTrackPopupActivated:Jack route: activePopupWidget:%p this:%p class:%s route type:%d\n",
                QApplication::activePopupWidget(), this, metaObject()->className(), route.type);
        
        if(route.type == MusECore::Route::MIDI_PORT_ROUTE)
        {
          DEBUG_PRST_ROUTES(stderr, "RoutePopupMenu::midiTrackPopupActivated Midi port to Jack route\n");
          jackRouteActivated(action, route, rem_route, operations);
        }
      }
    }
  }
}

void RoutePopupMenu::trackPopupActivated(QAction* action, MusECore::Route& rem_route, MusECore::PendingOperationList& operations)
{
  DEBUG_PRST_ROUTES(stderr, "RoutePopupMenu::trackPopupActivated: action text:%s checked:%d name:%s\n", 
          action->text().toLatin1().constData(), action->isChecked(), 
          action->objectName().toLatin1().constData());
  
  MusECore::Track* track = _route.track;
  // Make sure the track still exists.
  if(MusEGlobal::song->tracks()->find(track) == MusEGlobal::song->tracks()->end())
    return;
  
  if(track->isMidiTrack())
    midiTrackPopupActivated(action, rem_route, operations);
  else
    audioTrackPopupActivated(action, rem_route, operations);
}

void RoutePopupMenu::routePopupActivated(QAction* action)
{
  if(!action || !_route.isValid() || actions().isEmpty())
    return;

  MusECore::PendingOperationList operations;
  
  // Handle any non-route items.
  if(!action->data().canConvert<MusECore::Route>())
  {
    bool ok = false;
    const int n = action->data().toInt(&ok);
    if(ok)
    {
      switch(n)
      {
        case _OPEN_MIDI_CONFIG_:
          MusEGlobal::muse->configMidiPorts();
        break;  
        
#ifdef _USE_CUSTOM_WIDGET_ACTIONS_
        case _ALIASES_WIDGET_ACTION_:
        {
          // Check for custom widget action.
          RoutingMatrixWidgetAction* wa = qobject_cast<RoutingMatrixWidgetAction*>(action);
          if(wa)
          {
            if(wa->array()->value(0))
              MusEGlobal::config.preferredRouteNameOrAlias = MusEGlobal::RoutePreferFirstAlias;
            else if(wa->array()->value(1))
              MusEGlobal::config.preferredRouteNameOrAlias = MusEGlobal::RoutePreferSecondAlias;
            else
              MusEGlobal::config.preferredRouteNameOrAlias = MusEGlobal::RoutePreferCanonicalName;
            
            MusEGlobal::song->update(SC_PORT_ALIAS_PREFERENCE);
          }
        }
        break;
        
        case _GROUPING_CHANNELS_WIDGET_ACTION_:
        {
          // Check for custom widget action.
          RoutingMatrixWidgetAction* wa = qobject_cast<RoutingMatrixWidgetAction*>(action);
          if(wa)
          {
            if(wa->array()->value(0))
              MusEGlobal::config.routerGroupingChannels = 1;
            else
              MusEGlobal::config.routerGroupingChannels = 2;
            
            MusEGlobal::song->update(SC_ROUTER_CHANNEL_GROUPING);
          }
        }
        break;
#endif
        
        case _SHOW_CANONICAL_NAMES_:
        {
          MusEGlobal::config.preferredRouteNameOrAlias = MusEGlobal::RoutePreferCanonicalName;
          MusEGlobal::song->update(SC_PORT_ALIAS_PREFERENCE);
        }
        break;
        
        case _SHOW_FIRST_ALIASES_:
        {
          MusEGlobal::config.preferredRouteNameOrAlias = action->isChecked() ? 
            MusEGlobal::RoutePreferFirstAlias : MusEGlobal::RoutePreferCanonicalName;
          MusEGlobal::song->update(SC_PORT_ALIAS_PREFERENCE);
        }
        break;
        
        case _SHOW_SECOND_ALIASES_:
        {
          MusEGlobal::config.preferredRouteNameOrAlias = action->isChecked() ? 
            MusEGlobal::RoutePreferSecondAlias : MusEGlobal::RoutePreferCanonicalName;
          MusEGlobal::song->update(SC_PORT_ALIAS_PREFERENCE);
        }
        break;
        
        case _OPEN_ROUTING_DIALOG_:
          MusEGlobal::muse->startRouteDialog();
        break;  
        
        default:
        break;  
      }
    }

    return;
  }
  
  DEBUG_PRST_ROUTES(stderr, "RoutePopupMenu::popupActivated: action data is a Route\n");
//   MusECore::Route rem_route = action->data().value<MusECore::Route>();
  
  // Support grouping together of channels.
  int act_group_sz = 0;
  int act_idx = 0;
  int act_start = 0;
//   int act_count = 0;
  bool use_act_list = false;
  QList < QAction* > act_list; 
  const QActionGroup* act_group = action->actionGroup();
  if(act_group && !act_group->isExclusive())
  {
    act_list = act_group->actions();
    act_idx = act_list.indexOf(action);
    if(act_idx != -1)
    {
      use_act_list = true;
      act_group_sz = act_list.size();
      
// Attempt to optimize by only doing the actions whose channels changed. Flawed. Just do the whole list.
//       act_start = act_idx;
//       if((act_start + MusEGlobal::config.routerGroupingChannels) > act_group_sz)
//         act_start = act_group_sz - MusEGlobal::config.routerGroupingChannels;
//       if(act_start < 0 )
//         act_start = 0;
//       act_count = MusEGlobal::config.routerGroupingChannels;
//       if((act_start + act_count) > act_group_sz)
//         act_count = act_group_sz - act_start; 
      
// FIXME TODO: If an external event causes a connection 'behind our back' while the menu is open, we need to update the menu by 
//       detecting the song changed SC_ROUTE flag. Otherwise when the menu checks all the actions against current connections, it finds 
//       a connection and tries to turn it off because we've not updated the menu and that channel is 'off' in the menu right now.
    }
  }
  
  while(1)
  {
    QAction* act = use_act_list ? act_list.at(act_start) : action;
    DEBUG_PRST_ROUTES_2(stderr, "RoutePopupMenu::popupActivated this:%p act:%p active action:%p act_group_sz:%d act_start:%d\n", // act_count:%d\n", 
            this, act, activeAction(), act_group_sz, act_start); //, act_count); 
    if(!act)
      break;
    MusECore::Route rem_route = act->data().value<MusECore::Route>();
    switch(_route.type)
    {
      case MusECore::Route::TRACK_ROUTE:
        trackPopupActivated(act, rem_route, operations);
      break;
      
      case MusECore::Route::JACK_ROUTE:
      break;
      
      case MusECore::Route::MIDI_DEVICE_ROUTE:
        jackRouteActivated(act, _route, rem_route, operations);
      break;
      
      case MusECore::Route::MIDI_PORT_ROUTE:
      break;
      
    }
    if(use_act_list)
    {
      ++act_start;
//       if(--act_count == 0)
      if(--act_group_sz == 0)
        break;
    }
    else
      break;
  }
  
  if(!operations.empty())
  {
    DEBUG_PRST_ROUTES_2(stderr, "RoutePopupMenu::popupActivated: executing operations\n");
    MusEGlobal::audio->msgExecutePendingOperations(operations, true);
//     MusEGlobal::song->update(SC_ROUTE);
  }
}

void RoutePopupMenu::prepare()
{
  if(!_route.isValid())
    return;
   
  connect(this, SIGNAL(triggered(QAction*)), SLOT(routePopupActivated(QAction*)));
  
  QAction* route_act = addAction(tr("Open advanced router..."));
  route_act->setIcon(*dummySVGIcon);
  route_act->setCheckable(false);
  route_act->setData(_OPEN_ROUTING_DIALOG_);

  switch(_route.type)
  {
    case MusECore::Route::TRACK_ROUTE:
    {
      MusECore::Track* const track = _route.track;
      if(track->isMidiTrack())
      {
        QAction* act = nullptr;
        // Warn if no devices available. Add an item to open midi config. 
        int pi = 0;
        for( ; pi < MusECore::MIDI_PORTS; ++pi)
        {
          MusECore::MidiDevice* md = MusEGlobal::midiPorts[pi].device();
          //if(md && !md->isSynti() && (md->rwFlags() & 2))
          //if(md && (md->rwFlags() & 2 || md->isSynti()) )  // p4.0.27 Reverted p4.0.35 
          if(md && (md->rwFlags() & (_isOutMenu ? 1 : 2))) // Allow synth as input.
            break;
        }
        if(pi == MusECore::MIDI_PORTS)
        {
          if(_isOutMenu)
            act = addAction(tr("Warning: No output devices!"));
          else
            act = addAction(tr("Warning: No input devices!"));
          act->setCheckable(false);
          act->setData(-1);
        }
        act = addAction(QIcon(*ankerSVGIcon), tr("Open midi config..."));
        act->setCheckable(false);
        act->setData(_OPEN_MIDI_CONFIG_);
      }
    }
    break;
    
    default:
    break;
  }

  addSeparator();
  
  if(_isOutMenu)
    addAction(new MenuTitleItem(tr("Output routes:"), this));
  else
    addAction(new MenuTitleItem(tr("Input routes:"), this));
  //addSeparator();
  
  switch(_route.type)
  {
    case MusECore::Route::TRACK_ROUTE:
    {
      MusECore::Track* const track = _route.track;
      if(track->isMidiTrack())
      {
        QAction* act = nullptr;
        addMidiPorts(track, this, _isOutMenu, true, _isOutMenu);
        if(_isOutMenu)   
        {
          
#ifdef _USE_SIMPLIFIED_SOLO_CHAIN_
          // Support Midi Track to Audio Input track soloing chain routes.
          // Support omni routes only, because if channels are supported, the graphical router becomes more complicated.
          const MusECore::InputList* const il = MusEGlobal::song->inputs();
          if(!il->empty())
          {
            addSeparator();
            addAction(new MenuTitleItem(tr("Soloing chain"), this)); 
            RoutePopupMenu* subp = new RoutePopupMenu(_route, this, _isOutMenu, _broadcastChanges);
            subp->setTitle(tr("Audio returns")); 
            for(MusECore::ciAudioInput ai = il->begin(); ai != il->end(); ++ai)
            {
              // Add omni route:
              MusECore::Track* t = *ai;
              act = subp->addAction(t->displayName());
              act->setCheckable(true);
              const MusECore::Route r(t, -1);
              act->setData(QVariant::fromValue(r));
              if(track->outRoutes()->contains(r))
                act->setChecked(true);
            }
            addMenu(subp);
          }
#else // _USE_SIMPLIFIED_SOLO_CHAIN_             
          // Support Midi Port to Audio Input track routes. 
          int port = ((MusECore::MidiTrack*)track)->outPort();
          if(port >= 0 && port < MIDI_PORTS)
          {
            MusECore::MidiPort* mp = &MusEGlobal::midiPorts[port];
            
            // Do not list synth devices! Requiring valid device is desirable, 
            //  but could lead to 'hidden' routes unless we add more support
            //  such as removing the existing routes when user changes flags.
            // So for now, just list all valid ports whether read or write.
            if(mp->device() && !mp->device()->isSynti())  
            {
              MusECore::RouteList* rl = mp->outRoutes();
              //int chbits = 1 << ((MusECore::MidiTrack*)track)->outChannel();
              //MusECore::MidiDevice* md = mp->device();
              //if(!md)
              //  continue;
              
              addSeparator();
              addAction(new MenuTitleItem(tr("Soloing chain"), this)); 
              PopupMenu* subp = new PopupMenu(this, true);
              subp->setTitle(tr("Audio returns")); 
              addMenu(subp);
              
              MusECore::InputList* al = MusEGlobal::song->inputs();

#ifdef _USE_CUSTOM_WIDGET_ACTIONS_
              for (MusECore::ciAudioInput ai = al->begin(); ai != al->end(); ++ai) 
              {
                // Add omni route:
                MusECore::Track* t = *ai;
                act = subp->addAction(t->displayName());
                act->setCheckable(true);
                const MusECore::Route r(t, -1);
                act->setData(QVariant::fromValue(r));   
                if(rl->exists(r))
                  act->setChecked(true);
                
                // Add channel routes:
                RoutePopupMenu* subp = new RoutePopupMenu(_route, this, _isOutMenu, _broadcastChanges);
                wa_subp->addAction(new MenuTitleItem(tr("Channels"), this));
                act->setMenu(wa_subp);
                //RoutingMatrixWidgetAction* wa = new RoutingMatrixWidgetAction(1, MusECore::MUSE_MIDI_CHANNELS, redLedIcon, darkRedLedIcon, this);
                RoutingMatrixWidgetAction* wa = new RoutingMatrixWidgetAction(1, MusECore::MUSE_MIDI_CHANNELS, 0, 0, this);
                wa->setData(QVariant::fromValue(r)); // Ignore the routing channel and channels - our action holds the channels.
                int chans = 0;
                // Is there already a route?
                for(MusECore::ciRoute ir = rl->begin(); ir != rl->end(); ++ir)
                {
                  switch(ir->type)
                  {
                    case MusECore::Route::TRACK_ROUTE:
                      if(ir->track == t)
                        chans = ir->channel; // Grab the channels.
                    break;  
                    case MusECore::Route::MIDI_PORT_ROUTE:
                    case MusECore::Route::JACK_ROUTE:
                    case MusECore::Route::MIDI_DEVICE_ROUTE:
                    break;  
                  }
                  if(chans != 0)
                    break;
                }
                if(chans != 0 && chans != -1)
                {
                  for(int col = 0; col < MusECore::MUSE_MIDI_CHANNELS; ++col)
                  {
                    if(chans & (1 << col))
                      wa->array()->setValue(0, col, true);
                  }
                }
                
                // Must rebuild array after text changes.
                wa->updateChannelArray();
                wa_subp->addAction(wa);
              }
#else
              int chbits = 1 << ((MusECore::MidiTrack*)track)->outChannel();
              for (MusECore::ciAudioInput i = al->begin(); i != al->end(); ++i) 
              {
                MusECore::Track* t = *i;
                QString s(t->displayName());
                act = subp->addAction(s);
                act->setCheckable(true);
                MusECore::Route r(t, chbits);
                act->setData(QVariant::fromValue(r));   
                for(MusECore::ciRoute ir = mprl->begin(); ir != mprl->end(); ++ir) 
                {
                  if(ir->type == MusECore::Route::TRACK_ROUTE && ir->track == t && (ir->channel & chbits))
                  {
                    act->setChecked(true);
                    break;
                  }  
                }
              }
#endif // _USE_CUSTOM_WIDGET_ACTIONS_
              
            }     
          }
#endif // _USE_SIMPLIFIED_SOLO_CHAIN_

        }
        else
        {
          #if 0
          // p4.0.17 List ports with no device and no in routes, in a separate popup.
          PopupMenu* morep = new PopupMenu(pup, true);
          morep->setTitle(tr("More...")); 
          for(int i = 0; i < MIDI_PORTS; ++i)
          {
            MusECore::MidiPort* mp = &MusEGlobal::midiPorts[i];
            if(mp->device())
              continue;
            
            PopupMenu* subp = new PopupMenu(morep, true);
            subp->setTitle(QString("%1:%2").arg(i).arg(tr("<none>"))); 
            
            // MusE-2: Check this - needed with QMenu? Help says no. No - verified, it actually causes double triggers!
            //connect(subp, SIGNAL(triggered(QAction*)), pup, SIGNAL(triggered(QAction*)));
            //connect(subp, SIGNAL(aboutToHide()), pup, SIGNAL(aboutToHide()));
            
            iRoute ir = rl->begin();
            for( ; ir != rl->end(); ++ir)   
            {
              if(ir->type == MusECore::Route::MIDI_PORT_ROUTE && ir->midiPort == i) 
                break;
            }
            if(ir != rl->end())
              continue;
            
            for(int ch = 0; ch < MusECore::MUSE_MIDI_CHANNELS; ++ch) 
            {
              act = subp->addAction(QString("Channel %1").arg(ch+1));
              act->setCheckable(true);
              act->setData(gid);
              
              int chbit = 1 << ch;
              MusECore::Route srcRoute(i, chbit);    // In accordance with new channel mask, use the bit position.
              
              gRoutingMenuMap.insert( pRouteMenuMap(gid, srcRoute) );
              
              //if(chanmask & chbit)                  // Is the channel already set? Show item check mark.
              //  act->setChecked(true);
              
              ++gid;  
            }
            //gid = MIDI_PORTS * MusECore::MUSE_MIDI_CHANNELS + i;           // Make sure each 'toggle' item gets a unique id.
            act = subp->addAction(QString("Toggle all"));
            //act->setCheckable(true);
            act->setData(gid);
            MusECore::Route togRoute(i, (1 << MusECore::MUSE_MIDI_CHANNELS) - 1);    // Set all channel bits.
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
        MusECore::AudioTrack* t = (MusECore::AudioTrack*)track;
        MusECore::RouteCapabilitiesStruct rcaps = t->routeCapabilities();
        
        
        if(_isOutMenu)   
        {
          const int t_ochs = rcaps._trackChannels._outChannels;
          int gid = 0;
          
          switch(track->type()) 
          {
            case MusECore::Track::AUDIO_OUTPUT:
            {
              addGroupingChannelsAction(this);
              addJackPorts(_route, this);
              
              if(!MusEGlobal::song->inputs()->empty())
              {
                //
                // Display using separate menu for audio inputs:
                //
                addSeparator();
                addAction(new MenuTitleItem(tr("Soloing chain"), this)); 
                RoutePopupMenu* subp = new RoutePopupMenu(_route, this, _isOutMenu, _broadcastChanges);
                subp->setTitle(tr("Audio returns")); 
                addMenu(subp);
                gid = addInPorts(t, subp, gid, -1, -1, true);  
                //
                // Display all in the same menu:
                //
                //addSeparator();
                //MenuTitleItem* title = new MenuTitleItem(tr("Audio returns"), this);
                //addAction(title); 
                //gid = addInPorts(t, this, gid, -1, -1, true);  
              }
            }
            break;
            
            case MusECore::Track::AUDIO_INPUT:
            case MusECore::Track::WAVE:
            case MusECore::Track::AUDIO_GROUP:
            case MusECore::Track::AUDIO_AUX:
            case MusECore::Track::AUDIO_SOFTSYNTH:
              if(t_ochs > 0)
              {
                addGroupingChannelsAction(this);
                addAction(new RoutingMatrixHeaderWidgetAction(tr("Omni"), tr("Tracks"), QString(), this));
                gid = addWavePorts(        t, this, gid, -1, -1, true);  
                gid = addOutPorts(         t, this, gid, -1, -1, true);
                gid = addGroupPorts(       t, this, gid, -1, -1, true);
                gid = addSynthPorts(       t, this, gid, -1, -1, true);
              }
            break;
            
            default:
              clear();
              return;
            break;
          }
          
    #ifndef _USE_CUSTOM_WIDGET_ACTIONS_
          switch(_track->type()) 
          {
            case MusECore::Track::AUDIO_INPUT:
            case MusECore::Track::WAVE:
            case MusECore::Track::AUDIO_GROUP:
            case MusECore::Track::AUDIO_AUX:
            case MusECore::Track::AUDIO_SOFTSYNTH:
              if(t_ochs > 0)
              {
                addSeparator();
                addAction(new MenuTitleItem(tr("Channels"), this));
                for(int i = 0; i < t_ochs; ++i)
                {
                  PopupMenu* subp = new PopupMenu(this, true);
                  subp->setTitle(QString::number(i + 1)); 
                  subp->addAction(new MenuTitleItem(tr("Destinations:"), this));
                  addMenu(subp);
                  gid = addWavePorts( t, subp, gid, i, 1, true);  
                  gid = addOutPorts(  t, subp, gid, i, 1, true);
                  gid = addGroupPorts(t, subp, gid, i, 1, true);
                  gid = addSynthPorts(t, subp, gid, i, 1, true);
                }
              }
            break;

            default:
            break;
          }
    #endif

        }
        else
        {
          if(track->type() == MusECore::Track::AUDIO_AUX)
            return;
            
          const int t_ichs = rcaps._trackChannels._inChannels;
          int gid = 0;
          
          switch(track->type()) 
          {
            case MusECore::Track::AUDIO_INPUT:
            {
              addGroupingChannelsAction(this);
              addJackPorts(_route, this);
              
              if(!MusEGlobal::song->outputs()->empty() || !MusEGlobal::song->midis()->empty())
              {
                RoutePopupMenu* subp;
                //
                // Display using separate menus for midi ports and audio outputs:
                //
                addSeparator();
                addAction(new MenuTitleItem(tr("Soloing chain"), this));
                if(!MusEGlobal::song->outputs()->empty())
                {
                  subp = new RoutePopupMenu(_route, this, _isOutMenu, _broadcastChanges);
                  subp->setTitle(tr("Audio sends")); 
                  addMenu(subp);
                  gid = addOutPorts(t, subp, gid, -1, -1, false);  
                }
                if(!MusEGlobal::song->midis()->empty())
                {
                  subp = new RoutePopupMenu(_route, this, true, _broadcastChanges);
                  subp->setTitle(tr("Midi sends")); 
                  addMenu(subp);
                  addMidiTracks(t, subp, false);
                  //
                  // Display all in the same menu:
                  //
                  //addAction(new MenuTitleItem(tr("Audio sends"), this)); 
                  //gid = addOutPorts(t, this, gid, -1, -1, false);  
                  //addSeparator();
                  //addAction(new MenuTitleItem(tr("Midi sends"), this)); 
                  //addMidiPorts(t, this, gid, false);
                }
              }
            }
            break;
            
            case MusECore::Track::AUDIO_OUTPUT:
            case MusECore::Track::WAVE:
            case MusECore::Track::AUDIO_GROUP:
            case MusECore::Track::AUDIO_SOFTSYNTH:
              if(t_ichs > 0)
              {
                addGroupingChannelsAction(this);
                addAction(new RoutingMatrixHeaderWidgetAction(tr("Omni"), tr("Tracks"), QString(), this));
                gid = addWavePorts( t, this, gid, -1, -1, false);
                gid = addInPorts(   t, this, gid, -1, -1, false);
                gid = addGroupPorts(t, this, gid, -1, -1, false);
                gid = addAuxPorts(  t, this, gid, -1, -1, false);  
                gid = addSynthPorts(t, this, gid, -1, -1, false);  
              }
            break;
                  
            default:
              clear();
              return;
            break;  
          }  
          
    #ifndef _USE_CUSTOM_WIDGET_ACTIONS_
          switch(_track->type()) 
          {
            case MusECore::Track::AUDIO_OUTPUT:
            case MusECore::Track::WAVE:
            case MusECore::Track::AUDIO_GROUP:
            case MusECore::Track::AUDIO_SOFTSYNTH:
              if(t_ichs > 0)
              {
                addSeparator();
                addAction(new MenuTitleItem(tr("Channels"), this));
                for(int i = 0; i < t_ichs; ++i)
                {
                  PopupMenu* subp = new PopupMenu(this, true);
                  subp->setTitle(QString::number(i + 1)); 
                  subp->addAction(new MenuTitleItem(tr("Sources:"), this));
                  addMenu(subp);
                  gid = addWavePorts( t, subp, gid, i, 1, false);  
                  gid = addInPorts(   t, subp, gid, i, 1, false);
                  gid = addGroupPorts(t, subp, gid, i, 1, false);
                  gid = addAuxPorts(  t, subp, gid, i, 1, false);  
                  gid = addSynthPorts(t, subp, gid, i, 1, false);
                }
              }
            break;

            default:
            break;
          }
    #endif

        }  
      }
      
      
    }
    break;
    
    case MusECore::Route::MIDI_DEVICE_ROUTE:
      addJackPorts(_route, this);
    break;

    case MusECore::Route::JACK_ROUTE:
    case MusECore::Route::MIDI_PORT_ROUTE:
    break;
  
  }
}

void RoutePopupMenu::exec(const MusECore::Route& route, bool isOutput)
{
  if(route.isValid())
  {
    _route = route;
    _isOutMenu = isOutput;
  }  
  prepare();
  PopupMenu::exec();
}

void RoutePopupMenu::exec(const QPoint& p, const MusECore::Route& route, bool isOutput)
{
  if(route.isValid())
  {
    _route = route;
    _isOutMenu = isOutput;
  }  
  prepare();
  PopupMenu::exec(p);
}

void RoutePopupMenu::popup(const QPoint& p, const MusECore::Route& route, bool isOutput)
{
  if(route.isValid())
  {
    _route = route;
    _isOutMenu = isOutput;
  }  
  prepare();
  PopupMenu::popup(p);
}

} // namespace MusEGui
