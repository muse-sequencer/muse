//=========================================================
//  MusE
//  Linux Music Editor
//
//  RoutePopupMenu.cpp 
//  (C) Copyright 2011 Tim E. Real (terminator356 A T sourceforge D O T net)
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
#include <qtextcodec.h>
#include <QActionGroup>
#include <list>

#include "app.h"
#include "routepopup.h"
#include "midiport.h"
#include "mididev.h"
#include "audio.h"
#include "driver/audiodev.h"
#include "song.h"
#include "track.h"
#include "synth.h"
// REMOVE Tim. Persistent routes. Removed.
//#include "route.h"
#include "icons.h"
#include "menutitleitem.h"
#include "popupmenu.h"
#include "operations.h"

#include "custom_widget_actions.h"
#include "globaldefs.h"

#define _USE_CUSTOM_WIDGET_ACTIONS_ 

#define _SHOW_CANONICAL_NAMES_ 0x1000
#define _SHOW_FIRST_ALIASES_  0x1001
#define _SHOW_SECOND_ALIASES_ 0x1002
#define _ALIASES_WIDGET_ACTION_ 0x2000
#define _PREFER_CANONICAL_NAMES_ 0
#define _PREFER_FIRST_ALIASES_ 1
#define _PREFER_SECOND_ALIASES_ 2

namespace MusEGui {

// TODO: Temporary until a more 'global' variable is added (accessible from graphical router for example).
int preferredRouteNameOrAlias = _PREFER_CANONICAL_NAMES_;
  
//---------------------------------------------------------
//   addMenuItem
//---------------------------------------------------------

// int RoutePopupMenu::addMenuItem(MusECore::AudioTrack* track, MusECore::Track* route_track, MusEGui::PopupMenu* lb, 
//                                 int id, int channel, int /*channels*/, bool isOutput)
// {
//   // totalInChannels is only used by syntis.
//   //int toch = ((MusECore::AudioTrack*)track)->totalOutChannels();
//   // If track channels = 1, it must be a mono synth. And synti channels cannot be changed by user.
//   //if(track->channels() == 1)
//   //  toch = 1;
//   
//   // Don't add the last stray mono route if the track is stereo.
//   //if(route_track->channels() > 1 && (channel+1 == chans))
//   //  return id;
//     
//   // REMOVE Tim. Persistent routes. Added.
//   if(route_track->isMidiTrack())
//     return ++id;
//   
//   MusECore::RouteList* rl = isOutput ? track->outRoutes() : track->inRoutes();
//   
//   // REMOVE Tim. Persistent routes. Removed.
//   //QAction* act;
//   
//   // REMOVE Tim. Persistent routes. Removed.
//   //QString s(route_track->name());
// 
// 
//   // REMOVE Tim. Persistent routes. Added.
//   const bool circ_route = (isOutput ? track : route_track)->isCircularRoute(isOutput ? route_track : track);
//   const int t_chans = isOutput ? track->totalRoutableOutputs(MusECore::Route::TRACK_ROUTE) : track->totalRoutableInputs(MusECore::Route::TRACK_ROUTE);
//   const int rt_chans = isOutput ? route_track->totalRoutableInputs(MusECore::Route::TRACK_ROUTE) : route_track->totalRoutableOutputs(MusECore::Route::TRACK_ROUTE);
//   if(isOutput && track->type() == MusECore::Track::AUDIO_OUTPUT && route_track->type() == MusECore::Track::AUDIO_INPUT)
//   {
//     // Only support omnibus routes for now.
//     if(channel != -1 || track->totalRoutableOutputs(MusECore::Route::JACK_ROUTE) <= 0 || route_track->totalRoutableInputs(MusECore::Route::JACK_ROUTE) <= 0)
//       return ++id;
//   }
//   else
//   if(!isOutput && track->type() == MusECore::Track::AUDIO_INPUT && route_track->type() == MusECore::Track::AUDIO_OUTPUT)
//   {
//     // Only support omnibus routes for now.
//     if(channel != -1 || track->totalRoutableInputs(MusECore::Route::JACK_ROUTE) <= 0 || route_track->totalRoutableOutputs(MusECore::Route::JACK_ROUTE) <= 0)
//       return ++id;
//   }
//   else
//   {
//     if(t_chans <= 0)
//       return ++id;
//     if(rt_chans <= 0)
//       return ++id;
//   }
//   // Is it an omnibus route?
//   if(channel == -1)
//   {
//     //if((isOutput && track->type() == MusECore::Track::AUDIO_OUTPUT && route_track->type() == MusECore::Track::AUDIO_INPUT &&
//     //   (track->totalRoutableOutputs(MusECore::Route::JACK_ROUTE) <= 0 || route_track->totalRoutableInputs(MusECore::Route::JACK_ROUTE) <= 0)) ||
//     //   (!isOutput && track->type() == MusECore::Track::AUDIO_INPUT && route_track->type() == MusECore::Track::AUDIO_OUTPUT &&
//     //   (track->totalRoutableInputs(MusECore::Route::JACK_ROUTE) <= 0 || route_track->totalRoutableOutputs(MusECore::Route::JACK_ROUTE) <= 0)))
//     //  return ++id;
//       
//     QAction* act = lb->addAction(route_track->name());
//     act->setCheckable(true);
//     MusECore::Route r(route_track, -1);
//     act->setData(QVariant::fromValue(r));   
//     for(MusECore::ciRoute ir = rl->begin(); ir != rl->end(); ++ir) 
//     {
//       if(ir->type == MusECore::Route::TRACK_ROUTE && ir->track == route_track && 
//           ir->remoteChannel == r.channel && 
//           ir->channel == r.remoteChannel && 
//           ir->channels == r.channels)
//       {
//         act->setChecked(true);
//         break;
//       }  
//     }
//     if(!act->isChecked() && circ_route)  // If circular route exists, allow user to break it, otherwise forbidden.
//       act->setEnabled(false);
//   }
//   else
//   {
//     
// #ifdef _USE_CUSTOM_WIDGET_ACTIONS_
//         
//     QBitArray ba(rt_chans); 
//     for(int i = 0; i < rt_chans; ++i)
//     {  
//       MusECore::Route r(route_track, i, 1);
//       for(MusECore::ciRoute ir = rl->begin(); ir != rl->end(); ++ir) 
//       {
//         if(ir->type == MusECore::Route::TRACK_ROUTE && ir->track == route_track && 
//             ir->remoteChannel == r.channel && 
//             ir->channel == r.remoteChannel && 
//             ir->channels == r.channels)
//         {
//           ba.setBit(i);
//           break;
//         }  
//       }
//     }
//     PixmapButtonsWidgetAction* wa = new PixmapButtonsWidgetAction(route_track->name(), redLedIcon, darkRedLedIcon, ba, this);
//     MusECore::Route r(route_track, 0, 1); // Ignore the routing channels - our action holds the channels.   
//     wa->setData(QVariant::fromValue(r));   
//     lb->addAction(wa);  
//     
// #else
//     
//     // It's not an omnibus route. Add the individual channels...
//     PopupMenu* subp = new PopupMenu(this, true);
//     subp->setTitle(route_track->name());
//     subp->addAction(new MenuTitleItem(tr("Channels"), this));
//     for(int i = 0; i < rt_chans; ++i)
//     {
//       QAction* act = subp->addAction(QString::number(i + 1));
//       act->setCheckable(true);
//       MusECore::Route r(route_track, i, 1);
//       r.remoteChannel = channel;
//       act->setData(QVariant::fromValue(r));
//       
//       for(MusECore::ciRoute ir = rl->begin(); ir != rl->end(); ++ir) 
//       {
//         if(ir->type == MusECore::Route::TRACK_ROUTE && ir->track == route_track && 
//             ir->remoteChannel == r.channel && 
//             ir->channel == r.remoteChannel && 
//             ir->channels == r.channels)
//         {
//           act->setChecked(true);
//           break;
//         }  
//       }
//       if(!act->isChecked() && circ_route)  // If circular route exists, allow user to break it, otherwise forbidden.
//         act->setEnabled(false);
//     }
//     lb->addMenu(subp);
// #endif
//     
//   }
//   
//   return ++id;
// }  
  
int RoutePopupMenu::addMenuItem(MusECore::AudioTrack* track, MusECore::Track* route_track, MusEGui::PopupMenu* lb, 
                                int id, int channel, int /*channels*/, bool isOutput)
{
  // totalInChannels is only used by syntis.
  //int toch = ((MusECore::AudioTrack*)track)->totalOutChannels();
  // If track channels = 1, it must be a mono synth. And synti channels cannot be changed by user.
  //if(track->channels() == 1)
  //  toch = 1;
  
  // Don't add the last stray mono route if the track is stereo.
  //if(route_track->channels() > 1 && (channel+1 == chans))
  //  return id;
    
  // REMOVE Tim. Persistent routes. Added.
  if(route_track->isMidiTrack())
    return ++id;
  
  MusECore::RouteList* rl = isOutput ? track->outRoutes() : track->inRoutes();
  
  // REMOVE Tim. Persistent routes. Removed.
  //QAction* act;
  
  // REMOVE Tim. Persistent routes. Removed.
  //QString s(route_track->name());


  // REMOVE Tim. Persistent routes. Added.
  const bool circ_route = (isOutput ? track : route_track)->isCircularRoute(isOutput ? route_track : track);
  const int t_chans = isOutput ? track->totalRoutableOutputs(MusECore::Route::TRACK_ROUTE) : track->totalRoutableInputs(MusECore::Route::TRACK_ROUTE);
  const int rt_chans = isOutput ? route_track->totalRoutableInputs(MusECore::Route::TRACK_ROUTE) : route_track->totalRoutableOutputs(MusECore::Route::TRACK_ROUTE);
  if(isOutput && track->type() == MusECore::Track::AUDIO_OUTPUT && route_track->type() == MusECore::Track::AUDIO_INPUT)
  {
    // Only support omnibus routes for now.
    if(channel != -1 || track->totalRoutableOutputs(MusECore::Route::JACK_ROUTE) <= 0 || route_track->totalRoutableInputs(MusECore::Route::JACK_ROUTE) <= 0)
      return ++id;
  }
  else
  if(!isOutput && track->type() == MusECore::Track::AUDIO_INPUT && route_track->type() == MusECore::Track::AUDIO_OUTPUT)
  {
    // Only support omnibus routes for now.
    if(channel != -1 || track->totalRoutableInputs(MusECore::Route::JACK_ROUTE) <= 0 || route_track->totalRoutableOutputs(MusECore::Route::JACK_ROUTE) <= 0)
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
    //if((isOutput && track->type() == MusECore::Track::AUDIO_OUTPUT && route_track->type() == MusECore::Track::AUDIO_INPUT &&
    //   (track->totalRoutableOutputs(MusECore::Route::JACK_ROUTE) <= 0 || route_track->totalRoutableInputs(MusECore::Route::JACK_ROUTE) <= 0)) ||
    //   (!isOutput && track->type() == MusECore::Track::AUDIO_INPUT && route_track->type() == MusECore::Track::AUDIO_OUTPUT &&
    //   (track->totalRoutableInputs(MusECore::Route::JACK_ROUTE) <= 0 || route_track->totalRoutableOutputs(MusECore::Route::JACK_ROUTE) <= 0)))
    //  return ++id;
      
    QAction* act = lb->addAction(route_track->name());
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
    
    QAction* act = lb->addAction(route_track->name());
    act->setCheckable(true);
    const MusECore::Route r(route_track, -1);
    act->setData(QVariant::fromValue(r));   
    for(MusECore::ciRoute ir = rl->begin(); ir != rl->end(); ++ir) 
    {
      if(ir->type == MusECore::Route::TRACK_ROUTE && ir->track == route_track && 
          ir->remoteChannel == -1 && ir->channel == -1 && ir->channels == -1)
      {
        act->setChecked(true);
        break;
      }  
    }
    
    if(rt_chans != 0 && t_chans != 0)
    {
      PopupMenu* subp = new PopupMenu(this, true);
      subp->addAction(new MenuTitleItem(tr("Channels"), this));
      act->setMenu(subp);
      RoutingMatrixWidgetAction* wa = new RoutingMatrixWidgetAction(rt_chans, t_chans, redLedIcon, darkRedLedIcon, this);
      wa->setData(QVariant::fromValue(r)); // Ignore the routing channel and channels - our action holds the channels.
      for(int row = 0; row < rt_chans; ++row)
      {
        for(int col = 0; col < t_chans; ++col)
        {  
          for(MusECore::ciRoute ir = rl->begin(); ir != rl->end(); ++ir) 
          {
            if(ir->type == MusECore::Route::TRACK_ROUTE && ir->track == route_track && 
                ir->remoteChannel == row && 
                ir->channel == col && 
                ir->channels == 1)
            {
              wa->array()->setValue(row, col, true);
              break;
            }  
          }
        }
      }
      subp->addAction(wa);
    }
    
    if(!act->isChecked() && circ_route)  // If circular route exists, allow user to break it, otherwise forbidden.
      act->setEnabled(false);
    
    lb->addAction(act);  
    
#else
    
    // It's not an omnibus route. Add the individual channels...
    PopupMenu* subp = new PopupMenu(this, true);
    subp->setTitle(route_track->name());
    subp->addAction(new MenuTitleItem(tr("Channels"), this));
    for(int i = 0; i < rt_chans; ++i)
    {
      QAction* act = subp->addAction(QString::number(i + 1));
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
    lb->addMenu(subp);
#endif
    
  }
  
  return ++id;
}  
  
// REMOVE Tim. Persistent routes. Removed.
/*  
  // REMOVE Tim. Persistent routes. Changed.
  //act = lb->addAction(s);
  QAction* act = lb->addAction(route_track->name());

  act->setCheckable(true);
  
// REMOVE Tim. Persistent routes. Removed.
//   int ach = channel;
//   int bch = -1;
  
  // REMOVE Tim. Persistent routes. Changed.
  //MusECore::Route r(route_track, isOutput ? ach : bch, channels);
  MusECore::Route r(route_track, channel, channels);
  
  // REMOVE Tim. Persistent routes. Changed.
  //r.remoteChannel = isOutput ? bch : ach;
  r.remoteChannel = remote_channel;
  
  act->setData(QVariant::fromValue(r));   
  
  for(MusECore::ciRoute ir = rl->begin(); ir != rl->end(); ++ir) 
  {
    // REMOVE Tim. Persistent routes. Changed.
    //if(ir->type == MusECore::Route::TRACK_ROUTE && ir->track == route_track && ir->remoteChannel == r.remoteChannel)
    if(ir->type == MusECore::Route::TRACK_ROUTE && ir->track == route_track && 
       ir->remoteChannel == r.channel && 
       ir->channel == r.remoteChannel && 
       ir->channels == r.channels)
    {
      
// REMOVE Tim. Persistent routes. Changed.
//       int tcompch = r.channel;
//       if(tcompch == -1)
//         tcompch = 0;
//       int tcompchs = r.channels;
//       if(tcompchs == -1)
//         tcompchs = isOutput ? track->channels() : route_track->channels();
//       
//       int compch = ir->channel;
//       if(compch == -1)
//         compch = 0;
//       int compchs = ir->channels;
//       if(compchs == -1)
//         compchs = isOutput ? track->channels() : ir->track->channels();
//       
//       if(compch == tcompch && compchs == tcompchs) 
      {
        act->setChecked(true);
        break;
      }
    }  
  }
  
  if(!act->isChecked())  // If circular route exists, allow user to break it, otherwise forbidden.
  {
    if( (isOutput ? track : route_track)->isCircularRoute(isOutput ? route_track : track) ) 
      act->setEnabled(false);
  }

  return ++id;      
}*/

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

// REMOVE Tim. Persistent routes. Removed.
// //---------------------------------------------------------
// //   addSyntiPorts
// //---------------------------------------------------------
// 
// int RoutePopupMenu::addSyntiPorts(MusECore::AudioTrack* t, PopupMenu* lb, int id, 
//                          int channel, int channels, bool isOutput)
// {
//       MusECore::RouteList* rl = isOutput ? t->outRoutes() : t->inRoutes();
//       
//       QAction* act;
//       
//       MusECore::SynthIList* al = MusEGlobal::song->syntis();
//       for (MusECore::iSynthI i = al->begin(); i != al->end(); ++i) 
//       {
//             MusECore::Track* track = *i;
//             if (t == track)
//                   continue;
//             int toch = ((MusECore::AudioTrack*)track)->totalOutChannels();
//             // If track channels = 1, it must be a mono synth. And synti channels cannot be changed by user.
//             if(track->channels() == 1)
//               toch = 1;
//             
//             // totalInChannels is only used by syntis.
//             int chans = (!isOutput || track->type() != MusECore::Track::AUDIO_SOFTSYNTH) ? toch : ((MusECore::AudioTrack*)track)->totalInChannels();
//             
//             int tchans = (channels != -1) ? channels: t->channels();
//             if(tchans == 2)
//             {
//               // Ignore odd numbered left-over mono channel.
//               //chans = chans & ~1;
//               //if(chans != 0)
//                 chans -= 1;
//             }
//             
//             if(chans > 0)
//             {
//               PopupMenu* chpup = new PopupMenu(lb, true);
//               chpup->setTitle(track->name());
//               for(int ch = 0; ch < chans; ++ch)
//               {
//                 //char buffer[128];
//                 QString chBuffer;
//                 if(tchans == 2)
//                   chBuffer = tr("Channel") + QString(" ") + QString::number(ch + 1) + QString(",") + QString::number(ch + 2);
//                   //snprintf(buffer, 128, "%s %d,%d", tr("Channel").toLatin1().constData(), ch+1, ch+2);
//                 else
//                   chBuffer = tr("Channel") + QString(" ") + QString::number(ch + 1);
//                   //snprintf(buffer, 128, "%s %d", tr("Channel").toLatin1().constData(), ch+1);
//                 act = chpup->addAction(chBuffer);
//                 act->setCheckable(true);
//                 
//                 int ach = (channel == -1) ? ch : channel;
//                 int bch = (channel == -1) ? -1 : ch;
//                 
//                 MusECore::Route rt(track, (t->type() != MusECore::Track::AUDIO_SOFTSYNTH || isOutput) ? ach : bch, tchans);
//                 rt.remoteChannel = (t->type() != MusECore::Track::AUDIO_SOFTSYNTH || isOutput) ? bch : ach;
//                 
//                 act->setData(QVariant::fromValue(rt));   
//                 
//                 for(MusECore::ciRoute ir = rl->begin(); ir != rl->end(); ++ir) 
//                 {
//                   if(ir->type == MusECore::Route::TRACK_ROUTE && ir->track == track && ir->remoteChannel == rt.remoteChannel)
//                   {
//                     int tcompch = rt.channel;
//                     if(tcompch == -1)
//                       tcompch = 0;
//                     int tcompchs = rt.channels;
//                     if(tcompchs == -1)
//                       tcompchs = isOutput ? t->channels() : track->channels();
//                     
//                     int compch = ir->channel;
//                     if(compch == -1)
//                       compch = 0;
//                     int compchs = ir->channels;
//                     if(compchs == -1)
//                       compchs = isOutput ? t->channels() : ir->track->channels();
//                     
//                     if(compch == tcompch && compchs == tcompchs) 
//                     {
//                       act->setChecked(true);
//                       break;
//                     }
//                   }
//                 }  
// 
//                 if(!act->isChecked())  // If circular route exists, allow user to break it, otherwise forbidden.
//                 {
//                   if( (isOutput ? t : track)->isCircularRoute(isOutput ? track : t) ) 
//                     act->setEnabled(false);
//                 }
// 
//                 ++id;
//               }
//             
//               lb->addMenu(chpup);
//             }
//       }
//       return id;      
// }

// REMOVE Tim. Persistent routes. Removed.
// //---------------------------------------------------------
// //   addMultiChannelOutPorts
// //---------------------------------------------------------
// 
// int RoutePopupMenu::addMultiChannelPorts(MusECore::AudioTrack* t, PopupMenu* pup, int id, bool isOutput)
// {
//   int toch = t->totalOutChannels();
//   // If track channels = 1, it must be a mono synth. And synti channels cannot be changed by user.
//   if(t->channels() == 1)
//     toch = 1;
//   
//   // Number of allocated buffers is always MAX_CHANNELS or more, even if _totalOutChannels is less. 
//   // totalInChannels is only used by syntis.
//   int chans = (isOutput || t->type() != MusECore::Track::AUDIO_SOFTSYNTH) ? toch : t->totalInChannels();
// 
//   if(chans > 1)
//     pup->addAction(new MenuTitleItem("<Mono>", pup)); 
//   
//   //
//   // If it's more than one channel, create a sub-menu. If it's just one channel, don't bother with a sub-menu...
//   //
// 
//   PopupMenu* chpup = pup;
//   
//   for(int ch = 0; ch < chans; ++ch)
//   {
//     // If more than one channel, create the sub-menu.
//     if(chans > 1)
//       chpup = new PopupMenu(pup, true);
//     
//     if(isOutput)
//     {
//       switch(t->type()) 
//       {
//         
//         case MusECore::Track::AUDIO_INPUT:
//         case MusECore::Track::WAVE:
//         case MusECore::Track::AUDIO_GROUP:
//         case MusECore::Track::AUDIO_SOFTSYNTH:
//         case MusECore::Track::AUDIO_AUX:        
//               id = addWavePorts(t, chpup, id, ch, 1, isOutput);
//               id = addOutPorts(t, chpup, id, ch, 1, isOutput);
//               id = addGroupPorts(t, chpup, id, ch, 1, isOutput);
//               id = addSyntiPorts(t, chpup, id, ch, 1, isOutput);
//               break;
//         default:
//               break;
//       }
//     }
//     else
//     {
//       switch(t->type()) 
//       {
//         
//         case MusECore::Track::AUDIO_OUTPUT:
//               id = addWavePorts(t, chpup, id, ch, 1, isOutput);
//               id = addInPorts(t, chpup, id, ch, 1, isOutput);
//               id = addGroupPorts(t, chpup, id, ch, 1, isOutput);
//               id = addAuxPorts(t, chpup, id, ch, 1, isOutput);
//               id = addSyntiPorts(t, chpup, id, ch, 1, isOutput);
//               break;
//         case MusECore::Track::WAVE:
//         case MusECore::Track::AUDIO_SOFTSYNTH:
//         case MusECore::Track::AUDIO_GROUP:
//               id = addWavePorts(t, chpup, id, ch, 1, isOutput);
//               id = addInPorts(t, chpup, id, ch, 1, isOutput);
//               id = addGroupPorts(t, chpup, id, ch, 1, isOutput);
//               id = addAuxPorts(t, chpup, id, ch, 1, isOutput);     
//               id = addSyntiPorts(t, chpup, id, ch, 1, isOutput);
//               break;
//         default:
//               break;
//       }
//     }
//       
//     // If more than one channel, add the created sub-menu.
//     if(chans > 1)
//     {
//       //char buffer[128];
//       //snprintf(buffer, 128, "%s %d", tr("Channel").toLatin1().constData(), ch+1);
//       QString chBuffer = tr("Channel") + QString(" ") + QString::number(ch + 1);
//       chpup->setTitle(chBuffer);
//       pup->addMenu(chpup);
//     }  
//   } 
//        
//   // For stereo listing, ignore odd numbered left-over channels.
//   chans -= 1;
//   if(chans > 0)
//   {
//     // Ignore odd numbered left-over channels.
//     //int schans = (chans & ~1) - 1;
//     
//     pup->addSeparator();
//     pup->addAction(new MenuTitleItem("<Stereo>", pup));
//   
//     //
//     // If it's more than two channels, create a sub-menu. If it's just two channels, don't bother with a sub-menu...
//     //
//     
//     chpup = pup;
//     if(chans <= 2)
//       // Just do one iteration.
//       chans = 1;
//     
//     for(int ch = 0; ch < chans; ++ch)
//     {
//       // If more than two channels, create the sub-menu.
//       if(chans > 2)
//         chpup = new PopupMenu(pup, true);
//       
//       if(isOutput)
//       {
//         switch(t->type()) 
//         {
//           case MusECore::Track::AUDIO_INPUT:
//           case MusECore::Track::WAVE:
//           case MusECore::Track::AUDIO_GROUP:
//           case MusECore::Track::AUDIO_SOFTSYNTH:
//           case MusECore::Track::AUDIO_AUX:                                          
//                 id = addWavePorts(t, chpup, id, ch, 2, isOutput);     
//                 id = addOutPorts(t, chpup, id, ch, 2, isOutput);
//                 id = addGroupPorts(t, chpup, id, ch, 2, isOutput);
//                 id = addSyntiPorts(t, chpup, id, ch, 2, isOutput);
//                 break;
//           default:
//                 break;
//         }
//       }    
//       else
//       {
//         switch(t->type()) 
//         {
//           case MusECore::Track::AUDIO_OUTPUT:
//                 id = addWavePorts(t, chpup, id, ch, 2, isOutput);
//                 id = addInPorts(t, chpup, id, ch, 2, isOutput);
//                 id = addGroupPorts(t, chpup, id, ch, 2, isOutput);
//                 id = addAuxPorts(t, chpup, id, ch, 2, isOutput);
//                 id = addSyntiPorts(t, chpup, id, ch, 2, isOutput);
//                 break;
//           case MusECore::Track::WAVE:
//           case MusECore::Track::AUDIO_SOFTSYNTH:
//           case MusECore::Track::AUDIO_GROUP:
//                 id = addWavePorts(t, chpup, id, ch, 2, isOutput);
//                 id = addInPorts(t, chpup, id, ch, 2, isOutput);
//                 id = addGroupPorts(t, chpup, id, ch, 2, isOutput);
//                 id = addAuxPorts(t, chpup, id, ch, 2, isOutput);     
//                 id = addSyntiPorts(t, chpup, id, ch, 2, isOutput);
//                 break;
//           default:
//                 break;
//         }
//       }
//       
//       // If more than two channels, add the created sub-menu.
//       if(chans > 2)
//       {
//         //char buffer[128];
//         //snprintf(buffer, 128, "%s %d,%d", tr("Channel").toLatin1().constData(), ch+1, ch+2);
//         QString chBuffer = tr("Channel") + QString(" ") + QString::number(ch + 1) + QString(",") + QString::number(ch + 2);
//         chpup->setTitle(chBuffer);
//         pup->addMenu(chpup);
//       }  
//     } 
//   }
//   
//   return id;
// }

// REMOVE Tim. Persistent routes. Removed.
// //---------------------------------------------------------
// //   nonSyntiTrackAddSyntis
// //---------------------------------------------------------
// 
// int RoutePopupMenu::nonSyntiTrackAddSyntis(MusECore::AudioTrack* t, PopupMenu* lb, int id, bool isOutput)
// {
//       MusECore::RouteList* rl = isOutput ? t->outRoutes() : t->inRoutes();
//       
//       QAction* act;
//       MusECore::SynthIList* al = MusEGlobal::song->syntis();
//       for (MusECore::iSynthI i = al->begin(); i != al->end(); ++i) 
//       {
//             MusECore::Track* track = *i;
//             if (t == track)
//                   continue;
//             
//             int toch = ((MusECore::AudioTrack*)track)->totalOutChannels();
//             // If track channels = 1, it must be a mono synth. And synti channels cannot be changed by user.
//             if(track->channels() == 1)
//               toch = 1;
//             
//             // totalInChannels is only used by syntis.
//             int chans = (!isOutput || track->type() != MusECore::Track::AUDIO_SOFTSYNTH) ? toch : ((MusECore::AudioTrack*)track)->totalInChannels();
//             
//             //int schans = synti->channels();
//             //if(schans < chans)
//             //  chans = schans;
// //            int tchans = (channels != -1) ? channels: t->channels();
// //            if(tchans == 2)
// //            {
//               // Ignore odd numbered left-over mono channel.
//               //chans = chans & ~1;
//               //if(chans != 0)
// //                chans -= 1;
// //            }
//             //int tchans = (channels != -1) ? channels: t->channels();
//             
//             if(chans > 0)
//             {
//               PopupMenu* chpup = new PopupMenu(lb, true);
//               chpup->setTitle(track->name());
//               if(chans > 1)
//                 chpup->addAction(new MenuTitleItem("<Mono>", chpup));
//               
//               for(int ch = 0; ch < chans; ++ch)
//               {
//                 //char buffer[128];
//                 //snprintf(buffer, 128, "%s %d", tr("Channel").toLatin1().constData(), ch+1);
//                 QString chBuffer = tr("Channel") + QString(" ") + QString::number(ch + 1);
//                 act = chpup->addAction(chBuffer);
//                 act->setCheckable(true);
//                 
//                 int ach = ch;
//                 int bch = -1;
//                 
//                 MusECore::Route rt(track, isOutput ? bch : ach, 1);
//                 
//                 rt.remoteChannel = isOutput ? ach : bch;
//                 
//                 act->setData(QVariant::fromValue(rt));   
//                 
//                 for(MusECore::ciRoute ir = rl->begin(); ir != rl->end(); ++ir) 
//                 {
//                   if(ir->type == MusECore::Route::TRACK_ROUTE && ir->track == track && ir->remoteChannel == rt.remoteChannel)
//                   {
//                     int tcompch = rt.channel;
//                     if(tcompch == -1)
//                       tcompch = 0;
//                     int tcompchs = rt.channels;
//                     if(tcompchs == -1)
//                       tcompchs = isOutput ? t->channels() : track->channels();
//                     
//                     int compch = ir->channel;
//                     if(compch == -1)
//                       compch = 0;
//                     int compchs = ir->channels;
//                     if(compchs == -1)
//                       compchs = isOutput ? t->channels() : ir->track->channels();
//                     
//                     if(compch == tcompch && compchs == tcompchs) 
//                     {
//                       act->setChecked(true);
//                       break;
//                     }
//                   }
//                 }
//                 
//                 if(!act->isChecked())  // If circular route exists, allow user to break it, otherwise forbidden.
//                 {
//                   if( (isOutput ? t : track)->isCircularRoute(isOutput ? track : t) ) 
//                     act->setEnabled(false);
//                 }
// 
//                 ++id;
//               }
//             
//               chans -= 1;
//               if(chans > 0)
//               {
//                 // Ignore odd numbered left-over channels.
//                 //int schans = (chans & ~1) - 1;
//                 
//                 chpup->addSeparator();
//                 chpup->addAction(new MenuTitleItem("<Stereo>", chpup)); 
//               
//                 for(int ch = 0; ch < chans; ++ch)
//                 {
//                   //char buffer[128];
//                   //snprintf(buffer, 128, "%s %d,%d", tr("Channel").toLatin1().constData(), ch+1, ch+2);
//                   QString chBuffer = tr("Channel") + QString(" ") + QString::number(ch + 1) + QString(",") + QString::number(ch + 2);
//                   act = chpup->addAction(chBuffer);
//                   act->setCheckable(true);
//                   
//                   int ach = ch;
//                   int bch = -1;
//                   
//                   MusECore::Route rt(track, isOutput ? bch : ach, 2);
//                   
//                   rt.remoteChannel = isOutput ? ach : bch;
//                   
//                   act->setData(QVariant::fromValue(rt));   
//                   
//                   for(MusECore::ciRoute ir = rl->begin(); ir != rl->end(); ++ir) 
//                   {
//                     if(ir->type == MusECore::Route::TRACK_ROUTE && ir->track == track && ir->remoteChannel == rt.remoteChannel)
//                     {
//                       int tcompch = rt.channel;
//                       if(tcompch == -1)
//                         tcompch = 0;
//                       int tcompchs = rt.channels;
//                       if(tcompchs == -1)
//                         tcompchs = isOutput ? t->channels() : track->channels();
//                       
//                       int compch = ir->channel;
//                       if(compch == -1)
//                         compch = 0;
//                       int compchs = ir->channels;
//                       if(compchs == -1)
//                         compchs = isOutput ? t->channels() : ir->track->channels();
//                       
//                       if(compch == tcompch && compchs == tcompchs) 
//                       {
//                         act->setChecked(true);
//                         break;
//                       }
//                     }  
//                   }
//                   
//                   if(!act->isChecked())  // If circular route exists, allow user to break it, otherwise forbidden.
//                   {
//                     if( (isOutput ? t : track)->isCircularRoute(isOutput ? track : t) ) 
//                       act->setEnabled(false);
//                   }
// 
//                   ++id;
//                 }
//               }
//               
//               lb->addMenu(chpup);
//             }
//       }
//       return id;      
// }

//---------------------------------------------------------
//   addMidiPorts
//---------------------------------------------------------

int RoutePopupMenu::addMidiPorts(MusECore::AudioTrack* t, PopupMenu* pup, int id, bool isOutput)
{

#ifndef _USE_CUSTOM_WIDGET_ACTIONS_

  QAction* act;
  
#endif
  
#ifdef _USE_CUSTOM_WIDGET_ACTIONS_

        PixmapButtonsHeaderWidgetAction* wa_hdr = new PixmapButtonsHeaderWidgetAction("Output port/device", darkRedLedIcon, MIDI_CHANNELS, pup);
        pup->addAction(wa_hdr);  
        ++id;
#else   
      pup->addAction(new MenuTitleItem(qApp->translate("@default", QT_TRANSLATE_NOOP("@default", "Output port/device")), pup)); 
#endif
        
  for(int i = 0; i < MIDI_PORTS; ++i)
  {
    MusECore::MidiPort* mp = &MusEGlobal::midiPorts[i];
    MusECore::MidiDevice* md = mp->device();
    
    // This is desirable, but could lead to 'hidden' routes unless we add more support
    //  such as removing the existing routes when user changes flags.
    // So for now, just list all valid ports whether read or write.
    //if(!md)
    //  continue;
    if(!md || !(md->rwFlags() & (isOutput ? 2 : 1)))  // If this is an input click we are looking for midi outputs here.
      continue;
          
    // Do not list synth devices!
    if(md->isSynti())
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
    
#ifdef _USE_CUSTOM_WIDGET_ACTIONS_

    QBitArray ba(MIDI_CHANNELS); 
    for(int mch = 0; mch < MIDI_CHANNELS; ++mch)
    {  
      if(chanmask & (1 << mch))
        ba.setBit(mch);
    }
    PixmapButtonsWidgetAction* wa = new PixmapButtonsWidgetAction(QString::number(i + 1) + ":" + (md ? md->name() : tr("<none>")), 
                                                                  redLedIcon, darkRedLedIcon, ba, pup);
    MusECore::Route srcRoute(i, 0); // Ignore the routing channels - our action holds the channels.   
    //wa->setData(id++);   
    wa->setData(QVariant::fromValue(srcRoute));   
    pup->addAction(wa);  
    ++id;

#else    

    PopupMenu* subp = new PopupMenu(pup, true);
    subp->setTitle(md->name()); 
    
    for(int ch = 0; ch < MIDI_CHANNELS; ++ch) 
    {
      act = subp->addAction(QString("Channel %1").arg(ch+1));
      act->setCheckable(true);
      
      int chbit = 1 << ch;
      MusECore::Route srcRoute(i, chbit);    // In accordance with channel mask, use the bit position.
      
      act->setData(QVariant::fromValue(srcRoute));   
      
      if(chanmask & chbit)                  // Is the channel already set? Show item check mark.
        act->setChecked(true);
      
      ++id;
    }
    
    //gid = MIDI_PORTS * MIDI_CHANNELS + i;           // Make sure each 'toggle' item gets a unique id.
    act = subp->addAction(QString("Toggle all"));
    //act->setCheckable(true);
    MusECore::Route togRoute(i, (1 << MIDI_CHANNELS) - 1);    // Set all channel bits.
    act->setData(QVariant::fromValue(togRoute));   
    ++id;
    
    pup->addMenu(subp);
    
#endif // _USE_CUSTOM_WIDGET_ACTIONS_    
    
  }    
  return id;      
}

// REMOVE Tim. Persistent routes. Added.
//---------------------------------------------------------
//   addSynthPorts
//---------------------------------------------------------

int RoutePopupMenu::addSynthPorts(MusECore::AudioTrack* t, MusEGui::PopupMenu* lb, int id, int channel, int channels, bool isOutput)
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

//======================
// RoutePopupMenu
//======================

// REMOVE Tim. Persistent routes. Added.
RoutePopupMenu::RoutePopupMenu(QWidget* parent, bool isOutput)
               : PopupMenu(parent, true), _isOutMenu(isOutput)
{
  init();
}

// REMOVE Tim. Persistent routes. Changed.
//RoutePopupMenu::RoutePopupMenu(QWidget* parent, MusECore::Track* track, bool isOutput) 
RoutePopupMenu::RoutePopupMenu(const MusECore::Route& route, QWidget* parent, bool isOutput) 
               //: PopupMenu(parent, true), _track(track), _isOutMenu(isOutput)
               : PopupMenu(parent, true), _route(route), _isOutMenu(isOutput)
{
  init();
}

// REMOVE Tim. Persistent routes. Changed.
//RoutePopupMenu::RoutePopupMenu(const QString& title, QWidget* parent, MusECore::Track* track, bool isOutput)
RoutePopupMenu::RoutePopupMenu(const MusECore::Route& route, const QString& title, QWidget* parent, bool isOutput)
               //: PopupMenu(title, parent, true), _track(track), _isOutMenu(isOutput)
               : PopupMenu(title, parent, true), _route(route), _isOutMenu(isOutput)
{
  init();        
}

void RoutePopupMenu::init()
{
  //printf("RoutePopupMenu::init this:%p\n", this);  
  connect(MusEGlobal::song, SIGNAL(songChanged(MusECore::SongChangedFlags_t)), SLOT(songChanged(MusECore::SongChangedFlags_t)));
}

void RoutePopupMenu::resizeEvent(QResizeEvent* e)
{
  fprintf(stderr, "RoutePopupMenu::resizeEvent\n");
  PopupMenu::resizeEvent(e);
}

void RoutePopupMenu::songChanged(MusECore::SongChangedFlags_t val)
{
  if(val & (SC_ROUTE | SC_CHANNELS | SC_CONFIG))     
    updateRouteMenus();             
}

// REMOVE Tim. Persistent routes. Added.
bool RoutePopupMenu::updateItemTexts(PopupMenu* menu)
{
  if(!menu)
    menu = this;
  QList<QAction*> list = menu->actions();
  bool changed = false;
  for(int i = 0; i < list.size(); ++i)
  {
    QAction* act = list[i];
    PopupMenu* pup = dynamic_cast<PopupMenu*>(act->menu());
    if(pup)
    {
      const bool res = updateItemTexts(pup); // Recursive.
      if(res)
        changed = true;
    }

    RoutingMatrixWidgetAction* wa = dynamic_cast<RoutingMatrixWidgetAction*>(act);
    if(wa)
    {
      // Take care of struct Route first. Insert other future custom structures here too !
      //if(act->data().canConvert<MusECore::Route>() && v.canConvert<MusECore::Route>())
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
              const int h_rows = wa->header()->rows();
              for(int row = 0; row < h_rows; ++row)
              {
                const QString str = wa->header()->text(row, -1);
                const char* port_name = str.toLatin1().constData();
                void* const port = MusEGlobal::audioDevice->findPort(port_name);
                if(port)
                {
                  MusEGlobal::audioDevice->portName(port, good_name, ROUTE_PERSISTENT_NAME_SIZE, preferredRouteNameOrAlias);
                  port_name = good_name;
                  if(str != port_name)
                  {
                    wa->header()->setText(row, -1, port_name);
                    // REMOVE Tim. Persistent routes. Changed. JUST FOR TEST.
                    //wa->header()->setText(row, -1, "GRNIGDMIGDMIDG<HD<P>PSF<PSFMPFAMPADODOP{F{DFD{SD{FF{F{R{R{R{REPORIERIRIOWIEOERIOERIOWRIOIEWRIOWIROIWRJFDSFKJSFDKJ");
                    changed = true;
                  }
                }
              }
              if(changed)
              {
                wa->updateChannelArray();
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
    //if(act->data().canConvert<MusECore::Route>() && v.canConvert<MusECore::Route>())
    if(act->data().canConvert<MusECore::Route>())
    {
      const MusECore::Route r = act->data().value<MusECore::Route>();
      switch(r.type)
      {
        case MusECore::Route::JACK_ROUTE:
          act->setText(r.name(preferredRouteNameOrAlias));
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
          for(int ch = 0; ch < MIDI_CHANNELS; ++ch)
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
      for(int ch = 0; ch < MIDI_CHANNELS; ++ch)
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
      
      printf("RoutePopupMenu::updateRouteMenus other irl type:%d\n", irl->type);  // REMOVE TIm.
      if(act)
      {  
        //printf("RoutePopupMenu::updateRouteMenus found other irl type:%d\n", irl->type);  // 
        act->setChecked(true);
      }
    }
  }
*/  
}      

// REMOVE Tim. Persistent routes. Changed.
// void RoutePopupMenu::popupActivated(QAction* action)
// {
//   if(!action || !_track || actions().isEmpty())
//     return;
// 
//   // Make sure the track still exists.
//   if(MusEGlobal::song->tracks()->find(_track) == MusEGlobal::song->tracks()->end())
//     return;
//   
//   // REMOVE Tim. Persistent routes. Added.
//   fprintf(stderr, "RoutePopupMenu::popupActivated: action text:%s checked:%d name:%s\n", 
//           action->text().toLatin1().constData(), action->isChecked(), 
//           action->objectName().toLatin1().constData());
//   
//   MusECore::PendingOperationList operations;
//   
//   if(_track->isMidiTrack())
//   {
//     MusECore::RouteList* rl = _isOutMenu ? _track->outRoutes() : _track->inRoutes();
//     
//     // Take care of Route data items first... 
//     if(action->data().canConvert<MusECore::Route>())
//     {
//       MusECore::Route aRoute = action->data().value<MusECore::Route>();
//       
//       // Support Midi Port to Audio Input track routes. 
//       if(aRoute.type == MusECore::Route::TRACK_ROUTE && aRoute.track && aRoute.track->type() == MusECore::Track::AUDIO_INPUT)
//       {
//         //if(gIsOutRoutingPopupMenu)    // Try to avoid splitting like this. 
//         {
//           int chbit = aRoute.channel;
//           int port = ((MusECore::MidiTrack*)_track)->outPort();
//           if(port < 0 || port >= MIDI_PORTS)
//             return;
//           
//           MusECore::MidiPort* mp = &MusEGlobal::midiPorts[port];
//           //MusECore::MidiDevice* md = mp->device();
//           
//           // This is desirable, but could lead to 'hidden' routes unless we add more support
//           //  such as removing the existing routes when user changes flags.
//           // So for now, just list all valid ports whether read or write.
//           //if(!md)
//           //  return;
//           //if(!(md->rwFlags() & (gIsOutRoutingPopupMenu ? 1 : 2)))
//           //  return;
//           
//           MusECore::Route bRoute(port, chbit);
//           
//           int chmask = 0;                   
//           MusECore::RouteList* mprl = _isOutMenu ? mp->outRoutes() : mp->inRoutes();
//           MusECore::ciRoute ir = mprl->begin();
//           for (; ir != mprl->end(); ++ir) 
//             if(ir->type == MusECore::Route::TRACK_ROUTE && ir->track == aRoute.track) {   // Is there already a route to this port?
//               chmask = ir->channel;  // Grab the channel mask.
//               break;
//             }      
//           if ((chmask & chbit) == chbit)             // Is the channel's bit(s) set?
//           {
//             // disconnect
//             if(_isOutMenu)
//               //MusEGlobal::audio->msgRemoveRoute(bRoute, aRoute);
//               operations.add(MusECore::PendingOperationItem(bRoute, aRoute, MusECore::PendingOperationItem::DeleteRoute));
//             else
//               //MusEGlobal::audio->msgRemoveRoute(aRoute, bRoute);
//               operations.add(MusECore::PendingOperationItem(aRoute, bRoute, MusECore::PendingOperationItem::DeleteRoute));
//           }
//           else 
//           {
//             // connect
//             if(_isOutMenu)
//               //MusEGlobal::audio->msgAddRoute(bRoute, aRoute);
//               operations.add(MusECore::PendingOperationItem(bRoute, aRoute, MusECore::PendingOperationItem::AddRoute));
//             else
//               //MusEGlobal::audio->msgAddRoute(aRoute, bRoute);
//               operations.add(MusECore::PendingOperationItem(aRoute, bRoute, MusECore::PendingOperationItem::AddRoute));
//           }
//           
//           //MusEGlobal::audio->msgUpdateSoloStates();
//           //MusEGlobal::song->update(SC_ROUTE);
//           
//         }
//         //return;
//       }
//       // Support Audio Input track to Midi Port routes. 
//       else if(aRoute.type == MusECore::Route::MIDI_PORT_ROUTE)
//       {
//         // Check for custom midi channel select action.
//         PixmapButtonsWidgetAction* cs_wa = dynamic_cast<PixmapButtonsWidgetAction*>(action);
//         if(cs_wa)
//         {
//           MusECore::Route aRoute = action->data().value<MusECore::Route>();
//           const QBitArray ba = cs_wa->currentState();
//           const int ba_sz = ba.size();
//           int chbits = 0;
//           for(int mch = 0; mch < MIDI_CHANNELS && mch < ba_sz; ++mch)
//           {
//             fprintf(stderr, "RoutePopupMenu::popupActivated: mch:%d", mch); // REMOVE Tim. Persistent routes. Added. 
//             if(ba.at(mch))
//             {
//               fprintf(stderr, " bit is set"); // REMOVE Tim. Persistent routes. Added. 
//               chbits |= (1 << mch);
//             }
//             fprintf(stderr, "\n"); // REMOVE Tim. Persistent routes. Added. 
//           }
//           fprintf(stderr, " chbits:%d\n", chbits); // REMOVE Tim. Persistent routes. Added. 
//             
//           aRoute.channel = chbits;  // Restore the desired route channels from the custom widget action state.
//           
//           int mdidx = aRoute.midiPort;
//           MusECore::MidiPort* mp = &MusEGlobal::midiPorts[mdidx];
//   
//           MusECore::MidiDevice* md = mp->device();
//           //if(!md)    // Rem. Allow connections to ports with no device.
//           //  return;
//           
//           //if(!(md->rwFlags() & 2))
//           //if(!(md->rwFlags() & (gIsOutRoutingPopupMenu ? 1 : 2)))
//           if(md && !(md->rwFlags() & (_isOutMenu ? 1 : 2)))   
//               return;
//           
//           int chmask = 0;                   
//           MusECore::ciRoute iir = rl->begin();
//           for (; iir != rl->end(); ++iir) 
//             if(iir->type == MusECore::Route::MIDI_PORT_ROUTE && iir->midiPort == mdidx) {   // Is there already a route to this port?
//               chmask = iir->channel;  // Grab the channel mask.
//               break;  
//             }      
//           
//           // Only if something changed...
//           if(chmask != chbits)
//           {
//             if(chmask != 0)
//             {
//               MusECore::Route bRoute(_track, chmask);
//               // Disconnect all existing channels.
//               if(_isOutMenu)
//                 //MusEGlobal::audio->msgRemoveRoute(bRoute, *iir);
//                 operations.add(MusECore::PendingOperationItem(bRoute, *iir, MusECore::PendingOperationItem::DeleteRoute));
//               else
//                 //MusEGlobal::audio->msgRemoveRoute(*iir, bRoute);
//                 operations.add(MusECore::PendingOperationItem(*iir, bRoute, MusECore::PendingOperationItem::DeleteRoute));
//             }  
//             if(chbits != 0)
//             {
//               // Connect desired channels.
//               MusECore::Route bRoute(_track, chbits);
//               if(_isOutMenu)
//                 //MusEGlobal::audio->msgAddRoute(bRoute, aRoute);
//                 operations.add(MusECore::PendingOperationItem(bRoute, aRoute, MusECore::PendingOperationItem::AddRoute));
//               else
//                 //MusEGlobal::audio->msgAddRoute(aRoute, bRoute);
//                 operations.add(MusECore::PendingOperationItem(aRoute, bRoute, MusECore::PendingOperationItem::AddRoute));
//             }
//             //MusEGlobal::audio->msgUpdateSoloStates();
//             //MusEGlobal::song->update(SC_ROUTE);
//           }  
//           //return;
//         }
//         else
//         {
//           int chbit = aRoute.channel;
//           MusECore::Route bRoute(_track, chbit);
//           int mdidx = aRoute.midiPort;
//   
//           MusECore::MidiPort* mp = &MusEGlobal::midiPorts[mdidx];
//           MusECore::MidiDevice* md = mp->device();
//           //if(!md)    // Rem. Allow connections to ports with no device.
//           //  return;
//           
//           //if(!(md->rwFlags() & 2))
//           //if(!(md->rwFlags() & (gIsOutRoutingPopupMenu ? 1 : 2)))
//           if(md && !(md->rwFlags() & (_isOutMenu ? 1 : 2)))   
//               return;
//           
//           int chmask = 0;                   
//           MusECore::ciRoute iir = rl->begin();
//           for (; iir != rl->end(); ++iir) 
//           {
//             if(iir->type == MusECore::Route::MIDI_PORT_ROUTE && iir->midiPort == mdidx)    // Is there already a route to this port?
//             {
//                   chmask = iir->channel;  // Grab the channel mask.
//                   break;
//             }      
//           }
//           if ((chmask & chbit) == chbit)             // Is the channel's bit(s) set?
//           {
//             // disconnect
//             if(_isOutMenu)
//               //MusEGlobal::audio->msgRemoveRoute(bRoute, aRoute);
//               operations.add(MusECore::PendingOperationItem(bRoute, aRoute, MusECore::PendingOperationItem::DeleteRoute));
//             else
//               //MusEGlobal::audio->msgRemoveRoute(aRoute, bRoute);
//               operations.add(MusECore::PendingOperationItem(aRoute, bRoute, MusECore::PendingOperationItem::DeleteRoute));
//           }
//           else 
//           {
//             // connect
//             if(_isOutMenu)
//               //MusEGlobal::audio->msgAddRoute(bRoute, aRoute);
//               operations.add(MusECore::PendingOperationItem(bRoute, aRoute, MusECore::PendingOperationItem::AddRoute));
//             else
//               //MusEGlobal::audio->msgAddRoute(aRoute, bRoute);
//               operations.add(MusECore::PendingOperationItem(aRoute, bRoute, MusECore::PendingOperationItem::AddRoute));
//           }
//           
//           //MusEGlobal::audio->msgUpdateSoloStates();
//           //MusEGlobal::song->update(SC_ROUTE);
//         }
//       }  
//     }  
//     // ... now take care of integer data items.
//     else if(action->data().canConvert<int>())
//     {
//       int n = action->data().value<int>();
//       if(!_isOutMenu && n == 0)
//         MusEGlobal::muse->configMidiPorts();
//       return;  
//     }
//   }
//   else
//   {
//     MusECore::AudioTrack* t = (MusECore::AudioTrack*)_track;
//     MusECore::RouteList* rl = _isOutMenu ? t->outRoutes() : t->inRoutes();
//     
//     // REMOVE Tim. Persistent routes. Changed.
//     //if(!action->data().canConvert<MusECore::Route>())
//     //  return; 
//     // Take care of Route data items first... 
//     if(action->data().canConvert<MusECore::Route>())
//     {  
//       fprintf(stderr, "RoutePopupMenu::popupActivated: action data is a Route\n"); // REMOVE Tim. Persistent routes. Added. 
// 
//       MusECore::Route rem_route = action->data().value<MusECore::Route>();
//       // REMOVE Tim. Persistent routes. Added.
//       // Check for custom routing matrix action.
//       RoutingMatrixWidgetAction* matrix_wa = dynamic_cast<RoutingMatrixWidgetAction*>(action);
//       if(matrix_wa)
//       {
//         fprintf(stderr, "RoutePopupMenu::popupActivated: Matrix\n"); // REMOVE Tim. Persistent routes. Added. 
//         //MusECore::Track* rem_track = rem_route.type == MusECore::Route::TRACK_ROUTE ? rem_route.track : 0;
//         //const MusECore::RouteList* const rem_rl = rem_track ? (_isOutMenu ? rem_track->outRoutes() : rem_track->inRoutes()) : 0;
//         
//         //MusECore::RouteList* rem_rl = _isOutMenu ? t->inRoutes() : t->outRoutes();
//         
//         // Make sure the track still exists.
//         if(rem_route.type == MusECore::Route::TRACK_ROUTE && 
//             MusEGlobal::song->tracks()->find(rem_route.track) != MusEGlobal::song->tracks()->end())
//         {
//           const int rows = matrix_wa->array()->rows();
//           const int cols = matrix_wa->array()->columns();
//           for(int row = 0; row < rows; ++row)
//           {
//             for(int col = 0; col < cols; ++col)
//             {
//               //rem_route.channel = col;
//               //rem_route.remoteChannel = row;
//               //rem_route.channels = 1;
//               
//               // Check if the remote route exists in the route list.
// //                 const bool rem_found = rl->exists(rem_route);
// //                 MusECore::Route this_route(t, row, 1);
// //                 this_route.remoteChannel = col;
// //                 const bool this_found = rem_rl ? rem_rl->exists(this_route) : false;
// //                 
// //                 // Rearrange the parameters suitable for the addRoute and deleteRoute commands.
// //                 this_route.channel = col;
// //                 this_route.remoteChannel = -1;
// //                 rem_route.channel = row;
// //                 rem_route.remoteChannel = -1;
//               MusECore::Route this_route(t, col, 1);
//               //this_route.remoteChannel = row;
//               rem_route.channel = row;
//               rem_route.channels = 1;
//               
//               const MusECore::Route& src = _isOutMenu ? this_route : rem_route;
//               const MusECore::Route& dst = _isOutMenu ? rem_route : this_route;
//               
//               fprintf(stderr, "RoutePopupMenu::popupActivated: Matrix: checking operations\n"); // REMOVE Tim. Persistent routes. Added. 
//               const bool val = matrix_wa->array()->value(row, col);
//               // Connect if route does not exist. Allow it to reconnect a partial route.
//               //if(val && (!rem_found || !this_found))
//               if(val && MusECore::routeCanConnect(src, dst))
//               {
//                 fprintf(stderr, "RoutePopupMenu::popupActivated: Matrix: adding AddRoute operation\n"); // REMOVE Tim. Persistent routes. Added. 
//                 operations.add(MusECore::PendingOperationItem(src, dst, MusECore::PendingOperationItem::AddRoute));
//               }
//               // Disconnect if route exists. Allow it to reconnect a partial route.
//               else if(!val && MusECore::routeCanDisconnect(src, dst))
//               {
//                 fprintf(stderr, "RoutePopupMenu::popupActivated: Matrix: adding DeleteRoute operation\n"); // REMOVE Tim. Persistent routes. Added. 
//                 operations.add(MusECore::PendingOperationItem(src, dst, MusECore::PendingOperationItem::DeleteRoute));
//               }
//             }
//           }
//         }
//       }
//       // Support Midi Port to Audio Input routes. 
//       else if(!_isOutMenu && _track->type() == MusECore::Track::AUDIO_INPUT && rem_route.type == MusECore::Route::MIDI_PORT_ROUTE)
//       {
//         // Check for custom midi channel select action.
//         PixmapButtonsWidgetAction* cs_wa = dynamic_cast<PixmapButtonsWidgetAction*>(action);
//         if(cs_wa)
//         {
//           const QBitArray ba = cs_wa->currentState();
//           const int ba_sz = ba.size();
//           int chbits = 0;
//           for(int mch = 0; mch < MIDI_CHANNELS && mch < ba_sz; ++mch)
//           {
//             if(ba.at(mch))
//               chbits |= (1 << mch);
//           }
// 
//           rem_route.channel = chbits;  // Restore the desired route channels from the custom widget action state.
//           int mdidx = rem_route.midiPort;
// 
//           int chmask = 0;                   
//           MusECore::ciRoute iir = rl->begin();
//           for (; iir != rl->end(); ++iir) 
//             if(iir->type == MusECore::Route::MIDI_PORT_ROUTE && iir->midiPort == mdidx) {   // Is there already a route to this port?
//               chmask = iir->channel;  // Grab the channel mask.
//               break;
//             }      
//           
//           // Only if something changed...
//           if(chmask != chbits)
//           {
//             if(chmask != 0)
//             {
//               // Disconnect all existing channels.
//               MusECore::Route dstRoute(t, chmask);
//               //MusEGlobal::audio->msgRemoveRoute(*iir, dstRoute);
//               operations.add(MusECore::PendingOperationItem(*iir, dstRoute, MusECore::PendingOperationItem::DeleteRoute));
//             }
//             if(chbits != 0)
//             {
//               // Connect desired channels.
//               MusECore::Route dstRoute(t, chbits);
//               //MusEGlobal::audio->msgAddRoute(rem_route, dstRoute);
//               operations.add(MusECore::PendingOperationItem(rem_route, dstRoute, MusECore::PendingOperationItem::AddRoute));
//             }
//             //MusEGlobal::audio->msgUpdateSoloStates();
//             //MusEGlobal::song->update(SC_ROUTE);
//           }  
//           //return;
//         }
//         else  
//         {
//           int chbit = rem_route.channel;
//           MusECore::Route dstRoute(t, chbit);
//           int mdidx = rem_route.midiPort;
//           int chmask = 0;                   
//           MusECore::ciRoute iir = rl->begin();
//           for (; iir != rl->end(); ++iir) 
//           {
//             if(iir->type == MusECore::Route::MIDI_PORT_ROUTE && iir->midiPort == mdidx)    // Is there already a route to this port?
//             {
//               chmask = iir->channel;  // Grab the channel mask.
//               break;
//             }      
//           }
//           
//           if ((chmask & chbit) == chbit)             // Is the channel's bit(s) set?
//           {
//             //printf("routingPopupMenuActivated: removing src route ch:%d dst route ch:%d\n", srcRoute.channel, dstRoute.channel); 
//             //MusEGlobal::audio->msgRemoveRoute(rem_route, dstRoute);
//             operations.add(MusECore::PendingOperationItem(rem_route, dstRoute, MusECore::PendingOperationItem::DeleteRoute));
//           }
//           else 
//           {
//             //printf("routingPopupMenuActivated: adding src route ch:%d dst route ch:%d\n", srcRoute.channel, dstRoute.channel); 
//             //MusEGlobal::audio->msgAddRoute(rem_route, dstRoute);
//             operations.add(MusECore::PendingOperationItem(rem_route, dstRoute, MusECore::PendingOperationItem::AddRoute));
//           }
//           
//           //MusEGlobal::audio->msgUpdateSoloStates();
//           //MusEGlobal::song->update(SC_ROUTE);
//           //return;
//         }  
//       }
//       else
//       {
//         
//         MusECore::Route this_route(t, rem_route.channel, rem_route.channels);
//         this_route.remoteChannel = rem_route.remoteChannel;
// //         switch(rem_route.type)
// //         {
// //           case MusECore::Route::JACK_ROUTE:
// //             // Clear the information.
// //             rem_route.channel = -1;
// //             rem_route.remoteChannel = -1;
// //             rem_route.channels = -1;
// //           break;
// //           
// //           case MusECore::Route::TRACK_ROUTE:
// //           case MusECore::Route::MIDI_DEVICE_ROUTE:
// //           case MusECore::Route::MIDI_PORT_ROUTE:
// //           break;
// //         }
// 
//         const MusECore::Route& src = _isOutMenu ? this_route : rem_route;
//         const MusECore::Route& dst = _isOutMenu ? rem_route : this_route;
//         
//         // Connect if route does not exist. Allow it to reconnect a partial route.
//         if(action->isChecked() && MusECore::routeCanConnect(src, dst))
//         {
//           fprintf(stderr, "RoutePopupMenu::popupActivated: Route: adding AddRoute operation\n"); // REMOVE Tim. Persistent routes. Added. 
//           operations.add(MusECore::PendingOperationItem(src, dst, MusECore::PendingOperationItem::AddRoute));
//         }
//         // Disconnect if route exists. Allow it to reconnect a partial route.
//         else if(!action->isChecked() && MusECore::routeCanDisconnect(src, dst))
//         {
//           fprintf(stderr, "RoutePopupMenu::popupActivated: Route: adding DeleteRoute operation\n"); // REMOVE Tim. Persistent routes. Added. 
//           operations.add(MusECore::PendingOperationItem(src, dst, MusECore::PendingOperationItem::DeleteRoute));
//         }
//       }
//     }         
// //               if(_isOutMenu)
// //               {  
// //                 // REMOVE Tim. Persistent routes. Changed.
// //                 //MusECore::Route srcRoute(t, rem_route.channel, rem_route.channels);
// //                 //srcRoute.remoteChannel = rem_route.remoteChannel;
// //                 //MusECore::Route srcRoute(t, rem_route.remoteChannel, rem_route.channels);
// //                 //srcRoute.remoteChannel = rem_route.channel;
// //         
// //                 // check if route src->dst exists:
// //     //             MusECore::ciRoute irl = rl->begin();
// //     //             for (; irl != rl->end(); ++irl) {
// //     //                   if (*irl == rem_route)
// //     //                         break;
// //     //                   }
// //     //             if (irl != rl->end()) {
// //     //             if (rl->exists(rem_route)) {
// //     //                   // disconnect if route exists
// //     //                   MusEGlobal::audio->msgRemoveRoute(srcRoute, rem_route);
// //     //                   }
// //     //             else {
// //     //                   // connect if route does not exist
// //     //                   MusEGlobal::audio->msgAddRoute(srcRoute, rem_route);
// //     //                   }
// //     //             MusEGlobal::audio->msgUpdateSoloStates();
// //     //             MusEGlobal::song->update(SC_ROUTE);
// //                 
// //               }
// //             }
// //             else
// //             {
// //               // Support Midi Port to Audio Input routes. 
// //               if(_track->type() == MusECore::Track::AUDIO_INPUT && rem_route.type == MusECore::Route::MIDI_PORT_ROUTE)
// //               {
// //                 // Check for custom midi channel select action.
// //                 PixmapButtonsWidgetAction* cs_wa = dynamic_cast<PixmapButtonsWidgetAction*>(action);
// //                 if(cs_wa)
// //                 {
// //                   const QBitArray ba = cs_wa->currentState();
// //                   const int ba_sz = ba.size();
// //                   int chbits = 0;
// //                   for(int mch = 0; mch < MIDI_CHANNELS && mch < ba_sz; ++mch)
// //                   {
// //                     if(ba.at(mch))
// //                       chbits |= (1 << mch);
// //                   }
// // 
// //                   rem_route.channel = chbits;  // Restore the desired route channels from the custom widget action state.
// //                   int mdidx = rem_route.midiPort;
// // 
// //                   int chmask = 0;                   
// //                   MusECore::ciRoute iir = rl->begin();
// //                   for (; iir != rl->end(); ++iir) 
// //                     if(iir->type == MusECore::Route::MIDI_PORT_ROUTE && iir->midiPort == mdidx) {   // Is there already a route to this port?
// //                       chmask = iir->channel;  // Grab the channel mask.
// //                       break;
// //                     }      
// //                   
// //                   // Only if something changed...
// //                   if(chmask != chbits)
// //                   {
// //                     if(chmask != 0)
// //                     {
// //                       // Disconnect all existing channels.
// //                       MusECore::Route dstRoute(t, chmask);
// //                       MusEGlobal::audio->msgRemoveRoute(*iir, dstRoute);
// //                     }
// //                     if(chbits != 0)
// //                     {
// //                       // Connect desired channels.
// //                       MusECore::Route dstRoute(t, chbits);
// //                       MusEGlobal::audio->msgAddRoute(rem_route, dstRoute);
// //                     }
// //                     MusEGlobal::audio->msgUpdateSoloStates();
// //                     MusEGlobal::song->update(SC_ROUTE);
// //                   }  
// //                   return;
// //                 }
// //                 else  
// //                 {
// //                   int chbit = rem_route.channel;
// //                   MusECore::Route dstRoute(t, chbit);
// //                   int mdidx = rem_route.midiPort;
// //                   int chmask = 0;                   
// //                   MusECore::ciRoute iir = rl->begin();
// //                   for (; iir != rl->end(); ++iir) 
// //                   {
// //                     if(iir->type == MusECore::Route::MIDI_PORT_ROUTE && iir->midiPort == mdidx)    // Is there already a route to this port?
// //                     {
// //                       chmask = iir->channel;  // Grab the channel mask.
// //                       break;
// //                     }      
// //                   }
// //                   
// //                   if ((chmask & chbit) == chbit)             // Is the channel's bit(s) set?
// //                   {
// //                     //printf("routingPopupMenuActivated: removing src route ch:%d dst route ch:%d\n", srcRoute.channel, dstRoute.channel); 
// //                     MusEGlobal::audio->msgRemoveRoute(rem_route, dstRoute);
// //                   }
// //                   else 
// //                   {
// //                     //printf("routingPopupMenuActivated: adding src route ch:%d dst route ch:%d\n", srcRoute.channel, dstRoute.channel); 
// //                     MusEGlobal::audio->msgAddRoute(rem_route, dstRoute);
// //                   }
// //                   
// //                   MusEGlobal::audio->msgUpdateSoloStates();
// //                   MusEGlobal::song->update(SC_ROUTE);
// //                   return;
// //                 }  
// //               }
// //               
// //               // REMOVE Tim. Persistent routes. Changed.
// //               //MusECore::Route dstRoute(t, srcRoute.channel, srcRoute.channels);     
// //               //dstRoute.remoteChannel = srcRoute.remoteChannel;
// //               MusECore::Route dstRoute(t, rem_route.remoteChannel, rem_route.channels);     
// //               dstRoute.remoteChannel = rem_route.channel;
// //       
// //               MusECore::ciRoute irl = rl->begin();
// //               for (; irl != rl->end(); ++irl) {
// //                     if (*irl == rem_route)
// //                           break;
// //                     }
// //               if (irl != rl->end()) {
// //                     // disconnect
// //                     MusEGlobal::audio->msgRemoveRoute(rem_route, dstRoute);
// //                     }
// //               else {
// //                     // connect
// //                     MusEGlobal::audio->msgAddRoute(rem_route, dstRoute);
// //                     }
// //               MusEGlobal::audio->msgUpdateSoloStates();
// //               MusEGlobal::song->update(SC_ROUTE);
// //             }
// //           }
// //         }
//   }
//       
//   if(!operations.empty())
//   {
//     fprintf(stderr, "RoutePopupMenu::popupActivated: executing operations\n"); // REMOVE Tim. Persistent routes. Added. 
//     MusEGlobal::audio->msgExecutePendingOperations(operations);
//     MusEGlobal::audio->msgUpdateSoloStates(); // TODO Include this in operations.
//     MusEGlobal::song->update(SC_ROUTE);
//   }
// }
// 

void RoutePopupMenu::popupActivated(QAction* action)
{
  //if(!action || !_track || actions().isEmpty())
  if(!action || !_route.isValid() || actions().isEmpty())
    return;

  
  // Handle any non-route items.
  if(!action->data().canConvert<MusECore::Route>())
  {
    bool ok = false;
    const int n = action->data().toInt(&ok);
    if(ok)
    {
      switch(n)
      {
        
#ifdef _USE_CUSTOM_WIDGET_ACTIONS_
        case _ALIASES_WIDGET_ACTION_:
        {
          // Check for custom widget action.
          RoutingMatrixWidgetAction* wa = dynamic_cast<RoutingMatrixWidgetAction*>(action);
          if(wa)
          {
            if(wa->array()->value(0, 0))
              preferredRouteNameOrAlias = _PREFER_FIRST_ALIASES_ ;
            else if(wa->array()->value(0, 1))
              preferredRouteNameOrAlias = _PREFER_SECOND_ALIASES_;
            else 
              preferredRouteNameOrAlias = _PREFER_CANONICAL_NAMES_;
            if(updateItemTexts())
            {
              //adjustSize();
              //update();
            }
          }
        }
        break;
#endif
        
        case _SHOW_CANONICAL_NAMES_:
        {
          preferredRouteNameOrAlias = _PREFER_CANONICAL_NAMES_;
          if(updateItemTexts())
          {
            //adjustSize();
            //update();
          }
        }
        break;
        
        case _SHOW_FIRST_ALIASES_:
        {
          preferredRouteNameOrAlias = action->isChecked() ? _PREFER_FIRST_ALIASES_ : _PREFER_CANONICAL_NAMES_;
          if(updateItemTexts())
          {
            //adjustSize();
            //update();
          }
          // REMOVE Tim. Persistent routes. Added. JUST A TEST.
          //action->setText("SGDGJKSJKHFHKJ DDDDFKHDJGHGJKNKGDKKNG KKKKGNJDDDDDDDDDHF");
        }
        break;
        
        case _SHOW_SECOND_ALIASES_:
        {
          preferredRouteNameOrAlias = action->isChecked() ? _PREFER_SECOND_ALIASES_ : _PREFER_CANONICAL_NAMES_;
          if(updateItemTexts())
          {
            //adjustSize();
            //update();
          }
          // REMOVE Tim. Persistent routes. Added. JUST A TEST.
          //action->setText("dsjflksjsg fdjlgkjklgjlgdjgld hjlkjdlgkjfdglkdjgk dfjtlkjlrekjtklejterlktjer");
        }
        break;
        default:
        break;  
      }
    }

    return;
  }
  
  MusECore::PendingOperationList operations;
      
  switch(_route.type)
  {
    case MusECore::Route::TRACK_ROUTE:
    {
      MusECore::Track* track = _route.track;
      // Make sure the track still exists.
      if(MusEGlobal::song->tracks()->find(track) == MusEGlobal::song->tracks()->end())
        return;
      
      // REMOVE Tim. Persistent routes. Added.
      fprintf(stderr, "RoutePopupMenu::popupActivated: action text:%s checked:%d name:%s\n", 
              action->text().toLatin1().constData(), action->isChecked(), 
              action->objectName().toLatin1().constData());
      
      if(track->isMidiTrack())
      {
        MusECore::RouteList* rl = _isOutMenu ? track->outRoutes() : track->inRoutes();
        
        // Take care of Route data items first... 
        if(action->data().canConvert<MusECore::Route>())
        {
          MusECore::Route aRoute = action->data().value<MusECore::Route>();
          
          // Support Midi Port to Audio Input track routes. 
          if(aRoute.type == MusECore::Route::TRACK_ROUTE && aRoute.track && aRoute.track->type() == MusECore::Track::AUDIO_INPUT)
          {
            //if(gIsOutRoutingPopupMenu)    // Try to avoid splitting like this. 
            {
              int chbit = aRoute.channel;
              int port = ((MusECore::MidiTrack*)track)->outPort();
              if(port < 0 || port >= MIDI_PORTS)
                return;
              
              MusECore::MidiPort* mp = &MusEGlobal::midiPorts[port];
              //MusECore::MidiDevice* md = mp->device();
              
              // This is desirable, but could lead to 'hidden' routes unless we add more support
              //  such as removing the existing routes when user changes flags.
              // So for now, just list all valid ports whether read or write.
              //if(!md)
              //  return;
              //if(!(md->rwFlags() & (gIsOutRoutingPopupMenu ? 1 : 2)))
              //  return;
              
              MusECore::Route bRoute(port, chbit);
              
              int chmask = 0;                   
              MusECore::RouteList* mprl = _isOutMenu ? mp->outRoutes() : mp->inRoutes();
              MusECore::ciRoute ir = mprl->begin();
              for (; ir != mprl->end(); ++ir) 
                if(ir->type == MusECore::Route::TRACK_ROUTE && ir->track == aRoute.track) {   // Is there already a route to this port?
                  chmask = ir->channel;  // Grab the channel mask.
                  break;
                }      
              if ((chmask & chbit) == chbit)             // Is the channel's bit(s) set?
              {
                // disconnect
                if(_isOutMenu)
                  //MusEGlobal::audio->msgRemoveRoute(bRoute, aRoute);
                  operations.add(MusECore::PendingOperationItem(bRoute, aRoute, MusECore::PendingOperationItem::DeleteRoute));
                else
                  //MusEGlobal::audio->msgRemoveRoute(aRoute, bRoute);
                  operations.add(MusECore::PendingOperationItem(aRoute, bRoute, MusECore::PendingOperationItem::DeleteRoute));
              }
              else 
              {
                // connect
                if(_isOutMenu)
                  //MusEGlobal::audio->msgAddRoute(bRoute, aRoute);
                  operations.add(MusECore::PendingOperationItem(bRoute, aRoute, MusECore::PendingOperationItem::AddRoute));
                else
                  //MusEGlobal::audio->msgAddRoute(aRoute, bRoute);
                  operations.add(MusECore::PendingOperationItem(aRoute, bRoute, MusECore::PendingOperationItem::AddRoute));
              }
              
              //MusEGlobal::audio->msgUpdateSoloStates();
              //MusEGlobal::song->update(SC_ROUTE);
              
            }
            //return;
          }
          // Support Audio Input track to Midi Port routes. 
          else if(aRoute.type == MusECore::Route::MIDI_PORT_ROUTE)
          {
            // Check for custom midi channel select action.
            PixmapButtonsWidgetAction* cs_wa = dynamic_cast<PixmapButtonsWidgetAction*>(action);
            if(cs_wa)
            {
              MusECore::Route aRoute = action->data().value<MusECore::Route>();
              const QBitArray ba = cs_wa->currentState();
              const int ba_sz = ba.size();
              int chbits = 0;
              for(int mch = 0; mch < MIDI_CHANNELS && mch < ba_sz; ++mch)
              {
                fprintf(stderr, "RoutePopupMenu::popupActivated: mch:%d", mch); // REMOVE Tim. Persistent routes. Added. 
                if(ba.at(mch))
                {
                  fprintf(stderr, " bit is set"); // REMOVE Tim. Persistent routes. Added. 
                  chbits |= (1 << mch);
                }
                fprintf(stderr, "\n"); // REMOVE Tim. Persistent routes. Added. 
              }
              fprintf(stderr, " chbits:%d\n", chbits); // REMOVE Tim. Persistent routes. Added. 
                
              aRoute.channel = chbits;  // Restore the desired route channels from the custom widget action state.
              
              int mdidx = aRoute.midiPort;
              MusECore::MidiPort* mp = &MusEGlobal::midiPorts[mdidx];
      
              MusECore::MidiDevice* md = mp->device();
              //if(!md)    // Rem. Allow connections to ports with no device.
              //  return;
              
              //if(!(md->rwFlags() & 2))
              //if(!(md->rwFlags() & (gIsOutRoutingPopupMenu ? 1 : 2)))
              if(md && !(md->rwFlags() & (_isOutMenu ? 1 : 2)))   
                  return;
              
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
                  MusECore::Route bRoute(track, chmask);
                  // Disconnect all existing channels.
                  if(_isOutMenu)
                    //MusEGlobal::audio->msgRemoveRoute(bRoute, *iir);
                    operations.add(MusECore::PendingOperationItem(bRoute, *iir, MusECore::PendingOperationItem::DeleteRoute));
                  else
                    //MusEGlobal::audio->msgRemoveRoute(*iir, bRoute);
                    operations.add(MusECore::PendingOperationItem(*iir, bRoute, MusECore::PendingOperationItem::DeleteRoute));
                }  
                if(chbits != 0)
                {
                  // Connect desired channels.
                  MusECore::Route bRoute(track, chbits);
                  if(_isOutMenu)
                    //MusEGlobal::audio->msgAddRoute(bRoute, aRoute);
                    operations.add(MusECore::PendingOperationItem(bRoute, aRoute, MusECore::PendingOperationItem::AddRoute));
                  else
                    //MusEGlobal::audio->msgAddRoute(aRoute, bRoute);
                    operations.add(MusECore::PendingOperationItem(aRoute, bRoute, MusECore::PendingOperationItem::AddRoute));
                }
                //MusEGlobal::audio->msgUpdateSoloStates();
                //MusEGlobal::song->update(SC_ROUTE);
              }  
              //return;
            }
            else
            {
              int chbit = aRoute.channel;
              MusECore::Route bRoute(track, chbit);
              int mdidx = aRoute.midiPort;
      
              MusECore::MidiPort* mp = &MusEGlobal::midiPorts[mdidx];
              MusECore::MidiDevice* md = mp->device();
              //if(!md)    // Rem. Allow connections to ports with no device.
              //  return;
              
              //if(!(md->rwFlags() & 2))
              //if(!(md->rwFlags() & (gIsOutRoutingPopupMenu ? 1 : 2)))
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
                  //MusEGlobal::audio->msgRemoveRoute(bRoute, aRoute);
                  operations.add(MusECore::PendingOperationItem(bRoute, aRoute, MusECore::PendingOperationItem::DeleteRoute));
                else
                  //MusEGlobal::audio->msgRemoveRoute(aRoute, bRoute);
                  operations.add(MusECore::PendingOperationItem(aRoute, bRoute, MusECore::PendingOperationItem::DeleteRoute));
              }
              else 
              {
                // connect
                if(_isOutMenu)
                  //MusEGlobal::audio->msgAddRoute(bRoute, aRoute);
                  operations.add(MusECore::PendingOperationItem(bRoute, aRoute, MusECore::PendingOperationItem::AddRoute));
                else
                  //MusEGlobal::audio->msgAddRoute(aRoute, bRoute);
                  operations.add(MusECore::PendingOperationItem(aRoute, bRoute, MusECore::PendingOperationItem::AddRoute));
              }
              
              //MusEGlobal::audio->msgUpdateSoloStates();
              //MusEGlobal::song->update(SC_ROUTE);
            }
          }  
        }  
        // ... now take care of integer data items.
        else if(action->data().canConvert<int>())
        {
          int n = action->data().value<int>();
          if(!_isOutMenu && n == 0)
            MusEGlobal::muse->configMidiPorts();
          return;  
        }
      }
      else
      {
        MusECore::AudioTrack* t = (MusECore::AudioTrack*)track;
        MusECore::RouteList* rl = _isOutMenu ? t->outRoutes() : t->inRoutes();
        
        // REMOVE Tim. Persistent routes. Changed.
        //if(!action->data().canConvert<MusECore::Route>())
        //  return; 
        // Take care of Route data items first... 
        if(action->data().canConvert<MusECore::Route>())
        {  
          fprintf(stderr, "RoutePopupMenu::popupActivated: action data is a Route\n"); // REMOVE Tim. Persistent routes. Added. 

          MusECore::Route rem_route = action->data().value<MusECore::Route>();
          // REMOVE Tim. Persistent routes. Added.
          // Check for custom routing matrix action.
          RoutingMatrixWidgetAction* matrix_wa = dynamic_cast<RoutingMatrixWidgetAction*>(action);
          if(matrix_wa)
          {
            fprintf(stderr, "RoutePopupMenu::popupActivated: Matrix\n"); // REMOVE Tim. Persistent routes. Added. 
            //MusECore::Track* rem_track = rem_route.type == MusECore::Route::TRACK_ROUTE ? rem_route.track : 0;
            //const MusECore::RouteList* const rem_rl = rem_track ? (_isOutMenu ? rem_track->outRoutes() : rem_track->inRoutes()) : 0;
            
            //MusECore::RouteList* rem_rl = _isOutMenu ? t->inRoutes() : t->outRoutes();

            const int rows = matrix_wa->array()->rows();
            const int cols = matrix_wa->array()->columns();
            
            switch(rem_route.type)
            {
              case MusECore::Route::JACK_ROUTE:
              {
                if(MusEGlobal::checkAudioDevice())
                {
                  //char good_name[ROUTE_PERSISTENT_NAME_SIZE];
                  for(int row = 0; row < rows; ++row)
                  {
                    const QString str = matrix_wa->header()->text(row, -1);
                    if(!str.isEmpty())
                    {
                      const char* port_name = str.toLatin1().constData();
                      void* const port = MusEGlobal::audioDevice->findPort(port_name);
                      if(port)
                      {
                        //MusEGlobal::audioDevice->portName(port, good_name, ROUTE_PERSISTENT_NAME_SIZE);
                        //port_name = good_name;
                      //}
                      
                        for(int col = 0; col < cols; ++col)
                        {
                          const MusECore::Route this_route(t, col, 1);
                          //this_route.remoteChannel = row;
                          //rem_route.channel = row;
                          //rem_route.channel = col;
                          //rem_route.channels = 1;
                          //const MusECore::Route r_route(MusECore::Route::JACK_ROUTE, -1, NULL, col, -1, -1, port_name);
                          const MusECore::Route r_route(port);
                          
                          //const MusECore::Route& src = _isOutMenu ? this_route : rem_route;
                          //const MusECore::Route& dst = _isOutMenu ? rem_route : this_route;
                          const MusECore::Route& src = _isOutMenu ? this_route : r_route;
                          const MusECore::Route& dst = _isOutMenu ? r_route : this_route;
                          
                          fprintf(stderr, "RoutePopupMenu::popupActivated: Jack: Matrix: checking operations\n"); // REMOVE Tim. Persistent routes. Added. 
                          const bool val = matrix_wa->array()->value(row, col);
                          // Connect if route does not exist. Allow it to reconnect a partial route.
                          //if(val && (!rem_found || !this_found))
                          if(val && MusECore::routeCanConnect(src, dst))
                          {
                            fprintf(stderr, "RoutePopupMenu::popupActivated: Jack: Matrix: adding AddRoute operation\n"); // REMOVE Tim. Persistent routes. Added. 
                            operations.add(MusECore::PendingOperationItem(src, dst, MusECore::PendingOperationItem::AddRoute));
                          }
                          // Disconnect if route exists. Allow it to reconnect a partial route.
                          else if(!val && MusECore::routeCanDisconnect(src, dst))
                          {
                            fprintf(stderr, "RoutePopupMenu::popupActivated: Jack: Matrix: adding DeleteRoute operation\n"); // REMOVE Tim. Persistent routes. Added. 
                            operations.add(MusECore::PendingOperationItem(src, dst, MusECore::PendingOperationItem::DeleteRoute));
                          }
                        }
                      }
                    }
                  }                
                }
              }
              break;  
              
              case MusECore::Route::TRACK_ROUTE:
              {
                // Make sure the track still exists.
                if(MusEGlobal::song->tracks()->find(rem_route.track) != MusEGlobal::song->tracks()->end())
                {
                  for(int row = 0; row < rows; ++row)
                  {
                    for(int col = 0; col < cols; ++col)
                    {
                      MusECore::Route this_route(t, col, 1);
                      //this_route.remoteChannel = row;
                      rem_route.channel = row;
                      rem_route.channels = 1;
                      
                      const MusECore::Route& src = _isOutMenu ? this_route : rem_route;
                      const MusECore::Route& dst = _isOutMenu ? rem_route : this_route;
                      
                      fprintf(stderr, "RoutePopupMenu::popupActivated: Matrix: checking operations\n"); // REMOVE Tim. Persistent routes. Added. 
                      const bool val = matrix_wa->array()->value(row, col);
                      // Connect if route does not exist. Allow it to reconnect a partial route.
                      //if(val && (!rem_found || !this_found))
                      if(val && MusECore::routeCanConnect(src, dst))
                      {
                        fprintf(stderr, "RoutePopupMenu::popupActivated: Matrix: adding AddRoute operation\n"); // REMOVE Tim. Persistent routes. Added. 
                        operations.add(MusECore::PendingOperationItem(src, dst, MusECore::PendingOperationItem::AddRoute));
                      }
                      // Disconnect if route exists. Allow it to reconnect a partial route.
                      else if(!val && MusECore::routeCanDisconnect(src, dst))
                      {
                        fprintf(stderr, "RoutePopupMenu::popupActivated: Matrix: adding DeleteRoute operation\n"); // REMOVE Tim. Persistent routes. Added. 
                        operations.add(MusECore::PendingOperationItem(src, dst, MusECore::PendingOperationItem::DeleteRoute));
                      }
                    }
                  }                  
                }
              } 
              break;  
              case MusECore::Route::MIDI_DEVICE_ROUTE:
              break;  
              case MusECore::Route::MIDI_PORT_ROUTE:
              break;  
            }
            
//             // Make sure the track still exists.
//             if(rem_route.type == MusECore::Route::TRACK_ROUTE && 
//                 MusEGlobal::song->tracks()->find(rem_route.track) != MusEGlobal::song->tracks()->end())
//             {
//               const int rows = matrix_wa->array()->rows();
//               const int cols = matrix_wa->array()->columns();
//               for(int row = 0; row < rows; ++row)
//               {
//                 for(int col = 0; col < cols; ++col)
//                 {
//                   //rem_route.channel = col;
//                   //rem_route.remoteChannel = row;
//                   //rem_route.channels = 1;
//                   
//                   // Check if the remote route exists in the route list.
//     //                 const bool rem_found = rl->exists(rem_route);
//     //                 MusECore::Route this_route(t, row, 1);
//     //                 this_route.remoteChannel = col;
//     //                 const bool this_found = rem_rl ? rem_rl->exists(this_route) : false;
//     //                 
//     //                 // Rearrange the parameters suitable for the addRoute and deleteRoute commands.
//     //                 this_route.channel = col;
//     //                 this_route.remoteChannel = -1;
//     //                 rem_route.channel = row;
//     //                 rem_route.remoteChannel = -1;
//                   MusECore::Route this_route(t, col, 1);
//                   //this_route.remoteChannel = row;
//                   rem_route.channel = row;
//                   rem_route.channels = 1;
//                   
//                   const MusECore::Route& src = _isOutMenu ? this_route : rem_route;
//                   const MusECore::Route& dst = _isOutMenu ? rem_route : this_route;
//                   
//                   fprintf(stderr, "RoutePopupMenu::popupActivated: Matrix: checking operations\n"); // REMOVE Tim. Persistent routes. Added. 
//                   const bool val = matrix_wa->array()->value(row, col);
//                   // Connect if route does not exist. Allow it to reconnect a partial route.
//                   //if(val && (!rem_found || !this_found))
//                   if(val && MusECore::routeCanConnect(src, dst))
//                   {
//                     fprintf(stderr, "RoutePopupMenu::popupActivated: Matrix: adding AddRoute operation\n"); // REMOVE Tim. Persistent routes. Added. 
//                     operations.add(MusECore::PendingOperationItem(src, dst, MusECore::PendingOperationItem::AddRoute));
//                   }
//                   // Disconnect if route exists. Allow it to reconnect a partial route.
//                   else if(!val && MusECore::routeCanDisconnect(src, dst))
//                   {
//                     fprintf(stderr, "RoutePopupMenu::popupActivated: Matrix: adding DeleteRoute operation\n"); // REMOVE Tim. Persistent routes. Added. 
//                     operations.add(MusECore::PendingOperationItem(src, dst, MusECore::PendingOperationItem::DeleteRoute));
//                   }
//                 }
//               }
//             }

          }
          // Support Midi Port to Audio Input routes. 
          else if(!_isOutMenu && track->type() == MusECore::Track::AUDIO_INPUT && rem_route.type == MusECore::Route::MIDI_PORT_ROUTE)
          {
            // Check for custom midi channel select action.
            PixmapButtonsWidgetAction* cs_wa = dynamic_cast<PixmapButtonsWidgetAction*>(action);
            if(cs_wa)
            {
              const QBitArray ba = cs_wa->currentState();
              const int ba_sz = ba.size();
              int chbits = 0;
              for(int mch = 0; mch < MIDI_CHANNELS && mch < ba_sz; ++mch)
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
                  //MusEGlobal::audio->msgRemoveRoute(*iir, dstRoute);
                  operations.add(MusECore::PendingOperationItem(*iir, dstRoute, MusECore::PendingOperationItem::DeleteRoute));
                }
                if(chbits != 0)
                {
                  // Connect desired channels.
                  MusECore::Route dstRoute(t, chbits);
                  //MusEGlobal::audio->msgAddRoute(rem_route, dstRoute);
                  operations.add(MusECore::PendingOperationItem(rem_route, dstRoute, MusECore::PendingOperationItem::AddRoute));
                }
                //MusEGlobal::audio->msgUpdateSoloStates();
                //MusEGlobal::song->update(SC_ROUTE);
              }  
              //return;
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
              {
                //printf("routingPopupMenuActivated: removing src route ch:%d dst route ch:%d\n", srcRoute.channel, dstRoute.channel); 
                //MusEGlobal::audio->msgRemoveRoute(rem_route, dstRoute);
                operations.add(MusECore::PendingOperationItem(rem_route, dstRoute, MusECore::PendingOperationItem::DeleteRoute));
              }
              else 
              {
                //printf("routingPopupMenuActivated: adding src route ch:%d dst route ch:%d\n", srcRoute.channel, dstRoute.channel); 
                //MusEGlobal::audio->msgAddRoute(rem_route, dstRoute);
                operations.add(MusECore::PendingOperationItem(rem_route, dstRoute, MusECore::PendingOperationItem::AddRoute));
              }
              
              //MusEGlobal::audio->msgUpdateSoloStates();
              //MusEGlobal::song->update(SC_ROUTE);
              //return;
            }  
          }
          else
          {
            
            MusECore::Route this_route(t, rem_route.channel, rem_route.channels);
            this_route.remoteChannel = rem_route.remoteChannel;
    //         switch(rem_route.type)
    //         {
    //           case MusECore::Route::JACK_ROUTE:
    //             // Clear the information.
    //             rem_route.channel = -1;
    //             rem_route.remoteChannel = -1;
    //             rem_route.channels = -1;
    //           break;
    //           
    //           case MusECore::Route::TRACK_ROUTE:
    //           case MusECore::Route::MIDI_DEVICE_ROUTE:
    //           case MusECore::Route::MIDI_PORT_ROUTE:
    //           break;
    //         }

            const MusECore::Route& src = _isOutMenu ? this_route : rem_route;
            const MusECore::Route& dst = _isOutMenu ? rem_route : this_route;
            
            // Connect if route does not exist. Allow it to reconnect a partial route.
            if(action->isChecked() && MusECore::routeCanConnect(src, dst))
            {
              fprintf(stderr, "RoutePopupMenu::popupActivated: Route: adding AddRoute operation\n"); // REMOVE Tim. Persistent routes. Added. 
              operations.add(MusECore::PendingOperationItem(src, dst, MusECore::PendingOperationItem::AddRoute));
            }
            // Disconnect if route exists. Allow it to reconnect a partial route.
            else if(!action->isChecked() && MusECore::routeCanDisconnect(src, dst))
            {
              fprintf(stderr, "RoutePopupMenu::popupActivated: Route: adding DeleteRoute operation\n"); // REMOVE Tim. Persistent routes. Added. 
              operations.add(MusECore::PendingOperationItem(src, dst, MusECore::PendingOperationItem::DeleteRoute));
            }
          }
        }
        else
        {
          
        }
        
    //               if(_isOutMenu)
    //               {  
    //                 // REMOVE Tim. Persistent routes. Changed.
    //                 //MusECore::Route srcRoute(t, rem_route.channel, rem_route.channels);
    //                 //srcRoute.remoteChannel = rem_route.remoteChannel;
    //                 //MusECore::Route srcRoute(t, rem_route.remoteChannel, rem_route.channels);
    //                 //srcRoute.remoteChannel = rem_route.channel;
    //         
    //                 // check if route src->dst exists:
    //     //             MusECore::ciRoute irl = rl->begin();
    //     //             for (; irl != rl->end(); ++irl) {
    //     //                   if (*irl == rem_route)
    //     //                         break;
    //     //                   }
    //     //             if (irl != rl->end()) {
    //     //             if (rl->exists(rem_route)) {
    //     //                   // disconnect if route exists
    //     //                   MusEGlobal::audio->msgRemoveRoute(srcRoute, rem_route);
    //     //                   }
    //     //             else {
    //     //                   // connect if route does not exist
    //     //                   MusEGlobal::audio->msgAddRoute(srcRoute, rem_route);
    //     //                   }
    //     //             MusEGlobal::audio->msgUpdateSoloStates();
    //     //             MusEGlobal::song->update(SC_ROUTE);
    //                 
    //               }
    //             }
    //             else
    //             {
    //               // Support Midi Port to Audio Input routes. 
    //               if(_track->type() == MusECore::Track::AUDIO_INPUT && rem_route.type == MusECore::Route::MIDI_PORT_ROUTE)
    //               {
    //                 // Check for custom midi channel select action.
    //                 PixmapButtonsWidgetAction* cs_wa = dynamic_cast<PixmapButtonsWidgetAction*>(action);
    //                 if(cs_wa)
    //                 {
    //                   const QBitArray ba = cs_wa->currentState();
    //                   const int ba_sz = ba.size();
    //                   int chbits = 0;
    //                   for(int mch = 0; mch < MIDI_CHANNELS && mch < ba_sz; ++mch)
    //                   {
    //                     if(ba.at(mch))
    //                       chbits |= (1 << mch);
    //                   }
    // 
    //                   rem_route.channel = chbits;  // Restore the desired route channels from the custom widget action state.
    //                   int mdidx = rem_route.midiPort;
    // 
    //                   int chmask = 0;                   
    //                   MusECore::ciRoute iir = rl->begin();
    //                   for (; iir != rl->end(); ++iir) 
    //                     if(iir->type == MusECore::Route::MIDI_PORT_ROUTE && iir->midiPort == mdidx) {   // Is there already a route to this port?
    //                       chmask = iir->channel;  // Grab the channel mask.
    //                       break;
    //                     }      
    //                   
    //                   // Only if something changed...
    //                   if(chmask != chbits)
    //                   {
    //                     if(chmask != 0)
    //                     {
    //                       // Disconnect all existing channels.
    //                       MusECore::Route dstRoute(t, chmask);
    //                       MusEGlobal::audio->msgRemoveRoute(*iir, dstRoute);
    //                     }
    //                     if(chbits != 0)
    //                     {
    //                       // Connect desired channels.
    //                       MusECore::Route dstRoute(t, chbits);
    //                       MusEGlobal::audio->msgAddRoute(rem_route, dstRoute);
    //                     }
    //                     MusEGlobal::audio->msgUpdateSoloStates();
    //                     MusEGlobal::song->update(SC_ROUTE);
    //                   }  
    //                   return;
    //                 }
    //                 else  
    //                 {
    //                   int chbit = rem_route.channel;
    //                   MusECore::Route dstRoute(t, chbit);
    //                   int mdidx = rem_route.midiPort;
    //                   int chmask = 0;                   
    //                   MusECore::ciRoute iir = rl->begin();
    //                   for (; iir != rl->end(); ++iir) 
    //                   {
    //                     if(iir->type == MusECore::Route::MIDI_PORT_ROUTE && iir->midiPort == mdidx)    // Is there already a route to this port?
    //                     {
    //                       chmask = iir->channel;  // Grab the channel mask.
    //                       break;
    //                     }      
    //                   }
    //                   
    //                   if ((chmask & chbit) == chbit)             // Is the channel's bit(s) set?
    //                   {
    //                     //printf("routingPopupMenuActivated: removing src route ch:%d dst route ch:%d\n", srcRoute.channel, dstRoute.channel); 
    //                     MusEGlobal::audio->msgRemoveRoute(rem_route, dstRoute);
    //                   }
    //                   else 
    //                   {
    //                     //printf("routingPopupMenuActivated: adding src route ch:%d dst route ch:%d\n", srcRoute.channel, dstRoute.channel); 
    //                     MusEGlobal::audio->msgAddRoute(rem_route, dstRoute);
    //                   }
    //                   
    //                   MusEGlobal::audio->msgUpdateSoloStates();
    //                   MusEGlobal::song->update(SC_ROUTE);
    //                   return;
    //                 }  
    //               }
    //               
    //               // REMOVE Tim. Persistent routes. Changed.
    //               //MusECore::Route dstRoute(t, srcRoute.channel, srcRoute.channels);     
    //               //dstRoute.remoteChannel = srcRoute.remoteChannel;
    //               MusECore::Route dstRoute(t, rem_route.remoteChannel, rem_route.channels);     
    //               dstRoute.remoteChannel = rem_route.channel;
    //       
    //               MusECore::ciRoute irl = rl->begin();
    //               for (; irl != rl->end(); ++irl) {
    //                     if (*irl == rem_route)
    //                           break;
    //                     }
    //               if (irl != rl->end()) {
    //                     // disconnect
    //                     MusEGlobal::audio->msgRemoveRoute(rem_route, dstRoute);
    //                     }
    //               else {
    //                     // connect
    //                     MusEGlobal::audio->msgAddRoute(rem_route, dstRoute);
    //                     }
    //               MusEGlobal::audio->msgUpdateSoloStates();
    //               MusEGlobal::song->update(SC_ROUTE);
    //             }
    //           }
    //         }
      }
    }
    break;
    
    case MusECore::Route::JACK_ROUTE:
    break;
    case MusECore::Route::MIDI_DEVICE_ROUTE:
    break;
    case MusECore::Route::MIDI_PORT_ROUTE:
    break;
    
  }
  
  if(!operations.empty())
  {
    fprintf(stderr, "RoutePopupMenu::popupActivated: executing operations\n"); // REMOVE Tim. Persistent routes. Added. 
    MusEGlobal::audio->msgExecutePendingOperations(operations);
    MusEGlobal::audio->msgUpdateSoloStates(); // TODO Include this in operations.
    MusEGlobal::song->update(SC_ROUTE);
  }
}


// REMOVE Tim. Persistent routes. Changed.
// void RoutePopupMenu::prepare()
// {
//   ///disconnect();
//   ///clear();
//    
//   if(!_track)
//     return;
//    
//   connect(this, SIGNAL(triggered(QAction*)), SLOT(popupActivated(QAction*)));
//   
//   if(_isOutMenu)
//     addAction(new MenuTitleItem(tr("Output routes:"), this));
//   else
//     addAction(new MenuTitleItem(tr("Input routes:"), this));
//   addSeparator();
//   
//   if(_track->isMidiTrack())
//   {
//     MusECore::RouteList* rl = _isOutMenu ? _track->outRoutes() : _track->inRoutes();
//     
//     int gid = 0;
//     QAction* act = 0;
//     
//     if(_isOutMenu)   
//     {
//       // Support Midi Port to Audio Input track routes. 
//       int port = ((MusECore::MidiTrack*)_track)->outPort();
//       if(port >= 0 && port < MIDI_PORTS)
//       {
//         MusECore::MidiPort* mp = &MusEGlobal::midiPorts[port];
//         
//         // Do not list synth devices! Requiring valid device is desirable, 
//         //  but could lead to 'hidden' routes unless we add more support
//         //  such as removing the existing routes when user changes flags.
//         // So for now, just list all valid ports whether read or write.
//         if(mp->device() && !mp->device()->isSynti())  
//         {
//           MusECore::RouteList* mprl = mp->outRoutes();
//           int chbits = 1 << ((MusECore::MidiTrack*)_track)->outChannel();
//           //MusECore::MidiDevice* md = mp->device();
//           //if(!md)
//           //  continue;
//           
//           addSeparator();
//           addAction(new MenuTitleItem(tr("Soloing chain"), this)); 
//           PopupMenu* subp = new PopupMenu(this, true);
//           subp->setTitle(tr("Audio returns")); 
//           addMenu(subp);
//           
//           MusECore::InputList* al = MusEGlobal::song->inputs();
//           for (MusECore::ciAudioInput i = al->begin(); i != al->end(); ++i) 
//           {
//             MusECore::Track* t = *i;
//             QString s(t->name());
//             act = subp->addAction(s);
//             act->setCheckable(true);
//             MusECore::Route r(t, chbits);
//             act->setData(QVariant::fromValue(r));   
//             for(MusECore::ciRoute ir = mprl->begin(); ir != mprl->end(); ++ir) 
//             {
//               if(ir->type == MusECore::Route::TRACK_ROUTE && ir->track == t && (ir->channel & chbits))
//               {
//                 act->setChecked(true);
//                 break;
//               }  
//             }
//             ++gid;      
//           }
//         }     
//       }  
//     }
//     else
//     {
//       // Warn if no devices available. Add an item to open midi config. 
//       int pi = 0;
//       for( ; pi < MIDI_PORTS; ++pi)
//       {
//         MusECore::MidiDevice* md = MusEGlobal::midiPorts[pi].device();
//         if(md && !md->isSynti() && (md->rwFlags() & 2))
//         //if(md && (md->rwFlags() & 2 || md->isSynti()) )  // p4.0.27 Reverted p4.0.35 
//           break;
//       }
//       if(pi == MIDI_PORTS)
//       {
//         act = addAction(tr("Warning: No input devices!"));
//         act->setCheckable(false);
//         act->setData(-1);
//         addSeparator();
//       }
//       act = addAction(QIcon(*settings_midiport_softsynthsIcon), tr("Open midi config..."));
//       act->setCheckable(false);
//       act->setData(gid);
//       addSeparator();
//       ++gid;
//       
// #ifdef _USE_CUSTOM_WIDGET_ACTIONS_
// 
//         PixmapButtonsHeaderWidgetAction* wa_hdr = new PixmapButtonsHeaderWidgetAction("Input port/device", darkRedLedIcon, MIDI_CHANNELS, this);
//         addAction(wa_hdr);  
//         ++gid;
// #else   
//       addAction(new MenuTitleItem("Input port/device", this)); 
// #endif
//         
//       for(int i = 0; i < MIDI_PORTS; ++i)
//       {
//         // NOTE: Could possibly list all devices, bypassing ports, but no, let's stick with ports.
//         MusECore::MidiPort* mp = &MusEGlobal::midiPorts[i];
//         MusECore::MidiDevice* md = mp->device();
//         //if(!md)
//         //  continue;
//         
//         // Do not list synth devices!
//         if( md && (!(md->rwFlags() & 2) || md->isSynti()) )
//         // p4.0.27 Go ahead. Synths esp MESS send out stuff. Reverted p4.0.35 
//         //if( md && !(md->rwFlags() & 2) && !md->isSynti() )
//           continue;
//           
//         //printf("MusE::prepareRoutingPopupMenu adding submenu portnum:%d\n", i);
//         
//         int chanmask = 0;
//         // To reduce number of routes required, from one per channel to just one containing a channel mask. 
//         // Look for the first route to this midi port. There should always be only a single route for each midi port, now.
//         MusECore::ciRoute ir = rl->begin();
//         for( ; ir != rl->end(); ++ir)   
//         {
//           if(ir->type == MusECore::Route::MIDI_PORT_ROUTE && ir->midiPort == i) 
//           {
//             // We have a route to the midi port. Grab the channel mask.
//             chanmask = ir->channel;
//             break;
//           }
//         }
//         // List ports with no device, but with routes to this track, in the main popup.
//         if(!md && ir == rl->end())
//           continue;
//         
// #ifdef _USE_CUSTOM_WIDGET_ACTIONS_
//         
//         QBitArray ba(MIDI_CHANNELS); 
//         for(int mch = 0; mch < MIDI_CHANNELS; ++mch)
//         {  
//           if(chanmask & (1 << mch))
//             ba.setBit(mch);
//         }
// 
//         PixmapButtonsWidgetAction* wa = new PixmapButtonsWidgetAction(QString::number(i + 1) + ":" + (md ? md->name() : tr("<none>")), 
//                                                                       redLedIcon, darkRedLedIcon, ba, this);
//         MusECore::Route srcRoute(i, 0); // Ignore the routing channels - our action holds the channels.   
//         //wa->setData(id++);   
//         wa->setData(QVariant::fromValue(srcRoute));   
//         addAction(wa);  
//         
//         
//         // REMOVE Tim. Persistent routes. Added. TESTING
//         //PopupMenu* subp = new PopupMenu(this, true);
//         //subp->setTitle("TEST"); 
//         //subp->addAction("Testing");
//         //wa->(subp);
//         
//         
//         ++gid;
// 
// #else    
// 
//         PopupMenu* subp = new PopupMenu(this, true);
//         subp->setTitle(QString("%1:").arg(i+1) + (md ? md->name() : tr("<none>"))); 
//         
//         for(int ch = 0; ch < MIDI_CHANNELS; ++ch) 
//         {
//           act = subp->addAction(QString("Channel %1").arg(ch+1));
//           act->setCheckable(true);
//           int chbit = 1 << ch;
//           MusECore::Route srcRoute(i, chbit);    // In accordance with channel mask, use the bit position.
//           act->setData(QVariant::fromValue(srcRoute));   
//           if(chanmask & chbit)                  // Is the channel already set? Show item check mark.
//             act->setChecked(true);
//           ++gid;  
//         }
//         //gid = MIDI_PORTS * MIDI_CHANNELS + i;           // Make sure each 'toggle' item gets a unique id.
//         act = subp->addAction(tr("Toggle all"));
//         //act->setCheckable(true);
//         MusECore::Route togRoute(i, (1 << MIDI_CHANNELS) - 1);    // Set all channel bits.
//         act->setData(QVariant::fromValue(togRoute));   
//         ++gid;
//         addMenu(subp);
//         
// #endif // _USE_CUSTOM_WIDGET_ACTIONS_
//         
//       }
//       
//       #if 0
//       // p4.0.17 List ports with no device and no in routes, in a separate popup.
//       PopupMenu* morep = new PopupMenu(pup, true);
//       morep->setTitle(tr("More...")); 
//       for(int i = 0; i < MIDI_PORTS; ++i)
//       {
//         MusECore::MidiPort* mp = &MusEGlobal::midiPorts[i];
//         if(mp->device())
//           continue;
//         
//         PopupMenu* subp = new PopupMenu(morep, true);
//         subp->setTitle(QString("%1:%2").arg(i).arg(tr("<none>"))); 
//         
//         // MusE-2: Check this - needed with QMenu? Help says no. No - verified, it actually causes double triggers!
//         //connect(subp, SIGNAL(triggered(QAction*)), pup, SIGNAL(triggered(QAction*)));
//         //connect(subp, SIGNAL(aboutToHide()), pup, SIGNAL(aboutToHide()));
//         
//         iRoute ir = rl->begin();
//         for( ; ir != rl->end(); ++ir)   
//         {
//           if(ir->type == MusECore::Route::MIDI_PORT_ROUTE && ir->midiPort == i) 
//             break;
//         }
//         if(ir != rl->end())
//           continue;
//         
//         for(int ch = 0; ch < MIDI_CHANNELS; ++ch) 
//         {
//           act = subp->addAction(QString("Channel %1").arg(ch+1));
//           act->setCheckable(true);
//           act->setData(gid);
//           
//           int chbit = 1 << ch;
//           MusECore::Route srcRoute(i, chbit);    // In accordance with new channel mask, use the bit position.
//           
//           gRoutingMenuMap.insert( pRouteMenuMap(gid, srcRoute) );
//           
//           //if(chanmask & chbit)                  // Is the channel already set? Show item check mark.
//           //  act->setChecked(true);
//           
//           ++gid;  
//         }
//         //gid = MIDI_PORTS * MIDI_CHANNELS + i;           // Make sure each 'toggle' item gets a unique id.
//         act = subp->addAction(QString("Toggle all"));
//         //act->setCheckable(true);
//         act->setData(gid);
//         MusECore::Route togRoute(i, (1 << MIDI_CHANNELS) - 1);    // Set all channel bits.
//         gRoutingMenuMap.insert( pRouteMenuMap(gid, togRoute) );
//         ++gid;
//         morep->addMenu(subp);
//       }      
//       pup->addMenu(morep);
//       #endif
//       
//     }
//     return;
//   }
//   else
//   {
//     MusECore::AudioTrack* t = (MusECore::AudioTrack*)_track;
//     //int chans = t->channels();
//     if(_isOutMenu)   
//     {
//       // REMOVE Tim. Persistent routes. Added.
//       const int t_ochs = t->totalRoutableOutputs(MusECore::Route::TRACK_ROUTE);
//       const int t_jochs = t->totalRoutableOutputs(MusECore::Route::JACK_ROUTE);
//     
//       MusECore::RouteList* orl = t->outRoutes();
// 
//       QAction* act = 0;
//       int gid = 0;
//       gid = 0;
//       
//       switch(_track->type()) 
//       {
//         case MusECore::Track::AUDIO_OUTPUT:
//         {
//           // REMOVE Tim. Persistent routes. Changed.
//           //for(int i = 0; i < chans; ++i) 
//           for(int i = 0; i < t_jochs; ++i) 
//           {
//             //char buffer[128];
//             //snprintf(buffer, 128, "%s %d", tr("Channel").toLatin1().constData(), i+1);
//             QString chBuffer = tr("Channel") + QString(" ") + QString::number(i + 1);
//             MenuTitleItem* titel = new MenuTitleItem(chBuffer, this);
//             addAction(titel); 
//   
//             if(!MusEGlobal::checkAudioDevice())
//             { 
//               clear();
//               return;
//             }
//             std::list<QString> ol = MusEGlobal::audioDevice->inputPorts();
//             for(std::list<QString>::iterator ip = ol.begin(); ip != ol.end(); ++ip) 
//             {
//               act = addAction(*ip);
//               act->setCheckable(true);
//               
//               const char* port_name = (*ip).toLatin1().constData();
//               char good_name[ROUTE_PERSISTENT_NAME_SIZE];
//               void* port = MusEGlobal::audioDevice->findPort(port_name);
//               if(port)
//               {
//                 MusEGlobal::audioDevice->portName(port, good_name, ROUTE_PERSISTENT_NAME_SIZE);
//                 port_name = good_name;
//               }
//               MusECore::Route dst(MusECore::Route::JACK_ROUTE, -1, NULL, i, -1, -1, port_name);
//               
//               act->setData(QVariant::fromValue(dst));   
//               ++gid;
//               for(MusECore::ciRoute ir = orl->begin(); ir != orl->end(); ++ir) 
//               {
//                 if(*ir == dst) 
//                 {
//                   act->setChecked(true);
//                   break;
//                 }
//               }
//             }
//             // REMOVE Tim. Persistent routes. Changed.
//             //if(i+1 != chans)
//             if(i+1 != t_ochs)
//               addSeparator();
//           }      
//           
//           //
//           // Display using separate menu for audio inputs:
//           //
//           addSeparator();
//           addAction(new MenuTitleItem(tr("Soloing chain"), this)); 
//           PopupMenu* subp = new PopupMenu(this, true);
//           subp->setTitle(tr("Audio returns")); 
//           addMenu(subp);
//           gid = addInPorts(t, subp, gid, -1, -1, true);  
//           //
//           // Display all in the same menu:
//           //
//           //addSeparator();
//           //MenuTitleItem* title = new MenuTitleItem(tr("Audio returns"), this);
//           //addAction(title); 
//           //gid = addInPorts(t, this, gid, -1, -1, true);  
//         }
//         break;
//         
//         
//         // REMOVE Tim. Persistent routes. Changed.
// //         case MusECore::Track::AUDIO_SOFTSYNTH:
// //               gid = addMultiChannelPorts(t, this, gid, true);
// //         break;
// //         
// //         case MusECore::Track::AUDIO_INPUT:
// //         case MusECore::Track::WAVE:
// //         case MusECore::Track::AUDIO_GROUP:
// //         case MusECore::Track::AUDIO_AUX:
// //               gid = addWavePorts(        t, this, gid, -1, -1, true);  
// //               gid = addOutPorts(         t, this, gid, -1, -1, true);
// //               gid = addGroupPorts(       t, this, gid, -1, -1, true);
// //               gid = nonSyntiTrackAddSyntis(t, this, gid, true);
// //         break;
//         case MusECore::Track::AUDIO_INPUT:
//         case MusECore::Track::WAVE:
//         case MusECore::Track::AUDIO_GROUP:
//         case MusECore::Track::AUDIO_AUX:
//         case MusECore::Track::AUDIO_SOFTSYNTH:
//           if(t_ochs > 0)
//           {
// #ifndef _USE_CUSTOM_WIDGET_ACTIONS_
//             addAction(new MenuTitleItem(tr("Omni"), this)); 
// #endif            
//             gid = addWavePorts(        t, this, gid, -1, -1, true);  
//             gid = addOutPorts(         t, this, gid, -1, -1, true);
//             gid = addGroupPorts(       t, this, gid, -1, -1, true);
//             gid = addSynthPorts(       t, this, gid, -1, -1, true);
//           }
//         break;
//         
//         default:
//           clear();
//           return;
//         break;
//       }
//       
// #ifndef _USE_CUSTOM_WIDGET_ACTIONS_
//       // REMOVE Tim. Persistent routes. Added.
//       switch(_track->type()) 
//       {
//         case MusECore::Track::AUDIO_INPUT:
//         case MusECore::Track::WAVE:
//         case MusECore::Track::AUDIO_GROUP:
//         case MusECore::Track::AUDIO_AUX:
//         case MusECore::Track::AUDIO_SOFTSYNTH:
//           if(t_ochs > 0)
//           {
//             addSeparator();
//             addAction(new MenuTitleItem(tr("Channels"), this));
//             for(int i = 0; i < t_ochs; ++i)
//             {
//               PopupMenu* subp = new PopupMenu(this, true);
//               subp->setTitle(QString::number(i + 1)); 
//               subp->addAction(new MenuTitleItem(tr("Destinations:"), this));
//               addMenu(subp);
//               gid = addWavePorts( t, subp, gid, i, 1, true);  
//               gid = addOutPorts(  t, subp, gid, i, 1, true);
//               gid = addGroupPorts(t, subp, gid, i, 1, true);
//               gid = addSynthPorts(t, subp, gid, i, 1, true);
//             }
//           }
//         break;
// 
//         default:
//         break;
//       }
// #endif
// 
//     }
//     else
//     {
//       if(_track->type() == MusECore::Track::AUDIO_AUX)
//         return;
//         
//       // REMOVE Tim. Persistent routes. Added.
//       const int t_ichs = t->totalRoutableInputs(MusECore::Route::TRACK_ROUTE);
//       const int t_jichs = t->totalRoutableInputs(MusECore::Route::JACK_ROUTE);
//       
//       MusECore::RouteList* irl = t->inRoutes();
//   
//       QAction* act = 0;
//       int gid = 0;
//       gid = 0;
//       
//       switch(_track->type()) 
//       {
//         case MusECore::Track::AUDIO_INPUT:
//         {
//           // REMOVE Tim. Persistent routes. Changed.
//           //for(int i = 0; i < chans; ++i) 
//           for(int i = 0; i < t_jichs; ++i) 
//           {
//             //char buffer[128];
//             //snprintf(buffer, 128, "%s %d", tr("Channel").toLatin1().constData(), i+1);
//             QString chBuffer = tr("Channel") + QString(" ") + QString::number(i + 1);
//             MenuTitleItem* titel = new MenuTitleItem(chBuffer, this);
//             addAction(titel); 
//   
//             if(!MusEGlobal::checkAudioDevice())
//             { 
//               clear();
//               return;
//             }
//             std::list<QString> ol = MusEGlobal::audioDevice->outputPorts();
//             for(std::list<QString>::iterator ip = ol.begin(); ip != ol.end(); ++ip) 
//             {
//               act = addAction(*ip);
//               act->setCheckable(true);
//               
//               const char* port_name = (*ip).toLatin1().constData();
//               char good_name[ROUTE_PERSISTENT_NAME_SIZE];
//               void* port = MusEGlobal::audioDevice->findPort(port_name);
//               if(port)
//               {
//                 MusEGlobal::audioDevice->portName(port, good_name, ROUTE_PERSISTENT_NAME_SIZE);
//                 port_name = good_name;
//               }
//               MusECore::Route dst(MusECore::Route::JACK_ROUTE, -1, NULL, i, -1, -1, port_name);
//               
//               act->setData(QVariant::fromValue(dst));   
//               ++gid;
//               for(MusECore::ciRoute ir = irl->begin(); ir != irl->end(); ++ir) 
//               {
//                 if(*ir == dst) 
//                 {
//                   act->setChecked(true);
//                   break;
//                 }
//               }
//             }
//             // REMOVE Tim. Persistent routes. Changed.
//             //if(i+1 != chans)
//             if(i+1 != t_ichs)
//               addSeparator();
//           }
//           
//           //
//           // Display using separate menus for midi ports and audio outputs:
//           //
//           addSeparator();
//           addAction(new MenuTitleItem(tr("Soloing chain"), this)); 
//           PopupMenu* subp = new PopupMenu(this, true);
//           subp->setTitle(tr("Audio sends")); 
//           addMenu(subp);
//           gid = addOutPorts(t, subp, gid, -1, -1, false);  
//           subp = new PopupMenu(this, true);
//           subp->setTitle(tr("Midi port sends")); 
//           addMenu(subp);
//           addMidiPorts(t, subp, gid, false);
//           //
//           // Display all in the same menu:
//           //
//           //addAction(new MenuTitleItem(tr("Audio sends"), this)); 
//           //gid = addOutPorts(t, this, gid, -1, -1, false);  
//           //addSeparator();
//           //addAction(new MenuTitleItem(tr("Midi sends"), this)); 
//           //addMidiPorts(t, this, gid, false);
//         }
//         break;
//         
//         // REMOVE Tim. Persistent routes. Changed.
// //         case MusECore::Track::AUDIO_OUTPUT:
// //               gid = addWavePorts( t, this, gid, -1, -1, false);
// //               gid = addInPorts(   t, this, gid, -1, -1, false);
// //               gid = addGroupPorts(t, this, gid, -1, -1, false);
// //               gid = addAuxPorts(  t, this, gid, -1, -1, false);
// //               gid = nonSyntiTrackAddSyntis(t, this, gid, false);
// //               break;
// //         case MusECore::Track::WAVE:
// //               gid = addWavePorts( t, this, gid, -1, -1, false);  
// //               gid = addInPorts(   t, this, gid, -1, -1, false);
// //               gid = addGroupPorts(t, this, gid, -1, -1, false);  
// //               gid = addAuxPorts(  t, this, gid, -1, -1, false);  
// //               gid = nonSyntiTrackAddSyntis(t, this, gid, false); 
// //               break;
// //         case MusECore::Track::AUDIO_GROUP:
// //               gid = addWavePorts( t, this, gid, -1, -1, false);
// //               gid = addInPorts(   t, this, gid, -1, -1, false);
// //               gid = addGroupPorts(t, this, gid, -1, -1, false);
// //               gid = addAuxPorts(  t, this, gid, -1, -1, false);  
// //               gid = nonSyntiTrackAddSyntis(t, this, gid, false);
// //               break;
// //         
// //         case MusECore::Track::AUDIO_SOFTSYNTH:
// //               gid = addMultiChannelPorts(t, this, gid, false);
// //               break;
//         case MusECore::Track::AUDIO_OUTPUT:
//         case MusECore::Track::WAVE:
//         case MusECore::Track::AUDIO_GROUP:
//         case MusECore::Track::AUDIO_SOFTSYNTH:
//           if(t_ichs > 0)
//           {
// #ifndef _USE_CUSTOM_WIDGET_ACTIONS_
//             addAction(new MenuTitleItem(tr("Omni"), this)); 
// #endif            
//             gid = addWavePorts( t, this, gid, -1, -1, false);
//             gid = addInPorts(   t, this, gid, -1, -1, false);
//             gid = addGroupPorts(t, this, gid, -1, -1, false);
//             gid = addAuxPorts(  t, this, gid, -1, -1, false);  
//             gid = addSynthPorts(t, this, gid, -1, -1, false);  
//           }
//         break;
//               
//         default:
//           clear();
//           return;
//         break;  
//       }  
//       
// #ifndef _USE_CUSTOM_WIDGET_ACTIONS_
//       // REMOVE Tim. Persistent routes. Added.
//       switch(_track->type()) 
//       {
//         case MusECore::Track::AUDIO_OUTPUT:
//         case MusECore::Track::WAVE:
//         case MusECore::Track::AUDIO_GROUP:
//         case MusECore::Track::AUDIO_SOFTSYNTH:
//           if(t_ichs > 0)
//           {
//             addSeparator();
//             addAction(new MenuTitleItem(tr("Channels"), this));
//             for(int i = 0; i < t_ichs; ++i)
//             {
//               PopupMenu* subp = new PopupMenu(this, true);
//               subp->setTitle(QString::number(i + 1)); 
//               subp->addAction(new MenuTitleItem(tr("Sources:"), this));
//               addMenu(subp);
//               gid = addWavePorts( t, subp, gid, i, 1, false);  
//               gid = addInPorts(   t, subp, gid, i, 1, false);
//               gid = addGroupPorts(t, subp, gid, i, 1, false);
//               gid = addAuxPorts(  t, subp, gid, i, 1, false);  
//               gid = addSynthPorts(t, subp, gid, i, 1, false);
//             }
//           }
//         break;
// 
//         default:
//         break;
//       }
// #endif
// 
//     }  
//   }
// }
// 
void RoutePopupMenu::prepare()
{
  ///disconnect();
  ///clear();
   
  //if(!_track)
  if(!_route.isValid())
    return;
   
  connect(this, SIGNAL(triggered(QAction*)), SLOT(popupActivated(QAction*)));
  
  if(_isOutMenu)
    addAction(new MenuTitleItem(tr("Output routes:"), this));
  else
    addAction(new MenuTitleItem(tr("Input routes:"), this));
  addSeparator();
  
  switch(_route.type)
  {
    case MusECore::Route::TRACK_ROUTE:
    {
      MusECore::Track* const track = _route.track;
      if(track->isMidiTrack())
      {
        const MusECore::RouteList* const rl = _isOutMenu ? track->outRoutes() : track->inRoutes();
        
        int gid = 0;
        QAction* act = 0;
        
        if(_isOutMenu)   
        {
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
              MusECore::RouteList* mprl = mp->outRoutes();
              int chbits = 1 << ((MusECore::MidiTrack*)track)->outChannel();
              //MusECore::MidiDevice* md = mp->device();
              //if(!md)
              //  continue;
              
              addSeparator();
              addAction(new MenuTitleItem(tr("Soloing chain"), this)); 
              PopupMenu* subp = new PopupMenu(this, true);
              subp->setTitle(tr("Audio returns")); 
              addMenu(subp);
              
              MusECore::InputList* al = MusEGlobal::song->inputs();
              for (MusECore::ciAudioInput i = al->begin(); i != al->end(); ++i) 
              {
                MusECore::Track* t = *i;
                QString s(t->name());
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
            MusECore::MidiDevice* md = MusEGlobal::midiPorts[pi].device();
            if(md && !md->isSynti() && (md->rwFlags() & 2))
            //if(md && (md->rwFlags() & 2 || md->isSynti()) )  // p4.0.27 Reverted p4.0.35 
              break;
          }
          if(pi == MIDI_PORTS)
          {
            act = addAction(tr("Warning: No input devices!"));
            act->setCheckable(false);
            act->setData(-1);
            addSeparator();
          }
          act = addAction(QIcon(*settings_midiport_softsynthsIcon), tr("Open midi config..."));
          act->setCheckable(false);
          act->setData(gid);
          addSeparator();
          ++gid;
          
    #ifdef _USE_CUSTOM_WIDGET_ACTIONS_

            PixmapButtonsHeaderWidgetAction* wa_hdr = new PixmapButtonsHeaderWidgetAction("Input port/device", darkRedLedIcon, MIDI_CHANNELS, this);
            addAction(wa_hdr);  
            ++gid;
    #else   
          addAction(new MenuTitleItem("Input port/device", this)); 
    #endif
            
          for(int i = 0; i < MIDI_PORTS; ++i)
          {
            // NOTE: Could possibly list all devices, bypassing ports, but no, let's stick with ports.
            MusECore::MidiPort* mp = &MusEGlobal::midiPorts[i];
            MusECore::MidiDevice* md = mp->device();
            //if(!md)
            //  continue;
            
            // Do not list synth devices!
            if( md && (!(md->rwFlags() & 2) || md->isSynti()) )
            // p4.0.27 Go ahead. Synths esp MESS send out stuff. Reverted p4.0.35 
            //if( md && !(md->rwFlags() & 2) && !md->isSynti() )
              continue;
              
            //printf("MusE::prepareRoutingPopupMenu adding submenu portnum:%d\n", i);
            
            int chanmask = 0;
            // To reduce number of routes required, from one per channel to just one containing a channel mask. 
            // Look for the first route to this midi port. There should always be only a single route for each midi port, now.
            MusECore::ciRoute ir = rl->begin();
            for( ; ir != rl->end(); ++ir)   
            {
              if(ir->type == MusECore::Route::MIDI_PORT_ROUTE && ir->midiPort == i) 
              {
                // We have a route to the midi port. Grab the channel mask.
                chanmask = ir->channel;
                break;
              }
            }
            // List ports with no device, but with routes to this track, in the main popup.
            if(!md && ir == rl->end())
              continue;
            
    #ifdef _USE_CUSTOM_WIDGET_ACTIONS_
            
            QBitArray ba(MIDI_CHANNELS); 
            for(int mch = 0; mch < MIDI_CHANNELS; ++mch)
            {  
              if(chanmask & (1 << mch))
                ba.setBit(mch);
            }

            PixmapButtonsWidgetAction* wa = new PixmapButtonsWidgetAction(QString::number(i + 1) + ":" + (md ? md->name() : tr("<none>")), 
                                                                          redLedIcon, darkRedLedIcon, ba, this);
            MusECore::Route srcRoute(i, 0); // Ignore the routing channels - our action holds the channels.   
            //wa->setData(id++);   
            wa->setData(QVariant::fromValue(srcRoute));   
            addAction(wa);  
            
            
            // REMOVE Tim. Persistent routes. Added. TESTING
            //PopupMenu* subp = new PopupMenu(this, true);
            //subp->setTitle("TEST"); 
            //subp->addAction("Testing");
            //wa->(subp);
            
            
            ++gid;

    #else    

            PopupMenu* subp = new PopupMenu(this, true);
            subp->setTitle(QString("%1:").arg(i+1) + (md ? md->name() : tr("<none>"))); 
            
            for(int ch = 0; ch < MIDI_CHANNELS; ++ch) 
            {
              act = subp->addAction(QString("Channel %1").arg(ch+1));
              act->setCheckable(true);
              int chbit = 1 << ch;
              MusECore::Route srcRoute(i, chbit);    // In accordance with channel mask, use the bit position.
              act->setData(QVariant::fromValue(srcRoute));   
              if(chanmask & chbit)                  // Is the channel already set? Show item check mark.
                act->setChecked(true);
              ++gid;  
            }
            //gid = MIDI_PORTS * MIDI_CHANNELS + i;           // Make sure each 'toggle' item gets a unique id.
            act = subp->addAction(tr("Toggle all"));
            //act->setCheckable(true);
            MusECore::Route togRoute(i, (1 << MIDI_CHANNELS) - 1);    // Set all channel bits.
            act->setData(QVariant::fromValue(togRoute));   
            ++gid;
            addMenu(subp);
            
    #endif // _USE_CUSTOM_WIDGET_ACTIONS_
            
          }
          
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
            
            for(int ch = 0; ch < MIDI_CHANNELS; ++ch) 
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
            //gid = MIDI_PORTS * MIDI_CHANNELS + i;           // Make sure each 'toggle' item gets a unique id.
            act = subp->addAction(QString("Toggle all"));
            //act->setCheckable(true);
            act->setData(gid);
            MusECore::Route togRoute(i, (1 << MIDI_CHANNELS) - 1);    // Set all channel bits.
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
        //int chans = t->channels();
        if(_isOutMenu)   
        {
          // REMOVE Tim. Persistent routes. Added.
          const int t_ochs = t->totalRoutableOutputs(MusECore::Route::TRACK_ROUTE);
          const int t_jochs = t->totalRoutableOutputs(MusECore::Route::JACK_ROUTE);
        
          MusECore::RouteList* orl = t->outRoutes();

          int gid = 0;
          
          switch(track->type()) 
          {
            case MusECore::Track::AUDIO_OUTPUT:
            {
              if(!MusEGlobal::checkAudioDevice())
              { 
                clear();
                return;
              }

              std::list<QString> ol = MusEGlobal::audioDevice->inputPorts();
              const int sz = ol.size();
              if(sz != 0)
              {
                
#ifdef _USE_CUSTOM_WIDGET_ACTIONS_
                RoutingMatrixWidgetAction* name_wa = new RoutingMatrixWidgetAction(1, 2, redLedIcon, darkRedLedIcon, this);
                name_wa->setData(_ALIASES_WIDGET_ACTION_);
                name_wa->array()->setRowsExclusive(true);
                name_wa->array()->setColumnsExclusive(true);
                name_wa->array()->setExclusiveToggle(true);
                name_wa->header()->setText(0, -1, tr("Show alias:"));
                name_wa->header()->setText(-1, 0, tr("1 "));
                name_wa->header()->setText(-1, 1, tr("2"));
                if(preferredRouteNameOrAlias == 1)
                  name_wa->array()->setValue(0, 0, true);
                else if(preferredRouteNameOrAlias == 2)
                  name_wa->array()->setValue(0, 1, true);
                // Must rebuild array after text changes.
                name_wa->updateChannelArray();
                addAction(name_wa);
                addSeparator();
#else
                QActionGroup* act_grp = new QActionGroup(this);
                act_grp->setExclusive(true);
                act = act_grp->addAction(tr("Show names"));
                act->setCheckable(true);
                act->setData(_SHOW_CANONICAL_NAMES_);
                if(preferredRouteNameOrAlias == 0)
                  act->setChecked(true); 
                act = act_grp->addAction(tr("Show first aliases"));
                act->setCheckable(true);
                act->setData(_SHOW_FIRST_ALIASES_);
                if(preferredRouteNameOrAlias == 1)
                  act->setChecked(true); 
                act = act_grp->addAction(tr("Show second aliases"));
                act->setCheckable(true);
                act->setData(_SHOW_SECOND_ALIASES_);
                if(preferredRouteNameOrAlias == 2)
                  act->setChecked(true); 
                addActions(act_grp->actions());
                addSeparator();
#endif                
                
#ifdef _USE_CUSTOM_WIDGET_ACTIONS_
                
                RoutingMatrixWidgetAction* wa = new RoutingMatrixWidgetAction(sz, t_jochs, redLedIcon, darkRedLedIcon, this);
                wa->setData(QVariant::fromValue(MusECore::Route(MusECore::Route::JACK_ROUTE, -1, NULL, -1, -1, -1, NULL)));   

                int row = 0;
                for(std::list<QString>::iterator ip = ol.begin(); ip != ol.end(); ++ip, ++row)
                {
                  const char* port_name = (*ip).toLatin1().constData();
                  char good_name[ROUTE_PERSISTENT_NAME_SIZE];
                  void* const port = MusEGlobal::audioDevice->findPort(port_name);
                  if(port)
                  {
                    MusEGlobal::audioDevice->portName(port, good_name, ROUTE_PERSISTENT_NAME_SIZE, preferredRouteNameOrAlias);
                    port_name = good_name;
                  //}
                    wa->header()->setText(row, -1, port_name);
                    for(int i = 0; i < t_jochs; ++i) 
                    {
                      const MusECore::Route r(MusECore::Route::JACK_ROUTE, -1, port, i, -1, -1, NULL);
                      if(orl->exists(r))
                        wa->array()->setValue(row, i, true);
                    }
                  }
                }  
                // Must rebuild array after text changes.
                wa->updateChannelArray();
                addAction(wa);

#else
                
                // REMOVE Tim. Persistent routes. Changed.
                QAction* act = 0;
                //for(int i = 0; i < chans; ++i) 
                for(int i = 0; i < t_jochs; ++i) 
                {
                  //char buffer[128];
                  //snprintf(buffer, 128, "%s %d", tr("Channel").toLatin1().constData(), i+1);
                  QString chBuffer = tr("Channel") + QString(" ") + QString::number(i + 1);
                  MenuTitleItem* titel = new MenuTitleItem(chBuffer, this);
                  addAction(titel); 
        
                  if(!MusEGlobal::checkAudioDevice())
                  { 
                    clear();
                    return;
                  }
                  //std::list<QString> ol = MusEGlobal::audioDevice->inputPorts();
                  for(std::list<QString>::iterator ip = ol.begin(); ip != ol.end(); ++ip) 
                  {
                    act = addAction(*ip);
                    act->setCheckable(true);
                    
                    const char* port_name = (*ip).toLatin1().constData();
                    char good_name[ROUTE_PERSISTENT_NAME_SIZE];
                    void* const port = MusEGlobal::audioDevice->findPort(port_name);
                    if(port)
                    {
                      MusEGlobal::audioDevice->portName(port, good_name, ROUTE_PERSISTENT_NAME_SIZE);
                      port_name = good_name;
                    }
                    MusECore::Route dst(MusECore::Route::JACK_ROUTE, -1, NULL, i, -1, -1, port_name);
                    
                    act->setData(QVariant::fromValue(dst));   
                    ++gid;
                    for(MusECore::ciRoute ir = orl->begin(); ir != orl->end(); ++ir) 
                    {
                      if(*ir == dst) 
                      {
                        act->setChecked(true);
                        break;
                      }
                    }
                  }
                  // REMOVE Tim. Persistent routes. Changed.
                  //if(i+1 != chans)
                  if(i+1 != t_ochs)
                    addSeparator();
                }    
#endif                

              }
                
              
              //
              // Display using separate menu for audio inputs:
              //
              addSeparator();
              addAction(new MenuTitleItem(tr("Soloing chain"), this)); 
              PopupMenu* subp = new PopupMenu(this, true);
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
            break;
            
            
            // REMOVE Tim. Persistent routes. Changed.
    //         case MusECore::Track::AUDIO_SOFTSYNTH:
    //               gid = addMultiChannelPorts(t, this, gid, true);
    //         break;
    //         
    //         case MusECore::Track::AUDIO_INPUT:
    //         case MusECore::Track::WAVE:
    //         case MusECore::Track::AUDIO_GROUP:
    //         case MusECore::Track::AUDIO_AUX:
    //               gid = addWavePorts(        t, this, gid, -1, -1, true);  
    //               gid = addOutPorts(         t, this, gid, -1, -1, true);
    //               gid = addGroupPorts(       t, this, gid, -1, -1, true);
    //               gid = nonSyntiTrackAddSyntis(t, this, gid, true);
    //         break;
            case MusECore::Track::AUDIO_INPUT:
            case MusECore::Track::WAVE:
            case MusECore::Track::AUDIO_GROUP:
            case MusECore::Track::AUDIO_AUX:
            case MusECore::Track::AUDIO_SOFTSYNTH:
              if(t_ochs > 0)
              {
    //#ifndef _USE_CUSTOM_WIDGET_ACTIONS_
                addAction(new MenuTitleItem(tr("Omnis"), this)); 
    //#endif            
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
          // REMOVE Tim. Persistent routes. Added.
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
            
          // REMOVE Tim. Persistent routes. Added.
          const int t_ichs = t->totalRoutableInputs(MusECore::Route::TRACK_ROUTE);
          const int t_jichs = t->totalRoutableInputs(MusECore::Route::JACK_ROUTE);
          
          MusECore::RouteList* irl = t->inRoutes();
      
          int gid = 0;
          
          switch(track->type()) 
          {
            case MusECore::Track::AUDIO_INPUT:
            {
              if(!MusEGlobal::checkAudioDevice())
              { 
                clear();
                return;
              }

              std::list<QString> ol = MusEGlobal::audioDevice->outputPorts();
              const int sz = ol.size();
              if(sz != 0)
              {
                
#ifdef _USE_CUSTOM_WIDGET_ACTIONS_
                RoutingMatrixWidgetAction* name_wa = new RoutingMatrixWidgetAction(1, 2, redLedIcon, darkRedLedIcon, this);
                name_wa->setData(_ALIASES_WIDGET_ACTION_);
                name_wa->array()->setRowsExclusive(true);
                name_wa->array()->setColumnsExclusive(true);
                name_wa->array()->setExclusiveToggle(true);
                name_wa->header()->setText(0, -1, tr("Show alias:"));
                name_wa->header()->setText(-1, 0, tr("1 "));
                name_wa->header()->setText(-1, 1, tr("2"));
                if(preferredRouteNameOrAlias == 1)
                  name_wa->array()->setValue(0, 0, true);
                else if(preferredRouteNameOrAlias == 2)
                  name_wa->array()->setValue(0, 1, true);
                // Must rebuild array after text changes.
                name_wa->updateChannelArray();
                addAction(name_wa);
                addSeparator();
#else
                QActionGroup* act_grp = new QActionGroup(this);
                act_grp->setExclusive(true);
                act = act_grp->addAction(tr("Show names"));
                act->setCheckable(true);
                act->setData(_SHOW_CANONICAL_NAMES_);
                if(preferredRouteNameOrAlias == 0)
                  act->setChecked(true); 
                act = act_grp->addAction(tr("Show first aliases"));
                act->setCheckable(true);
                act->setData(_SHOW_FIRST_ALIASES_);
                if(preferredRouteNameOrAlias == 1)
                  act->setChecked(true); 
                act = act_grp->addAction(tr("Show second aliases"));
                act->setCheckable(true);
                act->setData(_SHOW_SECOND_ALIASES_);
                if(preferredRouteNameOrAlias == 2)
                  act->setChecked(true); 
                addActions(act_grp->actions());
                addSeparator();
#endif                
                
#ifdef _USE_CUSTOM_WIDGET_ACTIONS_
                
                RoutingMatrixWidgetAction* wa = new RoutingMatrixWidgetAction(sz, t_jichs, redLedIcon, darkRedLedIcon, this);
                // Ignore the routing channel and channels - our action holds the channels.
                wa->setData(QVariant::fromValue(MusECore::Route(MusECore::Route::JACK_ROUTE, -1, NULL, -1, -1, -1, NULL)));   
                int row = 0;
                for(std::list<QString>::iterator ip = ol.begin(); ip != ol.end(); ++ip, ++row)
                {
                  const char* port_name = (*ip).toLatin1().constData();
                  char good_name[ROUTE_PERSISTENT_NAME_SIZE];
                  void* const port = MusEGlobal::audioDevice->findPort(port_name);
                  if(port)
                  {
                    MusEGlobal::audioDevice->portName(port, good_name, ROUTE_PERSISTENT_NAME_SIZE, preferredRouteNameOrAlias);
                    port_name = good_name;
                  //}
                    wa->header()->setText(row, -1, port_name);
                    for(int i = 0; i < t_jichs; ++i) 
                    {
                      const MusECore::Route r(MusECore::Route::JACK_ROUTE, -1, port, i, -1, -1, NULL);
                      if(irl->exists(r))
                        wa->array()->setValue(row, i, true);
                    }
                  }
                }  
                // Must rebuild array after text changes.
                wa->updateChannelArray();
                addAction(wa);
                
#else
                
                // REMOVE Tim. Persistent routes. Changed.
                QAction* act = 0;
                //for(int i = 0; i < chans; ++i) 
                for(int i = 0; i < t_jichs; ++i) 
                {
                  //char buffer[128];
                  //snprintf(buffer, 128, "%s %d", tr("Channel").toLatin1().constData(), i+1);
                  QString chBuffer = tr("Channel") + QString(" ") + QString::number(i + 1);
                  MenuTitleItem* titel = new MenuTitleItem(chBuffer, this);
                  addAction(titel); 
        
                  for(std::list<QString>::iterator ip = ol.begin(); ip != ol.end(); ++ip) 
                  {
                    act = addAction(*ip);
                    act->setCheckable(true);
                    
                    const char* port_name = (*ip).toLatin1().constData();
                    char good_name[ROUTE_PERSISTENT_NAME_SIZE];
                    void* const port = MusEGlobal::audioDevice->findPort(port_name);
                    if(port)
                    {
                      MusEGlobal::audioDevice->portName(port, good_name, ROUTE_PERSISTENT_NAME_SIZE);
                      port_name = good_name;
                    }
                    MusECore::Route dst(MusECore::Route::JACK_ROUTE, -1, NULL, i, -1, -1, port_name);
                    
                    act->setData(QVariant::fromValue(dst));   
                    ++gid;
                    for(MusECore::ciRoute ir = irl->begin(); ir != irl->end(); ++ir) 
                    {
                      if(*ir == dst) 
                      {
                        act->setChecked(true);
                        break;
                      }
                    }
                  }
                  // REMOVE Tim. Persistent routes. Changed.
                  //if(i+1 != chans)
                  if(i+1 != t_ichs)
                    addSeparator();
                }
#endif                
                
              }
              
              //
              // Display using separate menus for midi ports and audio outputs:
              //
              addSeparator();
              addAction(new MenuTitleItem(tr("Soloing chain"), this)); 
              PopupMenu* subp = new PopupMenu(this, true);
              subp->setTitle(tr("Audio sends")); 
              addMenu(subp);
              gid = addOutPorts(t, subp, gid, -1, -1, false);  
              subp = new PopupMenu(this, true);
              subp->setTitle(tr("Midi port sends")); 
              addMenu(subp);
              addMidiPorts(t, subp, gid, false);
              //
              // Display all in the same menu:
              //
              //addAction(new MenuTitleItem(tr("Audio sends"), this)); 
              //gid = addOutPorts(t, this, gid, -1, -1, false);  
              //addSeparator();
              //addAction(new MenuTitleItem(tr("Midi sends"), this)); 
              //addMidiPorts(t, this, gid, false);
            }
            break;
            
            // REMOVE Tim. Persistent routes. Changed.
    //         case MusECore::Track::AUDIO_OUTPUT:
    //               gid = addWavePorts( t, this, gid, -1, -1, false);
    //               gid = addInPorts(   t, this, gid, -1, -1, false);
    //               gid = addGroupPorts(t, this, gid, -1, -1, false);
    //               gid = addAuxPorts(  t, this, gid, -1, -1, false);
    //               gid = nonSyntiTrackAddSyntis(t, this, gid, false);
    //               break;
    //         case MusECore::Track::WAVE:
    //               gid = addWavePorts( t, this, gid, -1, -1, false);  
    //               gid = addInPorts(   t, this, gid, -1, -1, false);
    //               gid = addGroupPorts(t, this, gid, -1, -1, false);  
    //               gid = addAuxPorts(  t, this, gid, -1, -1, false);  
    //               gid = nonSyntiTrackAddSyntis(t, this, gid, false); 
    //               break;
    //         case MusECore::Track::AUDIO_GROUP:
    //               gid = addWavePorts( t, this, gid, -1, -1, false);
    //               gid = addInPorts(   t, this, gid, -1, -1, false);
    //               gid = addGroupPorts(t, this, gid, -1, -1, false);
    //               gid = addAuxPorts(  t, this, gid, -1, -1, false);  
    //               gid = nonSyntiTrackAddSyntis(t, this, gid, false);
    //               break;
    //         
    //         case MusECore::Track::AUDIO_SOFTSYNTH:
    //               gid = addMultiChannelPorts(t, this, gid, false);
    //               break;
            case MusECore::Track::AUDIO_OUTPUT:
            case MusECore::Track::WAVE:
            case MusECore::Track::AUDIO_GROUP:
            case MusECore::Track::AUDIO_SOFTSYNTH:
              if(t_ichs > 0)
              {
    //#ifndef _USE_CUSTOM_WIDGET_ACTIONS_
                addAction(new MenuTitleItem(tr("Omnis"), this)); 
    //#endif            
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
          // REMOVE Tim. Persistent routes. Added.
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
    
    case MusECore::Route::JACK_ROUTE:
    break;
    case MusECore::Route::MIDI_DEVICE_ROUTE:
    break;
    case MusECore::Route::MIDI_PORT_ROUTE:
    break;
  
  }
}

// REMOVE Tim. Persistent routes. Changed.
//void RoutePopupMenu::exec(MusECore::Track* track, bool isOutput)
void RoutePopupMenu::exec(const MusECore::Route& route, bool isOutput)
{
  //if(track)
  if(route.isValid())
  {
    //_track = track;
    _route = route;
    _isOutMenu = isOutput;
  }  
  prepare();
  PopupMenu::exec();
}

// REMOVE Tim. Persistent routes. Changed.
//void RoutePopupMenu::exec(const QPoint& p, MusECore::Track* track, bool isOutput)
void RoutePopupMenu::exec(const QPoint& p, const MusECore::Route& route, bool isOutput)
{
  //if(track)
  if(route.isValid())
  {
    //_track = track;
    _route = route;
    _isOutMenu = isOutput;
  }  
  prepare();
  PopupMenu::exec(p);
}

// REMOVE Tim. Persistent routes. Changed.
//void RoutePopupMenu::popup(const QPoint& p, MusECore::Track* track, bool isOutput)
void RoutePopupMenu::popup(const QPoint& p, const MusECore::Route& route, bool isOutput)
{
  //if(track)
  if(route.isValid())
  {
    //_track = track;
    _route = route;
    _isOutMenu = isOutput;
  }  
  prepare();
  PopupMenu::popup(p);
}

} // namespace MusEGui
